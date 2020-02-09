typedef struct {
	MyGameState *gameState;
	MyGameMode newMode;

	MyEntityManager *entityManager;
	MyGameStateVariables *gameVariables;

	Entity * player;

	EasyCamera *camera;
} MyTransitionData;

static MyTransitionData *getTransitionData(MyGameState *gameState, MyGameMode newMode, EasyCamera *camera) {
	MyTransitionData *result = (MyTransitionData *)malloc(sizeof(MyTransitionData));

	result->gameState = gameState;
	result->newMode = newMode;
	result->camera = camera;

	return result;
}
