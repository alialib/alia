#version 130

out vec4 frag_color;

uniform vec2 corner;
uniform vec2 size;
uniform float zoom;

uniform float t;

uniform float curl;
uniform float thickness;
uniform float cosx_factor;
uniform float cosy_factor;
uniform float siny_factor;
uniform float sinz_factor;

uniform bool normalize_rays;
uniform float step_scaling;
uniform int iterations;

uniform bool invert;
uniform vec3 color;
uniform float color_factor;
uniform float sine_factor;
uniform float depth_scale;

const float tau = radians(360.0);

float sdf(vec3 p) {
    p.z -= t;
    float a = mod(p.z * 0.1, tau) * curl;
    p.xy *= mat2(cos(a), sin(a), -sin(a), cos(a));
    return thickness -
        length(vec2(cos(p.x) * cosx_factor, cos(p.y) * cosy_factor) +
               vec2(sin(p.y) * siny_factor, sin(p.z) * sinz_factor));
}

void main() {
    // Get the location of the pixel in the viewport, normalized to [-1, 1].
    vec2 half_size = size / 2.0;
    vec2 uv = (gl_FragCoord.xy - corner - half_size) / (zoom * half_size.y);

    // Initialize the ray direction.
    vec3 direction;
    if (normalize_rays) {
        direction = normalize(vec3(uv, 1)) * step_scaling;
    } else {
        direction = vec3(uv, step_scaling);
    }

    // March the ray.
    float distance = 0.0;
    for(int i = 0; i < iterations; i++) {
        distance += sdf(direction * distance);
    }

    // Set the color of the pixel.
    vec3 p = vec3(direction * distance);
    float depth_factor =
        invert
            ? (length(p) * depth_scale / 120)
            : 1.0 / (length(p) * depth_scale);
    frag_color =
        vec4((sin(p) * sine_factor + color * color_factor) * depth_factor, 1);
}
