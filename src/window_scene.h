
typedef enum {
	WINDOW_TYPE_SCENE_GAME,
	WINDOW_TYPE_SCENE_LOGIC_BLOCKS,
	WINDOW_TYPE_SCENE_LOGIC_BLOCKS_CHOOSER,
	WINDOW_TYPE_SCENE_ASSETS
} WindowType;

typedef enum {
	WINDOW_DOCK_LEFT,
	WINDOW_DOCK_MIDDLE,
	WINDOW_DOCK_RIGHT,
} WindowDockType;

typedef struct {
	WindowType type;
	Rect2f dim;

	WindowDockType dockType;

	//NOTE(ollie): Actual position
	V2 cameraPos;
	//NOTE(ollie): The goal position to lerp towards, which we change with the keys
	V2 goalCameraPos;

	//NOTE(ollie): Whether we can see the window
	bool isVisible;
	//NOTE(ollie): Whether were interacting with the window
	bool isActive;
} WindowScene;