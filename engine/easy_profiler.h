/*
How to use: 

DEBUG_TIME_BLOCK(); //put this at the begining of a scope like a fiunction 
DEBUG_TIME_BLOCK_NAMED(name); //this is if you want to profile just a block

*/


typedef enum {
	EASY_PROFILER_PUSH_SAMPLE,
	EASY_PROFILER_POP_SAMPLE,
} EasyProfile_SampleType;


typedef struct {
	EasyProfile_SampleType type;

	//NOTE: Key
	u32 lineNumber;
	char *fileName;
	char *functionName;	
	//

	s64 startTimeCounter; //from query performance counter
	float totalTime; //milli seconds

	u64 timeStamp;
	u64 beginTimeStamp;
} EasyProfile_TimeStamp;

typedef struct {
	char stringId[256];
	u32 drawCount;
} EasyProfiler_DrawCountWithStringId;

typedef struct {
	//NOTE(ollie): This is to uniquely identify the models to see whos drawing more than one batch
	u32 uniqueModelCount;
	EasyProfiler_DrawCountWithStringId ids[256];
	/////

	//NOTE(ollie): The total draw count for this SHAPE_TYPE
	u32 drawCount;

	//NOTE(ollie): This is for when we're drawing it, we can open it and see the string ids 
	bool isOpened;

	//NOTE(ollie): This is when we're drawing it & want to scroll to see all the data. 
	bool holdingScrollBar;

	//NOTE(ollie): Where the scroll handle is when we draw it, bewtween 0 & 1
	//NOTE(ollie): From top of screen (0) down to (1) 
	float scrollAt_01;
} EasyProfiler_DrawCountInfo;

#define EASY_PROFILER_SAMPLES_INCREMENT 24000

typedef struct {
	//NOTE(ollie): The array to store the frames data

	u32 timeStampAt; //this index into the array
	u32 dataCount; //number of samples in data
	EasyProfile_TimeStamp *data;
	
	//NOTE(ollie): This are the times we take percentages of
	u64 beginCountForFrame;
	u64 countsForFrame;

	s64 beginTimeCountForFrame; //From query performance counter
	float millisecondsForFrame;

	EasyProfiler_DrawCountInfo drawCounts[RENDER_SHAPE_COUNT];
	
} EasyProfile_FrameData;

///////////////////////************* All the frame data for the profiler************////////////////////

#define EASY_PROFILER_FRAMES 64
typedef struct {
	int frameAt;
	int framesFilled;

	EasyProfile_FrameData frames[EASY_PROFILER_FRAMES];

	bool queuePause; //We have to pause at the end of a profiler frame in order to match up PUSH/POP samples
	EasyProfile_FrameData *lookingAtSingleFrame;
} EasyProfiler_State;

static inline EasyProfiler_State *EasyProfiler_initProfilerState() {
	//NOTE(ollie): This is a pretty massive struct, not sure what the consequences are 
	u32 bytes = sizeof(EasyProfiler_State);
	EasyProfiler_State *result = (EasyProfiler_State *)easyPlatform_allocateMemory(bytes, EASY_PLATFORM_MEMORY_NONE);

	//NOTE(ollie): Clear the array to zero. Note: the data * in each frame has to be zero & datacount has to be zero
	memset(result, 0, bytes);

	//just check this is zero
	assert(result->frames[0].data == 0);
	assert(result->frames[0].dataCount == 0);

	result->frameAt = 0;
	result->framesFilled = 0;

	result->lookingAtSingleFrame = 0;
	result->queuePause = false;

	///////////////////////************ Initialize the array to zero *************////////////////////
	for(int i = 0; i < arrayCount(result->frames); i++) {
		EasyProfile_FrameData *frameData = &result->frames[result->frameAt];

		frameData->timeStampAt = 0;
		frameData->beginCountForFrame = 0;
		frameData->countsForFrame = 0;
	}

	////////////////////////////////////////////////////////////////////

	DEBUG_global_ProfilePaused = false;

	return result;
}

static EasyProfiler_State *DEBUG_globalEasyEngineProfilerState;// = &DEBUG_globalEasyEngineProfilerState_;

////////////////////////////////////////////////////////////////////
static u64 EasyProfile_AddSample(u32 lineNumber, char *fileName, char *functionName, u64 timeStamp, u64 beginTimeStamp, s64 startTimeCounter, float totalTimeMilliseconds, EasyProfile_SampleType type, bool isOverallFrame) {
	EasyProfiler_State *state = DEBUG_globalEasyEngineProfilerState;
	assert(state);

	assert(state->frameAt < EASY_PROFILER_FRAMES);
	EasyProfile_FrameData *frame = &state->frames[state->frameAt];

	
	
	///////////////////////************ Setup the datum for the frame i.e. the overall size of the frame *************////////////////////
	if(isOverallFrame) {
		if(type == EASY_PROFILER_PUSH_SAMPLE) {
			assert(timeStamp == beginTimeStamp);
			frame->beginCountForFrame = beginTimeStamp;
			frame->beginTimeCountForFrame = startTimeCounter;
		} else {
			assert(type == EASY_PROFILER_POP_SAMPLE);
			frame->countsForFrame = timeStamp;
			frame->millisecondsForFrame = totalTimeMilliseconds;
		}
	}


	///////////////////////************ Actually add the sample to the data set *************////////////////////

	//NOTE(ollie): Expand the array
	if(frame->timeStampAt >= frame->dataCount) {
		assert(frame->timeStampAt == frame->dataCount);

		u32 newCount = frame->dataCount + EASY_PROFILER_SAMPLES_INCREMENT;
		printf("malloc: %d\n", newCount);
		EasyProfile_TimeStamp *newArray = (EasyProfile_TimeStamp *)malloc(newCount*sizeof(EasyProfile_TimeStamp));		
 		
 		if(frame->dataCount > 0) {
 			//NOTE(ollie): Copy data over to new array
 			assert(frame->data);
			memcpy(newArray, frame->data, frame->dataCount*sizeof(EasyProfile_TimeStamp));	
		}

		//assign the new array & count
		frame->dataCount = newCount;
		frame->data = newArray;

	}

	assert(frame->timeStampAt < frame->dataCount);
	EasyProfile_TimeStamp *sample = frame->data + frame->timeStampAt++;

	memset(sample, 0, sizeof(EasyProfile_TimeStamp));

	sample->type = type;

	sample->lineNumber = lineNumber;
	sample->fileName = fileName;
	sample->functionName = functionName;	

	sample->startTimeCounter = startTimeCounter;
	sample->totalTime = totalTimeMilliseconds;

	sample->timeStamp = timeStamp;
	sample->beginTimeStamp = beginTimeStamp;

	return timeStamp;
}

class EasyProfileBlock {
	public: 
	u32 lineNumber;
	char *fileName;
	char *functionName;
	bool isOverallFrame;	
	u64 startTimeStamp; 
	s64 startTimeSecondsCount; //This is the performance counter result. We have to convert it to actually make sense. See easy_time.h  

	bool wasPaused;

	EasyProfileBlock(u32 lineNumber, char *fileName, char *functionName, bool isOverallFrame) {
		if(!DEBUG_global_ProfilePaused) {
			u64 ts = __rdtsc();
			s64 startTimeSecondsCount = EasyTime_GetTimeCount(); //Since unix epoch I guess?
			//We just duplicate the ts & timeStart since these are used for the total time & is only for when we POP a sample (not PUSH)
			EasyProfile_AddSample(lineNumber, fileName, functionName, ts, ts, startTimeSecondsCount, 0, EASY_PROFILER_PUSH_SAMPLE, isOverallFrame);

			this->startTimeStamp = ts;
			this->lineNumber = lineNumber;
			this->fileName = fileName;
			this->functionName = functionName;	
			this->isOverallFrame = isOverallFrame;
			this->startTimeSecondsCount = startTimeSecondsCount;

		} else {
			this->isOverallFrame = false;
		}

		this->wasPaused = DEBUG_global_ProfilePaused;
	}

	~EasyProfileBlock() {
		if(!this->wasPaused) {
			u64 ts = __rdtsc();
			s64 timeAt = EasyTime_GetTimeCount();
			assert(timeAt >= this->startTimeSecondsCount);
			assert(ts >= startTimeStamp);
			EasyProfile_AddSample(lineNumber, fileName, functionName, ts - startTimeStamp, this->startTimeStamp, timeAt, EasyTime_GetMillisecondsElapsed(timeAt, this->startTimeSecondsCount), EASY_PROFILER_POP_SAMPLE, isOverallFrame);
		}
	}
};


static inline void EasyProfile_MoveToNextFrame(bool hotKeyWasPressed) {

	EasyProfiler_State *state = DEBUG_globalEasyEngineProfilerState;
	assert(state);

	///////////////////////*********** Use the Pause hot key *************////////////////////
	
	//NOTE(ollie): It has to be here since we know all PUSH samples are matched with POP samples
	if(hotKeyWasPressed) {
	    DEBUG_global_ProfilePaused = !DEBUG_global_ProfilePaused;
	    
	    if(!DEBUG_global_ProfilePaused) {
	    	state->lookingAtSingleFrame = 0;
	    }
	} 

	///////////////////////************* Update if anyone wanted to pause during the frame ************////////////////////

	if(state->queuePause) {
		DEBUG_global_ProfilePaused = true;
	}

	state->queuePause = false;

	////////////////////////////////////////////////////////////////////


	if(!DEBUG_global_ProfilePaused) {
		
		///////////////////////************ Move to the next frame if we aren't paused *************////////////////////

		//NOTE(ollie): We advanced the index & count, & warp the index if it is overflown 
		state->frameAt++;
		state->framesFilled++;

		if(state->framesFilled >= EASY_PROFILER_FRAMES) {
			//make sure the frames filled is equal to the value. Not really needed.
			state->framesFilled = EASY_PROFILER_FRAMES;
		}

		if(state->frameAt >= EASY_PROFILER_FRAMES) {
			assert(state->frameAt == EASY_PROFILER_FRAMES);
			state->frameAt = 0;
		}

		EasyProfile_FrameData *frameData = &state->frames[state->frameAt];

		frameData->timeStampAt = 0;
		frameData->beginCountForFrame = 0;
		frameData->countsForFrame = 0;
		frameData->beginTimeCountForFrame = 0;
		frameData->millisecondsForFrame = 0;

		//NOTE(ollie): Clear out draw counts
		for(int i = 0; i < arrayCount(frameData->drawCounts); ++i) {
			EasyProfiler_DrawCountInfo *info = &frameData->drawCounts[i];

			info->drawCount = 0;
			info->isOpened = false;
			info->holdingScrollBar = false;
			info->scrollAt_01 = 0;

			for(int stringIdIndex = 0; stringIdIndex < info->uniqueModelCount; ++stringIdIndex) {
				info->ids[stringIdIndex].drawCount = 0;
			}
			info->uniqueModelCount = 0;
		}
	}
}

static inline void easyProfiler_addDrawCount(u32 drawCount, u32 shapeTypeIndexIntoArray, char *stringId) {
	if(!DEBUG_global_ProfilePaused) {
		EasyProfiler_DrawCountInfo *info = &DEBUG_globalEasyEngineProfilerState->frames[DEBUG_globalEasyEngineProfilerState->frameAt].drawCounts[shapeTypeIndexIntoArray];

		//NOTE(ollie): Increment overall drawCount
		info->drawCount += drawCount;

		//NOTE(ollie): Look for the string and add the draw count to it
		bool found = false;
		for(int i = 0; i < info->uniqueModelCount && !found; ++i) {
			EasyProfiler_DrawCountWithStringId *id = &info->ids[i];
			if(cmpStrNull(id->stringId, stringId)) {
				id->drawCount += drawCount;
				found = true;
				break;
			}
		}

		//NOTE(ollie): Not found, so create a new id
		if(!found) {
			if(info->uniqueModelCount < arrayCount(info->ids)) {
				//NOTE(ollie): New one so fill out it's info
				EasyProfiler_DrawCountWithStringId *id = &info->ids[info->uniqueModelCount++];
				id->drawCount += drawCount;
	#if DEVELOPER_MODE
				easyPlatform_copyMemory(id->stringId, stringId, sizeof(char)*arrayCount(id->stringId));
	#endif
			} else {
				//NOTE(ollie): Buffer to small
				assert(!"overflow buffer");
			}
		}
	}
}

#if DEVELOPER_MODE
#define DEBUG_TIME_BLOCK() EasyProfileBlock pb_##__FILE__##__LINE__##__FUNCTION__(__LINE__, __FILE__, __FUNCTION__, false); 
#define DEBUG_TIME_BLOCK_FOR_FRAME_BEGIN(varName, functionName) DEBUG_globalEasyEngineProfilerState = EasyProfiler_initProfilerState(); EasyProfileBlock *varName = new EasyProfileBlock(__LINE__, __FILE__, functionName, true); 
#define DEBUG_TIME_BLOCK_FOR_FRAME_START(varName, functionName) varName = new EasyProfileBlock(__LINE__, __FILE__, functionName, true); 
#define DEBUG_TIME_BLOCK_FOR_FRAME_END(varName, hotKeyWasPressed) delete(varName); EasyProfile_MoveToNextFrame(hotKeyWasPressed); 
#define DEBUG_TIME_BLOCK_NAMED(name) EasyProfileBlock pb_##__FILE__##__LINE__##__FUNCTION__(__LINE__, __FILE__, name, false);
#else 
#define DEBUG_TIME_BLOCK() //just compile out 
#define DEBUG_TIME_BLOCK_NAMED(name)
#define DEBUG_TIME_BLOCK_FOR_FRAME_BEGIN(varName, functionName)
#define DEBUG_TIME_BLOCK_FOR_FRAME_START(varName, functionName)
#define DEBUG_TIME_BLOCK_FOR_FRAME_END(varName, hotKeyWasPressed)
#endif
