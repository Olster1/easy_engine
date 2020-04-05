#if !defined EASY_MATH_H
/*
New File
*/

#define sqr(a) (a*a)
#define cube(a) (a*a*a)

float roundToHalf(float value) {
    float result = (int)(2*value);
    result /= 2;
    return result;
}
float safeRatio0(float a, float b) {
    float result = 0;
    if(b != 0) {
        result = a / b;
    }
    return result;
}

float signOf(float a) {
    float result = 1;
    if(a < 0) {
        result = -1;
    }
    return result;
    
}

float absVal(float a) {
    float result = a;
    if(a < 0) {
        result = -a;
    }
    return result;
    
}


float lerp(float a, float t, float b) {
    float value = a + t*(b - a);
    return value;
}


float easyMath_degreesToRadians(float degrees) {
    return ((TAU32 * degrees) / 360.0f);
}

float easyMath_radiansToDegrees(float radians) {
    return ((360.0f * radians) / TAU32);
}

bool floatEqual_withError(float a, float b) {
    float error = 0.00001;
    bool result = (a >= (b - error) && a < (b + error));
    return result;
}

inline float ATan2_0toTau(float Y, float X) {
    float Result = (float)atan2(Y, X);
    if(Result < 0) {
        Result += TAU32; // is in the bottom range ie. 180->360. -PI32 being PI32. So we can flip it up by adding TAU32
    }
    
    assert(Result >= 0 && Result <= (TAU32 + 0.00001));
    return Result;
}

typedef union {
    struct {
        float x, y;
    };
    struct {
        float E[2];
    };
} V2;

typedef union {
    struct {
        float E[3];
    };
    struct {
        float x, y, z;
    };
    struct {
        V2 xy;
        float _ignore1;
    };
} V3;

typedef union {
    struct {
        float E[4];
    };
    struct {
        float x, y, z, w;
        
    }; 
    struct {
        V2 xy;
        float _ignore1;
        float _ignore2;
    };
    struct {
        V3 xyz;
        float _ignore3;
    };
} V4;

V2 v2(float x, float y) {
    V2 result = {};
    result.x = x;
    result.y = y;
    return result;
}

bool v2Equal(V2 a, V2 b) {
    bool result = (a.x == b.x && a.y == b.y);
    return result;
}

bool v2Equal_withError(V2 a, V2 b, float error) {
    bool result = (a.x >= (b.x - error) && a.x < (b.x + error) &&
                   a.y >= (b.y - error) && a.y < (b.y + error));
    return result;
}

V4 v4(float x, float y, float z, float w) {
    V4 result = {};
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    return result;
}

V3 v3(float x, float y, float z) {
    V3 result = {};
    result.x = x;
    result.y = y;
    result.z = z;
    
    return result;
}

bool v4Equal(V4 a, V4 b) {
    bool result = (a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w);
    return result;
}

V4 v3ToV4Homogenous(V3 a) {
    V4 result = v4(a.x, a.y, a.z, 1);
    return result;
}

V3 v2ToV3(V2 a, float z) {
    V3 result = v3(a.x, a.y, z);
    return result;
}

V4 v3ToV4(V3 a, float w) {
    V4 result = v4(a.x, a.y, a.z, w);
    return result;
}

V2 v2_negate(V2 a) {
    V2 result = v2(-a.x, -a.y);
    return result;
}

V2 v2_floor(V2 a) {
    V2 result = {(float)floor(a.x), (float)floor(a.y)};
    return result;
}


V2 v2_ceil(V2 a) {
    V2 result = {(float)ceil(a.x), (float)ceil(a.y)};
    return result;
}

V2 v2_minus(V2 a, V2 b) {
    V2 result = v2(a.x - b.x, a.y - b.y);
    return result;
}

V2 v2_plus(V2 a, V2 b) {
    V2 result = {a.x + b.x, a.y + b.y};
    return result;
}

V2 v2_scale(float a, V2 b) {
    V2 result = v2(a*b.x, a*b.y);
    return result;
}

V2 v2_hadamard(V2 a, V2 b) {
    V2 result = v2(a.x*b.x, a.y*b.y);
    return result;
}

V2 v2_inverseHadamard(V2 a, V2 b) {
    //Change to safe ratio
    V2 result = v2(safeRatio0(a.x, b.x), safeRatio0(a.y, b.y));
    return result;
}

float getLength(V2 a) {
    float result = sqrt(sqr(a.x) + sqr(a.y));
    return result;
}

float getLengthV3(V3 a) {
    float result = sqrt(sqr(a.x) + sqr(a.y) + sqr(a.z));
    return result;
}

static inline float Beizer(float p0, float p1, float p2, float p3, float t) {
    float result = cube((1 - t))*p0 + cube(t)*p3 + 3*sqr(t)*(1 - t)*p2 + 3*sqr((1 - t))*t*p1;
    return result;
}

float dotV2(V2 a, V2 b) {
    float result = a.x*b.x + a.y*b.y;
    return result;
}

//anti-clockwise
V2 perp(V2 a) {
    V2 result = v2(-a.y, a.x);
    return result;
}

//clockwise
static inline float cross2D(V2 a, V2 b)
{
    return a.x * b.y - a.y * b.x;
}

float getLengthSqr(V2 a) {
    float result = dotV2(a, a);
    return result;
} 

V2 normalize_(V2 a, float len) {
    V2 result = v2(safeRatio0(a.x, len), safeRatio0(a.y, len));
    return result;
}

V2 normalizeV2(V2 a) {
    float len = getLength(a);
    V2 result = v2(safeRatio0(a.x, len), safeRatio0(a.y, len));
    return result;
}
V3 v3_minus(V3 a, V3 b) {
    V3 result = v3(a.x - b.x, a.y - b.y, a.z - b.z);
    return result;
}

V3 v3_plus(V3 a, V3 b) {
    V3 result = v3(a.x + b.x, a.y + b.y, a.z + b.z);
    return result;
}

V3 v3_scale(float a, V3 b) {
    V3 result = v3(a*b.x, a*b.y, a*b.z);
    return result;
}

V3 v3_negate(V3 b) {
    V3 result = v3(-b.x, -b.y, -b.z);
    return result;
}

V3 v3_crossProduct(V3 a, V3 b) {
    V3 c = v3(0, 0, 0);
    
    c.x = (a.y*b.z) - (a.z*b.y);
    c.y = (a.z*b.x) - (a.x*b.z);
    c.z = (a.x*b.y) - (a.y*b.x);
    
    return c;
}

float dotV3(V3 a, V3 b) {
    float result = a.x*b.x + a.y*b.y + a.z*b.z;
    return result;
}

V3 v3_hadamard(V3 a, V3 b) {
    V3 result = v3(a.x*b.x, a.y*b.y, a.z*b.z);
    return result;
}

V4 v4_minus(V4 a, V4 b) {
    V4 result = {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
    return result;
}

V4 v4_plus(V4 a, V4 b) {
    V4 result = {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
    return result;
}

V4 v4_scale(float a, V4 b) {
    V4 result = v4(a*b.x, a*b.y, a*b.z, a*b.w);
    return result;
}

V4 v4_hadamard(V4 a, V4 b) {
    V4 result = v4(a.x*b.x, a.y*b.y, a.z*b.z, a.w*b.w);
    return result;
}

V4 v2ToV4Homogenous(V2 a) {
    V4 result = v4(a.x, a.y, 0, 1);
    return result;
}

V3 normalizeV3(V3 a) {
    float len = getLengthV3(a);
    V3 result = v3(safeRatio0(a.x, len), safeRatio0(a.y, len), safeRatio0(a.z, len));
    return result;
}

V3 normalize_V3(V3 a, float len) {
    V3 result = v3(safeRatio0(a.x, len), safeRatio0(a.y, len), safeRatio0(a.z, len));
    return result;
}

typedef union {
    struct {
        float minX;
        float minY;
        float minZ;
        float maxX;
        float maxY;
        float maxZ;
    };
    struct {
        V3 min;
        V3 max;
    };
} Rect3f;

Rect3f rect3f(float minX, 
              float minY,
              float minZ, 
              float maxX, 
              float maxY,
              float maxZ) {
    
    Rect3f result = {};
    
    result.minX = minX;
    result.maxX = maxX;
    result.minY = minY;
    result.maxY = maxY;
    result.minZ = minZ;
    result.maxZ = maxZ;
    
    return result;
    
}

Rect3f rect3fNull() {
    Rect3f result = {};
    return result;
}



#define INFINITY_VALUE 1000000
    


Rect3f InverseInfinityRect3f() {
    Rect3f result = {};
    
    result.minX = INFINITY_VALUE;
    result.maxX = -INFINITY_VALUE;
    result.minY = INFINITY_VALUE;
    result.maxY = -INFINITY_VALUE;
    result.minZ = INFINITY_VALUE;
    result.maxZ = -INFINITY_VALUE;
    
    return result;
}

static inline Rect3f v3_to_rect3f(V3 a) {
    Rect3f result = rect3f(a.x, a.y, a.z, a.x, a.y, a.z);
    return result;
}

V3 getDimRect3f(Rect3f b) {
    V3 res = v3(b.maxX - b.minX, b.maxY - b.minY, b.maxZ - b.minZ);
    return res;
}

static inline float easyMath_getLargestAxis(Rect3f b) {
    V3 dim = getDimRect3f(b);

    float result = max(dim.z, max(dim.x, dim.y));

    return result;
}

V3 easyMath_getRect3fBoundsOffset(Rect3f b) { //plusing this with where you want the rect3f to go, will center the rect3f
    V3 negativeHalfBounds = v3_scale(-0.5f, getDimRect3f(b));
    V3 res = v3_minus(negativeHalfBounds, b.min);
    return res;
}

V3 getCenterRect3f(Rect3f b) {
    V3 res = v3(b.minX + 0.5f*(b.maxX - b.minX), b.minY + 0.5f*(b.maxY - b.minY), b.minZ + 0.5f*(b.maxZ - b.minZ));
    return res;
}

Rect3f unionRect3f(Rect3f a, Rect3f b) {
    Rect3f result = {};
    
    result.minX = (a.minX > b.minX) ? b.minX : a.minX;
    result.minY = (a.minY > b.minY) ? b.minY : a.minY;
    result.minZ = (a.minZ > b.minZ) ? b.minZ : a.minZ;

    result.maxX = (a.maxX < b.maxX) ? b.maxX : a.maxX;
    result.maxY = (a.maxY < b.maxY) ? b.maxY : a.maxY;
    result.maxZ = (a.maxZ < b.maxZ) ? b.maxZ : a.maxZ;

    
    return result;
    
}

Rect3f rect3fMinDim(float minX, 
                    float minY,
                    float minZ, 
                    float dimX, 
                    float dimY,
                    float dimZ) {
    
    Rect3f result = {};
    
    result.minX = minX;
    result.maxX = minX + dimX;
    result.minY = minY;
    result.maxY = minY + dimY;
    result.minZ = minZ;
    result.maxZ = minZ + dimZ;
    
    return result;
    
}

Rect3f rect3fMinMax(float minX, 
                    float minY, 
                    float minZ,
                    float maxX, 
                    float maxY, 
                    float maxZ) {
    
    Rect3f result = {};
    
    result.minX = minX;
    result.maxX = maxX;
    result.minY = minY;
    result.maxY = maxY;
    result.minZ = minZ;
    result.maxZ = maxZ;
    
    return result;
    
}

Rect3f rect3fCenterDim(float centerX, 
                       float centerY, 
                       float centerZ,
                       float dimX, 
                       float dimY,
                       float dimZ) {
    
    Rect3f result = {};
    
    float halfDimX = 0.5f*dimX;
    float halfDimY = 0.5f*dimY;
    float halfDimZ = 0.5f*dimZ;
    
    result.minX = centerX - halfDimX;
    result.minY = centerY - halfDimY;
    result.minZ = centerZ - halfDimZ;
    result.maxX = centerX + halfDimX;
    result.maxY = centerY + halfDimY;
    result.maxZ = centerZ + halfDimZ;
    
    return result;
    
}

Rect3f rect3fCenterDimV3(V3 pos, V3 dim) {
    
    Rect3f result = rect3fCenterDim(pos.x, pos.y, pos.z, dim.x, dim.y, dim.z);
    return result;
    
}

bool inBoundsV3(V3 p, Rect3f rect) {
    bool result = (p.x >= rect.minX &&
                   p.y >= rect.minY &&
                   p.z >= rect.minZ &&
                   p.x < rect.maxX &&
                   p.y < rect.maxY &&
                   p.z < rect.maxZ);
    return result;
}

typedef union {
    struct {
        float E[4];
    };
    struct {
        float minX;
        float minY;
        float maxX;
        float maxY;
    };
    struct {
        V2 min;
        V2 max;
    };
} Rect2f;

Rect2f rect2f(float minX, 
              float minY, 
              float maxX, 
              float maxY) {
    
    Rect2f result = {};
    
    result.minX = minX;
    result.maxX = maxX;
    result.minY = minY;
    result.maxY = maxY;
    
    return result;
    
}

    
Rect2f InfinityRect2f() {
    Rect2f result = {};
    
    result.minX = 0;
    result.maxX = INFINITY_VALUE;
    result.minY = 0;
    result.maxY = INFINITY_VALUE;
    
    return result;
}

Rect2f rect2fNull() {
    Rect2f result = {};
    return result;
}

Rect2f rect2fMinDim(float minX, 
                    float minY, 
                    float dimX, 
                    float dimY) {
    
    Rect2f result = {};
    
    result.minX = minX;
    result.maxX = minX + dimX;
    result.minY = minY;
    result.maxY = minY + dimY;
    
    return result;
    
}

Rect2f rect2fMinMax(float minX, 
                    float minY, 
                    float maxX, 
                    float maxY) {
    
    Rect2f result = {};
    
    result.minX = minX;
    result.maxX = maxX;
    result.minY = minY;
    result.maxY = maxY;
    
    return result;
    
}

Rect2f rect2fCenterDim(float centerX, 
                       float centerY, 
                       float dimX, 
                       float dimY) {
    
    Rect2f result = {};
    
    float halfDimX = 0.5f*dimX;
    float halfDimY = 0.5f*dimY;
    
    result.minX = centerX - halfDimX;
    result.minY = centerY - halfDimY;
    result.maxX = centerX + halfDimX;
    result.maxY = centerY + halfDimY;
    
    return result;
    
}

Rect2f rect2fCenterDimV2(V2 center,  V2 dim) {
    Rect2f result = rect2fCenterDim(center.x, 
                                    center.y, 
                                    dim.x, 
                                    dim.y);
    return result;
}


Rect2f rect2fMinDimV2(V2 a, V2 b) {
    
    Rect2f result = rect2fMinDim(a.x, a.y, b.x, b.y);
    
    return result;
}

Rect2f reevalRect2f(Rect2f rect) {
    Rect2f result = {};
    result.min = v2(min(rect.minX, rect.maxX), min(rect.maxY, rect.minY));
    result.max = v2(max(rect.maxX, rect.minX), max(rect.maxY, rect.minY));
    return result;
}


Rect2f InverseInfinityRect2f() {
    Rect2f result = {};
    
    result.minX = INFINITY_VALUE;
    result.maxX = -INFINITY_VALUE;
    result.minY = INFINITY_VALUE;
    result.maxY = -INFINITY_VALUE;
    
    return result;
}

Rect2f unionRect2f(Rect2f a, Rect2f b) {
    Rect2f result = {};
    
    result.minX = (a.minX > b.minX) ? b.minX : a.minX;
    result.minY = (a.minY > b.minY) ? b.minY : a.minY;
    result.maxX = (a.maxX < b.maxX) ? b.maxX : a.maxX;
    result.maxY = (a.maxY < b.maxY) ? b.maxY : a.maxY;
    
    return result;
    
}

bool inRect(V2 p, Rect2f rect) {
    bool result = (p.x >= rect.minX &&
                   p.y >= rect.minY &&
                   p.x < rect.maxX &&
                   p.y < rect.maxY);
    
    return result;
}

V2 getDim(Rect2f b) {
    V2 res = v2(b.maxX - b.minX, b.maxY - b.minY);
    return res;
}

V2 getCenter(Rect2f b) {
    V2 res = v2(b.minX + 0.5f*(b.maxX - b.minX), b.minY + 0.5f*(b.maxY - b.minY));
    return res;
}

//matrix operations
//matrix operations
typedef union {
    struct {
        float E[2][2];
    };
    struct {
        V2 a;
        V2 b;
    };
} Matrix2;

Matrix2 mat2() {
    Matrix2 result = {{
            1, 0,
            0, 1
        }};
    return result;
}

V2 mat2_project(Matrix2 a, V2 b) {
    V2 result = v2_plus(v2_scale(b.x, a.a),  v2_scale(b.y, a.b));
    return result;
}

typedef union {
    struct {
        float E_[16];
    };
    struct {
        float E[4][4];
    };
    struct {
        V4 a;
        V4 b;
        V4 c;
        V4 d;
    };
    struct {
        float val[16];
    };
} Matrix4;

Matrix4 mat4() {
    Matrix4 result = {{
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        }};
    return result;
}

typedef union {
    union {
        struct {
            float E[4];
        };
        struct {
            float r, i, j, k;
        };
    };
} Quaternion;

Quaternion identityQuaternion() {
    Quaternion result = {};
    result.r = 1;
    
    return result;
}

V3 easyMath_getZAxis(Matrix4 m) {
    return m.c.xyz;
}

V3 easyMath_getXAxis(Matrix4 m) {
    return m.a.xyz;
}

V3 easyMath_getYAxis(Matrix4 m) {
    return m.b.xyz;
}

Matrix4 mat4_transpose(Matrix4 val) {
    Matrix4 result = mat4();
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
            result.E[j][i] = val.E[i][j];
        }
    }
    return result;
}

Matrix4 quaternionToMatrix(Quaternion q) {
    Matrix4 result = mat4();
    
    result.E_[0] = 1 - (2*q.j*q.j + 2*q.k*q.k);
    result.E_[4] = 2*q.i*q.j + 2*q.k*q.r;
    result.E_[8] = 2*q.i*q.k - 2*q.j*q.r;
    
    result.E_[1] = 2*q.i*q.j - 2*q.k*q.r;
    result.E_[5] = 1 - (2*q.i*q.i  + 2*q.k*q.k);
    result.E_[9] = 2*q.j*q.k + 2*q.i*q.r;
    
    result.E_[2] = 2*q.i*q.k + 2*q.j*q.r;
    result.E_[6] = 2*q.j*q.k - 2*q.i*q.r;
    result.E_[10] = 1 - (2*q.i*q.i  + 2*q.j*q.j);

    //This is because we're using column notation instead of row notation
    //TODO @speed: take this out and swap the values manually
    result = mat4_transpose(result);
    //
    
    return result;
    
}

static Quaternion easyMath_matrix4ToQuaternion(Matrix4 m) { //only use 3x3 values (the rotation part)
    Quaternion q = identityQuaternion();

    float fourWSquaredMinus1 = m.E[0][0] + m.E[1][1] + m.E[2][2];
    float fourXSquaredMinus1 = m.E[0][0] - m.E[1][1] - m.E[2][2];
    float fourYSquaredMinus1 = m.E[1][1] - m.E[0][0] - m.E[2][2];
    float fourZSquaredMinus1 = m.E[2][2] - m.E[0][0] - m.E[1][1];

    int biggestIndex = 0;
    float fourBiggestSquaredMinus1 = fourWSquaredMinus1;
    if(fourXSquaredMinus1 > fourBiggestSquaredMinus1) {
        fourBiggestSquaredMinus1 = fourXSquaredMinus1;
        biggestIndex = 1;
    }
    if(fourYSquaredMinus1 > fourBiggestSquaredMinus1) {
        fourBiggestSquaredMinus1 = fourYSquaredMinus1;
        biggestIndex = 2;
    }
    if(fourZSquaredMinus1 > fourBiggestSquaredMinus1) {
        fourBiggestSquaredMinus1 = fourZSquaredMinus1;
        biggestIndex = 3;
    }

    float biggestVal = sqrt(fourBiggestSquaredMinus1 + 1.0f) * 0.5f;
    float mult = 0.25f / (float)biggestVal;

    switch(biggestIndex) {
        case 0: {
            q.r = biggestVal;
            q.i = (m.E[1][2] - m.E[2][1]) * mult;
            q.j = (m.E[2][0] - m.E[0][2]) * mult;
            q.k = (m.E[0][1] - m.E[1][0]) * mult;
        } break;
        case 1: {
            q.i = biggestVal;
            q.r = (m.E[1][2] - m.E[2][1]) * mult;
            q.j = (m.E[0][1] + m.E[1][0]) * mult;
            q.k = (m.E[2][0] + m.E[0][2]) * mult;
        } break;
        case 2: {
            q.j = biggestVal;
            q.r = (m.E[2][0] - m.E[0][2]) * mult;
            q.i = (m.E[0][1] + m.E[1][0]) * mult;
            q.k = (m.E[1][2] + m.E[2][1]) * mult;
        } break;
        case 3: {
            q.k = biggestVal;
            q.r = (m.E[0][1] - m.E[1][0]) * mult;
            q.i = (m.E[2][0] + m.E[0][2]) * mult;
            q.j = (m.E[1][2] + m.E[2][1]) * mult;
        } break;
    }

    return q;

}

Quaternion quaternion(float r, float i, float j, float k) {
    Quaternion result = {};
    result.r = r;
    result.i = i;
    result.j = j;
    result.k = k;
    
    return result;   
}

//the arguments are in order of math operation ie. q1*q2 -> have q2 rotation and rotating by q1
Quaternion quaternion_mult(Quaternion q, Quaternion q2) {
    Quaternion result = {};
    
    result.r = q.r*q2.r - q.i*q2.i -
        q.j*q2.j - q.k*q2.k;
    result.i = q.r*q2.i + q.i*q2.r +
        q.j*q2.k - q.k*q2.j;
    result.j = q.r*q2.j + q.j*q2.r +
        q.k*q2.i - q.i*q2.k;
    result.k = q.r*q2.k + q.k*q2.r +
        q.i*q2.j - q.j*q2.i;
    
    return result;
}

//this is for 'intergrating' a quaternion, like updating the rotation per frame in physics
Quaternion addScaledVectorToQuaternion(Quaternion q_, V3 vector, float timeScale) {
    Quaternion result = q_;
    Quaternion q = quaternion(0,
                              vector.x * timeScale,
                              vector.y * timeScale,
                              vector.z * timeScale);

    q = quaternion_mult(q_, q);
    result.r += q.r * 0.5f;
    result.i += q.i * 0.5f;
    result.j += q.j * 0.5f;
    result.k += q.k * 0.5f;
    
    return result;
}

Quaternion easyMath_normalizeQuaternion(Quaternion q) {
    float n = sqrt(q.i*q.i + q.j*q.j + q.k*q.k + q.r*q.r);
    
    q.i /= n;
    q.j /= n;
    q.k /= n;
    q.r /= n;

    return q;
}

Quaternion eulerAnglesToQuaternion(float y, float x, float z) { //in radians
    Quaternion result = {};
    float h = y / 2;
    float p = x / 2;
    float b = z / 2;
    
    result.r = cos(h)*cos(p)*cos(b) + sin(h)*sin(p)*sin(b);
    result.i = cos(h)*sin(p)*cos(b) + sin(h)*cos(p)*sin(b);
    result.j = sin(h)*cos(p)*cos(b) - cos(h)*sin(p)*sin(b);
    result.k = cos(h)*cos(p)*sin(b) - sin(h)*sin(p)*cos(b);
    
    return result;
}

static V3 easyMath_QuaternionToEulerAngles(Quaternion q) { 
    V3 result = {};

    float sp = -2.0f * (q.j*q.k - q.r*q.i);

    //check for gimbal lock
    if(absVal(sp) > 0.9999f) {
        //looking straight up or down
        result.x = 0.5f*PI32 * sp; //pitch

        result.y = atan2(-q.i*q.k + q.r*q.j, 0.5f - q.j*q.j - q.k*q.k);
        result.z = 0; //no bank
    } else {
        result.x = asin(sp);
        result.y = atan2(q.i*q.k + q.r*q.j, 0.5f - q.i*q.i - q.j*q.j);
        result.z = atan2(q.i*q.j + q.r*q.k, 0.5f - q.i*q.i - q.k*q.k);
    }
    
    return result;
}

Quaternion easyMath_lerpQuaternion(Quaternion a, float t, Quaternion b) {
    Quaternion result = {};
    
    result.r = lerp(a.r, t, b.r);
    result.i = lerp(a.i, t, b.i);
    result.j = lerp(a.j, t, b.j);
    result.k = lerp(a.k, t, b.k);

    return result;
}


Matrix4 mat4_setOrientationAndPos(Quaternion q, V3 pos) {
    Matrix4 result = quaternionToMatrix(q);
    
    result.E_[12] = pos.x;
    result.E_[13] = pos.y;
    result.E_[14] = pos.z;
    
    return result;
}

Matrix4 mat4_noTranslate(Matrix4 m) {
    m.E_[12] = 0;
    m.E_[13] = 0;
    m.E_[14] = 0;
    
    return m;
}


Matrix4 mat4_angle_aroundZ(float angle) {
    Matrix4 result = {{
            (float)cos(angle), (float)sin(angle), 0, 0,
            (float)cos(angle + HALF_PI32), (float)sin(angle + HALF_PI32), 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        }};
    return result;
}

Matrix4 mat4_xyAxis(V2 xAxis, V2 yAxis) {
    Matrix4 result = {{
            xAxis.x, xAxis.y, 0, 0,
            yAxis.x, yAxis.y, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        }};
    return result;
}

Matrix4 mat4_xyzAxis(V3 xAxis, V3 yAxis, V3 zAxis) {
    Matrix4 result = {{
            xAxis.x, xAxis.y, xAxis.z, 0,
            yAxis.x, yAxis.y, yAxis.z, 0,
            zAxis.x, zAxis.y, zAxis.z, 0,
            0, 0, 0, 1
        }};
    return result;
}

Matrix4 mat4_xyzwAxis(V3 xAxis, V3 yAxis, V3 zAxis, V3 wAxis) {
    Matrix4 result = {{
            xAxis.x, xAxis.y, xAxis.z, 0,
            yAxis.x, yAxis.y, yAxis.z, 0,
            zAxis.x, zAxis.y, zAxis.z, 0,
            wAxis.x, wAxis.y, wAxis.z, 1
        }};
    return result;
}

bool mat4_inverse(float m[16], float invOut[16]) {
        double inv[16], det;
        int i;

        inv[0] = m[5]  * m[10] * m[15] - 
                 m[5]  * m[11] * m[14] - 
                 m[9]  * m[6]  * m[15] + 
                 m[9]  * m[7]  * m[14] +
                 m[13] * m[6]  * m[11] - 
                 m[13] * m[7]  * m[10];

        inv[4] = -m[4]  * m[10] * m[15] + 
                  m[4]  * m[11] * m[14] + 
                  m[8]  * m[6]  * m[15] - 
                  m[8]  * m[7]  * m[14] - 
                  m[12] * m[6]  * m[11] + 
                  m[12] * m[7]  * m[10];

        inv[8] = m[4]  * m[9] * m[15] - 
                 m[4]  * m[11] * m[13] - 
                 m[8]  * m[5] * m[15] + 
                 m[8]  * m[7] * m[13] + 
                 m[12] * m[5] * m[11] - 
                 m[12] * m[7] * m[9];

        inv[12] = -m[4]  * m[9] * m[14] + 
                   m[4]  * m[10] * m[13] +
                   m[8]  * m[5] * m[14] - 
                   m[8]  * m[6] * m[13] - 
                   m[12] * m[5] * m[10] + 
                   m[12] * m[6] * m[9];

        inv[1] = -m[1]  * m[10] * m[15] + 
                  m[1]  * m[11] * m[14] + 
                  m[9]  * m[2] * m[15] - 
                  m[9]  * m[3] * m[14] - 
                  m[13] * m[2] * m[11] + 
                  m[13] * m[3] * m[10];

        inv[5] = m[0]  * m[10] * m[15] - 
                 m[0]  * m[11] * m[14] - 
                 m[8]  * m[2] * m[15] + 
                 m[8]  * m[3] * m[14] + 
                 m[12] * m[2] * m[11] - 
                 m[12] * m[3] * m[10];

        inv[9] = -m[0]  * m[9] * m[15] + 
                  m[0]  * m[11] * m[13] + 
                  m[8]  * m[1] * m[15] - 
                  m[8]  * m[3] * m[13] - 
                  m[12] * m[1] * m[11] + 
                  m[12] * m[3] * m[9];

        inv[13] = m[0]  * m[9] * m[14] - 
                  m[0]  * m[10] * m[13] - 
                  m[8]  * m[1] * m[14] + 
                  m[8]  * m[2] * m[13] + 
                  m[12] * m[1] * m[10] - 
                  m[12] * m[2] * m[9];

        inv[2] = m[1]  * m[6] * m[15] - 
                 m[1]  * m[7] * m[14] - 
                 m[5]  * m[2] * m[15] + 
                 m[5]  * m[3] * m[14] + 
                 m[13] * m[2] * m[7] - 
                 m[13] * m[3] * m[6];

        inv[6] = -m[0]  * m[6] * m[15] + 
                  m[0]  * m[7] * m[14] + 
                  m[4]  * m[2] * m[15] - 
                  m[4]  * m[3] * m[14] - 
                  m[12] * m[2] * m[7] + 
                  m[12] * m[3] * m[6];

        inv[10] = m[0]  * m[5] * m[15] - 
                  m[0]  * m[7] * m[13] - 
                  m[4]  * m[1] * m[15] + 
                  m[4]  * m[3] * m[13] + 
                  m[12] * m[1] * m[7] - 
                  m[12] * m[3] * m[5];

        inv[14] = -m[0]  * m[5] * m[14] + 
                   m[0]  * m[6] * m[13] + 
                   m[4]  * m[1] * m[14] - 
                   m[4]  * m[2] * m[13] - 
                   m[12] * m[1] * m[6] + 
                   m[12] * m[2] * m[5];

        inv[3] = -m[1] * m[6] * m[11] + 
                  m[1] * m[7] * m[10] + 
                  m[5] * m[2] * m[11] - 
                  m[5] * m[3] * m[10] - 
                  m[9] * m[2] * m[7] + 
                  m[9] * m[3] * m[6];

        inv[7] = m[0] * m[6] * m[11] - 
                 m[0] * m[7] * m[10] - 
                 m[4] * m[2] * m[11] + 
                 m[4] * m[3] * m[10] + 
                 m[8] * m[2] * m[7] - 
                 m[8] * m[3] * m[6];

        inv[11] = -m[0] * m[5] * m[11] + 
                   m[0] * m[7] * m[9] + 
                   m[4] * m[1] * m[11] - 
                   m[4] * m[3] * m[9] - 
                   m[8] * m[1] * m[7] + 
                   m[8] * m[3] * m[5];

        inv[15] = m[0] * m[5] * m[10] - 
                  m[0] * m[6] * m[9] - 
                  m[4] * m[1] * m[10] + 
                  m[4] * m[2] * m[9] + 
                  m[8] * m[1] * m[6] - 
                  m[8] * m[2] * m[5];

        det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

        if (det == 0)
            return false;

        det = 1.0 / det;

        for (i = 0; i < 16; i++)
            invOut[i] = inv[i] * det;

        return true;
}


Matrix4 mat4_axisAngle(V3 axis, float angle) {
    //NOTE: this is around the wrong way I think, should be transposed
    axis = normalizeV3(axis);
    float x = axis.x;
    float y = axis.y;
    float z = axis.z;
    Matrix4 result = {{
            1 + (1-(float)cos(angle))*(x*x-1), -z*(float)sin(angle)+(1-(float)cos(angle))*x*y, y*(float)sin(angle)+(1-(float)cos(angle))*x*z, 0,
            z*(float)sin(angle)+(1-(float)cos(angle))*x*y, 1 + (1-(float)cos(angle))*(y*y-1),  -x*(float)sin(angle)+(1-(float)cos(angle))*y*z, 0,
            -y*(float)sin(angle)+(1-(float)cos(angle))*x*z, x*(float)sin(angle)+(1-(float)cos(angle))*y*z, 1 + (1-(float)cos(angle))*(z*z-1), 0,
            0, 0, 0, 1
        }};
    return result;
}

Matrix4 Matrix4_translate(Matrix4 a, V3 b) {
    a.d.x += b.x;
    a.d.y += b.y;
    a.d.z += b.z;
    return a;
}

Matrix4 Matrix4_scale(Matrix4 a, V3 b) {
    a.a.xyz = v3_scale(b.x, a.a.xyz);
    a.b.xyz = v3_scale(b.y, a.b.xyz);
    a.c.xyz = v3_scale(b.z, a.c.xyz);
    return a;
}

Matrix4 Matrix4_scale_uniformly(Matrix4 a, float b) {
    a.a.xyz = v3_scale(b, a.a.xyz);
    a.b.xyz = v3_scale(b, a.b.xyz);
    a.c.xyz = v3_scale(b, a.c.xyz);
    return a;
}


//order same as math operations a = View matrix, b = model matrix.
Matrix4 Mat4Mult(Matrix4 a, Matrix4 b) {
    //SIMD this. Can we use V4 instead of float 
    Matrix4 result = {};
    
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
            
            result.E[i][j] = 
                a.E[0][j] * b.E[i][0] + 
                a.E[1][j] * b.E[i][1] + 
                a.E[2][j] * b.E[i][2] + 
                a.E[3][j] * b.E[i][3];
            
        }
    }
    
    return result;
}

inline bool easyMath_mat4AreEqual(Matrix4 *a, Matrix4 *b) {
    bool result = true;
    for(int i = 0; i < 4 && result; ++i) {
        for(int j = 0; j < 4 && result; ++j) {
            if(a->E[i][j] == b->E[i][j]) {

            } else {
                result = false;
                break;
            }
        }
    }


    return result;
}

V4 V4MultMat4(V4 a, Matrix4 b) {
    V4 result = {};
    
    V4 x = v4_scale(a.x, b.a);
    V4 y = v4_scale(a.y, b.b);
    V4 z = v4_scale(a.z, b.c);
    V4 w = v4_scale(a.w, b.d);
    
    result = v4_plus(v4_plus(x, y), v4_plus(z, w));
    return result;
}

Matrix4 mat4TopLeftToBottomLeft(float bufferHeight) {
    Matrix4 transform = mat4_xyAxis(v2(1, 0), v2(0, -1));
    transform = Matrix4_translate(transform, v3(0, bufferHeight, 0));
    return transform;
}

float clamp(float min, float value, float max) {
    if(value < min) {
        value = min;
    }
    if(value > max) {
        value = max;
    }
    return value;
}

float clamp01(float value) {
    float result = min(max(0, value), 1);
    return result;
}

float randomBetween(float a, float b) { // including a and b
    float result = ((float)rand() / (float)RAND_MAX);
    assert(result >= 0 && result <= 1.0f);
    result = lerp(a, result, b);
    return result;
}


float lerp_bounded(float a, float t, float b) {
    float value = a + t*(b - a);
    
    float minVal = min(a, b);
    float maxVal = max(a, b);
    value = clamp(minVal, value, maxVal);
    
    return value;
}

float inverse_lerp(float a, float c, float b) {
    float denominator = (b - a);
    float t = 0;
    if(denominator != 0) {
        t = (c - a) / denominator;    
    }
    
    return t;
}

float mapValue(float value, float minA, float maxA, float minB, float maxB) {
    float result = inverse_lerp(minA, value, maxA);
    result = lerp(minB, result, maxB);
    return result;
}


float smoothStep01(float a, float t, float b) {
    //TODO: change to using formula instead of sin
    float mappedT = sin(t*PI32/2);
    float value = lerp(a, mappedT, b);
    return value;
}

float smoothStep00(float a, float t, float b) { 
    float mappedT = -cos(t*2*PI32);
    mappedT = inverse_lerp(-1, mappedT, 1);
    assert(mappedT >= 0.0f && mappedT <= 1.0f);
    float value = lerp(a, mappedT, b);
    return value;
}

float smoothStep01010(float a, float t, float b) {
    float mappedT = sin(t*TAU32);
    if(mappedT < 0) {
        mappedT *= -1;
    }
    
    float value = lerp(a, mappedT, b);
    
    return value;
}


V2 lerpV2(V2 a, float t, V2 b) {
    V2 value = {};
    
    value.x = lerp(a.x, t, b.x);
    value.y = lerp(a.y, t, b.y);
    
    return value;
}

V3 lerpV3(V3 a, float t, V3 b) {
    V3 value = {};
    
    value.x = lerp(a.x, t, b.x);
    value.y = lerp(a.y, t, b.y);
    value.z = lerp(a.z, t, b.z);
    
    return value;
}

V4 lerpV4(V4 a, float t, V4 b) {
    V4 value = {};
    
    value.x = lerp(a.x, t, b.x);
    value.y = lerp(a.y, t, b.y);
    value.z = lerp(a.z, t, b.z);
    value.w = lerp(a.w, t, b.w);
    
    return value;
}

V3 smoothStep01V3(V3 a, float t, V3 b) {
    V3 value = {};
    
    value.x = smoothStep01(a.x, t, b.x);
    value.y = smoothStep01(a.y, t, b.y);
    value.z = smoothStep01(a.z, t, b.z);
    
    return value;
}

V3 smoothStep00V3(V3 a, float t, V3 b) {
    V3 value = {};
    
    value.x = smoothStep00(a.x, t, b.x);
    value.y = smoothStep00(a.y, t, b.y);
    value.z = smoothStep00(a.z, t, b.z);
    
    return value;
}

V4 smoothStep01V4(V4 a, float t, V4 b) {
    V4 value = {};
    
    value.x = smoothStep01(a.x, t, b.x);
    value.y = smoothStep01(a.y, t, b.y);
    value.z = smoothStep01(a.z, t, b.z);
    value.w = smoothStep01(a.w, t, b.w);
    
    return value;
}

V4 smoothStep00V4(V4 a, float t, V4 b) {
    V4 value = {};
    
    value.x = smoothStep00(a.x, t, b.x);
    value.y = smoothStep00(a.y, t, b.y);
    value.z = smoothStep00(a.z, t, b.z);
    value.w = smoothStep00(a.w, t, b.w);
    
    return value;
}
V4 smoothStep01010V4(V4 a, float t, V4 b) {
    V4 value = {};
    
    value.x = smoothStep01010(a.x, t, b.x);
    value.y = smoothStep01010(a.y, t, b.y);
    value.z = smoothStep01010(a.z, t, b.z);
    value.w = smoothStep01010(a.w, t, b.w);
    
    return value;
}


bool inCircle(V2 point, Rect2f bounds) {
    bool result = false;
    
    V2 pos = getCenter(bounds);
    V2 dim = getDim(bounds);
    
    V2 rel = v2_minus(point, pos);
    
    float lenSqrPoi = getLengthSqr(rel);
    float lenSqrCir = getLengthSqr(dim);
    
    if(lenSqrPoi < lenSqrCir) {
        result = true;
        
    }
    
    return result;
}

typedef enum {
    BOUNDS_CIRCLE,
    BOUNDS_RECT,
} BoundsType;

bool inBounds(V2 point, Rect2f bounds, BoundsType type) {
    bool result = false;
    if(type == BOUNDS_CIRCLE) {
        result = inCircle(point, bounds);
    } else if(type == BOUNDS_RECT) {
        result = inRect(point, bounds);
    }
    return result;
}

Rect2f expandRectf(Rect2f bounds, V2 expansion) {
    bounds.minX -= expansion.x;
    bounds.maxX += expansion.x;
    bounds.minY -= expansion.y;
    bounds.maxY += expansion.y;
    
    return bounds;
}

V2 transformPosition(V2 pos, Matrix4 offsetTransform) {
    V4 centerV4 = v2ToV4Homogenous(pos);
    V2 deltaP = V4MultMat4(centerV4, offsetTransform).xy; 
    
    return deltaP;
}

V3 transformPositionV3(V3 pos, Matrix4 transform) {
    V4 centerV4 = v3ToV4Homogenous(pos);
    V3 deltaP = V4MultMat4(centerV4, transform).xyz; 
    
    return deltaP;
}

V4 transformPositionV3ToV4(V3 pos, Matrix4 transform) {
    V4 centerV4 = v3ToV4Homogenous(pos);
    V4 deltaP = V4MultMat4(centerV4, transform); 
    
    return deltaP;
}

Rect2f transformRect2f(Rect2f rect, Matrix4 offsetTransform) {
    Rect2f result;
    result.min = V4MultMat4(v2ToV4Homogenous(rect.min), offsetTransform).xy; 
    result.max = V4MultMat4(v2ToV4Homogenous(rect.max), offsetTransform).xy; 
    
    result = reevalRect2f(result);
    return result;
}

V2 v2_transformPerspective(V2 a, float zValue) {
    if(zValue == 0) {
        zValue = 1;
    }
    V2 result = v2(a.x/zValue, a.y/zValue);
    return result;
}

bool isNanf(float a) {
    return isnan(a);
}

bool isNanV2(V2 a) {
    bool result = false;
    if(isnan(a.x) || isnan(a.y)) {
        result = true;
    }
    return result;
}

bool isNanV3(V3 a) {
    bool result = false;
    if(isnan(a.x) || isnan(a.y) || isnan(a.z)) {
        result = true;
    }
    return result;
}

void transformRectangleToSides(V2 *points, V2 position , V2 dim, Matrix4 rotationMatrix) {
    Rect2f rectA = rect2fCenterDimV2(v2(0, 0), dim);
    points[0] = v2_plus(transformPosition(v2(rectA.minX, rectA.minY), rotationMatrix), position); 
    points[1] = v2_plus(transformPosition(v2(rectA.minX, rectA.maxY), rotationMatrix), position); 
    points[2] = v2_plus(transformPosition(v2(rectA.maxX, rectA.maxY), rotationMatrix), position); 
    points[3] = v2_plus(transformPosition(v2(rectA.maxX, rectA.minY), rotationMatrix), position);
}

typedef struct {
    V3 origin;
    V3 direction;
} EasyRay;

typedef struct {
    V3 origin;
    V3 normal;
} EasyPlane;

static inline bool easyMath_castRayAgainstPlane(EasyRay r, EasyPlane p, V3 *hitPoint, float *tAt) {
    bool didHit = false;

    //NOTE(ollie): Normalize the ray & plane normal
    p.normal = normalizeV3(p.normal);
    r.direction = normalizeV3(r.direction);
    //

    float denom = dotV3(p.normal, r.direction); 
    if (absVal(denom) > 1e-6) {  //ray is perpindicular to the plane, therefore either infinity or zero solutions
        V3 p0l0 = v3_minus(p.origin, r.origin); 
        *tAt = dotV3(p0l0, p.normal) / denom; 
        
        *hitPoint = v3_plus(r.origin, v3_scale(*tAt, r.direction));
        didHit = true;
    } 


    return didHit;
}

static inline EasyRay EasyMath_transformRay(EasyRay r, Matrix4 m) {
    r.origin =  V4MultMat4(v4(r.origin.x, r.origin.y, r.origin.z, 0), m).xyz;
    r.direction  =  V4MultMat4(v4(r.direction.x, r.direction.y, r.direction.z, 0), m).xyz;
    return r;
}

#define NUMDIM  3
#define RIGHT   0
#define LEFT    1
#define MIDDLE  2

static inline bool easyMath_rayVsAABB3f(V3 origin, V3 dir, Rect3f b, V3 *hitPoint, float *tAt) {
    bool inside = true;
    int quadrant[NUMDIM];
    int i;
    int whichPlane;
    double maxT[NUMDIM];
    double candidatePlane[NUMDIM];

    dir = normalizeV3(dir);

    /* Find candidate planes; this loop can be avoided if
    rays cast all from the eye(assume perpsective view) */
    for (i=0; i<NUMDIM; i++)
        if(origin.E[i] < b.min.E[i]) {
            quadrant[i] = LEFT;
            candidatePlane[i] = b.min.E[i];
            inside = false;
        }else if (origin.E[i] > b.max.E[i]) {
            quadrant[i] = RIGHT;
            candidatePlane[i] = b.max.E[i];
            inside = false;
        }else   {
            quadrant[i] = MIDDLE;
        }

    /* Ray origin inside bounding box */
    if(inside)  {
        *hitPoint = origin;
        return (TRUE);
    }


    /* Calculate T distances to candidate planes */
    for (i = 0; i < NUMDIM; i++)
        if (quadrant[i] != MIDDLE && dir.E[i] !=0.) //not in the middle & no parrallel
            maxT[i] = (candidatePlane[i]-origin.E[i]) / dir.E[i];
        else
            maxT[i] = -1.;

    /* Get largest of the maxT's for final choice of intersection */
    whichPlane = 0;
    for (i = 1; i < NUMDIM; i++) {
        if (maxT[whichPlane] < maxT[i]) {
            whichPlane = i;
        }
    }
    *tAt = maxT[whichPlane];
    /* Check final candidate actually inside box */
    if (maxT[whichPlane] < 0.) return (false); //behind the ray
    for (i = 0; i < NUMDIM; i++)
        if (whichPlane != i) {
            hitPoint->E[i] = origin.E[i] + maxT[whichPlane] *dir.E[i]; //our lerp
            if (hitPoint->E[i] < b.min.E[i] || hitPoint->E[i] > b.max.E[i]) //outside of the AABB
                return false;
        } else {
            hitPoint->E[i] = candidatePlane[i];
        }
    return true;              /* ray hits box */
}

#define X_AXIS v3(1, 0, 0)
#define Y_AXIS v3(0, 1, 0)
#define Z_AXIS v3(0, 0, 1)

#define NULL_VECTOR2 v2(0, 0)
#define NULL_VECTOR3 v3(0, 0, 0)

#define EASY_MATH_H 1
#endif