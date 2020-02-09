typedef enum {
	MY_GAME_MODE_START_MENU,
	MY_GAME_MODE_LEVEL_SELECTOR,
	MY_GAME_MODE_PLAY,
	MY_GAME_MODE_PAUSE,
	MY_GAME_MODE_SCORE,
	MY_GAME_MODE_END_ROUND,
	MY_GAME_MODE_INSTRUCTION_CARD,
	MY_GAME_MODE_START,
	MY_GAME_MODE_EDIT_LEVEL
} MyGameMode;

typedef enum {
	GAME_INSTRUCTION_NULL,
	GAME_INSTRUCTION_DROPLET,
	GAME_INSTRUCTION_CHOCOLATE,
	GAME_INSTRUCTION_TOILET,
	GAME_INSTRUCTION_CRAMP,

	////////////////////////////////////////////////////////////////////
	GAME_INSTRUCTION_COUNT,
} GameInstructionType;

typedef struct
{
	MyGameMode lastGameMode;
	MyGameMode currentGameMode;

	//NOTE(ollie): For the incoming animation cards like score card & instruction cards
	Timer animationTimer;
	bool isIn;
	//

	///////////////////////*********** Tutorial Information **************////////////////////
	
	bool tutorialMode;
	bool gameInstructionsHaveRun[GAME_INSTRUCTION_COUNT];
	GameInstructionType instructionType;

	///////////////////////************ For editing levels *************////////////////////

	int currentLevelEditing;
	void *hotEntity; //Don't have Entity type here, so just have a void *
	bool holdingEntity;

	////////////////////////////////////////////////////////////////////
} MyGameState;



static inline MyGameState *myGame_initGameState() {

	MyGameState *gameState = pushStruct(&globalLongTermArena, MyGameState);

	///////////////////////************* Editor stuff ************////////////////////
	gameState->currentLevelEditing = 0;
	gameState->hotEntity = 0;
	gameState->holdingEntity = false;
	////////////////////////////////////////////////////////////////////

	turnTimerOff(&gameState->animationTimer);
	gameState->isIn = false;

	gameState->currentGameMode = gameState->lastGameMode = MY_GAME_MODE_START_MENU;
	setSoundType(AUDIO_FLAG_SCORE_CARD);

	///////////////////////*********** Tutorials **************////////////////////
	gameState->tutorialMode = true;
	for(int i = 0; i < arrayCount(gameState->gameInstructionsHaveRun); ++i) {
	        gameState->gameInstructionsHaveRun[i] = false;
	}
	gameState->instructionType = GAME_INSTRUCTION_NULL;
	////////////////////////////////////////////////////////////////////

	return gameState;
} 


