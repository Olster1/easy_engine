in vec2 texUV_out;
out vec4 color;

uniform sampler2D tex;
uniform float exposure;

void main(void) {

    const float gamma = 2.2;

    vec3 hdrColor = texture(tex, texUV_out).rgb;
    //Tone mapping

    vec3 toneMappedColor = vec3(1.0) - exp(-hdrColor*exposure);

    // Gamma correction 
    toneMappedColor = pow(toneMappedColor, vec3(1.0 / gamma));

    color = vec4(toneMappedColor, 1.0f);
}