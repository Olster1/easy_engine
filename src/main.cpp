#if DEVELOPER_MODE
#define RESOURCE_PATH_EXTENSION "../res/" 
#else 
#define RESOURCE_PATH_EXTENSION "res/"
#define NDEBUG
#endif

#include <GL/gl3w.h>


#include "../SDL2/sdl.h"
#include "../SDL2/SDL_syswm.h"

#include "defines.h"
#include "easy_headers.h"
#include "easy_asset_loader.h"

#if __APPLE__
#include <unistd.h>
#endif

/*
    Have white texture known to the renderer 
    Have arenas setup automatically 
    have all the other stuff like monifotr refresh rate setup automatically 
    have it so you don't have to have an extra buffer
    Make button a more reusable concept
    Use matrixes instead of meters per pixels

*/


int main(int argc, char *args[]) {
    V2 screenDim = v2(0, 0); //init in create app function
    V2 resolution = v2(0, 0);
    screenDim = resolution;
    OSAppInfo appInfo = easyOS_createApp("Physics 2D", &screenDim, false);
    assert(appInfo.valid);
    
    if(appInfo.valid) {
        Arena longTermArena = createArena(Kilobytes(200));
        assets = (Asset **)pushSize(&longTermArena, 4096*sizeof(Asset *));
        AppSetupInfo setupInfo = easyOS_setupApp(&resolution, RESOURCE_PATH_EXTENSION, &longTermArena);
        
        float monitorRefreshRate = (float)setupInfo.refresh_rate;
        float dt = 1.0f / min(monitorRefreshRate, 60.0f); //use monitor refresh rate 
        float idealFrameTime = 1.0f / 60.0f;
        
        loadAndAddImagesToAssets("img/");
        
        ////INIT FONTS
        char *fontName = concat(globalExeBasePath, "/fonts/Khand-Regular.ttf");//Roboto-Regular.ttf");/);
        Font mainFont = initFont(fontName, 32);
        ///
        FrameBuffer mainFrameBuffer = createFrameBuffer(resolution.x, resolution.y, 0);

        unsigned int lastTime = SDL_GetTicks();
        bool scrollBarActive = false;

        bool fullscreen = true;
        bool hasBlackBars = true;
        bool running = true;
        AppKeyStates keyStates = {};


        globalRenderGroup->whiteTexture = findTextureAsset("white.png");
        
        while(running) {
            easyOS_processKeyStates(&keyStates, resolution, &screenDim, &running, !hasBlackBars);
            easyOS_beginFrame(resolution);
            
            beginRenderGroupForFrame(globalRenderGroup);
            

            clearBufferAndBind(appInfo.frameBackBufferId, COLOR_BLACK);
            clearBufferAndBind(mainFrameBuffer.bufferId, COLOR_PINK);
            
            renderEnableDepthTest(globalRenderGroup);
            setBlendFuncType(globalRenderGroup, BLEND_FUNC_STANDARD);
            renderSetViewPort(0, 0, resolution.x, resolution.y);

            ///Drawing here
            

            //
                
            drawRenderGroup(globalRenderGroup);
            
            easyOS_endFrame(resolution, screenDim, &dt, appInfo.windowHandle, mainFrameBuffer.bufferId, appInfo.frameBackBufferId, appInfo.renderBackBufferId, &lastTime, idealFrameTime, hasBlackBars);
            easyOS_endKeyState(&keyStates);
        }
        easyOS_endProgram(&appInfo);
    }
    return(0);
}
    