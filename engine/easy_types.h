#include <stdint.h>


#if !defined arrayCount
#define arrayCount(array1) (sizeof(array1) / sizeof(array1[0]))
#endif 

#define ENUM(value) value,
#define STRING(value) #value,

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef intptr_t intprt;

#if !__cplusplus
typedef u32 bool;
#define true 1
#define false 0
#endif 

#define internal static 

void zeroSize(void *memory, size_t bytes) {
    char *at = (char *)memory;
    for(int i = 0; i < bytes; i++) {
        *at = 0;
        at++;
    }
}


bool stringsMatchN(char *a, int aLength, char *b, int bLength) {
    bool result = true;
    
    int indexCount = 0;
    while(indexCount < aLength && indexCount < bLength) {
        indexCount++;
        result &= (*a == *b);
        a++;
        b++;
    }
    result &= (indexCount == bLength && indexCount == aLength);
    
    return result;
} 


bool stringsMatchNullN(char *a, char *b, int bLen) {
    bool result = stringsMatchN(a, strlen(a), b, bLen);
    return result;
}

bool cmpStrNull(char *a, char *b) {
    bool result = stringsMatchN(a, strlen(a), b, strlen(b));
    return result;
}
