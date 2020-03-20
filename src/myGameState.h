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
	MY_WORLD_NULL = 0,
	MY_WORLD_BOSS = 1 << 0,
	MY_WORLD_FIRE_BOSS = 1 << 1,
	MY_WORLD_PUZZLE = 1 << 2,
	MY_WORLD_ENEMIES = 1 << 3,
	MY_WORLD_OBSTACLES = 1 << 4,
	MY_WORLD_SPACE = 1 << 5,
	MY_WORLD_EARTH = 1 << 6,

	////////////////////////////////////////////////////////////////////

} MyWorldFlags;


typedef enum {
	MY_VIEW_ANGLE_BOTTOM = 0,
	MY_VIEW_ANGLE_RIGHT = 1,
	MY_VIEW_ANGLE_TOP = 2,
	MY_VIEW_ANGLE_LEFT = 3,
} MyGameState_ViewAngle;


typedef enum {
	MY_WORLD_BIOME_SPACE,
	MY_WORLD_BIOME_EARTH,
} MyWorldBiomeType;

typedef struct {
	u64 flags;
	u32 levelId;

	s32 usedCount;
	bool valid;
} MyWorldTagInfo;	

typedef struct {
	//NOTE(ollie): About the boss
	bool hasBoss;
	EntityBossType bossType;
	bool defeatedBoss;
	bool inBoss;

	bool hasPuzzle;
	bool hasEnemies;
	bool hasObstacles;

	bool fightingEnemies;

	MyWorldBiomeType biomeType;

	//NOTE(ollie): For during the level
	s32 roomsSincePuzzle;
	s32 roomsSinceObstacle;
	s32 roomsSinceEnemies;

	s32 totalRoomCount;

	u32 levelCount;
	MyWorldTagInfo levels[1028]; 

} MyWorldState;


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
	Entity *hotEntity; //Don't have Entity type here, so just have a void *
	bool holdingEntity;

	Entity *currentRoomBeingEdited;

	EntityBossType editorSelectedBossType;

	//NOTE(ollie): Tags for the level
	char *currentRoomTags[256];
	s32 currentRoomTagCount;

	///////////////////////************ For the editor level, selecting models from the drop down *************////////////////////
	int modelSelected;

	//NOTE(ollie): For the altitude slider
	float altitudeSliderAt;
	bool holdingAltitudeSlider;

	////////////////////////////////////////////////////////////////////

	///////////////////////*********** For animation **************////////////////////
	animation_list_item *animationItemFreeListPtr;

	////////////////////////////////////////////////////////////////////

	///////////////////////************* For world generation************////////////////////

	MyWorldState *worldState;
	////////////////////////////////////////////////////////////////////

} MyGameState;


typedef struct {
	Rect2f r;
	EasyAssetLoader_AssetInfo *m;
} MyGameState_Rect2fModelInfo;