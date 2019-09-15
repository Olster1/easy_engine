typedef enum {
	EASY_CONSOLE_CLOSED,
	EASY_CONSOLE_OPEN_MID,
	EASY_CONSOLE_OPEN_LARGE,

	///
	EASY_CONSOLE_COUNT
} EasyConsoleState;

typedef struct {
	ButtonType hotkey;
	EasyConsoleState state;

	Timer expandTimer;

	InputBuffer buffer;

	char lastString[INPUT_BUFFER_SIZE];

	EasyTokenizer tokenizer;
} EasyConsole;

inline void easyConsole_initConsole(EasyConsole *c, ButtonType hotkey) {
	c->hotkey = hotkey;
	c->state = EASY_CONSOLE_CLOSED;
	c->expandTimer = initTimer(0.2f, false);
	turnTimerOff(&c->expandTimer);
	c->buffer.length = 0;
	c->buffer.cursorAt = 0;
	c->buffer.chars[0] = '\0';

	c->tokenizer.parsing = false;


}

inline void easyConsole_setHotKey(EasyConsole *c, ButtonType hotkey) {
	c->hotkey = hotkey;
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
		case EASY_CONSOLE_OPEN_LARGE: {
			height = smoothStep01(0.3f*resolution.y, v, 0.8f*resolution.y);
		} break;
		default: {
			assert(!"shouldn't be here");
		} break;
	}

	

	bool result = wasPressed(keyStates->gameButtons, BUTTON_ENTER);
	if(height != 0) {
		if(!result) {
			if(keyStates->inputString && !wasPressed) {
				splice(&c->buffer, keyStates->inputString, true);
			}

			if(wasPressed(keyStates->gameButtons, BUTTON_BACKSPACE)) {
				splice(&c->buffer, "1", false);
			}

			if(wasPressed(keyStates->gameButtons, BUTTON_LEFT) && c->buffer.cursorAt > 0) {
				c->buffer.cursorAt--;
			}
			if(wasPressed(keyStates->gameButtons, BUTTON_RIGHT) && c->buffer.cursorAt < c->buffer.length) {
				c->buffer.cursorAt++;
			}
		}
		// renderEnableDepthTest(RenderGroup *group);
		renderDrawRectCenterDim(v3(0.5f*resolution.x, 0.5f*height, 1), v2(resolution.x, height), COLOR_GREY, 0, mat4TopLeftToBottomLeft(resolution.y), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));
	 	outputText_with_cursor(&globalDebugFont, 0, height, 0.5f, resolution, c->buffer.chars, rect2fMinMax(0, 0, 1000, 1000), COLOR_WHITE, 1, c->buffer.cursorAt, COLOR_YELLOW, true, relativeSize);
	 }
	 // outputText(&globalDebugFont, 0, height, -1, "hey there");


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
inline void easyConsole_parseDefault(EasyConsole *c) {

}

