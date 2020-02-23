#define OPENGL_BACKEND 1
#define RENDER_BACKEND OPENGL_BACKEND
#define OPENGL_MAJOR 3
#define OPENGL_MINOR 1

#define PHYSICS_TIME_STEP 0.008f

#define DEFINES_RESOLUTION_X 1920/2
#define DEFINES_RESOLUTION_Y 1080/2

#define DEFINES_WINDOW_SIZE_X 800
#define DEFINES_WINDOW_SIZE_Y 800

#define VSYNC_ON 1

#define DEFINES_SOUNDS_FOLDER "sounds/"
#define DEFINES_AUDIO_SAMPLE_RATE 44100

#if DEVELOPER_MODE
#define RESOURCE_PATH_EXTENSION "../res/" 
#else 
#define RESOURCE_PATH_EXTENSION "res/"
#define NDEBUG
#endif


typedef enum {
	ENTITY_PLAYER,
	ENTITY_BULLET,
	ENTITY_BUCKET,
	ENTITY_DROPLET,
	ENTITY_CHOC_BAR,
	ENTITY_ROOM,
	ENTITY_CRAMP,
	ENTITY_SCENERY,
	ENTITY_STAR,
	ENTITY_TILE,
	ENTITY_UNDERPANTS,
} EntityType;