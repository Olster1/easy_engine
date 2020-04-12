#include "defines.h"
#include "easy_headers.h"

#include "window_scene.h"
#include "logicBlock.c"
#include "logic_ui.c"
#include "gameState.h"
#include "gameScene.c"
#include "draw_windows.c"


int main(int argc, char *args[]) {
    
    EASY_ENGINE_ENTRY_SETUP()
    
    V2 screenDim = v2(DEFINES_WINDOW_SIZE_X, DEFINES_WINDOW_SIZE_Y); //init in create app function
    V2 resolution = v2(DEFINES_RESOLUTION_X, DEFINES_RESOLUTION_Y);
    
    OSAppInfo appInfo = easyOS_createApp("Easy Engine", &screenDim, false);
    
    if(appInfo.valid) {
        
        easyOS_setupApp(&appInfo, &resolution, RESOURCE_PATH_EXTENSION);

        FrameBuffer mainFrameBuffer;
        FrameBuffer toneMappedBuffer;
        FrameBuffer bloomFrameBuffer;
        FrameBuffer cachedFrameBuffer;

        {
            DEBUG_TIME_BLOCK_NAMED("Create frame buffers");
            
            //******** CREATE THE FRAME BUFFERS ********///
            
            mainFrameBuffer = createFrameBuffer(resolution.x, resolution.y, FRAMEBUFFER_COLOR | FRAMEBUFFER_DEPTH | FRAMEBUFFER_STENCIL | FRAMEBUFFER_HDR, 2);
            toneMappedBuffer = createFrameBuffer(resolution.x, resolution.y, FRAMEBUFFER_COLOR | FRAMEBUFFER_DEPTH | FRAMEBUFFER_STENCIL, 1);
            
            bloomFrameBuffer = createFrameBuffer(resolution.x, resolution.y, FRAMEBUFFER_COLOR | FRAMEBUFFER_HDR, 1);
            
            cachedFrameBuffer = createFrameBuffer(resolution.x, resolution.y, FRAMEBUFFER_COLOR | FRAMEBUFFER_HDR, 1);
            //////////////////////////////////////////////////
        
        }

        loadAndAddImagesToAssets("img/engine_icons/");
        
        EasyCamera camera;
        easy3d_initCamera(&camera, v3(0, 0, 0));
        
        EasyTransform sunTransform;
        easyTransform_initTransform(&sunTransform, v3(0, -10, 0), EASY_TRANSFORM_TRANSIENT_ID);
        
        EasyLight *light = easy_makeLight(&sunTransform, EASY_LIGHT_DIRECTIONAL, 1.0f, v3(1, 1, 1));//easy_makeLight(v4(0, -1, 0, 0), v3(1, 1, 1), v3(1, 1, 1), v3(1, 1, 1));
        easy_addLight(globalRenderGroup, light);
        

        GameState *gameState = initGameState((resolution.y / resolution.x));

        GameScene *mainScene = findGameSceneByName(gameState, "Main");

        WindowScene *playWindow = findFirstWindowByType(gameState, WINDOW_TYPE_SCENE_GAME);

        assert(mainScene);

        pushLogicBlock(&mainScene->logicSet, LOGIC_BLOCK_CLEAR_COLOR);
        pushLogicBlock(&mainScene->logicSet, LOGIC_BLOCK_LOOP);
        pushLogicBlock(&mainScene->logicSet, LOGIC_BLOCK_DRAW_RECTANGLE);
        // pushLogicBlock(&mainScene->logicSet, LOGIC_BLOCK_POP_SCOPE);
        // pushLogicBlock(&mainScene->logicSet, LOGIC_BLOCK_POP_SCOPE);

        assert(playWindow);
        compileLogicBlocks(&mainScene->logicSet, &mainScene->vmMachine, easyRender_getDefaultFauxResolution(getDim(playWindow->dim)));


        ///////////************************/////////////////
        while(appInfo.running) {
            
            easyOS_processKeyStates(&appInfo.keyStates, resolution, &screenDim, &appInfo.running, !appInfo.hasBlackBars);
            easyOS_beginFrame(resolution, &appInfo);
            
            beginRenderGroupForFrame(globalRenderGroup);
            
            clearBufferAndBind(appInfo.frameBackBufferId, COLOR_BLACK, FRAMEBUFFER_COLOR, 0);
            clearBufferAndBind(mainFrameBuffer.bufferId, COLOR_WHITE, mainFrameBuffer.flags, globalRenderGroup);
            clearBufferAndBind(toneMappedBuffer.bufferId, COLOR_PINK, toneMappedBuffer.flags, globalRenderGroup);
            
            renderEnableDepthTest(globalRenderGroup);
            renderEnableCulling(globalRenderGroup);
            setBlendFuncType(globalRenderGroup, BLEND_FUNC_STANDARD_PREMULTIPLED_ALPHA);
            renderSetViewport(globalRenderGroup, 0, 0, resolution.x, resolution.y);
            
            
            EasyCamera_MoveType camMove = (EasyCamera_MoveType)(EASY_CAMERA_MOVE | EASY_CAMERA_ROTATE | EASY_CAMERA_ZOOM);
                
            easy3d_updateCamera(&camera, &appInfo.keyStates, 1, 100.0f, appInfo.dt, camMove);

            easy_setEyePosition(globalRenderGroup, camera.pos);
            
            // update camera first
            Matrix4 viewMatrix = easy3d_getWorldToView(&camera);
            Matrix4 perspectiveMatrix = projectionMatrixFOV(camera.zoom, resolution.x/resolution.y);

            ///////////////////////************* Update the Windows ************////////////////////

            updateAndRenderSceneWindows(gameState, &appInfo);
            
            ////////////////////////////////////////////////////////////////////


            drawRenderGroup(globalRenderGroup, (RenderDrawSettings)(RENDER_DRAW_SORT));

            //NOTE(ollie): Make sure the scene chooser is on top
            renderClearDepthBuffer(toneMappedBuffer.bufferId);
            
            ///////////////////////************ Draw the scenes menu *************////////////////////

            updateAndRenderGameSceneMenuBar(gameState, &appInfo);

            ////////////////////////////////////////////////////////////////////

            drawRenderGroup(globalRenderGroup, (RenderDrawSettings)(RENDER_DRAW_SORT));
            
            //NOTE(ollie): Update the console
            if(easyConsole_update(&appInfo.console, &appInfo.keyStates, appInfo.dt, (resolution.y / resolution.x))) {
                EasyToken token = easyConsole_getNextToken(&appInfo.console);
                if(token.type == TOKEN_WORD) {
                    // if(stringsMatchNullN("camPower", token.at, token.size)) {
                    // }
                } else {
                    easyConsole_parseDefault(&appInfo.console, token);
                }
            }


            //////////////////////////////////////////////////////////////////////////////////////////////

            easyOS_endFrame(resolution, screenDim, toneMappedBuffer.bufferId, &appInfo, appInfo.hasBlackBars);
            DEBUG_TIME_BLOCK_FOR_FRAME_END(beginFrame, wasPressed(appInfo.keyStates.gameButtons, BUTTON_F4))
            DEBUG_TIME_BLOCK_FOR_FRAME_START(beginFrame, "Per frame")
            easyOS_endKeyState(&appInfo.keyStates);
        }
        easyOS_endProgram(&appInfo);
    }
    return(0);
}
