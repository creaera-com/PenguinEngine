// Shader source for SDL GPU renderer (sampler space 2, fragment uniforms space 3).
Texture2D image : register(t0, space2);
SamplerState image_sampler : register(s0, space2);
cbuffer Parameters : register(b0, space3) {
    float4 sampling; // texel step x/y, integer radius, mode: average=0, max=1, tint=2
    float4 tint;
};
struct Input { float4 color : COLOR0; float2 uv : TEXCOORD0; };
float alpha_at(float2 uv) {
    if (any(uv < 0.0) || any(uv >= 1.0)) return 0.0;
    return image.Sample(image_sampler, uv).a;
}
float4 main(Input input) : SV_Target {
    if (sampling.w > 1.5) return float4(tint.rgb, alpha_at(input.uv) * tint.a);
    int radius = (int)sampling.z;
    float alpha = 0.0;
    for (int i = -radius; i <= radius; ++i) {
        float value = alpha_at(input.uv + float(i) * sampling.xy);
        alpha = sampling.w > 0.5 ? max(alpha, value) : alpha + value;
    }
    if (sampling.w < 0.5) alpha /= float(2 * radius + 1);
    return float4(0.0, 0.0, 0.0, alpha);
}
