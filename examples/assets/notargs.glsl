#version 130

out vec4 frag_color;

uniform vec2 corner;
uniform vec2 size;
uniform float zoom;

uniform float t;

uniform float curl;
uniform float thickness;

uniform bool normalize_rays;
uniform float step_scaling;
uniform int iterations;

uniform vec3 color;

const float tau = radians(360.0);

float sdf(vec3 p) {
    p.z -= t;
    float a = mod(p.z * 0.1, tau) * curl;
    p.xy *= mat2(cos(a), sin(a), -sin(a), cos(a));
    return thickness - length(cos(p.xy) + sin(p.yz));
}

void main() {
    // Get the location of the pixel in the viewport, normalized to [-1, 1].
    vec2 half_size = size / 2.0;
    vec2 uv = (gl_FragCoord.xy - corner - half_size) / (zoom * half_size.y);

    // Initialize the ray direction.
    vec3 d;
    if (normalize_rays) {
        d = normalize(vec3(uv, 1)) * step_scaling;
    } else {
        d = vec3(uv, step_scaling);
    }

    // March the ray.
    float x = 0.0;
    for(int i = 0; i < iterations; i++) {
        x += sdf(d * x);
    }

    // Set the color of the pixel.
    // frag_color = vec4(-x / 10, -x / 10, -x / 10, 1);
    vec3 p = vec3(d * x);
    frag_color = vec4((sin(p)+color)/length(p), 1);
}
