/*
Header file to build texture atlases which outputs the texture_name, uv_coords & the original width and height 
of the image. It requires stb_image_write also easy_text_io, eas_lex & easy_array header files to work for text writing/parsing 
and stretchy buffer respectively
*/

typedef enum {
	EASY_ATLAS_TEXTURE_ATLAS,
	EASY_ATLAS_FONT_ATLAS,
} EasyAtlas_AtlasType;

typedef struct {
	char *shortName;
	char *longName;

	union {
		struct { //Fonts
			s32 xOffset;
			s32 yOffset;
			s32 codepoint;
			s32 fontHeight;
			bool hasTexture;
		};
	};
	
	//NOTE(ollie): Width, height & image data in this 
	Texture tex;
	bool added;
    
} Easy_AtlasElm;

static inline void easyAtlas_sortBySize(InfiniteAlloc *atlasElms) {
	bool sorted = false;
	int max = (atlasElms->count - 1);
	for(int index = 0; index < max; ) {
		bool incrementIndex = true;
		assert(index + 1 < atlasElms->count);
		
		Easy_AtlasElm *infoA = (Easy_AtlasElm *)getElementFromAlloc_(atlasElms, index);
		Easy_AtlasElm *infoB = (Easy_AtlasElm *)getElementFromAlloc_(atlasElms, index + 1);
		assert(infoA && infoB);
		int sizeA = infoA->tex.width*infoA->tex.height;
    	int sizeB = infoB->tex.width*infoB->tex.height;
		bool swap = sizeA < sizeB;
		if(swap) {
		    Easy_AtlasElm temp = *infoA;
		    *infoA = *infoB;
		    *infoB = temp;
		    sorted = true;
		}   
		if(index == (max - 1) && sorted) {
		    index = 0; 
		    sorted = false;
		    incrementIndex = false;
		}
        
		if(incrementIndex) {
			index++;
		}
	}
    
	//NOTE: for debugging to check it actually sorted correctly 
	// for(int index = 0; index < max; ++index) {
	// 	Easy_AtlasElm *infoA = (Easy_AtlasElm *)getElementFromAlloc_(atlasElms, index);
	// 	Easy_AtlasElm *infoB = (Easy_AtlasElm *)getElementFromAlloc_(atlasElms, index + 1);
	// 	int sizeA = infoA->tex.width*infoA->tex.height;
    //    	int sizeB = infoB->tex.width*infoB->tex.height;
	// 	assert(sizeA >= sizeB);
	// }
}

typedef struct EasyAtlas_BinPartition EasyAtlas_BinPartition;
typedef struct EasyAtlas_BinPartition {
	EasyAtlas_BinPartition *next;
	EasyAtlas_BinPartition *prev;
	
	Rect2f rect;
    
} EasyAtlas_BinPartition;


/*
- Place new bin on the shortest side of the image 
- partition bin into two new bins 
- best fit - search list for smallest partition the image will fit in
- paritions in a doubley linked list 

*/

typedef struct {
	EasyAtlas_BinPartition sentinel;
	EasyAtlas_BinPartition *freeList;
    
	Arena *memoryArena;
} EasyAtlas_BinState;

#include "float.h"

static inline EasyAtlas_BinPartition *easyAtlas_findBestFitBin(EasyAtlas_BinState *state, Texture *tex, int EASY_ATLAS_PADDING) {
	EasyAtlas_BinPartition *sentinel = &state->sentinel;
	EasyAtlas_BinPartition *binAt = sentinel->next;
	EasyAtlas_BinPartition *bestFit = 0;
	V2 bestDimDiff = v2(FLT_MAX, FLT_MAX);
	while (binAt != sentinel) {
		V2 dimDiff = v2_minus(getDim(binAt->rect), v2(tex->width + EASY_ATLAS_PADDING, tex->height + EASY_ATLAS_PADDING));
		if(dimDiff.x >= 0 && dimDiff.x < bestDimDiff.x && dimDiff.y >= 0 && dimDiff.y < bestDimDiff.y) {	
			bestDimDiff = dimDiff;
			bestFit = binAt;
		}
		binAt = binAt->next;
	}
	return bestFit;
}

static inline void easyAtlas_addBin(Rect2f a, EasyAtlas_BinState *state) {
	V2 aDim = getDim(a);
	if(aDim.x != 0 && aDim.y != 0) {
		EasyAtlas_BinPartition *bin = 0;
		if(state->freeList) {
			bin = state->freeList;
			state->freeList = bin->next;
		} else {
			bin = pushStruct(state->memoryArena, EasyAtlas_BinPartition);	
		}
		assert(bin);
		
		//add to the doubley linked list. 
		bin->next = &state->sentinel;
		assert(state->sentinel.prev->next == &state->sentinel);
		bin->prev = state->sentinel.prev;
        
		state->sentinel.prev->next = bin;
		state->sentinel.prev = bin;
		
		bin->rect = a;
		
	}
}

static inline void easyAtlas_removeBin(EasyAtlas_BinPartition *bin, EasyAtlas_BinState *state) {
	assert(bin->prev->next == bin);
	bin->prev->next = bin->next;
	bin->next->prev = bin->prev;
    
	bin->prev = 0;
	bin->next = state->freeList;
	state->freeList = bin;
}

static inline void easyAtlas_partitionBin(EasyAtlas_BinState *state, EasyAtlas_BinPartition *bin, Texture *tex, int EASY_ATLAS_PADDING) {
	V2 texDim = v2(tex->width + EASY_ATLAS_PADDING, tex->height + EASY_ATLAS_PADDING);
	Rect2f br = bin->rect;
	Rect2f a = br;
	Rect2f b = br;
	
	if(texDim.x > texDim.y) {
		a.minX = br.minX + tex->width + EASY_ATLAS_PADDING;
		a.maxY = br.minY + tex->height + EASY_ATLAS_PADDING;
        
		b.minY = br.minY + tex->height + EASY_ATLAS_PADDING;
	} else {
		a.minY = br.minY + tex->height + EASY_ATLAS_PADDING;
		a.maxX = br.minX + tex->width + EASY_ATLAS_PADDING;
        
		b.minX = br.minX + tex->width + EASY_ATLAS_PADDING;
	}
    
	easyAtlas_removeBin(bin, state);	
    
	easyAtlas_addBin(a, state);
	easyAtlas_addBin(b, state);
	
}

static inline bool easyAtlas_allElmsBeenAdded(InfiniteAlloc *atlasElms) {
	int addedCount = 0;
	for(int index = 0; index < atlasElms->count; ++index) {
		Easy_AtlasElm *atlasElm = (Easy_AtlasElm *)getElementFromAlloc_(atlasElms, index);
		if(atlasElm->added) {
			addedCount++;
		}
	}
	bool result = (addedCount == atlasElms->count);
	return result;
}

//NOTE(ollie): Go through and set all elements to 'not' been added
static inline void easyAtlas_refreshAllElements(InfiniteAlloc *atlasElms) {
	for(int index = 0; index < atlasElms->count; ++index) {
		Easy_AtlasElm *atlasElm = (Easy_AtlasElm *)getElementFromAlloc_(atlasElms, index);
		atlasElm->added = false;
	}
}

typedef struct {
	u32 count;
	Rect2f dimensions[128];
} EasyAtlas_Dimensions;

static inline EasyAtlas_Dimensions easyAtlas_drawAtlas(char *folderName, Arena *memoryArena, InfiniteAlloc *atlasElms, bool outputImageFile, int EASY_ATLAS_PADDING, int sizeX, int sizeY, EasyAtlas_AtlasType type, float scale) {
    
    EasyAtlas_Dimensions result = {};

	MemoryArenaMark tempMark = takeMemoryMark(memoryArena);
	
	EasyAtlas_BinState state = {};
	state.sentinel.next = state.sentinel.prev = &state.sentinel;
	state.freeList = 0;
	state.memoryArena = memoryArena;
    
	int addedCount = 0;
	int loopCount = 0;
	while(!easyAtlas_allElmsBeenAdded(atlasElms)) {
		loopCount++;
        
		V2 bufferDim = v2(sizeX, sizeY);

		renderSetViewport(globalRenderGroup, 0, 0, bufferDim.x, bufferDim.y);

		//NOTE(ollie): Add new dimension to the dimensions
		assert(result.count < arrayCount(result.dimensions));
		result.count++;

		FrameBuffer atlasBuffer = {};
		if(outputImageFile) {
			atlasBuffer = createFrameBuffer(bufferDim.x, bufferDim.y, FRAMEBUFFER_COLOR, 1);
			// initRenderGroup(globalRenderGroup);
			clearBufferAndBind(atlasBuffer.bufferId, COLOR_NULL, atlasBuffer.flags, globalRenderGroup);
			setFrameBufferId(globalRenderGroup, atlasBuffer.bufferId);
			 setBlendFuncType(globalRenderGroup, BLEND_FUNC_STANDARD_PREMULTIPLED_ALPHA);
        }

		//draw all the rects into the frame buffer
		easyAtlas_addBin(rect2f(0, 0, bufferDim.x, bufferDim.y), &state);
        
		InfiniteAlloc fileMemoryToWrite = initInfinteAlloc(char);
        
		bool bufferHasRoom = true;
		for(int index = 0; index < atlasElms->count; ++index) {
			Easy_AtlasElm *atlasElm = (Easy_AtlasElm *)getElementFromAlloc_(atlasElms, index);
			if(!atlasElm->added) {
				Texture texOnStack = atlasElm->tex;

				//NOTE(ollie): Scale by the texture size
				texOnStack.width *= scale;
				texOnStack.height *= scale;
                
				if(state.sentinel.next != &state.sentinel) {
					EasyAtlas_BinPartition *bin = easyAtlas_findBestFitBin(&state, &texOnStack, EASY_ATLAS_PADDING);
					if(bin) {
						atlasElm->added = true;
                        	
						Rect2f texAsRect = rect2f(bin->rect.minX + 0.5f*EASY_ATLAS_PADDING, bin->rect.minY + 0.5f*EASY_ATLAS_PADDING, bin->rect.minX + texOnStack.width, bin->rect.minY + texOnStack.height);
						
						Rect2f resultDim = result.dimensions[result.count - 1];

						//NOTE(ollie): Absorb the new rectangle
						resultDim = unionRect2f(resultDim, texAsRect);
						result.dimensions[result.count - 1] = resultDim;

						if(outputImageFile) {
							Matrix4 flipMatrix = mat4();

							if(type == EASY_ATLAS_FONT_ATLAS) {
								flipMatrix = mat4TopLeftToBottomLeft(bufferDim.y);
							}
							renderTextureCentreDim(&texOnStack, v2ToV3(getCenter(texAsRect), NEAR_CLIP_PLANE), getDim(texAsRect), COLOR_WHITE, 0, flipMatrix, mat4(), OrthoMatrixToScreen_BottomLeft(bufferDim.x, bufferDim.y));
							// renderDrawRectOutlineCenterDim_(v2ToV3(getCenter(bin->rect), -0.5f), getDim(bin->rect), COLOR_RED, 0, mat4(), OrthoMatrixToScreen_BottomLeft(bufferDim.x, bufferDim.y), 4); 
	                        
							texOnStack.uvCoords = rect2f(texAsRect.minX / bufferDim.x, texAsRect.minY / bufferDim.y, texAsRect.maxX / bufferDim.x, texAsRect.maxY / bufferDim.y);
						    assert(atlasBuffer.colorBufferCount != 0);
						    texOnStack.id = atlasBuffer.textureIds[0];
						    
	                        
						    char *startBr = "{";
							addElementInifinteAllocWithCount_(&fileMemoryToWrite, startBr, 1); 
	                        
						    addVar(&fileMemoryToWrite, &texOnStack.width, "width", VAR_INT);
						    addVar(&fileMemoryToWrite, &texOnStack.height, "height", VAR_INT);
						    addVar(&fileMemoryToWrite, texOnStack.uvCoords.E, "uvCoords", VAR_V4);
						    addVar(&fileMemoryToWrite, atlasElm->shortName, "name", VAR_CHAR_STAR);
						    addVar(&fileMemoryToWrite, atlasElm->longName, "fullName", VAR_CHAR_STAR);

						    //NOTE(ollie): Also add offets & codepoint for font atlases
						    if(type == EASY_ATLAS_FONT_ATLAS) {
								addVar(&fileMemoryToWrite, &atlasElm->xOffset, "xoffset", VAR_INT);
								addVar(&fileMemoryToWrite, &atlasElm->yOffset, "yoffset", VAR_INT);						    	
								addVar(&fileMemoryToWrite, &atlasElm->codepoint, "codepoint", VAR_INT);		
								addVar(&fileMemoryToWrite, &atlasElm->fontHeight, "fontHeight", VAR_INT);	
								addVar(&fileMemoryToWrite, &atlasElm->hasTexture, "hasTexture", VAR_INT);	
						    }
	                        
						    char *endBr = "}";
						    addElementInifinteAllocWithCount_(&fileMemoryToWrite, endBr, 1); 
	                        
						    Texture *tex = (Texture *)calloc(sizeof(Texture), 1);
						    memcpy(tex, &texOnStack, sizeof(Texture));
						    Asset *result = addAssetTexture(atlasElm->shortName, tex);
                        }
					    easyAtlas_partitionBin(&state, bin, &texOnStack, EASY_ATLAS_PADDING);
					} else {
						//make sure the texture can at least fit on the biggest size. 
						assert(texOnStack.width + EASY_ATLAS_PADDING < sizeX && texOnStack.height + EASY_ATLAS_PADDING < sizeY);
					}
				} else {
					printf("%s\n", "no more bins");
					//no more bins so make a new texture 
				}
			} 
		}
        
        if(outputImageFile) {
			
			drawRenderGroup(globalRenderGroup, RENDER_DRAW_DEFAULT);
	        
			glFlush();
			printf("%s\n", "successfullly rendered group");
	        
	        //NOTE(ollie): Make sure the name fits in a buffer
            char *formatString = "%s_%d.txt";
            //NOTE(ollie): Get the string length
			int stringLengthToAlloc = snprintf(0, 0, formatString, folderName, loopCount) + 1;
			//NOTE(ollie): Pushe the array
			char *strArray = pushArray(&globalPerFrameArena, stringLengthToAlloc, char);
			//NOTE(ollie): Write the string into the buffer
			snprintf(strArray, stringLengthToAlloc, formatString, folderName, loopCount);

			////////////////////////////////////////////////////////////////////

			//NOTE(ollie): Open or create the file
			game_file_handle fileHandle = platformBeginFileWrite(strArray);
	        
			platformWriteFile(&fileHandle, fileMemoryToWrite.memory, fileMemoryToWrite.sizeOfMember*fileMemoryToWrite.count, 0);
	        
			platformEndFile(fileHandle);
	        
			printf("%s\n", "wrote file");
            	
            //NOTE(ollie): Make sure the name fits in a buffer
            formatString = "%s_%d.png";
            //NOTE(ollie): Get the string length
			stringLengthToAlloc = snprintf(0, 0, formatString, folderName, loopCount) + 1; //for null terminator
			//NOTE(ollie): Pushe the array
			strArray = pushArray(&globalPerFrameArena, stringLengthToAlloc, char);
			//NOTE(ollie): Write the string into the buffer
			snprintf(strArray, stringLengthToAlloc, formatString, folderName, loopCount);

			////////////////////////////////////////////////////////////////////
				
			///////////////////////*********** Write the png file to disk **************////////////////////
			size_t bytesPerPixel = 4;
			size_t sizeToAlloc = bufferDim.x*bufferDim.y*bytesPerPixel;
			int stride_in_bytes = bytesPerPixel*bufferDim.x;
			
			u8 *pixelBuffer = (u8 *)calloc(sizeToAlloc, 1);
			
			renderReadPixels(atlasBuffer.bufferId, 0, 0,
                             bufferDim.x,
                             bufferDim.y,
                             GL_RGBA,
                             GL_UNSIGNED_BYTE,
                             pixelBuffer);
            
			
			int writeResult = stbi_write_png(strArray, bufferDim.x, bufferDim.y, 4, pixelBuffer, stride_in_bytes);
            	
            //NOTE(ollie): Notify user of progress
            printf("%s\n", "wrote image");

            ///////////////////////************* Cleanup ************////////////////////
			
			free(pixelBuffer);
			deleteFrameBuffer(&atlasBuffer);
		} 
	}
    
	releaseMemoryMark(&tempMark);

	return result;
}

static inline void easyAtlas_loadTextureAtlas(char *fileName, RenderTextureFilter filter) {
	char buffer0[512] = {};
	sprintf(buffer0, "%s.txt", fileName);
    
	char buffer1[512] = {};
	sprintf(buffer1, "%s.png", fileName);
	
	bool premultiplyAlpha = false;
	Texture atlasTex = loadImage(buffer1, filter, true, premultiplyAlpha);
    
	FileContents contentsText = getFileContentsNullTerminate(buffer0);
	
	EasyTokenizer tokenizer = lexBeginParsing((char *)contentsText.memory, EASY_LEX_OPTION_EAT_WHITE_SPACE);
	bool parsing = true;
    
	char imageName[256] = {};
	Rect2f uvCoords = rect2f(0, 0, 0, 0);
	int imgWidth = 0;
	int imgHeight = 0;
	
	while(parsing) {
	    EasyToken token = lexGetNextToken(&tokenizer);
	    InfiniteAlloc data = {};
	    switch(token.type) {
	        case TOKEN_NULL_TERMINATOR: {
	            parsing = false;
	        } break;
	        case TOKEN_CLOSE_BRACKET: {
	        	// printf("%s\n", imageName);
	        	
	        	Texture *tex = (Texture *)calloc(sizeof(Texture), 1);
                
	        	tex->id = atlasTex.id;
	        	tex->width = imgWidth;
	        	tex->height = imgHeight;
	        	tex->uvCoords = uvCoords;
	        	tex->aspectRatio_h_over_w = easyRender_getTextureAspectRatio_HOverW(tex);
	        	tex->name = imageName; 
                
	        	Asset *result = addAssetTexture(imageName, tex);
	        	assert(result);
	        } break;
	        case TOKEN_WORD: {
	            if(stringsMatchNullN("name", token.at, token.size)) {
	                char *string = getStringFromDataObjects_memoryUnsafe(&data, &tokenizer);
                    int strSize = strlen(string); 
                    nullTerminateBuffer(imageName, string, min(arrayCount(imageName) - 1, strSize));

                    //NOTE(ollie): release the memory
                    releaseInfiniteAlloc(&data);
	            }
	            if(stringsMatchNullN("width", token.at, token.size)) {
	                imgWidth = getIntFromDataObjects(&data, &tokenizer);
	            }
	            if(stringsMatchNullN("height", token.at, token.size)) {
	                imgHeight = getIntFromDataObjects(&data, &tokenizer);
	            }
	            if(stringsMatchNullN("uvCoords", token.at, token.size)) {
	                V4 uv = buildV4FromDataObjects(&data, &tokenizer);
	                //copy over to make a rect4 instead of a V4
	                uvCoords.E[0] = uv.E[0];
	                uvCoords.E[1] = uv.E[1];
	                uvCoords.E[2] = uv.E[2];
	                uvCoords.E[3] = uv.E[3];
	            }
	        } break;
	        default: {
                
	        }
	    }
	}
}	

#define easyAtlas_createTextureAtlas(folderName, ouputFolderName, memoryArena, filter, padding, atlasX, atlasY) easyAtlas_createTextureAtlas_withDownsize(folderName, ouputFolderName, memoryArena, filter, padding, 1, atlasX, atlasY)
static inline void easyAtlas_createTextureAtlas_withDownsize(char *folderName, char *ouputFolderName, Arena *memoryArena, RenderTextureFilter filter, int padding, float scale, float atlasX, float atlasY) {
	MemoryArenaMark tempMark = takeMemoryMark(memoryArena);

	char *imgFileTypes[] = {"jpg", "jpeg", "png", "bmp", "PNG"};
	folderName = concat(globalExeBasePath, folderName);
	ouputFolderName = concat(globalExeBasePath, ouputFolderName);
	FileNameOfType fileNames = getDirectoryFilesOfType(folderName, imgFileTypes, arrayCount(imgFileTypes));
    
	InfiniteAlloc atlasElms = initInfinteAlloc(Easy_AtlasElm);
    
	for(int i = 0; i < fileNames.count; ++i) {
	    char *fullName = fileNames.names[i];
	    char *shortName = getFileLastPortionWithArena(fullName, memoryArena);
	    if(shortName[0] != '.') { //don't load hidden file 
	        Asset *asset = findAsset(shortName);
	        assert(!asset);
            
        	Easy_AtlasElm elm = {};
        	elm.shortName = shortName;
        	elm.longName = fullName;

        	bool premultiplyAlpha = true;
        	elm.tex = loadImage(fullName, TEXTURE_FILTER_LINEAR, true, premultiplyAlpha);

        	//NOTE(ollie): Add it to the atlas elements
        	addElementInifinteAllocWithCount_(&atlasElms, &elm, 1);
	        assert(shortName);
	    }
	}
    
	//sorting them by size so the bigger images get first preference
	easyAtlas_sortBySize(&atlasElms);
	stbi_flip_vertically_on_write(true);//flip bytes vertically
    
	easyAtlas_drawAtlas(ouputFolderName, memoryArena, &atlasElms, true, padding, atlasX, atlasY, EASY_ATLAS_TEXTURE_ATLAS, scale);
    
	releaseInfiniteAlloc(&atlasElms);
    
	free(folderName);
	free(ouputFolderName);

	for(int i = 0; i < fileNames.count; ++i) {
	    char *fullName = fileNames.names[i];
	    free(fullName);
	}

	releaseMemoryMark(&tempMark);
}