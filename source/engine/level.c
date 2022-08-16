#include "level.h"
#include <stdio.h>
#include <string.h>
#include "vec.h"
#include "gameobject.h"
#include "resources.h"

void save_level(char name[MAXNAME])
{
    FILE *lfile = res_open(name, "wb+");

    if (!lfile) return;


    int objs = gameobjects->len;
    fwrite(&objs, sizeof(objs), 1, lfile);

    for (int i = 0; i < objs; i++) {
	gameobject_save(vec_get(gameobjects, i), lfile);
    }

    fclose(lfile);
}

void load_level(char name[MAXNAME])
{
    FILE *lfile = fopen(name, "rb");

    if (!lfile) return;


    int objs;
    fread(&objs, sizeof(objs), 1, lfile);

    vec_clear(gameobjects);


    for (int i = 0; i < objs; i++) {
	struct mGameObject *go = vec_add(gameobjects, NULL);
	fread(go, sizeof(struct mGameObject), 1, lfile);
	go->components = vec_make(1,1);
	gameobject_init(go, lfile);
    }

    fclose(lfile);

}

void new_level()
{
    vec_clear(gameobjects);
}
