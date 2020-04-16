uniform sampler2D tex;
in vec4 colorOut;
in vec2 texUV_out;
in vec2 uv01;

out vec4 color;

float outlineSize = 0.01;
void main (void) {
	vec4 texColor = texture(tex, texUV_out);
	float alpha = texColor.w;
	vec4 b = colorOut*colorOut.w;
	vec4 c = b*texColor;

	if((uv01.x < outlineSize || uv01.x > 1.0f - outlineSize) || (uv01.y < outlineSize || uv01.y > 1.0f - outlineSize)) {
		alpha = 1.0f; 
	} else {
		discard;
	}

	c *= alpha;

	if(alpha == 0) discard;
    color = c;
}