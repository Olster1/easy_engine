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
        
    //NOTE(ollie): Reference to player
    Entity *player;

    //NOTE(ollie): Ammo info
    s32 totalAmmoCount;
} MyGameStateVariables;

////////////////////////////////////////////////////////////////////


static inline MyEntityManager *initEntityManager() {
    MyEntityManager *result = pushStruct(&globalLongTermArena, MyEntityManager);
    initArray(&result->entities, Entity);
    
    result->toCreateCount = 0;
    
    EasyPhysics_beginWorld(&result->physicsWorld);


    //NOTE(ollie): Create the idle animation
    char *formatStr = "teleporter-%d.png";
    char buffer[512];
    ////////////////////////////////////////////////////////////////////

    //NOTE(ollie): Init the animation
    result->teleporterAnimation.Name = "Teleporter Idle";
    result->teleporterAnimation.state = ANIM_IDLE;
    result->teleporterAnimation.FrameCount = 0;
    
    //NOTE(ollie): Loop through image names & add them to the animation
    for(int loopIndex = 1; loopIndex <= 150; ++loopIndex) {
        
        //NOTE(ollie): Print the texture ID
        snprintf(buffer, arrayCount(buffer), formatStr, loopIndex); 
        
        //NOTE(ollie): Add it to the array
        assert(result->teleporterAnimation.FrameCount < arrayCount(result->teleporterAnimation.Frames));
        result->teleporterAnimation.Frames[result->teleporterAnimation.FrameCount++] = easyString_copyToArena(buffer, &globalLongTermArena);
        
    }
    ////////////////////////////////////////////////////////////////////
    
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

static s32 getLaneForX(float x) {
    s32 result = (x - (-2*GAME_LANE_WIDTH)) / GAME_LANE_WIDTH;
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

static inline bool myEntity_shouldEntityBeSaved(EntityType type) {
    bool result = (type != ENTITY_ROOM && type != ENTITY_TILE && type != ENTITY_PLAYER && type != ENTITY_BULLET && type != ENTITY_ENEMY_BULLET);
    return result;
}


static inline void myEntity_setPlayerPosToMiddle(Entity *player) {
    player->laneIndex = 2;
    player->T.pos.x = getLaneX(player->laneIndex);
    turnTimerOff(&player->moveTimer);
}

static inline void myEntity_setPlayerPosToNewPosition(Entity *player, V3 worldP) {
    //NOTE(ollie): Set the world position of the player
    easyTransform_setWorldPos(&player->T, worldP);
    player->laneIndex = getLaneForX(worldP.x);
    turnTimerOff(&player->moveTimer);
}

static inline void myEntity_restartPlayerInMiddle(Entity *player) {
    turnTimerOff(&player->moveTimer);
    easyEntity_emptyMoveList(player);
    if(player->laneIndex == 2) { //already in the middle
        myEntity_setPlayerPosToMiddle(player);
    } else {
        //NOTE(ollie): Fade out to the middle
        player->moveViaType = MOVE_VIA_HURT;
        turnTimerOn(&player->restartToMiddleTimer);	
    }
    
}

static inline void myEntity_teleportPlayer(Entity *player, V3 worldPos) {

    playGameSound(&globalLongTermArena, easyAudio_findSound("reload.wav"), 0, AUDIO_FOREGROUND);
    player->teleportPos = worldPos;

    turnTimerOff(&player->moveTimer);
    easyEntity_emptyMoveList(player);
    //NOTE(ollie): Fade out to the middle
    player->moveViaType = MOVE_VIA_TELEPORT;
    turnTimerOn(&player->restartToMiddleTimer); 
    
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


#define MyEntity_findEntityById_static(manager, id) MyEntity_findEntityById_(manager, id, EASY_TRANSFORM_STATIC_ID)
#define MyEntity_findEntityById_transient(manager, id) MyEntity_findEntityById_(manager, id, EASY_TRANSFORM_TRANSIENT_ID)
static inline Entity *MyEntity_findEntityById_(MyEntityManager *manager, u32 id, EasyTransform_IdType idType) {
    Entity *result = 0;
    for(int i = 0; i < manager->entities.count && !result; ++i) {
        Entity *e = (Entity *)getElement(&manager->entities, i);
        if(e && e->T.id == id && e->T.idType == idType) { //can be null
            result = e;
            break;
        }
    }

    return result;
}



////////////////////////////////////////////////////////////////////


///////////////////////************ Drawing helper functions *************////////////////////


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
                            
                        e->colorTint = COLOR_RED;
                        e->colorTint.w = smoothStep01010(1, info.canonicalVal, 0);

                        if(info.finished) {
                            turnTimerOff(&e->hurtTimer);
                            e->colorTint = COLOR_WHITE;
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
                                if(e->moveViaType == MOVE_VIA_TELEPORT) {
                                    myEntity_setPlayerPosToNewPosition(e, e->teleportPos); 
                                } else if(e->moveViaType == MOVE_VIA_HURT) {
                                    myEntity_setPlayerPosToMiddle(e);   
                                } else {
                                    assert(false);
                                }
                                
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
                    } else {
                        e->rb->dP.x = 0;
                    }
                } break;
                case ENTITY_BOSS: {

                } break;
                case ENTITY_ENEMY: {

                } break;
                case ENTITY_ENEMY_BULLET: {

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
                        // e->T.pos.y += dt*variables->roomSpeed;
                    } else {
                        e->rb->dP.y = 0;
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
            e->T.markForDeletion = true;	
        }
    }

    //NOTE(ollie): Reset the ids, since we are deleting entities they no longer need the ids
    GLOBAL_transformID_static = 0;
    GLOBAL_transformID_transient = 0; 
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

                    	//NOTE(ollie): The player is the listener
                    	easySound_updateListenerPosition(globalSoundState, easyTransform_getWorldPos(&e->T));
                        //


                        if(!isOn(&e->playerReloadTimer)) {
                            //NOTE(ollie): automatic recharge ammo
                            e->ammoCount += dt*0.5f;

                            if(wasPressed(keyStates->gameButtons, BUTTON_SPACE) && ((s32)e->ammoCount) > 0) {
                                V3 initPos = easyTransform_getWorldPos(&e->T);
                                e->ammoCount -= 1.0f;
                                MyEntity_AddToCreateList(manager, ENTITY_BULLET, initPos);
                            }

                            if(e->ammoCount > gameStateVariables->totalAmmoCount) {
                                //NOTE(ollie): Make sure can't get more than max ammo
                                e->ammoCount = gameStateVariables->totalAmmoCount;
                            }

                            //NOTE(ollie): If run out, turn on reload timer that prevents player from shooting
                            if(e->ammoCount < 1.0f) {
                                e->ammoCount = 0;
                                turnTimerOn(&e->playerReloadTimer);
                                playGameSound(&globalLongTermArena, easyAudio_findSound("reload.wav"), 0, AUDIO_FOREGROUND);
                            }
                        } else {
                            //NOTE(ollie): Reloading
                            TimerReturnInfo info = updateTimer(&e->playerReloadTimer, dt);

                            e->ammoCount = lerp(0, info.canonicalVal, gameStateVariables->totalAmmoCount);
                            if(info.finished) {
                                turnTimerOff(&e->playerReloadTimer);                                
                                e->ammoCount = gameStateVariables->totalAmmoCount;
                            }
                        }
                        
                       
                        
                    } break;
                    case ENTITY_BULLET: {
                        
                        TimerReturnInfo lifeSpanInfo = updateTimer(&e->lifespanTimer, dt);
                        
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
                        
                        
                        if(lifeSpanInfo.finished) {
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

                        

                        if(!isOn(&e->fadeTimer) && e->collider->collisionCount > 0) {

                            ///////////////////////************ First check for the exit collision to clear teleporter*************////////////////////
                            MyEntity_CollisionInfo exitInfo = MyEntity_hadCollisionWithType(manager, e->collider, ENTITY_PLAYER, EASY_COLLISION_EXIT); 
                            if(exitInfo.found && exitInfo.e->lastTeleporter == e) {
                                //NOTE(ollie): Exited off the teleporter so safe to get rid of it
                                exitInfo.e->lastTeleporter = 0;
                            }


                            MyEntity_CollisionInfo info = MyEntity_hadCollisionWithType(manager, e->collider, ENTITY_PLAYER, EASY_COLLISION_ENTER); 
                            
                            //NOTE(ollie): There was a collision and it wasn't this teleporter
                            if(info.found && info.e->lastTeleporter != e) {
                                assert(info.e->type == ENTITY_PLAYER);
                                
                                //NOTE(ollie): Get the teleporter partner
                                Entity *partner = e->teleporterPartner;

                                //NOTE(ollie): This is so the player has to move off the teleporter before it will trigger again
                                info.e->lastTeleporter = partner;

                                V3 partnerWorldPos = easyTransform_getWorldPos(&partner->T);

                                myEntity_teleportPlayer(info.e, partnerWorldPos);
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
                    case ENTITY_ENEMY: {
                        bool createBullet = false;

                        ///////////////////////********** Check if the boss got hit by bullet ***************////////////////////
                        MyEntity_CollisionInfo collisionInfo = MyEntity_hadCollisionWithType(manager, e->collider, ENTITY_BULLET, EASY_COLLISION_ENTER);    
                        
                        if(collisionInfo.found && !isOn(&e->hurtColorTimer)) {
                            assert(collisionInfo.e->type == ENTITY_BULLET);

                            //NOTE(ollie): Check if have life yet
                            e->enemyHP--;

                            if(e->enemyHP == 0) {
                                turnTimerOn(&e->fadeTimer); 
                                //NOTE(ollie): Do stuff to notify we beat the boss

                            } else {
                                turnTimerOn(&e->hurtColorTimer);
                            }
                            
                            ////////////////////////////////////////////////////////////////////
                            //Play particle effects
                            //Spawn smaller rocks
                            
                            //NOTE(ollie): Play hurt sound from boss
                            playGameSound(&globalLongTermArena, easyAudio_findSound("bossHurt.wav"), 0, AUDIO_FOREGROUND);
                        }

                        //NOTE(ollie): Hurt color
                        if(isOn(&e->hurtColorTimer)) {
                            TimerReturnInfo info = updateTimer(&e->hurtColorTimer, dt);

                            e->colorTint = smoothStep00V4(COLOR_WHITE, info.canonicalVal, COLOR_RED);
                            if(info.finished) {
                                turnTimerOff(&e->hurtColorTimer);
                                e->colorTint = COLOR_WHITE;
                            }
                        }


                        //NOTE(ollie): Update state timer
                        if(!isOn(&e->aiStateTimer)) {
                            turnTimerOn(&e->aiStateTimer);
                        }

                        if(isOn(&e->aiStateTimer)) {
                            TimerReturnInfo info = updateTimer(&e->aiStateTimer, dt);

                            if(info.finished) {
                                turnTimerOn(&e->aiStateTimer);
                                createBullet = true;
                            }
                        }  

                        if(createBullet) {
                            V3 initPos = easyTransform_getWorldPos(&e->T);
                            MyEntity_AddToCreateList(manager, ENTITY_ENEMY_BULLET, initPos);
                        }
                    } break;
                    case ENTITY_BOSS: {

                        if(isOn(&e->hurtColorTimer)) {
                            TimerReturnInfo info = updateTimer(&e->hurtColorTimer, dt);

                            e->colorTint = smoothStep00V4(COLOR_WHITE, info.canonicalVal, COLOR_RED);
                            if(info.finished) {
                                turnTimerOff(&e->hurtColorTimer);
                                e->colorTint = COLOR_WHITE;
                            }
                        }

                        bool createBullet = false;
                        s32 halfRoomWidth = floor(0.5f*MAX_LANE_COUNT);

                        switch(e->aiStateAt) {
                            
                            case MY_AI_STATE_TAKE_HITS: {

                                ///////////////////////********** Check if the boss got hit by bullet ***************////////////////////
                                MyEntity_CollisionInfo collisionInfo = MyEntity_hadCollisionWithType(manager, e->collider, ENTITY_BULLET, EASY_COLLISION_ENTER);    
                                
                                if(collisionInfo.found && !isOn(&e->hurtColorTimer)) {
                                    assert(collisionInfo.e->type == ENTITY_BULLET);

                                    //NOTE(ollie): Check if have life yet
                                    e->enemyHP--;

                                    if(e->enemyHP == 0) {
                                        turnTimerOn(&e->fadeTimer); 
                                        //NOTE(ollie): Do stuff to notify we beat the boss

                                        gameState->worldState->defeatedBoss = true;
                                        gameState->worldState->inBoss = false;
                                    } else {
                                        turnTimerOn(&e->hurtColorTimer);
                                    }
                                    
                                    ////////////////////////////////////////////////////////////////////
                                    //Play particle effects
                                    //Spawn smaller rocks
                                    
                                    //NOTE(ollie): Play hurt sound from boss
                                    playGameSound(&globalLongTermArena, easyAudio_findSound("bossHurt.wav"), 0, AUDIO_FOREGROUND);
                                    
                                } 

                                //NOTE(ollie): Update state timer
                                if(!isOn(&e->aiStateTimer)) {
                                    turnTimerOn(&e->aiStateTimer);
                                }

                                if(isOn(&e->aiStateTimer)) {
                                    TimerReturnInfo info = updateTimer(&e->aiStateTimer, dt);

                                    //NOTE(ollie): Try go to lane zero
                                    e->T.pos.x = lerp(e->startPosForBoss, info.canonicalVal, -halfRoomWidth);

                                    if(info.finished) {
                                        turnTimerOn(&e->aiStateTimer);
                                        //NOTE(ollie): Go to being angry
                                        e->aiStateAt = MY_AI_STATE_ANGRY;
                                        //NOTE(ollie): minus 0.5 so it's 2.5 instead of 2
                                        e->lastInterval = -halfRoomWidth - 0.5f;
                                        e->aiStateTimer.period = 2.0f;
                                    }
                                }  

                            } break;
                            case MY_AI_STATE_ANGRY: {
                                switch(e->bossType) {
                                    case ENTITY_BOSS_FIRE: {
                                        //NOTE(ollie): Update state timer
                                        if(!isOn(&e->aiStateTimer)) {
                                            turnTimerOn(&e->aiStateTimer);
                                        }

                                        if(isOn(&e->aiStateTimer)) {
                                            TimerReturnInfo info = updateTimer(&e->aiStateTimer, dt);

                                            
                                            e->T.pos.x = lerp(-halfRoomWidth, info.canonicalVal, halfRoomWidth);

                                            if(e->T.pos.x >= e->lastInterval && e->T.pos.x < (e->lastInterval + 1)) {
                                                e->lastInterval++;
                                                createBullet = true;
                                            }

                                            if(info.finished) {
                                                turnTimerOn(&e->aiStateTimer);
                                                //NOTE(ollie): Go to being angry
                                                e->aiStateAt = MY_AI_STATE_TAKE_HITS;
                                                e->startPosForBoss = e->T.pos.x;
                                                e->aiStateTimer.period = 2.0f;
                                            }
                                        }  


                                    } break;
                                    case ENTITY_BOSS_DUPLICATE: {

                                    } break;
                                    case ENTITY_BOSS_NULL: {
                                        assert(false);
                                    } break;
                                }
                            } break;
                            case MY_AI_STATE_NULL: {
                                assert(false);
                                //NOTE(ollie): Recovery in release mode
                                e->aiStateAt = MY_AI_STATE_ANGRY;
                            } break;
                        }


                        
                        if(createBullet) {
                            V3 initPos = easyTransform_getWorldPos(&e->T);
                            MyEntity_AddToCreateList(manager, ENTITY_ENEMY_BULLET, initPos);
                        }
                    } break;
                    case ENTITY_ENEMY_BULLET: {
                        ///////////////////////*********** Update lifespan timers **************////////////////////
                       
                        TimerReturnInfo lifespanInfo = updateTimer(&e->lifespanTimer, dt);
                        
                        ///////////////////////************ Update the color of the bullet *************////////////////////
                        if(isOn(&e->bulletColorTimer)) {
                            TimerReturnInfo info = updateTimer(&e->bulletColorTimer, dt);
                            
                            // float hueVal = 10*info.canonicalVal*360.0f;
                            
                            // hueVal -= ((int)(hueVal / 360.0f))*360;
                            
                            // e->colorTint = easyColor_hsvToRgb(hueVal, 1, 1);
                            
                            if(info.finished) {
                                turnTimerOn(&e->bulletColorTimer);
                            }
                        }
                        ////////////////////////////////////////////////////////////////////
                        
                        
                        if(lifespanInfo.finished) {
                            turnTimerOn(&e->fadeTimer);
                        }


                        if(!isOn(&e->fadeTimer) && e->collider->collisionCount > 0) {
                            
                            ///////////////////////*********** Collision with Player **************////////////////////
                            MyEntity_CollisionInfo info = MyEntity_hadCollisionWithType(manager, e->collider, ENTITY_PLAYER, EASY_COLLISION_STAY);  
                            
                            if(info.found) {
                                assert(info.e->type == ENTITY_PLAYER);
                                Entity *player = info.e;
                                
                                playGameSound(&globalLongTermArena, easyAudio_findSound("splatSound.wav"), 0, AUDIO_FOREGROUND);
                                
                                ///////////////////////************ Generic stuff for all things that hurt the player *************////////////////////
                                if(myEntity_canPlayerBeHurt(player)) {
                                    // myEntity_decreasePlayerHealthPoint(player, gameStateVariables);
                                    // myEntity_checkHealthPoints(player, gameState, gameStateVariables);
                                }
                                
                                ////////////////////////////////////////////////////////////////////
                                
                                //NOTE(ollie): Cramp is removed after it fades out
                                turnTimerOn(&e->fadeTimer);
                            }   
                        }

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
            
            if(e->playingSound) {
                //NOTE(ollie): Set the teleporter sound to the correct location
                e->playingSound->location = easyTransform_getWorldPos(&e->T);
            }
            
            setModelTransform(renderGroup, easyTransform_getTransform(&e->T));
            
            if(flags & (u32)MY_ENTITIES_RENDER) {
                if(e->sprite) {
                    renderDrawSprite(renderGroup, e->sprite, e->colorTint);
                }	
                
                if(e->model || e->type == ENTITY_BULLET || e->type == ENTITY_SOUND_CHANGER || e->type == ENTITY_ENEMY_BULLET) {
                    renderSetShader(renderGroup, &phongProgram);
                    if(e->type == ENTITY_BULLET || e->type == ENTITY_SOUND_CHANGER || e->type == ENTITY_ENEMY_BULLET) {
                        renderDrawCube(globalRenderGroup, &globalWhiteMaterial, e->colorTint);
                    } else {
                        renderModel(renderGroup, e->model, e->colorTint);	
                    }
                    
                    renderSetShader(renderGroup, mainShader);
                }
            }	
            
            ///////////////////////************ Debug Collider viewing *************////////////////////
            if(DEBUG_global_ViewColliders && e->collider) 
            {
                
                EasyTransform tempT = e->T;
                
                //NOTE(ollie): Get temp shader to revert state afterwards
                RenderProgram *tempShader = renderGroup->currentShader;
                
                switch(e->collider->type) {
                    case EASY_COLLIDER_SPHERE:
                    case EASY_COLLIDER_BOX:
                    case EASY_COLLIDER_CIRCLE: {
                        
                        renderSetShader(renderGroup, &phongProgram);
                        
                        float dim = 2*e->collider->radius;
                        tempT.scale = v3(dim, dim, dim);
                        setModelTransform(renderGroup, easyTransform_getTransform(&tempT));
                            
                        renderDrawCube(globalRenderGroup, &globalWhiteMaterial, COLOR_RED);
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
    easyTransform_initTransform_withScale(&e->T, v3(0, 0, LAYER3), v3(scale, scale*tex->aspectRatio_h_over_w, scale), EASY_TRANSFORM_TRANSIENT_ID); 
    e->T.Q = eulerAnglesToQuaternion(0, 0.5f*PI32, 0);
    
    e->lastTeleporter = 0;
    e->active = true;
    e->moveDirection = ENTITY_MOVE_NULL;
    e->laneIndex = 2;
    e->dropletCount = 0;
    e->healthPoints = e->maxHealthPoints;
    e->dropletCountStore = 0;
    e->moveViaType = MOVE_VIA_NULL;
    
    e->fadeTimer = initTimer(0.3f, false);
    turnTimerOff(&e->fadeTimer);
    
    
    e->hurtTimer = initTimer(2.0f, false);
    turnTimerOff(&e->hurtTimer);
    e->invincibleTimer = initTimer(2.0f, false);
    turnTimerOff(&e->invincibleTimer);
    
    e->restartToMiddleTimer = initTimer(0.3f, false);
    turnTimerOff(&e->restartToMiddleTimer);

    e->playerReloadTimer = initTimer(10.0f, false);
    turnTimerOff(&e->playerReloadTimer);
        
    assert(variables->totalAmmoCount > 0);
    e->ammoCount = variables->totalAmmoCount; 
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
    
    // e->model = findModelAsset("SmallSpaceFighter.obj");
    
    e->model = findModelAsset("UltravioletIntruder.obj");
    
    
    e->fadeTimer = initTimer(0.3f, false);
    turnTimerOff(&e->fadeTimer);
    
    ////Physics 
    
    e->rb = EasyPhysics_AddRigidBody(&m->physicsWorld, 1 / 10.0f, 0);
    e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_SPHERE, NULL_VECTOR3, false, v3(MY_ENTITY_DEFAULT_DIM, 0, 0));
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
    easyTransform_initTransform_withScale(&e->T, pos, v3(scale, scale, scale), EASY_TRANSFORM_TRANSIENT_ID); 
    
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
    e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_SPHERE, NULL_VECTOR3, true, v3(MY_ENTITY_DEFAULT_DIM, 0, 0));
    
    /////
    
    e->rb->dP.y = 5;
    //NOTE(ollie): Rotate around all directions
    e->rb->dA = v3(1, 1, 0);
    
    return e;
}
////////////////////////////////////////////////////////////////////

//NOTE(ollie): Asteroid
static Entity *initAsteroid(MyEntityManager *m, V3 pos, EasyModel *model, Entity* parent) {
    Entity *e = (Entity *)getEmptyElement(&m->entities);
    
    e->name = "Asteroid";
    float scale = 0.2f;
    easyTransform_initTransform_withScale(&e->T, pos, v3(scale, scale, scale), EASY_TRANSFORM_STATIC_ID); 

    e->T.parent = &parent->T;
    
    e->active = true;
    e->colorTint = COLOR_WHITE;
    e->type = ENTITY_ASTEROID;
    
    e->fadeTimer = initTimer(0.3f, false);
    turnTimerOff(&e->fadeTimer);
    
    e->model = model;
    
    ////Physics 
    
    e->rb = EasyPhysics_AddRigidBody(&m->physicsWorld, 1 / 10.0f, 0);
    e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_SPHERE, NULL_VECTOR3, true, v3(MY_ENTITY_DEFAULT_DIM, 0, 0));
    
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
    easyTransform_initTransform_withScale(&e->T, pos, v3(scale, scale, scale), EASY_TRANSFORM_STATIC_ID); 
    
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

static Entity *initDroplet(MyEntityManager *m, EasyModel *model, V3 pos, Entity *parent) {
    Entity *e = (Entity *)getEmptyElement(&m->entities);
    
    e->name = "Droplet";
    float width = 0.6f;
    easyTransform_initTransform_withScale(&e->T, pos, v3(width, width, width), EASY_TRANSFORM_STATIC_ID); 
    e->T.Q = eulerAnglesToQuaternion(0, -0.5f*PI32, 0);

    if(parent) e->T.parent = &parent->T;
    
    e->active = true;
#if DEBUG_ENTITY_COLOR
    e->colorTint = COLOR_PINK;
#else 
    e->colorTint = COLOR_WHITE;
#endif
    e->type = ENTITY_DROPLET;
    
    e->model = model;
    
    e->fadeTimer = initTimer(0.3f, false);
    turnTimerOff(&e->fadeTimer);
    
    ////Physics 
    
    e->rb = EasyPhysics_AddRigidBody(&m->physicsWorld, 1 / 10.0f, 0);
    e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_SPHERE, NULL_VECTOR3, true, v3(MY_ENTITY_DEFAULT_DIM, 0, 0));
    /////

    e->rb->dA = v3(0, 1, 0);
    
    return e;
}

////////////////////////////////////////////////////////////////////

//NOTE(ollie): Boss

static Entity *initBoss(MyEntityManager *m, EntityBossType bossType, V3 pos, Entity *parent) {
    Entity *e = (Entity *)getEmptyElement(&m->entities);
    
    e->name = "Boss";
    float width = 1.0f;
    easyTransform_initTransform_withScale(&e->T, pos, v3(width, width, width), EASY_TRANSFORM_STATIC_ID); 
    if(parent) e->T.parent = &parent->T;


    e->T.Q = eulerAnglesToQuaternion(-0.5f*PI32, 0, 0);

    e->active = true;
#if DEBUG_ENTITY_COLOR
    e->colorTint = COLOR_PINK;
#else 
    e->colorTint = COLOR_WHITE;
#endif
    e->type = ENTITY_BOSS;

    e->bossType = bossType;

    e->enemyHP = 5;

    switch(bossType) {
        case ENTITY_BOSS_DUPLICATE: {
            e->model = findModelAsset("MicroRecon.obj");
        } break;
        case ENTITY_BOSS_FIRE: {
            e->model = findModelAsset("Arachnoid.obj");
        } break;
        default: {
            assert(false);
        }
    }

    e->aiStateTimer = initTimer(2.0f, false);
    turnTimerOff(&e->aiStateTimer);

    e->hurtColorTimer = initTimer(1.0f, false);
    turnTimerOff(&e->hurtColorTimer);

    e->startPosForBoss = e->T.pos.x;
    
    e->aiStateAt = MY_AI_STATE_TAKE_HITS;    
    
    e->fadeTimer = initTimer(0.3f, false);
    turnTimerOff(&e->fadeTimer);
    
    e->lastInterval = -floor(0.5f*MAX_LANE_COUNT) - 0.5f;
    ////Physics 
    
    e->rb = EasyPhysics_AddRigidBody(&m->physicsWorld, 1 / 10.0f, 0);
    e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_SPHERE, NULL_VECTOR3, true, v3(width, width, width));
    /////
    
    return e;
}

////////////////////////////////////////////////////////////////////


//NOTE(ollie): Enemy

static Entity *initEnemy(MyEntityManager *m, V3 pos, Entity *parent) {
    Entity *e = (Entity *)getEmptyElement(&m->entities);
    
    e->name = "Enemy";
    float width = 0.3f;
    easyTransform_initTransform_withScale(&e->T, pos, v3(width, width, width), EASY_TRANSFORM_STATIC_ID); 
    if(parent) e->T.parent = &parent->T;


    e->T.Q = eulerAnglesToQuaternion(0, -0.5f*PI32, 0);

    e->active = true;
#if DEBUG_ENTITY_COLOR
    e->colorTint = COLOR_PINK;
#else 
    e->colorTint = COLOR_WHITE;
#endif
    e->type = ENTITY_ENEMY;

    e->enemyHP = 5;

    e->model = findModelAsset("Arachnoid.obj");

    //NOTE(ollie): Just used for shooting at the moment
    e->aiStateTimer = initTimer(0.5f, false);
    turnTimerOff(&e->aiStateTimer);

    e->hurtColorTimer = initTimer(1.0f, false);
    turnTimerOff(&e->hurtColorTimer);

    e->fadeTimer = initTimer(0.3f, false);
    turnTimerOff(&e->fadeTimer);
    
    ////Physics 
    
    e->rb = EasyPhysics_AddRigidBody(&m->physicsWorld, 1 / 10.0f, 0);
    e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_SPHERE, NULL_VECTOR3, true, v3(width, width, width));
    /////
    
    return e;
}

////////////////////////////////////////////////////////////////////

//NOTE(ollie): Enemy Bullet

static Entity *initEnemyBullet(MyEntityManager *m, V3 pos) {
    Entity *e = (Entity *)getEmptyElement(&m->entities);
    
    e->name = "Enemy Bullet";
    float width = 0.3f;
    easyTransform_initTransform_withScale(&e->T, pos, v3(width, width, width), EASY_TRANSFORM_TRANSIENT_ID); 
    
    e->active = true;
#if DEBUG_ENTITY_COLOR
    e->colorTint = COLOR_PINK;
#else 
    e->colorTint = v4(0.9f, 0.3f, 0, 1);
#endif
    e->type = ENTITY_ENEMY_BULLET;

    // e->model = findModelAsset("voxel_sphere.obj");    
    
    ////Physics 
    
    e->lifespanTimer = initTimer(3.0f, false);
    turnTimerOn(&e->lifespanTimer); 
    
    e->bulletColorTimer = initTimer(6.0f, false);
    turnTimerOn(&e->bulletColorTimer);  
    
    e->fadeTimer = initTimer(0.3f, false);
    turnTimerOff(&e->fadeTimer);
    
    ////Physics 
    
    e->rb = EasyPhysics_AddRigidBody(&m->physicsWorld, 1 / 10.0f, 0);
    e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_SPHERE, NULL_VECTOR3, true, v3(MY_ENTITY_DEFAULT_DIM, 0, 0));
    
    /////
    
    //NOTE(ollie): Rotate around all directions
    e->rb->dA = v3(1, 1, 0);

    //NOTE(ollie): Start bullet moving
    e->rb->dP.y = -5;
    
    return e;
}

////////////////////////////////////////////////////////////////////

//NOTE(ollie): Teleporter

static Entity *initTeleporter(MyEntityManager *m, V3 pos, Entity *parent, MyGameState *gameState) {
    Entity *e = (Entity *)getEmptyElement(&m->entities);
    
    e->name = "Teleporter";
    float width = 1.1f;
    
    ///////////////////////************* Setup the animation ************////////////////////
    //NOTE(ollie): Set the sentinel up
    e->animationListSentintel.Next = e->animationListSentintel.Prev = &e->animationListSentintel;
    ////////////////////////////////////////////////////////////////////
    //NOTE(ollie): Add the animation to the list
    AddAnimationToList(&gameState->animationItemFreeListPtr, &globalLongTermArena, &e->animationListSentintel, &m->teleporterAnimation);
    
    ////////////////////////////////////////////////////////////////////
    
    easyTransform_initTransform_withScale(&e->T, pos, v3(width, width, 1), EASY_TRANSFORM_STATIC_ID); 
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
    e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_SPHERE, NULL_VECTOR3, true, v3(MY_ENTITY_DEFAULT_DIM, 0, 0));
    /////
    
    //NOTE(ollie): Play the teleporter sound

    e->playingSound = playLocationGameSound(&globalLongTermArena, easyAudio_findSound("teleporter.wav"), 0, AUDIO_FOREGROUND, easyTransform_getWorldPos(&e->T));
    EasySound_LoopSound(e->playingSound);

    ////////////////////////////////////////////////////////////////////

    return e;
    
    
    
}

////////////////////////////////////////////////////////////////////

//NOTE(ollie): Underpants

static Entity *initUnderpants(MyEntityManager *m, Texture *sprite, V3 pos, Entity *parent) {
    Entity *e = (Entity *)getEmptyElement(&m->entities);
    
    e->name = "Underpants";
    float width = 0.6f;
    easyTransform_initTransform_withScale(&e->T, pos, v3(width, sprite->aspectRatio_h_over_w*width, width), EASY_TRANSFORM_STATIC_ID); 
    if(parent) e->T.parent = &parent->T;
    
    e->active = true;
#if DEBUG_ENTITY_COLOR
    e->colorTint = COLOR_PINK;
#else 
    e->colorTint = COLOR_WHITE;
#endif
    e->type = ENTITY_UNDERPANTS;
    
    e->sprite = sprite;
    // e->model = model;
    
    e->fadeTimer = initTimer(0.3f, false);
    turnTimerOff(&e->fadeTimer);
    
    ////Physics 
    
    e->rb = EasyPhysics_AddRigidBody(&m->physicsWorld, 1 / 10.0f, 0);
    e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_SPHERE, NULL_VECTOR3, true, v3(MY_ENTITY_DEFAULT_DIM, 0, 0));
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
    easyTransform_initTransform(&e->T, pos, EASY_TRANSFORM_STATIC_ID); 
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
    e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_SPHERE, NULL_VECTOR3, true, v3(getDimRect3f(model->bounds).x, 0, 0)); //only the x bounds is actually used for circle types
    
    /////
    
    return e;
}


////////////////////////////////////////////////////////////////////


//NOTE(ollie): Cramp
static Entity *initCramp(MyEntityManager *m, EasyModel *model, V3 pos, Entity *parent) {
    Entity *e = (Entity *)getEmptyElement(&m->entities);
    
    e->name = "Cramp";
    float scale = 0.8f;
    easyTransform_initTransform_withScale(&e->T, pos, v3(scale, scale, scale), EASY_TRANSFORM_STATIC_ID); 
    if(parent) e->T.parent = &parent->T;
    
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
    e->rb = EasyPhysics_AddRigidBody(&m->physicsWorld, 1 / 10.0f, 0);
    e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_SPHERE, NULL_VECTOR3, true, v3(MY_ENTITY_DEFAULT_DIM, 0, 0));
    
    /////

    e->rb->dA = v3(0, 0, 5);
    
    return e;
    
}

////////////////////////////////////////////////////////////////////

//NOTE(ollie): Star
static Entity *initStar(MyEntityManager *m, V3 pos, Entity *parent) {
    Entity *e = (Entity *)getEmptyElement(&m->entities);
    Texture *sprite = findTextureAsset("coinGold.png");
    
    e->name = "Star";
    easyTransform_initTransform_withScale(&e->T, pos, v3(1, sprite->aspectRatio_h_over_w, 1), EASY_TRANSFORM_STATIC_ID); 
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
    e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_SPHERE, NULL_VECTOR3, true, v3(MY_ENTITY_DEFAULT_DIM, 0, 0));
    
    /////
    
    //spin the star around
    e->rb->dA = v3(0, 0, 5);
    
    return e;
    
}



//NOTE(ollie): Bucket
static Entity *initBucket(MyEntityManager *m, Texture *sprite, V3 pos, Entity *parent) {
    Entity *e = (Entity *)getEmptyElement(&m->entities);
    
    e->name = "Bucket";
    easyTransform_initTransform_withScale(&e->T, pos, v3(1, sprite->aspectRatio_h_over_w, 1), EASY_TRANSFORM_STATIC_ID); 
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
    e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_SPHERE, NULL_VECTOR3, true, v3(MY_ENTITY_DEFAULT_DIM, 0, 0));
    
    /////
    
    return e;
    
}

////////////////////////////////////////////////////////////////////


//NOTE(ollie): Choc bar
static Entity *initChocBar(MyEntityManager *m, Texture *sprite, V3 pos, Entity *parent) {
    Entity *e = (Entity *)getEmptyElement(&m->entities);
    
    e->name = "Chocolate Bar";
    easyTransform_initTransform_withScale(&e->T, pos, v3(1, sprite->aspectRatio_h_over_w, 1), EASY_TRANSFORM_STATIC_ID); 
    if(parent) e->T.parent = &parent->T;
    
    e->active = true;
    e->colorTint = COLOR_WHITE;
    e->type = ENTITY_CHOC_BAR;
    
    e->sprite = sprite;
    e->fadeTimer = initTimer(0.3f, false);
    turnTimerOff(&e->fadeTimer);
    
    ////Physics 
    e->rb = EasyPhysics_AddRigidBody(&m->physicsWorld, 1 / 10.0f, 0);
    e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_SPHERE, NULL_VECTOR3, true, v3(MY_ENTITY_DEFAULT_DIM, 0, 0));
    
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
    easyTransform_initTransform(&e->T, pos, EASY_TRANSFORM_TRANSIENT_ID); 
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
    e->collider = EasyPhysics_AddCollider(&m->physicsWorld, &e->T, e->rb, EASY_COLLIDER_SPHERE, NULL_VECTOR3, true, v3(MY_ENTITY_DEFAULT_DIM, 0, 0));
    
    /////
    
    return e;
    
}
//NOTE(ollie): Tile
static Entity *initTile(MyEntityManager *m, V3 pos, Entity *parent) {
    Entity *e = (Entity *)getEmptyElement(&m->entities);
    assert(!e->updatedFrame);
    assert(!e->updatedPhysics);
    
    e->name = "Tile";
    easyTransform_initTransform(&e->T, pos, EASY_TRANSFORM_TRANSIENT_ID); 
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
    e->collider = 0;
    
    /////
    
    return e;
}

static bool myEntity_entityIsClamped(EntityType type) {
    bool result = true;
    if(type == ENTITY_SCENERY || type == ENTITY_ASTEROID || type == ENTITY_BOSS || type == ENTITY_ENEMY_BULLET || type == ENTITY_ENEMY) {
        result = false;
    }

    return result;
}

static Entity *initEntityByType(MyEntityManager *entityManager, V3 worldP, EntityType type, Entity *room, MyGameState *gameState, bool clampPosition, EasyModel *m, EntityBossType bossType, bool isOnLoad) {
    Entity *result = 0;
    
    V3 clampedPosition = worldP;

    if(clampPosition && myEntity_entityIsClamped(type)) {
        //NOTE(ollie): Get the clamped position for the entities that have to be on the board
        clampedPosition = v3(floor(worldP.x + 0.5f), floor(worldP.y + 0.5f), 0);
        
        //NOTE(ollie): Clamp the position to be inside the lanes
        clampedPosition.y = clamp(0, clampedPosition.y, MY_ROOM_HEIGHT);
    }

    switch(type) {
        case ENTITY_ASTEROID: {
            EasyModel *asteroid = findModelAsset_Safe("rock.obj");
            if(asteroid) {
                result = initAsteroid(entityManager, worldP, asteroid, room);	
            } else {
                easyConsole_addToStream(DEBUG_globalEasyConsole, "Couldn't find rock model");
            }
            
        } break;
        case ENTITY_SOUND_CHANGER: {
            
        } break;
        case ENTITY_BUCKET: {
            result = initBucket(entityManager, findTextureAsset("fuel_tank.png"), clampedPosition, room);
        } break;
        case ENTITY_BOSS: {
            //NOTE(ollie): Boss doesn't move with the room
            result = initBoss(entityManager, bossType, worldP, 0);
        } break;
        case ENTITY_ENEMY: { 
            //NOTE(ollie): Enemies don't move with the room 
            result = initEnemy(entityManager, worldP, room);
        } break;
        case ENTITY_ENEMY_BULLET: {
            
        } break;
        case ENTITY_DROPLET: {
            result = initDroplet(entityManager, findModelAsset("Crystal.obj"), clampedPosition, room);
        } break;
        case ENTITY_UNDERPANTS: {
            result = initUnderpants(entityManager, findTextureAsset("spaceship.png"), clampedPosition, room);
        } break;
        case ENTITY_CRAMP: {
            result = initCramp(entityManager, findModelAsset("rock.obj"), clampedPosition, room);
        } break;
        case ENTITY_TELEPORTER: {
            //NOTE(ollie): We create two teleporter entities next to each other 
            if(isOnLoad) {
                result = initTeleporter(entityManager, clampedPosition, room, gameState);
            } else {
                Entity *t0 = initTeleporter(entityManager, clampedPosition, room, gameState);
                Entity *t1 = initTeleporter(entityManager, v3_plus(clampedPosition, v3(1, 0, 0)), room, gameState);

                //NOTE(ollie): Then assign partners so you can teleport through them
                t0->teleporterPartner = t1;
                t1->teleporterPartner = t0;

                //NOTE(ollie): Assign the entity to the result 
                result = t0;    
            }
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


///////////////////////************ Function declarations*************////////////////////

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
                playLocationGameSound(&globalLongTermArena, easyAudio_findSound("blaster1.wav"), 0, AUDIO_FOREGROUND, info.pos);
                //NOTE(ollie): Init the bullet a little further so it isn't on the ship
                initBullet(manager, v3_plus(info.pos, v3(0, 1, 0)));
            } break;
            case ENTITY_ROOM: {
                variables->mostRecentRoom = variables->lastRoomCreated;
                variables->lastRoomCreated = myWorlds_findNextRoom(gameState->worldState, manager, info.pos, gameState);
                
            } break;
            case ENTITY_ENEMY_BULLET: {
                playLocationGameSound(&globalLongTermArena, easyAudio_findSound("blaster1.wav"), 0, AUDIO_FOREGROUND, info.pos);
                //NOTE(ollie): Init the bullet a little further so it isn't on the ship
                initEnemyBullet(manager, v3_plus(info.pos, v3(0, -1, 0)));
            } break;
            default: {
                assert(false);//not handled
            }
        }
        
        
    }
    
    manager->toCreateCount = 0;
}



////////////////////////////////////////////////////////////////////