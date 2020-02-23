typedef enum {
	MY_DIALOGUE_ENDED,
	MY_DIALOGUE_RUNNING,
	MY_DIALOGUE_WAITIMG_FOR_PLAYER_INPUT,
	MY_DIALOGUE_WAITING_FOR_NPC_DIALOGUE
} MyDialogue_ProgramMode;

typedef struct {
	char *name;

	u32 offsetInStack; //to get the data out
	u32 sizeInBytes;

	int arraySize; 

	VarType type;
} MyDialogue_ProgramVariable;

typedef struct {
	u32 offsetAt;
} MyDialogue_StackSnapshot;


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

} MyDialogue_ProgramStack;

///////////////////////************ Helper functions *************////////////////////
static MyDialogue_ProgramVariable *myDialogue_addVariableWithoutData(MyDialogue_ProgramStack *stack, char *name) {
	assert(stack->variableCount < arrayCount(stack->variables));

	MyDialogue_ProgramVariable *var = stack->variables + stack->variableCount++;

	memset(var, 0, sizeof(MyDialogue_ProgramVariable)); 

	var->name = name;
	var->offsetInStack = stack->stackOffset;
	
	return var;
}

static void myDialogue_addDataToVariable(MyDialogue_ProgramStack *stack, MyDialogue_ProgramVariable *var, void *data, u32 sizeInBytes, VarType type, int count) {
	var->sizeInBytes += sizeInBytes;
	var->type = type;
	var->arraySize += count;

	u32 newStackSize = var->offsetInStack + sizeInBytes;
	if(newStackSize <= arrayCount(stack->stack)) {

		//NOTE(ollie): Get the stack pointer
		u8 *ptrAt = stack->stack + var->offsetInStack;

		//NOTE(ollie): Add the data to the stack
		memcpy(ptrAt, data, sizeInBytes);

		//NOTE(ollie): Advance the stack offset
		stack->stackOffset += sizeInBytes;
		} else {
	} else {
		assert(!"stack overflow");
	}

	return var;
} 

static MyDialogue_ProgramVariable *myDialogue_addVariable(MyDialogue_ProgramStack *stack, char *name, void *data, u32 sizeInBytes, VarType type, int count) {
	assert(stack->variableCount < arrayCount(stack->variables));

	MyDialogue_ProgramVariable *var = stack->variables + stack->variableCount++;

	var->name = name;
	var->sizeInBytes = sizeInBytes;
	var->type = type;
	var->arraySize = count;
	var->offsetInStack = stack->stackOffset;

	u32 newStackSize = stack->stackOffset + sizeInBytes;
	if(newStackSize <= arrayCount(stack->stack)) {

		//NOTE(ollie): Get the stack pointer
		u8 *ptrAt = stack->stack + stack->stackOffset;

		//NOTE(ollie): Add the data to the stack
		memcpy(ptrAt, data, sizeInBytes);

		//NOTE(ollie): Advance the stack offset
		stack->stackOffset += sizeInBytes;
	} else {
		assert(!"stack overflow");
	}

	return var;
}

static MyDialogue_ProgramVariable *myDialogue_findVariable(MyDialogue_ProgramStack *stack, char *name, u32 strLength) {
	MyDialogue_ProgramVariable *result = 0;
	for(int i = 0; i < stack->variableCount && !result; ++i) {
		MyDialogue_ProgramVariable *var = stack->variables + i;
		if(stringsMatchNullN(var->name, name, strLength)) {
			result = var;
			break;
		}
	}
	return result;
}

static void myDialogue_pushStackFrame(MyDialogue_ProgramStack *stack) {
	//NOTE(ollie): Get the next snapshot out of the arary
	assert(snapshotCount < arrayCount(stack->snapshots));
	MyDialogue_StackSnapshot *snapshot = stack->snapshots + stack->snapshotCount++;

	//NOTE(ollie): Take the snapshot
	snapshot->offsetAt = stack->stackOffset;
}

static void myDialogue_popStackFrame(MyDialogue_ProgramStack *stack) {

	//NOTE(ollie): Get the snapshop we're removing & decrement the snapshot count
	MyDialogue_StackSnapshot *snapshot = stack->snapshots[--stack->snapshotCount];

	//NOTE(ollie): Remove all the variables that existed in that scope
	for(int i = 0; i < stack->variableCount; ) {

		//NOTE(ollie): Get the variable
		MyDialogue_ProgramVariable *var = stack->variables + stack->variableCount;

		//NOTE(ollie): Identify them by the offset they are in the stack
		if(var->offsetInStack >= snapshot->offsetAt) {

			//NOTE(ollie): Get the last variable data of the end & block copy it in the spot of this variable
			*var = stack->variables[--stack->variableCount];
		} else {
			//NOTE(ollie): Only increment if we didn't remove a variable
			++i;
		}
	}
	
	//NOTE(ollie): Take the snapshot
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
static EasyAst_Node *myDialogue_parseVariableAssignment(MyDialogue_Program *program, MyDialogue_ProgramVariable *var) {

	EasyAst_Node *nodeAt = nodeAt->type;

	bool isArray = false;
	u32 sizeOfArray = 0;

	EasyAst_Node *varNode = 0;
	VarType varType  = VAR_NULL;



	bool parsing = true;
	while(parsing) {
		switch(nodeAt->type) {
			case EASY_AST_NODE_OPERATOR_ASSIGNMENT: {
				if(var) {
					easyAst_addError(program->ast, "This variable is already in use.", nodeAt->token.lineNumber);
				} else {
					varNode = nodeAt;
					var = myDialogue_addVariableWithoutData(&program->stack, varNode.token.at, varNode.token.size);

					nodeAt = nodeAt->type;
					
				}
			} break;
			case EASY_AST_NODE_OPERATOR_NEW_ASSIGNMENT: {
				if(var) {
					easyAst_addError(program->ast, "This variable is already in use.", nodeAt->token.lineNumber);
				} else {
					varNode = nodeAt;
					
					if(nodeAt->next) {
						nodeAt = nodeAt->next;
					} else {
						easyAst_addError(program->ast, "Line not finished.", nodeAt->token.lineNumber);
					}
					
					// myDialogue_addVariable(MyDialogue_ProgramStack *stack, char *name, void *data, u32 sizeInBytes, VarType type);
					
				}
			} break;
			case EASY_AST_NODE_OPERATOR_ASSIGNMENT: {
				if(!var) {
					easyAst_addError(program->ast, "This variable hasn't been set before.", nodeAt->token.lineNumber);
				} else {
					
					
				}
			} break;
			case EASY_AST_NODE_ARRAY_IDENTIFIER: {
				if(nodeAt->next && *nodeAt->token.at == '[') {
					nodeAt = nodeAt->next;

					if(nodeAt->type == EASY_AST_NODE_ARRAY_IDENTIFIER && *nodeAt->token.at == ']') {
						isArray = true;
						sizeOfArray = -1;


					} else if(nodeAt->type == EASY_AST_NODE_NUMBER) {
						isArray = true;
						sizeOfArray = nodeAt->token.intVal;

						if(myDialogue_checkNodeHasNext(program, nodeAt)) {
							nodeAt = nodeAt->next;

							if(nodeAt->type == EASY_AST_NODE_ARRAY_IDENTIFIER && *nodeAt->token.at == ']') {
								nodeAt = nodeAt->next;
							} else {
								easyAst_addError(program->ast, "Array specifier wasn't finished. Did you forget ']' ?", nodeAt->token.lineNumber);	
							}
						}

					} else {
						easyAst_addError(program->ast, "Please specify the size of the array or leave it blank.", nodeAt->token.lineNumber);	
					}
				} else {
					easyAst_addError(program->ast, "You didn't finish this line.", nodeAt->token.lineNumber);
				}

			} break;
			case EASY_AST_NODE_NUMBER: {

			} break;
			case EASY_AST_NODE_VARIABLE: {
				int tempArrayCount = 0;
				
				while(nodeAt->type != EASY_AST_NODE_OPERATOR_END_LINE) {
					bool addData = false;
					switch (nodeAt->token.type) {
						case TOKEN_INTEGER: {
							if(varType == VAR_NULL) {
								varType = VAR_INT;
								addData = true;
							} else if(isArray) {
								if(nodeAt->token.type == TOKEN_INT) {
									//do nothing
									addData = true;
								} else {
									easyAst_addError(program->ast, "You are mixing types in this array. The array is type INT, but you've passed type xxxx to it", nodeAt->token.lineNumber);
								}
							} else {
								easyAst_addError(program->ast, "You need to specify if this is an array you intend to make", nodeAt->token.lineNumber);
							}

							if(addData) {
								myDialogue_addDataToVariable(&program->stack, &nodeAt->token.intVal, sizeof(int), VAR_INT, 1);
							}
						} break;
						case TOKEN_FLOAT: {
							if(varType == VAR_NULL) {
								varType = VAR_FLOAT;
								addData = true;
							} else if(isArray) {
								if(nodeAt->token.type == TOKEN_FLOAT) {
									//do nothing
									addData = true;
								} else {
									easyAst_addError(program->ast, "You are mixing types in this array. The array is type FLOAT, but you've passed type xxxx to it", nodeAt->token.lineNumber);
								}
							} else {
								easyAst_addError(program->ast, "You need to specify if this is an array you intend to make", nodeAt->token.lineNumber);
							}

							if(addData) {
								myDialogue_addDataToVariable(&program->stack, &nodeAt->token.floatVal, sizeof(float), VAR_FLOAT, 1);
							}
						} break;
						case TOKEN_STRING: {
							if(varType == VAR_NULL) {
								varType = VAR_STRING;
								addData = true;
							} else if(isArray) {
								if(nodeAt->token.type == TOKEN_STRING) {
									//do nothing
									addData = true;
								} else {
									easyAst_addError(program->ast, "You are mixing types in this array. The array is type STRING, but you've passed type xxxx to it", nodeAt->token.lineNumber);
								}
							} else {
								easyAst_addError(program->ast, "You need to specify if this is an array you intend to make", nodeAt->token.lineNumber);
							}

							if(addData) {
								myDialogue_addDataToVariable(&program->stack, nodeAt->token.at, sizeof(char)*nodeAt->token.size, VAR_STRING, 1);
							}
						} break;
						default: {

						}
					}
				}
			} break;
			case EASY_AST_NODE_OPERATOR_END_LINE: {
				parsing = false;
			} break;
			default: {

			}	
		} 
	}
	

	return nodeAt;
}

static void myDialogue_runProgram(MyDialogue_Program *program) { //TODO(ollie): In the future it would cool to convert it to bytecode

	EasyAst_Node *nodeAt = program->nodeAt;
	assert(nodeAt);

	int level = 0;

	while(nodeAt) {

		switch(noteAt->type) {
			case EASY_AST_NODE_OPERATOR_IF: {

			} break;
			case EASY_AST_NODE_VARIABLE: {
				MyDialogue_ProgramVariable *var = myDialogue_findVariable(&program->stack, nodeAt->token.at, nodeAt->token.size);
				if(nodeAt->next) {
					myDialogue_parseVariableAssignment();
				} else {
					easyAst_addError(program->ast, "There is a variable not doing anything.", nodeAt->token.lineNumber);
				}
			} break;
			case EASY_AST_NODE_OPERATOR_ASSIGNMENT: {
				easyAst_addError(program->ast, "Nothing to assign to. Did you forget a left-hand variable?", nodeAt->token.lineNumber);
			} break;
			default: {
				//case not handled
			}

		}

		// char *a = EasyAst_NodeTypeStrings[(int)nodeAt->type];
		// char *b = LexTokenTypeStrings[(int)nodeAt->token.type];
		// char *c = EasyAst_NodePrecedenceStrings[(int)nodeAt->precedence];



		///////////////////////************** Move to the next node in the tree***********////////////////////

		//NOTE(ollie): Has a child node so evaluate that node first
		if(nodeAt->child) {
			
			//NOTE(ollie): Increase the level we're at
			level++;
			//NOTE(ollie): assign the new node to the child node
			nodeAt = nodeAt->child;

		} else if(!nodeAt->next) { //NOTE(ollie): The node doesn't have any children or any nodes next to it, so pop up to the parent node
			//NOTE(ollie): We keep popping up looking for a node we have't evaluated
			
			bool keepPopping = true;

			while(keepPopping) {
				//NOTE(ollie): If there is a parent of this nod, pop up because we know the current node does't have a next node 
				if(nodeAt->parent) {
					assert(!nodeAt->next);
					//NOTE(ollie): Decrease the level
					level--;
					nodeAt = nodeAt->parent;
				} else {
					assert(!nodeAt->next);
					keepPopping = false;

					//NOTE(ollie): Just for debug
					finished = true;
					//NOTE(ollie): We're finished evaluating the syntax tree, so stop walking it.
					nodeAt = 0;
				}

				if(nodeAt && nodeAt->next) {
					//NOTE(ollie):  Since we pop back to the node we would have entered & therefore have already evaluated, we want to go to the next node
					nodeAt = nodeAt->next;
					keepPopping = false;
				} 
			} 
			
		} else {
			nodeAt = nodeAt->next;
		}

		////////////////////////////////////////////////////////////////////

	}

}