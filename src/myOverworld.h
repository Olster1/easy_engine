#define MY_OVERWORLD_ENTITY_TYPE(FUNC) \
FUNC(OVERWORLD_ENTITY_NULL)\
FUNC(OVERWORLD_ENTITY_SHIP)\
FUNC(OVERWORLD_ENTITY_PERSON)\
FUNC(OVERWORLD_ENTITY_SHOP)\
FUNC(OVERWORLD_ENTITY_LEVEL)\
FUNC(OVERWORLD_ENTITY_COLLISION)\

typedef enum {
	MY_OVERWORLD_ENTITY_TYPE(ENUM)
} OverworldEntityType;

static char *MyOverworldEntity_EntityTypeStrings[] = { MY_OVERWORLD_ENTITY_TYPE(STRING) };

typedef struct {
	//NOTE(ollie): The mission
	MyMission *mission;

	//NOTE(ollie): The message & responses
	char *message;
	char *yesAnswer;
	char *noAnswer;
	
} MyMessage;

typedef struct {
	bool transitionToLevel;
	u64 levelFlagsToLoad;
	u32 levelLength;

} MyOverworld_ReturnData;

typedef struct {
	OverworldEntityType type;
	EasyTransform T;
	V4 colorTint;

	//NOTE(ollie): For the human types
	u32 messageCount;
	u32 messageAt;
	MyMessage messages[4];
	bool resetMessage;
	float lerpValue;

	Texture *sprite;

	
	////////////////////////////////////////////////////////////////////

	//NOTE(ollie): For the ship
	//NOTE(ollie): For when we were using the mouse to move
	V3 goalPosition;

	particle_system shipFumes;
	////////////////////////////////////////////////////////////////////
	//NOTE(ollie): To rotate by itself
	float goalAngle;
	//NOTE(ollie): To move with arrow keys
	V2 velocity;
	////////////////////////////////////////////////////////////////////
	float timeSinceLastBoost;

	//NOTE(ollie): For the shop 
	////////////////////////////////////////////////////////////////////

	//NOTE(ollie): For the level 
	u64 levelFlags;
	//NOTE(ollie): Ideal level length, which should be save in the level description file
	u32 levelLength;
	//NOTE(ollie): Telelporter animation 
	animation_list_item animationListSentintel;

	////////////////////////////////////////////////////////////////////
} OverworldEntity;

typedef struct {
	u32 entityCount;
	OverworldEntity entities[512];
} OverworldEntityManager;

typedef struct {
	OverworldEntityManager manager;

	//NOTE(ollie): To edit the overworld
	bool editing;
	OverworldEntityType typeToCreate;
	OverworldEntity *hotEntity;

	EasyCamera camera;

	OverworldEntity *player;
	       
	Matrix4 projectionMatrix;
	Matrix4 orthoMatrix;

	EasyFont_Font *font;
	V2 fauxResolution;

	///////////////////////************ Teleporter animation*************////////////////////
	//NOTE(ollie): Since we only need a reference to the animation, we only have to create it once, then just all levels reference it
	animation teleporterAnimation;
	////////////////////////////////////////////////////////////////////

	//NOTE(ollie): In bounds of text, so don't move player. We're a frame late since so we don't have to loop through the entities twice 
	bool inBounds;
	bool lastInBounds;
} MyOverworldState;



