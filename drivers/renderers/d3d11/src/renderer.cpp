#include <alia/renderers/d3d11/renderer.h>

#include <alia/abi/base/arena.h>
#include <alia/abi/panic.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/drawing/primitives.h>
#include <alia/abi/ui/drawing/shader.h>
#include <alia/abi/ui/drawing/system.h>
#include <alia/abi/ui/drawing/targets.h>
#include <alia/abi/ui/system/api.h>
#include <alia/abi/ui/system/memory.h>
#include <alia/abi/ui/system/renderer.h>
#include <alia/impl/base/arena.hpp>
#include <alia/ui/drawing/system.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <d3d11.h>
#include <d3dcompiler.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <new>
#include <vector>

struct alia_d3d11_renderer;

struct d3d11_user_shader_slot
{
    alia_d3d11_renderer* renderer = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3D11Buffer* params_cb = nullptr;
    size_t params_size = 0;
    size_t cb_bytes = 0;
};

struct d3d11_offscreen_target
{
    bool in_use = false;
    uint32_t width = 0;
    uint32_t height = 0;
    ID3D11Texture2D* tex = nullptr;
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
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
    // pool ID for rectangle instances filled during a primitive draw
    alia_memory_pool_id rect_instance_pool = 0;
    bool rect_instance_pool_registered = false;

    // Shared user-shader geometry; each registered shader is its own material.
    ID3D11VertexShader* user_shader_vs = nullptr;
    ID3D11InputLayout* user_shader_layout = nullptr;
    ID3D11Buffer* user_shader_quad_vb = nullptr;
    ID3D11BlendState* user_shader_blend = nullptr;
    // Stock frame CB (region + surface) bound at b0 for all user shaders.
    ID3D11Buffer* user_shader_frame_cb = nullptr;
    std::vector<std::unique_ptr<d3d11_user_shader_slot>> user_shaders;

    // Offscreen draw targets. Slot 0 is unused (ALIA_DRAW_TARGET_PRIMARY).
    std::vector<d3d11_offscreen_target> draw_targets;
    // Captured at draw_pass_begin; primary bind restores these.
    ID3D11RenderTargetView* pass_rtv = nullptr;
    ID3D11DepthStencilView* pass_dsv = nullptr;

    ID3D11VertexShader* blit_vs = nullptr;
    ID3D11PixelShader* blit_ps = nullptr;
    ID3D11InputLayout* blit_layout = nullptr;
    ID3D11Buffer* blit_cb = nullptr;
    ID3D11SamplerState* blit_sampler = nullptr;
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
    float4 border_lab : TEXCOORD7;
};

static const int k_gradient_fill = 256;
static const int k_gradient_border = 512;

float3 srgb_to_linear_vs(float3 srgb)
{
    float3 low = srgb / 12.92;
    float3 high = pow((srgb + 0.055) / 1.055, 2.4);
    float3 mask = step(0.04045, srgb);
    return lerp(low, high, mask);
}

float4 srgba_to_linear_vs(float4 srgba)
{
    return float4(srgb_to_linear_vs(srgba.rgb) * srgba.a, srgba.a);
}

float4 unpack_srgba(float packed_as_float)
{
    uint packed_bits = asuint(packed_as_float);
    return float4(
        float(packed_bits & 255u),
        float((packed_bits >> 8u) & 255u),
        float((packed_bits >> 16u) & 255u),
        float((packed_bits >> 24u) & 255u)) / 255.0;
}

float3 linear_to_oklab(float3 c)
{
    float l = dot(c, float3(0.4122214708, 0.5363325363, 0.0514459929));
    float m = dot(c, float3(0.2119034982, 0.6806995451, 0.1073969566));
    float s = dot(c, float3(0.0883024619, 0.2817188376, 0.6299787005));
    float3 lms = float3(
        pow(max(l, 0.0), 1.0 / 3.0),
        pow(max(m, 0.0), 1.0 / 3.0),
        pow(max(s, 0.0), 1.0 / 3.0));
    return float3(
        dot(lms, float3(0.2104542553, 0.7936177850, -0.0040720468)),
        dot(lms, float3(1.9779984951, -2.4285922050, 0.4505937099)),
        dot(lms, float3(0.0259040371, 0.7827717662, -0.8086757660)));
}

float4 oklab_stop(float4 srgba)
{
    return float4(linear_to_oklab(srgb_to_linear_vs(srgba.rgb)), srgba.a);
}

PSIn vs_main(VSIn input)
{
    PSIn o;
    o.border_lab = float4(0.0, 0.0, 0.0, 0.0);

    if (input.i_primitive_type == 3)
    {
        // MSDF glyphs: no box AA padding.
        float2 scaled = input.a_pos * input.i_size + input.i_pos;
        o.pos = mul(u_projection, float4(scaled, 0.0, 1.0));
        o.world = float2(0.0, 0.0);
        o.center = float2(0.0, 0.0);
        o.half_size = float2(0.0, 0.0);
        o.primitive_type = input.i_primitive_type;
        o.data_a = input.i_data_a;
        o.color = srgba_to_linear_vs(input.i_color);

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
        float sigma = input.i_data_b.z;
        float pad = sigma > 0.0 ? max(1.0, 3.0 * sigma) : 1.0;
        float2 scaled = input.a_pos * (input.i_size + float2(2.0 * pad, 2.0 * pad))
                      + input.i_pos - float2(pad, pad);
        o.pos = mul(u_projection, float4(scaled, 0.0, 1.0));
        o.world = scaled;
        o.center = input.i_pos + input.i_size * 0.5;
        o.half_size = input.i_size * 0.5;
        o.data_a = float4(input.i_data_a.xyz, sigma);
        o.uv_msdf = float2(0.0, 0.0);
        o.msdf_sdf_scale = 0.0;

        o.primitive_type = input.i_primitive_type;
        bool fill_grad = (input.i_primitive_type & k_gradient_fill) != 0;
        bool border_grad = (input.i_primitive_type & k_gradient_border) != 0;

        float2 local = scaled - o.center;
        float angle = input.i_data_b.y;
        float2 dir = float2(cos(angle), sin(angle));
        float span = dot(abs(dir), o.half_size);
        float t = span > 0.0
                    ? 0.5 + 0.5 * dot(local, dir) / span
                    : 0.0;

        if (fill_grad)
        {
            o.color = lerp(
                oklab_stop(input.i_color),
                oklab_stop(unpack_srgba(input.i_data_a.w)),
                t);
        }
        else
        {
            o.color = srgba_to_linear_vs(input.i_color);
        }

        if (border_grad)
        {
            o.border_lab = lerp(
                oklab_stop(unpack_srgba(input.i_data_a.z)),
                oklab_stop(unpack_srgba(input.i_data_b.x)),
                t);
        }
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

float3 oklab_to_linear(float3 c)
{
    float l_ = c.x + 0.3963377774 * c.y + 0.2158037573 * c.z;
    float m_ = c.x - 0.1055613458 * c.y - 0.0638541728 * c.z;
    float s_ = c.x - 0.0894841775 * c.y - 1.2914855480 * c.z;
    float l = l_ * l_ * l_;
    float m = m_ * m_ * m_;
    float s = s_ * s_ * s_;
    return float3(
        +4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s,
        -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s,
        -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s);
}

float4 premul_oklab(float4 lab_a)
{
    float3 rgb = clamp(oklab_to_linear(lab_a.xyz), 0.0, 1.0);
    float a = clamp(lab_a.w, 0.0, 1.0);
    return float4(rgb * a, a);
}

float shape_coverage(float d, float sigma)
{
    // Sigma is flat across the quad, so the untaken side does not run.
    const float aa = 0.5;
    if (sigma > 0.0)
    {
        return 1.0 / (1.0 + exp(1.6 * d / sigma));
    }
    return saturate(smoothstep(aa, -aa, d));
}

float4 shade_shape(PSIn input, float d)
{
    bool fill_grad = (input.primitive_type & k_gradient_fill) != 0;
    bool border_grad = (input.primitive_type & k_gradient_border) != 0;
    float4 fill = fill_grad ? premul_oklab(input.color) : input.color;
    // A zero-width border leaves the antialiased fringe on the fill.
    if (input.data_a.y <= 0.0)
        return fill * shape_coverage(d, input.data_a.w);
    float4 border = border_grad ? premul_oklab(input.border_lab)
                                : unpack_border_color_linear(input.data_a.z);
    float band = smoothstep(-0.5, 0.5, d + input.data_a.y);
    return lerp(fill, border, band) * shape_coverage(d, input.data_a.w);
}

float4 ps_main(PSIn input) : SV_TARGET
{
    if ((input.primitive_type & 255) == 3)
    {
        float3 msd = u_msdf.Sample(u_msdf_samp, input.uv_msdf).rgb;
        float sd = median_msdf(msd) - 0.5;
        float screen_px_distance = input.msdf_sdf_scale * sd;
        float opacity = saturate(screen_px_distance + 0.5);
        return input.color * opacity;
    }

    float2 local = input.world - input.center;
    int primitive = input.primitive_type & 255;

    if (primitive == 0)
    {
        float corner = min(
            input.data_a.x, min(input.half_size.x, input.half_size.y));
        float d = sd_round_rect(local, input.half_size, corner);
        return shade_shape(input, d);
    }

    if (primitive == 1)
    {
        float R = min(input.half_size.x, input.half_size.y);
        float d = sd_equilateral_triangle(
            rotate_point_clockwise(local, -input.data_a.x), R);
        return shade_shape(input, d);
    }

    if (primitive == 2)
    {
        float d = sd_squircle(local, input.data_a.x);
        return shade_shape(input, d);
    }

    return float4(0, 0, 0, 0);
}
)";

#pragma pack(push, 1)
struct primitive_instance
{
    float min[2];
    float size[2];
    uint8_t color[4];
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
    char const* entry, char const* target, ID3DBlob** out_blob)
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
         DXGI_FORMAT_R8G8B8A8_UNORM,
         1,
         16,
         D3D11_INPUT_PER_INSTANCE_DATA,
         1},
        {"I_TYPE",
         0,
         DXGI_FORMAT_R32_SINT,
         1,
         20,
         D3D11_INPUT_PER_INSTANCE_DATA,
         1},
        {"I_DATA_A",
         0,
         DXGI_FORMAT_R32G32B32A32_FLOAT,
         1,
         24,
         D3D11_INPUT_PER_INSTANCE_DATA,
         1},
        {"I_DATA_B",
         0,
         DXGI_FORMAT_R32G32B32A32_FLOAT,
         1,
         40,
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
    blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
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

    char const* const k_blit_hlsl = R"(
cbuffer BlitCB : register(b0)
{
    float4x4 u_projection;
    float2 dest_min;
    float2 dest_size;
    float opacity;
    float3 _pad;
};

Texture2D u_source : register(t0);
SamplerState u_samp : register(s0);

struct PSIn
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    nointerpolation float opacity : TEXCOORD1;
};

PSIn vs_main(float2 a_pos : POSITION)
{
    PSIn o;
    float2 world = a_pos * dest_size + dest_min;
    o.pos = mul(u_projection, float4(world, 0.0, 1.0));
    o.uv = a_pos;
    o.opacity = opacity;
    return o;
}

float4 ps_main(PSIn input) : SV_TARGET
{
    float4 c = u_source.Sample(u_samp, input.uv);
    return c * input.opacity;
}
)";

    ID3DBlob* blit_vs_blob = nullptr;
    ID3DBlob* blit_ps_blob = nullptr;
    if (!compile_shader(
            k_blit_hlsl,
            "alia_d3d11_blit.hlsl",
            "vs_main",
            "vs_5_0",
            &blit_vs_blob))
        return false;
    if (!compile_shader(
            k_blit_hlsl,
            "alia_d3d11_blit.hlsl",
            "ps_main",
            "ps_5_0",
            &blit_ps_blob))
    {
        blit_vs_blob->Release();
        return false;
    }
    hr = device->CreateVertexShader(
        blit_vs_blob->GetBufferPointer(),
        blit_vs_blob->GetBufferSize(),
        nullptr,
        &renderer->blit_vs);
    if (FAILED(hr))
    {
        blit_vs_blob->Release();
        blit_ps_blob->Release();
        return false;
    }
    hr = device->CreatePixelShader(
        blit_ps_blob->GetBufferPointer(),
        blit_ps_blob->GetBufferSize(),
        nullptr,
        &renderer->blit_ps);
    blit_ps_blob->Release();
    if (FAILED(hr))
    {
        blit_vs_blob->Release();
        return false;
    }
    D3D11_INPUT_ELEMENT_DESC blit_layout_desc[] = {
        {"POSITION",
         0,
         DXGI_FORMAT_R32G32_FLOAT,
         0,
         0,
         D3D11_INPUT_PER_VERTEX_DATA,
         0},
    };
    hr = device->CreateInputLayout(
        blit_layout_desc,
        1,
        blit_vs_blob->GetBufferPointer(),
        blit_vs_blob->GetBufferSize(),
        &renderer->blit_layout);
    blit_vs_blob->Release();
    if (FAILED(hr))
        return false;

    D3D11_BUFFER_DESC blit_cb_desc{};
    blit_cb_desc.ByteWidth = 96; // float4x4 + float2 + float2 + float + pad
    blit_cb_desc.Usage = D3D11_USAGE_DYNAMIC;
    blit_cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    blit_cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = device->CreateBuffer(&blit_cb_desc, nullptr, &renderer->blit_cb);
    if (FAILED(hr))
        return false;

    hr = device->CreateSamplerState(&samp, &renderer->blit_sampler);
    if (FAILED(hr))
        return false;

    // Slot 0 reserved for ALIA_DRAW_TARGET_PRIMARY.
    renderer->draw_targets.resize(1);

    renderer->rect_instance_pool = alia_memory_register_pool(
        alia_ui_memory(renderer->system),
        ALIA_MEMORY_POOL_BUMP,
        ALIA_MEMORY_RELEASE_ON_IDLE);
    renderer->rect_instance_pool_registered = true;
    return true;
}

bool
ensure_instance_capacity(alia_d3d11_renderer* renderer, UINT count)
{
    if (count <= renderer->instance_capacity && renderer->instance_vb)
        return true;

    release_t(renderer->instance_vb);
    UINT capacity
        = renderer->instance_capacity ? renderer->instance_capacity : 64;
    while (capacity < count)
        capacity *= 2;

    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = capacity * sizeof(primitive_instance);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    HRESULT hr = renderer->device->CreateBuffer(
        &desc, nullptr, &renderer->instance_vb);
    if (FAILED(hr))
        return false;
    renderer->instance_capacity = capacity;
    return true;
}

void
release_offscreen_target(d3d11_offscreen_target& target)
{
    release_t(target.srv);
    release_t(target.rtv);
    release_t(target.tex);
    target.width = 0;
    target.height = 0;
}

bool
ensure_offscreen_target(
    alia_d3d11_renderer* renderer,
    d3d11_offscreen_target& target,
    uint32_t width,
    uint32_t height)
{
    if (width == 0 || height == 0)
        return false;
    if (target.tex && target.width == width && target.height == height)
        return true;

    release_offscreen_target(target);

    // TYPELESS so we can create sRGB RTV/SRV views (same linear->sRGB encode
    // path as the swapchain backbuffer).
    D3D11_TEXTURE2D_DESC td{};
    td.Width = width;
    td.Height = height;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_B8G8R8A8_TYPELESS;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = renderer->device->CreateTexture2D(&td, nullptr, &target.tex);
    if (FAILED(hr) || !target.tex)
    {
        std::fprintf(
            stderr,
            "[alia d3d11] CreateTexture2D failed: 0x%08lx\n",
            (unsigned long) hr);
        return false;
    }

    D3D11_RENDER_TARGET_VIEW_DESC rtv_desc{};
    rtv_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    hr = renderer->device->CreateRenderTargetView(
        target.tex, &rtv_desc, &target.rtv);
    if (FAILED(hr) || !target.rtv)
    {
        std::fprintf(
            stderr,
            "[alia d3d11] CreateRenderTargetView failed: 0x%08lx\n",
            (unsigned long) hr);
        release_offscreen_target(target);
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
    srv_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = 1;
    hr = renderer->device->CreateShaderResourceView(
        target.tex, &srv_desc, &target.srv);
    if (FAILED(hr) || !target.srv)
    {
        std::fprintf(
            stderr,
            "[alia d3d11] CreateShaderResourceView failed: 0x%08lx\n",
            (unsigned long) hr);
        release_offscreen_target(target);
        return false;
    }

    target.width = width;
    target.height = height;
    return true;
}

d3d11_offscreen_target*
offscreen_target_slot(alia_d3d11_renderer* renderer, alia_draw_target_id id)
{
    if (id == ALIA_DRAW_TARGET_PRIMARY || id >= renderer->draw_targets.size())
        return nullptr;
    d3d11_offscreen_target& slot = renderer->draw_targets[id];
    if (!slot.in_use)
        return nullptr;
    return &slot;
}

alia_draw_target_id
d3d11_draw_target_create(void* user)
{
    auto* renderer = static_cast<alia_d3d11_renderer*>(user);
    for (size_t i = 1; i < renderer->draw_targets.size(); ++i)
    {
        if (!renderer->draw_targets[i].in_use)
        {
            renderer->draw_targets[i] = {};
            renderer->draw_targets[i].in_use = true;
            return alia_draw_target_id(i);
        }
    }
    if (renderer->draw_targets.size() >= 0xffff)
        return ALIA_DRAW_TARGET_PRIMARY;
    alia_draw_target_id const id
        = alia_draw_target_id(renderer->draw_targets.size());
    d3d11_offscreen_target slot{};
    slot.in_use = true;
    renderer->draw_targets.push_back(slot);
    return id;
}

void
d3d11_draw_target_destroy(void* user, alia_draw_target_id target)
{
    auto* renderer = static_cast<alia_d3d11_renderer*>(user);
    if (target == ALIA_DRAW_TARGET_PRIMARY
        || target >= renderer->draw_targets.size())
        return;
    release_offscreen_target(renderer->draw_targets[target]);
    renderer->draw_targets[target] = {};
}

void
d3d11_draw_target_ensure_size(
    void* user, alia_draw_target_id target, alia_vec2i size)
{
    auto* renderer = static_cast<alia_d3d11_renderer*>(user);
    if (target == ALIA_DRAW_TARGET_PRIMARY
        || target >= renderer->draw_targets.size())
        return;
    if (size.x <= 0 || size.y <= 0)
        return;
    if (!ensure_offscreen_target(
            renderer,
            renderer->draw_targets[target],
            uint32_t(size.x),
            uint32_t(size.y)))
    {
        std::fprintf(
            stderr,
            "[alia d3d11] draw_target_ensure_size failed for %u (%d x %d)\n",
            unsigned(target),
            size.x,
            size.y);
    }
}

void
d3d11_draw_pass_begin(void* user)
{
    auto* renderer = static_cast<alia_d3d11_renderer*>(user);
    release_t(renderer->pass_rtv);
    release_t(renderer->pass_dsv);
    renderer->context->OMGetRenderTargets(
        1, &renderer->pass_rtv, &renderer->pass_dsv);
}

void
d3d11_draw_pass_end(void* user)
{
    auto* renderer = static_cast<alia_d3d11_renderer*>(user);
    if (renderer->pass_rtv)
        renderer->context->OMSetRenderTargets(
            1, &renderer->pass_rtv, renderer->pass_dsv);
    release_t(renderer->pass_rtv);
    release_t(renderer->pass_dsv);
}

void
d3d11_draw_target_bind(void* user, alia_draw_target_id target)
{
    auto* renderer = static_cast<alia_d3d11_renderer*>(user);
    ID3D11ShaderResourceView* null_srv[1] = {nullptr};
    renderer->context->PSSetShaderResources(0, 1, null_srv);

    if (target == ALIA_DRAW_TARGET_PRIMARY)
    {
        if (renderer->pass_rtv)
            renderer->context->OMSetRenderTargets(
                1, &renderer->pass_rtv, renderer->pass_dsv);
        return;
    }

    d3d11_offscreen_target* slot = offscreen_target_slot(renderer, target);
    if (!slot || !slot->rtv)
    {
        std::fprintf(
            stderr,
            "[alia d3d11] draw_target_bind: target %u has no RTV\n",
            unsigned(target));
        if (renderer->pass_rtv)
            renderer->context->OMSetRenderTargets(
                1, &renderer->pass_rtv, renderer->pass_dsv);
        return;
    }
    renderer->context->OMSetRenderTargets(1, &slot->rtv, nullptr);
}

void
d3d11_draw_target_clear(
    void* user, alia_draw_target_id target, float const rgba[4])
{
    auto* renderer = static_cast<alia_d3d11_renderer*>(user);
    d3d11_offscreen_target* slot = offscreen_target_slot(renderer, target);
    if (!slot || !slot->rtv)
        return;
    renderer->context->ClearRenderTargetView(slot->rtv, rgba);
}

struct blit_cb_data
{
    float projection[16];
    float dest_min[2];
    float dest_size[2];
    float opacity;
    float pad[3];
};

void
render_draw_target_command_list(void* user, alia_draw_bucket const* bucket)
{
    auto* renderer = static_cast<alia_d3d11_renderer*>(user);
    if (!bucket || bucket->count == 0 || !bucket->clip_rect)
        return;
    if (!renderer->blit_vs || !renderer->blit_ps || !renderer->blit_cb)
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

    ctx->IASetInputLayout(renderer->blit_layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    UINT stride = sizeof(float) * 2;
    UINT offset = 0;
    ctx->IASetVertexBuffers(0, 1, &renderer->quad_vb, &stride, &offset);
    ctx->VSSetShader(renderer->blit_vs, nullptr, 0);
    ctx->PSSetShader(renderer->blit_ps, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, &renderer->blit_cb);
    ctx->PSSetSamplers(0, 1, &renderer->blit_sampler);
    ctx->OMSetBlendState(renderer->blend, nullptr, 0xffffffff);
    ctx->RSSetState(renderer->rasterizer);
    ctx->OMSetDepthStencilState(renderer->depth, 0);

    for (auto const* cmd = bucket->head; cmd; cmd = cmd->next)
    {
        auto const* blit = alia::downcast<alia_draw_target_command>(cmd);
        d3d11_offscreen_target* source
            = offscreen_target_slot(renderer, blit->source);
        if (!source || !source->srv)
            continue;

        blit_cb_data cb{};
        std::memcpy(cb.projection, ortho, sizeof(ortho));
        cb.dest_min[0] = blit->dest.min.x;
        cb.dest_min[1] = blit->dest.min.y;
        cb.dest_size[0] = blit->dest.size.x;
        cb.dest_size[1] = blit->dest.size.y;
        cb.opacity = blit->opacity;

        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (FAILED(ctx->Map(
                renderer->blit_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
            continue;
        std::memcpy(mapped.pData, &cb, sizeof(cb));
        ctx->Unmap(renderer->blit_cb, 0);

        ctx->PSSetShaderResources(0, 1, &source->srv);
        ctx->Draw(4, 0);
    }

    ID3D11ShaderResourceView* null_srv = nullptr;
    ctx->PSSetShaderResources(0, 1, &null_srv);
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
    if (FAILED(ctx->Map(
            renderer->constants, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        return;
    std::memcpy(mapped.pData, &cb, sizeof(cb));
    ctx->Unmap(renderer->constants, 0);

    alia_memory_system* memory = alia_ui_memory(renderer->system);
    alia_bump_allocator rect_alloc
        = alia_memory_open_bump(memory, renderer->rect_instance_pool);
    auto* instances = alia::arena_alloc_array<primitive_instance>(
        rect_alloc, bucket->count);

    auto pack_color = [](alia_srgba8 c) -> float {
        uint32_t packed = uint32_t(c.r) | (uint32_t(c.g) << 8u)
                        | (uint32_t(c.b) << 16u) | (uint32_t(c.a) << 24u);
        float f;
        std::memcpy(&f, &packed, sizeof(f));
        return f;
    };

    float const degrees_to_radians = 3.14159265358979323846f / 180.0f;

    auto pack_shape = [&](primitive_instance& inst,
                          alia_shape_style const& style,
                          float geometry,
                          float rotation_radians) {
        inst.data_a[0] = geometry;
        inst.data_a[1] = style.border_width;
        inst.data_a[2] = pack_color(style.border_color);
        inst.data_a[3] = pack_color(style.fill_end_color);
        inst.data_b[0] = pack_color(style.border_end_color);
        inst.data_b[2] = style.sigma;
        if (style.gradient)
        {
            inst.primitive_type |= ALIA_PRIMITIVE_FLAG_GRADIENT_FILL;
            if (style.border_width > 0.f
                && (style.border_color.r != style.border_end_color.r
                    || style.border_color.g != style.border_end_color.g
                    || style.border_color.b != style.border_end_color.b
                    || style.border_color.a != style.border_end_color.a))
            {
                inst.primitive_type |= ALIA_PRIMITIVE_FLAG_GRADIENT_BORDER;
            }
            inst.data_b[1]
                = style.gradient_angle * degrees_to_radians
                + rotation_radians;
        }
        else
        {
            inst.data_b[1] = 0.f;
        }
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
        inst.color[0] = primitive->color.r;
        inst.color[1] = primitive->color.g;
        inst.color[2] = primitive->color.b;
        inst.color[3] = primitive->color.a;
        inst.primitive_type
            = int(primitive->primitive_type & ALIA_PRIMITIVE_TYPE_MASK);
        std::memset(inst.data_a, 0, sizeof(inst.data_a));
        std::memset(inst.data_b, 0, sizeof(inst.data_b));

        switch (primitive->primitive_type & ALIA_PRIMITIVE_TYPE_MASK)
        {
            case ALIA_PRIMITIVE_BOX:
                pack_shape(
                    inst,
                    primitive->payload.box.style,
                    primitive->payload.box.corner_radius,
                    0.f);
                break;
            case ALIA_PRIMITIVE_EQUILATERAL_TRIANGLE: {
                float const radians
                    = primitive->payload.triangle.rotation_degrees
                    * degrees_to_radians;
                pack_shape(
                    inst, primitive->payload.triangle.style, radians, radians);
                break;
            }
            case ALIA_PRIMITIVE_SQUIRCLE:
                pack_shape(
                    inst,
                    primitive->payload.squircle.style,
                    primitive->payload.squircle.radius,
                    0.f);
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

    auto finish_instances = [&]() {
        alia_memory_commit_bump(
            memory, renderer->rect_instance_pool, &rect_alloc);
        alia_memory_trim(
            memory, renderer->rect_instance_pool, ALIA_MEMORY_RETAIN_PEAK);
    };

    if (written == 0)
    {
        finish_instances();
        return;
    }

    if (!ensure_instance_capacity(renderer, written))
    {
        finish_instances();
        return;
    }

    if (FAILED(ctx->Map(
            renderer->instance_vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        finish_instances();
        return;
    }
    std::memcpy(mapped.pData, instances, sizeof(primitive_instance) * written);
    ctx->Unmap(renderer->instance_vb, 0);
    finish_instances();

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

char const* const k_user_shader_vs_hlsl = R"(
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
user_shader_params_cb_bytes(size_t params_size)
{
    if (params_size == 0)
        return 16; // D3D11 constant buffers must be non-empty multiples of 16.
    return (params_size + 15u) & ~size_t(15u);
}

bool
ensure_user_shader_pipeline(alia_d3d11_renderer* renderer)
{
    if (renderer->user_shader_vs && renderer->user_shader_layout
        && renderer->user_shader_quad_vb && renderer->user_shader_blend
        && renderer->user_shader_frame_cb)
        return true;

    ID3DBlob* vs_blob = nullptr;
    if (!compile_shader(
            k_user_shader_vs_hlsl,
            "alia_d3d11_user_shader_vs.hlsl",
            "vs_main",
            "vs_5_0",
            &vs_blob))
        return false;

    ID3D11Device* device = renderer->device;
    HRESULT hr = device->CreateVertexShader(
        vs_blob->GetBufferPointer(),
        vs_blob->GetBufferSize(),
        nullptr,
        &renderer->user_shader_vs);
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
        &renderer->user_shader_layout);
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
    hr = device->CreateBuffer(
        &vb_desc, &vb_data, &renderer->user_shader_quad_vb);
    if (FAILED(hr))
        return false;

    D3D11_BLEND_DESC blend{};
    blend.RenderTarget[0].BlendEnable = FALSE;
    blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    hr = device->CreateBlendState(&blend, &renderer->user_shader_blend);
    if (FAILED(hr))
        return false;

    D3D11_BUFFER_DESC frame_desc{};
    frame_desc.ByteWidth = 32; // region float4 + surface float4
    frame_desc.Usage = D3D11_USAGE_DYNAMIC;
    frame_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    frame_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = device->CreateBuffer(
        &frame_desc, nullptr, &renderer->user_shader_frame_cb);
    return SUCCEEDED(hr);
}

void
render_user_shader_command_list(void* user, alia_draw_bucket const* bucket)
{
    auto* slot = static_cast<d3d11_user_shader_slot*>(user);
    if (!slot || !slot->renderer || !bucket || bucket->count == 0
        || !bucket->clip_rect)
        return;

    alia_d3d11_renderer* renderer = slot->renderer;
    if (!renderer->user_shader_vs || !renderer->user_shader_layout
        || !renderer->user_shader_quad_vb || !renderer->user_shader_frame_cb
        || !slot->ps || !slot->params_cb)
        return;

    alia_box const* clip = bucket->clip_rect;
    if (clip->size.x <= 0.f || clip->size.y <= 0.f)
        return;

    alia_vec2f const surface_size
        = alia_vec2i_to_vec2f(alia_ui_surface_get_size(renderer->system));

    ID3D11DeviceContext* ctx = renderer->context;
    ctx->IASetInputLayout(renderer->user_shader_layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    UINT stride = sizeof(float) * 2;
    UINT offset = 0;
    ctx->IASetVertexBuffers(
        0, 1, &renderer->user_shader_quad_vb, &stride, &offset);
    ctx->VSSetShader(renderer->user_shader_vs, nullptr, 0);
    ctx->PSSetShader(slot->ps, nullptr, 0);
    ctx->OMSetBlendState(renderer->user_shader_blend, nullptr, 0xffffffff);
    ctx->RSSetState(renderer->rasterizer);
    ctx->OMSetDepthStencilState(renderer->depth, 0);

    D3D11_RECT scissor{
        LONG(clip->min.x),
        LONG(clip->min.y),
        LONG(clip->min.x + clip->size.x),
        LONG(clip->min.y + clip->size.y)};
    ctx->RSSetScissorRects(1, &scissor);

    ID3D11Buffer* cbs[2] = {renderer->user_shader_frame_cb, slot->params_cb};
    ctx->PSSetConstantBuffers(0, 2, cbs);

    for (auto const* cmd = bucket->head; cmd; cmd = cmd->next)
    {
        auto const* shader_cmd = alia::downcast<alia_shader_draw_command>(cmd);

        alia_box const& r = shader_cmd->region;
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
                renderer->user_shader_frame_cb,
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
        ctx->Unmap(renderer->user_shader_frame_cb, 0);

        if (FAILED(ctx->Map(
                slot->params_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
            continue;
        std::memset(mapped.pData, 0, slot->cb_bytes);
        if (shader_cmd->params_size > 0 && shader_cmd->params)
        {
            size_t const copy_n = shader_cmd->params_size < slot->params_size
                                    ? shader_cmd->params_size
                                    : slot->params_size;
            std::memcpy(mapped.pData, shader_cmd->params, copy_n);
        }
        ctx->Unmap(slot->params_cb, 0);

        ctx->Draw(4, 0);
    }
}

int
register_shader_blob(
    alia_d3d11_renderer* renderer,
    alia_shader_desc const* desc,
    alia_draw_material_id* out_material_id)
{
    ALIA_ASSERT(renderer);
    ALIA_ASSERT(renderer->system);
    ALIA_ASSERT(renderer->device);
    ALIA_ASSERT(desc);
    ALIA_ASSERT(out_material_id);

    if (desc->shader.format != ALIA_D3D11_SHADER_FORMAT || !desc->shader.data
        || desc->shader.size == 0)
    {
        std::fprintf(
            stderr,
            "[alia d3d11] shader requires FourCC DXBC "
            "(ALIA_D3D11_SHADER_FORMAT)\n");
        return -1;
    }

    if (!ensure_user_shader_pipeline(renderer))
    {
        std::fprintf(
            stderr, "[alia d3d11] user-shader pipeline init failed\n");
        return -1;
    }

    auto slot = std::make_unique<d3d11_user_shader_slot>();
    slot->renderer = renderer;

    HRESULT hr = renderer->device->CreatePixelShader(
        desc->shader.data, desc->shader.size, nullptr, &slot->ps);
    if (FAILED(hr))
    {
        std::fprintf(stderr, "[alia d3d11] CreatePixelShader failed\n");
        return -1;
    }

    slot->params_size = desc->params_size;
    slot->cb_bytes = user_shader_params_cb_bytes(desc->params_size);
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
        alia_material_vtable{.draw_bucket = render_user_shader_command_list},
        slot.get());

    renderer->user_shaders.push_back(std::move(slot));
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
    alia_material_register(
        ui,
        ALIA_DRAW_TARGET_MATERIAL_ID,
        alia_material_vtable{.draw_bucket = render_draw_target_command_list},
        renderer);

    alia_renderer_ops const ops = {
        .upload_msdf_atlas =
            [](void* user, alia_msdf_atlas_image const* image) {
                alia_d3d11_renderer_upload_msdf_atlas(
                    static_cast<alia_d3d11_renderer*>(user), image);
            },
        .register_shader =
            [](void* user,
               alia_shader_desc const* desc,
               alia_draw_material_id* out_material_id) {
                return register_shader_blob(
                    static_cast<alia_d3d11_renderer*>(user),
                    desc,
                    out_material_id);
            },
        .draw_target_create = d3d11_draw_target_create,
        .draw_target_destroy = d3d11_draw_target_destroy,
        .draw_target_ensure_size = d3d11_draw_target_ensure_size,
        .draw_pass_begin = d3d11_draw_pass_begin,
        .draw_pass_end = d3d11_draw_pass_end,
        .draw_target_bind = d3d11_draw_target_bind,
        .draw_target_clear = d3d11_draw_target_clear,
        .user = renderer,
    };
    alia_ui_system_set_renderer_ops(ui, &ops);
}

int
alia_d3d11_shader_register(
    alia_d3d11_renderer* renderer,
    alia_d3d11_shader_desc const* desc,
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
            "alia_d3d11_user_shader.hlsl",
            entry,
            "ps_5_0",
            &ps_blob))
        return -1;

    alia_shader_desc const portable = {
        .shader =
            {
                .format = ALIA_D3D11_SHADER_FORMAT,
                .data = ps_blob->GetBufferPointer(),
                .size = ps_blob->GetBufferSize(),
            },
        .params_size = desc->params_size,
    };
    int const rc = register_shader_blob(renderer, &portable, out_material_id);
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

    HRESULT hr
        = renderer->device->CreateTexture2D(&td, &init, &renderer->msdf_atlas);
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

alia_memory_pool_id
alia_d3d11_renderer_pool(alia_d3d11_renderer* renderer, alia_d3d11_pool name)
{
    ALIA_ASSERT(renderer);
    switch (name)
    {
        case ALIA_D3D11_POOL_RECT_INSTANCES:
            return renderer->rect_instance_pool;
    }
    alia_panic_info info = {
        .reason = ALIA_PANIC_API_MISUSE,
        .subsystem = "memory",
        .msg = "D3D11 pool name",
    };
    alia_panic_now(&info);
    return 0;
}

void
alia_d3d11_renderer_destroy(alia_d3d11_renderer* renderer)
{
    if (!renderer)
        return;
    if (renderer->rect_instance_pool_registered && renderer->system)
    {
        alia_memory_unregister_pool(
            alia_ui_memory(renderer->system), renderer->rect_instance_pool);
        renderer->rect_instance_pool_registered = false;
    }
    for (auto& slot : renderer->user_shaders)
    {
        if (!slot)
            continue;
        release_t(slot->ps);
        release_t(slot->params_cb);
    }
    renderer->user_shaders.clear();
    for (auto& target : renderer->draw_targets)
        release_offscreen_target(target);
    renderer->draw_targets.clear();
    release_t(renderer->pass_rtv);
    release_t(renderer->pass_dsv);
    release_t(renderer->blit_sampler);
    release_t(renderer->blit_cb);
    release_t(renderer->blit_layout);
    release_t(renderer->blit_ps);
    release_t(renderer->blit_vs);
    release_t(renderer->user_shader_frame_cb);
    release_t(renderer->user_shader_blend);
    release_t(renderer->user_shader_quad_vb);
    release_t(renderer->user_shader_layout);
    release_t(renderer->user_shader_vs);
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
