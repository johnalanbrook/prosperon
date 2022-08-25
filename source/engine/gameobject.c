#include "gameobject.h"

#include "shader.h"
#include "sprite.h"
#include "registry.h"
#include "2dphysics.h"
#include "script.h"
#include "vec.h"
#include "input.h"
#include <string.h>
#include <chipmunk/chipmunk.h>
#include "resources.h"
#include "nuke.h"

struct vec *gameobjects = NULL;

const int nameBuf[MAXNAME] = { 0 };
const int prefabNameBuf[MAXNAME] = { 0 };

void init_gameobjects()
{
    gameobjects = vec_make(sizeof(struct mGameObject), 100);
}

struct mGameObject *get_gameobject_from_id(int id)
{
    return vec_get(gameobjects, id - 1);
}

static void gameobject_setpickcolor(struct mGameObject *go)
{
    float r = ((go->editor.id & 0x000000FF) >> 0) / 255.f;
    float g = ((go->editor.id & 0x0000FF00) >> 8) / 255.f;
    float b = ((go->editor.id & 0x00FF0000) >> 16) / 255.f;

    go->editor.color[0] = r;
    go->editor.color[1] = g;
    go->editor.color[2] = b;
}

struct mGameObject *MakeGameobject()
{
    struct mGameObject *go = vec_add(gameobjects, NULL);
    go->editor.id = gameobjects->len - 1;
    go->transform.scale = 1.f;
    gameobject_setpickcolor(go);
    strncpy(go->editor.mname, "New object", MAXNAME);
    go->scale = 1.f;
    go->bodytype = CP_BODY_TYPE_STATIC;
    go->mass = 1.f;
    go->body = cpSpaceAddBody(space, cpBodyNew(go->mass, 1.f));

    go->components = vec_make(sizeof(struct component), 10);

    return go;
}

void gameobject_addcomponent(struct mGameObject *go, struct component *c)
{
    struct component *newc = vec_add(go->components, c);
    newc->go = go;
    newc->data = newc->make(newc->go);
}

void gameobject_delete(int id)
{
    vec_delete(gameobjects, id);
}

void gameobject_delcomponent(struct mGameObject *go, int n)
{
    vec_del_order(go->components, n);
}

void setup_model_transform(struct mTransform *t, struct mShader *s, float scale)
{
    mfloat_t modelT[16] = { 0.f };
    mfloat_t matbuff[16] = { 0.f };
    memcpy(modelT, UNITMAT4, sizeof(modelT));
    mat4_translate_vec3(modelT, t->position);
    mat4_multiply(modelT, modelT, mat4_rotation_quat(matbuff, t->rotation));
    mat4_scale_vec3f(modelT, scale);
    shader_setmat4(s, "model", modelT);

}

void gameobject_save(struct mGameObject *go, FILE * file)
{
    fwrite(go, sizeof(*go), 1, file);

    vec_store(go->components, file);
    for (int i = 0; i < go->components->len; i++) {
	struct component *c = vec_get(go->components, i);
	fwrite(c->data, c->datasize, 1, file);
    }
}

void gameobject_makefromprefab(char *path)
{
    FILE *fprefab = fopen(path, "rb");
    if (fprefab == NULL) {
	return;
    }

    struct mGameObject *new = MakeGameobject();
    struct vec *hold = new->components;
    fread(new, sizeof(*new), 1, fprefab);
    new->components = hold;

    new->editor.id = gameobjects->len - 1;

    gameobject_init(new, fprefab);

    fclose(fprefab);
}

void gameobject_init(struct mGameObject *go, FILE * fprefab)
{
    go->body = cpSpaceAddBody(space, cpBodyNew(go->mass, 1.f));

    vec_load(go->components, fprefab);

    for (int i = 0; i < go->components->len; i++) {
	vec_set(go->components, i, &components[((struct component *) vec_get(go->components, i))->id]);
	struct component *newc = vec_get(go->components, i);
	newc->go = go;
	newc->data = malloc(newc->datasize);
	fread(newc->data, newc->datasize, 1, fprefab);
	newc->init(newc->data, go);
    }
}

void gameobject_saveprefab(struct mGameObject *go)
{
    char prefabfname[60] = { '\0' };
    strncat(prefabfname, go->editor.prefabName, MAXNAME);
    strncat(prefabfname, EXT_PREFAB, 10);
    FILE *pfile = fopen(prefabfname, "wb+");
    gameobject_save(go, pfile);
    fclose(pfile);

    findPrefabs();
}





void gameobject_syncprefabs(char *revertPath)
{
/*
   struct mGameObject **go = objects;
   int i = 0;
   while(i != nobjects) {
   	if ((*go)->editor.curPrefabPath && !strcmp((*go)->editor.curPrefabPath, revertPath)) { ; }//objectRevertPrefab(go); //TODO: revertprefab
   }
*/
}

void gameobject_revertprefab(struct mGameObject *go)
{

}

void toggleprefab(struct mGameObject *go)
{
    go->editor.prefabSync = !go->editor.prefabSync;

    if (go->editor.prefabSync) {
	strcpy(go->editor.prefabName, go->editor.rootPrefabName);
	gameobject_revertprefab(go);	//TODO: object revert prefab
    } else {
	go->editor.prefabName[0] = '\0';
    }
}

void gameobject_update(struct mGameObject *go)
{
    if (go->script) {
	script_run(go->script);
    }
}

void gameobject_move(struct mGameObject *go, float xs, float ys)
{
    cpVect p = cpBodyGetPosition(go->body);
    p.x += xs * deltaT;
    p.y += ys * deltaT;
    cpBodySetPosition(go->body, p);
}

void gameobject_rotate(struct mGameObject *go, float as)
{
    cpFloat a = cpBodyGetAngle(go->body);
    a += as * deltaT;
    cpBodySetAngle(go->body, a);
}

void update_gameobjects() {
    vec_walk(gameobjects, &gameobject_update);
}


void object_gui(struct mGameObject *go)
{
    float temp_pos[2];
    temp_pos[0] = cpBodyGetPosition(go->body).x;
    temp_pos[1] = cpBodyGetPosition(go->body).y;

    draw_point(temp_pos[0], temp_pos[1], 3);

    nk_property_float2(ctx, "Position", -1000000.f, temp_pos, 1000000.f, 1.f, 0.5f);

    cpVect tvect = { temp_pos[0], temp_pos[1] };
    cpBodySetPosition(go->body, tvect);

    float mtry = cpBodyGetAngle(go->body);
    float modtry = fmodf(mtry * RAD2DEGS, 360.f);
    if (modtry < 0.f)
      modtry += 360.f;

    float modtry2 = modtry;
    nk_property_float(ctx, "Angle", -1000.f, &modtry, 1000.f, 0.5f, 0.5f);
    modtry -= modtry2;
    cpBodySetAngle(go->body, mtry + (modtry * DEG2RADS));

    nk_property_float(ctx, "Scale", 0.f, &go->scale, 1000.f, 0.01f, go->scale * 0.01f);

    nk_layout_row_dynamic(ctx, 25, 3);
    nk_radio_button_label(ctx, "Static", &go->bodytype, CP_BODY_TYPE_STATIC);
    nk_radio_button_label(ctx, "Dynamic", &go->bodytype, CP_BODY_TYPE_DYNAMIC);
    nk_radio_button_label(ctx, "Kinematic", &go->bodytype, CP_BODY_TYPE_KINEMATIC);

    cpBodySetType(go->body, go->bodytype);

    if (go->bodytype == CP_BODY_TYPE_DYNAMIC) {
         nk_property_float(ctx, "Mass", 0.01f, &go->mass, 1000.f, 0.01f, 0.01f);
	cpBodySetMass(go->body, go->mass);
    }

    nk_property_float(ctx, "Friction", 0.f, &go->f, 10.f, 0.01f, 0.01f);
    nk_property_float(ctx, "Elasticity", 0.f, &go->e, 2.f, 0.01f, 0.01f);

    int n = -1;



    for (int i = 0; i < go->components->len; i++) {
	struct component *c = vec_get(go->components, i);

	if (c->draw_debug)
	    c->draw_debug(c->data);


 nuke_nel(5);
     if (nk_button_label(ctx, "Del")) n = i;
	if (nk_tree_push_id(ctx, NK_TREE_NODE, c->name, NK_MINIMIZED, i)) {

	    c->draw_gui(c->data);

	    nk_tree_pop(ctx);
	}


    }

    if (n >= 0)
	gameobject_delcomponent(go, n);

}