#define SCENE_TRANSITION_TIME 1.0f

typedef void transition_callback(void *data);

typedef enum {
    EASY_TRANSITION_BLINDERS,
    EASY_TRANSITION_FADE
} EasyTransitionType;

typedef struct EasySceneTransition EasySceneTransition;
typedef struct EasySceneTransition {
    transition_callback *callback;
    void *data;

    Timer timer;
    bool direction; //true for in //false for out

    EasyTransitionType type;

    EasySceneTransition *next;
} EasySceneTransition;

typedef struct {
    EasySceneTransition *freeListTransitions;
    EasySceneTransition *currentTransition;

    WavFile *transitionSound;
    Arena *soundArena;
    Arena *longTermArena;
} EasyTransitionState;

static EasyTransitionState *EasyTransition_initTransitionState(WavFile *soundToPlay) {

    EasyTransitionState *result = pushStruct(&globalLongTermArena, EasyTransitionState);

    result->freeListTransitions = 0;
    result->currentTransition = 0;
    result->transitionSound = soundToPlay;
    result->soundArena = &globalLongTermArena;
    result->longTermArena = &globalLongTermArena;

    return result;
}

EasySceneTransition *EasyTransition_PushTransition(EasyTransitionState *state, transition_callback *callback, void *data, EasyTransitionType type) {
    EasySceneTransition *trans = state->freeListTransitions;
    if(trans) {
        //pull off free list. 
        state->freeListTransitions = trans->next;
    } else {
        trans = pushStruct(state->longTermArena, EasySceneTransition);
    }

    if(state->transitionSound) {
        playSound(state->soundArena, state->transitionSound, 0, AUDIO_FOREGROUND);    
    }
    

    trans->timer = initTimer(SCENE_TRANSITION_TIME, false);
    trans->data = data;
    trans->callback = callback;
    trans->direction = true;
    trans->type = type;

    trans->next = state->currentTransition;

    state->currentTransition = trans;

    return trans;
}

static bool EasyTransition_InTransition(EasyTransitionState *transState) {
    EasySceneTransition *trans = transState->currentTransition;
    
    return (trans);
}

static bool EasyTransition_updateTransitions(EasyTransitionState *transState, V2 resolution, float dt) {
    EasySceneTransition *trans = transState->currentTransition;
    bool result = false;
    if(trans) {
        result = true;
        TimerReturnInfo timeInfo = updateTimer(&trans->timer, dt);
        float halfScreen = 0.5f*resolution.x;

        //this should really be a smoothstep00 and somehow find out when we reach halfway to do the close interval. This could be another timer. 
        float tValue = timeInfo.canonicalVal;
        if(!trans->direction) {
            tValue = 1.0f - timeInfo.canonicalVal;
        }

        switch(trans->type) {
            case EASY_TRANSITION_BLINDERS: {
                float transWidth = smoothStep01(0, tValue, halfScreen);   
                //These are the black rects to make it look like a shutter opening and closing. 
                V2 halfRes = v2_scale(0.5f, resolution);
                Rect2f rect1 = rect2f(-halfRes.x, -halfRes.y, -halfRes.x + transWidth, halfRes.y + resolution.y);
                Rect2f rect2 = rect2f(halfRes.x - transWidth, -halfRes.y, halfRes.x, halfRes.y);
                renderDrawRect(rect1, NEAR_CLIP_PLANE, COLOR_BLACK, 0, mat4(), OrthoMatrixToScreen(resolution.x, resolution.y));                    
                renderDrawRect(rect2, NEAR_CLIP_PLANE, COLOR_BLACK, 0, mat4(), OrthoMatrixToScreen(resolution.x, resolution.y));                    
                
            } break;
            case EASY_TRANSITION_FADE: {
                renderDrawRect(rect2f(0, 0, resolution.x, resolution.y), NEAR_CLIP_PLANE, v4(0, 0, 0, tValue), 0, mat4(), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y));                    
            } break;
            default: {
                assert(false);
            }

        }
        

        if(timeInfo.finished) {
            if(trans->direction) {
                assert(trans->data);
                trans->callback(trans->data);

                trans->direction = false;
                turnTimerOn(&trans->timer);
            } else {
                //finished the transition
                transState->currentTransition = trans->next;
                trans->next = transState->freeListTransitions;
                transState->freeListTransitions = trans;
            }
        }
    
    }
    return result;
}