/*

How to use: 

EasyAst ast = easyAst_generateAst(char *streamNullTerminated);

easyAst_printAst(&ast, char *fileLocationYouWantItToPrintInto)

*/

#define EASY_AST_NODE_TYPE(FUNC) \
FUNC(EASY_AST_NODE_PARENT)\
FUNC(EASY_AST_NODE_OPERATOR_MATH)\
FUNC(EASY_AST_NODE_OPERATOR_IF)\
FUNC(EASY_AST_NODE_OPERATOR_WHILE)\
FUNC(EASY_AST_NODE_OPERATOR_FUNCTION)\
FUNC(EASY_AST_NODE_CONDITIONAL)\
FUNC(EASY_AST_NODE_BRANCH)\
FUNC(EASY_AST_NODE_SCOPE)\
FUNC(EASY_AST_NODE_EVALUATE)\
FUNC(EASY_AST_NODE_PRIMITIVE)\
FUNC(EASY_AST_NODE_VARIABLE)\
FUNC(EASY_AST_NODE_OPERATOR_ASSIGNMENT)\
FUNC(EASY_AST_NODE_OPERATOR_NEW_ASSIGNMENT)\
FUNC(EASY_AST_NODE_OPERATOR_COMMA)\
FUNC(EASY_AST_NODE_OPERATOR_CONDITIONAL)\
FUNC(EASY_AST_NODE_OPERATOR_MAIN)\
FUNC(EASY_AST_NODE_ARRAY_IDENTIFIER)\
FUNC(EASY_AST_NODE_OPERATOR_END_LINE)\

typedef enum {
	EASY_AST_NODE_TYPE(ENUM)
} EasyAst_NodeType;

static char *EasyAst_NodeTypeStrings[] = { EASY_AST_NODE_TYPE(STRING) };

#define EASY_AST_NODE_PRECEDENCE(FUNC) \
FUNC(EASY_AST_NODE_CURRENT_PRECEDENCE)\
FUNC(EASY_AST_NODE_PRECEDENCE_PLUS_MINUS)\
FUNC(EASY_AST_NODE_PRECEDENCE_MULTIPLY_DIVIDE)\
FUNC(EASY_AST_NODE_PRECEDENCE_BRANCH)\

typedef enum {
	EASY_AST_NODE_PRECEDENCE(ENUM)
} EasyAst_NodePrecedence;

static char *EasyAst_NodePrecedenceStrings[] = { EASY_AST_NODE_PRECEDENCE(STRING) };

////////////////////////////////////////////////////////////////////

typedef struct EasyAst_Node EasyAst_Node;
typedef struct EasyAst_Node {
	EasyAst_NodeType type;
	EasyToken token;

	EasyAst_NodePrecedence precedence;

	EasyAst_Node *parent;
	EasyAst_Node *child;

	EasyAst_Node *next;
} EasyAst_Node;

///////////////////////************* Error logging ************////////////////////

typedef struct {
	u32 lineNumber;
	char *reason;
} EasyAst_ErrorMessage;

typedef struct {
	u32 errorCount;
	EasyAst_ErrorMessage errors[128];
	bool overflow; //Check if we overflow our errors

	bool crashCompilation;
} EasyAst_ErrorLogger;


////////////////////////////////////////////////////////////////////

typedef struct {
	EasyAst_ErrorLogger errors;

	EasyAst_Node parentNode;

	EasyAst_Node *currentNode;

	//NOTE(ollie): Arena we're creating things in
	Arena *arena;	

	///////////////////////************ For in use, shouldn't be used outside of this module *************////////////////////

	EasyTokenizer *tokenizer;
} EasyAst;

////////////////////////////////////////////////////////////////////

static void easyAst_addError(EasyAst *ast, char *message, u32 lineNumber) {
	//NOTE(ollie): Get the logger out of the ast
	EasyAst_ErrorLogger *logger = &ast->errors;

	//NOTE(ollie): Check if there is any more room for errors
	if(logger->errorCount < arrayCount(logger->errors)) {
		//NOTE(ollie): Add the error
		EasyAst_ErrorMessage *e = logger->errors + logger->errorCount++;	

		//NOTE(ollie): Assign the error information
		e->reason = message;
		e->lineNumber = lineNumber;
	} else {
		//NOTE(ollie): We overflowed our errors
		logger->overflow = true;
	}
	
}

////////////////////////////////////////////////////////////////////

static void easyAst_initAst(EasyAst *ast, Arena *arena, EasyTokenizer *tokenizer) {

	///////////////////////************ Init the error struct *************////////////////////
	ast->errors.errorCount = 0;
	ast->errors.overflow = false;
	ast->errors.crashCompilation = false;

	///////////////////////************* Init the first ast node ************////////////////////

	ast->parentNode.parent = 0;
	ast->parentNode.next = 0;
	ast->parentNode.child = 0;
	ast->parentNode.type = EASY_AST_NODE_PARENT;
	ast->parentNode.precedence = EASY_AST_NODE_PRECEDENCE_PLUS_MINUS;
	ast->parentNode.token = {};

	//NOTE(ollie): Set current node to the parent node
	ast->currentNode = &ast->parentNode;

	////////////////////////////////////////////////////////////////////

	ast->arena = arena;

	////////////////////////////////////////////////////////////////////
	//NOTE(ollie): Add the tokenizer
	ast->tokenizer = tokenizer;
}

static EasyAst_Node *easyAst_pushNode(EasyAst *ast, EasyAst_NodeType type, EasyToken token, EasyAst_NodePrecedence precedence) {
	EasyAst_Node *currentNode = ast->currentNode;

	///////////////////////************ Create a new node *************////////////////////

	EasyAst_Node *node = pushStruct(ast->arena, EasyAst_Node);

	node->type = type;
	node->token = token;

	//We know that this node is being put on as another branch so there should always be a current node on the same level
	node->precedence = (precedence == EASY_AST_NODE_CURRENT_PRECEDENCE) ? currentNode->precedence : precedence;

	node->next = 0;
	node->child = 0;
	node->parent = 0;

	node->precedence = precedence;

	////////////////////////////////////////////////////////////////////

	assert(currentNode->precedence != EASY_AST_NODE_CURRENT_PRECEDENCE);

	if(currentNode->precedence == precedence || currentNode->precedence > precedence || precedence == EASY_AST_NODE_CURRENT_PRECEDENCE) {
		//NOTE(ollie): Make sure the current node has a parent
		// assert(currentNode->parent || currentNode->type == EASY_AST_NODE_PARENT);

		//NOTE(ollie): Set the parent node to the same as the current one
		node->parent = currentNode->parent;

		assert(precedence != EASY_AST_NODE_PRECEDENCE_BRANCH);
		if(precedence == EASY_AST_NODE_CURRENT_PRECEDENCE) {

			assert(currentNode->precedence != EASY_AST_NODE_PRECEDENCE_BRANCH);

			node->precedence = currentNode->precedence; 
		}

		//NOTE(ollie): Set the new node to follow on from the last one
		currentNode->next = node;

	} else if(currentNode->precedence < precedence || precedence == EASY_AST_NODE_PRECEDENCE_BRANCH) {
		//NOTE(ollie): Precedence is higher so new brach in the tree
		
		//NOTE(ollie): If this node already has a child, move across one node then create the branch
		if(currentNode->child) {
			//NOTE(ollie): Push an empty node 
			EasyToken emptyToken = {};
			currentNode = easyAst_pushNode(ast, EASY_AST_NODE_PARENT, emptyToken, EASY_AST_NODE_CURRENT_PRECEDENCE);
			assert(currentNode = ast->currentNode);
			assert(!currentNode->child);
		}  

		if(precedence == EASY_AST_NODE_PRECEDENCE_BRANCH) {
			//NOTE(ollie): Go back to lowest precedence
			node->precedence = EASY_AST_NODE_PRECEDENCE_PLUS_MINUS;
		}

		assert(!currentNode->child);	
		currentNode->child = node;
		node->parent = currentNode;
	} 

	//NOTE(ollie): Assign the new node to the current node
	ast->currentNode = node;

	return node;
}

static void easyAst_popNode(EasyAst *ast, EasyAst_NodeType type, EasyToken token, EasyAst_NodePrecedence precedence) {
	//NOTE(ollie): Push the node first to finish the scope etc.
	easyAst_pushNode(ast, type, token, precedence);

	if(!ast->currentNode->parent) {
		easyAst_addError(ast, "Brackets do not match. Are you missing a start or end bracket somewhere?", ast->tokenizer->lineNumber);
		ast->errors.crashCompilation = true;
	} else {
		ast->currentNode = ast->currentNode->parent; 	
	}
	
}

static void easyAst_printAst(EasyAst *ast) {

	InfiniteAlloc fileContents = initInfinteAlloc(char);

	////////////////////////////////////////////////////////////////////

	for(int errorIndex = 0; errorIndex < ast->errors.errorCount; ++errorIndex) {
		char *title = "Errors:\n\n";

		addElementInifinteAllocWithCount_(&fileContents, title, strlen(title));

		EasyAst_ErrorMessage *message = &ast->errors.errors[errorIndex];

		char *formatString = "error message: %s on line number: %d\n";
		int stringLengthToAlloc = snprintf(0, 0, formatString, message->reason, message->lineNumber) + 1; //for null terminator, just to be sure
		
		char *strArray = pushArray(&globalPerFrameArena, stringLengthToAlloc, char);

		snprintf(strArray, stringLengthToAlloc, formatString, message->reason, message->lineNumber); //for null terminator, just to be sure

		addElementInifinteAllocWithCount_(&fileContents, strArray, stringLengthToAlloc);
	}
	
	EasyAst_Node *nodeAt = &ast->parentNode;
	assert(nodeAt);

	int level = 0;

	char *title = "Nodes:\n\n";

	addElementInifinteAllocWithCount_(&fileContents, title, strlen(title));
	bool finished = false;

	while(nodeAt) {

		char *formatString = "Level: %d, Node Type: %s, Token Type: %s, Precedence: %s, Line Number: %d\n\n";
		char *a = EasyAst_NodeTypeStrings[(int)nodeAt->type];
		char *b = LexTokenTypeStrings[(int)nodeAt->token.type];
		char *c = EasyAst_NodePrecedenceStrings[(int)nodeAt->precedence];

		u32 d = nodeAt->token.lineNumber;

		char bogus[4];
		int stringLengthToAlloc = snprintf(bogus, 1, formatString, level, a, b, c, d) + 1; //for null terminator, just to be sure
		
		char *strArray = pushArray(&globalPerFrameArena, stringLengthToAlloc, char);

		snprintf(strArray, stringLengthToAlloc, formatString, level, a, b, c, d); //for null terminator, just to be sure

		addElementInifinteAllocWithCount_(&fileContents, strArray, stringLengthToAlloc - 1); //- 1 so it doesn't put the null terminator in

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

	}
	assert(finished);

	///////////////////////************ Write the file to disk *************////////////////////

	char *writeName = concat(globalExeBasePath, "test_ast.txt");
	
	game_file_handle handle = platformBeginFileWrite(writeName);
	platformWriteFile(&handle, fileContents.memory, fileContents.count*fileContents.sizeOfMember, 0);
	platformEndFile(handle);

	///////////////////////************* Clean up the memory ************////////////////////	

	releaseInfiniteAlloc(&fileContents);

	free(writeName);
}



static EasyAst easyAst_generateAst(char *streamNullTerminated) {

	bool parsing = true;
    EasyTokenizer tokenizer = lexBeginParsing((char *)streamNullTerminated, EASY_LEX_OPTION_EAT_WHITE_SPACE);


	EasyAst ast;
	easyAst_initAst(&ast, &globalLongTermArena, &tokenizer);

    while(parsing && !ast.errors.crashCompilation) {
        EasyToken token = lexGetNextToken(&tokenizer);
        switch(token.type) {
        	case TOKEN_NULL_TERMINATOR: {
        		parsing = false;
        	} break;
        	case TOKEN_ASTRIX: {
        		easyAst_pushNode(&ast, EASY_AST_NODE_OPERATOR_MATH, token, EASY_AST_NODE_PRECEDENCE_MULTIPLY_DIVIDE);
        	} break;
        	case TOKEN_FORWARD_SLASH: {
        		easyAst_pushNode(&ast, EASY_AST_NODE_OPERATOR_MATH, token, EASY_AST_NODE_PRECEDENCE_MULTIPLY_DIVIDE);
        	} break;
        	case TOKEN_COMMA: {
        		easyAst_pushNode(&ast, EASY_AST_NODE_OPERATOR_COMMA, token, EASY_AST_NODE_CURRENT_PRECEDENCE);
        	} break;
        	case TOKEN_COLON: {
        		EasyToken nxtToken = lexSeeNextToken(&tokenizer);
        		if(nxtToken.type == TOKEN_EQUALS) {
        			//NOTE(ollie): Advance token past equals sign
        			lexGetNextToken(&tokenizer);

        			//NOTE(ollie): Push node for assignment operator
        			easyAst_pushNode(&ast, EASY_AST_NODE_OPERATOR_NEW_ASSIGNMENT, token, EASY_AST_NODE_CURRENT_PRECEDENCE);
        		} else {
        			easyAst_addError(&ast, "colon is only valid with an equals sign following it", tokenizer.lineNumber);
        		}

        	} break;
        	case TOKEN_PERIOD: {

        	} break;
        	case TOKEN_EQUALS: {
        		EasyToken nxtToken = lexSeeNextToken(&tokenizer);
        		if(nxtToken.type == TOKEN_EQUALS) {
        			//NOTE(ollie): Eat the equal token
        			nxtToken = lexGetNextToken(&tokenizer);

        			easyAst_pushNode(&ast, EASY_AST_NODE_OPERATOR_CONDITIONAL, token, EASY_AST_NODE_CURRENT_PRECEDENCE);
        		} else {
        			easyAst_pushNode(&ast, EASY_AST_NODE_OPERATOR_ASSIGNMENT, token, EASY_AST_NODE_CURRENT_PRECEDENCE);	
        		}
        		
        	} break;
        	case TOKEN_PLUS: {
        		easyAst_pushNode(&ast, EASY_AST_NODE_OPERATOR_MATH, token, EASY_AST_NODE_PRECEDENCE_PLUS_MINUS);
        	} break;
        	case TOKEN_MINUS: {
        		easyAst_pushNode(&ast, EASY_AST_NODE_OPERATOR_MATH, token, EASY_AST_NODE_PRECEDENCE_PLUS_MINUS);
        	} break;
        	case TOKEN_SEMI_COLON: {
        		easyAst_pushNode(&ast, EASY_AST_NODE_OPERATOR_END_LINE, token, EASY_AST_NODE_CURRENT_PRECEDENCE);
        	} break;
        	case TOKEN_STRING: {
        		easyAst_pushNode(&ast, EASY_AST_NODE_PRIMITIVE, token, EASY_AST_NODE_CURRENT_PRECEDENCE);
        	} break;
        	case TOKEN_OPEN_PARENTHESIS: {
        		easyAst_pushNode(&ast, EASY_AST_NODE_EVALUATE, token, EASY_AST_NODE_PRECEDENCE_BRANCH);
        	} break;
        	case TOKEN_CLOSE_PARENTHESIS: {
        		easyAst_popNode(&ast, EASY_AST_NODE_EVALUATE, token, EASY_AST_NODE_CURRENT_PRECEDENCE);
        	} break;
        	case TOKEN_OPEN_BRACKET: {
        		easyAst_pushNode(&ast, EASY_AST_NODE_SCOPE, token, EASY_AST_NODE_PRECEDENCE_BRANCH);
        	} break;
        	case TOKEN_CLOSE_BRACKET: {
        		easyAst_popNode(&ast, EASY_AST_NODE_SCOPE, token, EASY_AST_NODE_CURRENT_PRECEDENCE);
        	} break;
        	case TOKEN_OPEN_SQUARE_BRACKET: {
        		easyAst_pushNode(&ast, EASY_AST_NODE_ARRAY_IDENTIFIER, token, EASY_AST_NODE_CURRENT_PRECEDENCE);
        	} break;
        	case TOKEN_CLOSE_SQUARE_BRACKET: {
        		easyAst_pushNode(&ast, EASY_AST_NODE_ARRAY_IDENTIFIER, token, EASY_AST_NODE_CURRENT_PRECEDENCE);
        	} break;
        	case TOKEN_WORD: {
        		///////////////////////************ Language Keywords *************////////////////////
        		if(stringsMatchNullN("main", token.at, token.size)) {
        			easyAst_pushNode(&ast, EASY_AST_NODE_OPERATOR_MAIN, token, EASY_AST_NODE_CURRENT_PRECEDENCE);	
        		} else if(stringsMatchNullN("if", token.at, token.size)) {
        			easyAst_pushNode(&ast, EASY_AST_NODE_OPERATOR_IF, token, EASY_AST_NODE_CURRENT_PRECEDENCE);	
        		} else {
        			EasyToken nxtToken = lexSeeNextToken(&tokenizer);
        			if(nxtToken.type == TOKEN_OPEN_PARENTHESIS) {
        				//NOTE(ollie): Is a function 
        				easyAst_pushNode(&ast, EASY_AST_NODE_OPERATOR_FUNCTION, token, EASY_AST_NODE_CURRENT_PRECEDENCE);	
        			} else {
        				//NOTE(ollie): Is a variable
        				easyAst_pushNode(&ast, EASY_AST_NODE_VARIABLE, token, EASY_AST_NODE_CURRENT_PRECEDENCE);	
        			}
        		}
        	} break;
        	case TOKEN_INTEGER: {
        		easyAst_pushNode(&ast, EASY_AST_NODE_PRIMITIVE, token, EASY_AST_NODE_CURRENT_PRECEDENCE);
        	} break;
        	case TOKEN_FLOAT: {
        		easyAst_pushNode(&ast, EASY_AST_NODE_PRIMITIVE, token, EASY_AST_NODE_CURRENT_PRECEDENCE);
        	} break;
        	default: {

        	}
        }
    }

    return ast;
}