#include <GL/gl3w.h>

#include "../SDL2/sdl.h"
#include "../SDL2/SDL_syswm.h"

#include "defines.h"
#include "easy_headers.h"

// #if __APPLE__
// #include <unistd.h>
// #endif

/*
    Have white texture known to the renderer 
    have it so you don't have to have an extra buffer
    Make button a more reusable concept
    Use matrixes instead of meters per pixels

*/


int main(int argc, char *args[]) {
    V2 screenDim = v2(0, 0); //init in create app function
    V2 resolution = v2(0, 0);
    screenDim = resolution;
    bool fullscreen = false;
    OSAppInfo appInfo = easyOS_createApp("Physics 2D", &screenDim, fullscreen);
    assert(appInfo.valid);
    if(appInfo.valid) {
        
        easyOS_setupApp(&appInfo, &resolution, RESOURCE_PATH_EXTENSION);
        
        loadAndAddImagesToAssets("img/");
        
        ////INIT FONTS
        char *fontName = concatInArena(globalExeBasePath, "/fonts/Khand-Regular.ttf", &globalPerFrameArena);
        Font mainFont = initFont(fontName, 32);
        ///
        FrameBuffer mainFrameBuffer = createFrameBuffer(resolution.x, resolution.y, 0);
        
        bool hasBlackBars = true;
        bool running = true;
        AppKeyStates keyStates = {};

        globalRenderGroup->whiteTexture = findTextureAsset("white.png");
        
        while(running) {
            easyOS_processKeyStates(&keyStates, resolution, &screenDim, &running, !hasBlackBars);
            easyOS_beginFrame(resolution, &appInfo);
            
            beginRenderGroupForFrame(globalRenderGroup);

            clearBufferAndBind(appInfo.frameBackBufferId, COLOR_BLACK);
            clearBufferAndBind(mainFrameBuffer.bufferId, COLOR_PINK);
            
            renderEnableDepthTest(globalRenderGroup);
            setBlendFuncType(globalRenderGroup, BLEND_FUNC_STANDARD);
            renderSetViewPort(0, 0, resolution.x, resolution.y);

            ///Drawing here
            outputText(&mainFont, 0.5f*resolution.x, 0.5f*resolution.y, -1, resolution, "hey there", rect2fMinMax(0, 0, 1000, 1000), COLOR_BLACK, 1, true, appInfo.screenRelativeSize);
            //What is should be...size should be baked into the font. 
            // outputText(&mainFont, 0.5f*resolution.x, 0.5f*resolution.y, -1, "hey there");

            //
                
            drawRenderGroup(globalRenderGroup);
            
            easyOS_endFrame(resolution, screenDim, mainFrameBuffer.bufferId, &appInfo, hasBlackBars);
            easyOS_endKeyState(&keyStates);
        }
        easyOS_endProgram(&appInfo);
    }
    return(0);
}
    