#define loadAndAddImagesToAssets(folderName) loadAndAddImagesToAssets_(concatInArena(globalExeBasePath, folderName, &globalPerFrameArena))
int loadAndAddImagesToAssets_(char *folderNameAbsolute) {
	DEBUG_TIME_BLOCK()
	char *imgFileTypes[] = {"jpg", "jpeg", "png", "bmp", "PNG"};
	FileNameOfType fileNames = getDirectoryFilesOfType(folderNameAbsolute, imgFileTypes, arrayCount(imgFileTypes));
	int result = fileNames.count;
	
	for(int i = 0; i < fileNames.count; ++i) {
	    char *fullName = fileNames.names[i];
	    char *shortName = getFileLastPortion(fullName);
	    if(shortName[0] != '.') { //don't load hidden file 
	        Asset *asset = findAsset(shortName);
	        assert(!asset);
	        if(!asset) {
	        	bool premultiplyAlpha = true;
	            asset = loadImageAsset(fullName, premultiplyAlpha);
	        }
	        asset = findAsset(shortName);
	        assert(shortName);
	    }
	    free(fullName);
	    free(shortName);
	}
	return result;
}

int loadAndAddSoundsToAssets(char *folderName, SDL_AudioSpec *audioSpec) {
	DEBUG_TIME_BLOCK()
	char *soundFileTypes[] = {"wav"};
	FileNameOfType soundFileNames = getDirectoryFilesOfType(concat(globalExeBasePath, folderName), soundFileTypes, arrayCount(soundFileTypes));
	int result = soundFileNames.count;
	for(int i = 0; i < soundFileNames.count; ++i) {
	    char *fullName = soundFileNames.names[i];
	    char *shortName = getFileLastPortion(fullName);
	    if(shortName[0] != '.') { //don't load hidden file 
	        Asset *asset = findAsset(shortName);
	        assert(!asset);
	        if(!asset) {
	            asset = loadSoundAsset(fullName, audioSpec);
	        }
	        asset = findAsset(shortName);
	        assert(shortName);
	    }
	    free(fullName);
	    free(shortName);
	}
	return result;
}

typedef struct {
	union {
		struct { //for the models
			float scale; 
			EasyModel *model;
		};
	};
	
	
} EasyAssetLoader_AssetInfo;

typedef struct {
	int count;
	AssetType assetType;
	EasyAssetLoader_AssetInfo array[512];
} EasyAssetLoader_AssetArray;

typedef enum {
	EASY_ASSET_LOADER_FLAGS_NULL = 0,
	EASY_ASSET_LOADER_FLAGS_COMPILED_OBJ = 1 << 0
} EasyAssetLoader_Flag;

//TODO(ollie): This should cover all asset types? 
static void easyAssetLoader_loadAndAddAssets(EasyAssetLoader_AssetArray *result, char *folderName, char **fileTypes, int fileTypeCount, AssetType resourceType, EasyAssetLoader_Flag flags) {
	DEBUG_TIME_BLOCK()
	//NOTE(ollie): Get the names
	FileNameOfType allFileNames = getDirectoryFilesOfType(concat(globalExeBasePath, folderName), fileTypes, fileTypeCount);
	if(result) {
		assert(result->assetType == resourceType);
	}
	//loop through all the names
	for(int i = 0; i < allFileNames.count; ++i) {
		//NOTE(ollie): Get the fullname
	    char *fullName = allFileNames.names[i];

	    //NOTE(ollie): Get the short name for testing purposes
	    char *shortName = getFileLastPortion(fullName);

	    
	    //NOTE(ollie): don't load hidden file 
	    if(shortName[0] != '.') { 

	        Asset *asset = findAsset(shortName);
	        assert(!asset);
	        if(!asset) {
	        	if(resourceType == ASSET_MATERIAL) {
	        		//NOTE(ollie): Load the material
	        		easy3d_loadMtl(fullName, EASY_FILE_NAME_GLOBAL);
	        	} else if(resourceType == ASSET_MODEL) {
	        		assert(result->count < arrayCount(result->array));

	        		EasyAssetLoader_AssetInfo *modelInfo = &result->array[result->count++];
	        		EasyModel *model = pushStruct(&globalLongTermArena, EasyModel);
	        		modelInfo->model = model;
	        		if(flags & EASY_ASSET_LOADER_FLAGS_COMPILED_OBJ) {
	        			easy3d_loadCompiledObj_version1(fullName, model);
	        		} else {
	        			easy3d_loadObj(fullName, model, EASY_FILE_NAME_GLOBAL);	
	        		}
	        		
	        		
	        		float axis = easyMath_getLargestAxis(model->bounds);
	        		float relAxis = 5.0f;
	        		modelInfo->scale = relAxis / axis;
	        	}
	        	
	        }
#if DEVELOPER_MODE
	        // asset = findAsset(shortName);
	        // assert(asset);
#endif
	        
	    }
	    free(fullName);
	    free(shortName);
	}
}



static void easyAssetLoader_loadAndCompileObjsFiles(char **modelDirs, u32 modelDirCount) {
	for(int dirIndex = 0; dirIndex < modelDirCount; ++dirIndex) {
	    char *dir = modelDirs[dirIndex];

	    char *folder = concatInArena(globalExeBasePath, dir, &globalPerFrameArena);
	    {
		    //NOTE(ollie): Load materials first
		    char *fileTypes[] = { "mtl"};

		    //NOTE(ollie): Get the names
		    FileNameOfType allFileNames = getDirectoryFilesOfType(folder, fileTypes, arrayCount(fileTypes));
		    //loop through all the names
		    for(int i = 0; i < allFileNames.count; ++i) {
		        //NOTE(ollie): Get the fullname
		        char *fullName = allFileNames.names[i];
		        char *shortName = getFileLastPortion(fullName);
		        //NOTE(ollie): don't load hidden file 
		        if(shortName[0] != '.') { 

		            //NOTE(ollie): Load the material
		            easy3d_loadMtl(fullName, EASY_FILE_NAME_GLOBAL);
		        }
		        free(fullName);
		        free(shortName);
		    }
		}

	    ///////////////////////*********** Load Objs **************////////////////////
	    {
		    //NOTE(ollie): Load materials first
		    char *fileTypes[] = { "obj"};

		    //NOTE(ollie): Get the names
		    FileNameOfType allFileNames = getDirectoryFilesOfType(folder, fileTypes, arrayCount(fileTypes));
		    //loop through all the names
		    for(int i = 0; i < allFileNames.count; ++i) {
		        //NOTE(ollie): Get the fullname
		        char *fullName = allFileNames.names[i];
		        char *shortName = getFileLastPortion(fullName);
		        //NOTE(ollie): don't load hidden file 
		        if(shortName[0] != '.') { 
		            //NOTE(ollie): Make obj file
		            char *name = getFileLastPortionWithoutExtension_arena(shortName, &globalPerFrameArena);
		            char *nameWithExtension = concatInArena(name, ".easy3d", &globalPerFrameArena);
		            char *folderAndName = concatInArena("compiled_models/", nameWithExtension, &globalPerFrameArena);
		            easy3d_compileObj(fullName, concatInArena(globalExeBasePath, folderAndName, &globalPerFrameArena));
		        }
		        free(shortName);
		        free(fullName);
		    }
		}
	    
	}
}