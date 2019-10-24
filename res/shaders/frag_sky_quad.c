in vec3 direction_frag; 
out vec4 color;
out vec4 BrightColor;

uniform vec4 skyColor;
uniform vec3 eyePos;

uniform float tScale;

vec3 windDir;
uniform float timeScale;

uniform vec3 sunDirection;

#define NOISE_SCALE 0.1f
#define NOISE_THRESHOLD 0.6f
#define TIME_SCALE_MULTIPLIER 0.1f
#define NUMBER_OF_SAMPLES 10

float hash(float n) { return fract(sin(n) * 1e4); }

float noise3d(vec3 x) {
	const vec3 step = vec3(110, 241, 171);

	vec3 i = floor(x);
	vec3 f = fract(x);
 
	// For performance, compute the base input to a 1D hash from the integer part of the argument and the 
	// incremental change to the 1D based on the 3D -> 1D wrapping
    float n = dot(i, step);

	vec3 u = f * f * (3.0 - 2.0 * f);
	return mix(mix(mix( hash(n + dot(step, vec3(0, 0, 0))), hash(n + dot(step, vec3(1, 0, 0))), u.x),
                   mix( hash(n + dot(step, vec3(0, 1, 0))), hash(n + dot(step, vec3(1, 1, 0))), u.x), u.y),
               mix(mix( hash(n + dot(step, vec3(0, 0, 1))), hash(n + dot(step, vec3(1, 0, 1))), u.x),
                   mix( hash(n + dot(step, vec3(0, 1, 1))), hash(n + dot(step, vec3(1, 1, 1))), u.x), u.y), u.z);
}

#define OCTAVES 6
float fbm (vec3 st) {
    // Initial values
    float value = 0.0;
    float amplitude = .5;
    float frequency = 0.;
    //
    // Loop of octaves
    for (int i = 0; i < OCTAVES; i++) {
        value += amplitude * noise3d(st);
        st *= 2.;
        amplitude *= .5;
    }
    return value;
}

float getClosestT(float radius, vec3 startP, vec3 dir) {
	float t0 = 0;
	float t1 = 0;
	float radius2 = radius*radius;

	vec3 L = vec3(0.0f) - startP; 
    float tca = dot(L, dir); 
    // if (tca < 0) return false;
    float d2 = dot(L, L) - tca * tca; 
    if (d2 > radius2) {
    	t0 = -1;
    } else {
    	float thc = sqrt(radius2 - d2); 
    	t0 = tca - thc; 
    	t1 = tca + thc;

    	if(t0 > t1) {
    		float temp = t0;
    		t0 = t1;
    		t1 = temp;
    	} 

    	if(t0 < 0) {
    		t0 = t1;
    	} 

    	if(t0 < 0) {
    		t0 = -1;
    	}

    	
    }

    return t0;
    
}


void main (void) {
    vec3 direction_frag_norm = normalize(direction_frag);

    vec4 tempColor = skyColor;
    float tVal = dot(-normalize(sunDirection), direction_frag_norm);
    if(tVal > 0.9999f) {
    	tempColor = vec4(10000, 10000, 10000, 1);//mix(tempColor, vec4(1, 1, 0, 1), smoothstep(0, 1, tVal));

    }
    vec4 colorOfSky = mix(tempColor, vec4(1, 1, 1, 1), pow(1.0f - max(direction_frag_norm.y, 0.0f), 3));

   	windDir = normalize(vec3(1, -0.01, 0.2));
    float cloudContrubtion = 0;

    float lowerY = 150;
    float upperY = 151;

    float t0 = getClosestT(lowerY, eyePos, direction_frag_norm);
    float t1 = getClosestT(upperY, eyePos, direction_frag_norm);

    if(t0 >= 0.0f || t1 >= 0.0f) { //parrellel
    	if(t0 > t1) {
    		float temp = t0;
    		t0 = t1;
    		t1 = temp;
    	} 

    	vec3 startP = eyePos;
    	
    	if(t0 > 0.0f) { //in front of ray
    		startP = eyePos + t0*direction_frag_norm;
    	}

    	
    	if(t1 > 0.0f) {
    		vec3 endP = eyePos + t1*direction_frag_norm;

    		float deltaT = length(endP - startP) / NUMBER_OF_SAMPLES; //TODO: Can we use the dot product instead
    		
    		for(int sIndex = 0; sIndex < NUMBER_OF_SAMPLES; ++sIndex) {
    			vec3 sampleP = startP + (sIndex*deltaT)*direction_frag_norm;
    			float val = fbm(NOISE_SCALE*sampleP + TIME_SCALE_MULTIPLIER*windDir*timeScale);
    			if(val > NOISE_THRESHOLD) {
    				cloudContrubtion += 0.7f*deltaT*val;
    			}
    			
    		}
    	} else {
    		//not looking at the clouds
    		cloudContrubtion = 0;
    	}
    }

    cloudContrubtion = cloudContrubtion; //nomralize the value
    float blendFactor = 1.0f - exp(-cloudContrubtion);
    
    vec3 cloudColor = vec3(1.5) + cloudContrubtion;
    color = mix(colorOfSky, vec4(cloudColor, 1.0f), blendFactor);

    float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
        BrightColor = vec4(color.rgb, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}