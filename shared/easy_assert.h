#define invalidCodePathStr(msg) { printf(msg); exit(0); }
#if DEVELOPER_MODE //turn off for crash assert
// #define assert(statement) if(!(statement)) { int *a = 0; a = 0;}
#define assert(statement) if(!(statement)) {printf("Something went wrong at %d in %s\n", __LINE__, __FILE__);  int *a = 0; *a = 0;}
#define assertStr(statement, str) if(!(statement)) { printf("%s\n", str); } assert(statement); 
#else
#define assert(statement) 
#define assertStr(statement, str)
#endif
