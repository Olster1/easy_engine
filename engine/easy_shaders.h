static char *fragment_shader_texture_shader = "uniform sampler2D tex;\n"
"\n"
"in vec4 colorOut;\n"
"\n"
"in vec2 texUV_out;\n"
"\n"
"\n"
"\n"
"out vec4 color;\n"
"\n"
"void main (void) {\n"
"\n"
"	vec4 texColor = texture(tex, texUV_out);\n"
"\n"
"	float alpha = texColor.w;\n"
"\n"
"	vec4 b = colorOut*colorOut.w;\n"
"\n"
"	vec4 c = b*texColor;\n"
"\n"
"	c *= alpha;\n"
"\n"
"    color = c;\n"
"\n"
"}";

static char *frag_blur_shader = "out vec4 FragColor;\n"
"\n"
"out vec4 BrightColor;\n"
"\n"
"\n"
"\n"
"in vec2 texUV_out;\n"
"\n"
"\n"
"\n"
"uniform float horizontal;\n"
"\n"
"uniform sampler2D image;\n"
"\n"
"\n"
"\n"
"uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);\n"
"\n"
"\n"
"\n"
"void main()\n"
"\n"
"{    \n"
"\n"
"	vec2 tex_offset = textureSize(image, 0); // gets size of single texel\n"
"\n"
"	tex_offset.x = 1.0f / tex_offset.x;\n"
"\n"
"	tex_offset.y = 1.0f / tex_offset.y;\n"
"\n"
"\n"
"\n"
"    vec3 result = texture(image, texUV_out).rgb * weight[0]; // current fragment's contribution\n"
"\n"
"    if(horizontal > 0.0f)\n"
"\n"
"    {\n"
"\n"
"        for(int i = 1; i < 5; ++i)\n"
"\n"
"        {\n"
"\n"
"            result += texture(image, texUV_out + vec2(tex_offset.x * i, 0.0)).rgb * weight[i]; //convert per pixel to uv space\n"
"\n"
"            result += texture(image, texUV_out - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];\n"
"\n"
"        }\n"
"\n"
"    }\n"
"\n"
"    else\n"
"\n"
"    {\n"
"\n"
"        for(int i = 1; i < 5; ++i)\n"
"\n"
"        {\n"
"\n"
"            result += texture(image, texUV_out + vec2(0.0, tex_offset.y * i)).rgb * weight[i];\n"
"\n"
"            result += texture(image, texUV_out - vec2(0.0, tex_offset.y * i)).rgb * weight[i];\n"
"\n"
"        }\n"
"\n"
"    }\n"
"\n"
"\n"
"\n"
"    FragColor = vec4(result, 1.0f);\n"
"\n"
"\n"
"\n"
"   \n"
"\n"
"}";

static char *frag_circle_shader = "uniform sampler2D tex;\n"
"\n"
"in vec4 colorOut;\n"
"\n"
"in vec2 texUV_out;\n"
"\n"
"\n"
"\n"
"out vec4 color;\n"
"\n"
"void main (void) {\n"
"\n"
"	vec4 texColor = texture(tex, texUV_out);\n"
"\n"
"	float alpha = texColor.w;\n"
"\n"
"	if(alpha == 0) discard; \n"
"\n"
"\n"
"\n"
"\n"
"\n"
"	vec2 localToOrigin = 2*texUV_out - vec2(1, 1); \n"
"\n"
"	\n"
"\n"
"	if(length(localToOrigin) > 1.0f) {\n"
"\n"
"		discard;\n"
"\n"
"	}\n"
"\n"
"	vec4 b = colorOut*colorOut.w;\n"
"\n"
"	vec4 c = b*texColor;\n"
"\n"
"	c *= alpha;\n"
"\n"
"    color = c;\n"
"\n"
"}";

static char *frag_color_wheel_shader = "out vec4 FragColor;\n"
"\n"
"in vec2 texUV_out;\n"
"\n"
"in vec4 colorOut;\n"
"\n"
"\n"
"\n"
"uniform float value;\n"
"\n"
"\n"
"\n"
"vec4 hsvToRgb(float hueF, float saturation, float value) {  //h 0 - 360\n"
"\n"
"		float chroma = value * saturation;\n"
"\n"
"	    float hue1 = (hueF / 60);\n"
"\n"
"	    float modValue = mod(hue1, 2.0f);\n"
"\n"
"	    float x = chroma * (1- abs(modValue - 1));\n"
"\n"
"	    vec4 result = vec4(1.0f);\n"
"\n"
"	    if (hue1 >= 0 && hue1 <= 1) {\n"
"\n"
"	    	result.x = chroma;\n"
"\n"
"	    	result.y = x;\n"
"\n"
"	    	result.z = 0;\n"
"\n"
"	    } else if (hue1 >= 1 && hue1 <= 2) {\n"
"\n"
"	    	result.x = x;\n"
"\n"
"	    	result.y = chroma;\n"
"\n"
"	    	result.z = 0;\n"
"\n"
"	    } else if (hue1 >= 2 && hue1 <= 3) {\n"
"\n"
"	    	result.x = 0;\n"
"\n"
"	    	result.y = chroma;\n"
"\n"
"	    	result.z = x;\n"
"\n"
"	    } else if (hue1 >= 3 && hue1 <= 4) {\n"
"\n"
"	    	result.x = 0;\n"
"\n"
"	    	result.y = x;\n"
"\n"
"	    	result.z = chroma;\n"
"\n"
"	    } else if (hue1 >= 4 && hue1 <= 5) {\n"
"\n"
"	    	result.x = x;\n"
"\n"
"	    	result.y = 0;\n"
"\n"
"	    	result.z = chroma;\n"
"\n"
"	    } else if (hue1 >= 5 && hue1 <= 6) {\n"
"\n"
"	    	result.x = chroma;\n"
"\n"
"	    	result.y = 0;\n"
"\n"
"	    	result.z = x;\n"
"\n"
"	    }\n"
"\n"
"	    \n"
"\n"
"	    float m = value - chroma;\n"
"\n"
"    	result.x += m;	\n"
"\n"
"    	result.y += m;	\n"
"\n"
"    	result.z += m;	\n"
"\n"
"\n"
"\n"
"    	return result;\n"
"\n"
"}\n"
"\n"
"\n"
"\n"
"vec2 toOrigin(vec2 a) {\n"
"\n"
"	vec2 b = 2*a - vec2(1, 1);\n"
"\n"
"	return b;\n"
"\n"
"} \n"
"\n"
"\n"
"\n"
"#define M_PI 3.1415926535897932384626433832795\n"
"\n"
"\n"
"\n"
"void main (void) {\n"
"\n"
"\n"
"\n"
"	float inversePI = 1.0f / M_PI;\n"
"\n"
"	vec2 xFlip = vec2(1.0f - texUV_out.x, 1.0f - texUV_out.y);\n"
"\n"
"	vec2 localToOrigin = toOrigin(xFlip);\n"
"\n"
"	float hue = 0.0f;\n"
"\n"
"	if(localToOrigin.x != 0.0f) {\n"
"\n"
"		hue = atan(localToOrigin.y, localToOrigin.x);\n"
"\n"
"		hue = inversePI*hue*180.0f; //convert to -180 to 180 \n"
"\n"
"\n"
"\n"
"		hue += 180.0f; //between 0 - 360 \n"
"\n"
"\n"
"\n"
"		// hue /= 360.0f; //between 0 - 1\n"
"\n"
"	}\n"
"\n"
"\n"
"\n"
"	float saturation = length(localToOrigin);\n"
"\n"
"\n"
"\n"
"	vec4 color = hsvToRgb(hue, saturation, value);\n"
"\n"
"\n"
"\n"
"	float alphaVal = 1.0f;\n"
"\n"
"	if(saturation > 1.0f) {\n"
"\n"
"		// alphaVal = 0.0f;	\n"
"\n"
"		alphaVal = mix(1.0f, 0.0f, (saturation - 1.0f) / 0.01f);\n"
"\n"
"	} \n"
"\n"
"\n"
"\n"
"	// alphaVal = max(fwidth(alphaVal), alphaVal);\n"
"\n"
"	\n"
"\n"
"	FragColor = vec4(color.r, color.g, color.b, alphaVal);\n"
"\n"
"}";

static char *frag_font_shader = "uniform sampler2D tex;\n"
"\n"
"in vec4 colorOut;\n"
"\n"
"in vec2 texUV_out;\n"
"\n"
"\n"
"\n"
"out vec4 color;\n"
"\n"
"\n"
"\n"
"uniform float smoothing;\n"
"\n"
"\n"
"\n"
"void main (void) {\n"
"\n"
"\n"
"\n"
"	vec4 texColor = texture(tex, texUV_out);\n"
"\n"
"	float alpha = texColor.w;\n"
"\n"
"	\n"
"\n"
"\n"
"\n"
"	vec4 c = colorOut; //NOTE(ollie): Premultiply the alpha\n"
"\n"
"	alpha = smoothstep(0.5f, 0.5f + smoothing, alpha);\n"
"\n"
"\n"
"\n"
"	c.r*=alpha*c.a;\n"
"\n"
"	c.g*=alpha*c.a;\n"
"\n"
"	c.b*=alpha*c.a;\n"
"\n"
"	c.a*=alpha;\n"
"\n"
"\n"
"\n"
"	if(c.a <= 0.0f) discard;\n"
"\n"
"\n"
"\n"
"	// c = vec4(1, 0, 0, 1);\n"
"\n"
"    color = c;\n"
"\n"
"}";

static char *frag_gloss_shader = "uniform sampler2D tex;\n"
"\n"
"in vec4 colorOut;\n"
"\n"
"in vec2 texUV_out;\n"
"\n"
"\n"
"\n"
"out vec4 color;\n"
"\n"
"\n"
"\n"
"in vec3 fragNormalInWorld;\n"
"\n"
"in vec3 fragPosInWorld;\n"
"\n"
"\n"
"\n"
"uniform vec3 eyePosition; \n"
"\n"
"vec3 lightDirection = vec3(0, 0, -1); //looking down z-axis\n"
"\n"
"\n"
"\n"
"void main (void) {\n"
"\n"
"	vec4 texColor = texture(tex, texUV_out);\n"
"\n"
"		\n"
"\n"
"	vec4 preMultAlphaColor = colorOut;\n"
"\n"
"	\n"
"\n"
"	vec4 c = preMultAlphaColor*texColor;\n"
"\n"
"\n"
"\n"
"	c.rgb *= preMultAlphaColor.a; \n"
"\n"
"\n"
"\n"
"	vec3 normal = normalize(fragNormalInWorld);\n"
"\n"
"\n"
"\n"
"	vec3 eyeVector = normalize(eyePosition - fragPosInWorld);\n"
"\n"
"\n"
"\n"
"	vec3 ab = eyeVector;//normalize(eyePosition);\n"
"\n"
"\n"
"\n"
"	vec3 halfwayVec = normalize(eyeVector + lightDirection);\n"
"\n"
"\n"
"\n"
"	float specular =  pow(max(0.0f, dot(normal, halfwayVec)), 1000);\n"
"\n"
"\n"
"\n"
"	float diffuse = max(0.0f, (dot(eyeVector, normal)));\n"
"\n"
"	float constant = 0.3f;\n"
"\n"
"	//c = c + vec4(constant, constant, constant, 0)*specular; //diffuse shading\n"
"\n"
"    color = c;//vec4(specular, specular, specular, 1);\n"
"\n"
"}";

static char *frag_model_shader = "struct Material {\n"
"\n"
"    sampler2D diffuse;\n"
"\n"
"    sampler2D specular;\n"
"\n"
"    \n"
"\n"
"    float specularConstant;\n"
"\n"
"\n"
"\n"
"    vec4 ambientDefault;\n"
"\n"
"    vec4 diffuseDefault;\n"
"\n"
"    vec4 specularDefault;\n"
"\n"
"};\n"
"\n"
"\n"
"\n"
"struct DirLight {\n"
"\n"
"	vec4 ambient;\n"
"\n"
"	vec4 diffuse;\n"
"\n"
"	vec4 specular;\n"
"\n"
"\n"
"\n"
"	vec4 direction;\n"
"\n"
"};\n"
"\n"
"\n"
"\n"
"in vec4 color_frag; \n"
"\n"
"in vec3 normal_frag; //worldspace\n"
"\n"
"in vec2 uv_frag; \n"
"\n"
"in vec3 fragPos; //worldspace\n"
"\n"
"in vec3 fragPosInViewSpace; //worldspace\n"
"\n"
"\n"
"\n"
"uniform Material material;\n"
"\n"
"uniform DirLight lights[16];\n"
"\n"
"out vec4 color;\n"
"\n"
"out vec4 BrightColor;\n"
"\n"
"uniform vec3 eye_worldspace; \n"
"\n"
"\n"
"\n"
"void main (void) {\n"
"\n"
"    float gamma = 2.2;\n"
"\n"
"\n"
"\n"
"    vec4 c = color_frag;\n"
"\n"
"    //Normalize in the frag shader since interpolated one across vertexes isn't neccessarily normalized\n"
"\n"
"    vec3 normal = normalize(normal_frag);\n"
"\n"
"\n"
"\n"
"    vec4 diffSample = texture(material.diffuse, uv_frag);\n"
"\n"
"    if(diffSample.w == 0.0f) {\n"
"\n"
"        discard;\n"
"\n"
"    }\n"
"\n"
"    vec4 ambTex = material.ambientDefault*diffSample;\n"
"\n"
"    vec4 diffTex = material.diffuseDefault*diffSample;\n"
"\n"
"    vec4 specTex = material.specularDefault*texture(material.specular, uv_frag);\n"
"\n"
"\n"
"\n"
"    //NOTE(ol): Gamma correction\n"
"\n"
"    // ambTex = pow(ambTex, vec4(gamma));\n"
"\n"
"    // diffTex = pow(diffTex, vec4(gamma));\n"
"\n"
"    // specTex = pow(specTex, vec4(gamma));\n"
"\n"
"    \n"
"\n"
"    vec3 lightDir = vec3(normalize(-lights[0].direction));\n"
"\n"
"    float diff = max(dot(normal, lightDir), 0.2);\n"
"\n"
"\n"
"\n"
"    vec3 viewDir = normalize(eye_worldspace - fragPos);\n"
"\n"
"    vec3 halfwayVector = normalize(viewDir + lightDir);\n"
"\n"
"    float spec = pow(max(dot(normal, halfwayVector), 0.0), material.specularConstant);\n"
"\n"
"\n"
"\n"
"    vec4 ambientColor = ambTex * lights[0].ambient;\n"
"\n"
"    vec4 diffuseColor = diff * diffTex * lights[0].diffuse;\n"
"\n"
"    vec4 specularColor = spec * specTex * lights[0].specular;\n"
"\n"
"\n"
"\n"
"    ambientColor.w = ambTex.w;\n"
"\n"
"    diffuseColor.w = diffTex.w;\n"
"\n"
"    specularColor.w = specTex.w;\n"
"\n"
"\n"
"\n"
"    vec4 tempColor = c * vec4(ambientColor + diffuseColor + specularColor);\n"
"\n"
"    tempColor.w = min(1, tempColor.w);\n"
"\n"
"\n"
"\n"
"    float tAt = min(fragPosInViewSpace.z/100.0f, 1.0f);\n"
"\n"
"    color = tempColor;//mix(tempColor, vec4(0.4f, 0.6f, 1.0f, 1.0f), pow(tAt, 3.0f));\n"
"\n"
"\n"
"\n"
"    // color = pow(color, vec4(1.0/gamma));\n"
"\n"
"\n"
"\n"
"    float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));\n"
"\n"
"    if(brightness > 1.0)\n"
"\n"
"        BrightColor = vec4(color.rgb, 1.0);\n"
"\n"
"    else\n"
"\n"
"        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
"\n"
"}";

static char *frag_skybox_shader = "out vec4 FragColor;\n"
"\n"
"\n"
"\n"
"in vec3 TexCoords;\n"
"\n"
"\n"
"\n"
"uniform samplerCube skybox;\n"
"\n"
"\n"
"\n"
"void main()\n"
"\n"
"{    \n"
"\n"
"    FragColor = texture(skybox, TexCoords);\n"
"\n"
"}";

static char *frag_sky_quad_shader = "in vec3 direction_frag; \n"
"\n"
"out vec4 color;\n"
"\n"
"out vec4 BrightColor;\n"
"\n"
"\n"
"\n"
"uniform vec4 skyColor;\n"
"\n"
"uniform vec3 eyePos;\n"
"\n"
"\n"
"\n"
"uniform float tScale;\n"
"\n"
"\n"
"\n"
"vec3 windDir;\n"
"\n"
"uniform float timeScale;\n"
"\n"
"\n"
"\n"
"uniform vec3 sunDirection;\n"
"\n"
"\n"
"\n"
"#define NOISE_SCALE 0.1f\n"
"\n"
"#define NOISE_THRESHOLD 0.6f\n"
"\n"
"#define TIME_SCALE_MULTIPLIER 0.1f\n"
"\n"
"#define NUMBER_OF_SAMPLES 10\n"
"\n"
"\n"
"\n"
"float hash(float n) { return fract(sin(n) * 1e4); }\n"
"\n"
"\n"
"\n"
"float noise3d(vec3 x) {\n"
"\n"
"	const vec3 step = vec3(110, 241, 171);\n"
"\n"
"\n"
"\n"
"	vec3 i = floor(x);\n"
"\n"
"	vec3 f = fract(x);\n"
"\n"
" \n"
"\n"
"	// For performance, compute the base input to a 1D hash from the integer part of the argument and the \n"
"\n"
"	// incremental change to the 1D based on the 3D -> 1D wrapping\n"
"\n"
"    float n = dot(i, step);\n"
"\n"
"\n"
"\n"
"	vec3 u = f * f * (3.0 - 2.0 * f);\n"
"\n"
"	return mix(mix(mix( hash(n + dot(step, vec3(0, 0, 0))), hash(n + dot(step, vec3(1, 0, 0))), u.x),\n"
"\n"
"                   mix( hash(n + dot(step, vec3(0, 1, 0))), hash(n + dot(step, vec3(1, 1, 0))), u.x), u.y),\n"
"\n"
"               mix(mix( hash(n + dot(step, vec3(0, 0, 1))), hash(n + dot(step, vec3(1, 0, 1))), u.x),\n"
"\n"
"                   mix( hash(n + dot(step, vec3(0, 1, 1))), hash(n + dot(step, vec3(1, 1, 1))), u.x), u.y), u.z);\n"
"\n"
"}\n"
"\n"
"\n"
"\n"
"#define OCTAVES 6\n"
"\n"
"float fbm (vec3 st) {\n"
"\n"
"    // Initial values\n"
"\n"
"    float value = 0.0;\n"
"\n"
"    float amplitude = .5;\n"
"\n"
"    float frequency = 0.;\n"
"\n"
"    //\n"
"\n"
"    // Loop of octaves\n"
"\n"
"    for (int i = 0; i < OCTAVES; i++) {\n"
"\n"
"        value += amplitude * noise3d(st);\n"
"\n"
"        st *= 2.;\n"
"\n"
"        amplitude *= .5;\n"
"\n"
"    }\n"
"\n"
"    return value;\n"
"\n"
"}\n"
"\n"
"\n"
"\n"
"float getClosestT(float radius, vec3 startP, vec3 dir) {\n"
"\n"
"	float t0 = 0;\n"
"\n"
"	float t1 = 0;\n"
"\n"
"	float radius2 = radius*radius;\n"
"\n"
"\n"
"\n"
"	vec3 L = vec3(0.0f) - startP; \n"
"\n"
"    float tca = dot(L, dir); \n"
"\n"
"    // if (tca < 0) return false;\n"
"\n"
"    float d2 = dot(L, L) - tca * tca; \n"
"\n"
"    if (d2 > radius2) {\n"
"\n"
"    	t0 = -1;\n"
"\n"
"    } else {\n"
"\n"
"    	float thc = sqrt(radius2 - d2); \n"
"\n"
"    	t0 = tca - thc; \n"
"\n"
"    	t1 = tca + thc;\n"
"\n"
"\n"
"\n"
"    	if(t0 > t1) {\n"
"\n"
"    		float temp = t0;\n"
"\n"
"    		t0 = t1;\n"
"\n"
"    		t1 = temp;\n"
"\n"
"    	} \n"
"\n"
"\n"
"\n"
"    	if(t0 < 0) {\n"
"\n"
"    		t0 = t1;\n"
"\n"
"    	} \n"
"\n"
"\n"
"\n"
"    	if(t0 < 0) {\n"
"\n"
"    		t0 = -1;\n"
"\n"
"    	}\n"
"\n"
"\n"
"\n"
"    	\n"
"\n"
"    }\n"
"\n"
"\n"
"\n"
"    return t0;\n"
"\n"
"    \n"
"\n"
"}\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"void main (void) {\n"
"\n"
"    vec3 direction_frag_norm = normalize(direction_frag);\n"
"\n"
"\n"
"\n"
"    vec4 tempColor = skyColor;\n"
"\n"
"    float tVal = dot(-normalize(sunDirection), direction_frag_norm);\n"
"\n"
"    if(tVal > 0.9999f) {\n"
"\n"
"    	tempColor = vec4(10000, 10000, 10000, 1);//mix(tempColor, vec4(1, 1, 0, 1), smoothstep(0, 1, tVal));\n"
"\n"
"\n"
"\n"
"    }\n"
"\n"
"    vec4 colorOfSky = mix(tempColor, vec4(1, 1, 2, 1), pow(1.0f - max(direction_frag_norm.y, 0.0f), 3));\n"
"\n"
"\n"
"\n"
"   	windDir = normalize(vec3(1, -0.01, 0.2));\n"
"\n"
"    float cloudContrubtion = 0;\n"
"\n"
"\n"
"\n"
"    float lowerY = 150;\n"
"\n"
"    float upperY = 151;\n"
"\n"
"\n"
"\n"
"    float t0 = getClosestT(lowerY, eyePos, direction_frag_norm);\n"
"\n"
"    float t1 = getClosestT(upperY, eyePos, direction_frag_norm);\n"
"\n"
"\n"
"\n"
"    if(t0 >= 0.0f || t1 >= 0.0f) { //parrellel\n"
"\n"
"    	if(t0 > t1) {\n"
"\n"
"    		float temp = t0;\n"
"\n"
"    		t0 = t1;\n"
"\n"
"    		t1 = temp;\n"
"\n"
"    	} \n"
"\n"
"\n"
"\n"
"    	vec3 startP = eyePos;\n"
"\n"
"    	\n"
"\n"
"    	if(t0 > 0.0f) { //in front of ray\n"
"\n"
"    		startP = eyePos + t0*direction_frag_norm;\n"
"\n"
"    	}\n"
"\n"
"\n"
"\n"
"    	\n"
"\n"
"    	if(t1 > 0.0f) {\n"
"\n"
"    		vec3 endP = eyePos + t1*direction_frag_norm;\n"
"\n"
"\n"
"\n"
"    		float deltaT = length(endP - startP) / NUMBER_OF_SAMPLES; //TODO: Can we use the dot product instead\n"
"\n"
"    		\n"
"\n"
"    		for(int sIndex = 0; sIndex < NUMBER_OF_SAMPLES; ++sIndex) {\n"
"\n"
"    			vec3 sampleP = startP + (sIndex*deltaT)*direction_frag_norm;\n"
"\n"
"    			float val = fbm(NOISE_SCALE*sampleP + TIME_SCALE_MULTIPLIER*windDir*timeScale);\n"
"\n"
"    			if(val > NOISE_THRESHOLD) {\n"
"\n"
"    				cloudContrubtion += 0.7f*deltaT*val;\n"
"\n"
"    			}\n"
"\n"
"    			\n"
"\n"
"    		}\n"
"\n"
"    	} else {\n"
"\n"
"    		//not looking at the clouds\n"
"\n"
"    		cloudContrubtion = 0;\n"
"\n"
"    	}\n"
"\n"
"    }\n"
"\n"
"\n"
"\n"
"    cloudContrubtion = cloudContrubtion; //nomralize the value\n"
"\n"
"    float blendFactor = 1.0f - exp(-cloudContrubtion);\n"
"\n"
"    \n"
"\n"
"    vec3 cloudColor = vec3(1.5) + cloudContrubtion;\n"
"\n"
"    color = mix(colorOfSky, vec4(cloudColor, 1.0f), blendFactor);\n"
"\n"
"\n"
"\n"
"    float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));\n"
"\n"
"    if(brightness > 1.0)\n"
"\n"
"        BrightColor = vec4(color.rgb, 1.0);\n"
"\n"
"    else\n"
"\n"
"        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
"\n"
"}";

static char *frag_terrain_shader = "uniform sampler2D tex0;\n"
"\n"
"uniform sampler2D tex1;\n"
"\n"
"uniform sampler2D tex2;\n"
"\n"
"uniform sampler2D tex3;\n"
"\n"
"\n"
"\n"
"uniform sampler2D blendMap;\n"
"\n"
"\n"
"\n"
"in vec2 uv_frag; \n"
"\n"
"out vec4 color;\n"
"\n"
"\n"
"\n"
"out vec4 BrightColor;\n"
"\n"
"\n"
"\n"
"uniform vec2 dim;\n"
"\n"
"\n"
"\n"
"void main (void) {\n"
"\n"
"\n"
"\n"
"    vec2 uv = fract(uv_frag/dim);\n"
"\n"
"    vec4 blend = texture(blendMap, uv);\n"
"\n"
"    float scale = 1.0f;\n"
"\n"
"    vec2 localUv = fract(scale*uv_frag);\n"
"\n"
"    vec4 blendA = texture(tex0, localUv);\n"
"\n"
"    vec4 blendB = texture(tex1, localUv);\n"
"\n"
"    vec4 blendC = texture(tex2, localUv);\n"
"\n"
"\n"
"\n"
"    vec4 baseColor = texture(tex3, localUv);\n"
"\n"
"\n"
"\n"
"    float baseAmount = 1.0f - clamp(blend.x + blend.y + blend.z, 0.0f, 1.0f);\n"
"\n"
"\n"
"\n"
"    vec4 r = blend.x*blendA;\n"
"\n"
"    vec4 g = blend.y*blendB;\n"
"\n"
"    vec4 b = blend.z*blendC;\n"
"\n"
"    vec4 a = baseAmount*baseColor;\n"
"\n"
"    \n"
"\n"
"    color = r + g + b + a;\n"
"\n"
"    // color = blend;//vec4(1.0f);\n"
"\n"
"\n"
"\n"
"    float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));\n"
"\n"
"    if(brightness > 1.0)\n"
"\n"
"        BrightColor = vec4(color.rgb, 1.0);\n"
"\n"
"    else\n"
"\n"
"        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
"\n"
"}";

static char *frag_tone_map_shader = "in vec2 texUV_out;\n"
"\n"
"out vec4 color;\n"
"\n"
"\n"
"\n"
"uniform sampler2D tex;\n"
"\n"
"uniform sampler2D bloom;\n"
"\n"
"uniform float exposure;\n"
"\n"
"\n"
"\n"
"void main(void) {\n"
"\n"
"\n"
"\n"
"    const float gamma = 2.2;\n"
"\n"
"\n"
"\n"
"    vec3 hdrColor = texture(tex, texUV_out).rgb + texture(bloom, texUV_out).rgb;\n"
"\n"
"    //Tone mapping\n"
"\n"
"\n"
"\n"
"    vec3 toneMappedColor = vec3(1.0) - exp(-hdrColor*exposure);\n"
"\n"
"\n"
"\n"
"    // Gamma correction \n"
"\n"
"    toneMappedColor = pow(toneMappedColor, vec3(1.0 / gamma));\n"
"\n"
"\n"
"\n"
"    color = vec4(toneMappedColor, 1.0f);\n"
"\n"
"}";

static char *vertex_fullscreen_quad_shader = "in vec3 vertex;\n"
"\n"
"in vec3 normal;\n"
"\n"
"in vec2 texUV;	\n"
"\n"
"\n"
"\n"
"//outgoing variables\n"
"\n"
"out vec2 texUV_out; \n"
"\n"
"\n"
"\n"
"void main() {\n"
"\n"
"    gl_Position = vec4(vertex.x, vertex.y, -1.0f, 1);\n"
"\n"
"    texUV_out = texUV;\n"
"\n"
"}";

static char *vertex_gloss_shader = "in vec3 vertex;\n"
"\n"
"in vec3 normal;\n"
"\n"
"in vec2 texUV;	\n"
"\n"
"\n"
"\n"
"in mat4 M;\n"
"\n"
"in mat4 V;\n"
"\n"
"\n"
"\n"
"in vec4 color;\n"
"\n"
"in vec4 uvAtlas;	\n"
"\n"
"\n"
"\n"
"uniform mat4 projection;\n"
"\n"
"\n"
"\n"
"out vec4 colorOut; //out going\n"
"\n"
"out vec2 texUV_out;\n"
"\n"
"\n"
"\n"
"out vec3 fragNormalInWorld;\n"
"\n"
"out vec3 fragPosInWorld;\n"
"\n"
"\n"
"\n"
"void main() {\n"
"\n"
"    \n"
"\n"
"    gl_Position = projection * V * M * vec4(vertex, 1);\n"
"\n"
"    colorOut = color;\n"
"\n"
"    \n"
"\n"
"    int xAt = int(texUV.x*2);\n"
"\n"
"    int yAt = int(texUV.y*2) + 1;\n"
"\n"
"    texUV_out = vec2(uvAtlas[xAt], uvAtlas[yAt]);\n"
"\n"
"\n"
"\n"
"    fragNormalInWorld = mat3(transpose(inverse(M))) * normal; //knock out the translation vectors\n"
"\n"
"    fragPosInWorld = (M * vec4(vertex, 1)).xyz; //knock out the translation vectors\n"
"\n"
"}";

static char *vertex_model_shader = "in vec3 vertex;\n"
"\n"
"in vec3 normal;\n"
"\n"
"in vec2 texUV;	\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"//Instanced variables\n"
"\n"
"in mat4 M;\n"
"\n"
"in mat4 V;\n"
"\n"
"\n"
"\n"
"in vec4 color;\n"
"\n"
"in vec4 uvAtlas;	\n"
"\n"
"\n"
"\n"
"uniform mat4 projection;\n"
"\n"
"//outgoing variables\n"
"\n"
"out vec4 color_frag; \n"
"\n"
"out vec3 normal_frag;\n"
"\n"
"out vec2 uv_frag;\n"
"\n"
"out vec3 fragPos;\n"
"\n"
"out vec3 fragPosInViewSpace;\n"
"\n"
"\n"
"\n"
"void main() {\n"
"\n"
"    \n"
"\n"
"    gl_Position = projection * V * M * vec4(vertex, 1);\n"
"\n"
"    color_frag = color;\n"
"\n"
"    normal_frag = mat3(transpose(inverse(M))) * normal; //transpose(inverse(M))\n"
"\n"
"    fragPos = vec3(M * vec4(vertex, 1));\n"
"\n"
"    fragPosInViewSpace = vec3(V*M * vec4(vertex, 1));\n"
"\n"
"\n"
"\n"
"    uv_frag = texUV;//vec2(mix(uvAtlas.x, uvAtlas.y, texUV.x), mix(uvAtlas.z, uvAtlas.w, texUV.y));\n"
"\n"
"}";

static char *vertex_model_as_2d_image_shader = "in vec3 vertex;\n"
"\n"
"in vec3 normal;\n"
"\n"
"in vec2 texUV;	\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"//Instanced variables\n"
"\n"
"in mat4 M;\n"
"\n"
"in mat4 V;\n"
"\n"
"\n"
"\n"
"in vec4 color;\n"
"\n"
"in vec4 uvAtlas;	//not using a atlas\n"
"\n"
"\n"
"\n"
"uniform mat4 projection;\n"
"\n"
"\n"
"\n"
"//NOTE(ollie): This matrix means we can't batch multiple calls, however we could get rid of it by putting the divide by w in the matrix (but not sure if you can do this? SInce the your dividing by the value of the coordinate, like double differential equations?)\n"
"\n"
"uniform mat4 orthoToScreen; //this is the matrix the transforms lerps between the x & y positions then from there transforms it back into NDC space\n"
"\n"
"\n"
"\n"
"//outgoing variables\n"
"\n"
"out vec4 color_frag; \n"
"\n"
"out vec3 normal_frag;\n"
"\n"
"out vec2 uv_frag;\n"
"\n"
"out vec3 fragPos;\n"
"\n"
"out vec3 fragPosInViewSpace;\n"
"\n"
"\n"
"\n"
"void main() {\n"
"\n"
"    \n"
"\n"
"    vec4 pos = projection * V * M * vec4(vertex, 1);\n"
"\n"
"    //Now in clip \n"
"\n"
"    pos.x /= pos.w;\n"
"\n"
"    pos.y /= pos.w;\n"
"\n"
"    pos.z /= pos.w;\n"
"\n"
"    pos.w  = 1.0f;\n"
"\n"
"\n"
"\n"
"    //Convert between -1 -> 1 to 0 -> 1\n"
"\n"
"    pos.x += 1.0f;\n"
"\n"
"    pos.x /= 2.0f;\n"
"\n"
"    \n"
"\n"
"    pos.y += 1.0f;\n"
"\n"
"    pos.y /= 2.0f;\n"
"\n"
"\n"
"\n"
"    //We don't do z since we are using the z-coordinate based off the z we send in the ortho matrix\n"
"\n"
"\n"
"\n"
"    //Now in NDC space\n"
"\n"
"    gl_Position = orthoToScreen * pos; //homegenous coordinrate since we want the translation component as well\n"
"\n"
"    color_frag = color;\n"
"\n"
"    normal_frag = mat3(transpose(inverse(M))) * normal; //transpose(inverse(M))\n"
"\n"
"    uv_frag = texUV;\n"
"\n"
"    fragPos = vec3(M * vec4(vertex, 1));\n"
"\n"
"    fragPosInViewSpace = vec3(V*M * vec4(vertex, 1));\n"
"\n"
"}";

static char *vertex_shader_tex_attrib_shader = "in vec3 vertex;\n"
"\n"
"in vec2 texUV;	\n"
"\n"
"\n"
"\n"
"in mat4 M;\n"
"\n"
"in mat4 V;\n"
"\n"
"\n"
"\n"
"in vec4 color;\n"
"\n"
"in vec4 uvAtlas;	\n"
"\n"
"\n"
"\n"
"uniform mat4 projection;\n"
"\n"
"\n"
"\n"
"out vec4 colorOut; //out going\n"
"\n"
"out vec2 texUV_out;\n"
"\n"
"\n"
"\n"
"void main() {\n"
"\n"
"    \n"
"\n"
"    gl_Position = projection * V * M * vec4(vertex, 1);\n"
"\n"
"    colorOut = color;\n"
"\n"
"    \n"
"\n"
"    int xAt = int(texUV.x*2);\n"
"\n"
"    int yAt = int(texUV.y*2) + 1;\n"
"\n"
"    texUV_out = vec2(uvAtlas[xAt], uvAtlas[yAt]);\n"
"\n"
"}";

static char *vertex_skybox_shader = "in vec3 vertex;\n"
"\n"
"\n"
"\n"
"out vec3 TexCoords;\n"
"\n"
"\n"
"\n"
"uniform mat4 projection;\n"
"\n"
"uniform mat4 view;\n"
"\n"
"\n"
"\n"
"void main()\n"
"\n"
"{\n"
"\n"
"    TexCoords = vertex;\n"
"\n"
"    vec4 pos = projection * view * vec4(vertex, 1.0);\n"
"\n"
"    gl_Position = pos.xyww;\n"
"\n"
"}  ";

static char *vertex_sky_quad_shader = "in vec3 vertex;\n"
"\n"
"in vec3 normal;\n"
"\n"
"in vec2 texUV;	\n"
"\n"
"\n"
"\n"
"uniform float fov;\n"
"\n"
"uniform float aspectRatio;\n"
"\n"
"uniform mat4 camera_to_world_transform;\n"
"\n"
"uniform float nearZ;\n"
"\n"
"\n"
"\n"
"//outgoing variables\n"
"\n"
"out vec3 direction_frag; \n"
"\n"
"\n"
"\n"
"void main() {\n"
"\n"
"    \n"
"\n"
"    gl_Position = vec4(vertex.x, vertex.y, 1.0f, 1);\n"
"\n"
"    float t = tan(fov/2)*nearZ;\n"
"\n"
"	float r = t*aspectRatio;\n"
"\n"
"	vec3 ray = vec3(mix(-r, r, texUV.x), mix(-t, t, texUV.y), nearZ);\n"
"\n"
"\n"
"\n"
"    direction_frag = mat3(camera_to_world_transform) * ray;\n"
"\n"
"}";

static char *vertex_terrain_shader = "in vec3 vertex;\n"
"\n"
"in vec3 normal;\n"
"\n"
"in vec2 texUV;	\n"
"\n"
"\n"
"\n"
"out vec2 uv_frag;\n"
"\n"
"\n"
"\n"
"uniform mat4 projection;\n"
"\n"
"// uniform mat4 view;\n"
"\n"
"// uniform mat4 model;\n"
"\n"
"\n"
"\n"
"in mat4 M;\n"
"\n"
"in mat4 V;\n"
"\n"
"\n"
"\n"
"in vec4 color;\n"
"\n"
"in vec4 uvAtlas;	//not using a atlas\n"
"\n"
"\n"
"\n"
"void main()\n"
"\n"
"{\n"
"\n"
"    uv_frag = texUV;\n"
"\n"
"    gl_Position = projection * V * M * vec4(vertex, 1.0);\n"
"\n"
"}  ";

