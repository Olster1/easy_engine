#define MY_OVERWORLD_ENTITY_TYPE(FUNC) \
FUNC(OVERWORLD_ENTITY_NULL)\
FUNC(OVERWORLD_ENTITY_SHIP)\
FUNC(OVERWORLD_ENTITY_PERSON)\
FUNC(OVERWORLD_ENTITY_SHOP)\
FUNC(OVERWORLD_ENTITY_LEVEL)\

typedef enum {
	MY_OVERWORLD_ENTITY_TYPE(ENUM)
} OverworldEntityType;

static char *MyOverworldEntity_EntityTypeStrings[] = { MY_OVERWORLD_ENTITY_TYPE(STRING) };

typedef struct {
	OverworldEntityType type;
	EasyTransform T;
	V4 colorTint;

	//NOTE(ollie): For the human types
	char *message;
	////////////////////////////////////////////////////////////////////

	//NOTE(ollie): For the ship
	V3 goalPosition;
	float goalAngle;
	////////////////////////////////////////////////////////////////////

	//NOTE(ollie): For the shop 
	////////////////////////////////////////////////////////////////////

	//NOTE(ollie): For the level 
	u32 levelId;
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
	       
	Matrix4 projectionMatrix;

	V2 screenResolution;
} MyOverworldState;



