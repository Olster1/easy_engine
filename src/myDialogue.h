typedef enum {
	MY_DIALOGUE_ENDED,
	MY_DIALOGUE_RUNNING,
	MY_DIALOGUE_WAITIMG_FOR_PLAYER_INPUT,
	MY_DIALOGUE_WAITING_FOR_NPC_DIALOGUE
} MyDialogue_ProgramMode;

typedef enum {
	MY_DIALOGUE_VARIABLE_DEFAULT = 1 << 0,
	MY_DIALOGUE_VARIABLE_VISIBLE = 1 << 1,
} MyDialogue_VariableFlags;

typedef struct {
	u32 offsetAt;
	bool isBlinder;
} MyDialogue_StackSnapshot;

typedef struct {
	char *name;


	u32 offsetInStack; //to get the data out
	u32 sizeInBytes;

	//NOTE(ollie): For array sizes
	int arraySize;
	int maxArraySize;
	bool isExpanding; 

	//NOTE(ollie): inheitance union for different variable type
	union {
		struct { //STRING TYPE
			u32 stringLength;
			bool stringInUse;
		};
	};

	MyDialogue_VariableFlags flags;

	MyDialogue_StackSnapshot *blinderSnapShot;

	VarType type;
} MyDialogue_ProgramVariable;

typedef struct {
	char *name;
	VarType type;
} MyDialogue_FunctionParameter;

typedef struct {
	//NOTE(ollie): Name of funciton
	char *name;

	u32 paramCount;
	MyDialogue_FunctionParameter params[64]; //NOTE(ollie): Max 64 function params

	//NOTE(ollie): VAR_NULL if it doesn't return anything
	VarType returnType;

	//NOTE(ollie): Can return arrays
	u32 returnCount; 
} MyDialogue_Function;



typedef struct {
	//NOTE(ollie): actual stack
	u32 stackOffset;
	u8 stack[4096];// stack is 2048 bytes big

	//NOTE(ollie): Snapshots of the stack for when we enter & exit a new scope
	u32 snapshotCount;
	MyDialogue_StackSnapshot snapshots[64]; //64 scopes can be pushed onto the stack 

	//NOTE(ollie): Variable table to map name to data in the stack
	u32 variableCount;
	MyDialogue_ProgramVariable variables[64];

	//NOTE(ollie): Function table to map name to data in the stack
	u32 functionCount;
	MyDialogue_Function functions[64];

	int stackScopeCount;

} MyDialogue_ProgramStack;


///////////////////////************ Function signatures*************////////////////////

EasyAst_Node *myDialogue_evaluateFunction(MyDialogue_Program *program, EasyAst_Node *nodeAt, MyDialogue_ProgramVariable *varToFillOut, u32 indexIntoArray);
EasyAst_Node *myDialogue_evaluateExpression(MyDialogue_Program *program, EasyAst_Node *nodeAt, MyDialogue_EvaluateFlags flags, MyDialogue_ProgramVariable *varToFillOut);

///////////////////////************ Helper functions *************////////////////////

#define MY_DIALOGUE_MINIMUM_VARIABLE_SIZE_ON_STACK 4 //bytes //NOTE(ollie): This is so we can change the type of variables to booleans & know they still fit when evaluating expressions
static inline u32 myDialogue_getSizeOfType_withMinimum(VarType type) {
	u32 result = 0;
	switch(type) {
		case VAR_STRING: {
			result = sizeof(char *);
		} break;
		case VAR_FLOAT: {
			result = sizeof(float);
		} break;
		case VAR_INT: {
			result = sizeof(int);
		} break;	
		default: {
			assert(!"type not handled");
		}		
	}

	//NOTE(ollie): Make sure it's a above the minimum
	if(result < MY_DIALOGUE_MINIMUM_VARIABLE_SIZE_ON_STACK) {
		result = MY_DIALOGUE_MINIMUM_VARIABLE_SIZE_ON_STACK;
	}
	return result;
}

static inline VarType myDialogue_getVariableTypeFromToken(EasyTokenType tokenType) {
	VarType result = VAR_NULL;
	switch(tokenType) {
		case TOKEN_STRING: {
			result = VAR_STRING;
		} break;
		case TOKEN_FLOAT: {
			result = VAR_FLOAT;
		} break;
		case TOKEN_INTEGER: {
			result = VAR_INTEGER;
		} break;	
		default: {
			assert(!"type not handled");
		}		
	}
	return result;
}

static inline MyDialogue_Function *myDialogue_findFunction(MyDialogue_ProgramStack *program, char *name, u32 strLength) {
	MyDialogue_Function *result = 0;
	for(int i = 0; i < stack->functionCount && !result; ++i) {
		MyDialogue_Function *func = stack->functions + i;

		//NOTE(ollie): Check name
		if(stringsMatchNullN(func->name, name, strLength)) {
			result = func;
			break;
		}
	}
	return result;
}

static inline int myDialogue_castVariableAsInt(MyDialogue_ProgramStack *stack, MyDialogue_ProgramVariable *var, u32 indexIntoArray) {
	assert(var->type == VAR_INT);
	assert(var->sizeInBytes == sizeof(int));

	u8 *ptr = stack->stack + var->offsetInStack + (indexIntoArray*myDialogue_getSizeOfType_withMinimum(var->type)); 
	assert(var->offsetInStack <= stack->stackOffset);


	int result = *((int *)(ptr));

	return result;
}

static inline char *myDialogue_castVariableAsString(MyDialogue_ProgramStack *stack, MyDialogue_ProgramVariable *var, u32 indexIntoArray) {
	assert(var->type == VAR_STRING);

	u8 *ptr = stack->stack + var->offsetInStack + (indexIntoArray*myDialogue_getSizeOfType_withMinimum(var->type)); 
	assert(var->offsetInStack <= stack->stackOffset);

	char *result = (char *)(ptr);

	return result;
}


static inline float myDialogue_castVariableAsFloat(MyDialogue_ProgramStack *stack, MyDialogue_ProgramVariable *var, u32 indexIntoArray) {
	assert(var->type == VAR_FLOAT);
	assert(var->sizeInBytes == sizeof(float));

	u8 *ptr = stack->stack + var->offsetInStack + (indexIntoArray*myDialogue_getSizeOfType_withMinimum(var->type)); 
	assert(var->offsetInStack <= stack->stackOffset);

	float result = *((float *)(ptr));

	return result;
}

static MyDialogue_ProgramVariable *myDialogue_pushVariableToStack_withoutData(MyDialogue_ProgramStack *stack, char *name, u32 maxArraySize, bool isExpanding, MyDialogue_VariableFlags flags) {
	assert(stack->variableCount < arrayCount(stack->variables));

	MyDialogue_ProgramVariable *var = stack->variables + stack->variableCount++;

	memset(var, 0, sizeof(MyDialogue_ProgramVariable)); 

	//NOTE(ollie): We initalise the variable to just be a null type
	var->type = VAR_NULL; 
	var->name = name;
	var->offsetInStack = stack->stackOffset;
	var->flags = flags;

	var->blinderSnapShot = 0;

	var->arraySize = 0;
	assert(maxArraySize >= 1);
	var->maxArraySize = maxArraySize;
	var->isExpanding = isExpanding; 
	var->stringInUse = false;
	
	return var;
}

static void myDialogue_addDataToVariable(MyDialogue_ProgramStack *stack, MyDialogue_ProgramVariable *var, void *data, u32 sizeInBytes, VarType type, int count) {
	//NOTE(ollie): Nothing has been added to the stack since this one. I.e. is contigous
	assert(stack->stackOffset == (var->offsetInStack + var->sizeInBytes));

	if(var->type != VAR_NULL) {
		assert(var->type == type);
	} 

	var->type = type;
	var->arraySize += count;

	if(var->isExpanding && var->arraySize > var->maxArraySize) {
		//NOTE(ollie): Expand the array if it can
		var->maxArraySize = var->arraySize;
	} else if(var->arraySize > var->maxArraySize) {
		assert(!"shold check this outside of this function");
	}

	//NOTE(ollie): Outside code should catch this and issue an error to user
	assert(var->arraySize <= var->maxArraySize);


	if((stack->stackOffset + sizeInBytes) <= arrayCount(stack->stack)) {
		if(data) {
			//NOTE(ollie): Get the stack pointer
			
			u8 *ptrAt = stack->stack + var->offsetInStack + var->sizeInBytes;
			//NOTE(ollie): Add the data to the stack
			memcpy(ptrAt, data, sizeInBytes);
		}
		
		//NOTE(ollie): Advance the stack offset
		stack->stackOffset += sizeInBytes;
	} else {
		assert(!"stack overflow");
	}

	//NOTE(ollie): Advance the size of the variable afterwards so we can use sizeInBytes to find where the next offset in stack is
	var->sizeInBytes += sizeInBytes;
	
	return var;
} 

static MyDialogue_ProgramVariable *myDialogue_findVariable(MyDialogue_ProgramStack *stack, char *name, u32 strLength) {
	MyDialogue_ProgramVariable *result = 0;
	for(int i = 0; i < stack->variableCount && !result; ++i) {
		MyDialogue_ProgramVariable *var = stack->variables + i;

		//NOTE(ollie): Make sure this variable is visible
		bool isVisible = (var->flags & MY_DIALOGUE_VARIABLE_VISIBLE) && !var->blinderSnapShot;

		if(var->blinderSnapShot) {
			assert(var->blinderSnapShot->isBlinder);
		}

		//NOTE(ollie): Check name
		if(stringsMatchNullN(var->name, name, strLength) && isVisible) {
			result = var;
			assert(var->type != VAR_NULL);
			break;
		}
	}
	return result;
}

//NOTE(ollie): This is to disable variables when we enter functions, so everything isn't essentially a global variable
static void myDialogue_pushStackBlinder(MyDialogue_ProgramStack *stack) {
	//NOTE(ollie): Get the next snapshot out of the arary
	assert(snapshotCount < arrayCount(stack->snapshots));
	MyDialogue_StackSnapshot *snapshot = stack->snapshots + stack->snapshotCount++;

	snapshot->isBlinder = true;

	//NOTE(ollie): Take the snapshot
	snapshot->offsetAt = stack->stackOffset;

	//NOTE(ollie): Hide all variables if not already blinded
	for(int i = 0; i < stack->variableCount; i++) {

		//NOTE(ollie): Get the variable
		MyDialogue_ProgramVariable *var = stack->variables + stack->variableCount;

		if(!var->blinderSnapShot) {
			var->blinderSnapShot = snapshot;	
		}
	}
}

static void myDialogue_popStackBlinder(MyDialogue_ProgramStack *stack) {

	//NOTE(ollie): Get the snapshop we're removing & decrement the snapshot count
	MyDialogue_StackSnapshot *snapshot = stack->snapshots[--stack->snapshotCount];

	//NOTE(ollie): Make sure it is a blinder. If it is, we forgot to release the last frame snapshot 
	assert(snapshot->blinder);

	//NOTE(ollie): If variable was blinded before by the snapshot, remove the blinder
	for(int i = 0; i < stack->variableCount; i++) {

		//NOTE(ollie): Get the variable
		MyDialogue_ProgramVariable *var = stack->variables + stack->variableCount;

		//NOTE(ollie): Identify them by the offset they are in the stack
		if(var->offsetInStack >= snapshot->offsetAt) {
			//NOTE(ollie): Remove blinder flag so can be found 
			assert(var->blinderSnapShot);
			var->blinderSnapShot = 0;
		}

	}
}

static void myDialogue_pushStackFrame(MyDialogue_ProgramStack *stack) {
	//NOTE(ollie): Get the next snapshot out of the arary
	assert(snapshotCount < arrayCount(stack->snapshots));
	MyDialogue_StackSnapshot *snapshot = stack->snapshots + stack->snapshotCount++;

	snapshot->isBlinder = false;

	//NOTE(ollie): Take the snapshot
	snapshot->offsetAt = stack->stackOffset;
}

typedef enum {
} MyDialogue_StackFrameFlag;

#define myDialogue_popStackFrame(stack) myDialogue_popStackFrame_withFlags(stack, 0)
static void myDialogue_popStackFrame_withFlags(MyDialogue_ProgramStack *stack, MyDialogue_StackFrameFlag flags) {

	//NOTE(ollie): Get the snapshop we're removing & decrement the snapshot count
	MyDialogue_StackSnapshot *snapshot = stack->snapshots[--stack->snapshotCount];

	//NOTE(ollie): Make sure it isn't a blinder. If it is, we forgot to release the last blinder 
	assert(!snapshot->blinder);

	//NOTE(ollie): Remove all the variables that existed in that scope
	for(int i = 0; i < stack->variableCount; ) {

		//NOTE(ollie): Get the variable
		MyDialogue_ProgramVariable *var = stack->variables + stack->variableCount;

		//NOTE(ollie): Identify them by the offset they are in the stack
		if(var->offsetInStack >= snapshot->offsetAt) {

			//NOTE(ollie): Only free string if it isn't in use 
			if(var->type == VAR_STRING && !var->stringInUse) {
				//NOTE(ollie): Free all the strings that are in use when we pop a string variable
				for(int strIndex = 0; strIndex < var0->sizeOfArray; ++strIndex) {
					char *str = myDialogue_castVariableAsString(stack, var, strIndex);
					assert(str);

					easyPlatform_freeMemory(str);
				}
			}
			//NOTE(ollie): Get the last variable data of the end & block copy it in the spot of this variable
			*var = stack->variables[--stack->variableCount];
		} else {
			//NOTE(ollie): Only increment if we didn't remove a variable
			++i;
		}
	}
		
	//NOTE(ollie): Reverse the snapshot
	snapshot->offsetAt = stack->stackOffset;
	
}


typedef struct {
	EasyAst ast;

	EasyAst_Node *nodeAt;

	MyDialogue_ProgramStack stack;

	MyDialogue_ProgramMode mode;

	bool valid;

} MyDialogue_Program;

static MyDialogue_Program myDialogue_compileProgram(char *filename) {
	MyDialogue_Program result = {};

	bool isFileValid = platformDoesFileExist(filename);
	assert(isFileValid);

	if(isFileValid) {

		FileContents contents = getFileContentsNullTerminate(filename);
		result.ast = easyAst_generateAst((char *)contents.memory);

		easyAst_printAst(result.ast);

		///////////////////////************* Init the dialogue program ************////////////////////
		
		result.valid = true; //TODO(ollie): Check for errors
		result.mode = MY_DIALOGUE_RUNNING;
		result.nodeAt = &ast.parentNode;

		///////////////////////************ Init the program stack *************////////////////////
		result.stack.stackScopeCount = 0;
		result.stack.stackOffset = 0;
		result.stack.snapshotCount = 0;
		result.stack.variableCount = 0;
		result.stack.functionCount = 0;

		////////////////////////////////////////////////////////////////////1
	}
	return result;
}

static bool myDialogue_checkNodeHasNext(MyDialogue_Program *program, EasyAst_Node *node) {
	if(node->next) {
		return true;
	} else {
		easyAst_addError(program->ast, "Did you finish the line?", nodeAt->token.lineNumber);		
		return false;
	}
}

static bool myDialogue_advanceNode(MyDialogue_Program *program, EasyAst_Node **node) {
	if((*node)->child) {
		easyAst_addError(program->ast, "Did you put in brackets in a place that doesn't make sense?", (*node)->token.lineNumber);		
	}

	if((*node)->next) {
		*node = node->next;
	} else {
		easyAst_addError(program->ast, "Did you finish the line?", (*node)->token.lineNumber);		
	}
}

static myDialogue_checkArraySize(MyDialogue_Program *program, EasyAst_Node *node, MyDialogue_ProgramVariable *var, int sizeOfArray) {
	if(sizeOfArray > 0) {
		if(sizeOfArray < var->arraySize) {
			easyAst_addError(program->ast, "There are more elements in this array then you specified.", node->token.lineNumber);	
		}
	}
}

static void myDialogue_deepCopyString(MyDialogue_ProgramStack *stack, MyDialogue_ProgramVariable *var0, MyDialogue_ProgramVariable *var1, u32 indexIntoArray0, u32 indexIntoArray1) {
	//NOTE(ollie): Since string is a pointer, we want to copy the actual string

	//NOTE(ollie): Get the string we want to copy
	char *stringFromVar1 = myDialogue_castVariableAsString(stack, var1, indexIntoArray1);

	//NOTE(ollie): We allocate a temporary string, then free it when we pop the variables

	//NOTE(ollie): Find size in bytes
	assert(var1->stringLength > 0);
	u32 stringSizeInBytes = sizeof(char)*(var1->stringLength);

	//NOTE(ollie): Allocate the bytes
	char *stringOnHeap = (char *)easyPlatform_allocateMemory(stringSizeInBytes, 0);

	//NOTE(ollie): copy the string data to the new string
	easyPlatform_copyMemory(stringOnHeap, stringFromVar1, stringSizeInBytes);

	assert(stringsMatchN(stringOnHeap, var1->stringLength, stringFromVar1, var1->stringLength));

	//NOTE(ollie): Get the location of the string
	char **ptr = (char **)(stack->stack + var0->offsetInStack + indexIntoArray0*myDialogue_getSizeOfType_withMinimum(var0)); 

	//NOTE(ollie): Assign the pointer to the string
	*ptr = stringOnHeap;

	var0->stringLength = var1->stringLength;

}

static u32 myDialogue_inheritVariable(MyDialogue_ProgramVariable *varToFillOut, MyDialogue_ProgramVariable *tempVar) {
	//NOTE(ollie): Make sure it's contigous in stack
	assert(tempVar->offsetInStack == varToFillOut->offsetInStack);

	assert(varToFillOut->type == VAR_NULL);
	assert(varToFillOut->sizeInBytes == 0);
	assert(varToFillOut->arraySize == 0);
	assert(tempVar->arraySize == 1);
	varToFillOut->type = tempVar->type;
	varToFillOut->sizeInBytes = myDialogue_getSizeOfType_withMinimum(tempVar->type);
	varToFillOut->offsetInStack = tempVar->offsetInStack;
	varToFillOut->arraySize = tempVar->arraySize;
	varToFillOut->stringLength = tempVar->stringLength;

	if(varToFillOut->type == VAR_STRING) {
		// myDialogue_deepCopyString(stack, var0, var1, indexIntoArray0, indexIntoArray1);
		//NOTE(ollie): don't delete the string
		var1->stringInUse = true;
	}

	return (varToFillOut->offsetInStack + varToFillOut->sizeInBytes);
}


//NOTE(ollie): Var0 is the first variable & the outgoing variable
static void myDialogue_copyVariableData(MyDialogue_ProgramStack *stack, MyDialogue_ProgramVariable *var0, MyDialogue_ProgramVariable *var1, u32 indexIntoArray0, u32 indexIntoArray1) {
	assert(var0->sizeInBytes >= var1->sizeInBytes)

	assert(var1.arraySize == 1);
	
	assert(var0->type != VAR_NULL && var1->type != VAR_NULL);
	assert(var0->type == var1->type)

	u32 sizeOfType = myDialogue_getSizeOfType_withMinimum(var0->type);

	if(var0->type == VAR_STRING) {
		myDialogue_deepCopyString(stack, var0, var1, indexIntoArray0, indexIntoArray1);
	} else {
		//NOTE(ollie): Get the stack pointer
		u8 *ptrAt_0 = stack->stack + var0->offsetInStack + indexIntoArray*sizeOfType;

		u8 *ptrAt_1 = stack->stack + var1->offsetInStack + indexIntoArray*sizeOfType;
		

		//NOTE(ollie): Copy variable data from var0 to var1
		easyPlatform_copyMemory(ptrAt_0, ptrAt_1, var1->sizeInBytes);
	}

	
}

//TODO(ollie): This is a bit hacky
static inline void myDialogue_updateVariableToNewType(MyDialogue_ProgramVariable *p, VarType type) {
	u32 oldSize = p->maxArraySize*myDialogue_getSizeOfType_withMinimum(p->type);
	u32 newSize = p->arraySize*myDialogue_getSizeOfType_withMinimum(type);

	assert(oldSize >= newSize);

	p->type = type;

}

static EasyAst_Node *myDialogue_evaluateVariable(MyDialogue_Program *program, EasyAst_Node *nodeAt, MyDialogue_EvaluateFlags flags, MyDialogue_ProgramVariable *varToFillOut, EasyTokenType operator, MyDialogue_Function *func) {
	///////////////////////************* Allocate space on the stack if we know what type it is ************////////////////////
	bool dataFilledOut = false;
	if(varToFillOut->type != VAR_NULL) {
		//NOTE(ollie): We know the type, so we can create a space on the stack for it, ready to be copied in
		dataFilledOut = true;
		//NOTE(ollie): Allocate space for the data to be stored in there. 
		if(varToFillOut->arraySize < varToFillOut->maxArraySize || varToFillOut->isExpanding) {
			myDialogue_addDataToVariable(&program->stack, varToFillOut, 0, myDialogue_getSizeOfType_withMinimum(varToFillOut->type), varToFillOut->type, 1);
		} else {
			easyAst_addError(program->ast, "Variable overflow. There is too much data for this variable.", nodeAt->token.lineNumber);
		}
	}

	//NOTE(ollie): We only use this if we are inheriting a variable, & we actually want to keep the stack data alive
	u32 stackOffset = 0;

	///////////////////////************* Enter the node & bring back the result ************////////////////////
	myDialogue_pushStackFrame(&program->stack);

	MyDialogue_ProgramVariable *tempVar = myDialogue_pushVariableToStack_withoutData(&program->stack, varNode.token.at, varNode.token.size, 1, false, MY_DIALOGUE_VARIABLE_DEFAULT);

	//NOTE(ollie): Leave it null so it can be assigned a type
	assert(tempVar->type == VAR_NULL);

	if(func) {
		nodeAt = myDialogue_evaluateFunction(program, nodeAt, func, tempVar);
	} else {
		//NOTE(ollie): Actually run the function now that we have the variable set up
		nodeAt = myDialogue_evaluateExpression(program, nodeAt, flags, tempVar);	
	}
	

	///////////////////////************ Copy the data to the new variable or inherit it if we knew we had the space already *************////////////////////
	if(tempVar->type == VAR_NULL) {
		easyAst_addError(program->ast, "There wasn't anything in the parenthesis", nodeAt->token.lineNumber);
	} else if(tempVar->type == varToFillOut->type || varToFillOut->type == VAR_NULL){
		if(!dataFilledOut) {
			//NOTE(ollie): newly copying
			assert(varToFillOut->arraySize == 0);
			assert(nextOperator == TOKEN_UNINITIALISED);
			//NOTE(ollie): Block copy the variable
			stackOffset = myDialogue_inheritVariable(varToFillOut, tempVar);
		} else {
			if(nextOperator == TOKEN_UNINITIALISED) {
				//NOTE(ollie): Add the data to the variable
				myDialogue_copyVariableData(&program->stack, varToFillOut, tempVar, indexAt, 0);	
			} else {
				varToFillOut = myDialogue_combineVariablesUsingOperator(&program->stack, varToFillOut, tempVar, nextOperator, 0, 0);
			}
			
		}
		
	} else {
		easyAst_addError(program->ast, "The expression does not evaluate to the same type as the variable.", nodeAt->token.lineNumber);
	}
	////////////////////////////////////////////////////////////////////

	//NOTE(ollie): Pop the temp variable off the stack
	myDialogue_popStackFrame(&program->stack);

	//NOTE(ollie): restore stack data if we need to
	if(!dataFilledOut) {
		assert(program->stack.offsetAt <= stackOffset);
		program->stack.offsetAt = stackOffset;
	}

	return nodeAt;
}

static MyDialogue_ProgramVariable *myDialogue_combineVariablesUsingOperator(MyDialogue_ProgramStack *stack, MyDialogue_ProgramVariable *var0, MyDialogue_ProgramVariable *var1, EasyTokenType operatorType, u32 indexIntoArray0, u32 indexIntoArray1) {
	assert(var0->type == var1->type);
	
	//NOTE(ollie): The size might not be the same since it can be an array element
	// assert(var0.sizeInBytes == var1.sizeInBytes);
	assert(var0->sizeInBytes != 0);

	switch(operatorType) {
		case TOKEN_PLUS: {
			switch(var0->type) {
				case VAR_INT: {
					//NOTE(ollie): LOAD 
					int a = myDialogue_castVariableAsInt(stack, var0, indexIntoArray0);
					int b = myDialogue_castVariableAsInt(stack, var1, indexIntoArray1);
					int result = a + b;

					int *ptr = (int *)(stack->stack + var0->offsetInStack + indexIntoArray0*myDialogue_getSizeOfType_withMinimum(var0)); 

					*ptr = result;

				} break;
				case VAR_FLOAT: {
					float a = myDialogue_castVariableAsFloat(stack, var0, indexIntoArray0);
					float b = myDialogue_castVariableAsFloat(stack, var1, indexIntoArray1);
					float result = a + b;

					float *ptr = (float *)(stack->stack + var0->offsetInStack + indexIntoArray0*myDialogue_getSizeOfType_withMinimum(var0)); 

					*ptr = result;
				} break;
				case VAR_STRING: {
					char *a = myDialogue_castVariableAsString(stack, var0, indexIntoArray0);
					char *b = myDialogue_castVariableAsString(stack, var1, indexIntoArray1);

					char *result = concat_withLength(a, a->stringLength, b, b->stringLength);

					u32 newStringSize = strlen(result);

					//NOTE(ollie): free the memory from string a, because we're about to set the string to the new memory
					easyPlatform_freeMemory(a);

					char **ptr = (char **)(stack->stack + var0->offsetInStack + indexIntoArray0*myDialogue_getSizeOfType_withMinimum(var0)); 

					*ptr = result;

					//NOTE(ollie): assign the new string size
					var0->stringLength = newStringSize;

				} break;
				default: {
					assert(!"Type not handled");
				}
			}
		} break;
		case TOKEN_MINUS: {
			switch(var0.type) {
				case VAR_INT: {
					//NOTE(ollie): LOAD 
					int a = myDialogue_castVariableAsInt(stack, var0, indexIntoArray0);
					int b = myDialogue_castVariableAsInt(stack, var1, indexIntoArray1);
					int result = a - b;

					int *ptr = (int *)(stack->stack + var0->offsetInStack + indexIntoArray0*myDialogue_getSizeOfType_withMinimum(var0)); 

					*ptr = result;

				} break;
				case VAR_FLOAT: {
					float a = myDialogue_castVariableAsFloat(stack, var0, indexIntoArray0);
					float b = myDialogue_castVariableAsFloat(stack, var1, indexIntoArray1);
					float result = a - b;

					float *ptr = (float *)(stack->stack + var0->offsetInStack) + indexIntoArray0*myDialogue_getSizeOfType_withMinimum(var0); 

					*ptr = result;
				} break;
				case VAR_STRING: {
					assert(!"case not handled")
				} break;
				default: {
					assert(!"Type not handled");
				}
			}
		} break;
		case TOKEN_ASTRIX: {
			switch(var0.type) {
				case VAR_INT: {
					//NOTE(ollie): LOAD 
					int a = myDialogue_castVariableAsInt(stack, var0, indexIntoArray0);
					int b = myDialogue_castVariableAsInt(stack, var1, indexIntoArray1);
					int result = a * b;

					int *ptr = (int *)(stack->stack + var0->offsetInStack + indexIntoArray0*myDialogue_getSizeOfType_withMinimum(var0)); 

					*ptr = result;

				} break;
				case VAR_FLOAT: {
					float a = myDialogue_castVariableAsFloat(stack, var0, indexIntoArray0);
					float b = myDialogue_castVariableAsFloat(stack, var1, indexIntoArray1);
					float result = a * b;

					float *ptr = (float *)(stack->stack + var0->offsetInStack + indexIntoArray0*myDialogue_getSizeOfType_withMinimum(var0)); 

					*ptr = result;
				} break;
				case VAR_STRING: {
					assert(!"case not handled")
				} break;
				default: {
					assert(!"Type not handled");
				}
			}
		} break;
		case TOKEN_FORWARD_SLASH: {
			switch(var0.type) {
				case VAR_INT: {
					//NOTE(ollie): LOAD 
					int a = myDialogue_castVariableAsInt(stack, var0, indexIntoArray0);
					int b = myDialogue_castVariableAsInt(stack, var1, indexIntoArray1);
					int result = a / b;

					int *ptr = (int *)(stack->stack + var0->offsetInStack + indexIntoArray0*myDialogue_getSizeOfType_withMinimum(var0)); 

					*ptr = result;

				} break;
				case VAR_FLOAT: {
					float a = myDialogue_castVariableAsFloat(stack, var0, indexIntoArray0);
					float b = myDialogue_castVariableAsFloat(stack, var1, , indexIntoArray1);
					float result = a / b;

					float *ptr = (float *)(stack->stack + var0->offsetInStack + indexIntoArray0*myDialogue_getSizeOfType_withMinimum(var0)); 

					*ptr = result;
				} break;
				case VAR_STRING: {
					assert(!"case not handled")
				} break;
				default: {
					assert(!"Type not handled");
				}
			}
		} break;
		case TOKEN_DOUBLE_EQUAL: {
			switch(var0.type) {
				case VAR_INT: {
					//NOTE(ollie): LOAD 
					int a = myDialogue_castVariableAsInt(stack, var0, indexIntoArray0);
					int b = myDialogue_castVariableAsInt(stack, var1, indexIntoArray1);
					bool result = (a == b);

					int *ptr = (int *)(stack->stack + var0->offsetInStack + indexIntoArray0*myDialogue_getSizeOfType_withMinimum(var0)); 

					*ptr = (int)result;

					myDialogue_updateVariableToNewType(var0, VAR_BOOL, indexIntoArray0);

				} break;
				case VAR_FLOAT: {
					float a = myDialogue_castVariableAsFloat(stack, var0, indexIntoArray0);
					float b = myDialogue_castVariableAsFloat(stack, var1, indexIntoArray1);
					bool result = (a == b);

					int *ptr = (int *)(stack->stack + var0->offsetInStack + indexIntoArray0*myDialogue_getSizeOfType_withMinimum(var0)); 

					*ptr = (int)result;

					myDialogue_updateVariableToNewType(var0, VAR_BOOL, indexIntoArray0);
				} break;
				case VAR_STRING: {
					char *a = myDialogue_castVariableAsString(stack, var0, indexIntoArray0);
					char *b = myDialogue_castVariableAsString(stack, var1, indexIntoArray1);

					bool result = cmpStrNull(a, b);

					//NOTE(ollie): free the memory from string a, because we're about to set the string to the new memory
					easyPlatform_freeMemory(a);

					//NOTE(ollie): cast as an integer value
					int *ptr = (int *)(stack->stack + var0->offsetInStack + indexIntoArray0*myDialogue_getSizeOfType_withMinimum(var0)); 

					*ptr = (int)result;

					myDialogue_updateVariableToNewType(var0, VAR_BOOL, indexIntoArray0);
				} break;
				default: {
					assert(!"Type not handled");
				}
			}
		} break;
		default: {
			assert(!"operator node handled");
		}
	} 

	return var0;
}

typedef enum {
	MY_DIALOGUE_EVALUATE_DEFAULT = 1 << 0, //don't evaluate comma. Will cause error if it finds one
	MY_DIALOGUE_EVALUATE_COMMA = 1 << 1,
} MyDialogue_EvaluateFlags;

static EasyAst_Node *myDialogue_evaluateExpression(MyDialogue_Program *program, EasyAst_Node *nodeAt, MyDialogue_EvaluateFlags flags, MyDialogue_ProgramVariable *varToFillOut) {
	bool parsing = true;

	assert(var);
	//NOTE(ollie): Keeping track of number of operators vs data
	int operatorCount = 0;

	EasyTokenType nextOperator = TOKEN_UNINITIALISED;

	bool hadAParenthesis = false;
	while(parsing && program->ast.errorCount == 0) {
		bool addData = false;
		
		switch(nodeAt->type) {
			case EASY_AST_NODE_PARENT: {
				//NOTE(ollie): There is a sub-expression so go evaluate that, & bring back the result;
				assert(nodeAt->child);

				//NOTE(ollie): Take temporary node
				EasyAst_Node *tempNode = nodeAt;

				//NOTE(ollie): Move to the child node
				nodeAt = nodeAt->child;

				myDialogue_evaluateVariable(program, nodeAt, flags, varToFillOut, nextOperator, 0);

				//NOTE(ollie): restore the node
				nodeAt = tempNode;

				//NOTE(ollie): advance the node
				myDialogue_advanceNode(program, &nodeAt);
			} break;
			case EASY_AST_NODE_OPERATOR_COMMA: {
				if(flags & MY_DIALOGUE_EVALUATE_COMMA) {
					parsing = false;
				} else {
					easyAst_addError(program->ast, "There shouldn't be a comma in this expression.", nodeAt->token.lineNumber);
				}
				
				myDialogue_advanceNode(program, &nodeAt);
			} break;
			case EASY_AST_NODE_OPERATOR_FUNCTION: {
				MyDialogue_Function *function = myDialogue_findFunction(program, nodeAt->token.at, nodeAt->token.size);

				if(function) {

					if(function->returnType != VAR_NULL) {
						nodeAt = myDialogue_evaluateVariable(program, nodeAt, flags, varToFillOut, nextOperator, function);
					} else {
						easyAst_addError(program->ast, "This function doesn't return anything.", nodeAt->token.lineNumber);
					}		
				} else {
					easyAst_addError(program->ast, "This function hasn't been declared. Have you spelled it correctly?", nodeAt->token.lineNumber);
				}	
			} break;
			case EASY_AST_NODE_OPERATOR_MATH: {
				if(operatorCount == 0) {
					switch(node->token.type) {
						case TOKEN_PLUS: {
							nextOperator = TOKEN_PLUS;
						} break;
						case TOKEN_MINUS: {
							nextOperator = TOKEN_MINUS;
						} break;
						case TOKEN_FORWARD_SLASH: {
							nextOperator = TOKEN_FORWARD_SLASH;
						} break;
						case TOKEN_ASTRIX: {
							nextOperator = TOKEN_ASTRIX;
						} break;
						default: {
							easyAst_addError(program->ast, "Operator not supported.", nodeAt->token.lineNumber);
						}
					}
					operatorCount++;	
					myDialogue_advanceNode(program, &nodeAt);
				} else {
					easyAst_addError(program->ast, "You've put to many operators in a row", nodeAt->token.lineNumber);
				}
				
			} break;
			case EASY_AST_NODE_OPERATOR_CONDITIONAL: {
				if(operatorCount == 0) {
					operatorCount++;	
					nextOperator = TOKEN_DOUBLE_EQUAL;
					myDialogue_advanceNode(program, &nodeAt);
				} else {
					easyAst_addError(program->ast, "You've put to many operators in a row", nodeAt->token.lineNumber);
				}
			} break;
			case EASY_AST_NODE_OPERATOR_EVALUATE: {
				if(nodeAt->token.type == TOKEN_OPEN_PARENTHESIS) {
					parenthesisCount++;
					myDialogue_advanceNode(nodeAt);
				} else if(nodeAt->token.type == TOKEN_CLOSE_PARENTHESIS) {
					parenthesisCount--;
					//End parsing, so persumably pop back to the parent node
					assert(nodeAt->parent);
					parsing = false;
				} else {
					assert(false);
				}
			} break;
			case EASY_AST_NODE_PRIMITIVE: 
			case EASY_AST_NODE_VARIABLE: {
				if(varToFillOut->type != VAR_NULL && varToFillOut->arraySize <= indexIntoArray && (!varToFillOut->isExpanding || indexIntoArray >= varToFillOut->maxArraySize)) {
					easyAst_addError(program->ast, "Array index out of bounds.", nodeAt->token.lineNumber);
				}	

				///////////////////////************ Find the variables or primitives & get the types *************////////////////////
				MyDialogue_ProgramVariable *thisVar = 0;
				int thisVarIndex = 0;

				VarType thisType = VAR_NULL;
				if(nodeAt->type == EASY_AST_NODE_VARIABLE) {
					thisVar = myDialogue_findVariable(&program->stack, nodeAt->token.at, nodeAt->token.size);

					if(thisVar) {
						thisType = thisVar.returnType;
						//NOTE(ollie): Is an array
						if(thisVar.sizeOfArray > 1) {
							EasyAst_Node *tempNode = nodeAt->next;
							if(tempNode && tempNode->type == EASY_AST_NODE_ARRAY_IDENTIFIER && tempNode->token.type == TOKEN_OPEN_SQUARE_BRACKET) {

								tempNode = tempNode->next;
								if(tempNode && tempNode->type == EASY_AST_NODE_PRIMITIVE && tempNode->token.type == TOKEN_INTEGER) {
									thisVarIndex = tempNode->token.intVal;
									if(thisVar.sizeOfArray <= thisVarIndex) {
										easyAst_addError(program->ast, "Array out of bounds. Your trying to access an element outside of the array. ", nodeAt->token.lineNumber);
									}

									tempNode = tempNode->next;
									if(tempNode && tempNode->type == EASY_AST_NODE_ARRAY_IDENTIFIER && tempNode->token.type == TOKEN_CLOSE_SQUARE_BRACKET) {

									} else {
										easyAst_addError(program->ast, "You forgot ']' on the array element.", nodeAt->token.lineNumber);
									}
								} else {
									easyAst_addError(program->ast, "If your getting an element out, you need an integer index.", nodeAt->token.lineNumber);
								}

							} else {
								easyAst_addError(program->ast, "This is an array. You need to access an element.", nodeAt->token.lineNumber);
							}
						} else {
							EasyAst_Node *tempNode = nodeAt->next;
							if(tempNode && tempNode->type == EASY_AST_NODE_ARRAY_IDENTIFIER) {
								easyAst_addError(program->ast, "This is variable isn't an array.", nodeAt->token.lineNumber);
							}
						}
						
					} else {
						easyAst_addError(program->ast, "Undeclared variable trying to be used.", nodeAt->token.lineNumber);
					}
				} else {
					assert(nodeAt->type == EASY_AST_NODE_PRIMITIVE);
					thisType = myDialogue_getVariableTypeFromToken(nodeAt->token.type);
				}

				////////////////////////////////////////////////////////////////////

				assert(thisType != VAR_NULL);

				///////////////////////************* Add space size to the stack for the variable if we need to ************////////////////////

				int countToAlloc = 1;

				//NOTE(ollie): Empty variable or we are initialising an array so it's OK to expand the array. We're not going to stomp data on the stack 
				if(varToFillOut->type == VAR_NULL || (varToFillOut->arraySize <= indexIntoArray && (varToFillOut->isExpanding || indexIntoArray < varToFillOut->maxArraySize))) {

					if(nodeAt->type == EASY_AST_NODE_VARIABLE) {
						countToAlloc = thisVar->returnCount;
					}

					//NOTE(ollie): Should go in sync with array size
					assert(indexIntoArray == varToFillOut->arraySize);
					myDialogue_addDataToVariable(&program->stack, varToFillOut, 0, myDialogue_getSizeOfType_withMinimum(varToFillOut->type)*countToAlloc, varToFillOut->type, countToAlloc);
				}	



				///////////////////////************ Actually enter into the expression and get the return type *************////////////////////

				if(varToFillOut->type == thisType) {
					assert(varToFillOut->arraySize > 0);
					assert(varToFillOut->type != VAR_NULL);
					
					myDialogue_pushStackFrame(&program->stack);

					////////////////////////////////////////////////////////////////////
					if(!thisVar) {
						assert(nodeAt->type == EASY_AST_NODE_PRIMITIVE);

						//NOTE(ollie): Is a primitive so create a temp variable so you can do the operations
						thisVar = myDialogue_pushVariableToStack_withoutData(&program->stack, varNode.token.at, varNode.token.size, 1, false, MY_DIALOGUE_VARIABLE_DEFAULT);

						void *value = 0;
						
						switch(nodeAt->token.type) {
							case TOKEN_INTEGER: {
								value = &nodeAt->token.intVal;
							} break;
							case TOKEN_FLOAT: {
								value = &nodeAt->token.floatVal;
							} break;
							case TOKEN_STRING: {
								//NOTE(ollie): Allocate a temporary string, then free it when we pop the variables
								u32 stringSizeInBytes = sizeof(char)*(nodeAt->token.size);
								char *stringOnHeap = (char *)easyPlatform_allocateMemory(stringSizeInBytes, 0);

								//NOTE(ollie): copy the string to the variable
								easyPlatform_copyMemory(stringOnHeap, nodeAt->token.at, stringSizeInBytes);
								value = stringOnHeap;
								thisVar->stringLength = nodeAt->token.size;
							} break;
							default: {
								easyAst_addError(program->ast, "Primitive type not handled.", nodeAt->token.lineNumber);
							}
						}
						//NOTE(ollie): Pointer has been assigned
						assert(value);
						myDialogue_addDataToVariable(&program->stack, thisVar, value, myDialogue_getSizeOfType_withMinimum(thisType), thisType, 1);
					}

					assert(thisVar);

					//NOTE(ollie): Run the operator if there is one
					if(nextOperator == TOKEN_UNINITIALISED) {
						//NOTE(ollie): Block copy the tempVar into the varToFillOut
						myDialogue_copyVariableData(&program->stack, varToFillOut, thisVar, indexIntoArray, thisVarIndex);
					} else {
						varToFillOut = myDialogue_combineVariablesUsingOperator(&program->stack, varToFillOut, thisVar, nextOperator, indexIntoArray, thisVarIndex);
					}

					myDialogue_popStackFrame(&program->stack);


					////////////////////////////////////////////////////////////////////
				} else {
					easyAst_addError(program->ast, "This primitive or variable is the wrong type.", nodeAt->token.lineNumber);	
				} 


				if(nodeAt->type == EASY_AST_NODE_VARIABLE) {
					//NOTE(ollie): This would have been checked earlier when we actually looked for the index. So no need to do checks. 
					myDialogue_advanceNode(nodeAt);
					if(nodeAt && nodeAt->type == EASY_AST_NODE_ARRAY_IDENTIFIER) {
						myDialogue_advanceNode(nodeAt);
						if(nodeAt && nodeAt->type == EASY_AST_NODE_PRIMITIVE) {
							assert(nodeAt->token.type == TOKEN_INTEGER);
							myDialogue_advanceNode(nodeAt);
							if(nodeAt && nodeAt->type == EASY_AST_NODE_ARRAY_IDENTIFIER) {
								myDialogue_advanceNode(nodeAt);
							}
						}
					}
						
				} else {
					//primitive so just move once
					myDialogue_advanceNode(nodeAt);		
				}
				

			} break;
			case EASY_AST_NODE_OPERATOR_END_LINE: {
				parsing = false;
				myDialogue_advanceNode(nodeAt);
			} break;
			
			default: {
				easyAst_addError(program->ast, "There is an erroneous type here", nodeAt->token.lineNumber);
				parsing = false;
			}
		}
	}

	if(parenthesisCount != 0) {
		easyAst_addError(program->ast, "Did you forget an end parenthesis?", nodeAt->token.lineNumber);
	}

	if(operatorCount != 0) {
		easyAst_addError(program->ast, "Haven't used all the operators ", nodeAt->token.lineNumber);
	}

	return nodeAt;
}

static EasyAst_Node *myDialogue_parseVariableAssignment(MyDialogue_Program *program, EasyAst_Node *nodeAt) {

	bool isArray = false;
	u32 sizeOfArray = 1;

	EasyAst_Node *varNode = 0;
	VarType varType  = VAR_NULL;

	//TODO(ollie): In the future we'll want to expand this to set multiple variables at once. 
	MyDialogue_ProgramVariable *var = 0;
	bool varIsSet = false;
	bool equalSign = false;

	int commaCount = 0; 
	bool hasBeenFound = false;
	bool isNewVariable = false;

	int indexAt = 0;

	MyDialogue_EvaluateFlags evaluateFlags = MY_DIALOGUE_EVALUATE_DEFAULT;

	bool parsing = true;
	while(parsing && program->ast.errorCount == 0) {
		switch(nodeAt->type) {
			case EASY_AST_NODE_VARIABLE: {
				if(!equalSign) {
					//NOTE(ollie): Try find the variable
					if(varIsSet) {
						easyAst_addError(program->ast, "Multiple variable assignments aren't supported.", nodeAt->token.lineNumber);
					} else {
						varNode = nodeAt;
						//NOTE(ollie): could find it or maybe couldn't

						MyDialogue_ProgramVariable *var = myDialogue_findVariable(&program->stack, nodeAt->token.at, nodeAt->token.size);	
						hasBeenFound = (var != 0);
					}

					varIsSet = true;
					
					if(nodeAt->next) {

						//NOTE(ollie): Advance the node 
						nodeAt = nodeAt->next;					
					} else {
						easyAst_addError(program->ast, "There is a variable not doing anything.", nodeAt->token.lineNumber);
					}
				} else {
					if(varIsSet) {
						if(commaCount == 0) {
							commaCount++;

							nodeAt = myDialogue_evaluateVariable(program, nodeAt, flags, varToFillOut, nextOperator, function);

							//NOTE(ollie): advance the node
							myDialogue_advanceNode(program, &nodeAt);
						} else {
							easyAst_addError(program->ast, "Too many commas in a row. Did you add too many commas?", nodeAt->token.lineNumber);
						}
					} else {
						easyAst_addError(program->ast, "You haven't set a variable name. What are you trying to assign to?", nodeAt->token.lineNumber);
					}
				}
				
			} break;
			case EASY_AST_NODE_OPERATOR_FUNCTION: {
				if(equalSign) {
					if(varIsSet) {
						if(commaCount == 0) {
							commaCount++;

							nodeAt = myDialogue_evaluateVariable(program, nodeAt, flags, varToFillOut, nextOperator, function);

							//NOTE(ollie): advance the node
							myDialogue_advanceNode(program, &nodeAt);

						} else {
							easyAst_addError(program->ast, "Too many commas in a row. Did you add too many commas?", nodeAt->token.lineNumber);
						}
					} else {
						easyAst_addError(program->ast, "You haven't set a variable name.", nodeAt->token.lineNumber);
					}
				} else {
					easyAst_addError(program->ast, "There isn't an assignment operator. Did you have to set an equal sign.", nodeAt->token.lineNumber);
				}
			} break;
			//NOTE(ollie): Not a new variable, juat assigning data to one that already exists. 
			case EASY_AST_NODE_OPERATOR_ASSIGNMENT: {
				if(equalSign) {
					easyAst_addError(program->ast, "You've already used the assignment operator. The language doesn't support multiple assignments atm.", nodeAt->token.lineNumber);
				}

				if(!var) {
					easyAst_addError(program->ast, "This variable hasn't been declared before. Did you mean to use the ':=' operator?", nodeAt->token.lineNumber);
				} else {
					equalSign = true;

					myDialogue_advanceNode(program, &nodeAt);
				}
			} break;
			case EASY_AST_NODE_OPERATOR_NEW_ASSIGNMENT: {
				if(equalSign) {
					easyAst_addError(program->ast, "You've already used the assignment operator. The language doesn't support multiple assignments atm.", nodeAt->token.lineNumber);
				}

				if(var) {
					easyAst_addError(program->ast, "This variable is already in use.", nodeAt->token.lineNumber);
				} else if(!varIsSet) {
					easyAst_addError(program->ast, "No variable to set to. Did you forget the name of a new variable.", nodeAt->token.lineNumber);
				} else {
					equalSign = true;
					isNewVariable = true;

					if(nodeAt->next) {
						//NOTE(ollie): Create a new variable, we haven't actually put any data on the stack, we do it as we find it. 
						//TODO(ollie): This could be error prone & won't work if we want to do multiple variable assignments at once. 
						u32 maxSize = sizeOfArray; //should be 1 or more
						bool expand = false;
						if(sizeOfArray < 0) {
							//we know it's an array with no specified size, so we want it to expand
							expand = true;
							maxSize = 1;
						}

						assert(maxSize >= 1);

						var = myDialogue_pushVariableToStack_withoutData(&program->stack, varNode.token.at, varNode.token.size, maxSize, expand, MY_DIALOGUE_VARIABLE_DEFAULT | MY_DIALOGUE_VARIABLE_VISIBLE);
						assert(var);

						//NOTE(ollie): Advance to the next node
						nodeAt = nodeAt->next;
					} else {
						easyAst_addError(program->ast, "You aren't setting any data to this variable.", nodeAt->token.lineNumber);
					}
				}
			} break;
			//NOTE(ollie): Parse the array details
			case EASY_AST_NODE_ARRAY_IDENTIFIER: {
				if(nodeAt->next && *nodeAt->token.at == '[') {

					//NOTE(ollie): Advance the node
					nodeAt = nodeAt->next;

					//NOTE(ollie): An array, so when we evaluate an expression, take a comma as the end of the expression
					evaluateFlags |= MY_DIALOGUE_EVALUATE_COMMA;

					if(nodeAt->type == EASY_AST_NODE_ARRAY_IDENTIFIER && *nodeAt->token.at == ']') {
						isArray = true;

						sizeOfArray = -1; //we know we set it as we find the data
						
						if(nodeAt->next) {
							//NOTE(ollie): Advance the node
							nodeAt = nodeAt->next;	
						} else {
							easyAst_addError(program->ast, "You didn't finish the line. There's no data.", nodeAt->token.lineNumber);
						}
						
					} else if(nodeAt->type == EASY_AST_NODE_NUMBER || nodeAt->type == EASY_AST_NODE_VARIABLE) {
						isArray = true;

						if(nodeAt->type == EASY_AST_NODE_NUMBER) {
							sizeOfArray = nodeAt->token.intVal;	
						} else if(nodeAt->type == EASY_AST_NODE_VARIABLE) {
							MyDialogue_ProgramVariable *tempVar = myDialogue_findVariable(&program->stack, nodeAt->token.at, nodeAt->token.size);
							if(tempVar) {
								if(tempVar->type == VAR_INT) {
									sizeOfArray = myDialogue_castVariableAsInt(&program->stack, tempVar);
								} else {
									easyAst_addError(program->ast, "Variable in array size specifier has to me a type integer.", nodeAt->token.lineNumber);	
								}
							} else {
								easyAst_addError(program->ast, "This variable doesn't exist in the array size specifier.", nodeAt->token.lineNumber);
							}
							
						} else {
							assert(false);
						}
						

						if(nodeAt->next) {
							//NOTE(ollie): Advance the node
							nodeAt = nodeAt->next;

							if(nodeAt->type == EASY_AST_NODE_ARRAY_IDENTIFIER && *nodeAt->token.at == ']') {
								//NOTE(ollie): Advance the node
								myDialogue_advanceNode(program, &nodeAt);
							} else {
								easyAst_addError(program->ast, "Array specifier wasn't finished. Did you forget ']' ?", nodeAt->token.lineNumber);	
							}
						} else {
							easyAst_addError(program->ast, "You didn't finish specifying the array.", nodeAt->token.lineNumber);	
						}

					} else {
						easyAst_addError(program->ast, "Please specify the size of the array or leave it blank.", nodeAt->token.lineNumber);	
					}
				} else {
					easyAst_addError(program->ast, "You didn't finish this line or did you forget '[' ?", nodeAt->token.lineNumber);
				}

			} break;
			case EASY_AST_NODE_PARENT: {
				//NOTE(ollie): There is a sub-expression so go evaluate that, & bring back the result;
				if(equalSign) {
					if(varIsSet) {
						if(commaCount == 0) {
							commaCount++;
							
							nodeAt = myDialogue_evaluateVariable(program, nodeAt, flags, varToFillOut, TOKEN_UNINITIALISED, 0);

							//NOTE(ollie): advance the node
							myDialogue_advanceNode(program, &nodeAt);
						} else {
							easyAst_addError(program->ast, "Too many commas in a row. Did you add too many commas?", nodeAt->token.lineNumber);
						}
					} else {
						easyAst_addError(program->ast, "You haven't set a variable name.", nodeAt->token.lineNumber);
					}
				} else {
					easyAst_addError(program->ast, "There isn't an assignment operator. Did you have to set an equal sign.", nodeAt->token.lineNumber);
				}

			} break;
			case EASY_AST_NODE_PRIMITIVE: {
				if(equalSign) {
					if(varIsSet) {
						if(commaCount == 0) {
							commaCount++;
							nodeAt = myDialogue_evaluateVariable(program, nodeAt, flags, varToFillOut, TOKEN_UNINITIALISED, function);
						} else {
							easyAst_addError(program->ast, "Too many commas in a row. Did you add too many commas?", nodeAt->token.lineNumber);
						}
					} else {
						easyAst_addError(program->ast, "You haven't set a variable name.", nodeAt->token.lineNumber);
					}
				} else {
					easyAst_addError(program->ast, "There isn't an assignment operator. Did you have to set an equal sign.", nodeAt->token.lineNumber);
				}
			} break;
			case EASY_AST_NODE_OPERATOR_COMMA: {
				//NOTE(ollie): Should only be commas if it's an array
				if(isArray) {
					commaCount--;

					//NOTE(ollie): Advance the index we're at
					indexAt++;
					if(commaCount != 0) {
						easyAst_addError(program->ast, "Did you have too many commas or miss a comma?", nodeAt->token.lineNumber);
					} else {
						//NOTE(ollie): Advance past the comma
						myDialogue_advanceNode(program, &nodeAt);
					}
				} else {
					easyAst_addError(program->ast, "This isn't an array type. Did you mean to specify an array with '[]' syntax?", nodeAt->token.lineNumber);					
				}
			} break;
			case EASY_AST_NODE_OPERATOR_END_LINE: {
				var->isExpanding = false;
				if(commaCount == 0) {
					easyAst_addError(program->ast, "There's a comma at the end of this line.", nodeAt->token.lineNumber);
				}

				myDialogue_advanceNode(program, &nodeAt);
				parsing = false;
			} break;
			default: {
				easyAst_addError(program->ast, "Unidentified value. Did you make mean something else? Or forget a semicolon at the end?", nodeAt->token.lineNumber);
			}	
		} 
	}

	return nodeAt;
}

typedef enum {
	MY_DIALOGUE_CODE_BLOCK_HAS_BRACKET,
} MyDialogue_CodeBlockFlag;

static EasyAst_Node *myDialogue_evaluateCodeBlock(MyDialogue_Program *program, EasyAst_Node *nodeAt, MyDialogue_ProgramVariable *varToFillOut, MyDialogue_CodeBlockFlag flags) {
	assert(nodeAt);

	EasyAst_Node *startNode = nodeAt;

	if(flags & MY_DIALOGUE_CODE_BLOCK_HAS_BRACKET) {
		//NOTE(ollie): First thing you should see when evaluating a new code block is an open block
		assert(nodeAt->type == EASY_AST_NODE_OPERATOR_SCOPE && nodeAt->token.at[0] == '{');
	}

	bool parsing = true;

	while(nodeAt && parsing && program->errors.errorCount == 0) {

		switch(noteAt->type) {
			case EASY_AST_NODE_OPERATOR_IF: {

			} break;
			case EASY_AST_NODE_VARIABLE: {
				nodeAt = myDialogue_parseVariableAssignment(program);
			} break;
			case EASY_AST_NODE_PARENT: {
				//NOTE(ollie): There is a sub-expression so go evaluate that, & bring back the result;
				assert(nodeAt->child);

				//NOTE(ollie): Take temporary node
				EasyAst_Node *tempNode = nodeAt;

				//NOTE(ollie): Move to the child node
				nodeAt = nodeAt->child;

				if(nodeAt->type == EASY_AST_NODE_OPERATOR_SCOPE) {
					myDialogue_evaluateCode(program, nodeAt, varToFillOut, indexIntoArray, MY_DIALOGUE_CODE_BLOCK_HAS_BRACKET);
				} else {
					easyAst_addError(program->ast, "There is a parenthesis here that shouldn't exist", nodeAt->token.lineNumber);		
				}

				//NOTE(ollie): restore the node
				nodeAt = tempNode;

				//NOTE(ollie): advance the node
				myDialogue_advanceNode(program, &nodeAt);
			} break;
			case EASY_AST_NODE_OPERATOR_ASSIGNMENT: {
				easyAst_addError(program->ast, "Nothing to assign to. Did you forget a left-hand variable?", nodeAt->token.lineNumber);
			} break;
			case EASY_AST_NODE_OPERATOR_FUNCTION: {

				MyDialogue_Function *function = myDialogue_findFunction(program, nodeAt->token.at, nodeAt->token.size);

				if(!function) {
					easyAst_addError(program->ast, "The function you are refering to hasn't be declared.", nodeAt->token.lineNumber);
				} else {	

					myDialogue_pushStackFrame(&program->stack);

					MyDialogue_ProgramVariable *tempVar = myDialogue_pushVariableToStack_withoutData(&program->stack, varNode->token.at, varNode->token.size, function->returnCount, false, MY_DIALOGUE_VARIABLE_DEFAULT);
					myDialogue_addDataToVariable(&program->stack, tempVar, 0, myDialogue_getSizeOfType_withMinimum(function->returnType)*function->returnCount, function->returnType, function->returnCount);


					//NOTE(ollie): Function that doesn't what store anything in a variable so just discard the variable
					nodeAt = myDialogue_evaluateFunction(program, nodeAt, tempVar, 0);

					myDialogue_popStackFrame(&program->stack);
				}

				
			} break;
			EASY_AST_NODE_OPERATOR_MAIN: {
				//just keep going past it
				myDialogue_advanceNode(program, &nodeAt);
			} break;
			EASY_AST_NODE_OPERATOR_SCOPE: {
				if(nodeAt->token.type == TOKEN_OPEN_BRACKET) {
					program->stackScopeCount++;
					myDialogue_pushStackFrame(program);

					myDialogue_advanceNode(program, &nodeAt);
				} else if(nodeAt->token.type == TOKEN_CLOSE_BRACKET) {
					program->stackScopeCount--;
					
					assert(nodeAt->parent == startNode->parent);

					//NOTE(ollie): End parsing
					parsing = false;

					myDialogue_popStackFrame(program);
				} else {
					assert(!"shouldn't be here");
				}
				
			} break;
			case EASY_AST_NODE_OPERATOR_EVALUATE: {
				easyAst_addError(program->ast, "There shouldn't be a parenthesis here.", nodeAt->token.lineNumber);

			} break;
			default: {
				//case not handled
				myDialogue_advanceNode(program, &nodeAt);
			}

		}

	}


	return nodeAt;

}

static EasyAst_Node *myDialogue_evaluateFunction(MyDialogue_Program *program, EasyAst_Node *nodeAt, MyDialogue_Function *func, MyDialogue_ProgramVariable *varToFillOut) { 

	myDialogue_pushStackBlinder(&program->stack);

	myDialogue_pushStackFrame(&program->stack);

	///////////////////////*********** Parse & set the variables in the input params**************////////////////////


	////////////////////////////////////////////////////////////////////

	myDialogue_evaluateCodeBlock(program, nodeAt, varToFillOut, MY_DIALOGUE_CODE_BLOCK_HAS_BRACKET);


	myDialogue_popStackFrame(&program->stack);

	myDialogue_popStackBlinder(&program->stack);
}