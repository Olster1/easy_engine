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
#define LAYER4 0.8f

static float globalTileScale = 0.9f;

#define MY_ROOM_HEIGHT 5
#define DEBUG_ENTITY_COLOR 0

#define CHOC_INCREMENT 0.1f


#define MAX_LANE_COUNT 5

static bool DEBUG_global_movePlayer = true;

#define MY_ENTITY_TYPE(FUNC) \
FUNC(ENTITY_NULL)\
FUNC(ENTITY_PLAYER)\
FUNC(ENTITY_BULLET)\
FUNC(ENTITY_BUCKET)\
FUNC(ENTITY_DROPLET)\
FUNC(ENTITY_CHOC_BAR)\
FUNC(ENTITY_ROOM)\
FUNC(ENTITY_CRAMP)\
FUNC(ENTITY_SCENERY)\
FUNC(ENTITY_STAR)\
FUNC(ENTITY_TILE)\
FUNC(ENTITY_ASTEROID)\
FUNC(ENTITY_SOUND_CHANGER)\
FUNC(ENTITY_UNDERPANTS)\
FUNC(ENTITY_TELEPORTER)\

typedef enum {
	MY_ENTITY_TYPE(ENUM)
} EntityType;

static char *MyEntity_EntityTypeStrings[] = { MY_ENTITY_TYPE(STRING) };


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
		struct { //Teleporter
			animation idleAnimation;
			animation_list_item animationListSentintel;
		};
		struct { //Player

			Timer moveTimer;
			int startLane; //for the move lerp

			MoveOption moveList;
			MoveOption *freeList;

			Texture *sprites[3];

			int laneIndex;

			int healthPoints;
			int dropletCount;
			int maxHealthPoints;

			int dropletCountStore;
			MoveDirection moveDirection;

			//NOTE(ollie): This is for the player flashing when hurt, & can't get hurt if it's on
			Timer hurtTimer; 
			Timer invincibleTimer;
			Timer restartToMiddleTimer;
			float lastValueForTimer;

		};
		struct { //Bullet
			Timer lifespanTimer;
			Timer bulletColorTimer;
		};
		struct { //Bucket
			
		};
		struct { //Droplet
			
		};
		struct { //Room
		};
		struct { //sound changer
			WavFile *soundToPlay;
			
		};
	};

} Entity;

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


	//NOTE(ollie): This is for splatting the blood splat
	int liveTimerCount;
	Timer liveTimers[10];

	//NOTE(ollie): For making the score screen have a loading feel to it
	Timer pointLoadTimer;
	int lastCount; //So we can make a click noise when it changes
	//

	//NOTE(ollie): Boost pad 
	float cachedSpeed;
	Timer boostTimer;
	//

	///////////////////////************* For editor level for lerping the camera ************////////////////////
	MyGameState_ViewAngle angleType;
	float angleDegreesAltitude;
	Timer lerpTimer; //NOTE(ollie): Lerping between positions

	//NOTE(ollie): Camera positions
	V3 centerPos;
	V3 targetCenterPos; //we ease towards the target position & update the center position using the keys

	V3 startPos; //of lerp

	////////////////////////////////////////////////////////////////////

	//NOTE(ollie): For camera easing
	float cameraDistance;
	float cameraTargetPos;

} MyGameStateVariables;

////////////////////////////////////////////////////////////////////


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
	assert(e->type == ENTITY_PLAYER);

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

inline static void easyEntity_emptyMoveList(Entity *e) {
	assert(e->type == ENTITY_PLAYER);

	while(e->moveList.next != &e->moveList) {
		//keep pulling them all off
		pullDirectionOffList(e);
	}
} 


static inline void myEntity_setPlayerPosToMiddle(Entity *player) {
	player->laneIndex = 2;
	player->T.pos.x = getLaneX(player->laneIndex);
	turnTimerOff(&player->moveTimer);
}

static inline void myEntity_restartPlayerInMiddle(Entity *player) {
	turnTimerOff(&player->moveTimer);
	easyEntity_emptyMoveList(player);
	if(player->laneIndex == 2) { //already in the middle
		myEntity_setPlayerPosToMiddle(player);
	} else {
		//NOTE(ollie): Fade out to the middle
		turnTimerOn(&player->restartToMiddleTimer);	
	}
	
}

static inline void myEntity_spinEntity(Entity *e) {
	if(e->rb) {
		e->rb->dA = v3(0, 13, 0);
	} else {
		assert(false);
	}
}

static inline Entity *MyEntity_findEntityByName(char *name) {
	assert(false);//not implemented
	return 0;
}


////////////////////////////////////////////////////////////////////


typedef enum {
	MY_ENTITIES_UPDATE = 1 << 0,
	MY_ENTITIES_RENDER = 1 << 1,
} MyEntityUpdateFlag;

///////////////////////************* pre collision update ************////////////////////


static void updateEntitiesPrePhysics(MyEntityManager *manager, AppKeyStates *keyStates, MyGameStateVariables *variables, float dt) {
	DEBUG_TIME_BLOCK();
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
					
					MoveDirection newDirection = ENTITY_MOVE_NULL;
					if(DEBUG_global_movePlayer) {
						if(wasPressed(keyStates->gameButtons, BUTTON_LEFT)) {
							newDirection = ENTITY_MOVE_LEFT;
						} else if(wasPressed(keyStates->gameButtons, BUTTON_RIGHT)) {
							newDirection = ENTITY_MOVE_RIGHT;
						}
					}


					///////////////////////************ Get move off list if there are any *************////////////////////

					if(!isOn(&e->moveTimer)) {
						//NOTE(ollie): If we are finished moving we are going to check any moves are on the list
						MoveDirection dir = pullDirectionOffList(e);

						if(dir != ENTITY_MOVE_NULL) {
							if(dir == ENTITY_MOVE_RIGHT && e->laneIndex < (MAX_LANE_COUNT - 1)) {
								e->startLane = e->laneIndex;
								e->laneIndex++;
								turnTimerOn(&e->moveTimer);
								playGameSound(&globalLongTermArena, easyAudio_findSound("boosterSound.wav"), 0, AUDIO_FOREGROUND);
								e->moveDirection = dir;
							}
							if(dir == ENTITY_MOVE_LEFT && e->laneIndex > 0) {
								e->startLane = e->laneIndex;
								e->laneIndex--;
								turnTimerOn(&e->moveTimer);
								playGameSound(&globalLongTermArena, easyAudio_findSound("boosterSound.wav"), 0, AUDIO_FOREGROUND);
								e->moveDirection = dir;
							}
							
						}
					}
					////////////////////////////////////////////////////////////////////

					if(newDirection != ENTITY_MOVE_NULL) {
						
						if(newDirection == ENTITY_MOVE_RIGHT) {
							if(e->laneIndex < (MAX_LANE_COUNT - 1)) {
								
								bool isSafe = !(e->moveDirection == ENTITY_MOVE_RIGHT && isOn(&e->moveTimer));
								
								if(isSafe) {
									e->startLane = e->laneIndex;
									e->laneIndex++;

									if(isOn(&e->moveTimer)) {
										assert(e->moveDirection == ENTITY_MOVE_LEFT);
										e->moveTimer.value_ = e->moveTimer.period - e->moveTimer.value_;
										easyEntity_emptyMoveList(e);
									} else {
										turnTimerOn(&e->moveTimer);
										playGameSound(&globalLongTermArena, easyAudio_findSound("boosterSound.wav"), 0, AUDIO_FOREGROUND);
									}
								} else {
									//Already moving in that direction, so add it to the list
									pushDirectionOntoList(newDirection, e);
								}
								e->moveDirection = ENTITY_MOVE_RIGHT;
							}
						} else if (newDirection == ENTITY_MOVE_LEFT) {
							if(e->laneIndex > 0) {
								bool isSafe = !(e->moveDirection == ENTITY_MOVE_LEFT && isOn(&e->moveTimer));
								
								if(isSafe) {
									e->startLane = e->laneIndex;
									e->laneIndex--;	

									if(isOn(&e->moveTimer)) {
										assert(e->moveDirection == ENTITY_MOVE_RIGHT);
										e->moveTimer.value_ = e->moveTimer.period - e->moveTimer.value_;
										easyEntity_emptyMoveList(e);
									} else {
										turnTimerOn(&e->moveTimer);
										playGameSound(&globalLongTermArena, easyAudio_findSound("boosterSound.wav"), 0, AUDIO_FOREGROUND);
									}
								} else {
									//Already moving in that direction, so add it to the list
									pushDirectionOntoList(ENTITY_MOVE_LEFT, e);
								}
								e->moveDirection = ENTITY_MOVE_LEFT;
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

						float shipRotation = easyMath_degreesToRadians(smoothStep00(0, info.canonicalVal, 90.0f));

						if(e->moveDirection == ENTITY_MOVE_RIGHT) {
							shipRotation *= -1;
						}

						//NOTE(ollie): 0.5f*PI32 is so the ship is around the right way - the resting position
						e->T.Q = eulerAnglesToQuaternion(shipRotation, 0.5f*PI32, 0);						

						if(info.finished) {
							turnTimerOff(&e->moveTimer);
						}
					}

					////////////////////////////////////////////////////////////////////

					///////////////////////************ Update the hurt & invincible timers *************////////////////////

					if(isOn(&e->hurtTimer) && !isOn(&e->restartToMiddleTimer)) {
						TimerReturnInfo info = updateTimer(&e->hurtTimer, dt);

						e->colorTint.w = smoothStep01010(1, info.canonicalVal, 0);
						e->colorTint.x = e->colorTint.y = e->colorTint.z = 1;
						if(info.finished) {
							turnTimerOff(&e->hurtTimer);
							e->colorTint.w = 1;
						}
					}
					////////////////////////////////////////////////////////////////////
					if(isOn(&e->invincibleTimer)) {
						TimerReturnInfo info = updateTimer(&e->invincibleTimer, dt);

						float hueVal = 10*info.canonicalVal*360.0f;

						hueVal -= ((int)(hueVal / 360.0f))*360;

						e->colorTint = easyColor_hsvToRgb(hueVal, 1, 1);
						if(info.finished) {
							turnTimerOff(&e->invincibleTimer);
							e->colorTint = COLOR_WHITE;
							variables->roomSpeed = variables->cachedSpeed;
						}
					}
					////////////////////////////////////////////////////////////////////
					if(isOn(&e->restartToMiddleTimer)) {
						TimerReturnInfo info = updateTimer(&e->restartToMiddleTimer, dt);
						
						float t = info.canonicalVal;
						if(t < 0.5f) {
							e->colorTint.w = lerp(1, t / 0.5f, 0);	
						} else {
							e->colorTint.w = lerp(0, (t - 0.5f) / 0.5f, 1);
							if(e->lastValueForTimer < 0.5f) {
								myEntity_setPlayerPosToMiddle(e);	
							}
							
						}

						e->lastValueForTimer = info.canonicalVal;
						
						if(info.finished) {
							turnTimerOff(&e->restartToMiddleTimer);
							e->colorTint = COLOR_WHITE;
						}
					}

					////////////////////////////////////////////////////////////////////
					if(isOn(&variables->boostTimer)) {
						TimerReturnInfo info = updateTimer(&variables->boostTimer, dt);
						if(info.finished) {
							turnTimerOff(&variables->boostTimer);
							variables->roomSpeed = variables->cachedSpeed;
						}
					}

					
					


				} break;
				case ENTITY_TELEPORTER: {

				} break;
				case ENTITY_BULLET: {
					// e->rb->accumTorque = v3(0, 0, 30.0f);
				} break;
				case ENTITY_ASTEROID: {
					if(!DEBUG_global_PauseGame) {
						e->rb->dP.x = 5;
					}
				} break;
				case ENTITY_SOUND_CHANGER: {

				} break;
				case ENTITY_BUCKET: {

				} break;
				case ENTITY_DROPLET: {

				} break;
				case ENTITY_UNDERPANTS: {

				} break;
				case ENTITY_ROOM: {
					if(!DEBUG_global_PauseGame) {
						//NOTE(ollie): move downwards
						e->rb->dP.y = variables->roomSpeed;	
						e->T.pos.y += dt*variables->roomSpeed;
					}
					
				} break;	
				case ENTITY_CRAMP: {

				} break;
				case ENTITY_STAR: {

				} break;
				case ENTITY_SCENERY: {

				} break;
				case ENTITY_TILE: {

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

///////////////////////*********** Helper functions **************////////////////////

static void myEntity_recusiveDelete(Entity *parent, MyEntityManager *manager) {
	for(int i = 0; i < manager->entities.count; ++i) {
		Entity *e = (Entity *)getElement(&manager->entities, i);
		if(e && e != parent && e->T.parent == &parent->T) {
			myEntity_recusiveDelete(e, manager);
			MyEntity_MarkForDeletion(&e->T);
		}
	}
}

void easyEntity_endRound(MyEntityManager *manager) {
	for(int i = 0; i < manager->entities.count; ++i) {
		Entity *e = (Entity *)getElement(&manager->entities, i);
		if(e) { //can be null
			if(e->type == ENTITY_PLAYER) {
				//NOTE: Do nothing
			} else {
				e->T.markForDeletion = true;	
			}
		}
	}
}

static inline void myEntity_addInstructionCard(MyGameState *gameState, GameInstructionType instructionType) {
	if(gameState->tutorialMode && !gameState->gameInstructionsHaveRun[instructionType]) {
		gameState->gameInstructionsHaveRun[instructionType] = true;
		gameState->instructionType = instructionType;
		
		gameState->animationTimer = initTimer(0.5f, false);
		gameState->isIn = true;

		gameState->currentGameMode = MY_GAME_MODE_INSTRUCTION_CARD;
	}
}

static inline void myEntity_checkHealthPoints(Entity *player, MyGameState *gameState, MyGameStateVariables *gameStateVariables) {
	if(player->healthPoints < 0) {
		assert(player->type == ENTITY_PLAYER);

		player->healthPoints = player->maxHealthPoints;
		//check game state

		gameState->lastGameMode = gameState->currentGameMode;
		gameState->currentGameMode = MY_GAME_MODE_END_ROUND;

		setParentChannelVolume(AUDIO_FLAG_MAIN, 0, 0);
		setParentChannelVolume(AUDIO_FLAG_SCORE_CARD, 1, 0);

		setSoundType(AUDIO_FLAG_SCORE_CARD);

		gameStateVariables->pointLoadTimer = initTimer(0.3f*player->dropletCountStore, false);
		gameStateVariables->lastCount = 0;

		gameState->animationTimer = initTimer(0.5f, false);
		gameState->isIn = true;

	}
}

static inline void myEntity_decreasePlayerHealthPoint(Entity *player, MyGameStateVariables *gameStateVariables) {
	assert(player->type == ENTITY_PLAYER);

	player->healthPoints--;
	
	assert(gameStateVariables->liveTimerCount < arrayCount(gameStateVariables->liveTimers));
	Timer *healthTimer = gameStateVariables->liveTimers + gameStateVariables->liveTimerCount++;

	*healthTimer = initTimer(0.5f, false);
	turnTimerOn(healthTimer);

	turnTimerOff(&player->invincibleTimer);
	turnTimerOn(&player->hurtTimer);

	myEntity_restartPlayerInMiddle(player);

}

static inline bool myEntity_canPlayerBeHurt(Entity *player) {
	assert(player->type == ENTITY_PLAYER);
	bool result = true;

	if(isOn(&player->invincibleTimer) || isOn(&player->hurtTimer)) {
		result = false;
	}

	return result;
}	

static inline void myEntity_startBoostPad(MyGameStateVariables *gameStateVariables, bool turnBoostTimerOn) {
	if(turnBoostTimerOn) {
		turnTimerOn(&gameStateVariables->boostTimer);	
	}
	
	gameStateVariables->cachedSpeed = gameStateVariables->roomSpeed;
	gameStateVariables->roomSpeed *= 2;

}

///////////////////////************ Post collision update *************////////////////////

static void updateEntities(MyEntityManager *manager, MyGameState *gameState, MyGameStateVariables *gameStateVariables, AppKeyStates *keyStates, RenderGroup *renderGroup, Matrix4 viewMatrix, Matrix4 perspectiveMatrix, float dt, u32 flags) {
	DEBUG_TIME_BLOCK();
	RenderProgram *mainShader = &glossProgram;
	renderSetShader(renderGroup, mainShader);
	setViewTransform(renderGroup, viewMatrix);
	setProjectionTransform(renderGroup, perspectiveMatrix);

	for(int i = 0; i < manager->entities.count; ++i) {
		Entity *e = (Entity *)getElement(&manager->entities, i);
		if(e && e->active) { //can be null
			assert(!e->updatedFrame);
			e->updatedFrame = true;
			
			
			// if(e->type != ENTITY_ROOM && e->type != ENTITY_PLAYER) assert(e->rb == 0);

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

						TimerReturnInfo info = updateTimer(&e->lifespanTimer, dt);

						///////////////////////************ Update the color of the bullet *************////////////////////
						if(isOn(&e->bulletColorTimer)) {
							TimerReturnInfo info = updateTimer(&e->bulletColorTimer, dt);

							float hueVal = 10*info.canonicalVal*360.0f;

							hueVal -= ((int)(hueVal / 360.0f))*360;

							e->colorTint = easyColor_hsvToRgb(hueVal, 1, 1);

							if(info.finished) {
								turnTimerOn(&e->bulletColorTimer);
							}
						}
						////////////////////////////////////////////////////////////////////


						if(info.finished) {
							turnTimerOn(&e->fadeTimer);
						}
						
					} break;
					case ENTITY_BUCKET: {
						if(!isOn(&e->fadeTimer) && e->collider->collisionCount > 0) {
							MyEntity_CollisionInfo info = MyEntity_hadCollisionWithType(manager, e->collider, ENTITY_PLAYER, EASY_COLLISION_ENTER);	

							if(info.found) {
								assert(info.e->type == ENTITY_PLAYER);
								info.e->dropletCountStore += info.e->dropletCount; //deposit blood
								info.e->dropletCount = 0;

								playGameSound(&globalLongTermArena, easyAudio_findSound("flush.wav"), 0, AUDIO_FOREGROUND);

								myEntity_addInstructionCard(gameState, GAME_INSTRUCTION_TOILET);

								myEntity_spinEntity(e);
							}					
						}
					} break;
					case ENTITY_TELEPORTER: {
						//NOTE(ollie): Update the animation

						char *animationOn = UpdateAnimation(&gameState->animationItemFreeListPtr, &globalLongTermArena, &e->animationListSentintel, dt, 0);
						e->sprite = findTextureAsset(animationOn);
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
								myEntity_spinEntity(e);

								myEntity_addInstructionCard(gameState, GAME_INSTRUCTION_CHOCOLATE);
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
								myEntity_spinEntity(e);
								playGameSound(&globalLongTermArena, easyAudio_findSound("ting.wav"), 0, AUDIO_FOREGROUND);
								

								myEntity_addInstructionCard(gameState, GAME_INSTRUCTION_DROPLET);
							}					
						}

					} break;
					case ENTITY_UNDERPANTS: {
						assert(e->T.parent);
						assert(e->T.parent->parent == 0);
						
						if(!isOn(&e->fadeTimer) && e->collider->collisionCount > 0) {
							MyEntity_CollisionInfo info = MyEntity_hadCollisionWithType(manager, e->collider, ENTITY_PLAYER, EASY_COLLISION_ENTER);	

							if(info.found) {
								assert(info.e->type == ENTITY_PLAYER);
								info.e->dropletCount++;

								Entity *player = info.e;

								turnTimerOn(&e->fadeTimer);
								myEntity_spinEntity(e);
								playGameSound(&globalLongTermArena, easyAudio_findSound("ting.wav"), 0, AUDIO_FOREGROUND);
									
								if(player->healthPoints < player->maxHealthPoints) {
									//can take one back
									player->healthPoints++;
									
									assert(gameStateVariables->liveTimerCount > 0);
									gameStateVariables->liveTimerCount--; //revert the live timer
								}

								myEntity_addInstructionCard(gameState, GAME_INSTRUCTION_UNDERPANTS);
							}					
						}

					} break;
					case ENTITY_TILE: {
						if(easyTransform_getWorldPos(&e->T).y < 0) {
							e->T.pos.z -= dt;

						}
						e->T.scale = v3(globalTileScale, globalTileScale, globalTileScale);
					} break;
					case ENTITY_SCENERY: {
					// 	if(e->collider->collisionCount > 0) {
					// 		MyEntity_CollisionInfo info = MyEntity_hadCollisionWithType(manager, e->collider, ENTITY_PLAYER, EASY_COLLISION_STAY);	

					// 		if(info.found) {
					// 			assert(info.e->type == ENTITY_PLAYER);
					// 			Entity *player = info.e;

					// 			if(myEntity_canPlayerBeHurt(player)) {
					// 				playGameSound(&globalLongTermArena, easyAudio_findSound("splatSound.wav"), 0, AUDIO_FOREGROUND);
					// 				myEntity_decreasePlayerHealthPoint(player, gameStateVariables);
					// 				myEntity_checkHealthPoints(player, gameState, gameStateVariables);
					// 			}
					// 		}
					// 	}
					} break;
					case ENTITY_STAR: {
						if(!isOn(&e->fadeTimer) && e->collider->collisionCount > 0) {

							///////////////////////*********** Collision with Player **************////////////////////
							MyEntity_CollisionInfo info = MyEntity_hadCollisionWithType(manager, e->collider, ENTITY_PLAYER, EASY_COLLISION_STAY);	

							if(info.found) {
								assert(info.e->type == ENTITY_PLAYER);
								Entity *player = info.e;

								if(!isOn(&player->invincibleTimer)) {

									myEntity_startBoostPad(gameStateVariables, false);
									playGameSound(&globalLongTermArena, easyAudio_findSound("starSound.wav"), 0, AUDIO_FOREGROUND);
									
									turnTimerOn(&player->invincibleTimer);

									////////////////////////////////////////////////////////////////////

									//NOTE(ollie): Star is removed after it fades out
									turnTimerOn(&e->fadeTimer);
									myEntity_spinEntity(e);
								}
							}
						} 
					} break;
					case ENTITY_ASTEROID: {
						if(!isOn(&e->fadeTimer) && e->collider->collisionCount > 0) {

							///////////////////////*********** Collision with Player **************////////////////////
							MyEntity_CollisionInfo info = MyEntity_hadCollisionWithType(manager, e->collider, ENTITY_PLAYER, EASY_COLLISION_STAY);	

							if(info.found) {
								assert(info.e->type == ENTITY_PLAYER);
								Entity *player = info.e;

								///////////////////////********** Logic for when player hits an asteroid ***************////////////////////
								
								//NOTE(ollie): Play explosion of ship sound
								playGameSound(&globalLongTermArena, easyAudio_findSound("crash_sound.wav"), 0, AUDIO_FOREGROUND);

								///////////////////////************ Generic stuff for all things that hurt the player *************////////////////////
								if(myEntity_canPlayerBeHurt(player)) {
									myEntity_decreasePlayerHealthPoint(player, gameStateVariables);
									myEntity_checkHealthPoints(player, gameState, gameStateVariables);
									myEntity_addInstructionCard(gameState, GAME_INSTRUCTION_CRAMP);
								}

								////////////////////////////////////////////////////////////////////

								//NOTE(ollie): Cramp is removed after it fades out
								turnTimerOn(&e->fadeTimer);
								myEntity_spinEntity(e);
							}	

							///////////////////////*********** Collision with Bullet **************////////////////////

							info = MyEntity_hadCollisionWithType(manager, e->collider, ENTITY_BULLET, EASY_COLLISION_ENTER);	

							if(info.found) {
								assert(info.e->type == ENTITY_BULLET);
								turnTimerOn(&e->fadeTimer);

								////////////////////////////////////////////////////////////////////
								//Play particle effects
								//Spawn smaller rocks

								//NOTE(ollie): Play explosion of rock sound
								playGameSound(&globalLongTermArena, easyAudio_findSound("crash_sound.wav"), 0, AUDIO_FOREGROUND);
							
							}	

							////////////////////////////////////////////////////////////////////					
						}
					} break;
					case ENTITY_SOUND_CHANGER: {
						if(e->collider->collisionCount > 0) {

							///////////////////////*********** Collision with Player **************////////////////////
							MyEntity_CollisionInfo info = MyEntity_hadCollisionWithType(manager, e->collider, ENTITY_PLAYER, EASY_COLLISION_ENTER);	

							if(info.found) {
								assert(info.e->type == ENTITY_PLAYER);
								Entity *player = info.e;

								//NOTE(ollie): Change background sound 
							}	
						}

					} break;
					case ENTITY_CRAMP: {
						if(!isOn(&e->fadeTimer) && e->collider->collisionCount > 0) {

							///////////////////////*********** Collision with Player **************////////////////////
							MyEntity_CollisionInfo info = MyEntity_hadCollisionWithType(manager, e->collider, ENTITY_PLAYER, EASY_COLLISION_STAY);	

							if(info.found) {
								assert(info.e->type == ENTITY_PLAYER);
								Entity *player = info.e;

								playGameSound(&globalLongTermArena, easyAudio_findSound("splatSound.wav"), 0, AUDIO_FOREGROUND);

								///////////////////////************ Generic stuff for all things that hurt the player *************////////////////////
								if(myEntity_canPlayerBeHurt(player)) {
									myEntity_decreasePlayerHealthPoint(player, gameStateVariables);
									myEntity_checkHealthPoints(player, gameState, gameStateVariables);
									myEntity_addInstructionCard(gameState, GAME_INSTRUCTION_CRAMP);
								}

								////////////////////////////////////////////////////////////////////

								//NOTE(ollie): Cramp is removed after it fades out
								turnTimerOn(&e->fadeTimer);
								myEntity_spinEntity(e);
							}	

							///////////////////////*********** Collision with Bullet **************////////////////////

							info = MyEntity_hadCollisionWithType(manager, e->collider, ENTITY_BULLET, EASY_COLLISION_ENTER);	

							if(info.found) {
								assert(info.e->type == ENTITY_BULLET);
								turnTimerOn(&e->fadeTimer);
							
							}	

							////////////////////////////////////////////////////////////////////					
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

						#define SAFE_REGION_TO_DESTROY 4
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

			
			
			setModelTransform(renderGroup, easyTransform_getTransform(&e->T));
			
			if(flags & (u32)MY_ENTITIES_RENDER) {
				if(e->sprite) {
					renderDrawSprite(renderGroup, e->sprite, e->colorTint);
				}	

				if(e->model || e->type == ENTITY_BULLET || e->type == ENTITY_SOUND_CHANGER) {
					renderSetShader(renderGroup, &phongProgram);
					if(e->type == ENTITY_BULLET || e->type == ENTITY_SOUND_CHANGER) {
						renderDrawCube(globalRenderGroup, &globalWhiteMaterial, e->colorTint);
					} else {
						renderModel(renderGroup, e->model, e->colorTint);	
					}
					
					renderSetShader(renderGroup, mainShader);
				}
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

static void resetPlayer(Entity *e, MyGameStateVariables *variables, Texture *tex) { //for restarting player without the memory leaks
	e->moveTimer = initTimer(variables->playerMoveSpeed, false);
	turnTimerOff(&e->moveTimer);


	float scale = 0.2f;
	easyTransform_initTransform_withScale(&e->T, v3(0, 0, LAYER3), v3(scale, scale*tex->aspectRatio_h_over_w, scale)); 
	e->T.Q = eulerAnglesToQuaternion(0, 0.5f*PI32, 0);

	e->active = true;
	e->moveDirection = ENTITY_MOVE_NULL;
	e->laneIndex = 2;
	e->dropletCount = 0;
	e->healthPoints = e->maxHealthPoints;
	e->dropletCountStore = 0;

	e->fadeTimer = initTimer(0.3f, false);
	turnTimerOff(&e->fadeTimer);


	e->hurtTimer = initTimer(2.0f, false);
	turnTimerOff(&e->hurtTimer);
	e->invincibleTimer = initTimer(2.0f, false);
	turnTimerOff(&e->invincibleTimer);

	e->restartToMiddleTimer = initTimer(0.3f, false);
	turnTimerOff(&e->restartToMiddleTimer);


}
static Entity *initPlayer(MyEntityManager *m, MyGameStateVariables *variables, Texture *empty,  Texture *halfEmpty) {
	Entity *e = (Entity *)getEmptyElement(&m->entities);

	e->moveTimer = initTimer(variables->playerMoveSpeed, false);
	turnTimerOff(&e->moveTimer);

	e->name = "Player";

	e->active = true;
	e->colorTint = COLOR_WHITE;
	e->type = ENTITY_PLAYER;
	
	e->maxHealthPoints = 3;
	resetPlayer(e, variables, empty);

	e->moveList.next = e->moveList.prev = &e->moveList;
	e->freeList = 0;

	e->sprite = empty;

	e->sprites[0] = empty;
	e->sprites[1] = halfEmpty;

	e->model = findModelAsset("SmallSpaceFighter.obj");



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

static Entity *initBullet(MyEntityManager *m, V3 pos) {
	Entity *e = (Entity *)getEmptyElement(&m->entities);

	e->name = "Bullet";
	pos.z = LAYER1;
	float scale = 0.2f;
	easyTransform_initTransform_withScale(&e->T, pos, v3(scale, scale, scale)); 

	assert(!e->T.parent);
	e->active = true;
	e->colorTint = COLOR_WHITE;
	e->type = ENTITY_BULLET;

	e->lifespanTimer = initTimer(3.0f, false);
	turnTimerOn(&e->lifespanTimer);	

	e->bulletColorTimer = initTimer(6.0f, false);
	turnTimerOn(&e->bulletColorTimer);	

	e->fadeTimer = initTimer(0.3f, false);
	turnTimerOff(&e->fadeTimer);

	////Physics 

	e->rb = EasyPhysics_AddRigidBody(&m->physicsWorld, 1 / 10.0f, 0);
	e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_CIRCLE, NULL_VECTOR3, true, v3(MY_ENTITY_DEFAULT_DIM, 0, 0));

	/////

	e->rb->dP.y = 5;
	//NOTE(ollie): Rotate around all directions
	e->rb->dA = v3(1, 1, 0);

	return e;
}
////////////////////////////////////////////////////////////////////

//NOTE(ollie): Asteroid
static Entity *initAsteroid(MyEntityManager *m, V3 pos, EasyModel *model) {
	Entity *e = (Entity *)getEmptyElement(&m->entities);

	e->name = "Asteroid";
	float scale = 0.2f;
	easyTransform_initTransform_withScale(&e->T, pos, v3(scale, scale, scale)); 

	assert(!e->T.parent);
	e->active = true;
	e->colorTint = COLOR_WHITE;
	e->type = ENTITY_ASTEROID;

	e->fadeTimer = initTimer(0.3f, false);
	turnTimerOff(&e->fadeTimer);

	e->model = model;

	////Physics 

	e->rb = EasyPhysics_AddRigidBody(&m->physicsWorld, 1 / 10.0f, 0);
	e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_CIRCLE, NULL_VECTOR3, true, v3(MY_ENTITY_DEFAULT_DIM, 0, 0));

	/////

	
	//NOTE(ollie): Rotate around all directions
	e->rb->dA = v3(1, 1, 0);

	return e;
}


////////////////////////////////////////////////////////////////////

//NOTE(ollie): Sound changer
static Entity *initSoundchanger(MyEntityManager *m, V3 pos, WavFile *soundToPlay) {
	Entity *e = (Entity *)getEmptyElement(&m->entities);

	e->name = "Sound changer";
	float scale = 0.2f;
	easyTransform_initTransform_withScale(&e->T, pos, v3(scale, scale, scale)); 

	assert(!e->T.parent);
	e->active = true;
	e->colorTint = COLOR_WHITE;
	e->type = ENTITY_SOUND_CHANGER;

	e->soundToPlay = soundToPlay;

	////Physics 

	e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, 0, EASY_COLLIDER_RECTANGLE, NULL_VECTOR3, true, v3(MAX_LANE_COUNT, 1, 1));
	/////

	e->rb->dP.x = 5;
	//NOTE(ollie): Rotate around all directions
	e->rb->dA = v3(1, 1, 0);

	return e;
}


////////////////////////////////////////////////////////////////////

//NOTE(ollie): Droplet

static Entity *initDroplet(MyEntityManager *m, Texture *sprite, V3 pos, Entity *parent) {
	Entity *e = (Entity *)getEmptyElement(&m->entities);

	e->name = "Droplet";
	float width = 0.6f;
	easyTransform_initTransform_withScale(&e->T, pos, v3(width, width*sprite->aspectRatio_h_over_w, 1)); 
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

	e->rb = EasyPhysics_AddRigidBody(&m->physicsWorld, 1 / 10.0f, 0);
	e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_CIRCLE, NULL_VECTOR3, true, v3(MY_ENTITY_DEFAULT_DIM, 0, 0));
	/////

	return e;
}

////////////////////////////////////////////////////////////////////

//NOTE(ollie): Teleporter

static Entity *initTeleporter(MyEntityManager *m, V3 pos, Entity *parent, MyGameState *gameState) {
	Entity *e = (Entity *)getEmptyElement(&m->entities);

	e->name = "Teleporter";
	float width = 1.0f;

	///////////////////////************* Setup the animation ************////////////////////
	//NOTE(ollie): Set the sentinel up
	e->animationListSentintel.Next = e->animationListSentintel.Prev = &e->animationListSentintel;
	////////////////////////////////////////////////////////////////////

	//NOTE(ollie): Create the idle animation
	char *formatStr = "teleporter-%d.png";
	char buffer[512];
	////////////////////////////////////////////////////////////////////

	//NOTE(ollie): Init the animation
	e->idleAnimation.Name = "Teleporter Idle";
	e->idleAnimation.state = ANIM_IDLE;
	e->idleAnimation.FrameCount = 0;

	//NOTE(ollie): Loop through image names & add them to the animation
	for(int loopIndex = 1; loopIndex <= 150; ++loopIndex) {

		//NOTE(ollie): Print the texture ID
		snprintf(buffer, arrayCount(buffer), formatStr, loopIndex);	

		//NOTE(ollie): Add it to the array
		assert(e->idleAnimation.FrameCount < arrayCount(e->idleAnimation.Frames));
		e->idleAnimation.Frames[e->idleAnimation.FrameCount++] = easyString_copyToArena(buffer, &globalLongTermArena);
		
	}
	////////////////////////////////////////////////////////////////////
	
	AddAnimationToList(&gameState->animationItemFreeListPtr, &globalLongTermArena, &e->animationListSentintel, &e->idleAnimation);

	////////////////////////////////////////////////////////////////////

	easyTransform_initTransform_withScale(&e->T, pos, v3(width, width, 1)); 
	if(parent) e->T.parent = &parent->T;
	
	e->active = true;
	#if DEBUG_ENTITY_COLOR
		e->colorTint = COLOR_PINK;
	#else 
		e->colorTint = COLOR_WHITE;
	#endif
	e->type = ENTITY_TELEPORTER;

	e->fadeTimer = initTimer(0.3f, false);
	turnTimerOff(&e->fadeTimer);

	////Physics 

	e->rb = EasyPhysics_AddRigidBody(&m->physicsWorld, 1 / 10.0f, 0);
	e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_CIRCLE, NULL_VECTOR3, true, v3(MY_ENTITY_DEFAULT_DIM, 0, 0));
	/////

	return e;

	

}

////////////////////////////////////////////////////////////////////

//NOTE(ollie): Underpants

static Entity *initUnderpants(MyEntityManager *m, Texture *sprite, V3 pos, Entity *parent) {
	Entity *e = (Entity *)getEmptyElement(&m->entities);

	e->name = "Underpants";
	float width = 0.6f;
	easyTransform_initTransform_withScale(&e->T, pos, v3(width, width*sprite->aspectRatio_h_over_w, 1)); 
	if(parent) e->T.parent = &parent->T;
	
	e->active = true;
	#if DEBUG_ENTITY_COLOR
		e->colorTint = COLOR_PINK;
	#else 
		e->colorTint = COLOR_WHITE;
	#endif
	e->type = ENTITY_UNDERPANTS;

	e->sprite = sprite;

	e->fadeTimer = initTimer(0.3f, false);
	turnTimerOff(&e->fadeTimer);

	////Physics 

	e->rb = EasyPhysics_AddRigidBody(&m->physicsWorld, 1 / 10.0f, 0);
	e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_CIRCLE, NULL_VECTOR3, true, v3(MY_ENTITY_DEFAULT_DIM, 0, 0));
	/////

	return e;
}

////////////////////////////////////////////////////////////////////
//NOTE(ollie): Scenery item


//NOTE(ollie): Scenery 1x1 means it takes up 1x1 grid cell
static Entity *initScenery1x1(MyEntityManager *m, char *name, EasyModel *model, V3 pos, Entity *parent) {
	Entity *e = (Entity *)getEmptyElement(&m->entities);
	assert(!e->updatedFrame);
	assert(!e->updatedPhysics);

	e->name = name;
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
	e->type = ENTITY_SCENERY;

	e->model = model;

	e->fadeTimer = initTimer(0.3f, false);
	turnTimerOff(&e->fadeTimer);

	////Physics 
	e->rb = EasyPhysics_AddRigidBody(&m->physicsWorld, 1 / 10.0f, 0);
	e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_CIRCLE, NULL_VECTOR3, true, v3(getDimRect3f(model->bounds).x, 0, 0)); //only the x bounds is actually used for circle types

	/////

	return e;
}


////////////////////////////////////////////////////////////////////


//NOTE(ollie): Cramp
static Entity *initCramp(MyEntityManager *m, Texture *sprite, V3 pos, Entity *parent) {
	Entity *e = (Entity *)getEmptyElement(&m->entities);

	e->name = "Cramp";
	easyTransform_initTransform_withScale(&e->T, pos, v3(1, sprite->aspectRatio_h_over_w, 1)); 
	if(parent) e->T.parent = &parent->T;

	e->active = true;
#if DEBUG_ENTITY_COLOR
	e->colorTint = COLOR_BLUE;
#else 
	e->colorTint = COLOR_WHITE;
#endif
	
	e->type = ENTITY_CRAMP;

	e->sprite = sprite;
	e->model = 0;

	e->fadeTimer = initTimer(0.3f, false);
	turnTimerOff(&e->fadeTimer);

	////Physics 
	e->rb = EasyPhysics_AddRigidBody(&m->physicsWorld, 1 / 10.0f, 0);
	e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_CIRCLE, NULL_VECTOR3, true, v3(MY_ENTITY_DEFAULT_DIM, 0, 0));

	/////

	return e;

}

////////////////////////////////////////////////////////////////////

//NOTE(ollie): Star
static Entity *initStar(MyEntityManager *m, V3 pos, Entity *parent) {
	Entity *e = (Entity *)getEmptyElement(&m->entities);
	Texture *sprite = findTextureAsset("coinGold.png");

	e->name = "Star";
	easyTransform_initTransform_withScale(&e->T, pos, v3(1, sprite->aspectRatio_h_over_w, 1)); 
	if(parent) e->T.parent = &parent->T;

	e->active = true;
#if DEBUG_ENTITY_COLOR
	e->colorTint = COLOR_BLUE;
#else 
	e->colorTint = COLOR_WHITE;
#endif
	
	e->type = ENTITY_STAR;

	e->sprite = sprite;
	e->model = 0;

	e->fadeTimer = initTimer(0.3f, false);
	turnTimerOff(&e->fadeTimer);

	////Physics 
	e->rb = EasyPhysics_AddRigidBody(&m->physicsWorld, 1 / 10.0f, 0);
	e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_CIRCLE, NULL_VECTOR3, true, v3(MY_ENTITY_DEFAULT_DIM, 0, 0));

	/////

	//spin the star around
	e->rb->dA = v3(0, 0, 5);

	return e;

}



//NOTE(ollie): Bucket
static Entity *initBucket(MyEntityManager *m, Texture *sprite, V3 pos, Entity *parent) {
	Entity *e = (Entity *)getEmptyElement(&m->entities);

	e->name = "Bucket";
	easyTransform_initTransform_withScale(&e->T, pos, v3(1, sprite->aspectRatio_h_over_w, 1)); 
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
	e->rb = EasyPhysics_AddRigidBody(&m->physicsWorld, 1 / 10.0f, 0);
	e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_CIRCLE, NULL_VECTOR3, true, v3(MY_ENTITY_DEFAULT_DIM, 0, 0));

	/////

	return e;

}

////////////////////////////////////////////////////////////////////


//NOTE(ollie): Choc bar
static Entity *initChocBar(MyEntityManager *m, Texture *sprite, V3 pos, Entity *parent) {
	Entity *e = (Entity *)getEmptyElement(&m->entities);

	e->name = "Chocolate Bar";
	easyTransform_initTransform_withScale(&e->T, pos, v3(1, sprite->aspectRatio_h_over_w, 1)); 
	if(parent) e->T.parent = &parent->T;

	e->active = true;
	e->colorTint = COLOR_WHITE;
	e->type = ENTITY_CHOC_BAR;

	e->sprite = sprite;
	e->fadeTimer = initTimer(0.3f, false);
	turnTimerOff(&e->fadeTimer);

	////Physics 
	e->rb = EasyPhysics_AddRigidBody(&m->physicsWorld, 1 / 10.0f, 0);
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
//NOTE(ollie): Tile
static Entity *initTile(MyEntityManager *m, V3 pos, Entity *parent) {
	Entity *e = (Entity *)getEmptyElement(&m->entities);
		assert(!e->updatedFrame);
		assert(!e->updatedPhysics);

		e->name = "Tile";
		easyTransform_initTransform(&e->T, pos); 
		if(parent) e->T.parent = &parent->T;

		//NOTE(ollie): Set the transform 
		float scale = globalTileScale;
		e->T.scale = v3(scale, scale, scale);
		e->T.Q = eulerAnglesToQuaternion(0, -0.5f*PI32, 0);


		e->active = true;
		#if DEBUG_ENTITY_COLOR
				e->colorTint = COLOR_BLUE;
			#else 
				e->colorTint = COLOR_WHITE;
			#endif
		e->type = ENTITY_TILE;

		e->model = 0;

		e->fadeTimer = initTimer(0.3f, false);
		turnTimerOff(&e->fadeTimer);

		////Physics 
		e->rb = EasyPhysics_AddRigidBody(&m->physicsWorld, 1 / 10.0f, 0);
		e->collider = 0;//EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_CIRCLE, NULL_VECTOR3, true, v3(getDimRect3f(e->model->bounds).x, 0, 0)); //only the x bounds is actually used for circle types

		/////

		return e;
}

static Entity *initEntityByType(MyEntityManager *entityManager, V3 worldP, EntityType type, Entity *room, MyGameState *gameState) {
	Entity *result = 0;

	//NOTE(ollie): Get the clamped position for the entities that have to be on the board
	V3 clampedPosition = v3((int)worldP.x, (int)worldP.y, 0);

	//NOTE(ollie): Clamp the position to be inside the lanes
	clampedPosition.x = clamp(0, clampedPosition.x, MAX_LANE_COUNT);
	clampedPosition.y = clamp(0, clampedPosition.y, MAX_LANE_COUNT);

	switch(type) {
		case ENTITY_ASTEROID: {
			EasyModel *asteroid = findModelAsset_Safe("rock.obj");
			if(asteroid) {
				result = initAsteroid(entityManager, worldP, asteroid);	
			} else {
				easyConsole_addToStream(DEBUG_globalEasyConsole, "Couldn't find rock model");
			}
			
		} break;
		case ENTITY_SOUND_CHANGER: {

		} break;
		case ENTITY_BUCKET: {
			result = initBucket(entityManager, findTextureAsset("toilet1.png"), clampedPosition, room);
		} break;
		case ENTITY_DROPLET: {
			result = initDroplet(entityManager, findTextureAsset("blood_droplet.PNG"), clampedPosition, room);
		} break;
		case ENTITY_UNDERPANTS: {
			result = initUnderpants(entityManager, findTextureAsset("underwear.png"), clampedPosition, room);
		} break;
		case ENTITY_CRAMP: {
			result = initCramp(entityManager, findTextureAsset("cramp.PNG"), clampedPosition, room);
		} break;
		case ENTITY_TELEPORTER: {
			result = initTeleporter(entityManager, clampedPosition, room, gameState);
		} break;
		case ENTITY_STAR: {
			result = initStar(entityManager, clampedPosition, room);
		} break;
		case ENTITY_CHOC_BAR: {
			result = initChocBar(entityManager, findTextureAsset("choc_bar.png"), clampedPosition, room);
		} break;
		default: {
			easyConsole_addToStream(DEBUG_globalEasyConsole, "Tried to create entity, but type not handled.");

		}
	}			
	return result;
}


static int myLevels_getLevelWidth(char *level) {
	char *at = level;
	int result = 0;
	while(*at != '\0' && *at != '\n' && *at != '\r') {
		if(*at != ' ' && *at != '\t') {
			result++;	
		}
		
		at++;
	}
	return result;
}


static Entity *myLevels_generateLevel_(char *level, MyEntityManager *entityManager, V3 roomPos) {
	Entity *room = initRoom(entityManager, roomPos);

	char *at = level;

	float startX = -1*(int)(0.5f*myLevels_getLevelWidth(level));
	float startY = MY_ROOM_HEIGHT - 1;

	V2 posAt = v2(startX, startY);
	while(*at != '\0') {
		switch(*at) {
			case '*': { //empty space
				initTile(entityManager, v3(posAt.x, posAt.y, LAYER4), room);
				posAt.x++;
			} break;
			case '\n': { //empty space
				posAt.y--;
				posAt.x = startX;
			} break;
			case 'e': { //choc bar
				initTile(entityManager, v3(posAt.x, posAt.y, LAYER4), room);
				initChocBar(entityManager, findTextureAsset("choc_bar.png"), v3(posAt.x, posAt.y, LAYER0), room);
				posAt.x++;
			} break;
			case 'c': { //cramp
				initTile(entityManager, v3(posAt.x, posAt.y, LAYER4), room);
				initCramp(entityManager, findTextureAsset("cramp.PNG"), v3(posAt.x, posAt.y, LAYER0), room);
				posAt.x++;
			} break;
			case 'd': { //droplet
				initTile(entityManager, v3(posAt.x, posAt.y, LAYER4), room);
				initDroplet(entityManager, findTextureAsset("blood_droplet.PNG"), v3(posAt.x, posAt.y, LAYER0), room);
				posAt.x++;
			} break;
			case 'u': {
				initTile(entityManager, v3(posAt.x, posAt.y, LAYER4), room);
				initUnderpants(entityManager, findTextureAsset("underwear.png"), v3(posAt.x, posAt.y, LAYER0), room);
				posAt.x++;
			} break;
			case 's': { //droplet
				initTile(entityManager, v3(posAt.x, posAt.y, LAYER4), room);
				initScenery1x1(entityManager, "Crystal", findModelAsset("Crystal.obj"), v3(posAt.x, posAt.y, LAYER0), room);
				posAt.x++;
			} break;
			case 'q': { //droplet
				initTile(entityManager, v3(posAt.x, posAt.y, LAYER4), room);
				initScenery1x1(entityManager, "Oak Tree", findModelAsset("Oak_Tree.obj"), v3(posAt.x, posAt.y, LAYER0), room);
				posAt.x++;
			} break;
			case 'w': { //droplet
				initTile(entityManager, v3(posAt.x, posAt.y, LAYER4), room);
				initScenery1x1(entityManager, "Palm Tree", findModelAsset("Palm_Tree.obj"), v3(posAt.x, posAt.y, LAYER0), room);
				posAt.x++;
			} break;
			case 't': { //toilet
				initTile(entityManager, v3(posAt.x, posAt.y, LAYER4), room);
				initBucket(entityManager, findTextureAsset("toilet1.png"), v3(posAt.x, posAt.y, LAYER0), room);
				posAt.x++;
			} break;
			case 'i': {
				initTile(entityManager, v3(posAt.x, posAt.y, LAYER4), room);
				initStar(entityManager, v3(posAt.x, posAt.y, LAYER0), room);
			} break;
			default: {

			} 
		}

		at++;
	}

	return room;

}


///////////////////////************ Function declarations*************////////////////////
static inline Entity *myLevels_loadLevel(int level, MyEntityManager *entityManager, V3 startPos, MyGameState *gameState);

///////////////////////*************  Clean up at end of frame ************////////////////////

static void cleanUpEntities(MyEntityManager *manager, MyGameStateVariables *variables, MyGameState *gameState) {
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
				playGameSound(&globalLongTermArena, easyAudio_findSound("blaster1.wav"), 0, AUDIO_FOREGROUND);
				//NOTE(ollie): Init the bullet a little further so it isn't on the ship
				initBullet(manager, v3_plus(info.pos, v3(0, 1, 0)));
			} break;
			case ENTITY_ROOM: {
				int randomLevel = variables->lastLevelIndex = myLevels_getLevel(variables->lastLevelIndex); 
				variables->mostRecentRoom = variables->lastRoomCreated;
				variables->lastRoomCreated = myLevels_loadLevel(randomLevel, manager, info.pos, gameState);
			
			} break;
			default: {
				assert(false);//not handled
			}
		}


	}

	manager->toCreateCount = 0;
}



////////////////////////////////////////////////////////////////////