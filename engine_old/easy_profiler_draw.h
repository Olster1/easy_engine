typedef enum {
	EASY_PROFILER_DRAW_OVERVIEW,
	EASY_PROFILER_DRAW_ACCUMALTED,
} EasyProfile_DrawType;

typedef enum {
	EASY_PROFILER_DRAW_OPEN,
	EASY_PROFILER_DRAW_CLOSED,
} EasyDrawProfile_OpenState;

typedef struct {
	int hotIndex;

	EasyProfile_DrawType drawType;

	//For navigating the samples in a frame
	float zoomLevel;
	float xOffset; //for the scroll bar
	bool holdingScrollBar;
	float scroll_dP; //velocity of scroll
	float scrollPercent;
	float grabOffset;

	V2 lastMouseP;
	bool firstFrame;

	EasyDrawProfile_OpenState openState;
	Timer openTimer;

	int level;
} EasyProfile_ProfilerDrawState;

static void EasyProfile_DrawGraph(EasyProfile_ProfilerDrawState *drawState, V2 resolution, AppKeyStates *keyStates, float screenRelativeSize, float dt) {
	DEBUG_TIME_BLOCK()
	EasyProfiler_State *state = DEBUG_globalEasyEngineProfilerState;

	V4 colors[] = { COLOR_GREEN, COLOR_RED, COLOR_PINK, COLOR_BLUE, COLOR_AQUA, COLOR_YELLOW };

	
	if(drawState->firstFrame) {
		drawState->lastMouseP = keyStates->mouseP_left_up;
		drawState->firstFrame = false;
	}

	if(state->framesFilled > 0) { //don't output anything on the first frames worth

		float backdropHeight = (state->lookingAtSingleFrame) ? resolution.y : 0.6f*resolution.y;
		float backdropY = 0;

		if(isOn(&drawState->openTimer)) {
			TimerReturnInfo timeInfo = updateTimer(&drawState->openTimer, dt);

			if(drawState->openState == EASY_PROFILER_DRAW_OPEN) {
				backdropY = smoothStep01(-backdropHeight, timeInfo.canonicalVal, 0);	
			} else {
				backdropY = smoothStep01(0, timeInfo.canonicalVal, -backdropHeight);
			}

			if(timeInfo.finished) {
				turnTimerOff(&drawState->openTimer);
			}
		} 

		if(drawState->openState == EASY_PROFILER_DRAW_OPEN || isOn(&drawState->openTimer)) {

			///////////////////////************ Draw the backdrop *************////////////////////
			Rect2f backDropRect = rect2fMinDim(0, backdropY, resolution.x, backdropHeight);

			renderDrawRect(backDropRect, NEAR_CLIP_PLANE + 0.5f, COLOR_LIGHT_GREY, 0, mat4(), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));
			
			////////////////////////////////////////////////////////////////////
			
			if(wasPressed(keyStates->gameButtons, BUTTON_ESCAPE) && state->lookingAtSingleFrame) {
				state->lookingAtSingleFrame = 0;
			}

			float graphWidth = 0.8f*resolution.x;
			int level = 0;

			if(drawState->drawType == EASY_PROFILER_DRAW_OVERVIEW) {
				if(state->lookingAtSingleFrame) {
					assert(DEBUG_global_ProfilePaused);//has to be paused


			

					///////////////////////*********** Scroll bar**************////////////////////
					
					V4 color = COLOR_GREY;
					float xMin = 0.1f*resolution.x;
					float xMax = 0.9f*resolution.x;

					float handleY = 0.06f*resolution.y;

					float handleWidth = (1.0f / drawState->zoomLevel)*graphWidth;
					
					if(handleWidth < 0.01*resolution.x) {
						handleWidth = 0.01*resolution.x;
					}
					float handleHeight = 0.04f*resolution.y;

					Rect2f r = rect2fMinDim(xMin + drawState->xOffset*graphWidth, handleY + backdropY, handleWidth, handleHeight);

					if(inBounds(keyStates->mouseP_left_up, r, BOUNDS_RECT)) {
						color = COLOR_YELLOW;
						if(wasPressed(keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
							drawState->holdingScrollBar = true;
							drawState->grabOffset = keyStates->mouseP_left_up.x - (drawState->xOffset*graphWidth + xMin);
							assert(drawState->grabOffset >= 0); //xOffset is the start of the handle
						}					
					} 

					if(drawState->holdingScrollBar) {
						color = COLOR_GREEN;

						float goalPos = ((keyStates->mouseP_left_up.x - drawState->grabOffset) - xMin) / graphWidth;
						//update position
						drawState->xOffset = lerp(drawState->xOffset, 0.4f, goalPos);
					}

					///////////////////////************* Clamp the scroll bar between the range ************////////////////////
					//NOTE(ollie): Clamp the min range
					if(drawState->xOffset < 0) {
						drawState->xOffset = 0.0f;
						drawState->scroll_dP = 0;
					} 

					float maxPercent = ((graphWidth - handleWidth) / graphWidth);
					//NOTE(ollie): Clamp the max range
					if(drawState->xOffset > maxPercent) {
						drawState->xOffset = maxPercent; 
						drawState->scroll_dP = 0;
					} 

					//NOTE(ollie): Update the offset when we scroll & the handle changes size
					if(drawState->xOffset*graphWidth + handleWidth > graphWidth) {
						drawState->xOffset = (graphWidth - handleWidth) / graphWidth;
					}

					///////////////////////************* Get the scroll bar percent ************////////////////////

					float dividend = (graphWidth - handleWidth);
					if(dividend == 0) {
						drawState->scrollPercent = 0.0f;
					} else {
						drawState->scrollPercent = drawState->xOffset*graphWidth / dividend;	
					}

					////////////////////////////////////////////////////////////////////
					

					if(wasReleased(keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
						drawState->holdingScrollBar = false;
					}

					///////////////////////************* Draw the scroll bar handle ************////////////////////
					//recalculate the handle again
					r = rect2fMinDim(xMin + drawState->xOffset*graphWidth, handleY + backdropY, handleWidth, handleHeight);

					renderDrawRect(r, NEAR_CLIP_PLANE + 0.01f, color, 0, mat4(), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));
					
					////////////////////////////////////////////////////////////////////

					float defaultGraphWidth = graphWidth; 
					graphWidth *= drawState->zoomLevel;

					EasyProfile_FrameData *frame = state->lookingAtSingleFrame;

					for(int i = 0; i < frame->timeStampAt; ++i) {

						EasyProfile_TimeStamp ts = frame->data[i];

						if(ts.type == EASY_PROFILER_PUSH_SAMPLE) {
							level++;
						} else {

							drawState->zoomLevel += drawState->zoomLevel*0.005f*keyStates->scrollWheelY*dt;

							if(drawState->zoomLevel < 1.0f) { drawState->zoomLevel = 1.0f; }

							assert(ts.type == EASY_PROFILER_POP_SAMPLE);
							assert(level > 0);
							
							float percent = (float)ts.timeStamp / (float)frame->countsForFrame;
							assert(percent <= 1.0f);

							float percentAcross = (float)(ts.beginTimeStamp - frame->beginCountForFrame) / (float)frame->countsForFrame; 

							float barWidth = percent*graphWidth;
							float barHeight = 0.05f*resolution.y;

							float expandedSize = (graphWidth - defaultGraphWidth);
							float xStart = 0.1f*resolution.x - drawState->scrollPercent*expandedSize;
							float yStart = 0.1f*resolution.y;

							Rect2f r = rect2fMinDim(xStart + percentAcross*graphWidth, yStart + level*barHeight + backdropY, barWidth, barHeight);

							V4 color = colors[i % arrayCount(colors)];
							if(inBounds(keyStates->mouseP_left_up, r, BOUNDS_RECT)) {

								///////////////////////*********** Draw the name of it **************////////////////////
								char digitBuffer[1028] = {};
								sprintf(digitBuffer, "%d", ts.lineNumber);

								char *formatString = "File name: %s\nFunction name: %s\nLine number: %d\nMilliseonds: %f\nTotal Cycles: %ld\n\n\nFrame Information:\nTotal Milliseconds for frame: %f\nTotal Draw Count for frame: %d\n\n";
								int stringLengthToAlloc = snprintf(0, 0, formatString, ts.fileName, ts.functionName, ts.lineNumber, ts.totalTime, ts.timeStamp, state->lookingAtSingleFrame->millisecondsForFrame, state->lookingAtSingleFrame->drawCount) + 1; //for null terminator, just to be sure
								
								char *strArray = pushArray(&globalPerFrameArena, stringLengthToAlloc, char);

								snprintf(strArray, stringLengthToAlloc, formatString, ts.fileName, ts.functionName, ts.lineNumber, ts.totalTime, ts.timeStamp, state->lookingAtSingleFrame->millisecondsForFrame, state->lookingAtSingleFrame->drawCount); //for null terminator, just to be sure

								V2 b = getBounds(strArray, rect2fMinDim(0, 0, resolution.x, resolution.y), &globalDebugFont, 1, resolution, screenRelativeSize);

								float xAt = 0.05*resolution.x;
								float yAt = 2*globalDebugFont.fontHeight;

								Rect2f margin = rect2fMinDim(xAt, yAt - globalDebugFont.fontHeight, resolution.x, resolution.y);

								outputTextNoBacking(&globalDebugFont, xAt, yAt, NEAR_CLIP_PLANE, resolution, strArray, margin, COLOR_BLACK, 1, true, screenRelativeSize);

								////////////////////////////////////////////////////////////////////

								color = COLOR_GREEN;
							}

							renderDrawRect(r, NEAR_CLIP_PLANE + 0.01f, color, 0, mat4(), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));

							if(drawState->hotIndex == i) {

							}

							level--;
						}
					} 

				} else {
					EasyProfile_FrameData *frame = state->lookingAtSingleFrame;

					//NOTE(ollie): Do a while loop since it is a ring buffer
					//NOTE(ollie):  state->frameAt will be the array is the array we are currently writing in to, so we want to not show this one.  
					s32 index = 0;
					assert(state->frameAt < EASY_PROFILER_FRAMES); 
					if(state->framesFilled < EASY_PROFILER_FRAMES) {
						//We haven't yet filled up the buffer so we will start at 0
						index = 0;
						assert(state->frameAt < state->framesFilled);
					} else {
						index = (s32)state->frameAt + 1;
						if(index >= EASY_PROFILER_FRAMES) {
							//Loop back to the end of the buffer
							index = 0;
						}
					}

					///////////////////////*********** Loop through first and accumulate the total **************////////////////////
					s64 totalTime = 0;
					s64 beginTime = state->frames[index].beginCountForFrame;
					int tempIndex = index;
					while(tempIndex != state->frameAt) {
						EasyProfile_FrameData *frame = &state->frames[tempIndex];
						////////////////////////////////////////////////////////////////////

						totalTime += frame->countsForFrame;

						////////////////////////////////////////////////////////////////////					
						tempIndex++;
						if(tempIndex == state->framesFilled) {
							assert(tempIndex != state->frameAt);
							tempIndex = 0;
						}
					}
					////////////////////////////////////////////////////////////////////

					float xStart = 0.1f*resolution.x;
					
					
					while(index != state->frameAt) {
						EasyProfile_FrameData *frame = &state->frames[index];
						V4 color = colors[index % arrayCount(colors)];

						float percent = (float)frame->countsForFrame / (float)totalTime;
						assert(frame->beginCountForFrame >= beginTime);
						float percentForStart = (float)(frame->beginCountForFrame - beginTime) / (float)totalTime;

						Rect2f r = rect2fMinDim(xStart + percentForStart*graphWidth, 0.1f*resolution.y + backdropY, percent*graphWidth, 0.1f*resolution.y);
						
						if(inBounds(keyStates->mouseP_left_up, r, BOUNDS_RECT) ) {
							color = COLOR_WHITE;

							///////////////////////*********** Draw milliseconds of it **************////////////////////
							char strBuffer[1028] = {};
							snprintf(strBuffer, arrayCount(strBuffer), "Milliseconds for frame: %fms", frame->millisecondsForFrame);

							float textWidth = 0.2f*resolution.x;
							
							V2 b = getBounds(strBuffer, rect2fMinDim(0, 0, textWidth, resolution.y), &globalDebugFont, 1, resolution, screenRelativeSize);

							float xAt = keyStates->mouseP_left_up.x;
							float yAt = resolution.y - keyStates->mouseP_left_up.y - b.y + backdropY;

							Rect2f margin = rect2fMinDim(xAt, yAt - globalDebugFont.fontHeight, textWidth, 2*resolution.y);

							outputTextNoBacking(&globalDebugFont, xAt, yAt, NEAR_CLIP_PLANE, resolution, strBuffer, margin, COLOR_BLACK, 1, true, screenRelativeSize);

							////////////////////////////////////////////////////////////////////

							if(wasPressed(keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
								state->lookingAtSingleFrame = frame;
								state->queuePause = true;
							}
						}

						renderDrawRect(r, NEAR_CLIP_PLANE, color, 0, mat4(), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));

						index++;
						if(index == state->framesFilled) {
							assert(index != state->frameAt);
							index = 0;
						}
					}
				}
			} else {

			}
		}
	}

	drawState->lastMouseP = keyStates->mouseP_left_up;

	if(wasPressed(keyStates->gameButtons, BUTTON_F3)) {
		if(isOn(&drawState->openTimer)) {
			drawState->openTimer.value_ = drawState->openTimer.period = drawState->openTimer.value_;
		} else {
			turnTimerOn(&drawState->openTimer);	
		}
		
		if(drawState->openState == EASY_PROFILER_DRAW_OPEN) {
			drawState->openState = EASY_PROFILER_DRAW_CLOSED;
		} else if(drawState->openState == EASY_PROFILER_DRAW_CLOSED) {
			drawState->openState = EASY_PROFILER_DRAW_OPEN;
		} 
	}
}

static EasyProfile_ProfilerDrawState *EasyProfiler_initProfiler() {
	EasyProfile_ProfilerDrawState *result = pushStruct(&globalLongTermArena, EasyProfile_ProfilerDrawState);
		
	EasyProfiler_State *state = DEBUG_globalEasyEngineProfilerState;

	result->hotIndex = -1;
	result->level = 0;
	result->drawType = EASY_PROFILER_DRAW_OVERVIEW;

	result->zoomLevel = 1;
	result->holdingScrollBar = false;
	result->xOffset = 0;
	result->scroll_dP = 0;
	result->lastMouseP =  v2(0, 0);
	result->firstFrame = true;
	result->grabOffset = 0;

	result->openTimer = initTimer(0.4f, false);
	turnTimerOff(&result->openTimer);
	result->openState = EASY_PROFILER_DRAW_CLOSED;

	///////////////////////************ Initialize the array to zero *************////////////////////
	for(int i = 0; i < arrayCount(state->frames); i++) {
		EasyProfile_FrameData *frameData = &state->frames[state->frameAt];

		frameData->timeStampAt = 0;
		frameData->beginCountForFrame = 0;
		frameData->countsForFrame = 0;
	}

	state->frameAt = 0;
	state->framesFilled = 0;

	////////////////////////////////////////////////////////////////////

	return result;

}