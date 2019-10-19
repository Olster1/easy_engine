#include <GL/gl3w.h>

#include "../SDL2/sdl.h"
#include "../SDL2/SDL_syswm.h"

#include "defines.h"
#include "easy_headers.h"

// #if __APPLE__
// #include <unistd.h>
// #endif

/*

*/

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
    assert(appInfo.valid);
    if(appInfo.valid) {
        
        easyOS_setupApp(&appInfo, &resolution, RESOURCE_PATH_EXTENSION);
        
        loadAndAddImagesToAssets("img/");
        
        ////INIT FONTS
        char *fontName = concatInArena(globalExeBasePath, "/fonts/Khand-Regular.ttf", &globalPerFrameArena);
        Font mainFont = initFont(fontName, 88);

        char *fontName1 = concatInArena(globalExeBasePath, "/fonts/UbuntuMono-Regular.ttf", &globalPerFrameArena);
        globalDebugFont = initFont(fontName1, 32);
        ///
        FrameBuffer mainFrameBuffer = createFrameBuffer(resolution.x, resolution.y, FRAMEBUFFER_COLOR | FRAMEBUFFER_DEPTH | FRAMEBUFFER_STENCIL | FRAMEBUFFER_HDR, 2);
        FrameBuffer toneMappedBuffer = createFrameBuffer(resolution.x, resolution.y, 0, 1);
        
        bool hasBlackBars = true;
        bool running = true;
        AppKeyStates keyStates = {};

        // globalRenderGroup->whiteTexture = findTextureAsset("flower.png");

        //SETUP
        EasyMaterial crateMaterial = easyCreateMaterial(findTextureAsset("crate.png"), 0, findTextureAsset("crate_specular.png"), 32);
        EasyMaterial emptyMaterial = easyCreateMaterial(findTextureAsset("grey_texture.jpg"), 0, findTextureAsset("grey_texture.jpg"), 32);
        EasyMaterial flowerMaterial = easyCreateMaterial(findTextureAsset("flower.png"), 0, findTextureAsset("grey_texture.jpg"), 32);
        // VaoHandle floorMesh = {};
        // generateHeightMap(&floorMesh, 100, 100);

        EasyCamera camera;
        easy3d_initCamera(&camera, v3(0, 0, -2));

        EasySkyBoxImages skyboxImages;
        skyboxImages.fileNames[0] = "skybox_images/right.jpg";
        skyboxImages.fileNames[1] = "skybox_images/left.jpg";
        skyboxImages.fileNames[2] = "skybox_images/top.jpg";
        skyboxImages.fileNames[3] = "skybox_images/bottom.jpg";
        skyboxImages.fileNames[4] = "skybox_images/front.jpg";
        skyboxImages.fileNames[5] = "skybox_images/back.jpg";
        
        EasySkyBox *skybox = easy_makeSkybox(&skyboxImages);

        globalRenderGroup->skybox = skybox;

        easy3d_loadMtl("models/low_poly_tree_model/tree_mtl.mtl");
        EasyModel treeModel = {};
        easy3d_loadObj("models/low_poly_tree_model/tree_obj.obj", &treeModel);

        easy3d_loadMtl("models/Carrot.mtl");
        EasyModel carrotModel = {};
        easy3d_loadObj("models/Carrot.obj", &carrotModel);

        easy3d_loadMtl("models/Palm_Tree.mtl");
        EasyModel palmTree = {};
        easy3d_loadObj("models/Palm_Tree.obj", &palmTree);

        easy3d_loadMtl("models/Poplar_Tree.mtl");
        EasyModel poplarTree = {};
        easy3d_loadObj("models/Poplar_Tree.obj", &poplarTree);

        easy3d_loadMtl("models/Fir_Tree.mtl");
        EasyModel firTree = {};
        easy3d_loadObj("models/Fir_Tree.obj", &firTree);

        easy3d_loadMtl("models/Oak_Tree.mtl");
        EasyModel oakTree = {};
        easy3d_loadObj("models/Oak_Tree.obj", &oakTree);

        easy3d_loadMtl("models/ship_dark.mtl");
        EasyModel ship = {};
        easy3d_loadObj("models/ship_dark.obj", &ship);

        easy3d_loadMtl("models/basicCharacter.mtl");
        EasyModel character = {};
        easy3d_loadObj("models/basicCharacter.obj", &character);

        easy3d_loadMtl("models/tower.mtl");
        EasyModel tower = {};
        easy3d_loadObj("models/tower.obj", &tower);

        easy3d_loadMtl("models/terrain/fern.mtl");
        EasyModel fern = {};
        easy3d_loadObj("models/terrain/fern.obj", &fern);

        easy3d_loadMtl("models/terrain/grass.mtl");
        EasyModel grass = {};
        easy3d_loadObj("models/terrain/grass.obj", &grass);

        easy3d_loadMtl("models/terrain/violet_tree.mtl");
        EasyModel vine = {};
        easy3d_loadObj("models/terrain/violet_tree.obj", &vine);

         bool controllingPlayer = true;

        //  easy3d_loadMtl(concatInArena(globalExeBasePath, "models/zombie/Zombie.mtl", &globalPerFrameArena));
        // EasyModel treeModel = {};
        // easy3d_loadObj(concatInArena(globalExeBasePath, "models/zombie/Zombie.obj", &globalPerFrameArena), &treeModel);

        Array_Dynamic gameObjects;
        initArray(&gameObjects, EasyRigidBody);

        V3 lowerHalfP = screenSpaceToWorldSpace(projectionMatrixFOV(camera.zoom, resolution.x/resolution.y), v2(0.5f*screenDim.x, 0.2f*screenDim.y), resolution, -10, mat4());
        EasyPhysics_AddRigidBody(&gameObjects, 0, 0, lowerHalfP);

        lowerHalfP = screenSpaceToWorldSpace(projectionMatrixFOV(camera.zoom, resolution.x/resolution.y), v2(0.5f*screenDim.x, 0.8f*screenDim.y), resolution, -10, mat4());
        EasyPhysics_AddRigidBody(&gameObjects, 0.1, 1, lowerHalfP);

        EasyTransform sunTransform;
        easyTransform_initTransform(&sunTransform, v3(0, 10, 0));

        EasyLight *light = easy_makeLight(&sunTransform, EASY_LIGHT_DIRECTIONAL, 1.0f, v3(1, 1, 0));//easy_makeLight(v4(0, -1, 0, 0), v3(1, 1, 1), v3(1, 1, 1), v3(1, 1, 1));
        easy_addLight(globalRenderGroup, light);
        float tAt = 0;
        float physicsTime = 0;

        EasyConsole console = {};
        easyConsole_initConsole(&console, BUTTON_TILDE);

        float x[20] = {};
        float y[20] = {};

        for(int r = 0; r < 20; ++r) {
            
            x[r] = randomBetween(-10, 10);
            y[r] = randomBetween(-10, 10);
        }

        float x1[20] = {};
        float y1[20] = {};

        for(int r = 0; r < 20; ++r) {
            
            x1[r] = randomBetween(-10, 10);
            y1[r] = randomBetween(-10, 10);
        }

        float cameraMovePower = 2000.0f;
        bool debug_IsFlyMode = true;

        RenderGroup postProcessRenderGroup = {};
        initRenderGroup(&postProcessRenderGroup);


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


        EasyEditor *editor = pushStruct(&globalLongTermArena, EasyEditor);
        easyEditor_initEditor(editor, globalRenderGroup, &globalDebugFont, OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y), resolution, appInfo.screenRelativeSize, &keyStates);

        EasyTransform T;
        easyTransform_initTransform(&T, v3(0, 0, 0));

        bool inEditor = false;


        while(running) {
            easyOS_processKeyStates(&keyStates, resolution, &screenDim, &running, !hasBlackBars);
            easyOS_beginFrame(resolution, &appInfo);
            
            beginRenderGroupForFrame(globalRenderGroup);

            clearBufferAndBind(appInfo.frameBackBufferId, COLOR_BLACK, FRAMEBUFFER_COLOR, 0);
            clearBufferAndBind(mainFrameBuffer.bufferId, COLOR_PINK, mainFrameBuffer.flags, globalRenderGroup);
            
            renderEnableDepthTest(globalRenderGroup);
            renderEnableCulling(globalRenderGroup);
            setBlendFuncType(globalRenderGroup, BLEND_FUNC_STANDARD);
            renderSetViewPort(0, 0, resolution.x, resolution.y);

            tAt += appInfo.dt;


            Matrix4 lightDir = easy3d_lookAt(v3(10*cos(0.707f), 10*sin(0.707f), 2), v3(0, 0, 0), v3(0, 1, 0));
           

            float zAt = -10;//-2*sin(tAt);
            // V3 eyepos = v3(2*cos(tAt), 0, zAt);
            // V3 eyepos = v3(0, 0, -10);
            // Matrix4 viewMatrix = easy3d_lookAt(eyepos, v3(0, 0, 0), v3(0, 1, 0));
            
            if(wasPressed(keyStates.gameButtons, BUTTON_F1)) {
                inEditor = !inEditor;
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
            globalRenderGroup->fov = camera.zoom;
            globalRenderGroup->aspectRatio = resolution.x/resolution.y;
            globalRenderGroup->cameraToWorldTransform = easy3d_getViewToWorld(&camera);


            Matrix4 identity = Matrix4_translate(mat4(), v3(2, 0, 2));//mat4_angle_aroundZ(tAt);\
    
            if(controllingPlayer) {

            } else {


            }
            //
            //
            // light->direction.xyz = normalizeV3(easyMath_getZAxis(easy3d_getViewToWorld(&camera)));//
            // // printf("%s %f %f %f\n", "color: ", treeModel.meshes[1]->material->defaultAmbient.x, treeModel.meshes[1]->material->defaultAmbient.y, treeModel.meshes[1]->material->defaultAmbient.z);
            // renderDrawRectCenterDim(v3(100, 100, 1), v2(100, 100), COLOR_BLACK, 0, mat4(), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));
            // renderTextureCentreDim(findTextureAsset("path.png"), v3(400, 400, 1), v2(400, 400), COLOR_WHITE, 0, mat4(), mat4(), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));
            ///Drawing here
            //outputText(&mainFont, 0.5f*resolution.x, 0.5f*resolution.y, 1, resolution, "hey there", rect2fMinMax(0, 0, 1000, 1000), COLOR_BLACK, 1, true, appInfo.screenRelativeSize);
            //What this is should be...size should be baked into the font. 
            // outputText(&mainFont, 0.5f*resolution.x, 0.5f*resolution.y, -1, "hey there");
            renderSetShader(globalRenderGroup, &phongProgram);
            setModelTransform(globalRenderGroup, identity);
            setViewTransform(globalRenderGroup, viewMatrix);
            setProjectionTransform(globalRenderGroup, perspectiveMatrix);

            renderEnableDepthTest(globalRenderGroup);
            // renderDrawCube(globalRenderGroup, &crateMaterial, COLOR_WHITE);
            // renderMesh(globalRenderGroup, &floorMesh, hexARGBTo01Color(0xFFCEFF74), &emptyMaterial);

            Matrix4 scaledIdentity = Matrix4_scale(identity, v3(1, 1, 1));
            setModelTransform(globalRenderGroup, scaledIdentity);
            // renderModel(globalRenderGroup, &treeModel, COLOR_WHITE);

            renderDisableCulling(globalRenderGroup);
                        // renderDrawBillBoardQuad(globalRenderGroup,  &flowerMaterial, COLOR_WHITE, v3(10, 10, 10), v3(0, 1, 0), v3(0, 0, 0), camera.pos);
            

            setModelTransform(globalRenderGroup, Matrix4_translate(scaledIdentity, v3(1, 0, 1)));
            // renderModel(globalRenderGroup, &vine, COLOR_WHITE);
            setModelTransform(globalRenderGroup, Matrix4_translate(scaledIdentity, v3(1, -1, 1)));
            renderModel(globalRenderGroup, &grass, COLOR_WHITE);
            setModelTransform(globalRenderGroup, Matrix4_translate(scaledIdentity, v3(1, 1, 1)));

            static float tAT = 0;
            tAT += 0.1f;
            Matrix4 m = mat4_axisAngle(v3(0, 1, 1), tAT);
            setModelTransform(globalRenderGroup,  easyTransform_getTransform(&T));
            renderModel(globalRenderGroup, &fern, COLOR_WHITE);

            // float hitT = 0;
            // V3 hitP = {};
            // V3 direction = easyMath_getZAxis(easy3d_getViewToWorld(&camera));
            // EasyRay r = {};
            // r.origin = camera.pos;
            // r.direction = direction;
            // r = EasyMath_transformRay(r, mat4_transpose(m));
            // bool h = easyMath_rayVsAABB3f(r.origin, r.direction, fern.bounds, &hitP, &hitT);
            // if(h) {
            //     renderDrawCubeOutline(globalRenderGroup, fern.bounds, COLOR_WHITE);
            // }
            easyTerrain_renderTerrain(terrain, globalRenderGroup, &emptyMaterial);

            renderEnableCulling(globalRenderGroup);
            // setModelTransform(globalRenderGroup, Matrix4_translate(scaledIdentity, v3(1, 0, 1)));
            // renderModel(globalRenderGroup, &carrotModel, COLOR_WHITE);

            // for(int i = 0; i < 20; ++i) {
            //     setModelTransform(globalRenderGroup, Matrix4_translate(scaledIdentity, v3(x[i], 0, y[i])));
            //     renderModel(globalRenderGroup, &palmTree, COLOR_WHITE);
            // }

            // for(int i = 0; i < 20; ++i) {
            //    setModelTransform(globalRenderGroup, Matrix4_translate(scaledIdentity, v3(x1[i], 0, y1[i])));
            //    renderModel(globalRenderGroup, &firTree, COLOR_WHITE);
            // }


            // setModelTransform(globalRenderGroup, Matrix4_translate(scaledIdentity, v3(20, 0, 1)));
            // renderModel(globalRenderGroup, &oakTree, COLOR_WHITE);

            // setModelTransform(globalRenderGroup, Matrix4_translate(scaledIdentity, v3(30, 0, 1)));
            // renderModel(globalRenderGroup, &firTree, COLOR_WHITE);


            // setModelTransform(globalRenderGroup,  Matrix4_scale(identity, v3(0.3, 0.3, 0.3)));
            // renderModel(globalRenderGroup, &character, COLOR_WHITE);

            // setModelTransform(globalRenderGroup, Matrix4_translate(Matrix4_scale(identity, v3(0.1f, 0.1f, 0.1f)), v3(7, 0, 10)));
            // renderModel(globalRenderGroup, &tower, COLOR_WHITE);

            // setModelTransform(globalRenderGroup, Matrix4_translate(Matrix4_scale(identity, v3(0.1f, 0.1f, 0.1f)), v3(7, 0, 1)));
            // renderModel(globalRenderGroup, &ship, COLOR_WHITE);

            // setModelTransform(globalRenderGroup, Matrix4_translate(scaledIdentity, v3(40, 0, 1)));
            // renderModel(globalRenderGroup, &poplarTree, COLOR_WHITE);

            // renderDisableCulling(globalRenderGroup);
            // renderDrawBillBoardQuad(globalRenderGroup,  &flowerMaterial, COLOR_WHITE, v3(10, 10, 10), v3(0, 1, 0), v3(0, 0, 0), camera.pos);
            // renderEnableCulling(globalRenderGroup);
           
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
                    } else {
                        easyConsole_parseDefault(&console);
                    }
                } else {
                    easyConsole_parseDefault(&console);
                }
            }


            if(inEditor) {
                easyEditor_startWindow(editor, "Lister Panel");
                
                static V3 v = {};
                easyEditor_pushFloat3(editor, "Position:", &T.pos.x, &T.pos.y, &T.pos.z);
                easyEditor_pushFloat3(editor, "Rotation:", &T.Q.i, &T.Q.j, &T.Q.k);
                easyEditor_pushFloat3(editor, "Scale:", &T.scale.x, &T.scale.y, &T.scale.z);

                static bool saveLevel = false;
                if(easyEditor_pushButton(editor, "Save Level")) {
                    saveLevel = !saveLevel;
                }

                if(saveLevel) {
                    easyEditor_pushFloat1(editor, "", &v.x);
                    easyEditor_pushFloat2(editor, "", &v.x, &v.y);
                    easyEditor_pushFloat3(editor, "", &v.x, &v.y, &v.z);
                }

                easyEditor_endWindow(editor); //might not actuall need this
                

                easyEditor_endEditorForFrame(editor);
            }


            //a.T->pos = screenSpaceToWorldSpace(perspectiveMatrix, keyStates.mouseP_left_up, resolution, -10, mat4());
            //Matrix4 m = mat4_angle_aroundZ(1.28);
            //a.T->T = m;
            
            // if(wasPressed(keyStates.gameButtons, BUTTON_LEFT_MOUSE)) {
            //     printf("%s\n", "wasPressed");
            //     V3 screenP = screenSpaceToWorldSpace(perspectiveMatrix, keyStates.mouseP_left_up, resolution, -10, mat4());
            //     EasyPhysics_AddRigidBody(&gameObjects, 0.1f, 1, screenP);

            // }
            // physicsTime += appInfo.dt;
            // float timeInterval = 0.002f;
            // while(physicsTime > timeInterval) {
            //     ProcessPhysics(&gameObjects, timeInterval);
            //     physicsTime -= timeInterval;
            // }

            // for (int i = 0; i < gameObjects.count; ++i)
            // {
            //     EasyRigidBody *a = (EasyRigidBody *)getElement(&gameObjects, i);
            //     a->T->T = mat4_angle_aroundZ(a->angle);
            //     if(a) {
            //         renderDrawRectCenterDim(a->T->pos, v2(1, 1), COLOR_BLACK, a->angle, mat4(), perspectiveMatrix);
            //     }
            // }
            
            //////
            drawRenderGroup(globalRenderGroup);


            clearBufferAndBind(toneMappedBuffer.bufferId, COLOR_YELLOW, toneMappedBuffer.flags, &postProcessRenderGroup); 
            renderSetShader(&postProcessRenderGroup, &toneMappingProgram);
            renderBlitQuad(&postProcessRenderGroup, mainFrameBuffer.textureIds[0]);

            drawRenderGroup(&postProcessRenderGroup);
            
            easyOS_endFrame(resolution, screenDim, toneMappedBuffer.bufferId, &appInfo, hasBlackBars);
            easyOS_endKeyState(&keyStates);
        }
        easyOS_endProgram(&appInfo);
    }
    return(0);
}
    