///////////////////////************ Declarations *************////////////////////

Entity *myLevels_loadLevel(int level, MyEntityManager *entityManager, V3 startPos, MyGameState *gameState);
void myLevels_getLevelInfo(int level, MyWorldTagInfo *tagInfo);

////////////////////////////////////////////////////////////////////


///////////////////////************ Functions *************////////////////////

//NOTE(ollie): You pass in the tags of the world
static void myWorlds_generateWorld(MyWorldFlags flags, MyWorldState *worldState) {

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

//NOTE(ollie): This is the important function that chooses the next room
static Entity *myWorlds_findNextRoom(MyWorldState *state, MyEntityManager *entityManager, V3 startPos, MyGameState *gameState) {

	//NOTE(ollie): Start with first level
	MyWorldTagInfo *bestInfo = &state->levels[0];

	#define ROOMS_TILL_BOSS 20
	//NOTE(ollie): Check for boss conditions
	if(state->hasBoss && !state->defeatedBoss && !state->inBoss) {
		if(state->totalRoomCount >= ROOMS_TILL_BOSS && !state->fightingEnemies) {

			//NOTE(ollie): Look for the boss level
			for(int i = 0; i < state->levelCount && !bestInfo; ++i) {
				MyWorldTagInfo *tagInfo = state->levels + i;

				
				if(tagInfo->flags & myWorld_getFlagForBossType(state->bossType)) {
					bestInfo = tagInfo;
					break;
				}
			}
		}
	}

	//NOTE(ollie): Keep looking for next level

	MyWorldFlagsU64 biomeFlag = myWorld_getFlagForBiomeType(state->biomeType);

	//NOTE(ollie): Look for puzzle level
	if(!bestInfo && state->hasPuzzle && !state->fightingEnemies && state->roomsSincePuzzle > 10) {
		for(int i = 0; i < state->levelCount; ++i) {
			MyWorldTagInfo *tagInfo = state->levels + i; 

			if((tagInfo->flags & (MyWorldFlagsU64)MY_WORLD_PUZZLE) && (tagInfo->flags & biomeFlag)) {
				bestInfo = tagInfo;
				break;
			}
		}
	}

	//NOTE(ollie): Look for enemy level
	if(!bestInfo && state->hasEnemies && !state->fightingEnemies && state->roomsSinceEnemies > 3) {
		for(int i = 0; i < state->levelCount; ++i) {
			MyWorldTagInfo *tagInfo = state->levels + i; 

			if((tagInfo->flags & (MyWorldFlagsU64)MY_WORLD_ENEMIES) && (tagInfo->flags & biomeFlag)) {
				bestInfo = tagInfo;
				break;
			}
		}
	}

	//NOTE(ollie): Look for obstacle level
	if(!bestInfo && state->hasObstacles && state->roomsSinceObstacle > 1) {
		for(int i = 0; i < state->levelCount; ++i) {
			MyWorldTagInfo *tagInfo = state->levels + i; 

			if((tagInfo->flags & (MyWorldFlagsU64)MY_WORLD_OBSTACLES) && (tagInfo->flags & biomeFlag)) {
				bestInfo = tagInfo;
				break;
			}
		}
	} 

	//NOTE(ollie): Default rooms to look for
	if(!bestInfo) {
		for(int i = 0; i < state->levelCount; ++i) {
			MyWorldTagInfo *tagInfo = state->levels + i; 

			if((tagInfo->flags & biomeFlag) && !(tagInfo->flags & (MyWorldFlagsU64)MY_WORLD_BOSS) && !(tagInfo->flags & (MyWorldFlagsU64)MY_WORLD_ENEMIES)) {
				bestInfo = tagInfo;
				break;
			}
		}
	}

	//NOTE(ollie): Make sure we found a level
	assert(bestInfo);
	Entity *room = myLevels_loadLevel(bestInfo->levelId, entityManager, startPos, gameState);

	return room;
}
