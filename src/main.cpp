#include "defines.h"
#include "easy_headers.h"

 #include "myGameState.h"
#include "myLevels.h"
#include "myEntity.h"
#include "mySaveLoad.h"
#include "myTransitions.h"
// #include "myDialogue.h"


///////////////////////************ Init Game State *************////////////////////
//NOTE(ollie): Go straight to editing the level or actually play the level

/*
MY_GAME_MODE_START_MENU,
MY_GAME_MODE_LEVEL_SELECTOR,
MY_GAME_MODE_PLAY,
MY_GAME_MODE_PAUSE,
MY_GAME_MODE_SCORE,
MY_GAME_MODE_END_ROUND,
MY_GAME_MODE_INSTRUCTION_CARD,
MY_GAME_MODE_START,
MY_GAME_MODE_EDIT_LEVEL
*/
#define GAME_STATE_TO_LOAD MY_GAME_MODE_EDIT_LEVEL

//If we are editing a level, the level we want to enter into on startup
#define LEVEL_TO_LOAD 0

static inline MyGameState *myGame_initGameState(MyEntityManager *entityManager, EasyCamera *cam) {
    //NOTE(ollie): This is only called once on startup 
    // THIS IS NOT THE FUNCTION FOR INITING A ROUND

    MyGameState *gameState = pushStruct(&globalLongTermArena, MyGameState);

    ///////////////////////************* Editor stuff ************////////////////////
    gameState->currentLevelEditing = 0;
    gameState->hotEntity = 0;
    gameState->holdingEntity = false;
    ////////////////////////////////////////////////////////////////////

    turnTimerOff(&gameState->animationTimer);
    gameState->isIn = false;

    gameState->currentGameMode = gameState->lastGameMode = GAME_STATE_TO_LOAD;

    if(GAME_STATE_TO_LOAD == MY_GAME_MODE_EDIT_LEVEL) {
        //NOTE(ollie): We aren't begining a round so we need to create a level,
        // See beginRound func to see us using this function again. 
        myLevels_loadLevel(LEVEL_TO_LOAD, entityManager, v3(0, 0, 0));

        DEBUG_global_IsFlyMode = true;
        DEBUG_global_CameraMoveXY = true;

        cam->pos.z = -12;
    }

    setSoundType(AUDIO_FLAG_SCORE_CARD);

    ///////////////////////*********** Tutorials **************////////////////////
    gameState->tutorialMode = false;
    for(int i = 0; i < arrayCount(gameState->gameInstructionsHaveRun); ++i) {
            gameState->gameInstructionsHaveRun[i] = false;
    }
    gameState->instructionType = GAME_INSTRUCTION_NULL;
    ////////////////////////////////////////////////////////////////////

    gameState->altitudeSliderAt = 1.0f;
    gameState->holdingAltitudeSlider = false;

    ////////////////////////////////////////////////////////////////////

    return gameState;
} 

static Quaternion myGame_getCameraOrientation(V3 cameraPos, V3 targetPos) {
    Matrix4 m = easy3d_lookAt(cameraPos, targetPos, v3(0, 0, -1));

    //since the matrix that lookat gives back is from world to view, we actually want the 
    //transpose the camera orientation is defined in world space i.e. is view to world (the reverse)
    m = mat4_transpose(m);

    Quaternion result = easyMath_normalizeQuaternion(easyMath_matrix4ToQuaternion(m));

    return result;
}

static V3 myGame_getCameraPosition(float altitudeDegrees, MyGameState_ViewAngle type) {
    float altitudeRadians = TAU32 * (altitudeDegrees / 360.0f);
    V3 yAxis = v3_scale(sin(altitudeRadians), v3(0, 0, -1)); //-ve z direction for the y axis
    V3 xAxis = v3(0, 0, 0);
    switch(type) {
        case MY_VIEW_ANGLE_BOTTOM: {
            xAxis = v3(0, -1, 0);
        } break;
        case MY_VIEW_ANGLE_RIGHT: {
            xAxis = v3(1, 0, 0);
        } break;
        case MY_VIEW_ANGLE_TOP: {
            xAxis = v3(0, 1, 0);
        } break;
        case MY_VIEW_ANGLE_LEFT: {
            xAxis = v3(-1, 0, 0);
        } break;
    }
    xAxis = v3_scale(cos(altitudeRadians), xAxis); 

    V3 result = v3_plus(xAxis, yAxis);

    return result;
}


static V3 myGame_getCameraGlobalPosition(MyGameStateVariables *gameVariables, float distanceFromCamera) {
    V3 targetPos = myGame_getCameraPosition(gameVariables->angleDegreesAltitude, gameVariables->angleType);
    V3 relPos = v3_scale(distanceFromCamera, normalizeV3(targetPos));
    V3 result = v3_plus(relPos, gameVariables->centerPos);
    return result;
}


static void myGame_resetGameVariables(MyGameStateVariables *gameVariables, EasyCamera *cam) {
    gameVariables->roomSpeed = -1.0f;
    gameVariables->playerMoveSpeed = 0.3f;

    gameVariables->minPlayerMoveSpeed = 0.1f;
    gameVariables->maxPlayerMoveSpeed = 1.0f;

    
    gameVariables->liveTimerCount = 0;

    gameVariables->boostTimer = initTimer(1.0f, false); 
    turnTimerOff(&gameVariables->boostTimer);

    ///////////////////////************ Editor State *************////////////////////

    gameVariables->angleType = MY_VIEW_ANGLE_BOTTOM;
    gameVariables->angleDegreesAltitude = 89.0f; //degrees
    gameVariables->lerpTimer = initTimer(1.0f, false);
    turnTimerOff(&gameVariables->lerpTimer) ;

    gameVariables->targetCenterPos = v3(0, 0, 0);
    gameVariables->centerPos = v3(0, 0, 0);
    gameVariables->startPos = v3(0, 0, 0);

    ////////////////////////////////////////////////////////////////////
    //NOTE(ollie): Setup camera's initial position
    gameVariables->cameraDistance = 12;
    cam->pos = v3_scale(gameVariables->cameraDistance, myGame_getCameraPosition(gameVariables->angleDegreesAltitude, gameVariables->angleType)); 

    cam->orientation = myGame_getCameraOrientation(cam->pos, v3(0, 0, 0));

    gameVariables->cameraTargetPos = 6.0f;
    
    ////////////////////////////////////////////////////////////////////

    gameVariables->cachedSpeed = 0;

    gameVariables->mostRecentRoom = 0;
    gameVariables->lastRoomCreated = 0;
}


static Entity *myGame_beginRound(MyGameStateVariables *gameVariables, MyEntityManager *entityManager, bool createPlayer, Entity *player, EasyCamera *cam) {

    setParentChannelVolume(AUDIO_FLAG_SCORE_CARD, 0, 0.5f);
    setParentChannelVolume(AUDIO_FLAG_MAIN, 1, 0.5f);

    myGame_resetGameVariables(gameVariables, cam);

    if(createPlayer) {
        player = initPlayer(entityManager, gameVariables, findTextureAsset("cup_empty.png"), findTextureAsset("cup_half_full.png"));
    } else {
        resetPlayer(player, gameVariables,findTextureAsset("cup_empty.png"));
    }

    gameVariables->mostRecentRoom = myLevels_loadLevel(0, entityManager, v3(0, 0, 0));
    gameVariables->lastRoomCreated = myLevels_loadLevel(1, entityManager, v3(0, 5, 0));

    return player;
}


static void transitionCallBackRestartRound(void *data_) {
    MyTransitionData *data = (MyTransitionData *)data_;

    data->gameState->lastGameMode = data->gameState->currentGameMode;
    data->gameState->currentGameMode = data->newMode;   

    setSoundType(AUDIO_FLAG_MAIN);

    easyEntity_endRound(data->entityManager);
    cleanUpEntities(data->entityManager, data->gameVariables);
    myGame_beginRound(data->gameVariables, data->entityManager, false, data->player, data->camera);


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
    DEBUG_TIME_BLOCK_FOR_FRAME_BEGIN(beginFrame, "Main: Intial setup");

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
        EasyTime_setupTimeDatums();
       
        
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

        char *minerDialogue = concatInArena(globalExeBasePath, "/../src/dialogue/test.txt", &globalPerFrameArena);

        // myDialogue_compileProgram(minerDialogue);
        // exit(0);

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
        easy3d_initCamera(&camera, v3(0, 0, 0));

        globalRenderGroup->skybox = initSkyBox();

#define LOAD_MODELS_AUTOMATICALLY 1
///////////////////////************* Loading the models ************////////////////////
#if !LOAD_MODELS_AUTOMATICALLY
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

        easy3d_loadMtl("models/tile.mtl");
        easy3d_loadObj("models/tile.obj", 0);        

        int modelLoadedCount = 6;
        char *modelsLoaded[] = {"Crystal.obj", "grass.obj", "fern.obj", "tower.obj", "Oak_Tree.obj", "Palm_Tree.obj"};
#else  
        char *modelDirs[] = { "models/"};

        EasyAssetLoader_AssetArray allModelsForEditor = {};
        allModelsForEditor.count = 0;
        allModelsForEditor.assetType = ASSET_MODEL;

        for(int dirIndex = 0; dirIndex < arrayCount(modelDirs); ++dirIndex) {
            char *dir = modelDirs[dirIndex];
            //NOTE(ollie): Load materials first
            char *fileTypes[] = { "mtl" };
            easyAssetLoader_loadAndAddAssets(0, dir, fileTypes, arrayCount(fileTypes), ASSET_MATERIAL);
            
            //NOTE(ollie): Then load models
            fileTypes[0] = "obj";
            easyAssetLoader_loadAndAddAssets(&allModelsForEditor, dir, fileTypes, arrayCount(fileTypes), ASSET_MODEL);
            
        }
#endif

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
        
    Entity *player = 0;
    if(GAME_STATE_TO_LOAD != MY_GAME_MODE_EDIT_LEVEL) {
        player = myGame_beginRound(&gameVariables, entityManager, true, 0, &camera);
    } else {
        myGame_resetGameVariables(&gameVariables, &camera);
        player = initPlayer(entityManager, &gameVariables, findTextureAsset("cup_empty.png"), findTextureAsset("cup_half_full.png"));    
    }
    
    MyGameState *gameState = myGame_initGameState(entityManager, &camera); 

////////////////////////////////////////////////////////////////////      


///////////////////////************ Setup Audio Sound tracks *************////////////////////

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

        EasyProfile_ProfilerDrawState *profilerState = EasyProfiler_initProfiler(); 


///////////************************/////////////////
        while(running) {

            easyOS_processKeyStates(&keyStates, resolution, &screenDim, &running, !hasBlackBars);
            easyOS_beginFrame(resolution, &appInfo);
            
            beginRenderGroupForFrame(globalRenderGroup);

            clearBufferAndBind(appInfo.frameBackBufferId, COLOR_BLACK, FRAMEBUFFER_COLOR, 0);
            clearBufferAndBind(mainFrameBuffer.bufferId, COLOR_WHITE, mainFrameBuffer.flags, globalRenderGroup);
            
            renderEnableDepthTest(globalRenderGroup);
            renderEnableCulling(globalRenderGroup);
            setBlendFuncType(globalRenderGroup, BLEND_FUNC_STANDARD_PREMULTIPLED_ALPHA);
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

            camMove = EASY_CAMERA_MOVE_NULL;

            // EasyRay r = {};
            // r.origin = camera.pos;
            // r.direction = EasyCamera_getZAxis(&camera);

            // EasyPlane p = {};
            // p.normal = v3(0, 0, -1);

            // float tAt = 0;

            // if(!isOn(&gameVariables.lerpTimer)) {
            //     bool didHit = easyMath_castRayAgainstPlane(r, p, &gameVariables.centerPos, &tAt); //set the center pos
            //     assert(didHit && tAt >= 0); //should always be looking towards the center 
            // }
            // printf("point that got hit: %f %f %f \n", gameVariables.centerPos.x, gameVariables.centerPos.y, gameVariables.centerPos.z);


            if(gameState->currentGameMode == MY_GAME_MODE_EDIT_LEVEL) {

                V2 movePower = v2(0, 0);
                float power = 10;
                ///////////////////////************* Move the center position ************////////////////////
                if(isDown(keyStates.gameButtons, BUTTON_RIGHT)) {
                    movePower.x = power;
                }
                if(isDown(keyStates.gameButtons, BUTTON_LEFT)) {
                    movePower.x = -power;
                }
                if(isDown(keyStates.gameButtons, BUTTON_UP)) {
                    movePower.y = power;
                }
                if(isDown(keyStates.gameButtons, BUTTON_DOWN)) {
                    movePower.y = -power;
                }

                V3 forwardAxis = EasyCamera_getZAxis(&camera);
                //NOTE(ollie): Slam the z axis since we don't want to move in that axis 
                forwardAxis.z = 0; 
                //NOTE(ollie): Normalize the axis so all acceleration is equal strength (i.e. doesn;t matter the angle we're looking)
                forwardAxis = normalizeV3(forwardAxis);

                //NOTE(ollie): Up axis is the negative z-axis
                V3 upAxis = v3(0, 0, -1); 

                //NOTE(ollie): Get perindicular axis
                V3 rightAxis = v3_crossProduct(upAxis, forwardAxis);
                //NOTE(ollie): Slam the z axis again
                rightAxis.z = 0;
                rightAxis = normalizeV3(rightAxis);
                assert(getLengthV3(rightAxis) > 0.0f);

                V3 cameraMoveDirection = v3_plus(v3_scale(appInfo.dt*movePower.y, forwardAxis), v3_scale(appInfo.dt*movePower.x, rightAxis));
                
                //NOTE(ollie): Set the target center position
                gameVariables.targetCenterPos = v3_plus(gameVariables.targetCenterPos, cameraMoveDirection);

                printf("target center pos: %f %f %f \n", gameVariables.targetCenterPos.x, gameVariables.targetCenterPos.y, gameVariables.targetCenterPos.z);

                //NOTE(ollie): Update the center position
                gameVariables.centerPos = lerpV3(gameVariables.centerPos, 4.0f*appInfo.dt, gameVariables.targetCenterPos);

                if(!isOn(&gameVariables.lerpTimer)) {
                    camera.pos = myGame_getCameraGlobalPosition(&gameVariables, gameVariables.cameraDistance);
                }
                printf("center pos: %f %f %f \n", gameVariables.centerPos.x, gameVariables.centerPos.y, gameVariables.centerPos.z);

                ///////////////////////*********** Altitude slider **************////////////////////
                float sliderSize = 0.05f*resolution.x;
                float sliderMin = 0.2f*resolution.y;
                float sliderMax = 0.8f*resolution.y;

                float sliderScreenPos = lerp(sliderMin, gameState->altitudeSliderAt, sliderMax);

                V4 altitudeColor = COLOR_AQUA;
                Rect2f altitudeSlider = rect2fMinDim(0, sliderScreenPos, sliderSize, sliderSize);
                if(inBounds(keyStates.mouseP_left_up, altitudeSlider, BOUNDS_RECT)) {
                    altitudeColor = COLOR_YELLOW;
                    if(wasPressed(keyStates.gameButtons, BUTTON_LEFT_MOUSE)) {
                        gameState->holdingAltitudeSlider = true; 
                    }
                }

                if(gameState->holdingAltitudeSlider) {
                    altitudeColor = COLOR_BLUE; 

                    //NOTE(ollie): Work out slider's new position
                    float targetPos = lerp(sliderScreenPos, 0.25f, keyStates.mouseP_left_up.y);
                    gameState->altitudeSliderAt = (targetPos - sliderMin) / (sliderMax - sliderMin);

                    //NOTE(ollie): Clamp the slider
                    if(gameState->altitudeSliderAt > 1.0f) {
                        gameState->altitudeSliderAt = 1.0f;
                    }
                    if(gameState->altitudeSliderAt < 0.0f) {
                        gameState->altitudeSliderAt = 0.0f;
                    }

                    //NOTE(ollie): Calculate new altitude and set it
                    gameVariables.angleDegreesAltitude = lerp(1, gameState->altitudeSliderAt, 89.0f);

                    if(!isOn(&gameVariables.lerpTimer)) {
                        camera.pos = myGame_getCameraGlobalPosition(&gameVariables, gameVariables.cameraDistance);
                        camera.orientation = myGame_getCameraOrientation(camera.pos, gameVariables.centerPos);
                    }


                }

               if(wasReleased(keyStates.gameButtons, BUTTON_LEFT_MOUSE)) {
                   gameState->holdingAltitudeSlider = false;
                   altitudeColor = COLOR_AQUA;
               }


               //NOTE(ollie): recalculate quad since we change it's position
               altitudeSlider = rect2fMinDim(0, lerp(sliderMin, gameState->altitudeSliderAt, sliderMax), sliderSize, sliderSize);
              
               //NOTE(ollie): Draw the quad
               renderDrawRect(altitudeSlider, NEAR_CLIP_PLANE + 0.01f, altitudeColor, 0, mat4(), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));                    

               ////////////////////////////////////////////////////////////////////

                if(wasPressed(keyStates.gameButtons, BUTTON_BOARD_LEFT)) {
                    gameVariables.startPos = myGame_getCameraPosition(gameVariables.angleDegreesAltitude, gameVariables.angleType);
                    int value = (int)gameVariables.angleType - 1;
                    if(value < 0) {
                        value = (int)MY_VIEW_ANGLE_LEFT;
                    }
                    gameVariables.angleType = (MyGameState_ViewAngle)value;
                    ////////////////////////////////////////////////////////////////////
                    turnTimerOn(&gameVariables.lerpTimer);
                }

                if(wasPressed(keyStates.gameButtons, BUTTON_BOARD_RIGHT)) {
                    gameVariables.startPos = myGame_getCameraPosition(gameVariables.angleDegreesAltitude, gameVariables.angleType);

                    int value = (int)gameVariables.angleType + 1;
                    if(value > (int)MY_VIEW_ANGLE_LEFT) {
                        value = 0;
                    }
                    gameVariables.angleType = (MyGameState_ViewAngle)value;
                    ////////////////////////////////////////////////////////////////////

                    
                    turnTimerOn(&gameVariables.lerpTimer);
                }

                if(isOn(&gameVariables.lerpTimer)) {
                    TimerReturnInfo timerInfo = updateTimer(&gameVariables.lerpTimer, appInfo.dt);    

                    float canVal = timerInfo.canonicalVal;

                    V3 targetPos = myGame_getCameraPosition(gameVariables.angleDegreesAltitude, gameVariables.angleType);
                    V3 relPos = v3_scale(gameVariables.cameraDistance, normalizeV3(smoothStep01V3(gameVariables.startPos, canVal, targetPos)));
                    camera.pos = v3_plus(relPos, gameVariables.centerPos);
                    camera.orientation = myGame_getCameraOrientation(camera.pos, gameVariables.centerPos);

                    if(timerInfo.finished) {
                        turnTimerOff(&gameVariables.lerpTimer);
                    }
                }
                  
            }
             
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
            if((gameState->currentGameMode == MY_GAME_MODE_PLAY || gameState->currentGameMode == MY_GAME_MODE_EDIT_LEVEL)) updateFlags |= MY_ENTITIES_UPDATE;
            
            if(gameState->currentGameMode == MY_GAME_MODE_PLAY ||
                gameState->currentGameMode == MY_GAME_MODE_PAUSE ||
                gameState->currentGameMode == MY_GAME_MODE_SCORE ||
                gameState->currentGameMode == MY_GAME_MODE_END_ROUND ||
                gameState->currentGameMode == MY_GAME_MODE_INSTRUCTION_CARD ||
                gameState->currentGameMode == MY_GAME_MODE_EDIT_LEVEL) {
                updateFlags |= MY_ENTITIES_RENDER;
            }

            if(updateFlags & MY_ENTITIES_RENDER) {

                ///////////////////////********* Drawing the background ****************////////////////////
                Texture *bgTexture = findTextureAsset("africa.png");
                
                float aspectRatio = (float)bgTexture->height / (float)bgTexture->width;
                float xWidth = resolution.x;
                float xHeight = xWidth*aspectRatio;

                // renderTextureCentreDim(bgTexture, v3(0, 0, 10), v2(xWidth, xHeight), COLOR_WHITE, 0, mat4(), mat4(),  OrthoMatrixToScreen(resolution.x, resolution.y));                
                // drawRenderGroup(globalRenderGroup, RENDER_DRAW_DEFAULT);
                // renderClearDepthBuffer(toneMappedBuffer.bufferId);

                ////////////////////////////////////////////////////////////////////

                if(updateFlags & MY_ENTITIES_UPDATE) {
                    if(!DEBUG_global_IsFlyMode) {
                        if(!isOn(&gameVariables.lerpTimer)) {
                            //NOTE(ollie): Update the camera distance 
                            gameVariables.cameraDistance = lerp(gameVariables.cameraDistance, appInfo.dt*1, gameVariables.cameraTargetPos); 
                            camera.pos = myGame_getCameraGlobalPosition(&gameVariables, gameVariables.cameraDistance);
                        }
                    }
                    updateEntitiesPrePhysics(entityManager, &keyStates, &gameVariables, appInfo.dt);
                    EasyPhysics_UpdateWorld(&entityManager->physicsWorld, appInfo.dt);
                }

                

                ////////////////////////////////////////////////////////////////////

                updateEntities(entityManager, gameState, &gameVariables, &keyStates, globalRenderGroup, viewMatrix, perspectiveMatrix, appInfo.dt, updateFlags);

                cleanUpEntities(entityManager, &gameVariables);

                bool shouldDrawHUD = true;

                if(gameState->currentGameMode == MY_GAME_MODE_EDIT_LEVEL) shouldDrawHUD = false;

                if(shouldDrawHUD) {
                    if(!player) {
                        player = MyEntity_findEntityByName("Player");
                    }
                    
                    assert(player);
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

                        
                        if(liveIndex < gameVariables.liveTimerCount) {
                            Timer *liveTimer = &gameVariables.liveTimers[liveIndex]; 
                            
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
                }

                if(gameState->currentGameMode != MY_GAME_MODE_PLAY && gameState->currentGameMode != MY_GAME_MODE_EDIT_LEVEL) {
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
                    if(wasPressed(keyStates.gameButtons, BUTTON_ENTER) && !easyConsole_isConsoleOpen(&console)) {

                        MyTransitionData *data = getTransitionData(gameState, MY_GAME_MODE_PLAY, &camera);
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

                    if(wasPressed(keyStates.gameButtons, BUTTON_ENTER) && !easyConsole_isConsoleOpen(&console)) {
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
                        case GAME_INSTRUCTION_UNDERPANTS: {
                            imageToDisplay = findTextureAsset("underwear.png");
                            stringToDisplay = "You can find clean underpants on your journey. This will give you another life.";
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

                    if(wasPressed(keyStates.gameButtons, BUTTON_ENTER) && !EasyTransition_InTransition(transitionState) && !easyConsole_isConsoleOpen(&console)) {
                        MyTransitionData *data = getTransitionData(gameState, MY_GAME_MODE_PLAY, &camera);
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
                case MY_GAME_MODE_EDIT_LEVEL: {
                    ///////////////////////*********** Draw the HUD **************////////////////////

                    #if 0
                    //NOTE(ollie): Choose Grid Size
                    float xAt = 0.1f*resolution.x;
                    float startXAt = xAt; 

                    float yAt = 0.1f*resolution.y;

                    float imageSize = 0.05*resolution.x;
                    float imageMargin = 0.5f*imageSize;

                    //SAVE STATE
                    EasyRender_ShaderAndTransformState state = easyRender_saveShaderAndTransformState(globalRenderGroup);
                    
                    renderSetShader(globalRenderGroup, &model3dTo2dImageProgram);
                    static float angle = 0;
                    angle += appInfo.dt;
                    
                    setViewTransform(globalRenderGroup, easy3d_lookAt(v3(0, 0, -7), v3(0, 0, 0), v3(0, 1, 0)));
                    float aspectRatio = 1;
                    setProjectionTransform(globalRenderGroup, projectionMatrixFOV(60, aspectRatio));
                    Matrix4 orthoMat = OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y);

                    int maxCollums = 6;

                    InfiniteAlloc rectModels = initInfinteAlloc(MyGameState_Rect2fModelInfo);
                    bool loop = true;
                    int modelIndex = 0;
                    while(loop) {
                        for(int collumIndex = 0; collumIndex < maxCollums && loop; ++collumIndex) {
                            

                            MyGameState_Rect2fModelInfo data = {};
                            data.r = rect2fMinDim(xAt, yAt, imageSize, imageSize);
                            assert(modelIndex < allModelsForEditor.count);
                            data.m = &allModelsForEditor.array[modelIndex++];
                            Rect2f imageR = rect2fMinDim(xAt, yAt, imageSize, imageSize);   
                            

                            addElementInfinteAlloc_notPointer(&rectModels, data);

                            V4 color = COLOR_GREY;
                            if(inBounds(keyStates.mouseP_left_up, imageR, BOUNDS_RECT)) {
                                color = COLOR_YELLOW;
                                if(wasPressed(keyStates.gameButtons, BUTTON_LEFT_MOUSE)) {
                                    gameState->modelSelected = modelIndex - 1; //minus one since we already incremented it
                                }
                            }

                            renderDrawRect(imageR, 2.0f, color, 0, mat4(), orthoMat);
                            
                            if(modelIndex >= allModelsForEditor.count) {
                                assert(modelIndex == allModelsForEditor.count);

                                loop = false; //no more models to show
                            }
                            xAt += imageSize + imageMargin;
                        }
                        yAt += imageSize + imageMargin;

                        xAt = startXAt;
                    }

                    for(int rectIndex = 0; rectIndex < rectModels.count; ++rectIndex) {
                        MyGameState_Rect2fModelInfo *imageR = getElementFromAlloc(&rectModels, rectIndex, MyGameState_Rect2fModelInfo);

                        EasyAssetLoader_AssetInfo *modelInfo = imageR->m;
                        EasyModel *model = modelInfo->model;
                        V3 boundsOffset = v3_scale(modelInfo->scale, easyMath_getRect3fBoundsOffset(model->bounds));

                        //NOTE(ollie): Will have to actually scale the models
                        Matrix4 scale = Matrix4_scale_uniformly(mat4(), modelInfo->scale);
                        Matrix4 rotation = mat4_axisAngle(v3(0, 1, 0), angle);
                        Matrix4 translation = Matrix4_translate(mat4(),  boundsOffset);
                        setModelTransform(globalRenderGroup, Mat4Mult(rotation, Mat4Mult(translation, scale)));
                        
                        easyRender_renderModelAs2dImage(globalRenderGroup, imageR->r, NEAR_CLIP_PLANE, model, COLOR_WHITE, orthoMat);

                    }
                    

                     //RESTORE STATE
                     easyRender_restoreShaderAndTransformState(globalRenderGroup, &state);

                     ////////////////////////////////////////////////////////////////////
#endif
                    Texture *gridImage = findTextureAsset("grid.png");
                    V2 dim = v2(0.05f*resolution.x, 0);
                    dim.y = dim.x*gridImage->aspectRatio_h_over_w;

                    renderTextureCentreDim(gridImage, v3(0.4f*resolution.x, 0.5f*dim.y, NEAR_CLIP_PLANE), dim, COLOR_WHITE, 0, mat4TopLeftToBottomLeft(resolution.y), mat4(),  OrthoMatrixToScreen(resolution.x, resolution.y));

                    ////////////////////////////////////////////////////////////////////

                    float zAtInViewSpace = -1 * camera.pos.z;

                    if(isDown(keyStates.gameButtons, BUTTON_R)) {
                        DEBUG_global_CamNoRotate = false;
                    } else {
                        DEBUG_global_CamNoRotate = true;
                    }

                    easyEditor_startDockedWindow(editor, "Editor Tools", EASY_EDITOR_DOCK_BOTTOM_RIGHT);
                    
                    // int modelSelected = easyEditor_pushList(editor, "Model: ", modelsLoaded, modelLoadedCount);

                    //TODO(ollie): Get this to be more reliable!! it crashes right now
                    // easyConsole_addToStream(&console, str);
                    if(easyEditor_pushButton(editor, "Save Room")) {
                        easyFlashText_addText(&globalFlashTextManager, "SAVED");
                        myLevels_saveLevel(gameState->currentLevelEditing, entityManager);
                    }
                    
                    easyEditor_endWindow(editor); //might not actually need this

                    if(wasPressed(keyStates.gameButtons, BUTTON_F6)) {
                        gameState->currentGameMode = MY_GAME_MODE_PLAY;
                        easyEntity_endRound(entityManager);
                        cleanUpEntities(entityManager, &gameVariables);
                        myGame_beginRound(&gameVariables, entityManager, false, player, &camera);

                        //NOTE(ollie): Revert back to game settings
                        DEBUG_global_IsFlyMode = false;
                        DEBUG_global_CameraMoveXY = false;
                    }

                    if(wasPressed(keyStates.gameButtons, BUTTON_LEFT_MOUSE) && !editor->isHovering) {

                        if(isDown(keyStates.gameButtons, BUTTON_SHIFT)) {
                            //NOTE(ollie): Create a new model
                            assert(gameState->modelSelected < allModelsForEditor.count);
                            EasyModel *model = allModelsForEditor.array[gameState->modelSelected].model;

                            if(model) {
                                gameState->holdingEntity = true;
                                //NOTE(ollie): It should exist since we are pulling 
                                //             from an array that has the loaded models in It
                                Matrix4 cameraToWorld = easy3d_getViewToWorld(&camera);
                                V3 worldP = screenSpaceToWorldSpace(perspectiveMatrix, keyStates.mouseP_left_up, resolution, zAtInViewSpace, cameraToWorld);
                                Entity *newEntity = initScenery1x1(entityManager, model->name, model, worldP, 0);
                                gameState->hotEntity = newEntity;
                            }
                        } else {
                           

                     
                        }
                    } 

                    if(!gameState->holdingEntity) {
                       
                        //NOTE(ollie): Try grabbing one
                        //cast ray against entities
                        EasyPhysics_RayCastAABB3f_Info rayInfo = {};
                        rayInfo.hitT = INFINITY;
                        rayInfo.didHit = false;

                        Entity *entityHit = 0;

                        //NOTE(ollie): Build the ray
                        Matrix4 cameraToWorld = easy3d_getViewToWorld(&camera);
                        

                        for(int i = 0; i < entityManager->entities.count; ++i) {
                            Entity *e = (Entity *)getElement(&entityManager->entities, i);
                            if(e && e->active) { //can be null

                                if(e->type == ENTITY_SCENERY && e->model) {
                                    e->colorTint = COLOR_WHITE;

                                    V3 worldP = screenSpaceToWorldSpace(perspectiveMatrix, keyStates.mouseP_left_up, resolution, zAtInViewSpace, cameraToWorld);
                                    V3 rayDirection = v3_minus(worldP, camera.pos);

                                    EasyPhysics_RayCastAABB3f_Info newRayInfo = EasyPhysics_CastRayAgainstAABB3f(easyTransform_getWorldRotation(&e->T), easyTransform_getWorldPos(&e->T), easyTransform_getWorldScale(&e->T), e->model->bounds, rayDirection, camera.pos);
                                    
                                    if(newRayInfo.didHit && newRayInfo.hitT < rayInfo.hitT && newRayInfo.hitT > 0.0f) {
                                        rayInfo = newRayInfo;
                                        entityHit = e;

                                    }
                                }
                            }
                        }

                        if(rayInfo.didHit) {
                            if(!editor->isHovering) {
                                entityHit->colorTint = COLOR_BLUE;
                                if(wasPressed(keyStates.gameButtons, BUTTON_LEFT_MOUSE)) {
                                    gameState->holdingEntity = true;
                                    gameState->hotEntity = entityHit;    
                                }
                            }
                        }   
                    }

                    if(wasReleased(keyStates.gameButtons, BUTTON_LEFT_MOUSE)) {
                        gameState->holdingEntity = false;
                    }

                    if(gameState->hotEntity) {
                        Entity *hotEntity = (Entity *)gameState->hotEntity;
                        EasyTransform *T = &hotEntity->T;

                        if(gameState->holdingEntity) {
                            //NOTE(ollie): Set the entity position 

                            ///////////////////////*********** Working out the z from camera **************////////////////////
                            V3 entPos = easyTransform_getWorldPos(T);
                            V3 zAxis = normalizeV3(easyMath_getZAxis(quaternionToMatrix(camera.orientation)));
                            float zFromCamera = dotV3(v3_minus(entPos, camera.pos), zAxis);
                            ////////////////////////////////////////////////////////////////////

                            V3 worldP = screenSpaceToWorldSpace(perspectiveMatrix, keyStates.mouseP_left_up, resolution, zFromCamera, easy3d_getViewToWorld(&camera));
                            T->pos = worldP;
                        }

                        easyEditor_startDockedWindow(editor, "Entity", EASY_EDITOR_DOCK_BOTTOM_LEFT);

                        easyEditor_pushFloat3(editor, "Position:", &T->pos.x, &T->pos.y, &T->pos.z);

                        ////////////////////////////////////////////////////////////////////
                        //NOTE(ollie): Rotation with euler angles

                        V3 tempEulerAngles = easyMath_QuaternionToEulerAngles(T->Q);


                        //NOTE(ollie): Swap to degrees 
                        tempEulerAngles.x = easyMath_radiansToDegrees(tempEulerAngles.x);
                        tempEulerAngles.y = easyMath_radiansToDegrees(tempEulerAngles.y);
                        tempEulerAngles.z = easyMath_radiansToDegrees(tempEulerAngles.z);

                        
                        easyEditor_pushFloat3(editor, "Rotation:", &tempEulerAngles.x, &tempEulerAngles.y, &tempEulerAngles.z);

                        //NOTE(ollie): Convert back to radians
                        tempEulerAngles.x = easyMath_degreesToRadians(tempEulerAngles.x);
                        tempEulerAngles.y = easyMath_degreesToRadians(tempEulerAngles.y);
                        tempEulerAngles.z = easyMath_degreesToRadians(tempEulerAngles.z);

                        T->Q = eulerAnglesToQuaternion(tempEulerAngles.y, tempEulerAngles.x, tempEulerAngles.z);

                        ////////////////////////////////////////////////////////////////////

                        easyEditor_pushFloat1(editor, "Scale:", &T->scale.x);
                        T->scale.y = T->scale.z = T->scale.x;

                        easyEditor_pushColor(editor, "Color: ", &hotEntity->colorTint);
                        
                        if(easyEditor_pushButton(editor, "Delete")) {
                            easyFlashText_addText(&globalFlashTextManager, "Deleted");
                            MyEntity_MarkForDeletion(&hotEntity->T);
                            gameState->hotEntity = 0;
                            gameState->holdingEntity = false;
                        }

                        if(easyEditor_pushButton(editor, "Release entity")) {
                            gameState->hotEntity = 0;
                            gameState->holdingEntity = false;
                        }
                        
                        easyEditor_endWindow(editor); //might not actually need this

                    }
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
                    } else if(stringsMatchNullN("edit", token.at, token.size)) {
                        token = easyConsole_seeNextToken(&console);
                        if(token.type == TOKEN_INTEGER) {
                            token = easyConsole_getNextToken(&console);
                            int levelToLoad = token.intVal;

                            gameState->currentGameMode = MY_GAME_MODE_EDIT_LEVEL;
                            easyEntity_endRound(entityManager);
                            cleanUpEntities(entityManager, &gameVariables);

                            myLevels_loadLevel(levelToLoad, entityManager, v3(0, 0, 0));
                            easyConsole_addToStream(&console, "loaded level");

                            DEBUG_global_IsFlyMode = true;
                            DEBUG_global_CameraMoveXY = true;

                        } else {
                            easyConsole_addToStream(&console, "must pass a number");
                        }
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
                easyEditor_startWindow(editor, "Lister Panel", 100, 100);
                {
                    easyEditor_pushSlider(editor, "Level move speed: ", &gameVariables.roomSpeed, -3.0f, -0.5f);
                    easyEditor_pushSlider(editor, "Player move speed: ", &gameVariables.playerMoveSpeed, 0.1f, 1.0f);

                    easyEditor_pushFloat1(editor, "min player move speed: ", &gameVariables.minPlayerMoveSpeed);
                    easyEditor_pushFloat1(editor, "max player move speed: ", &gameVariables.maxPlayerMoveSpeed);
                    easyEditor_pushSlider(editor, "Scale: ", &globalTileScale, 0.1f, 1.0f);

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
                

                
            }
            easyEditor_endEditorForFrame(editor);

            easyFlashText_updateManager(&globalFlashTextManager, globalRenderGroup, appInfo.dt);

//////////////////////////////////// DRAW THE UI ITEMS ///////

            drawRenderGroup(globalRenderGroup, RENDER_DRAW_SORT);


//////////////////////////////////////////////////////////////////////////////////////////////

            //NOTE(ollie): Make sure the transition is on top
            renderClearDepthBuffer(toneMappedBuffer.bufferId);

            EasyTransition_updateTransitions(transitionState, resolution, appInfo.dt);
            
            drawRenderGroup(globalRenderGroup, RENDER_DRAW_DEFAULT);

            ///////////////////////********** Drawing the Profiler Graph ***************////////////////////
            renderClearDepthBuffer(toneMappedBuffer.bufferId);

            EasyProfile_DrawGraph(profilerState, resolution, &keyStates, appInfo.screenRelativeSize, appInfo.dt);
            
            drawRenderGroup(globalRenderGroup, RENDER_DRAW_DEFAULT);

////////////////////////////////////////////////////////////////////////////

            easyOS_updateHotKeys(&keyStates);
            easyOS_endFrame(resolution, screenDim, toneMappedBuffer.bufferId, &appInfo, hasBlackBars);
            
            DEBUG_TIME_BLOCK_FOR_FRAME_END(beginFrame, wasPressed(keyStates.gameButtons, BUTTON_F4))
            DEBUG_TIME_BLOCK_FOR_FRAME_START(beginFrame, "Per frame")
            easyOS_endKeyState(&keyStates);
        }
        easyOS_endProgram(&appInfo);
    }
    return(0);
}
    