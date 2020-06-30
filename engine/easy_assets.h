typedef enum {
    ASSET_TEXTURE,
    ASSET_SOUND, 
    ASSET_ANIMATION,
    ASSET_EVENT,
    ASSET_MATERIAL,
    ASSET_MODEL      
} AssetType;

typedef struct
{
    
} Event;

typedef struct Asset Asset;
typedef struct Asset {
    char *name;

	void *file;

	Asset *next;
} Asset;

typedef struct {
    char *name;
    AssetType type;
} EasyAssetIdentifier;

#define GLOBAL_ASSET_ARRAY_SIZE 4096
//NOTE(ol): This gets allocated in the easy_os when starting up the app
static Asset **assets = 0;

#define EASY_ASSET_IDENTIFIER_INCREMENT 512

typedef struct {
    u32 totalCount;
    u32 count;

    EasyAssetIdentifier *identifiers;

} EasyAssetIdentifier_State;

static EasyAssetIdentifier_State global_easyArrayIdentifierstate;

static void easyAssets_initAssetIdentifier(EasyAssetIdentifier_State *state) {
    state->count = 0;
    state->totalCount = 0;
    state->identifiers = 0;
}

static void easyAssets_addAssetIdentifier(EasyAssetIdentifier_State *state, char *name, AssetType type) {
    EasyAssetIdentifier id = {0};
    id.name = name;
    id.type = type;

    if(state->count >= state->totalCount) {
        //NOTE(ollie): Resize the array 
        u32 oldSize = state->count*sizeof(EasyAssetIdentifier);

        state->totalCount += EASY_ASSET_IDENTIFIER_INCREMENT;

        u32 newSize = state->totalCount*sizeof(EasyAssetIdentifier);

        if(state->identifiers) {
            state->identifiers = (EasyAssetIdentifier *)easyPlatform_reallocMemory(state->identifiers, oldSize, newSize);
        } else {
            state->identifiers = (EasyAssetIdentifier *)easyPlatform_allocateMemory(newSize, EASY_PLATFORM_MEMORY_NONE);
        }

    }

    assert(state->count < state->totalCount);
    state->identifiers[state->count++] = id;
    
}

int getAssetHash(char *at, int maxSize) {
    DEBUG_TIME_BLOCK()
	int hashKey = 0;
    while(*at) {
        //Make the hash look up different prime numbers. 
        hashKey += (*at)*19;
        at++;
    }
    hashKey %= maxSize;
    return hashKey;
}

Asset *findAsset(char *fileName) {
    DEBUG_TIME_BLOCK()
    int hashKey = getAssetHash(fileName, GLOBAL_ASSET_ARRAY_SIZE);
    
    Asset *file = assets[hashKey];
    Asset *result = 0;
    
    bool found = false;
    
    while(!found && file) {
        assert(file);
        assert(file->file);
        assert(file->name);
        if(cmpStrNull(fileName, file->name)) {
            result = file;
            found = true;
        } else {
            file = file->next;
        }
    }
    return result;
}

inline static EasyModel *findModelAsset_Safe(char *fileName) { 
    DEBUG_TIME_BLOCK()
    Asset *a = findAsset(fileName); 
    EasyModel *result = 0;
    if(a) {
        result = (EasyModel *)a->file;
    }
    return result;

}
#define findModelAsset(fileName) (EasyModel *)findAsset(fileName)->file
#define findMaterialAsset(fileName) (EasyMaterial *)findAsset(fileName)->file
#define findTextureAsset(fileName) (Texture *)findAsset(fileName)->file
#define findSoundAsset(fileName) (WavFile *)findAsset(fileName)->file
#define findAnimationAsset(fileName) (AnimationParent *)findAsset(fileName)->file
#define findEventAsset(fileName) (Event *)findAsset(fileName)->file


static Texture *getTextureAsset(Asset *assetPtr) {
    Texture *result = (Texture *)(assetPtr->file);
    assert(result);
    return result;
}

static WavFile *getSoundAsset(Asset *assetPtr) {
    WavFile *result = (WavFile *)(assetPtr->file);
    assert(result);
    return result;
}

static AnimationParent *getAnimationAsset(Asset *assetPtr) {
    AnimationParent *result = (AnimationParent *)(assetPtr->file);
    assert(result);
    return result;
}

static Event *getEventAsset(Asset *assetPtr) {
    Event *result = (Event *)(assetPtr->file);
    assert(result);
    return result;
}

static Asset *addAsset_(char *fileName, void *asset) { 
    DEBUG_TIME_BLOCK()
    char *truncName = getFileLastPortion(fileName);
    int hashKey = getAssetHash(truncName, GLOBAL_ASSET_ARRAY_SIZE);
    assert(fileName != truncName);
    Asset **filePtr = assets + hashKey;
    
    bool found = false; 
    Asset *result = 0;
    while(!found) {
        Asset *file = *filePtr;
        if(!file) {
            file = (Asset *)easyPlatform_allocateMemory(sizeof(Asset), EASY_PLATFORM_MEMORY_ZERO);
            file->file = asset;
            file->name = truncName;
            file->next = 0;
            *filePtr = file;
            result = file;
            found = true;
        } else {
            filePtr = &file->next;
        }
    }
    assert(found);
    return result;
}

static void easyAsset_removeAsset(char *fileName) {
    DEBUG_TIME_BLOCK()

    int hashKey = getAssetHash(fileName, GLOBAL_ASSET_ARRAY_SIZE);
    
    Asset **file = &assets[hashKey];
    Asset *result = 0;
    
    bool found = false;
    
    while(!found && *file) {
        if(cmpStrNull(fileName, (*file)->name)) {
            Asset *asset = *file;
            easyPlatform_freeMemory(asset->name);

            *file = asset->next;

            easyPlatform_freeMemory(asset);
        } else {
            file = &(*file)->next;
        }
    }
}

Asset *addAssetTexture(char *fileName, Texture *asset) { // we have these for type checking
    Asset *result = addAsset_(fileName, asset);

    easyAssets_addAssetIdentifier(&global_easyArrayIdentifierstate, result->name, ASSET_TEXTURE);
    return result;
}

Asset *addAssetSound(char *fileName, WavFile *asset) { // we have these for type checking
    Asset *result = addAsset_(fileName, asset);
    easyAssets_addAssetIdentifier(&global_easyArrayIdentifierstate, result->name, ASSET_SOUND);
    return result;
}

Asset *addAssetEvent(char *fileName, Event *asset) { // we have these for type checking
    Asset *result = addAsset_(fileName, asset);
    easyAssets_addAssetIdentifier(&global_easyArrayIdentifierstate, result->name, ASSET_EVENT);
    return result;
}

Asset *addAssetMaterial(char *fileName, EasyMaterial *asset) { // we have these for type checking
    Asset *result = addAsset_(fileName, asset);
    easyAssets_addAssetIdentifier(&global_easyArrayIdentifierstate, result->name, ASSET_MATERIAL);
    return result;
}

Asset *addAssetModel(char *fileName, EasyModel *asset) { // we have these for type checking
    Asset *result = addAsset_(fileName, asset);
    easyAssets_addAssetIdentifier(&global_easyArrayIdentifierstate, result->name, ASSET_MODEL);
    return result;
}

Asset *loadImageAsset(char *fileName, bool premultiplyAlpha) {
    DEBUG_TIME_BLOCK()
    Texture texOnStack = loadImage(fileName, TEXTURE_FILTER_LINEAR, true, premultiplyAlpha);
    Texture *tex = (Texture *)calloc(sizeof(Texture), 1);
    memcpy(tex, &texOnStack, sizeof(Texture));
    Asset *result = addAssetTexture(fileName, tex);
    assert(result);
    return result;
}

Asset *loadSoundAsset(char *fileName, SDL_AudioSpec *audioSpec) {
    DEBUG_TIME_BLOCK()
    WavFile *sound = (WavFile *)calloc(sizeof(WavFile), 1);
    loadWavFile(sound, fileName, audioSpec);
    Asset *result = addAssetSound(fileName, sound);
    assert(result);
    //free(fileName);
    return result;
}