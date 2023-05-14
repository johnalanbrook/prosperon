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

static sg_pipeline grid_pipe;
static sg_bindings grid_bind;
static sg_shader grid_shader;
static int grid_c = 0;

static sg_pipeline rect_pipe;
static sg_bindings rect_bind;
static sg_shader rect_shader;
static int rect_c = 0;

static sg_pipeline poly_pipe;
static sg_bindings poly_bind;
static sg_shader poly_shader;
static int poly_c = 0;
static int poly_v = 0;
static int poly_vc = 7;

static sg_pipeline circle_pipe;
static sg_bindings circle_bind;
static sg_shader csg;
static int circle_count = 0;
static int circle_vert_c = 7;

void debug_flush()
{
  sg_apply_pipeline(circle_pipe);
  sg_apply_bindings(&circle_bind);
  sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE_REF(projection));
  
  sg_draw(0,4,circle_count);
  circle_count = 0;

  sg_apply_pipeline(rect_pipe);
  sg_apply_bindings(&rect_bind);
  sg_apply_uniforms(SG_SHADERSTAGE_VS,0,SG_RANGE_REF(projection));
  sg_draw(0,rect_c*2,1);
  rect_c = 0;
  
  sg_apply_pipeline(poly_pipe);
  sg_apply_bindings(&poly_bind);
  sg_apply_uniforms(SG_SHADERSTAGE_VS,0,SG_RANGE_REF(projection));
  sg_draw(0,poly_c,1);
  poly_c = 0;
  poly_v = 0;
}

static sg_shader_uniform_block_desc projection_ubo = {
  .size = sizeof(projection),
  .uniforms = {
    [0] = { .name = "proj", .type = SG_UNIFORMTYPE_MAT4 },
  }
};

void debugdraw_init()
{
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
	  [1].format = SG_VERTEXFORMAT_FLOAT3,
	  [2].format = SG_VERTEXFORMAT_FLOAT2,
	  [3].format = SG_VERTEXFORMAT_FLOAT
	},
      .buffers[0].step_func = SG_VERTEXSTEP_PER_INSTANCE,
    },
    .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
    .cull_mode = SG_CULLMODE_BACK,
    .colors[0].blend = {
      .enabled = true,
      .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
      .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
      .src_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA,
      .src_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
    },
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
      .size = sizeof(float)*5,
      .uniforms = {
        [0] = { .name = "thickness", .type = SG_UNIFORMTYPE_FLOAT },
	[1] = { .name = "span", .type = SG_UNIFORMTYPE_FLOAT },
	[2] = { .name = "color", .type = SG_UNIFORMTYPE_FLOAT3 },
     }
   },
  });

  grid_pipe = sg_make_pipeline(&(sg_pipeline_desc){
    .shader = grid_shader,
    .layout = {
      .attrs = {
        [0].format = SG_VERTEXFORMAT_FLOAT2
      }
    },
    .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
//    .cull_mode = SG_CULLMODE_BACK,
    .label = "grid pipeline",
    .colors[0] = {
      .blend = {
        .enabled = true,
	.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
	.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
	.op_rgb = SG_BLENDOP_ADD,
	.src_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA,
	.dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
	.op_alpha = SG_BLENDOP_ADD
      },
    },
  });

  grid_bind.vertex_buffers[0] = circle_bind.vertex_buffers[1];

  rect_shader = sg_make_shader(&(sg_shader_desc){
    .vs.source = slurp_text("shaders/linevert.glsl"),
    .fs.source = slurp_text("shaders/linefrag.glsl"),
    .vs.uniform_blocks[0] = projection_ubo
  });

  rect_pipe = sg_make_pipeline(&(sg_pipeline_desc){
    .shader = rect_shader,
    .layout = {
      .attrs = { [0].format = SG_VERTEXFORMAT_FLOAT2 }
    },
    .primitive_type = SG_PRIMITIVETYPE_LINES
  });

  rect_bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
    .size = sizeof(float)*2*10000,
    .usage = SG_USAGE_STREAM
  });
  
  poly_shader = sg_make_shader(&(sg_shader_desc){
    .vs.source = slurp_text("shaders/poly_v.glsl"),
    .fs.source = slurp_text("shaders/poly_f.glsl"),
    .vs.uniform_blocks[0] = projection_ubo,
  });
  
  poly_pipe = sg_make_pipeline(&(sg_pipeline_desc){
    .shader = poly_shader,
    .layout = {
      .attrs = { [0].format = SG_VERTEXFORMAT_FLOAT2,
		 [1].format = SG_VERTEXFORMAT_FLOAT2,
		 [2].format = SG_VERTEXFORMAT_FLOAT3
       }
    },
    .index_type = SG_INDEXTYPE_UINT32
  });
  
  poly_bind.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
    .size = sizeof(float)*poly_vc*1000,
    .usage = SG_USAGE_STREAM,
    .type = SG_BUFFERTYPE_VERTEXBUFFER,
  });
  
/*  poly_bind.vertex_buffers[1] = sg_make_buffer(&(sg_buffer_desc){
    .size = sizeof(float)*1000,
    .usage = SG_USAGE_STREAM,
  });
*/
      
  poly_bind.index_buffer = sg_make_buffer(&(sg_buffer_desc){
    .size = sizeof(uint32_t)*6*1000,
    .usage = SG_USAGE_STREAM,
    .type = SG_BUFFERTYPE_INDEXBUFFER
  });
}

void draw_line(cpVect s, cpVect e, float *color)
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

void draw_edge(cpVect  *points, int n, struct color color, int thickness)
{
  return;
    static_assert(sizeof(cpVect) == 2*sizeof(float));

  float col[3] = {(float)color.r/255, (float)color.g/255, (float)color.b/255};
  
  /* TODO: Should be dashed, and filled. Use a texture. */  
    
  parsl_position par_v[n];
  
  for (int i = 0; i < n; i++) {
    par_v[i].x = points[i].x;
    par_v[i].y = points[i].y;
  }

  uint16_t spine_lens[] = {n};
  
  parsl_context *par_ctx = parsl_create_context((parsl_config){
    .thickness = thickness
  });
  
  parsl_mesh *mesh = parsl_mesh_from_lines(par_ctx, (parsl_spine_list){
    .num_vertices = n,
    .num_spines = 1,
    .vertices = points,
    .spine_lengths = spine_lens,
  });
  
  sg_range pt = {
    .ptr = mesh->positions,
    .size = sizeof(float)*2*mesh->num_vertices
  };
  
  for (int i = 0; i < mesh->num_triangles*3; i++)
    mesh->triangle_indices[i] += poly_v;
    
  float mesh_colors[3*mesh->num_vertices];
  
  for (int i = 0; i < 3*mesh->num_vertices; i+=3)
    for (int j = 0; j < 3; j++)
      mesh_colors[i+j] = col[j];
      
      
  sg_range it = {
    .ptr = mesh->triangle_indices,
    .size = sizeof(uint32_t)*mesh->num_triangles*3
  };
  
  sg_range ct = {
    .ptr = mesh_colors,
    .size = sizeof(float)*mesh->num_vertices*3
  };
  
  sg_append_buffer(poly_bind.vertex_buffers[0], &pt);
  sg_append_buffer(poly_bind.index_buffer, &it);
  sg_append_buffer(poly_bind.vertex_buffers[1], &ct);
  
  poly_c += mesh->num_triangles*3;
  poly_v += mesh->num_vertices;
  
  parsl_destroy_context(par_ctx);
}

void draw_circle(int x, int y, float radius, int pixels, float *color, int fill)
{
  float cv[circle_vert_c];
  cv[0] = color[0];
  cv[1] = color[1];
  cv[2] = color[2];
  cv[3] = x;
  cv[4] = y;
  cv[5] = radius;
  cv[6] = fill;
  sg_append_buffer(circle_bind.vertex_buffers[0], SG_RANGE_REF(cv));
  circle_count++;
}

void draw_rect(int x, int y, int w, int h, float *color)
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

void draw_box(struct cpVect c, struct cpVect wh, struct color color)
{
  float col[3] = {(float)color.r/255, (float)color.g/255, (float)color.b/255};
  draw_rect(c.x, c.y, wh.x, wh.y, col);
}

void draw_arrow(struct cpVect start, struct cpVect end, struct color color, int capsize)
{
  float col[3] = {(float)color.r/255, (float)color.g/255, (float)color.b/255};
  draw_line(start, end, col);
  
  draw_cppoint(end, capsize, color);
}

void draw_grid(int width, int span)
{
    cpVect offset = cam_pos();
    offset = cpvmult(offset, 1/cam_zoom());
    offset.x -= mainwin->width/2;
    offset.y -= mainwin->height/2;

  sg_apply_pipeline(grid_pipe);
  sg_apply_bindings(&grid_bind);

  float col[3] = { 0.3, 0.5, 0.8};

  float fubo[5];
  fubo[0] = width;
  fubo[1] = span;
  fubo[2] = col;
  
  sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, SG_RANGE_REF(projection));
  sg_apply_uniforms(SG_SHADERSTAGE_VS, 1, SG_RANGE_REF(offset));
  sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, SG_RANGE_REF(fubo));

  sg_draw(0,4,1);
}

void draw_point(int x, int y, float r, float *color)
{
    draw_circle(x,y,r,r,color,1);
}

void draw_cppoint(struct cpVect point, float r, struct color color)
{
    float col[3] = {(float)color.r/255, (float)color.g/255, (float)color.b/255};
    draw_point(point.x, point.y, r, col);
}

void draw_points(struct cpVect *points, int n, float size, float *color)
{
    for (int i = 0; i < n; i++)
        draw_point(points[i].x, points[i].y, size, color);
}

void draw_poly(cpVect *points, int n, float *color)
{
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
    .closed = true
  });
  
  for (int i = 0; i < mesh->num_triangles*3; i++)
    mesh->triangle_indices[i] += poly_v;
      
  sg_range it = {
    .ptr = mesh->triangle_indices,
    .size = sizeof(uint32_t)*mesh->num_triangles*3
  };
  
  float vertices[poly_vc*mesh->num_vertices];
  
  for (int i = 0, vert = 0; i < mesh->num_vertices; i++, vert+=poly_vc) {
    vertices[vert] = mesh->positions[i].x;
    vertices[vert+1] = mesh->positions[i].y;
    vertices[vert+2] = mesh->annotations[i].u_along_curve;
    vertices[vert+3] = mesh->annotations[i].v_across_curve;
    vertices[vert+4] = color[0];
    vertices[vert+5] = color[1];
    vertices[vert+6] = color[2];
  }
  
  sg_range vvt = {
    .ptr = vertices,
    .size = sizeof(float)*poly_vc*mesh->num_vertices
  };

  sg_append_buffer(poly_bind.vertex_buffers[0], &vvt);  
  sg_append_buffer(poly_bind.index_buffer, &it);
  
  poly_c += mesh->num_triangles*3;
  poly_v += mesh->num_vertices;
  
  parsl_destroy_context(par_ctx);
  
  return;
}

void debugdraw_flush()
{

}
