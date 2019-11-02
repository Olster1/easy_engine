
/*
Use example::


easyEditor_initEditor(EasyEditor *e, RenderGroup *group, Font *font, Matrix4 orthoMatrix, V2 fuaxResolution, float screenRelSize, AppKeyStates *keyStates);

while(running) {
	easyEditor_startWindow(e)
	
	static V3 v = {};
	//blah blah
	easyEditor_pushFloat1(e, &v);
	easyEditor_pushFloat2(e, &v, &v);
	easyEditor_pushFloat3(e, &v, &v, &v);

	easyEditor_endWindow(e); //might not actuall need this
	

	easyEditor_endEditorForFrame(e)
}


*/


#define EASY_EDITOR_MARGIN 10
#define EASY_EDITOR_MIN_TEXT_FIELD_WIDTH 100

typedef struct {
	char *fileName;
	int lineNumber;
	int index;
} EasyEditorId;

typedef struct {
	EasyEditorId id;

	char *name;
	Rect2f margin;

	V2 topCorner;
	V2 at;
	float maxX;
} EasyEditorWindow;

typedef struct {
	EasyEditorId id;
	InputBuffer buffer;
} EasyEditorTextField;


typedef struct {
	EasyEditorId id;	
	
	V2 coord;
	float value; //side bar of black to white
} EasyEditorColorSelection;

typedef enum  {
		EASY_EDITOR_INTERACT_NULL,
		EASY_EDITOR_INTERACT_WINDOW,
		EASY_EDITOR_INTERACT_TEXT_FIELD,
		EASY_EDITOR_INTERACT_COLOR_WHEEL,
		EASY_EDITOR_INTERACT_BUTTON,
} EasyEditorInteractType;

typedef struct {
	EasyEditorId id;

	EasyEditorInteractType type;
	void *item;
	bool visitedThisFrame;

	V2 dragOffset; //for dragging interactions
} EasyEditorInteraction;

typedef struct {
	RenderGroup *group;

	int windowCount;
	EasyEditorWindow windows[16];	

	EasyEditorWindow *currentWindow;

	int textFieldCount;
	EasyEditorTextField textFields[64];

	int colorSelectionCount;
	EasyEditorColorSelection colorSelections[16];

	Font *font;

	EasyEditorInteraction interactingWith;

	Matrix4 orthoMatrix;
	V2 fuaxResolution;

	float screenRelSize;

	int bogusItem; //for interactions that have nothing that needs storing

	bool isHovering;

	AppKeyStates *keyStates;

} EasyEditor;

static inline bool easyEditor_isInteracting(EasyEditor *e) {
	return (e->interactingWith.item || e->isHovering);
}


static inline EasyEditorId easyEditor_getNullId() {
	EasyEditorId result = {};
	result.lineNumber = -1;
	result.index = -1;
	result.fileName = 0;
	return result;
}

static inline void easyEditor_stopInteracting(EasyEditor *e) {

	if(e->interactingWith.type == EASY_EDITOR_INTERACT_TEXT_FIELD) {
		EasyEditorTextField *field = (EasyEditorTextField *)e->interactingWith.item;

		field->buffer.length = 0;
		field->buffer.cursorAt = 0;
	}
	e->interactingWith.item = 0;
	e->interactingWith.type = EASY_EDITOR_INTERACT_NULL;
	e->interactingWith.id = easyEditor_getNullId();
}

static inline void easyEditor_initEditor(EasyEditor *e, RenderGroup *group, Font *font, Matrix4 orthoMatrix, V2 fuaxResolution, float screenRelSize, AppKeyStates *keyStates) {
	e->group = group;

	e->windowCount = 0;
	e->currentWindow = 0; //used for the local positions of the windows

	e->font = font;

	e->textFieldCount = 0;

	easyEditor_stopInteracting(e); //null the interacting with

	e->orthoMatrix = orthoMatrix; 
	e->fuaxResolution = fuaxResolution;
	e->screenRelSize = screenRelSize;
	e->keyStates = keyStates;
}




static inline void easyEditor_startInteracting(EasyEditor *e, void *item, int lineNumber, char *fileName, int index, EasyEditorInteractType type) {
	e->interactingWith.item = item;
	e->interactingWith.id.lineNumber = lineNumber;
	e->interactingWith.id.fileName = fileName;
	e->interactingWith.id.index = index;
	e->interactingWith.visitedThisFrame = true;
	e->interactingWith.type = type;
}

static inline bool easyEditor_idEqual(EasyEditorId id, int lineNumber, char *fileName, int index) {
	return (lineNumber == id.lineNumber && cmpStrNull(fileName, id.fileName) && id.index == index);
}

static inline EasyEditorWindow *easyEditor_hasWindow(EasyEditor *e, int lineNumber, char *fileName) {
	EasyEditorWindow *result = 0;
	for(int i = 0; i < e->windowCount && !result; ++i) {
		EasyEditorWindow *w = e->windows + i;
		if(easyEditor_idEqual(w->id, lineNumber, fileName, 0)) {
			result = w;
			break;
		}
	}
	return result;
}

static inline EasyEditorTextField *easyEditor_hasTextField(EasyEditor *e, int lineNumber, char *fileName, int index) {
	EasyEditorTextField *result = 0;
	for(int i = 0; i < e->textFieldCount && !result; ++i) {
		EasyEditorTextField *t = e->textFields + i;
		if(easyEditor_idEqual(t->id, lineNumber, fileName, index)) {
			result = t;
			break;
		}
	}
	return result;
}

static inline EasyEditorTextField *easyEditor_addTextField(EasyEditor *e, int lineNumber, char *fileName, int index) {
	EasyEditorTextField *t = easyEditor_hasTextField(e, lineNumber, fileName, index);
	if(!t) {
		assert(e->textFieldCount < arrayCount(e->textFields));
		t = &e->textFields[e->textFieldCount++];
		t->id.lineNumber = lineNumber;
		t->id.fileName = fileName;
		t->id.index = index;

		t->buffer.length = 0;
		t->buffer.cursorAt = 0;

	}
	return t;
}

static inline EasyEditorColorSelection *easyEditor_hasColorSelector(EasyEditor *e, int lineNumber, char *fileName, int index) {
	EasyEditorColorSelection *result = 0;
	for(int i = 0; i < e->colorSelectionCount && !result; ++i) {
		EasyEditorColorSelection *t = e->colorSelections + i;
		if(easyEditor_idEqual(t->id, lineNumber, fileName, index)) {
			result = t;
			break;
		}
	}
	return result;
}

static inline EasyEditorColorSelection *easyEditor_addColorSelector(EasyEditor *e, int lineNumber, char *fileName, int index) {
	EasyEditorColorSelection *t = easyEditor_hasColorSelector(e, lineNumber, fileName, index);
	if(!t) {
		assert(e->colorSelectionCount < arrayCount(e->colorSelections));
		t = &e->colorSelections[e->colorSelectionCount++];
		t->id.lineNumber = lineNumber;
		t->id.fileName = fileName;
		t->id.index = index;

		t->coord = v2(0, 0);
		t->value = 1.0f;

	}
	return t;
}

#define easyEditor_startWindow(e, name) easyEditor_startWindow_(e, name, __LINE__, __FILE__)
static inline void easyEditor_startWindow_(EasyEditor *e, char *name, int lineNumber, char *fileName) {
	EasyEditorWindow *w = easyEditor_hasWindow(e, lineNumber, fileName);
	if(!w) {
		assert(e->windowCount < arrayCount(e->windows));
		w = &e->windows[e->windowCount++];
		w->id.lineNumber = lineNumber;
		w->id.fileName = fileName;
		w->id.index = 0;

		w->margin = InfinityRect2f();

		w->topCorner = v2(100, 100); //NOTE: Default top corner
	}

	assert(w);
	w->name = name;

	e->currentWindow = w; 
	w->at = w->topCorner; 
	w->maxX = w->topCorner.x;
	w->at.y += EASY_EDITOR_MARGIN;

}



static inline void easyEditor_endWindow(EasyEditor *e) {
	assert(e->currentWindow);
	EasyEditorWindow *w = e->currentWindow;
	Rect2f bounds = rect2fMinMax(w->topCorner.x - EASY_EDITOR_MARGIN, w->topCorner.y - e->font->fontHeight, w->maxX + EASY_EDITOR_MARGIN, w->at.y); 
	renderDrawRect(bounds, 5.1f, COLOR_BLACK, 0, mat4TopLeftToBottomLeft(e->fuaxResolution.y), e->orthoMatrix);

	int lineNumber = w->id.lineNumber;
	char *fileName = w->id.fileName;

	//update the handle

	Rect2f rect = outputTextNoBacking(e->font, w->topCorner.x, w->topCorner.y - e->font->fontHeight - EASY_EDITOR_MARGIN, 1, e->fuaxResolution, w->name, w->margin, COLOR_WHITE, 1, true, e->screenRelSize);
	rect = expandRectf(rect, v2(EASY_EDITOR_MARGIN, EASY_EDITOR_MARGIN));
	rect.maxX = w->maxX + EASY_EDITOR_MARGIN;
	// float barHeight = getDim(titleBounds).y + EASY_EDITOR_MARGIN;
	// Rect2f rect = rect2fMinDimV2(v2_minus(w->topCorner, v2(EASY_EDITOR_MARGIN, barHeight)), v2(getDim(bounds).x, barHeight));
	V4 color = COLOR_BLUE;

	if(inBounds(e->keyStates->mouseP, rect, BOUNDS_RECT) && !e->interactingWith.item) {
		color = COLOR_YELLOW;
		if(wasPressed(e->keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
			easyEditor_startInteracting(e, w, lineNumber, fileName, 0, EASY_EDITOR_INTERACT_WINDOW);

			e->interactingWith.dragOffset = v2_minus(e->keyStates->mouseP, w->topCorner);
		} 
	}

	if(e->interactingWith.item && easyEditor_idEqual(e->interactingWith.id, lineNumber, fileName, 0)) {
		e->interactingWith.visitedThisFrame = true;
		color = COLOR_GREEN;

		if(wasReleased(e->keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
			easyEditor_stopInteracting(e);
		} else {
			w->topCorner = v2_minus(e->keyStates->mouseP, e->interactingWith.dragOffset);
		}
	}

	if(inBounds(e->keyStates->mouseP, bounds, BOUNDS_RECT)) {
		e->isHovering = true;
	} else {
		e->isHovering = false;
	}
	renderDrawRect(rect, 3, color, 0, mat4TopLeftToBottomLeft(e->fuaxResolution.y), e->orthoMatrix);


	//////

	e->currentWindow = 0;
}

#define easyEditor_pushButton(e, name) easyEditor_pushButton_(e, name, __LINE__, __FILE__)
static inline bool easyEditor_pushButton_(EasyEditor *e, char *name, int lineNumber, char *fileName) {
	assert(e->currentWindow);
	bool result = false;


	EasyEditorWindow *w = e->currentWindow;

	w->at.y += EASY_EDITOR_MARGIN;

	V4 color = COLOR_WHITE;

	Rect2f bounds = outputTextNoBacking(e->font, w->at.x, w->at.y, 1, e->fuaxResolution, name, w->margin, COLOR_BLACK, 1, true, e->screenRelSize);
	bounds = expandRectf(bounds, v2(EASY_EDITOR_MARGIN, EASY_EDITOR_MARGIN));
	bool hover = inBounds(e->keyStates->mouseP, bounds, BOUNDS_RECT);
	if(e->interactingWith.item && easyEditor_idEqual(e->interactingWith.id, lineNumber, fileName, 0)) {
		e->interactingWith.visitedThisFrame = true;
		color = COLOR_GREEN;

		if(wasReleased(e->keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
			if(hover) {
				result = true;
			}
			
			easyEditor_stopInteracting(e);	
		}
	} 

	
	if(hover) {
		if(!e->interactingWith.item) {
			color = COLOR_YELLOW;	
		}
		
		if(wasPressed(e->keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
			if(e->interactingWith.item) {
				//NOTE: Clicking on a different cell will swap it
				easyEditor_stopInteracting(e);	
			}
			easyEditor_startInteracting(e, &e->bogusItem, lineNumber, fileName, 0, EASY_EDITOR_INTERACT_BUTTON);
		} 
	}

	renderDrawRect(bounds, 2, color, 0, mat4TopLeftToBottomLeft(e->fuaxResolution.y), e->orthoMatrix);

	w->at.y += getDim(bounds).y + EASY_EDITOR_MARGIN;

	return result;

}

static inline Rect2f easyEditor_renderFloatBox_(EasyEditor *e, float *f, int lineNumber, char *fileName, int index) {
	assert(e->currentWindow);
	EasyEditorWindow *w = e->currentWindow;

	V4 color = COLOR_WHITE;

	EasyEditorTextField *field = easyEditor_addTextField(e, lineNumber, fileName, index);

	if(e->interactingWith.item && easyEditor_idEqual(e->interactingWith.id, lineNumber, fileName, index)) {
		e->interactingWith.visitedThisFrame = true;
		assert(field == (EasyEditorTextField *)e->interactingWith.item);
		if(wasPressed(e->keyStates->gameButtons, BUTTON_ENTER)) {
			float newValue = atof(field->buffer.chars);
			*f = newValue;
			easyEditor_stopInteracting(e);
		} else {
			color = COLOR_GREEN;

			//NOTE: update buffer input
			if(e->keyStates->inputString) {
				splice(&field->buffer, e->keyStates->inputString, true);
			}

			if(wasPressed(e->keyStates->gameButtons, BUTTON_BACKSPACE)) {
				splice(&field->buffer, "1", false);
			}

			if(wasPressed(e->keyStates->gameButtons, BUTTON_BOARD_LEFT) && field->buffer.cursorAt > 0) {
				field->buffer.cursorAt--;
			}
			if(wasPressed(e->keyStates->gameButtons, BUTTON_BOARD_RIGHT) && field->buffer.cursorAt < field->buffer.length) {
				field->buffer.cursorAt++;
			}
			////
		}
	} else {
		//Where we first put the string in the buffer 
		sprintf(field->buffer.chars, "%f", *f); 
	} 

	char *string = field->buffer.chars;
	if(strlen(string) == 0) {
		string = " ";
	}
	Rect2f bounds = outputTextNoBacking(e->font, w->at.x, w->at.y, 1, e->fuaxResolution, string, w->margin, COLOR_BLACK, 1, true, e->screenRelSize);
	if(getDim(bounds).x < EASY_EDITOR_MIN_TEXT_FIELD_WIDTH) {
		bounds.maxX = bounds.minX + EASY_EDITOR_MIN_TEXT_FIELD_WIDTH;
	}
	bounds = expandRectf(bounds, v2(EASY_EDITOR_MARGIN, EASY_EDITOR_MARGIN)); 
	if(inBounds(e->keyStates->mouseP, bounds, BOUNDS_RECT)) {
		if(!e->interactingWith.item) {
			color = COLOR_YELLOW;	
		}
		
		if(wasPressed(e->keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
			if(e->interactingWith.item) {
				//NOTE: Clicking on a different cell will swap it
				easyEditor_stopInteracting(e);	
			}
			easyEditor_startInteracting(e, field, lineNumber, fileName, index, EASY_EDITOR_INTERACT_TEXT_FIELD);
		} 
	}

	renderDrawRect(bounds, 2.0f, color, 0, mat4TopLeftToBottomLeft(e->fuaxResolution.y), e->orthoMatrix);

	return bounds;
}

#define easyEditor_pushFloat1(e, name, f0) easyEditor_pushFloat1_(e, name, f0, __LINE__, __FILE__)
static inline void easyEditor_pushFloat1_(EasyEditor *e, char *name, float *f, int lineNumber, char *fileName) {
	EasyEditorWindow *w = e->currentWindow;
	assert(w);
	if(name && strlen(name) > 0) {
		Rect2f rect = outputTextNoBacking(e->font, w->at.x, w->at.y, 1, e->fuaxResolution, name, w->margin, COLOR_WHITE, 1, true, e->screenRelSize);
		w->at.x += getDim(rect).x + 3*EASY_EDITOR_MARGIN;
	}

	Rect2f bounds = easyEditor_renderFloatBox_(e, f, lineNumber, fileName, 0);

	if(w->maxX < w->at.x) {
		w->maxX = w->at.x;
	}
	w->at.x = w->topCorner.x;
	w->at.y += getDim(bounds).y + EASY_EDITOR_MARGIN;

}

#define easyEditor_pushFloat2(e, name, f0, f1) easyEditor_pushFloat2_(e, name, f0, f1, __LINE__, __FILE__)
static inline void easyEditor_pushFloat2_(EasyEditor *e, char *name, float *f0, float *f1, int lineNumber, char *fileName) {
	EasyEditorWindow *w = e->currentWindow;
	assert(w);

	if(name && strlen(name) > 0) {
		Rect2f rect = outputTextNoBacking(e->font, w->at.x, w->at.y, 1, e->fuaxResolution, name, w->margin, COLOR_WHITE, 1, true, e->screenRelSize);
		w->at.x += getDim(rect).x + 3*EASY_EDITOR_MARGIN;
	}

	Rect2f bounds = easyEditor_renderFloatBox_(e, f0, lineNumber, fileName, 0);

	w->at.x += getDim(bounds).x + EASY_EDITOR_MARGIN;
	bounds = easyEditor_renderFloatBox_(e, f1, lineNumber, fileName, 1);
	w->at.x += getDim(bounds).x + EASY_EDITOR_MARGIN;

	if(w->maxX < w->at.x) {
		w->maxX = w->at.x;
	}
	w->at.x = w->topCorner.x;
	w->at.y += getDim(bounds).y + EASY_EDITOR_MARGIN;
}


#define easyEditor_pushFloat3(e, name, f0, f1, f2) easyEditor_pushFloat3_(e, name, f0, f1, f2, __LINE__, __FILE__)
static inline void easyEditor_pushFloat3_(EasyEditor *e, char *name, float *f0, float *f1, float *f2, int lineNumber, char *fileName) {
	EasyEditorWindow *w = e->currentWindow;
	assert(w);
	
	if(name && strlen(name) > 0) {
		Rect2f rect = outputTextNoBacking(e->font, w->at.x, w->at.y, 1, e->fuaxResolution, name, w->margin, COLOR_WHITE, 1, true, e->screenRelSize);
		w->at.x += getDim(rect).x + 3*EASY_EDITOR_MARGIN;
	}

	Rect2f bounds = easyEditor_renderFloatBox_(e, f0, lineNumber, fileName, 0);

	w->at.x += getDim(bounds).x + EASY_EDITOR_MARGIN;
	bounds = easyEditor_renderFloatBox_(e, f1, lineNumber, fileName, 1);
	w->at.x += getDim(bounds).x + EASY_EDITOR_MARGIN;
	bounds = easyEditor_renderFloatBox_(e, f2, lineNumber, fileName, 2);
	w->at.x += getDim(bounds).x + EASY_EDITOR_MARGIN;

	if(w->maxX < w->at.x) {
		w->maxX = w->at.x;
	}

	w->at.x = w->topCorner.x;
	w->at.y += getDim(bounds).y + EASY_EDITOR_MARGIN;
}

#define easyEditor_pushBool(e, name, b0) easyEditor_pushBool_(e, name, b0, __LINE__, __FILE__)
static inline void easyEditor_pushBool_(EasyEditor *e, char *name, bool *b, int lineNumber, char *fileName) {


	
}

static inline void easyEditor_endEditorForFrame(EasyEditor *e) {
	if(!e->interactingWith.visitedThisFrame) {
		easyEditor_stopInteracting(e);
	}

	e->interactingWith.visitedThisFrame = false;
	
}


#define easyEditor_pushColor(e, name, color) easyEditor_pushColor_(e, name, color, __LINE__, __FILE__)
static inline void easyEditor_pushColor_(EasyEditor *e, char *name, V4 *color, int lineNumber, char *fileName) {
	EasyEditorWindow *w = e->currentWindow;
	assert(w);

	EasyEditorColorSelection *field = easyEditor_addColorSelector(e, lineNumber, fileName, 0);

	Rect2f rect = rect2f(0, 0, 30, 10);
	if(name && strlen(name) > 0) {
		rect = outputTextNoBacking(e->font, w->at.x, w->at.y, 1, e->fuaxResolution, name, w->margin, COLOR_WHITE, 1, true, e->screenRelSize);
		w->at.x += getDim(rect).x + 3*EASY_EDITOR_MARGIN;
	}

	V2 at = v2_minus(w->at, v2(0, getDim(rect).y));
	Rect2f bounds = rect2fMinDimV2(at, getDim(rect));
	renderDrawRect(bounds, 2, *color, 0, mat4TopLeftToBottomLeft(e->fuaxResolution.y), e->orthoMatrix);
	w->at.x += getDim(bounds).x;

	w->at.y += getDim(bounds).y;


	float advanceY = 0;
	if(e->interactingWith.item && easyEditor_idEqual(e->interactingWith.id, lineNumber, fileName, 0)) {

		e->interactingWith.visitedThisFrame = true;
		assert(field == (EasyEditorColorSelection *)e->interactingWith.item);
		if(wasPressed(e->keyStates->gameButtons, BUTTON_ENTER)) {
			easyEditor_stopInteracting(e);
		}

		V2 dim = v2(150, 150);
		V3 center = v3(w->at.x + 0.5f*dim.x, w->at.y + 0.5f*dim.y, 1);
			
		//NOTE(ol): Take snapshot of state
		BlendFuncType tempType = globalRenderGroup->blendFuncType;
		//
		//change to other type of blend
		setBlendFuncType(globalRenderGroup, BLEND_FUNC_STANDARD_NO_PREMULTIPLED_ALPHA);
		//
		renderColorWheel(center, dim, 2.0f,mat4TopLeftToBottomLeft(e->fuaxResolution.y), e->orthoMatrix, mat4());
		
		//restore the state
		setBlendFuncType(globalRenderGroup, tempType);
		//

		advanceY = dim.y + e->font->fontHeight;

	} else {
		
		if(inBounds(e->keyStates->mouseP, bounds, BOUNDS_RECT)) {
			if(!e->interactingWith.item) {
				// color = COLOR_YELLOW;	
			}
			
			if(wasPressed(e->keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
				if(e->interactingWith.item) {
					//NOTE: Clicking on a different cell will swap it
					easyEditor_stopInteracting(e);	
				}
				//update the field we are interacting with
				easyEditor_startInteracting(e, field, lineNumber, fileName, 0, EASY_EDITOR_INTERACT_COLOR_WHEEL);
			} 
		}


	}
	

	if(w->maxX < w->at.x) {
		w->maxX = w->at.x;
	}

	w->at.x = w->topCorner.x;
	w->at.y += advanceY + EASY_EDITOR_MARGIN;
}


