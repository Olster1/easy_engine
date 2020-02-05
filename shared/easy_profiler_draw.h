typedef enum {
	EASY_PROFILER_DRAW_OVERVIEW,
	EASY_PROFILER_DRAW_ACCUMALTED,
} EasyProfile_DrawState;

typedef struct {
	int hotIndex;

	EasyProfile_FrameData *lookingAtSingleFrame;
	EasyProfile_DrawState drawState;

	int level;
} EasyProfile_ProfilerState;

static void EasyProfile_DrawGraph(EasyProfile_ProfilerState *state, V2 resolution, AppKeyStates *keyStates, float screenRelativeSize) {
	DEBUG_TIME_BLOCK()
	bool savedPaused = DEBUG_global_ProfilePaused;
	DEBUG_global_ProfilePaused = true; //NOTE: To prevent any more debug calls getting pushed onto the stack

	V4 colors[] = { COLOR_GREEN, COLOR_RED, COLOR_PINK, COLOR_BLUE };

	if(DEBUG_global_DrawProfiler && Global_EasyProfiler_framesFilled > 0) { //don't ouput anything on the first frames worth

		if(wasPressed(keyStates->gameButtons, BUTTON_ESCAPE) && state->lookingAtSingleFrame) {
			state->lookingAtSingleFrame = 0;
		}

		float graphWidth = 0.8f*resolution.x;
		int level = 0;

		if(state->drawState == EASY_PROFILER_DRAW_OVERVIEW) {
			if(state->lookingAtSingleFrame) {
				assert(DEBUG_global_ProfilePaused);//has to be paused

				EasyProfile_FrameData *frame = state->lookingAtSingleFrame;

				for(int i = 0; i < frame->timeStampAt; ++i) {

					EasyProfile_TimeStamp ts = frame->data[i];

					if(ts.type == EASY_PROFILER_PUSH_SAMPLE) {
						level++;
						if(ts.lineNumber == 260) {
							int er =0 ;
						}
					} else {
						if(ts.lineNumber == 260) {
							int er =0 ;
						}
						assert(ts.type == EASY_PROFILER_POP_SAMPLE);
						assert(level > 0);
						
						float percent = (float)ts.timeStamp / (float)frame->countsForFrame;
						assert(percent <= 1.0f);

						float percentAcross = (float)(ts.beginTimeStamp - frame->beginCountForFrame) / (float)frame->countsForFrame; 

						float barWidth = percent*graphWidth;
						float barHeight = 0.05f*resolution.y;

						float xStart = 0.1f*resolution.x;
						float yStart = 0.1f*resolution.y;

						Rect2f r = rect2fMinDim(xStart + percentAcross*graphWidth, yStart + level*barHeight, barWidth, barHeight);

						V4 color = colors[i % 4];
						if(inBounds(keyStates->mouseP_left_up, r, BOUNDS_RECT)) {

							///////////////////////*********** Draw the name of it **************////////////////////
							//TODO: Actally find out how long it is.
							char digitBuffer[1028] = {};
							sprintf(digitBuffer, "%d", ts.lineNumber);

							char *str0 = concatInArena(ts.fileName, ts.functionName, &globalPerFrameArena);
							char *str1 = concatInArena(str0, digitBuffer, &globalPerFrameArena);

							float textWidth = 0.2f*resolution.x;
							
							V2 b = getBounds(str1, rect2fMinDim(0, 0, textWidth, resolution.y), &globalDebugFont, 1, resolution, screenRelativeSize);

							float xAt = keyStates->mouseP_left_up.x;
							float yAt = resolution.y - keyStates->mouseP_left_up.y - b.y;

							Rect2f margin = rect2fMinDim(xAt, yAt - globalDebugFont.fontHeight, textWidth, 2*resolution.y);

							outputTextNoBacking(&globalDebugFont, xAt, yAt, NEAR_CLIP_PLANE, resolution, str1, margin, COLOR_BLACK, 1, true, screenRelativeSize);

							////////////////////////////////////////////////////////////////////

							color = COLOR_GREEN;
						}

						renderDrawRect(r, NEAR_CLIP_PLANE + 0.01f, color, 0, mat4(), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));

						if(state->hotIndex == i) {

						}

						level--;
					}
				} 

			} else {
				EasyProfile_FrameData *frame = state->lookingAtSingleFrame;

				//NOTE(ollie): Do a while loop since it is a ring buffer
				//NOTE(ollie):  Global_EasyProfiler_frameAt will be the array is the array we are currently writing in to, so we want to not show this one.  
				s32 index = 0;
				assert(Global_EasyProfiler_frameAt < EASY_PROFILER_FRAMES); 
				if(Global_EasyProfiler_framesFilled < EASY_PROFILER_FRAMES) {
					//We haven't yet filled up the buffer so we will start at 0
					index = 0;
					assert(Global_EasyProfiler_frameAt < Global_EasyProfiler_framesFilled);
				} else {
					index = (s32)Global_EasyProfiler_frameAt + 1;
					if(index >= EASY_PROFILER_FRAMES) {
						//Loop back to the end of the buffer
						index = 0;
					}
				}

				///////////////////////*********** Loop through first and acculate the total **************////////////////////
				s64 totalTime = 0;
				s64 beginTime = DEBUG_globalProfilerArray[index].beginCountForFrame;
				int tempIndex = index;
				while(tempIndex != Global_EasyProfiler_frameAt) {
					EasyProfile_FrameData *frame = &DEBUG_globalProfilerArray[tempIndex];
					////////////////////////////////////////////////////////////////////

					totalTime += frame->countsForFrame;

					////////////////////////////////////////////////////////////////////					
					tempIndex++;
					if(tempIndex == Global_EasyProfiler_framesFilled) {
						assert(tempIndex != Global_EasyProfiler_frameAt);
						tempIndex = 0;
					}
				}
				////////////////////////////////////////////////////////////////////

				float xStart = 0.1f*resolution.x;
				
				
				while(index != Global_EasyProfiler_frameAt) {
					EasyProfile_FrameData *frame = &DEBUG_globalProfilerArray[index];
					V4 color = colors[index % 4];

					float percent = (float)frame->countsForFrame / (float)totalTime;
					assert(frame->beginCountForFrame >= beginTime);
					float percentForStart = (float)(frame->beginCountForFrame - beginTime) / (float)totalTime;

					Rect2f r = rect2fMinDim(xStart + percentForStart*graphWidth, 0.1f*resolution.y, percent*graphWidth, 0.1f*resolution.y);
					
					if(inBounds(keyStates->mouseP_left_up, r, BOUNDS_RECT) ) {
						color = COLOR_WHITE;

						if(wasPressed(keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
							state->lookingAtSingleFrame = frame;
							savedPaused = true;
						}
					}

					renderDrawRect(r, NEAR_CLIP_PLANE, color, 0, mat4(), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));

					index++;
					if(index == Global_EasyProfiler_framesFilled) {
						assert(index != Global_EasyProfiler_frameAt);
						index = 0;
					}
				}
			}
		} else {

		}
	}
	
	///////////////////////*********** Use the Pause hot key *************////////////////////
	//NOTE(ollie): It has to be here since we want to know to empty the array or not.
	if(wasPressed(keyStates->gameButtons, BUTTON_F4)) {
	    DEBUG_global_ProfilePaused = !savedPaused;

	    if(!DEBUG_global_ProfilePaused) {
	    	state->lookingAtSingleFrame = 0;
	    }
	} else {
		DEBUG_global_ProfilePaused = savedPaused;
	}
}

static EasyProfile_ProfilerState *EasyProfiler_initProfiler() {
	EasyProfile_ProfilerState *result = pushStruct(&globalLongTermArena, EasyProfile_ProfilerState);
	
	result->hotIndex = -1;
	result->level = 0;
	result->drawState = EASY_PROFILER_DRAW_OVERVIEW;
	result->lookingAtSingleFrame = 0;

	///////////////////////************ Initialize the array to zero *************////////////////////
	for(int i = 0; i < arrayCount(DEBUG_globalProfilerArray); i++) {
		EasyProfile_FrameData *frameData = &DEBUG_globalProfilerArray[Global_EasyProfiler_frameAt];

		frameData->timeStampAt = 0;
		frameData->beginCountForFrame = 0;
		frameData->countsForFrame = 0;
	}

	Global_EasyProfiler_frameAt = 0;
	Global_EasyProfiler_framesFilled = 0;

	////////////////////////////////////////////////////////////////////

	return result;

}