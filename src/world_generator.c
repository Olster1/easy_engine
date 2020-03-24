///////////////////////************ Declarations *************////////////////////

Entity *myLevels_loadLevel(int level, MyEntityManager *entityManager, V3 startPos, MyGameState *gameState);
void myLevels_getLevelInfo(int level, MyWorldTagInfo *tagInfo);

////////////////////////////////////////////////////////////////////


///////////////////////************ Functions *************////////////////////

//NOTE(ollie): You pass in the tags of the world
static void myWorlds_generateWorld(MyWorldState *worldState, MyWorldFlags flags) {

	//NOTE(ollie): Clear the world out
	zeroStruct(worldState, MyWorldState);

	//NOTE(ollie): Set up the world state
	if(flags & MY_WORLD_BOSS) { worldState->hasBoss = true; }
	if(flags & MY_WORLD_FIRE_BOSS) { worldState->bossType = ENTITY_BOSS_FIRE; }
	if(flags & MY_WORLD_PUZZLE) { worldState->hasPuzzle = true; }
	if(flags & MY_WORLD_ENEMIES) { worldState->hasEnemies = true; }
	if(flags & MY_WORLD_OBSTACLES) { worldState->hasObstacles = true; }
	if(flags & MY_WORLD_SPACE) { worldState->biomeType = MY_WORLD_BIOME_SPACE; }
	if(flags & MY_WORLD_EARTH) { worldState->biomeType = MY_WORLD_BIOME_EARTH; }

	worldState->levelCount = 0;
	
	//NOTE(ollie): Fill the tags array
	for(int i = 0; i < arrayCount(worldState->levels); ++i) {
		MyWorldTagInfo *tagInfo = worldState->levels + worldState->levelCount++; 
		tagInfo->usedCount = 0;
		tagInfo->id = i;
		myLevels_getLevelInfo(i, tagInfo);

	}

}

typedef u64 MyWorldFlagsU64; 

static MyWorldFlagsU64 myWorld_getFlagForBossType(EntityBossType type) {
	MyWorldFlagsU64 result = MY_WORLD_NULL;
	switch(type) {
		case ENTITY_BOSS_FIRE: {
			result |= MY_WORLD_FIRE_BOSS;
		} break;
		case ENTITY_BOSS_DUPLICATE: {
			assert(false);
		} break;
		default: {
			assert(false);
		}
	}
	return result;
}

static MyWorldFlagsU64 myWorld_getFlagForBiomeType(MyWorldBiomeType type) {
	MyWorldFlagsU64 result = MY_WORLD_NULL;
	switch(type) {
		case MY_WORLD_BIOME_SPACE: {
			result |= MY_WORLD_SPACE;
		} break;
		case MY_WORLD_BIOME_EARTH: {
			result |= MY_WORLD_EARTH;
		} break;
		default: {
			assert(false);
		}
	}
	return result;
}

//NOTE(ollie): Our sort function
s32 myWorld_sortItemsOnUsedCount(const void * a, const void* b) {
	MyWorldTagInfo *tagA = (MyWorldTagInfo *)a;
	MyWorldTagInfo *tagB = (MyWorldTagInfo *)b;

	s32 result = -1;
	//NOTE(ollie): Swap if the used count is higher	
	if(tagA->usedCount > tagB->usedCount) {
		result = 1;
	} 

	return result;

}

///////////////////////************ inline functions *************////////////////////
#define myWorld_roomsSincePuzzle(state) (state->totalRoomCount - state->lastRoomId_withPuzzle)
#define myWorld_roomsSinceEnemy(state) (state->totalRoomCount - state->lastRoomId_withEnemy)
#define myWorld_roomsSinceObstacle(state) (state->totalRoomCount - state->lastRoomId_withObstacle)

////////////////////////////////////////////////////////////////////

//NOTE(ollie): This is the important function that chooses the next room
static Entity *myWorlds_findNextRoom(MyWorldState *state, MyEntityManager *entityManager, V3 startPos, MyGameState *gameState) {

	//NOTE(ollie): Do sort first since if we do it afterwards, we lose the right pointer
	///////////////////////************ Sort tags based on used count*************////////////////////
	qsort(state->levels, state->levelCount, sizeof(MyWorldTagInfo), myWorld_sortItemsOnUsedCount);
	///////////////////////*************************////////////////////

	//NOTE(ollie): Start with first level
	MyWorldTagInfo *bestInfo = 0;

	#define ROOMS_TILL_BOSS 20
	//NOTE(ollie): Check for boss conditions
	if(state->hasBoss && !state->defeatedBoss && !state->inBoss) {
		if(state->totalRoomCount >= ROOMS_TILL_BOSS) {

			//NOTE(ollie): Look for the boss level
			for(int i = 0; i < state->levelCount && !bestInfo; ++i) {
				MyWorldTagInfo *tagInfo = state->levels + i;

				
				if(tagInfo->flags & myWorld_getFlagForBossType(state->bossType)) {
					bestInfo = tagInfo;

					state->inBoss = true;

					break;
				}
			}
		}
	}

	//NOTE(ollie): Keep looking for next level

	MyWorldFlagsU64 biomeFlag = myWorld_getFlagForBiomeType(state->biomeType);

	//NOTE(ollie): Look for puzzle level
	if(!bestInfo && state->hasPuzzle && myWorld_roomsSincePuzzle(state) > 10) {
		for(int i = 0; i < state->levelCount && !bestInfo; ++i) {
			MyWorldTagInfo *tagInfo = state->levels + i; 

			if((tagInfo->flags & (MyWorldFlagsU64)MY_WORLD_PUZZLE) && (tagInfo->flags & biomeFlag)) {
				//NOTE(ollie): Update that we create a room with an puzzle
				state->lastRoomId_withPuzzle = state->totalRoomCount;
				bestInfo = tagInfo;
				break;
			}
		}
	}

	//NOTE(ollie): Look for enemy level
	if(!bestInfo && state->hasEnemies &&  myWorld_roomsSinceEnemy(state) > 3) {
		for(int i = 0; i < state->levelCount && !bestInfo; ++i) {
			MyWorldTagInfo *tagInfo = state->levels + i; 

			if((tagInfo->flags & (MyWorldFlagsU64)MY_WORLD_ENEMIES) && (tagInfo->flags & biomeFlag)) {
				//NOTE(ollie): Update that we create a room with an enemy
				state->lastRoomId_withEnemy = state->totalRoomCount;
				bestInfo = tagInfo;
				break;
			}
		}
	}

	//NOTE(ollie): Look for obstacle level
	if(!bestInfo && state->hasObstacles && myWorld_roomsSinceObstacle(state) > 1) {
		for(int i = 0; i < state->levelCount && !bestInfo; ++i) {
			MyWorldTagInfo *tagInfo = state->levels + i; 

			if((tagInfo->flags & (MyWorldFlagsU64)MY_WORLD_OBSTACLES) && (tagInfo->flags & biomeFlag)) {
				//NOTE(ollie): Update that we create a room with an obstacle
				state->lastRoomId_withObstacle = state->totalRoomCount;
				bestInfo = tagInfo;
				break;
			}
		}
	} 

	//NOTE(ollie): Default rooms to look for
	if(!bestInfo) {
		for(int i = 0; i < state->levelCount && !bestInfo; ++i) {
			MyWorldTagInfo *tagInfo = state->levels + i; 

			if((tagInfo->flags & biomeFlag) && !(tagInfo->flags & (MyWorldFlagsU64)MY_WORLD_BOSS) && !(tagInfo->flags & (MyWorldFlagsU64)MY_WORLD_ENEMIES)) {
				bestInfo = tagInfo;
				break;
			}
		}
	}

	bestInfo->usedCount++;
	state->totalRoomCount++;

	///////////////////////*************************////////////////////

	//NOTE(ollie): Make sure we found a level
	assert(bestInfo);
	Entity *room = myLevels_loadLevel(bestInfo->levelId, entityManager, startPos, gameState);

	return room;
}
