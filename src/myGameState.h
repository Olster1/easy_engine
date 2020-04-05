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
	MY_GAME_MODE_OVERWORLD,
	MY_GAME_MODE_MISSIONS
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
	MY_WORLD_END_LEVEL = 1 << 7,
	MY_WORLD_EMPTY = 1 << 8,
	MY_WORLD_AMBER = 1 << 9, //NOTE(ollie): For collectables

	////////////////////////////////////////////////////////////////////

} MyWorldFlags;




typedef enum {
	MY_WORLD_BIOME_SPACE,
	MY_WORLD_BIOME_EARTH,
} MyWorldBiomeType;

typedef struct {
	//NOTE(ollie): This is to search for it after we sort, since we have a pointer to it & qsort sorts the data not the pointers
	u32 id;

	u64 flags;
	u32 levelId;

	s32 usedCount;

	bool valid;
} MyWorldTagInfo;	

typedef struct {
	///////////////////////************ Get set on initing the world *************////////////////////
	//NOTE(ollie): About the boss
	bool hasBoss;
	EntityBossType bossType;


	bool hasPuzzle;
	bool hasEnemies;
	bool hasObstacles;

	MyWorldBiomeType biomeType;

	u32 levelCount;
	MyWorldTagInfo levels[1028]; 

	////////////////////////////////////////////////////////////////////

	///////////////////////************ For during the level*************////////////////////	
	//NOTE(ollie): These are altered in the update loop
	bool defeatedBoss;
	bool fightingEnemies;

	//NOTE(ollie): Whether level is about collecting amber
	bool collectingAmber;

	//NOTE(ollie): We know we created a end teleporter, so don't create any more rooms
	bool endedLevel;

	//NOTE(ollie): These are set when we create a new level
	bool inBoss;

	//NOTE(ollie): Probably overkill using u64 but incase player plays for a very long time
	u64 lastRoomId_withPuzzle;
	u64 lastRoomId_withObstacle;
	u64 lastRoomId_withEnemy;

	u64 totalRoomCount;

	//NOTE(ollie): The ideal length of the room. This isn't the exact length because you might be fighting a boss etc. 
	//NOTE(ollie): But after you've been playing for more than this many rooms, and satisfy other criteria, end the level. 
	u32 idealLength;

	////////////////////////////////////////////////////////////////////

} MyWorldState;
	
typedef enum {
	MY_ITEM_HEALTH_POTION,
	MY_ITEM_SHIP_REPAIR,
} MyItemType;

typedef struct {
	MyItemType type;
} MyItem;

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

	///////////////////////************ Missions *************////////////////////
	//NOTE(ollie): Allocated on start up so we can just pass pointer around
	MyMission *missions[128];
	u32 missionCount;

	//NOTE(ollie): For fading mission menu overlay
	float missionLerpValue;


	//NOTE(ollie): Mission complete or mission added notification
	Timer missionNotficationTimer; 
	bool completedMission; //NOTE(ollie): Complete for true or false for added

	////////////////////////////////////////////////////////////////////

	///////////////////////*********** For animation **************////////////////////
	animation_list_item *animationItemFreeListPtr;

	////////////////////////////////////////////////////////////////////

	///////////////////////************* For world generation************////////////////////

	MyWorldState *worldState;
	////////////////////////////////////////////////////////////////////


	///////////////////////************ Player state info *************////////////////////
	//NOTE(ollie): This is like the stuff that defines the stuff that should get saved 
	u32 amberCount; //NOTE(ollie): Like your money in the game

	//NOTE(ollie): Items
	u32 itemCount;
	MyItem items[512];

	////////////////////////////////////////////////////////////////////


	///////////////////////************* For main menu ************////////////////////
	

	particle_system mainMenu_particleSystem;
	////////////////////////////////////////////////////////////////////
} MyGameState;


typedef struct {
	Rect2f r;
	EasyAssetLoader_AssetInfo *m;
} MyGameState_Rect2fModelInfo;