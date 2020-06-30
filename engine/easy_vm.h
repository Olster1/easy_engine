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
	OP_CODE_WHEN,
	OP_CODE_GOTO,
	OP_CODE_PRINT, //NOTE(ollie): Printing to the console
	OP_CODE_RETURN, //NOTE(ollie): Return from a function
	OP_CODE_EXIT, //NOTE(ollie): Exit the program
	//NOTE(ollie): Intrinsics 
	OP_CODE_SIN,
	OP_CODE_COSINE,

	OP_CODE_EQUAL_TO,

	OP_CODE_LESS_THAN,
	OP_CODE_GREATER_THAN,
	OP_CODE_LESS_THAN_OR_EQUAL_TO,
	OP_CODE_GREATER_THAN_OR_EQUAL_TO,

	//NOTE(ollie): Stuff the engine actually does
	OP_CODE_DRAW_CUBE,
	OP_CODE_DRAW_RECTANGLE,
	OP_CODE_CLEAR_COLOR,
	OP_CODE_RANDOM_BETWEEN,
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

	//NOTE(ollie): Whether the value is a pointer
	bool isPointer;

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
			void *ptrVal;
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
	s32 partnerId;
	u32 offsetAt; //Value that gets patched
	u32 gotoLoopEnd;
} EasyVM_GotoLiteral;

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

	///////////////////////*********** Used for compiling **************////////////////////
	EasyVM_GotoLiteral gotoLiterals[1028];
	u32 gotoLiteralsCount;
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

	state->gotoLiteralsCount = 0;

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

static inline void easyVM_emptyLocalVariables_exceptGlobals(EasyVM_State *state) {
	state->depthAt = 0;

	//NOTE(ollie): Walk backwards so we know when we reach a different depth we can exit
	for(int i = (state->localVariableCount - 1); i >= 0; --i) {
		EasyVM_Variable *l = &state->localVariables[i];

		if(l->depth > state->depthAt) {
			//NOTE(ollie): Get rid of the variable
			state->localVariableCount--;
		}
	}
}

static inline void easyVM_popScopeDepth(EasyVM_State *state) {
	state->depthAt--;

	//NOTE(ollie): Walk backwards so we know when we reach a different depth we can exit
	for(int i = (state->localVariableCount - 1); i >= 0; --i) {
		EasyVM_Variable *l = &state->localVariables[i];

		if(l->depth > state->depthAt) {
			//NOTE(ollie): Cleanup any memory

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

static EasyVM_Variable *easyVM_assignVariable(EasyVM_Variable *var, VarType type, u8 *value, bool isPointer) {
	var->name = 0;
	var->type = type;

	if(value) {
		if(isPointer) {
			var->ptrVal = value;
		} else {
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

					//NOTE(ollie): SHould always be a pointer, so shouldn't get here
					assert(false);
				} break;
			}
		}
	}

	var->isPointer = isPointer;

	return var;
}

//NOTE(ollie): for literal values
static inline void easyVM_pushOnStack_literal(EasyVM_State *state, VarType type, u8 *value, bool isPointer) {
	u32 sizeOfVar = sizeof(EasyVM_Variable);

	if((state->offsetAt + sizeOfVar) <= state->stackSize) {
		EasyVM_Variable *var = (EasyVM_Variable *)(state->stack + state->offsetAt);
		easyVM_assignVariable(var, type, value, isPointer);	
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

//NOTE(ollie): Finds the variable in the variable pool
static EasyVM_Variable *easyVM_findVariable(EasyVM_State *state, char *name) {
	EasyVM_Variable *result = 0;
	//TODO(ollie): Either move to hash table or better yet, index ids
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

//NOTE(ollie): Loads variable onto the stack from the variable pool 
//NOTE(ollie): OP_CODE_LOAD
static inline void easyVM_loadVariableOntoStack(EasyVM_State *state, char *name) {
	EasyVM_Variable *var = easyVM_findVariable(state, name);

	EasyVM_Variable varOnStack = {0};

	///////////////////////************* If pointer, dereference ************////////////////////
	if(var->isPointer) {
		bool varOnStackPtr = (var->type == VAR_STRING);
		easyVM_assignVariable(&varOnStack, var->type, (u8 *)var->ptrVal, varOnStackPtr);
		varOnStack.name = name;
	} else {
		easyVM_assignVariable(&varOnStack, var->type, var->bytes, false);
		varOnStack.name = name;
	}
	////////////////////////////////////////////////////////////////////

	easyVM_pushOnStack_variable(state, &varOnStack);
}

//NOTE(ollie): OP_CODE_UPDATE
static void easyVM_updateVariable(EasyVM_State *state, char *name, EasyVM_Variable *incomingVar) {
	EasyVM_Variable *var = easyVM_findVariable(state, name);
	assert(var);

	//TODO(ollie): This is error prone for future stuff we stick in a variable. 
	//TODO(ollie): I'm not sure the best way to handle this?
	incomingVar->depth = var->depth;
	incomingVar->isPointer = var->isPointer;

	//NOTE(ollie): Copy Variable to storage
	*var = *incomingVar;
}

//NOTE(ollie): OP_CODE_STORE
static EasyVM_Variable *easyVM_createVariable(EasyVM_State *state, char *name, VarType type, u8 *value, bool isPointer) {
	EasyVM_Variable *var = 0;

	if(state->localVariableCount < arrayCount(state->localVariables)) {

		var = state->localVariables + state->localVariableCount++;

		easyVM_assignVariable(var, type, value, isPointer);

		//NOTE(ollie): The name exists in the data, so don't need to allocate anything, & the string will be around
		//NOTE(ollie): For the lifetime of the virtual machine
		var->name = name;

	} else {
		//NOTE(ollie): local variable array full up
		//NOTE(ollie): Shouldn't happen for user
		//TODO(ollie): Show error message to user
		assert(false);
	}

	var->depth = state->depthAt;

	return var;
}

static inline EasyVM_Variable *easyVM_createVariablePtr(EasyVM_State *state, char *name, VarType type, u8 *value) {
	EasyVM_Variable *var = easyVM_createVariable(state, name, type, 0, true);
	
	var->isPointer = true;
	var->ptrVal = value;

	return var;
}

////////////////////////////////////////////////////////////////////

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

	state->depthAt = 1;

	bool running = true;
	while(byteOffset < state->opCodeStreamLength && running) {

		bool incrementOpCode = true;

		EasyVM_OpCode *opCode = (EasyVM_OpCode *)(state->opCodeStream + byteOffset);

		//NOTE(ollie): For debugging, to ensure were reading the right bytes. Remove from release build
		assert(opCode->magicNumber == EASY_VM_OP_CODE_MAGIC);

		switch(opCode->type) {
			case OP_CODE_EQUAL_TO: {
					EasyVM_Variable var1 = easyVM_popOffStack(state, VAR_NULL);
					EasyVM_Variable var2 = easyVM_popOffStack(state, VAR_NULL);

					EasyVM_Variable result = {0};
					result.type = VAR_INT;

					if(var1.type == VAR_INT) {
						if(var2.type == VAR_FLOAT) {
							result.intVal =  var2.floatVal == var1.intVal; 
						}
						if(var2.type == VAR_INT) {
							result.intVal = var2.intVal == var2.intVal; 
						}
					}

					if(var1.type == VAR_FLOAT) {
						if(var2.type == VAR_FLOAT) {
							result.intVal = var2.floatVal == var1.floatVal; 
						}
						if(var2.type == VAR_INT) {
							result.intVal = var2.floatVal == var1.intVal; 
						}
					}

					easyVM_pushOnStack_literal(state, result.type, result.bytes, false);
			} break;
			case OP_CODE_LESS_THAN: {
				EasyVM_Variable var1 = easyVM_popOffStack(state, VAR_NULL);
				EasyVM_Variable var2 = easyVM_popOffStack(state, VAR_NULL);

				EasyVM_Variable result = {0};
				result.type = VAR_INT;

				if(var1.type == VAR_INT) {
					if(var2.type == VAR_FLOAT) {
						result.intVal =  var2.floatVal < var1.intVal; 
					}
					if(var2.type == VAR_INT) {
						result.intVal = var2.intVal < var2.intVal; 
					}
				}

				if(var1.type == VAR_FLOAT) {
					if(var2.type == VAR_FLOAT) {
						result.intVal = var2.floatVal < var1.floatVal; 
					}
					if(var2.type == VAR_INT) {
						result.intVal = var2.floatVal < var1.intVal; 
					}
				}

				easyVM_pushOnStack_literal(state, result.type, result.bytes, false);
			} break;
			case OP_CODE_GREATER_THAN: {
				EasyVM_Variable var1 = easyVM_popOffStack(state, VAR_NULL);
				EasyVM_Variable var2 = easyVM_popOffStack(state, VAR_NULL);

				EasyVM_Variable result = {0};
				result.type = VAR_INT;

				if(var1.type == VAR_INT) {
					if(var2.type == VAR_FLOAT) {
						result.intVal =  var2.floatVal > var1.intVal; 
					}	
					if(var2.type == VAR_INT) {
						result.intVal = var2.intVal > var2.intVal; 
					}
				}

				if(var1.type == VAR_FLOAT) {
					if(var2.type == VAR_FLOAT) {
						result.intVal = var2.floatVal > var1.floatVal; 
					}
					if(var2.type == VAR_INT) {
						result.intVal = var2.floatVal > var1.intVal; 
					}
				}

				easyVM_pushOnStack_literal(state, result.type, result.bytes, false);
			} break;
			case OP_CODE_LESS_THAN_OR_EQUAL_TO: {
				EasyVM_Variable var1 = easyVM_popOffStack(state, VAR_NULL);
				EasyVM_Variable var2 = easyVM_popOffStack(state, VAR_NULL);

				EasyVM_Variable result = {0};
				result.type = VAR_INT;

				if(var1.type == VAR_INT) {
					if(var2.type == VAR_FLOAT) {
						result.intVal =  var2.floatVal <= var1.intVal; 
					}
					if(var2.type == VAR_INT) {
						result.intVal = var2.intVal <= var2.intVal; 
					}
				}

				if(var1.type == VAR_FLOAT) {
					if(var2.type == VAR_FLOAT) {
						result.intVal = var2.floatVal <= var1.floatVal; 
					}
					if(var2.type == VAR_INT) {
						result.intVal = var2.floatVal <= var1.intVal; 
					}
				}

				easyVM_pushOnStack_literal(state, result.type, result.bytes, false);
			} break;
			case OP_CODE_GREATER_THAN_OR_EQUAL_TO: {
				EasyVM_Variable var1 = easyVM_popOffStack(state, VAR_NULL);
				EasyVM_Variable var2 = easyVM_popOffStack(state, VAR_NULL);

				EasyVM_Variable result = {0};
				result.type = VAR_INT;

				if(var1.type == VAR_INT) {
					if(var2.type == VAR_FLOAT) {
						result.intVal =  var2.floatVal >= var1.intVal; 
					}	
					if(var2.type == VAR_INT) {
						result.intVal = var2.intVal >= var2.intVal; 
					}
				}

				if(var1.type == VAR_FLOAT) {
					if(var2.type == VAR_FLOAT) {
						result.intVal = var2.floatVal >= var1.floatVal; 
					}
					if(var2.type == VAR_INT) {
						result.intVal = var2.floatVal >= var1.intVal; 
					}
				}

				easyVM_pushOnStack_literal(state, result.type, result.bytes, false);
			} break;
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

				easyVM_pushOnStack_literal(state, result.type, result.bytes, false);
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

				easyVM_pushOnStack_literal(state, result.type, result.bytes, false);
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

				easyVM_pushOnStack_literal(state, result.type, result.bytes, false);
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

				easyVM_pushOnStack_literal(state, result.type, result.bytes, false);
			} break;
			case OP_CODE_OFFSET_BASE_POINTER: {

			} break;
			case OP_CODE_LOAD: {
				char *varName = easyVM_popOffStack(state, VAR_STRING).stringVal;

				easyVM_loadVariableOntoStack(state, varName);

				byteOffset = opCode->nextOpCode;
			} break;
			case OP_CODE_STORE: {
				EasyVM_Variable var = easyVM_popOffStack(state, VAR_NULL);

				char *varName = easyVM_popOffStack(state, VAR_STRING).stringVal;

				u8 *data = var.bytes;
				if(var.isPointer) {
					data = (u8 *)var.ptrVal;
				}

				easyVM_createVariable(state, varName, var.type, data, var.isPointer);

			} break;
			case OP_CODE_UPDATE: {
				EasyVM_Variable var = easyVM_popOffStack(state, VAR_NULL);
				char *varName = easyVM_popOffStack(state, VAR_STRING).stringVal;


				var.name = varName;
				easyVM_updateVariable(state, varName, &var);

			} break;
			case OP_CODE_LITERAL: {
				byteOffset += sizeof(EasyVM_OpCode);

				EasyVM_Variable *nextVar = (EasyVM_Variable *)(state->opCodeStream + byteOffset);

				void *ptrToData = nextVar->bytes;
				if(nextVar->isPointer) {
					ptrToData = nextVar->ptrVal;
				}
				easyVM_pushOnStack_literal(state, nextVar->type, (u8 *)ptrToData, nextVar->isPointer);
			} break;
			case OP_CODE_SIN: {
				EasyVM_Variable var = easyVM_popOffStack(state, VAR_NULL);

				assert(var.type == VAR_FLOAT || var.type == VAR_INT);

				if(var.type == VAR_INT) {
					var.type = VAR_FLOAT;
					var.floatVal = (float)var.intVal;
				}

				float newValue = (float)sin(var.floatVal);
				easyVM_pushOnStack_literal(state, VAR_FLOAT, (u8 *)&newValue, false);
			} break;
			case OP_CODE_COSINE: {
				EasyVM_Variable var = easyVM_popOffStack(state, VAR_NULL);

				assert(var.type == VAR_FLOAT || var.type == VAR_INT);

				if(var.type == VAR_INT) {
					var.type = VAR_FLOAT;
					var.floatVal = (float)var.intVal;
				}

				float newValue = (float)cos(var.floatVal);
				easyVM_pushOnStack_literal(state, VAR_FLOAT, (u8 *)&newValue, false);
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
				incrementOpCode = false;
			} break;
			case OP_CODE_RETURN: {

			} break;
			case OP_CODE_WHEN: {
				s32 offsetGoTo = easyVM_popOffStack(state, VAR_INT).intVal;
				EasyVM_Variable var = easyVM_popOffStack(state, VAR_NULL);

				bool value = false;
				if(var.type == VAR_INT) {
					value = (bool)var.intVal;
				} else if(var.type == VAR_BOOL) {
					value = (bool)var.boolVal;
				} else if(var.type == VAR_FLOAT) {
					value = (bool)(var.floatVal > 0.0f);
				} else {
					//NOTE(ollie): Wrong type
					assert(false);
				}

				if(!value) {
					byteOffset = offsetGoTo;
					incrementOpCode = false;
				}
			} break;
			case OP_CODE_EXIT: {
				state->offsetAt = 0;

				//NOTE(ollie): Empty all the variables except globals
				//NOTE(ollie): Since globals get created at compile time, we want to keep them around
				//NOTE(ollie): And not destory them
				easyVM_emptyLocalVariables_exceptGlobals(state);

				state->depthAt = 0;
				state->scopeOffsetCount = 0;
				state->basePointerAt = 0;

				running = false;
			} break;
			case OP_CODE_DRAW_CUBE: {
				V4 color = easyVM_buildV4FromStack(state);
				V3 centerPos = easyVM_buildV3FromStack(state);
				V3 rotation = easyVM_buildV3FromStack(state);
				V3 dim = easyVM_buildV3FromStack(state);
				
				Matrix4 matrix = Matrix4_scale(mat4(), dim);

				Matrix4 rotationMat = quaternionToMatrix(eulerAnglesToQuaternion(rotation.y, rotation.x, rotation.z));

				matrix = Mat4Mult(rotationMat, matrix);

				renderSetShader(globalRenderGroup, &phongProgram);
				////////////////////////////////////////////////////////////////////
				setModelTransform(globalRenderGroup, Matrix4_translate(matrix, centerPos));
				renderDrawCube(globalRenderGroup, &globalWhiteMaterial, color);
				////////////////////////////////////////////////////////////////////
				renderSetShader(globalRenderGroup, &phongProgram);
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
			case OP_CODE_RANDOM_BETWEEN: {
				float max = easyVM_popOffStack(state, VAR_FLOAT).floatVal;
				float min = easyVM_popOffStack(state, VAR_FLOAT).floatVal;

				float value = randomBetween(min, max);
				easyVM_pushOnStack_literal(state, VAR_FLOAT, (u8 *)&value, false);

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
		if(incrementOpCode) {
			byteOffset = opCode->nextOpCode;
		}
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

static u32 easyVM_writeOpCode(EasyVM_State *state, EasyVM_OpCodeType type) {
	easyVM_tryExpandOpcodeMemory(state, sizeof(EasyVM_OpCode));

	u32 opcodeAt = state->opCodeStreamLength;

	EasyVM_OpCode *at = (EasyVM_OpCode *)(state->opCodeStream + state->opCodeStreamLength);	

	at->magicNumber = EASY_VM_OP_CODE_MAGIC;

	at->type = type;

	state->opCodeStreamLength += sizeof(EasyVM_OpCode);
	assert(state->opCodeStreamLength <= state->opCodeStreamTotal);
	at->nextOpCode = state->opCodeStreamLength;

	return opcodeAt;
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

	} else if(nodeAt->prev->token.type == TOKEN_GREATER_THAN) {
		easyVM_writeOpCode(state, OP_CODE_GREATER_THAN);

	} else if(nodeAt->prev->token.type == TOKEN_LESS_THAN) {
		easyVM_writeOpCode(state, OP_CODE_LESS_THAN);

	} else if(nodeAt->prev->token.type == TOKEN_GREATER_THAN_OR_EQUAL_TO) {
		easyVM_writeOpCode(state, OP_CODE_GREATER_THAN_OR_EQUAL_TO);

	} else if(nodeAt->prev->token.type == TOKEN_LESS_THAN_OR_EQUAL_TO) {
		easyVM_writeOpCode(state, OP_CODE_LESS_THAN_OR_EQUAL_TO);
	} else if(nodeAt->prev->token.type == TOKEN_DOUBLE_EQUAL) {
			easyVM_writeOpCode(state, OP_CODE_EQUAL_TO);
	} else {
		assert(!"something went wrong!");
	}
}

static void easyVM_maybeWriteMathOpcode(EasyVM_State *state, EasyAst_Node *nodeAt) {
	if(nodeAt->prev && nodeAt->prev->type == EASY_AST_NODE_OPERATOR_MATH) {
		easyVM_writeMathOpCode(state, nodeAt);
	}
}

static s32 easyVM_writeLiteral(EasyVM_State *state, EasyVM_Variable var) {
	//NOTE(ollie): This Pointer isn't safe, since we reallocate the array, so we just return the offset where it is in the stream
	u32 opcodeOffset = easyVM_writeOpCode(state, OP_CODE_LITERAL);

	u32 sizeOfAddition = sizeof(EasyVM_Variable);
	easyVM_tryExpandOpcodeMemory(state, sizeOfAddition);

	EasyVM_Variable *at = (EasyVM_Variable *)(state->opCodeStream + state->opCodeStreamLength);	

	s32 literalAt = state->opCodeStreamLength;

	*at = var;

	state->opCodeStreamLength += sizeOfAddition;
	assert(state->opCodeStreamLength <= state->opCodeStreamTotal);

	//NOTE(ollie): Advance where the next opcode is
	EasyVM_OpCode *opCode = (EasyVM_OpCode *)(state->opCodeStream + opcodeOffset);
	opCode->nextOpCode = state->opCodeStreamLength;

	return literalAt;
	
}



static void easyVM_pushGoToLiteral(EasyVM_State *state, EasyVM_GotoLiteral lit) {
	assert(state->gotoLiteralsCount < arrayCount(state->gotoLiterals));
	state->gotoLiterals[state->gotoLiteralsCount++] = lit;
}

static EasyVM_GotoLiteral easyVM_popGoToLiteral(EasyVM_State *state) {
	assert(state->gotoLiteralsCount > 0);
	EasyVM_GotoLiteral result = state->gotoLiterals[--state->gotoLiteralsCount];

	return result;
}



////////////////////////////////////////////////////////////////////



