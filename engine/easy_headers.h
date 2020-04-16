#include <GL/gl3w.h>

#include "../SDL2/sdl.h"
#include "../SDL2/SDL_syswm.h"

#include <stdio.h>
#include <time.h> // to init random generator
#include "easy_types.h"
// #include <string.h>

#if DEVELOPER_MODE
// #include "stb_stretch_buffer.h"
#endif

#include "easy_assert.h"
#include "easy_logger.h"
#include "easy_debug_variables.h"
#include "easy_time.h"


///////////////////////************* From render file, used in the profiler for counting draw calls ************////////////////////

#define EASY_RENDER_SHAPE_TYPE(FUNC)\
FUNC(SHAPE_RECTANGLE)\
FUNC(SHAPE_RECTANGLE_GRAD)\
FUNC(SHAPE_TEXTURE)\
FUNC(SHAPE_MODEL)\
FUNC(SHAPE_SKYBOX)\
FUNC(SHAPE_SKY_QUAD)\
FUNC(SHAPE_TONE_MAP)\
FUNC(SHAPE_TERRAIN)\
FUNC(SHAPE_SHADOW)\
FUNC(SHAPE_CIRCLE)\
FUNC(SHAPE_LINE)\
FUNC(SHAPE_BLUR)\
FUNC(SHAPE_COLOR_WHEEL)\
FUNC(SHAPE_MODEL_AS_2D_IMAGE)\
FUNC(SHAPE_CIRCLE_TRANSITION)\
FUNC(RENDER_SHAPE_COUNT)\


typedef enum {
    EASY_RENDER_SHAPE_TYPE(ENUM)
} ShapeType;

static char *EasyRender_ShapeTypeStrings[] = { EASY_RENDER_SHAPE_TYPE(STRING) };
////////////////////////////////////////////////////////////////////


//NOTE(ollie): Above profiler, can't be profiled
#include "easy_platform.h"
#include "easy_profiler.h"
#include "easy.h"

static char* globalExeBasePath;

#include "easy_files.h"
#include "easy_math.h"
#include "easy_error.h"
#include "easy_array.h"
#include "easy_sound.h"
#include "easy_lex.h"
#include "easy_transform.h"
#include "easy_color.h"
#include "easy_render.h"

#include "easy_perlin.h"

#include "easy_utf8.h"
#include "easy_string.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "easy_timer.h"
#include "easy_animation.h"
#include "easy_assets.h"
#include "easy_text_io.h"
#include "easy_texture_atlas.h"
#include "easy_font.h"

#include "easy_3d.h"
#include "easy_terrain.h"

//#include "easy_sdl_joystick.h"

#include "easy_tweaks.h"
#include "easy_particle_effects.h"
#include "easy_string_compile.h"
// #include "easy_skeleton.h"

#include "easy_transition.h"

#include "easy_asset_loader.h"


#include "easy_keyState.h"
#include "easy_console.h"
#include "easy_flash_text.h"
#include "easy_editor.h"
#include "easy_profiler_draw.h"

#include "easy_os.h"
#include "easy_camera.h"
#include "easy_tile.h"
// #include "easy_ui.h"

#include "easy_ast.h"

#define GJK_IMPLEMENTATION 
#include "easy_gjk.h"

#include "easy_collision.h"
#include "easy_physics.h"
#include "easy_vm.h"

// #include "easy_write.h"



