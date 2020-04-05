typedef struct {
	MyGameState *gameState;
	MyGameMode newMode;

	MyEntityManager *entityManager;
	MyGameStateVariables *gameVariables;

	EasyCamera *camera;
} MyTransitionData;

static MyTransitionData *getTransitionData(MyGameState *gameState, MyGameMode newMode, EasyCamera *camera, MyEntityManager *entityManager, MyGameStateVariables *gameVariables) {
	//NOTE(ollie): We free this when 
	MyTransitionData *result = (MyTransitionData *)easyPlatform_allocateMemory(sizeof(MyTransitionData), EASY_PLATFORM_MEMORY_ZERO);

	result->gameState = gameState;
	result->newMode = newMode;
	result->camera = camera;

	result->entityManager = entityManager;
	result->gameVariables = gameVariables;

	return result;
}


static void transitionCallBack(void *data_);
