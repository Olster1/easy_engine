static void myOverworld_updateOverworldState(RenderGroup *renderGroup, AppKeyStates *keyStates, MyOverworldState *state) {
	DEBUG_TIME_BLOCK();

	renderSetShader(renderGroup, &textureProgram);
	setViewTransform(renderGroup, state->viewMatrix);
	setProjectionTransform(renderGroup, state->projectionMatrix);


	for(int i = 0; i < state->manager.entityCount; ++i) {
		OverworldEntity *e = &state->manager.entities[i];

		setModelTransform(renderGroup, easyTransform_getTransform(&e->T));

		if(e->type == OVERWORLD_ENTITY_SHIP) {
			renderDrawSprite(renderGroup, findTextureAsset("spaceship.png"), COLOR_WHITE);
		}

	}
	
}
