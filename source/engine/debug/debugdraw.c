#include "debugdraw.h"

#include "render.h"
#include "yugine.h"
#include "log.h"
#include <assert.h>
#include "debug.h"
#include "window.h"
#include "2dphysics.h"
#include "stb_ds.h"
#include "sokol/sokol_gfx.h"

#include "point.sglsl.h"
#include "poly.sglsl.h"
#include "circle.sglsl.h"
#include "line.sglsl.h"
#include "grid.sglsl.h"

#define PAR_STREAMLINES_IMPLEMENTATION
#include "par/par_streamlines.h"

#include "font.h"

#define v_amt 5000

struct flush {
  sg_shader shader;
  sg_pipeline pipe;
  sg_bindings bind;
  void *verts;
  int c;
  int v;
  int sc;
  int sv;
};
typedef struct flush flush;

static flush fpoint;
static flush circle;

void flushview(flush *f, HMM_Mat4 *view)
{
  sg_apply_pipeline(f->pipe);
  sg_apply_bindings(&f->bind);
  sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE_REF(*view));
  sg_draw(f->sc, f->c, 1);
}

void flushpass(flush *f)
{
  f->sc = f->c;
  f->c = 0;
  f->sv = f->v;
  f->v = 0;
}

static sg_shader point_shader;
static sg_pipeline point_pipe;
static sg_bindings point_bind;
struct point_vertex {
  struct draw_p pos;
  struct rgba color;
  float radius;
};
static int point_c = 0;
static int point_sc = 0;

static sg_shader line_shader;
static sg_pipeline line_pipe;
static sg_bindings line_bind;
struct line_vert {
  struct draw_p pos;
  float dist;
  struct rgba color;
  float seg_len;
  float seg_speed;
};
static int line_c = 0;
static int line_v = 0;
static int line_sc = 0;
static int line_sv = 0;
static sg_pipeline grid_pipe;
static sg_bindings grid_bind;
static sg_shader grid_shader;
static int grid_c = 0;

static sg_pipeline poly_pipe;
static sg_bindings poly_bind;
static sg_shader poly_shader;
static int poly_c = 0;
static int poly_v = 0;
static int poly_sc = 0;
static int poly_sv = 0;
struct poly_vertex {
  struct draw_p pos;
  float uv[2];
  struct rgba color;
};

static sg_pipeline circle_pipe;
static sg_bindings circle_bind;
static sg_shader csg;
static int circle_count = 0;
static int circle_sc = 0;
struct circle_vertex {
  struct draw_p pos;
  float radius;
  struct rgba color;
  float segsize;
  float fill;
};

/* Writes debug data to buffers, and draws */
void debug_flush(HMM_Mat4 *view)
{
  if (poly_c != 0) {
    sg_apply_pipeline(poly_pipe);
    sg_apply_bindings(&poly_bind);
    sg_apply_uniforms(SG_SHADERSTAGE_VS,0,SG_RANGE_REF(*view));
    sg_draw(poly_sc,poly_c,1);
  }

  if (point_c != 0) {
    sg_apply_pipeline(point_pipe);
    sg_apply_bindings(&point_bind);
    sg_apply_uniforms(SG_SHADERSTAGE_VS,0,SG_RANGE_REF(*view));
    sg_draw(point_sc,point_c,1);
  }

  if (line_c != 0) {
    sg_apply_pipeline(line_pipe);
    sg_apply_bindings(&line_bind);
    sg_apply_uniforms(SG_SHADERSTAGE_VS,0,SG_RANGE_REF(*view));
    lfs_params_t lt;
    lt.time = apptime();
    sg_apply_uniforms(SG_SHADERSTAGE_FS,0,SG_RANGE_REF(lt));
    sg_draw(line_sc,line_c,1);
   }

  if (circle_count != 0) {
    sg_apply_pipeline(circle_pipe);
    sg_apply_bindings(&circle_bind);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE_REF(*view));
    sg_draw(circle_sc,4,circle_count);
  }
}

void debug_nextpass()
{
  point_sc = point_c;
  point_c = 0;

  circle_sc = circle_count;
  circle_count = 0;

  line_sv = line_v;
  line_v = 0;
  line_sc = line_c;
  line_c = 0;

  poly_sc = poly_c;
  poly_c = 0;

  poly_sv = poly_v;
  poly_v = 0;
}

void debug_newframe()
{
  point_sc = 0;
  point_c = 0;
  circle_sc = circle_count = line_sv = line_v = line_sc = line_c = poly_sc = poly_c = 0;
  poly_sv = poly_v = 0;
  
}

static sg_shader_uniform_block_desc projection_ubo = {
  .size = sizeof(projection),
  .uniforms = {
    [0] = { .name = "proj", .type = SG_UNIFORMTYPE_MAT4 },
  }
};

static sg_shader_uniform_block_desc time_ubo = {
  .size = sizeof(float),
  .uniforms = {
    [0] = { .name = "time", .type = SG_UNIFORMTYPE_FLOAT },
  }
};

void debugdraw_init()
{
  point_shader = sg_make_shader(point_shader_desc(sg_query_backend()));
  
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
    .colors[0].blend = blend_trans,
    .label = "dbg point",
  });
  
  point_bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
    .size = sizeof(struct point_vertex)*v_amt,
    .usage = SG_USAGE_STREAM
  });
  
  line_shader = sg_make_shader(line_shader_desc(sg_query_backend()));
  
  line_pipe = sg_make_pipeline(&(sg_pipeline_desc){
    .shader = line_shader,
    .layout = {
      .attrs = {
        [0].format = SG_VERTEXFORMAT_FLOAT2, /* pos */
	[1].format = SG_VERTEXFORMAT_FLOAT, /* dist */
	[2].format = SG_VERTEXFORMAT_UBYTE4N, /* color */
	[3].format = SG_VERTEXFORMAT_FLOAT, /* seg length */
	[4].format = SG_VERTEXFORMAT_FLOAT /* dashed line speed */
      }
    },
    .primitive_type = SG_PRIMITIVETYPE_LINES,
    .index_type = SG_INDEXTYPE_UINT16,
    .colors[0].blend = blend_trans,
    .label = "dbg line",
  });
  
  line_bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
    .size = sizeof(struct line_vert)*v_amt,
    .usage = SG_USAGE_STREAM
  });
  
  line_bind.index_buffer = sg_make_buffer(&(sg_buffer_desc){
    .size = sizeof(uint16_t)*v_amt,
    .usage = SG_USAGE_STREAM,
    .type = SG_BUFFERTYPE_INDEXBUFFER
  });

  csg = sg_make_shader(circle_shader_desc(sg_query_backend()));
    circle_pipe = sg_make_pipeline(&(sg_pipeline_desc){
      .shader = csg,
      .layout = {
        .attrs = {
	  [0].format = SG_VERTEXFORMAT_FLOAT2,
	  [0].buffer_index = 1,
	  [1].format = SG_VERTEXFORMAT_FLOAT2,
	  [2].format = SG_VERTEXFORMAT_FLOAT,
	  [3].format = SG_VERTEXFORMAT_UBYTE4N,
	  [4].format = SG_VERTEXFORMAT_FLOAT,
	  [5].format = SG_VERTEXFORMAT_FLOAT
	},
      .buffers[0].step_func = SG_VERTEXSTEP_PER_INSTANCE,
    },
    .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
    .cull_mode = SG_CULLMODE_BACK,
    .colors[0].blend = blend_trans,
    .label = "circle pipeline"
  });

  circle_bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
    .size = sizeof(struct circle_vertex)*v_amt,
    .usage = SG_USAGE_STREAM,
  });

  float circleverts[8] = {
    -1,-1,
    -1,1,
    1,-1,
    1,1
  };

  circle_bind.vertex_buffers[1] = sg_make_buffer(&(sg_buffer_desc){
    .data = (sg_range){.ptr = circleverts, .size = sizeof(float)*8},
    .usage = SG_USAGE_IMMUTABLE,
  });

  grid_shader = sg_make_shader(grid_shader_desc(sg_query_backend()));
  
  grid_pipe = sg_make_pipeline(&(sg_pipeline_desc){
    .shader = grid_shader,
    .layout = {
      .attrs = {
        [0].format = SG_VERTEXFORMAT_FLOAT2, /* pos */
      }
    },
    .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
    .cull_mode = SG_CULLMODE_BACK,
    .label = "grid pipeline",
    .colors[0].blend = blend_trans,
  });
  
  grid_bind.vertex_buffers[0] = circle_bind.vertex_buffers[1];

  poly_shader = sg_make_shader(poly_shader_desc(sg_query_backend()));
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
//    .cull_mode = SG_CULLMODE_FRONT,
    .label = "dbg poly",
  });
  poly_bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
    .size = sizeof(struct poly_vertex)*v_amt,
    .usage = SG_USAGE_STREAM,
    .type = SG_BUFFERTYPE_VERTEXBUFFER,
  });
  
  poly_bind.index_buffer = sg_make_buffer(&(sg_buffer_desc){
    .size = sizeof(uint32_t)*6*v_amt,
    .usage = SG_USAGE_STREAM,
    .type = SG_BUFFERTYPE_INDEXBUFFER
  });
}

void draw_line(HMM_Vec2 *points, int n, struct rgba color, float seg_len, float seg_speed)
{
  if (n < 2) return;
  
  seg_speed = 1;
  
  struct line_vert v[n];
  float dist = 0;

  for (int i = 0; i < n; i++) {
    v[i].pos.x = points[i].x;
    v[i].pos.y = points[i].y;
    v[i].color = color;
    v[i].seg_len = seg_len;
    v[i].seg_speed = seg_speed;
  }

  v[0].dist = 0;
  for (int i = 1; i < n; i++) {
    dist += HMM_DistV2(points[i-1], points[i]);
    v[i].dist = dist;    
  }
  
  int i_c = (n-1)*2;
  
  uint16_t idxs[i_c];
  for (int i = 0, d = 0; i < n-1; i++, d+=2) {
    idxs[d] = i + line_v + line_sv;
    idxs[d+1] = idxs[d]+1;
  }
  
  sg_range vr = {
    .ptr = v,
    .size = sizeof(struct line_vert)*n
  };
  
  sg_range ir = {
    .ptr = idxs,
    .size = sizeof(uint16_t)*i_c
  };
  
  sg_append_buffer(line_bind.vertex_buffers[0], &vr);
  sg_append_buffer(line_bind.index_buffer, &ir);

  line_c += i_c;
  line_v += n;
}

HMM_Vec2 center_of_vects(HMM_Vec2 *v, int n)
{
  HMM_Vec2 c;
  for (int i = 0; i < n; i++) {
    c.x += v[i].x;
    c.y += v[i].y;
  }

  c.x /= n;
  c.y /= n;
  return c;
}

/* Given a series of points p, computes a new series with them expanded on either side by d */
HMM_Vec2 *inflatepoints(HMM_Vec2 *p, float d, int n)
{
  if (d == 0) {
    HMM_Vec2 *ret = NULL;
    arraddn(ret,n);
       for (int i = 0; i < n; i++)
         ret[i] = p[i];

       return ret;
     }

  parsl_position par_v[n];
  uint16_t spine_lens[] = {n};
  for (int i = 0; i < n; i++) {
    par_v[i].x = p[i].x;
    par_v[i].y = p[i].y;
  };

  parsl_context *par_ctx = parsl_create_context((parsl_config){
    .thickness = d,
    .flags= PARSL_FLAG_ANNOTATIONS,
    .u_mode = PAR_U_MODE_DISTANCE
  });
  
  parsl_mesh *mesh = parsl_mesh_from_lines(par_ctx, (parsl_spine_list){
    .num_vertices = n,
    .num_spines = 1,
    .vertices = par_v,
    .spine_lengths = spine_lens,
    .closed = 0,
  });

  HMM_Vec2 *ret = NULL;
  arraddn(ret,mesh->num_vertices);
  for (int i = 0; i < mesh->num_vertices; i++) {
    ret[i].x = mesh->positions[i].x;
    ret[i].y = mesh->positions[i].y;
  };
  return ret;
}

/* Given a strip of points, draws them as segments. So 5 points is 4 segments, and ultimately 8 vertices */
void draw_edge(HMM_Vec2 *points, int n, struct rgba color, int thickness, int flags, struct rgba line_color, float line_seg)
{
  int closed = 0;
  if (thickness <= 1) {
    draw_line(points,n,line_color,0,0);
    return;
  }

  /* todo: should be dashed, and filled. use a texture. */  
  /* draw polygon outline */
  if (HMM_EqV2(points[0], points[n-1])) {
    closed = true;
    n--;
  }

  parsl_position par_v[n];
  
  for (int i = 0; i < n; i++) {
    par_v[i].x = points[i].x;
    par_v[i].y = points[i].y;
  }

  uint16_t spine_lens[] = {n};
  
  parsl_context *par_ctx = parsl_create_context((parsl_config){
    .thickness = thickness,
    .flags = PARSL_FLAG_ANNOTATIONS,
  });
  
  parsl_mesh *mesh = parsl_mesh_from_lines(par_ctx, (parsl_spine_list){
    .num_vertices = n,
    .num_spines = 1,
    .vertices = par_v,
    .spine_lengths = spine_lens,
    .closed = closed
  });
  
  for (int i = 0; i < mesh->num_triangles*3; i++)
    mesh->triangle_indices[i] += (poly_v+poly_sv);
      
  struct poly_vertex vertices[mesh->num_vertices];
  
  for (int i = 0; i < mesh->num_vertices; i++) {
    vertices[i].pos = (struct draw_p){ .x = mesh->positions[i].x, .y = mesh->positions[i].y };
    vertices[i].uv[0] = mesh->annotations[i].u_along_curve;
    vertices[i].uv[1] = mesh->annotations[i].v_across_curve;
    vertices[i].color = color;
  }

  sg_append_buffer(poly_bind.vertex_buffers[0], &(sg_range){.ptr = vertices, .size = sizeof(struct poly_vertex)*mesh->num_vertices});
  sg_append_buffer(poly_bind.index_buffer, &(sg_range){.ptr = mesh->triangle_indices, sizeof(uint32_t)*mesh->num_triangles*3});
  
  poly_c += mesh->num_triangles*3;
  poly_v += mesh->num_vertices;    
  
  parsl_destroy_context(par_ctx);

  /* Now drawing the line outlines */
  if (thickness == 1) {
    draw_line(points,n,line_color,line_seg, 0);
  } else {
    HMM_Vec2 in_p[n];
    HMM_Vec2 out_p[n];
    
    for (int i = 1, v = 0; i < n*2+1; i+=2, v++) {
      in_p[v].x = vertices[i].pos.x;
      in_p[v].y = vertices[i].pos.y;
    }

    for (int i = 0, v = 0; i < n*2; i+=2,v++) {
      out_p[v].x = vertices[i].pos.x;
      out_p[v].y = vertices[i].pos.y;
    }

    if (!closed) {
      HMM_Vec2 p[n*2];
      for (int i = 0; i < n; i++)
	p[i] = in_p[i];
      for (int i = n-1, v = n; i >= 0; i--,v++)
	p[v] = out_p[i];

      draw_line(p,n*2,line_color,line_seg,0);
      return;
    }

    draw_line(in_p,n,line_color,line_seg,0);
    draw_line(out_p,n,line_color,line_seg,0);
  }
}

void draw_circle(HMM_Vec2 pos, float radius, float pixels, struct rgba color, float seg)
{
  struct circle_vertex cv;
  cv.pos.x = pos.x;
  cv.pos.y = pos.y;
  cv.radius = radius;
  cv.color = color;
  cv.segsize = seg/radius;
  cv.fill = pixels/radius;
  sg_append_buffer(circle_bind.vertex_buffers[0], &(sg_range){.ptr = &cv, .size = sizeof(struct circle_vertex)});
  circle_count++;
}

void draw_box(HMM_Vec2 c, HMM_Vec2 wh, struct rgba color)
{
    float hw = wh.x / 2.f;
    float hh = wh.y / 2.f;

    HMM_Vec2 verts[4] = {
      { .x = c.x-hw, .y = c.y-hh },
      { .x = c.x+hw, .y = c.y-hh },
      { .x = c.x+hw, .y = c.y+hh },
      { .x = c.x-hw, .y = c.y+hh }
    };

    draw_poly(verts, 4, color);
}

void draw_grid(float width, float span, struct rgba color)
{
  HMM_Vec2 offset = (HMM_Vec2)cam_pos();
  offset = HMM_MulV2F(offset, 1/cam_zoom());

  float ubo[4];
  ubo[0] = offset.x;
  ubo[1] = offset.y;
  ubo[2] = mainwin.width;
  ubo[3] = mainwin.height;

  sg_apply_pipeline(grid_pipe);
  sg_apply_bindings(&grid_bind);
  float col[4];
  rgba2floats(col,color);

  fs_params_t pt;
  pt.thickness = (float)width;
  pt.span = span/cam_zoom();
  memcpy(&pt.color, col, sizeof(float)*4);
  sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE_REF(ubo));
  sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, SG_RANGE_REF(pt));
  sg_draw(0,4,1);
}

void draw_cppoint(HMM_Vec2 point, float r, struct rgba color)
{
  struct point_vertex p = {
    .color = color,
    .radius = r
  };
  p.pos.x = point.X;
  p.pos.y = point.Y;
  sg_append_buffer(point_bind.vertex_buffers[0], &(sg_range){.ptr = &p, .size = sizeof(struct point_vertex)});
  point_c++;
}

void draw_points(HMM_Vec2 *points, int n, float size, struct rgba color)
{
    for (int i = 0; i < n; i++)
      draw_cppoint(points[i], size, color);
}

void draw_poly(HMM_Vec2 *points, int n, struct rgba color)
{
  /* Find polygon mesh */
  int tric = n - 2;
  
  if (tric < 1) return;

  uint32_t tridxs[tric*3];
  
  for (int i = 2, ti = 0; i < n; i++, ti+=3) {
    tridxs[ti] = 0;
    tridxs[ti+1] = i-1;
    tridxs[ti+2] = i;
  }
  
  for (int i = 0; i < tric*3; i++)
    tridxs[i] += poly_v+poly_sv;
  
  struct poly_vertex polyverts[n];
  
  for (int i = 0; i < n; i++) {
    polyverts[i].pos = (struct draw_p) { .x = points[i].x, .y = points[i].y};
    polyverts[i].uv[0] = 0.0;
    polyverts[i].uv[1] = 0.0;
    polyverts[i].color = color;
  }

  sg_append_buffer(poly_bind.vertex_buffers[0], &(sg_range){.ptr = polyverts, .size = sizeof(struct poly_vertex)*n});
  sg_append_buffer(poly_bind.index_buffer, &(sg_range){.ptr = tridxs, sizeof(uint32_t)*3*tric});
  
  poly_c += tric*3;
  poly_v += n;
}
