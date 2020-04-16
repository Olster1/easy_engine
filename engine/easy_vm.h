/*
	Stack based Virtual machine
*/

typedef enum {
	OP_CODE_NULL,
	OP_CODE_ADD,
	OP_CODE_MULT,
	OP_CODE_MINUS,
	OP_CODE_DIVIDE,
	OP_CODE_OFFSET_BASE_POINTER,
	OP_CODE_LOAD,
	OP_CODE_STORE,
	OP_CODE_UPDATE,
	OP_CODE_LITERAL, //NOTE(ollie): Reads next byte code and pushes it onto the stack
	OP_CODE_PUSH_SCOPE,
	OP_CODE_POP_SCOPE,
	OP_CODE_GOTO,
	OP_CODE_PRINT, //NOTE(ollie): Printing to the console
	OP_CODE_RETURN, //NOTE(ollie): Return from a function
	OP_CODE_EXIT, //NOTE(ollie): Exit the program

	//NOTE(ollie): Stuff the engine actually does
	OP_CODE_DRAW_RECTANGLE,
	OP_CODE_CLEAR_COLOR,
} EasyVM_OpCodeType;
	
#define EASY_VM_OP_CODE_MAGIC 'EASY'
//NOTE(ollie): For type safety we decided to just explicity create opcode so we know what we're getting
typedef struct {
	//NOTE(ollie): Array
	u32 magicNumber;

	EasyVM_OpCodeType type;
	u32 nextOpCode;
} EasyVM_OpCode;

typedef struct {
	//NOTE(ollie): Allocated, so have to free
	char *name; //NOTE(ollie): also check if it's a variable or literal
	//NOTE(ollie): Scope depth
	s32 depth;

	////////////////////////////////////////////////////////////////////
	//NOTE(ollie): Actual variable

	VarType type;
	union {
		struct {
			s32 intVal;
		};
		struct {
			float floatVal;
		};
		struct {
			bool boolVal;
		};
		struct {
			
		};
		struct {
			V2 v2Val;
		};
		struct {
			V3 v3Val;
		};
		struct {
			V4 v4Val;
		};
		struct {
			//NOTE(ollie): Point to a string allocated in memory
			char *stringVal;
		};
		struct {
			float floatValues[4];
		};
		struct {
			u8 bytes[16];
		};
	};
} EasyVM_Variable;

typedef struct {
	s32 offsetAt;
	s32 basePointerAt;
} EasyVM_ScopeInfo;

typedef struct {
	//NOTE(ollie): A mixture of op codes & actual data
	u8 *opCodeStream;	
	u32 opCodeStreamLength; //NOTE(ollie): In bytes
	u32 opCodeStreamTotal;

	////////////////////////////////////////////////////////////////////

	//NOTE(ollie): The stack to push and pop things onto
	s32 offsetAt;
	u32 stackSize;

	u8 *stack;

	//NOTE(ollie): For where the instructions start
	s32 basePointerAt; 

	//NOTE(ollie): Scope offsets saved to revert stack 
	u32 scopeOffsetCount;
	EasyVM_ScopeInfo scopeOffsetAt[2048];

	//NOTE(ollie): For when we enter a new scope
	s32 depthAt;
	////////////////////////////////////////////////////////////////////

	u32 localVariableCount;
	EasyVM_Variable localVariables[512];

	////////////////////////////////////////////////////////////////////

	//NOTE(ollie): Where all the strings live for the lifetime of the VM
	Arena stringArena;
	//NOTE(ollie): To reset the arena when were done with the VM
	MemoryArenaMark stringMemoryMark;

} EasyVM_State;

#define EASY_ENGINE_VIRTUAL_MACHINE_INCREMENT_BYTES 2048

static void easyVM_resetVM(EasyVM_State *state) {
	releaseMemoryMark(&state->stringMemoryMark);

	state->offsetAt = 0;
	state->opCodeStreamLength = 0;

	state->depthAt= 0;
	state->scopeOffsetCount = 0;
	state->basePointerAt = 0;

	state->localVariableCount = 0;

	state ->stringMemoryMark = takeMemoryMark(&state->stringArena);

}

static void easyVM_initVM(EasyVM_State *state) {
	state->opCodeStreamTotal = EASY_ENGINE_VIRTUAL_MACHINE_INCREMENT_BYTES;
	state->opCodeStream = (u8 *)easyPlatform_allocateMemory(state->opCodeStreamTotal, EASY_PLATFORM_MEMORY_ZERO);

	state->stackSize = 20000;
	state->stack = (u8 *)pushSize(&globalLongTermArena, state->stackSize);

	state->stringArena = createArena(Kilobytes(20));

	//NOTE(ollie): Just take one so we can release it, so we can keep in sync
	state ->stringMemoryMark = takeMemoryMark(&state->stringArena);
	easyVM_resetVM(state);
}



///////////////////////*********** Functions to interact with **************////////////////////

static inline void easyVM_pushScopeDepth(EasyVM_State *state) {
	state->depthAt++;

	assert(state->scopeOffsetCount < arrayCount(state->scopeOffsetAt));
	EasyVM_ScopeInfo *scope = &state->scopeOffsetAt[state->scopeOffsetCount++];
	scope->offsetAt = state->offsetAt;
	scope->basePointerAt = state->basePointerAt;
}

static inline void easyVM_popScopeDepth(EasyVM_State *state) {
	state->depthAt--;

	//NOTE(ollie): Walk backwards so we know when we reach a different depth we can exit
	for(int i = (state->localVariableCount - 1); i >= 0; --i) {
		EasyVM_Variable *l = &state->localVariables[i];

		if(l->depth > state->depthAt) {
			//NOTE(ollie): Cleanup any memory
			if(l->type == VAR_STRING) {
				easyPlatform_freeMemory(l->stringVal);	
			}

			easyPlatform_freeMemory(l->name);	

			//NOTE(ollie): Get rid of the variable
			state->localVariableCount--;
		}
	}

	//NOTE(ollie): Revert stack back
	assert(state->scopeOffsetCount > 0);
	EasyVM_ScopeInfo *scope = &state->scopeOffsetAt[--state->scopeOffsetCount];
	state->offsetAt = scope->offsetAt;
	state->basePointerAt = scope->basePointerAt;

}

static void easyVM_assignVariable(EasyVM_Variable *var, VarType type, u8 *value) {
	var->name = 0;
	var->type = type;

	if(value) {
		switch(type) {
			case VAR_INT: {
				var->intVal = *((s32 *)value);
			} break;
			case VAR_V2: {
				var->v2Val = *((V2 *)value);
			} break;
			case VAR_V3: {
				var->v3Val = *((V3 *)value);
			} break;
			case VAR_V4: {
				var->v4Val = *((V4 *)value);
			} break;
			case VAR_FLOAT: {
				var->floatVal = *((float *)value);
			} break;
			case VAR_BOOL: {
				var->boolVal = *((bool *)value);
			} break;
			case VAR_STRING: {
				//NOTE(ollie): We just assume memory is handled for us for strings
				//TODO(ollie): Will have to revist this when variables get added
				var->stringVal = (char *)value;
				// u32 bytes = easyString_getStringSizeInBytes(value);
				// var->stringVal = (char *)easyPlatform_allocateMemory(bytes, EASY_PLATFORM_MEMORY_ZERO);

				// //NOTE(ollie): Copy the string
				// easyPlatform_copyMemory(var->stringVal, value, bytes);
			} break;
		}
	}
}

//NOTE(ollie): for literal values
static inline void easyVM_pushOnStack_literal(EasyVM_State *state, VarType type, u8 *value) {
	u32 sizeOfVar = sizeof(EasyVM_Variable);

	if((state->offsetAt + sizeOfVar) <= state->stackSize) {
		EasyVM_Variable *var = (EasyVM_Variable *)(state->stack + state->offsetAt);
		easyVM_assignVariable(var, type, value);	
		state->offsetAt += sizeOfVar;
	} else {
		//NOTE(ollie): Message the user stack overflow, shouldn't get there though i think
		assert(false);
	}
}


//NOTE(ollie): for variable values
static inline void easyVM_pushOnStack_variable(EasyVM_State *state, EasyVM_Variable *var) {
	u32 sizeOfVar = sizeof(EasyVM_Variable);

	if((state->offsetAt + sizeOfVar) <= state->stackSize) {
		EasyVM_Variable *varOnStack = (EasyVM_Variable *)(state->stack + state->offsetAt);
		*varOnStack = *var;	
		state->offsetAt += sizeOfVar;
	} else {
		//NOTE(ollie): Message the user stack overflow, shouldn't get there though i think
		assert(false);
	}
}

static inline EasyVM_Variable easyVM_popOffStack(EasyVM_State *state, VarType type) {
	state->offsetAt -= sizeof(EasyVM_Variable);
	assert(state->offsetAt >= 0);

	//NOTE(ollie): We copy it, not return pointer, since we basically freed this memory already from above. 
	EasyVM_Variable var = *(EasyVM_Variable *)(state->stack + state->offsetAt);

	assert(type == VAR_NULL || var.type == type);

	return var;
}

///////////////////////************ Load & Store instructions *************////////////////////
static EasyVM_Variable *easyVM_findVariable(EasyVM_State *state, char *name) {
	EasyVM_Variable *result = 0;
	//TODO(ollie): Either move to hash table or better yet, indexe ids
	//NOTE(ollie): Walk backwards so we know when we reach variable faster?
	for(int i = (state->localVariableCount - 1); i >= 0 && !result; --i) {
		EasyVM_Variable *l = &state->localVariables[i];

		if(cmpStrNull(l->name, name)) {
			result = l;
			break;
		}
	}
	return result;
}

static inline void easyVM_loadVariableOntoStack(EasyVM_State *state, char *name) {
	EasyVM_Variable *var = easyVM_findVariable(state, name);

	easyVM_pushOnStack_variable(state, var);
}

//NOTE(ollie): OP_CODE_UPDATE
static void easyVM_updateVariable(EasyVM_State *state, char *name, EasyVM_Variable *incomingVar) {
	EasyVM_Variable *var = easyVM_findVariable(state, name);
	assert(var);

	//NOTE(ollie): Copy Variable to storage
	*var = *incomingVar;
}

//NOTE(ollie): STORE INSTRUCTION
static EasyVM_Variable *easyVM_createVariable(EasyVM_State *state, char *name, VarType type, u8 *value) {
	EasyVM_Variable *var = 0;

	if(state->localVariableCount < arrayCount(state->localVariables)) {

		var = state->localVariables + state->localVariableCount++;

		easyVM_assignVariable(var, type, value);

		//NOTE(ollie): Only if it is a local variable do we set the name
		u32 nameSizeInBytes = easyString_getStringSizeInBytes((u8 *)name);
		//NOTE(ollie): Allocate the memory for the string
		var->name = (char *)easyPlatform_allocateMemory(nameSizeInBytes, EASY_PLATFORM_MEMORY_ZERO);

		//NOTE(ollie): Copy the string
		easyPlatform_copyMemory(var->name, name, nameSizeInBytes);	

	} else {
		//TODO(ollie): Show error message to user
		assert(false);
	}

	var->depth = state->depthAt;

	return var;
}

static V4 easyVM_buildV4FromStack(EasyVM_State *state) {
	V4 result = {0};

	result.w = easyVM_popOffStack(state, VAR_FLOAT).floatVal;
	result.z = easyVM_popOffStack(state, VAR_FLOAT).floatVal;
	result.y = easyVM_popOffStack(state, VAR_FLOAT).floatVal;
	result.x = easyVM_popOffStack(state, VAR_FLOAT).floatVal;

	return result;
}

static V3 easyVM_buildV3FromStack(EasyVM_State *state) {
	V3 result = {0};

	result.z = easyVM_popOffStack(state, VAR_FLOAT).floatVal;
	result.y = easyVM_popOffStack(state, VAR_FLOAT).floatVal;
	result.x = easyVM_popOffStack(state, VAR_FLOAT).floatVal;

	return result;
}

static V2 easyVM_buildV2FromStack(EasyVM_State *state) {
	V2 result = {0};

	result.y = easyVM_popOffStack(state, VAR_FLOAT).floatVal;
	result.x = easyVM_popOffStack(state, VAR_FLOAT).floatVal;

	return result;
}


///////////////////////************ Below is implementation*************////////////////////

static void easyVM_runVM(EasyVM_State *state) {
	u32 byteOffset = 0; //NOTE(ollie): Into the opcodes

	bool running = true;
	while(byteOffset < state->opCodeStreamLength && running) {

		EasyVM_OpCode *opCode = (EasyVM_OpCode *)(state->opCodeStream + byteOffset);

		//NOTE(ollie): For debugging, to ensure were reading the right bytes. Remove from release build
		assert(opCode->magicNumber == EASY_VM_OP_CODE_MAGIC);

		switch(opCode->type) {
			case OP_CODE_ADD: {
				EasyVM_Variable var1 = easyVM_popOffStack(state, VAR_NULL);
				EasyVM_Variable var2 = easyVM_popOffStack(state, VAR_NULL);

				EasyVM_Variable result = {0};

				if(var1.type == VAR_INT) {
					if(var2.type == VAR_FLOAT) {
						result.floatVal = var1.intVal + var2.floatVal; 
						result.type = VAR_FLOAT;
					}
					if(var2.type == VAR_INT) {
						result.intVal = var1.intVal + var2.intVal; 
						result.type = VAR_INT;
					}
				}

				if(var1.type == VAR_FLOAT) {
					if(var2.type == VAR_FLOAT) {
						result.floatVal = var1.floatVal + var2.floatVal; 
						result.type = VAR_FLOAT;
					}
					if(var2.type == VAR_INT) {
						result.intVal = var1.floatVal + var2.intVal; 
						result.type = VAR_FLOAT;
					}
				}

				easyVM_pushOnStack_literal(state, result.type, result.bytes);
			} break;
			case OP_CODE_MULT: {
				EasyVM_Variable var1 = easyVM_popOffStack(state, VAR_NULL);
				EasyVM_Variable var2 = easyVM_popOffStack(state, VAR_NULL);

				EasyVM_Variable result = {0};

				if(var1.type == VAR_INT) {
					if(var2.type == VAR_FLOAT) {
						result.floatVal = var1.intVal * var2.floatVal; 
						result.type = VAR_FLOAT;
					}
					if(var2.type == VAR_INT) {
						result.intVal = var1.intVal * var2.intVal; 
						result.type = VAR_INT;
					}
				}

				if(var1.type == VAR_FLOAT) {
					if(var2.type == VAR_FLOAT) {
						result.floatVal = var1.floatVal * var2.floatVal; 
						result.type = VAR_FLOAT;
					}
					if(var2.type == VAR_INT) {
						result.intVal = var1.floatVal * var2.intVal; 
						result.type = VAR_FLOAT;
					}
				}

				easyVM_pushOnStack_literal(state, result.type, result.bytes);
			} break;
			case OP_CODE_MINUS: {
				EasyVM_Variable var1 = easyVM_popOffStack(state, VAR_NULL);
				EasyVM_Variable var2 = easyVM_popOffStack(state, VAR_NULL);

				EasyVM_Variable result = {0};

				if(var1.type == VAR_INT) {
					if(var2.type == VAR_FLOAT) {
						result.floatVal = var2.floatVal - var1.intVal; 
						result.type = VAR_FLOAT;
					}
					if(var2.type == VAR_INT) {
						result.intVal = var2.intVal - var1.intVal; 
						result.type = VAR_INT;
					}
				}

				if(var1.type == VAR_FLOAT) {
					if(var2.type == VAR_FLOAT) {
						result.floatVal = var2.floatVal - var1.floatVal; 
						result.type = VAR_FLOAT;
					}
					if(var2.type == VAR_INT) {
						result.intVal = var2.intVal - var1.floatVal; 
						result.type = VAR_FLOAT;
					}
				}

				easyVM_pushOnStack_literal(state, result.type, result.bytes);
			} break;
			case OP_CODE_DIVIDE: {
				EasyVM_Variable var1 = easyVM_popOffStack(state, VAR_NULL);
				EasyVM_Variable var2 = easyVM_popOffStack(state, VAR_NULL);

				EasyVM_Variable result = {0};

				if(var1.type == VAR_INT) {
					if(var2.type == VAR_FLOAT) {
						result.floatVal = var2.floatVal / var1.intVal; 
						result.type = VAR_FLOAT;
					}
					if(var2.type == VAR_INT) {
						result.intVal = var2.intVal / var1.intVal; 
						result.type = VAR_INT;
					}
				}

				if(var1.type == VAR_FLOAT) {
					if(var2.type == VAR_FLOAT) {
						result.floatVal = var2.floatVal / var1.floatVal; 
						result.type = VAR_FLOAT;
					}
					if(var2.type == VAR_INT) {
						result.intVal = var2.intVal / var1.floatVal; 
						result.type = VAR_FLOAT;
					}
				}

				easyVM_pushOnStack_literal(state, result.type, result.bytes);
			} break;
			case OP_CODE_OFFSET_BASE_POINTER: {

			} break;
			case OP_CODE_LOAD: {
				char *varName = easyVM_popOffStack(state, VAR_STRING).stringVal;

				easyVM_loadVariableOntoStack(state, varName);

				//NOTE(ollie): Free the string now
				easyPlatform_freeMemory(varName);
								
				byteOffset = opCode->nextOpCode;
			} break;
			case OP_CODE_STORE: {
				char *varName = easyVM_popOffStack(state, VAR_STRING).stringVal;

				EasyVM_Variable var = easyVM_popOffStack(state, VAR_NULL);

				easyVM_createVariable(state, varName, var.type, var.bytes);

				//NOTE(ollie): Free the string now
				easyPlatform_freeMemory(varName);

			} break;
			case OP_CODE_UPDATE: {
				char *varName = easyVM_popOffStack(state, VAR_STRING).stringVal;

				EasyVM_Variable var = easyVM_popOffStack(state, VAR_NULL);

				easyVM_updateVariable(state, varName, &var);

				//NOTE(ollie): Free the string now
				easyPlatform_freeMemory(varName);

			} break;
			case OP_CODE_LITERAL: {
				byteOffset += sizeof(EasyVM_OpCode);

				EasyVM_Variable *nextVar = (EasyVM_Variable *)(state->opCodeStream + byteOffset);

				void *ptrToData = nextVar->bytes;
				if(nextVar->type == VAR_STRING) {
					ptrToData = nextVar->stringVal;
				}
				easyVM_pushOnStack_literal(state, nextVar->type, (u8 *)ptrToData);
			} break;
			case OP_CODE_PUSH_SCOPE: {
				easyVM_pushScopeDepth(state);
			} break;
			case OP_CODE_POP_SCOPE: {
				easyVM_popScopeDepth(state);
			} break;
			case OP_CODE_GOTO: {
				s32 offsetFromBasePointer = easyVM_popOffStack(state, VAR_INT).intVal;
				byteOffset = offsetFromBasePointer + state->basePointerAt; 
			} break;
			case OP_CODE_RETURN: {

			} break;
			case OP_CODE_EXIT: {
				state->offsetAt = 0;

				//NOTE(ollie): Free all string memory
				state->depthAt= 0;
				state->scopeOffsetCount = 0;
				state->basePointerAt = 0;
				state->localVariableCount = 0;

				running = false;
			} break;
			case OP_CODE_DRAW_RECTANGLE: {
				V4 color = easyVM_buildV4FromStack(state);
				V3 centerPos = easyVM_buildV3FromStack(state);
				V2 dim = easyVM_buildV2FromStack(state);
				
				setModelTransform(globalRenderGroup, Matrix4_translate(Matrix4_scale(mat4(), v3(dim.x, dim.y, 0)), centerPos));
				renderDrawQuad(globalRenderGroup, color);
			} break;
			case OP_CODE_CLEAR_COLOR: {
				drawRenderGroup(globalRenderGroup, (RenderDrawSettings)(RENDER_DRAW_SORT));
				
				V2 dim = easyVM_buildV2FromStack(state);
				V4 color = easyVM_buildV4FromStack(state);

				bool oldVal = renderDisableDepthTest(globalRenderGroup);
				renderDrawRectCenterDim(v3(0.5f*dim.x, 0.5f*dim.y, NEAR_CLIP_PLANE), dim, color, 0, mat4(), OrthoMatrixToScreen_BottomLeft(dim.x, dim.y));
				globalRenderGroup->currentDepthTest = oldVal;

				renderClearDepthBuffer(globalRenderGroup->currentBufferId);
				drawRenderGroup(globalRenderGroup, (RenderDrawSettings)(RENDER_DRAW_SORT));
			} break;
			case OP_CODE_PRINT: {
				EasyVM_Variable var = easyVM_popOffStack(state, VAR_NULL);

				bool freeBuffer= true;
				char *buffer = 0;
				switch(var.type) {
					case VAR_INT: {
						s32 length = snprintf(0, 0, "%d", var.intVal);

						s32 sizeOfBuffer = length*sizeof(char);

						buffer = (char *)easyPlatform_allocateMemory(sizeOfBuffer + 1, EASY_PLATFORM_MEMORY_ZERO);

						snprintf(buffer, length, "%d", var.intVal);

					} break;
					case VAR_FLOAT: {
						s32 length = snprintf(0, 0, "%f", var.floatVal);

						s32 sizeOfBuffer = length*sizeof(char);

						buffer = (char *)easyPlatform_allocateMemory(sizeOfBuffer + 1, EASY_PLATFORM_MEMORY_ZERO);

						snprintf(buffer, length, "%f", var.floatVal);
					} break;
					case VAR_STRING: {
						buffer = var.stringVal;
						freeBuffer = false;
					} break;
					default: {
						assert(false);
					}
				}
				easyConsole_addToStream(DEBUG_globalEasyConsole, buffer);

				if(freeBuffer) {
					easyPlatform_freeMemory(buffer);	
				}
				
			} break;
			default: {
				assert(false);
			}
		}

		byteOffset = opCode->nextOpCode;
	}
	

	assert(byteOffset <= state->opCodeStreamLength);

} 

static void easyVM_tryExpandOpcodeMemory(EasyVM_State *state, u32 size) {
	u32 amountToIncrement = max(size, EASY_ENGINE_VIRTUAL_MACHINE_INCREMENT_BYTES);
	if((state->opCodeStreamLength + size) >= state->opCodeStreamTotal) {

		state->opCodeStreamTotal += amountToIncrement;

		state->opCodeStream = easyPlatform_reallocMemory(state->opCodeStream, state->opCodeStreamLength, state->opCodeStreamTotal);
	}
}

static EasyVM_OpCode *easyVM_writeOpCode(EasyVM_State *state, EasyVM_OpCodeType type) {
	easyVM_tryExpandOpcodeMemory(state, sizeof(EasyVM_OpCode));

	EasyVM_OpCode *at = (EasyVM_OpCode *)(state->opCodeStream + state->opCodeStreamLength);	

	at->magicNumber = EASY_VM_OP_CODE_MAGIC;

	at->type = type;

	state->opCodeStreamLength += sizeof(EasyVM_OpCode);
	assert(state->opCodeStreamLength <= state->opCodeStreamTotal);
	at->nextOpCode = state->opCodeStreamLength;

	return at;
}

static void easyVM_writeMathOpCode(EasyVM_State *state, EasyAst_Node *nodeAt) {
	if(nodeAt->prev->token.type == TOKEN_PLUS) {
		easyVM_writeOpCode(state, OP_CODE_ADD);

	} else if(nodeAt->prev->token.type == TOKEN_MINUS) {
		easyVM_writeOpCode(state, OP_CODE_MINUS);

	} else if(nodeAt->prev->token.type == TOKEN_ASTRIX) {
		easyVM_writeOpCode(state, OP_CODE_MULT);

	} else if(nodeAt->prev->token.type == TOKEN_FORWARD_SLASH) {
		easyVM_writeOpCode(state, OP_CODE_DIVIDE);
	} else {
		assert(!"something went wrong!");
	}
}

static void easyVM_writeLiteral(EasyVM_State *state, EasyVM_Variable var) {
	EasyVM_OpCode *opCode = easyVM_writeOpCode(state, OP_CODE_LITERAL);

	u32 sizeOfAddition = sizeof(EasyVM_Variable);
	easyVM_tryExpandOpcodeMemory(state, sizeOfAddition);

	EasyVM_Variable *at = (EasyVM_Variable *)(state->opCodeStream + state->opCodeStreamLength);	

	*at = var;

	state->opCodeStreamLength += sizeOfAddition;
	assert(state->opCodeStreamLength <= state->opCodeStreamTotal);

	//NOTE(ollie): Advance where the next opcode is
	opCode->nextOpCode = state->opCodeStreamLength;
	
}


////////////////////////////////////////////////////////////////////



