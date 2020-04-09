

static void updateAndRenderGameSceneMenuBar(GameState *gameState, OSAppInfo *appInfo) {

	GameScene_MenuBar *bar = &gameState->gameScene_menuBar;


	if(wasPressed(appInfo->keyStates.gameButtons, BUTTON_F1)) {
		turnTimerOn(&bar->openTimer);
		bar->isOpen = !bar->isOpen; 
	}

	//NOTE(ollie): Update Timer
	if(isOn(&bar->openTimer)) {

		TimerReturnInfo info = updateTimer(&bar->openTimer, appInfo->dt);

		bar->offsetY = info.canonicalVal;

		if(!bar->isOpen) {
			bar->offsetY = 1.0f - bar->offsetY;
		}

		if(info.finished) {
			turnTimerOff(&bar->openTimer);
		}
	}

	V2 dim = v2(100, 100);

	//NOTE(ollie): Coordinates at
	float xAt = 100;
	float yAt = bar->offsetY*150;

	//NOTE(ollie): Draw backing
	renderDrawRectCenterDim(v3(0.5f*gameState->fuaxResolution.x, yAt, NEAR_CLIP_PLANE + 0.2f), v2(gameState->fuaxResolution.x, 2*yAt), COLOR_GREY, 0, mat4(), gameState->orthoFuaxMatrix);

	for(int sceneIndex = 0; sceneIndex < gameState->sceneCount; ++sceneIndex) {
	    GameScene *scene = gameState->scenes + sceneIndex;

	    V4 color = COLOR_LIGHT_GREY;
	    

	    Rect2f bounds = rect2fCenterDim(xAt, yAt, dim.x, dim.y);

	    if(inBounds(easyInput_mouseToResolution_originLeftBottomCorner(&appInfo->keyStates, gameState->fuaxResolution), bounds, BOUNDS_RECT)) {
	    	color = COLOR_BLUE;	
	    	if(wasPressed(appInfo->keyStates.gameButtons, BUTTON_LEFT_MOUSE)) {
	    		gameState->activeScene = sceneIndex;
	    	}
	    	
	    }

	    if(gameState->activeScene == sceneIndex) {
	    	color = COLOR_GOLD;
	    }

	    renderDrawRectCenterDim(v3(xAt, yAt, NEAR_CLIP_PLANE + 0.1f), dim, color, 0, mat4(), gameState->orthoFuaxMatrix);


	    //NOTE(ollie): Text Y at 
	    float textY = gameState->fuaxResolution.y - 0.5f*yAt;

	    Rect2f scissorsMargin = rect2fMinDim(xAt - 0.5f*dim.x, textY - globalDebugFont->fontHeight, dim.x, 2*globalDebugFont->fontHeight);

	    //NOTE(ollie): Push Scissors 
	    easyRender_pushScissors(globalRenderGroup, scissorsMargin, NEAR_CLIP_PLANE, mat4TopLeftToBottomLeft(gameState->fuaxResolution.y), gameState->orthoFuaxMatrix, gameState->fuaxResolution);

	    outputTextNoBacking(globalDebugFont, xAt - 0.5f*dim.x, textY, NEAR_CLIP_PLANE, gameState->fuaxResolution, scene->name, InfinityRect2f(), COLOR_WHITE, 1, true, 1);
	    
	    //NOTE(ollie): Pop Scissors 
	    easyRender_disableScissors(globalRenderGroup);
	    
	    //NOTE(ollie): Advance cursor
	    xAt += 1.4f*dim.x; 
	}

	//NOTE(ollie): Draw Plus sign at end to create new scene
}
