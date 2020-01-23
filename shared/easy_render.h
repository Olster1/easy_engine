#define PI32 3.14159265359
#define RENDER_HANDNESS 1 //positive is left hand handess -> z going into the screen. 
#define NEAR_CLIP_PLANE 0.1f
#define FAR_CLIP_PLANE 1000.0f

#define PRINT_NUMBER_DRAW_CALLS 0

#if !DESKTOP
#define GL_TEXTURE_BUFFER                 0x8C2A
#define RENDER_TEXTURE_BUFFER_ENUM GL_TEXTURE_BUFFER
#else 
#define RENDER_TEXTURE_BUFFER_ENUM GL_TEXTURE_BUFFER
#endif

#if !defined arrayCount
#define arrayCount(arg) (sizeof(arg) / sizeof(arg[0])) 
#endif

#if !defined EASY_MATH_H
#include "easy_math.h"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static float globalTimeSinceStart = 0.0f;


#include "easy_shaders.h"

#define EASY_MAX_LIGHT_COUNT 16

#define VERTEX_ATTRIB_LOCATION 0
#define NORMAL_ATTRIB_LOCATION 1
#define UV_ATTRIB_LOCATION 2
#define MODEL_ATTRIB_LOCATION 3
#define VIEW_ATTRIB_LOCATION 7
#define COLOR_ATTRIB_LOCATION 11
#define UVATLAS_ATTRIB_LOCATION 12

#define PROJECTION_TYPE(FUNC) \
FUNC(PERSPECTIVE_MATRIX) \
FUNC(ORTHO_MATRIX) \

static bool globalImmediateModeGraphics = false;

static V4 globalSkyColor = v4(0.4f, 0.6f, 2.0f, 1.0f);


#if DEVELOPER_MODE
static bool DEBUG_drawWireFrame = false;
static bool DEBUG_drawBounds = true;
#endif

typedef enum {
    PROJECTION_TYPE(ENUM)
} ProjectionType;

static char *ProjectionTypeStrings[] = { PROJECTION_TYPE(STRING) };

typedef enum {
    EASY_LIGHT_DIRECTIONAL,
    EASY_LIGHT_POINT,
    EASY_LIGHT_FLASH_LIGHT,
} EasyLightType;

typedef struct {
    // V4 direction; //or position w component == 1 for points lights, w == 0 for directional lights
    
    // V3 ambient;
    // V3 diffuse;
    // V3 specular;

    EasyTransform *T; //for drawing model & direction of light, could also effect rdius??

    EasyLightType type;
    float brightness;
    V3 color;

    union {
        struct {
            //point lights
            float innerRadius; 
            float outerRadius;
        };
    };
} EasyLight;

typedef struct {
    s32 handle;
    char *name;
} ShaderVal;

typedef struct {
    GLuint glProgram;
    GLuint glShaderV;
    GLuint glShaderF;
    
    ShaderVal uniforms[16];
    int uniformCount;
    
    ShaderVal attribs[16];
    int attribCount;
    
    bool valid;
} RenderProgram;

RenderProgram phongProgram;
RenderProgram glossProgram;
RenderProgram skyboxProgram;
RenderProgram textureProgram;
RenderProgram skyQuadProgram;
RenderProgram terrainProgram;
RenderProgram toneMappingProgram;
RenderProgram blurProgram;
RenderProgram colorWheelProgram;
RenderProgram circleProgram;


typedef struct {
    V3 pos;
    V3 dim; 
    V3 transformPos;
    V3 transformDim;
    Matrix4 pvm;
} RenderInfo;

RenderInfo calculateRenderInfo(V3 pos, V3 dim, V3 cameraPos, Matrix4 metresToPixels) {
    RenderInfo info = {};
    info.pos = pos;
    
    info.pvm = Mat4Mult(metresToPixels, Matrix4_translate(mat4(), v3_scale(-1, cameraPos)));
    
    info.transformPos = V4MultMat4(v4(pos.x, pos.y, pos.z, 1), info.pvm).xyz;
    
    info.dim = dim;
    info.transformDim = transformPositionV3(dim, metresToPixels);
    return info;
}

FileContents loadShader(char *fileName) {
    FileContents fileContents = getFileContentsNullTerminate(fileName);
    return fileContents;
}

typedef struct {
    union {
        struct {
            float E[13]; //position -> normal -> textureUVs -> colors
        };
        struct {
            V3 position;
            V3 normal;
            V2 texUV;
            V4 color;
            u32 instanceIndex;
        };
    };
} Vertex;

static inline Vertex vertex(V3 pos, V3 normal, V2 texUV) {
    Vertex v = {};
    v.position = pos;
    v.normal = normal;
    v.texUV = texUV;

    return v;
}


#define getOffsetForVertex(attrib) (void *)(&(((Vertex *)(0))->attrib))

typedef enum {
    SHAPE_RECTANGLE,
    SHAPE_RECTANGLE_GRAD,
    SHAPE_TEXTURE,
    SHAPE_MODEL,
    SHAPE_SKYBOX,
    SHAPE_SKY_QUAD,
    SHAPE_TONE_MAP,
    SHAPE_TERRAIN,
    SHAPE_SHADOW,
    SHAPE_CIRCLE,
    SHAPE_LINE,
    SHAPE_BLUR,
    SHAPE_COLOR_WHEEL,
} ShapeType;

typedef enum {
    TEXTURE_FILTER_LINEAR, 
    TEXTURE_FILTER_NEAREST, 
} RenderTextureFilter;

typedef enum {
    BLEND_FUNC_STANDARD,
    BLEND_FUNC_STANDARD_NO_PREMULTIPLED_ALPHA,
    BLEND_FUNC_ZERO_ONE_ZERO_ONE_MINUS_ALPHA, 
} BlendFuncType;

typedef struct {
    GLuint vaoHandle;
    int indexCount; // this is to keep around so opnegl knows how many triangles to draw after the initialization frame
    bool valid;
    GLuint vboInstanceData; //this is for instancing data
} VaoHandle;

//just has a dim of 1 by 1 and you can rotate, scale etc. by a model matrix
static Vertex globalQuadPositionData[4] = {
    vertex(v3(-0.5f, -0.5f, 0), v3(0, 0, -1), v2(0, 0)),
    vertex(v3(-0.5f, 0.5f, 0), v3(0, 0, -1), v2(0, 1)),
    vertex(v3(0.5f, 0.5f, 0), v3(0, 0, -1), v2(1, 1)),
    vertex(v3(0.5f, -0.5f, 0), v3(0, 0, -1), v2(1, 0))
};

static unsigned int globalQuadIndicesData[6] = {0, 1, 2, 0, 2, 3};

static VaoHandle globalQuadVaoHandle = {};

//just has a dim of 1 by 1 and you can rotate, scale etc. by a model matrix
static Vertex globalFullscreenQuadPositionData[4] = {
    vertex(v3(-1.0f, -1.0f, 1.0f), v3(0, 0, -1), v2(0, 0)),
    vertex(v3(-1.0f, 1.0f, 1.0f), v3(0, 0, -1), v2(0, 1)),
    vertex(v3(1.0f, 1.0f, 1.0f), v3(0, 0, -1), v2(1, 1)),
    vertex(v3(1.0f, -1.0f, 1.0f), v3(0, 0, -1), v2(1, 0))
};

static unsigned int globalFullscreenQuadIndicesData[6] = {0, 1, 2, 0, 2, 3};

static VaoHandle globalFullscreenQuadVaoHandle = {};

//For a cude
static Vertex globalCubeVertexData[24] = { //anti-clockwise 
    vertex(v3(-0.5f, -0.5f, 0.5f), v3(0, 0, 1), v2(0, 0)), //back panel
    vertex(v3(-0.5f, 0.5f, 0.5f), v3(0, 0, 1), v2(0, 1)),
    vertex(v3(0.5f, 0.5f, 0.5f), v3(0, 0, 1), v2(1, 1)),
    vertex(v3(0.5f, -0.5f, 0.5f), v3(0, 0, 1), v2(1, 0)),

    vertex(v3(-0.5f, -0.5f, -0.5f), v3(0, 0, -1), v2(0, 0)), //front panel 
    vertex(v3(0.5f, -0.5f, -0.5f), v3(0, 0, -1), v2(1, 0)),
    vertex(v3(0.5f, 0.5f, -0.5f), v3(0, 0, -1), v2(1, 1)),
    vertex(v3(-0.5f, 0.5f, -0.5f), v3(0, 0, -1), v2(0, 1)),

    vertex(v3(-0.5f, -0.5f, 0.5f), v3(-1, 0, 0), v2(0, 0)), // left side
    vertex(v3(-0.5f, -0.5f, -0.5f), v3(-1, 0, 0), v2(1, 0)),
    vertex(v3(-0.5f, 0.5f, -0.5f), v3(-1, 0, 0), v2(1, 1)),
    vertex(v3(-0.5f, 0.5f, 0.5f), v3(-1, 0, 0), v2(0, 1)),

    vertex(v3(0.5f, -0.5f, 0.5f), v3(1, 0, 0), v2(0, 0)), //right side
    vertex(v3(0.5f, 0.5f, 0.5f), v3(1, 0, 0), v2(0, 1)),
    vertex(v3(0.5f, 0.5f, -0.5f), v3(1, 0, 0), v2(1, 1)),
    vertex(v3(0.5f, -0.5f, -0.5f), v3(1, 0, 0), v2(1, 0)),

    vertex(v3(0.5f, 0.5f, -0.5f), v3(0, 1, 0), v2(0, 0)), //top panel
    vertex(v3(0.5f, 0.5f, 0.5f), v3(0, 1, 0), v2(1, 0)),
    vertex(v3(-0.5f, 0.5f, 0.5f), v3(0, 1, 0), v2(1, 1)),
    vertex(v3(-0.5f, 0.5f, -0.5f), v3(0, 1, 0), v2(0, 1)),

    vertex(v3(0.5f, -0.5f, -0.5f), v3(0, -1, 0), v2(0, 0)), //bottom panel
    vertex(v3(-0.5f, -0.5f, -0.5f), v3(0, -1, 0), v2(0, 1)),
    vertex(v3(-0.5f, -0.5f, 0.5f), v3(0, -1, 0), v2(1, 1)),
    vertex(v3(0.5f, -0.5f, 0.5f), v3(0, -1, 0), v2(1, 0))
};


static Vertex globalCubeMapVertexData[24] = { //anti-clockwise 
    

    vertex(v3(1.0f, -1.0f, 1.0f), v3(1, 0, 0), v2(0, 0)), //right side
    vertex(v3(1.0f, 1.0f, 1.0f), v3(1, 0, 0), v2(0, 1)),
    vertex(v3(1.0f, 1.0f, -1.0f), v3(1, 0, 0), v2(1, 1)),
    vertex(v3(1.0f, -1.0f, -1.0f), v3(1, 0, 0), v2(1, 0)),

    vertex(v3(-1.0f, -1.0f, 1.0f), v3(-1, 0, 0), v2(0, 0)), // left side
    vertex(v3(-1.0f, -1.0f, -1.0f), v3(-1, 0, 0), v2(1, 0)),
    vertex(v3(-1.0f, 1.0f, -1.0f), v3(-1, 0, 0), v2(1, 1)),
    vertex(v3(-1.0f, 1.0f, 1.0f), v3(-1, 0, 0), v2(0, 1)),

    vertex(v3(1.0f, 1.0f, -1.0f), v3(0, 1, 0), v2(0, 0)), //top panel
    vertex(v3(1.0f, 1.0f, 1.0f), v3(0, 1, 0), v2(1, 0)),
    vertex(v3(-1.0f, 1.0f, 1.0f), v3(0, 1, 0), v2(1, 1)),
    vertex(v3(-1.0f, 1.0f, -1.0f), v3(0, 1, 0), v2(0, 1)),

    vertex(v3(1.0f, -1.0f, -1.0f), v3(0, -1, 0), v2(0, 0)), //bottom panel
    vertex(v3(-1.0f, -1.0f, -1.0f), v3(0, -1, 0), v2(0, 1)),
    vertex(v3(-1.0f, -1.0f, 1.0f), v3(0, -1, 0), v2(1, 1)),
    vertex(v3(1.0f, -1.0f, 1.0f), v3(0, -1, 0), v2(1, 0)),

    vertex(v3(-1.0f, -1.0f, 1.0f), v3(0, 0, 1), v2(0, 0)), //back panel
    vertex(v3(-1.0f, 1.0f, 1.0f), v3(0, 0, 1), v2(0, 1)),
    vertex(v3(1.0f, 1.0f, 1.0f), v3(0, 0, 1), v2(1, 1)),
    vertex(v3(1.0f, -1.0f, 1.0f), v3(0, 0, 1), v2(1, 0)),

    vertex(v3(-1.0f, -1.0f, -1.0f), v3(0, 0, -1), v2(0, 0)), //front panel 
    vertex(v3(1.0f, -1.0f, -1.0f), v3(0, 0, -1), v2(1, 0)),
    vertex(v3(1.0f, 1.0f, -1.0f), v3(0, 0, -1), v2(1, 1)),
    vertex(v3(-1.0f, 1.0f, -1.0f), v3(0, 0, -1), v2(0, 1))
};

static unsigned int globalCubeIndicesData[36] = 
{0, 1, 2, 0, 2, 3, 
4, 5, 6, 4, 6, 7, 
8, 9, 10, 8, 10, 11,
12, 13, 14, 12, 14, 15,
16, 17, 18, 16, 18, 19,
20, 21, 22, 20, 22, 23 };

static VaoHandle globalCubeVaoHandle = {};
static VaoHandle globalCubeMapVaoHandle = {};

typedef struct {
    GLuint id;
    int width;
    int height;
    Rect2f uvCoords;
} Texture;

static Texture globalWhiteTexture;
static bool globalWhiteTextureInited;

typedef struct {
    int w;
    int h;
    int comp;
    unsigned char *image;
} EasyImage;

#define renderCheckError() renderCheckError_(__LINE__, (char *)__FILE__)
void renderCheckError_(int lineNumber, char *fileName) {
    GLenum err = glGetError();
    if(err) {
        printf((char *)"GL error check: %x at %d in %s\n", err, lineNumber, fileName);
        assert(!err);
    }
    
}

Texture createTextureOnGPU(unsigned char *image, int w, int h, int comp, RenderTextureFilter filter, bool hasMipMaps) {
    Texture result = {};
    if(image) {
        
        result.width = w;
        result.height = h;
        result.uvCoords = rect2f(0, 0, 1, 1);
        
        glGenTextures(1, &result.id);
        renderCheckError();
        
        glBindTexture(GL_TEXTURE_2D, result.id);
        renderCheckError();
        
        GLuint filterVal = GL_LINEAR;
        GLuint mipMapFilter = GL_LINEAR_MIPMAP_LINEAR;
        if(filter == TEXTURE_FILTER_NEAREST) {
            filterVal = GL_NEAREST;
            mipMapFilter = GL_NEAREST_MIPMAP_NEAREST;
        } 
        
        if(comp == 3) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
            renderCheckError();
        } else if(comp == 4) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
            renderCheckError();
        } else {
            assert(!"Channel number not handled!");
        }

        if(hasMipMaps) {
            glGenerateMipmap(GL_TEXTURE_2D);   
            renderCheckError(); 

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0);
            renderCheckError();
        } else {
            mipMapFilter = filterVal;
        }
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipMapFilter);
        renderCheckError();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterVal);
        renderCheckError();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        renderCheckError();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        renderCheckError();
        
        glBindTexture(GL_TEXTURE_2D, 0);
        renderCheckError();
    } 
    
    return result;
}

static inline EasyImage loadImage_(char *fileName) {
    EasyImage result;
    result.comp = 4;
    result.image = (unsigned char *)stbi_load(fileName, &result.w, &result.h, &result.comp, STBI_rgb_alpha);
    
    if(result.image) {
        // assert(result.comp == 4);
    } else {
        printf("%s\n", fileName);
        assert(!"no image found");
    }
    
    
    return result;
}

static inline void easy_endImage(EasyImage *image) {
    if(image->image) {
        stbi_image_free(image->image);
    }
}

Texture loadImage(char *fileName, RenderTextureFilter filter, bool hasMipMaps) {
    EasyImage image = loadImage_(fileName);
    
    Texture result = createTextureOnGPU(image.image, image.w, image.h, image.comp, filter, hasMipMaps);
    easy_endImage(&image);
    return result;
}




// typedef enum {
//     RENDER_LIGHT_DIRECTIONAL,
//     RENDER_LIGHT_POINT,
//     RENDER_LIGHT_FLASHLIGHT
// } RenderLightType;

// // typedef struct {
// //     RenderLightType type;
    
// //     V3 position; //or direction

// //     V3 ambient;
// //     V3 diffuse; 
// //     V3 specular; //Usually black->white mono

// //     //for point light and flash light -> represents the attentuation as you get further away
// //     float constant;
// //     float linear;
// //     float quadratic;
// //     ///

// //     //for just the flash light -> for the cone of the flash light
// //     float cutoff;
// //     float outerCutoff;
// //     //
// // } EasyLight;

typedef struct {
    Texture *diffuseMap;
    Texture *normalMap;
    Texture *specularMap;

    V4 defaultAmbient;
    V4 defaultDiffuse;
    V4 defaultSpecular;

    float specularConstant;
    
    char *name;
} EasyMaterial;

static EasyMaterial globalWhiteMaterial;

typedef struct {
    int vertexCount;
    VaoHandle vaoHandle;
    
    //NOTE(ol): This is a copy  of the vertex buffer and indicies buffer stored on the CPU. 
    // We can then load them onto the GPU when we actually want to draw the mesh
    InfiniteAlloc vertexData;
    InfiniteAlloc indicesData;
    
    //TODO: store a copy of a material instead of a pointer so a mesh can change the values?
    EasyMaterial *material;

    Rect3f bounds;
} EasyMesh;

typedef struct {
    //NOTE(ol): to do parent child transform operations i.e. get the model matrix. 
    //We might want to have a graph for this but makes it harder with our renderer to do batch rendering
    // EasyTransform *parentTransform; 
    int meshCount;
    EasyMesh *meshes[128];    

    Rect3f bounds;
    V4 colorTint;

    // EasyTransform *T;
} EasyModel;


EasyMaterial easyCreateMaterial(Texture *diffuseMap, Texture *normalMap, Texture *specularMap, float constant) {
    EasyMaterial material = {};
    
    material.diffuseMap = diffuseMap;
    material.normalMap = normalMap;
    material.specularMap = specularMap;
    material.specularConstant = constant;

    material.defaultAmbient = v4(1, 1, 1, 1);
    material.defaultDiffuse = v4(1, 1, 1, 1);
    material.defaultSpecular = v4(1, 1, 1, 1);

    return material;
} 

static void easy3d_initMaterial(EasyMaterial *mat) {
    mat->defaultAmbient = v4(1, 1, 1, 1);
    mat->defaultDiffuse = v4(1, 1, 1, 1);
    mat->defaultSpecular = v4(1, 1, 1, 1);
    mat->specularConstant = 16;

    mat->diffuseMap = &globalWhiteTexture;
    mat->normalMap = &globalWhiteTexture;
    mat->specularMap = &globalWhiteTexture;
}

typedef struct {
    int textureCount;
    Texture *textures[4];
    Texture *blendMap;
    V3 dim;
} EasyTerrainDataPacket;

typedef struct {
    float value;
} EasyRender_ColorWheel_DataPacket;

typedef struct {
    u32 mainBufferTexId;
    u32 bloomBufferTexId;

    float exposure;
} EasyRender_ToneMap_Bloom_DataPacket;


typedef struct {
    Vertex *triangleData;
    int triCount; 
    
    unsigned int *indicesData; 
    int indexCount; 
    
    RenderProgram *program; 
    ShapeType type; 
    
    u32 textureHandle;
    Rect2f textureUVs;    
    
    Matrix4 mMat;  
    Matrix4 vMat;  //TODOL change to just VM
    Matrix4 pMat; //P of PVM

    EasyMaterial *material;

    float zAt;
    
    int id;

    int projectionId;//So we match against if instead of working out if the matrix is equal for batching. 
    int scissorId;
    
    V4 color;

    void *dataPacket; //Cast later on in the function
    
    int bufferId;
    bool depthTest;
    BlendFuncType blendFuncType;

    bool cullingEnabled;
    
    VaoHandle *bufferHandles;
} RenderItem;

typedef struct {
    GLuint tbo; // this is attached to the buffer
    GLuint buffer;
} BufferStorage;

typedef struct {
    // GL_TEXTURE_CUBE_MAP_POSITIVE_X //right panel
    // GL_TEXTURE_CUBE_MAP_NEGATIVE_X //left panel
    // GL_TEXTURE_CUBE_MAP_POSITIVE_Y //top panel
    // GL_TEXTURE_CUBE_MAP_NEGATIVE_Y //bottom panel
    // GL_TEXTURE_CUBE_MAP_POSITIVE_Z //back panel 
    // GL_TEXTURE_CUBE_MAP_NEGATIVE_Z //front panel
    char *fileNames[6];
} EasySkyBoxImages;

typedef struct {
    //NOTE: This is the handle to the cubemap
    unsigned int gpuHandle;
} EasySkyBox;



//Cube Maps have been specified to follow the RenderMan specification 
//(for whatever reason), and RenderMan assumes the images' origin 
//being in the upper left, contrary to the usual OpenGL behaviour 
//of having the image origin in the lower left. That's why things get 
//swapped in the Y direction. It totally breaks with the usual OpenGL 
//semantics and doesn't make sense at all. But now we're stuck with it.

static inline EasySkyBox *easy_makeSkybox(EasySkyBoxImages *images) {
    stbi_set_flip_vertically_on_load(false);
    
    EasySkyBox *skyBox = pushStruct(&globalLongTermArena, EasySkyBox);
    
#if OPENGL_BACKEND
        unsigned int textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
        skyBox->gpuHandle = textureID;
#endif
    
    for(int i = 0; i < arrayCount(images->fileNames); ++i) {
        char *fileName = images->fileNames[i];
        EasyImage image = loadImage_(concatInArena(globalExeBasePath, fileName, &globalPerFrameArena));
#if OPENGL_BACKEND
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                    0, GL_RGB, image.w, image.h, 0, GL_RGB, GL_UNSIGNED_BYTE, image.image
                    );
#endif
        easy_endImage(&image);
        }

#if OPENGL_BACKEND
       glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
       glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
       glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
       glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
       glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

       glBindTexture(GL_TEXTURE_2D, 0);
#endif
    stbi_set_flip_vertically_on_load(true);
    
    return skyBox;

}

typedef struct EasyRenderBatch EasyRenderBatch;

typedef struct EasyRenderBatch {
    InfiniteAlloc items; //type RenderItem

    EasyRenderBatch *next;

} EasyRenderBatch;

typedef struct {
    RenderProgram *program; 
    ShapeType type; 
    
    s32 textureHandle;
    
    EasyMaterial *material;

    int scissorId;
    
    int bufferId;
    bool depthTest;
    BlendFuncType blendFuncType;

    bool cullingEnabled;

    Matrix4 pMat;
    
    VaoHandle *bufferHandles;
} EasyRender_BatchQuery;

#define RENDER_BATCH_HASH_COUNT 4096

typedef struct {
    int currentBufferId;
    bool currentDepthTest;
    VaoHandle *currentBufferHandles;
    BlendFuncType blendFuncType;
    
    int idAt; 
    EasyRenderBatch *batches[RENDER_BATCH_HASH_COUNT];
    
    // int lastStorageBufferCount;
    // BufferStorage lastBufferStorage[512];

    //NOTE: you can do a declaritive style of shaders via SetShader
    RenderProgram *currentShader;
    Matrix4 modelTransform;
    Matrix4 viewTransform;
    Matrix4 projectionTransform;

    bool scissorsEnabled;
    InfiniteAlloc scissorsTests;

    V3 eyePos;

    bool cullingEnabled;

    EasyLight *lights[EASY_MAX_LIGHT_COUNT];
    EasySkyBox *skybox;
    int lightCount;

    //NOTE: This is for the sky quad
    float fov;
    float aspectRatio; 
    Matrix4 cameraToWorldTransform;
    //

    bool initied;
    
} RenderGroup;



static inline void easyRender_updateSkyQuad(RenderGroup *g, float zoom, float aspectRatio, Matrix4 viewToWorld) {
    g->fov = zoom;
    g->aspectRatio = aspectRatio;
    g->cameraToWorldTransform = viewToWorld;
}




static inline u64 easyRender_getBatchKey(EasyRender_BatchQuery *i) {
    //TODO(ollie): Handle multiple data packets for the batch 
    u64 result = 0;
    result += 19*(intptr_t)i->textureHandle;
    result += 23*(intptr_t)i->bufferHandles;
    result += 29*(intptr_t)i->program;
    result += 31*(intptr_t)i->material;
    result += 31*(int)i->type;
    result += 3*i->scissorId;
    result += 7*i->cullingEnabled;
    result += 3*i->bufferId;
    result += 3*i->depthTest;
    result += 11*i->blendFuncType;

    result %= RENDER_BATCH_HASH_COUNT;
    
    return result;
} 

static inline bool easyRender_canItemBeBatched(RenderItem *i, EasyRender_BatchQuery *j) {
    bool result = (
        i->textureHandle == j->textureHandle && 
        i->bufferHandles == j->bufferHandles && 
        i->program == j->program && 
        i->material == j->material && 
        i->scissorId == j->scissorId && 
        i->cullingEnabled == j->cullingEnabled && 
        i->type == j->type && 
        i->bufferId == j->bufferId &&
        i->depthTest == j->depthTest &&
        i->blendFuncType == j->blendFuncType &&
        easyMath_mat4AreEqual(&i->pMat, &j->pMat));


    return result;

}   

static inline RenderItem *EasyRender_getRenderItem(RenderGroup *g, EasyRender_BatchQuery *i) {
    u64 key = easyRender_getBatchKey(i);
    EasyRenderBatch *batch = g->batches[key];

    bool looking = true;
    while(batch && looking) {
        if(batch->items.count > 0) {
            RenderItem *testItem = getElementFromAlloc(&batch->items, 0, RenderItem);
            if(easyRender_canItemBeBatched(testItem, i)) {
                looking = false;
                break;
            } else {
                batch = batch->next;
            }
        } else {
            looking = false;
            break;
        }
    } 

    if(!batch) {
        batch = pushStruct(&globalLongTermArena, EasyRenderBatch);
        zeroStruct(batch, EasyRenderBatch);
        //NOTE: put at start of linked list
        batch->next = g->batches[key];
        g->batches[key] = batch;
    }

    assert(batch);

    if(batch->items.count == 0 && batch->items.sizeOfMember == 0) {
        batch->items = initInfinteAlloc(RenderItem);
    }

    RenderItem *result = (RenderItem *)addElementInifinteAlloc_(&batch->items, 0);
    return result;
}



static inline void easyRender_disableScissors(RenderGroup *g) {
    g->scissorsEnabled = false;
}
    
static inline EasyLight *easy_makeLight(EasyTransform *T, EasyLightType type, float brightness, V3 color) {
    EasyLight *light = pushStruct(&globalLongTermArena, EasyLight);

    light->T = T; //for drawing model & direction of light, could also effect radius??

    light->type = type;
    light->brightness = brightness;
    light->color = color;

    return light;
}

static inline void easy_addLight(RenderGroup *group, EasyLight *light) {
    assert(group->lightCount < arrayCount(group->lights));
    group->lights[group->lightCount++] = light;
}

void initRenderGroup(RenderGroup *group) {
    assert(!group->initied);
    group->currentDepthTest = true;
    group->blendFuncType = BLEND_FUNC_STANDARD;
    
    group->currentShader = 0;
    group->modelTransform = mat4();
    group->viewTransform = mat4();
    group->projectionTransform = mat4();

    group->skybox = 0;
    group->initied = true;

    group->scissorsEnabled = false;
    group->scissorsTests = initInfinteAlloc(Rect2f);

    zeroSize(&group->batches, sizeof(EasyRenderBatch *)*RENDER_BATCH_HASH_COUNT);

    if(!globalWhiteTextureInited) {
        u32 *imageData = pushArray(&globalPerFrameArena, 32*32, u32);
        for(int y = 0; y < 32; ++y) {
            for(int x = 0; x < 32; ++x) {
                imageData[y*32 + x] = 0xFFFFFFFF;     
            }
        }
        
        globalWhiteTexture = createTextureOnGPU((unsigned char *)imageData, 32, 32, 4, TEXTURE_FILTER_LINEAR, false);
        globalWhiteTextureInited = true;

        easy3d_initMaterial(&globalWhiteMaterial);
    } 
}

static inline void renderClearDepthBuffer(u32 frameBufferId) {
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)frameBufferId); 
    
    glClearDepthf(1.0f);
    renderCheckError();

    glClear(GL_DEPTH_BUFFER_BIT);
    renderCheckError();
    
    
}

void renderSetShader(RenderGroup *group, RenderProgram *shader) {
    group->currentShader = shader;
}

void easy_setEyePosition(RenderGroup *group, V3 pos) {
    group->eyePos = pos;
}

void setModelTransform(RenderGroup *group, Matrix4 trans) {
    group->modelTransform = trans;
}

void setViewTransform(RenderGroup *group, Matrix4 trans) {
    group->viewTransform = trans;
}

void setProjectionTransform(RenderGroup *group, Matrix4 trans) {
    group->projectionTransform = trans;
}

void setFrameBufferId(RenderGroup *group, int bufferId) {
    group->currentBufferId = bufferId;
    
}

void renderDisableCulling(RenderGroup *group) {
    group->cullingEnabled = false;
}

void renderEnableCulling(RenderGroup *group) {
    group->cullingEnabled = true;
}


void renderDisableDepthTest(RenderGroup *group) {
    group->currentDepthTest = false;
}

void renderEnableDepthTest(RenderGroup *group) {
    group->currentDepthTest = true;
}

void setBlendFuncType(RenderGroup *group, BlendFuncType type) {
    group->blendFuncType = type;
}

static RenderGroup *globalRenderGroup;

void renderSetViewPort(float x0, float y0, float x1, float y1) {
    glViewport(x0, y0, x1, y1);
}

RenderItem *pushRenderItem(VaoHandle *handles, RenderGroup *group, Vertex *triangleData, int triCount, unsigned int *indicesData, int indexCount, RenderProgram *program, ShapeType type, Texture *texture, Matrix4 mMat, Matrix4 vMat, Matrix4 pMat, V4 color, float zAt, EasyMaterial *material) {
    assert(group->initied);
        
    int scissorId = -1;
    if(group->scissorsEnabled) {
        assert(group->scissorsTests.count > 0);//something pushed on to the stack
        scissorId = group->scissorsTests.count - 1;
    }

    EasyRender_BatchQuery query;
    query.program = program; 
    query.type = type;
    if(texture) {
        query.textureHandle = texture->id;
        assert(query.textureHandle > 0);    
    } else {
        query.textureHandle = 0; //no Texture
        if(type == SHAPE_RECTANGLE) {
            query.program = &textureProgram;
            query.type = SHAPE_TEXTURE; 

            assert(globalWhiteTextureInited);
            query.textureHandle = globalWhiteTexture.id;
        } 
        
    }
    query.pMat = pMat;
    query.material = material;
    query.scissorId = scissorId;
    query.bufferId = group->currentBufferId;
    query.depthTest = group->currentDepthTest;
    query.blendFuncType = group->blendFuncType;
    query.cullingEnabled = group->cullingEnabled;
    query.bufferHandles = handles;

    RenderItem *info = EasyRender_getRenderItem(group, &query);

    assert(info);
    info->bufferId = group->currentBufferId;
    info->depthTest = group->currentDepthTest;
    info->blendFuncType = group->blendFuncType;
    info->bufferHandles = handles;
    info->color = color;
    info->zAt = zAt;

    info->material = material;
    info->triCount = triCount; 
    info->triangleData = triangleData;
    info->indicesData = indicesData;
    
    info->indexCount = indexCount;
    
    info->id = group->idAt++;

    info->cullingEnabled = group->cullingEnabled;

    info->scissorId = -1;
    if(group->scissorsEnabled) {
        assert(group->scissorsTests.count > 0);//something pushed on to the stack
        info->scissorId = group->scissorsTests.count - 1;
    }
    
    info->program = program;
    info->type = type;
    //We are now overriding rectangles with texture calls & using a white dummy texture.
    //Could make this cleaner by original making it a SHAPE_TEXTURE. Might prevent possible bugs that 
    //might occur??!! - Oliver 22/1/19
    if(info->type == SHAPE_RECTANGLE) {
        assert(program == &textureProgram);
        info->program = &textureProgram;
        info->type = SHAPE_TEXTURE; 
        //set the texture so the render item gets assigned the uv data.
        assert(globalWhiteTextureInited);
        texture = &globalWhiteTexture;
    } 
    
    if(texture) {
        info->textureHandle = (u32)texture->id;
        assert(info->textureHandle);
        info->textureUVs = texture->uvCoords;
    } 

    info->pMat = pMat;
    info->vMat = vMat;
    info->mMat = mMat;

    return info;
}

void easyRender_pushScissors(RenderGroup *g, Rect2f rect, float zAt, Matrix4 offsetTransform, Matrix4 viewToClipMatrix, V2 viewPort) {
    Matrix4 modelMatrix = mat4();
    modelMatrix = Matrix4_translate(modelMatrix, transformPositionV3(v2ToV3(getCenter(rect), zAt), offsetTransform));
    V2 dim = getDim(rect);
    Matrix4 toClipSpace = Matrix4_scale(modelMatrix, v3(dim.x, dim.y, 1));
    toClipSpace = Mat4Mult(viewToClipMatrix, toClipSpace);

    V4 pointA = transformPositionV3ToV4(v3(-0.5f, -0.5f, zAt), toClipSpace);
    V4 pointB = transformPositionV3ToV4(v3(0.5f, 0.5f, zAt), toClipSpace);

    pointA.x /= pointA.w;
    pointA.y /= pointA.w;

    pointB.x /= pointB.w;
    pointB.y /= pointB.w;

    //Now in NDC space
    pointA.x = ((pointA.x + 1.0f) / 2.0f)*viewPort.x;
    pointA.y = ((pointA.y + 1.0f) / 2.0f)*viewPort.y;

    pointB.x = ((pointB.x + 1.0f) / 2.0f)*viewPort.x;
    pointB.y = ((pointB.y + 1.0f) / 2.0f)*viewPort.y;

    Rect2f *scissors = (Rect2f *)addElementInifinteAlloc_(&g->scissorsTests, 0);
    *scissors = rect2f(pointA.x, pointA.y, pointB.x, pointB.y);
    g->scissorsEnabled = true;
}


RenderProgram createRenderProgram(char *vShaderSource, char *fShaderSource) {
    RenderProgram result = {};
    
    result.valid = true;
    
    result.glShaderV = glCreateShader(GL_VERTEX_SHADER);
    renderCheckError();
    result.glShaderF = glCreateShader(GL_FRAGMENT_SHADER);
    renderCheckError();
    
    glShaderSource(result.glShaderV, 1, (const GLchar **)(&vShaderSource), 0);
    renderCheckError();
    glShaderSource(result.glShaderF, 1, (const GLchar **)(&fShaderSource), 0);
    renderCheckError();
    
    glCompileShader(result.glShaderV);
    renderCheckError();
    glCompileShader(result.glShaderF);
    renderCheckError();
    result.glProgram = glCreateProgram();
    renderCheckError();
    glAttachShader(result.glProgram, result.glShaderV);
    renderCheckError();
    glAttachShader(result.glProgram, result.glShaderF);
    renderCheckError();

    

    int max_attribs;
    glGetIntegerv (GL_MAX_VERTEX_ATTRIBS, &max_attribs);

    printf("max attribs: %d\n", max_attribs);

    glBindAttribLocation(result.glProgram, VERTEX_ATTRIB_LOCATION, "vertex");
    renderCheckError();
    glBindAttribLocation(result.glProgram, NORMAL_ATTRIB_LOCATION, "normal");
    renderCheckError();
    glBindAttribLocation(result.glProgram, UV_ATTRIB_LOCATION, "texUV");
    renderCheckError();
    glBindAttribLocation(result.glProgram, MODEL_ATTRIB_LOCATION, "M");
    renderCheckError();
    glBindAttribLocation(result.glProgram, VIEW_ATTRIB_LOCATION, "V");
    renderCheckError();
    glBindAttribLocation(result.glProgram, COLOR_ATTRIB_LOCATION, "color");
    renderCheckError();
    glBindAttribLocation(result.glProgram, UVATLAS_ATTRIB_LOCATION, "uvAtlas");
    renderCheckError();
    

    glLinkProgram(result.glProgram);
    renderCheckError();
    glUseProgram(result.glProgram);
    // renderCheckError();

    
    GLint success = 0;
    glGetShaderiv(result.glShaderV, GL_COMPILE_STATUS, &success);
    
    
    GLint success1 = 0;
    glGetShaderiv(result.glShaderF, GL_COMPILE_STATUS, &success1); 

    
    
    if(success == GL_FALSE || success1 == GL_FALSE) {
        result.valid = false;
        int  vlength,    flength,    plength;
        char vlog[2048];
        char flog[2048];
        char plog[2048];
        glGetShaderInfoLog(result.glShaderV, 2048, &vlength, vlog);
        glGetShaderInfoLog(result.glShaderF, 2048, &flength, flog);
        glGetProgramInfoLog(result.glProgram, 2048, &plength, plog);
        
        if(vlength || flength || plength) {
            
            printf("%s\n", vShaderSource);
            printf("%s\n", fShaderSource);
            printf("%s\n", vlog);
            printf("%s\n", flog);
            printf("%s\n", plog);
            
        }
    }
    
    assert(result.valid);
    
    return result;
}


typedef struct {
    s32 handle;
    bool valid;
} ShaderValInfo;

// ShaderValInfo getAttribFromProgram(RenderProgram *prog, char *name) {
//     ShaderValInfo result = {};
//     for(int i = 0; i < prog->attribCount; ++i) {
//         ShaderVal *val = prog->attribs + i;
//         if(cmpStrNull(name, val->name)) {
//             result.handle = val->handle;
//             result.valid = true;
//             break;
//         }
//     }
//     if(!result.valid) {
//         printf("%s\n", name);
//     }
//     // assert(result.valid);
//     return result;
// }

// ShaderValInfo getUniformFromProgram(RenderProgram *prog, char *name) {
//     ShaderValInfo result = {};
//     for(int i = 0; i < prog->uniformCount; ++i) {
//         ShaderVal *val = prog->uniforms + i;
//         if(cmpStrNull(name, val->name)) {
//             result.handle = val->handle;
//             //printf("%d\n", val->handle);
//             //assert(result.handle > 0);
//             result.valid = true;
//             break;
//         }
//     }
//     if(!result.valid) {
//         printf("%s\n", name);
//     }
//     assert(result.valid);
//     return result;
// }

GLuint renderGetUniformLocation(RenderProgram *program, char *name) {
    GLuint result = glGetUniformLocation(program->glProgram, name);
    renderCheckError();
    return result;
}

GLuint renderGetAttribLocation(RenderProgram *program, char *name) {
    GLuint result = glGetAttribLocation(program->glProgram, name);
    renderCheckError();
    return result;
    
}

void findAttribsAndUniforms(RenderProgram *prog, char *stream, bool isVertexShader) {
    EasyTokenizer tokenizer = lexBeginParsing(stream, EASY_LEX_OPTION_EAT_WHITE_SPACE);
    bool parsing = true;
    
    while(parsing) {
        char *at = tokenizer.src;
        EasyToken token = lexGetNextToken(&tokenizer);
        assert(at != tokenizer.src);
        switch(token.type) {
            case TOKEN_NULL_TERMINATOR: {
                parsing = false;
            } break;
            case TOKEN_WORD: {
                // lexPrintToken(&token);
                if(stringsMatchNullN("uniform", token.at, token.size)) {
                    lexGetNextToken(&tokenizer); //the type of it
                    token = lexGetNextToken(&tokenizer);
                    char *name = nullTerminate(token.at, token.size);
                    //printf("Uniform Found: %s\n", name);
                    assert(prog->uniformCount < arrayCount(prog->uniforms));
                    ShaderVal *val = prog->uniforms + prog->uniformCount++;
                    val->name = name;
                    // printf("%s\n", name);
                    val->handle = renderGetUniformLocation(prog, name);
                    
                }
                if(stringsMatchNullN("in", token.at, token.size) && isVertexShader) {
                    lexGetNextToken(&tokenizer); //this is the type
                    token = lexGetNextToken(&tokenizer);
                    char *name = nullTerminate(token.at, token.size);
                    // printf("Attrib Found: %s\n", name);
                    assert(prog->attribCount < arrayCount(prog->attribs));
                    ShaderVal *val = prog->attribs + prog->attribCount++;
                    val->name = name;
                    val->handle = renderGetAttribLocation(prog, name);
                    
                }
            } break;
            default: {
                //don't mind
            }
        }
    }
}


RenderProgram createProgramFromFile(char *vertexShaderFilename, char *fragmentShaderFilename, bool isFileName) {
    char *vertMemory = vertexShaderFilename;
    char *fragMemory = fragmentShaderFilename;
    if(isFileName) {
        vertMemory = (char *)loadShader(vertexShaderFilename).memory;
        fragMemory= (char *)loadShader(fragmentShaderFilename).memory;
    } 
    
#if DESKTOP
    char *shaderVersion = "#version 150\n";
#else
    char *shaderVersion = "#version 300 es\nprecision mediump float;\n";
#endif
    char *vertStream = concat(shaderVersion, vertMemory);
    char *fragStream = concat(shaderVersion, fragMemory);
    
    RenderProgram result = createRenderProgram(vertStream, fragStream);
    
    // findAttribsAndUniforms(&result, vertStream, true);
    // findAttribsAndUniforms(&result, fragStream, false);
    
    free(vertStream);
    free(fragStream);
    if(isFileName) {
        free(vertMemory);
        free(fragMemory);
    }
    
    return result;
}

Matrix4 projectionMatrixFOV(float FOV_degrees, float aspectRatio) { //where aspect ratio = width/height of frame buffer resolution
    float nearClip = NEAR_CLIP_PLANE;
    float farClip = FAR_CLIP_PLANE;
    
    float FOV_radians = (FOV_degrees*PI32) / 180.0f;
    float t = tan(FOV_radians/2);
    float r = t*aspectRatio;
    
    Matrix4 result = {{
            1 / r,  0,  0,  0,
            0,  1 / t,  0,  0,
            0,  0,  (farClip + nearClip)/(farClip - nearClip),  1, 
            0, 0,  (-2*nearClip*farClip)/(farClip - nearClip),  0
        }};
    
    return result;
}


Matrix4 OrthoMatrixToScreen_(int width, int height, float offsetX, float offsetY) {
    float a = 2.0f / (float)width; 
    float b = 2.0f / (float)height;
    
    float nearClip = NEAR_CLIP_PLANE;
    float farClip = FAR_CLIP_PLANE;
    
    Matrix4 result = {{
            a,  0,  0,  0,
            0,  b,  0,  0,
            0,  0,  (2)/(farClip - nearClip), 0, //definitley the projection coordinate. 
            offsetX, offsetY, -((farClip + nearClip)/(farClip - nearClip)),  1
        }};
    
    return result;
}

Matrix4 OrthoMatrixToScreen(int width, int height) {
    return OrthoMatrixToScreen_(width, height, 0, 0);
}

Matrix4 OrthoMatrixToScreen_BottomLeft(int width, int height) {
    return OrthoMatrixToScreen_(width, height, -1, -1);
}

static inline V3 screenSpaceToWorldSpace(Matrix4 perspectiveMat, V2 screenP, V2 resolution, float zAt, Matrix4 cameraToWorld) {
    
    V2 screenP_01 = v2(screenP.x / resolution.x, screenP.y / resolution.y);
    V2 ndcSpace = v2(lerp(-1, screenP_01.x, 1), lerp(-1, screenP_01.y, 1));

    V4 probePos = v4(0, 0, zAt, 1); //transform a position in the world to see what the clip space z would be
    
    V4 clipSpaceP_ = V4MultMat4(probePos, perspectiveMat);

    V4 clipSpaceP = v4(ndcSpace.x*clipSpaceP_.w, ndcSpace.y*clipSpaceP_.w, clipSpaceP_.z, clipSpaceP_.w);

    Matrix4 inverseMat = mat4();
    bool valid = mat4_inverse(perspectiveMat.E_, inverseMat.E_);
    assert(valid);
        
    V3 cameraSpaceP = V4MultMat4(clipSpaceP, inverseMat).xyz;

    V3 worldP = V4MultMat4(v3ToV4Homogenous(cameraSpaceP), cameraToWorld).xyz;

    return worldP;

}

// V2 transformWorldPToScreenP(V2 inputA, float zPos, V2 resolution, V2 screenDim, ProjectionType type) {
//     Matrix4 projMat;
//     if(type == ORTHO_MATRIX) {
//         projMat = OrthoMatrixToScreen(resolution.x, resolution.y);   
//     } else if(type == PERSPECTIVE_MATRIX) {
//         projMat = projectionMatrixToScreen(resolution.x, resolution.y);   
//     }
//     V4 screenSpace = transformPositionV3ToV4(v2ToV3(inputA, zPos), projMat);
//     //Homogeneous divide -> does the perspective divide. 
//     V3 screenSpaceV3 = v3(inputA.x / screenSpace.w, inputA.y / screenSpace.w, screenSpace.z / screenSpace.w);
    
//     //Map back onto the screen. 
//     V2 result = v2_plus(screenSpaceV3.xy, v2_scale(0.5f, resolution));
//     result.x /= resolution.x;
//     result.x *= screenDim.x;
//     result.y /= resolution.y;
//     result.y *= screenDim.y;
//     return result;
// }

void render_enableCullFace() {
    // glEnable(GL_CULL_FACE); 
    // glCullFace(GL_BACK);  
    // glFrontFace(GL_CW);  

}

void render_disableCullFace() {
    glDisable(GL_CULL_FACE);
}
// V3 transformScreenPToWorldP(V2 inputA, float zPos, V2 resolution, V2 screenDim, Matrix4 metresToPixels, V3 cameraPos) {
//     inputA.x /= screenDim.x;
//     inputA.y /= screenDim.y;
//     inputA.x *= resolution.x;
//     inputA.y *= resolution.y;
    
//     inputA = v2_minus(inputA, v2_scale(0.5f, resolution));
    
//     V4 trans = transformPositionV3ToV4(v2ToV3(inputA, zPos), mat4_transpose(projectionMatrixToScreen(resolution.x, resolution.y)));
    
//     inputA.x *= trans.w;
//     inputA.y *= trans.w;
    
//     V3 result = v2ToV3(inputA, zPos);
//     result = V4MultMat4(v4(result.x, result.y, result.z, 1), mat4_transpose(metresToPixels)).xyz;
//     result = v3_plus(result, cameraPos);
    
//     return result;
// }

void enableRenderer(int width, int height, Arena *arena) {
    
    globalRenderGroup = pushStruct(arena, RenderGroup);
#if RENDER_BACKEND == OPENGL_BACKEND
    glViewport(0, 0, width, height);
    renderCheckError();
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    renderCheckError();
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);//Non premultiplied alpha textures! 
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    renderCheckError();
    //Premultiplied alpha textures! 
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);  
    renderCheckError();
    glDepthFunc(GL_LEQUAL);///GL_LESS);//////GL_LEQUAL);//
    
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);

    render_enableCullFace();
    //SRGB TEXTURE???
    // glEnable(GL_SAMPLE_ALPHA_TO_ONE);
    // glEnable(GL_DEBUG_OUTPUT);
    // renderCheckError();
    
#if DESKTOP
    glEnable(GL_MULTISAMPLE);
#endif
    
    phongProgram = createProgramFromFile(vertex_model_shader, frag_model_shader, false);
    renderCheckError();

    circleProgram = createProgramFromFile(vertex_shader_tex_attrib_shader, frag_circle_shader, false);
    renderCheckError();

    glossProgram = createProgramFromFile(vertex_gloss_shader, frag_gloss_shader, false);
    renderCheckError();

    skyboxProgram = createProgramFromFile(vertex_skybox_shader, frag_skybox_shader, false);
    renderCheckError();
    
    textureProgram = createProgramFromFile(vertex_shader_tex_attrib_shader, fragment_shader_texture_shader, false);
    renderCheckError();

    skyQuadProgram = createProgramFromFile(vertex_sky_quad_shader, frag_sky_quad_shader, false);
    renderCheckError();

    terrainProgram = createProgramFromFile(vertex_terrain_shader, frag_terrain_shader, false);
    renderCheckError();

    toneMappingProgram = createProgramFromFile(vertex_fullscreen_quad_shader, frag_tone_map_shader, false);
    renderCheckError();

    blurProgram = createProgramFromFile(vertex_fullscreen_quad_shader, frag_blur_shader, false);
    renderCheckError();

    colorWheelProgram = createProgramFromFile(vertex_shader_tex_attrib_shader, frag_color_wheel_shader, false);
    renderCheckError();
    
    
#endif

    //Init global Render Group
    initRenderGroup(globalRenderGroup);
    ////
}


//alpha is at 24 place
static inline V4 hexARGBTo01Color(unsigned int color) {
    V4 result = {};
    
    result.x = (float)((color >> 16) & 0xFF) / 255.0f; //red
    result.z = (float)((color >> 0) & 0xFF) / 255.0f;
    result.y = (float)((color >> 8) & 0xFF) / 255.0f;
    result.w = (float)((color >> 24) & 0xFF) / 255.0f;
    return result;
}

typedef enum {
    RENDER_TEXTURE_DEFAULT,
    RENDER_TEXTURE_HDR,
} RenderTextureFlag;


GLuint renderLoadTexture(int width, int height, void *imageData, RenderTextureFlag flags) {
#if RENDER_BACKEND == OPENGL_BACKEND
    GLuint resultId;
    glGenTextures(1, &resultId);
    renderCheckError();
    
    glBindTexture(GL_TEXTURE_2D, resultId);
    renderCheckError();
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    renderCheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    renderCheckError();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    renderCheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    renderCheckError();
    
    
    GLenum pixelType = GL_UNSIGNED_BYTE;
    GLenum internalFormat = GL_RGBA8;
    if(flags & RENDER_TEXTURE_HDR) {
        pixelType = GL_FLOAT; //NOTE: I'm not sure this does anything since we normally aren't passing in any iamge data for this function
        internalFormat = GL_RGBA16F; //NOTE: Out HDR buffer
    } 

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_RGBA, pixelType, imageData);
    renderCheckError();
    
    glBindTexture(GL_TEXTURE_2D, 0);
    renderCheckError();
#endif
    return resultId;
}

#define RENDER_MAX_COLOR_ATTACHMENTS 3


typedef enum {
    FRAMEBUFFER_COLOR = 1 << 0, //default everyone has color. Just to refer with later
    FRAMEBUFFER_DEPTH = 1 << 1,
    FRAMEBUFFER_STENCIL = 1 << 2,
    FRAMEBUFFER_HDR = 1 << 3,

} FrameBufferFlag;

typedef struct {
    int colorBufferCount;
    GLuint bufferId;
    GLuint textureIds[RENDER_MAX_COLOR_ATTACHMENTS];
    GLuint depthId;
    int flags;

    int width;
    int height;

} FrameBuffer;





void renderReadPixels(u32 bufferId, int x0, int y0,
                      int x1,
                      int y1,
                      u32 layout,
                      u32 format,
                      u8 *stream) {
    
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)bufferId);
    
    glReadPixels(x0, y0,
                 x1, y1,
                 layout,
                 format,
                 stream);
}

void renderDeleteTextures(int count, GLuint *handle) {
	glDeleteTextures(1, handle);
}

void renderDeleteFramebuffers(int count, GLuint *handle) {
	glDeleteFramebuffers(1, handle);
}

void deleteFrameBuffer(FrameBuffer *frameBuffer) {
    if(frameBuffer->depthId != -1) {
        renderDeleteTextures(1, &frameBuffer->depthId);
    }
    
    renderDeleteFramebuffers(1, &frameBuffer->bufferId);
    for(int i = 0; i < frameBuffer->colorBufferCount; ++i) {
        renderDeleteTextures(1, &frameBuffer->textureIds[i]);
       
    }
}


FrameBuffer createFrameBuffer(int width, int height, int flags, int numColorBuffers) {   
    assert(numColorBuffers <= RENDER_MAX_COLOR_ATTACHMENTS);
    GLuint frameBufferHandle = 1;
    glGenFramebuffers(1, &frameBufferHandle);
    renderCheckError();
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferHandle);
    renderCheckError();
    
    GLuint depthId = -1;

    FrameBuffer result = {};
    result.flags = flags;
    
    RenderTextureFlag textureType = (flags & FRAMEBUFFER_HDR) ? RENDER_TEXTURE_HDR : RENDER_TEXTURE_DEFAULT;
    for(int cIndex = 0; cIndex < numColorBuffers; ++cIndex) {
        assert(result.colorBufferCount < RENDER_MAX_COLOR_ATTACHMENTS);
        
        result.textureIds[result.colorBufferCount++] = renderLoadTexture(width, height, 0, textureType);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + cIndex, GL_TEXTURE_2D, result.textureIds[cIndex], 0);
        renderCheckError();
        
    }

    if(numColorBuffers > 1) {
        unsigned int attachments[RENDER_MAX_COLOR_ATTACHMENTS] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
        glDrawBuffers(numColorBuffers, attachments);    
    }
     
    
    if(flags & FRAMEBUFFER_DEPTH) {
        glGenTextures(1, &depthId);
        renderCheckError();
        
        glBindTexture(GL_TEXTURE_2D, depthId);
        renderCheckError();
        
        if(!(flags & FRAMEBUFFER_STENCIL)) { //Just depth buffer
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
            renderCheckError();
            
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthId, 0);
            renderCheckError();    
        } else {
            //CREATE STENCIL BUFFER ALONG WITH DEPTH BUFFER
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
            renderCheckError();
            
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthId, 0);
            renderCheckError();    
        } 
    }
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    
    result.bufferId = frameBufferHandle;
    result.depthId = depthId; 
    result.width = width;
    result.height = height;
    
    return result;
}


FrameBuffer createFrameBufferMultiSample(int width, int height, int flags, int sampleCount) {
#if !DESKTOP
    FrameBuffer result = createFrameBuffer(width, height, flags);
#else

    FrameBuffer result = {};
    GLuint textureId;
    glGenTextures(1, &textureId);
    renderCheckError();
    result.flags = flags;
    
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureId);
    renderCheckError();
    
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, sampleCount, GL_RGBA8, width, height, GL_FALSE);
    renderCheckError();
    
    GLuint frameBufferHandle;
    glGenFramebuffers(1, &frameBufferHandle);
    renderCheckError();
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferHandle);
    renderCheckError();
    
    if(flags) {
        glGenTextures(1, &result.depthId);
        renderCheckError();
        
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, result.depthId);
        renderCheckError();
        
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, sampleCount, GL_DEPTH24_STENCIL8, width, height, GL_FALSE);
        renderCheckError();
        
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, result.depthId, 0);
        renderCheckError();
        
    }
    
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, 
                           textureId, 0);
    renderCheckError();
    
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    
    result.textureIds[result.colorBufferCount++] = textureId;
    result.bufferId = frameBufferHandle;
#endif    
    return result;
}

static inline void clearBufferAndBind(GLuint frameBufferId, V4 color, int flags, RenderGroup *g) {
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)frameBufferId); 
    
    if(g) {
        setFrameBufferId(g, frameBufferId);
    }
    
    glClearColor(color.x, color.y, color.z, color.w);
    renderCheckError();

    GLenum clearFlags = GL_COLOR_BUFFER_BIT;
    if(flags & FRAMEBUFFER_STENCIL) {
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        renderCheckError();
        glStencilMask(0xFF);
        renderCheckError();
        clearFlags |= GL_STENCIL_BUFFER_BIT;
    }

     if(flags & FRAMEBUFFER_DEPTH) {
        glClearDepthf(1.0f);
        renderCheckError();
        clearFlags |= GL_DEPTH_BUFFER_BIT;
    }
   
    glClear(clearFlags); 
    renderCheckError();
}

static inline void renderSetFrameBuffer(GLuint frameBufferId, RenderGroup *g) {
    if(g) {
        setFrameBufferId(g, frameBufferId);
    }
}

typedef enum {
    DRAWCALL_SINGLE,
    DRAWCALL_INSTANCED,   
} DrawCallType;

static inline void addInstanceAttribForMatrix(int index, GLuint attribLoc, int numOfFloats, size_t offsetForStruct, size_t offsetInStruct, int divisor) {
    
    glEnableVertexAttribArray(attribLoc + index);  
    renderCheckError();
    
    glVertexAttribPointer(attribLoc + index, numOfFloats, GL_FLOAT, GL_FALSE, offsetForStruct, ((char *)0) + offsetInStruct + (4*sizeof(float)*index));
    renderCheckError();
    glVertexAttribDivisor(attribLoc + index, divisor);
    renderCheckError();
}

static inline void addInstancingAttrib (GLuint attribLoc, int numOfFloats, size_t offsetForStruct, size_t offsetInStruct, int divisor) {
        
    // printf("attrib loc: %u\n", attribLoc);
    assert(attribLoc < GL_MAX_VERTEX_ATTRIBS);
    assert(offsetForStruct > 0);
    if(numOfFloats == 16) {
        addInstanceAttribForMatrix(0, attribLoc, 4, offsetForStruct, offsetInStruct, divisor);
        addInstanceAttribForMatrix(1, attribLoc, 4, offsetForStruct, offsetInStruct, divisor);
        addInstanceAttribForMatrix(2, attribLoc, 4, offsetForStruct, offsetInStruct, divisor);
        addInstanceAttribForMatrix(3, attribLoc, 4, offsetForStruct, offsetInStruct, divisor);
    } else {
        glEnableVertexAttribArray(attribLoc);  
        renderCheckError();
        
        assert(numOfFloats <= 4);
        glVertexAttribPointer(attribLoc, numOfFloats, GL_FLOAT, GL_FALSE, offsetForStruct, ((char *)0) + offsetInStruct);
        renderCheckError();
        
        glVertexAttribDivisor(attribLoc, divisor);
        renderCheckError();
    }
}

static void renderDrawCube(RenderGroup *group, EasyMaterial *material, V4 colorTint) {
    pushRenderItem(&globalCubeVaoHandle, group, globalCubeVertexData, arrayCount(globalCubeVertexData), globalCubeIndicesData, arrayCount(globalCubeIndicesData), group->currentShader, SHAPE_MODEL, 0, group->modelTransform, group->viewTransform, group->projectionTransform, colorTint, group->modelTransform.E_[14], material);
}

static void renderDrawSprite(RenderGroup *group, Texture *sprite, V4 colorTint) {
    pushRenderItem(&globalQuadVaoHandle, group, globalQuadPositionData, arrayCount(globalQuadPositionData), globalQuadIndicesData, arrayCount(globalQuadIndicesData), group->currentShader, SHAPE_TEXTURE, sprite, group->modelTransform, group->viewTransform, group->projectionTransform, colorTint, group->modelTransform.E_[14], 0);
}

static void renderDrawQuad(RenderGroup *group, V4 colorTint) {
    pushRenderItem(&globalQuadVaoHandle, group, globalQuadPositionData, arrayCount(globalQuadPositionData), globalQuadIndicesData, arrayCount(globalQuadIndicesData), group->currentShader, SHAPE_TEXTURE, &globalWhiteTexture, group->modelTransform, group->viewTransform, group->projectionTransform, colorTint, group->modelTransform.E_[14], 0);
}

#if DEVELOPER_MODE
static inline void renderDrawCubeOutline(RenderGroup *g, Rect3f b, V4 color) {
    V3 dim = getDimRect3f(b);

    V3 c = getCenterRect3f(b);

    Matrix4 tempModelMat = g->modelTransform; //take snapshot of model matrix to restore at end
    float lineSize = 0.2f;
    //NOTE: rotate & scale
    Matrix4 scalex = Matrix4_scale(mat4(), v3(dim.x, lineSize, lineSize));
    Matrix4 scaley = Matrix4_scale(mat4(), v3(lineSize, dim.y, lineSize));
    Matrix4 scalez = Matrix4_scale(mat4(), v3(lineSize, lineSize, dim.z));
    
    Matrix4 m = g->modelTransform;
    float hx = b.min.x;
    float hy = b.min.y;
    float hz = b.min.z;
    float h1x = b.max.x;
    float h1y = b.max.y;
    float h1z = b.max.z;
    Matrix4 l0 = Mat4Mult(m, Matrix4_translate(scalez, v3(hx, hy, c.z))); 
    Matrix4 l1 = Mat4Mult(m, Matrix4_translate(scalez, v3(h1x, hy, c.z))); 
    Matrix4 l2 = Mat4Mult(m, Matrix4_translate(scalex, v3(c.x, hy, hz))); 
    Matrix4 l3 = Mat4Mult(m, Matrix4_translate(scalex, v3(c.x, hy, h1z))); 

    Matrix4 l4 = Mat4Mult(m, Matrix4_translate(scalez, v3(hx, h1y, c.z))); 
    Matrix4 l5 = Mat4Mult(m, Matrix4_translate(scalez, v3(h1x, h1y, c.z))); 
    Matrix4 l6 = Mat4Mult(m, Matrix4_translate(scalex, v3(c.x, h1y, hz))); 
    Matrix4 l7 = Mat4Mult(m, Matrix4_translate(scalex, v3(c.x, h1y, h1z))); 

    Matrix4 l8 = Mat4Mult(m, Matrix4_translate(scaley, v3(hx, c.y, hz))); 
    Matrix4 l9 = Mat4Mult(m, Matrix4_translate(scaley, v3(h1x, c.y, hz))); 
    Matrix4 l10 = Mat4Mult(m, Matrix4_translate(scaley, v3(hx, c.y, h1z))); 
    Matrix4 l11 = Mat4Mult(m, Matrix4_translate(scaley, v3(h1x, c.y, h1z))); 

    setModelTransform(g, l0);
    renderDrawCube(g, &globalWhiteMaterial, COLOR_BLUE);
    setModelTransform(g, l1);
    renderDrawCube(g, &globalWhiteMaterial, COLOR_BLUE);
    setModelTransform(g, l2);
    renderDrawCube(g, &globalWhiteMaterial, COLOR_BLUE);
    setModelTransform(g, l3);
    renderDrawCube(g, &globalWhiteMaterial, COLOR_BLUE);

    setModelTransform(g, l4);
    renderDrawCube(g, &globalWhiteMaterial, COLOR_RED);
    setModelTransform(g, l5);
    renderDrawCube(g, &globalWhiteMaterial, COLOR_RED);
    setModelTransform(g, l6);
    renderDrawCube(g, &globalWhiteMaterial, COLOR_RED);
    setModelTransform(g, l7);
    renderDrawCube(g, &globalWhiteMaterial, COLOR_RED);

    setModelTransform(g, l8);
    renderDrawCube(g, &globalWhiteMaterial, COLOR_YELLOW);
    setModelTransform(g, l9);
    renderDrawCube(g, &globalWhiteMaterial, COLOR_YELLOW);
    setModelTransform(g, l10);
    renderDrawCube(g, &globalWhiteMaterial, COLOR_YELLOW);
    setModelTransform(g, l11);
    renderDrawCube(g, &globalWhiteMaterial, COLOR_YELLOW);

    //restore state
    g->modelTransform = tempModelMat;


}
#endif

static inline void renderMesh(RenderGroup *group, EasyMesh *mesh, V4 colorTint, EasyMaterial *material) {  
    pushRenderItem(&mesh->vaoHandle, group, 0, 0, 0, mesh->vaoHandle.indexCount, group->currentShader, SHAPE_MODEL, 0, group->modelTransform, group->viewTransform, group->projectionTransform, colorTint, group->modelTransform.E_[14], material);
#if DEVELOPER_MODE
    if(DEBUG_drawBounds) {
        renderDrawCubeOutline(group, mesh->bounds, COLOR_PINK);  
    }
#endif

}

static inline void renderTerrain(RenderGroup *group, VaoHandle *bufferHandle, V4 colorTint, EasyMaterial *material, void *packet) {
    RenderItem * i = pushRenderItem(bufferHandle, group, 0, 0, 0, bufferHandle->indexCount, &terrainProgram, SHAPE_TERRAIN, 0, group->modelTransform, group->viewTransform, group->projectionTransform, colorTint, group->modelTransform.E_[14], material);
    i->dataPacket = packet; 
}

static inline void renderModel(RenderGroup *group, EasyModel *model, V4 colorTint) {  
    for(int meshIndex = 0; meshIndex < model->meshCount; ++meshIndex) {
        EasyMesh *thisMesh = model->meshes[meshIndex];
        VaoHandle *bufferHandle = &thisMesh->vaoHandle;
        pushRenderItem(bufferHandle, group, 0, 0, 0, bufferHandle->indexCount, group->currentShader, SHAPE_MODEL, 0, group->modelTransform, group->viewTransform, group->projectionTransform, colorTint, group->modelTransform.E_[14], thisMesh->material);
#if DEVELOPER_MODE
        if(DEBUG_drawBounds) {
            renderDrawCubeOutline(group, thisMesh->bounds, COLOR_PINK);  
        }
#endif
    }

#if DEVELOPER_MODE
    if(DEBUG_drawBounds) {
        renderDrawCubeOutline(group, model->bounds, COLOR_PINK);  
    }
#endif
   
}

static inline void initVao(VaoHandle *bufferHandles, Vertex *triangleData, int triCount, unsigned int *indicesData, int indexCount) {
    if(!bufferHandles->valid) {
        glGenVertexArrays(1, &bufferHandles->vaoHandle);
        renderCheckError();
        glBindVertexArray(bufferHandles->vaoHandle);
        renderCheckError();
        
        GLuint vertices;
        GLuint indices;
        
        glGenBuffers(1, &vertices);
        renderCheckError();
        
        glBindBuffer(GL_ARRAY_BUFFER, vertices);
        renderCheckError();
        
        glBufferData(GL_ARRAY_BUFFER, triCount*sizeof(Vertex), triangleData, GL_STATIC_DRAW);
        renderCheckError();
        
        glGenBuffers(1, &indices);
        renderCheckError();
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices);
        renderCheckError();
        
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount*sizeof(unsigned int), indicesData, GL_STATIC_DRAW);
        renderCheckError();
        
        bufferHandles->indexCount = indexCount;
        
        assert(!bufferHandles->valid);
        bufferHandles->valid = true;
        
        //these can also be retrieved before hand to speed up the process!!!
        GLint vertexAttrib = VERTEX_ATTRIB_LOCATION;
        renderCheckError();
        // ShaderValInfo texUV_val= 2;
        // renderCheckError();

        // ShaderValInfo normals = 1;
        // renderCheckError();
        
        // if(texUV_val.valid) {
            
            GLint texUVAttrib = UV_ATTRIB_LOCATION;
            glEnableVertexAttribArray(texUVAttrib);  
            renderCheckError();
            unsigned int uvByteOffset = (intptr_t)(&(((Vertex *)0)->texUV));
            glVertexAttribPointer(texUVAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), ((char *)0) + uvByteOffset);
            renderCheckError();
        // }

        // if (normals.valid) {
            GLint normalsAttrib = NORMAL_ATTRIB_LOCATION;
            glEnableVertexAttribArray(normalsAttrib);  
            renderCheckError();
            unsigned int normalOffset = (intptr_t)(&(((Vertex *)0)->normal));
            glVertexAttribPointer(normalsAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), ((char *)0) + normalOffset);
            renderCheckError();
        // }
        
        glEnableVertexAttribArray(vertexAttrib);  
        renderCheckError();
        glVertexAttribPointer(vertexAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
        renderCheckError();

        glGenBuffers(1, &bufferHandles->vboInstanceData);
        renderCheckError();

        glBindBuffer(GL_ARRAY_BUFFER, bufferHandles->vboInstanceData);
        renderCheckError();

        GLint mAttrib = MODEL_ATTRIB_LOCATION;
        renderCheckError();
        GLint vAttrib = VIEW_ATTRIB_LOCATION;
        renderCheckError();
        GLint colorAttrib = COLOR_ATTRIB_LOCATION;
        renderCheckError();
        
        size_t offsetForStruct = sizeof(float)*(16+16+4+4); 
        
        //matrix plus vector4 plus vector4
        addInstancingAttrib (mAttrib, 16, offsetForStruct, 0, 1);
        // printf("m attrib: %d\n", mAttrib);
        addInstancingAttrib (vAttrib, 16, offsetForStruct, sizeof(float)*16, 1);
        // printf("v attrib: %d\n", vAttrib);
        // addInstancingAttrib (pAttrib, 16, offsetForStruct, sizeof(float)*32, 1);
        // printf("p attrib: %d\n", pAttrib);
        addInstancingAttrib (colorAttrib, 4, offsetForStruct, sizeof(float)*32, 1);
        // printf("color attrib: %d\n", colorAttrib);
        // if(hasUvs) 

        addInstancingAttrib (UVATLAS_ATTRIB_LOCATION, 4, offsetForStruct, sizeof(float)*36, 1);
        renderCheckError();
        
        assert(offsetForStruct == sizeof(float)*40);
        
        glBindVertexArray(0);
        
        glDeleteBuffers(1, &vertices);
        glDeleteBuffers(1, &indices);
    }
}


void easy_BindTexture(char *uniformName, int slotId, GLint textureId, RenderProgram *program) {

    // GLint texUniform = getUniformFromProgram(program, uniformName).handle;
    GLint texUniform = glGetUniformLocation(program->glProgram, uniformName); 
    renderCheckError();
    
    glUniform1i(texUniform, slotId);
    renderCheckError();
    
    glActiveTexture(GL_TEXTURE0 + slotId);
    renderCheckError();
    
    // printf("texture id: %d\n", textureId);
    glBindTexture(GL_TEXTURE_2D, textureId); 
    renderCheckError();
}

static inline void easyRender_blurBuffer(FrameBuffer *src, FrameBuffer *dest, int colorAttachementId) {
    int blurCount = 10;
    float horizontal = 0.0f;

#if OPENGL_BACKEND
    glUseProgram(blurProgram.glProgram);
    renderCheckError();

    initVao(&globalFullscreenQuadVaoHandle, globalFullscreenQuadPositionData, arrayCount(globalFullscreenQuadPositionData), globalFullscreenQuadIndicesData, arrayCount(globalFullscreenQuadIndicesData));
    
    glBindVertexArray(globalFullscreenQuadVaoHandle.vaoHandle);
    renderCheckError();

    glDepthFunc(GL_ALWAYS);
    
    ////if the buffer is goim to blurred we can cache this
    FrameBuffer tempBuffer =  createFrameBuffer(src->width, src->height, FRAMEBUFFER_COLOR | FRAMEBUFFER_HDR, 1);

    for (int i = 0; i < blurCount; ++i)
    {
        GLuint texId = 0;
        if(i % 2) {
            glBindFramebuffer(GL_FRAMEBUFFER, dest->bufferId); 
            renderCheckError();
            texId = tempBuffer.textureIds[0];
            horizontal = 1.0f;
        } else {
            assert(i != (blurCount - 1));
            glBindFramebuffer(GL_FRAMEBUFFER, tempBuffer.bufferId); 
            renderCheckError();
            if(i == 0) {
                //first time we get the texture from the src
                texId = src->textureIds[colorAttachementId];    
            } else {
                texId = dest->textureIds[0];
            }
            
            horizontal = 0.0f;
        }

        glUniform1f(glGetUniformLocation(blurProgram.glProgram, "horizontal"),  horizontal);
        renderCheckError();

        easy_BindTexture("image", 1, texId, &blurProgram);

        glDrawElements(GL_TRIANGLES, globalFullscreenQuadVaoHandle.indexCount, GL_UNSIGNED_INT, 0); 
        renderCheckError();
    }

    glDepthFunc(GL_LESS);

    glBindVertexArray(0);
    renderCheckError();    

    glUseProgram(0);
    renderCheckError();

    glBindFramebuffer(GL_FRAMEBUFFER, 0); 
    deleteFrameBuffer(&tempBuffer);


#endif

}


static V2 globalBlurDir = {};
void drawVao(VaoHandle *bufferHandles, RenderProgram *program, ShapeType type, u32 textureId, DrawCallType drawCallType, int instanceCount, EasyMaterial *material, RenderGroup *group, Matrix4 *projectionTransform, void *dataPacket) {
    
    assert(bufferHandles);
    assert(bufferHandles->valid);
    
    glUseProgram(program->glProgram);
    renderCheckError();
    
    glBindVertexArray(bufferHandles->vaoHandle);
    renderCheckError();
    
#if 0
    glBindBuffer(GL_ARRAY_BUFFER, bufferHandles->vboHandle);
    renderCheckError();
#endif

    if(type == SHAPE_COLOR_WHEEL) {
        EasyRender_ColorWheel_DataPacket *packet = (EasyRender_ColorWheel_DataPacket *)dataPacket;
        glUniformMatrix4fv(glGetUniformLocation(program->glProgram, "projection"), 1, GL_FALSE, projectionTransform->E_);
        renderCheckError();
        glUniform1f(glGetUniformLocation(program->glProgram, "value"),  packet->value);
        renderCheckError();
    }


    if(type == SHAPE_TERRAIN) {
        EasyTerrainDataPacket *packet = (EasyTerrainDataPacket *)dataPacket;

        glUniformMatrix4fv(glGetUniformLocation(program->glProgram, "projection"), 1, GL_FALSE, projectionTransform->E_);
        renderCheckError();

        // glUniformMatrix4fv(glGetUniformLocation(program->glProgram, "projection"), 1, GL_FALSE, group->projectionTransform.E_);
        // renderCheckError();
        // glUniformMatrix4fv(glGetUniformLocation(program->glProgram, "view"), 1, GL_FALSE, group->viewTransform.E_);
        // renderCheckError();
        // glUniformMatrix4fv(glGetUniformLocation(program->glProgram, "model"), 1, GL_FALSE, group->modelTransform.E_);
        // renderCheckError();

        for(int i = 0; i < packet->textureCount; ++i) {
            Texture *t = packet->textures[i];
            char buffer[64];
            sprintf(buffer, "%s%d", "tex", i);
            easy_BindTexture(buffer, i, t->id, program);
        }

        easy_BindTexture("blendMap", 5, packet->blendMap->id, program);

        glUniform2f(glGetUniformLocation(program->glProgram, "dim"),  packet->dim.x, packet->dim.y);
        renderCheckError();
    }

    if(type == SHAPE_SKY_QUAD) {
        glUniformMatrix4fv(glGetUniformLocation(program->glProgram, "camera_to_world_transform"), 1, GL_FALSE, group->cameraToWorldTransform.E_);
        renderCheckError();

        glUniform4f(glGetUniformLocation(program->glProgram, "skyColor"),  globalSkyColor.x, globalSkyColor.y, globalSkyColor.z, globalSkyColor.w);
        renderCheckError();

        glUniform3f(glGetUniformLocation(program->glProgram, "eyePos"),  0, 0, 0);
        renderCheckError();

        V3 direction = easyTransform_getZAxis(group->lights[0]->T);
        glUniform3f(glGetUniformLocation(program->glProgram, "sunDirection"),  direction.x, direction.y, direction.z);//group->eyePos.x, group->eyePos.y, group->eyePos.z);
        renderCheckError();

        glUniform1f(glGetUniformLocation(program->glProgram, "fov"),  group->fov/360.0f*PI32);
        renderCheckError();

        glUniform1f(glGetUniformLocation(program->glProgram, "aspectRatio"),  group->aspectRatio);
        renderCheckError();

        glUniform1f(glGetUniformLocation(program->glProgram, "nearZ"),  (float)NEAR_CLIP_PLANE);
        renderCheckError();

        glUniform1f(glGetUniformLocation(program->glProgram, "timeScale"),  globalTimeSinceStart);
        renderCheckError();



    }

    if(type == SHAPE_SKYBOX) {
        GLint texUniform = glGetUniformLocation(program->glProgram, "skybox"); 
        renderCheckError();
        
        glUniform1i(texUniform, 1);
        renderCheckError();

        glActiveTexture(GL_TEXTURE1);
        renderCheckError();
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);
        renderCheckError();

        Matrix4 result = mat4_noTranslate(group->viewTransform);
        // Matrix4 result = group->viewTransform;
        // printf("%f %f %f \n", result.a.x, result.a.y, result.a.z);
        // printf("%f %f %f \n", result.b.x, result.b.y, result.b.z);
        // printf("%f %f %f \n", result.c.x, result.c.y, result.c.z);
        // printf("%f %f %f \n", result.d.x, result.d.y, result.d.z);
        // printf("%s\n", "----------------");

        glUniformMatrix4fv(glGetUniformLocation(program->glProgram, "projection"), 1, GL_FALSE, projectionTransform->E_);
        renderCheckError();
        glUniformMatrix4fv(glGetUniformLocation(program->glProgram, "view"), 1, GL_FALSE, result.E_);
        renderCheckError();
    }

    if(type == SHAPE_TONE_MAP) {
            
        EasyRender_ToneMap_Bloom_DataPacket *bloomPacket = (EasyRender_ToneMap_Bloom_DataPacket *)dataPacket;
        GLint exposureConstant = glGetUniformLocation(program->glProgram, "exposure"); 
        renderCheckError();
        glUniform1f(exposureConstant, bloomPacket->exposure);
        renderCheckError();

        easy_BindTexture("tex", 1, bloomPacket->mainBufferTexId, program);
        renderCheckError();

        easy_BindTexture("bloom", 2, bloomPacket->bloomBufferTexId, program);
        renderCheckError();
    }

    if(type == SHAPE_MODEL) {
        glUniformMatrix4fv(glGetUniformLocation(program->glProgram, "projection"), 1, GL_FALSE, projectionTransform->E_);
        renderCheckError();
        assert(material);

        //NOTE: Always have a texture available but default it to white
        assert(material->diffuseMap);
        assert(material->specularMap);

        easy_BindTexture("material.diffuse", 1, material->diffuseMap->id, program);
        renderCheckError();
        
        glUniform4f(glGetUniformLocation(program->glProgram, "material.ambientDefault"),  material->defaultAmbient.x, material->defaultAmbient.y, material->defaultAmbient.z, material->defaultAmbient.w);
        glUniform4f(glGetUniformLocation(program->glProgram, "material.diffuseDefault"),  material->defaultDiffuse.x, material->defaultDiffuse.y, material->defaultDiffuse.z, material->defaultDiffuse.w);
        glUniform4f(glGetUniformLocation(program->glProgram, "material.specularDefault"),  material->defaultSpecular.x, material->defaultSpecular.y, material->defaultSpecular.z, material->defaultSpecular.w);
            // printf("%s %f %f %f\n", material->defaultSpecular.x, material->defaultSpecular.y, material->defaultSpecular.z);
        easy_BindTexture("material.specular", 2, material->specularMap->id, program);

        //NOTE(ol): Normal map stuff here
        // if(getUniformFromProgram(program, "material.normalMap").valid) {
        //     if(material->normal) {
        //         easy_BindTexture("material.normals", 1, material->normalMap->id, program);    
        //     } else {
        //         //dummy texture if there isn't a normal map
        //         easy_BindTexture("material.normals", 1, material->diffuseMap->id, program);
        //     }
        // }

        // GLint specularConstant = getUniformFromProgram(program, "material.specularConstant").handle;
        GLint specularConstant = glGetUniformLocation(program->glProgram, "material.specularConstant"); 
        renderCheckError();
        glUniform1f(specularConstant, material->specularConstant);
        // printf("%f\n", material->specularConstant);
        renderCheckError();

        GLint eyepos = glGetUniformLocation(program->glProgram, "eye_worldspace"); 
        renderCheckError();
        glUniform3f(eyepos, group->eyePos.x, group->eyePos.y, group->eyePos.z);
        renderCheckError();

        //lights 
        for(int i = 0; i < group->lightCount; ++i) {
            EasyLight *light = group->lights[i];

            // glUniform3fv
            char str[512]; 
            sprintf(str, "lights[%d].direction", i);
            // GLint lightDir = getUniformFromProgram(program, str).handle;
            GLint lightDir = glGetUniformLocation(program->glProgram, str); 
            renderCheckError();
            V3 direction = easyTransform_getZAxis(light->T);
            float pointOfDirectional = (light->type != EASY_LIGHT_DIRECTIONAL) ? 1.0f : 0.0f; //For homogenous coordinates
            glUniform4f(lightDir, direction.x, direction.y, direction.z, pointOfDirectional);
            renderCheckError();

            sprintf(str, "lights[%d].ambient", i);
            // GLint amb = getUniformFromProgram(program, str).handle;
            GLint amb = glGetUniformLocation(program->glProgram, str); 
            renderCheckError();
            glUniform4f(amb, light->color.x, light->color.y, light->color.z, 1);
            renderCheckError();

            sprintf(str, "lights[%d].diffuse", i);
            // GLint diff = getUniformFromProgram(program, str).handle;
            GLint diff = glGetUniformLocation(program->glProgram, str); 
            renderCheckError();
            glUniform4f(diff, light->color.x, light->color.y, light->color.z, 1);
            renderCheckError();

            sprintf(str, "lights[%d].specular", i);
            // GLint spec = getUniformFromProgram(program, str).handle;
            GLint spec = glGetUniformLocation(program->glProgram, str); 
            renderCheckError();
            glUniform4f(spec, light->color.x, light->color.y, light->color.z, 1);
            renderCheckError();
            
        }
        

        
    }

    if(type == SHAPE_TEXTURE) {

        GLuint eyeLoc = glGetUniformLocation(program->glProgram, "eyePosition");
        if(eyeLoc >= 0) { //has this value
            glUniform3f(eyeLoc, group->eyePos.x, group->eyePos.y, group->eyePos.z);
            renderCheckError();    
        }
        

        easy_BindTexture("tex", 3, textureId, program);
        glUniformMatrix4fv(glGetUniformLocation(program->glProgram, "projection"), 1, GL_FALSE, projectionTransform->E_);
        renderCheckError();

    } 
    
    if(drawCallType == DRAWCALL_SINGLE) {
        glDrawElements(GL_TRIANGLES, bufferHandles->indexCount, GL_UNSIGNED_INT, 0); 
        renderCheckError();
    } else if(drawCallType == DRAWCALL_INSTANCED) {
        
        glDrawElementsInstanced(GL_TRIANGLES, bufferHandles->indexCount, GL_UNSIGNED_INT, 0, instanceCount); 
        renderCheckError();
#if DEVELOPER_MODE
        if(DEBUG_drawWireFrame) {
            glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
            glDrawElementsInstanced(GL_TRIANGLES, bufferHandles->indexCount, GL_UNSIGNED_INT, 0, instanceCount); 
            renderCheckError();
            glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );    
        }
#endif
    }
    
    glBindVertexArray(0);

    renderCheckError();    
    glUseProgram(0);
    renderCheckError();
    
}


void renderDrawBillBoardQuad(RenderGroup *group, EasyMaterial *material, V4 colorTint, V3 scale, V3 upAxis, V3 pos, V3 eyePos) {
    V3 direction = v3_minus(eyePos, pos);
    //NOTE: Things are faceing the camera (front face is CW), when the z axis is pointing away from the camera, i.e. z = 1 or identity matrix
    V3 rightVec = normalizeV3(v3_crossProduct(direction, upAxis));
    V3 zAxis = normalizeV3(v3_crossProduct(rightVec, upAxis));

    Matrix4 mT = mat4_xyzwAxis(v3_scale(scale.x, rightVec), v3_scale(scale.y, upAxis), v3_scale(scale.z, zAxis), pos);
    pushRenderItem(&globalQuadVaoHandle, group, globalQuadPositionData, arrayCount(globalQuadPositionData), globalQuadIndicesData, arrayCount(globalQuadIndicesData), group->currentShader, SHAPE_MODEL, 0, mT, group->viewTransform, group->projectionTransform, colorTint, pos.z, material);
}

#define renderDrawRectOutlineCenterDim(center, dim, color, rot, offsetTransform, projectionMatrix) renderDrawRectOutlineCenterDim_(center, dim, color, rot, offsetTransform, projectionMatrix, 0.1f)
void renderDrawRectOutlineCenterDim_(V3 center, V2 dim, V4 color, float rot, Matrix4 offsetTransform, Matrix4 projectionMatrix, float thickness) {
    V3 deltaP = transformPositionV3(center, offsetTransform);
    
    float a1 = cos(rot);
    float a2 = sin(rot);
    float b1 = cos(rot + HALF_PI32);
    float b2 = sin(rot + HALF_PI32);
    
    Matrix4 rotationMat = {{
            a1,  a2,  0,  0,
            b1,  b2,  0,  0,
            0,  0,  1,  0,
            deltaP.x, deltaP.y, deltaP.z,  1
        }};
    
    
    V2 halfDim = v2(0.5f*dim.x, 0.5f*dim.y);
    
    float rotations[4] = {
        0,
        (float)(PI32 + HALF_PI32),
        (float)(PI32),
        (float)(HALF_PI32)
    };
    
    float lengths[4] = {
        dim.y,
        dim.x, 
        dim.y,
        dim.x,
    };
    
    V2 offsets[4] = {
        v2(-halfDim.x, 0),
        v2(0, halfDim.y),
        v2(halfDim.x, 0),
        v2(0, -halfDim.y),
    };
    
    for(int i = 0; i < 4; ++i) {
        float rotat = rotations[i];
        float halfLen = 0.5f*lengths[i];
        V2 offset = offsets[i];
        
        Matrix4 rotationMat1 = {{
                thickness*(float)cos(rotat),  thickness*(float)sin(rotat),  0,  0,
                lengths[i]*-(float)sin(rotat),  lengths[i]*(float)cos(rotat),  0,  0,
                0,  0,  1,  0,
                offset.x, offset.y, 0,  1
            }};
        pushRenderItem(&globalQuadVaoHandle, globalRenderGroup, globalQuadPositionData, arrayCount(globalQuadPositionData), globalQuadIndicesData, arrayCount(globalQuadIndicesData), &textureProgram, SHAPE_RECTANGLE, 0, rotationMat1, rotationMat, projectionMatrix, color, center.z, 0);
    }
}

void renderDrawRectOutlineRect2f(Rect2f rect, V4 color, float rot, Matrix4 offsetTransform, Matrix4 projectionMatrix) {
    renderDrawRectOutlineCenterDim(v2ToV3(getCenter(rect), -1), getDim(rect), color, rot, offsetTransform, projectionMatrix);
}

//
RenderItem *renderDrawRectCenterDim_(V3 center, V2 dim, V4 *colors, float rot, Matrix4 offsetTransform, Texture *texture, ShapeType type, RenderProgram *program, Matrix4 viewMatrix, Matrix4 projectionMatrix) {
    RenderItem *result = 0;
    float a1 = cos(rot);
    float a2 = sin(rot);
    float b1 = cos(rot + HALF_PI32);
    float b2 = sin(rot + HALF_PI32);
    
    V3 deltaP = transformPositionV3(center, offsetTransform);
    Matrix4 rotationMat = {{
            dim.x*a1,  dim.x*a2,  0,  0,
            dim.y*b1,  dim.y*b2,  0,  0,
            0,  0,  1,  0,
            deltaP.x, deltaP.y, deltaP.z,  1
        }};
    
    
    if(globalImmediateModeGraphics) {
    } else {
        int indicesCount = arrayCount(globalQuadIndicesData);
        result = pushRenderItem(&globalQuadVaoHandle, globalRenderGroup, globalQuadPositionData, arrayCount(globalQuadPositionData), 
                       globalQuadIndicesData, indicesCount, program, type, texture, rotationMat,
                       viewMatrix, projectionMatrix, colors[0], center.z, 0);
    }   

    return result; 
}

void renderDrawRectCenterDim(V3 center, V2 dim, V4 color, float rot, Matrix4 offsetTransform, Matrix4 projectionMatrix) {
    V4 colors[4] = {color, color, color, color}; 
    renderDrawRectCenterDim_(center, dim, colors, rot, offsetTransform, 0, SHAPE_RECTANGLE, &textureProgram, mat4(), projectionMatrix);
}

void renderDrawRect(Rect2f rect, float zValue, V4 color, float rot, Matrix4 offsetTransform, Matrix4 projectionMatrix) {
    V4 colors[4] = {color, color, color, color}; 
    renderDrawRectCenterDim_(v2ToV3(getCenter(rect), zValue), getDim(rect), colors, rot, 
                             offsetTransform, 0, SHAPE_RECTANGLE, &textureProgram, mat4(), projectionMatrix);
}

void renderTextureCentreDim(Texture *texture, V3 center, V2 dim, V4 color, float rot, Matrix4 offsetTransform, Matrix4 viewMatrix, Matrix4 projectionMatrix) {
    V4 colors[4] = {color, color, color, color}; 
    renderDrawRectCenterDim_(center, dim, colors, rot, offsetTransform, texture, SHAPE_TEXTURE, &textureProgram, viewMatrix, projectionMatrix);
}

void renderColorWheel(V3 center, V2 dim, EasyRender_ColorWheel_DataPacket *packet, Matrix4 offsetTransform, Matrix4 viewMatrix, Matrix4 projectionMatrix) {
    V4 colors[4]; 
    RenderItem * i = renderDrawRectCenterDim_(center, dim, colors, 0, offsetTransform, &globalWhiteTexture, SHAPE_COLOR_WHEEL, &colorWheelProgram, viewMatrix, projectionMatrix);
    i->dataPacket = packet;

}


// void renderTextureCentreDim(Texture *texture, V3 center, V2 dim, V4 color, float rot) {
//     V4 colors[4] = {color, color, color, color}; 
//     //Matrix4 offsetTransform, Matrix4 viewMatrix, Matrix4 projectionMatrix
//     renderDrawRectCenterDim_(center, dim, colors, rot, offsetTransform, texture, SHAPE_TEXTURE, &textureProgram);
// }

void renderDeleteVaoHandle(VaoHandle *handles) {
    glDeleteVertexArrays(1, &handles->vaoHandle);
    renderCheckError();
    handles->vaoHandle = 0;
    handles->indexCount = 0;
    handles->valid = false;
}

BufferStorage createBufferStorage(InfiniteAlloc *array) {
    BufferStorage result = {};
    glGenBuffers(1, &result.tbo);
    renderCheckError();
    // printf("TBO: %d\n", result.tbo);
    glBindBuffer(RENDER_TEXTURE_BUFFER_ENUM, result.tbo);
    renderCheckError();
    glBufferData(RENDER_TEXTURE_BUFFER_ENUM, array->sizeOfMember*array->count, array->memory, GL_DYNAMIC_DRAW);
    renderCheckError();
    
    glGenTextures(1, &result.buffer);
    renderCheckError();
    glBindTexture(RENDER_TEXTURE_BUFFER_ENUM, result.buffer);
    renderCheckError();
    
    glTexBuffer(RENDER_TEXTURE_BUFFER_ENUM, GL_RGBA32F, result.tbo);
    renderCheckError();
    
    return result;
}


// // Instancing data
// typedef struct {
//     float pvm[16];
//     float color[4];
//     float uvs[4];
// } Easy_InstancePacket;



//This is using vertex attribs
void createBufferStorage2(VaoHandle *vao, InfiniteAlloc *array, RenderProgram *program, bool hasUvs) {
    glBindVertexArray(vao->vaoHandle);
    renderCheckError();
    glBindBuffer(GL_ARRAY_BUFFER, vao->vboInstanceData);
    renderCheckError();
    
    //send the data to GPU. glBufferData deletes the old one
    glBufferData(GL_ARRAY_BUFFER, array->sizeOfMember*array->count, array->memory, GL_DYNAMIC_DRAW);
    renderCheckError();
    
    glBindVertexArray(0);
    renderCheckError();
}

void deleteBufferStorage(BufferStorage *store) {
    // printf("buffer id: %d\n", store->buffer);
    glDeleteTextures(1, &store->buffer);
    renderCheckError();
    
    // printf("tbo id: %d\n", store->tbo);
    glDeleteBuffers(1, &store->tbo);
    renderCheckError();
}

int cmpRenderItemFunc (const void * a, const void * b) {
    RenderItem *itemA = (RenderItem *)a;
    RenderItem *itemB = (RenderItem *)b;
    bool result = true;
    
    //if(itemA->zAt == itemB->zAt) {
        if(itemA->textureHandle == itemB->textureHandle) {
            if(itemA->bufferHandles == itemB->bufferHandles) {
                if(itemA->program == itemB->program) {
                    if(itemA->material == itemB->material) {
                        if(itemA->scissorId == itemB->scissorId) {
                            if(itemA->cullingEnabled == itemB->cullingEnabled) {
                                result = false;    
                            } else {
                                result = itemA->cullingEnabled != itemB->cullingEnabled;
                            }
                        } else {
                             result = itemA->scissorId > itemB->scissorId;
                        }
                    } else {
                        result = (intptr_t)itemA->material < (intptr_t)itemB->material;
                    }
                    
                } else {
                    result = (intptr_t)itemA->program > (intptr_t)itemB->program;
                }
            } else {
                result = (intptr_t)itemA->bufferHandles > (intptr_t)itemB->bufferHandles;                
            }
        } else {
            result = (intptr_t)itemA->textureHandle > (intptr_t)itemB->textureHandle;    
        }
    // } else {
    //     result = itemA->zAt < itemB->zAt;
    // }
    
    return result;
}

void sortItems(InfiniteAlloc *items) {
    //this is a bubble sort. I think this is typically a bit slow. 
    bool sorted = false;
    int max = (items->count - 1);
    for (int index = 0; index < max;) {
        bool incrementIndex = true;
        RenderItem *infoA = (RenderItem *)getElementFromAlloc_(items, index);
        RenderItem *infoB = (RenderItem *)getElementFromAlloc_(items, index + 1);
        assert(infoA && infoB);
        bool swap = cmpRenderItemFunc(infoA, infoB);
        if(swap) {
            RenderItem temp = *infoA;
            *infoA = *infoB;
            *infoB = temp;
            sorted = true;
        }   
        if(index == (max - 1) && sorted) {
            index = 0; 
            sorted = false;
            incrementIndex = false;
        }
        
        if(incrementIndex) {
            index++;
        }
    }
}

void beginRenderGroupForFrame(RenderGroup *group) {
    
}

static inline int easyRender_DrawBatch(RenderGroup *group, InfiniteAlloc *items) {
    int drawCallCount = 0;
    if(items->count > 0) {
        
        RenderItem *info = (RenderItem *)getElementFromAlloc_(items, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, info->bufferId);
        renderCheckError();
        
        if(info->depthTest) {
            glEnable(GL_DEPTH_TEST);
            renderCheckError();
        } else {
            glDisable(GL_DEPTH_TEST);
            renderCheckError();
        }

        if(info->cullingEnabled) {
            render_enableCullFace();
        } else {
            render_disableCullFace();
        }

        if(info->scissorId >= 0) {
            assert(info->scissorId < group->scissorsTests.count);
            Rect2f scRect = *getElementFromAlloc(&group->scissorsTests, info->scissorId, Rect2f);
            V2 d = getDim(scRect);
            glEnable(GL_SCISSOR_TEST);
            renderCheckError();
            glScissor(scRect.minX, scRect.minY, d.x, d.y); 
            renderCheckError();
        } else {
            glDisable(GL_SCISSOR_TEST);
            renderCheckError();
        }
        
        switch(info->blendFuncType) {
            case BLEND_FUNC_ZERO_ONE_ZERO_ONE_MINUS_ALPHA: {
                glBlendFuncSeparate(GL_ZERO, GL_ONE, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
                renderCheckError();
            } break;
            case BLEND_FUNC_STANDARD: {
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
                renderCheckError();
            } break;
            case BLEND_FUNC_STANDARD_NO_PREMULTIPLED_ALPHA: {
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                renderCheckError();
            } break;
            default: {
                assert(!"case not handled");
            }
        }
        InfiniteAlloc allInstanceData = initInfinteAlloc(float);        
        
        for(int i = 0; i < items->count; ++i) {
            RenderItem *nextItem = (RenderItem *)getElementFromAlloc_(items, i);
            if(nextItem) {
                // if(info->bufferHandles == nextItem->bufferHandles && info->textureHandle == nextItem->textureHandle && info->program == nextItem->program && info->material == nextItem->material && easyMath_mat4AreEqual(&info->pMat, &nextItem->pMat) && info->scissorId == nextItem->scissorId && info->cullingEnabled == nextItem->cullingEnabled) {
                assert(info->blendFuncType == nextItem->blendFuncType);
                assert(info->depthTest == nextItem->depthTest);
                //collect data
                addElementInifinteAllocWithCount_(&allInstanceData, nextItem->mMat.val, 16);

                addElementInifinteAllocWithCount_(&allInstanceData, nextItem->vMat.val, 16);

                // addElementInifinteAllocWithCount_(&allInstanceData, nextItem->pMat.val, 16);
                
                addElementInifinteAllocWithCount_(&allInstanceData, nextItem->color.E, 4);
                
                if(nextItem->textureHandle) {
                    addElementInifinteAllocWithCount_(&allInstanceData, nextItem->textureUVs.E, 4);
                } else {
                    //We do this to keep the spacing correct for the struct.
                    float nullData[4] = {};
                    addElementInifinteAllocWithCount_(&allInstanceData, nullData, 4);
                }
                // } 
            } 
        }
            
            
        initVao(info->bufferHandles, info->triangleData, info->triCount, info->indicesData, info->indexCount);
    
        createBufferStorage2(info->bufferHandles, &allInstanceData, info->program, info->textureHandle);

        {
            drawVao(info->bufferHandles, info->program, info->type, info->textureHandle, DRAWCALL_INSTANCED, items->count, info->material, group, &info->pMat, info->dataPacket);
            drawCallCount++;
            
        }
        
        releaseInfiniteAlloc(&allInstanceData);
    }
    return drawCallCount;
}

static inline void easyRender_EndBatch(EasyRenderBatch *b) {
    assert(b->items.sizeOfMember > 0);
    memset(b->items.memory, 0, b->items.totalCount*b->items.sizeOfMember);
    b->items.count = 0;
}

int cmpDepthfunc (const void * a, const void * b) {
    EasyRenderBatch *a1 = *(EasyRenderBatch **)a;
    EasyRenderBatch *b1 = *(EasyRenderBatch **)b;

    RenderItem *i = (RenderItem *)getElementFromAlloc_(&a1->items, 0);
    RenderItem *j = (RenderItem *)getElementFromAlloc_(&b1->items, 0);
   return ( i->zAt < j->zAt);
}

void renderBlitQuad(RenderGroup *group, void *packet) {
    //TODO This is a hack. 
    RenderItem * i = pushRenderItem(&globalFullscreenQuadVaoHandle, group, globalFullscreenQuadPositionData, arrayCount(globalFullscreenQuadPositionData), globalFullscreenQuadIndicesData, arrayCount(globalFullscreenQuadIndicesData), group->currentShader, SHAPE_TONE_MAP, 0, group->modelTransform, group->viewTransform, group->projectionTransform, COLOR_WHITE, group->modelTransform.E_[14], 0);
    i->dataPacket = packet;
}

typedef enum {
    RENDER_DRAW_DEFAULT = 1 << 0,
    RENDER_DRAW_SORT = 1 << 1,
    RENDER_DRAW_SKYBOX = 1 << 2,
} RenderDrawSettings;

void drawRenderGroup(RenderGroup *group, RenderDrawSettings settings) {
    // sortItems(group);

    int drawCallCount = 0;

    render_enableCullFace();
    
    if(settings & RENDER_DRAW_DEFAULT) {
        for(int i = 0; i < RENDER_BATCH_HASH_COUNT; ++i) {
            if(group->batches[i]) {
                EasyRenderBatch *b = group->batches[i];
                while(b) {
                    drawCallCount += easyRender_DrawBatch(group, &b->items);
                    easyRender_EndBatch(b);
                    
                    b = b->next;    
                }
            }
        }
    } else if (settings & RENDER_DRAW_SORT) {

        //NOTE(ol): Going to cache the groups in an array 
        InfiniteAlloc renderBatches = initInfinteAlloc(EasyRenderBatch *);

        //NOTE(Ol): get all the batches that need drawing
        for(int i = 0; i < RENDER_BATCH_HASH_COUNT; ++i) {
            if(group->batches[i]) {
                EasyRenderBatch *b = group->batches[i];
                while(b) {
                    if(b->items.count) {
                        EasyRenderBatch **result = (EasyRenderBatch **)addElementInifinteAlloc_(&renderBatches, 0);
                        *result = b;
                    }
                                        
                    b = b->next;    
                }
            }
        }
        //

        //Sort the items
        qsort(renderBatches.memory, renderBatches.count, sizeof(EasyRenderBatch *), cmpDepthfunc);
        //

        //Draw the render batches from z+ve big to z small
        for(int i = 0; i < renderBatches.count; ++i) {
            EasyRenderBatch *b = *((EasyRenderBatch **)getElementFromAlloc_(&renderBatches, i));
            if(b) {
                drawCallCount += easyRender_DrawBatch(group, &b->items);
                easyRender_EndBatch(b);
            }
        }
        ////

        releaseInfiniteAlloc(&renderBatches);
    }
    

    glDisable(GL_SCISSOR_TEST);

    glEnable(GL_CULL_FACE); 
    glCullFace(GL_FRONT);  
    glFrontFace(GL_CCW);  

    if(group->skybox && (settings & RENDER_DRAW_SKYBOX)) {
         glDepthFunc(GL_LEQUAL);
#if USE_SKY_BOX
        if(!globalCubeMapVaoHandle.valid) {
            initVao(&globalCubeMapVaoHandle, globalCubeMapVertexData, arrayCount(globalCubeMapVertexData), globalCubeIndicesData, arrayCount(globalCubeIndicesData));
        }
        drawVao(&globalCubeMapVaoHandle, &skyboxProgram, SHAPE_SKYBOX, group->skybox->gpuHandle, DRAWCALL_INSTANCED, 1, 0, group, &group->projectionTransform, 0);
#else
        if(!globalFullscreenQuadVaoHandle.valid) {
            initVao(&globalFullscreenQuadVaoHandle, globalFullscreenQuadPositionData, arrayCount(globalFullscreenQuadPositionData), globalFullscreenQuadIndicesData, arrayCount(globalFullscreenQuadIndicesData));
        }
        drawVao(&globalFullscreenQuadVaoHandle, &skyQuadProgram, SHAPE_SKY_QUAD, 0, DRAWCALL_INSTANCED, 1, 0, group, &group->projectionTransform, 0);
#endif
        glDepthFunc(GL_LESS);
    }

    
#if PRINT_NUMBER_DRAW_CALLS
    printf("NUMBER OF DRAW CALLS: %d\n", drawCallCount);
#endif
    group->idAt = 0;
    group->scissorsTests.count = 0;
    group->scissorsEnabled = false;
}