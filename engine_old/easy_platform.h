#ifndef EASY_PLATFORM_H
/*
Code for platform depedent functions 
*/

typedef enum {
	EASY_PLATFORM_MEMORY_NONE = 0,
	EASY_PLATFORM_MEMORY_ZERO = 1 << 0,
} EasyPlatform_MemoryFlag;

static void *easyPlatform_allocateMemory(u32 sizeInBytes, EasyPlatform_MemoryFlag flags) {

	void *result = 0;
#if 0//_WIN32
	
	///////////////////////************ Win32 *************////////////////////
	
	result = HeapAlloc(GetProcessHeap(), 0, sizeInBytes);

	////////////////////////////////////////////////////////////////////

#else
	///////////////////////************ C runtime *************//////////////////// 
	
	result = malloc(sizeInBytes);

	////////////////////////////////////////////////////////////////////
#endif
	
	if(!result) {
		easyLogger_addLog("Platform out of memory on heap allocate!");
	}

	if(flags & EASY_PLATFORM_MEMORY_ZERO) {
		zeroSize(result, sizeInBytes);
	}

	return result;
}

static void easyPlatform_freeMemory(void *memory) {

#if 0//_WIN32
	HeapFree(GetProcessHeap(), 0, memory);
#else 
	free(memory);
#endif
}


static void easyPlatform_copyMemory(char *to, char *from, u32 sizeInBytes) {
	memcpy(to, from, sizeInBytes);
}


#define EASY_PLATFORM_H 1
#endif