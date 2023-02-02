#include "gameobject.h"

#include "shader.h"
#include "sprite.h"
#include "2dphysics.h"
#include "script.h"
#include "input.h"
#include <string.h>
#include <chipmunk/chipmunk.h>
#include "resources.h"
#include "nuke.h"
#include "log.h"
#include "debugdraw.h"

#include "stb_ds.h"

struct gameobject *gameobjects = NULL;
static int first = -1;

const int nameBuf[MAXNAME] = { 0 };
const int prefabNameBuf[MAXNAME] = { 0 };

struct gameobject *get_gameobject_from_id(int id)
{  if (id < 0) return NULL;

    return &gameobjects[id];
}

struct gameobject *id2go(int id) {
    if (id < 0) return NULL;

    return &gameobjects[id];
}



int pos2gameobject(cpVect pos)
{
  cpShape *hit = phys2d_query_pos(pos);

  if (hit) {
    struct phys2d_shape *shape = cpShapeGetUserData(hit);
    return shape->go;
  }

  for (int i = 0; i < arrlen(gameobjects); i++) {
    cpVect gpos = cpBodyGetPosition(gameobjects[i].body);
    float dist = cpvlength(cpvsub(gpos, pos));

    if (dist <= 25) return i;
  }
  return -1;
}

int id_from_gameobject(struct gameobject *go) {
    for (int i = 0; i < arrlen(gameobjects); i++) {
        if (&gameobjects[i] == go) return i;
    }

    return -1;
}

void go_shape_apply(cpBody *body, cpShape *shape, struct gameobject *go)
{
    cpShapeSetFriction(shape, go->f);
    cpShapeSetElasticity(shape, go->e);
//    cpShapeSetFilter(shape, go->filter);
    
//    YughLog("Set filter; %d", go->filter.mask);
}

void gameobject_apply(struct gameobject *go)
{
    cpBodySetType(go->body, go->bodytype);

    if (go->bodytype == CP_BODY_TYPE_DYNAMIC)
        cpBodySetMass(go->body, go->mass);

    cpBodyEachShape(go->body, go_shape_apply, go);
}

static void gameobject_setpickcolor(struct gameobject *go)
{
/*
    float r = ((go->editor.id & 0x000000FF) >> 0) / 255.f;
    float g = ((go->editor.id & 0x0000FF00) >> 8) / 255.f;
    float b = ((go->editor.id & 0x00FF0000) >> 16) / 255.f;

    go->editor.color[0] = r;
    go->editor.color[1] = g;
    go->editor.color[2] = b;
    */
}

int MakeGameobject()
{
    struct gameobject go = {
        .scale = 1.f,
        .bodytype = CP_BODY_TYPE_STATIC,
        .mass = 1.f,
        .next = -1
    };

    go.body = cpSpaceAddBody(space, cpBodyNew(go.mass, 1.f));

    int retid;

    if (first<0) {
        arrput(gameobjects, go);
        retid = arrlast(gameobjects).id = arrlen(gameobjects)-1;
    } else {
        retid = first;
        first = id2go(first)->next;
        *id2go(retid) = go;
    }
    go.filter.group = retid;
    go.filter.mask = CP_ALL_CATEGORIES;
    go.filter.categories = CP_ALL_CATEGORIES;
    cpBodySetUserData(go.body, (int)retid);
    return retid;
}

void rm_body_shapes(cpBody *body, cpShape *shape, void *data) {
    struct phys2d_shape *s = cpShapeGetUserData(shape);
    if (s->data) {
      free(s->data);
      s->data = NULL;
    }
    cpSpaceRemoveShape(space, shape);
    cpShapeFree(shape);
}

/* Really more of a "mark for deletion" ... */
void gameobject_delete(int id)
{
    id2go(id)->next = first;
    first = id;
}

/* Free this gameobject */
void gameobject_clean(int id) {
    struct gameobject *go = id2go(id);
    cpBodyEachShape(go->body, rm_body_shapes, NULL);
    cpSpaceRemoveBody(space, go->body);
    cpBodyFree(go->body);
    go->body = NULL;
}

void gameobjects_cleanup() {
    int clean = first;

    while (clean > 0 && id2go(clean)->body) {
        gameobject_clean(clean);
        clean = id2go(clean)->next;
    }
}

void gameobject_save(struct gameobject *go, FILE * file)
{
/*
    fwrite(go, sizeof(*go), 1, file);

    YughInfo("Number of components is %d.", arrlen(go->components));

    int n = arrlen(go->components);
    fwrite(&n, sizeof(n), 1, file);
    for (int i = 0; i < n; i++) {
        fwrite(&go->components[i].id, sizeof(int), 1, file);

        if (go->components[i].io == NULL)
            fwrite(go->components[i].data, go->components[i].datasize, 1, file);
        else
            go->components[i].io(go->components[i].data, file, 0);
    }
    */
}

void gameobject_init(struct gameobject *go, FILE * fprefab)
{
/*
    go->body = cpSpaceAddBody(space, cpBodyNew(go->mass, 1.f));
    cpBodySetType(go->body, go->bodytype);
    cpBodySetUserData(go->body, go);



    int comp_n;
    fread(&comp_n, sizeof(int), 1, fprefab);
    arrfree(go->components);
    int n;

    for (int i = 0; i < comp_n; i++) {
        fread(&n, sizeof(int), 1, fprefab);
        arrput(go->components, components[n]);
        struct component *newc = &arrlast(go->components);
        newc->go = go;
        newc->data = newc->make(newc->go);

        if (newc->io == NULL)
            fread(newc->data, newc->datasize, 1, fprefab);
        else
            newc->io(newc->data, fprefab, 1);

        newc->init(newc->data, go);
    }
    */
}

void gameobject_saveprefab(struct gameobject *go)
{
/*
    char prefabfname[60] = { '\0' };
    strncat(prefabfname, go->editor.prefabName, MAXNAME);
    strncat(prefabfname, EXT_PREFAB, 10);
    FILE *pfile = fopen(prefabfname, "wb+");
    gameobject_save(go, pfile);
    fclose(pfile);

    findPrefabs();
    */
}

void gameobject_syncprefabs(char *revertPath)
{
/*
   struct gameobject **go = objects;
   int i = 0;
   while(i != nobjects) {
   	if ((*go)->editor.curPrefabPath && !strcmp((*go)->editor.curPrefabPath, revertPath)) { ; }//objectRevertPrefab(go); //TODO: revertprefab
   }
*/
}

void gameobject_revertprefab(struct gameobject *go)
{

}

void toggleprefab(struct gameobject *go)
{
/*
    go->editor.prefabSync = !go->editor.prefabSync;

    if (go->editor.prefabSync) {
	strcpy(go->editor.prefabName, go->editor.rootPrefabName);
	gameobject_revertprefab(go);	//TODO: object revert prefab
    } else {
	go->editor.prefabName[0] = '\0';
    }
    */
}

void gameobject_move(struct gameobject *go, cpVect vec)
{
    cpVect p = cpBodyGetPosition(go->body);
    p.x += vec.x;
    p.y += vec.y;
    cpBodySetPosition(go->body, p);

    phys2d_reindex_body(go->body);
}

void gameobject_rotate(struct gameobject *go, float as)
{
    cpFloat a = cpBodyGetAngle(go->body);
    a += as * deltaT;
    cpBodySetAngle(go->body, a);

        phys2d_reindex_body(go->body);
}

void gameobject_setangle(struct gameobject *go, float angle) {
    cpBodySetAngle(go->body, angle);

        phys2d_reindex_body(go->body);
}

void gameobject_setpos(struct gameobject *go, cpVect vec) {
    if (!go || !go->body) return;
    cpBodySetPosition(go->body, vec);

    phys2d_reindex_body(go->body);
}

void object_gui(struct gameobject *go)
{
/*
    float temp_pos[2];
    temp_pos[0] = cpBodyGetPosition(go->body).x;
    temp_pos[1] = cpBodyGetPosition(go->body).y;

    draw_point(temp_pos[0], temp_pos[1], 3);

    nuke_property_float2("Position", -1000000.f, temp_pos, 1000000.f, 1.f, 0.5f);

    cpVect tvect = { temp_pos[0], temp_pos[1] };
    cpBodySetPosition(go->body, tvect);

    float mtry = cpBodyGetAngle(go->body);
    float modtry = fmodf(mtry * RAD2DEGS, 360.f);
    if (modtry < 0.f)
      modtry += 360.f;

    float modtry2 = modtry;
    nuke_property_float("Angle", -1000.f, &modtry, 1000.f, 0.5f, 0.5f);
    modtry -= modtry2;
    cpBodySetAngle(go->body, mtry + (modtry * DEG2RADS));

    nuke_property_float("Scale", 0.f, &go->scale, 1000.f, 0.01f, go->scale * 0.01f);

    nuke_nel(3);
    nuke_radio_btn("Static", &go->bodytype, CP_BODY_TYPE_STATIC);
    nuke_radio_btn("Dynamic", &go->bodytype, CP_BODY_TYPE_DYNAMIC);
    nuke_radio_btn("Kinematic", &go->bodytype, CP_BODY_TYPE_KINEMATIC);

    cpBodySetType(go->body, go->bodytype);

    if (go->bodytype == CP_BODY_TYPE_DYNAMIC) {
         nuke_property_float("Mass", 0.01f, &go->mass, 1000.f, 0.01f, 0.01f);
	cpBodySetMass(go->body, go->mass);
    }

    nuke_property_float("Friction", 0.f, &go->f, 10.f, 0.01f, 0.01f);
    nuke_property_float("Elasticity", 0.f, &go->e, 2.f, 0.01f, 0.01f);

    int n = -1;



    for (int i = 0; i < arrlen(go->components); i++) {
	struct component *c = &go->components[i];

         comp_draw_debug(c);

     nuke_nel(5);
     if (nuke_btn("Del")) n = i;

     if (nuke_push_tree_id(c->ref->name, i)) {
	    comp_draw_gui(c);
	    nuke_tree_pop();
	}


    }

    if (n >= 0)
	gameobject_delcomponent(go, n);
*/
}


void body_draw_shapes_dbg(cpBody *body, cpShape *shape, void *data) {
    struct phys2d_shape *s = cpShapeGetUserData(shape);
    s->debugdraw(s->data);
}

void gameobject_draw_debugs() {
    for (int i = 0; i < arrlen(gameobjects); i++) {
        if (!gameobjects[i].body) continue;

        cpVect pos = cpBodyGetPosition(gameobjects[i].body);
        float color[3] = {0.76f, 0.38f, 1.f};
        draw_point(pos.x, pos.y, 3.f, color);
        cpBodyEachShape(gameobjects[i].body, body_draw_shapes_dbg, NULL);
    }

}


static struct {struct gameobject go; cpVect pos; float angle; } *saveobjects = NULL;

void gameobject_saveall() {
    arrfree(saveobjects);
    arrsetlen(saveobjects, arrlen(gameobjects));

    for (int i = 0; i < arrlen(gameobjects); i++) {
        saveobjects[i].go = gameobjects[i];
        saveobjects[i].pos = cpBodyGetPosition(gameobjects[i].body);
        saveobjects[i].angle = cpBodyGetAngle(gameobjects[i].body);
    }
}

void gameobject_loadall() {
    YughInfo("N gameobjects: %d, N saved: %d", arrlen(gameobjects), arrlen(saveobjects));
    for (int i = 0; i < arrlen(saveobjects); i++) {
        gameobjects[i] = saveobjects[i].go;
        cpBodySetPosition(gameobjects[i].body, saveobjects[i].pos);
        cpBodySetAngle(gameobjects[i].body, saveobjects[i].angle);
        cpBodySetVelocity(gameobjects[i].body, cpvzero);
        cpBodySetAngularVelocity(gameobjects[i].body, 0.f);
    }

    arrfree(saveobjects);
}

int gameobjects_saved() {
    return arrlen(saveobjects);
}
