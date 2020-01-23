typedef enum {
	EASY_CONSOLE_CLOSED,
	EASY_CONSOLE_OPEN_MID,
	// EASY_CONSOLE_OPEN_LARGE,

	///
	EASY_CONSOLE_COUNT
} EasyConsoleState;

typedef struct {
	ButtonType hotkey;
	EasyConsoleState state;

	Timer expandTimer;

	InputBuffer buffer;

	char lastString[INPUT_BUFFER_SIZE];

	int streamSize;
	int streamAt;
	char *bufferStream; //allocated

	EasyTokenizer tokenizer;
} EasyConsole;

static EasyConsole *DEBUG_globalEasyConsole;

inline void easyConsole_initConsole(EasyConsole *c, ButtonType hotkey) {
	c->hotkey = hotkey;
	c->state = EASY_CONSOLE_CLOSED;
	c->expandTimer = initTimer(0.2f, false);
	turnTimerOff(&c->expandTimer);
	c->buffer.length = 0;
	c->buffer.cursorAt = 0;
	c->buffer.chars[0] = '\0';

	c->tokenizer.parsing = false;

	c->streamAt = 0;
	c->streamSize = 10000;
	c->bufferStream = pushArray(&globalLongTermArena, c->streamSize, char);
	c->bufferStream[0] = '\0';
}

inline void easyConsole_addToStream(EasyConsole *c, char *toAdd) {
	char *at = toAdd;

	bool readyToBreak = false;
	bool copying = true;
	while(copying) {
		if(c->streamAt >= c->streamSize) {
			assert(c->streamAt == c->streamSize);
			// @Speed
			int baseIndex = 0;
			int offset = 2048;
			for(int i = offset; i < c->streamAt; ++i) {
			    c->bufferStream[baseIndex++] = c->bufferStream[i]; 
			}
			c->bufferStream[baseIndex++] = '\0';
			assert(baseIndex <= c->streamSize);
			c->streamAt -= offset;
		}
		if(readyToBreak) {
			assert(c->streamAt < c->streamSize);
			c->bufferStream[c->streamAt++] = '\n';
			copying = false;
			break;
		}

		assert(*at != '\0');
		if(copying) {
			assert(c->streamAt < c->streamSize);
			c->bufferStream[c->streamAt++] =  *at;
			at++;

			if(*at == '\0') {
				readyToBreak = true;
			}
		}

		
	}
}


static inline void easyConsole_pushFloat(EasyConsole *c, float i) {
	char buf[32];
	sprintf(buf, "%f", i);
	easyConsole_addToStream(c, buf);
}

static inline void easyConsole_pushInt(EasyConsole *c, int i) {
	char buf[32];
	sprintf(buf, "%d", i);
	easyConsole_addToStream(c, buf);
}

inline void easyConsole_setHotKey(EasyConsole *c, ButtonType hotkey) {
	c->hotkey = hotkey;
}

inline bool easyConsole_isOpen(EasyConsole *c) {
	return (c->state != EASY_CONSOLE_CLOSED);
	
}

inline bool easyConsole_update(EasyConsole *c, AppKeyStates *keyStates, float dt, V2 resolution, float relativeSize) {
	bool wasPressed = false;
	if(wasPressed(keyStates->gameButtons, c->hotkey)) {
		c->state = (EasyConsoleState)((int)c->state + 1);
		if(c->state >= (int)EASY_CONSOLE_COUNT) {
			c->state = EASY_CONSOLE_CLOSED;
		}
		turnTimerOn(&c->expandTimer);
		wasPressed = true;
	}

	float v = 1.0f;
	if(isOn(&c->expandTimer)) {
		TimerReturnInfo info = updateTimer(&c->expandTimer, dt);
		v = info.canonicalVal;
		if(info.finished) {
			turnTimerOff(&c->expandTimer);
		}
	}
	float height = 0;//
	switch(c->state) {
		case EASY_CONSOLE_CLOSED: {
			height = smoothStep01(0.8f*resolution.y, v, 0);
		} break;
		case EASY_CONSOLE_OPEN_MID: {
			height = smoothStep01(0, v, 0.3f*resolution.y);
		} break;
		// case EASY_CONSOLE_OPEN_LARGE: {
		// 	height = smoothStep01(0.3f*resolution.y, v, 0.8f*resolution.y);
		// } break;
		default: {
			assert(!"shouldn't be here");
		} break;
	}

	bool result = false;

	if(c->state != EASY_CONSOLE_CLOSED) {
		result = wasPressed(keyStates->gameButtons, BUTTON_ENTER);
		if(height != 0) {
			if(!result) {
				if(keyStates->inputString && !wasPressed) {
					splice(&c->buffer, keyStates->inputString, true);
				}

				if(wasPressed(keyStates->gameButtons, BUTTON_BACKSPACE)) {
					splice(&c->buffer, "1", false);
				}

				if(wasPressed(keyStates->gameButtons, BUTTON_BOARD_LEFT) && c->buffer.cursorAt > 0) {
					c->buffer.cursorAt--;
				}
				if(wasPressed(keyStates->gameButtons, BUTTON_BOARD_RIGHT) && c->buffer.cursorAt < c->buffer.length) {
					c->buffer.cursorAt++;
				}
			}
			// renderEnableDepthTest(RenderGroup *group);
			renderDrawRectCenterDim(v3(0.5f*resolution.x, 0.5f*height, 2), v2(resolution.x, height), COLOR_GREY, 0, mat4TopLeftToBottomLeft(resolution.y), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));
		 	outputText_with_cursor(&globalDebugFont, 0, height, 1.5f, resolution, c->buffer.chars, rect2fMinMax(0, height - globalDebugFont.fontHeight, resolution.x, height + 0.4f*globalDebugFont.fontHeight), COLOR_WHITE, 1, c->buffer.cursorAt, COLOR_YELLOW, true, relativeSize);
			
			V2 bounds = getBounds(c->bufferStream, rect2fMinMax(0, 0, resolution.x, height - globalDebugFont.fontHeight), &globalDebugFont, 1, resolution, relativeSize);
			outputText(&globalDebugFont, 0, height - globalDebugFont.fontHeight - bounds.y, 1.5f, resolution, c->bufferStream, rect2fMinMax(0, 0, resolution.x, height), COLOR_WHITE, 1, true, relativeSize);
		}
	}

	if(result) {
		nullTerminateBuffer(c->lastString, c->buffer.chars, c->buffer.length);
		c->buffer.length = 0;
		c->buffer.cursorAt = 0;
		c->buffer.chars[0] = '\0';

		c->tokenizer = lexBeginParsing(c->lastString, (EasyLexOptions)(EASY_LEX_OPTION_EAT_WHITE_SPACE | EASY_LEX_DONT_EAT_SLASH_COMMENTS));
	}
	return result;
}

inline EasyToken easyConsole_getNextToken(EasyConsole *console) {
	return lexGetNextToken(&console->tokenizer);
}

inline EasyToken easyConsole_seeNextToken(EasyConsole *console) {
	return lexSeeNextToken(&console->tokenizer);
}

/*
	
	Short cuts: 

	fly - fly the camera around

	camMoveXY - only move in the XY coordinates

	camRotate - toggle camera rotating off/on
*/

inline void easyConsole_parseDefault(EasyConsole *c, EasyToken token) {
	if(stringsMatchNullN("fly", token.at, token.size)) {
        DEBUG_global_IsFlyMode = !DEBUG_global_IsFlyMode;
    } else if(stringsMatchNullN("camMoveXY", token.at, token.size)) {
        DEBUG_global_CameraMoveXY = !DEBUG_global_CameraMoveXY;
    } else if(stringsMatchNullN("pause", token.at, token.size)) {
        DEBUG_global_PauseGame = !DEBUG_global_PauseGame;
    } else if(stringsMatchNullN("camRotate", token.at, token.size)) {
        DEBUG_global_CamNoRotate = !DEBUG_global_CamNoRotate;
    } else if(stringsMatchNullN("help", token.at, token.size)) {
        easyConsole_addToStream(c, "camMoveXY");
        easyConsole_addToStream(c, "fly");
        easyConsole_addToStream(c, "camRotate");
         easyConsole_addToStream(c, "pause");
    } else if(stringsMatchNullN("command", token.at, token.size)) {
    	STARTUPINFO startUpInfo = {};
    	_PROCESS_INFORMATION lpProcessInformation;

    	if(!CreateProcessA(
    	   "c:\\windows\\system32\\cmd.exe",
    	  "/c mkdir",
    	  NULL,
    	  NULL,
    	  FALSE,
    	  NORMAL_PRIORITY_CLASS,
    	  NULL,
    	  "C://",
    	  &startUpInfo,
    	  &lpProcessInformation
    	)) {
    		easyConsole_addToStream(c, "parameter not understood");
    	}
    } else {
    	easyConsole_addToStream(c, "parameter not understood");
    } 
	
}

