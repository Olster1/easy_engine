/*
File to enable writing with fonts easy to add to your project. Uses Sean Barret's stb_truetype file to load and write the fonts. Requires a current openGL context. 
*/

#define STB_TRUETYPE_IMPLEMENTATION 
#include "stb_truetype.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


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

static Font globalDebugFont;

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

//This does unicode now 
Rect2f my_stbtt_print_(Font *font, float x, float y, float zAt, V2 resolution, char *text_, Rect2f margin, V4 color, float size, CursorInfo *cursorInfo, bool display, float resolutionDiffScale) {
    Rect2f bounds = InverseInfinityRect2f();
    size *= resolutionDiffScale;
    if(x < margin.min.x) { x = margin.min.x; }

    easyRender_pushScissors(globalRenderGroup, margin, zAt, mat4TopLeftToBottomLeft(resolution.y), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y), resolution);

    //if(bounds.min.y > y) { y = bounds.min.x; }
    float width = 0; 
    float height = 0;
    V2 pos = v2(0, 0);
    
    //For individual text boxes: DEBUG
    // Rect2f points[512] = {};
    // int pC = 0;
    
    // V2 points1[512] = {};
    // int pC1 = 0;
    
    GlyphInfo qs[256] = {};
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
        // unsigned int unicodePoint = *text;
        // int unicodeLen = 1;

        assert(tempR == text);
        bool increment = true;
        bool drawText = false;
        //points1[pC1++] = v2(x, y);
        FontSheet *sheet = findFontSheet(font, unicodePoint);
        assert(sheet);

        if(unicodePoint != '\n') {
            if (((int)(unicodePoint) >= sheet->minText && (int)(unicodePoint) < sheet->maxText)) {
                stbtt_aligned_quad q  = {};
                // if(unicodePoint == 'y') {
                //     printf("%s\n", "is y before");
                //     Rect2f uvCoords = rect2f(q.s0, q.t1, q.s1, q.t0);
                //     error_printFloat4("uv values", uvCoords.E);
                // }
                stbtt_GetBakedQuad(sheet->cdata, FONT_SIZE, FONT_SIZE, unicodePoint - sheet->minText, &x, &y, &q, 1);
                assert(quadCount < arrayCount(qs));
                GlyphInfo *glyph = qs + quadCount++; 
                glyph->textureHandle = sheet->handle;
                // if(unicodePoint == 'y') {
                //     printf("%s\n", "is y");
                //     Texture tempTex = {};
                //     tempTex.id = glyph->textureHandle;
                //     tempTex.uvCoords = rect2f(q.s0, q.t1, q.s1, q.t0);

                //     renderTextureCentreDim(&tempTex, v2ToV3(v2(100, 100), zAt), v2(100, 100), COLOR_BLACK, 0, mat4TopLeftToBottomLeft(resolution.y), mat4(), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));            
                // }
                // printf("sheet handles: %d ", sheet->handle);
                glyph->q = q;
                
                glyph->index = (int)(text - text_);
                glyph->lastXY = lastXY;
                glyph->unicodePoint = unicodePoint;
            } else {
                assert(!"false");
            }
        }
        
        bool overflowed = (transformToSizeX(x) > margin.maxX && inWord);
        
        if(unicodePoint == ' ' || unicodePoint == '\n') {
            inWord = false;
            hasBeenToNewLine = false;
            drawText = true;
        } else {
            inWord = true;
        }
        
        if(overflowed && !hasBeenToNewLine && inWord) {
            text = tempAt;
            increment = false;
            quadCount = 0;  //empty the quads. We lose the data because we broke the word.  
            hasBeenToNewLine = true;
        }
        
        if(overflowed || unicodePoint == '\n') {
            x = margin.minX;
            y += size*font->fontHeight; 
        }
        
        bool lastCharacter = (*(text + unicodeLen) == '\0');
        
        if(drawText || lastCharacter) {
            for(int i = 0; i < quadCount; ++i) {
                GlyphInfo *glyph = qs + i;
                stbtt_aligned_quad q = glyph->q;
                // if(glyph->unicodePoint == 'y') {
                //     printf("%s\n", "is y after");
                //     Rect2f uvCoords = rect2f(q.s0, q.t1, q.s1, q.t0);
                //     // Rect2f uvCoords = rect2f(q.x0, q.x1, q.y1, q.y0);
                //     error_printFloat4("uv values", uvCoords.E);
                // }
                q.x0 = ((q.x0 - startP.x)*size) + startP.x;
                q.y0 = ((q.y0 - startP.y)*size) + startP.y;
                q.y1 = ((q.y1 - startP.y)*size) + startP.y;
                q.x1 = ((q.x1 - startP.x)*size) + startP.x;
                Rect2f b = rect2f(q.x0, q.y0, q.x1, q.y1);
                bounds = unionRect2f(bounds, b);
                //points[pC++] = b;
                
                if(display) {
                    Texture tempTex = {};
                    tempTex.id = glyph->textureHandle;
                    tempTex.uvCoords = rect2f(q.s0, q.t1, q.s1, q.t0);

                    renderTextureCentreDim(&tempTex, v2ToV3(getCenter(b), zAt), getDim(b), color, 0, mat4TopLeftToBottomLeft(resolution.y), mat4(), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));            

                    // tempTex.id = glyph->textureHandle;
                    // tempTex.uvCoords = rect2f(0, 0, 1, 1);

                    // renderTextureCentreDim(&tempTex, v2ToV3(getCenter(b), zAt), getDim(b), color, 0, mat4TopLeftToBottomLeft(resolution.y), mat4(), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));            

                }
                if(cursorInfo && (glyph->index == cursorInfo->index)) {
                    width = q.x1 - q.x0;
                    height = font->fontHeight*size;//q.y1 - q.y0;
                    pos = v2(transformToSizeX(glyph->lastXY.x) + 0.5f*width, transformToSizeY(glyph->lastXY.y) - 0.5f*height);
                }
                
            }
            
            quadCount = 0; //empty the quads now that we have finished drawing them
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
    // endFontGlyphs(&drawState, bufferWidth, bufferHeight);
    
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

    easyRender_disableScissors(globalRenderGroup);
    
    return bounds;
    
}

Rect2f outputText_with_cursor(Font *font, float x, float y, float z, V2 resolution, char *text, Rect2f margin, V4 color, float size, int cursorIndex, V4 cursorColor, bool display, float resolutionDiffScale) {
    CursorInfo cursorInfo = {};
    cursorInfo.index = cursorIndex;
    cursorInfo.color = cursorColor;
    
    Rect2f result = my_stbtt_print_(font, x, y, z, resolution, text, margin, color, size, &cursorInfo, display, resolutionDiffScale);
    return result;
}

Rect2f outputText(Font *font, float x, float y, float z, V2 resolution, char *text, Rect2f margin, V4 color, float size, bool display, float resolutionDiffScale) {

    Rect2f result = my_stbtt_print_(font, x, y, z, resolution, text, margin, color, size, 0, display, resolutionDiffScale);
    float offset = 3*resolutionDiffScale;
    my_stbtt_print_(font, x + offset, y + offset, z + 0.1f, resolution, text, margin, v4(0.5f, 0.5f, 0.5f, min(color.w, 0.8f)), size, 0, display, resolutionDiffScale);
    return result;
}

Rect2f outputTextNoBacking(Font *font, float x, float y, float z, V2 resolution, char *text, Rect2f margin, V4 color, float size, bool display, float resolutionDiffScale) {

    Rect2f result = my_stbtt_print_(font, x, y, z, resolution, text, margin, color, size, 0, display, resolutionDiffScale);
    return result;
}

Rect2f outputTextWithLength(Font *font, float x, float y, float z, V2 resolution, char *allText, int textLength, Rect2f margin, V4 color, float size, bool display, float resolutionDiffScale) {
    char *text = (char *)malloc(sizeof(char)*(textLength + 1));
    for(int i = 0; i < textLength; ++i) {
        text[i] = allText[i];
    }

    text[textLength] = '\0'; //null terminate. 
    Rect2f result = my_stbtt_print_(font, x, y, z, resolution, text, margin, color, size, 0, display, resolutionDiffScale);
    free(text);
    return result;
}

V2 getBounds(char *string, Rect2f margin, Font *font, float size, V2 resolution, float resolutionDiffScale) {
    Rect2f bounds  = outputText(font, margin.minX, margin.minY, -1, resolution, string, margin, v4(0, 0, 0, 1), size, false, resolutionDiffScale);
    
    V2 result = getDim(bounds);
    return result;
}


Rect2f getBoundsRectf(char *string, float xAt, float yAt, Rect2f margin, Font *font, float size, V2 resolution, float resolutionDiffScale) {
    Rect2f bounds  = outputText(font, xAt, yAt, -1, resolution, string, margin, v4(0, 0, 0, 1), size, false, resolutionDiffScale);
    
    return bounds;
}
