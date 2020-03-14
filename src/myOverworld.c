static char *myOverworld_getOverworldFileLocation() {
	char *folderPath = concatInArena(globalExeBasePath, "levels/", &globalPerFrameArena);
	char *writeName = concatInArena(folderPath, "overworld.txt", &globalPerFrameArena);

	return writeName;
}

///////////////////////*********** DEBUG **************////////////////////
static void DEBUG_myOverworld_saveOverworld(MyOverworldState *state) {

	InfiniteAlloc fileContents = initInfinteAlloc(char);

	for(int i = 0; i < state->manager.entityCount; ++i) {
		OverworldEntity *e = &state->manager.entities[i];

		//NOTE(ollie): Save everything these types

		EasyTransform *T = &e->T;
		char *entityType = MyOverworldEntity_EntityTypeStrings[(int)e->type];
		addVar(&fileContents, entityType, "entityType", VAR_CHAR_STAR);
		addVar(&fileContents, T->pos.E, "position", VAR_V3);
		addVar(&fileContents, T->Q.E, "rotation", VAR_V4);
		addVar(&fileContents, T->scale.E, "scale", VAR_V3);
		addVar(&fileContents, e->colorTint.E, "color", VAR_V4);

		char buffer[32];
	    sprintf(buffer, "}\n\n");
        addElementInifinteAllocWithCount_(&fileContents, buffer, strlen(buffer));
	}

	///////////////////////************ Write the file to disk *************////////////////////

	game_file_handle handle = platformBeginFileWrite(myOverworld_getOverworldFileLocation());
	platformWriteFile(&handle, fileContents.memory, fileContents.count*fileContents.sizeOfMember, 0);
	platformEndFile(handle);

	///////////////////////************* Clean up the memory ************////////////////////	

	releaseInfiniteAlloc(&fileContents);

}

static void DEBUG_updateDebugOverworld(AppKeyStates *keyStates, MyOverworldState *state, EasyEditor *editor) {
	if(state->editing) {
		easyEditor_startWindow(editor, "Overworld Panel", 100, 100);
		state->typeToCreate = (OverworldEntityType)easyEditor_pushList(editor, "OverworldType:", MyOverworldEntity_EntityTypeStrings, arrayCount(MyOverworldEntity_EntityTypeStrings));
		
		if(easyEditor_pushButton(editor, "Save Room")) {
			easyConsole_addToStream(DEBUG_globalEasyConsole, "Saved world");
			easyFlashText_addText(&globalFlashTextManager, "Saved World");
		    DEBUG_myOverworld_saveOverworld(state);
		}
		
		easyEditor_endWindow(editor); 

		//NOTE(ollie): See where the mouse is in the world. Always at 0 Z
		V3 worldP = screenSpaceToWorldSpace(state->projectionMatrix, keyStates->mouseP_left_up, state->screenResolution, -1*state->camera.pos.z, easy3d_getViewToWorld(&state->camera));

		//NOTE(ollie): Check if you grabbed one
		for(int i = 0; i < state->manager.entityCount; ++i) {
			OverworldEntity *e = &state->manager.entities[i];

			Rect2f entBounds = rect2fCenterDim(e->T.pos.x, e->T.pos.y, e->T.scale.x, e->T.scale.y);

			if(inBounds(worldP.xy, entBounds, BOUNDS_RECT)) {
			    if(wasPressed(keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
			        state->hotEntity = e; 
			    }
			    e->colorTint = COLOR_BLUE;
			} else {
				e->colorTint = COLOR_WHITE;
			}

		}

		///////////////////////************ Move the entity if we have one grabbed *************////////////////////
		
		if(state->hotEntity) {
			state->hotEntity->T.pos = worldP;
		}
		////////////////////////////////////////////////////////////////////

		///////////////////////*********** Create entity on click**************////////////////////
		OverworldEntityManager *manager = &state->manager;
		if(wasPressed(keyStates->gameButtons, BUTTON_LEFT_MOUSE) && !state->hotEntity && !editor->isHovering && state->typeToCreate != OVERWORLD_ENTITY_NULL) {

			if(manager->entityCount < arrayCount(manager->entities)) {
				OverworldEntity *ent = &manager->entities[manager->entityCount++];

				easyTransform_initTransform_withScale(&ent->T, worldP, v3(1, 1, 1));
				ent->type = state->typeToCreate;

				easyFlashText_addText(&globalFlashTextManager, "Created Entity");
				easyConsole_addToStream(DEBUG_globalEasyConsole, "Created Entity");
			} else {
				easyConsole_addToStream(DEBUG_globalEasyConsole, "Can't create entity, entity array full");
			}
		}
		////////////////////////////////////////////////////////////////////
		
		//NOTE(ollie): Release the entity if we have one grabbed
		if(wasReleased(keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
			state->hotEntity = 0;
		}
	} 
} 



////////////////////////////////////////////////////////////////////


static void myOverworld_loadOverworld(MyOverworldState *state) {
	char *loadName = myOverworld_getOverworldFileLocation();
		bool isFileValid = platformDoesFileExist(loadName);
		        
		if(isFileValid) {
			bool parsing = true;
		    FileContents contents = getFileContentsNullTerminate(loadName);
		    EasyTokenizer tokenizer = lexBeginParsing((char *)contents.memory, EASY_LEX_OPTION_EAT_WHITE_SPACE);

		    V3 pos = v3(1, 1, 1);
		    Quaternion rotation;
		    V3 scale = v3(1, 1, 1);
		    V4 color = v4(1, 1, 1, 1);
		    OverworldEntityType entType = OVERWORLD_ENTITY_NULL;

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
		                if(stringsMatchNullN("entityType", token.at, token.size)) {
		                	char *typeString = getStringFromDataObjects_memoryUnsafe(&data, &tokenizer);

		                    entType = (OverworldEntityType)findEnumValue(typeString, MyOverworldEntity_EntityTypeStrings, arrayCount(MyOverworldEntity_EntityTypeStrings));

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
		            } break;
		            case TOKEN_CLOSE_BRACKET: {
		            	OverworldEntityManager *manager = &state->manager;
		            	if(manager->entityCount < arrayCount(manager->entities)) {
		            		OverworldEntity *newEntity = &manager->entities[manager->entityCount++];

		            		assert(entType != OVERWORLD_ENTITY_NULL);
		            		easyTransform_initTransform_withScale(&newEntity->T, pos, scale);
		            		newEntity->type = entType;
			                newEntity->T.Q = rotation;
			                newEntity->colorTint = color;
			            } else {
			            	easyConsole_addToStream(DEBUG_globalEasyConsole, "Couldn't save room");
			            }

		            } break;
		            case TOKEN_OPEN_BRACKET: {
		                
		            } break;
		            default: {

		            }
		        }
		    }
		    easyFile_endFileContents(&contents);
		} else {

		}
}


static MyOverworldState *initOverworld(Matrix4 projectionMatrix, V2 resolution) {

	MyOverworldState *state = pushStruct(&globalLongTermArena, MyOverworldState);
	easy3d_initCamera(&state->camera, v3(0, 0, -9));

	state->manager.entityCount = 0;

	state->editing = false;
	state->typeToCreate = OVERWORLD_ENTITY_NULL;
	       
	state->projectionMatrix = projectionMatrix;
	state->screenResolution = resolution;

	myOverworld_loadOverworld(state);

	return state;
}

static void myOverworld_updateOverworldState(RenderGroup *renderGroup, AppKeyStates *keyStates, MyOverworldState *state, EasyEditor *editor) {
	DEBUG_TIME_BLOCK();

	renderSetShader(renderGroup, &textureProgram);
	setViewTransform(renderGroup, easy3d_getWorldToView(&state->camera));
	setProjectionTransform(renderGroup, state->projectionMatrix);

#if DEVELOPER_MODE	

	if(wasPressed(keyStates->gameButtons, BUTTON_F1)) {
	    state->editing = !state->editing;
	    easyEditor_stopInteracting(editor);
	}
	DEBUG_updateDebugOverworld(keyStates, state, editor);
#endif

	for(int i = 0; i < state->manager.entityCount; ++i) {
		OverworldEntity *e = &state->manager.entities[i];

		setModelTransform(renderGroup, easyTransform_getTransform(&e->T));

		switch(e->type) {
			case OVERWORLD_ENTITY_SHIP: {
				

				if(wasPressed(keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
					V3 worldP = screenSpaceToWorldSpace(state->projectionMatrix, keyStates->mouseP_left_up, state->screenResolution, -1*state->camera.pos.z, easy3d_getViewToWorld(&state->camera));
				
					e->goalPosition = worldP;
					// e->goalAngle;

					playGameSound(&globalLongTermArena, easyAudio_findSound("boosterSound.wav"), 0, AUDIO_FOREGROUND);
				}

				e->T.pos = lerpV3(e->T.pos, 0.4f, e->goalPosition);


				setModelTransform(renderGroup, easyTransform_getTransform(&e->T));

				renderDrawSprite(renderGroup, findTextureAsset("spaceship.png"), COLOR_WHITE);
			} break;
			case OVERWORLD_ENTITY_PERSON: {

				//NOTE(ollie): If in radius talk to the player
				// if()
				renderDrawSprite(renderGroup, findTextureAsset("alien-icon.png"), COLOR_WHITE);
			} break;
			case OVERWORLD_ENTITY_SHOP: {
				//NOTE(ollie): if in radius visit the shop, ask player if they want to enter
				renderDrawSprite(renderGroup, findTextureAsset("shop-icon.png"), COLOR_WHITE);
			} break;
			case OVERWORLD_ENTITY_LEVEL: {
				renderDrawSprite(renderGroup, findTextureAsset("star-icon.png"), COLOR_WHITE);
			} break;
			default: {
				assert(!"case not handled");
			}
		}
		
	}
	
}
