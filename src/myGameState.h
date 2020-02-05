typedef enum {
	MY_GAME_MODE_START_MENU,
	MY_GAME_MODE_LEVEL_SELECTOR,
	MY_GAME_MODE_PLAY,
	MY_GAME_MODE_PAUSE,
	MY_GAME_MODE_SCORE,
	MY_GAME_MODE_END_ROUND,
	MY_GAME_MODE_INSTRUCTION_CARD,
	MY_GAME_MODE_START,
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

	////////////////////////////////////////////////////////////////////

} MyGameState;




