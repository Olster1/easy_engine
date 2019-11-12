
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

typedef enum  {
		EASY_EDITOR_INTERACT_NULL,
		EASY_EDITOR_INTERACT_WINDOW,
		EASY_EDITOR_INTERACT_TEXT_FIELD,
		EASY_EDITOR_INTERACT_COLOR_WHEEL,
		EASY_EDITOR_INTERACT_SLIDER,
		EASY_EDITOR_INTERACT_BUTTON,
} EasyEditorInteractType;

typedef struct {
	EasyEditorId id;

	EasyEditorInteractType type;

	union {
		struct { //window state
			char *name;
			Rect2f margin;

			V2 topCorner;
			V2 at;
			float maxX;
		};
		struct { //color selection state
			bool isOpen;
			V2 coord; //from the center
			float value; //side bar of black to white
		};
		struct { //text field state
			InputBuffer buffer;	
		};
		struct { //range 
			float min;
			float max;
		};
	};
} EasyEditorState;


typedef struct {
	EasyEditorId id;

	EasyEditorInteractType type;

	EasyEditorState *item;
	
	bool visitedThisFrame;

	V2 dragOffset; //for dragging interactions
} EasyEditorInteraction;

typedef struct {
	RenderGroup *group;

	int stateCount;
	EasyEditorState states[128];	

	EasyEditorState *currentWindow;

	Font *font;

	EasyEditorInteraction interactingWith;

	Matrix4 orthoMatrix;
	V2 fuaxResolution;

	float screenRelSize;

	EasyEditorState bogusItem; //for interactions that have nothing that needs storing

	bool isHovering;

	AppKeyStates *keyStates;

} EasyEditor;

static inline bool easyEditor_isInteracting(EasyEditor *e) {
	return (e->interactingWith.item);
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
		EasyEditorState *field = e->interactingWith.item;

		field->buffer.length = 0;
		field->buffer.cursorAt = 0;
	}
	e->interactingWith.item = 0;
	e->interactingWith.type = EASY_EDITOR_INTERACT_NULL;
	e->interactingWith.id = easyEditor_getNullId();
}

static inline void easyEditor_initEditor(EasyEditor *e, RenderGroup *group, Font *font, Matrix4 orthoMatrix, V2 fuaxResolution, float screenRelSize, AppKeyStates *keyStates) {
	e->group = group;

	e->stateCount = 0;
	e->currentWindow = 0; //used for the local positions of the windows

	e->font = font;

	easyEditor_stopInteracting(e); //null the interacting with

	e->orthoMatrix = orthoMatrix; 
	e->fuaxResolution = fuaxResolution;
	e->screenRelSize = screenRelSize;
	e->keyStates = keyStates;
}

static inline void easyEditor_startInteracting(EasyEditor *e, EasyEditorState *item, int lineNumber, char *fileName, int index, EasyEditorInteractType type) {
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

static inline EasyEditorState *easyEditor_hasState(EasyEditor *e, int lineNumber, char *fileName, int index, EasyEditorInteractType type) {
	EasyEditorState *result = 0;
	for(int i = 0; i < e->stateCount && !result; ++i) {
		EasyEditorState *w = e->states + i;
		if(easyEditor_idEqual(w->id, lineNumber, fileName, index)) {
			result = w;
			assert(result->type == type);
			break;
		}
	}
	return result;
}

static inline EasyEditorState *addState(EasyEditor *e, int lineNumber, char *fileName, int index, EasyEditorInteractType type) {
	assert(e->stateCount < arrayCount(e->states));
	EasyEditorState *t = &e->states[e->stateCount++];
	t->id.lineNumber = lineNumber;
	t->id.fileName = fileName;
	t->id.index = index;
	t->type = type;

	return t;
}

static inline EasyEditorState *easyEditor_addTextField(EasyEditor *e, int lineNumber, char *fileName, int index) {
	EasyEditorState *t = easyEditor_hasState(e, lineNumber, fileName, index, EASY_EDITOR_INTERACT_TEXT_FIELD);
	if(!t) {
		t = addState(e, lineNumber, fileName, index, EASY_EDITOR_INTERACT_TEXT_FIELD);

		t->buffer.length = 0;
		t->buffer.cursorAt = 0;

	}
	return t;
}

static inline EasyEditorState *easyEditor_addColorSelector(EasyEditor *e, V4 color, int lineNumber, char *fileName, int index) {
	EasyEditorState *t = easyEditor_hasState(e, lineNumber, fileName, index, EASY_EDITOR_INTERACT_COLOR_WHEEL);
	if(!t) {
		t = addState(e, lineNumber, fileName, index, EASY_EDITOR_INTERACT_COLOR_WHEEL);

		t->isOpen = false;
	

		//NOTE(ollie): Set the original color		
		EasyColor_HSV hsv = easyColor_rgbToHsv(color.x, color.y, color.z);

		float rads = easyMath_degreesToRadians(hsv.h);
		t->coord = v2_scale(hsv.s, v2(cos(rads), sin(rads)));
		t->value = hsv.v;
	}
	return t;
}

typedef enum {
	EASY_SLIDER_HORIZONTAL, 
	EASY_SLIDER_VERTICAL, 
} EasyEditor_SliderType;

static inline EasyEditorState *easyEditor_addSlider(EasyEditor *e, int lineNumber, char *fileName, int index, float min, float max) {
	EasyEditorState *t = easyEditor_hasState(e, lineNumber, fileName, index, EASY_EDITOR_INTERACT_SLIDER);
	if(!t) {
		t = addState(e, lineNumber, fileName, index, EASY_EDITOR_INTERACT_SLIDER);

		t->min = min;
		t->max = max;
	}
	return t;
}

#define easyEditor_startWindow(e, name) easyEditor_startWindow_(e, name, __LINE__, __FILE__)
static inline void easyEditor_startWindow_(EasyEditor *e, char *name, int lineNumber, char *fileName) {
	EasyEditorState *w = easyEditor_hasState(e, lineNumber, fileName, 0, EASY_EDITOR_INTERACT_WINDOW);
	if(!w) {
		w = addState(e, lineNumber, fileName, 0, EASY_EDITOR_INTERACT_WINDOW);

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
	EasyEditorState *w = e->currentWindow;
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


	EasyEditorState *w = e->currentWindow;

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
	EasyEditorState *w = e->currentWindow;

	V4 color = COLOR_WHITE;

	EasyEditorState *field = easyEditor_addTextField(e, lineNumber, fileName, index);

	if(e->interactingWith.item && easyEditor_idEqual(e->interactingWith.id, lineNumber, fileName, index)) {
		e->interactingWith.visitedThisFrame = true;
		assert(field == (EasyEditorState *)e->interactingWith.item);
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
	EasyEditorState *w = e->currentWindow;
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
	EasyEditorState *w = e->currentWindow;
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
	EasyEditorState *w = e->currentWindow;
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

#define easyEditor_pushSlider(e, name, value, min, max) easyEditor_pushSlider_(e, name, value, min, max, __LINE__, __FILE__, 0)
static inline void easyEditor_pushSlider_(EasyEditor *e, char *name, float *value, float min, float max, int lineNumber, char *fileName, int index) {
	EasyEditorState *w = e->currentWindow;
	assert(w);

///////////////////////*************Add the slider state************////////////////////
	EasyEditorState *field = easyEditor_addSlider(e, lineNumber, fileName, index, min, max);

	Rect2f rect = rect2f(0, 0, 0, e->font->fontHeight);
	if(name && strlen(name) > 0) {
		rect = outputTextNoBacking(e->font, w->at.x, w->at.y, 1, e->fuaxResolution, name, w->margin, COLOR_WHITE, 1, true, e->screenRelSize);
		w->at.x += getDim(rect).x + 3*EASY_EDITOR_MARGIN;
	}

	float sliderX = 300;
	float sliderY = 20;
	Rect2f sliderRect = rect2fMinDim(w->at.x, w->at.y - sliderY, sliderX, sliderY);

	float greyValue = 0.2f;
	renderDrawRect(sliderRect, 2, v4(greyValue, greyValue, greyValue, 1.0f), 0, mat4TopLeftToBottomLeft(e->fuaxResolution.y), e->orthoMatrix);


	float handleX = 20;
	float handleY = 20;

	float percent = ((*value - min) / (max - min));
	if(percent < 0.0f)  percent = 0; 
	if(percent > 1.0f)  percent = 1; 

	float offset = w->at.x + sliderX*percent;
	
	Rect2f handleRect = rect2fCenterDim(offset, w->at.y - sliderY, handleX, handleY);

	V4 handleColor = COLOR_WHITE;

///////////////////////***********check if were interacting with it**************////////////////////
	if(e->interactingWith.item && easyEditor_idEqual(e->interactingWith.id, lineNumber, fileName, index)) {
		e->interactingWith.visitedThisFrame = true;

		if(!isDown(e->keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
			easyEditor_stopInteracting(e);
		} else {
			float tVal = (e->keyStates->mouseP.x - w->at.x) / sliderX;
			*value = clamp(min, lerp(min, tVal, max), max);
		}

		handleColor = COLOR_GREY;
	} else {
		if(inBounds(e->keyStates->mouseP, handleRect, BOUNDS_RECT)) {
			if(wasPressed(e->keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
				if(e->interactingWith.item) {
					easyEditor_stopInteracting(e);	
				}
				easyEditor_startInteracting(e, field, lineNumber, fileName, index, EASY_EDITOR_INTERACT_SLIDER);
			} 
			handleColor = COLOR_YELLOW;
		}
	}

	

	renderDrawRect(handleRect, 1, handleColor, 0, mat4TopLeftToBottomLeft(e->fuaxResolution.y), e->orthoMatrix);

///////////////////////*************advance the cursor ************////////////////////

	w->at.x += sliderX;


	if(w->maxX < w->at.x) {
		w->maxX = w->at.x;
	}

	w->at.x = w->topCorner.x;
	w->at.y += getDim(rect).y + EASY_EDITOR_MARGIN;

}

#define easyEditor_pushColor(e, name, color) easyEditor_pushColor_(e, name, color, __LINE__, __FILE__)
static inline void easyEditor_pushColor_(EasyEditor *e, char *name, V4 *color, int lineNumber, char *fileName) {
	EasyEditorState *w = e->currentWindow;
	assert(w);

	//NOTE(ol): Take snapshot of state
	BlendFuncType tempType = globalRenderGroup->blendFuncType;
	//

	EasyEditorState *field = easyEditor_addColorSelector(e, *color, lineNumber, fileName, 0);

	Rect2f rect = rect2f(0, 0, 30, 10);
	if(name && strlen(name) > 0) {
		rect = outputTextNoBacking(e->font, w->at.x, w->at.y, 1, e->fuaxResolution, name, w->margin, COLOR_WHITE, 1, true, e->screenRelSize);
		w->at.x += getDim(rect).x + 3*EASY_EDITOR_MARGIN;
	}

	V2 at = v2_minus(w->at, v2(0, getDim(rect).y));
	Rect2f bounds = rect2fMinDimV2(at, getDim(rect));

	//change to other type of blend
	setBlendFuncType(globalRenderGroup, BLEND_FUNC_STANDARD_NO_PREMULTIPLED_ALPHA);

	renderDrawRect(bounds, 2, *color, 0, mat4TopLeftToBottomLeft(e->fuaxResolution.y), e->orthoMatrix);

	//restore the state
	setBlendFuncType(globalRenderGroup, tempType);
	//
	w->at.x += getDim(bounds).x;

	w->at.y += getDim(bounds).y;


	float advanceY = 0;
	if(field->isOpen) {

		if(wasPressed(e->keyStates->gameButtons, BUTTON_BACKSPACE)) {
			if(e->interactingWith.item) {
				easyEditor_stopInteracting(e);	
			}
			field->coord = v2(0, 0);
			*color = COLOR_WHITE;
		}
/////////////************ DRAW THE COLOR WHEEL ***********///////////////
		V2 dim = v2(150, 150);
		float radius = 0.5f*dim.x;
		V3 center = v3(w->at.x + 0.5f*dim.x, w->at.y + 0.5f*dim.y, 1);

		advanceY = dim.y + e->font->fontHeight;
///////////////////////////////////////////////////////////////////////////

		Rect2f colorSelectorBounds = rect2fCenterDimV2(v2_plus(v2_scale(radius, field->coord), center.xy), v2(15, 15));
		V4 selectorColor = COLOR_BLACK;

		///// IF INTERACTING WITH THE COLOR SELECTOR
		if(e->interactingWith.item && easyEditor_idEqual(e->interactingWith.id, lineNumber, fileName, 0)) {
			e->interactingWith.visitedThisFrame = true;
			assert(field == e->interactingWith.item);

			if(!isDown(e->keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
				easyEditor_stopInteracting(e);
			}

			selectorColor = *color;

///////////////////////***********SET POSITION OF SELECTOR**************////////////////////

			field->coord = v2_minus(e->keyStates->mouseP, center.xy);
			float length = getLength(field->coord) / radius;

			if(length > 1) { length = 1; }

			field->coord = v2_scale(length, normalizeV2(field->coord));
			
		} else {
			//NOTE: Check whether we are hovering over the button
			if(inBounds(e->keyStates->mouseP, colorSelectorBounds, BOUNDS_RECT)) {
				selectorColor = COLOR_YELLOW;	
				
				if(wasPressed(e->keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
					if(e->interactingWith.item) {
						easyEditor_stopInteracting(e);	
					}
					easyEditor_startInteracting(e, field, lineNumber, fileName, 0, EASY_EDITOR_INTERACT_COLOR_WHEEL);
				} 
			}
		}

		//change to other type of blend
		setBlendFuncType(globalRenderGroup, BLEND_FUNC_STANDARD_NO_PREMULTIPLED_ALPHA);
		//

		EasyRender_ColorWheel_DataPacket *packet = pushStruct(&globalPerFrameArena, EasyRender_ColorWheel_DataPacket);
		
		packet->value = field->value;

		renderColorWheel(center, dim, packet, mat4TopLeftToBottomLeft(e->fuaxResolution.y), e->orthoMatrix, mat4());
		
		//restore the state
		setBlendFuncType(globalRenderGroup, tempType);
		//
		renderDrawRect(colorSelectorBounds, 1, selectorColor, 0, mat4TopLeftToBottomLeft(e->fuaxResolution.y), e->orthoMatrix);

		w->at.y += advanceY + EASY_EDITOR_MARGIN;

		easyEditor_pushSlider_(e, "", &field->value, 0, 1, lineNumber, fileName, 1);

///////////////////////*************WORK OUT THE NEW COLOR & SET IT ************////////////////////
		float hue = 0;
		float saturation = getLength(field->coord);
		if(field->coord.x != 0.0f) {
			float flippedY = field->coord.y;
			float flippedX = -1*field->coord.x;


			hue = atan2(flippedY, flippedX);
			//assert(hue >= -PI32 && hue <= PI32);
			hue /= PI32;
			assert(hue >= -1.0f && hue <= 1.0f);
			hue += 1.0f;
			hue /= 2.0f;
			assert(hue >= 0.0f && hue <= 1.0f);
			hue *= 360.0f; 
			assert(hue >= 0 && hue <= 360);
		}

		*color = easyColor_hsvToRgb(hue, saturation, field->value);

	} else {
		w->at.y += advanceY + EASY_EDITOR_MARGIN;
	}


	if(inBounds(e->keyStates->mouseP, bounds, BOUNDS_RECT)) {
		if(wasPressed(e->keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
			field->isOpen = !field->isOpen;
		} 
	}

	

	if(w->maxX < w->at.x) {
		w->maxX = w->at.x;
	}

	w->at.x = w->topCorner.x;


	
}


