typedef struct {
	MyGameState *gameState;
	MyGameMode newMode;

	MyEntityManager *entityManager;
	MyGameStateVariables *gameVariables;

	Entity * player;
} MyTransitionData;

static MyTransitionData *getTransitionData(MyGameState *gameState, MyGameMode newMode) {
	MyTransitionData *result = (MyTransitionData *)malloc(sizeof(MyTransitionData));

	result->gameState = gameState;
	result->newMode = newMode;

	return result;
}
