typedef struct {
	char *name;
	
	LogicBlock_Set logicSet;

	float FOV;

	//NOTE(ollie): Each scene has a camera associated with it
	EasyCamera camera;

	//NOTE(ollie): For simulation of the scene, run each frame expect for the constructors 
	EasyVM_State vmMachine;

} GameScene;

typedef struct {

	float scrollOffsetX;

	//NOTE(ollie): For when it appears from the bottom
	float offsetY;

	bool isOpen;
	Timer openTimer;

} GameScene_MenuBar;

typedef struct {
	//NOTE(ollie): The game scenes of the game
	u32 activeScene;
	u32 sceneCount;
	GameScene scenes[128];	

	//NOTE(ollie): For the game scene menu bar
	GameScene_MenuBar gameScene_menuBar;

	//NOTE(ollie): Ortho Matrix for rendering
	Matrix4 orthoFuaxMatrix;
	V2 fuaxResolution;

	//NOTE(ollie): For the windows that are active
	u32 windowSceneCount;
	WindowScene windowScenes[64];

	//NOTE(ollie): Interaction state
	InteractionState interactionState;

	//NOTE(ollie): Animation state stuff
	ProgramInteraction activeUIAnimation; //NOTE(ollie): Make to an array

} GameState;


///////////////////////************ GameState functions *************////////////////////

static void gameScene_initMenuBar(GameState *state) {
	GameScene_MenuBar *bar = &state->gameScene_menuBar;

	bar->openTimer = initTimer(0.5f, false);
	turnTimerOff(&bar->openTimer);
}

static void gameState_pushGameScene(GameState *state, char *name) {
	if(state->sceneCount < arrayCount(state->scenes)) {
		GameScene *scene = state->scenes + state->sceneCount++;

		scene->name = name;

		initLogicBlockSet(&scene->logicSet);
		easy3d_initCamera(&scene->camera, v3(0, 0, 0));
		scene->FOV = 60.0f;

		easyVM_initVM(&scene->vmMachine);

	} else {
		easyConsole_addToStream(DEBUG_globalEasyConsole, "Reached max scenes");
	}
	
}

static GameScene *findGameSceneByName(GameState *state, char *name) {
	GameScene *result = 0;
	for(int sceneIndex = 0; sceneIndex < state->sceneCount && !result; ++sceneIndex) {
	    GameScene *scene = state->scenes + sceneIndex;

	    if(cmpStrNull(name, scene->name)) {
	    	result = scene;
	    	break;
	    }
	}
	return result;
}

static WindowScene *findFirstWindowByType(GameState *state, WindowType type) {
	WindowScene *result = 0;
	for(int sceneIndex = 0; sceneIndex < state->windowSceneCount && !result; ++sceneIndex) {
	    WindowScene *scene = state->windowScenes + sceneIndex;

	    if(scene->type == type) {
	    	result = scene;
	    	break;
	    }
	}
	return result;
}

///////////////////////************ Window State Functions *************////////////////////
static bool pushWindowScene(GameState *state, WindowType type) {
	bool result = true;
	//NOTE(ollie): Check if room for any more
	if(state->windowSceneCount < arrayCount(state->windowScenes)) {
		WindowScene *window = state->windowScenes + state->windowSceneCount++;

		//NOTE(ollie): Init the window 
		window->type = type;

		//TODO(ollie): Use some alogrithm to find best place to create window 
		//NOTE(ollie): For now just hard code where they start
		Rect2f dim = rect2f(0, 0, 0, 0);

		V2 fullR = state->fuaxResolution;
		V2 halfR = v2_scale(0.5f, state->fuaxResolution); 
		V2 quarterR = v2_scale(0.5f, halfR); 
		switch(type) {
			case WINDOW_TYPE_SCENE_GAME: {
				dim = rect2fMinDim(halfR.x, 0, halfR.x, fullR.y);
				window->dockType = WINDOW_DOCK_RIGHT;
			} break;
			case WINDOW_TYPE_SCENE_LOGIC_BLOCKS: {
				dim = rect2fMinDim(0, 0, quarterR.x, fullR.y);
				window->dockType = WINDOW_DOCK_LEFT;
			} break;
			case WINDOW_TYPE_SCENE_LOGIC_BLOCKS_CHOOSER: {
				dim = rect2fMinDim(quarterR.x, 0, quarterR.x, fullR.y);
				window->dockType = WINDOW_DOCK_MIDDLE;
			} break;
			default: {
				//NOTE(ollie): Case not handled
				assert(false);
			}
		}
		window->dim = dim;
		window->isActive = true;
		window->isVisible = true;
		window->cameraPos = v2(-100, -100);

	} else {
		easyConsole_addToStream(DEBUG_globalEasyConsole, "Reached max scenes");
		result = false;
	}
	return result;
}
////////////////////////////////////////////////////////////////////

static GameState *initGameState(float yOverX_aspectRatio) {
	GameState *state = pushStruct(&globalLongTermArena, GameState);

	///////////////////////************* Setup all game scenes************////////////////////
	state->activeScene = 0;
	state->sceneCount = 0;

	gameState_pushGameScene(state, "Main");
	gameState_pushGameScene(state, "Another Test Scene");

	////////////////////////////////////////////////////////////////////

	///////////////////////************ Ortho matrix to use for rendering UI *************////////////////////
	float fuaxWidth = 1920.0f;
	state->fuaxResolution = v2(fuaxWidth, fuaxWidth*yOverX_aspectRatio);
	state->orthoFuaxMatrix = OrthoMatrixToScreen_BottomLeft(state->fuaxResolution.x, state->fuaxResolution.y); 

	////////////////////////////////////////////////////////////////////

	//NOTE(ollie): Which windows you can see
	state->windowSceneCount = 0;
	pushWindowScene(state, WINDOW_TYPE_SCENE_GAME);
	pushWindowScene(state, WINDOW_TYPE_SCENE_LOGIC_BLOCKS);
	pushWindowScene(state, WINDOW_TYPE_SCENE_LOGIC_BLOCKS_CHOOSER);
	////////////////////////////////////////////////////////////////////

	gameScene_initMenuBar(state);

	////////////////////////////////////////////////////////////////////
	initInteractionState(&state->interactionState);

	return state;
}	



static WindowScene *findWindowByDockType(GameState *gameState, WindowDockType dockType) {
	WindowScene *result = 0;
	for(int i = 0; i < gameState->windowSceneCount && !result; ++i) {
		WindowScene *s = gameState->windowScenes + i;
		if(s->dockType == dockType) {
			result = s;
			break;
		}
	}
	return result;
}

////////////////////////////////////////////////////////////////////