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
	float scrollPercent;
	float grabOffset;

	V2 lastMouseP;
	bool firstFrame;

	EasyDrawProfile_OpenState openState;
	Timer openTimer;

	int level;
} EasyProfile_ProfilerDrawState;


static void sortStringIds(EasyProfiler_DrawCountWithStringId *arr, int arrCount) {
    DEBUG_TIME_BLOCK()
    //this is a bubble sort. I think this is typically a bit slow. 
    bool sorted = false;
    int max = (arrCount - 1);
    for (int index = 0; index < max;) {
        bool incrementIndex = true;

        EasyProfiler_DrawCountWithStringId *infoA = &arr[index];
        EasyProfiler_DrawCountWithStringId *infoB = &arr[index + 1];
        assert(infoA && infoB);
        bool swap = infoA->drawCount < infoB->drawCount;
        if(swap) {
            EasyProfiler_DrawCountWithStringId temp = *infoA;
            *infoA = *infoB;
            *infoB = temp;
            sorted = true;
        }   
        if(index == (max - 1) && sorted) {
            index = 0; 
            sorted = false;
            incrementIndex = false;
        }
        
        if(incrementIndex) {
            index++;
        }
    }
}

static void EasyProfile_DrawGraph(EasyProfile_ProfilerDrawState *drawState, float aspectRatio_yOverX, AppKeyStates *keyStates, float dt, V2 viewportDim) {
	DEBUG_TIME_BLOCK()
	EasyProfiler_State *state = DEBUG_globalEasyEngineProfilerState;

	V4 colors[] = { COLOR_GREEN, COLOR_RED, COLOR_PINK, COLOR_BLUE, COLOR_AQUA, COLOR_YELLOW };

	//NOTE(ollie): We render things to a fake resolution then it just scales when it goes out to the viewport. 
	float fuaxWidth = 1920;
	V2 resolution = v2(fuaxWidth, fuaxWidth*aspectRatio_yOverX);
	float screenRelativeSize = 1.0f;

	
	if(drawState->firstFrame) {
		drawState->lastMouseP = easyInput_mouseToResolution_originLeftBottomCorner(keyStates, resolution);
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

					if(inBounds(easyInput_mouseToResolution_originLeftBottomCorner(keyStates, resolution), r, BOUNDS_RECT)) {
						color = COLOR_YELLOW;
						if(wasPressed(keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
							drawState->holdingScrollBar = true;
							drawState->grabOffset = easyInput_mouseToResolution_originLeftBottomCorner(keyStates, resolution).x - (drawState->xOffset*graphWidth + xMin);
							assert(drawState->grabOffset >= 0); //xOffset is the start of the handle
						}					
					} 

					if(drawState->holdingScrollBar) {
						color = COLOR_GREEN;

						float goalPos = ((easyInput_mouseToResolution_originLeftBottomCorner(keyStates, resolution).x - drawState->grabOffset) - xMin) / graphWidth;
						//update position
						drawState->xOffset = lerp(drawState->xOffset, 0.4f, goalPos);
					}

					///////////////////////************* Clamp the scroll bar between the range ************////////////////////
					//NOTE(ollie): Clamp the min range
					if(drawState->xOffset < 0) {
						drawState->xOffset = 0.0f;
					} 

					float maxPercent = ((graphWidth - handleWidth) / graphWidth);
					//NOTE(ollie): Clamp the max range
					if(drawState->xOffset > maxPercent) {
						drawState->xOffset = maxPercent; 
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

					///////////////////////************ Draw infomration for farame*************////////////////////

					float xAt = 0.05*resolution.x;
					float yAt = 2*globalDebugFont->fontHeight;

					char *formatString = "Frame Information:\nTotal Milliseconds for frame: %f\n";

					char frameInfoString[1028];
					snprintf(frameInfoString, arrayCount(frameInfoString), formatString, state->lookingAtSingleFrame->millisecondsForFrame);

					Rect2f fontDim = outputTextNoBacking(globalDebugFont, xAt, yAt, NEAR_CLIP_PLANE, resolution, frameInfoString, rect2fMinDim(xAt, 0, resolution.x, resolution.y), COLOR_BLACK, 1, true, screenRelativeSize);

					yAt += getDim(fontDim).y;

					for(int drawCountIndex = 0; drawCountIndex < arrayCount(frame->drawCounts); ++drawCountIndex) {
						
						EasyProfiler_DrawCountInfo *countInfo = &state->lookingAtSingleFrame->drawCounts[drawCountIndex];
							
						u32 drawCount = countInfo->drawCount;
						//NOTE(ollie): If there are actually anything drawn so it's easier to see the information. 
						if(drawCount > 0) {
							char drawCountString[512];
							snprintf(drawCountString, arrayCount(drawCountString), "Draw count for %s: %d\n", EasyRender_ShapeTypeStrings[drawCountIndex], drawCount);

							Rect2f bounds = outputTextNoBacking(globalDebugFont, xAt, yAt, NEAR_CLIP_PLANE, resolution, drawCountString, rect2fMinDim(xAt, 0, resolution.x, resolution.y), COLOR_BLACK, 1, false, screenRelativeSize);
							V4 highlightColor = COLOR_BLACK;

							if(inBounds(easyInput_mouseToResolution(keyStates, resolution), bounds, BOUNDS_RECT)) {
								if(wasPressed(keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
									//NOTE(ollie): Toggle the string ids on or off 
									countInfo->isOpened = !countInfo->isOpened;
								}
								highlightColor = COLOR_GOLD;
							}

							bounds = outputTextNoBacking(globalDebugFont, xAt, yAt, NEAR_CLIP_PLANE, resolution, drawCountString, rect2fMinDim(xAt, 0, resolution.x, resolution.y), highlightColor, 1, true, screenRelativeSize);

							yAt += getDim(bounds).y;

							if(countInfo->isOpened) {
								
								///////////////////////************ Draw scroll handle *************////////////////////
								float handleHeight = 50;
								float handleWidth = 50;

								//NOTE(ollie): The drop down is open so get the height of the drop down
								//NOTE(ollie): We assume each info fits on one line each
								float scrollWindowHeight = countInfo->uniqueModelCount*globalDebugFont->fontHeight - handleHeight;

								float visibleScrollWindowHeight = 0.2f*resolution.y - handleHeight; 

								bool neededScroll = false;
								//NOTE(ollie): If you need to scroll
								if(scrollWindowHeight > visibleScrollWindowHeight) {
									neededScroll = true;
									V2 handlePos = v2(0.01*resolution.x, (yAt - globalDebugFont->fontHeight) + visibleScrollWindowHeight*countInfo->scrollAt_01);

									Rect2f scrollHandle = rect2fMinDim(handlePos.x, handlePos.y, handleWidth, handleHeight);
									
									V4 handleColor = COLOR_GOLD;

									if(inBounds(easyInput_mouseToResolution(keyStates, resolution), scrollHandle, BOUNDS_RECT)) {
										handleColor = COLOR_YELLOW;
										 if(wasPressed(keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
											//NOTE(ollie): grab the handle
											countInfo->holdingScrollBar = true;									 	
										 }
										
									}

									if(countInfo->holdingScrollBar) {
										handleColor = COLOR_GREEN;

										///////////////////////************ Update the handle position *************////////////////////
										float goalPos = easyInput_mouseToResolution(keyStates, resolution).y - yAt;
										//update position
										countInfo->scrollAt_01 = lerp(visibleScrollWindowHeight*countInfo->scrollAt_01, 0.4f, goalPos) / visibleScrollWindowHeight;
										countInfo->scrollAt_01 = clamp01(countInfo->scrollAt_01);

										////////////////////////////////////////////////////////////////////

										//NOTE(ollie): Let go of the handle
										if(wasReleased(keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
											countInfo->holdingScrollBar = false;
										}
									}
									
									scrollHandle = rect2fMinDim(handlePos.x, handlePos.y, handleWidth, handleHeight);

									renderDrawRect(scrollHandle, NEAR_CLIP_PLANE, handleColor, 0, mat4TopLeftToBottomLeft(resolution.y), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));
								}
									////////////////////////////////////////////////////////////////////

								u32 bookKeepingCount = 0;

								float windowBoundsY = INFINITY;
								float scrollOffset = 0;

								if(neededScroll) {
									Rect2f scrollWindow = rect2fMinDim(xAt, yAt - globalDebugFont->fontHeight, resolution.x, visibleScrollWindowHeight + handleHeight);
									
									windowBoundsY = scrollWindow.max.y;
									easyRender_pushScissors(globalRenderGroup, scrollWindow, NEAR_CLIP_PLANE, mat4TopLeftToBottomLeft(resolution.y), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y), viewportDim);
									scrollOffset = countInfo->scrollAt_01*(scrollWindowHeight - visibleScrollWindowHeight);
								}

								float cached_yAt = yAt;

								// sort on id->drawCount so shows heightest first
								sortStringIds(countInfo->ids, countInfo->uniqueModelCount);

								for(int stringIndex = 0; stringIndex < countInfo->uniqueModelCount; ++stringIndex) {
									EasyProfiler_DrawCountWithStringId *id = &countInfo->ids[stringIndex];
									//NOTE(ollie): Outside of scroll window
									if((yAt - scrollOffset) < windowBoundsY) {
										char *modelFormatString = "%s %d\n";

										char modelString[512];
										snprintf(modelString, arrayCount(modelString), modelFormatString, id->stringId, id->drawCount);

										float textAtY = yAt - scrollOffset;
										Rect2f bounds = outputTextNoBacking(globalDebugFont, xAt, textAtY, NEAR_CLIP_PLANE, resolution, modelString, rect2fMinDim(xAt, 0, resolution.x, resolution.y), COLOR_BLUE, 1, true, screenRelativeSize);
										yAt += getDim(bounds).y;
									}

									bookKeepingCount += id->drawCount;
								}

								yAt = cached_yAt + visibleScrollWindowHeight + globalDebugFont->fontHeight;

								//NOTE(ollie): Make sure they equal to each other. 
								assert(bookKeepingCount == countInfo->drawCount);
								if(neededScroll) {
									easyRender_disableScissors(globalRenderGroup);
								}
							}
						}
					}

					////////////////////////////////////////////////////////////////////

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
							if(inBounds(easyInput_mouseToResolution_originLeftBottomCorner(keyStates, resolution), r, BOUNDS_RECT)) {

								///////////////////////*********** Draw the name of it **************////////////////////
								char digitBuffer[1028] = {};
								sprintf(digitBuffer, "%d", ts.lineNumber);

								
								char *formatString = "File name: %s\n\nFunction name: %s\nLine number: %d\nMilliseonds: %f\nTotal Cycles: %ld\n";
								int stringLengthToAlloc = snprintf(0, 0, formatString, ts.fileName, ts.functionName, ts.lineNumber, ts.totalTime, ts.timeStamp) + 1; //for null terminator, just to be sure
								
								char *strArray = pushArray(&globalPerFrameArena, stringLengthToAlloc, char);

								snprintf(strArray, stringLengthToAlloc, formatString, ts.fileName, ts.functionName, ts.lineNumber, ts.totalTime, ts.timeStamp); //for null terminator, just to be sure

							

								Rect2f margin = rect2fMinDim(xAt, yAt - globalDebugFont->fontHeight, resolution.x, resolution.y);

								outputTextNoBacking(globalDebugFont, xAt, yAt, NEAR_CLIP_PLANE, resolution, strArray, margin, COLOR_BLACK, 1, true, screenRelativeSize);

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
						
						if(inBounds(easyInput_mouseToResolution_originLeftBottomCorner(keyStates, resolution), r, BOUNDS_RECT) ) {
							color = COLOR_WHITE;

							///////////////////////*********** Draw milliseconds of it **************////////////////////
							char strBuffer[1028] = {};
							snprintf(strBuffer, arrayCount(strBuffer), "Milliseconds for frame: %fms", frame->millisecondsForFrame);

							float textWidth = 0.2f*resolution.x;
							
							V2 b = getBounds(strBuffer, rect2fMinDim(0, 0, textWidth, resolution.y), globalDebugFont, 1, resolution, screenRelativeSize);

							float xAt = easyInput_mouseToResolution_originLeftBottomCorner(keyStates, resolution).x;
							float yAt = resolution.y - easyInput_mouseToResolution_originLeftBottomCorner(keyStates, resolution).y - b.y + backdropY;

							Rect2f margin = rect2fMinDim(xAt, yAt - globalDebugFont->fontHeight, textWidth, 2*resolution.y);

							outputTextNoBacking(globalDebugFont, xAt, yAt, NEAR_CLIP_PLANE, resolution, strBuffer, margin, COLOR_BLACK, 1, true, screenRelativeSize);

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

	drawState->lastMouseP = easyInput_mouseToResolution_originLeftBottomCorner(keyStates, resolution);

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