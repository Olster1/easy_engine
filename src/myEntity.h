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

#define LAYER0 0
#define LAYER1 0.1f
#define LAYER2 0.2f
#define LAYER3 0.3f
#define MY_ROOM_HEIGHT 5
#define DEBUG_ENTITY_COLOR 0


#define CHOC_INCREMENT 0.1f


#define MAX_LANE_COUNT 5

static bool DEBUG_global_movePlayer = true;


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
	Texture *sprite;

	EasyModel  *model;

	bool updatedPhysics;
	bool updatedFrame;

	//NOTE(ollie): physics world hold both colliders & rigidbodies

	EasyCollider *collider; //trigger
	EasyRigidBody *rb;  

	////////


	//NOTE(ol): For fading out things
	Timer fadeTimer;
	//

	union {
		struct { //Player

			Timer moveTimer;
			int startLane; //for the move lerp

			MoveOption moveList;
			MoveOption *freeList;

			Texture *sprites[3];

			int laneIndex;

			int healthPoints;
			int dropletCount;

			int dropletCountStore;
		};
		struct { //Bullet
			Timer lifespanTimer;
		};
		struct { //Bucket
			
		};
		struct { //Droplet
			
		};
		struct { //Room
		};
	};

} Entity;

///////////////////////*********** Game Variables **************////////////////////

typedef struct {
	float roomSpeed;	
	float playerMoveSpeed;

	float maxPlayerMoveSpeed;
	float minPlayerMoveSpeed;

	//NOTE: Use Entity ids instead
	Entity *mostRecentRoom; //the one at the front
	Entity *lastRoomCreated; //the one at the back
	int lastLevelIndex;

	//NOTE(ollie): For camera easing
	float cameraTargetPos;

} MyGameStateVariables;

////////////////////////////////////////////////////////////////////

typedef struct {
	EntityType type;
	V3 pos;
} EntityCreateInfo;

typedef struct {
	Array_Dynamic entities;

	EasyPhysics_World physicsWorld;

	int toCreateCount;
	EntityCreateInfo toCreate[512];

} MyEntityManager;

static inline MyEntityManager *initEntityManager() {
	MyEntityManager *result = pushStruct(&globalLongTermArena, MyEntityManager);
	initArray(&result->entities, Entity);

	result->toCreateCount = 0;

	EasyPhysics_beginWorld(&result->physicsWorld);

	return result;
}

static inline void MyEntity_AddToCreateList(MyEntityManager *m, EntityType t, V3 globalPos) {
	assert(m->toCreateCount < arrayCount(m->toCreate));
	//TODO(ollie): Change this to a stretchy buffer

	EntityCreateInfo *info = &m->toCreate[m->toCreateCount++];

	info->type = t;
	info->pos = globalPos;
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

///////////////////////************* Collision helper functions ************////////////////////

typedef struct {
	Entity *e;
	EasyCollisionType collisionType;
	bool found;
} MyEntity_CollisionInfo;

static MyEntity_CollisionInfo MyEntity_hadCollisionWithType(MyEntityManager *manager, EasyCollider *collider, EntityType type, EasyCollisionType colType) {
	
	MyEntity_CollisionInfo result;
	result.found = false;

	for(int i = 0; i < collider->collisionCount && !result.found; ++i) {
		EasyCollisionInfo info = collider->collisions[i]; 
		if(info.type == colType) {
			int id = info.objectId; 
			for(int j = 0; j < manager->entities.count && !result.found; ++j) {
				Entity *e = (Entity *)getElement(&manager->entities, j);
				if(e && e->active) { //can be null
					if(e->T.id == id && e->type == type) {
						result.e = e;
						result.collisionType = info.type;
						result.found = true;
						break;
					}
				}
			}
		}
	} 

	return result;
}


////////////////////////////////////////////////////////////////////

///////////////////////*********** General entity helper functions **************////////////////////

static void MyEntity_MarkForDeletion(EasyTransform *T) {
	T->markForDeletion = true;
}

////////////////////////////////////////////////////////////////////


typedef enum {
	MY_ENTITIES_UPDATE = 1 << 0,
	MY_ENTITIES_RENDER = 1 << 1,
} MyEntityUpdateFlag;

///////////////////////************* pre collision update ************////////////////////


static void updateEntitiesPrePhysics(MyEntityManager *manager, AppKeyStates *keyStates, MyGameStateVariables *variables, float dt) {
	
	///////////////////////*********** Update Game Variables **************////////////////////

	variables->roomSpeed -= 0.1f*dt;

	float maxRoomSpeed = -3.0f;
	if(variables->roomSpeed < maxRoomSpeed) variables->roomSpeed = maxRoomSpeed;


	//NOTE(ollie): increase

	variables->playerMoveSpeed += 0.01f*dt;
	if(variables->playerMoveSpeed > variables->maxPlayerMoveSpeed) variables->playerMoveSpeed = variables->maxPlayerMoveSpeed;

	////////////////////////////////////////////////////////////////////

	for(int i = 0; i < manager->entities.count; ++i) {
		Entity *e = (Entity *)getElement(&manager->entities, i);
		if(e && e->active) { //can be null
			assert(!e->updatedPhysics);
			e->updatedPhysics = true;
			switch(e->type) {
				case ENTITY_PLAYER: {
					///////////////////////************ UPDATE PLAYER MOVEMENT *************////////////////////
					
					if(DEBUG_global_movePlayer) {
						if(wasPressed(keyStates->gameButtons, BUTTON_LEFT)) {
							pushDirectionOntoList(ENTITY_MOVE_LEFT, e);
						} else if(wasPressed(keyStates->gameButtons, BUTTON_RIGHT)) {
							pushDirectionOntoList(ENTITY_MOVE_RIGHT, e);
						}
					}

					if(!isMoveListEmpty(&e->moveList) && !isOn(&e->moveTimer)) {
						
						MoveDirection dir = pullDirectionOffList(e);
						if(dir == ENTITY_MOVE_RIGHT) {
							if(e->laneIndex < (MAX_LANE_COUNT - 1)) {
								e->startLane = e->laneIndex;
								e->laneIndex++;
								turnTimerOn(&e->moveTimer);
							}
						} else if (dir == ENTITY_MOVE_LEFT) {
							if(e->laneIndex > 0) {
								e->startLane = e->laneIndex;
								e->laneIndex--;	
								turnTimerOn(&e->moveTimer);
							}
						} else {
							assert(false);
						}
					}

					//NOTE(ollie): Set the player move speed
					e->moveTimer.period = variables->playerMoveSpeed;

					if(isOn(&e->moveTimer)) {
						TimerReturnInfo info = updateTimer(&e->moveTimer, dt);

						float a = getLaneX(e->startLane);
						float b = getLaneX(e->laneIndex);

						e->T.pos.x = lerp(a, info.canonicalVal, b);

						if(info.finished) {
							turnTimerOff(&e->moveTimer);
						}
					}

					////////////////////////////////////////////////////////////////////

				} break;
				case ENTITY_BULLET: {
					// e->rb->accumTorque = v3(0, 0, 30.0f);
				} break;
				case ENTITY_BUCKET: {

				} break;
				case ENTITY_DROPLET: {

				} break;
				case ENTITY_ROOM: {
					//NOTE(ollie): move downwards
					e->rb->dP.y = variables->roomSpeed;
					
				} break;	
				case ENTITY_CRAMP: {

				} break;
				case ENTITY_CHOC_BAR: {

				} break;
				default: {
					assert(false);
				}
			}			
		}
	}
}

static void myEntity_recusiveDelete(Entity *parent, MyEntityManager *manager) {
	for(int i = 0; i < manager->entities.count; ++i) {
		Entity *e = (Entity *)getElement(&manager->entities, i);
		if(e && e != parent && e->T.parent == &parent->T) {
			myEntity_recusiveDelete(e, manager);
			MyEntity_MarkForDeletion(&e->T);
		}
	}
}

///////////////////////************ Post collision update *************////////////////////

static void updateEntities(MyEntityManager *manager, MyGameStateVariables *gameStateVariables, AppKeyStates *keyStates, RenderGroup *renderGroup, Matrix4 viewMatrix, Matrix4 perspectiveMatrix, float dt, u32 flags) {
	RenderProgram *mainShader = &glossProgram;
	renderSetShader(renderGroup, mainShader);
	setViewTransform(renderGroup, viewMatrix);
	setProjectionTransform(renderGroup, perspectiveMatrix);

	for(int i = 0; i < manager->entities.count; ++i) {
		Entity *e = (Entity *)getElement(&manager->entities, i);
		if(e && e->active) { //can be null
			assert(!e->updatedFrame);
			e->updatedFrame = true;
			
			setModelTransform(renderGroup, easyTransform_getTransform(&e->T));
			if(e->type != ENTITY_ROOM && e->type != ENTITY_PLAYER) assert(e->rb == 0);

			if(flags & MY_ENTITIES_UPDATE) {
				switch(e->type) {
					case ENTITY_PLAYER: {

						if(wasPressed(keyStates->gameButtons, BUTTON_SPACE)) {
							V3 initPos = easyTransform_getWorldPos(&e->T);
							initPos.z -= 0.1f;
							MyEntity_AddToCreateList(manager, ENTITY_BULLET, initPos);
						}

						if(e->dropletCount > 0) {
							e->sprite = e->sprites[1];
						} else {
							e->sprite = e->sprites[0];
						}

						
					} break;
					case ENTITY_BULLET: {

						assert(isOn(&e->lifespanTimer));
						TimerReturnInfo info = updateTimer(&e->lifespanTimer, dt);

						if(info.finished) {
							MyEntity_MarkForDeletion(&e->T);
						}
						
					} break;
					case ENTITY_BUCKET: {
						if(!isOn(&e->fadeTimer) && e->collider->collisionCount > 0) {
							MyEntity_CollisionInfo info = MyEntity_hadCollisionWithType(manager, e->collider, ENTITY_PLAYER, EASY_COLLISION_ENTER);	

							if(info.found) {
								assert(info.e->type == ENTITY_PLAYER);
								info.e->dropletCountStore = info.e->dropletCount; //deposit blood
								info.e->dropletCount = 0;

								playGameSound(&globalLongTermArena, easyAudio_findSound("flush.wav"), 0, AUDIO_FOREGROUND);
							}					
						}
					} break;
					case ENTITY_CHOC_BAR: {
						if(!isOn(&e->fadeTimer) && e->collider->collisionCount > 0) {
							MyEntity_CollisionInfo info = MyEntity_hadCollisionWithType(manager, e->collider, ENTITY_PLAYER, EASY_COLLISION_ENTER);	

							if(info.found) {
								assert(info.e->type == ENTITY_PLAYER);

								gameStateVariables->playerMoveSpeed -= CHOC_INCREMENT;
								if(gameStateVariables->playerMoveSpeed < gameStateVariables->minPlayerMoveSpeed) gameStateVariables->playerMoveSpeed = gameStateVariables->minPlayerMoveSpeed;


								playGameSound(&globalLongTermArena, easyAudio_findSound("bite.wav"), 0, AUDIO_FOREGROUND);

								turnTimerOn(&e->fadeTimer); //chocBar dissapears
							}					
						}
					} break;
					case ENTITY_DROPLET: {
						assert(e->T.parent);
						assert(e->T.parent->parent == 0);
						
						if(!isOn(&e->fadeTimer) && e->collider->collisionCount > 0) {
							MyEntity_CollisionInfo info = MyEntity_hadCollisionWithType(manager, e->collider, ENTITY_PLAYER, EASY_COLLISION_ENTER);	

							if(info.found) {
								assert(info.e->type == ENTITY_PLAYER);
								info.e->dropletCount++;

								turnTimerOn(&e->fadeTimer);
							}					
						}

					} break;
					case ENTITY_CRAMP: {
						if(!isOn(&e->fadeTimer) && e->collider->collisionCount > 0) {
							MyEntity_CollisionInfo info = MyEntity_hadCollisionWithType(manager, e->collider, ENTITY_PLAYER, EASY_COLLISION_ENTER);	

							if(info.found) {
								assert(info.e->type == ENTITY_PLAYER);
								info.e->healthPoints--;
								//check game state 
		
							}	

							info = MyEntity_hadCollisionWithType(manager, e->collider, ENTITY_BULLET, EASY_COLLISION_ENTER);	

							if(info.found) {
								assert(info.e->type == ENTITY_BULLET);
								turnTimerOn(&e->fadeTimer);
							
							}						
						}


					} break;
					case ENTITY_ROOM: {
						V3 roomWorldP = easyTransform_getWorldPos(&e->T);
						if(gameStateVariables->mostRecentRoom == e) { //NOTE(ollie): Is the last room created, so see if it is time to create a new room
							if(roomWorldP.y <= -MY_ROOM_HEIGHT) {
								V3 newPos = easyTransform_getWorldPos(&gameStateVariables->lastRoomCreated->T);
								newPos.y += MY_ROOM_HEIGHT;
								MyEntity_AddToCreateList(manager, ENTITY_ROOM, newPos);
							}	
						}

						assert(e->T.parent == 0);

						#define SAFE_REGION_TO_DESTROY 2
						if(roomWorldP.y <= -SAFE_REGION_TO_DESTROY*MY_ROOM_HEIGHT) {
 							assert(gameStateVariables->mostRecentRoom != e);

 							// for(int i = 0; i < manager->entities.count; ++i) {
 							// 	Entity *e1 = (Entity *)getElement(&manager->entities, i);
 							// 	if(e1 && e1 != e && e1->T.parent == &e->T) {
 							// 		MyEntity_MarkForDeletion(&e1->T);
 							// 	}
 							// }

							myEntity_recusiveDelete(e, manager);
							MyEntity_MarkForDeletion(&e->T);
						}
						
						
					} break;
					default: {
						assert(false);
					}
				}

				if(isOn(&e->fadeTimer)) {
					TimerReturnInfo info = updateTimer(&e->fadeTimer, dt);

					e->colorTint.w = 1.0f - info.canonicalVal;

					if(info.finished) {
						turnTimerOff(&e->fadeTimer);
						MyEntity_MarkForDeletion(&e->T);
					}
				}
			}

			if(e->sprite && (flags & (u32)MY_ENTITIES_RENDER)) {
				renderDrawSprite(renderGroup, e->sprite, e->colorTint);
			}	

			if(e->model && (flags & (u32)MY_ENTITIES_RENDER)) {
				renderSetShader(renderGroup, &phongProgram);
				renderModel(renderGroup, e->model, e->colorTint);
				renderSetShader(renderGroup, mainShader);
			}	

			///////////////////////************ Debug Collider viewing *************////////////////////
			if(e->collider) {

				EasyTransform tempT = e->T;
				
				//NOTE(ollie): Get temp shader to revert state afterwards
				RenderProgram *tempShader = renderGroup->currentShader;

				switch(e->collider->type) {
					case EASY_COLLIDER_CIRCLE: {

						
						renderSetShader(renderGroup, &circleProgram);

						float dim = 2*e->collider->radius;
						tempT.scale = v3(dim, dim, 0);
						setModelTransform(renderGroup, easyTransform_getTransform(&tempT));

						// renderDrawQuad(renderGroup, COLOR_RED);


					} break;
					default: {
						assert(false);
					}
				}

				renderSetShader(renderGroup, tempShader);


			}		

			////////////////////////////////////////////////////////////////////
		}
	}


}

////////////////////////////////////////////////////////////////////


///////////////////////************ init entity types *************////////////////////


#define MY_ENTITY_DEFAULT_DIM 0.4f

//NOTE(ollie): Player
static Entity *initPlayer(MyEntityManager *m, MyGameStateVariables *variables, Texture *empty,  Texture *halfEmpty) {
	Entity *e = (Entity *)getEmptyElement(&m->entities);

	e->moveTimer = initTimer(variables->playerMoveSpeed, false);
	turnTimerOff(&e->moveTimer);

	e->name = "Player";
	easyTransform_initTransform(&e->T, v3(0, 0, LAYER3)); 
	e->active = true;
	e->colorTint = COLOR_WHITE;
	e->type = ENTITY_PLAYER;

	e->laneIndex = 2;
	e->dropletCount = 0;
	e->healthPoints = 3;

	e->moveList.next = e->moveList.prev = &e->moveList;
	e->freeList = 0;

	e->sprite = empty;

	e->sprites[0] = empty;
	e->sprites[1] = halfEmpty;


	e->fadeTimer = initTimer(0.3f, false);
	turnTimerOff(&e->fadeTimer);

	////Physics 

	e->rb = EasyPhysics_AddRigidBody(&m->physicsWorld, 1 / 10.0f, 0);
	e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_CIRCLE, NULL_VECTOR3, false, v3(MY_ENTITY_DEFAULT_DIM, 0, 0));
	/////

	return e;

}

////////////////////////////////////////////////////////////////////

//NOTE(ollie): Bullet

static Entity *initBullet(MyEntityManager *m, Texture *sprite, V3 pos) {
	Entity *e = (Entity *)getEmptyElement(&m->entities);

	e->name = "Bullet";
	pos.z = LAYER1;
	easyTransform_initTransform(&e->T, pos); 
	assert(!e->T.parent);
	e->active = true;
	e->colorTint = COLOR_WHITE;
	e->type = ENTITY_BULLET;

	e->sprite = sprite;


	e->lifespanTimer = initTimer(3.0f, false);
	turnTimerOn(&e->lifespanTimer);	

	e->fadeTimer = initTimer(0.3f, false);
	turnTimerOff(&e->fadeTimer);

	////Physics 

	e->rb = EasyPhysics_AddRigidBody(&m->physicsWorld, 1 / 10.0f, 0);
	e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_CIRCLE, NULL_VECTOR3, true, v3(MY_ENTITY_DEFAULT_DIM, 0, 0));

	/////

	e->rb->dP.y = 1;
	e->rb->dA = v3(0, 0, 1);

	return e;
}

////////////////////////////////////////////////////////////////////

//NOTE(ollie): Droplet

static Entity *initDroplet(MyEntityManager *m, Texture *sprite, V3 pos, Entity *parent) {
	Entity *e = (Entity *)getEmptyElement(&m->entities);

	e->name = "Droplet";
	easyTransform_initTransform(&e->T, pos); 
	if(parent) e->T.parent = &parent->T;
	
	e->active = true;
	#if DEBUG_ENTITY_COLOR
		e->colorTint = COLOR_PINK;
	#else 
		e->colorTint = COLOR_WHITE;
	#endif
	e->type = ENTITY_DROPLET;

	e->sprite = sprite;

	e->fadeTimer = initTimer(0.3f, false);
	turnTimerOff(&e->fadeTimer);

	////Physics 

	e->rb = 0;
	e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_CIRCLE, NULL_VECTOR3, true, v3(MY_ENTITY_DEFAULT_DIM, 0, 0));
	/////

	return e;
}

////////////////////////////////////////////////////////////////////
//NOTE(ollie): Scenery item


//NOTE(ollie): Scenery 1x1 means it takes up 1x1 grid cell
static Entity *initScenery1x1(MyEntityManager *m, EasyModel *model, V3 pos, Entity *parent) {
	Entity *e = (Entity *)getEmptyElement(&m->entities);

	e->name = "Scenery1x1";
	easyTransform_initTransform(&e->T, pos); 
	if(parent) e->T.parent = &parent->T;

	//NOTE(ollie): Set the transform 
	float scale = 0.8f;
	e->T.scale = v3(scale, scale, scale);
	e->T.Q = eulerAnglesToQuaternion(0, -0.5f*PI32, 0);


	e->active = true;
	#if DEBUG_ENTITY_COLOR
			e->colorTint = COLOR_BLUE;
		#else 
			e->colorTint = COLOR_WHITE;
		#endif
	e->type = ENTITY_CRAMP;

	e->model = model;

	e->fadeTimer = initTimer(0.3f, false);
	turnTimerOff(&e->fadeTimer);

	////Physics 
	e->rb = 0;
	e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_CIRCLE, NULL_VECTOR3, true, v3(getDimRect3f(model->bounds).x, 0, 0)); //only the x bounds is actually used for circle types

	/////

	return e;
}


////////////////////////////////////////////////////////////////////


//NOTE(ollie): Cramp
static Entity *initCramp(MyEntityManager *m, Texture *sprite, V3 pos, Entity *parent) {
	Entity *e = (Entity *)getEmptyElement(&m->entities);

	e->name = "Cramp";
	easyTransform_initTransform(&e->T, pos); 
	if(parent) e->T.parent = &parent->T;

	e->active = true;
#if DEBUG_ENTITY_COLOR
	e->colorTint = COLOR_BLUE;
#else 
	e->colorTint = COLOR_WHITE;
#endif
	
	e->type = ENTITY_CRAMP;

	e->sprite = sprite;

	e->fadeTimer = initTimer(0.3f, false);
	turnTimerOff(&e->fadeTimer);

	////Physics 
	e->rb = 0;
	e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_CIRCLE, NULL_VECTOR3, true, v3(MY_ENTITY_DEFAULT_DIM, 0, 0));

	/////

	return e;

}

////////////////////////////////////////////////////////////////////


//NOTE(ollie): Bucket
static Entity *initBucket(MyEntityManager *m, Texture *sprite, V3 pos, Entity *parent) {
	Entity *e = (Entity *)getEmptyElement(&m->entities);

	e->name = "Bucket";
	easyTransform_initTransform(&e->T, pos); 
	if(parent) e->T.parent = &parent->T;

	e->active = true;
	#if DEBUG_ENTITY_COLOR
		e->colorTint = COLOR_RED;
	#else 
		e->colorTint = COLOR_WHITE;
	#endif
	e->type = ENTITY_BUCKET;

	e->sprite = sprite;
	e->fadeTimer = initTimer(0.3f, false);
	turnTimerOff(&e->fadeTimer);

	////Physics 
	e->rb = 0;
	e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_CIRCLE, NULL_VECTOR3, true, v3(MY_ENTITY_DEFAULT_DIM, 0, 0));

	/////

	return e;

}

////////////////////////////////////////////////////////////////////


//NOTE(ollie): Choc bar
static Entity *initChocBar(MyEntityManager *m, Texture *sprite, V3 pos, Entity *parent) {
	Entity *e = (Entity *)getEmptyElement(&m->entities);

	e->name = "Chocolate Bar";
	easyTransform_initTransform(&e->T, pos); 
	if(parent) e->T.parent = &parent->T;

	e->active = true;
	e->colorTint = COLOR_WHITE;
	e->type = ENTITY_CHOC_BAR;

	e->sprite = sprite;
	e->fadeTimer = initTimer(0.3f, false);
	turnTimerOff(&e->fadeTimer);

	////Physics 
	e->rb = 0;
	e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_CIRCLE, NULL_VECTOR3, true, v3(MY_ENTITY_DEFAULT_DIM, 0, 0));

	/////

	return e;

}

////////////////////////////////////////////////////////////////////

//NOTE(ollie): Room
static Entity *initRoom(MyEntityManager *m, V3 pos) {
	Entity *e = (Entity *)getEmptyElement(&m->entities);

	zeroStruct(e, Entity);

	e->sprite = 0;//findTextureAsset("choc_bar.png");

	e->name = "Room";
	easyTransform_initTransform(&e->T, pos); 
	e->T.scale = v3(1, 1, 1);
	e->active = true;
#if DEBUG_ENTITY_COLOR
	e->colorTint = COLOR_GREEN;
#else 
	e->colorTint = COLOR_WHITE;
#endif
	e->type = ENTITY_ROOM;

	e->fadeTimer = initTimer(0.3f, false);
	turnTimerOff(&e->fadeTimer);

	////Physics 
	e->rb = EasyPhysics_AddRigidBody(&m->physicsWorld, 1 / 10.0f, 0);
	e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_CIRCLE, NULL_VECTOR3, true, v3(MY_ENTITY_DEFAULT_DIM, 0, 0));

	/////

	return e;

}



static Entity *myLevels_generateLevel(char *level, MyEntityManager *entityManager, V3 roomPos) {
	Entity *room = initRoom(entityManager, roomPos);

	char *at = level;

	float startX = -2;
	float startY = MY_ROOM_HEIGHT - 1;

	V2 posAt = v2(startX, startY);
	while(*at != '\0') {
		switch(*at) {
			case '*': { //empty space
				posAt.x++;
			} break;
			case '\n': { //empty space
				posAt.y--;
				posAt.x = startX;
			} break;
			case 'e': { //choc bar
				initChocBar(entityManager, findTextureAsset("choc_bar.png"), v3(posAt.x, posAt.y, LAYER0), room);
				posAt.x++;
			} break;
			case 'c': { //cramp
				initCramp(entityManager, findTextureAsset("cramp.PNG"), v3(posAt.x, posAt.y, LAYER0), room);
				posAt.x++;
			} break;
			case 'd': { //droplet
				initDroplet(entityManager, findTextureAsset("blood_droplet.PNG"), v3(posAt.x, posAt.y, LAYER0), room);
				posAt.x++;
			} break;
			case 's': { //droplet
				initScenery1x1(entityManager, findModelAsset("Crystal.obj"), v3(posAt.x, posAt.y, LAYER0), room);
				posAt.x++;
			} break;
			case 't': { //toilet
				initBucket(entityManager, findTextureAsset("toilet1.png"), v3(posAt.x, posAt.y, LAYER0), room);
				posAt.x++;
			} break;
			default: {

			} 
		}

		at++;
	}

	return room;

}

////////////////////////////////////////////////////////////////////


///////////////////////*************  Clean up at end of frame ************////////////////////


static void cleanUpEntities(MyEntityManager *manager, MyGameStateVariables *variables) {
	for(int i = 0; i < manager->entities.count; ++i) {
		Entity *e = (Entity *)getElement(&manager->entities, i);
		if(e) { //can be null
			e->updatedPhysics = false;
			e->updatedFrame = false;

			if(e->T.markForDeletion) {
				//remove from array
				e->active = false;
				if(e->collider) {
					removeElement_ordered(&manager->physicsWorld.colliders, e->collider->arrayIndex);
					memset(e->collider, 0, sizeof(EasyCollider));
					e->collider = 0;
				}

				if(e->rb) {
					removeElement_ordered(&manager->physicsWorld.rigidBodies, e->rb->arrayIndex);
					memset(e->rb, 0, sizeof(EasyRigidBody));
					e->rb = 0;
				}
				memset(e, 0, sizeof(Entity));
				assert(e->T.parent == 0);
				removeElement_ordered(&manager->entities, i);
			}		
		}
	}

	for(int i = 0; i < manager->toCreateCount; ++i) {
		EntityCreateInfo info = manager->toCreate[i];

		switch(info.type) {
			case ENTITY_BULLET: {
				initBullet(manager, findTextureAsset("tablet.png"), info.pos);
			} break;
			case ENTITY_ROOM: {
				variables->lastLevelIndex = myLevels_getLevel(variables->lastLevelIndex); 
				//Only have two rooms at a time
				variables->mostRecentRoom = variables->lastRoomCreated;
				variables->lastRoomCreated = myLevels_generateLevel(myLevels_getLevelForIndex(variables->lastLevelIndex), manager, info.pos);
			
			} break;
			default: {
				assert(false);//not handled
			}
		}


	}

	manager->toCreateCount = 0;
}



////////////////////////////////////////////////////////////////////