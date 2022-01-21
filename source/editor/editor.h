#ifndef EDITOR_H
#define EDITOR_H

#include <config.h>
#include <SDL2/SDL.h>
#include <stdbool.h>
#include "resources.h"


struct mCamera;

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

struct gameproject {
    char name[127];
    char path[MAXPATH];
};

struct Texture;

void pickGameObject(int pickID);
int is_allowed_extension(const char *ext);

void editor_init(struct mSDLWindow *window);
void editor_input(struct mSDLWindow *window, SDL_Event * e);
void editor_render();
int editor_wantkeyboard();
void editor_save();
void editor_makenewobject();

void editor_project_gui();

void editor_init_project(struct gameproject *gp);
void editor_save_projects();
void editor_load_projects();
void editor_proj_select_gui();
void editor_import_project(char *path);
void editor_make_project(char *path);

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

///////// Object GUIs
void light_gui(struct mLight *light);
void pointlight_gui(struct mPointLight *light);
void spotlight_gui(struct mSpotLight *spot);
void staticactor_gui(struct mStaticActor *sa);
void trans_drawgui(struct mTransform *T);
void object_gui(struct mGameObject *go);
void sprite_gui(struct mSprite *sprite);
void circle_gui(struct phys2d_circle *circle);
void segment_gui(struct phys2d_segment *seg);
void box_gui(struct phys2d_box *box);
void poly_gui(struct phys2d_poly *poly);
void edge_gui(struct phys2d_edge *edge);
void shape_gui(struct phys2d_shape *shape);
void pinball_flipper_gui(struct flipper *flip);

int obj_gui_hierarchy(struct mGameObject *selected);

#endif
