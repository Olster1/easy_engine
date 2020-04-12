#define LOGIC_BLOCK_TYPE(FUNC)\
FUNC(LOGIC_BLOCK_NULL)\
FUNC(LOGIC_BLOCK_CLEAR_COLOR)\
FUNC(LOGIC_BLOCK_PUSH_SCOPE)\
FUNC(LOGIC_BLOCK_POP_SCOPE)\
FUNC(LOGIC_BLOCK_LOOP)\
FUNC(LOGIC_BLOCK_DRAW_RECTANGLE)\
FUNC(LOGIC_BLOCK_PLAY_SOUND)\
FUNC(LOGIC_BLOCK_WHEN)\
FUNC(LOGIC_BLOCK_BUTTON_PRESSED)\
FUNC(LOGIC_BLOCK_BUTTON_DOWN)\
FUNC(LOGIC_BLOCK_BUTTON_RELEASED)\
FUNC(LOGIC_BLOCK_DRAW_CIRCLE)\
FUNC(LOGIC_BLOCK_WHILE)\
FUNC(LOGIC_BLOCK_DRAW_CUBE)\
FUNC(LOGIC_BLOCK_VARIABLE_INIT)\


typedef enum {
    LOGIC_BLOCK_TYPE(ENUM)
} LogicBlockType;

static char *global_logicBlockTypeStrings[] = { LOGIC_BLOCK_TYPE(STRING) };

static char *global_logicBlockTypeTidyStrings[] = { "Null", "Clear Color", "Push Scope", "Pop Scope", "Loop", "Draw Rectangle", "Set Background Color", "Play Sound", "When",  "Button Pressed", "Button Held Down", "Button Released", "Draw Circle", "While",  "Draw Cube", "Create Variable" };

static LogicBlockType global_logicBlockTypes[] = { LOGIC_BLOCK_TYPE(ENUM) };

typedef struct {
	LogicBlockType type;

	//NOTE(ollie): For rendering when the user hovers over the block, to make room
	float extraSpacing;
	float currentSpacing;

	//NOTE(ollie): For rendering the drop down options
	bool isOpen;

	//NOTE(ollie): Parameters into the functions
	u32 parameterCount;
	EasyVM_Variable parameters[32];	
} LogicBlock;

static void logicBlock_addParameter(LogicBlock *block, char *name, VarType type) {
	assert(block->parameterCount < arrayCount(block->parameters));
	EasyVM_Variable *var = block->parameters + block->parameterCount++;

	u8 bogus[256];
	easyVM_assignVariable(var, type, bogus);
}


typedef struct {
	InfiniteAlloc logicBlocks;
} LogicBlock_Set;

static LogicBlock initBlock(LogicBlockType type) {
	//NOTE(ollie): Clear all fields
	LogicBlock block = {0};

	//NOTE(ollie): Set the type
	block.type = type;

	//NOTE(ollie): Add parameters to the functions
	switch(type) {
		case LOGIC_BLOCK_CLEAR_COLOR: {
			logicBlock_addParameter(&block, "Color", VAR_V4);
		} break;
		case LOGIC_BLOCK_PUSH_SCOPE: {

		} break;
		case LOGIC_BLOCK_POP_SCOPE: {

		} break;
		case LOGIC_BLOCK_LOOP: {
			logicBlock_addParameter(&block, "Count", VAR_INT);
		} break;
		case LOGIC_BLOCK_DRAW_RECTANGLE: {
			logicBlock_addParameter(&block, "Size", VAR_V2);
			logicBlock_addParameter(&block, "Position", VAR_V3);
			logicBlock_addParameter(&block, "Color", VAR_V4);
		} break;
		case LOGIC_BLOCK_PLAY_SOUND: {

		} break;
		case LOGIC_BLOCK_WHEN: {

		} break;
		case LOGIC_BLOCK_BUTTON_PRESSED: {

		} break;
		case LOGIC_BLOCK_BUTTON_DOWN: {

		} break;
		case LOGIC_BLOCK_BUTTON_RELEASED: {

		} break;
		case LOGIC_BLOCK_DRAW_CIRCLE: {

		} break;
		case LOGIC_BLOCK_WHILE: {

		} break;
		case LOGIC_BLOCK_DRAW_CUBE: {

		} break;
		case LOGIC_BLOCK_VARIABLE_INIT: {

		} break;
		default: {
			assert(false);
		}
	}
	
	return block;
}

static void initLogicBlockSet(LogicBlock_Set *set) {
	//NOTE(ollie): Initialize the array
	set->logicBlocks = initInfinteAlloc(LogicBlock);
}

static void pushLogicBlock(LogicBlock_Set *set, LogicBlockType type) {
	LogicBlock block = initBlock(type);
	
	addElementInfinteAlloc_notPointer(&set->logicBlocks, block);
}

static void spliceLogicBlock(LogicBlock_Set *set, LogicBlockType type, s32 index) {
	//NOTE(ollie): Add space for the one being added
	//NOTE(ollie): Just has to be empty because the data is about ot be override
	LogicBlock bogusBlock = {0};
	addElementInfinteAlloc_notPointer(&set->logicBlocks, bogusBlock);

	//NOTE(ollie): Start from the other end, and move them all up one
	for(int i = (set->logicBlocks.count - 1); i > index; i--) {
		LogicBlock *blockA = getElementFromAlloc(&set->logicBlocks, i, LogicBlock);
		LogicBlock *blockB = getElementFromAlloc(&set->logicBlocks, i - 1, LogicBlock);
		*blockA = *blockB;
	}

	//NOTE(ollie): Now add the new one
	LogicBlock *block = getElementFromAlloc(&set->logicBlocks, index, LogicBlock);
	assert(block);

	LogicBlock newBlock = initBlock(type);
	newBlock.type = type;

	//NOTE(ollie): Override the one in the place
	*block = newBlock;
}

static void removeLogicBlock(LogicBlock_Set *set, s32 index) {
	//NOTE(ollie): Start from the other end, and move them all up one
	for(int i = (set->logicBlocks.count - 1); i > index; i--) {
		LogicBlock *blockA = getElementFromAlloc(&set->logicBlocks, i, LogicBlock);
		LogicBlock *blockB = getElementFromAlloc(&set->logicBlocks, i - 1, LogicBlock);
		*blockB = *blockA;
	}

	//NOTE(ollie): Now minus 1 of the end
	set->logicBlocks.count--;
}

///////////////////////************* Init var methods ************////////////////////
static inline EasyVM_Variable logicBlock_initVarV2(float x, float y) {
	EasyVM_Variable var = {0};
	var.type = VAR_V2;
	var.v2Val = v2(x, y);
	return var;
}

static inline EasyVM_Variable logicBlock_initVarV3(float x, float y, float z) {
	EasyVM_Variable var = {0};
	var.type = VAR_V3;
	var.v3Val = v3(x, y, z);
	return var;
}

static inline EasyVM_Variable logicBlock_initVarV4(float x, float y, float z, float w) {
	EasyVM_Variable var = {0};
	var.type = VAR_V4;
	var.v4Val = v4(x, y, z, w);
	return var;
}

////////////////////////////////////////////////////////////////////

static void compileLogicBlocks(LogicBlock_Set *set, EasyVM_State *state, V2 fuaxResolution) {
	// sceneDim = Rect2f
	//NOTE(ollie): Reset the bytes
	state->opCodeStreamLength = 0;
	for(u32 blockIndex = 0; blockIndex < set->logicBlocks.count; ++blockIndex) {
	    LogicBlock *block = getElementFromAlloc(&set->logicBlocks, blockIndex, LogicBlock);

	    // OP_CODE_NULL,
	    // OP_CODE_ADD,
	    // OP_CODE_MULT,
	    // OP_CODE_MINUS,
	    // OP_CODE_DIVIDE,
	    // OP_CODE_OFFSET_BASE_POINTER,
	    // OP_CODE_LOAD,
	    // OP_CODE_STORE,
	    // OP_CODE_UPDATE,
	    // OP_CODE_LITERAL, //NOTE(ollie): Reads next byte code and pushes it onto the stack
	    // OP_CODE_PUSH_SCOPE,
	    // OP_CODE_POP_SCOPE,
	    // OP_CODE_GOTO,
	    // OP_CODE_RETURN, //NOTE(ollie): Return from a function
	    // OP_CODE_EXIT, //NOTE(ollie): Exit the program
	    // //NOTE(ollie): Stuff the engine actually does
	    // OP_CODE_DRAW_RECTANGLE,
	    // OP_CODE_CLEAR_COLOR,

	    switch(block->type) {
	    	case LOGIC_BLOCK_PUSH_SCOPE: {
	    		easyVM_writeOpCode(state, OP_CODE_PUSH_SCOPE);
	    	} break;
	    	case LOGIC_BLOCK_POP_SCOPE: {
	    		easyVM_writeOpCode(state, OP_CODE_POP_SCOPE);
	    	} break;
	    	case LOGIC_BLOCK_LOOP: {
	    		
	    		
	    	} break;
	    	case LOGIC_BLOCK_DRAW_RECTANGLE: {
	    		easyVM_writeLiteral(state, logicBlock_initVarV4(1, 0, 0, 1));
	    		easyVM_writeLiteral(state, logicBlock_initVarV2(100, 100));
	    		easyVM_writeLiteral(state, logicBlock_initVarV3(100, 100, NEAR_CLIP_PLANE));
	    		easyVM_writeOpCode(state, OP_CODE_DRAW_RECTANGLE);
	    	} break;
	    	case LOGIC_BLOCK_CLEAR_COLOR: {
	    		easyVM_writeLiteral(state, logicBlock_initVarV4(1, 1, 0, 1));
	    		easyVM_writeLiteral(state, logicBlock_initVarV2(fuaxResolution.x, fuaxResolution.y));
	    		easyVM_writeOpCode(state, OP_CODE_CLEAR_COLOR);
	    	} break;
	    	default: {
	    		//NOTE(ollie): Shouldn't be here!
	    		assert(false);
	    	}
	    }
	}

	 easyVM_writeOpCode(state, OP_CODE_EXIT);
}

