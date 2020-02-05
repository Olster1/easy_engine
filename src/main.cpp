#include "defines.h"
#include "easy_headers.h"

#include "myGameState.h"
#include "myLevels.h"
#include "myEntity.h"
#include "myTransitions.h"


static Entity *myGame_beginRound(MyGameStateVariables *gameVariables, MyEntityManager *entityManager, bool createPlayer, Entity *player) {
    gameVariables->roomSpeed = -1.0f;
    gameVariables->playerMoveSpeed = 0.3f;

    gameVariables->minPlayerMoveSpeed = 0.1f;
    gameVariables->maxPlayerMoveSpeed = 1.0f;

    gameVariables->cameraTargetPos = -6.0f;
    gameVariables->liveTimerCount = 0;

    setParentChannelVolume(AUDIO_FLAG_SCORE_CARD, 0, 0.5f);
    setParentChannelVolume(AUDIO_FLAG_MAIN, 1, 0.5f);

    if(createPlayer) {
        player = initPlayer(entityManager, gameVariables, findTextureAsset("cup_empty.png"), findTextureAsset("cup_half_full.png"));
    } else {
        resetPlayer(player, gameVariables,findTextureAsset("cup_empty.png"));
    }

    gameVariables->mostRecentRoom = myLevels_generateLevel(global_periodRoom0, entityManager, v3(0, 0, 0));
    gameVariables->lastRoomCreated = myLevels_generateLevel(global_periodRoom1, entityManager, v3(0, 5, 0));

    return player;
}


static void transitionCallBackRestartRound(void *data_) {
    MyTransitionData *data = (MyTransitionData *)data_;

    data->gameState->lastGameMode = data->gameState->currentGameMode;
    data->gameState->currentGameMode = data->newMode;   

    setSoundType(AUDIO_FLAG_MAIN);

    easyEntity_endRound(data->entityManager);
    cleanUpEntities(data->entityManager, data->gameVariables);
    myGame_beginRound(data->gameVariables, data->entityManager, false, data->player);


    //NOTE(ol): Right now just using malloc & free
    free(data);
}


static void transitionCallBack(void *data_) {
    MyTransitionData *data = (MyTransitionData *)data_;

    data->gameState->lastGameMode = data->gameState->currentGameMode;
    data->gameState->currentGameMode = data->newMode;

    setParentChannelVolume(AUDIO_FLAG_SCORE_CARD, 0, 0.5f);
    setParentChannelVolume(AUDIO_FLAG_MAIN, 1, 0.5f);

    //NOTE(ol): Right now just using malloc & free
    free(data);
}

static EasyTerrain *initTerrain(EasyModel fern, EasyModel grass) {
    //TERRAIN STUFF
    EasyTerrain *terrain = pushStruct(&globalLongTermArena, EasyTerrain);
    easyTerrain_initTerrain(terrain);

    easyTerrain_AddTexture(terrain, findTextureAsset("grass.png")); 
    easyTerrain_AddTexture(terrain, findTextureAsset("mud.png")); 
    easyTerrain_AddTexture(terrain, findTextureAsset("path.png")); 
    easyTerrain_AddTexture(terrain, findTextureAsset("grassy3.png")); 

    easyTerrain_AddModel(terrain, &fern);
    easyTerrain_AddModel(terrain, &grass);

    easyTerrain_addChunk(terrain, "/img/terrain_data/heightMap.png", "/img/terrain_data/blendMap.png", v3(0, 0, 0), v3(100, 100, 10));

    return terrain;
}

static EasySkyBox *initSkyBox() {
    EasySkyBoxImages skyboxImages;
    skyboxImages.fileNames[0] = "skybox_images/right.jpg";
    skyboxImages.fileNames[1] = "skybox_images/left.jpg";
    skyboxImages.fileNames[2] = "skybox_images/top.jpg";
    skyboxImages.fileNames[3] = "skybox_images/bottom.jpg";
    skyboxImages.fileNames[4] = "skybox_images/front.jpg";
    skyboxImages.fileNames[5] = "skybox_images/back.jpg";
    
    EasySkyBox *skybox = easy_makeSkybox(&skyboxImages);
    return skybox;
}

int main(int argc, char *args[]) {

    DEBUG_TIME_BLOCK_FOR_FRAME_BEGIN(beginFrame)

    if(argc > 1) {
        for(int i = 0; i < argc; i++) {
            if(cmpStrNull("shaders", args[i])) {
                globalDebugWriteShaders = true;    
            }
            
        }
    }

    V2 screenDim = v2(DEFINES_WINDOW_SIZE_X, DEFINES_WINDOW_SIZE_Y); //init in create app function
    V2 resolution = v2(DEFINES_RESOLUTION_X, DEFINES_RESOLUTION_Y);


    // screenDim = resolution;
    bool fullscreen = false;
    OSAppInfo appInfo = easyOS_createApp("Easy Engine", &screenDim, fullscreen);

    if(appInfo.valid) {
        
        easyOS_setupApp(&appInfo, &resolution, RESOURCE_PATH_EXTENSION);
        
        loadAndAddImagesToAssets("img/");

        
        loadAndAddImagesToAssets("img/period_game/");
        
        ////INIT FONTS
        char *fontName = concatInArena(globalExeBasePath, "/fonts/BebasNeue-Regular.ttf", &globalPerFrameArena);
        Font mainFont = initFont(fontName, 88);
        Font smallMainFont = initFont(fontName, 52);

        char *fontName1 = concatInArena(globalExeBasePath, "/fonts/UbuntuMono-Regular.ttf", &globalPerFrameArena);
        
        //Set the debug font 
        globalDebugFont = initFont(fontName1, 32);
        ///

//******** CREATE THE FRAME BUFFERS ********///

        FrameBuffer mainFrameBuffer = createFrameBuffer(resolution.x, resolution.y, FRAMEBUFFER_COLOR | FRAMEBUFFER_DEPTH | FRAMEBUFFER_STENCIL | FRAMEBUFFER_HDR, 2);
        FrameBuffer toneMappedBuffer = createFrameBuffer(resolution.x, resolution.y, FRAMEBUFFER_COLOR | FRAMEBUFFER_DEPTH | FRAMEBUFFER_STENCIL, 1);

        FrameBuffer bloomFrameBuffer = createFrameBuffer(resolution.x, resolution.y, FRAMEBUFFER_COLOR | FRAMEBUFFER_HDR, 1);
//////////////////////////////////////////////////


/////************** Engine requirments *************//////////////

        bool hasBlackBars = true;
        bool running = true;
        AppKeyStates keyStates = {};


        EasyConsole console = {};
        easyConsole_initConsole(&console, BUTTON_TILDE);

        DEBUG_globalEasyConsole = &console;
        
        EasyEditor *editor = pushStruct(&globalLongTermArena, EasyEditor);
        easyEditor_initEditor(editor, globalRenderGroup, &globalDebugFont, OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y), resolution, appInfo.screenRelativeSize, &keyStates);

        easyFlashText_initManager(&globalFlashTextManager, &mainFont, resolution, appInfo.screenRelativeSize);

        bool inEditor = false;

        EasyTransitionState *transitionState = EasyTransition_initTransitionState(easyAudio_findSound("ting.wav"));

///////////************************/////////////////


        EasyCamera camera;
        easy3d_initCamera(&camera, v3(0, 0, -12));

        globalRenderGroup->skybox = initSkyBox();

        easy3d_loadMtl("models/terrain/fern.mtl");
        EasyModel fern = {};
        easy3d_loadObj("models/terrain/fern.obj", &fern);

        easy3d_loadMtl("models/Crystal.mtl");
        EasyModel Crystal = {};
        easy3d_loadObj("models/Crystal.obj", &Crystal);

        easy3d_loadMtl("models/terrain/grass.mtl");
        EasyModel grass = {};
        easy3d_loadObj("models/terrain/grass.obj", &grass);

        easy3d_loadMtl("models/tower.mtl");
        EasyModel castle = {};
        easy3d_loadObj("models/tower.obj", &castle);

        easy3d_loadMtl("models/Oak_Tree.mtl");
        easy3d_loadObj("models/Oak_Tree.obj", 0);

        easy3d_loadMtl("models/Palm_Tree.mtl");
        easy3d_loadObj("models/Palm_Tree.obj", 0);
        
        // EasyTerrain *terrain = initTerrain(fern, grass);

        EasyMaterial crateMaterial = easyCreateMaterial(findTextureAsset("crate.png"), 0, findTextureAsset("crate_specular.png"), 32);
        EasyMaterial emptyMaterial = easyCreateMaterial(findTextureAsset("grey_texture.jpg"), 0, findTextureAsset("grey_texture.jpg"), 32);
        EasyMaterial flowerMaterial = easyCreateMaterial(findTextureAsset("flower.png"), 0, findTextureAsset("grey_texture.jpg"), 32);

        EasyTransform sunTransform;
        easyTransform_initTransform(&sunTransform, v3(0, -10, 0));

        EasyLight *light = easy_makeLight(&sunTransform, EASY_LIGHT_DIRECTIONAL, 1.0f, v3(1, 1, 1));//easy_makeLight(v4(0, -1, 0, 0), v3(1, 1, 1), v3(1, 1, 1), v3(1, 1, 1));
        easy_addLight(globalRenderGroup, light);
        

///////////////////////*************************////////////////////

//NOTE(ollie): Initing the Entity Manager

    MyEntityManager *entityManager = initEntityManager();

    ///////////////////////************ Load the resources *************////////////////////


    ////////////////////////////////////////////////////////////////////


    MyGameStateVariables gameVariables = {};
    
    Entity *player = myGame_beginRound(&gameVariables, entityManager, true, 0);

    MyGameState *gameState = pushStruct(&globalLongTermArena, MyGameState);

    turnTimerOff(&gameState->animationTimer);

    gameState->currentGameMode = gameState->lastGameMode = MY_GAME_MODE_START_MENU;
    setSoundType(AUDIO_FLAG_SCORE_CARD);

    ///////////////////////*********** Tutorials **************////////////////////
    gameState->tutorialMode = true;
    for(int i = 0; i < arrayCount(gameState->gameInstructionsHaveRun); ++i) {
            gameState->gameInstructionsHaveRun[i] = false;
    }
    gameState->instructionType = GAME_INSTRUCTION_NULL;
    ////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////      


///////////////////////************ Setup level stuff*************////////////////////


EasySound_LoopSound(playGameSound(&globalLongTermArena, easyAudio_findSound("zoo_track.wav"), 0, AUDIO_BACKGROUND));
EasySound_LoopSound(playScoreBoardSound(&globalLongTermArena, easyAudio_findSound("ambient1.wav"), 0, AUDIO_BACKGROUND));

setParentChannelVolume(AUDIO_FLAG_MAIN, 0, 0);
setParentChannelVolume(AUDIO_FLAG_SCORE_CARD, 1, 0);
////////////////////////////////////////////////////////////////////  

////////////*** Variables *****//////
        float cameraMovePower = 2000.0f;
        
        float exposureTerm = 1.0f;
        int toneMapId = 0;
        bool controllingPlayer = true;

        EasyTransform T;
        easyTransform_initTransform(&T, v3(0, 0, 0));

        EasyProfile_ProfilerState *profilerState = EasyProfiler_initProfiler(); 


///////////************************/////////////////
        while(running) {
            {
        
            easyOS_processKeyStates(&keyStates, resolution, &screenDim, &running, !hasBlackBars);
            easyOS_beginFrame(resolution, &appInfo);
            
            beginRenderGroupForFrame(globalRenderGroup);

            clearBufferAndBind(appInfo.frameBackBufferId, COLOR_BLACK, FRAMEBUFFER_COLOR, 0);
            clearBufferAndBind(mainFrameBuffer.bufferId, COLOR_WHITE, mainFrameBuffer.flags, globalRenderGroup);
            
            renderEnableDepthTest(globalRenderGroup);
            renderEnableCulling(globalRenderGroup);
            setBlendFuncType(globalRenderGroup, BLEND_FUNC_STANDARD);
            renderSetViewPort(0, 0, resolution.x, resolution.y);

            if(wasPressed(keyStates.gameButtons, BUTTON_F1)) {
                inEditor = !inEditor;
                easyEditor_stopInteracting(editor);
            }


            EasyCamera_MoveType camMove = EASY_CAMERA_MOVE_NULL;
            if(DEBUG_global_IsFlyMode && !easyConsole_isOpen(&console) && !easyEditor_isInteracting(editor) && !inEditor) {
                camMove = (EasyCamera_MoveType)(EASY_CAMERA_MOVE | EASY_CAMERA_ROTATE | EASY_CAMERA_ZOOM);

                if(DEBUG_global_CameraMoveXY) camMove = easyCamera_addFlag(camMove, EASY_CAMERA_MOVE_XY);

                if(DEBUG_global_CamNoRotate) camMove = easyCamera_removeFlag(camMove, EASY_CAMERA_ROTATE);
                    
            }
             
            easy3d_updateCamera(&camera, &keyStates, 1, cameraMovePower, appInfo.dt, camMove);

            easy_setEyePosition(globalRenderGroup, camera.pos);

            // update camera first
            Matrix4 viewMatrix = easy3d_getWorldToView(&camera);
            Matrix4 perspectiveMatrix = projectionMatrixFOV(camera.zoom, resolution.x/resolution.y);

            //NOTE: This is to draw the skyquad 
            easyRender_updateSkyQuad(globalRenderGroup, camera.zoom, resolution.x/resolution.y, easy3d_getViewToWorld(&camera));

            renderSetShader(globalRenderGroup, &phongProgram);
            setModelTransform(globalRenderGroup, mat4());
            setViewTransform(globalRenderGroup, viewMatrix);
            setProjectionTransform(globalRenderGroup, perspectiveMatrix);

            renderEnableDepthTest(globalRenderGroup);

            renderDisableCulling(globalRenderGroup);
            
            setModelTransform(globalRenderGroup, Matrix4_translate(mat4(), v3(1, -1, 1)));
            // renderModel(globalRenderGroup, &Crystal, COLOR_WHITE);

            // setModelTransform(globalRenderGroup, Matrix4_translate(mat4(), v3(1, 1, 1)));
            // renderModel(globalRenderGroup, &fern, color);

            // easyTerrain_renderTerrain(terrain, globalRenderGroup, &emptyMaterial);

            renderEnableCulling(globalRenderGroup);

            renderSetShader(globalRenderGroup, &phongProgram);

            // setModelTransform(globalRenderGroup, Matrix4_translate(mat4(), v3(1, 1, 1)));
            // renderDrawCube(globalRenderGroup, &crateMaterial, COLOR_WHITE);


            // setModelTransform(globalRenderGroup, Matrix4_translate(mat4(), v3(1, 2, 1)));
            // renderModel(globalRenderGroup, &castle, COLOR_WHITE);

            
            drawRenderGroup(globalRenderGroup, (RenderDrawSettings)(RENDER_DRAW_DEFAULT));

            easyRender_blurBuffer(&mainFrameBuffer, &bloomFrameBuffer, 1);


//////////////// TONE MAPPING /////////////////////////
            clearBufferAndBind(toneMappedBuffer.bufferId, COLOR_YELLOW, toneMappedBuffer.flags, globalRenderGroup); 
            renderSetShader(globalRenderGroup, &toneMappingProgram);
            
            EasyRender_ToneMap_Bloom_DataPacket *packet = pushStruct(&globalPerFrameArena, EasyRender_ToneMap_Bloom_DataPacket);
            
            packet->mainBufferTexId = mainFrameBuffer.textureIds[toneMapId];
            packet->bloomBufferTexId = bloomFrameBuffer.textureIds[0];

            packet->exposure = exposureTerm;

            renderBlitQuad(globalRenderGroup, packet);

            drawRenderGroup(globalRenderGroup, RENDER_DRAW_DEFAULT);

////////////////////////////////////////////////////////////////////



///////////////////////// DEBUG UI /////////////////////////////////////////

            renderClearDepthBuffer(toneMappedBuffer.bufferId);
            renderSetFrameBuffer(toneMappedBuffer.bufferId, globalRenderGroup);


///////////////////////************ Update entities here *************////////////////////

            u32 updateFlags = 0;
            if(gameState->currentGameMode == MY_GAME_MODE_PLAY && !DEBUG_global_PauseGame) updateFlags |= MY_ENTITIES_UPDATE;
            
            if(gameState->currentGameMode == MY_GAME_MODE_PLAY ||
                gameState->currentGameMode == MY_GAME_MODE_PAUSE ||
                gameState->currentGameMode == MY_GAME_MODE_SCORE ||
                gameState->currentGameMode == MY_GAME_MODE_END_ROUND ||
                gameState->currentGameMode == MY_GAME_MODE_INSTRUCTION_CARD) {
                updateFlags |= MY_ENTITIES_RENDER;
            }

            if(updateFlags & MY_ENTITIES_RENDER) {

                ///////////////////////********* Drawing the background ****************////////////////////
                Texture *bgTexture = findTextureAsset("africa.png");
                
                float aspectRatio = (float)bgTexture->height / (float)bgTexture->width;
                float xWidth = resolution.x;
                float xHeight = xWidth*aspectRatio;

                renderTextureCentreDim(bgTexture, v3(0, 0, 10), v2(xWidth, xHeight), COLOR_WHITE, 0, mat4(), mat4(),  OrthoMatrixToScreen(resolution.x, resolution.y));                
                drawRenderGroup(globalRenderGroup, RENDER_DRAW_DEFAULT);
                renderClearDepthBuffer(toneMappedBuffer.bufferId);

                ////////////////////////////////////////////////////////////////////

                if(updateFlags & MY_ENTITIES_UPDATE) {
                    if(!DEBUG_global_IsFlyMode) {
                        camera.pos.z = lerp(camera.pos.z, appInfo.dt*1, gameVariables.cameraTargetPos); 
                    }
                    updateEntitiesPrePhysics(entityManager, &keyStates, &gameVariables, appInfo.dt);
                    EasyPhysics_UpdateWorld(&entityManager->physicsWorld, appInfo.dt);
                }

                ///////////////////////************ Draw the lives *************////////////////////

                Texture *bloodSplat = findTextureAsset("blood_splat.png");
                Texture *underpants = findTextureAsset("underwear.png");

                float f0 = easyRender_getTextureAspectRatio_HOverW(bloodSplat);
                float f1 = easyRender_getTextureAspectRatio_HOverW(underpants);

                float xAt = 0.1f*resolution.x;
                float yAt = 0.4f*resolution.y;
                float increment = 0.08f*resolution.x;

                outputText(&mainFont, xAt - 1*increment, yAt + 0.1f*increment, 1.0f, resolution, "Lives: ", InfinityRect2f(), COLOR_WHITE, 1, true, appInfo.screenRelativeSize);

                xAt += increment;
                for(int liveIndex = 0; liveIndex < player->maxHealthPoints; ++liveIndex) {

                    float w1 = 0.05f*resolution.x;
                    float h1 = w1*f0;
                    float h2 = w1*f1;
                    renderTextureCentreDim(underpants, v3(xAt + increment*liveIndex, yAt, NEAR_CLIP_PLANE + 0.1f), v2(w1, h2), COLOR_WHITE, 0, mat4TopLeftToBottomLeft(resolution.y), mat4(),  OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));

                    Timer *liveTimer = &gameVariables.liveTimers[liveIndex]; 
                    if(liveIndex < gameVariables.liveTimerCount) {
                        float canVal = 1.0f;

                        if(isOn(liveTimer)) {
                            TimerReturnInfo timerInfo = updateTimer(liveTimer, appInfo.dt);    

                            canVal = timerInfo.canonicalVal;
                            if(timerInfo.finished) {
                                turnTimerOff(liveTimer);
                            }
                        }
                        
                        renderTextureCentreDim(bloodSplat, v3(xAt + increment*liveIndex, yAt, NEAR_CLIP_PLANE), v2_scale(canVal, v2(w1, h1)), COLOR_WHITE, 0, mat4TopLeftToBottomLeft(resolution.y), mat4(),  OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));    
                                
                    }
                    
                }

                ////////////////////////////////////////////////////////////////////



                updateEntities(entityManager, gameState, &gameVariables, &keyStates, globalRenderGroup, viewMatrix, perspectiveMatrix, appInfo.dt, updateFlags);

                cleanUpEntities(entityManager, &gameVariables);

                ///////////////////////************ Draw the choc meter *************////////////////////

                float canonicalVal = 1.0f - inverse_lerp(gameVariables.minPlayerMoveSpeed, gameVariables.playerMoveSpeed, gameVariables.maxPlayerMoveSpeed);
                assert(canonicalVal >= 0.0f && canonicalVal <= 1.0f);

                float barWidth = 0.15f*resolution.x;
                float barHeight = 0.1f*resolution.y;
                renderDrawRect(rect2fMinDim(barWidth, barHeight, barWidth, barHeight), NEAR_CLIP_PLANE + 0.01f, COLOR_BLACK, 0, mat4(), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));                    
                renderDrawRect(rect2fMinDim(barWidth, barHeight, canonicalVal*barWidth, barHeight), NEAR_CLIP_PLANE, COLOR_GREEN, 0, mat4(), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));                    

                ////////////////////////////////////////////////////////////////////

                ///////////////////////*********** Draw the ui points board**************////////////////////
                char buffer[512];
                // sprintf(buffer, "%d", player->healthPoints);

                Texture *dropletTex = findTextureAsset("blood_droplet.PNG");
                aspectRatio = (float)dropletTex->height / (float)dropletTex->width;
                xWidth = 0.05f*resolution.x;
                xHeight = xWidth*aspectRatio;
                renderTextureCentreDim(dropletTex, v3(0.1f*resolution.x, 0.1f*resolution.y, 1), v2(xWidth, xHeight), COLOR_WHITE, 0, mat4TopLeftToBottomLeft(resolution.y), mat4(),  OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));
                sprintf(buffer, "%d", player->dropletCount);
                outputText(&mainFont, 0.15f*resolution.x, 0.1f*resolution.y + 0.3f*xHeight, 1.0f, resolution, buffer, InfinityRect2f(), COLOR_WHITE, 1, true, appInfo.screenRelativeSize);


                Texture *cupTexture = (player->dropletCountStore > 0) ? findTextureAsset("cup_half_full.png") : findTextureAsset("cup_empty.png");
                
                aspectRatio = (float)cupTexture->height / (float)cupTexture->width;
                xWidth = 0.05f*resolution.x;
                xHeight = xWidth*aspectRatio;

                renderTextureCentreDim(cupTexture, v3(0.1f*resolution.x, 0.25f*resolution.y, 1), v2(xWidth, xHeight), COLOR_WHITE, 0, mat4TopLeftToBottomLeft(resolution.y), mat4(),  OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));
                sprintf(buffer, "%d", player->dropletCountStore);
                outputText(&mainFont, 0.15f*resolution.x, 0.25f*resolution.y + 0.3f*xHeight, 1.0f, resolution, buffer, InfinityRect2f(), COLOR_WHITE, 1, true, appInfo.screenRelativeSize);

                ////////////////////////////////////////////////////////////////////

                if(gameState->currentGameMode != MY_GAME_MODE_PLAY) {
                    drawRenderGroup(globalRenderGroup, RENDER_DRAW_SORT);
                    easyRender_blurBuffer(&toneMappedBuffer, &toneMappedBuffer, 0);

                    renderClearDepthBuffer(toneMappedBuffer.bufferId);
                }
            }

            // renderTextureCentreDim(findTextureAsset("cup_half_full.png"), v3(100, 100, 1), v2(100, 100), COLOR_WHITE, 0, mat4(), mat4(),  OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));

///////////////////////************* Update the other game modes ************////////////////////
            static float zCoord = 1.0f; 

            if(DEBUG_global_DrawFrameRate) {
                char frameRate[256];
                sprintf(frameRate, "%f", 1.0f / appInfo.dt);
                outputTextNoBacking(&mainFont, 0.1f*resolution.x, 0.1f*resolution.y, NEAR_CLIP_PLANE, resolution, frameRate, InfinityRect2f(), COLOR_GREEN, 1, true, appInfo.screenRelativeSize);
            }

            switch (gameState->currentGameMode) {
                case MY_GAME_MODE_START_MENU: {
                    if(wasPressed(keyStates.gameButtons, BUTTON_ENTER)) {

                        MyTransitionData *data = getTransitionData(gameState, MY_GAME_MODE_PLAY);
                        EasyTransition_PushTransition(transitionState, transitionCallBack, data, EASY_TRANSITION_FADE);

                    }

                    Texture *bloodDrop = findTextureAsset("blood_droplet.PNG");
                    renderTextureCentreDim(bloodDrop, v3(-0.15f*resolution.x, 0, 0.5f), v2(0.15*resolution.x, 0.15*resolution.x*bloodDrop->aspectRatio_h_over_w), COLOR_WHITE, 0, mat4(), mat4(),  OrthoMatrixToScreen(resolution.x, resolution.y));
                    outputTextNoBacking(&mainFont, resolution.x / 2, resolution.y / 2, NEAR_CLIP_PLANE, resolution, "The Period Game", InfinityRect2f(), COLOR_BLACK, 1, true, appInfo.screenRelativeSize);
                
                } break;
                case MY_GAME_MODE_PLAY: {
                    if(wasPressed(keyStates.gameButtons, BUTTON_ESCAPE)) {
                        gameState->lastGameMode = gameState->currentGameMode;
                        gameState->currentGameMode = MY_GAME_MODE_PAUSE;
                    }

                } break;
                case MY_GAME_MODE_PAUSE: {
                    if(wasPressed(keyStates.gameButtons, BUTTON_ESCAPE)) {
                        gameState->lastGameMode = gameState->currentGameMode;
                        gameState->currentGameMode = MY_GAME_MODE_PLAY;
                    }

                    outputTextNoBacking(&mainFont, resolution.x / 2, resolution.y / 2, zCoord, resolution, "IS PAUSED!", InfinityRect2f(), COLOR_BLACK, 1, true, appInfo.screenRelativeSize);
                } break;
                case MY_GAME_MODE_INSTRUCTION_CARD: {

                    if(wasPressed(keyStates.gameButtons, BUTTON_ENTER)) {
                        gameState->animationTimer = initTimer(0.5f, false);
                        gameState->isIn = false;
                    }

                    float xAt = 0;

                    if(isOn(&gameState->animationTimer)) {
                        TimerReturnInfo timerInfo = updateTimer(&gameState->animationTimer, appInfo.dt);    

                        if(gameState->isIn) {
                            xAt = lerp(-resolution.x, timerInfo.canonicalVal, 0);
                        } else {
                            xAt = lerp(0, timerInfo.canonicalVal, resolution.x);
                        }

                        if(timerInfo.finished) {
                            turnTimerOff(&gameState->animationTimer);
                            if(!gameState->isIn) {
                                gameState->lastGameMode = gameState->currentGameMode;
                                gameState->currentGameMode = MY_GAME_MODE_PLAY;
                            }
                        }
                    }

                    Texture *imageToDisplay = 0;
                    char *stringToDisplay = "(null)";
                    switch (gameState->instructionType) {
                        case GAME_INSTRUCTION_DROPLET: {
                            imageToDisplay = findTextureAsset("blood_droplet.PNG");
                            stringToDisplay = "The aim is to collect blood droplets as you go.";
                        } break;
                        case GAME_INSTRUCTION_CHOCOLATE: {
                            imageToDisplay = findTextureAsset("choc_bar.png");
                            stringToDisplay = "Chocolate helps you move fast through each lane, & dodge obstacles.";
                        } break;
                        case GAME_INSTRUCTION_TOILET: {
                            imageToDisplay = findTextureAsset("toilet1.png");
                            stringToDisplay = "Look out for places to empty your cup. If you crash before you do, you'll lose your points.";
                        } break;
                        case GAME_INSTRUCTION_CRAMP: {
                            imageToDisplay = findTextureAsset("cramp.PNG");
                            stringToDisplay = "Ouch! Watch out for these little guys, they pack a punch. You can also press SpaceBar to shoot them. You'll also lose a set of underpants.";
                        } break;
                        default: {
                            assert(false);
                        }
                    }
                    
                    renderTextureCentreDim(imageToDisplay, v3(xAt+0.3f*resolution.x, 0.5f*resolution.y, 0.5f), v2(0.15*resolution.x, 0.15*resolution.x*imageToDisplay->aspectRatio_h_over_w), COLOR_WHITE, 0, mat4TopLeftToBottomLeft(resolution.y), mat4(),  OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));

                    Rect2f m = rect2fMinDim(xAt+0.5*resolution.x, 0.1*resolution.y, 0.3*resolution.x, resolution.y);

                    V2 bounds = getBounds(stringToDisplay, m, &smallMainFont, 1, resolution, appInfo.screenRelativeSize);

                    renderDrawRectCenterDim(v3(xAt, 0, 1), v2_scale(0.7f, v2(resolution.x, resolution.y)), COLOR_WHITE, 0, mat4(), OrthoMatrixToScreen(resolution.x, resolution.y));
                    
                    outputTextNoBacking(&smallMainFont, m.min.x, 0.5f*resolution.y - 0.5f*bounds.y + 1.1f*smallMainFont.fontHeight, NEAR_CLIP_PLANE, resolution, stringToDisplay, m, COLOR_BLACK, 1, true, appInfo.screenRelativeSize);
                } break;
                case MY_GAME_MODE_END_ROUND: {

                    if(wasPressed(keyStates.gameButtons, BUTTON_ENTER) && !EasyTransition_InTransition(transitionState)) {
                        MyTransitionData *data = getTransitionData(gameState, MY_GAME_MODE_PLAY);
                        data->entityManager = entityManager;
                        data->gameVariables = &gameVariables;
                        data->player = player;
                        EasyTransition_PushTransition(transitionState, transitionCallBackRestartRound, data, EASY_TRANSITION_FADE);

                        gameState->animationTimer = initTimer(0.5f, false);
                        gameState->isIn = false;

                        assert(isOn(&gameState->animationTimer));
                    }

                    float xAt = 0;


                    if(isOn(&gameState->animationTimer)) {
                        TimerReturnInfo timerInfo = updateTimer(&gameState->animationTimer, appInfo.dt);    

                        if(gameState->isIn) {
                            xAt = lerp(-resolution.x, timerInfo.canonicalVal, 0);
                        } else {
                            xAt = lerp(0, timerInfo.canonicalVal, resolution.x);
                        }

                        if(timerInfo.finished) {
                            turnTimerOff(&gameState->animationTimer);
                        }
                    } else if(EasyTransition_InTransition(transitionState)) {
                        xAt = resolution.x;       
                    }

                    Texture *dropletTex = findTextureAsset("blood_droplet.PNG");
                    renderDrawRectCenterDim(v3(xAt, 0, 1), v2_scale(0.7f, v2(resolution.x, resolution.y)), COLOR_WHITE, 0, mat4(), OrthoMatrixToScreen(resolution.x, resolution.y));
                    renderTextureCentreDim(dropletTex, v3(xAt+0.3f*resolution.x, 0.6f*resolution.y, 0.5f), v2(0.05*resolution.x, 0.05*resolution.x*dropletTex->aspectRatio_h_over_w), COLOR_WHITE, 0, mat4TopLeftToBottomLeft(resolution.y), mat4(),  OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));
                    
                    Rect2f m = rect2fMinDim(xAt+0.2*resolution.x, 0, 0.6*resolution.x, resolution.y);

                    int dropCount = player->dropletCountStore;

                    Timer *pointLoadTimer = &gameVariables.pointLoadTimer;
                    if(isOn(pointLoadTimer)) {
                        TimerReturnInfo timerInfo = updateTimer(pointLoadTimer, appInfo.dt);    

                        float canVal = timerInfo.canonicalVal;

                        dropCount = (int)(canVal*player->dropletCountStore);

                        if(gameVariables.lastCount != dropCount) {
                            playScoreBoardSound(&globalLongTermArena, easyAudio_findSound("click2.wav"), 0, AUDIO_FOREGROUND);
                        }
                        gameVariables.lastCount = dropCount;

                        if(timerInfo.finished) {
                            turnTimerOff(pointLoadTimer);
                        }
                    }

                    char points[512];
                    sprintf(points, "%d", dropCount);

                    outputTextNoBacking(&mainFont, xAt+0.35f*resolution.x, 0.65f*resolution.y, 0.5f, resolution, points, m, COLOR_BLACK, 1, true, appInfo.screenRelativeSize);
                    
                    char *str = "Ah! You have no more clean underwear.";
            

                    outputTextNoBacking(&mainFont, xAt+0.2f*resolution.x, 0.3f*resolution.y, 0.5f, resolution, str, m, COLOR_BLACK, 1, true, appInfo.screenRelativeSize);
                } break;
                case MY_GAME_MODE_SCORE: {

                } break;
                case MY_GAME_MODE_LEVEL_SELECTOR: {

                } break;
                case MY_GAME_MODE_START: {

                } break;
                default: {
                    assert(false);
                }
            }

////////////////////////////////////////////////////////////////////



/////////////////////// DRAWING & UPDATE CONSOLE /////////////////////////////////
            if(easyConsole_update(&console, &keyStates, appInfo.dt, resolution, appInfo.screenRelativeSize)) {
                EasyToken token = easyConsole_getNextToken(&console);
                if(token.type == TOKEN_WORD) {
                    if(stringsMatchNullN("camPower", token.at, token.size)) {
                        token = easyConsole_seeNextToken(&console);
                        if(token.type == TOKEN_FLOAT) {
                            token = easyConsole_getNextToken(&console);
                            cameraMovePower = token.floatVal;
                        } else if(token.type == TOKEN_INTEGER) {
                            token = easyConsole_getNextToken(&console);
                            cameraMovePower = (float)token.intVal;    
                        } else {
                            easyConsole_addToStream(&console, "must pass a number");
                        }
                        
                    } else if(stringsMatchNullN("wireframe", token.at, token.size)) {
                        token = easyConsole_getNextToken(&console);
                        if(stringsMatchNullN("on", token.at, token.size)) {
                            DEBUG_drawWireFrame = true;
                        } else if(stringsMatchNullN("off", token.at, token.size)) {
                            DEBUG_drawWireFrame = false;
                        } else {
                            easyConsole_addToStream(&console, "parameter not understood");
                        }
                    
                    } else if(stringsMatchNullN("bloom", token.at, token.size)) {
                        toneMapId = (toneMapId) ? 0 : 1;
                    } else if(stringsMatchNullN("camPos", token.at, token.size)) { 
                        char buffer[256];
                        sprintf(buffer, "camera pos: %f %f %f\n", camera.pos.x, camera.pos.y, camera.pos.z);
                        easyConsole_addToStream(&console, buffer);
                    } else if(stringsMatchNullN("movePlayer", token.at, token.size)) { 
                        DEBUG_global_movePlayer = !DEBUG_global_movePlayer;
                    } else {
                        easyConsole_parseDefault(&console, token);
                    }
                } else {
                    easyConsole_parseDefault(&console, token);
                }
            }
//////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////// DRAWING & UPDATE IN GAME EDITOR /////////////////////////////////                    
            if(inEditor) {
                easyEditor_startWindow(editor, "Lister Panel");
                {
                    easyEditor_pushSlider(editor, "Level move speed: ", &gameVariables.roomSpeed, -3.0f, -0.5f);
                    easyEditor_pushSlider(editor, "Player move speed: ", &gameVariables.playerMoveSpeed, 0.1f, 1.0f);

                    easyEditor_pushFloat1(editor, "min player move speed: ", &gameVariables.minPlayerMoveSpeed);
                    easyEditor_pushFloat1(editor, "max player move speed: ", &gameVariables.maxPlayerMoveSpeed);

                }
                // static V3 v = {};
                // easyEditor_pushFloat3(editor, "Position:", &T.pos.x, &T.pos.y, &T.pos.z);
                // easyEditor_pushFloat3(editor, "Rotation:", &T.Q.i, &T.Q.j, &T.Q.k);
                // easyEditor_pushFloat3(editor, "Scale:", &T.scale.x, &T.scale.y, &T.scale.z);

                // easyEditor_pushSlider(editor, "Exposure:", &exposureTerm, 0.3f, 3.0f);
                // easyEditor_pushSlider(editor, "camera z:", &camera.pos.z, -5.0f, 5.0f);
    
                // static V4 color = COLOR_GREEN;
                // easyEditor_pushColor(editor, "Color: ", &color);
                // static float a = 1.0f;
                // easyEditor_pushSlider(editor, "Some Value: ", &a, 0, 10);

                // static bool saveLevel = false;
                // if(easyEditor_pushButton(editor, "Save Level")) {
                //     saveLevel = !saveLevel;
                //      easyFlashText_addText(&globalFlashTextManager, "SAVED!");
                // }

                // if(saveLevel) {
                //     easyEditor_pushFloat1(editor, "", &v.x);
                //     easyEditor_pushFloat2(editor, "", &v.x, &v.y);
                //     easyEditor_pushFloat3(editor, "", &v.x, &v.y, &v.z);
                // }

                easyEditor_endWindow(editor); //might not actuall need this
                

                easyEditor_endEditorForFrame(editor);
            }

            easyFlashText_updateManager(&globalFlashTextManager, globalRenderGroup, appInfo.dt);

//////////////////////////////////// DRAW THE UI ITEMS ///////

            drawRenderGroup(globalRenderGroup, RENDER_DRAW_SORT);


//////////////////////////////////////////////////////////////////////////////////////////////

            //NOTE(ollie): Make sure the transition is on top
            renderClearDepthBuffer(toneMappedBuffer.bufferId);
            }

            EasyTransition_updateTransitions(transitionState, resolution, appInfo.dt);
            
            drawRenderGroup(globalRenderGroup, RENDER_DRAW_DEFAULT);

            ///////////////////////********** Drawing the Profiler Graph ***************////////////////////
            renderClearDepthBuffer(toneMappedBuffer.bufferId);

            EasyProfile_DrawGraph(profilerState, resolution, &keyStates, appInfo.screenRelativeSize);
            
            drawRenderGroup(globalRenderGroup, RENDER_DRAW_DEFAULT);

////////////////////////////////////////////////////////////////////////////

            easyOS_updateHotKeys(&keyStates);
            easyOS_endFrame(resolution, screenDim, toneMappedBuffer.bufferId, &appInfo, hasBlackBars);
            
            DEBUG_TIME_BLOCK_FOR_FRAME_END(beginFrame)
            DEBUG_TIME_BLOCK_FOR_FRAME_START(beginFrame)
            easyOS_endKeyState(&keyStates);
        }
        easyOS_endProgram(&appInfo);
    }
    return(0);
}
    