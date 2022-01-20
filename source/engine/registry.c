#include "registry.h"
#include "gameobject.h"
#include "2dphysics.h"
#include <editor.h>
#include "sprite.h"
#include "pinball.h"

struct component components[MAXNAME] = { 0 };

int ncomponent = 0;

void registry_init()
{
    register_component("Sprite", sizeof(struct mSprite), &MakeSprite, NULL,
		       sprite_gui, &sprite_init, NULL);
    register_component("2D Circle Collider", sizeof(struct phys2d_circle),
		       &Make2DCircle, &phys2d_dbgdrawcircle, &circle_gui,
		       &phys2d_circleinit, NULL);
    register_component("2D Segment", sizeof(struct phys2d_segment),
		       &Make2DSegment, &phys2d_dbgdrawseg, &segment_gui,
		       &phys2d_seginit, NULL);
    register_component("2D Box", sizeof(struct phys2d_box), &Make2DBox,
		       &phys2d_dbgdrawbox, &box_gui, &phys2d_boxinit,
		       NULL);
    register_component("2D Polygon", sizeof(struct phys2d_poly),
		       &Make2DPoly, &phys2d_dbgdrawpoly, &poly_gui,
		       &phys2d_polyinit, NULL);
    register_component("2D Edge", sizeof(struct phys2d_edge), &Make2DEdge,
		       &phys2d_dbgdrawedge, &edge_gui, &phys2d_edgeinit,
		       NULL);
    register_component("Flipper", sizeof(struct flipper),
		       &pinball_flipper_make, NULL, &pinball_flipper_gui,
		       &pinball_flipper_init, &pinball_flipper_update);
}

void register_component(const char *name, size_t size,
			void (*make)(struct mGameObject * go,
				     struct component * c),
			void(*draw_debug)(void *data),
			void(*draw_gui)(void *data),
			void(*init)(void *data, struct mGameObject * go),
			void(*update)(void *data, struct mGameObject * go))
{
    struct component *c = &components[ncomponent++];
    c->name = name;
    c->make = make;
    c->draw_debug = draw_debug;
    c->draw_gui = draw_gui;
    c->init = init;
    c->data = NULL;
    c->id = ncomponent - 1;
    c->datasize = size;
}
