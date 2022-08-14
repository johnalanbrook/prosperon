#ifndef EDITOR_H
#define EDITOR_H

#include <config.h>
#include <stdbool.h>
#include "resources.h"

#define ASSET_TYPE_NULL 0
#define ASSET_TYPE_IMAGE 1
#define ASSET_TYPE_TEXT 2

struct fileasset {
    char *filename;
    short extension_len;
    short filename_len;
    bool searched;
    short type;
    void *data;
};

struct editorVars {
    bool showStats;
    bool showHierarchy;
    bool showLighting;
    bool showGameSettings;
    bool showViewmode;
    bool showDebugMenu;
    bool showAssetMenu;
    bool showREPL;
    bool showExport;
    bool showLevel;
};

struct vec;
struct gameproject;
struct mSprite;

extern struct gameproject *cur_project;
extern struct vec *projects;


struct Texture;
struct mSDLWindow;

void pickGameObject(int pickID);
int is_allowed_extension(const char *ext);

void editor_init(struct mSDLWindow *window);
void editor_input();
void editor_render();
int editor_wantkeyboard();
void editor_save();
void editor_makenewobject();

void editor_project_gui();

void editor_selectasset(struct fileasset *asset);
void editor_selectasset_str(char *path);
void editor_asset_gui(struct fileasset *asset);
void editor_asset_tex_gui(struct Texture *tex);
void editor_asset_text_gui(char *text);

void editor_level_btn(char *level);
void editor_prefab_btn(char *prefab);

void game_start();
void game_resume();
void game_stop();
void game_pause();

void get_levels();

int obj_gui_hierarchy(struct mGameObject *selected);

void sprite_gui(struct mSprite *sprite);

#endif
