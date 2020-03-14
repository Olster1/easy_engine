/*
File to enable writing with fonts easy to add to your project. Uses Sean Barret's stb_truetype file to load and write the fonts. Requires a current openGL context. 
*/

#define STB_TRUETYPE_IMPLEMENTATION 
#include "stb_truetype.h"

///////////////////////************ New API *************////////////////////
static void easyFont_createSDFFont(char *fileName, u32 startCodePoint, u32 endCodePoint) {
    FileContents contents = platformReadEntireFile(fileName, false);
    
    //NOTE(ollie): This stores all the info about the font
    stbtt_fontinfo *fontInfo = pushStruct(&globalLongTermArena, stbtt_fontinfo);
    
    //NOTE(ollie): Fill out the font info
    stbtt_InitFont(fontInfo, contents.memory, 0);
    
    //NOTE(ollie): Get the 'scale' for the max pixel height 
    float maxHeightForFontInPixels = 32;//pixels
    float scale = stbtt_ScaleForPixelHeight(fontInfo, maxHeightForFontInPixels);
    
    //NOTE(ollie): Scale the padding around the glyph proportional to the size of the glyph
    s32 padding = (s32)(maxHeightForFontInPixels / 3);
    //NOTE(ollie): The distance from the glyph center that determines the edge (i.e. all the 'in' pixels)
    u8 onedge_value = (u8)(0.8f*255); 
    //NOTE(ollie): The rate at which the distance from the center should increase
    float pixel_dist_scale = (float)onedge_value/(float)padding;
    
    InfiniteAlloc atlasElms = initInfinteAlloc(Easy_AtlasElm);
    
    char *fontName = getFileLastPortionWithoutExtension_arena(fileName, &globalPerFrameArena);
        
    ///////////////////////************ Kerning Table *************////////////////////
    // //NOTE(ollie): Get the kerning table length i.e. number of entries
    // int  kerningTableLength = stbtt_GetKerningTableLength(fontInfo);
    
    // //NOTE(ollie): Allocate kerning table
    // stbtt_kerningentry* kerningTable = pushArray(&globalPerFrameArena, kerningTableLength, stbtt_kerningentry);
    
    // //NOTE(ollie): Fill out the table
    // stbtt_GetKerningTable(fontInfo, kerningTable, kerningTableLength);
    
    for(s32 codeIndex = startCodePoint; codeIndex <= endCodePoint; ++codeIndex) {
        
        int width;
        int height;
        int xoffset; 
        int yoffset;
        
        u8 *sdfBitmap = stbtt_GetCodepointSDF(fontInfo, scale, codeIndex, padding, onedge_value, pixel_dist_scale, &width, &height, &xoffset, &yoffset);    
        
        if(width > 0 && height > 0) {
            Easy_AtlasElm elm = {};
            
            //NOTE(ollie): Make sure names are in a memory arena, not on the stack
            char buffer[512];
            sprintf(buffer, "_%d", codeIndex);
            elm.shortName = concatInArena(fontName, buffer, &globalPerFrameArena);
            elm.longName = concatInArena(fileName, buffer, &globalPerFrameArena);
            
            elm.tex = {};
            elm.tex.width = width;
            elm.tex.height = height;
            elm.tex.aspectRatio_h_over_w = height / width;
            elm.tex.uvCoords = rect2f(0, 0, 1, 1);
            
            //NOTE(ollie): Set the font specific information
            elm.xOffset = xoffset;
            elm.yOffset = yoffset;
            elm.codepoint = codeIndex;
            elm.fontHeight = maxHeightForFontInPixels + (2*padding);
            elm.hasTexture = true;
            /////
            
            if(sdfBitmap) {
                //NOTE(ollie): Take memory mark before we push new bitmap on 
                perFrameArenaMark = takeMemoryMark(&globalPerFrameArena);
                
                //NOTE(ollie): Blow out bitmap to 32bits per pixel instead of 8 so it's easier to load to the GPU
                u32 *sdfBitmap_32 = (u32 *)pushSize(&globalPerFrameArena, width*height*sizeof(u32));
                for(int y = 0; y < height; ++y) {
                    for(int x = 0; x < width; ++x) {
                        u8 alpha = sdfBitmap[y*width + x];
                        if(alpha != 0) {
                            int j = 0;
                        }
                        sdfBitmap_32[y*width + x] = 0x00000000 | (u32)(((u32)alpha) << 24);
                    }
                }
                ////////////////////////////////////////////////////////////////////
                
                //NOTE(ollie): Load the texture to the GPU
                elm.tex.id = renderLoadTexture(width, height, sdfBitmap_32, RENDER_TEXTURE_DEFAULT);
                
                //NOTE(ollie): Release memory from 32bit bitmap
                releaseMemoryMark(&perFrameArenaMark);
            } else {
                elm.tex.id = globalWhiteTexture.id;
                elm.hasTexture = false;
            }
            
            //NOTE(ollie): Add to the array
            addElementInifinteAllocWithCount_(&atlasElms, &elm, 1);
        }
        
        //NOTE(ollie): Free the bitmap data
        stbtt_FreeSDF(sdfBitmap, 0);
    }
    
    //NOTE(ollie): Sort the elements by size first to get better layout
    easyAtlas_sortBySize(&atlasElms);
    
    
    //NOTE(ollie): Starting dimensions
    EasyAtlas_Dimensions dimensions = {};
    
    //NOTE(ollie): Start at 512 & move down. It would be better to do a binary search between 0-4096 to find the best size
    float rectX = 1028;
    float rectY = 1028;
    
    float increment = 32;
    
    char *outputFileName = concatInArena(concatInArena(globalExeBasePath, "./fontAtlas_", &globalPerFrameArena), fontName, &globalPerFrameArena);
    
    // NOTE(ollie): Try find the smallest size to fint everyone in
    do {
        dimensions = easyAtlas_drawAtlas(outputFileName, &globalPerFrameArena, &atlasElms, false, 5, rectX, rectY, EASY_ATLAS_FONT_ATLAS, 1);
        easyAtlas_refreshAllElements(&atlasElms);
        rectX -= increment;
        rectY -= increment;
    } while(dimensions.count == 1);
    
    rectX += 2*increment;
    rectY += 2*increment;
    
    easyAtlas_refreshAllElements(&atlasElms);
    //NOTE(ollie): Actually draw the atlas to disk now
    easyAtlas_drawAtlas(outputFileName, &globalPerFrameArena, &atlasElms, true, 5, rectX, rectY, EASY_ATLAS_FONT_ATLAS, 1);

    ///////////////////////*********** Cleanup the memory used**************////////////////////

    for(int index = 0; index < atlasElms.count; ++index) {
        Easy_AtlasElm *atlasElm = (Easy_AtlasElm *)getElementFromAlloc_(&atlasElms, index);
        if(atlasElm->tex.id > 0 && atlasElm->tex.id != globalWhiteTexture.id) {
            //NOTE(ollie): Delete all the textures we allocated
            renderDeleteTexture(&atlasElm->tex);
        }
    }

    //NOTE(ollie): Release the memory were using
    releaseInfiniteAlloc(&atlasElms);
    
    /*
    Bitmap functions

    stbtt_MakeCodepointBitmap()         
    stbtt_GetCodepointBitmapBox()
    */
}

typedef struct {
    Texture texture;
    
    s32 xOffset;
    s32 yOffset;
    
    u32 codepoint;

    //NOTE(ollie): Whether it has an actual texture or is just empty whitespace like 'space'
    bool hasTexture;    
    
} EasyFont_Glyph;

#define EASY_MAX_GLYPHS_PER_FONT 4096


typedef struct {
    u32 glyphCount;
    EasyFont_Glyph glyphs[EASY_MAX_GLYPHS_PER_FONT];
    
    s32 fontHeight;
} EasyFont_Font;

static EasyFont_Font *easyFont_loadFontAtlas(char *fileName, Arena *arena) {
    
    EasyFont_Font *font = pushStruct(arena, EasyFont_Font); 

    bool stillLoading = true;

    //NOTE(ollie): Atlas start at 1 for the loop index, so we want to mimic that
    u32 countAt = 1; 
    while(stillLoading) {
    
        char buffer0[4096] = {};
        snprintf(buffer0, arrayCount(buffer0), "%s_%d.txt", fileName, countAt);

        if(!platformDoesFileExist(buffer0)) {
            //NOTE(ollie): No more atlas files to load
            stillLoading = false;
        } else {
        
            char buffer1[4096] = {};
            snprintf(buffer1, arrayCount(buffer1), "%s_%d.png", fileName, countAt);
            
            Texture atlasTex = loadImage(buffer1, TEXTURE_FILTER_LINEAR, false);
            
            FileContents contentsText = getFileContentsNullTerminate(buffer0);
            assert(contentsText.valid);
            
            EasyTokenizer tokenizer = lexBeginParsing((char *)contentsText.memory, EASY_LEX_OPTION_EAT_WHITE_SPACE);
            bool parsing = true;
            
            Rect2f uvCoords = rect2f(0, 0, 0, 0);
            int imgWidth = 0;
            int imgHeight = 0;
            s32 codepoint = 0;
            s32 xoffset = 0;
            s32 yoffset = 0;
            bool hasTexture = true;
            
            while(parsing) {
                EasyToken token = lexGetNextToken(&tokenizer);
                InfiniteAlloc data = {};
                switch(token.type) {
                    case TOKEN_NULL_TERMINATOR: {
                        parsing = false;
                    } break;
                    case TOKEN_CLOSE_BRACKET: {
                        if(font->glyphCount < arrayCount(font->glyphs)) {
                            EasyFont_Glyph *g = font->glyphs + font->glyphCount++;
                            
                            g->texture.id = atlasTex.id;
                            g->texture.width = imgWidth;
                            g->texture.height = imgHeight;
                            g->texture.uvCoords = uvCoords;
                            g->texture.aspectRatio_h_over_w = easyRender_getTextureAspectRatio_HOverW(&g->texture);
                            g->texture.name = "glyph";

                            g->codepoint = codepoint;
                            g->xOffset = xoffset;
                            g->yOffset = yoffset;
                            g->hasTexture = hasTexture;
                        }                
                        
                    } break;
                    case TOKEN_WORD: {
                        if(stringsMatchNullN("width", token.at, token.size)) {
                            imgWidth = getIntFromDataObjects(&data, &tokenizer);
                        }
                        if(stringsMatchNullN("height", token.at, token.size)) {
                            imgHeight = getIntFromDataObjects(&data, &tokenizer);
                        }
                        if(stringsMatchNullN("fontHeight", token.at, token.size)) {
                            font->fontHeight = getIntFromDataObjects(&data, &tokenizer);
                        }
                        if(stringsMatchNullN("hasTexture", token.at, token.size)) {
                            hasTexture = (bool)getIntFromDataObjects(&data, &tokenizer);
                        }
                        if(stringsMatchNullN("uvCoords", token.at, token.size)) {
                            V4 uv = buildV4FromDataObjects(&data, &tokenizer);
                            //copy over to make a rect4 instead of a V4
                            uvCoords.E[0] = uv.E[0];
                            uvCoords.E[1] = uv.E[1];
                            uvCoords.E[2] = uv.E[2];
                            uvCoords.E[3] = uv.E[3];
                        }
                        if(stringsMatchNullN("xoffset", token.at, token.size)) {
                            xoffset = getIntFromDataObjects(&data, &tokenizer);
                        }
                        if(stringsMatchNullN("yoffset", token.at, token.size)) {
                            yoffset = getIntFromDataObjects(&data, &tokenizer);
                        }
                        if(stringsMatchNullN("codepoint", token.at, token.size)) {
                            codepoint = getIntFromDataObjects(&data, &tokenizer);
                        }
                    } break;
                    default: {
                        
                    }
                }
            }

            easyFile_endFileContents(&contentsText);
        }
        //NOTE(ollie): Increment the loop count
        countAt++;
    }
    
    return font;
}


////////////////////////////////////////////////////////////////////

//TODO API: 
// draw font needs to take less parameters & have one without a margin, also using a unit_scale measurement based on dpi 
// TODO: Needs to create a font bitmap the knows when it's filled. Using the texture atlas info. 

typedef struct {
    int index;
    V4 color;
} CursorInfo;

typedef struct FontSheet FontSheet;
typedef struct FontSheet {
    stbtt_bakedchar *cdata;
    GLuint handle;
    int minText;
    int maxText;
    
    FontSheet *next;
    
} FontSheet;

typedef struct {
    char *fileName;
    int fontHeight;
    FontSheet *sheets;
} Font;

static EasyFont_Font *globalDebugFont;

// TESTING: static stbtt_bakedchar *gh;
#define FONT_SIZE 1028 //This matters. We need to pack our own font glyphs in the future, since it's not garunteed to fit!!!
FontSheet *createFontSheet(Font *font, int firstChar, int endChar) {
    
    FontSheet *sheet = (FontSheet *)calloc(sizeof(FontSheet), 1);
    sheet->minText = firstChar;
    sheet->maxText = endChar;
    sheet->next = 0;
    const int bitmapW = FONT_SIZE;
    const int bitmapH = FONT_SIZE;
    const int numOfPixels = bitmapH*bitmapW;
    const int bytesPerPixelForMono = 1;
    u32 sizeOfBitmap = sizeof(u8)*numOfPixels*bytesPerPixelForMono;
    u8 *temp_bitmap = (u8 *)calloc(sizeOfBitmap, 1); //bakefontbitmap is one byte per pixel. 
    
    memset(temp_bitmap, 0, sizeOfBitmap);
    
    FileContents contents = platformReadEntireFile(font->fileName, false);
    
    int numOfChars = (endChar - firstChar - 1);
    //TODO: do we want to use an arena for this?
    sheet->cdata = (stbtt_bakedchar *)calloc(numOfChars*sizeof(stbtt_bakedchar), 1);
    //
    
    stbtt_BakeFontBitmap(contents.memory, 0, font->fontHeight, (unsigned char *)temp_bitmap, bitmapW, bitmapH, firstChar, numOfChars, sheet->cdata);
    // no guarantee this fits!
    // gh = sheet->cdata + (int)'y';
    
    free(contents.memory);
    // NOTE(Oliver): We expand the the data out from 8 bits per pixel to 32 bits per pixel. It doens't matter that the char data is based on the smaller size since getBakedQuad divides by the width of original, smaller bitmap. 
    
    // TODO(Oliver): can i free this once its sent to the graphics card? 
    unsigned int *bitmapTexture = (unsigned int *)calloc(sizeof(unsigned int)*numOfPixels, 1);
    u8 *src = temp_bitmap;
    unsigned int *dest = bitmapTexture;
    for(int y = 0; y < bitmapH; ++y) {
        for(int x = 0; x < bitmapW; ++x) {
            u8 alpha = *src++;
            *dest = 
                alpha << 24 |
                0xFF << 16 |
                0xFF << 8 |
                0xFF << 0;
            dest++;
        }
    }
    
#define WRITE_FONT_PNG 0
#if WRITE_FONT_PNG
    int bytesPerPixel = 4;
    int stride_in_bytes = bytesPerPixel*bitmapW;
    
    char nameBuf[1028] = {};
    sprintf(nameBuf, "./fontBitmap%d.png", firstChar);
    int writeResult = stbi_write_png(concat(globalExeBasePath, nameBuf), bitmapW, bitmapH, 4, bitmapTexture, stride_in_bytes);
#endif
    sheet->handle = renderLoadTexture(FONT_SIZE, FONT_SIZE, bitmapTexture, RENDER_TEXTURE_DEFAULT);
    assert(sheet->handle);
    free(bitmapTexture);
    free(temp_bitmap);
    
    return sheet;
}

FontSheet *findFontSheet(Font *font, unsigned int character) {
    assert(font);
    FontSheet *sheet = font->sheets;
    FontSheet *result = 0;
    while(sheet) {
        if(character >= sheet->minText && character < sheet->maxText) {
            result = sheet;
            break;
        }
        sheet = sheet->next;
    }
    
    if(!result) {
        // printf("%s\n", "Creating sheet");
        unsigned int interval = 256;
        unsigned int firstUnicode = ((int)(character / interval)) * interval;
        result = sheet = createFontSheet(font, firstUnicode, firstUnicode + interval);
        
        //put at the start of the list
        sheet->next = font->sheets;
        font->sheets = sheet;
    }
    assert(result);
    
    return result;
}

Font initFont_(char *fileName, int firstChar, int endChar, int fontHeight) {
    Font result = {}; 
    result.fontHeight = fontHeight;
    result.fileName = fileName;
    
    result.sheets = createFontSheet(&result, firstChar, endChar);
    return result;
}


Font initFont(char *fileName, int fontHeight) {
    Font result = initFont_(fileName, 0, 256, fontHeight); // ASCII 0..256 not inclusive
    return result;
}

typedef struct {
    stbtt_aligned_quad q;
    int index;
    V2 lastXY;
    u32 textureHandle;
    Rect2f uvCoords;
    u32 unicodePoint;
} GlyphInfo;


static inline GlyphInfo easyFont_getGlyph(Font *font, u32 unicodePoint) {
    GlyphInfo glyph = {};
    FontSheet *sheet = findFontSheet(font, unicodePoint);
    assert(sheet);
    
    if(unicodePoint != '\n') {
        if (((int)(unicodePoint) >= sheet->minText && (int)(unicodePoint) < sheet->maxText)) {
            stbtt_aligned_quad q  = {};
            
            float x = 0;
            float y = 0;
            stbtt_GetBakedQuad(sheet->cdata, FONT_SIZE, FONT_SIZE, unicodePoint - sheet->minText, &x,&y,&q, 1);
            
            glyph.textureHandle = sheet->handle;
            glyph.q = q;
            glyph.uvCoords = rect2f(q.s0, q.t1, q.s1, q.t0);
            
        } else {
            assert(!"invalid code path");
        }
    }
    return glyph;
    
}

static inline EasyFont_Glyph *easyFont_findGlyph(EasyFont_Font *font, u32 unicodePoint) {
    //NOTE(ollie): @speed 
    EasyFont_Glyph *result = 0;
    for(int i = 0; i < font->glyphCount && !result; ++i) {
        EasyFont_Glyph *glyph = font->glyphs + i;
        
        if(glyph->codepoint == unicodePoint) {
            result = glyph;
            break;
        }
        
    }
    return result;
}

//This does unicode now 
Rect2f my_stbtt_print_(EasyFont_Font *font, float x, float y, float zAt, V2 resolution, char *text_, Rect2f margin, V4 color, float size, CursorInfo *cursorInfo, bool display, float resolutionDiffScale) {
    Rect2f bounds = InverseInfinityRect2f();
    size *= resolutionDiffScale;
    if(x < margin.min.x) { x = margin.min.x; }
    
    // easyRender_pushScissors(globalRenderGroup, margin, zAt, mat4TopLeftToBottomLeft(resolution.y), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y), resolution);
    
    float width = 0; 
    float height = 0;
    V2 pos = v2(0, 0);
    
    EasyFont_Glyph *glyphs[1028] = {}; //characters in a line until it will break;
    int quadCount = 0;
    
    char *text = text_;
    V2 lastXY = v2(x, y);
    V2 startP = lastXY;
#define transformToSizeX(val) ((val - startP.x)*size + startP.x)
#define transformToSizeY(val) ((val - startP.y)*size + startP.y)
    bool inWord = false;
    bool hasBeenToNewLine = false;
    char *tempAt = text;
    while (*text) {
        char *tempR = text;
        unsigned int unicodePoint = easyUnicode_utf8ToUtf32((unsigned char **)&text, false);
        int unicodeLen = easyUnicode_unicodeLength(*text);
        
        assert(tempR == text);
        bool increment = true;
        bool drawText = false;
        
        bool overflowed = (transformToSizeX(x) > margin.maxX && inWord);
        
        if(unicodePoint == ' ' || unicodePoint == '\n') {
            inWord = false;
            hasBeenToNewLine = false;
            drawText = true;
        } else {
            inWord = true;
        }
        
        //NOTE(ollie): Add glyph to the batch if not a new line
        if(unicodePoint != '\n') {
            if(quadCount < arrayCount(glyphs)) {
                glyphs[quadCount++] = easyFont_findGlyph(font, unicodePoint); 
            } else {
                //NOTE(ollie): Can't draw this glyph so sumbit the batch we've already got
                drawText = true;
                increment = false;
                assert(false);
            }
        }
        
        
        if(overflowed && !hasBeenToNewLine && inWord) {
            text = tempAt;
            increment = false;
            quadCount = 0;  //empty the quads. We lose the data because we broke the word.  
            hasBeenToNewLine = true;
        }
        
        bool lastCharacter = (*(text + unicodeLen) == '\0');
        
        if(drawText || lastCharacter) {
            for(int i = 0; i < quadCount; ++i) {
                EasyFont_Glyph *glyph = glyphs[i];
                
                if(glyph) {
                    
                    Rect2f b = rect2fMinDim(x + size*glyph->xOffset, y + size*glyph->yOffset, size*glyph->texture.width, size*glyph->texture.height);
                    
                    x += size*glyph->texture.width + size*glyph->xOffset;
                    
                    bounds = unionRect2f(bounds, b);
                    
                    if(display && glyph->hasTexture) {
                        renderGlyphCentreDim(&glyph->texture, v2ToV3(getCenter(b), zAt), getDim(b), color, 0, mat4TopLeftToBottomLeft(resolution.y), mat4(), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));            
                    }
                    
                    //NOTE(ollie): Get where the cursor was
                    // if(cursorInfo && (glyph->index == cursorInfo->index)) {
                    //     width = q.x1 - q.x0;
                    //     height = font->fontHeight*size;//q.y1 - q.y0;
                    //     pos = v2(transformToSizeX(glyph->lastXY.x) + 0.5f*width, transformToSizeY(glyph->lastXY.y) - 0.5f*height);
                    // }
                }
                
            }
            
            quadCount = 0; //empty the quads now that we have finished drawing them
        }

        //NOTE(ollie): 
        if(overflowed || unicodePoint == '\n') {
            x = margin.minX;
            y += size*font->fontHeight; 
        }
        
        lastXY.x = x;
        lastXY.y = y;
        if(increment) {
            
            text += unicodeLen;
        }
        if(!inWord) {
            tempAt = text;
        }
    }
    
    if(cursorInfo && (int)(text - text_) <= cursorInfo->index) {
        width = size*16;
        height = size*32;
        
        pos = v2(transformToSizeX(x) + 0.5f*width, transformToSizeY(y) - 0.5f*height);
    }
    
    if(cursorInfo) {
        renderDrawRectCenterDim(v2ToV3(pos, zAt - 0.1f), v2(16, height), cursorInfo->color, 0, mat4TopLeftToBottomLeft(resolution.y), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));
    }
#if 0 //draw idvidual text boxes
    for(int i = 0; i < pC; ++i) {
        Rect2f b = points[i];
        V2 d = points1[i];
        
        float w = b.maxX - b.minX;
        float h = b.maxY - b.minY;
        V2 c = v2(b.minX + 0.5f*w, b.minY + 0.5f*h);
        openGlDrawRectOutlineCenterDim(c, v2(w, h), v4(1, 0, 0, 1), 0, v2(1, -1), v2(0, bufferHeight));
        
    }
#endif
#if 0 //draw text bounds
    openGlDrawRectOutlineCenterDim(getCenter(bounds), getDim(bounds), v4(1, 0, 0, 1), 0, v2(1, -1), v2(0, bufferHeight));
#endif
    
    if(text == text_) { //there wasn't any text
        bounds = rect2f(0, 0, 0, 0);
    }
    
    // easyRender_disableScissors(globalRenderGroup);
    
    return bounds;
    
}

Rect2f outputText_with_cursor(EasyFont_Font *font, float x, float y, float z, V2 resolution, char *text, Rect2f margin, V4 color, float size, int cursorIndex, V4 cursorColor, bool display, float resolutionDiffScale) {
    CursorInfo cursorInfo = {};
    cursorInfo.index = cursorIndex;
    cursorInfo.color = cursorColor;
    
    Rect2f result = my_stbtt_print_(font, x, y, z, resolution, text, margin, color, size, &cursorInfo, display, resolutionDiffScale);
    return result;
}

Rect2f outputText(EasyFont_Font *font, float x, float y, float z, V2 resolution, char *text, Rect2f margin, V4 color, float size, bool display, float resolutionDiffScale) {
    
    Rect2f result = my_stbtt_print_(font, x, y, z, resolution, text, margin, color, size, 0, display, resolutionDiffScale);
    float offset = 3*resolutionDiffScale;
    my_stbtt_print_(font, x + offset, y + offset, z + 0.1f, resolution, text, margin, v4(0.5f, 0.5f, 0.5f, min(color.w, 0.8f)), size, 0, display, resolutionDiffScale);
    return result;
}

Rect2f outputTextNoBacking(EasyFont_Font *font, float x, float y, float z, V2 resolution, char *text, Rect2f margin, V4 color, float size, bool display, float resolutionDiffScale) {
    
    Rect2f result = my_stbtt_print_(font, x, y, z, resolution, text, margin, color, size, 0, display, resolutionDiffScale);
    return result;
}

Rect2f outputTextWithLength(EasyFont_Font *font, float x, float y, float z, V2 resolution, char *allText, int textLength, Rect2f margin, V4 color, float size, bool display, float resolutionDiffScale) {
    char *text = (char *)malloc(sizeof(char)*(textLength + 1));
    for(int i = 0; i < textLength; ++i) {
        text[i] = allText[i];
    }
    
    text[textLength] = '\0'; //null terminate. 
    Rect2f result = my_stbtt_print_(font, x, y, z, resolution, text, margin, color, size, 0, display, resolutionDiffScale);
    free(text);
    return result;
}

V2 getBounds(char *string, Rect2f margin, EasyFont_Font *font, float size, V2 resolution, float resolutionDiffScale) {
    Rect2f bounds  = outputText(font, margin.minX, margin.minY, -1, resolution, string, margin, v4(0, 0, 0, 1), size, false, resolutionDiffScale);
    
    V2 result = getDim(bounds);
    return result;
}


Rect2f getBoundsRectf(char *string, float xAt, float yAt, Rect2f margin, EasyFont_Font *font, float size, V2 resolution, float resolutionDiffScale) {
    Rect2f bounds  = outputText(font, xAt, yAt, -1, resolution, string, margin, v4(0, 0, 0, 1), size, false, resolutionDiffScale);
    
    return bounds;
}

