#include <GL/gl3w.h>

#include "../SDL2/sdl.h"
#include "../SDL2/SDL_syswm.h"

#include "defines.h"
#include "easy_headers.h"

// #if __APPLE__
// #include <unistd.h>
// #endif

/*
    have it so you don't have to have an extra buffer
    Use matrixes instead of meters per pixels

*/

void EasyPhysics_AddRigidBody(Array_Dynamic *gameObjects, float inverseWeight, float inverseIntertia, V3 pos) {
    EasyRigidBody *rb = (EasyRigidBody *)getEmptyElement(gameObjects);
    
    EasyTransform *T = pushStruct(&globalLongTermArena, EasyTransform);
    T->pos = pos;

    rb->T = T;
    rb->dP = v3(0, 0, 0);
    rb->inverseWeight = inverseWeight;
    rb->inverse_I = inverseIntertia;
    rb->angle = 0; 
    rb->dA = 0;
}

void ProcessPhysics(Array_Dynamic *gameObjects, float dt) {
    // printf("%d\n",  gameObjects->count);
    for (int i = 0; i < gameObjects->count; ++i)
    {
        EasyRigidBody *a = (EasyRigidBody *)getElement(gameObjects, i);
        if(a) {
            V3 lastPos = a->T->pos;
            float lastAngle = a->dA;
            V3 lastVelocity = a->dP;

            V3 nextPos = v3_plus(v3_scale(dt, a->dP), a->T->pos);
            float nextAngle = a->dA*dt + a->angle;
            V3 nextVelocity = v3_plus(v3_scale(dt, v3(0, a->inverseWeight*-9.81f, 0)), a->dP);
            // printf("%f %f\n", nextVelocity.x, nextVelocity.y);
            // nextVelocity = v3_minus(nextVelocity, v3_scale(0.1f, nextVelocity));

            a->T->pos = nextPos;
            a->angle = nextAngle;
            a->dP = nextVelocity;
            
            float smallestDistance = 0.1f;
            EasyRigidBody *hitEnt = 0;
            EasyCollisionOutput outputInfo;
            bool didHit = false;
            for (int j = 0; j < gameObjects->count; ++j)
            {
                if(i != j) {
                    EasyRigidBody *b = (EasyRigidBody *)getElement(gameObjects, j);
                    if(b && !(a->inverseWeight == 0 && b->inverseWeight == 0)) {
                        EasyCollisionOutput out = EasyPhysics_SolveRigidBodies(a, b);

                        // out.pointA.z = -10;
                        // out.pointB.z = -10;
                        if(out.distance <= smallestDistance) {
                            hitEnt = b;
                            smallestDistance = out.distance;
                            outputInfo = out;
                        }
                        // renderDrawRectCenterDim(out.pointA, v2(0.1, 0.1), COLOR_BLUE, 0, mat4(), perspectiveMatrix);
                        // renderDrawRectCenterDim(out.pointB, v2(0.1, 0.1), COLOR_BLUE, 0, mat4(), perspectiveMatrix);            
                    }
                }
            }

            if(hitEnt) {
                EasyPhysics_ResolveCollisions(a, hitEnt, &outputInfo);
            } else {
                // printf("%s\n", "hey");
                //keep at new positions
            }
        }
    }

}

int main(int argc, char *args[]) {
    V2 screenDim = v2(400, 400); //init in create app function
    V2 resolution = v2(0, 0);
    // screenDim = resolution;
    bool fullscreen = false;
    OSAppInfo appInfo = easyOS_createApp("Physics 2D", &screenDim, fullscreen);
    assert(appInfo.valid);
    if(appInfo.valid) {
        
        easyOS_setupApp(&appInfo, &resolution, RESOURCE_PATH_EXTENSION);
        
        loadAndAddImagesToAssets("img/");
        
        ////INIT FONTS
        char *fontName = concatInArena(globalExeBasePath, "/fonts/Khand-Regular.ttf", &globalPerFrameArena);
        Font mainFont = initFont(fontName, 88);
        ///
        FrameBuffer mainFrameBuffer = createFrameBuffer(resolution.x, resolution.y, FRAMEBUFFER_DEPTH | FRAMEBUFFER_STENCIL);
        
        bool hasBlackBars = true;
        bool running = true;
        AppKeyStates keyStates = {};

        // globalRenderGroup->whiteTexture = findTextureAsset("white.png");

        //SETUP
        EasyMaterial crateMaterial = easyCreateMaterial(findTextureAsset("crate.png"), 0, findTextureAsset("crate_specular.png"), 32);
        EasyMaterial emptyMaterial = easyCreateMaterial(findTextureAsset("grey_texture.jpg"), 0, findTextureAsset("grey_texture.jpg"), 32);

        //


#define perlinWidth 100
#define perlinHeight 100
        float perlinWordData[perlinHeight*perlinWidth];

        InfiniteAlloc floormeshdata = initInfinteAlloc(Vertex);
        InfiniteAlloc indicesData = initInfinteAlloc(unsigned int);

        int triCount = 0;
        // for(s32 y = 0; y < perlinHeight; y++) {
        //     for(s32 x = 0; x < perlinWidth; x++) {
        //         s32 subY = y - (perlinHeight/2);
        //         float height = perlinWordData[x + y*perlinWidth] = perlin2d(x, subY, 0.1, 8);
        //         float height1 = perlin2d(x + 1, subY, 1, 8);
        //         float height2 = perlin2d(x, subY + 1, 1, 8);
        //         V3 p1 = v3(x + 1, height1, subY);
        //         V3 p2 = v3(x, height2, subY + 1);
        //         V3 a = normalizeV3(v3_minus(p1, v3(x, height, subY)));
        //         V3 b = normalizeV3(v3_minus(p2, v3(x, height, subY)));

        //         V3 normal = v3_crossProduct(a, b);

        //         // printf("%f %f %f\n", a.x, a.y, a.z);
        //         // printf("%f %f %f\n", b.x, b.y, b.z);
        //         // printf("---------\n");

        //         Vertex v = vertex(v3(0.2f*x, height, 0.2f*subY), normal, v2(0, 0));
        //         addElementInifinteAlloc_(&floormeshdata, &v);
        //         if(y < (perlinHeight - 1) && x < (perlinWidth - 1)) { //not on edge

        //             unsigned int index[6] = {
        //                 x + (perlinWidth*y), 
        //                 x + 1 + (perlinWidth*y),
        //                 x + 1 + (perlinWidth*(y+1)), 
        //                 x + (perlinWidth*y), 
        //                 x + 1 + (perlinWidth*(y+1)), 
        //                 x + (perlinWidth*(y+1)) 
        //             };
        //             addElementInifinteAllocWithCount_(&indicesData, &index, 6);
        //             triCount += 2;
        //         }
                
        //     }
        // }

        
        VaoHandle floorMesh = {};
        initVao(&floorMesh, (Vertex *)floormeshdata.memory, triCount, (unsigned int *)indicesData.memory, indicesData.count);

        // for(s32 y = 0; y < perlinHeight; y++) {
        //     for(s32 x = 0; x < perlinWidth; x++) {
        //         perlinWordData[x + y*perlinWidth] = perlin2d(x, y, 0.1, 8);
        //     }
        // }

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

        // globalRenderGroup->skybox = skybox;

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

            light->direction.xyz = v3(cos(tAt), sin(tAt), 0);

            float zAt = -10;//-2*sin(tAt);
            V3 eyepos = v3(2*cos(tAt), 0, zAt);
            // V3 eyepos = v3(0, 0, -10);
            // Matrix4 viewMatrix = easy3d_lookAt(eyepos, v3(0, 0, 0), v3(0, 1, 0));
            // easy_setEyePosition(globalRenderGroup, eyepos);

            easy3d_updateCamera(&camera, &keyStates, 1, 400.0f, appInfo.dt);
            // update camera first
            Matrix4 viewMatrix = easy3d_getWorldToView(&camera);
            Matrix4 perspectiveMatrix = projectionMatrixFOV(camera.zoom, resolution.x/resolution.y);
            Matrix4 identity = mat4();//mat4_angle_aroundZ(tAt);
            //

            // renderDrawRectCenterDim(v3(100, 100, 1), v2(100, 100), COLOR_BLACK, 0, mat4(), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));
            //renderTextureCentreDim(findTextureAsset("front.jpg"), v3(400, 400, 1), v2(400, 400), COLOR_WHITE, 0, mat4(), mat4(), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));
            ///Drawing here
            // outputText(&mainFont, 0.5f*resolution.x, 0.5f*resolution.y, 1, resolution, "hey there", rect2fMinMax(0, 0, 1000, 1000), COLOR_BLACK, 1, true, appInfo.screenRelativeSize);
            //What thisis should be...size should be baked into the font. 
            // outputText(&mainFont, 0.5f*resolution.x, 0.5f*resolution.y, -1, "hey there");
            renderSetShader(globalRenderGroup, &phongProgram);
            setModelTransform(globalRenderGroup, identity);
            setViewTransform(globalRenderGroup, viewMatrix);
            setProjectionTransform(globalRenderGroup, perspectiveMatrix);

            renderEnableDepthTest(globalRenderGroup);
            // renderDrawCube(globalRenderGroup, &crateMaterial, COLOR_WHITE);
            // renderModel(globalRenderGroup, &floorMesh, hexARGBTo01Color(0xFFCEFF74), &emptyMaterial);

            //a.T->pos = screenSpaceToWorldSpace(perspectiveMatrix, keyStates.mouseP_left_up, resolution, -10, mat4());
            //Matrix4 m = mat4_angle_aroundZ(1.28);
            //a.T->T = m;
            
            if(wasPressed(&keyStates.gameButtons, BUTTON_LEFT_MOUSE)) {
                V3 screenP = screenSpaceToWorldSpace(perspectiveMatrix, keyStates.mouseP_left_up, resolution, -10, mat4());
                // EasyPhysics_AddRigidBody(&gameObjects, 0.1f, screenP);

            }
            physicsTime += appInfo.dt;
            float timeInterval = 0.002f;
            while(physicsTime > timeInterval) {
                ProcessPhysics(&gameObjects, timeInterval);
                physicsTime -= timeInterval;
            }

            for (int i = 0; i < gameObjects.count; ++i)
            {
                EasyRigidBody *a = (EasyRigidBody *)getElement(&gameObjects, i);
                a->T->T = mat4_angle_aroundZ(a->angle);
                if(a) {
                    renderDrawRectCenterDim(a->T->pos, v2(1, 1), COLOR_BLACK, a->angle, mat4(), perspectiveMatrix);
                }
            }
            
            //////
            drawRenderGroup(globalRenderGroup);
            
            easyOS_endFrame(resolution, screenDim, mainFrameBuffer.bufferId, &appInfo, hasBlackBars);
            easyOS_endKeyState(&keyStates);
        }
        easyOS_endProgram(&appInfo);
    }
    return(0);
}
    