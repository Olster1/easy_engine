typedef enum {
	MY_GAME_MODE_START_MENU,
	MY_GAME_MODE_LEVEL_SELECTOR,
	MY_GAME_MODE_PLAY,
	MY_GAME_MODE_PAUSE,
	MY_GAME_MODE_SCORE,
	MY_GAME_MODE_START,
} MyGameMode;


typedef struct
{
	MyGameMode lastGameMode;
	MyGameMode currentGameMode;
} MyGameState;


