/*
	How to use: 

	MyEntityManager *entityManager = initEntityManager(&world);
	
	initPlayer(entityManager, EasyModel *model);


	for(;;) {
		updateEntitiesPrePhysics(entityManager, &keyStates, dt);
		
		EasyPhysics_UpdateWorld(&entityManager->world, dt);

		updateEntities(entityManager, &keyStates, globalRenderGroup, viewMatrix, perspectiveMatrix);

		cleanUpEntities(entityManager);
	}
*/

#define MAX_LANE_COUNT 5

typedef enum {
	ENTITY_PLAYER,
	ENTITY_BULLET,
	ENTITY_BUCKET,
	ENTITY_DROPLET,
	ENTITY_ROOM
} EntityType;

typedef enum {
	ENTITY_MOVE_NULL,
	ENTITY_MOVE_LEFT,
	ENTITY_MOVE_RIGHT,
} MoveDirection;

typedef struct MoveOption MoveOption;
typedef struct MoveOption {
	MoveDirection dir;
	MoveOption *next;
	MoveOption *prev;
} MoveOption;

typedef struct {
	EasyTransform T;
	char *name;

	bool active;
	V4 colorTint;

	EntityType type;

	union {
		struct { //Player

			//NOTE(ollie): physics world hold both colliders & rigidbodies

			EasyCollider *collider;
			EasyRigidBody *rb;

			///////

			Timer moveTimer;
			int startLane; //for the move lerp

			MoveOption moveList;
			MoveOption *freeList;

			EasyModel *model;

			int laneIndex;

			int healthPoints;
			int dropletCount;
		};
		struct { //Bullet
			
			//NOTE(ollie): physics world hold both colliders & rigidbodies
			
			EasyCollider *collider;
			EasyRigidBody *rb;  

			//////

		};
		struct { //Bucket
			EasyCollider *collider; //trigger
		};
		struct { //Droplet
			EasyCollider *collider; //trigger
		};
		struct { //Room
			EasyCollider *collider; //trigger
		};
	};

} Entity;



typedef struct {
	Array_Dynamic entities;

	EasyPhysics_World physicsWorld;
} MyEntityManager;

static inline MyEntityManager *initEntityManager() {
	MyEntityManager *result = pushStruct(&globalLongTermArena, MyEntityManager);
	initArray(&result->entities, Entity);

	EasyPhysics_beginWorld(&result->physicsWorld);
	return result;
}

#define GAME_LANE_WIDTH 1

static float getLaneX(int laneIndex) {
	float result = (-2*GAME_LANE_WIDTH)  + (laneIndex * GAME_LANE_WIDTH);
	return result;
}

static void pushDirectionOntoList(MoveDirection dir, Entity *e) {
	assert(e->type == ENTITY_PLAYER);
	MoveOption *op = e->freeList;
	if(op) {
		e->freeList = e->freeList->next;
		
	} else {
		op = pushStruct(&globalLongTermArena, MoveOption);
	}
	assert(op);

	op->next = e->moveList.next;
	op->prev = &e->moveList;
	assert(e->moveList.next->prev == &e->moveList);
	e->moveList.next->prev = op;
	e->moveList.next = op;

	op->dir = dir;
}

static MoveDirection pullDirectionOffList(Entity *e) {
	MoveOption *op = e->moveList.prev;
	MoveDirection res = ENTITY_MOVE_NULL;

	if(op != &e->moveList) {
		assert(op->next == &e->moveList);
		op->prev->next = op->next;
		e->moveList.prev = op->prev;

		res = op->dir;
		op->next = e->freeList;
		e->freeList = op;
		assert(res != ENTITY_MOVE_NULL);
	}

	return res;

}

static inline bool isMoveListEmpty(MoveOption *list) {
	return (list->next == list);
}

///////////////////////************* pre collision update ************////////////////////


static void updateEntitiesPrePhysics(MyEntityManager *manager, AppKeyStates *keyStates, float dt) {
	for(int i = 0; i < manager->entities.count; ++i) {
		Entity *e = (Entity *)getElement(&manager->entities, i);
		if(e && e->active) { //can be null
			switch(e->type) {
				case ENTITY_PLAYER: {
					///////////////////////************ UPDATE PLAYER MOVEMENT *************////////////////////
					

					if(wasPressed(keyStates->gameButtons, BUTTON_LEFT) && e->laneIndex > 0) {
						pushDirectionOntoList(ENTITY_MOVE_LEFT, e);
					} else if(wasPressed(keyStates->gameButtons, BUTTON_RIGHT) && e->laneIndex < (MAX_LANE_COUNT - 1)) {
						pushDirectionOntoList(ENTITY_MOVE_RIGHT, e);
					}

					if(!isMoveListEmpty(&e->moveList) && !isOn(&e->moveTimer)) {
						turnTimerOn(&e->moveTimer);
						MoveDirection dir = pullDirectionOffList(e);
						if(dir == ENTITY_MOVE_RIGHT) {
							assert(e->laneIndex < (MAX_LANE_COUNT - 1));
							e->startLane = e->laneIndex;
							e->laneIndex++;

						} else if (dir == ENTITY_MOVE_LEFT) {
							assert(e->laneIndex > 0);
							e->startLane = e->laneIndex;
							e->laneIndex--;
						} else {
							assert(false);
						}
					}

					if(isOn(&e->moveTimer)) {
						TimerReturnInfo info = updateTimer(&e->moveTimer, dt);

						float a = getLaneX(e->startLane);
						float b = getLaneX(e->laneIndex);

						e->T.pos.x = smoothStep01(a, info.canonicalVal, b);

						if(info.finished) {
							turnTimerOff(&e->moveTimer);
						}
					}

					////////////////////////////////////////////////////////////////////

				} break;
				case ENTITY_BULLET: {
					
				} break;
				case ENTITY_BUCKET: {

				} break;
				case ENTITY_DROPLET: {

				} break;
				default: {
					assert(false);
				}
			}			
		}
	}
}


///////////////////////************ Post collision update *************////////////////////


static void updateEntities(MyEntityManager *manager, AppKeyStates *keyStates, RenderGroup *renderGroup, Matrix4 viewMatrix, Matrix4 perspectiveMatrix) {
	renderSetShader(renderGroup, &phongProgram);
	setViewTransform(renderGroup, viewMatrix);
	setProjectionTransform(renderGroup, perspectiveMatrix);


	for(int i = 0; i < manager->entities.count; ++i) {
		Entity *e = (Entity *)getElement(&manager->entities, i);
		if(e && e->active) { //can be null
			
			setModelTransform(renderGroup, easyTransform_getTransform(&e->T));

			switch(e->type) {
				case ENTITY_PLAYER: {


					

					renderModel(renderGroup, e->model, e->colorTint);
				} break;
				case ENTITY_BULLET: {
					renderModel(renderGroup, e->model, e->colorTint);
				} break;
				case ENTITY_BUCKET: {
					renderModel(renderGroup, e->model, e->colorTint);
				} break;
				case ENTITY_DROPLET: {
					renderModel(renderGroup, e->model, e->colorTint);
				} break;
				default: {
					assert(false);
				}
			}			
		}
	}
}

////////////////////////////////////////////////////////////////////

///////////////////////*************  Clean up at end of frame ************////////////////////


static void cleanUpEntities(MyEntityManager *manager) {
	for(int i = 0; i < manager->entities.count; ++i) {
		Entity *e = (Entity *)getElement(&manager->entities, i);
		if(e && e->active) { //can be null
			if(e->T.markForDeletion) {
				//remove from array
				removeElement_ordered(&manager->entities, i);
			}		
		}
	}
}


////////////////////////////////////////////////////////////////////



///////////////////////************ init entity types *************////////////////////

static Entity *initPlayer(MyEntityManager *m, EasyModel *model) {
	Entity *e = (Entity *)getEmptyElement(&m->entities);

	e->moveTimer = initTimer(0.5f, false);
	turnTimerOff(&e->moveTimer);

	e->name = "Player";
	easyTransform_initTransform(&e->T, v3(0, 0, 0)); 
	e->active = true;
	e->colorTint = COLOR_WHITE;
	e->type = ENTITY_PLAYER;

	e->laneIndex = 2;
	e->dropletCount = 0;
	e->healthPoints = 3;

	e->moveList.next = e->moveList.prev = &e->moveList;
	e->freeList = 0;

	e->model = model;


	////Physics 

	e->rb = EasyPhysics_AddRigidBody(&m->physicsWorld, 1 / 10.0f, 0);
	e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_CIRCLE, NULL_VECTOR3, false, v3(1, 0, 0));

	/////

	return e;

}