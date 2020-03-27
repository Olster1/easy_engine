typedef struct {
	EasyTransform T;
	int modelIndex; //Index into models on the 
} EasyTerrainModel;

typedef struct {
	InfiniteAlloc placements;

	EasyImage heightMap;
	EasyImage blendMap;
	Texture blendMapTex;

	VaoHandle mesh;

	V3 dim;
	V3 startP;//goes out in +ve X & +ve Z

} EasyTerrainChunk;

typedef struct {
	int chunkCount;
	EasyTerrainChunk *chunks[16];

	int modelCount;
	EasyModel *models[16];

	int textureCount;
	Texture *textures[4];
} EasyTerrain;

static inline float easyTerrain_getValue(EasyImage *img, V2 uv) {
	uv.x = max(min(1.0f, uv.x), 0.0f);
	uv.y = max(min(1.0f, uv.y), 0.0f);

	s32 imgW = img->w - 1;
	s32 imgH = img->h - 1;

	V2 texCoords = v2(uv.x*imgW, uv.y*imgH);

	u32 i = floor(texCoords.x);
	u32 j = floor(texCoords.y);

	float iFrac = texCoords.x - (float)i;
	float jFrac = texCoords.y - (float)j;

	assert(iFrac <= 1.0f && iFrac >= 0.0f);
	assert(jFrac <= 1.0f && jFrac >= 0.0f);

	float constant = 1.0f / 255.0f;
	u32 bytesPerPixel = img->comp;
	int val = (j*imgW + i)*bytesPerPixel;
	int val1 = (j*imgW + (i + 1))*bytesPerPixel;
	int val2 = ((j + 1)*imgW + i)*bytesPerPixel;
	int val3 = ((j + 1)*imgW + i + 1)*bytesPerPixel;
	float value0 = constant*(float)(img->image[val]);
	float value1 = constant*(float)(img->image[val1]);
	float value2 = constant*(float)(img->image[val2]);
	float value3 = constant*(float)(img->image[val3]);

	float result = lerp(lerp(value0, iFrac, value1), jFrac, lerp(value2, iFrac, value3)); //NOTE: bi-linear blend
	return result;

}

static void easyTerrain_generateHeightMap(VaoHandle *floorMesh, V3 dim, EasyImage *heightMap) {
    
    InfiniteAlloc floormeshdata = initInfinteAlloc(Vertex);
    InfiniteAlloc indicesData = initInfinteAlloc(unsigned int);
    	
    s32 width = (s32)dim.x;
    s32 length = (s32)dim.y;
    for(s32 y = 0; y < length; y++) {
        for(s32 x = 0; x < width; x++) {
            s32 subY = y; //this is so the floor starts in the center of the world i.e. has z negative values - can't have negative values since perlin noise looks into an array!!!!
            
            float height = 0;
            float height1 = 0;
            float height2 = 0;
            if(heightMap) {
            	height = dim.z*easyTerrain_getValue(heightMap, v2((float)x / (float)width, (float)subY / (float)length));
            	height1 = dim.z*easyTerrain_getValue(heightMap, v2((float)(x + 1) / (float)width, (float)subY / (float)length));
            	height2 = dim.z*easyTerrain_getValue(heightMap, v2((float)x / (float)width, (float)(subY + 1) / (float)length));
            } else {
            	height = dim.z*perlin2d(x, subY, 1, 8);
            	height1 = dim.z*perlin2d(x + 1, subY, 1, 8);
            	height2 = dim.z*perlin2d(x, subY + 1, 1, 8);
            }
            
            V3 p1 = v3(x + 1, height1, subY);
            V3 p2 = v3(x, height2, subY + 1);
            V3 p0 = v3(x, height, subY);
            V3 a = normalizeV3(v3_minus(p1, p0));
            V3 b = normalizeV3(v3_minus(p2, p0));
            
            V3 normal = v3_crossProduct(a, b);
            
            Vertex v = vertex(p0, normal, v2(x, y));
            addElementInifinteAlloc_(&floormeshdata, &v);
            if(y < (length - 1) && x < (width - 1)) { //not on edge
                
                unsigned int index[6] = {
                    (unsigned int)(x + (width*y)), 
                    (unsigned int)(x + 1 + (width*y)),
                    (unsigned int)(x + 1 + (width*(y+1))), 
                    (unsigned int)(x + (width*y)), 
                    (unsigned int)(x + 1 + (width*(y+1))), 
                    (unsigned int)(x + (width*(y+1))) 
                };
                addElementInifinteAllocWithCount_(&indicesData, &index, 6);
            }
            
        }
    }
    
    
    
    initVao(floorMesh, (Vertex *)floormeshdata.memory, floormeshdata.count, (unsigned int *)indicesData.memory, indicesData.count);


    releaseInfiniteAlloc(&floormeshdata);
    releaseInfiniteAlloc(&indicesData);
}

static inline void easyTerrain_initTerrain(EasyTerrain *t) {
	t->chunkCount = 0;
	t->modelCount = 0;
	t->textureCount = 0;
}

static inline void easyTerrain_AddTexture(EasyTerrain *t, Texture *tex) {
	assert(t->textureCount < arrayCount(t->textures));
	t->textures[t->textureCount++] = tex;
}	
static inline void easyTerrain_plantGrass(EasyTerrain *t, EasyTerrainChunk *c) {
		for(int i = 0; i < 1000; ++i) {
			V2 localP = v2(randomBetween(0, 1.0f), randomBetween(0, 1.0f));

			float height = c->dim.z*easyTerrain_getValue(&c->heightMap, localP);

			V3 pos = v3_plus(c->startP, v3(localP.x*c->dim.x, height, localP.y*c->dim.y));

			EasyTerrainModel model = {};//pushStruct(&globalLongTermArena, EasyTerrainModel);
			easyTransform_initTransform(&model.T, pos, EASY_TRANSFORM_TRANSIENT_ID);
			model.modelIndex = (int)randomBetween(0, t->modelCount);
			if(model.modelIndex == t->modelCount) {
				model.modelIndex--;
			}
			addElementInifinteAllocWithCount_(&c->placements, &model, 1);
		}
}
static inline void easyTerrain_addChunk(EasyTerrain *t, char *heightMapFileName, char *blendMapFileName, V3 startP, V3 dim) {
	assert(t->chunkCount < arrayCount(t->chunks));
	EasyTerrainChunk *c = pushStruct(&globalLongTermArena, EasyTerrainChunk);
	t->chunks[t->chunkCount++] = c;
	c->placements = initInfinteAlloc(EasyTerrainModel);

	EasyImage heightImg = loadImage_(concatInArena(globalExeBasePath, heightMapFileName, &globalPerFrameArena), false);
	EasyImage blendImg = loadImage_(concatInArena(globalExeBasePath, blendMapFileName, &globalPerFrameArena), false);
	
	c->blendMapTex = createTextureOnGPU(blendImg.image, blendImg.w, blendImg.h, blendImg.comp, TEXTURE_FILTER_LINEAR, false);



	c->dim = dim;
	c->startP = startP;

	c->heightMap = heightImg;
	c->blendMap = blendImg;

	easyTerrain_generateHeightMap(&c->mesh, dim, &heightImg);
	easyTerrain_plantGrass(t, c);
}

static inline void easyTerrain_AddModel(EasyTerrain *t, EasyModel *m) {
	assert(t->modelCount < arrayCount(t->models));
	t->models[t->modelCount++] = m;
}	


static inline void easyTerrain_renderTerrain(EasyTerrain *t, RenderGroup *g, EasyMaterial *ma) {

	EasyTerrainDataPacket *packet = pushStruct(&globalPerFrameArena, EasyTerrainDataPacket);

	packet->textureCount = t->textureCount;
	for(int i = 0; i < packet->textureCount; ++i) {
		packet->textures[i] = t->textures[i];
	}

	for (int i = 0; i < t->chunkCount; ++i)
	{
		EasyTerrainChunk *c = t->chunks[i];
		packet->blendMap = &c->blendMapTex;
		EasyTerrainModel *m;
		for(int mIndex = 0; mIndex < c->placements.count; mIndex++) {
			m = getElementFromAlloc(&c->placements, mIndex, EasyTerrainModel);
			assert(m->modelIndex < t->modelCount
				);

			setModelTransform(globalRenderGroup, Matrix4_translate(Matrix4_scale(mat4(), v3(0.3f, 0.3f, 0.3f)), m->T.pos));
			renderModel(g, t->models[m->modelIndex], COLOR_WHITE);
		}

		packet->dim = c->dim;

		setModelTransform(globalRenderGroup, Matrix4_translate(mat4(), c->startP));
		renderSetShader(globalRenderGroup, &terrainProgram);
		renderTerrain(g, &c->mesh, COLOR_WHITE, ma, packet);
	}
}


