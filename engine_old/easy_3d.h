static EasyMesh *easy3d_allocAndInitMesh() {
    EasyMesh *m = pushStruct(&globalLongTermArena, EasyMesh);
    m->vertexCount = 0;
    m->vaoHandle.valid = false;
    m->bounds = InverseInfinityRect3f();
    
    m->material = 0;
    m->vertexData = initInfinteAlloc(Vertex);
    m->indicesData = initInfinteAlloc(unsigned int);
    
    return m;
}

static void easy3d_addMeshToModel(EasyMesh *mesh, EasyModel *model) {
    assert(model->meshCount < arrayCount(model->meshes));
    model->meshes[model->meshCount++] = mesh;
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

typedef enum {
    EASY_FILE_NAME_RELATIVE,
    EASY_FILE_NAME_GLOBAL
} EasyFile_NameType;

static void easy3d_loadMtl(char *fileName, EasyFile_NameType type) {

    if(type == EASY_FILE_NAME_RELATIVE) {
        fileName = concatInArena(globalExeBasePath, fileName, &globalPerFrameArena);
    }
    EasyMaterial *mat = 0; //NOTE(ol): this is the current material. There can be more than one marterial per file
    
    FileContents fileContents = getFileContentsNullTerminate(fileName);
    
    EasyTokenizer tokenizer = lexBeginParsing(fileContents.memory, (EasyLexOptions)(EASY_LEX_OPTION_EAT_WHITE_SPACE | EASY_LEX_DONT_EAT_SLASH_COMMENTS));
    bool parsing = true;
    
    bool hadKa = false;
    bool hadKd = false;
    bool hadKs = false;

    bool hadKaMap = false;
    bool hadKdMap = false;
    bool hadKsMap = false;

    // @Robustness
    //NOTE: Reference in obj file, and used to create unique material. Note: It may not be unique still
    //Will be unique only if all files are in the same directory!!
    char *materialFileName = getFileLastPortionWithArena(fileName, &globalPerFrameArena);

    while(parsing) {
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
                    easy3d_initMaterial(mat);

                    //NOTE: Reset 
                    hadKd = false;

                    hadKdMap = false;
                    hadKsMap = false;
                    //

                    //TODO: Add folder prefix for unique names 
                    char *materialName = nullTerminateArena(t.at, t.size, &globalPerFrameArena);
                    char *uniqueName = concatInArena(materialFileName, materialName, &globalLongTermArena);
                    mat->name = uniqueName;
                    addAssetMaterial(uniqueName, mat);
                }
                if(stringsMatchNullN("Ka", token.at, token.size)) {
                    // if(!hadKdMap && !hadKd) {
                        assert(mat);
                        // mat->defaultAmbient.xyz = easy3d_makeVector3(&tokenizer);
                    // }
                }
                if(stringsMatchNullN("Kd", token.at, token.size)) {
                    if(!hadKdMap) {
                        hadKd = true;
                        assert(mat);
                        mat->defaultDiffuse.xyz = easy3d_makeVector3(&tokenizer);
                        mat->defaultAmbient.xyz = v3(0, 0, 0);
                    }
                }
                if(stringsMatchNullN("Ks", token.at, token.size)) {
                    if(!hadKsMap) {
                        assert(mat);
                        mat->defaultSpecular.xyz = easy3d_makeVector3(&tokenizer);
                    }
                }
                if(stringsMatchNullN("Ns", token.at, token.size)) {
                    assert(mat);
                    mat->specularConstant = easy3d_getFloat(&tokenizer);
                }
                if(stringsMatchNullN("map_Ka", token.at, token.size)) {
                    assert(mat);
                    printf("%s\n", "Has ambient map, but engine only supports using diffuse maps as ambient maps");
                    // mat->ambientMap = easy3d_findTextureWithToken(&tokenizer);
                    // if(hadKa) {
                    //     mat->defaultAmbient.xyz = v3(1, 1, 1);
                    // }
                }
                if(stringsMatchNullN("map_Kd", token.at, token.size)) {
                    assert(mat);
                    mat->diffuseMap = easy3d_findTextureWithToken(&tokenizer);
                    mat->defaultAmbient.xyz = mat->defaultDiffuse.xyz = v3(1, 1, 1);
                    hadKdMap = true;
                }
                if(stringsMatchNullN("map_Ks", token.at, token.size)) {
                    assert(mat);
                    mat->specularMap = easy3d_findTextureWithToken(&tokenizer);
                    mat->defaultSpecular.xyz = v3(1, 1, 1);
                    hadKsMap = true;
                }
                if(stringsMatchNullN("map_Bump", token.at, token.size)) {
                    assert(mat);
                    mat->normalMap = easy3d_findTextureWithToken(&tokenizer);
                }
                if(stringsMatchNullN("map_Ns", token.at, token.size)) {
                    //NOTE(ol): This is a shiniess map instad of using the specular constant
                    //@Support: NOT SUPPORTING AT THE MOMENT 
                }
                if(stringsMatchNullN("d", token.at, token.size)) {
                    float alpha = easy3d_getFloat(&tokenizer);
                    assert(mat);
                    mat->defaultAmbient.w = alpha;
                    mat->defaultDiffuse.w = alpha;
                }
                if(stringsMatchNullN("Ts", token.at, token.size)) {
                    float alpha = 1.0f - easy3d_getFloat(&tokenizer);
                    assert(mat);
                    mat->defaultAmbient.w = alpha;
                    mat->defaultDiffuse.w = alpha;
                }
            } break;
            case TOKEN_HASH: {
                //eat the comments
                while(*tokenizer.src != '\0' && !lexIsNewLine(*tokenizer.src)) {
                    tokenizer.src++;
                }
            } break;
            default: {
                //don't mind
            }
        }
    }
}

#define easy3d_loadMtl_withGlobalName(fileName) easy3d_loadMtl(fileName, EASY_FILE_NAME_GLOBAL)
#define easy3d_loadMtl_withRelativeName(fileName) easy3d_loadMtl(fileName, EASY_FILE_NAME_RELATIVE)

static void easy3d_parseVertex(EasyTokenizer *tokenizer, EasyMesh *currentMesh, InfiniteAlloc *positionData, InfiniteAlloc *normalData, InfiniteAlloc *uvData) {
    EasyToken token = lexSeeNextToken(tokenizer);
    if(token.type == TOKEN_INTEGER || token.type == TOKEN_FORWARD_SLASH) {
        Vertex vert = {};
        if(token.type == TOKEN_INTEGER) {
            token = lexGetNextToken(tokenizer);
            //NOTE(ol): minus 1 since index starts at 1 for .obj files
            vert.position = *getElementFromAlloc(positionData, easy3d_getInteger(token) - 1, V3);
            currentMesh->bounds = unionRect3f(currentMesh->bounds, v3_to_rect3f(vert.position));
            token = lexSeeNextToken(tokenizer);
        } 
        
        if(token.type == TOKEN_FORWARD_SLASH) {
            token = lexGetNextToken(tokenizer);

            token = lexSeeNextToken(tokenizer);
            if(token.type == TOKEN_INTEGER) {
                token = lexGetNextToken(tokenizer);
                vert.texUV = *getElementFromAlloc(uvData, easy3d_getInteger(token) - 1, V2);
                token = lexSeeNextToken(tokenizer);
            }
            
            if(token.type == TOKEN_FORWARD_SLASH) {
                token = lexGetNextToken(tokenizer);
                token = lexGetNextToken(tokenizer);
                assert(token.type == TOKEN_INTEGER);
                
                vert.normal = *getElementFromAlloc(normalData, easy3d_getInteger(token) - 1, V3);
            }
        } else {
            //just to see if there are any just vertex faces. 

            assert(false);
        }

        addElementInifinteAlloc_(&currentMesh->vertexData, &vert);
        //NOTE(ol): We just add the indicies buffer incrementally. This is because vertexes might be different. 
        //For an optimized program we would do something more about this @Speed
        addElementInifinteAlloc_(&currentMesh->indicesData, &currentMesh->vertexCount);
        currentMesh->vertexCount++;
    }      
}

static EasyModel *easy3d_loadObj(char *fileName, EasyModel *model, EasyFile_NameType nameType) { 
    if(nameType == EASY_FILE_NAME_RELATIVE) {
        fileName = concatInArena(globalExeBasePath, fileName, &globalPerFrameArena);
    }
    FileContents fileContents = getFileContentsNullTerminate(fileName);
    if(!model) { //NOTE(ollie): no model passed in
        model = pushStruct(&globalLongTermArena, EasyModel);
        
    }

    //NOTE(ollie): add model to catalog, so add it to asset catalog
    //NOTE(ollie): This function takes the last portion, so don't need to do this. 
    // Although this means it won't be unique. 
    addAssetModel(fileName, model); 

    model->bounds = InverseInfinityRect3f();

    model->name = getFileLastPortion(fileName);
    
    //NOTE(ol): These are our 'pools' of information that we can build our final vertex buffers out of 
    InfiniteAlloc positionData = initInfinteAlloc(V3);
    InfiniteAlloc normalData = initInfinteAlloc(V3);
    InfiniteAlloc uvData = initInfinteAlloc(V2);
    
    EasyTokenizer tokenizer = lexBeginParsing(fileContents.memory, (EasyLexOptions)(EASY_LEX_DONT_EAT_SLASH_COMMENTS | EASY_LEX_OPTION_EAT_WHITE_SPACE));
    bool parsing = true;
    
    EasyMaterial *mat = 0;
    EasyMesh *currentMesh = 0;
    char *mtlFileName = "";
    while(parsing) {
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
                    int beginCount = currentMesh->vertexCount - 3;

                    while(peekToken.type == TOKEN_INTEGER) {  //More than 3 verticies 
                        
                        addElementInifinteAlloc_(&currentMesh->indicesData, &beginCount);
                        int count =  currentMesh->vertexCount - 1;
                        addElementInifinteAlloc_(&currentMesh->indicesData, &count);
                        easy3d_parseVertex(&tokenizer, currentMesh, &positionData, &normalData, &uvData);
                        peekToken = lexSeeNextToken(&tokenizer);
                        //assert(peekToken.type != TOKEN_INTEGER);
                    }
                    // peekToken = lexSeeNextToken(&tokenizer);
                } else if(stringsMatchNullN("v", token.at, token.size)) {
                    V3 pos = easy3d_makeVector3(&tokenizer);

                    model->bounds = unionRect3f(model->bounds, v3_to_rect3f(pos));
                    addElementInifinteAlloc_(&positionData, &pos);
                } else if(stringsMatchNullN("vt", token.at, token.size)) {
                    V2 uvCoord = easy3d_makeVector2(&tokenizer);
                    addElementInifinteAlloc_(&uvData, &uvCoord);
                } else if(stringsMatchNullN("vn", token.at, token.size)) {
                    V3 norm = easy3d_makeVector3(&tokenizer);
                    addElementInifinteAlloc_(&normalData, &norm);
                } else if(stringsMatchNullN("mtllib", token.at, token.size)) {
                    token = lexGetNextToken(&tokenizer);
                    assert(token.type == TOKEN_WORD);
                    mtlFileName = nullTerminateArena(token.at, token.size, &globalPerFrameArena);
                } else if(stringsMatchNullN("usemtl", token.at, token.size)) {
                    currentMesh = easy3d_allocAndInitMesh(); 
                    easy3d_addMeshToModel(currentMesh, model);
                    
                    token = lexGetNextToken(&tokenizer);
                    assert(token.type == TOKEN_WORD);
                    char *name = nullTerminate(token.at, token.size);
                    mat = findMaterialAsset(concatInArena(mtlFileName, name, &globalPerFrameArena));
                    
                    currentMesh->material = mat;
                    if(!mat) {
                        printf("%s %s\n", "couldn't find material:", name);
                    }
                    free(name);
                    
                }
                
            } break;
            case TOKEN_HASH: {
                //eat the comments
                while(*tokenizer.src != '\0' && !lexIsNewLine(*tokenizer.src)) {
                    tokenizer.src++;
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
    return model;
}

#define easy3d_loadObj_withRelativeName(fileName, model) easy3d_loadObj(fileName, model, EASY_FILE_NAME_RELATIVE)
#define easy3d_loadObj_withGlobalName(fileName, model) easy3d_loadObj(fileName, model, EASY_FILE_NAME_GLOBAL)

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
