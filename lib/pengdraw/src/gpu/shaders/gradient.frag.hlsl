// GPU gradient evaluated inside tessellated shape triangles.
Texture2D image : register(t0,space2);
SamplerState image_sampler : register(s0,space2);
cbuffer Parameters : register(b0,space3) { float4 crop; float4 origin; float4 shape; float4 axis; float4 ends; float4 colors[8]; float4 positions[8]; };
struct Input { float4 color : COLOR0; float2 uv : TEXCOORD0; };
float4 main(Input input) : SV_Target { float2 uv = input.uv;

    float2 pixel_position = (crop.xy + uv * crop.zw - origin.xy) / origin.z - shape.xy - shape.zw * 0.5;
    float2 local = float2(axis.x*pixel_position.x+axis.y*pixel_position.y, -axis.y*pixel_position.x+axis.x*pixel_position.y)+shape.zw*0.5;
    float2 delta = (ends.zw-ends.xy)*shape.zw;
    float2 offset = local-ends.xy*shape.zw;
    float length2 = dot(delta,delta);
    float t = length2 > 0.0 ? dot(offset,delta)/length2 : 0.0;
    if (origin.w > 1.5 && origin.w < 2.5) t = length2 > 0.0 ? length(offset)/sqrt(length2) : 0.0;
    else if (origin.w > 3.5) t = abs(t);
    else if (origin.w > 2.5) { t = (atan2(offset.y,offset.x)-atan2(delta.y,delta.x))/6.28318530718; t -= floor(t); }
    t = clamp(t,0.0,1.0);
    int end = 1;
    while (end+1 < int(axis.z) && t > positions[end].x) ++end;
    float u = (t-positions[end-1].x)/(positions[end].x-positions[end-1].x);
    float4 a = colors[end-1], b = colors[end];
    float alpha = a.a*(1.0-u)+b.a*u;
    float3 rgb = alpha > 0.0 ? (a.rgb*a.a*(1.0-u)+b.rgb*b.a*u)/alpha : float3(0.0,0.0,0.0);
    return float4(rgb,alpha);
}
