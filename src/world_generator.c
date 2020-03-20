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

//NOTE(ollie): This is the important function that chooses the next room
static Entity *myWorlds_findNextRoom(MyWorldState *worldState, MyEntityManager *entityManager, V3 startPos, MyGameState *gameState) {

	MyWorldTagInfo *bestInfo = 0;

	//NOTE(ollie): Loop through all the rooms and find the one that is best fit based on the current state of the world.  
	for(int i = 0; i < worldState->levelCount; ++i) {
		MyWorldTagInfo *tagInfo = worldState->levels + i; 

		if()


	}

	Entity *room = myLevels_loadLevel(bestInfo->levelId, entityManager, startPos, gameState);

	return room;
}
