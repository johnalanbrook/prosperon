#include "debugdraw.h"

#include "render.h"
#include "yugine.h"
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

#define v_amt 5000

static sg_shader point_shader;
static sg_pipeline point_pipe;
static sg_bindings point_bind;
struct point_vertex {
  cpVect pos;
  struct rgba color;
  float radius;
};
static int point_c = 0;
static int point_sc = 0;
static struct point_vertex point_b[v_amt];

static sg_shader line_shader;
static sg_pipeline line_pipe;
static sg_bindings line_bind;
struct line_vert {
  cpVect pos;
  float dist;
  struct rgba color;
  float seg_len;
  float seg_speed;
};
static int line_c = 0;
static int line_v = 0;
static int line_sc = 0;
static int line_sv = 0;
static struct line_vert line_b[v_amt];
static uint16_t line_bi[v_amt];

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
  cpVect pos;
  float uv[2];
  struct rgba color;
};
static struct poly_vertex poly_b[v_amt];
static uint32_t poly_bi[v_amt];

static sg_pipeline circle_pipe;
static sg_bindings circle_bind;
static sg_shader csg;
static int circle_count = 0;
static int circle_sc = 0;
struct circle_vertex {
  cpVect pos;
  float radius;
  struct rgba color;
  float segsize;
  float fill;
};

static struct circle_vertex circle_b[v_amt];

/* Writes debug data to buffers, and draws */
void debug_flush(HMM_Mat4 *view)
{
  if (poly_c != 0) {
    sg_apply_pipeline(poly_pipe);
    sg_apply_bindings(&poly_bind);
    sg_apply_uniforms(SG_SHADERSTAGE_VS,0,SG_RANGE_REF(*view));
    int b = sg_append_buffer(poly_bind.vertex_buffers[0], &(sg_range){
      .ptr = poly_b, .size = sizeof(struct poly_vertex)*poly_v});
    int bi = sg_append_buffer(poly_bind.index_buffer, &(sg_range){
      .ptr = poly_bi, .size = sizeof(uint32_t)*poly_c});
    sg_draw(poly_sc,poly_c,1);
  }
  
  if (point_c != 0) {
    sg_apply_pipeline(point_pipe);
    sg_apply_bindings(&point_bind);
    sg_apply_uniforms(SG_SHADERSTAGE_VS,0,SG_RANGE_REF(*view));
    sg_append_buffer(point_bind.vertex_buffers[0], &(sg_range){
      .ptr = point_b,
      .size = sizeof(struct point_vertex)*point_c});
    sg_draw(point_sc,point_c,1);
  }
  
  if (line_c != 0) {
    sg_apply_pipeline(line_pipe);
    sg_apply_bindings(&line_bind);
    sg_apply_uniforms(SG_SHADERSTAGE_VS,0,SG_RANGE_REF(*view));
    float time = appTime;
    sg_range tr = {
      .ptr = &time,
      .size = sizeof(float)
    };
    sg_apply_uniforms(SG_SHADERSTAGE_FS,0,&tr);
    sg_append_buffer(line_bind.vertex_buffers[0], &(sg_range){
      .ptr = line_b, .size = sizeof(struct line_vert)*line_v});
    sg_append_buffer(line_bind.index_buffer, &(sg_range){
      .ptr = line_bi, .size = sizeof(uint16_t)*line_c});
    sg_draw(line_sc,line_c,1);
  }

  if (circle_count != 0) {
    sg_apply_pipeline(circle_pipe);
    sg_apply_bindings(&circle_bind);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE_REF(*view));
    sg_append_buffer(circle_bind.vertex_buffers[0], &(sg_range){
      .ptr = circle_b,
      .size = sizeof(struct circle_vertex)*circle_count
    });
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
  point_shader = sg_compile_shader("shaders/point_v.glsl", "shaders/point_f.glsl", &(sg_shader_desc){
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
    .size = sizeof(struct point_vertex)*v_amt,
    .usage = SG_USAGE_STREAM
  });
  
  line_shader = sg_compile_shader("shaders/linevert.glsl", "shaders/linefrag.glsl", &(sg_shader_desc){
    .vs.uniform_blocks[0] = projection_ubo,
    .fs.uniform_blocks[0] = time_ubo
  });
  
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
    .colors[0].blend = blend_trans
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
  
    csg = sg_compile_shader("shaders/circlevert.glsl", "shaders/circlefrag.glsl", &(sg_shader_desc){
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
    .data = SG_RANGE(circleverts),
    .usage = SG_USAGE_IMMUTABLE,
  });
  
  grid_shader = sg_compile_shader("shaders/gridvert.glsl", "shaders/gridfrag.glsl", &(sg_shader_desc){
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
  
  poly_shader = sg_compile_shader("shaders/poly_v.glsl", "shaders/poly_f.glsl", &(sg_shader_desc){
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

void draw_line(cpVect *a_points, int a_n, struct rgba color, float seg_len, int closed, float seg_speed)
{
  if (a_n < 2) return;
  seg_speed = 1;

  int n = closed ? a_n+1 : a_n;
  cpVect points[n];

  memcpy(points, a_points, sizeof(cpVect)*n);
  if (closed)
    points[n-1] = a_points[0];

  struct line_vert v[n];
  float dist = 0;
  
  for (int i = 0; i < n-1; i++) {
    v[i].pos = points[i];
    v[i].dist = dist;
    v[i].color = color;
    v[i].seg_len = seg_len;
    v[i].seg_speed = seg_speed;
    dist += cpvdist(points[i], points[i+1]);
  }
  
  v[n-1].pos = points[n-1];
  v[n-1].dist = dist;
  v[n-1].color = color;
  v[n-1].seg_len = seg_len;
  v[n-1].seg_speed = seg_speed;
  
  int i_c = (n-1)*2;
  
  uint16_t idxs[i_c];
  
  for (int i = 0, d = 0; i < n-1; i++, d+=2) {
    idxs[d] = i + line_v + line_sv;
    idxs[d+1] = i+1 + line_v + line_sv;
  }
  
  sg_range vr = {
    .ptr = v,
    .size = sizeof(struct line_vert)*n
  };
  
  sg_range ir = {
    .ptr = idxs,
    .size = sizeof(uint16_t)*i_c
  };
  
  memcpy(line_b+line_v, v, sizeof(struct line_vert)*n);
  memcpy(line_bi+line_c, idxs, sizeof(uint16_t)*i_c);
  
  line_c += i_c;
  line_v += n;
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

void draw_edge(cpVect *points, int n, struct rgba color, int thickness, int closed, int flags, struct rgba line_color, float line_seg)
{
  static_assert(sizeof(cpVect) == 2*sizeof(float));
  if (thickness == 0) {
    thickness = 1;
  }

  /* todo: should be dashed, and filled. use a texture. */  
  /* draw polygon outline */
  if (cpveql(points[0], points[n-1])) {
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
    .u_mode = PAR_U_MODE_DISTANCE,
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
    vertices[i].pos = (cpVect){ .x = mesh->positions[i].x, .y = mesh->positions[i].y };
    vertices[i].uv[0] = mesh->annotations[i].u_along_curve;
    vertices[i].uv[1] = mesh->annotations[i].v_across_curve;
    vertices[i].color = color;
  }

  memcpy(poly_b+poly_v, vertices, sizeof(struct poly_vertex)*mesh->num_vertices);
  memcpy(poly_bi+poly_c, mesh->triangle_indices, sizeof(uint32_t)*mesh->num_triangles*3);
  
  poly_c += mesh->num_triangles*3;
  poly_v += mesh->num_vertices;    
  
  parsl_destroy_context(par_ctx);

  /* Now drawing the line outlines */
  if (thickness == 1) {
    draw_line(points,n,line_color,line_seg, 0, 0);
  } else {
    /* Draw inside and outside lines */
    cpVect in_p[n];
    cpVect out_p[n];
    
    for (int i = 0, v = 0; i < n*2+1; i+=2, v++)
      in_p[v] = vertices[i].pos;

    for (int i = 1, v = 0; i < n*2; i+=2,v++)
      out_p[v] = vertices[i].pos;

    draw_line(in_p,n,line_color,line_seg,1,0);
    draw_line(out_p,n,line_color,line_seg,1,0);
  }
}

void draw_circle(cpVect pos, float radius, float pixels, struct rgba color, float seg)
{
  struct circle_vertex cv;
  cv.pos = pos;
  cv.radius = radius;
  cv.color = color;
  cv.segsize = seg/radius;
  cv.fill = pixels/radius;
  memcpy(circle_b+circle_count, &cv, sizeof(struct circle_vertex));
  circle_count++;
}

void draw_box(struct cpVect c, struct cpVect wh, struct rgba color)
{
    float hw = wh.x / 2.f;
    float hh = wh.y / 2.f;

    cpVect verts[4] = {
      { .x = c.x-hw, .y = c.y-hh },
      { .x = c.x+hw, .y = c.y-hh },
      { .x = c.x+hw, .y = c.y+hh },
      { .x = c.x-hw, .y = c.y+hh }
    };
    
    draw_poly(verts, 4, color);
}

void draw_arrow(struct cpVect start, struct cpVect end, struct rgba color, int capsize)
{ 
  cpVect points[2] = {start, end};
  draw_line(points, 2, color, 0, 0,0);
  draw_cppoint(end, capsize, color);
}

void draw_grid(int width, int span, struct rgba color)
{
  cpVect offset = cam_pos();
  offset.x -= mainwin.width/2;
  offset.y -= mainwin.height/2;
  offset = cpvmult(offset, 1/cam_zoom());

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

void draw_cppoint(struct cpVect point, float r, struct rgba color)
{
  struct point_vertex p = {
    .pos = point,
    .color = color,
    .radius = r
  };

  memcpy(point_b+point_c, &p, sizeof(struct point_vertex));
  point_c++;
}

void draw_points(struct cpVect *points, int n, float size, struct rgba color)
{
    for (int i = 0; i < n; i++)
      draw_cppoint(points[i], size, color);
}

void draw_poly(cpVect *points, int n, struct rgba color)
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
    polyverts[i].pos = points[i];
    polyverts[i].uv[0] = 0.0;
    polyverts[i].uv[1] = 0.0;
    polyverts[i].color = color;
  }
  
  memcpy(poly_b+poly_v, polyverts, sizeof(struct poly_vertex)*n);
  memcpy(poly_bi+poly_c, tridxs, sizeof(uint32_t)*3*tric);
  
  poly_c += tric*3;
  poly_v += n;
}
