static EasyMesh *easy3d_allocAndInitMesh() {
    EasyMesh *m = pushStruct(&globalLongTermArena, EasyMesh);
    m->vertexCount = 0;
    m->vaoHandle.valid = false;
    
    m->material = 0;
    m->vertexData = initInfinteAlloc(Vertex);
    m->indicesData = initInfinteAlloc(unsigned int);
    
    return m;
}

static void easy3d_addMeshToModel(EasyMesh *mesh, EasyModel *model) {
    assert(model->meshCount < arrayCount(model->meshes));
    model->meshes[model->meshCount++] = mesh;
}

EasyMaterial easyCreateMaterial(Texture *diffuseMap, Texture *normalMap, Texture *specularMap, float constant) {
    EasyMaterial material = {};
    
    material.diffuseMap = diffuseMap;
    material.normalMap = normalMap;
    material.specularMap = specularMap;
    material.specularConstant = constant;
    
    return material;
} 

static float easy3d_getFloat(EasyTokenizer *tokenizer) {
    float result = 0;
    EasyToken t = lexGetNextToken(tokenizer); 
    if(t.type == TOKEN_INTEGER) {
        char *a = nullTerminate(t.at, t.size);
        result = atoi(a);
        free(a);
    } else if(t.type == TOKEN_FLOAT) {
        char *a = nullTerminate(t.at, t.size);
        result = atof(a);
        free(a);
    } else {
        assert(false);
    }
    return result;
}

static int easy3d_getInteger(EasyToken token) {
    char charBuffer[256] = {};
    nullTerminateBuffer(charBuffer, token.at, token.size);

    return atoi(charBuffer);
}

static V3 easy3d_makeVector3(EasyTokenizer *tokenizer) {
    V3 result = v3(0, 0, 0);
    for(int tIndex = 0; tIndex < 3; ++tIndex) {
        result.E[tIndex] = easy3d_getFloat(tokenizer);
    }
    return result;
}

static V2 easy3d_makeVector2(EasyTokenizer *tokenizer) {
    V2 result = v2(0, 0);
    for(int tIndex = 0; tIndex < 2; ++tIndex) {
        result.E[tIndex] = easy3d_getFloat(tokenizer);
    }
    return result;
}

static Texture *easy3d_findTextureWithToken(EasyTokenizer *tokenizer) {
    EasyToken t = lexGetNextToken(tokenizer); 
    assert(t.type == TOKEN_WORD);
    
    char *a = nullTerminate(t.at, t.size);
    Texture *result = findTextureAsset(a);
    assert(result);
    
    free(a);
    return result;
}

static void easy3d_loadMtl(char *fileName) {
    EasyMaterial *mat = 0; //NOTE(ol): this is the current material. There can be more than one marterial per file
    
    FileContents fileContents = getFileContentsNullTerminate(fileName);
    unsigned char *at = fileContents.memory;
    
    
    EasyTokenizer tokenizer = lexBeginParsing(at, (EasyLexOptions)(EASY_LEX_OPTION_EAT_WHITE_SPACE | EASY_LEX_DONT_EAT_SLASH_COMMENTS));
    bool parsing = true;
    
    while(parsing) {
        char *at = tokenizer.src;
        EasyToken token = lexGetNextToken(&tokenizer);
        switch(token.type) {
            case TOKEN_NULL_TERMINATOR: {
                parsing = false;
            } break;
            case TOKEN_WORD: {
                // lexPrintToken(&token);
                if(stringsMatchNullN("newmtl", token.at, token.size)) {
                    EasyToken t = lexGetNextToken(&tokenizer);
                    assert(t.type == TOKEN_WORD);
                    mat = pushStruct(&globalLongTermArena, EasyMaterial);
                    addAssetMaterial(nullTerminate(t.at, t.size), mat);
                }
                if(stringsMatchNullN("Ka", token.at, token.size)) {
                    assert(mat);
                    mat->defaultAmbient = v3ToV4(easy3d_makeVector3(&tokenizer), 1);
                }
                if(stringsMatchNullN("Kd", token.at, token.size)) {
                    assert(mat);
                    mat->defaultDiffuse = v3ToV4(easy3d_makeVector3(&tokenizer), 1);
                }
                if(stringsMatchNullN("Ks", token.at, token.size)) {
                    assert(mat);
                    mat->defaultSpecular = v3ToV4(easy3d_makeVector3(&tokenizer), 1);
                }
                if(stringsMatchNullN("Ns", token.at, token.size)) {
                    assert(mat);
                    mat->specularConstant = easy3d_getFloat(&tokenizer);
                }
                if(stringsMatchNullN("map_Ka", token.at, token.size)) {
                    assert(mat);
                    mat->ambientMap = easy3d_findTextureWithToken(&tokenizer);
                }
                if(stringsMatchNullN("map_Kd", token.at, token.size)) {
                    assert(mat);
                    mat->diffuseMap = easy3d_findTextureWithToken(&tokenizer);
                }
                if(stringsMatchNullN("map_Ks", token.at, token.size)) {
                    assert(mat);
                    mat->specularMap = easy3d_findTextureWithToken(&tokenizer);
                }
                if(stringsMatchNullN("map_Bump", token.at, token.size)) {
                    assert(mat);
                    mat->normalMap = easy3d_findTextureWithToken(&tokenizer);
                }
                if(stringsMatchNullN("map_Ns", token.at, token.size)) {
                    //NOTE(ol): I'm not sure what the ns is 
                }
                if(stringsMatchNullN("d", token.at, token.size)) {
                    float alpha = easy3d_getFloat(&tokenizer);
                    assert(mat);
                    mat->defaultAmbient.w = alpha;
                    mat->defaultDiffuse.w = alpha;
                    mat->defaultSpecular.w = alpha;
                }
                if(stringsMatchNullN("Ts", token.at, token.size)) {
                    float alpha = 1.0f - easy3d_getFloat(&tokenizer);
                    assert(mat);
                    mat->defaultAmbient.w = alpha;
                    mat->defaultDiffuse.w = alpha;
                    mat->defaultSpecular.w = alpha;
                }
            } break;
            case TOKEN_HASH: {
                //eat the comments
                while(*at && !lexIsNewLine(*at)) {
                    at++;
                }
            } break;
            default: {
                //don't mind
            }
        }
    }
}

static void easy3d_parseVertex(EasyTokenizer *tokenizer, EasyMesh *currentMesh, InfiniteAlloc *positionData, InfiniteAlloc *normalData, InfiniteAlloc *uvData) {
    EasyToken token = lexSeeNextToken(tokenizer);
    if(token.type != TOKEN_NEWLINE) {
        Vertex vert = {};
        EasyToken token = lexGetNextToken(tokenizer);
        if(token.type == TOKEN_INTEGER) {
            //NOTE(ol): minus 1 since index starts at 1 for .obj files
            vert.position = *getElementFromAlloc(positionData, easy3d_getInteger(token) - 1, V3);
            token = lexGetNextToken(tokenizer);
        } 
        
        if(token.type == TOKEN_FORWARD_SLASH) {
            token = lexGetNextToken(tokenizer);
            if(token.type == TOKEN_INTEGER) {
                vert.texUV = *getElementFromAlloc(uvData, easy3d_getInteger(token) - 1, V2);
                token = lexGetNextToken(tokenizer);
            }
            
            if(token.type == TOKEN_FORWARD_SLASH) {
                token = lexGetNextToken(tokenizer);
                assert(token.type == TOKEN_INTEGER);
                
                vert.normal = *getElementFromAlloc(normalData, easy3d_getInteger(token) - 1, V3);
            }
        }

        addElementInifinteAlloc_(&currentMesh->vertexData, &vert);
        //NOTE(ol): We just add the indicies buffer incrementally. This is because vertexes might be different. 
        //For an optimized program we would do something more about this @Speed
        addElementInifinteAlloc_(&currentMesh->indicesData, &currentMesh->vertexCount);
        currentMesh->vertexCount++;
    }      
}

static void easy3d_loadObj(char *fileName, EasyModel *model) { 
    FileContents fileContents = getFileContentsNullTerminate(fileName);
    unsigned char *at = fileContents.memory;
    
    //NOTE(ol): These are our 'pools' of information that we can build our final vertex buffers out of 
    InfiniteAlloc positionData = initInfinteAlloc(V3);
    InfiniteAlloc normalData = initInfinteAlloc(V3);
    InfiniteAlloc uvData = initInfinteAlloc(V2);
    
   
    EasyTokenizer tokenizer = lexBeginParsing(at, (EasyLexOptions)(EASY_LEX_DONT_EAT_SLASH_COMMENTS | EASY_LEX_OPTION_EAT_WHITE_SPACE));
    bool parsing = true;
    
    EasyMaterial *mat = 0;
    EasyMesh *currentMesh = 0;
    while(parsing) {
        char *at = tokenizer.src;
        EasyToken token = lexGetNextToken(&tokenizer);
        switch(token.type) {
            case TOKEN_NULL_TERMINATOR: {
                parsing = false;
            } break;
            case TOKEN_WORD: {
                // lexPrintToken(&token);
                if(stringsMatchNullN("f", token.at, token.size)) {
                    if(!currentMesh) { 
                        //NOTE(ol): This is if newmtl isn't sepcified so we create the mesh without a material
                        currentMesh = easy3d_allocAndInitMesh(); 
                        easy3d_addMeshToModel(currentMesh, model);
                        currentMesh->material = 0;
                    }

                    easy3d_parseVertex(&tokenizer, currentMesh, &positionData, &normalData, &uvData);
                    easy3d_parseVertex(&tokenizer, currentMesh, &positionData, &normalData, &uvData);
                    easy3d_parseVertex(&tokenizer, currentMesh, &positionData, &normalData, &uvData);


                    //NOTE(ol): Testing to see if a bigger shape then a triangle is there
                    EasyToken peekToken = lexSeeNextToken(&tokenizer);
                    if(peekToken.type != TOKEN_NEWLINE || peekToken.type != TOKEN_NULL_TERMINATOR) {
                        if(peekToken.type == TOKEN_INTEGER) {   
                            int count = currentMesh->vertexCount;
                            count -= 3;
                            addElementInifinteAlloc_(&currentMesh->indicesData, &count);
                            count += 2;
                            addElementInifinteAlloc_(&currentMesh->indicesData, &count);
                            easy3d_parseVertex(&tokenizer, currentMesh, &positionData, &normalData, &uvData);
                        }
                        // peekToken = lexSeeNextToken(&tokenizer);
                    }
                }
                if(stringsMatchNullN("v", token.at, token.size)) {
                    V3 pos = easy3d_makeVector3(&tokenizer);
                    addElementInifinteAlloc_(&positionData, &pos);
                }
                if(stringsMatchNullN("vt", token.at, token.size)) {
                    V2 uvCoord = easy3d_makeVector2(&tokenizer);
                    addElementInifinteAlloc_(&uvData, &uvCoord);
                }
                if(stringsMatchNullN("vn", token.at, token.size)) {
                    V3 norm = easy3d_makeVector3(&tokenizer);
                    addElementInifinteAlloc_(&normalData, &norm);
                }
                if(stringsMatchNullN("usemtl", token.at, token.size)) {
                    currentMesh = easy3d_allocAndInitMesh(); 
                    pushStruct(&globalLongTermArena, EasyMesh);
                    easy3d_addMeshToModel(currentMesh, model);
                    
                    token = lexGetNextToken(&tokenizer);
                    assert(token.type == TOKEN_WORD);
                    char *name = nullTerminate(token.at, token.size);
                    mat = findMaterialAsset(name);
                    
                    currentMesh->material = mat;
                    if(!mat) {
                        printf("%s %s\n", "couldn't find material:", name);
                    }
                    free(name);
                    
                }
                
            } break;
            case TOKEN_HASH: {
                //eat the comments
                while(*at && !lexIsNewLine(*at)) {
                    at++;
                }
            } break;
            default: {
                //don't mind
            }
        }
    }
    

    for(int meshIndex = 0; meshIndex < model->meshCount; ++meshIndex) {
        EasyMesh *thisMesh = model->meshes[meshIndex];
        initVao(&thisMesh->vaoHandle, (Vertex *)thisMesh->vertexData.memory, thisMesh->vertexData.count, (unsigned int *)thisMesh->indicesData.memory, thisMesh->indicesData.count);
    }
}

void easy3d_imm_renderModel(EasyMesh *mesh,
                            Matrix4 modelMatrix, 
                            Matrix4 viewMatrix,
                            Matrix4 perspectiveMatrix, 
                            RenderProgram *program) {
    
    glUseProgram(program->glProgram);
    renderCheckError();
    
    assert(mesh->vaoHandle.vaoHandle);
    glBindVertexArray(mesh->vaoHandle.vaoHandle);
    renderCheckError();
    
    GLuint modelUniform = glGetUniformLocation(program->glProgram, "M");
    assert(modelUniform);
    GLuint viewUniform = glGetUniformLocation(program->glProgram, "V"); 
    assert(viewUniform);
    GLuint perspectiveUniform = glGetUniformLocation(program->glProgram, "P"); 
    // assert(perspectiveUniform);
    
    glUniformMatrix4fv(modelUniform, 1, GL_FALSE, modelMatrix.val);
    renderCheckError();
    glUniformMatrix4fv(viewUniform, 1, GL_FALSE, viewMatrix.val);
    renderCheckError();
    glUniformMatrix4fv(perspectiveUniform, 1, GL_FALSE, perspectiveMatrix.val);
    renderCheckError();
    
    GLuint vertexAttrib = 0;//getAttribFromProgram(program, "vertex").handle; 
    // assert(vertexAttrib);
    // GLuint normalAttrib = getAttribFromProgram(program, "normal").handle; 
    // assert(normalAttrib);
    
    glEnableVertexAttribArray(vertexAttrib);  
    renderCheckError();
    glVertexAttribPointer(vertexAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), getOffsetForVertex(position)); 
    renderCheckError();
    
    // glEnableVertexAttribArray(normalAttrib);
    // renderCheckError();
    // glVertexAttribPointer(normalAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), getOffsetForVertex(normal));
    // renderCheckError();
    
    glDrawElements(GL_TRIANGLES, mesh->vaoHandle.indexCount, GL_UNSIGNED_INT, 0); //this is the number or verticies for the count. 
    renderCheckError();
    
    glBindVertexArray(0);
    renderCheckError();
    
    glUseProgram(0);
    renderCheckError();
}

static void generateHeightMap(VaoHandle *floorMesh, u32 perlinWidth, u32 perlinHeight) {
    
    // float *perlinWordData = pushArray(&globalPerFrameArena, (unsigned int)(perlinHeight*perlinWidth), float);
    // float perlinWordData[100*100];
    
    InfiniteAlloc floormeshdata = initInfinteAlloc(Vertex);
    InfiniteAlloc indicesData = initInfinteAlloc(unsigned int);
    
    for(s32 y = 0; y < perlinHeight; y++) {
        for(s32 x = 0; x < perlinWidth; x++) {
            s32 subY = y; //this is so the floor starts in the center of the world i.e. has z negative values - can't have negative values since perlin noise looks into an array!!!!
            float height = perlin2d(x, subY, 1, 8);
            // perlinWordData[x + y*perlinWidth] = height;
            float height1 = perlin2d(x + 1, subY, 1, 8);
            float height2 = perlin2d(x, subY + 1, 1, 8);
            V3 p1 = v3(x + 1, height1, subY);
            V3 p2 = v3(x, height2, subY + 1);
            V3 p0 = v3(x, height, subY);
            V3 a = normalizeV3(v3_minus(p1, p0));
            V3 b = normalizeV3(v3_minus(p2, p0));
            
            V3 normal = v3_crossProduct(a, b);
            
            // printf("%f %f %f\n", a.x, a.y, a.z);
            // printf("%f %f %f\n", b.x, b.y, b.z);
            // printf("---------\n");
            
            Vertex v = vertex(p0, normal, v2(0, 0));
            addElementInifinteAlloc_(&floormeshdata, &v);
            if(y < (perlinHeight - 1) && x < (perlinWidth - 1)) { //not on edge
                
                unsigned int index[6] = {
                    (unsigned int)(x + (perlinWidth*y)), 
                    (unsigned int)(x + 1 + (perlinWidth*y)),
                    (unsigned int)(x + 1 + (perlinWidth*(y+1))), 
                    (unsigned int)(x + (perlinWidth*y)), 
                    (unsigned int)(x + 1 + (perlinWidth*(y+1))), 
                    (unsigned int)(x + (perlinWidth*(y+1))) 
                };
                addElementInifinteAllocWithCount_(&indicesData, &index, 6);
            }
            
        }
    }
    
    
    
    initVao(floorMesh, (Vertex *)floormeshdata.memory, floormeshdata.count, (unsigned int *)indicesData.memory, indicesData.count);
}