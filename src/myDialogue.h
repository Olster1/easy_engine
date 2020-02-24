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

static inline int myDialogue_castVariableAsInt(MyDialogue_ProgramStack *stack, MyDialogue_ProgramVariable *var) {
	assert(var->type == VAR_INT);
	assert(var->sizeInBytes == sizeof(int));

	u8 *ptr = stack->stack + var->offsetInStack; 
	assert(var->offsetInStack <= stack->stackOffset);


	int result = *((int *)(ptr));

	return result;
}

static inline char *myDialogue_castVariableAsString(MyDialogue_ProgramStack *stack, MyDialogue_ProgramVariable *var) {
	assert(var->type == VAR_STRING);

	u8 *ptr = stack->stack + var->offsetInStack; 
	assert(var->offsetInStack <= stack->stackOffset);

	char *result = (char *)(ptr);

	return result;
}


static inline float myDialogue_castVariableAsFloat(MyDialogue_ProgramStack *stack, MyDialogue_ProgramVariable *var) {
	assert(var->type == VAR_FLOAT);
	assert(var->sizeInBytes == sizeof(float));

	u8 *ptr = stack->stack + var->offsetInStack; 
	assert(var->offsetInStack <= stack->stackOffset);

	float result = *((float *)(ptr));

	return result;
}

static MyDialogue_ProgramVariable *myDialogue_addVariableWithoutData(MyDialogue_ProgramStack *stack, char *name) {
	assert(stack->variableCount < arrayCount(stack->variables));

	MyDialogue_ProgramVariable *var = stack->variables + stack->variableCount++;

	memset(var, 0, sizeof(MyDialogue_ProgramVariable)); 


	//NOTE(ollie): We initalise the variable to just be a null type
	var->type = VAR_NULL; 
	var->name = name;
	var->offsetInStack = stack->stackOffset;
	
	return var;
}

static void myDialogue_addDataToVariable(MyDialogue_ProgramStack *stack, MyDialogue_ProgramVariable *var, void *data, u32 sizeInBytes, VarType type, int count) {
	//NOTE(ollie): Nothing has been added to the stack since this one. I.e. is contigous
	assert(stack->stackOffset == var->offsetInStack	+ var->sizeInBytes);

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
			assert(var->type != VAR_NULL);
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

static MyDialogue_ProgramVariable *myDialogue_combineVariablesUsingOperator(MyDialogue_ProgramStack *stack, MyDialogue_ProgramVariable *var0, MyDialogue_ProgramVariable *var1, EasyTokenType operatorType) {
	assert(var0.type == var1.type);

	
	assert(var0.sizeInBytes == var1.sizeInBytes);
	assert(var0.sizeInBytes != 0);

	//NOTE(ollie): Don't handle array concatination
	assert(var0.arraySize == 1);

	switch(operatorType) {
		case TOKEN_PLUS: {
			switch(var0.type) {
				case VAR_INT: {
					//NOTE(ollie): LOAD 
					int a = myDialogue_castVariableAsInt(stack, var0);
					int b = myDialogue_castVariableAsInt(stack, var1);
					int result = a + b;

					int *ptr = (int *)(stack->stack + var0->offsetInStack); 

					*ptr = result;

				} break;
				case VAR_FLOAT: {
					float a = myDialogue_castVariableAsFloat(stack, var0);
					float b = myDialogue_castVariableAsFloat(stack, var1);
					float result = a + b;

					float *ptr = (float *)(stack->stack + var0->offsetInStack); 

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
		case TOKEN_MINUS: {
			switch(var0.type) {
				case VAR_INT: {
					//NOTE(ollie): LOAD 
					int a = myDialogue_castVariableAsInt(stack, var0);
					int b = myDialogue_castVariableAsInt(stack, var1);
					int result = a - b;

					int *ptr = (int *)(stack->stack + var0->offsetInStack); 

					*ptr = result;

				} break;
				case VAR_FLOAT: {
					float a = myDialogue_castVariableAsFloat(stack, var0);
					float b = myDialogue_castVariableAsFloat(stack, var1);
					float result = a - b;

					float *ptr = (float *)(stack->stack + var0->offsetInStack); 

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
					int a = myDialogue_castVariableAsInt(stack, var0);
					int b = myDialogue_castVariableAsInt(stack, var1);
					int result = a * b;

					int *ptr = (int *)(stack->stack + var0->offsetInStack); 

					*ptr = result;

				} break;
				case VAR_FLOAT: {
					float a = myDialogue_castVariableAsFloat(stack, var0);
					float b = myDialogue_castVariableAsFloat(stack, var1);
					float result = a * b;

					float *ptr = (float *)(stack->stack + var0->offsetInStack); 

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
					int a = myDialogue_castVariableAsInt(stack, var0);
					int b = myDialogue_castVariableAsInt(stack, var1);
					int result = a / b;

					int *ptr = (int *)(stack->stack + var0->offsetInStack); 

					*ptr = result;

				} break;
				case VAR_FLOAT: {
					float a = myDialogue_castVariableAsFloat(stack, var0);
					float b = myDialogue_castVariableAsFloat(stack, var1);
					float result = a / b;

					float *ptr = (float *)(stack->stack + var0->offsetInStack); 

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
		default: {
			assert(!"operator node handled");
		}
	} 

	return var0;
}

typedef enum {
	MY_DIALOGUE_EVALUATE_DEFAULT = 0, //don't evaluate comma. Will cause error if it finds one
	MY_DIALOGUE_EVALUATE_COMMA = 1,
} MyDialogue_EvaluateFlags;

static void myDialogue_evaluateExpression(MyDialogue_Program *program, EasyAst_Node *nodeAt, MyDialogue_EvaluateFlags flags, MyDialogue_ProgramVariable *varToFillOut) {
	bool parsing = true;

	assert(var);
	//NOTE(ollie): Keeping track of number of operators vs data
	int operatorCount = 0;

	EasyTokenType nextOperator = TOKEN_UNINITIALISED;

	bool hadAParenthesis = false;
	while(parsing && program->ast.errorCount == 0) {
		bool addData = false;
		
		switch(nodeAt->type) {
			case EASY_AST_NODE_OPERATOR_COMMA: {
				if(flags & MY_DIALOGUE_EVALUATE_COMMA) {
					parsing = false;
				} else {
					easyAst_addError(program->ast, "There shouldn't be a comma in this expression.", nodeAt->token.lineNumber);
				}
			} break;
			case EASY_AST_NODE_OPERATOR_FUNCTION: {
				myDialogue_pushStackFrame(&program->stack);
				MyDialogue_ProgramVariable *tempVar = myDialogue_addVariableWithoutData(&program->stack, varNode.token.at, varNode.token.size);
				if(nextOperator == TOKEN_UNINITIALISED) {
					if(varToFillOut.type == VAR_NULL || varToFillOut.type == tempVar.type) {
						//do nothing
					} else {
						easyAst_addError(program->ast, "Mismatch types. Function does not return the same type as the expression.", nodeAt->token.lineNumber);
					}
					//TODO(ollie): This doesn't work
					assert(false);
					*varToFillOut = *tempVar
				} else {
					assert(varToFillOut.type != VAR_NULL);
					
					varToFillOut = myDialogue_combineVariablesUsingOperator(&program->stack, tempVar, varToFillOut, nextOperator);
				}
				myDialogue_popStackFrame(&program->stack);				
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
					myDialogue_advanceNode();
				} else {
					easyAst_addError(program->ast, "You've put to many operators in a row", nodeAt->token.lineNumber);
				}
			} break;
			case EASY_AST_NODE_OPERATOR_EVALUATE: {
				if(nodeAt->token.type == TOKEN_OPEN_PARENTHESIS) {
					parenthesisCount++;
				} else if(nodeAt->token.type == TOKEN_CLOSE_PARENTHESIS) {
					parenthesisCount--;
				} else {
					assert(false);
				}
			} break;
			case EASY_AST_NODE_PRIMITIVE: {
				switch (nodeAt->token.type) {
					case TOKEN_INTEGER: {
						if(commaCount == 0) {
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
								//NOTE(ollie): Add the actual data to the stack
								myDialogue_addDataToVariable(&program->stack, var, &nodeAt->token.intVal, sizeof(int), VAR_INT, 1);

								if(isArray) {
									//NOTE(ollie): Make sure the array isn't over specified
									myDialogue_checkArraySize(program, nodeAt, var, arraySize);
									commaCount++;
								}

								myDialogue_advanceNode(program, &nodeAt);
								
							}
						} else {
							easyAst_addError(program->ast, "Did you forget a comma?", nodeAt->token.lineNumber);
						}
					} break;
					case TOKEN_FLOAT: {
						if(commaCount == 0) {
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
								myDialogue_addDataToVariable(&program->stack, var, &nodeAt->token.floatVal, sizeof(float), VAR_FLOAT, 1);
								
								if(isArray) {
									//NOTE(ollie): Make sure the array isn't over specified
									myDialogue_checkArraySize(program, nodeAt, var, arraySize);
									commaCount++;
								}

								myDialogue_advanceNode(program, &nodeAt);

							}
						} else {
							easyAst_addError(program->ast, "Did you forget a comma?", nodeAt->token.lineNumber);
						}
					} break;
					case TOKEN_STRING: {
						if(commaCount == 0) {
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
								myDialogue_addDataToVariable(&program->stack, var, &nodeAt->token.at, sizeof(char)*nodeAt->token.size, VAR_STRING, 1);
								
								if(isArray) {
									//NOTE(ollie): Make sure the array isn't over specified
									myDialogue_checkArraySize(program, nodeAt, var, arraySize);
									commaCount++;
								}

								myDialogue_advanceNode(program, &nodeAt);
							}
						} else {
							easyAst_addError(program->ast, "Did you forget a comma?", nodeAt->token.lineNumber);
						}
					} break;
					
					default: {
						easyAst_addError(program->ast, "Value not supported for variables. Only integers, floats & strings", nodeAt->token.lineNumber);
					}
				}
			} break;
			case EASY_AST_NODE_VARIABLE: {

			} break;
			case EASY_AST_NODE_OPERATOR_END_LINE: {
				parsing = false;
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

	EasyAst_Node *nodeAt = nodeAt->type;

	bool isArray = false;
	u32 sizeOfArray = 0;

	EasyAst_Node *varNode = 0;
	VarType varType  = VAR_NULL;

	//TODO(ollie): In the future we'll want to expand this to set multiple variables at once. 
	MyDialogue_ProgramVariable *var = 0;
	bool varIsSet = false;
	bool equalSign = false;

	int commaCount = 0; 

	bool parsing = true;
	while(parsing && program->ast.errorCount == 0) {
		switch(nodeAt->type) {
			case EASY_AST_NODE_VARIABLE: {
				varIsSet = true;
				//NOTE(ollie): Try find the variable
				if(varIsSet) {
					easyAst_addError(program->ast, "Multiple variable assignments aren't supported.", nodeAt->token.lineNumber);
				} else {
					varNode = nodeAt;
					MyDialogue_ProgramVariable *var = myDialogue_findVariable(&program->stack, nodeAt->token.at, nodeAt->token.size);	
				}
				
				if(nodeAt->next) {

					//NOTE(ollie): Advance the node 
					nodeAt = nodeAt->next;					
				} else {
					easyAst_addError(program->ast, "There is a variable not doing anything.", nodeAt->token.lineNumber);
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

					if(nodeAt->next) {
						//NOTE(ollie): Create a new variable, we haven't actually put any data on the stack, we do it as we find it. 
						//TODO(ollie): This could be error prone & won't work if we want to do multiple variable assignments at once. 
						var = myDialogue_addVariableWithoutData(&program->stack, varNode.token.at, varNode.token.size);
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
			case EASY_AST_NODE_PRIMITIVE: {
				if(equalSign) {
					if(varIsSet) {
						if(commaCount == 0) {
							commaCount++;
							myDialogue_evaluateExpression(program, nodeAt);
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
			case EASY_AST_NODE_VARIABLE: {
				if(equalSign) {
					if(varIsSet) {
						if(commaCount == 0) {
							commaCount++;
							myDialogue_evaluateExpression(program, nodeAt);
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
				if(isArray) {
					commaCount--;
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
				if(commaCount != 0) {
					easyAst_addError(program->ast, "There's a unused comma at the end of this line.", nodeAt->token.lineNumber);
				}
				//Don't advance & end parsing
				parsing = false;
			} break;
			default: {
				easyAst_addError(program->ast, "Unidentified value. Did you make mean something else? Or forget a semicolon at the end?", nodeAt->token.lineNumber);
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
				myDialogue_parseVariableAssignment(program);
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