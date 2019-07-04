#define OPENGL_BACKEND 1
#define RENDER_BACKEND OPENGL_BACKEND
#define OPENGL_MAJOR 3
#define OPENGL_MINOR 1
#define RENDER_HANDNESS -1 //Right hand handess -> z going into the screen. 

#if DEVELOPER_MODE
#define RESOURCE_PATH_EXTENSION "../res/" 
#else 
#define RESOURCE_PATH_EXTENSION "res/"
#define NDEBUG
#endif