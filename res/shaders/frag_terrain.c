uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;

uniform sampler2D blendMap;

in vec2 uv_frag; 
out vec4 color;

uniform vec2 dim;

void main (void) {

    vec2 uv = fract(uv_frag/dim);
    vec4 blend = texture(blendMap, uv);
    float scale = 1.0f;
    vec2 localUv = fract(scale*uv_frag);
    vec4 blendA = texture(tex0, localUv);
    vec4 blendB = texture(tex1, localUv);
    vec4 blendC = texture(tex2, localUv);

    vec4 baseColor = texture(tex3, localUv);

    float baseAmount = 1.0f - clamp(blend.x + blend.y + blend.z, 0.0f, 1.0f);

    vec4 r = blend.x*blendA;
    vec4 g = blend.y*blendB;
    vec4 b = blend.z*blendC;
    vec4 a = baseAmount*baseColor;
    
    color = r + g + b + a;
    // color = blend;//vec4(1.0f);
}