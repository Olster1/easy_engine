typedef struct {
	bool isBold;
	bool isItalic;
} EasyTextGlyph;

typedef struct {
	InfiniteAlloc chars; //has the chars
	// InfiniteAlloc styles; //has the info
	int cursorAt;
	Rect2f margin;
	float zAt;
	V4 color;
	V4 hoverColor;
	int interactIndex;
	Rect2f padding;
	Timer cursorTimer;

} EasyTextBox;

static inline V2 easyWrite_mapMouseToPage(AppKeyStates *keyStates, Rect2f pageOnScreen, V2 pageResolution) {
	V2 result = keyStates->mouseP;

	result.x = inverse_lerp(pageOnScreen.min.x, result.x, pageOnScreen.max.x);
	result.y = inverse_lerp(pageOnScreen.min.y, result.y, pageOnScreen.max.y);

	result.x = result.x*pageResolution.x;
	result.y = result.y*pageResolution.y;

	return result;
}

static inline void easyWrite_initTextBox(EasyTextBox *box, Rect2f margin, float zAt, V4 color) {
	box->cursorAt = 0;
	box->chars = initInfinteAlloc(char);
	// box->styles = initInfinteAlloc(EasyTextGlyph);
	box->margin = margin;
	box->zAt = zAt;
	box->color = color;
	box->padding = rect2f(0, 0, 0, 0);
	box->cursorTimer = initTimer(1.0f, false);
	box->interactIndex = -1;
	turnTimerOn(&box->cursorTimer);
}

static inline void easyWrite_spliceString(EasyTextBox *box, char *string, bool addString) {
	if(string) {
		//copy the ones we are going to override
		InfiniteAlloc tempChars = initInfinteAlloc(char);
		// InfiniteAlloc tempStyles = initInfinteAlloc(EasyTextGlyph);

		u8 *sourceToCopy = ((u8 *)box->chars.memory) + (box->cursorAt*box->chars.sizeOfMember);
		addElementInifinteAllocWithCount_(&tempChars, sourceToCopy, box->chars.count - box->cursorAt);

		// u8 *sourceStylesToCopy = ((u8 *)box->styles.memory) + (box->cursorAt*box->styles.sizeOfMember);
		// addElementInifinteAllocWithCount_(&tempStyles, sourceStylesToCopy, box->styles.count - box->cursorAt);

		//copy the new ones in
		int stringLen = strlen(string);
	    if(addString) { //adding
	    	box->chars.count = box->cursorAt;
	        addElementInifinteAllocWithCount_(&box->chars, string, stringLen);
	        box->cursorAt += stringLen;
	        assert(box->cursorAt <= box->chars.count);
	    } else {
	    	if(box->cursorAt > 0) {
	    		assert(box->cursorAt <= box->chars.count);
				box->cursorAt = max(box->cursorAt - stringLen, 0);
				box->chars.count = box->cursorAt;
			}
	    }
		
		//put back the ones that we took out before
		addElementInifinteAllocWithCount_(&box->chars, tempChars.memory, tempChars.count);
		// addElementInifinteAllocWithCount_(&box->styles, tempStyles.memory, tempStyles.count);

		char nullTerminatorStr = '\0';
		addElementInifinteAllocWithCount_(&box->chars, &nullTerminatorStr, 1);
		box->chars.count--; //don't actually count hte null terminator

		releaseInfiniteAlloc(&tempChars);
		// releaseInfiniteAlloc(&tempStyles);
	}
}

//Put the string after the cursor in a new buffer, then put the new string in, then put the old one back in
static inline void easyWrite_UpdateTextBox(EasyTextBox *box, AppKeyStates *keystates, Rect2f pageOnScreen, V2 pageResolution) {
	char *string = keystates->inputString;
	if(isDown(keystates->gameButtons, BUTTON_BOARD_LEFT)) {
		box->cursorAt = max(box->cursorAt - 1, 0);
	} 
	if(isDown(keystates->gameButtons, BUTTON_BOARD_RIGHT)) {
		box->cursorAt = min(box->cursorAt + 1, max(box->chars.count, 0));

	}

	easyWrite_spliceString(box, keystates->inputString, true);
	if(isDown(keystates->gameButtons, BUTTON_BACKSPACE)) {
		easyWrite_spliceString(box, "1", false);
	}

	V2 relMouseP = easyWrite_mapMouseToPage(keystates, pageOnScreen, pageResolution);
	// printf("%f %f\n", relMouseP.x, relMouseP.y);
	renderDrawRectCenterDim(v2ToV3(relMouseP, -0.4f), v2(30, 30), COLOR_BLACK, 0, mat4TopLeftToBottomLeft(pageResolution.y), OrthoMatrixToScreen_BottomLeft(pageResolution.x, pageResolution.y));

	box->hoverColor = COLOR_WHITE;
	if(wasReleased(keystates->gameButtons, BUTTON_LEFT_MOUSE)) {
		box->interactIndex = -1;
	}
	int hotIndex = -1;
	V2 dim = v2(30, 30);
	Rect2f corners[4] = {
						rect2fCenterDim(box->margin.min.x, box->margin.min.y, dim.x, dim.y),
						 rect2fCenterDim(box->margin.min.x, box->margin.max.y, dim.x, dim.y),
						 rect2fCenterDim(box->margin.max.x, box->margin.min.y, dim.x, dim.y),
						 rect2fCenterDim(box->margin.max.x, box->margin.max.y, dim.x, dim.y)
						};
	bool mousePressed = wasPressed(keystates->gameButtons, BUTTON_LEFT_MOUSE);

	if(inRect(relMouseP, corners[0])) {
		hotIndex = 0;
	} else if(inRect(relMouseP, corners[1])) {
		hotIndex = 1;
	} else if(inRect(relMouseP, corners[2])) {
		hotIndex = 2;
	} else if(inRect(relMouseP, corners[3])) {
		hotIndex = 3;
	} else if(mousePressed) {
		//move entire text box

	} 
	if(inRect(relMouseP, box->margin)) {
		box->hoverColor = COLOR_BLUE;
		
	}

	//render handles
	for(int i = 0; i < 4; i++) {
		V4 color = COLOR_BLACK;
		if(box->interactIndex == i) {
			color = COLOR_BLUE;
		} else if(i == hotIndex) {
			if(mousePressed) {
				box->interactIndex = i;
				color = COLOR_BLUE;
			} else {
				color = COLOR_RED;
			}
		} 
		renderDrawRectCenterDim(v2ToV3(getCenter(corners[i]), box->zAt - 0.1f), getDim(corners[i]), color, 0, mat4TopLeftToBottomLeft(pageResolution.y), OrthoMatrixToScreen_BottomLeft(pageResolution.x, pageResolution.y));
	}

	if(box->interactIndex >= 0) {
		switch(box->interactIndex) {
			case 0: {
				box->margin.min.x = relMouseP.x;
				box->margin.min.y = relMouseP.y;
			} break;
			case 1: {
				box->margin.min.x = relMouseP.x;
				box->margin.max.y = relMouseP.y;
			} break;
			case 2: {
				box->margin.max.x = relMouseP.x;
				box->margin.min.y = relMouseP.y;
			} break;
			case 3: {
				box->margin.max.x = relMouseP.x;
				box->margin.max.y = relMouseP.y;
			} break;
			default: {

			}
		}
		
		
	}
}

static inline void easyWrite_RenderTextBox(EasyTextBox *box, V2 resolution, AppSetupInfo *setupInfo, Font *font, float dt) {
	char *textToWrite = "";
	if(box->chars.count > 0) {
		textToWrite = (char *)box->chars.memory;
	}
	TimerReturnInfo timeInfo = updateTimer(&box->cursorTimer, dt);
	if(timeInfo.finished) {
		turnTimerOn(&box->cursorTimer);
	}

	
	V4 cursorColor = smoothStep00V4(COLOR_NULL, timeInfo.canonicalVal, COLOR_BLACK);
	// error_printFloat4("VALUES: ", cursorColor.E);

	Rect2f adjustMargin = reevalRect2f(box->margin);

	outputText_with_cursor(font, adjustMargin.min.x, adjustMargin.min.y + font->fontHeight, box->zAt, resolution, textToWrite, adjustMargin, COLOR_BLACK, 1, box->cursorAt, cursorColor, true, setupInfo->screenRelativeSize);
	//backdrop
	V2 center = getCenter(adjustMargin);
	renderDrawRectCenterDim(v2ToV3(center, box->zAt - 0.1f), getDim(adjustMargin), v4_hadamard(box->hoverColor, box->color), 0, mat4TopLeftToBottomLeft(resolution.y), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));
}


