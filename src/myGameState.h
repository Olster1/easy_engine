typedef enum {
	MY_GAME_MODE_START_MENU,
	MY_GAME_MODE_LEVEL_SELECTOR,
	MY_GAME_MODE_PLAY,
	MY_GAME_MODE_PAUSE,
	MY_GAME_MODE_SCORE,
	MY_GAME_MODE_END_ROUND,
	MY_GAME_MODE_INSTRUCTION_CARD,
	MY_GAME_MODE_START,
	MY_GAME_MODE_EDIT_LEVEL,
	MY_GAME_MODE_OVERWORLD
} MyGameMode;

typedef enum {
	GAME_INSTRUCTION_NULL,
	GAME_INSTRUCTION_DROPLET,
	GAME_INSTRUCTION_CHOCOLATE,
	GAME_INSTRUCTION_TOILET,
	GAME_INSTRUCTION_CRAMP,
	GAME_INSTRUCTION_UNDERPANTS,

	////////////////////////////////////////////////////////////////////
	GAME_INSTRUCTION_COUNT,
} GameInstructionType;

typedef enum {
	MY_VIEW_ANGLE_BOTTOM = 0,
	MY_VIEW_ANGLE_RIGHT = 1,
	MY_VIEW_ANGLE_TOP = 2,
	MY_VIEW_ANGLE_LEFT = 3,
} MyGameState_ViewAngle;


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

	int entityTypeSelected;
	int currentLevelEditing;
	void *hotEntity; //Don't have Entity type here, so just have a void *
	bool holdingEntity;

	void *currentRoomBeingEdited;


	///////////////////////************ For the editor level, selecting models from the drop down *************////////////////////
	int modelSelected;

	//NOTE(ollie): For the altitude slider
	float altitudeSliderAt;
	bool holdingAltitudeSlider;

	////////////////////////////////////////////////////////////////////

	///////////////////////*********** For animation **************////////////////////
	animation_list_item *animationItemFreeListPtr;

	////////////////////////////////////////////////////////////////////

} MyGameState;


typedef struct {
	Rect2f r;
	EasyAssetLoader_AssetInfo *m;
} MyGameState_Rect2fModelInfo;