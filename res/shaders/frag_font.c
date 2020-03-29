uniform sampler2D tex;
in vec4 colorOut;
in vec2 texUV_out;

out vec4 color;

float smoothing = 0.3f;

void main (void) {

	vec4 texColor = texture(tex, texUV_out);
	float alpha = texColor.w;
	

	vec4 c = colorOut; //NOTE(ollie): Premultiply the alpha
	alpha = smoothstep(0.5f, 0.5f + smoothing, alpha);

	c.r*=alpha*c.a;
	c.g*=alpha*c.a;
	c.b*=alpha*c.a;
	c.a*=alpha;

	if(c.a <= 0.0f) discard;

	// c = vec4(1, 0, 0, 1);
    color = c;
}