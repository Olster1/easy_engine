typedef enum {
	OVERWORLD_ENTITY_SHIP,
	OVERWORLD_ENTITY_PERSON,
	OVERWORLD_ENTITY_SHOP,
	OVERWORLD_ENTITY_LEVEL,
} OverworldEntityType;

typedef struct {
	OverworldEntityType type;
	EasyTransform T;
} OverworldEntity;

typedef struct {
	u32 entityCount;
	OverworldEntity entities[512];
} OverworldEntityManager;

typedef struct {
	OverworldEntityManager manager;

	Matrix4 viewMatrix;
	Matrix4 projectionMatrix;
} MyOverworldState;

