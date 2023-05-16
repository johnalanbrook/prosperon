#include "debugdraw.h"

#include "openglrender.h"

#include "shader.h"
#include "log.h"
#include <assert.h>
#include "debug.h"
#include "window.h"
#include "2dphysics.h"
#include "stb_ds.h"
#include "sokol/sokol_gfx.h"

#define PAR_STREAMLINES_IMPLEMENTATION
#include "par/par_streamlines.h"

#include "font.h"

static sg_shader point_shader;
static sg_pipeline point_pipe;
static sg_bindings point_bind;
struct point_vertex {
  cpVect pos;
  struct rgba color;
  float radius;
};
static int point_c = 0;

static sg_pipeline grid_pipe;
static sg_bindings grid_bind;
static sg_shader grid_shader;
static int grid_c = 0;

static sg_pipeline poly_pipe;
static sg_bindings poly_bind;
static sg_shader poly_shader;
static int poly_c = 0;
static int poly_v = 0;
struct poly_vertex {
  float pos[2];
  float uv[2];
  struct rgba color;
};

static sg_pipeline circle_pipe;
static sg_bindings circle_bind;
static sg_shader csg;
static int circle_count = 0;
static int circle_vert_c = 7;
struct circle_vertex {
  float pos[2];
  float radius;
  struct rgba color;
};

void debug_flush()
{
  sg_apply_pipeline(circle_pipe);
  sg_apply_bindings(&circle_bind);
  sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE_REF(projection));
  
  sg_draw(0,4,circle_count);
  circle_count = 0;

  sg_apply_pipeline(poly_pipe);
  sg_apply_bindings(&poly_bind);
  sg_apply_uniforms(SG_SHADERSTAGE_VS,0,SG_RANGE_REF(projection));
  sg_draw(0,poly_c,1);
  poly_c = 0;
  poly_v = 0;
  
  sg_apply_pipeline(point_pipe);
  sg_apply_bindings(&point_bind);
  sg_apply_uniforms(SG_SHADERSTAGE_VS,0,SG_RANGE_REF(projection));
  sg_draw(0,point_c,1);
  point_c = 0;
}

static sg_shader_uniform_block_desc projection_ubo = {
  .size = sizeof(projection),
  .uniforms = {
    [0] = { .name = "proj", .type = SG_UNIFORMTYPE_MAT4 },
  }
};

sg_blend_state blend_trans = {
  .enabled = true,
  .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
  .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
  .src_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA,
  .src_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
};
  

void debugdraw_init()
{
  point_shader = sg_make_shader(&(sg_shader_desc){
    .vs.source = slurp_text("shaders/point_v.glsl"),
    .fs.source = slurp_text("shaders/point_f.glsl"),
    .vs.uniform_blocks[0] = projection_ubo
  });
  
  point_pipe = sg_make_pipeline(&(sg_pipeline_desc){
    .shader = point_shader,
    .layout = {
      .attrs = {
        [0].format = SG_VERTEXFORMAT_FLOAT2, /* pos */
	[1].format = SG_VERTEXFORMAT_UBYTE4N, /* color */
	[2].format = SG_VERTEXFORMAT_FLOAT /* radius */
      }
    },
    .primitive_type = SG_PRIMITIVETYPE_POINTS,
    .colors[0].blend = blend_trans
  });
  
  point_bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
    .size = sizeof(struct point_vertex)*5000,
    .usage = SG_USAGE_STREAM
  });
  
    csg = sg_make_shader(&(sg_shader_desc){
      .vs.source = slurp_text("shaders/circlevert.glsl"),
      .fs.source = slurp_text("shaders/circlefrag.glsl"),
      .vs.uniform_blocks[0] = projection_ubo,
    });

    circle_pipe = sg_make_pipeline(&(sg_pipeline_desc){
      .shader = csg,
      .layout = {
        .attrs = {
	  [0].format = SG_VERTEXFORMAT_FLOAT2,
	  [0].buffer_index = 1,
	  [1].format = SG_VERTEXFORMAT_FLOAT2,
	  [2].format = SG_VERTEXFORMAT_FLOAT,
	  [3].format = SG_VERTEXFORMAT_UBYTE4N
	},
      .buffers[0].step_func = SG_VERTEXSTEP_PER_INSTANCE,
    },
    .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
    .cull_mode = SG_CULLMODE_BACK,
    .colors[0].blend = blend_trans,
    .label = "circle pipeline"
  });

  circle_bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
    .size = sizeof(float)*circle_vert_c*5000,
    .usage = SG_USAGE_STREAM,
  });

  float circleverts[8] = {
    -1,-1,
    -1,1,
    1,-1,
    1,1
  };

  circle_bind.vertex_buffers[1] = sg_make_buffer(&(sg_buffer_desc){
    .data = SG_RANGE(circleverts),
    .usage = SG_USAGE_IMMUTABLE,
  });
  
  grid_shader = sg_make_shader(&(sg_shader_desc){
    .vs.source = slurp_text("shaders/gridvert.glsl"),
    .fs.source = slurp_text("shaders/gridfrag.glsl"),
    .vs.uniform_blocks[0] = projection_ubo,
    .vs.uniform_blocks[1] = {
      .size = sizeof(float)*2,
      .uniforms = { [0] = { .name = "offset", .type = SG_UNIFORMTYPE_FLOAT2 } } },
    .fs.uniform_blocks[0] = {
      .size = sizeof(float)*6,
      .uniforms = {
        [0] = { .name = "thickness", .type = SG_UNIFORMTYPE_FLOAT },
	[1] = { .name = "span", .type = SG_UNIFORMTYPE_FLOAT },
	[2] = { .name = "color", .type = SG_UNIFORMTYPE_FLOAT4 },
     }
   },
  });
  

  grid_pipe = sg_make_pipeline(&(sg_pipeline_desc){
    .shader = grid_shader,
    .layout = {
      .attrs = {
        [0].format = SG_VERTEXFORMAT_FLOAT2, /* pos */
      }
    },
    .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
//    .cull_mode = sg_cullmode_back,
    .label = "grid pipeline",
    .colors[0].blend = blend_trans,
  });
  
  grid_bind.vertex_buffers[0] = circle_bind.vertex_buffers[1];
  
  poly_shader = sg_make_shader(&(sg_shader_desc){
    .vs.source = slurp_text("shaders/poly_v.glsl"),
    .fs.source = slurp_text("shaders/poly_f.glsl"),
    .vs.uniform_blocks[0] = projection_ubo
  });
  
  poly_pipe = sg_make_pipeline(&(sg_pipeline_desc){
    .shader = poly_shader,
    .layout = {
      .attrs = { [0].format = SG_VERTEXFORMAT_FLOAT2, /* pos */
		 [1].format = SG_VERTEXFORMAT_FLOAT2, /* uv */
		 [2].format = SG_VERTEXFORMAT_UBYTE4N /* color rgba */
       }
    },
    .index_type = SG_INDEXTYPE_UINT32,
    .colors[0].blend = blend_trans,
  });
  
  poly_bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
    .size = sizeof(struct poly_vertex)*1000,
    .usage = SG_USAGE_STREAM,
    .type = SG_BUFFERTYPE_VERTEXBUFFER,
  });
  
  poly_bind.index_buffer = sg_make_buffer(&(sg_buffer_desc){
    .size = sizeof(uint32_t)*6*1000,
    .usage = SG_USAGE_STREAM,
    .type = SG_BUFFERTYPE_INDEXBUFFER
  });
}

void draw_line(cpVect s, cpVect e, struct rgba color)
{
  cpVect verts[2] = {s, e};
  draw_poly(verts, 2, color);
}

cpVect center_of_vects(cpVect *v, int n)
{
  cpVect c;
  for (int i = 0; i < n; i++) {
    c.x += v[i].x;
    c.y += v[i].y;
  }

  c.x /= n;
  c.y /= n;
  return c;
}

float vecs2m(cpVect a, cpVect b)
{
  return (b.y-a.y)/(b.x-a.x);
}

cpVect inflatepoint(cpVect a, cpVect b, cpVect c, float d)
{
  cpVect ba = cpvnormalize(cpvsub(a,b));
  cpVect bc = cpvnormalize(cpvsub(c,b));
  cpVect avg = cpvadd(ba, bc);
  avg = cpvmult(avg, 0.5);
  float dot = cpvdot(ba, bc);
  dot /= cpvlength(ba);
  dot /= cpvlength(bc);
  float mid = acos(dot)/2;
  avg = cpvnormalize(avg);
  return cpvadd(b, cpvmult(avg, d/sin(mid)));
}

void inflatepoints(cpVect *r, cpVect *p, float d, int n)
{
  if (d == 0) {
       for (int i = 0; i < n; i++)
         r[i] = p[i];

       return;
     }

     if (cpveql(p[0], p[n-1])) {
        r[0] = inflatepoint(p[n-2],p[0],p[1],d);
	r[n-1] = r[0];
      } else {
        cpVect outdir = cpvmult(cpvnormalize(cpvsub(p[0],p[1])),fabs(d));
	cpVect perp;
	if (d > 0)
          perp = cpvperp(outdir);
	else
	  perp = cpvrperp(outdir);
        r[0] = cpvadd(p[0],cpvadd(outdir,perp));
	
	outdir = cpvmult(cpvnormalize(cpvsub(p[n-1],p[n-2])),fabs(d));
	if (d > 0)
          perp = cpvrperp(outdir);
	else
	  perp = cpvperp(outdir);
        r[n-1] = cpvadd(p[n-1],cpvadd(outdir,perp));
      }

      for (int i = 0; i < n-2; i++)
        r[i+1] = inflatepoint(p[i],p[i+1],p[i+2], d);
}

void draw_edge(cpVect *points, int n, struct rgba color, int thickness, int closed, int flags)
{
  static_assert(sizeof(cpVect) == 2*sizeof(float));
  /* todo: should be dashed, and filled. use a texture. */  
  /* draw polygon outline */
  parsl_position par_v[n];
  
  for (int i = 0; i < n; i++) {
    par_v[i].x = points[i].x;
    par_v[i].y = points[i].y;
  }

  uint16_t spine_lens[] = {n};
  
  parsl_context *par_ctx = parsl_create_context((parsl_config){
    .thickness = 1,
    .flags = PARSL_FLAG_ANNOTATIONS
  });
  
  parsl_mesh *mesh = parsl_mesh_from_lines(par_ctx, (parsl_spine_list){
    .num_vertices = n,
    .num_spines = 1,
    .vertices = par_v,
    .spine_lengths = spine_lens,
    .closed = closed
  });
  
  for (int i = 0; i < mesh->num_triangles*3; i++)
    mesh->triangle_indices[i] += poly_v;
      
  sg_range it = {
    .ptr = mesh->triangle_indices,
    .size = sizeof(uint32_t)*mesh->num_triangles*3
  };
  
  struct poly_vertex vertices[mesh->num_vertices];
  
  for (int i = 0; i < mesh->num_vertices; i++) {
    vertices[i].pos[0] = mesh->positions[i].x;
    vertices[i].pos[1] = mesh->positions[i].y;
    vertices[i].uv[0] = mesh->annotations[i].u_along_curve;
    vertices[i].uv[1] = mesh->annotations[i].v_across_curve;
    vertices[i].color = color;
  }

  sg_range vvt = {
    .ptr = vertices,
    .size = sizeof(struct poly_vertex)*mesh->num_vertices
  };

  sg_append_buffer(poly_bind.vertex_buffers[0], &vvt);  
  sg_append_buffer(poly_bind.index_buffer, &it);
  
  poly_c += mesh->num_triangles*3;
  poly_v += mesh->num_vertices;    
  
  parsl_destroy_context(par_ctx);
}

void draw_circle(int x, int y, float radius, int pixels, struct rgba color, int fill)
{
  struct circle_vertex cv;
  cv.pos[0] = x;
  cv.pos[1] = y;
  cv.radius = radius;
  cv.color = color;
  sg_append_buffer(circle_bind.vertex_buffers[0], SG_RANGE_REF(cv));
  circle_count++;
}

void draw_rect(int x, int y, int w, int h, struct rgba color)
{
    float hw = w / 2.f;
    float hh = h / 2.f;

    cpVect verts[4] = {
      { .x = x-hw, .y = y-hh },
      { .x = x+hw, .y = y-hh },
      { .x = x+hw, .y = y+hh },
      { .x = x-hw, .y = y+hh }
    };
    
    draw_poly(verts, 4, color);
}

void draw_box(struct cpVect c, struct cpVect wh, struct rgba color)
{
  draw_rect(c.x, c.y, wh.x, wh.y, color);
}

void draw_arrow(struct cpVect start, struct cpVect end, struct rgba color, int capsize)
{
  draw_line(start, end, color);
  draw_cppoint(end, capsize, color);
}

void draw_grid(int width, int span, struct rgba color)
{
    cpVect offset = cam_pos();
    offset = cpvmult(offset, 1/cam_zoom());
    offset.x -= mainwin->width/2;
    offset.y -= mainwin->height/2;

  sg_apply_pipeline(grid_pipe);
  sg_apply_bindings(&grid_bind);
  
  float col[4] = { color.r/255.0 ,color.g/255.0 ,color.b/255.0 ,color.a/255.0 };
  
  float fubo[6];
  fubo[0] = 1;
  fubo[1] = span;
  memcpy(&fubo[2], col, sizeof(float)*4);
  sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE_REF(projection));
  sg_apply_uniforms(SG_SHADERSTAGE_VS, 1, SG_RANGE_REF(offset));
  sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, SG_RANGE_REF(fubo));
  sg_draw(0,4,1);
}

void draw_point(int x, int y, float r, struct rgba color)
{
  struct point_vertex p;
  p.pos.x = x;
  p.pos.y = y;
  p.color = color;
  p.radius = r;
  
  sg_range pt = {
    .ptr = &p,
    .size = sizeof(p)
  };
  
  sg_append_buffer(point_bind.vertex_buffers[0], &pt);
  point_c++;
}

void draw_cppoint(struct cpVect point, float r, struct rgba color)
{
  draw_point(point.x, point.y, r, color);
}

void draw_points(struct cpVect *points, int n, float size, struct rgba color)
{
    for (int i = 0; i < n; i++)
        draw_point(points[i].x, points[i].y, size, color);
}

void draw_poly(cpVect *points, int n, struct rgba color)
{
  draw_edge(points,n,color,1,1,0);
  
  color.a = 40;
  
  /* Find polygon mesh */
  int tric = n - 2;
  
  if (n < 1) return;
   
  uint32_t tridxs[tric*3];
  
  for (int i = 2, ti = 0; i < n; i++, ti+=3) {
    tridxs[ti] = 0;
    tridxs[ti+1] = i-1;
    tridxs[ti+2] = i;
  }
  
  for (int i = 0; i < tric*3; i++)
    tridxs[i] += poly_v;
  
  sg_range trip = {
    .ptr = tridxs,
    .size = sizeof(uint32_t)*3*tric
  };
  
  struct poly_vertex polyverts[n];
  
  for (int i = 0; i < n; i++) {
    polyverts[i].pos[0] = points[i].x;
    polyverts[i].pos[1] = points[i].y;
    polyverts[i].uv[0] = 0.0;
    polyverts[i].uv[1] = 0.0;
    polyverts[i].color = color;
  }
  
  sg_range ppp = {
    .ptr = polyverts,
    .size = sizeof(struct poly_vertex)*n
  };
  
  sg_append_buffer(poly_bind.vertex_buffers[0], &ppp);
  sg_append_buffer(poly_bind.index_buffer, &trip);
  
  poly_c += tric*3;
  poly_v += n;
}

void debugdraw_flush()
{

}
