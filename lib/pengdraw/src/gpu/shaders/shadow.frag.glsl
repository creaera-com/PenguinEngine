#version 450
layout(location=0) in vec4 color;
layout(location=1) in vec2 uv;
layout(location=0) out vec4 result;
layout(set=2,binding=0) uniform sampler2D image;
layout(std140,set=3,binding=0) uniform Parameters { vec4 sampling; vec4 tint; };
float alpha_at(vec2 point) {
    if (any(lessThan(point, vec2(0.0))) || any(greaterThanEqual(point, vec2(1.0)))) return 0.0;
    return texture(image, point).a;
}
void main() {
    if (sampling.w > 1.5) { result = vec4(tint.rgb, alpha_at(uv) * tint.a); return; }
    int radius = int(sampling.z);
    float alpha = 0.0;
    for (int i=-radius; i<=radius; ++i) {
        float value = alpha_at(uv + float(i) * sampling.xy);
        alpha = sampling.w > 0.5 ? max(alpha, value) : alpha + value;
    }
    if (sampling.w < 0.5) alpha /= float(2 * radius + 1);
    result = vec4(0.0, 0.0, 0.0, alpha);
}
