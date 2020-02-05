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

	u64 timeStamp;
	u64 beginTimeStamp;
} EasyProfile_TimeStamp;

#define EASY_PROFILER_SAMPLES 8*4096

typedef struct {
	//NOTE(ollie): The array to store the frames data
	u32 timeStampAt; //this index into the array
	EasyProfile_TimeStamp data[EASY_PROFILER_SAMPLES];
	
	//NOTE(ollie): This are the times we take percentages of
	u64 beginCountForFrame;
	u64 countsForFrame;
	
} EasyProfile_FrameData;

///////////////////////************* All the frame data for the profiler************////////////////////

#define EASY_PROFILER_FRAMES 64
static int Global_EasyProfiler_frameAt = 0;
static int Global_EasyProfiler_framesFilled = 0;

static EasyProfile_FrameData DEBUG_globalProfilerArray[EASY_PROFILER_FRAMES] = {};

////////////////////////////////////////////////////////////////////
static u64 EasyProfile_AddSample(u32 lineNumber, char *fileName, char *functionName, u64 timeStamp, u64 beginTimeStamp, EasyProfile_SampleType type, bool isOverallFrame) {
	

	EasyProfile_FrameData *frameData = &DEBUG_globalProfilerArray[Global_EasyProfiler_frameAt];
	
	///////////////////////************ Setup the datum for the frame i.e. the overall size of the frame *************////////////////////
	if(isOverallFrame) {
		if(type == EASY_PROFILER_PUSH_SAMPLE) {
			assert(timeStamp == beginTimeStamp);
			frameData->beginCountForFrame = beginTimeStamp;
		} else {
			assert(type == EASY_PROFILER_POP_SAMPLE);
			frameData->countsForFrame = timeStamp;
		}
	}


	///////////////////////************ Actually add the sample to the data set *************////////////////////
	assert(frameData->timeStampAt < EASY_PROFILER_SAMPLES);
	EasyProfile_TimeStamp *sample = frameData->data + frameData->timeStampAt++;

	sample->type = type;

	sample->lineNumber = lineNumber;
	sample->fileName = fileName;
	sample->functionName = functionName;	

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

	EasyProfileBlock(u32 lineNumber, char *fileName, char *functionName, bool isOverallFrame) {
		if(!DEBUG_global_ProfilePaused) {
			u64 ts = __rdtsc();
			EasyProfile_AddSample(lineNumber, fileName, functionName, ts, ts, EASY_PROFILER_PUSH_SAMPLE, isOverallFrame);

			this->startTimeStamp = ts;
			this->lineNumber = lineNumber;
			this->fileName = fileName;
			this->functionName = functionName;	
			this->isOverallFrame = isOverallFrame;

		} else {
			this->isOverallFrame = false;
		}
	}

	~EasyProfileBlock() {
		if(!DEBUG_global_ProfilePaused) {
			u64 ts = __rdtsc();
			EasyProfile_AddSample(lineNumber, fileName, functionName, ts - startTimeStamp, this->startTimeStamp, EASY_PROFILER_POP_SAMPLE, isOverallFrame);
		}
	}
};


static inline void EasyProfile_MoveToNextFrame() {
	if(!DEBUG_global_ProfilePaused) {
		//NOTE(ollie): We advanced the index & count, & warp the index if it is overflown 
		Global_EasyProfiler_frameAt++;
		Global_EasyProfiler_framesFilled++;

		if(Global_EasyProfiler_framesFilled >= EASY_PROFILER_FRAMES) {
			Global_EasyProfiler_framesFilled = EASY_PROFILER_FRAMES;
		}

		if(Global_EasyProfiler_frameAt >= Global_EasyProfiler_framesFilled) {
			Global_EasyProfiler_frameAt = 0;
		}

		EasyProfile_FrameData *frameData = &DEBUG_globalProfilerArray[Global_EasyProfiler_frameAt];

		frameData->timeStampAt = 0;
		frameData->beginCountForFrame = 0;
		frameData->countsForFrame = 0;
	}
}

#if DEVELOPER_MODE
#define DEBUG_TIME_BLOCK() EasyProfileBlock pb_##__FILE__##__LINE__##__FUNCTION__(__LINE__, __FILE__, __FUNCTION__, false); 
#define DEBUG_TIME_BLOCK_FOR_FRAME_BEGIN(varName) EasyProfileBlock *varName = new EasyProfileBlock(__LINE__, __FILE__, __FUNCTION__, true); 
#define DEBUG_TIME_BLOCK_FOR_FRAME_START(varName) varName = new EasyProfileBlock(__LINE__, __FILE__, __FUNCTION__, true); 
#define DEBUG_TIME_BLOCK_FOR_FRAME_END(varName) delete(varName); EasyProfile_MoveToNextFrame(); 
#define DEBUG_TIME_BLOCK_NAMED(name) EasyProfileBlock pb_##__FILE__##__LINE__##__FUNCTION__(__LINE__, __FILE__, name, false);
#else 
#define DEBUG_TIME_BLOCK() //just compile out 
#define DEBUG_TIME_BLOCK_NAMED(name)
#define DEBUG_TIME_BLOCK_FOR_FRAME_BEGIN()
#define DEBUG_TIME_BLOCK_FOR_FRAME_START()
#define DEBUG_TIME_BLOCK_FOR_FRAME_END()
#endif
