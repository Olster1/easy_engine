out vec4 FragColor;
in vec2 texUV_out;
in vec4 colorOut;

uniform float value;

vec4 hsvToRgb(float hueF, float saturation, float value) {  //h 0 - 360
		float chroma = value * saturation;
	    float hue1 = (hueF / 60);
	    float modValue = mod(hue1, 2.0f);
	    float x = chroma * (1- abs(modValue - 1));
	    vec4 result = vec4(1.0f);
	    if (hue1 >= 0 && hue1 <= 1) {
	    	result.x = chroma;
	    	result.y = x;
	    	result.z = 0;
	    } else if (hue1 >= 1 && hue1 <= 2) {
	    	result.x = x;
	    	result.y = chroma;
	    	result.z = 0;
	    } else if (hue1 >= 2 && hue1 <= 3) {
	    	result.x = 0;
	    	result.y = chroma;
	    	result.z = x;
	    } else if (hue1 >= 3 && hue1 <= 4) {
	    	result.x = 0;
	    	result.y = x;
	    	result.z = chroma;
	    } else if (hue1 >= 4 && hue1 <= 5) {
	    	result.x = x;
	    	result.y = 0;
	    	result.z = chroma;
	    } else if (hue1 >= 5 && hue1 <= 6) {
	    	result.x = chroma;
	    	result.y = 0;
	    	result.z = x;
	    }
	    
	    float m = value - chroma;
    	result.x += m;	
    	result.y += m;	
    	result.z += m;	

    	return result;
}

vec2 toOrigin(vec2 a) {
	vec2 b = 2*a - vec2(1, 1);
	return b;
} 

#define M_PI 3.1415926535897932384626433832795

void main (void) {

	float inversePI = 1.0f / M_PI;
	vec2 xFlip = vec2(1.0f - texUV_out.x, 1.0f - texUV_out.y);
	vec2 localToOrigin = toOrigin(xFlip);
	float hue = 0.0f;
	if(localToOrigin.x != 0.0f) {
		hue = atan(localToOrigin.y, localToOrigin.x);
		hue = inversePI*hue*180.0f; //convert to -180 to 180 

		hue += 180.0f; //between 0 - 360 

		// hue /= 360.0f; //between 0 - 1
	}

	float saturation = length(localToOrigin);

	vec4 color = hsvToRgb(hue, saturation, value);

	float alphaVal = 1.0f;
	if(saturation > 1.0f) {
		// alphaVal = 0.0f;	
		alphaVal = mix(1.0f, 0.0f, (saturation - 1.0f) / 0.01f);
	} 

	// alphaVal = max(fwidth(alphaVal), alphaVal);
	
	FragColor = vec4(color.r, color.g, color.b, alphaVal);
}