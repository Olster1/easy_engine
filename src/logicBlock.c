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
FUNC(LOGIC_BLOCK_PRINT)\
FUNC(LOGIC_BLOCK_END_WHEN)\
FUNC(LOGIC_BLOCK_END_FUNCTION)\
FUNC(LOGIC_BLOCK_START_FUNCTION)\
FUNC(LOGIC_BLOCK_UPDATE_VARIABLE)\
FUNC(LOGIC_BLOCK_LOOP_END)\
FUNC(LOGIC_BLOCK_WHILE_END)\


typedef enum {
    LOGIC_BLOCK_TYPE(ENUM)
} LogicBlockType;

static char *global_logicBlockTypeStrings[] = { LOGIC_BLOCK_TYPE(STRING) };

static char *global_logicBlockTypeTidyStrings[] = { "Null", "Clear Color", "Push Scope", "Pop Scope", "Loop", "Draw Rectangle", "Play Sound", "When",  "Button Pressed", "Button Held Down", "Button Released", "Draw Circle", "While",  "Draw Cube", "Create Variable", "Print", "End When", "End Function", "Start Function", "Update Variable", "Loop End", "While End"};

static LogicBlockType global_logicBlockTypes[] = { LOGIC_BLOCK_TYPE(ENUM) };

typedef enum {
	LOGIC_BLOCK_PARAM_DEFAULT = 0,
	LOGIC_BLOCK_PARAM_JUST_ALPHA_NUMERIC = 1 << 0,
} LogicBlock_ParamFlag;

typedef struct {
	//NOTE(ollie): This string is allocated as a global string stored in the exe, so we never have to free it. 
	char *name;
	VarType varType;

	LogicBlock_ParamFlag flags;

	//NOTE(ollie): Buffer to edit with the ui, we then create ast out of it when we compile
	s32 innerCount;
	InputBuffer buffers[8];
} LogicParameter;	

typedef struct {
	s32 id;
	LogicBlockType type;

	//NOTE(ollie): For rendering when the user hovers over the block, to make room
	//NOTE(ollie): Top spacing
	float extraSpacing;
	float currentSpacing;

	//NOTE(ollie): For rendering when it is open
	float bottomSpacing;
	float currentBottomSpacing;

	//NOTE(ollie): For when  & loop blocks
	s32 partnerId;


	//NOTE(ollie): For rendering the drop down options
	bool isOpen;

	//NOTE(ollie): Parameters into the functions
	u32 parameterCount;
	LogicParameter parameters[32];	

	//NOTE(ollie): Custom name
	char *customName;

} LogicBlock;


static inline s32 logicBlock_getInnerParamCount(LogicParameter *param) {
	s32 result = 0;

	VarType type = param->varType;
	if(type == VAR_V4) {
		result = 4;
	} else if(type == VAR_V3) {
		result = 3;
	} else if(type == VAR_V2) {
		result = 2;
	} else {
		result = 1;
	}

	return result;

}

static LogicParameter *logicBlock_addParameter(LogicBlock *block, char *name, VarType type, char *valueAsString) {
	assert(block->parameterCount < arrayCount(block->parameters));
	LogicParameter *var = block->parameters + block->parameterCount++;

	//NOTE(ollie): Assign the name
	//NOTE(ollie): This string should only be a global string, so we never have to free it!
	var->name = name;
	var->varType = type;
	var->flags = LOGIC_BLOCK_PARAM_DEFAULT;

	var->innerCount = logicBlock_getInnerParamCount(var);
	
	//Where we first put the string in the buffer, do it by the type of the variable? 
	for(int j = 0; j < var->innerCount; ++j) {
		easyString_initInputBuffer(&var->buffers[j], true);	
		splice(&var->buffers[j], valueAsString, true);	
	}

	return var;
}

typedef struct {
	char *name;
	u32 sizeInBytes;
	bool isReadOnly;
} LogicBlock_DeclaredVar;

typedef struct {
	InfiniteAlloc logicBlocks;

	u32 declaredVariableCount;
	LogicBlock_DeclaredVar declaredVars[1028];
} LogicBlock_Set;

static void logicBlocks_addDeclaredVar(LogicBlock_Set *set, char *name, u32 sizeInBytes, bool isReadOnly) {
	if(set->declaredVariableCount < arrayCount(set->declaredVars)) {
		LogicBlock_DeclaredVar *v = &set->declaredVars[set->declaredVariableCount++];
		v->name = name;
		v->sizeInBytes = sizeInBytes;
	}
}

static bool logicBlocks_hasBeenDeclared(LogicBlock_Set *set, char *name, u32 sizeInBytes) {
	bool result = false;
	for(u32 i = 0; i < set->declaredVariableCount && !result; ++i) {
		LogicBlock_DeclaredVar *v = set->declaredVars + i;

		if(stringsMatchN(v->name, v->sizeInBytes, name, sizeInBytes)) {
			result = true;
			break;
		}
	}
	return result;
}	

static void logicBlock_deinit(LogicBlock *block) {
	for(int i = 0; i < block->parameterCount; ++i) {
		LogicParameter *param = block->parameters + i;
		for(int j = 0; j < param->innerCount; ++j) {
			easyString_deleteInputBuffer(&param->buffers[j]);		
		}
	}
}

static s32 global_logicBlockId = 0;

static void initBlock(LogicBlock *block, LogicBlockType type) {
	//NOTE(ollie): Clear all fields
	zeroStruct(block, LogicBlock);

	//NOTE(ollie): Set the type
	block->type = type;
	block->id = global_logicBlockId++;

	//NOTE(ollie): Add parameters to the functions
	switch(type) {
		case LOGIC_BLOCK_CLEAR_COLOR: {
			logicBlock_addParameter(block, "Color", VAR_V4, "1.0");
		} break;
		case LOGIC_BLOCK_PUSH_SCOPE: {

		} break;
		case LOGIC_BLOCK_POP_SCOPE: {

		} break;
		case LOGIC_BLOCK_LOOP: {
			logicBlock_addParameter(block, "Count", VAR_INT, "10");
		} break;
		case LOGIC_BLOCK_DRAW_RECTANGLE: {
			logicBlock_addParameter(block, "Size", VAR_V2, "1");
			logicBlock_addParameter(block, "Position", VAR_V3, "0.0");
			logicBlock_addParameter(block, "Color", VAR_V4, "1.0");
		} break;
		case LOGIC_BLOCK_PLAY_SOUND: {
			logicBlock_addParameter(block, "File Name", VAR_BOOL, "pop.wav");
		} break;
		case LOGIC_BLOCK_WHEN: {
			logicBlock_addParameter(block, "Value", VAR_BOOL, "true");
		} break;
		case LOGIC_BLOCK_END_WHEN: {

		} break;
		case LOGIC_BLOCK_START_FUNCTION: {
			block->customName = "main:";
		} break;
		case LOGIC_BLOCK_END_FUNCTION: {

		} break;
		case LOGIC_BLOCK_BUTTON_PRESSED: {

		} break;
		case LOGIC_BLOCK_BUTTON_DOWN: {
			logicBlock_addParameter(block, "Button Name", VAR_INT, "BUTTON_LEFT_MOUSE");
		} break;
		case LOGIC_BLOCK_BUTTON_RELEASED: {
			logicBlock_addParameter(block, "Button Name", VAR_INT, "BUTTON_LEFT_MOUSE");
		} break;
		case LOGIC_BLOCK_DRAW_CIRCLE: {
			logicBlock_addParameter(block, "Size", VAR_V2, "1");
			logicBlock_addParameter(block, "Rotation (degrees)", VAR_V3, "0.0");
			logicBlock_addParameter(block, "Position", VAR_V3, "0");
			logicBlock_addParameter(block, "Color", VAR_V4, "1.0");
		} break;
		case LOGIC_BLOCK_WHILE_END: {

		} break;
		case LOGIC_BLOCK_WHILE: {
			logicBlock_addParameter(block, "Value", VAR_BOOL, "false");
		} break;
		case LOGIC_BLOCK_DRAW_CUBE: {
			logicBlock_addParameter(block, "Size", VAR_V3, "1");
			logicBlock_addParameter(block, "Rotation (degrees)", VAR_V3, "0.0");
			logicBlock_addParameter(block, "Position", VAR_V3, "0");
			logicBlock_addParameter(block, "Color", VAR_V4, "1.0");

		} break;
		case LOGIC_BLOCK_VARIABLE_INIT: {
			LogicParameter *param = logicBlock_addParameter(block, "Name", VAR_STRING, "Empty");
			param->flags = LOGIC_BLOCK_PARAM_JUST_ALPHA_NUMERIC;

			logicBlock_addParameter(block, "Value", VAR_NULL, "\"Hey there!\"");
		} break;
		case LOGIC_BLOCK_UPDATE_VARIABLE: {
			LogicParameter *param = logicBlock_addParameter(block, "Name", VAR_STRING, "Empty");
			param->flags = LOGIC_BLOCK_PARAM_JUST_ALPHA_NUMERIC;

			logicBlock_addParameter(block, "New Value", VAR_NULL, "\"Hey there!\"");
		} break;
		case LOGIC_BLOCK_PRINT: {
			logicBlock_addParameter(block, "Value", VAR_STRING, "\"Empty\"");
		} break;
		default: {
			assert(false);
		}
	}
}

static void initLogicBlockSet(LogicBlock_Set *set) {
	//NOTE(ollie): Initialize the array
	set->logicBlocks = initInfinteAlloc(LogicBlock);
}

static void pushLogicBlock(LogicBlock_Set *set, LogicBlockType type) {
	LogicBlock block;
	initBlock(&block, type);
	
	addElementInfinteAlloc_notPointer(&set->logicBlocks, block);
}

static s32 spliceLogicBlock(LogicBlock_Set *set, LogicBlockType type, s32 index) {
	//NOTE(ollie): Add space for the one being added
	//NOTE(ollie): Just has to be empty because the data is about ot be override
	LogicBlock bogusBlock = {0};
	addElementInfinteAlloc_notPointer(&set->logicBlocks, bogusBlock);

	{
		//NOTE(ollie): Start from the other end, and move them all up one
		for(int i = (set->logicBlocks.count - 1); i > index; i--) {
			LogicBlock *blockA = getElementFromAlloc(&set->logicBlocks, i, LogicBlock);
			LogicBlock *blockB = getElementFromAlloc(&set->logicBlocks, i - 1, LogicBlock);
			*blockA = *blockB;
		}
	}

	//NOTE(ollie): Now add the new one
	LogicBlock *block = getElementFromAlloc(&set->logicBlocks, index, LogicBlock);
	assert(block);
	
	initBlock(block, type);

	return block->id;
}

typedef struct {
	LogicBlock *ptr;
	s32 index;
} LogicBlockArrayInfo;
	
#define findLogicBlockPtr(set, id) findLogicBlock_fullInfo(set, id).ptr
#define findLogicBlockIndex(set, id) findLogicBlock_fullInfo(set, id).index 

static LogicBlockArrayInfo findLogicBlock_fullInfo(LogicBlock_Set *set, s32 id) {
	//NOTE(ollie): Init the info
	LogicBlockArrayInfo result = {0};
	result.index = -1;

	//NOTE(ollie): Loop through array
	bool found = false;
	for(int i = 0; i < set->logicBlocks.count && !found; i++) {
		LogicBlock *b = getElementFromAlloc(&set->logicBlocks, i, LogicBlock);
		if(b && b->id == id) {
			result.ptr = b;
			result.index = i; 
			break;
		}
	}

	//NOTE(ollie): return the result
	return result;
}

static void removeLogicBlock(LogicBlock_Set *set, s32 index) {

	//NOTE(ollie): Cleanup any memory being used, this is the input buffers used.
	LogicBlock *blockRemoved = getElementFromAlloc(&set->logicBlocks, index, LogicBlock);
	logicBlock_deinit(blockRemoved);
	////////////////////////////////////////////////////////////////////

	//NOTE(ollie): Start from the other end, and move them all up one
	for(int i = index; i < (set->logicBlocks.count - 1); i++) {
		LogicBlock *blockA = getElementFromAlloc(&set->logicBlocks, i, LogicBlock);
		LogicBlock *blockB = getElementFromAlloc(&set->logicBlocks, i + 1, LogicBlock);

		*blockA = *blockB;
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

static inline EasyVM_Variable logicBlock_initFloat(float value) {
	EasyVM_Variable var = {0};
	var.type = VAR_FLOAT;
	var.floatVal = value;
	return var;
}

static void writeArguments(EasyVM_State *state, LogicBlock *block) {
	//NOTE(ollie): Take memory mark
	MemoryArenaMark memoryMark = takeMemoryMark(&globalScratchArena);
	////////////////////////////////////////////////////////////////////

	for(int paramIndex = 0; paramIndex < block->parameterCount; ++paramIndex) {
		LogicParameter *param = block->parameters + paramIndex;

		/*
		2 + 3 / 9 + (2 * 4)

		2 + o
			|
			3 / 9 + o
					|
					2 * 4

		push value always
		if next node is parent, go down
		push value 
		check next node, if not parent
		push value
		then push math_operator
		then check if next node is parent, do down
		push value always
		check next node, 
		not parent 
		so push value, 
		then math_operator
		no end so go back to parent node
		look back and push math_operator

		continue on -> look past push, 
		go up to parent if finished
		look back push math math_operator 


		push 2 [2]
		push 3 [2 3]
		push 9 [2 3 9]
		divide
		push 2 [2 0.33 2]
		push 4 [2 0.33 2 4]
		mult
		add
		add
		*/

		EasyVM_OpCodeType variableOpcode = ((u32)param->flags & LOGIC_BLOCK_PARAM_JUST_ALPHA_NUMERIC) ? OP_CODE_STORE : OP_CODE_LOAD;

		for(int innerIndex = 0; innerIndex < param->innerCount; ++innerIndex) {

			EasyAst ast = easyAst_generateAst(param->buffers[innerIndex].chars, &globalScratchArena);

			EasyAst_Node *nodeAt = &ast.parentNode;

			assert(nodeAt->type == EASY_AST_NODE_BEGIN_NODE);
			assert(!nodeAt->child);
			
			//NOTE(ollie): Go to the next node of the beginning node
			nodeAt = nodeAt->next;

			//TODO(ollie): Issue message to user that the variable box is empty
			assert(nodeAt);

			bool finished = false;

			while(nodeAt) {

				//NOTE(ollie): When we go down a node, we shouldn't advance 
				bool advanceNode = true;

				///////////////////////*********** Add Literal Opcode **************////////////////////
				if(nodeAt->type == EASY_AST_NODE_PRIMITIVE) {
					//NOTE(ollie): We always push primitive op codes no matter what since it works out that way
					//NOTE(ollie): with our stack machine vm. But we have to do stuff to get the math operators in 
					//NOTE(ollie): order. Mainly we skip the math operator, add whatever is at that node, then go back one 
					//NOTE(ollie): Add add the math operator
					EasyVM_Variable varToWrite = {0}; 
					if(nodeAt->token.type == TOKEN_FLOAT) {
						//NOTE(ollie): Write the float to a variable
						varToWrite.type = VAR_FLOAT;
						varToWrite.floatVal = nodeAt->token.floatVal;
					} else if(nodeAt->token.type == TOKEN_INTEGER) {
						//NOTE(ollie): Write the integer to a variable
						//TODO(ollie): Hack: we just override types to be floats so we don't get error in vm when it expects float not integer
						varToWrite.type = VAR_FLOAT;
						varToWrite.floatVal = (float)nodeAt->token.intVal;
					} else if(nodeAt->token.type == TOKEN_STRING) {
						varToWrite.type = VAR_STRING;
							
						//NOTE(ollie): This string is stored in the text input buffer for the variables. 
						//NOTE(ollie): We want to copy this string to a new buffer (the VM string arena), 
						//NOTE(ollie): Since we will be altering it while VM is running. 
						//NOTE(ollie): In the program it then can look up this pointer to the arena
						varToWrite.stringVal = nullTerminateArena(nodeAt->token.at, nodeAt->token.size, &state->stringArena);
						varToWrite.isPointer = true;
						
					} else {
						assert(false);
					}

					easyVM_writeLiteral(state, varToWrite);

					//NOTE(ollie): There has to be a math operator every second node
					if(nodeAt->next && !(nodeAt->next->type == EASY_AST_NODE_OPERATOR_MATH || nodeAt->next->type == EASY_AST_NODE_EVALUATE || nodeAt->next->type == EASY_AST_NODE_OPERATOR_COMMA)) {
						finished = true;
						easyAst_addError(&ast, "There is supposed to be a math symbol following literals. ", 0);
						assert(false);
					}

				} else if(nodeAt->type == EASY_AST_NODE_VARIABLE) {
					//TODO(ollie): We need to check if this variable has actually been created
					//NOTE(ollie): Like above we store the string of the variable name in the string arena	
					EasyVM_Variable varToWrite = {0}; 				
					varToWrite.type = VAR_STRING;
					varToWrite.isPointer = true;
					varToWrite.stringVal = nullTerminateArena(nodeAt->token.at, nodeAt->token.size, &state->stringArena);
					easyVM_writeLiteral(state, varToWrite);
					//NOTE(ollie): Since we want to do different things based on whether we're declaring a variable,	
					//NOTE(ollie): or using one, a STORE opcode in the first case, & a LOAD opcode in the latter case. 
					if(variableOpcode == OP_CODE_LOAD) {
						easyVM_writeOpCode(state, OP_CODE_LOAD);
					}
				} else if(nodeAt->type == EASY_AST_NODE_EVALUATE) {
					//NOTE(ollie): Do nothing 
				} else if(nodeAt->type == EASY_AST_NODE_OPERATOR_FUNCTION) {

				} else if(nodeAt->type == EASY_AST_NODE_OPERATOR_MATH) {
					int l = 0;
				} else if(nodeAt->type == EASY_AST_NODE_PARENT) {
					assert(nodeAt->child);
					nodeAt = nodeAt->child;

					advanceNode = false;

				} else if(nodeAt->type == EASY_AST_NODE_FUNCTION_COS) {

				} else if(nodeAt->type == EASY_AST_NODE_FUNCTION_SIN) {

				} else if(nodeAt->type == EASY_AST_NODE_OPERATOR_COMMA) {

				} else {
					easyAst_addError(&ast, "There was a unkown symbol in the expression.", 0);
					assert(false);
				}

				if(advanceNode) {
					
					///////////////////////*********** Add Math & Function Opcodes **************////////////////////
					if(nodeAt->prev && nodeAt->prev->type == EASY_AST_NODE_OPERATOR_MATH) {
						easyVM_writeMathOpCode(state, nodeAt);
					} else if(nodeAt->prev && nodeAt->prev->type == EASY_AST_NODE_FUNCTION_COS) {
						//TODO(ollie): Error to user,forgot brackets?
						assert(false);
					} else if(nodeAt->prev && nodeAt->prev->type == EASY_AST_NODE_FUNCTION_SIN) {
						//TODO(ollie): Error to user,forgot brackets?
						assert(false);
					} else if(nodeAt->prev && nodeAt->prev->type == EASY_AST_NODE_OPERATOR_FUNCTION) {
						assert(false);
					}

					////////////////////////////////////////////////////////////////////

					if(nodeAt->next) {
						//NOTE(ollie): Just go to the next node straight away
						nodeAt = nodeAt->next;
					} else {
						bool keepPopping = true;

						while(keepPopping) {
							//NOTE(ollie): If there is a parent of this nod, pop up because we know the current node doesn't have a next node 
							if(nodeAt->parent) {
								assert(!nodeAt->next);
								//NOTE(ollie): Got back to the parent node, however we want to now add the math operator 
								nodeAt = nodeAt->parent;

								///////////////////////*********** Add Math Opcode **************////////////////////
								//NOTE(ollie): We add it here since we about to move on from this node, and the math symbol before this node 
								//NOTE(ollie): wouldn't have been added yet as an opcode 
								if(nodeAt->prev && nodeAt->prev->type == EASY_AST_NODE_OPERATOR_MATH) {
									easyVM_writeMathOpCode(state, nodeAt);
								} else if(nodeAt->prev && nodeAt->prev->type == EASY_AST_NODE_FUNCTION_COS) {
									easyVM_writeOpCode(state, OP_CODE_COSINE);
								} else if(nodeAt->prev && nodeAt->prev->type == EASY_AST_NODE_FUNCTION_SIN) {
									easyVM_writeOpCode(state, OP_CODE_SIN);
								} else if(nodeAt->prev && nodeAt->prev->type == EASY_AST_NODE_OPERATOR_FUNCTION) {
									if(stringsMatchNullN("rand", nodeAt->prev->token.at, nodeAt->prev->token.size)) {
										easyVM_writeOpCode(state, OP_CODE_RANDOM_BETWEEN);
									}
									
								}
								////////////////////////////////////////////////////////////////////
								////////////////////////////////////////////////////////////////////

							} else {
								//NOTE(ollie): We're finished evaluating the syntax tree, so stop walking it.
								assert(!nodeAt->next);
								keepPopping = false;

								//NOTE(ollie): Just for debug
								finished = true;
									
								//NOTE(ollie): Set nodeAt to null so the loop finishes
								nodeAt = 0;
							}

							if(nodeAt && nodeAt->next) {
								//NOTE(ollie):  Since we pop back to the node we would have entered & therefore have already evaluated, we want to go to the next node
								nodeAt = nodeAt->next;
								keepPopping = false;
							} 
						} 
					}	
				}
				
			}
		}
	}
	////////////////////////////////////////////////////////////////////
	releaseMemoryMark(&memoryMark);
}

////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////

static void compileLogicBlocks(LogicBlock_Set *set, EasyVM_State *state, V2 fuaxResolution) {
	///////////////////////********** Reset the VM ***************////////////////////
	easyVM_resetVM(state);
	////////////////////////////////////////////////////////////////////

	//NOTE(ollie): Add global variables
	char *globalTimeVarName = "inbuilt_time";
	easyVM_createVariablePtr(state, globalTimeVarName, VAR_FLOAT, (u8 *)&globalTimeSinceStart);
	logicBlocks_addDeclaredVar(set, globalTimeVarName, easyString_getSizeInBytes_utf8(globalTimeVarName), true);
	////////////////////////////////////////////////////////////////////
	set->declaredVariableCount = 0;

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
	    	case LOGIC_BLOCK_LOOP_END: {

	    	} break;
	    	case LOGIC_BLOCK_LOOP: {
	    		// easyVM_writeOpCode(state, OP_CODE_PUSH_SCOPE);

	    		// writeArguments(state, block);

	    		// {
		    	// 	EasyVM_Variable varToWrite = {0};
		    	// 	varToWrite.type = VAR_INT;


		    	// 	EasyVM_GotoLiteral lit = {0};

		    	// 	lit.offsetAt = easyVM_writeLiteral(state, varToWrite);
		    	// 	lit.partnerId = block->partnerId;

		    	// 	easyVM_pushGoToLiteral(state, lit);
	    		// }

	    		// easyVM_writeOpCode(state, OP_CODE_WHEN);
	    		// easyVM_writeOpCode(state, OP_CODE_PUSH_SCOPE);
	    		
	    	} break;
	    	case LOGIC_BLOCK_DRAW_CUBE: {
	    		writeArguments(state, block);
	    		easyVM_writeOpCode(state, OP_CODE_DRAW_CUBE);
	    	} break;
	    	case LOGIC_BLOCK_DRAW_RECTANGLE: {
	    		writeArguments(state, block);
	    		easyVM_writeOpCode(state, OP_CODE_DRAW_RECTANGLE);
	    	} break;
	    	case LOGIC_BLOCK_CLEAR_COLOR: {
	    		writeArguments(state, block);
	    		easyVM_writeLiteral(state, logicBlock_initFloat(fuaxResolution.x));
	    		easyVM_writeLiteral(state, logicBlock_initFloat(fuaxResolution.y));

	    		easyVM_writeOpCode(state, OP_CODE_CLEAR_COLOR);
	    	} break;
	    	case LOGIC_BLOCK_VARIABLE_INIT: {
	    		char *name = block->parameters[0].buffers[0].chars;
	    		u32 sizeInBytes = easyString_getSizeInBytes_utf8(name);

	    		if(!logicBlocks_hasBeenDeclared(set, name, sizeInBytes)) {
	    			writeArguments(state, block);
	    			easyVM_writeOpCode(state, OP_CODE_STORE);

	    			logicBlocks_addDeclaredVar(set, name, sizeInBytes, false);
	    		}
	    	} break;
	    	case LOGIC_BLOCK_UPDATE_VARIABLE: {
	    		char *name = block->parameters[0].buffers[0].chars;
	    		if(logicBlocks_hasBeenDeclared(set, name, easyString_getSizeInBytes_utf8(name))) {
		    		writeArguments(state, block);
		    		easyVM_writeOpCode(state, OP_CODE_UPDATE);
	    		}
	    	} break;
	    	case LOGIC_BLOCK_PRINT: {
	    		writeArguments(state, block);
	    		easyVM_writeOpCode(state, OP_CODE_PRINT);
	    	} break;
	    	case LOGIC_BLOCK_END_WHEN: {
	    		easyVM_writeOpCode(state, OP_CODE_POP_SCOPE);

	    		EasyVM_GotoLiteral lit = easyVM_popGoToLiteral(state);
	    		assert(lit.partnerId == block->id);

	    		EasyVM_Variable *at = (EasyVM_Variable *)(state->opCodeStream + lit.offsetAt);	
	    		assert(at->type == VAR_INT);
	    		//NOTE(ollie): Write where the opcode stream is up to 
	    		at->intVal = state->opCodeStreamLength;

	    		

	    	} break;
	    	case LOGIC_BLOCK_WHEN: {
	    		writeArguments(state, block);

	    		{
		    		EasyVM_Variable varToWrite = {0};
		    		varToWrite.type = VAR_INT;


		    		EasyVM_GotoLiteral lit = {0};

		    		lit.offsetAt = easyVM_writeLiteral(state, varToWrite);
		    		lit.partnerId = block->partnerId;

		    		easyVM_pushGoToLiteral(state, lit);
	    		}

	    		easyVM_writeOpCode(state, OP_CODE_WHEN);
	    		easyVM_writeOpCode(state, OP_CODE_PUSH_SCOPE);
	    	} break;
	    	case LOGIC_BLOCK_WHILE_END: {
	    		easyVM_writeOpCode(state, OP_CODE_POP_SCOPE);

	    		EasyVM_GotoLiteral lit = easyVM_popGoToLiteral(state);
	    		assert(lit.partnerId == block->id);

	    		//NOTE(ollie): Write Where to jump back to, to retest the if statement of the loop
	    		EasyVM_Variable varToWrite = {0};
	    		varToWrite.type = VAR_INT;
	    		varToWrite.intVal = lit.gotoLoopEnd;
	    		easyVM_writeLiteral(state, varToWrite);
	    		easyVM_writeOpCode(state, OP_CODE_GOTO);

	    		//NOTE(ollie): Now write Where to jump to, to avoid the while loop
	    		EasyVM_Variable *at = (EasyVM_Variable *)(state->opCodeStream + lit.offsetAt);	
	    		assert(at->type == VAR_INT);
	    		//NOTE(ollie): Write where the opcode stream is up to 
	    		at->intVal = state->opCodeStreamLength;

	    	} break;
	    	case LOGIC_BLOCK_WHILE: {
	    		u32 streamBeforeWhile = state->opCodeStreamLength;
	    		writeArguments(state, block);

	    		{
		    		EasyVM_Variable varToWrite = {0};
		    		varToWrite.type = VAR_INT;


		    		EasyVM_GotoLiteral lit = {0};

		    		lit.offsetAt = easyVM_writeLiteral(state, varToWrite);
		    		lit.partnerId = block->partnerId;
		    		lit.gotoLoopEnd = streamBeforeWhile;

		    		easyVM_pushGoToLiteral(state, lit);
	    		}

	    		easyVM_writeOpCode(state, OP_CODE_WHEN);
	    		easyVM_writeOpCode(state, OP_CODE_PUSH_SCOPE);
	    	} break;
	    	default: {
	    		//NOTE(ollie): Shouldn't be here!
	    		// assert(false);
	    	}
	    }
	}

	 easyVM_writeOpCode(state, OP_CODE_EXIT);
}


////////////////////////////////////////////////////////////////////

static LogicBlockType logicBlock_getEndBlockType(LogicBlockType type) {
	LogicBlockType result = LOGIC_BLOCK_NULL;

	switch(type) {
		case LOGIC_BLOCK_WHEN: {
			result = LOGIC_BLOCK_END_WHEN;
		} break;
		case LOGIC_BLOCK_LOOP: {
			result = LOGIC_BLOCK_LOOP_END;
		} break;
		case LOGIC_BLOCK_WHILE: {
			result = LOGIC_BLOCK_WHILE_END;
		} break;
		default: {
			assert(false);
		}
	}
	return result;
}

