#include "config.h"

#define SOKOL_TRACE_HOOKS
#define SOKOL_IMPL
#include "sokol/sokol_audio.h"
#include "sokol/sokol_time.h"
#include "sokol/sokol_args.h"
#include "sokol/sokol_gfx.h"
#include "sokol/sokol_app.h"
#include "sokol/sokol_fetch.h"
#include "sokol/util/sokol_gl.h"

#define CUTE_ASEPRITE_IMPLEMENTATION
#include "cute_aseprite.h"

#define LAY_FLOAT 1
#define LAY_IMPLEMENTATION
#include "layout.h"

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#define MSF_GIF_IMPL
#include "msf_gif.h"

#define STB_HEXWAVE_IMPLEMENTATION
#include "stb_hexwave.h"

#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>

#define STB_TRUETYPE_IMPLEMENTATION
#define STB_TRUETYPE_NO_STDIO
#include <stb_truetype.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#define STBI_NO_STDIO
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBIR_DEFAULT_FILTER_DOWNSAMPLE STBIR_FILTER_BOX
#include "stb_image_write.h"

#define PL_MPEG_IMPLEMENTATION
#include <pl_mpeg.h>

#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvg.h"
#include "nanosvgrast.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#define PAR_SHAPES_IMPLEMENTATION
#include "par/par_shapes.h"

#define PAR_STREAMLINES_IMPLEMENTATION
#include "par/par_streamlines.h"

#define QOI_IMPLEMENTATION
#include "qoi.h"

#define QOP_IMPLEMENTATION
#include "qop.h"
