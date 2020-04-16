static inline Rect2f drawBlockWithName(GameState *gameState, float *xAt, float *yAt, char *title, V4 color, V4 fontColor) {
	Rect2f dim = outputTextNoBacking(globalDebugFont, *xAt, *yAt, NEAR_CLIP_PLANE, gameState->fuaxResolution, title, InfinityRect2f(), fontColor, 1, true, 1);
		
	V2 fontDim = getDim(dim);
	V2 fontCenter = getCenter(dim);
	setModelTransform(globalRenderGroup, Matrix4_translate(Matrix4_scale(mat4(), v3(fontDim.x, fontDim.y, 0)), v3(fontCenter.x, gameState->fuaxResolution.y - fontCenter.y, 0)));
	renderDrawQuad(globalRenderGroup, color);
	// renderDrawRectCenterDim(v3(fontCenter.x, fontCenter.y, NEAR_CLIP_PLANE + 0.1f), fontDim, color, 0, mat4TopLeftToBottomLeft(gameState->fuaxResolution.y), gameState->orthoFuaxMatrix);

	//NOTE(ollie): Advance cursor
	*yAt = *yAt + globalDebugFont->fontHeight;

	return dim;
}
 

static void updateAndRenderLogicWindowForScene(GameState *gameState, GameScene *activeScene, WindowScene *window, OSAppInfo *appInfo) {
    LogicBlock_Set *logicSet = &activeScene->logicSet;

    //NOTE(ollie): Draw the trash can
    V3 trashCanCenter = v3(100 + window->dim.min.x, 150, NEAR_CLIP_PLANE);
    V2 trashCanDim = v2(50, 50);
    V4 trashCanColor = COLOR_WHITE; 

    renderSetShader(globalRenderGroup, &textureProgram);
    
    setViewTransform(globalRenderGroup, Matrix4_translate(mat4(), v3(0, 0, 4)));
    setProjectionTransform(globalRenderGroup, gameState->orthoFuaxMatrix);

    float xAt = 100 - window->cameraPos.x;
    float yAt = 100 - window->cameraPos.y;


    V4 colors[] = {COLOR_RED, COLOR_ORANGE, COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE, COLOR_GOLD };
    	
    float expansionSize = 1.1f*globalDebugFont->fontHeight;
    bool shouldDelete = false;
    s32 indexToDelete = 0;

    bool inSomeRegion = false;
    for(u32 blockIndex = 0; blockIndex < logicSet->logicBlocks.count; ++blockIndex) {
        LogicBlock *block = getElementFromAlloc(&logicSet->logicBlocks, blockIndex, LogicBlock);

         float maxBottomSpacing = block->parameterCount*globalDebugFont->fontHeight;

        switch(block->type) {
        	case LOGIC_BLOCK_PUSH_SCOPE: {
        		xAt += 100;
        	} break;
        	case LOGIC_BLOCK_POP_SCOPE: {
        		xAt -= 100;
        	} break;
        	default: {



        		block->currentSpacing = lerp(block->currentSpacing, 10*appInfo->dt, block->extraSpacing);
        		yAt += block->currentSpacing;

        		s32 typeAsInt = (int)block->type;
        		char *tidyName = global_logicBlockTypeTidyStrings[typeAsInt];

        		u32 colorIndex = typeAsInt % arrayCount(colors);

        		V2 startPos = v2(xAt, yAt);

        		V4 fontColor = (colorIndex == 2 || colorIndex == 3) ? COLOR_BLACK : COLOR_WHITE;

        		Rect2f bounds_ = drawBlockWithName(gameState, &xAt, &yAt, tidyName, colors[colorIndex], fontColor);

        		//NOTE(ollie): Half the bounds to get better overlap functionality
        		V2 dim = getDim(bounds_);
        		V2 center = getCenter(bounds_);
        		float padding = 10;
        		Rect2f bounds = rect2fCenterDim(center.x, center.y - 0.5f*dim.y, dim.x, 0.5f*dim.y);

        		V2 mouseP = easyInput_mouseToResolution(&appInfo->keyStates, gameState->fuaxResolution);

        		Rect2f checkBounds = rect2fMinDim(bounds_.min.x, bounds_.min.y - expansionSize - padding, dim.x, expansionSize + padding + dim.y);

        		///////////////////////************ Draw the drop down parameters if it's open*************////////////////////
        		
        		//NOTE(ollie): First update space for the menu to go in
        		block->currentBottomSpacing = lerp(block->currentBottomSpacing, 10*appInfo->dt, block->bottomSpacing);
        		float tempX = xAt;
        		float tempY = yAt; 

        		yAt += block->currentBottomSpacing;

        		///////////////////////************* if the param menu is open ************////////////////////
        		if(block->isOpen || block->currentBottomSpacing > 1) {
        			V4 color = COLOR_GREY;
        			color.w = block->currentBottomSpacing / maxBottomSpacing;

        			V4 fontColor = COLOR_BLACK;
        			fontColor.w = color.w;

        			#define LOGIC_PARAM_MARGIN 5

        			float cachedX = tempX;
        				
        			///////////////////////*********** Loop through all the parameters **************////////////////////
        			for(int paramIndex = 0; paramIndex < block->parameterCount; ++paramIndex) {
        				LogicParameter *param = block->parameters + paramIndex;
        				float temp = tempY; 
        				Rect2f paramBounds = drawBlockWithName(gameState, &tempX, &tempY, param->name, color, fontColor);
        				//NOTE(ollie): Restore y coord so we don't move down the line
        				tempY = temp;

        				V2 paramDim = getDim(paramBounds);
        			
        				tempX += paramDim.x + LOGIC_PARAM_MARGIN;

        				s32 innerIndexCount = param->innerCount;

			    		////////////////////////////////////////////////////////////////////

        				///////////////////////************ Loop through each number combining the variable *************////////////////////
			    		for(s32 innerIndex = 0; innerIndex < innerIndexCount; ++innerIndex) {

			    			V4 bufferColor = COLOR_WHITE;

			    			InteractionId paramId = interaction_getNullId();
			    			paramId.type = INTERACTION_EDIT_PARAMETER;
			    			paramId.windowId = window;
			    			paramId.blockType = block->type;
			    			paramId.index = blockIndex;
			    			paramId.index2 = paramIndex;
			    			paramId.index3 = innerIndex;

			    			//NOTE(ollie): Make sure we don't go over 8 values. I think this won't happen
			    			assert(innerIndex < arrayCount(param->buffers));

			    			InputBuffer *buffer = &param->buffers[innerIndex];

			    			if(logicUi_isInteraction(&gameState->interactionState, paramId)) {

			    				ProgramInteraction *hold = logicUI_visitInteraction(&gameState->interactionState);

			    				///////////////////////************ Where we actually interact with the values for the parameters*************////////////////////
			    				
		    					if(wasPressed(appInfo->keyStates.gameButtons, BUTTON_ENTER)) {

		    						//NOTE(ollie): Recompile what we added in
		    						WindowScene *playWindow = findFirstWindowByType(gameState, WINDOW_TYPE_SCENE_GAME);
		    						compileLogicBlocks(&activeScene->logicSet, &activeScene->vmMachine, easyRender_getDefaultFauxResolution(getDim(playWindow->dim)));

		    						logicUI_endInteraction(&gameState->interactionState);
		    					} else {
		    						bufferColor = COLOR_GREEN;

		    						//NOTE: update buffer input
		    						if(appInfo->keyStates.inputString) {
		    							splice(buffer, appInfo->keyStates.inputString, true);
		    						}

		    						if(wasPressed(appInfo->keyStates.gameButtons, BUTTON_BACKSPACE)) {
		    							splice(buffer, "1", false);
		    						}

		    						if(wasPressed(appInfo->keyStates.gameButtons, BUTTON_BOARD_LEFT) && buffer->cursorAt > 0) {
		    							buffer->cursorAt--;
		    						}
		    						if(wasPressed(appInfo->keyStates.gameButtons, BUTTON_BOARD_RIGHT) && buffer->cursorAt < buffer->length) {
		    							buffer->cursorAt++;
		    						}
		    						////
		    					}
		    				} 

		    				char *string = buffer->chars;
		    				bool displayString = true;
		    				if(strlen(string) == 0) {
		    					string = "Null";
		    					displayString = false;
		    				}
		    				temp = tempY; 
	        				Rect2f innerParamBounds = drawBlockWithName(gameState, &tempX, &tempY, buffer->chars, bufferColor, fontColor);

	        				///////////////////////********** Start interaction if user clicks ***************////////////////////

				    		//NOTE(ollie): Start interaction
			    			if(!logicUI_isInteracting(&gameState->interactionState) && inBounds(mouseP, innerParamBounds, BOUNDS_RECT)) {
			    				if(wasPressed(appInfo->keyStates.gameButtons, BUTTON_LEFT_MOUSE)) {
			    					ProgramInteraction *interaction = logicUI_startInteraction(&gameState->interactionState, paramId);
			    				}
			    			}
			    			////////////////////////////////////////////////////////////////////

	        				//NOTE(ollie): Restore y coord so we don't move down the line
	        				tempY = temp;

	        				V2 paramDim = getDim(innerParamBounds);

	        				tempX += paramDim.x + LOGIC_PARAM_MARGIN;

		    				////////////////////////////////////////////////////////////////////
			    		} //NOTE(ollie): End of the member params 

			    		tempY += globalDebugFont->fontHeight;
			    		tempX = cachedX;

        				

        			}

        			#undef LOGIC_PARAM_MARGIN
        			
        		}

        		///////////////////////*********** Add a new block into the list**************////////////////////
        		InteractionId id = gameState->interactionState.interaction.id;
        		if(logicUI_isInteracting(&gameState->interactionState) && id.type == INTERACTION_MOVE_LOGIC_BLOCK && id.windowId->type == WINDOW_TYPE_SCENE_LOGIC_BLOCKS_CHOOSER) {
        			
        			if(inBounds(mouseP, bounds, BOUNDS_RECT)) {
        				block->extraSpacing = expansionSize;
        			}

        			if(inBounds(mouseP, checkBounds, BOUNDS_RECT)) {
        				gameState->interactionState.interaction.indexToSpliceAt = blockIndex;
        				inSomeRegion = true;
        				
        			} else {
        				block->extraSpacing = 0;
        			}
        		} else {
        			block->extraSpacing = 0;
        		}
        		////////////////////////////////////////////////////////////////////

        		///////////////////////************ Check if we click the block to drop down the menu*************////////////////////
	    		id = interaction_getNullId();
	    		id.type = INTERACTION_MOVE_LOGIC_BLOCK;
	    		id.windowId = window;
	    		id.blockType = block->type;
	    		id.index = blockIndex;

        		///////////////////////************ Update the animation back to starting position *************////////////////////
    			//NOTE(ollie): The release animation 
    			if(interaction_areIdsEqual(gameState->activeUIAnimation.id, id)) {
    				globalRenderGroup->scissorsEnabled = false;
    				assert(isOn(&gameState->activeUIAnimation.animationTimer));
    				{
    					TimerReturnInfo timerInfo = updateTimer(&gameState->activeUIAnimation.animationTimer, appInfo->dt);

	    				V2 goalPos = gameState->activeUIAnimation.startPos;
	    				V2 atPos = gameState->activeUIAnimation.currentPos;

	    				V2 at = smoothStep01V3(v3(atPos.x, atPos.y, 0), timerInfo.canonicalVal, v3(goalPos.x, goalPos.y, 0)).xy;

	    				drawBlockWithName(gameState, &at.x, &at.y, tidyName, colors[colorIndex], fontColor);

	    				if(timerInfo.finished) {
	    					gameState->activeUIAnimation.id = interaction_getNullId();	
	    				}
    				}
    				globalRenderGroup->scissorsEnabled = true;
    				
    			}
    			////////////////////////////////////////////////////////////////////

	    		//NOTE(ollie): Start interaction
    			if(!logicUI_isInteracting(&gameState->interactionState) && inBounds(mouseP, bounds_, BOUNDS_RECT)) {
    				if(wasPressed(appInfo->keyStates.gameButtons, BUTTON_LEFT_MOUSE)) {
    					ProgramInteraction *interaction = logicUI_startInteraction(&gameState->interactionState, id);
    					interaction->startPos = startPos;
	    					
    				}
    			}

    			//NOTE(ollie): Process interaction
    			if(logicUi_isInteraction(&gameState->interactionState, id)) {
    			    ProgramInteraction *hold = logicUI_visitInteraction(&gameState->interactionState);

    			    float x = mouseP.x;
    			    float y = mouseP.y;

    			    //NOTE(ollie): Draw the block again
    			    Rect2f bounds = drawBlockWithName(gameState, &x, &y, tidyName, colors[colorIndex], fontColor);

    			    bool inTrashCan = false;
    			    if(inBounds(easyInput_mouseToResolution_originLeftBottomCorner(&appInfo->keyStates, gameState->fuaxResolution), rect2fCenterDimV2(trashCanCenter.xy, trashCanDim), BOUNDS_RECT)) {
    			    	inTrashCan = true;
    			    	trashCanColor = COLOR_RED;
    			    }

    			    if(wasReleased(appInfo->keyStates.gameButtons, BUTTON_LEFT_MOUSE)) {

    			    	ProgramInteraction *interaction = &gameState->interactionState.interaction;
    			    	//NOTE(ollie): Check if hovering over the trash can
    			    	if(inTrashCan) {
    			    		shouldDelete = true;
    			    		indexToDelete = blockIndex;
    			    	} else {
    			    		//NOTE(ollie): Retract back to start pos
    			    		gameState->interactionState.interaction.currentPos = mouseP;
    			    		gameState->activeUIAnimation = gameState->interactionState.interaction;
    			    		gameState->activeUIAnimation.animationTimer = initTimer(0.6f, false);	

    			    		// if(getLength(v2_minus(mouseP, interaction->startPos)) < 10) 
    			    		{ //NOTE(ollie): haven't moved it much
    			    			block->isOpen = !block->isOpen; 	
    			    			if(block->isOpen) {
    			    				//NOTE(ollie): Open the paramter menu
    			    				block->bottomSpacing = maxBottomSpacing;
    			    			} else {
    			    				//NOTE(ollie): Close the paramter menu
    			    				block->bottomSpacing = 0;
    			    			}
    			    		}
    			    		
    			    	}
    			    	
    			    	//NOTE(ollie): Stop interacting
    			    	logicUI_endInteraction(&gameState->interactionState);
    			    }


    			}
        		////////////////////////////////////////////////////////////////////
        	}
        }
    }  

    if(inSomeRegion) {
    	gameState->interactionState.interaction.okToSplice = true;
    } else {
    	gameState->interactionState.interaction.okToSplice = false;
    }

    renderTextureCentreDim(findTextureAsset("trash_can.png"), trashCanCenter, trashCanDim, trashCanColor, 0, mat4(), gameState->orthoFuaxMatrix, mat4());

    if(shouldDelete) {
    	removeLogicBlock(logicSet, indexToDelete);

    	//NOTE(ollie): Recompile the blocks
    	WindowScene *playWindow = findFirstWindowByType(gameState, WINDOW_TYPE_SCENE_GAME);
    	compileLogicBlocks(&activeScene->logicSet, &activeScene->vmMachine, easyRender_getDefaultFauxResolution(getDim(playWindow->dim)));
    }
}

static void updateAndRenderGameSimulation(GameState *gameState, GameScene *activeScene, OSAppInfo *appInfo, WindowScene *window) {
	renderSetShader(globalRenderGroup, &textureProgram);
	
	EasyCamera_MoveType camMoveTypes = (EasyCamera_MoveType)(EASY_CAMERA_MOVE | EASY_CAMERA_ROTATE | EASY_CAMERA_ZOOM);
	
	//NOTE(ollie): Update the camera
	// easy3d_updateCamera(&activeScene->camera, &appInfo->keyStates, 1, 100, appInfo->dt, camMoveTypes);
	
	//NOTE(ollie): For diffuse lighting
	easy_setEyePosition(globalRenderGroup, activeScene->camera.pos);		

	setViewTransform(globalRenderGroup, easy3d_getWorldToView(&activeScene->camera));

	V2 windowDim = getDim(window->dim);

	float aspectRatio = windowDim.y / windowDim.x;
	float width = 1920;
	setProjectionTransform(globalRenderGroup, OrthoMatrixToScreen_BottomLeft(width, width*aspectRatio));//gameState->orthoFuaxMatrix);//projectionMatrixFOV(activeScene->FOV, windowDim.x / windowDim.x));

	easyVM_runVM(&activeScene->vmMachine);
	
	
}

static void updateAndRenderLogicChooserForScene(GameState *gameState, GameScene *activeScene, WindowScene *window, OSAppInfo *appInfo) {
	LogicBlock_Set *logicSet = &activeScene->logicSet;

	float xAt = 100 + window->dim.min.x;
    float yAt = 100 + window->dim.min.y;

    char *hotTidyName = 0;
    LogicBlockType hotType;
    s32 hotColorIndex = 0;

    InteractionId hotId;

    V4 colors[] = {COLOR_RED, COLOR_ORANGE, COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE, COLOR_GOLD };

    //NOTE(ollie): Start at 1 to account for null type
	for(u32 blockIndex = 1; blockIndex < arrayCount(global_logicBlockTypeStrings); ++blockIndex) {
	    
	    LogicBlockType type = global_logicBlockTypes[blockIndex];
	    
	    switch(type) {
	    	case LOGIC_BLOCK_PUSH_SCOPE: {
	    	} break;
	    	case LOGIC_BLOCK_POP_SCOPE: {
	    	} break;
	    	default: {
	    		s32 typeAsInt = (int)type;
	    		char *tidyName = global_logicBlockTypeTidyStrings[typeAsInt];

	    		u32 colorIndex = typeAsInt % arrayCount(colors);

	    		V2 startPos = v2(xAt, yAt);

	    		V4 fontColor = (colorIndex == 2 || colorIndex == 3) ? COLOR_BLACK : COLOR_WHITE;
	    		Rect2f bounds = drawBlockWithName(gameState, &xAt, &yAt, tidyName, colors[colorIndex], fontColor);


	    		///////////////////////*********** Update the moving a code block into the code **************////////////////////
	    		InteractionId id = {0};
	    		id.type = INTERACTION_MOVE_LOGIC_BLOCK;
	    		id.windowId = window;
	    		id.blockType = type;

    			if(!logicUI_isInteracting(&gameState->interactionState) && inBounds(easyInput_mouseToResolution(&appInfo->keyStates, gameState->fuaxResolution), bounds, BOUNDS_RECT)) {
    				if(wasPressed(appInfo->keyStates.gameButtons, BUTTON_LEFT_MOUSE)) {
    					ProgramInteraction *interaction = logicUI_startInteraction(&gameState->interactionState, id);
    					interaction->startPos = startPos;
	    					
    				}
    			}


    			//NOTE(ollie): The release animation 
    			if(interaction_areIdsEqual(gameState->activeUIAnimation.id, id)) {
    				globalRenderGroup->scissorsEnabled = false;
    				assert(isOn(&gameState->activeUIAnimation.animationTimer));
    				{
    					TimerReturnInfo timerInfo = updateTimer(&gameState->activeUIAnimation.animationTimer, appInfo->dt);

	    				V2 goalPos = gameState->activeUIAnimation.startPos;
	    				V2 atPos = gameState->activeUIAnimation.currentPos;

	    				V2 at = smoothStep01V3(v3(atPos.x, atPos.y, 0), timerInfo.canonicalVal, v3(goalPos.x, goalPos.y, 0)).xy;

	    				drawBlockWithName(gameState, &at.x, &at.y, tidyName, colors[colorIndex], fontColor);

	    				if(timerInfo.finished) {
	    					gameState->activeUIAnimation.id = interaction_getNullId();	
	    				}
    				}
    				globalRenderGroup->scissorsEnabled = true;
    				
    			}

    			if(logicUi_isInteraction(&gameState->interactionState, id)) {
    				hotTidyName = tidyName;
    				hotColorIndex = colorIndex;
    				hotId = id;
    			}
	    	}
		}  
	}

	///////////////////////*********** Draw the block out side of the sissors margin **************////////////////////	
	easyRender_disableScissors(globalRenderGroup);
	
	if(logicUi_isInteraction(&gameState->interactionState, hotId)) {
		ProgramInteraction *hold = logicUI_visitInteraction(&gameState->interactionState);

		//NOTE(ollie): Actually update the dim now with the mouse location
		V2 mouseP = easyInput_mouseToResolution_originLeftBottomCorner(&appInfo->keyStates, gameState->fuaxResolution);

		V4 fontColor = (hotColorIndex == 2 || hotColorIndex == 3) ? COLOR_BLACK : COLOR_WHITE;

		float x = mouseP.x;
		float y = gameState->fuaxResolution.y - mouseP.y;

		V2 currentPos = v2(x, y);

		Rect2f bounds = drawBlockWithName(gameState, &x, &y, hotTidyName, colors[hotColorIndex], fontColor);

		if(wasReleased(appInfo->keyStates.gameButtons, BUTTON_LEFT_MOUSE)) {

			if(gameState->interactionState.interaction.okToSplice) {
				spliceLogicBlock(logicSet, gameState->interactionState.interaction.id.blockType, gameState->interactionState.interaction.indexToSpliceAt);

				WindowScene *playWindow = findFirstWindowByType(gameState, WINDOW_TYPE_SCENE_GAME);
				compileLogicBlocks(&activeScene->logicSet, &activeScene->vmMachine, easyRender_getDefaultFauxResolution(getDim(playWindow->dim)));
			} else {
				//NOTE(ollie): Retract back to menu
				gameState->interactionState.interaction.currentPos = currentPos;
				gameState->activeUIAnimation = gameState->interactionState.interaction;
				gameState->activeUIAnimation.animationTimer = initTimer(0.6f, false);	
			}
			
			//NOTE(ollie): Stop interacting
			logicUI_endInteraction(&gameState->interactionState);
		}

		///////////////////////*********** Draw outline if interacting**************////////////////////
		renderSetShader(globalRenderGroup, &textureOutlineProgram);

		V2 tempDim = getDim(bounds); 
		V2 tempCenter = getCenter(bounds); 

		setModelTransform(globalRenderGroup, Matrix4_translate(Matrix4_scale(mat4(), v3(tempDim.x, tempDim.y, 0)), v3(tempCenter.x, gameState->fuaxResolution.y - tempCenter.y, NEAR_CLIP_PLANE)));
		renderDrawQuad(globalRenderGroup, COLOR_WHITE);

		////////////////////////////////////////////////////////////////////
	}


	////////////////////////////////////////////////////////////////////
}

static void updateAndRenderSceneWindows(GameState *gameState, OSAppInfo *appInfo) {
	for(u32 windowIndex = 0; windowIndex < gameState->windowSceneCount; windowIndex++) {
		//NOTE(ollie): Get Window out of the array
		WindowScene *window = gameState->windowScenes + windowIndex;
		if(window->isVisible) {

			renderSetShader(globalRenderGroup, &textureOutlineProgram);
			setViewTransform(globalRenderGroup, Matrix4_translate(mat4(), v3(0, 0, 4)));
			setProjectionTransform(globalRenderGroup, gameState->orthoFuaxMatrix);

			V2 rectDim = getDim(window->dim); 
			V2 rectCenter = getCenter(window->dim); 

			///////////////////////*********** Draw window outline **************////////////////////
			//NOTE(ollie): Draw Outline of Window
			
			
			//NOTE(ollie): Update the interaction if holding 
			InteractionId id = {0};
			id.windowId = window;
			id.type = INTERACTION_WINDOW_RESIZE;

			V4 outlineColor = COLOR_YELLOW;

			float grabMargin = 30;

			u32 boundsCount = 0;
			Rect2f bounds[4] = {};
			switch(window->dockType) {
				case WINDOW_DOCK_LEFT: {
					bounds[boundsCount++] = rect2fMinDim(window->dim.max.x - 0.5f*grabMargin, 0, grabMargin, gameState->fuaxResolution.y);
				} break;
				case WINDOW_DOCK_RIGHT: {
					bounds[boundsCount++] = rect2fMinDim(window->dim.min.x - 0.5f*grabMargin, 0, grabMargin, gameState->fuaxResolution.y);
				} break;
				case WINDOW_DOCK_MIDDLE: {
					//NOTE(ollie): left side
					bounds[boundsCount++] = rect2fMinDim(window->dim.min.x - 0.5f*grabMargin, 0, grabMargin, gameState->fuaxResolution.y);
					//NOTE(ollie): Right side
					bounds[boundsCount++] = rect2fMinDim(window->dim.max.x - 0.5f*grabMargin, 0, grabMargin, gameState->fuaxResolution.y);
				} break;
				default: {
					assert(false);
				}
			}
			
			for(s32 grabIndex = 0; grabIndex < boundsCount; ++grabIndex) {
				if(!logicUI_isInteracting(&gameState->interactionState) && inBounds(easyInput_mouseToResolution_originLeftBottomCorner(&appInfo->keyStates, gameState->fuaxResolution), bounds[grabIndex], BOUNDS_RECT)) {
					outlineColor = COLOR_RED;
					if(wasPressed(appInfo->keyStates.gameButtons, BUTTON_LEFT_MOUSE)) {
						ProgramInteraction *interaction = logicUI_startInteraction(&gameState->interactionState, id);
						interaction->sideIndex  = grabIndex;
					}
				}
			}

			if(logicUi_isInteraction(&gameState->interactionState, id)) {
				ProgramInteraction *hold = logicUI_visitInteraction(&gameState->interactionState);

				Rect2f newRect = window->dim;

				//NOTE(ollie): Actually update the dim now with the mouse location
				V2 mouseP = easyInput_mouseToResolution_originLeftBottomCorner(&appInfo->keyStates, gameState->fuaxResolution);

				WindowScene *otherWindowAffected = 0;
				Rect2f otherRect = {0};

				u32 sideIndex = hold->sideIndex;
				switch(hold->id.windowId->dockType) {
					case WINDOW_DOCK_LEFT: {
						newRect.max.x = mouseP.x;

						otherWindowAffected = findWindowByDockType(gameState, WINDOW_DOCK_MIDDLE);
						otherRect = otherWindowAffected->dim;
						otherRect.min.x = mouseP.x;
					} break;
					case WINDOW_DOCK_RIGHT: {
						newRect.min.x = mouseP.x;

						otherWindowAffected = findWindowByDockType(gameState, WINDOW_DOCK_MIDDLE);
						otherRect = otherWindowAffected->dim;
						otherRect.max.x = mouseP.x;
					} break;
					case WINDOW_DOCK_MIDDLE: {
						if(sideIndex == 0) {
							newRect.min.x = mouseP.x;
							otherWindowAffected = findWindowByDockType(gameState, WINDOW_DOCK_LEFT);
							otherRect = otherWindowAffected->dim;
							otherRect.max.x = mouseP.x;
						} else if(sideIndex == 1) {
							newRect.max.x = mouseP.x;	
							otherWindowAffected = findWindowByDockType(gameState, WINDOW_DOCK_RIGHT);
							otherRect = otherWindowAffected->dim;
							otherRect.min.x = mouseP.x;
						}
						
					} break;
					default: {
						assert(false);
					}
				}

				////////////////////////////////////////////////////////////////////

				V2 tempDim = getDim(newRect); 
				V2 tempCenter = getCenter(newRect); 

				setModelTransform(globalRenderGroup, Matrix4_translate(Matrix4_scale(mat4(), v3(tempDim.x, tempDim.y, 0)), v3(tempCenter.x, tempCenter.y, 3)));
				renderDrawQuad(globalRenderGroup, COLOR_BLUE);

				if(wasReleased(appInfo->keyStates.gameButtons, BUTTON_LEFT_MOUSE)) {

					//NOTE(ollie): Actually rezie the window now
					window->dim = newRect;

					if(otherWindowAffected) {
						otherWindowAffected->dim = otherRect;
					}

					//NOTE(ollie): Stop interacting
					logicUI_endInteraction(&gameState->interactionState);
				}

			}


			setModelTransform(globalRenderGroup, Matrix4_translate(Matrix4_scale(mat4(), v3(rectDim.x, rectDim.y, 0)), v3(rectCenter.x, rectCenter.y, 3)));
			renderDrawQuad(globalRenderGroup, outlineColor);

			////////////////////////////////////////////////////////////////////

			renderSetShader(globalRenderGroup, &textureProgram);

			if(window->type != WINDOW_TYPE_SCENE_GAME) {
				//NOTE(ollie): Draw the backing of the window
				setModelTransform(globalRenderGroup, Matrix4_translate(Matrix4_scale(mat4(), v3(rectDim.x, rectDim.y, 0)), v3(rectCenter.x, rectCenter.y, 5)));
				renderDrawQuad(globalRenderGroup, COLOR_LIGHT_GREY);

			}

			//NOTE(ollie): Push Scissors 
			easyRender_pushScissors(globalRenderGroup, window->dim, NEAR_CLIP_PLANE, mat4(), gameState->orthoFuaxMatrix, gameState->fuaxResolution);
			
			//NOTE(ollie): Get the active scene out
			GameScene *activeScene = gameState->scenes + gameState->activeScene;

			switch(window->type) {
				case WINDOW_TYPE_SCENE_GAME: {
					Rect2f r = renderSetViewport(globalRenderGroup, window->dim.min.x, window->dim.min.y, window->dim.max.x, window->dim.max.y);
					updateAndRenderGameSimulation(gameState, activeScene, appInfo, window);
					renderSetViewport(globalRenderGroup, r.min.x, r.min.y, r.max.x, r.max.y);
				} break;
				case WINDOW_TYPE_SCENE_LOGIC_BLOCKS: {
					if(window->isActive) {

						float camAccel = 140.0f; //NOTE(ollie): 100 pixels per second
						if(isDown(appInfo->keyStates.gameButtons, BUTTON_LEFT)) { window->goalCameraPos.x += camAccel*appInfo->dt; }
						if(isDown(appInfo->keyStates.gameButtons, BUTTON_RIGHT)) { window->goalCameraPos.x -= camAccel*appInfo->dt; }
						if(isDown(appInfo->keyStates.gameButtons, BUTTON_UP)) { window->goalCameraPos.y += camAccel*appInfo->dt; }
						if(isDown(appInfo->keyStates.gameButtons, BUTTON_DOWN)) { window->goalCameraPos.y -= camAccel*appInfo->dt; }

						//NOTE(ollie): Update Camera
						window->cameraPos = lerpV2(window->cameraPos, 10*appInfo->dt, window->goalCameraPos);
					}

					
					updateAndRenderLogicWindowForScene(gameState, activeScene, window, appInfo);
				} break;
				case WINDOW_TYPE_SCENE_LOGIC_BLOCKS_CHOOSER: {
					updateAndRenderLogicChooserForScene(gameState, activeScene, window, appInfo);
				} break;
				default: {
					//NOTE(ollie): Case not handled
					assert(false);
				}
			}

			//NOTE(ollie): Pop Scissors 
			easyRender_disableScissors(globalRenderGroup);
				
		}
	}
}