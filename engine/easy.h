#ifdef __APPLE__ 
#define MemoryBarrier()  __sync_synchronize()
#define _ReadWriteBarrier() { SDL_CompilerBarrier(); }
#include <dirent.h>
#elif _WIN32

#endif

#if DEVELOPER_MODE
#define calloc(size, item) malloc(size)
#endif

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

float max(float a, float b) {
    float result = (a < b) ? b : a;
    return result;
}

float min(float a, float b) {
    float result = (a > b) ? b : a;
    return result;
}




#ifdef _WIN32
//printf doens't print to the console annoyingly!!
#define printf(...) {char str[256]; sprintf_s(str, __VA_ARGS__); OutputDebugString(str); }
#endif

#include <limits.h>

#define fori(Array) for(u32 Index = 0; Index < arrayCount(Array); ++Index)
#define fori_count(Count) for(u32 Index = 0; Index < Count; ++Index)

#define forN_(Count, Name) for(u32 Name = 0; Name < Count; Name++)
#define forN_s(Count, Name) for(s32 Name = 0; Name < Count; Name++)

#define forN(Count) forN_(Count, Count##Index)
#define forNs(Count) forN_s(Count, Count##Index)

#define PI32 3.14159265359
#define TAU32 6.283185307
#define HALF_PI32 0.5f*PI32
#define COLOR_NULL v4(0, 0, 0, 0)
#define COLOR_BLACK v4(0, 0, 0, 1)
#define COLOR_WHITE v4(1, 1, 1, 1)
#define COLOR_RED v4(1, 0, 0, 1)
#define COLOR_GREEN v4(0, 1, 0, 1)
#define COLOR_BLUE v4(0, 0, 1, 1)
#define COLOR_YELLOW v4(1, 1, 0, 1)
#define COLOR_PINK v4(1, 0, 1, 1)
#define COLOR_AQUA v4(0, 1, 1, 1)
#define COLOR_GREY v4(0.5f, 0.5f, 0.5f, 1)
#define COLOR_LIGHT_GREY v4(0.8f, 0.8f, 0.8f, 1)
#define COLOR_GOLD v4(1.0f, 0.8f, 0.0f, 1)

#define Kilobytes(size) (size*1024)
#define Megabytes(size) (Kilobytes(size)*1024) 
#define Gigabytes(size) (Megabytes(size)*1024) 

#define zeroStruct(memory, type) zeroSize(memory, sizeof(type))
#define zeroArray(array) zeroSize(array, sizeof(array))


typedef struct MemoryPiece MemoryPiece;
typedef struct MemoryPiece {
    void *memory;
    size_t totalSize; //size of just this memory block
    // size_t totalSizeOfArena; //size of total arena to roll back with
    size_t currentSize;

    MemoryPiece *next;

} MemoryPiece; //this is for the memory to remember 

typedef struct {
    //NOTE: everything is in pieces now
    // void *memory;
    // unsigned int totalSize; //include all memory blocks
    // unsigned int totalCurrentSize;//total current size of all memory blocks
    int markCount;

    MemoryPiece *pieces; //actual details in the memory block
    MemoryPiece *piecesFreeList;

} Arena;

#define pushStruct(arena, type) (type *)pushSize(arena, sizeof(type))

#define pushArray(arena, size, type) (type *)pushSize(arena, sizeof(type)*size)

void *pushSize(Arena *arena, size_t size) {
    if(!arena->pieces || ((arena->pieces->currentSize + size) > arena->pieces->totalSize)){ //doesn't fit in arena
        MemoryPiece *piece = arena->piecesFreeList; //get one of the free list

        size_t extension = max(Kilobytes(1028), size);
        if(piece)  {
            MemoryPiece **piecePtr = &arena->piecesFreeList;
            assert(piece->totalSize > 0);
            while(piece && piece->totalSize < extension) {//find the right size piece. 
                piecePtr = &piece->next; 
                piece = piece->next;
            }
            if(piece) {
                //take off list
                *piecePtr = piece->next;             
                piece->currentSize = 0;
            }
            
        } 

        if(!piece) {//need to allocate a new piece
            piece = (MemoryPiece *)calloc(sizeof(MemoryPiece), 1);
            piece->memory = calloc(extension, 1);
            piece->totalSize = extension;
            piece->currentSize = 0;
        }
        assert(piece);
        assert(piece->memory);
        assert(piece->totalSize > 0);
        assert(piece->currentSize == 0);

        //stick on list
        piece->next = arena->pieces;
        arena->pieces = piece;

        // piece->totalSizeOfArena = arena->totalSize;
        // assert((arena->currentSize_ + size) <= arena->totalSize); 
    }

    MemoryPiece *piece = arena->pieces;

    assert(piece);
    assert((piece->currentSize + size) <= piece->totalSize); 
    
    void *result = ((u8 *)piece->memory) + piece->currentSize;
    piece->currentSize += size;
    
    zeroSize(result, size);
    return result;
}

Arena createArena(size_t size) {
    Arena result = {};
    pushSize(&result, size);
    assert(result.pieces);
    assert(result.pieces->memory);
    return result;
}

typedef struct { 
    int id;
    Arena *arena;
    size_t memAt; //the actuall value we roll back, don't need to do anything else
    MemoryPiece *piece;
} MemoryArenaMark;

MemoryArenaMark takeMemoryMark(Arena *arena) {
    MemoryArenaMark result = {};
    result.arena = arena;
    result.memAt = arena->pieces->currentSize;
    result.id = arena->markCount++;
    result.piece = arena->pieces;
    return result;
}

void releaseMemoryMark(MemoryArenaMark *mark) {
    mark->arena->markCount--;
    Arena *arena = mark->arena;
    assert(mark->id == arena->markCount);
    assert(arena->markCount >= 0);
    assert(arena->pieces);
    //all ways the top piece is the current memory block for the arena. 
    MemoryPiece *piece = arena->pieces;
    if(mark->piece != piece) {
        //not on the same memory block
        bool found = false;
        while(!found) {
            piece = arena->pieces;
            if(piece == mark->piece) {
                //found the right one
                found = true;
                break;
            } else {
                arena->pieces = piece->next;
                assert(arena->pieces);
                //put on free list
                piece->next = arena->piecesFreeList;
                arena->piecesFreeList = piece;
            }
        }
        assert(found);
    } 
    assert(arena->pieces == mark->piece);
    //roll back size
    piece->currentSize = mark->memAt;
    assert(piece->currentSize <= piece->totalSize);
}

static Arena globalLongTermArena;
static Arena globalPerFrameArena;
static MemoryArenaMark perFrameArenaMark;


char *nullTerminateBuffer(char *result, char *string, int length) {
    for(int i = 0; i < length; ++i) {
        result[i]= string[i];
    }
    result[length] = '\0';
    return result;
}

#define nullTerminate(string, length) nullTerminateBuffer((char *)malloc(length + 1), string, length)
#define nullTerminateArena(string, length, arena) nullTerminateBuffer((char *)pushArray(arena, length + 1, char), string, length)

#define concat_withLength(a, aLength, b, bLength) concat_(a, aLength, b, bLength, 0)
#define concat(a, b) concat_(a, strlen(a), b, strlen(b), 0)
#define concatInArena(a, b, arena) concat_(a, strlen(a), b, strlen(b), arena)
char *concat_(char *a, s32 lengthA, char *b, s32 lengthB, Arena *arena) {
    int aLen = lengthA;
    int bLen = lengthB;
    
    int newStrLen = aLen + bLen + 1; // +1 for null terminator
    char *newString = 0;
    if(arena) {
        newString = (char *)pushArray(arena, newStrLen, char);
    } else {
        newString = (char *)calloc(newStrLen, 1); 
    }
    assert(newString);
    
    newString[newStrLen - 1] = '\0';
    
    char *at = newString;
    for (int i = 0; i < aLen; ++i)
    {
        *at++ = a[i];
    }
    
    for (int i = 0; i < bLen; ++i)
    {
        *at++ = b[i];
    }
    assert(at == &newString[newStrLen - 1])
    assert(newString[newStrLen - 1 ] == '\0');
    
    return newString;
}

//NOTE(ollie): I don't think this handles unicode?
static char *easyString_copyToHeap(char *at, s32 length) {
    //NOTE(ollie): Get memory from heap
    char *result = (char *)easyPlatform_allocateMemory(sizeof(char)*(length + 1), EASY_PLATFORM_MEMORY_NONE);
    //NOTE(ollie): Copy the string
    easyPlatform_copyMemory(result, at, sizeof(char)*length);
    //NOTE(ollie): Null terminate the string
    result[length] = '\0'; //Null terminate

    return result;
}

//TODO(ollie): Don't think this handles unicode?
static char *easyString_copyToBuffer(char *at, char *buffer, u32 bufferLen) {
    
    assert(strlen(at) < bufferLen); //NOTE(ollie): Accounting for the null terminator 
    //NOTE(ollie): Copy the string
    easyPlatform_copyMemory(buffer, at, sizeof(char)*bufferLen);
    //NOTE(ollie): Null terminate the string
    buffer[bufferLen - 1] = '\0'; //Null terminate

    return buffer;
}

char *easyString_copyToArena(char *a, Arena *arena) {
    s32 newStrLen = strlen(a) + 1;
    
    char *newString = (char *)pushArray(arena, newStrLen, char);

    assert(newString);
    
    newString[newStrLen - 1] = '\0';
    
    char *at = newString;
    for (int i = 0; i < (newStrLen - 1); ++i)
    {
        *at++ = a[i];
    }
    assert(newString[newStrLen - 1 ] == '\0');

    return newString;
}

int findEnumValue(char *name, char **names, int nameCount) {
    int result = -1; //not found
    for(int i = 0; i < nameCount; i++) {
        if(cmpStrNull(name, names[i])) {
            result = i;
            break;
        }
    }
    return result;
}

typedef enum {
    BUTTON_NULL,
    BUTTON_LEFT,
    BUTTON_RIGHT,
    BUTTON_BOARD_LEFT,
    BUTTON_BOARD_RIGHT,
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_SPACE,
    BUTTON_SHIFT,
    BUTTON_ENTER,
    BUTTON_BACKSPACE,
    BUTTON_ESCAPE,
    BUTTON_LEFT_MOUSE,
    BUTTON_RIGHT_MOUSE,
    BUTTON_1,
    BUTTON_2,
    BUTTON_3,
    BUTTON_4,
    BUTTON_5,
    BUTTON_6,
    BUTTON_7,
    BUTTON_8,
    BUTTON_9,
    BUTTON_F1,
    BUTTON_F2,
    BUTTON_F3,
    BUTTON_F4,
    BUTTON_F5,
    BUTTON_F6,
    BUTTON_F7,
    BUTTON_Z,
    BUTTON_R,
    BUTTON_COMMAND,
    BUTTON_TILDE,
    //
    BUTTON_COUNT
} ButtonType;


typedef struct {
    bool isDown;
    int transitionCount;
} GameButton;

#define INPUT_BUFFER_SIZE 1028
typedef struct {
    char chars[INPUT_BUFFER_SIZE];
    unsigned int length;
    int cursorAt;
} InputBuffer;



#define wasPressed(buttonArray, index) (buttonArray[index].isDown && buttonArray[index].transitionCount != 0)  

#define wasReleased(buttonArray, index) (!buttonArray[index].isDown && buttonArray[index].transitionCount != 0)  

#define isDown(buttonArray, index) (buttonArray[index].isDown)  

void sdlProcessGameKey(GameButton *button, bool isDown, bool repeated) {
    button->isDown = isDown;
    if(!repeated) {
        button->transitionCount++;
    }
}

void splice(InputBuffer *buffer, char *string, bool addString) { //if false will remove string
    char tempChars[INPUT_BUFFER_SIZE] = {};
    int tempCharCount = 0;
    
    char *at = string;
    
    //copy characters
    for(int i = buffer->cursorAt; i < buffer->length; ++i) {
        tempChars[tempCharCount++] = buffer->chars[i]; 
    }
    
    while(*at) {
        
        if(addString) { //adding
            
            if(buffer->length < (sizeof(buffer->chars) - 1)) {
                
                buffer->chars[buffer->cursorAt++] = *at;
                buffer->length++;
            }
            
        } else {
            if(buffer->length) {
                buffer->chars[buffer->cursorAt--] = *at;
                buffer->length--;
            }
        }
        
        at++;
    }
    //replace characters
    for(int i = 0; i < tempCharCount; ++i) {
        buffer->chars[buffer->cursorAt + i] = tempChars[i]; 
    }
    assert(buffer->length < arrayCount(buffer->chars));
    buffer->chars[buffer->length] = '\0'; //null terminate buffer
}

//TODO: Make this more robust TODO: I don't think this is neccessary??
char *getResPathFromExePath(char *exePath, char *folderName) {
    assert(!"invalid path");
    // unsigned int execPathLength = strlen(exePath) + 1; //for the null terminate character
    
    // char *at = exePath;
    // char *mostRecent = at;
    // while(*at) {
    //     if(*at == '/' && at[1]) { //don't collect last slash
    //         mostRecent = at;
    //     }
    //     *dest = *at;
    //     at++;
    //     dest++;
    // }
    // int indexAt = (int)(mostRecent - exePath) + 1; //plus one to keep the slash
    // resPath[indexAt] = 'r';
    // resPath[indexAt + 1] = 'e';
    // resPath[indexAt + 2] = 's';
    // resPath[indexAt + 3] = '/';
    // resPath[indexAt + 4] = '\0';
    // assert(strlen(resPath) <= execPathLength);
    
    return 0;
}

char *lastFilePortion(char *at) {
    // TODO(Oliver): Make this more robust
    char *recent = at;
    while(*at) {
        if(*at == '/' && at[1] != '\0') { 
            recent = (at + 1); //plus 1 to pass the slash
        }
        at++;
    }
    
    int length = (int)(at - recent);
    char *result = (char *)calloc(length, 1);
    
    memcpy(result, recent, length);
    assert(result[length - 1] == '\0');
    
    return result;
}


////////////////////////////////////////////////////////////////////

///////////////////////************ Null Types*************////////////////////
#define EASY_NULL_SPRITE 0