#include "gameobject.h"

#include "shader.h"
#include "sprite.h"
#include "registry.h"
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

const int nameBuf[MAXNAME] = { 0 };
const int prefabNameBuf[MAXNAME] = { 0 };

struct gameobject *get_gameobject_from_id(int id)
{
    return &gameobjects[id];
}

int id_from_gameobject(struct gameobject *go) {
    for (int i = 0; i < arrlen(gameobjects); i++) {
        if (&gameobjects[i] == i) return i;
    }

    return -1;
}

static void gameobject_setpickcolor(struct gameobject *go)
{
    float r = ((go->editor.id & 0x000000FF) >> 0) / 255.f;
    float g = ((go->editor.id & 0x0000FF00) >> 8) / 255.f;
    float b = ((go->editor.id & 0x00FF0000) >> 16) / 255.f;

    go->editor.color[0] = r;
    go->editor.color[1] = g;
    go->editor.color[2] = b;
}

int MakeGameobject()
{
    if (gameobjects == NULL) arrsetcap(gameobjects, 100);

    YughInfo("Making new gameobject");
    struct gameobject go = {
        .editor.id = arrlen(gameobjects),
        .transform.scale = 1.f,
        .scale = 1.f,
        .bodytype = CP_BODY_TYPE_STATIC,
        .mass = 1.f
    };

    gameobject_setpickcolor(&go);
    strncpy(go.editor.mname, "New object", MAXNAME);
    go.body = cpSpaceAddBody(space, cpBodyNew(go.mass, 1.f));

    arrput(gameobjects, go);

    cpBodySetUserData(go.body, &arrlast(gameobjects));

    return arrlen(gameobjects)-1;
}

void gameobject_addcomponent(struct gameobject *go, struct component_interface *c)
{
    struct component new;
    new.interface = c;
    new.data = c->make(go);
    new.go = go;
    arrput(go->components, c);
}

void gameobject_delete(int id)
{
    YughInfo("Deleting gameobject with id %d.", id);
    struct gameobject *go = &gameobjects[id];
    for (int i = 0; i < arrlen(go->components); i++) {
        go->components[i].delete(go->components[i].data);
        arrdel(go->components, i);
    }



    arrdelswap(gameobjects, id);
}

void gameobject_delcomponent(struct gameobject *go, int n)
{
    go->components[n].delete(go->components[n].data);
    arrdel(go->components, n);
}

void setup_model_transform(struct mTransform *t, struct shader *s, float scale)
{
    mfloat_t modelT[16] = { 0.f };
    mfloat_t matbuff[16] = { 0.f };
    memcpy(modelT, UNITMAT4, sizeof(modelT));
    mat4_translate_vec3(modelT, t->position);
    mat4_multiply(modelT, modelT, mat4_rotation_quat(matbuff, t->rotation));
    mat4_scale_vec3f(modelT, scale);
    shader_setmat4(s, "model", modelT);

}

void gameobject_save(struct gameobject *go, FILE * file)
{
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
}

int gameobject_makefromprefab(char *path)
{
    FILE *fprefab = fopen(path, "rb");
    if (fprefab == NULL) {
        YughError("Could not find prefab %s.", path);
	return -1;
    }

    struct gameobject *new = get_gameobject_from_id(MakeGameobject());
    fread(new, sizeof(*new), 1, fprefab);
    new->components = NULL;

    gameobject_init(new, fprefab);

    fclose(fprefab);

    arrlast(gameobjects).editor.id = arrlen(gameobjects)-1;

    return arrlen(gameobjects)-1;
}

void gameobject_init(struct gameobject *go, FILE * fprefab)
{
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


}

void gameobject_saveprefab(struct gameobject *go)
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
    go->editor.prefabSync = !go->editor.prefabSync;

    if (go->editor.prefabSync) {
	strcpy(go->editor.prefabName, go->editor.rootPrefabName);
	gameobject_revertprefab(go);	//TODO: object revert prefab
    } else {
	go->editor.prefabName[0] = '\0';
    }
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

	if (c->draw_debug)
	    c->draw_debug(c->data);


     nuke_nel(5);
     if (nuke_btn("Del")) n = i;

     if (nuke_push_tree_id(c->name, i)) {
	    c->draw_gui(c->data);
	    nuke_tree_pop();
	}


    }

    if (n >= 0)
	gameobject_delcomponent(go, n);

}

void gameobject_draw_debugs() {
    for (int i = 0; i < arrlen(gameobjects); i++) {
        for (int j = 0; j < arrlen(gameobjects[i].components); j++) {
            struct component *c = &gameobjects[i].components[j];

            if (c->draw_debug) c->draw_debug(c->data);
        }
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
