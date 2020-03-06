static inline void myLevels_saveLevel(int level, MyEntityManager *manager) {
	char levelName[256];
	sprintf(levelName, "level_%d.txt", level);

	InfiniteAlloc fileContents = initInfinteAlloc(char);

	for(int i = 0; i < manager->entities.count; ++i) {
		Entity *e = (Entity *)getElement(&manager->entities, i);
		if(e && e->active) { //can be null

			if(e->type == ENTITY_SCENERY) {

				assert(e->model);
				addVar(&fileContents, "MODEL", "type", VAR_CHAR_STAR);
		       	
		       	addVar(&fileContents, e->model->name, "modelName", VAR_CHAR_STAR);

				EasyTransform *T = &e->T;
				addVar(&fileContents, T->pos.E, "position", VAR_V3);
				addVar(&fileContents, T->Q.E, "rotation", VAR_V4);
				addVar(&fileContents, T->scale.E, "scale", VAR_V3);
				addVar(&fileContents, e->colorTint.E, "color", VAR_V4);

				char buffer[32];
	 	        sprintf(buffer, "}\n\n");
		        addElementInifinteAllocWithCount_(&fileContents, buffer, strlen(buffer));
				
			}
		}
	}

	///////////////////////************ Write the file to disk *************////////////////////

	char *folderPath = concat(globalExeBasePath, "levels/");
	char *writeName = concat(folderPath, levelName);
	
	game_file_handle handle = platformBeginFileWrite(writeName);
	platformWriteFile(&handle, fileContents.memory, fileContents.count*fileContents.sizeOfMember, 0);
	platformEndFile(handle);

	///////////////////////************* Clean up the memory ************////////////////////	

	releaseInfiniteAlloc(&fileContents);

	free(writeName);
	free(folderPath);

}

#define LOAD_FROM_STRING 1 //we want to eventually get rid of this & load everything from a file
static inline Entity *myLevels_loadLevel(int level, MyEntityManager *entityManager, V3 startPos) {
	char levelName[256];
	sprintf(levelName, "level_%d.txt", level);

#if LOAD_FROM_STRING
	char *levelString = global_periodLevelStrings[level];
	Entity *newRoom = myLevels_generateLevel_(levelString, entityManager, startPos);
#endif

	char *folderPath = concat(globalExeBasePath, "levels/");
	char *writeName = concat(folderPath, levelName);

	bool isFileValid = platformDoesFileExist(writeName);
	        
	if(isFileValid) {

		bool parsing = true;
	    FileContents contents = getFileContentsNullTerminate(writeName);
	    EasyTokenizer tokenizer = lexBeginParsing((char *)contents.memory, EASY_LEX_OPTION_EAT_WHITE_SPACE);
	    bool wasStar = false;

	    V3 pos = v3(1, 1, 1);
	    EasyModel *model = 0;
	    Quaternion rotation;
	    V3 scale = v3(1, 1, 1);
	    V4 color = v4(1, 1, 1, 1);

	    while(parsing) {
	        EasyToken token = lexGetNextToken(&tokenizer);
	        InfiniteAlloc data = {};
	        switch(token.type) {
	            case TOKEN_NULL_TERMINATOR: {
	                parsing = false;
	            } break;
	            case TOKEN_WORD: {
	                if(stringsMatchNullN("position", token.at, token.size)) {
	                    pos = buildV3FromDataObjects(&data, &tokenizer);
	                }
	                if(stringsMatchNullN("scale", token.at, token.size)) {
	                    scale = buildV3FromDataObjects(&data, &tokenizer);
	                }
	                if(stringsMatchNullN("rotation", token.at, token.size)) {
	                    V4 rot = buildV4FromDataObjects(&data, &tokenizer);
	                    rotation.E[0] = rot.x;
	                    rotation.E[1] = rot.y;
	                    rotation.E[2] = rot.z;
	                    rotation.E[3] = rot.w;
	                }
	                if(stringsMatchNullN("color", token.at, token.size)) {
	                    color = buildV4FromDataObjects(&data, &tokenizer);
	                }
	                if(stringsMatchNullN("modelName", token.at, token.size)) {
	                    char *name = getStringFromDataObjects_memoryUnsafe(&data, &tokenizer);
	                    model = findModelAsset_Safe(name);
	                    assert(model);

	                    releaseInfiniteAlloc(&data);
	                }
	            } break;
	            case TOKEN_CLOSE_BRACKET: {
	                Entity *newEntity = initScenery1x1(entityManager, model->name, model, pos, newRoom);
	                newEntity->T.Q = rotation;
	                newEntity->T.scale = scale;
	                newEntity->colorTint = color;

	            } break;
	            case TOKEN_OPEN_BRACKET: {
	                
	            } break;
	            default: {

	            }
	        }
	    }
	    easyFile_endFileContents(&contents);
	}
	
	///////////////////////************* Clean up the memory ************////////////////////	
	
	free(writeName);
	free(folderPath);

	return newRoom;
}