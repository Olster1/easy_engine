typedef struct {
	MyGameState *gameState;
	MyGameMode newMode;
} MyTransitionData;

static MyTransitionData *getTransitionData(MyGameState *gameState, MyGameMode newMode) {
	MyTransitionData *result = (MyTransitionData *)malloc(sizeof(MyTransitionData));

	result->gameState = gameState;
	result->newMode = newMode;

	return result;
}

static void transitionCallBack(void *data_) {
	MyTransitionData *data = (MyTransitionData *)data_;

	data->gameState->lastGameMode = data->gameState->currentGameMode;
	data->gameState->currentGameMode = data->newMode;

	//NOTE(ol): Right now just using malloc & free
	free(data);
}

