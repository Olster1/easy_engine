#include <GL/gl3w.h>

#include "../SDL2/sdl.h"
#include "../SDL2/SDL_syswm.h"

#include "defines.h"
#include "easy_headers.h"

#include "myEntity.h"

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

    if(argc > 1) {
        for(int i = 0; i < argc; i++) {
            if(cmpStrNull("shaders", args[i])) {
                globalDebugWriteShaders = true;    
            }
            
        }
    }

    V2 screenDim = v2(800, 800); //init in create app function
    V2 resolution = v2(0, 0);


    // screenDim = resolution;
    bool fullscreen = false;
    OSAppInfo appInfo = easyOS_createApp("Easy Engine", &screenDim, fullscreen);

    if(appInfo.valid) {
        
        easyOS_setupApp(&appInfo, &resolution, RESOURCE_PATH_EXTENSION);
        
        loadAndAddImagesToAssets("img/");
        
        ////INIT FONTS
        char *fontName = concatInArena(globalExeBasePath, "/fonts/Khand-Regular.ttf", &globalPerFrameArena);
        Font mainFont = initFont(fontName, 88);

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

///////////************************/////////////////


        EasyCamera camera;
        easy3d_initCamera(&camera, v3(0, 0, -2));

        globalRenderGroup->skybox = initSkyBox();

        easy3d_loadMtl("models/terrain/fern.mtl");
        EasyModel fern = {};
        easy3d_loadObj("models/terrain/fern.obj", &fern);

        easy3d_loadMtl("models/terrain/grass.mtl");
        EasyModel grass = {};
        easy3d_loadObj("models/terrain/grass.obj", &grass);

        EasyTerrain *terrain = initTerrain(fern, grass);

        EasyMaterial crateMaterial = easyCreateMaterial(findTextureAsset("crate.png"), 0, findTextureAsset("crate_specular.png"), 32);
        EasyMaterial emptyMaterial = easyCreateMaterial(findTextureAsset("grey_texture.jpg"), 0, findTextureAsset("grey_texture.jpg"), 32);
        EasyMaterial flowerMaterial = easyCreateMaterial(findTextureAsset("flower.png"), 0, findTextureAsset("grey_texture.jpg"), 32);


        EasyTransform sunTransform;
        easyTransform_initTransform(&sunTransform, v3(0, 10, 0));

        EasyLight *light = easy_makeLight(&sunTransform, EASY_LIGHT_DIRECTIONAL, 1.0f, v3(1, 1, 0));//easy_makeLight(v4(0, -1, 0, 0), v3(1, 1, 1), v3(1, 1, 1), v3(1, 1, 1));
        easy_addLight(globalRenderGroup, light);
        

///////////////////////*************************////////////////////

//NOTE(ollie): Initing the Entity Manager

    MyEntityManager *entityManager = initEntityManager();

    ///////////////////////*********** Load the player model **************////////////////////
    
    easy3d_loadMtl("models/player.mtl");

    //TODO(ollie): Change this to add models to the asset catalog
    EasyModel playerModel = {};
    easy3d_loadObj("models/player.obj", &playerModel);

    ////////////////////////////////////////////////////////////////////
    
    initPlayer(entityManager, &playerModel);

////////////////////////////////////////////////////////////////////        

////////////*** Variables *****//////
        float cameraMovePower = 2000.0f;
        bool debug_IsFlyMode = true;
        float exposureTerm = 1.0f;
        int toneMapId = 0;
        bool controllingPlayer = true;

        EasyTransform T;
        easyTransform_initTransform(&T, v3(0, 0, 0));


///////////************************/////////////////
        V4 color = COLOR_RED;
        while(running) {
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
            if(debug_IsFlyMode && !easyConsole_isOpen(&console) && !easyEditor_isInteracting(editor) && !inEditor) {
                camMove = (EasyCamera_MoveType)(EASY_CAMERA_MOVE | EASY_CAMERA_ROTATE | EASY_CAMERA_ZOOM);
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
            renderModel(globalRenderGroup, &grass, COLOR_WHITE);

            setModelTransform(globalRenderGroup, Matrix4_translate(mat4(), v3(1, 1, 1)));
            renderModel(globalRenderGroup, &fern, color);

            // easyTerrain_renderTerrain(terrain, globalRenderGroup, &emptyMaterial);

            renderEnableCulling(globalRenderGroup);

///////////////////////************ Update entities here *************////////////////////

            updateEntitiesPrePhysics(entityManager, &keyStates, appInfo.dt);
            
            EasyPhysics_UpdateWorld(&entityManager->physicsWorld, appInfo.dt);

            updateEntities(entityManager, &keyStates, globalRenderGroup, viewMatrix, perspectiveMatrix);

            cleanUpEntities(entityManager);

////////////////////////////////////////////////////////////////////

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
                    } else if(stringsMatchNullN("fly", token.at, token.size)) {
                        debug_IsFlyMode = !debug_IsFlyMode;
                    } else if(stringsMatchNullN("bounds", token.at, token.size)) {
                        DEBUG_drawBounds = !DEBUG_drawBounds;
                    } else if(stringsMatchNullN("bloom", token.at, token.size)) {
                        toneMapId = (toneMapId) ? 0 : 1;
                    } else {
                        easyConsole_parseDefault(&console);
                    }
                } else {
                    easyConsole_parseDefault(&console);
                }
            }
//////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////// DRAWING & UPDATE IN GAME EDITOR /////////////////////////////////                    
                    if(inEditor) {
                        easyEditor_startWindow(editor, "Lister Panel");
                        
                        static V3 v = {};
                        easyEditor_pushFloat3(editor, "Position:", &T.pos.x, &T.pos.y, &T.pos.z);
                        easyEditor_pushFloat3(editor, "Rotation:", &T.Q.i, &T.Q.j, &T.Q.k);
                        easyEditor_pushFloat3(editor, "Scale:", &T.scale.x, &T.scale.y, &T.scale.z);

                        easyEditor_pushFloat1(editor, "Exposure:", &exposureTerm);
           
                        easyEditor_pushColor(editor, "Color: ", &color);
                        static float a = 1.0f;
                        easyEditor_pushSlider(editor, "Some Value: ", &a, 0, 10);

                        static bool saveLevel = false;
                        if(easyEditor_pushButton(editor, "Save Level")) {
                            saveLevel = !saveLevel;
                             easyFlashText_addText(&globalFlashTextManager, "SAVED!");
                        }

                        if(saveLevel) {
                            easyEditor_pushFloat1(editor, "", &v.x);
                            easyEditor_pushFloat2(editor, "", &v.x, &v.y);
                            easyEditor_pushFloat3(editor, "", &v.x, &v.y, &v.z);
                        }

                        easyEditor_endWindow(editor); //might not actuall need this
                        

                        easyEditor_endEditorForFrame(editor);
                    }

                    easyFlashText_updateManager(&globalFlashTextManager, globalRenderGroup, appInfo.dt);

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// DRAW THE UI ITEMS ///////

                    drawRenderGroup(globalRenderGroup, RENDER_DRAW_SORT);

////////////////////////////////////////////////////////////////////////////

            easyOS_endFrame(resolution, screenDim, toneMappedBuffer.bufferId, &appInfo, hasBlackBars);
            easyOS_endKeyState(&keyStates);
        }
        easyOS_endProgram(&appInfo);
    }
    return(0);
}
    