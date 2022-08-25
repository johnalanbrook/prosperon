#ifndef EDITOR_H
#define EDITOR_H

#include <config.h>
#include <stdbool.h>
#include "resources.h"

#include "nuke.h"

#define ASSET_TYPE_NULL 0
#define ASSET_TYPE_IMAGE 1
#define ASSET_TYPE_TEXT 2
#define ASSET_TYPE_SOUND 3

struct fileasset {
    char *filename;
    bool searched;
    short type;
    void *data;  // Struct of the underlying asset - Texture struct, etc
};

typedef struct {
    bool show;
    struct nk_rect rect;
} editor_win;

struct editorVars {
    editor_win stats;
    editor_win hierarchy;
    editor_win lighting;
    editor_win gamesettings;
    editor_win viewmode;
    editor_win debug;
    editor_win assets;
    editor_win asset;
    editor_win repl;
    editor_win export;
    editor_win level;
    editor_win gameobject;
    editor_win components;
    editor_win simulate;
    editor_win prefab;
    nk_flags text_ed;
    nk_flags asset_srch;
};

struct mGameObject;

extern int show_desktop;

#define NK_MENU_START(VAR) if (editor.VAR.show && !show_desktop) { \
                                                                          if (editor.VAR.rect.w == 0) editor.VAR.rect = nk_rect_std; \
                                                                          if (nk_begin(ctx, #VAR, editor.VAR.rect, nuk_std)) { \
                                                                          editor.VAR.rect = nk_window_get_bounds(ctx);

#define NK_MENU_END() } nk_end(ctx); }

#define NK_FORCE(VAR) if (editor.VAR.rect.w == 0) editor.VAR.rect = nk_rect_std; \
                                                         if (!show_desktop && nk_begin(ctx, #VAR, editor.VAR.rect, nuk_std)) { \
                                                         editor.VAR.rect = nk_window_get_bounds(ctx);

#define NK_FORCE_END() nk_end(ctx); }

#define NEGATE(VAR) VAR = ! VAR

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
void editor_selectasset_str(const char *path);
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
