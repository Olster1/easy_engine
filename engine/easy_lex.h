#define PRINT_UNKNOWN_CHARACTERS 0
char *lexNullTerminateBuffer(char *result, char *string, int length) {
    for(int i = 0; i < length; ++i) {
        result[i]= string[i];
    }
    result[length] = '\0';
    return result;
}

bool lexIsNumeric(char charValue) {
    bool result = (charValue >= '0' && charValue <= '9');
    return result;
}

bool lexIsAlphaNumeric(char charValue) {
    bool result = ((charValue >= 'A' && charValue <= 'Z') || (charValue >= 'a' && charValue <= 'z') || charValue == '_');
    return result;
}

bool lexMatchString(char *A, char *B) {
    bool res = true;
    while(*A && *B) {
        res = (*A++ == *B++);
        if(!res) break;
    } 
    return res;
}

bool lexMatchStringLength(char *A, char *B, int length) {
    char nullTerminateB[256];
    for(int i = 0; i < length; ++i) {
        nullTerminateB[i] = B[i];
    }
    nullTerminateB[length] = '\0';
    bool res = lexMatchString(A, nullTerminateB);
    return res;
}

#define EASY_LEX_TOKEN_TYPE(FUNC) \
FUNC(TOKEN_UNINITIALISED) \
FUNC(TOKEN_WORD) \
FUNC(TOKEN_BOOL) \
FUNC(TOKEN_STRING) \
FUNC(TOKEN_INTEGER) \
FUNC(TOKEN_FLOAT) \
FUNC(TOKEN_UINT_TYPE) \
FUNC(TOKEN_INT_TYPE) \
FUNC(TOKEN_FLOAT_TYPE) \
FUNC(TOKEN_CHAR_TYPE) \
FUNC(TOKEN_STRING_TYPE) \
FUNC(TOKEN_BOOL_TYPE) \
FUNC(TOKEN_IF_KEYWORD) \
FUNC(TOKEN_WHILE_KEYWORD) \
FUNC(TOKEN_FOR_KEYWORD) \
FUNC(TOKEN_RETURN_KEYWORD) \
FUNC(TOKEN_BREAK_KEYWORD) \
FUNC(TOKEN_STRUCT_KEYWORD) \
FUNC(TOKEN_SEMI_COLON) \
FUNC(TOKEN_COLON) \
FUNC(TOKEN_OPEN_BRACKET) \
FUNC(TOKEN_CLOSE_BRACKET) \
FUNC(TOKEN_NULL_TERMINATOR) \
FUNC(TOKEN_OPEN_PARENTHESIS) \
FUNC(TOKEN_NEWLINE) \
FUNC(TOKEN_CLOSE_PARENTHESIS) \
FUNC(TOKEN_FORWARD_SLASH) \
FUNC(TOKEN_AT_SYMBOL) \
FUNC(TOKEN_ASTRIX) \
FUNC(TOKEN_COMMENT) \
FUNC(TOKEN_COMMA) \
FUNC(TOKEN_HASH) \
FUNC(TOKEN_PERIOD) \
FUNC(TOKEN_VALUE_WITH_UNIT) \
FUNC(TOKEN_HASH_NUMBER) \
FUNC(TOKEN_OPEN_SQUARE_BRACKET) \
FUNC(TOKEN_CLOSE_SQUARE_BRACKET) \
FUNC(TOKEN_TAB) \
FUNC(TOKEN_EQUALS) \
FUNC(TOKEN_PLUS) \
FUNC(TOKEN_MINUS) \
FUNC(TOKEN_DOUBLE_EQUAL)\


typedef enum {
    EASY_LEX_TOKEN_TYPE(ENUM)
} EasyTokenType;

static char *LexTokenTypeStrings[] = { EASY_LEX_TOKEN_TYPE(STRING) };

typedef struct {
    char *at;
    
    EasyTokenType type;
    int size;
    
    u32 lineNumber;
    
    union {
        struct {
            int intVal;
        };
        struct {
            float floatVal;
        };
    };
} EasyToken;

char *lexEatWhiteSpace(char *at) {
    while(*at == ' ' || *at == '\r' || *at == '\n' || *at == '\t') {
        at++;		
    }
    return at;
}

char *lexEatWhiteSpaceExceptNewLine(char *at) {
    while(*at == ' ' || *at == '\t') {
        at++;		
    }
    return at;
}

char *lexEatSpaces(char *at) {
    while(*at == ' ') {
        at++;		
    }
    return at;	
}

bool lexIsNewLine(char value) {
    return (value == '\n' || value == '\r');
}

EasyToken lexInitToken(EasyTokenType type, char *at, int size, u32 lineNumber) {
    EasyToken result = {};
    result.type = type;
    result.at = at;
    result.size = size;
    result.lineNumber = lineNumber;
    
    return result;
}

typedef struct {
    char *src;
    int lineNumber;
    bool parsing;
    bool eatWhiteSpace;
    bool parseComments;
} EasyTokenizer;

typedef enum {
    EASY_LEX_OPTION_EAT_WHITE_SPACE = 1 << 0,
    EASY_LEX_DONT_EAT_SLASH_COMMENTS = 1 << 1,
} EasyLexOptions;

EasyTokenizer lexBeginParsing(void *src, EasyLexOptions options) {
    EasyTokenizer result = {};
    result.src = (char *)src;
    result.eatWhiteSpace = options & EASY_LEX_OPTION_EAT_WHITE_SPACE;
    result.parseComments = !(options & EASY_LEX_DONT_EAT_SLASH_COMMENTS);
    result.parsing = true;
    return result;
}

bool lexInnerAlphaNumericCharacter(char value) {
    return (value == '-' || value == '_');
}

int lexStringLength(char *str) {
    int res = 0;
    while(*str) {
        str++;
        res++;
    }
    return res;
}

void lexAdvancePtrWithToken(EasyTokenizer *tokenizer, EasyToken token) {
    tokenizer->src = (token.at + token.size);
    if(token.type == TOKEN_STRING) { tokenizer->src++; } //to move past the last quote 
}

void lexPrintToken(EasyToken *token) {
    char buffer[256] = {};
    char *a = lexNullTerminateBuffer(buffer, token->at, token->size);
    
    printf("%s\n%s\n---\n", a, LexTokenTypeStrings[token->type]);
}

EasyToken lexGetToken_(EasyTokenizer *tokenizer, bool advanceWithToken) {
    char *at = tokenizer->src;
    int *lineNumber = &tokenizer->lineNumber;
    EasyToken token = lexInitToken(TOKEN_UNINITIALISED, at, 1, *lineNumber);
    if(tokenizer->eatWhiteSpace) { at = lexEatWhiteSpace(at); } else { at = lexEatSpaces(at); }
    
    switch(*at) {
        case ';': {
            token = lexInitToken(TOKEN_SEMI_COLON, at, 1, *lineNumber);
            at++;
        } break;
        case ':': {
            token = lexInitToken(TOKEN_COLON, at, 1, *lineNumber);
            at++;
        } break;
        case '.': {
            token = lexInitToken(TOKEN_PERIOD, at, 1, *lineNumber);
            at++;
        } break;
        case '\0': {
            token = lexInitToken(TOKEN_NULL_TERMINATOR, at, 1, *lineNumber);
            at++;
        } break;
        case ',': {
            token = lexInitToken(TOKEN_COMMA, at, 1, *lineNumber);
            at++;
        } break;
        case '\r': 
        case '\n': {
            token = lexInitToken(TOKEN_NEWLINE, at, 1, *lineNumber);
            at++;
        } break;
        case '\t': {
            token = lexInitToken(TOKEN_TAB, at, 1, *lineNumber);
            at++;
        } break;
        case '{': {
            token = lexInitToken(TOKEN_OPEN_BRACKET, at, 1, *lineNumber);
            at++;
        } break;
        case '}': {
            token = lexInitToken(TOKEN_CLOSE_BRACKET, at, 1, *lineNumber);
            at++;
        } break;
        case '[': {
            token = lexInitToken(TOKEN_OPEN_SQUARE_BRACKET, at, 1, *lineNumber);
            at++;
        } break;
        case ']': {
            token = lexInitToken(TOKEN_CLOSE_SQUARE_BRACKET, at, 1, *lineNumber);
            at++;
        } break;
        case '(': {
            token = lexInitToken(TOKEN_OPEN_PARENTHESIS, at, 1, *lineNumber);
            at++;
        } break;
        case ')': {
            token = lexInitToken(TOKEN_CLOSE_PARENTHESIS, at, 1, *lineNumber);
            at++;
        } break;
        case '@': {
            token = lexInitToken(TOKEN_AT_SYMBOL, at, 1, *lineNumber);
            at++;
        } break;
        case '=': {
            token = lexInitToken(TOKEN_EQUALS, at, 1, *lineNumber);
            at++;
        } break;
        case '*': {
            token = lexInitToken(TOKEN_ASTRIX, at, 1, *lineNumber);
            at++;
        } break;
        case '+': {
            token = lexInitToken(TOKEN_PLUS, at, 1, *lineNumber);
            at++;
        } break;
        case '#': {
            token = lexInitToken(TOKEN_HASH, at, 1, *lineNumber);
            at++;
            while(*at && (lexIsNumeric(*at) || lexIsAlphaNumeric(*at))) {
                token.type = TOKEN_HASH_NUMBER;
                at++;
            }
            token.size = (at - token.at);
        } break;
        case '\'': 
        case '\"': {
            token = lexInitToken(TOKEN_STRING, at, 1, *lineNumber);
            char endOfString = (*at == '\"') ? '\"' : '\'';
            at++;
            while(*at && *at != endOfString) {
                if(lexIsNewLine(*at)) {
                    //NOTE(ollie): Advance the line number
                    *lineNumber = *lineNumber + 1;
                }
                at++;
            }
            if(*at == endOfString) at++;
            token.size = (at - token.at);//quotation are kept with the value
        } break;
        case '/': {
            token = lexInitToken(TOKEN_FORWARD_SLASH, at, 1, *lineNumber);
            if(tokenizer->parseComments) {
                if(lexMatchString(at, "//")) {
                    token.type = TOKEN_COMMENT;
                    at += 2;
                    while(*at && !lexIsNewLine(*at)) {
                        at++;
                    }
                } else if(lexMatchString(at, "/*")) {
                    token.type = TOKEN_COMMENT;
                    at += 2;
                    
                    while(*at && !lexMatchString(at, "*/")) {
                        if(lexIsNewLine(*at)) {
                            *lineNumber = *lineNumber + 1;
                        }
                        at++;
                    }
                    if(*at) at += 2;
                }
                token.size = at - token.at;
            } else {
                at++;
            }
        } break;
        default: {
            token.at = at;
            if(lexIsAlphaNumeric(*at)) {
                token = lexInitToken(TOKEN_WORD, at, 1, *lineNumber);
                at++;
                bool hadDot = false;
                while(*at && (lexIsAlphaNumeric(*at) || lexIsNumeric(*at) || lexInnerAlphaNumericCharacter(*at) || (*at == '.' && !hadDot))) {
                    if(*at == '.') {
                        hadDot = true;
                    }
                    at++;
                }
                token.size = at - token.at;
                
            } else if(lexIsNumeric(*at) || *at == '-') {
                token = lexInitToken(TOKEN_INTEGER, at, 1, *lineNumber);
                int numberOfDecimal = 0;
                bool hadENotation = false;
                at++; //move past the first number
                while(!hadENotation && *at && (lexIsNumeric(*at) || *at == '.' || *at == 'E' || *at == 'e')) {
                    if(*at == '.') {
                        numberOfDecimal++;
                        if(numberOfDecimal > 1) {
                            printf("found more than one colon in number at lineNumber: %d", *lineNumber);
                            assert(false);
                            break;
                        }
                    }
                    
                    if(*at == 'E' || *at == 'e') {
                        assert(!hadENotation);
                        token.type = TOKEN_FLOAT;
                        char *a = nullTerminateArena(token.at, (at - token.at), &globalPerFrameArena);
                        token.floatVal = atof(a);
                        
                        char *beginExponent = ++at;
                        int exponentSize = 0;
                        float isNegative = 1;
                        
                        if(*(at) == '-') {
                            isNegative = -1;
                            at++;
                            exponentSize++;
                        }
                        
                        
                        while(*at && lexIsNumeric(*at)) {
                            exponentSize++;
                            at++;
                        }
                        
                        char *exponentStr = nullTerminateArena(beginExponent, exponentSize, &globalPerFrameArena);
                        int exponent = atoi(exponentStr);
                        
                        token.floatVal = token.floatVal*powf(10, exponent);
                        
                        hadENotation = true;
                    } else {
                        at++;
                    }
                    
                    
                }
                
                if(!hadENotation) {
                    char *a = nullTerminateArena(token.at, (at - token.at), &globalPerFrameArena);
                    if(numberOfDecimal > 0) {
                        token.type = TOKEN_FLOAT;
                        token.floatVal = atof(a);
                    } else {
                        token.intVal = atoi(a);
                    }
                }
                
                token.size = at - token.at;
            } else {
#if PRINT_UNKNOWN_CHARACTERS
                printf("character %.*s not known\n", 1, at);
#endif
                at++;
            }
        }
    }
    
    //lexPrintToken(&token);
    //assert(token.type != TOKEN_UNINITIALISED);
    
    assert(tokenizer->src != at); // this doesn't 
    assert(token.at);
    if(advanceWithToken) { tokenizer->src = at; }
    
    if(token.type == TOKEN_STRING) {
        token.size -= 2;
        token.at += 1;
    }
    return token;
}

#define lexGetNextToken(tokenizer) lexGetToken_(tokenizer, true)
#define lexSeeNextToken(tokenizer) lexGetToken_(tokenizer, false)


