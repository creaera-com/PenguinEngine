#version 450
layout(location=0) in vec4 color;
layout(location=1) in vec2 uv;
layout(location=0) out vec4 result;
layout(set=2,binding=0) uniform sampler2D image;
layout(std140,set=3,binding=0) uniform Parameters { vec4 crop; vec4 origin; vec4 shape; vec4 axis; vec4 ends; vec4 colors[8]; vec4 positions[8]; };
void main() {

    vec2 pixel_position = (crop.xy + uv * crop.zw - origin.xy) / origin.z - shape.xy - shape.zw * 0.5;
    vec2 local = vec2(axis.x*pixel_position.x+axis.y*pixel_position.y, -axis.y*pixel_position.x+axis.x*pixel_position.y)+shape.zw*0.5;
    vec2 delta = (ends.zw-ends.xy)*shape.zw;
    vec2 offset = local-ends.xy*shape.zw;
    float length2 = dot(delta,delta);
    float t = length2 > 0.0 ? dot(offset,delta)/length2 : 0.0;
    if (origin.w > 1.5 && origin.w < 2.5) t = length2 > 0.0 ? length(offset)/sqrt(length2) : 0.0;
    else if (origin.w > 3.5) t = abs(t);
    else if (origin.w > 2.5) { t = (atan(offset.y,offset.x)-atan(delta.y,delta.x))/6.28318530718; t -= floor(t); }
    t = clamp(t,0.0,1.0);
    int end = 1;
    while (end+1 < int(axis.z) && t > positions[end].x) ++end;
    float u = (t-positions[end-1].x)/(positions[end].x-positions[end-1].x);
    vec4 a = colors[end-1], b = colors[end];
    float alpha = a.a*(1.0-u)+b.a*u;
    vec3 rgb = alpha > 0.0 ? (a.rgb*a.a*(1.0-u)+b.rgb*b.a*u)/alpha : vec3(0.0,0.0,0.0);
    result = vec4(rgb,alpha);
}
