uniform sampler2D tex;
in vec4 colorOut;
in vec2 texUV_out;

out vec4 color;

in vec3 fragNormalInWorld;
in vec3 fragPosInWorld;

uniform vec3 eyePosition; 
vec3 lightDirection = vec3(0, 0, -1); //looking down z-axis

void main (void) {
	vec4 texColor = texture(tex, texUV_out);
	float alpha = texColor.w;
	
	vec4 b = colorOut*colorOut.w; //premultply alpha
	vec4 c = b*texColor;
	c *= alpha; //pre-multiply alpha

	vec3 normal = normalize(fragNormalInWorld);

	vec3 eyeVector = normalize(eyePosition - fragPosInWorld);

	vec3 ab = eyeVector;//normalize(eyePosition);

	vec3 halfwayVec = normalize(eyeVector + lightDirection);

	float specular =  pow(max(0.0f, dot(normal, halfwayVec)), 1000);

	float diffuse = max(0.0f, (dot(eyeVector, normal)));
	float constant = 0.3f;
	//c = c + vec4(constant, constant, constant, 0)*specular; //diffuse shading
    color = c;//vec4(specular, specular, specular, 1);
}