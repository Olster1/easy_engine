uniform sampler2D tex;
in vec4 colorOut;
in vec2 texUV_out;

out vec4 color;
void main (void) {
	vec4 texColor = texture(tex, texUV_out);
		
	vec4 preMultAlphaColor = colorOut;
	
	vec4 c = preMultAlphaColor*texColor;

	c.rgb *= preMultAlphaColor.a;

	if(c.a == 0) discard; 

    color = c;
}