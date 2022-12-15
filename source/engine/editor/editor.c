#include "editor.h"
#include "ed_project.h"

#include "2dphysics.h"
#include "camera.h"
#include "config.h"
#include "datastream.h"
#include "debug.h"
#include "debugdraw.h"
#include "editorstate.h"
#include "gameobject.h"
#include "input.h"
#include "level.h"
#include "math.h"
#include "openglrender.h"
#include "registry.h"
#include "resources.h"
#include "script.h"
#include "shader.h"
#include "sound.h"
#include "sprite.h"
#include "texture.h"
#include "vec.h"
#include "window.h"
#include <chipmunk/chipmunk.h>
#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include "nuke.h"

#include "log.h"

#include "ftw.h"

#include <stb_ds.h>
#define ASSET_TEXT_BUF 1024 * 1024 /* 1 MB buffer for editing text files */

struct gameproject *cur_project = NULL;
struct vec *projects = NULL;
static char setpath[MAXPATH];

// Menus
// TODO: Pack this into a bitfield
static struct editorVars editor = {0};

// Lighting effect flags
static bool renderAO = true;
static bool renderDynamicShadows = true;

// Debug render modes
static int renderGizmos = false;
static int showGrid = true;
static int debugDrawPhysics = false;

const char *allowed_extensions[] = {"jpg", "png", "rb", "wav", "mp3", };

void text_ed_cb(GLFWwindow *win, unsigned int codepoint);
void asset_srch_cb(GLFWwindow *win, unsigned int codepoint)
{
    YughInfo("Pushed %d.", codepoint);
}

static const char *editor_filename = "editor.ini";

static struct {
  char *key;
  struct fileasset *value;
} *assets = NULL;

static char asset_search_buffer[100] = {'\0'};

struct fileasset *selected_asset;

static int selected_index = -1;

int show_desktop = 0;

int tex_view = 0;

static int grid1_width = 1;
static int grid1_span = 100;
static int grid2_width = 3;
static int grid2_span = 1000;
static int grid1_draw = true;
static int grid2_draw = true;

static float tex_scale = 1.f;
static struct TexAnimation tex_gui_anim = {0};

char current_level[MAXNAME] = {'\0'};
char levelname[MAXNAME] = {'\0'};

static struct vec *levels = NULL;

static const int ASSET_WIN_SIZE = 512;

static const char *get_extension(const char *filepath) {
  return strrchr(filepath, '.');
}

size_t asset_side_size(struct fileasset *asset) {
    if (asset->type == ASSET_TYPE_IMAGE)
        return sizeof(struct Texture);

    return 0;
}

void save_asset(struct fileasset *asset) {
    if (asset == NULL) {
        YughWarn("No asset to save.", 0);
        return;
    }

    if (asset->type != ASSET_TYPE_IMAGE) return;

    FILE *f = res_open(str_replace_ext(asset->filename, EXT_ASSET), "w");
    fwrite(asset->data, asset_side_size(asset), 1, f);
    fclose(f);
}

void load_asset(struct fileasset *asset) {
    if (asset == NULL) {
        YughWarn("No asset to load.", 0);
        return;
    }

    if (asset->type != ASSET_TYPE_IMAGE)
      return;

    if (asset->data == NULL)
        asset->data = malloc(asset_side_size(asset));

    FILE *f = res_open(str_replace_ext(asset->filename, EXT_ASSET), "r");
    if (f == NULL)
        return;

    struct Texture tex;
    struct Texture *asset_tex = asset->data;

    fread(&tex, asset_side_size(asset), 1, f);

    asset_tex->opts = tex.opts;
    asset_tex->anim = tex.anim;
    fclose(f);
}



static int check_if_resource(const char *fpath, const struct stat *sb, int typeflag) {
  if (typeflag != FTW_F)
      return 0;

    const char *ext = get_extension(fpath);
    if (ext && is_allowed_extension(ext)) {
      struct fileasset *newasset = calloc(1, sizeof(struct fileasset));
      newasset->searched = true;

      if (!strcmp(ext+1, "png") || !strcmp(ext+1, "jpg"))
          newasset->type = ASSET_TYPE_IMAGE;
       else if (!strcmp(ext+1, "rb"))
          newasset->type = ASSET_TYPE_TEXT;
       else if (!strcmp(ext+1, "wav") || !strcmp(ext+1, "mp3"))
          newasset->type = ASSET_TYPE_SOUND;
       else
          newasset->type = ASSET_TYPE_NULL;

      newasset->filename = strdup(fpath);

      shput(assets, newasset->filename, newasset);
    }


  return 0;
}


static void print_files_in_directory(const char *dirpath) {
  struct fileasset *n = NULL;
  for (int i = 0; i < shlen(assets); i++) {
      free(assets[i].key);
      free(assets[i].value);
  }

  shfree(assets);
  ftw(dirpath, check_if_resource, 10);
}

static void get_all_files() { print_files_in_directory("."); }


static int *compute_prefix_function(const char *str) {
  int str_len = strlen(str);
  int *pi = (int *)malloc(sizeof(int) * str_len);
  pi[0] = 0;
  int k = 0;

  for (int q = 2; q < str_len; q++) {
    while (k > 0 && str[k + 1] != str[q])
      k = pi[k];

    if (str[k + 1] == str[q])
      k += 1;

    pi[q] = k;
  }

  return pi;
}

static bool kmp_match(const char *search, const char *text, int *pi) {
  int s_len = strlen(search);
  int t_len = strlen(text);
  //  int *pi = compute_prefix_function(search);
  int q = 0;
  bool found = false;

  for (int i = 0; i < t_len; i++) {
    while (q > 0 && search[q + 1] != text[i])
      q = pi[q];

    if (search[q + 1] == text[i])
      q += 1;

    if (q == s_len) {
      q = pi[q];
      found = true;
      goto end;
    }
  }

end:
  return found;
}

void filter_asset_srch()
{
    if (asset_search_buffer[0] == '\0') {
        for (int i = 0; i < shlen(assets); i++)
            assets[i].value->searched = true;
    } else {
        for (int i = 0; i < shlen(assets); i++)
            assets[i].value->searched = (strstr(assets[i].value->filename, asset_search_buffer) == NULL) ? false : true;
    }
}

void filter_autoindent()
{

}


void editor_save() {
  FILE *feditor = fopen(editor_filename, "w+");
  fwrite(&editor, sizeof(editor), 1, feditor);
  fclose(feditor);
}

static void edit_input_cb(GLFWwindow *w, int key, int scancode, int action, int mods) {
  if (editor_wantkeyboard()) {
      if (editor.asset_srch & NK_EDIT_ACTIVE) {
          filter_asset_srch();
      }

      if ((editor.text_ed & NK_EDIT_ACTIVE) && key == GLFW_KEY_ENTER) {
          filter_autoindent();
      }

      return;
  }

  if (action == GLFW_RELEASE) {
      switch(key) {
          case GLFW_KEY_TAB:
            show_desktop = 0;
            break;
      }
      return;
  }

  switch (key) {
  case GLFW_KEY_ESCAPE:
    quit = true;
    //editor_save_projects();
    editor_save();
    break;

  case GLFW_KEY_1:
    renderMode = LIT;
    break;

  case GLFW_KEY_2:
    renderMode = UNLIT;
    break;

  case GLFW_KEY_3:
    renderMode = WIREFRAME;
    break;

  case GLFW_KEY_4:
    renderMode = DIRSHADOWMAP;
    break;

  case GLFW_KEY_5:
    renderGizmos = !renderGizmos;
    break;

  case GLFW_KEY_6:
    debugDrawPhysics = !debugDrawPhysics;
    break;

  case GLFW_KEY_7:
    break;

  case GLFW_KEY_8:
    break;

  case GLFW_KEY_9:
    break;

  case GLFW_KEY_0:
    break;

  case GLFW_KEY_T:
    break;

  case GLFW_KEY_F2:
    NEGATE(editor.assets.show);
    break;

  case GLFW_KEY_F3:
    NEGATE(editor.stats.show);
    break;

  case GLFW_KEY_F4:
    NEGATE(editor.hierarchy.show);
    break;

  case GLFW_KEY_F5:

    break;

  case GLFW_KEY_F6:
    NEGATE(editor.gamesettings.show);
    break;

  case GLFW_KEY_F7:
    NEGATE(editor.viewmode.show);
    break;

  case GLFW_KEY_F8:
    NEGATE(editor.debug.show);
    break;

  case GLFW_KEY_F9:
    NEGATE(editor.export.show);
    break;

  case GLFW_KEY_F10:
    NEGATE(editor.level.show);
    break;

  case GLFW_KEY_F11:
    window_togglefullscreen(mainwin);
    break;

  case GLFW_KEY_GRAVE_ACCENT:
    NEGATE(editor.repl.show);
    break;

  case GLFW_KEY_K:
    showGrid = !showGrid;
    break;

  case GLFW_KEY_DELETE:
    break;

  case GLFW_KEY_F:
    /*
       if (selectedobject != NULL) {
       cam_goto_object(&camera, &selectedobject->transform);
       }
     */
    break;

  case GLFW_KEY_TAB:
    show_desktop = 1;
    break;
  }
}

static void edit_mouse_cb(GLFWwindow *w, int button, int action, int mods) {
  if (editor_wantkeyboard())
    return;

  if (action == GLFW_PRESS) {
    switch (button) {
    case GLFW_MOUSE_BUTTON_RIGHT:
      cursor_hide();
      break;

    case GLFW_MOUSE_BUTTON_MIDDLE:
      /*
    glBindFramebuffer(GL_FRAMEBUFFER, debugColorPickBO);
    int mx = 0;
    int my = 0;
    SDL_GetMouseState(&mx, &my);
    unsigned char data[4];
    glReadPixels(mx, SCREEN_HEIGHT - my, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
    int pickID = data[0] + data[1]*256 + data[2]*256*256;
    snprintf(objectName, 200, "Object %d", pickID);
    pickGameObject(pickID);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    */

      pickGameObject(-1);

      break;
    }
  } else if (action == GLFW_RELEASE) {
    switch (button) {
    case GLFW_MOUSE_BUTTON_RIGHT:
      cursor_show();
      break;
    }
  }
}

void editor_init(struct window *window) {
  levels = vec_make(MAXPATH, 10);
  get_levels();
  editor_load_projects();
  findPrefabs();

  FILE *feditor = fopen(editor_filename, "r");
  if (feditor == NULL) {
    editor_save();
  } else {
    fread(&editor, sizeof(editor), 1, feditor);
    fclose(feditor);
  }

  nuke_init(window);
  window->nuke_gui = editor_render;
  window_makefullscreen(window);
  glfwSetKeyCallback(window->window, edit_input_cb);
  glfwSetMouseButtonCallback(window->window, edit_mouse_cb);

  //glfwSetCharCallback(window->window, text_ed_cb);
  //glfwSetCharCallback(window->window, asset_srch_cb);

  get_all_files();
}

int editor_wantkeyboard() {
    if (editor.text_ed & NK_EDIT_ACTIVE)
        return 1;

    if (editor.asset_srch & NK_EDIT_ACTIVE)
        return 1;

    return 0;
}

const int nuk_std = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE;

const struct nk_rect nk_rect_std = {250, 250, 250, 250};

void editor_project_gui() {
  /* Grid, etc */
  if (grid1_draw)
    draw_grid(grid1_width, grid1_span);
  if (grid2_draw)
    draw_grid(grid2_width, grid2_span);

  if (debugDrawPhysics) {
    // for (int i = 0; i < number_of_gameobjects(); i++)
    // phys2d_dbgdrawcircle(objects[i]->circle);
  }

/*
    if (nk_menu_begin_label(ctx, "Windows", NK_TEXT_LEFT, nk_vec2(100, 200))) {
      nk_layout_row_dynamic(ctx, 25, 1);

      nk_checkbox_label(ctx, "Resources", &editor.assets.show);
      nk_checkbox_label(ctx, "Hierarchy", &editor.hierarchy.show);
      nk_checkbox_label(ctx, "Lighting F5", &editor.lighting.show);
      nk_checkbox_label(ctx, "Game Settings F6", &editor.gamesettings.show);
      nk_checkbox_label(ctx, "View F7", &editor.viewmode.show);
      nk_checkbox_label(ctx, "Debug F8", &editor.debug.show);
      nk_checkbox_label(ctx, "Export F9", &editor.export.show);
      nk_checkbox_label(ctx, "Level F10", &editor.level.show);
      nk_checkbox_label(ctx, "REPL", &editor.repl.show);

      nk_menu_end(ctx);
    }
*/
    NK_MENU_START(level)
      nuke_nel(1);
      nuke_labelf("Current level: %s", current_level[0] == '\0' ? "No level loaded." : current_level);

      nuke_nel(3);
      if (nk_button_label(ctx, "New")) {
        new_level();
        current_level[0] = '\0';
      }

      if (nk_button_label(ctx, "Save")) {
        if (strlen(current_level) == 0) {
            YughWarn("Can't save level that has no name.");
        } else {
            save_level(current_level);
            get_levels();
        }
      }

      if (nk_button_label(ctx, "Save as")) {
          if (strlen(current_level) == 0) {
              YughWarn("Can't save level that has no name.");
          } else {
                    strcpy(current_level, levelname);
          strncat(current_level, EXT_LEVEL, 10);
              save_level(current_level);
              levelname[0] = '\0';
              get_levels();



        }
      }
      nuke_nel(1);
      nk_edit_string_zero_terminated(ctx, NK_EDIT_SIMPLE, levelname, MAXNAME - 1, nk_filter_default);


      vec_walk(levels, editor_level_btn);
    NK_MENU_END()

  NK_MENU_START(export)
    nuke_nel(2);
    if (nuke_btn("Bake")) {
    }
    if (nuke_btn("Build")) {
    }

  NK_MENU_END()

  NK_MENU_START(gamesettings)
    nk_layout_row_dynamic(ctx,25,1);

    // nk_edit_string_zero_terminated(ctx, NK_EDIT_SIMPLE, cur_project->name,
    // 126, nk_filter_default);

    if (nk_tree_push(ctx, NK_TREE_NODE, "Physics", NK_MINIMIZED)) {
      nuke_prop_float("2d Gravity", -5000.f, &phys2d_gravity, 0.f, 1.f, 0.1f);
      phys2d_apply();
      nk_tree_pop(ctx);
    }

    if (nk_tree_push(ctx, NK_TREE_NODE, "Quality", NK_MINIMIZED)) {
      nk_tree_pop(ctx);
    }

  NK_MENU_END()

  NK_MENU_START(stats)
    nuke_labelf("FPS: %2.4f", 1.f / deltaT);
    nuke_labelf("Triangles rendered: %llu", triCount);
  NK_MENU_END()

  NK_MENU_START(repl)

    nk_layout_row_dynamic(ctx, 300, 1);

    nk_flags active;

    static char buffer[512] = {'\0'};

    active = nk_edit_string_zero_terminated(ctx, NK_EDIT_BOX | NK_EDIT_SIG_ENTER, buffer, 512 - 1, nk_filter_ascii);
    if (active && NK_EDIT_COMMITED) {
      script_run(buffer);
      buffer[0] = '\0';
    }

    NK_MENU_END()

  NK_MENU_START(hierarchy)
  nuke_nel(1);

    if (nuke_btn("New Object")) {
      MakeGameobject();
    }

    obj_gui_hierarchy(selectedobject);

    NK_MENU_END()

  NK_FORCE(simulate)
  nuke_nel(2);
  if (physOn) {
    if (nuke_btn("Pause"))
      game_pause();

    if (nuke_btn("Stop"))
      game_stop();
  } else {
    if (nuke_btn("Play"))
      game_start();
  }

  NK_FORCE_END()

  NK_FORCE(prefab)
    nuke_nel(1);

    vec_walk(prefabs, editor_prefab_btn);
  NK_FORCE_END()

  NK_MENU_START(assets)
    nuke_nel(1);
    editor.asset_srch = nk_edit_string_zero_terminated(ctx, NK_EDIT_SIMPLE, asset_search_buffer, 100, nk_filter_ascii);

    if (nk_button_label(ctx, "Reload all files"))
      get_all_files();


    for (int i = 0; i < shlen(assets); i++) {
      if (!assets[i].value->searched)
        continue;

      if (nk_button_label(ctx, assets[i].key))
        editor_selectasset_str(assets[i].key);

    }

    NK_MENU_END()

  if (selected_asset)
    editor_asset_gui(selected_asset);


  NK_MENU_START(debug)
      nuke_nel(1);
  if (nk_tree_push(ctx, NK_TREE_NODE, "Debug Draws", NK_MINIMIZED)) {
      nuke_checkbox("Gizmos", &renderGizmos);
      nuke_checkbox("Grid", &showGrid);
      nuke_checkbox("Physics", &debugDrawPhysics);
      nk_tree_pop(ctx);
    }

    if (nuke_btn("Reload Shaders")) {
      shader_compile_all();
    }

    nuke_property_int("Grid 1 Span", 1, &grid1_span, 500, 1);
    nuke_checkbox("Draw", &grid1_draw);

    nuke_property_int("Grid 2 Span", 10, &grid2_span, 1000, 1);
    nuke_checkbox("Draw", &grid2_draw);

    nuke_property_float("Grid Opacity", 0.f, &gridOpacity, 1.f, 0.01f, 0.01f);
    nuke_property_float("Small unit", 0.5f, &smallGridUnit, 5.f, 0.1f, 0.1f);
    nuke_property_float("Big unit", 10.f, &bigGridUnit, 50.f, 1.f, 0.1f);
    nuke_property_float("Small thickness", 1.f, &gridSmallThickness, 10.f, 0.1f, 0.1f);
    nuke_property_float("Big thickness", 1.f, &gridBigThickness, 10.f, 0.1f, 0.1f);

    static struct nk_colorf smgrd;
    static struct nk_colorf lgrd;

    nk_color_pick(ctx, &smgrd, NK_RGBA);
    nk_color_pick(ctx, &lgrd, NK_RGBA);

  NK_MENU_END()

startobjectgui:

  if (selectedobject) {
    draw_point(selectedobject->transform.position[0],
               selectedobject->transform.position[1], 5);

    NK_FORCE(gameobject)

    nuke_nel(3);


    if (nuke_btn("Save"))
      gameobject_saveprefab(selectedobject);

    if (nuke_btn("Del")) {
      gameobject_delete(selected_index);
      pickGameObject(-1);
      nk_end(ctx);
      goto startobjectgui;
    }

    if (selectedobject->editor.prefabSync && nk_button_label(ctx, "Revert"))
        gameobject_revertprefab(selectedobject);

    nuke_label("Name");
    nk_edit_string_zero_terminated(ctx, NK_EDIT_SIMPLE, selectedobject->editor.mname, 50, nk_filter_ascii);

    nuke_label("Prefab");
    nk_edit_string_zero_terminated(ctx, NK_EDIT_SIMPLE, selectedobject->editor.prefabName, 50, nk_filter_ascii);

    object_gui(selectedobject);


   // nuke_label("Components");
   nuke_nel(3);

    for (int i = 0; i < ncomponent; i++) {
      if (nuke_btn(components[i].name)) {
        gameobject_addcomponent(selectedobject, &components[i]);
      }
    }

    NK_FORCE_END()

/*
    NK_FORCE(components)
    nuke_nel(1);

    for (int i = 0; i < ncomponent; i++) {
      if (nuke_btn(components[i].name)) {
        gameobject_addcomponent(selectedobject, &components[i]);
      }
    }

   NK_FORCE_END()
   */
   }
}

void editor_render() { editor_project_gui(); }

void pickGameObject(int pickID) {
  if (pickID >= 0 && pickID < arrlen(gameobjects)) {
    selected_index = pickID;
    selectedobject = &gameobjects[pickID];
  } else {
    selected_index = -1;
    selectedobject = NULL;
  }
}

int is_allowed_extension(const char *ext) {
  for (size_t i = 0; i < sizeof(allowed_extensions) / sizeof(allowed_extensions[0]); i++) {
    if (!strcmp(ext + 1, allowed_extensions[i]))
      return true;
  }

  return false;
}

void editor_level_btn(char *level) {
  if (nuke_btn(level)) {
    load_level(level);
    strcpy(current_level, level);
  }
}

struct fileasset *asset_from_path(const char *p)
{
    return shget(assets, p);
}

void editor_selectasset_str(const char *path) {
  struct fileasset *asset = asset_from_path(path);



  FILE *fasset;

  switch (asset->type) {
      case ASSET_TYPE_IMAGE:
          if (asset->data == NULL) {
              asset->data = texture_loadfromfile(path);
              load_asset(asset);
          }
          else
              tex_pull(asset->data);

          tex_gui_anim.tex = asset->data;
          tex_anim_set(&tex_gui_anim);
          anim_setframe(&tex_gui_anim, 0);
          float tex_scale = (float) ASSET_WIN_SIZE / (float)tex_gui_anim.tex->width;
          if (tex_scale >= 10.f) {
             tex_scale = 10.f;
         }

          break;

       case ASSET_TYPE_TEXT:
           fasset = fopen(asset->filename, "rb");

            fseek(fasset, 0, SEEK_END);
            long length = ftell(fasset);
            fseek(fasset, 0, SEEK_SET);
            asset->data = malloc(ASSET_TEXT_BUF);
            fread(asset->data, 1, length, fasset);
            fclose(fasset);
            break;

        case ASSET_TYPE_SOUND:
            break;

        default:
            break;
  }

    load_asset(asset);

    if (selected_asset != NULL)
      save_asset(selected_asset);

   selected_asset = asset;

}

void editor_asset_tex_gui(struct Texture *tex) {
    nuke_labelf("%dx%d", tex->width, tex->height);
    nuke_prop_float("Zoom", 0.01f, &tex_scale, 10.f, 0.1f, 0.01f);
    int old_sprite = tex->opts.sprite;

    nuke_checkbox("Sprite", &tex->opts.sprite);

    if (old_sprite != tex->opts.sprite)
        tex_gpu_load(tex);

    nuke_nel(4);
    nuke_radio_btn("Raw", &tex_view, 0);
    nuke_radio_btn("View 1", &tex_view, 1);
    nuke_radio_btn("View 2", &tex_view, 2);

    nuke_checkbox("Animation", &tex->opts.animation);


    if (tex->opts.animation) {
        int old_frames = tex->anim.frames;
        int old_ms = tex->anim.ms;

        nuke_nel(2);
        nuke_property_int("Frames", 1, &tex->anim.frames, 20, 1);
        nuke_property_int("FPS", 1, &tex->anim.ms, 24, 1);

        if (tex_gui_anim.playing) {
            if (nuke_btn("Pause"))
                anim_pause(&tex_gui_anim);
            if (tex_gui_anim.playing && nuke_btn("Stop"))
                anim_stop(&tex_gui_anim);
        } else {
            nuke_nel(3);
            if (nuke_btn("Play"))
                anim_play(&tex_gui_anim);

            if (nuke_btn("Bkwd"))
                anim_bkwd(&tex_gui_anim);

            if (nuke_btn("Fwd"))
                anim_fwd(&tex_gui_anim);
        }

       nuke_nel(1);
        nuke_labelf("Frame %d/%d", tex_gui_anim.frame+1, tex_gui_anim.tex->anim.frames);

        if (old_frames != tex->anim.frames || old_ms != tex->anim.ms)
            tex_anim_set(&tex_gui_anim);

        nk_layout_row_static(ctx, tex->height*tex_scale*tex_gui_anim.uv.h, tex->width*tex_scale*tex_gui_anim.uv.w, 1);
        struct nk_rect r;
        r.x = tex_gui_anim.uv.x*tex->width;
        r.y = tex_gui_anim.uv.y*tex->height;
        r.w = tex_gui_anim.uv.w*tex->width;
        r.h = tex_gui_anim.uv.h*tex->height;

        nk_image(ctx, nk_subimage_id(tex->id, tex->width, tex->height, r));
    } else {
      nk_layout_row_static(ctx, tex->height*tex_scale, tex->width*tex_scale, 1);
      nk_image(ctx, nk_image_id(tex->id));
    }
}

void text_ed_cb(GLFWwindow *win, unsigned int codepoint)
{
    YughInfo("Pressed button %d", codepoint);
    if (editor.text_ed & NK_EDIT_ACTIVE) {
        if (codepoint == '\n') {
            YughInfo("Hit newline.");
        }
    }
}

void editor_asset_text_gui(char *text) {
    nk_layout_row_dynamic(ctx, 600, 1);
    editor.text_ed = nk_edit_string_zero_terminated(ctx, NK_EDIT_BOX, text, ASSET_TEXT_BUF, nk_filter_ascii);

    nuke_nel(4);
    if (nuke_btn("Save")) {
        FILE *f = fopen(selected_asset->filename, "wd");
        size_t len = strlen(text);
        fwrite(text, len, 1, f);
        fclose(f);
    }

    /* TODO: Nicer formatting for text input. Auto indent. */
}

void editor_asset_sound_gui(struct wav *wav)
{

}

void editor_asset_gui(struct fileasset *asset) {

  NK_FORCE(asset)

  nuke_nel(2);
  nk_labelf(ctx, NK_TEXT_LEFT, "%s", selected_asset->filename);

  if (nk_button_label(ctx, "Close"))
    selected_asset = NULL;

  nuke_nel(1);
  switch (asset->type) {
  case ASSET_TYPE_NULL:
    break;

  case ASSET_TYPE_IMAGE:
    editor_asset_tex_gui(asset->data);
    break;

  case ASSET_TYPE_TEXT:
    editor_asset_text_gui(asset->data);
    break;

  case ASSET_TYPE_SOUND:
    editor_asset_sound_gui(asset->data);
    break;
  }

  NK_FORCE_END()
}

void editor_makenewobject() {}

int obj_gui_hierarchy(struct gameobject *selected) {

  for (int i = 0; i < arrlen(gameobjects); i++) {
    struct gameobject *go = &gameobjects[i];

    if (nk_select_label(ctx, go->editor.mname, NK_TEXT_LEFT, go == selected)) {
      if (go != selected)
        pickGameObject(i);
    }
  }

  return 0;
}

void get_levels() { fill_extensions(levels, DATA_PATH, EXT_LEVEL); }

void editor_prefab_btn(char *prefab) {
  if (nk_button_label(ctx, prefab)) {
    YughInfo("Making prefab '%s'.", prefab);
    gameobject_makefromprefab(prefab);
  }
}

void game_start() { physOn = 1; }

void game_resume() { physOn = 1; }

void game_stop() { physOn = 0; }

void game_pause() { physOn = 0; }

void sprite_gui(struct sprite *sprite) {

  nuke_nel(2);
  //nk_labelf(ctx, NK_TEXT_LEFT, "Path %s", tex_get_path(sprite->tex));



  if (nk_button_label(ctx, "Load texture") && selected_asset != NULL) {
    sprite_loadtex(sprite, selected_asset->filename);
  }


  if (sprite->tex != NULL) {
    //nk_labelf(ctx, NK_TEXT_LEFT, "%s", tex_get_path(sprite->tex));
    nk_labelf(ctx, NK_TEXT_LEFT, "%dx%d", sprite->tex->width, sprite->tex->height);

    nk_layout_row_static(ctx, sprite->tex->height, sprite->tex->width, 1);
    if (nk_button_image(ctx, nk_image_id(sprite->tex->id)))
      editor_selectasset_str(tex_get_path(sprite->tex));
  }


  nk_property_float2(ctx, "Sprite Position", -1.f, sprite->pos, 0.f, 0.01f, 0.01f);

  nuke_nel(3);
  if (nk_button_label(ctx, "C")) {
    sprite->pos[0] = -0.5f;
    sprite->pos[1] = -0.5f;
  }

  if (nk_button_label(ctx, "U")) {
    sprite->pos[0] = -0.5f;
    sprite->pos[1] = -1.f;
  }

  if (nk_button_label(ctx, "D")) {
    sprite->pos[0] = -0.5f;
    sprite->pos[1] = 0.f;
  }

}

