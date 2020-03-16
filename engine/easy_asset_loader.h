int loadAndAddImagesToAssets(char *folderName) {
	DEBUG_TIME_BLOCK()
	char *imgFileTypes[] = {"jpg", "jpeg", "png", "bmp", "PNG"};
	FileNameOfType fileNames = getDirectoryFilesOfType(concat(globalExeBasePath, folderName), imgFileTypes, arrayCount(imgFileTypes));
	int result = fileNames.count;
	
	for(int i = 0; i < fileNames.count; ++i) {
	    char *fullName = fileNames.names[i];
	    char *shortName = getFileLastPortion(fullName);
	    if(shortName[0] != '.') { //don't load hidden file 
	        Asset *asset = findAsset(shortName);
	        assert(!asset);
	        if(!asset) {
	            asset = loadImageAsset(fullName);
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

//TODO(ollie): This should cover all asset types? 
static void easyAssetLoader_loadAndAddAssets(EasyAssetLoader_AssetArray *result, char *folderName, char **fileTypes, int fileTypeCount, AssetType resourceType) {
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

	    	char *ext = getFileExtension(shortName);
	        
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
	        		easy3d_loadObj(fullName, model, EASY_FILE_NAME_GLOBAL);
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