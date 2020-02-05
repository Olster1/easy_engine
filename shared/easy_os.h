#define PRINT_FRAME_RATE 1
static bool globalDebugWriteShaders = false;


typedef struct {
	unsigned int frameBackBufferId;
	unsigned int renderBackBufferId; //used for ios 
	SDL_Window *windowHandle;
	SDL_GLContext renderContext; 
	bool valid;

	//setup afterwards
	float monitorFrameTime;
	Matrix4 metresToPixels;
	Matrix4 pixelsToMeters;
	float screenRelativeSize;
	SDL_AudioSpec audioSpec;
	float dt;
	float idealFrameTime;
	s64 lastTime;

	bool firstFrame;
	//

} OSAppInfo;

OSAppInfo easyOS_createApp(char *windowName, V2 *screenDim, bool fullscreen) {
	OSAppInfo result = {};
	result.valid = true;
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER|SDL_INIT_GAMECONTROLLER) != 0) {
		assert(!"sdl not initialized");
		result.valid = false;
		return result;
	} 
    
#if DESKTOP
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, OPENGL_MAJOR);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, OPENGL_MINOR);
#else 
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif
    
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    
    if(screenDim->x == 0 && screenDim->y == 0) {
	    SDL_DisplayMode DM;
		SDL_GetCurrentDisplayMode(0, &DM);
		screenDim->x = DM.w;
		screenDim->y = DM.h;
	}
    
    u32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    // if(fullscreen) {
    // 	windowFlags |= SDL_WINDOW_FULLSCREEN;
    // 	printf("%s\n", "fullscreen");	
    // }
    
    result.windowHandle = SDL_CreateWindow(
        windowName,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        screenDim->x,
        screenDim->y,
        windowFlags);

    if(fullscreen) {
    	if(SDL_SetWindowFullscreen(result.windowHandle, SDL_WINDOW_FULLSCREEN) < 0) {
    	    printf("couldn't set to full screen\n");
    	}
    }
    
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1); 
    
    // result.threadContext = SDL_GL_CreateContext(result.windowHandle);
    result.renderContext = SDL_GL_CreateContext(result.windowHandle);
    if(result.renderContext) {
        
        if(SDL_GL_MakeCurrent(result.windowHandle, result.renderContext) == 0) {
            if(SDL_GL_SetSwapInterval(1) == 0) {
            } else {
#if DESKTOP
                assert(!"Couldn't set swap interval\n");
                result.valid = false;
#endif
            }
        } else {
            assert(!"Couldn't make context current\n");
            result.valid = false;
        }
    } else {
        assert(!"couldn't make a context");
        result.valid = false;
    }
    
    SDL_SysWMinfo sysInfo;
    SDL_VERSION(&sysInfo.version);
    
    SDL_bool sysResult = SDL_GetWindowWMInfo(result.windowHandle, &sysInfo);
    if(!sysResult) {
        printf("%s\n", SDL_GetError());
        assert(!"couldn't get info");
        result.valid = false;
    }
    
    //TODO make this handle windows //
#if !DESKTOP
    result.frameBackBufferId = sysInfo.info.uikit.framebuffer;
    result.renderBackBufferId = sysInfo.info.uikit.colorbuffer;
#endif
    
    return result;
}

void easyOS_setupApp(OSAppInfo *result, V2 *resolution, char *resPathFolder) {
	EasyTime_setupTimeDatums();

	globalLongTermArena = createArena(Kilobytes(200));
	globalPerFrameArena = createArena(Kilobytes(100));
    assets = (Asset **)pushSize(&globalLongTermArena, GLOBAL_ASSET_ARRAY_SIZE*sizeof(Asset *));

	V2 idealResolution = v2(1280, 720); // Not sure if this is the best place for this?? Have to see. 
    
    SDL_DisplayMode mode;
    SDL_GetCurrentDisplayMode(0, &mode);
    result->monitorFrameTime = 1.0f / mode.refresh_rate;

    result->dt = result->monitorFrameTime; //use monitor refresh rate 
    result->idealFrameTime = 1.0f / 60.0f;

    if(resolution->x == 0 || resolution->y == 0) {
    	resolution->x = mode.w;
    	resolution->y = mode.h;
    }

    if(resolution )
    
#if DESKTOP
    gl3wInit();
#endif
    
	// globalExeBasePath = getResPathFromExePath(SDL_GetBasePath(), resPathFolder);
    globalExeBasePath = concat(SDL_GetBasePath(), resPathFolder);
    
    //for stb_image so the images aren't upside down.
    stbi_set_flip_vertically_on_load(true);
    //
    
#if DEVELOPER_MODE
	if(globalDebugWriteShaders) {
	    char *fileTypes[]= {"glsl", "c"};
	    compileFiles("shaders/", fileTypes, arrayCount(fileTypes));
	    exit(0);
	}
#endif
    
    //////////SETUP AUDIO/////
    initAudioSpec(&result->audioSpec, DEFINES_AUDIO_SAMPLE_RATE);
    initAudio(&result->audioSpec);

    loadAndAddSoundsToAssets(DEFINES_SOUNDS_FOLDER, &result->audioSpec);
    //////////
    
    ////SETUP OPEN GL//
    enableRenderer(resolution->x, resolution->y, &globalLongTermArena);
    renderCheckError();
    //////
    
    ////INIT RANDOM GENERATOR
    srand (time(NULL));
    //////
    
    /////////Scale the size factor to be keep size of app constant. 
    // V2 screenRelativeSize = v2(idealResolution.x / resolution.x, idealResolution.y / resolution.y); 
    V2 screenRelativeSize_ = v2(resolution->x/idealResolution.x, resolution->y/idealResolution.y); 
    float screenRelativeSize;
    if(resolution->x > resolution->y) {
    	screenRelativeSize = screenRelativeSize_.y;
    } else {
    	screenRelativeSize = screenRelativeSize_.x;
    }

    result->firstFrame = true;

    float ratio = 64.0f*screenRelativeSize;
    // assert(ratio.x != 0);
    // assert(ratio.y != 0);
    result->metresToPixels = Matrix4_scale(mat4(), v3(ratio, ratio, 1));
    result->pixelsToMeters = Matrix4_scale(mat4(), v3(1.0f / ratio, 1.0f / ratio, 1));
    result->screenRelativeSize = screenRelativeSize;
    /////
}

void easyOS_endProgram(OSAppInfo *appInfo) {
	SDL_GL_DeleteContext(appInfo->renderContext);
    SDL_DestroyWindow(appInfo->windowHandle);
    SDL_Quit();
}

void easyOS_beginFrame(V2 resolution, OSAppInfo *appInfo) {
	DEBUG_TIME_BLOCK()
	glViewport(0, 0, resolution.x, resolution.y);

	perFrameArenaMark = takeMemoryMark(&globalPerFrameArena);

	if(appInfo->firstFrame) {
		appInfo->lastTime = EasyTime_GetTimeCount();
		appInfo->firstFrame = false;
	}
}

float easyOS_getScreenRatio(V2 screenDim, V2 resolution) {
	DEBUG_TIME_BLOCK()
	float screenRatio =  screenDim.x / resolution.x;
	float h1 = resolution.y * screenRatio;
	if(h1 > screenDim.y) {
	    screenRatio =  screenDim.y / resolution.y;
	    float w1 = resolution.x * screenRatio;
	    // assert(w1 <= screenDim.x);
	}
	return screenRatio;
}


typedef struct {
	GameButton gameButtons[BUTTON_COUNT];
    
	V2 mouseP;
	V2 mouseP_yUp;
	V2 mouseP_left_up;

	char *inputString;
    
	int scrollWheelY;
} AppKeyStates;

static inline void easyOS_updateHotKeys(AppKeyStates *keyStates) {
	///////////////////////*********** Update Any Hotkeys **************////////////////////

	if(wasPressed(keyStates->gameButtons, BUTTON_F2)) {
	    DEBUG_global_DrawFrameRate = !DEBUG_global_DrawFrameRate;
	}
	if(wasPressed(keyStates->gameButtons, BUTTON_F3)) {
	    DEBUG_global_DrawProfiler = !DEBUG_global_DrawProfiler;
	}
	

	////////////////////////////////////////////////////////////////////

}

static inline void easyOS_endFrame(V2 resolution, V2 screenDim, unsigned int compositedFrameBufferId, OSAppInfo *appInfo, bool blackBars) {
	DEBUG_TIME_BLOCK()
	SDL_Window *windowHandle = appInfo->windowHandle;
	unsigned int backBufferId = appInfo->frameBackBufferId;
	unsigned int renderbufferId = appInfo->renderBackBufferId;
	float monitorFrameTime = appInfo->monitorFrameTime; 
	float dt = appInfo->dt;


	////////Letterbox if the ratio isn't correct//
	float screenRatio = easyOS_getScreenRatio(screenDim, resolution);
	V2 screenActualSize = v2_scale(screenRatio, resolution);
    
	float wResidue = (screenDim.x - screenActualSize.x) / 2.0f;
	float yResidue = (screenDim.y - screenActualSize.y) / 2.0f;
    
	////Resolve Frame
	glViewport(0, 0, screenDim.x, screenDim.y);
	renderCheckError();
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, backBufferId);
	renderCheckError();
	glBindFramebuffer(GL_READ_FRAMEBUFFER, compositedFrameBufferId); 
	renderCheckError();

	if(blackBars) {
		glBlitFramebuffer(0, 0, resolution.x, resolution.y, wResidue, yResidue, screenDim.x - wResidue, screenDim.y - yResidue, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	} else {
		glBlitFramebuffer(0, 0, resolution.x, resolution.y, 0, 0, screenDim.x, screenDim.y, GL_COLOR_BUFFER_BIT, GL_LINEAR);	
	}
	renderCheckError();                    
	///////


    updateChannelVolumes(dt);
#if !DESKTOP
    glBindRenderbuffer(GL_RENDERBUFFER, renderbufferId);
#endif
    {
    	DEBUG_TIME_BLOCK_NAMED("SwapWindow")
    	SDL_GL_SwapWindow(windowHandle); 
	}
	
    s64 now = EasyTime_GetTimeCount();

    //NOTE: This is us choosing the best frame time within the intervals of possible frame rates!!!
    float acutalDt = dt = EasyTime_GetSecondsElapsed(now, appInfo->lastTime);

#define GUESS_FRAME_RATE 0
#if GUESS_FRAME_RATE
    float frameRates[] = {monitorFrameTime*1.0f, monitorFrameTime*2.0f, monitorFrameTime*3.0f, monitorFrameTime*4.0f};
    float smallestDiff = 0;
    bool set = false;
    float newRate = monitorFrameTime;
    for(int i = 0; i < arrayCount(frameRates); ++i) {
        float val = frameRates[i];
        float diff = dt - val;
        if(diff < 0.0f) {
            diff *= -1;
        }
        if(diff < smallestDiff || !set) {
            set = true;
            smallestDiff = diff;
            newRate = val;
        }
    }
#else 
    float newRate = acutalDt;
#endif


    // bool vsyncEnabled = true; //TODO:  Actually check if vsync is enabled by the user
    // if(!vsyncEnabled) {
    // 	while(EasyTime_GetSecondsElapsed(now, appInfo->lastTime) < newRate) {
    // 		now = EasyTime_GetTimeCount();
    // 	}
    // } 
   
    dt = appInfo->dt = newRate; //set the actual dt

    globalTimeSinceStart += dt;

#if PRINT_FRAME_RATE
    printf("%f\n", acutalDt); 
#endif
    appInfo->lastTime = now;

    {
    DEBUG_TIME_BLOCK_NAMED("release Memory Mark")
    releaseMemoryMark(&perFrameArenaMark);
	}
}

static inline void easyOS_processKeyStates(AppKeyStates *state, V2 resolution, V2 *screenDim, bool *running, bool stretched) {
	DEBUG_TIME_BLOCK()
	//Save state of last frame game buttons 
	bool mouseWasDown = isDown(state->gameButtons, BUTTON_LEFT_MOUSE);
	bool mouseWasDownRight = isDown(state->gameButtons, BUTTON_RIGHT_MOUSE);
	bool leftArrowWasDown = isDown(state->gameButtons, BUTTON_LEFT);
	bool rightArrowWasDown = isDown(state->gameButtons, BUTTON_RIGHT);
	bool upArrowWasDown = isDown(state->gameButtons, BUTTON_UP);
	bool downArrowWasDown = isDown(state->gameButtons, BUTTON_DOWN);
	bool shiftWasDown = isDown(state->gameButtons, BUTTON_SHIFT);
	bool commandWasDown = isDown(state->gameButtons, BUTTON_COMMAND);
	bool spaceWasDown = isDown(state->gameButtons, BUTTON_SPACE);
	/////
	zeroArray(state->gameButtons);
	assert(state->inputString == 0);
	
	assert(state->gameButtons[BUTTON_LEFT_MOUSE].transitionCount == 0);
	//ask player for new input
	SDL_Event event = {};
	state->scrollWheelY = 0;
	
	while( SDL_PollEvent( &event ) != 0 ) {
        switch(event.type) {
	        case SDL_APP_DIDENTERFOREGROUND: {
	        	SDL_Log("SDL_APP_DIDENTERFOREGROUND");
	        } break;
		    case SDL_APP_DIDENTERBACKGROUND: {
		        SDL_Log("SDL_APP_DIDENTERBACKGROUND");
		    } break;
		    case SDL_APP_LOWMEMORY: {
		        SDL_Log("SDL_APP_LOWMEMORY");
		    } break;
		    case SDL_APP_TERMINATING: {
		        SDL_Log("SDL_APP_TERMINATING");
		    } break;
		    case SDL_APP_WILLENTERBACKGROUND: {
		        SDL_Log("SDL_APP_WILLENTERBACKGROUND");
		    } break;
		    case SDL_APP_WILLENTERFOREGROUND: {
		        SDL_Log("SDL_APP_WILLENTERFOREGROUND");
		    } break;
		    case SDL_FINGERMOTION: {
		        // SDL_Log("Finger Motion");
		    } break;
		    case SDL_FINGERDOWN: {
		        // SDL_Log("Finger Down");
			} break;		        
		    case SDL_FINGERUP: {
		        // SDL_Log("Finger Up");
		    } break;
		}
		
	    if (event.type == SDL_WINDOWEVENT) {
	        switch(event.window.event) {
	            case SDL_WINDOWEVENT_RESIZED: {
	                screenDim->x = event.window.data1;
	                screenDim->y = event.window.data2;
	            } break;
	            default: {
	            }
	        }
	    }  else if( event.type == SDL_QUIT ) {
	        *running = false;
	    } else if(event.type == SDL_MOUSEWHEEL) {
	        state->scrollWheelY = event.wheel.y;
	    } else if(event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
	        
	        SDL_Keycode keyCode = event.key.keysym.sym;
	        
	        bool altKeyWasDown = (event.key.keysym.mod & KMOD_ALT);
	        bool shiftKeyWasDown = (event.key.keysym.mod & KMOD_SHIFT);
	        bool isDown = (event.key.state == SDL_PRESSED);
	        bool repeated = event.key.repeat;
	        ButtonType buttonType = BUTTON_NULL;
	        switch(keyCode) {
	            case SDLK_RETURN: {
	                buttonType = BUTTON_ENTER;
	                if(isDown) {
		                if(state->inputString) {
		                	state->inputString = concat(state->inputString, "\n");
		                } else {
		                	state->inputString = nullTerminate("\n", 1);
		                }
		            }
	            } break;
	            case SDLK_SPACE: {
	                buttonType = BUTTON_SPACE;
	            } break;
	            case SDLK_UP: {
	                // buttonType = BUTTON_UP;
	            } break;
	            case SDLK_ESCAPE: {
	                buttonType = BUTTON_ESCAPE;
	            } break;
	            case SDLK_DOWN: {
	                // buttonType = BUTTON_DOWN;
	            } break;
	            case SDLK_1: {
	                buttonType = BUTTON_1;
	            } break;
	            case SDLK_2: {
	                buttonType = BUTTON_2;
	            } break;
	            case SDLK_3: {
	                buttonType = BUTTON_3;
	            } break;
	            case SDLK_4: {
	                buttonType = BUTTON_4;
	            } break;
	            case SDLK_5: {
	                buttonType = BUTTON_5;
	            } break;
	            case SDLK_6: {
	                buttonType = BUTTON_6;
	            } break;
	            case SDLK_7: {
	                buttonType = BUTTON_7;
	            } break;
	            case SDLK_8: {
	                buttonType = BUTTON_8;
	            } break;
	            case SDLK_9: {
	                buttonType = BUTTON_9;
	            } break;
	            case SDLK_BACKQUOTE: {
	                buttonType = BUTTON_TILDE;
	            } break;
	            case SDLK_F1: {
	                buttonType = BUTTON_F1;
	            } break;
	             case SDLK_F2: {
	                buttonType = BUTTON_F2;
	            } break;
	             case SDLK_F3: {
	                buttonType = BUTTON_F3;
	            } break;
	             case SDLK_F4: {
	                buttonType = BUTTON_F4;
	            } break;
	             case SDLK_F5: {
	                buttonType = BUTTON_F5;
	            } break;
	             case SDLK_F6: {
	                buttonType = BUTTON_F6;
	            } break;
	             case SDLK_F7: {
	                buttonType = BUTTON_F7;
	            } break;
	            case SDLK_LGUI: {
	                // buttonType = BUTTON_COMMAND;
	            } break;
	            case SDLK_z: {
	                buttonType = BUTTON_Z;
	            } break;
	            case SDLK_r: {
	                buttonType = BUTTON_R;
	            } break;
	            case SDLK_LSHIFT: {
	                // buttonType = BUTTON_SHIFT;
	            } break;
	            case SDLK_LEFT: {
	                if(isDown) {
	                }
	                buttonType = BUTTON_BOARD_LEFT; //NOTE: use key states instead
	            } break;
	            case SDLK_RIGHT: {
	                if(isDown) {
	                }
	                buttonType = BUTTON_BOARD_RIGHT; //NOTE: use key states instead
	            } break;
	            case SDLK_BACKSPACE: {
	                buttonType = BUTTON_BACKSPACE; //NOTE: use key states instead
	            } break;
	        }
	        if(buttonType) {
	            sdlProcessGameKey(&state->gameButtons[buttonType], isDown, repeated);
	        }
	        
	    } else if( event.type == SDL_TEXTINPUT ) {
	        char *inputString = event.text.text;
	        if(state->inputString) {
	        	state->inputString = concat(state->inputString, inputString);
	        } else {
	        	state->inputString = nullTerminate(inputString, strlen(inputString));
	        }

	        // splice(&inputBuffer, inputString, 1);
	        // cursorTimer.value = 0;
	    }
	}
	
	const Uint8* keystates = SDL_GetKeyboardState(NULL);
    
	bool leftArrowIsDown = keystates[SDL_SCANCODE_LEFT] || keystates[SDL_SCANCODE_A];
	bool rightArrowIsDown = keystates[SDL_SCANCODE_RIGHT] || keystates[SDL_SCANCODE_D];
	bool upArrowIsDown = keystates[SDL_SCANCODE_UP] || keystates[SDL_SCANCODE_W];
	bool downArrowIsDown = keystates[SDL_SCANCODE_DOWN] || keystates[SDL_SCANCODE_S];
    
	bool shiftIsDown = keystates[SDL_SCANCODE_LSHIFT];
	bool commandIsDown = keystates[SDL_SCANCODE_LGUI];
	
	sdlProcessGameKey(&state->gameButtons[BUTTON_LEFT], leftArrowIsDown, leftArrowWasDown == leftArrowIsDown);
	sdlProcessGameKey(&state->gameButtons[BUTTON_RIGHT], rightArrowIsDown, rightArrowWasDown == rightArrowIsDown);
	sdlProcessGameKey(&state->gameButtons[BUTTON_DOWN], downArrowIsDown, downArrowWasDown == downArrowIsDown);
	sdlProcessGameKey(&state->gameButtons[BUTTON_UP], upArrowIsDown, upArrowWasDown == upArrowIsDown);
	sdlProcessGameKey(&state->gameButtons[BUTTON_SHIFT], shiftIsDown, shiftWasDown == shiftIsDown);
	sdlProcessGameKey(&state->gameButtons[BUTTON_COMMAND], commandIsDown, commandWasDown == commandIsDown);
	

	int mouseX, mouseY;
	   
	unsigned int mouseState = SDL_GetMouseState(&mouseX, &mouseY);
	   
	float screenRatio = easyOS_getScreenRatio(*screenDim, resolution);
	V2 screenActualSize = v2_scale(screenRatio, resolution);
	 
	float wResidue = (screenDim->x - screenActualSize.x) / 2.0f;
	float yResidue = (screenDim->y - screenActualSize.y) / 2.0f;

	if(stretched) {
		wResidue = 0;
		yResidue = 0;
		screenActualSize = *screenDim;
	}

	float newMouseX = inverse_lerp(wResidue, mouseX, wResidue + screenActualSize.x);
	float newMouseY = inverse_lerp(yResidue, mouseY, yResidue + screenActualSize.y);

	state->mouseP = v2(newMouseX*resolution.x, newMouseY*resolution.y);

	state->mouseP_left_up = v2(state->mouseP.x, -1*state->mouseP.y + resolution.y);
	
	state->mouseP_yUp = v2_minus(v2(state->mouseP.x, -1*state->mouseP.y + resolution.y), v2_scale(0.5f, resolution));
    
	bool leftMouseDown = mouseState & SDL_BUTTON_LMASK;
	sdlProcessGameKey(&state->gameButtons[BUTTON_LEFT_MOUSE], leftMouseDown, leftMouseDown == mouseWasDown);
	
	bool rightMouseDown = mouseState & SDL_BUTTON_RMASK;
	sdlProcessGameKey(&state->gameButtons[BUTTON_RIGHT_MOUSE], rightMouseDown, rightMouseDown == mouseWasDownRight);
	
}

void easyOS_endKeyState(AppKeyStates *keystates) {
	if(keystates->inputString) {
		free(keystates->inputString);
	}
	keystates->inputString = 0;
}