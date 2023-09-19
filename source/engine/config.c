#include "render.h"
#include "nuke.h"

#define SOKOL_TRACE_HOOKS
#define SOKOL_IMPL
#include "sokol/sokol_glue.h"
#include "sokol/sokol_audio.h"
#include "sokol/sokol_time.h"
#include "sokol/sokol_args.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_app.h"
#include "sokol/sokol_gfx_ext.h"

#define MSF_GIF_IMPL
#include "msf_gif.h"

#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>

#define STB_TRUETYPE_IMPLEMENTATION
#define STB_TRUETYPE_NO_STDIO
#include <stb_truetype.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#define STBI_NO_STDIO
#ifdef __TINYC__
#define STBI_NO_SIMD
#endif
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBIR_DEFAULT_FILTER_DOWNSAMPLE STBIR_FILTER_BOX
#include "stb_image_write.h"

#define PL_MPEG_IMPLEMENTATION
#include <pl_mpeg.h>

