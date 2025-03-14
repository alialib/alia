#version 130

out vec4 frag_color;

uniform float t;

uniform float curl;

uniform vec2 corner;
uniform vec2 size;

uniform float zoom;

uniform int iterations;

const float tau = radians(360.0);

float my_mod(float a, float b) {
    float c = b * trunc(a / b);
    return a < 0 ? c - a : a - c;
}

float f(vec3 p) {
    p.z -= t;
    float a = p.z * curl;
    p.xy *= mat2(cos(a), sin(a), -sin(a), cos(a));
    return 0.2 - length(cos(p.xy) + sin(p.yz));
}

void other_main() {
    vec2 half_size = size / 2.0;
    vec2 uv = (gl_FragCoord.xy - corner - half_size) / (zoom * half_size.y);
    vec3 d = normalize(vec3(uv, 1.0));
    // vec3 p = vec3(0);

    float x = 0.0;
    for(int i = 0; i < iterations * 4; i++) {
        float sd = f(d * x);
        x += sd;
    }

    frag_color = vec4(-x / 10, -x / 10, -x / 10, 1);
}

// Computes an approximate normal using finite differences
vec3 getNormal(vec3 p) {
    float epsilon = 0.001;
    vec2 e = vec2(epsilon, 0);
    return normalize(vec3(
        f(p + e.xyy) - f(p - e.xyy),
        f(p + e.yxy) - f(p - e.yxy),
        f(p + e.yyx) - f(p - e.yyx)
    ));
}

// Computes ambient occlusion by checking how much space is around the point
float ambientOcclusion(vec3 p, vec3 normal) {
    float ao = 0.0;
    float stepSize = 0.02;  // Small step away from the surface
    int samples = 5;        // Number of sample points

    for (int i = 1; i <= samples; i++) {
        float dist = f(p + normal * (i * stepSize)); // Sample along normal
        ao += dist / (i * stepSize); // Accumulate occlusion factor
    }

    return clamp(1.0 - (ao / samples), 0.0, 1.0);  // Normalize AO effect
}
// Soft Shadows: Marches along the light direction, checking occlusion
float softShadow(vec3 p, vec3 lightDir) {
    float shadow = 1.0;
    float stepSize = 0.05;  // Small step size for better accuracy
    float distFactor = 1.0;

    for (float t = 0.1; t < 1.5; t += stepSize) {
        float d = f(p + lightDir * t);
        if (d < 0.0) return 0.0; // Fully blocked
        shadow = min(shadow, 10.0 * d / t); // Smooth fade
    }

    return clamp(shadow, 0.2, 1.0); // Prevent full darkness
}

// Fog effect: Fades objects based on distance from the camera
vec3 applyFog(vec3 color, float depth) {
    float fogFactor = exp(-depth * 0.1);
    vec3 fogColor = vec3(0.2, 0.3, 0.5);
    return mix(fogColor, color, fogFactor);
}

void main() {
    vec2 half_size = size / 2.0;
    vec2 uv = (gl_FragCoord.xy - corner - half_size) / (zoom * half_size.y);
    vec3 d = normalize(vec3(uv, 1.0));
    vec3 p = vec3(0);

    for (int i = 0; i < 64; i++) {
        float dist = f(p);
        if (abs(dist) < 0.001) break;
        p += dist * d;
    }

    vec3 normal = getNormal(p);
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));

    // Compute Lambertian shading
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 color = diff * vec3(1.0, 0.8, 0.5) + 0.1;

    // Compute ambient occlusion
    float ao = ambientOcclusion(p, normal);
    color *= ao;

    // Compute soft shadow
    float shadow = softShadow(p, lightDir);
    color *= shadow;

    // Apply fog
    color = applyFog(color, length(p));

    frag_color = vec4(color, 1.0);
}
