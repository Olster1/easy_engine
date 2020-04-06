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

		if(e->type == OVERWORLD_ENTITY_LEVEL) {
			if(e->levelFlags & MY_WORLD_BOSS) {
				addVar(&fileContents, "boss", "tag", VAR_CHAR_STAR);
			}
			if(e->levelFlags & MY_WORLD_FIRE_BOSS) {
				addVar(&fileContents, "fireboss", "tag", VAR_CHAR_STAR);
			}
			if(e->levelFlags & MY_WORLD_PUZZLE) {
				addVar(&fileContents, "puzzle", "tag", VAR_CHAR_STAR);
			}
			if(e->levelFlags & MY_WORLD_AMBER) {
				addVar(&fileContents, "amber", "tag", VAR_CHAR_STAR);
			}
			if(e->levelFlags & MY_WORLD_ENEMIES) {
				addVar(&fileContents, "enemy", "tag", VAR_CHAR_STAR);
			}
			if(e->levelFlags & MY_WORLD_OBSTACLES) {
				addVar(&fileContents, "obstacle", "tag", VAR_CHAR_STAR);
			}
			if(e->levelFlags & MY_WORLD_SPACE) {
				addVar(&fileContents, "space", "tag", VAR_CHAR_STAR);
			}
			if(e->levelFlags & MY_WORLD_EARTH) {
				addVar(&fileContents, "earth", "tag", VAR_CHAR_STAR);
			}
		}

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


///////////////////////************ Add message*************////////////////////
static MyMessage *myOverworld_addMessageToEntity(OverworldEntity *e, char *response, char *yesMessage, char *noMessage, MyMission *mission) {
	assert(e->messageCount < arrayCount(e->messages));
	MyMessage *message = &e->messages[e->messageCount++];

	//NOTE(ollie): Assign mission
	message->mission = mission;

	//NOTE(ollie): The message & responses
	message->message = response;
	message->yesAnswer = yesMessage;
	message->noAnswer = noMessage;
	
	return message;		
}


////////////////////////////////////////////////////////////////////

static void myOverworld_initEntity_byType(MyGameState *gameState, MyOverworldState *state, OverworldEntity *newEntity, OverworldEntityType entType, V3 pos, V3 scale, Quaternion rotation, V4 color, u32 levelFlags, u32 levelLength) {
		switch (entType) {
			case OVERWORLD_ENTITY_SHIP: {
				newEntity->timeSinceLastBoost = 0.0f;

				newEntity->sprite = findTextureAsset("spaceship_player.png");
				//NOTE(ollie): Set scale to take into account the aspect ratio
				scale.y = newEntity->sprite->aspectRatio_h_over_w*scale.x;


				///////////////////////************* Particle system ************////////////////////
				particle_system_settings ps_set = InitParticlesSettings(PARTICLE_SYS_DEFAULT);

				ps_set.VelBias = rect3f(5, -1, 0, 10, 1, 0);
				ps_set.posBias = rect3f(0, -10, 0, 0, 10, 0);

				// ps_set.angleForce = v2(1, 10);

				ps_set.bitmapScale = 0.7f;

				pushParticleBitmap(&ps_set, findTextureAsset("light_01.png"), "particle1");
				// pushParticleBitmap(&ps_set, findTextureAsset("light_02.png"), "particle2");

				InitParticleSystem(&newEntity->shipFumes, &ps_set, 128);

				setParticleLifeSpan(&newEntity->shipFumes, 0.5f);

				newEntity->shipFumes.Active = true;
				newEntity->shipFumes.Set.Loop = true;

				prewarmParticleSystem(&newEntity->shipFumes, v3(1, 0, 0));

				////////////////////////////////////////////////////////////////////

			} break;
			case OVERWORLD_ENTITY_COLLISION: {
				newEntity->sprite = findTextureAsset("rock1.png");
				//NOTE(ollie): Set scale to take into account the aspect ratio
				scale.y = newEntity->sprite->aspectRatio_h_over_w*scale.x;

			} break;
			case OVERWORLD_ENTITY_PERSON: {
				MyMission *mission = pushStruct(&globalLongTermArena, MyMission);
				zeroStruct(mission, MyMission);		

				mission->title = "Collect 20 crystals, & return them to the person.";
				mission->description = "Do you want to enter the shop?";

				myOverworld_addMessageToEntity(newEntity, "This old space rig ain't going... Some crystals would get it going. Can you help me get some?", "Yes", "No way!", mission);
				myOverworld_addMessageToEntity(newEntity, "You've got the crystals?", "Yes, here you go.", "Nope", 0);

				newEntity->sprite = findTextureAsset("alien-icon.png");
				

				//NOTE(ollie): Set scale to take into account the aspect ratio
				scale.y = newEntity->sprite->aspectRatio_h_over_w*scale.x;


				newEntity->velocity = v2(0, 0);
				newEntity->resetMessage = false;


			} break;
			case OVERWORLD_ENTITY_SHOP: {
				myOverworld_addMessageToEntity(newEntity, "Do you want to enter the shop?", "Yes", "No", 0);

				newEntity->sprite = findTextureAsset("shop-icon.png");
				//NOTE(ollie): Set scale to take into account the aspect ratio
				scale.y = newEntity->sprite->aspectRatio_h_over_w*scale.x;

			} break;
			case OVERWORLD_ENTITY_LEVEL: {
				//NOTE(ollie): Set the sentinel up
				newEntity->animationListSentintel.Next = newEntity->animationListSentintel.Prev = &newEntity->animationListSentintel;
				////////////////////////////////////////////////////////////////////
				
				AddAnimationToList(&gameState->animationItemFreeListPtr, &globalLongTermArena, &newEntity->animationListSentintel, &state->teleporterAnimation);
				float s = randomBetween(2.0f, 5.0f);
				scale = v3(s, s, s);
				myOverworld_addMessageToEntity(newEntity, "Do you want to enter this space time vortex?", "Yes", "No", 0);
				
				//NOTE(ollie): Load from file
				newEntity->levelFlags = levelFlags;
				newEntity->levelLength = levelLength;
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
}

static float global_playerangle = 0.0f; 
static float global_goalAngle = 0.0f;

static void DEBUG_updateDebugOverworld(AppKeyStates *keyStates, MyOverworldState *state, EasyEditor *editor, MyGameState *gameState) {
	if(state->editing) {
		easyEditor_startWindow(editor, "Overworld Panel", 100, 100);
		state->typeToCreate = (OverworldEntityType)easyEditor_pushList(editor, "OverworldType:", MyOverworldEntity_EntityTypeStrings, arrayCount(MyOverworldEntity_EntityTypeStrings));
		
		if(easyEditor_pushButton(editor, "Save Room")) {
			easyConsole_addToStream(DEBUG_globalEasyConsole, "Saved world");
			easyFlashText_addText(&globalFlashTextManager, "Saved World");
		    DEBUG_myOverworld_saveOverworld(state);
		}

		easyEditor_pushFloat1(editor, "Player angle: ", &global_playerangle);
		easyEditor_pushFloat1(editor, "Goal angle: ", &global_goalAngle);
		
		

		//NOTE(ollie): See where the mouse is in the world. Always at 0 Z
		V3 worldP = screenSpaceToWorldSpace(state->projectionMatrix, easyInput_mouseToResolution_originLeftBottomCorner(keyStates, state->fauxResolution), state->fauxResolution, -1*state->camera.pos.z, easy3d_getViewToWorld(&state->camera));

		//NOTE(ollie): Check if you grabbed one
		for(int i = 0; i < state->manager.entityCount && !state->hotEntity; ++i) {
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
			if(isDown(keyStates->gameButtons, BUTTON_LEFT_MOUSE) && isDown(keyStates->gameButtons, BUTTON_SHIFT)) {
				state->hotEntity->T.pos = worldP;	
			}

			EasyTransform *T = &state->hotEntity->T;

			///////////////////////************ ALter Rotation *************////////////////////
			V3 tempEulerAngles = easyMath_QuaternionToEulerAngles(T->Q);

			//NOTE(ollie): Swap to degrees 
			tempEulerAngles.z = easyMath_radiansToDegrees(tempEulerAngles.z);
			
			easyEditor_pushFloat1(editor, "Rotation:", &tempEulerAngles.z);
				

			//NOTE(ollie): Convert back to radians
			tempEulerAngles.z = easyMath_degreesToRadians(tempEulerAngles.z);
			
			T->Q = eulerAnglesToQuaternion(tempEulerAngles.y, tempEulerAngles.x, tempEulerAngles.z);

			////////////////////////////////////////////////////////////////////

			///////////////////////************* Scale ************////////////////////

			easyEditor_pushFloat3(editor, "Scale:", &T->scale.x,&T->scale.y, &T->scale.z);

			////////////////////////////////////////////////////////////////////

			///////////////////////************ Release Entity *************////////////////////

			if(easyEditor_pushButton(editor, "Release Entity") || wasPressed(keyStates->gameButtons, BUTTON_ESCAPE)) {
				state->hotEntity = 0;
			}

			////////////////////////////////////////////////////////////////////
		}
		////////////////////////////////////////////////////////////////////

		///////////////////////*********** Create entity on click**************////////////////////
		OverworldEntityManager *manager = &state->manager;
		if(wasPressed(keyStates->gameButtons, BUTTON_LEFT_MOUSE) && !state->hotEntity && !editor->isHovering && state->typeToCreate != OVERWORLD_ENTITY_NULL) {

			if(manager->entityCount < arrayCount(manager->entities)) {
				OverworldEntity *ent = &manager->entities[manager->entityCount++];

				V3 scale = v3(1, 1, 1);
				if(state->typeToCreate == OVERWORLD_ENTITY_COLLISION) {
					scale.x = 3;
					scale.y = 3;
				}

				myOverworld_initEntity_byType(gameState, state, ent, state->typeToCreate, worldP, scale, identityQuaternion(), COLOR_WHITE, 0, 10);

				state->hotEntity = ent;
				easyFlashText_addText(&globalFlashTextManager, "Created Entity");
				easyConsole_addToStream(DEBUG_globalEasyConsole, "Created Entity");
			} else {
				easyConsole_addToStream(DEBUG_globalEasyConsole, "Can't create entity, entity array full");
			}
		}
		////////////////////////////////////////////////////////////////////
		
		//NOTE(ollie): Release the entity if we have one grabbed
		// if(wasReleased(keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
		// 	state->hotEntity = 0;
		// }

		easyEditor_endWindow(editor); 
	} 
} 




static void myOverworld_loadOverworld(MyOverworldState *state, MyGameState *gameState) {
	char *loadName = myOverworld_getOverworldFileLocation();
		bool isFileValid = platformDoesFileExist(loadName);
		        
		if(isFileValid) {
			bool parsing = true;
		    FileContents contents = getFileContentsNullTerminate(loadName);
		    EasyTokenizer tokenizer = lexBeginParsing((char *)contents.memory, EASY_LEX_OPTION_EAT_WHITE_SPACE);

		    V3 pos = v3(1, 1, 1);
		    Quaternion rotation = identityQuaternion();
		    V3 scale = v3(1, 1, 1);
		    V4 color = v4(1, 1, 1, 1);
		    OverworldEntityType entType = OVERWORLD_ENTITY_NULL;
		    int levelLength = 10;
		    u32 levelFlags = 0;

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
		                if(stringsMatchNullN("levelLength", token.at, token.size)) {
		                    levelLength = getIntFromDataObjects(&data, &tokenizer);
		                }
    	            	if(stringsMatchNullN("tag", token.at, token.size)) {
            				char *tagString = getStringFromDataObjects_memoryUnsafe(&data, &tokenizer);

            				if(cmpStrNull(tagString, "boss")) {
            					levelFlags |= MY_WORLD_BOSS;
            				}
            				if(cmpStrNull(tagString, "fireboss")) {
            					levelFlags |= MY_WORLD_FIRE_BOSS;
            				}
            				if(cmpStrNull(tagString, "puzzle")) {
            					levelFlags |= MY_WORLD_PUZZLE;
            				}
            				if(cmpStrNull(tagString, "enemy")) {
            					levelFlags |= MY_WORLD_ENEMIES;
            				}
            				if(cmpStrNull(tagString, "obstacle")) {
            					levelFlags |= MY_WORLD_OBSTACLES;
            				}
            				if(cmpStrNull(tagString, "space")) {
            					levelFlags |= MY_WORLD_SPACE;
            				}
            				if(cmpStrNull(tagString, "amber")) {
            					levelFlags |= MY_WORLD_AMBER;
            				}
            				if(cmpStrNull(tagString, "earth")) {
            					levelFlags |= MY_WORLD_EARTH;
            				}
            			    ////////////////////////////////////////////////////////////////////
            			    releaseInfiniteAlloc(&data);	
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

		            		myOverworld_initEntity_byType(gameState, state, newEntity, entType, pos, scale, rotation, color, levelFlags, levelLength);


			                levelFlags = MY_WORLD_SPACE;
			                levelLength = 10;
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

///////////////////////********** Missions Interface ***************////////////////////
static void myOverworld_addMission(MyGameState *gameState, MyMission *mission) {
	assert(gameState->missionCount < arrayCount(gameState->missions));
	MyMission **missionPtr = &gameState->missions[gameState->missionCount++];

	//NOTE(ollie): Assign the mission
	*missionPtr = mission;

	//NOTE(ollie): Make sure it's not complete
	assert(!(*missionPtr)->complete);

	//NOTE(ollie): Turn on notification
	turnTimerOn(&gameState->missionNotficationTimer);

}

static void myOverworld_completeMission(MyMission *mission, MyGameState *gameState) {
	
	bool completeState = true;	

	u32 newCrystalCount = gameState->amberCount;

	MyMission_Requirements *req = &mission->requirements;

	if(MY_MISSION_COLLECT_AMBER & req->flags) {
		if(gameState->amberCount >= req->crystalCount) {
			newCrystalCount -= req->crystalCount;
		} else {
			completeState = false;
		}
	} 

	if(MY_MISSION_DESTROY_ROBOTS & req->flags) {
	}

	if(completeState) {
		//NOTE(ollie): Complete the mission
		mission->complete = true;

		//NOTE(ollie): Run the rewards
		MyMission_Reward *reward = &mission->reward;

		if(MY_MISSION_COLLECT_AMBER & reward->flags) {
			newCrystalCount += reward->crystalCount;			
		}

		gameState->amberCount = newCrystalCount;


	}
}

////////////////////////////////////////////////////////////////////

static void drawMessage(MyOverworldState *state, MyGameState *gameState, OverworldEntity *e, AppKeyStates *keyStates, V3 thisP, float dt, bool inRadius, MyOverworld_ReturnData *returnData, Matrix4 perspectiveMat, Matrix4 viewMatrix) {

	float endGoal = (inRadius && !e->resetMessage) ? 1.0f : 0.0f;
	e->lerpValue = lerp(e->lerpValue, clamp01(10*dt), endGoal); 

	if(e->lerpValue > 0.01) {
		V3 ndcSpace = easyRender_worldSpace_To_NDCSpace(thisP, perspectiveMat, viewMatrix);
		float xAt = inverse_lerp(-1, ndcSpace.x, 1)*state->fauxResolution.x;
		float yAt = inverse_lerp(-1, ndcSpace.y, 1)*state->fauxResolution.y;

		//NOTE(ollie): Flip the y so it's top to bottom axis
		yAt = state->fauxResolution.y - yAt; 

		float fontSize = 0.8f;

		//NOTE(ollie): Get the message out of the array
		assert(e->messageAt < arrayCount(e->messages));
		MyMessage *m = &e->messages[e->messageAt];

		float marginWidth = 0.5f*fontSize*0.4f*state->fauxResolution.x;
		xAt -= 0.5f*marginWidth;
		Rect2f margin = rect2fMinDim(xAt, yAt - state->font->fontHeight, marginWidth, INFINITY);

		//NOTE(ollie): Get bounds first
		Rect2f a = outputTextNoBacking(state->font, xAt, yAt, NEAR_CLIP_PLANE, state->fauxResolution, m->message, margin, lerpV4(v4(0, 0, 0, 0), e->lerpValue, COLOR_BLACK), fontSize, false, 1.0f);
		
		//NOTE(ollie): Actually draw it now
		yAt -= getDim(a).y;
		a = outputTextNoBacking(state->font, xAt, yAt, NEAR_CLIP_PLANE, state->fauxResolution, m->message, margin, lerpV4(v4(0, 0, 0, 0), e->lerpValue, COLOR_BLACK), fontSize, true, 1.0f);

		///////////////////////*************************////////////////////
		Rect2f b = outputTextNoBacking(state->font, xAt, yAt + getDim(a).y + state->font->fontHeight, NEAR_CLIP_PLANE, state->fauxResolution, m->yesAnswer, margin, lerpV4(v4(0, 0, 0, 0), e->lerpValue, COLOR_BLACK), fontSize, false, 1.0f);

		{	
			V4 color = COLOR_BLUE;
			if(inBounds(easyInput_mouseToResolution(keyStates, state->fauxResolution), b, BOUNDS_RECT)) {
				color = COLOR_GOLD;

				if(wasPressed(keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
					if(e->type == OVERWORLD_ENTITY_PERSON && e->messages[e->messageAt].mission) {

						//NOTE(ollie): Add mission to the mission log
						myOverworld_addMission(gameState, e->messages[e->messageAt].mission);	
						
						//NOTE(ollie): Get rid of message 
						e->resetMessage = true;

						//NOTE(ollie): Move to next message if there are more
						assert(e->messageAt < (e->messageCount - 1));
						e->messageAt++;


						//NOTE(ollie): Play sound
						playSound(&globalLongTermArena, easyAudio_findSound("ting.wav"), 0, AUDIO_FOREGROUND);

					} else if(e->type == OVERWORLD_ENTITY_LEVEL) {
						playSound(&globalLongTermArena, easyAudio_findSound("ting.wav"), 0, AUDIO_FOREGROUND);

						returnData->transitionToLevel = true;
						returnData->levelFlagsToLoad = e->levelFlags;
						returnData->levelLength = e->levelLength;

						//NOTE(ollie): Get rid of message 
						e->resetMessage = true;


					}
					
				}
			}
			outputTextNoBacking(state->font, xAt, yAt + getDim(a).y + state->font->fontHeight, NEAR_CLIP_PLANE, state->fauxResolution, m->yesAnswer, margin, lerpV4(v4(0, 0, 0, 0), e->lerpValue, color), fontSize, true, 1.0f);	
		}
		////////////////////////////////////////////////////////////////////

		Rect2f boundsRect = unionRect2f(a, b);

		///////////////////////*************************////////////////////
		
		{	
			float x = xAt + 1.5f*getDim(b).x;
			float y = yAt + getDim(a).y + state->font->fontHeight;
			b = outputTextNoBacking(state->font, x, y, NEAR_CLIP_PLANE, state->fauxResolution, m->noAnswer, margin, lerpV4(v4(0, 0, 0, 0), e->lerpValue, COLOR_BLACK), fontSize, false, 1.0f);	
			V4 color = COLOR_BLUE;
			if(inBounds(easyInput_mouseToResolution(keyStates, state->fauxResolution), b, BOUNDS_RECT)) {
				color = COLOR_GOLD;

				if(wasPressed(keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
					
					//NOTE(ollie): Play sound
					playSound(&globalLongTermArena, easyAudio_findSound("ting.wav"), 0, AUDIO_FOREGROUND);

					//NOTE(ollie): Get rid of message 
					e->resetMessage = true;
				}
			}
			outputTextNoBacking(state->font, x, y, NEAR_CLIP_PLANE, state->fauxResolution, m->noAnswer, margin, lerpV4(v4(0, 0, 0, 0), e->lerpValue, color), fontSize, true, 1.0f);	
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
		renderTextureCentreDim(findTextureAsset("comic_box.png"), v3(xAt + 0.5f*bounds.x, yAt + 0.7f*bounds.y, NEAR_CLIP_PLANE + 0.1f), v2_scale(1.5f, messageBounds), COLOR_WHITE, 0, mat4TopLeftToBottomLeft(state->fauxResolution.y), mat4(), state->orthoMatrix);                	
		
	}
}

static MyOverworld_ReturnData myOverworld_updateOverworldState(RenderGroup *renderGroup, AppKeyStates *keyStates, MyOverworldState *state, EasyEditor *editor, float dt, MyGameState *gameState) {
	DEBUG_TIME_BLOCK();

	assert(state->player);

	renderSetShader(renderGroup, &textureProgram);

	MyOverworld_ReturnData returnData = {}; 

	state->camera.pos.xy = lerpV3(state->camera.pos, clamp01(dt), state->player->T.pos).xy;

	Matrix4 viewMatrix = easy3d_getWorldToView(&state->camera);
	setViewTransform(renderGroup, viewMatrix);
	setProjectionTransform(renderGroup, state->projectionMatrix);

#if DEVELOPER_MODE	

	if(wasPressed(keyStates->gameButtons, BUTTON_F1)) {
	    state->editing = !state->editing;
	    easyEditor_stopInteracting(editor);
	}
	DEBUG_updateDebugOverworld(keyStates, state, editor, gameState);
#endif

	state->inBounds = false;

	for(int i = 0; i < state->manager.entityCount; ++i) {
		OverworldEntity *e = &state->manager.entities[i];

		setModelTransform(renderGroup, easyTransform_getTransform(&e->T));

		if(e->messageCount > 0) {
			assert(e->messageAt < e->messageCount);

			//NOTE(ollie): If in radius talk to the player
			V3 playerP = easyTransform_getWorldPos(&state->player->T);
			V3 thisP = easyTransform_getWorldPos(&e->T);
			bool inRadius = (getLengthV3(v3_minus(playerP, thisP)) < MY_OVERWORLD_INTERACT_RADIUS);

			if(!inRadius) {
				e->resetMessage = false;
			}

			drawMessage(state, gameState, e, keyStates, thisP, dt, inRadius, &returnData, state->projectionMatrix, viewMatrix);

		}

		Texture *spriteToDraw = e->sprite;

		switch(e->type) {
			case OVERWORLD_ENTITY_SHIP: {
#define MOVE_WITH_MOUSE 0			
#if MOVE_WITH_MOUSE
				if(!state->lastInBounds && wasPressed(keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
					V3 worldP = screenSpaceToWorldSpace(state->projectionMatrix, easyInput_mouseToResolution_originLeftBottomCorner(keyStates, state->fauxResolution), state->fauxResolution, -1*state->camera.pos.z, easy3d_getViewToWorld(&state->camera));
				
					e->goalPosition = worldP;
					// e->goalAngle;

					playGameSound(&globalLongTermArena, easyAudio_findSound("boosterSound.wav"), 0, AUDIO_FOREGROUND);
				}

				e->T.pos = lerpV3(e->T.pos, 0.4f, e->goalPosition);
#else
				V2 moveAccel = v2(0, 0);
				if(isDown(keyStates->gameButtons, BUTTON_LEFT)) {
					moveAccel.x = -1;
				}
				if(isDown(keyStates->gameButtons, BUTTON_RIGHT)) {
					moveAccel.x = 1;
				}
				if(isDown(keyStates->gameButtons, BUTTON_UP)) {
					moveAccel.y = 1;
				}
				if(isDown(keyStates->gameButtons, BUTTON_DOWN)) {
					moveAccel.y = -1;
				}

				float power = 100;
				moveAccel = v2_scale(power, moveAccel);

				e->T.pos.xy = v2_plus(e->T.pos.xy, v2_plus(v2_scale(dt, e->velocity), v2_scale(0.5f*dt*dt, moveAccel)));
				e->velocity = v2_plus(e->velocity, v2_scale(dt, moveAccel));
				//TODO(ollie): Make drag framerate independent
				e->velocity = v2_minus(e->velocity, v2_scale(0.4f, e->velocity));


				if(e->timeSinceLastBoost > 1.0f) {
					// playOverworldSound(&globalLongTermArena, easyAudio_findSound("boosterSound.wav"), 0, AUDIO_FOREGROUND);	
					e->timeSinceLastBoost = 0.0f;
				}

				e->timeSinceLastBoost += dt;

				if(getLength(moveAccel) > 0.1f) {
					e->goalAngle = ATan2_0toTau(moveAccel.y, moveAccel.x);
					//NOTE(ollie): Wrap the angle so it moves from 0 -> Tau to -Pi -> PI
					if(e->goalAngle > PI32) {
						e->goalAngle = e->goalAngle - TAU32;
					}
				}

				float angle = easyMath_QuaternionToEulerAngles(e->T.Q).z; 

				float neighbourhoodGoal = e->goalAngle;

				if(absVal(neighbourhoodGoal - angle) >= PI32) {
					//NOTE(ollie): Get it closet to angle

					if(neighbourhoodGoal > angle) {
						neighbourhoodGoal -= TAU32;
					} else {
						neighbourhoodGoal += TAU32;
					}
				}

				//NOTE(ollie): Update rotation
				angle = lerp(angle, 10*dt, neighbourhoodGoal);

#if DEVELOPER_MODE
				global_playerangle = angle;
				global_goalAngle = e->goalAngle;

#endif

				e->T.Q = eulerAnglesToQuaternion(0, 0, angle);
				
#endif
#undef MOVE_WITH_MOUSE


				// drawAndUpdateParticleSystem(globalRenderGroup, &e->shipFumes, e->T.pos, dt, v3(0, 0, 0), COLOR_WHITE, true);

				setModelTransform(renderGroup, easyTransform_getTransform(&e->T));

			} break;
			case OVERWORLD_ENTITY_PERSON: {
			} break;
			case OVERWORLD_ENTITY_COLLISION: {
				assert(e->sprite);
			} break;
			case OVERWORLD_ENTITY_SHOP: {
				
			} break;
			case OVERWORLD_ENTITY_LEVEL: {
				char *animationOn = UpdateAnimation(&gameState->animationItemFreeListPtr, &globalLongTermArena, &e->animationListSentintel, dt, 0);
				spriteToDraw = findTextureAsset(animationOn);
			} break;
			default: {
				assert(!"case not handled");
			}
		}

		if(spriteToDraw) {
			renderDrawSprite(renderGroup, spriteToDraw, COLOR_WHITE);	
		}
	}

	state->lastInBounds = state->inBounds;
	
	return returnData;
}
