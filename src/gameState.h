typedef struct {
	char *name;
	
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
	} else {
		easyConsole_addToStream(DEBUG_globalEasyConsole, "Reached max scenes");
	}
	
}

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

	gameScene_initMenuBar(state);

	return state;
}	


////////////////////////////////////////////////////////////////////