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
	float lerpValue;
	////////////////////////////////////////////////////////////////////

	//NOTE(ollie): For the ship
	V3 goalPosition;
	float goalAngle;
	////////////////////////////////////////////////////////////////////

	//NOTE(ollie): For the shop 
	////////////////////////////////////////////////////////////////////

	//NOTE(ollie): For the level 
	u32 levelId;
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



