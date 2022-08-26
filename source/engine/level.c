#include "level.h"
#include <stdio.h>
#include <string.h>
#include "gameobject.h"
#include "resources.h"

#include "stb_ds.h"

void save_level(char name[MAXNAME])
{
    FILE *lfile = res_open(name, "wb+");

    if (!lfile) return;


    int objs = arrlen(gameobjects);
    fwrite(&objs, sizeof(objs), 1, lfile);

    for (int i = 0; i < objs; i++)
	gameobject_save(&gameobjects[i], lfile);

    fclose(lfile);
}

void load_level(char name[MAXNAME])
{
    FILE *lfile = fopen(name, "rb");

    if (!lfile) return;

    new_level();

    int objs;
    fread(&objs, sizeof(objs), 1, lfile);

    arraddn(gameobjects, objs);

    for (int i = 0; i < objs; i++) {
	struct mGameObject *go = &gameobjects[i];
	fread(go, sizeof(struct mGameObject), 1, lfile);
	go->components = NULL;
	gameobject_init(go, lfile);
    }

    fclose(lfile);
}

void new_level()
{
    for (int i = 0; i < arrlen(gameobjects); i++)
        gameobject_delete(i);

    arrfree(gameobjects);
}
