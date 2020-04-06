

#define PART_SYS_TYPE(FUNC) \
FUNC(PARTICLE_SYS_DEFAULT) \
FUNC(PARTICLE_SYS_CIRCULAR) \
FUNC(PARTICLE_SYS_SCALER) \

typedef enum {
    PART_SYS_TYPE(ENUM)
} ParticleSystemType;

static char *ParticleSystemTypeStrings[] = { PART_SYS_TYPE(STRING) };

#define Particles_DrawGrid 0
struct particle
{
    V3 ddP;
    V3 dP;
    V3 P;
    V4 dColor;
    V4 Color;
    float lifeAt;

    bool dead;

    V3 scale;

    float dA;
    float angle;

    Texture *bitmap;
};

struct particle_cel {
    float Density;
    V3 dP;
};

struct particle_system_settings {
    float LifeSpan;
    float MaxLifeSpan;
    bool Loop;
    
    unsigned int BitmapCount;
    Texture *Bitmaps[32];
    char *BitmapNames[32];
    unsigned int BitmapIndex;
    Rect3f VelBias;
    Rect3f posBias;

    V2 angleBias;
    V2 angleForce;

    bool finished;

    ParticleSystemType type;
    bool collidesWithFloor;

    V3 offsetP;

    float bitmapScale;

    bool pressureAffected;

};

#define CEL_GRID_SIZE 32
#define DEFAULT_MAX_PARTICLE_COUNT 32

struct particle_system {
    particle_cel ParticleGrid[CEL_GRID_SIZE][CEL_GRID_SIZE];
    unsigned int NextParticle;
    unsigned int MaxParticleCount;
    int particleCount;
    
    particle Particles[DEFAULT_MAX_PARTICLE_COUNT];
    particle_system_settings Set;
    
    Timer creationTimer; 

    ProjectionType viewType;

    bool Active;
};


inline void pushParticleBitmap(particle_system_settings *Settings, Texture *Bitmap, char *name) {
    assert(Settings->BitmapCount < arrayCount(Settings->Bitmaps));
    int indexAt = Settings->BitmapCount++;
    Settings->Bitmaps[indexAt] = Bitmap;
    Settings->BitmapNames[indexAt] = name;
    
}

inline void removeParticleBitmap(particle_system_settings *Settings, int index) {
    if(Settings->BitmapCount > 0) {
        int indexAt = --Settings->BitmapCount;
        Settings->Bitmaps[index] = Settings->Bitmaps[indexAt];
        Settings->BitmapNames[index] = Settings->BitmapNames[indexAt];
    }
    
}

internal inline particle_system_settings InitParticlesSettings(ParticleSystemType type) {
    particle_system_settings Set = {};
    Set.MaxLifeSpan = Set.LifeSpan = 3.0f;
    Set.bitmapScale = 0.8f;
    Set.type = type;
    Set.pressureAffected = true;
    return Set;
}

internal inline void InitParticleSystem(particle_system *System, particle_system_settings *Set, unsigned int MaxParticleCount = DEFAULT_MAX_PARTICLE_COUNT) {
    memset(System, 0, sizeof(particle_system));
    System->Set = *Set;
    System->MaxParticleCount = DEFAULT_MAX_PARTICLE_COUNT;
    //System->Active = true;
    //System->Set.Loop = true;
    //This is saying we create two particles per frame 
    System->creationTimer = initTimer(1.0f / 120.0f, false); //default to creating particle every 1 / 120 th of a second. 
    System->viewType = PERSPECTIVE_MATRIX;
}

inline void Reactivate(particle_system *System) {
    System->Set.LifeSpan = System->Set.MaxLifeSpan;
    if(!System->Set.Loop) {
        System->particleCount = 0;    
    }
    
    System->Active = true;
}

inline void setParticleLifeSpan(particle_system *partSys, float value) {
    partSys->creationTimer.period = value / partSys->MaxParticleCount;
}

internal inline void drawAndUpdateParticleSystem(RenderGroup *group, particle_system *System, V3 Origin, float dt, V3 Acceleration, V4 particleTint, bool render) {
    BlendFuncType blendFuncSaved;
    EasyRender_DepthFunc depthFuncSaved;
    if(group) {
        //NOTE(ollie): Additive blending for particles. Not sure if this fully works?
        blendFuncSaved = setBlendFuncType(group, BLEND_FUNC_ONE_ONE_ADDITIVE);

        depthFuncSaved = easyRender_setDepthFuncType(group, RENDER_DEPTH_FUNC_ALWAYS);
    }
    if(System->Active) {
        float particleLifeSpan = System->creationTimer.period*System->MaxParticleCount;

        float GridScale = 0.4f;
        float Inv_GridScale = 1.0f / GridScale;
        
        V3 CelGridOrigin = v3(0, 0, 0);
    
        int particlesToCreate = 0;        

        if(!System->Set.finished) {
            if(System->Set.type == PARTICLE_SYS_DEFAULT || System->Set.type == PARTICLE_SYS_SCALER) {        
                
                Timer *timer = &System->creationTimer;
                timer->value_ += dt;        
                if(timer->value_ >= timer->period) {
                    particlesToCreate = (int)(timer->value_ / timer->period);
                    assert(particlesToCreate > 0);
                    timer->value_ = 0;

                }
                if(System->particleCount == 0) {
                    particlesToCreate = System->MaxParticleCount;
                }

                //create new particles for the frame
                for(int ParticleIndex = 0;
                    ParticleIndex < particlesToCreate;
                    ++ParticleIndex)
                {
                    particle_system_settings *Set = &System->Set;
                    particle *Particle = System->Particles + System->NextParticle++;
                    Particle->Color = particleTint;
                        
                    Particle->dead = false;
                    //NOTE(oliver): Paricles start with motion 
                    Particle->scale = v3(2, 2, 2);
                    Particle->P = v3(randomBetween(System->Set.posBias.min.x, System->Set.posBias.max.x),
                                     randomBetween(System->Set.posBias.min.y, System->Set.posBias.max.y),
                                     randomBetween(System->Set.posBias.min.z, System->Set.posBias.max.z));

                    Particle->dP = v3(randomBetween(System->Set.VelBias.min.x, System->Set.VelBias.max.x),
                                      randomBetween(System->Set.VelBias.min.y, System->Set.VelBias.max.y),
                                      randomBetween(System->Set.VelBias.min.z, System->Set.VelBias.max.z));

                    Particle->ddP = Acceleration;
                    Particle->lifeAt = 0;

                    Particle->angle = randomBetween(System->Set.angleBias.x, System->Set.angleBias.y);
                    Particle->dA = randomBetween(System->Set.angleForce.x, System->Set.angleForce.y);

                    Particle->bitmap = 0;
                    if(Set->BitmapCount > 0) {
                        Particle->bitmap = Set->Bitmaps[Set->BitmapIndex++];
                        
                        if(Set->BitmapIndex >= Set->BitmapCount) {
                            Set->BitmapIndex = 0;
                        }
                    }

                    if(System->particleCount < System->NextParticle) {
                        System->particleCount = System->NextParticle;
                    }
                    Particle->dColor = v4_scale(0.002f, v4(1, 1, 1, 1));
                    if(System->NextParticle == System->MaxParticleCount) {
                        System->NextParticle = 0;
                    }
                }
            } else if(System->Set.type == PARTICLE_SYS_CIRCULAR) {
                particleLifeSpan = System->Set.MaxLifeSpan; //seting up the particle system
                if(System->particleCount == 0) {
                    float dTheta = (float)TAU32 / (float)System->MaxParticleCount;
                    
                    float theta = 0;
                    for(int i = 0; i < System->MaxParticleCount; ++i) {
                        V2 dp = v2_scale(1, v2(cos(theta), sin(theta)));
                        theta += dTheta;
                        particle *Particle = System->Particles + i;
                        Particle->dead = false;
                        Particle->Color = v4(1, 1, 1, 1);
                        Particle->P = v3(0, 0, 0);
                        Particle->dP = v2ToV3(dp, 0);
                        Particle->ddP = Acceleration;
                        Particle->dColor = v4_scale(0.002f, v4(1, 1, 1, 1));
                        Particle->lifeAt = 0;
                    }
                    System->particleCount = System->MaxParticleCount;
                }
            }
        }
         
        memset(&System->ParticleGrid, 0, sizeof(System->ParticleGrid));
        
        float halfGridWidth = 0.5f*GridScale*CEL_GRID_SIZE;
        float halfGridHeight = 0.5f*GridScale*CEL_GRID_SIZE;
        for(unsigned int ParticleIndex = 0;
            ParticleIndex < System->particleCount;
            ++ParticleIndex)
        {

            particle *Particle = System->Particles + ParticleIndex;
            
            V3 P = v3_scale(Inv_GridScale, Particle->P);
            
            int CelX = (int)(P.x + halfGridWidth);
            int CelY = (int)(P.y + halfGridHeight);

            //assert(CelX >= -1);
            if(CelX < 0) {
                CelX = 0;
            }
            if(CelY < 0) {
                CelY = 0;
            }
            
            if(CelX >= CEL_GRID_SIZE){ CelX = CEL_GRID_SIZE - 1;}
            if(CelY >= CEL_GRID_SIZE){ CelY = CEL_GRID_SIZE - 1;}
            assert(CelX >= 0 && CelX < CEL_GRID_SIZE && CelY >= 0 && CelY < CEL_GRID_SIZE);
            
            particle_cel *Cel = &System->ParticleGrid[CelY][CelX];
            
            float ParticleDensity = Particle->Color.w;
            Cel->Density += ParticleDensity;
            Cel->dP = v3_plus(v3_scale(ParticleDensity, Particle->dP), Cel->dP);
        }
        
#if Particles_DrawGrid
        {
            //NOTE(oliver): To draw Eulerian grid
            
            for(unsigned int Y = 0;
                Y < CEL_GRID_SIZE;
                ++Y)
            {
                for(unsigned int X = 0;
                    X < CEL_GRID_SIZE;
                    ++X)
                {
                    assert(X >= 0 && X < CEL_GRID_SIZE && Y >= 0 && Y < CEL_GRID_SIZE);
                    particle_cel *Cel = &System->ParticleGrid[Y][X];
                    
                    V3 P = CelGridOrigin + v3(GridScale*(float)X, GridScale*(float)Y, 0);
                    
                    float Density = 0.1f*Cel->Density;
                    V4 Color = v4(Density, Density, Density, 1);
                    Rect2f Rect = rect2MinDim(P.XY, v2(GridScale, GridScale));
                    //PushRectOutline(RenderGroup, Rect, 1,Color);
                }
            }
        }
#endif
        int deadCount = 0;
        V3 ddP = {};
        for(unsigned int ParticleIndex = 0;
            ParticleIndex < System->particleCount;
            ++ParticleIndex)
        {
            particle *Particle = System->Particles + ParticleIndex;

            if(Particle->dead) {
                deadCount++;
            } else {
                V3 P = v3_scale(Inv_GridScale, Particle->P);
                
                if(System->Set.pressureAffected) {
                    int CelX = (int)(P.x);
                    int CelY = (int)(P.y);
                    
                    if(CelX >= CEL_GRID_SIZE - 1){ CelX = CEL_GRID_SIZE - 2;}
                    if(CelY >= CEL_GRID_SIZE - 1){ CelY = CEL_GRID_SIZE - 2;}
                    
                    if(CelX < 1){ CelX = 1;}
                    if(CelY < 1){ CelY = 1;}
                    
                    assert(CelX >= 0 && CelX < CEL_GRID_SIZE && CelY >= 0 && CelY < CEL_GRID_SIZE);

                    particle_cel *CelCenter = &System->ParticleGrid[CelY][CelX];
                    particle_cel *CelLeft = &System->ParticleGrid[CelY][CelX - 1];
                    particle_cel *CelRight = &System->ParticleGrid[CelY][CelX + 1];
                    particle_cel *CelUp = &System->ParticleGrid[CelY + 1][CelX];
                    particle_cel *CelDown = &System->ParticleGrid[CelY - 1][CelX];
                    
                    
                    V3 VacumDisplacement = Acceleration;//V3(0, 0, 0);
                    float DisplacmentCoeff = 0.6f;
                    if(System->Set.type != PARTICLE_SYS_CIRCULAR) { //don't have vacuum displacement dor circular particle effects
                        VacumDisplacement = v3_plus(VacumDisplacement, v3_scale((CelCenter->Density - CelRight->Density), v3(1, 0, 0)));
                        VacumDisplacement = v3_plus(VacumDisplacement, v3_scale((CelCenter->Density - CelLeft->Density), v3(-1, 0, 0)));
                        VacumDisplacement = v3_plus(VacumDisplacement, v3_scale((CelCenter->Density - CelUp->Density), v3(0, 1, 0)));
                        VacumDisplacement = v3_plus(VacumDisplacement, v3_scale((CelCenter->Density - CelDown->Density), v3(0, -1, 0)));
                    }

                    ddP = v3_plus(Particle->ddP, v3_scale(DisplacmentCoeff, VacumDisplacement));
                }

                
                if(System->Set.type == PARTICLE_SYS_SCALER) {
                    Particle->scale = v3_plus(v3_scale(dt, Particle->dP), Particle->scale);
                } else {
                    //NOTE(oliver): Move particle
                    Particle->P = v3_plus(v3_plus(v3_scale(0.5f*sqr(dt), ddP),  v3_scale(dt, Particle->dP)),  Particle->P);
                    Particle->dP = v3_plus(v3_scale(dt, ddP), Particle->dP);
                }

                Particle->angle += Particle->dA*dt;
                //Particle->dA = v3_plus(v3_scale(dt, ddA), Particle->dA);
                
                //NOTE(oliver): Collision with ground
                if(System->Set.collidesWithFloor) {
                    if(Particle->P.y < 0.0f)
                    {
                        float CoefficentOfResitution = 0.5f;
                        Particle->P.y = 0.0f;
                        Particle->dP.y = -Particle->dP.y*CoefficentOfResitution;
                        Particle->dP.x *= 0.8f;
                    }
                }
                
                //NOTE(oliver): Color update
                Particle->Color = v4_plus(v4_scale(dt, Particle->dColor), Particle->Color);
                V4 Color = Particle->Color;
                
                ///////////////////////************ Update the alpha*************////////////////////
                Particle->lifeAt += dt;
                float minThreshold = 0.4f;
                float maxThreshold = 0.8f;

                float t = 1.0f;
                float life_01 = (Particle->lifeAt / particleLifeSpan);
                if(life_01 < minThreshold) {
                    t =  life_01 / minThreshold;
                }
                
                if(life_01 > maxThreshold) {
                    t =  (life_01 - maxThreshold) / (1.0f - maxThreshold);
                    t = 1.0f - t;
                }

                Color.w = clamp01(t);

                ////////////////////////////////////////////////////////////////////
               
                
                particle_system_settings *Set = &System->Set;
                
                Texture *Bitmap = Particle->bitmap;
                
                
                if(Particle->lifeAt >= particleLifeSpan) {
                    Particle->dead = true;
                }
                if(Bitmap && render) {

                    V3 scale = v3(Particle->scale.x*Set->bitmapScale, Particle->scale.y*Set->bitmapScale, 1);
                    //NOTE(ollie): Set the scale 
                    Matrix4 T = Matrix4_scale(mat4(), scale);

                    //NOTE(ollie): Set the rotation
                    T = Mat4Mult(mat4_axisAngle(Z_AXIS, Particle->angle), T);
                    
                    //NOTE(ollie): Add translation componet
                    T = Matrix4_translate(T, v3_plus(Particle->P, Origin));

                    //NOTE(ollie): Set the model transform
                    setModelTransform(globalRenderGroup, T);

                    //NOTE(ollie): Draw the sprite
                    renderDrawSprite(group, Bitmap, Color);
                    // renderTextureCentreDim(Bitmap, renderInfo.pos, renderInfo.dim.xy, Color, Particle->angle, mat4(), renderInfo.pvm, screenMatrix);    
                } else {
                    //renderDrawRing(&Particle->renderHandle, renderInfo.pos, renderInfo.dim.xy, Color, mat4(), renderInfo.pvm, screenMatrix);                
                }
            }
        }
        System->Set.LifeSpan -= dt;
        if(System->Set.LifeSpan <= 0.0f) {
            //NOTE(ollie): Don't create anymore particles
            System->Set.finished = true;            
            
        }

        if(System->Set.finished && deadCount == System->particleCount) {
            if(System->Set.Loop) {
                Reactivate(System);
            } else {
                System->Active = false;
            }
            System->Set.finished = false;
        }
    }

    if(group) {
        //NOTE(ollie): Reinistate the blend function
        setBlendFuncType(group, blendFuncSaved); 
        //NOTE(ollie): Reinistate the depth function
        easyRender_setDepthFuncType(group, depthFuncSaved);   
    }
}

void prewarmParticleSystem(particle_system *System, V3 Acceleration) {
    float dtLeft = System->Set.LifeSpan;
    float dtUpdate = 1.0f / 15.0f;
    while(dtLeft > 0) {
        drawAndUpdateParticleSystem(0, System, v3(0, 0, 0), dtUpdate, Acceleration, COLOR_WHITE, false);
        dtLeft -= dtUpdate;
        if(dtLeft < dtUpdate) {
            dtUpdate = dtLeft;
        }
    }
    Reactivate(System);
}

///////////////////////************* Ollie's new file for particle systems ************////////////////////
static inline void EasyParticles_SaveParticleSystem(char *fileName, particle_system *ps) {

    InfiniteAlloc fileContents = initInfinteAlloc(char);

    particle_system_settings *set = &ps->Set;

    // float MaxLifeSpan;
    // bool Loop;

    // VAR_NULL,
    // VAR_CHAR_STAR,
    // VAR_LONG_UNSIGNED_INT,
    // VAR_LONG_INT,
    // VAR_INT,
    // VAR_FLOAT,
    // VAR_V2,
    // VAR_V3,
    // VAR_V4,
    // VAR_BOOL,
    
    // unsigned int BitmapCount;
    // char *BitmapNames[32];

    // Rect2f VelBias;
    // Rect2f posBias;
    // V2 angleBias;
    // V2 angleForce;
    // ParticleSystemType type;
    // bool collidesWithFloor;
    // V3 offsetP;
    // float bitmapScale;
    // bool pressureAffected;
    // unsigned int MaxParticleCount;
    // creationTimer.period; 
    // bool Active;


    // addVar(&fileContents, &set->LifeSpan, "LifeSpan", VAR_FLOAT);
    // addVar(&fileContents, &set->MaxLifeSpan, "MaxLifeSpan", VAR_FLOAT);
    // addVar(&fileContents, &set->Loop, "Loop", VAR_BOOL);
    // addVarArray(&fileContents, &set->Loop, "Loop", VAR_BOOL );
    // addVar(&fileContents, e->model->name, "modelName", VAR_CHAR_STAR);

    // EasyTransform *T = &e->T;
    // addVar(&fileContents, T->pos.E, "position", VAR_V3);
    // addVar(&fileContents, T->Q.E, "rotation", VAR_V4);
    // addVar(&fileContents, T->scale.E, "scale", VAR_V3);
    // addVar(&fileContents, e->colorTint.E, "color", VAR_V4);


    // ///////////////////////************* End the buffer************////////////////////
    // char buffer[32];
    // sprintf(buffer, "}\n\n");
    // addElementInifinteAllocWithCount_(&fileContents, buffer, strlen(buffer));
                

    // ///////////////////////************ Write the file to disk *************////////////////////

    // char *folderPath = concat(globalExeBasePath, "levels/");
    // char *writeName = concat(folderPath, levelName);
    
    // game_file_handle handle = platformBeginFileWrite(writeName);
    // platformWriteFile(&handle, fileContents.memory, fileContents.count*fileContents.sizeOfMember, 0);
    // platformEndFile(handle);

    // ///////////////////////************* Clean up the memory ************////////////////////   

    // releaseInfiniteAlloc(&fileContents);

    // free(writeName);
    // free(folderPath);
}

static inline void EasyParticles_LoadParticleSystem(char *fileName) {
    
}

