static inline V4 easyColor_hsvToRgb(float hueF, float saturation, float value) { //h = 0 - 360 
	float chroma = value * saturation;
    float hue1 = (hueF / 60);
    float modValue = fmod(hue1, 2.0f);
    float x = chroma * (1- absVal(modValue - 1));
    V4 result = v4(1.0f, 1.0f, 1.0f, 1.0f);
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

typedef struct {
    float h;
    float s;
    float v;
} EasyColor_HSV;

static inline EasyColor_HSV easyColor_rgbToHsv(float r, float g, float b) { //rgb = 0 - 1 
    EasyColor_HSV out = { 0, 0, 0 };
    double      min, max, delta;

    min = r < g ? r : g;
    min = min  < b ? min  : b;

    max = r > g ? r : g;
    max = max  > b ? max  : b;

    out.v = max;                                // v
    delta = max - min;
    if (delta < 0.00001)
    {
        out.s = 0;
        out.h = 0; // undefined, maybe nan?
        return out;
    }
    if( max > 0.0 ) { // NOTE: if Max is == 0, this divide would cause a crash
        out.s = (delta / max);                  // s
    } else {
        // if max is 0, then r = g = b = 0              
        // s = 0, h is undefined
        out.s = 0.0;
        out.h = NAN;                            // its now undefined
        return out;
    }
    if( r >= max )                           // > is bogus, just keeps compilor happy
        out.h = ( g - b ) / delta;        // between yellow & magenta
    else
    if( g >= max )
        out.h = 2.0 + ( b - r ) / delta;  // between cyan & yellow
    else
        out.h = 4.0 + ( r - g ) / delta;  // between magenta & cyan

    out.h *= 60.0;                              // degrees

    if( out.h < 0.0 )
        out.h += 360.0;

    return out;
}