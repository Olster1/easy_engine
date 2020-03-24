
#define LAYER0 0
#define LAYER1 0.1f
#define LAYER2 0.2f
#define LAYER3 0.3f
#define LAYER4 0.8f

static float globalTileScale = 0.9f;

#define MAX_LANE_COUNT 5
#define MY_ROOM_HEIGHT 5
#define DEBUG_ENTITY_COLOR 0

#define MY_PLAYER_MAX_AMMO_COUNT 20

#define CHOC_INCREMENT 0.1f

static bool DEBUG_global_movePlayer = true;

typedef enum {
    MOVE_VIA_NULL,
    MOVE_VIA_TELEPORT,
    MOVE_VIA_HURT,
} PlayerMoveViaType; 

#define MY_BOSS_TYPE(FUNC) \
FUNC(ENTITY_BOSS_NULL)\
FUNC(ENTITY_BOSS_FIRE)\
FUNC(ENTITY_BOSS_DUPLICATE)\

typedef enum {
    MY_BOSS_TYPE(ENUM)
} EntityBossType;

typedef enum {
    MY_AI_STATE_NULL,
    MY_AI_STATE_TAKE_HITS,
    MY_AI_STATE_ANGRY
} MyEntity_AiState;

static char *MyEntity_EntityBossTypeStrings[] = { MY_BOSS_TYPE(STRING) };

#define MY_ENTITY_TYPE(FUNC) \
FUNC(ENTITY_NULL)\
FUNC(ENTITY_PLAYER)\
FUNC(ENTITY_BULLET)\
FUNC(ENTITY_BUCKET)\
FUNC(ENTITY_DROPLET)\
FUNC(ENTITY_CHOC_BAR)\
FUNC(ENTITY_ROOM)\
FUNC(ENTITY_CRAMP)\
FUNC(ENTITY_SCENERY)\
FUNC(ENTITY_STAR)\
FUNC(ENTITY_TILE)\
FUNC(ENTITY_ASTEROID)\
FUNC(ENTITY_SOUND_CHANGER)\
FUNC(ENTITY_UNDERPANTS)\
FUNC(ENTITY_TELEPORTER)\
FUNC(ENTITY_BOSS)\
FUNC(ENTITY_ENEMY_BULLET)\
FUNC(ENTITY_ENEMY)\

typedef enum {
    MY_ENTITY_TYPE(ENUM)
} EntityType;

static char *MyEntity_EntityTypeStrings[] = { MY_ENTITY_TYPE(STRING) };


typedef enum {
    ENTITY_MOVE_NULL,
    ENTITY_MOVE_LEFT,
    ENTITY_MOVE_RIGHT,
} MoveDirection;

typedef struct MoveOption MoveOption;
typedef struct MoveOption {
    MoveDirection dir;
    MoveOption *next;
    MoveOption *prev;
} MoveOption;

typedef struct Entity Entity;
typedef struct Entity {
    EasyTransform T;
    char *name;
    
    bool active;
    V4 colorTint;
    
    EntityType type;
    Texture *sprite;
    
    EasyModel  *model;
    
    bool updatedPhysics;
    bool updatedFrame;
    
    //NOTE(ollie): physics world hold both colliders & rigidbodies
    
    EasyCollider *collider; //trigger
    EasyRigidBody *rb;  
    ////////
    
    //NOTE(ol): For fading out things
    Timer fadeTimer;
    //

    //NOTE(ollie): For bullets
    Timer lifespanTimer;
    Timer bulletColorTimer;

    //NOTE(ollie): For AI types
    Timer aiStateTimer;
    MyEntity_AiState aiStateAt;
    Timer hurtColorTimer;
    s32 lastInterval;
    //NOTE(ollie): For lerping to begin attack
    float startPosForBoss;

    s32 enemyHP;

    PlayingSound *playingSound;
    
    union {
        struct { //Teleporter
            animation idleAnimation;
            animation_list_item animationListSentintel;

            
            Entity *teleporterPartner;
        };
        struct { //Player
            
            Timer moveTimer;
            int startLane; //for the move lerp
            
            MoveOption moveList;
            MoveOption *freeList;
            
            Texture *sprites[3];
            
            int laneIndex;
            
            int healthPoints;
            int dropletCount;
            int maxHealthPoints;
            
            int dropletCountStore;
            MoveDirection moveDirection;
            
            //NOTE(ollie): This is for the player flashing when hurt, & can't get hurt if it's on
            Timer hurtTimer; 
            Timer invincibleTimer;
            Timer restartToMiddleTimer;
            float lastValueForTimer;

            PlayerMoveViaType moveViaType;
            V3 teleportPos;

            Entity *lastTeleporter;

            Timer playerReloadTimer;
            float ammoCount;
            
        };
        struct { //Bullet
            
            
        };
        struct { //Bucket
            
        };
        struct { //Boss
            EntityBossType bossType;
        };
        struct { //Droplet
            
        };
        struct { //Room
        };
        struct { //sound changer
            WavFile *soundToPlay;
            
        };
    };
    
} Entity;


typedef struct {
    EntityType type;
    V3 pos;
} EntityCreateInfo;

typedef struct {
    Array_Dynamic entities;
    
    EasyPhysics_World physicsWorld;
    
    int toCreateCount;
    EntityCreateInfo toCreate[512];
    
} MyEntityManager;

