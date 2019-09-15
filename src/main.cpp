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

        globalDebugFont = mainFont;
        ///
        FrameBuffer mainFrameBuffer = createFrameBuffer(resolution.x, resolution.y, FRAMEBUFFER_DEPTH | FRAMEBUFFER_STENCIL);
        
        bool hasBlackBars = true;
        bool running = true;
        AppKeyStates keyStates = {};

        // globalRenderGroup->whiteTexture = findTextureAsset("white.png");

        //SETUP
        EasyMaterial crateMaterial = easyCreateMaterial(findTextureAsset("crate.png"), 0, findTextureAsset("crate_specular.png"), 32);
        EasyMaterial emptyMaterial = easyCreateMaterial(findTextureAsset("grey_texture.jpg"), 0, findTextureAsset("grey_texture.jpg"), 32);

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

        easy3d_loadMtl(concatInArena(globalExeBasePath, "models/low_poly_tree_model/tree_mtl.mtl", &globalPerFrameArena));
        EasyModel treeModel = {};
        easy3d_loadObj(concatInArena(globalExeBasePath, "models/low_poly_tree_model/tree_obj.obj", &globalPerFrameArena), &treeModel);

        //  easy3d_loadMtl(concatInArena(globalExeBasePath, "models/zombie/Zombie.mtl", &globalPerFrameArena));
        // EasyModel treeModel = {};
        // easy3d_loadObj(concatInArena(globalExeBasePath, "models/zombie/Zombie.obj", &globalPerFrameArena), &treeModel);

        Array_Dynamic gameObjects;
        initArray(&gameObjects, EasyRigidBody);

        V3 lowerHalfP = screenSpaceToWorldSpace(projectionMatrixFOV(camera.zoom, resolution.x/resolution.y), v2(0.5f*screenDim.x, 0.2f*screenDim.y), resolution, -10, mat4());
        EasyPhysics_AddRigidBody(&gameObjects, 0, 0, lowerHalfP);

        lowerHalfP = screenSpaceToWorldSpace(projectionMatrixFOV(camera.zoom, resolution.x/resolution.y), v2(0.5f*screenDim.x, 0.8f*screenDim.y), resolution, -10, mat4());
        EasyPhysics_AddRigidBody(&gameObjects, 0.1, 1, lowerHalfP);

        EasyLight *light = easy_makeLight(v4(0, 0, 1, 0), v3(1, 1, 1), v3(1, 1, 1), v3(1, 1, 1));
        easy_addLight(globalRenderGroup, light);
        float tAt = 0;
        float physicsTime = 0;

        EasyConsole console = {};
        easyConsole_initConsole(&console, BUTTON_TILDE);

        while(running) {
            easyOS_processKeyStates(&keyStates, resolution, &screenDim, &running, !hasBlackBars);
            easyOS_beginFrame(resolution, &appInfo);
            
            beginRenderGroupForFrame(globalRenderGroup);

            clearBufferAndBind(appInfo.frameBackBufferId, COLOR_BLACK);
            clearBufferAndBind(mainFrameBuffer.bufferId, COLOR_PINK);
            
            renderEnableDepthTest(globalRenderGroup);
            setBlendFuncType(globalRenderGroup, BLEND_FUNC_STANDARD);
            renderSetViewPort(0, 0, resolution.x, resolution.y);

            tAt += appInfo.dt;


            Matrix4 lightDir = easy3d_lookAt(v3(10*cos(0.707f), 10*sin(0.707f), 2), v3(0, 0, 0), v3(0, 1, 0));
           

            float zAt = -10;//-2*sin(tAt);
            // V3 eyepos = v3(2*cos(tAt), 0, zAt);
            // V3 eyepos = v3(0, 0, -10);
            // Matrix4 viewMatrix = easy3d_lookAt(eyepos, v3(0, 0, 0), v3(0, 1, 0));
            

            easy3d_updateCamera(&camera, &keyStates, 1, 2000.0f, appInfo.dt);

            easy_setEyePosition(globalRenderGroup, camera.pos);
            // update camera first
            Matrix4 viewMatrix = easy3d_getWorldToView(&camera);
            Matrix4 perspectiveMatrix = projectionMatrixFOV(camera.zoom, resolution.x/resolution.y);
            Matrix4 identity = Matrix4_translate(mat4(), v3(2, 0, 2));//mat4_angle_aroundZ(tAt);\

            //
            //
            light->direction.xyz = normalizeV3(v3_minus(v3(0, 0, 0), camera.pos));//easyMath_getXAxis(lightDir);//
            // // printf("%s %f %f %f\n", "color: ", treeModel.meshes[1]->material->defaultAmbient.x, treeModel.meshes[1]->material->defaultAmbient.y, treeModel.meshes[1]->material->defaultAmbient.z);
            // renderDrawRectCenterDim(v3(100, 100, 1), v2(100, 100), COLOR_BLACK, 0, mat4(), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));
            //renderTextureCentreDim(findTextureAsset("front.jpg"), v3(400, 400, 1), v2(400, 400), COLOR_WHITE, 0, mat4(), mat4(), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));
            ///Drawing here
            //outputText(&mainFont, 0.5f*resolution.x, 0.5f*resolution.y, 1, resolution, "hey there", rect2fMinMax(0, 0, 1000, 1000), COLOR_BLACK, 1, true, appInfo.screenRelativeSize);
            //What this is should be...size should be baked into the font. 
            // outputText(&mainFont, 0.5f*resolution.x, 0.5f*resolution.y, -1, "hey there");
            renderSetShader(globalRenderGroup, &phongProgram);
            setModelTransform(globalRenderGroup, identity);
            setViewTransform(globalRenderGroup, viewMatrix);
            setProjectionTransform(globalRenderGroup, perspectiveMatrix);

            renderEnableDepthTest(globalRenderGroup);
            renderDrawCube(globalRenderGroup, &crateMaterial, COLOR_WHITE);
            // renderMesh(globalRenderGroup, &floorMesh, hexARGBTo01Color(0xFFCEFF74), &emptyMaterial);

            setModelTransform(globalRenderGroup, Matrix4_scale(identity, v3(0.1f, 0.1f, 0.1f)));
            renderModel(globalRenderGroup, &treeModel, COLOR_WHITE);

            static bool godmode = false;
            if(easyConsole_update(&console, &keyStates, appInfo.dt, resolution, appInfo.screenRelativeSize)) {
                EasyToken token = easyConsole_getNextToken(&console);
                if(token.type == TOKEN_WORD) {
                    if(stringsMatchNullN("godmode", token.at, token.size)) {
                        godmode = !godmode;
                    } 
                } else {
                    easyConsole_parseDefault(&console);
                }
            }


            if(godmode) {
                outputText(&mainFont, 0.5f*resolution.x, 0.5f*resolution.y, 1, resolution, "hey there", rect2fMinMax(0, 0, 1000, 1000), COLOR_BLACK, 1, true, appInfo.screenRelativeSize);
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
            
            easyOS_endFrame(resolution, screenDim, mainFrameBuffer.bufferId, &appInfo, hasBlackBars);
            easyOS_endKeyState(&keyStates);
        }
        easyOS_endProgram(&appInfo);
    }
    return(0);
}
    