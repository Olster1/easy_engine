static inline void myLevels_saveLevel(int level, MyEntityManager *manager, MyGameState *gameState) {
	DEBUG_TIME_BLOCK()
	char levelName[256];
	sprintf(levelName, "level_%d.txt", level);

	InfiniteAlloc fileContents = initInfinteAlloc(char);

	for(int i = 0; i < manager->entities.count; ++i) {
		Entity *e = (Entity *)getElement(&manager->entities, i);
		if(e && e->active) { //can be null

			//NOTE(ollie): Save everything these types
			if(myEntity_shouldEntityBeSaved(e->type)) {
				addVar(&fileContents, &e->T.id, "id", VAR_INT);

				if(e->model) {
			       	addVar(&fileContents, e->model->name, "modelName", VAR_CHAR_STAR);
				}  

				if(e->type == ENTITY_TELEPORTER) {
					if(e->teleporterPartner) {
						addVar(&fileContents, &e->teleporterPartner->T.id, "teleporterId", VAR_INT);
					}
			       	
				}  

				if(e->type == ENTITY_BOSS) {
					addVar(&fileContents, MyEntity_EntityBossTypeStrings[(int)e->bossType], "entityBossType", VAR_CHAR_STAR);
				}

				EasyTransform *T = &e->T;
				char *entityType = MyEntity_EntityTypeStrings[(int)e->type];
				addVar(&fileContents, entityType, "entityType", VAR_CHAR_STAR);
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


	for(s32 tagIndex = 0; tagIndex < gameState->currentRoomTagCount; ++tagIndex) {
		addVar(&fileContents, gameState->currentRoomTags[tagIndex], "tag", VAR_CHAR_STAR);
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

static inline Entity *myLevels_loadLevel(int level, MyEntityManager *entityManager, V3 startPos, MyGameState *gameState) {
	DEBUG_TIME_BLOCK()
	char levelName[256];
	sprintf(levelName, "level_%d.txt", level);

	Entity *newRoom = initRoom(entityManager, startPos);

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
	    EntityType entType = ENTITY_NULL;
	    EntityBossType bossType = ENTITY_BOSS_NULL;

	    s32 teleporterIds[256];
	    Entity *teleportEnts[256];
	    u32 idCount = 0;
	    bool wasTeleporter = false;

	    bool foundId = false;
	    s32 id = 0;

	    while(parsing) {
	        EasyToken token = lexGetNextToken(&tokenizer);
	        InfiniteAlloc data = {};
	        switch(token.type) {
	            case TOKEN_NULL_TERMINATOR: {
	                parsing = false;
	            } break;
	            case TOKEN_WORD: {
	            	if(stringsMatchNullN("tag", token.at, token.size)) {
	            		if(gameState->currentRoomTagCount < arrayCount(gameState->currentRoomTags)) {
	            				char *tagString = getStringFromDataObjects_memoryUnsafe(&data, &tokenizer);

	            				char *newString = easyString_copyToHeap(tagString, strlen(tagString));

	            				gameState->currentRoomTags[gameState->currentRoomTagCount++] = newString;

	            			    ////////////////////////////////////////////////////////////////////
	            			    releaseInfiniteAlloc(&data);	
	            		} else {
	            			easyConsole_addToStream(DEBUG_globalEasyConsole, "Tags full");
	            		}
            	    	
	            	}
	                if(stringsMatchNullN("position", token.at, token.size)) {
	                    pos = buildV3FromDataObjects(&data, &tokenizer);
	                }
	                if(stringsMatchNullN("id", token.at, token.size)) {
	                    id = getIntFromDataObjects(&data, &tokenizer);
	                    foundId = true;
	                }
	                if(stringsMatchNullN("teleporterId", token.at, token.size)) {
	                	assert(idCount < arrayCount(teleporterIds));
	                    teleporterIds[idCount++] = getIntFromDataObjects(&data, &tokenizer);
	                    wasTeleporter = true;
	                }
	                
	                if(stringsMatchNullN("scale", token.at, token.size)) {
	                    scale = buildV3FromDataObjects(&data, &tokenizer);
	                }
	                if(stringsMatchNullN("entityType", token.at, token.size)) {
	                	char *typeString = getStringFromDataObjects_memoryUnsafe(&data, &tokenizer);

	                    entType = (EntityType)findEnumValue(typeString, MyEntity_EntityTypeStrings, arrayCount(MyEntity_EntityTypeStrings));

	                    ////////////////////////////////////////////////////////////////////
	                    releaseInfiniteAlloc(&data);
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

	                    ////////////////////////////////////////////////////////////////////
	                    releaseInfiniteAlloc(&data);
	                }
	                if(stringsMatchNullN("entityBossType", token.at, token.size)) {
	                	char *typeString = getStringFromDataObjects_memoryUnsafe(&data, &tokenizer);

	                    bossType = (EntityBossType)findEnumValue(typeString, MyEntity_EntityBossTypeStrings, arrayCount(MyEntity_EntityBossTypeStrings));

	                    ////////////////////////////////////////////////////////////////////
	                    releaseInfiniteAlloc(&data);
	                }
	            } break;
	            case TOKEN_CLOSE_BRACKET: {

	            	Entity *newEntity = 0;
	            	if(entType == ENTITY_NULL) {
	            		newEntity = initScenery1x1(entityManager, model->name, model, pos, newRoom);	
	            	} else {

		            	if(entType != ENTITY_SCENERY) {
		            		newEntity = initEntityByType(entityManager, pos, entType, newRoom, gameState, false, model, bossType, true);
		            	} else {
		            		newEntity = initScenery1x1(entityManager, model->name, model, pos, newRoom);	
		            	}
		            }
	            	
	            	assert(newEntity);

	            	//NOTE(ollie): Set the id
	            	if(foundId) {
		            	newEntity->T.id = id;
		            	if(id > GLOBAL_transformID_static) {
		            		GLOBAL_transformID_static = id;
		            	}
	            	}
	            	foundId = false;

	                newEntity->T.Q = rotation;
	                newEntity->T.scale = scale;
	                newEntity->colorTint = color;


	                bossType = ENTITY_BOSS_NULL;
	                pos = v3(1, 1, 1);
	                model = 0;
	                rotation;
	                scale = v3(1, 1, 1);
	                color = v4(1, 1, 1, 1);
	                entType = ENTITY_NULL;

	                if(wasTeleporter) {
	                	teleportEnts[idCount - 1] = newEntity;
	                }

	                wasTeleporter = false;

	            } break;
	            case TOKEN_OPEN_BRACKET: {
	                
	            } break;
	            default: {

	            }
	        }
	    }
	    easyFile_endFileContents(&contents);
	    for(int i = 0; i < idCount; ++i) {
	    	Entity * e = MyEntity_findEntityById_static(entityManager, teleporterIds[i]);
	    	assert(e->type == ENTITY_TELEPORTER);
	    	teleportEnts[i]->teleporterPartner = e;
	    }
	}


	
	///////////////////////************* Clean up the memory ************////////////////////	
	
	free(writeName);
	free(folderPath);

	return newRoom;
}


static void myLevels_getLevelInfo(int level, MyWorldTagInfo *tagInfo) {
	DEBUG_TIME_BLOCK()
	char levelName[256];
	sprintf(levelName, "level_%d.txt", level);

	tagInfo->valid = false;
	tagInfo->flags &= MY_WORLD_NULL;

	char *folderPath = concat(globalExeBasePath, "levels/");
	char *writeName = concat(folderPath, levelName);

	bool isFileValid = platformDoesFileExist(writeName);
	        
	if(isFileValid) {

		tagInfo->levelId = level;
		tagInfo->valid = true;

		bool parsing = true;
	    FileContents contents = getFileContentsNullTerminate(writeName);
	    EasyTokenizer tokenizer = lexBeginParsing((char *)contents.memory, EASY_LEX_OPTION_EAT_WHITE_SPACE);

	    while(parsing) {
	        EasyToken token = lexGetNextToken(&tokenizer);
	        InfiniteAlloc data = {};
	        switch(token.type) {
	            case TOKEN_NULL_TERMINATOR: {
	                parsing = false;
	            } break;
	            case TOKEN_WORD: {
	            	if(stringsMatchNullN("tag", token.at, token.size)) {
        				char *tagString = getStringFromDataObjects_memoryUnsafe(&data, &tokenizer);

        				if(cmpStrNull(tagString, "boss")) {
        					tagInfo->flags |= MY_WORLD_BOSS;
        				}
        				if(cmpStrNull(tagString, "fireboss")) {
        					tagInfo->flags |= MY_WORLD_FIRE_BOSS;
        				}
        				if(cmpStrNull(tagString, "puzzle")) {
        					tagInfo->flags |= MY_WORLD_PUZZLE;
        				}
        				if(cmpStrNull(tagString, "enemy")) {
        					tagInfo->flags |= MY_WORLD_ENEMIES;
        				}
        				if(cmpStrNull(tagString, "obstacle")) {
        					tagInfo->flags |= MY_WORLD_OBSTACLES;
        				}
        				if(cmpStrNull(tagString, "space")) {
        					tagInfo->flags |= MY_WORLD_SPACE;
        				}
        				if(cmpStrNull(tagString, "earth")) {
        					tagInfo->flags |= MY_WORLD_EARTH;
        				}

        			    ////////////////////////////////////////////////////////////////////
        			    releaseInfiniteAlloc(&data);	
	            	}
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

}