#include "registry.h"
#include "gameobject.h"
#include "2dphysics.h"
#include "editor.h"
#include "sprite.h"
#include "stb_ds.h"

struct component *components;
struct component_interface *interfaces;

void registry_init()
{

    register_component("Sprite",
                                         sizeof(struct sprite),
                                         make_sprite,
                                         sprite_delete,
                                         sprite_io,
                                         NULL,
                                         sprite_gui,
                                         sprite_init);

    register_component("2D Circle Collider",
                                          sizeof(struct phys2d_circle),
                                          Make2DCircle,
                                          phys2d_circledel,
                                          NULL,
                                          phys2d_dbgdrawcircle,
                                          circle_gui,
                                          phys2d_circleinit);

    register_component("2D Segment", sizeof(struct phys2d_segment), Make2DSegment, phys2d_segdel, NULL, phys2d_dbgdrawseg, segment_gui, phys2d_seginit);
    register_component("2D Box", sizeof(struct phys2d_box), Make2DBox, phys2d_boxdel, NULL, phys2d_dbgdrawbox, box_gui, phys2d_boxinit);
    register_component("2D Polygon", sizeof(struct phys2d_poly), Make2DPoly, phys2d_polydel, NULL, phys2d_dbgdrawpoly, poly_gui,phys2d_polyinit);
    register_component("2D Edge", sizeof(struct phys2d_edge), Make2DEdge, phys2d_edgedel, NULL, phys2d_dbgdrawedge, edge_gui, phys2d_edgeinit);
}

void register_component(const char *name, size_t size,
			void (*make)(struct gameobject * go),
			void (*delete)(void *data),
			void (*io)(void *data, FILE *f, int read),
			void(*draw_debug)(void *data),
			void(*draw_gui)(void *data),
			void(*init)(void *data, struct gameobject * go))
{
    struct component_interface c;
    c.name = name;
    c.make = make;
    c.io = io;
    c.draw_debug = draw_debug;
    c.draw_gui = draw_gui;
    c.delete = delete;
    c.init = init;
    arrput(interfaces, c);
}

struct component comp_make(struct component_interface *interface)
{
    struct component c;
    c.data = interface->make(NULL);
    c.ref = interface;

    return c;
}

void comp_draw_debug(struct component *c) {
    c->ref->draw_debug(c->data);
}

void comp_draw_gui(struct component *c) {
    c->ref->draw_gui(c->data);
}

void comp_delete(struct component *c)
{
    c->ref->delete(c->data);
}

void comp_init(struct component *c)
{
    c->ref->init(c->data);
}

void comp_io(struct component *c, int read)
{
    c->ref->io(c->data, read);
}