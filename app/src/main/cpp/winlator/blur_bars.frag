#version 450
layout(binding=0) uniform sampler2D scene;
layout(binding=1) uniform sampler2D blurred;
layout(location=0) in vec2 uv;
layout(location=0) out vec4 color;
layout(push_constant) uniform Params {
    vec4 rect; // useful game rectangle, normalized in the composed scene
    vec2 stepUV;
    float brightness;
    int mode; // 0: horizontal, 1: vertical, 2: decorate/copy
    float edgeWidth;
} pc;

void main() {
    if (pc.mode < 2) {
        // Two separable nine-tap passes at quarter resolution. Clamp every tap
        // to useful pixels, so the black margins never bleed into the blur.
        vec2 halfPixel = 0.5 / vec2(textureSize(scene, 0));
        vec2 lo = pc.rect.xy + halfPixel;
        vec2 hi = max(lo, pc.rect.zw - halfPixel);
        vec3 sum = vec3(0.0);
        float total = 0.0;
        for (int i = -4; i <= 4; ++i) {
            float weight = exp(-0.5 * float(i*i) / 4.0);
            sum += texture(scene, clamp(uv + float(i) * pc.stepUV, lo, hi)).rgb * weight;
            total += weight;
        }
        color = vec4(sum / total, 1.0);
        return;
    }
    if (all(greaterThanEqual(uv, pc.rect.xy)) && all(lessThan(uv, pc.rect.zw))) {
        // No filtering, darkening or blur in the original game rectangle.
        color = texelFetch(scene, ivec2(gl_FragCoord.xy), 0);
        return;
    }
    vec2 p = uv;
    vec2 strip = (pc.rect.zw - pc.rect.xy) * pc.edgeWidth;
    if (p.x < pc.rect.x) p.x = pc.rect.x + strip.x * (pc.rect.x-p.x) / max(pc.rect.x, 1e-6);
    else if (p.x >= pc.rect.z) p.x = pc.rect.z - strip.x * (p.x-pc.rect.z) / max(1.0-pc.rect.z, 1e-6);
    if (p.y < pc.rect.y) p.y = pc.rect.y + strip.y * (pc.rect.y-p.y) / max(pc.rect.y, 1e-6);
    else if (p.y >= pc.rect.w) p.y = pc.rect.w - strip.y * (p.y-pc.rect.w) / max(1.0-pc.rect.w, 1e-6);
    vec2 halfPixel = 0.5 / vec2(textureSize(blurred, 0));
    p = clamp(p, pc.rect.xy + halfPixel, max(pc.rect.xy + halfPixel, pc.rect.zw - halfPixel));
    color = vec4(texture(blurred, p).rgb * pc.brightness, 1.0);
}
