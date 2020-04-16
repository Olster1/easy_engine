
static u32 easyString_getSizeInBytes_utf8(char *string) {
    u32 result = 0;
    u8 *at = (u8 *)string;
    while(*at) {
        result++;
        at++;
    }
    return result;
}

static u32 easyString_getStringLength_utf8(char *string) {
    u32 result = 0;
    u8 *at = (u8 *)string;
    while(*at) {
        easyUnicode_utf8ToUtf32((unsigned char **)&at, true);
        result++;
    }
    return result;
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