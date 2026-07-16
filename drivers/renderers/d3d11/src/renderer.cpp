#include <alia/renderers/d3d11/renderer.h>

#include <alia/abi/base/arena.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/drawing.h>
#include <alia/abi/ui/effects.h>
#include <alia/abi/ui/system/api.h>
#include <alia/abi/ui/system/renderer.h>
#include <alia/impl/base/arena.hpp>
#include <alia/ui/drawing.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <d3d11.h>
#include <d3dcompiler.h>

#include <cstring>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <new>
#include <vector>

struct alia_d3d11_renderer;

struct d3d11_effect_slot
{
    alia_d3d11_renderer* renderer = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3D11Buffer* params_cb = nullptr;
    size_t params_size = 0;
    size_t cb_bytes = 0;
};

struct alia_d3d11_renderer
{
    alia_ui_system* system = nullptr;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;

    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3D11InputLayout* layout = nullptr;
    ID3D11Buffer* quad_vb = nullptr;
    ID3D11Buffer* instance_vb = nullptr;
    ID3D11Buffer* constants = nullptr;
    ID3D11BlendState* blend = nullptr;
    ID3D11RasterizerState* rasterizer = nullptr;
    ID3D11DepthStencilState* depth = nullptr;

    ID3D11Texture2D* msdf_atlas = nullptr;
    ID3D11ShaderResourceView* msdf_srv = nullptr;
    ID3D11SamplerState* msdf_sampler = nullptr;

    UINT instance_capacity = 0;
    alia_arena rect_instance_arena{};

    // Shared effect geometry; each registered effect is its own material.
    ID3D11VertexShader* effect_vs = nullptr;
    ID3D11InputLayout* effect_layout = nullptr;
    ID3D11Buffer* effect_quad_vb = nullptr;
    ID3D11BlendState* effect_blend = nullptr;
    // Stock frame CB (region + surface) bound at b0 for all effects.
    ID3D11Buffer* effect_frame_cb = nullptr;
    std::vector<std::unique_ptr<d3d11_effect_slot>> effects;
};

namespace {

char const* const k_hlsl = R"(
cbuffer Constants : register(b0)
{
    float4x4 u_projection;
};

Texture2D u_msdf : register(t0);
SamplerState u_msdf_samp : register(s0);

struct VSIn
{
    float2 a_pos : POSITION;
    float2 i_pos : I_POS;
    float2 i_size : I_SIZE;
    float4 i_color : I_COLOR;
    int i_primitive_type : I_TYPE;
    float4 i_data_a : I_DATA_A;
    float4 i_data_b : I_DATA_B;
};

struct PSIn
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 world : TEXCOORD0;
    float2 center : TEXCOORD1;
    float2 half_size : TEXCOORD2;
    nointerpolation int primitive_type : TEXCOORD3;
    nointerpolation float4 data_a : TEXCOORD4;
    float2 uv_msdf : TEXCOORD5;
    nointerpolation float msdf_sdf_scale : TEXCOORD6;
};

PSIn vs_main(VSIn input)
{
    PSIn o;
    o.primitive_type = input.i_primitive_type;
    o.data_a = input.i_data_a;
    o.color = input.i_color;

    if (input.i_primitive_type == 3)
    {
        // MSDF glyphs: no box AA padding.
        float2 scaled = input.a_pos * input.i_size + input.i_pos;
        o.pos = mul(u_projection, float4(scaled, 0.0, 1.0));
        o.world = float2(0.0, 0.0);
        o.center = float2(0.0, 0.0);
        o.half_size = float2(0.0, 0.0);

        float2 uv_min = input.i_data_a.xy;
        float2 uv_sz = input.i_data_a.zw;
        // y-down quad (a_pos.y=0 at top): flip v to match atlas.
        o.uv_msdf = float2(
            uv_min.x + input.a_pos.x * uv_sz.x,
            uv_min.y + (1.0 - input.a_pos.y) * uv_sz.y);
        o.msdf_sdf_scale = input.i_data_b.x;
    }
    else
    {
        float2 scaled = input.a_pos * (input.i_size + float2(2.0, 2.0))
                      + input.i_pos - float2(1.0, 1.0);
        o.pos = mul(u_projection, float4(scaled, 0.0, 1.0));
        o.world = scaled;
        o.center = input.i_pos + input.i_size * 0.5;
        o.half_size = input.i_size * 0.5;
        o.uv_msdf = float2(0.0, 0.0);
        o.msdf_sdf_scale = 0.0;
    }
    return o;
}

float sd_round_rect(float2 p, float2 b, float r)
{
    float2 q = abs(p) - (b - float2(r, r));
    float outside = length(max(q, 0.0));
    float inside = min(max(q.x, q.y), 0.0);
    return outside + inside - r;
}

float2 rotate_point_clockwise(float2 p, float radians)
{
    float cs = cos(radians);
    float sn = sin(radians);
    return float2(cs * p.x - sn * p.y, sn * p.x + cs * p.y);
}

float sd_equilateral_triangle(float2 p, float r)
{
    const float k = sqrt(3.0);
    p.x = abs(p.x) - r;
    p.y = r / k - p.y;
    if (p.x + k * p.y > 0.0)
        p = float2(p.x - k * p.y, -k * p.x - p.y) / 2.0;
    p.x -= clamp(p.x, -2.0 * r, 0.0);
    return -length(p) * sign(p.y);
}

float sd_squircle(float2 p, float R)
{
    float x2 = p.x * p.x;
    float y2 = p.y * p.y;
    return sqrt(sqrt(x2 * x2 + y2 * y2)) - R;
}

float3 srgb_to_linear(float3 srgb)
{
    float3 low = srgb / 12.92;
    float3 high = pow((srgb + 0.055) / 1.055, 2.4);
    float3 mask = step(0.04045, srgb);
    return lerp(low, high, mask);
}

float4 srgba_to_linear(float4 srgba)
{
    return float4(srgb_to_linear(srgba.rgb) * srgba.a, srgba.a);
}

float4 unpack_border_color_linear(float packed_as_float)
{
    uint packed_bits = asuint(packed_as_float);
    float4 border_srgba = float4(
        float(packed_bits & 255u),
        float((packed_bits >> 8u) & 255u),
        float((packed_bits >> 16u) & 255u),
        float((packed_bits >> 24u) & 255u)) / 255.0;
    return srgba_to_linear(border_srgba);
}

float median_msdf(float3 v)
{
    return max(min(v.r, v.g), min(max(v.r, v.g), v.b));
}

float4 ps_main(PSIn input) : SV_TARGET
{
    float aa = 0.5;
    float4 color = srgba_to_linear(input.color);

    if (input.primitive_type == 3)
    {
        float3 msd = u_msdf.Sample(u_msdf_samp, input.uv_msdf).rgb;
        float sd = median_msdf(msd) - 0.5;
        float screen_px_distance = input.msdf_sdf_scale * sd;
        float opacity = saturate(screen_px_distance + 0.5);
        return color * opacity;
    }

    float2 local = input.world - input.center;

    if (input.primitive_type == 0)
    {
        float corner = min(
            input.data_a.x, min(input.half_size.x, input.half_size.y));
        float border_width = input.data_a.y;
        float4 border_color = unpack_border_color_linear(input.data_a.z);
        float d = sd_round_rect(local, input.half_size, corner);
        float alpha_inner = smoothstep(-aa, aa, d + border_width);
        float alpha_outer = saturate(smoothstep(aa, -aa, d));
        float4 mix_color = lerp(color, border_color, alpha_inner);
        return mix_color * alpha_outer;
    }

    if (input.primitive_type == 1)
    {
        float R = min(input.half_size.x, input.half_size.y);
        float d = sd_equilateral_triangle(
            rotate_point_clockwise(local, -input.data_a.x), R);
        float alpha = saturate(smoothstep(aa, -aa, d));
        return color * alpha;
    }

    if (input.primitive_type == 2)
    {
        float squircle_radius = input.data_a.x;
        float border_width = input.data_a.y;
        float4 border_color = unpack_border_color_linear(input.data_a.z);
        float d = sd_squircle(local, squircle_radius);
        float alpha_inner = smoothstep(-aa, aa, d + border_width);
        float alpha_outer = saturate(smoothstep(aa, -aa, d));
        float4 mix_color = lerp(color, border_color, alpha_inner);
        return mix_color * alpha_outer;
    }

    return float4(0, 0, 0, 0);
}
)";

#pragma pack(push, 1)
struct primitive_instance
{
    float min[2];
    float size[2];
    float color[4];
    int primitive_type;
    float data_a[4];
    float data_b[4];
};
#pragma pack(pop)

struct constants_cb
{
    float projection[16];
};

template<class T>
void
release_t(T*& p)
{
    if (p)
    {
        p->Release();
        p = nullptr;
    }
}

bool
compile_shader(
    char const* source,
    char const* source_name,
    char const* entry,
    char const* target,
    ID3DBlob** out_blob)
{
    ID3DBlob* errors = nullptr;
    HRESULT hr = D3DCompile(
        source,
        std::strlen(source),
        source_name,
        nullptr,
        nullptr,
        entry,
        target,
        D3DCOMPILE_ENABLE_STRICTNESS,
        0,
        out_blob,
        &errors);
    if (FAILED(hr))
    {
        if (errors)
        {
            std::fprintf(
                stderr,
                "[alia d3d11] %s\n",
                static_cast<char*>(errors->GetBufferPointer()));
            errors->Release();
        }
        return false;
    }
    if (errors)
        errors->Release();
    return true;
}

bool
compile_primitive_shader(
    char const* entry,
    char const* target,
    ID3DBlob** out_blob)
{
    return compile_shader(
        k_hlsl, "alia_d3d11_primitives.hlsl", entry, target, out_blob);
}

bool
renderer_init_gpu(alia_d3d11_renderer* renderer)
{
    ID3DBlob* vs_blob = nullptr;
    ID3DBlob* ps_blob = nullptr;
    if (!compile_primitive_shader("vs_main", "vs_5_0", &vs_blob))
        return false;
    if (!compile_primitive_shader("ps_main", "ps_5_0", &ps_blob))
    {
        vs_blob->Release();
        return false;
    }

    ID3D11Device* device = renderer->device;
    HRESULT hr = device->CreateVertexShader(
        vs_blob->GetBufferPointer(),
        vs_blob->GetBufferSize(),
        nullptr,
        &renderer->vs);
    if (FAILED(hr))
    {
        vs_blob->Release();
        ps_blob->Release();
        return false;
    }
    hr = device->CreatePixelShader(
        ps_blob->GetBufferPointer(),
        ps_blob->GetBufferSize(),
        nullptr,
        &renderer->ps);
    ps_blob->Release();
    if (FAILED(hr))
    {
        vs_blob->Release();
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION",
         0,
         DXGI_FORMAT_R32G32_FLOAT,
         0,
         0,
         D3D11_INPUT_PER_VERTEX_DATA,
         0},
        {"I_POS",
         0,
         DXGI_FORMAT_R32G32_FLOAT,
         1,
         0,
         D3D11_INPUT_PER_INSTANCE_DATA,
         1},
        {"I_SIZE",
         0,
         DXGI_FORMAT_R32G32_FLOAT,
         1,
         8,
         D3D11_INPUT_PER_INSTANCE_DATA,
         1},
        {"I_COLOR",
         0,
         DXGI_FORMAT_R32G32B32A32_FLOAT,
         1,
         16,
         D3D11_INPUT_PER_INSTANCE_DATA,
         1},
        {"I_TYPE",
         0,
         DXGI_FORMAT_R32_SINT,
         1,
         32,
         D3D11_INPUT_PER_INSTANCE_DATA,
         1},
        {"I_DATA_A",
         0,
         DXGI_FORMAT_R32G32B32A32_FLOAT,
         1,
         36,
         D3D11_INPUT_PER_INSTANCE_DATA,
         1},
        {"I_DATA_B",
         0,
         DXGI_FORMAT_R32G32B32A32_FLOAT,
         1,
         52,
         D3D11_INPUT_PER_INSTANCE_DATA,
         1},
    };
    hr = device->CreateInputLayout(
        layout,
        UINT(sizeof(layout) / sizeof(layout[0])),
        vs_blob->GetBufferPointer(),
        vs_blob->GetBufferSize(),
        &renderer->layout);
    vs_blob->Release();
    if (FAILED(hr))
        return false;

    float quad[] = {0.f, 0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f};
    D3D11_BUFFER_DESC vb_desc{};
    vb_desc.ByteWidth = sizeof(quad);
    vb_desc.Usage = D3D11_USAGE_IMMUTABLE;
    vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vb_data{quad, 0, 0};
    hr = device->CreateBuffer(&vb_desc, &vb_data, &renderer->quad_vb);
    if (FAILED(hr))
        return false;

    D3D11_BUFFER_DESC cb_desc{};
    cb_desc.ByteWidth = sizeof(constants_cb);
    cb_desc.Usage = D3D11_USAGE_DYNAMIC;
    cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = device->CreateBuffer(&cb_desc, nullptr, &renderer->constants);
    if (FAILED(hr))
        return false;

    D3D11_BLEND_DESC blend{};
    blend.RenderTarget[0].BlendEnable = TRUE;
    blend.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blend.RenderTarget[0].RenderTargetWriteMask
        = D3D11_COLOR_WRITE_ENABLE_ALL;
    hr = device->CreateBlendState(&blend, &renderer->blend);
    if (FAILED(hr))
        return false;

    D3D11_RASTERIZER_DESC rs{};
    rs.FillMode = D3D11_FILL_SOLID;
    rs.CullMode = D3D11_CULL_NONE;
    rs.DepthClipEnable = TRUE;
    rs.ScissorEnable = TRUE;
    hr = device->CreateRasterizerState(&rs, &renderer->rasterizer);
    if (FAILED(hr))
        return false;

    D3D11_DEPTH_STENCIL_DESC ds{};
    ds.DepthEnable = FALSE;
    ds.StencilEnable = FALSE;
    hr = device->CreateDepthStencilState(&ds, &renderer->depth);
    if (FAILED(hr))
        return false;

    D3D11_SAMPLER_DESC samp{};
    samp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samp.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp.MaxLOD = D3D11_FLOAT32_MAX;
    hr = device->CreateSamplerState(&samp, &renderer->msdf_sampler);
    if (FAILED(hr))
        return false;

    alia::initialize_lazy_commit_arena(&renderer->rect_instance_arena);
    return true;
}

bool
ensure_instance_capacity(alia_d3d11_renderer* renderer, UINT count)
{
    if (count <= renderer->instance_capacity && renderer->instance_vb)
        return true;

    release_t(renderer->instance_vb);
    UINT capacity = renderer->instance_capacity ? renderer->instance_capacity : 64;
    while (capacity < count)
        capacity *= 2;

    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = capacity * sizeof(primitive_instance);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    HRESULT hr
        = renderer->device->CreateBuffer(&desc, nullptr, &renderer->instance_vb);
    if (FAILED(hr))
        return false;
    renderer->instance_capacity = capacity;
    return true;
}

void
render_primitive_command_list(void* user, alia_draw_bucket const* bucket)
{
    auto* renderer = static_cast<alia_d3d11_renderer*>(user);
    if (!bucket || bucket->count == 0 || !bucket->clip_rect)
        return;

    alia_box const* clip = bucket->clip_rect;
    if (clip->size.x <= 0.f || clip->size.y <= 0.f)
        return;

    ID3D11DeviceContext* ctx = renderer->context;

    D3D11_VIEWPORT vp{};
    vp.TopLeftX = clip->min.x;
    vp.TopLeftY = clip->min.y;
    vp.Width = clip->size.x;
    vp.Height = clip->size.y;
    vp.MinDepth = 0.f;
    vp.MaxDepth = 1.f;
    ctx->RSSetViewports(1, &vp);

    D3D11_RECT scissor{
        LONG(clip->min.x),
        LONG(clip->min.y),
        LONG(clip->min.x + clip->size.x),
        LONG(clip->min.y + clip->size.y)};
    ctx->RSSetScissorRects(1, &scissor);

    float l = clip->min.x;
    float r = clip->min.x + clip->size.x;
    float t = clip->min.y;
    float b = clip->min.y + clip->size.y;
    float n = -1.f;
    float f = 1.f;
    constants_cb cb{};
    float ortho[16] = {
        2.f / (r - l),
        0.f,
        0.f,
        0.f,
        0.f,
        2.f / (t - b),
        0.f,
        0.f,
        0.f,
        0.f,
        -2.f / (f - n),
        0.f,
        -(r + l) / (r - l),
        -(t + b) / (t - b),
        -(f + n) / (f - n),
        1.f};
    std::memcpy(cb.projection, ortho, sizeof(ortho));

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(ctx->Map(renderer->constants, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        return;
    std::memcpy(mapped.pData, &cb, sizeof(cb));
    ctx->Unmap(renderer->constants, 0);

    alia_bump_allocator rect_alloc;
    alia_bump_allocator_init(&rect_alloc, &renderer->rect_instance_arena);
    auto* instances = alia::arena_alloc_array<primitive_instance>(
        rect_alloc, bucket->count);

    auto pack_border_color = [](alia_srgba8 c) -> float {
        uint32_t packed = uint32_t(c.r) | (uint32_t(c.g) << 8u)
                        | (uint32_t(c.b) << 16u) | (uint32_t(c.a) << 24u);
        float f;
        std::memcpy(&f, &packed, sizeof(f));
        return f;
    };

    UINT written = 0;
    for (auto const* cmd = bucket->head; cmd; cmd = cmd->next)
    {
        auto const* primitive
            = alia::downcast<alia_draw_primitive_command>(cmd);

        primitive_instance& inst = instances[written++];
        inst.min[0] = primitive->box.min.x;
        inst.min[1] = primitive->box.min.y;
        inst.size[0] = primitive->box.size.x;
        inst.size[1] = primitive->box.size.y;
        inst.color[0] = primitive->color.r / 255.f;
        inst.color[1] = primitive->color.g / 255.f;
        inst.color[2] = primitive->color.b / 255.f;
        inst.color[3] = primitive->color.a / 255.f;
        inst.primitive_type = int(primitive->primitive_type);
        std::memset(inst.data_a, 0, sizeof(inst.data_a));
        std::memset(inst.data_b, 0, sizeof(inst.data_b));

        switch (primitive->primitive_type)
        {
            case ALIA_PRIMITIVE_BOX:
                inst.data_a[0] = primitive->payload.box.corner_radius;
                inst.data_a[1] = primitive->payload.box.border_width;
                inst.data_a[2]
                    = pack_border_color(primitive->payload.box.border_color);
                break;
            case ALIA_PRIMITIVE_EQUILATERAL_TRIANGLE: {
                float const degrees_to_radians
                    = 3.14159265358979323846f / 180.0f;
                inst.data_a[0]
                    = primitive->payload.triangle.rotation_degrees
                    * degrees_to_radians;
                break;
            }
            case ALIA_PRIMITIVE_SQUIRCLE:
                inst.data_a[0] = primitive->payload.squircle.radius;
                inst.data_a[1] = primitive->payload.squircle.border_width;
                inst.data_a[2] = pack_border_color(
                    primitive->payload.squircle.border_color);
                break;
            case ALIA_PRIMITIVE_MSDF_GLYPH:
                inst.data_a[0] = primitive->payload.msdf_glyph.uv_rect[0];
                inst.data_a[1] = primitive->payload.msdf_glyph.uv_rect[1];
                inst.data_a[2] = primitive->payload.msdf_glyph.uv_rect[2];
                inst.data_a[3] = primitive->payload.msdf_glyph.uv_rect[3];
                inst.data_b[0] = primitive->payload.msdf_glyph.sdf_scale;
                break;
            default:
                --written;
                break;
        }
    }
    if (written == 0)
        return;

    if (!ensure_instance_capacity(renderer, written))
        return;

    if (FAILED(ctx->Map(
            renderer->instance_vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        return;
    std::memcpy(
        mapped.pData, instances, sizeof(primitive_instance) * written);
    ctx->Unmap(renderer->instance_vb, 0);

    UINT strides[2] = {sizeof(float) * 2, sizeof(primitive_instance)};
    UINT offsets[2] = {0, 0};
    ID3D11Buffer* buffers[2] = {renderer->quad_vb, renderer->instance_vb};
    ctx->IASetVertexBuffers(0, 2, buffers, strides, offsets);
    ctx->IASetInputLayout(renderer->layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    ctx->VSSetShader(renderer->vs, nullptr, 0);
    ctx->PSSetShader(renderer->ps, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, &renderer->constants);
    if (renderer->msdf_srv && renderer->msdf_sampler)
    {
        ctx->PSSetShaderResources(0, 1, &renderer->msdf_srv);
        ctx->PSSetSamplers(0, 1, &renderer->msdf_sampler);
    }
    ctx->OMSetBlendState(renderer->blend, nullptr, 0xffffffff);
    ctx->RSSetState(renderer->rasterizer);
    ctx->OMSetDepthStencilState(renderer->depth, 0);
    ctx->DrawInstanced(4, written, 0, 0);

    ID3D11ShaderResourceView* null_srv = nullptr;
    ctx->PSSetShaderResources(0, 1, &null_srv);
}

char const* const k_effect_vs_hlsl = R"(
struct VSIn
{
    float2 pos : POSITION;
};

float4 vs_main(VSIn input) : SV_POSITION
{
    return float4(input.pos, 0.0, 1.0);
}
)";

static size_t
effect_params_cb_bytes(size_t params_size)
{
    if (params_size == 0)
        return 16; // D3D11 constant buffers must be non-empty multiples of 16.
    return (params_size + 15u) & ~size_t(15u);
}

bool
ensure_effect_pipeline(alia_d3d11_renderer* renderer)
{
    if (renderer->effect_vs && renderer->effect_layout && renderer->effect_quad_vb
        && renderer->effect_blend && renderer->effect_frame_cb)
        return true;

    ID3DBlob* vs_blob = nullptr;
    if (!compile_shader(
            k_effect_vs_hlsl,
            "alia_d3d11_effect_vs.hlsl",
            "vs_main",
            "vs_5_0",
            &vs_blob))
        return false;

    ID3D11Device* device = renderer->device;
    HRESULT hr = device->CreateVertexShader(
        vs_blob->GetBufferPointer(),
        vs_blob->GetBufferSize(),
        nullptr,
        &renderer->effect_vs);
    if (FAILED(hr))
    {
        vs_blob->Release();
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION",
         0,
         DXGI_FORMAT_R32G32_FLOAT,
         0,
         0,
         D3D11_INPUT_PER_VERTEX_DATA,
         0},
    };
    hr = device->CreateInputLayout(
        layout,
        1,
        vs_blob->GetBufferPointer(),
        vs_blob->GetBufferSize(),
        &renderer->effect_layout);
    vs_blob->Release();
    if (FAILED(hr))
        return false;

    float quad[] = {
        -1.f,
        -1.f,
        1.f,
        -1.f,
        -1.f,
        1.f,
        1.f,
        1.f,
    };
    D3D11_BUFFER_DESC vb_desc{};
    vb_desc.ByteWidth = sizeof(quad);
    vb_desc.Usage = D3D11_USAGE_IMMUTABLE;
    vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vb_data{quad, 0, 0};
    hr = device->CreateBuffer(&vb_desc, &vb_data, &renderer->effect_quad_vb);
    if (FAILED(hr))
        return false;

    D3D11_BLEND_DESC blend{};
    blend.RenderTarget[0].BlendEnable = FALSE;
    blend.RenderTarget[0].RenderTargetWriteMask
        = D3D11_COLOR_WRITE_ENABLE_ALL;
    hr = device->CreateBlendState(&blend, &renderer->effect_blend);
    if (FAILED(hr))
        return false;

    D3D11_BUFFER_DESC frame_desc{};
    frame_desc.ByteWidth = 32; // region float4 + surface float4
    frame_desc.Usage = D3D11_USAGE_DYNAMIC;
    frame_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    frame_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = device->CreateBuffer(&frame_desc, nullptr, &renderer->effect_frame_cb);
    return SUCCEEDED(hr);
}

void
render_effect_command_list(void* user, alia_draw_bucket const* bucket)
{
    auto* slot = static_cast<d3d11_effect_slot*>(user);
    if (!slot || !slot->renderer || !bucket || bucket->count == 0
        || !bucket->clip_rect)
        return;

    alia_d3d11_renderer* renderer = slot->renderer;
    if (!renderer->effect_vs || !renderer->effect_layout
        || !renderer->effect_quad_vb || !renderer->effect_frame_cb || !slot->ps
        || !slot->params_cb)
        return;

    alia_box const* clip = bucket->clip_rect;
    if (clip->size.x <= 0.f || clip->size.y <= 0.f)
        return;

    alia_vec2f const surface_size
        = alia_vec2i_to_vec2f(alia_ui_surface_get_size(renderer->system));

    ID3D11DeviceContext* ctx = renderer->context;
    ctx->IASetInputLayout(renderer->effect_layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    UINT stride = sizeof(float) * 2;
    UINT offset = 0;
    ctx->IASetVertexBuffers(0, 1, &renderer->effect_quad_vb, &stride, &offset);
    ctx->VSSetShader(renderer->effect_vs, nullptr, 0);
    ctx->PSSetShader(slot->ps, nullptr, 0);
    ctx->OMSetBlendState(renderer->effect_blend, nullptr, 0xffffffff);
    ctx->RSSetState(renderer->rasterizer);
    ctx->OMSetDepthStencilState(renderer->depth, 0);

    D3D11_RECT scissor{
        LONG(clip->min.x),
        LONG(clip->min.y),
        LONG(clip->min.x + clip->size.x),
        LONG(clip->min.y + clip->size.y)};
    ctx->RSSetScissorRects(1, &scissor);

    ID3D11Buffer* cbs[2] = {renderer->effect_frame_cb, slot->params_cb};
    ctx->PSSetConstantBuffers(0, 2, cbs);

    for (auto const* cmd = bucket->head; cmd; cmd = cmd->next)
    {
        auto const* effect_cmd
            = alia::downcast<alia_effect_draw_command>(cmd);

        alia_box const& r = effect_cmd->region;
        if (r.size.x <= 0.f || r.size.y <= 0.f)
            continue;

        D3D11_VIEWPORT vp{};
        vp.TopLeftX = r.min.x;
        vp.TopLeftY = r.min.y;
        vp.Width = r.size.x;
        vp.Height = r.size.y;
        vp.MinDepth = 0.f;
        vp.MaxDepth = 1.f;
        ctx->RSSetViewports(1, &vp);

        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (FAILED(ctx->Map(
                renderer->effect_frame_cb,
                0,
                D3D11_MAP_WRITE_DISCARD,
                0,
                &mapped)))
            continue;
        float frame[8] = {
            r.min.x,
            r.min.y,
            r.size.x,
            r.size.y,
            surface_size.x,
            surface_size.y,
            0.f,
            0.f};
        std::memcpy(mapped.pData, frame, sizeof(frame));
        ctx->Unmap(renderer->effect_frame_cb, 0);

        if (FAILED(ctx->Map(
                slot->params_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
            continue;
        std::memset(mapped.pData, 0, slot->cb_bytes);
        if (effect_cmd->params_size > 0 && effect_cmd->params)
        {
            size_t const copy_n
                = effect_cmd->params_size < slot->params_size
                    ? effect_cmd->params_size
                    : slot->params_size;
            std::memcpy(mapped.pData, effect_cmd->params, copy_n);
        }
        ctx->Unmap(slot->params_cb, 0);

        ctx->Draw(4, 0);
    }
}

int
register_effect_blob(
    alia_d3d11_renderer* renderer,
    alia_effect_desc const* desc,
    alia_draw_material_id* out_material_id)
{
    ALIA_ASSERT(renderer);
    ALIA_ASSERT(renderer->system);
    ALIA_ASSERT(renderer->device);
    ALIA_ASSERT(desc);
    ALIA_ASSERT(out_material_id);

    if (desc->shader.format != ALIA_SHADER_FORMAT_DXBC || !desc->shader.data
        || desc->shader.size == 0)
    {
        std::fprintf(
            stderr, "[alia d3d11] effect requires ALIA_SHADER_FORMAT_DXBC\n");
        return -1;
    }

    if (!ensure_effect_pipeline(renderer))
    {
        std::fprintf(stderr, "[alia d3d11] effect pipeline init failed\n");
        return -1;
    }

    auto slot = std::make_unique<d3d11_effect_slot>();
    slot->renderer = renderer;

    HRESULT hr = renderer->device->CreatePixelShader(
        desc->shader.data, desc->shader.size, nullptr, &slot->ps);
    if (FAILED(hr))
    {
        std::fprintf(stderr, "[alia d3d11] CreatePixelShader failed\n");
        return -1;
    }

    slot->params_size = desc->params_size;
    slot->cb_bytes = effect_params_cb_bytes(desc->params_size);
    D3D11_BUFFER_DESC cb_desc{};
    cb_desc.ByteWidth = UINT(slot->cb_bytes);
    cb_desc.Usage = D3D11_USAGE_DYNAMIC;
    cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = renderer->device->CreateBuffer(&cb_desc, nullptr, &slot->params_cb);
    if (FAILED(hr))
    {
        release_t(slot->ps);
        return -1;
    }

    alia_draw_material_id const material_id
        = alia_material_alloc_ids(renderer->system, 1);
    alia_material_register(
        renderer->system,
        material_id,
        alia_material_vtable{.draw_bucket = render_effect_command_list},
        slot.get());

    renderer->effects.push_back(std::move(slot));
    *out_material_id = material_id;
    return 0;
}

} // namespace

extern "C" {

alia_struct_spec
alia_d3d11_renderer_object_spec(void)
{
    return {sizeof(alia_d3d11_renderer), alignof(alia_d3d11_renderer)};
}

alia_d3d11_renderer*
alia_d3d11_renderer_init(void* object_storage)
{
    ALIA_ASSERT(object_storage);
    return new (object_storage) alia_d3d11_renderer{};
}

void
alia_d3d11_renderer_attach(
    alia_d3d11_renderer* renderer,
    alia_ui_system* ui,
    ID3D11Device* device,
    ID3D11DeviceContext* context)
{
    ALIA_ASSERT(renderer);
    ALIA_ASSERT(ui);
    ALIA_ASSERT(device);
    ALIA_ASSERT(context);

    renderer->system = ui;
    renderer->device = device;
    renderer->context = context;
    if (!renderer_init_gpu(renderer))
    {
        std::fprintf(stderr, "[alia d3d11] GPU init failed\n");
        return;
    }

    alia_material_register(
        ui,
        ALIA_PRIMITIVE_MATERIAL_ID,
        alia_material_vtable{.draw_bucket = render_primitive_command_list},
        renderer);

    alia_renderer_ops const ops = {
        .upload_msdf_atlas =
            [](void* user, alia_msdf_atlas_image const* image) {
                alia_d3d11_renderer_upload_msdf_atlas(
                    static_cast<alia_d3d11_renderer*>(user), image);
            },
        .register_effect =
            [](void* user,
               alia_effect_desc const* desc,
               alia_draw_material_id* out_material_id) {
                return register_effect_blob(
                    static_cast<alia_d3d11_renderer*>(user),
                    desc,
                    out_material_id);
            },
        .user = renderer,
    };
    alia_ui_system_set_renderer_ops(ui, &ops);
}

int
alia_d3d11_effect_register(
    alia_d3d11_renderer* renderer,
    alia_d3d11_effect_desc const* desc,
    alia_draw_material_id* out_material_id)
{
    ALIA_ASSERT(renderer);
    ALIA_ASSERT(desc);
    ALIA_ASSERT(desc->pixel_shader_hlsl);
    ALIA_ASSERT(out_material_id);

    char const* entry
        = desc->entry_point != nullptr ? desc->entry_point : "ps_main";
    ID3DBlob* ps_blob = nullptr;
    if (!compile_shader(
            desc->pixel_shader_hlsl,
            "alia_d3d11_effect.hlsl",
            entry,
            "ps_5_0",
            &ps_blob))
        return -1;

    alia_effect_desc const portable = {
        .shader =
            {
                .format = ALIA_SHADER_FORMAT_DXBC,
                .data = ps_blob->GetBufferPointer(),
                .size = ps_blob->GetBufferSize(),
            },
        .params_size = desc->params_size,
    };
    int const rc = register_effect_blob(renderer, &portable, out_material_id);
    ps_blob->Release();
    return rc;
}

void
alia_d3d11_renderer_upload_msdf_atlas(
    alia_d3d11_renderer* renderer, alia_msdf_atlas_image const* image)
{
    ALIA_ASSERT(renderer);
    ALIA_ASSERT(image);
    ALIA_ASSERT(image->rgb);
    ALIA_ASSERT(renderer->device);

    release_t(renderer->msdf_srv);
    release_t(renderer->msdf_atlas);

    int const width = image->width;
    int const height = image->height;
    // D3D11 has no RGB8 texture format; pad to RGBA8.
    std::vector<uint8_t> rgba(size_t(width) * size_t(height) * 4u);
    uint8_t const* rgb = image->rgb;
    for (int i = 0; i < width * height; ++i)
    {
        rgba[size_t(i) * 4u + 0] = rgb[size_t(i) * 3u + 0];
        rgba[size_t(i) * 4u + 1] = rgb[size_t(i) * 3u + 1];
        rgba[size_t(i) * 4u + 2] = rgb[size_t(i) * 3u + 2];
        rgba[size_t(i) * 4u + 3] = 255;
    }

    D3D11_TEXTURE2D_DESC td{};
    td.Width = UINT(width);
    td.Height = UINT(height);
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA init{};
    init.pSysMem = rgba.data();
    init.SysMemPitch = UINT(width) * 4u;

    HRESULT hr = renderer->device->CreateTexture2D(
        &td, &init, &renderer->msdf_atlas);
    if (FAILED(hr) || !renderer->msdf_atlas)
    {
        std::fprintf(stderr, "[alia d3d11] MSDF atlas texture failed\n");
        return;
    }

    hr = renderer->device->CreateShaderResourceView(
        renderer->msdf_atlas, nullptr, &renderer->msdf_srv);
    if (FAILED(hr) || !renderer->msdf_srv)
    {
        std::fprintf(stderr, "[alia d3d11] MSDF atlas SRV failed\n");
        release_t(renderer->msdf_atlas);
    }
}

void
alia_d3d11_renderer_destroy(alia_d3d11_renderer* renderer)
{
    if (!renderer)
        return;
    for (auto& slot : renderer->effects)
    {
        if (!slot)
            continue;
        release_t(slot->ps);
        release_t(slot->params_cb);
    }
    renderer->effects.clear();
    release_t(renderer->effect_frame_cb);
    release_t(renderer->effect_blend);
    release_t(renderer->effect_quad_vb);
    release_t(renderer->effect_layout);
    release_t(renderer->effect_vs);
    release_t(renderer->msdf_srv);
    release_t(renderer->msdf_atlas);
    release_t(renderer->msdf_sampler);
    release_t(renderer->vs);
    release_t(renderer->ps);
    release_t(renderer->layout);
    release_t(renderer->quad_vb);
    release_t(renderer->instance_vb);
    release_t(renderer->constants);
    release_t(renderer->blend);
    release_t(renderer->rasterizer);
    release_t(renderer->depth);
    renderer->device = nullptr;
    renderer->context = nullptr;
    renderer->system = nullptr;
}

} // extern "C"
