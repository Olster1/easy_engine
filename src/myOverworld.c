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
		V3 worldP = screenSpaceToWorldSpace(state->projectionMatrix, easyInput_mouseToResolution(keyStates, state->fauxResolution), state->fauxResolution, -1*state->camera.pos.z, easy3d_getViewToWorld(&state->camera));

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

				easyTransform_initTransform_withScale(&ent->T, worldP, v3(1, 1, 1), EASY_TRANSFORM_STATIC_ID);
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


static void myOverworld_loadOverworld(MyOverworldState *state, MyGameState *gameState) {
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

		            		if(entType == OVERWORLD_ENTITY_SHIP) {
		            			assert(!state->player);
		            			state->player = newEntity; 
		            		}

		            		switch (entType) {
		            			case OVERWORLD_ENTITY_SHIP: {
		            			} break;
		            			case OVERWORLD_ENTITY_PERSON: {
		            				newEntity->message = "I've been having trouble with some space robots destroying my rocks. Can you help";
		            			} break;
		            			case OVERWORLD_ENTITY_SHOP: {
		            				newEntity->message = "Do you want to enter the shop?";
		            			} break;
		            			case OVERWORLD_ENTITY_LEVEL: {
		            				//NOTE(ollie): Set the sentinel up
		            				newEntity->animationListSentintel.Next = newEntity->animationListSentintel.Prev = &newEntity->animationListSentintel;
		            				////////////////////////////////////////////////////////////////////
		            				
		            				AddAnimationToList(&gameState->animationItemFreeListPtr, &globalLongTermArena, &newEntity->animationListSentintel, &state->teleporterAnimation);
		            				scale = v3_scale(3.0f, scale);
		            				newEntity->message = "Do you want to enter this space time vortex?";
		            			} break;
		            			default: {

		            			}

		            		}
		            		
		            		newEntity->lerpValue = 0.0f;

		            		assert(entType != OVERWORLD_ENTITY_NULL);
		            		easyTransform_initTransform_withScale(&newEntity->T, pos, scale, EASY_TRANSFORM_STATIC_ID);
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


static MyOverworldState *initOverworld(Matrix4 projectionMatrix, float aspectRatio, EasyFont_Font *font, MyGameState *gameState) {

	MyOverworldState *state = pushStruct(&globalLongTermArena, MyOverworldState);
	easy3d_initCamera(&state->camera, v3(0, 0, -9));

	state->manager.entityCount = 0;

	state->editing = false;
	state->typeToCreate = OVERWORLD_ENTITY_NULL;

	float fuaxWidth = 1920;
	state->fauxResolution = v2(fuaxWidth, fuaxWidth*aspectRatio);

	//NOTE(ollie): Assign matricies
	state->projectionMatrix = projectionMatrix;
	state->orthoMatrix = OrthoMatrixToScreen_BottomLeft(state->fauxResolution.x, state->fauxResolution.y);

	state->player = 0;

	state->font = font;

	myOverworld_loadOverworld(state, gameState);

	///////////////////////************* Setup the animation ************////////////////////
	
	//NOTE(ollie): Create the idle animation
	char *formatStr = "teleporter-%d.png";
	char buffer[512];
	////////////////////////////////////////////////////////////////////

	//NOTE(ollie): Init the animation
	state->teleporterAnimation.Name = "Teleporter Idle";
	state->teleporterAnimation.state = ANIM_IDLE;
	state->teleporterAnimation.FrameCount = 0;
	
	//NOTE(ollie): Loop through image names & add them to the animation
	for(int loopIndex = 1; loopIndex <= 150; ++loopIndex) {
	    
	    //NOTE(ollie): Print the texture ID
	    snprintf(buffer, arrayCount(buffer), formatStr, loopIndex);	
	    
	    //NOTE(ollie): Add it to the array
	    assert(state->teleporterAnimation.FrameCount < arrayCount(state->teleporterAnimation.Frames));
	    state->teleporterAnimation.Frames[state->teleporterAnimation.FrameCount++] = easyString_copyToArena(buffer, &globalLongTermArena);
	    
	}
	////////////////////////////////////////////////////////////////////

	return state;
}

#define MY_OVERWORLD_INTERACT_RADIUS 1.4f

static void drawMessage(MyOverworldState *state, OverworldEntity *e, AppKeyStates *keyStates, V3 thisP, float dt, bool inRadius, char *yesMessage, char *noMessage) {

	float endGoal = (inRadius) ? 1.0f : 0.0f;
	e->lerpValue = lerp(e->lerpValue, clamp01(10*dt), endGoal); 

	if(e->lerpValue > 0.01) {
		float xAt = 0.5f*state->fauxResolution.x;
		float yAt = 0.5f*state->fauxResolution.y;

		float fontSize = 1.5f;

		Rect2f margin = rect2fMinDim(xAt, yAt - state->font->fontHeight, 0.4f*state->fauxResolution.x, INFINITY);

		Rect2f a = outputTextNoBacking(state->font, xAt, yAt, NEAR_CLIP_PLANE, state->fauxResolution, e->message, margin, lerpV4(v4(0, 0, 0, 0), e->lerpValue, COLOR_BLACK), fontSize, true, 1.0f);

		///////////////////////*************************////////////////////
		Rect2f b = outputTextNoBacking(state->font, xAt, yAt + getDim(a).y + state->font->fontHeight, NEAR_CLIP_PLANE, state->fauxResolution, yesMessage, margin, lerpV4(v4(0, 0, 0, 0), e->lerpValue, COLOR_BLACK), fontSize, false, 1.0f);

		{	
			V4 color = COLOR_BLUE;
			if(inBounds(easyInput_mouseToResolution(keyStates, state->fauxResolution), b, BOUNDS_RECT)) {
				color = COLOR_GOLD;

				if(wasPressed(keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {

				}
			}
			outputTextNoBacking(state->font, xAt, yAt + getDim(a).y + state->font->fontHeight, NEAR_CLIP_PLANE, state->fauxResolution, yesMessage, margin, lerpV4(v4(0, 0, 0, 0), e->lerpValue, color), fontSize, true, 1.0f);	
		}
		////////////////////////////////////////////////////////////////////
		

		Rect2f boundsRect = unionRect2f(a, b);

		///////////////////////*************************////////////////////
		
		{
			b = outputTextNoBacking(state->font, xAt + 1.5f*getDim(b).x, yAt + getDim(a).y + state->font->fontHeight, NEAR_CLIP_PLANE, state->fauxResolution, noMessage, margin, lerpV4(v4(0, 0, 0, 0), e->lerpValue, COLOR_BLACK), fontSize, false, 1.0f);	
			V4 color = COLOR_BLUE;
			if(inBounds(easyInput_mouseToResolution(keyStates, state->fauxResolution), b, BOUNDS_RECT)) {
				color = COLOR_GOLD;

				if(wasPressed(keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
					
				}
			}
			outputTextNoBacking(state->font, xAt + 1.5f*getDim(b).x, yAt + getDim(a).y + state->font->fontHeight, NEAR_CLIP_PLANE, state->fauxResolution, noMessage, margin, lerpV4(v4(0, 0, 0, 0), e->lerpValue, color), fontSize, true, 1.0f);	
		}
		////////////////////////////////////////////////////////////////////

		boundsRect = unionRect2f(boundsRect, b);

		///////////////////////*********** Check if player is hovering over text **************////////////////////
		if(inBounds(easyInput_mouseToResolution(keyStates, state->fauxResolution), boundsRect, BOUNDS_RECT)) {
			state->inBounds = true;

		}
		////////////////////////////////////////////////////////////////////
		
		V2 bounds = getDim(boundsRect);
		V2 messageBounds = lerpV2(v2(0, 0), e->lerpValue, v2_scale(1.3f, bounds));
		renderTextureCentreDim(findTextureAsset("rounded_square.png"), v3(xAt + 0.5f*bounds.x, yAt + 0.5f*bounds.y - 0.5f*state->font->fontHeight, NEAR_CLIP_PLANE + 0.1f), messageBounds, COLOR_WHITE, 0, mat4TopLeftToBottomLeft(state->fauxResolution.y), mat4(), state->orthoMatrix);                	
		
	}
}

static void myOverworld_updateOverworldState(RenderGroup *renderGroup, AppKeyStates *keyStates, MyOverworldState *state, EasyEditor *editor, float dt, MyGameState *gameState) {
	DEBUG_TIME_BLOCK();

	assert(state->player);

	renderSetShader(renderGroup, &textureProgram);


	state->camera.pos.xy = lerpV3(state->camera.pos, clamp01(dt), state->player->T.pos).xy;

	setViewTransform(renderGroup, easy3d_getWorldToView(&state->camera));
	setProjectionTransform(renderGroup, state->projectionMatrix);

#if DEVELOPER_MODE	

	if(wasPressed(keyStates->gameButtons, BUTTON_F1)) {
	    state->editing = !state->editing;
	    easyEditor_stopInteracting(editor);
	}
	DEBUG_updateDebugOverworld(keyStates, state, editor);
#endif

	state->inBounds = false;

	for(int i = 0; i < state->manager.entityCount; ++i) {
		OverworldEntity *e = &state->manager.entities[i];

		setModelTransform(renderGroup, easyTransform_getTransform(&e->T));

		if(e->message) {
			//NOTE(ollie): If in radius talk to the player
			V3 playerP = easyTransform_getWorldPos(&state->player->T);
			V3 thisP = easyTransform_getWorldPos(&e->T);
			bool inRadius = (getLengthV3(v3_minus(playerP, thisP)) < MY_OVERWORLD_INTERACT_RADIUS);

			drawMessage(state, e, keyStates, thisP, dt, inRadius, "Let's Go!", "No Thanks");

		}

		switch(e->type) {
			case OVERWORLD_ENTITY_SHIP: {
				if(!state->lastInBounds && wasPressed(keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
					V3 worldP = screenSpaceToWorldSpace(state->projectionMatrix, easyInput_mouseToResolution_originLeftBottomCorner(keyStates, state->fauxResolution), state->fauxResolution, -1*state->camera.pos.z, easy3d_getViewToWorld(&state->camera));
				
					e->goalPosition = worldP;
					// e->goalAngle;

					playGameSound(&globalLongTermArena, easyAudio_findSound("boosterSound.wav"), 0, AUDIO_FOREGROUND);
				}

				e->T.pos = lerpV3(e->T.pos, 0.4f, e->goalPosition);

				setModelTransform(renderGroup, easyTransform_getTransform(&e->T));

				renderDrawSprite(renderGroup, findTextureAsset("spaceship.png"), COLOR_WHITE);
			} break;
			case OVERWORLD_ENTITY_PERSON: {

				
				renderDrawSprite(renderGroup, findTextureAsset("alien-icon.png"), COLOR_WHITE);
			} break;
			case OVERWORLD_ENTITY_SHOP: {
				//NOTE(ollie): if in radius visit the shop, ask player if they want to enter
				renderDrawSprite(renderGroup, findTextureAsset("shop-icon.png"), COLOR_WHITE);
			} break;
			case OVERWORLD_ENTITY_LEVEL: {
				char *animationOn = UpdateAnimation(&gameState->animationItemFreeListPtr, &globalLongTermArena, &e->animationListSentintel, dt, 0);
				Texture *sprite = findTextureAsset(animationOn);

				renderDrawSprite(renderGroup, sprite, COLOR_WHITE);
			} break;
			default: {
				assert(!"case not handled");
			}
		}
		
	}

	state->lastInBounds = state->inBounds;
	
}
