#include "editor.h"

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

#define __USE_XOPEN_EXTENDED 1
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
static bool renderGizmos = false;
static bool showGrid = true;
static bool debugDrawPhysics = false;

const char *allowed_extensions[] = {"jpg", "png", "gltf", "glsl"};

static const char *editor_filename = "editor.ini";

struct asset {
  char *key;
  struct fileasset *value;
};

static struct asset *assets = NULL;

static char asset_search_buffer[100] = {0};

struct fileasset *selected_asset;

static int selected_index = -1;

static int grid1_width = 1;
static int grid1_span = 100;
static int grid2_width = 3;
static int grid2_span = 1000;
static bool grid1_draw = true;
static bool grid2_draw = true;

static float tex_scale = 1.f;
static struct TexAnimation tex_gui_anim = {0};

char current_level[MAXNAME] = {'\0'};
char levelname[MAXNAME] = {'\0'};

static struct vec *levels = NULL;

static const int ASSET_WIN_SIZE = 512;

static const char *get_extension(const char *filepath) {
  return strrchr(filepath, '.');
}

static int check_if_resource(const char *fpath, const struct stat *sb,
                             int typeflag, struct FTW *ftwbuf) {
  if (typeflag == FTW_F) {
    const char *ext = get_extension(fpath);
    if (ext && is_allowed_extension(ext)) {
      struct fileasset *newasset =
          (struct fileasset *)calloc(1, sizeof(struct fileasset));
      newasset->filename = (char *)malloc(sizeof(char) * strlen(fpath) + 1);
      strcpy(newasset->filename, fpath);
      newasset->extension_len = strlen(ext);
      newasset->searched = true;
      shput(assets, newasset->filename, newasset);
    }
  }

  return 0;
}

static void print_files_in_directory(const char *dirpath) {
  int nflags = 0;
  shfree(assets);
  nftw(dirpath, &check_if_resource, 10, nflags);
}

static void get_all_files() { print_files_in_directory(DATA_PATH); }

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

static int MyCallback() {
  /*
      if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion) {
          data->InsertChars(data->CursorPos, "..");
      } else if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
          if (data->EventKey == ImGuiKey_UpArrow) {
              data->DeleteChars(0, data->BufTextLen);
              data->InsertChars(0, "Pressed Up!");
              data->SelectAll();
          } else if (data->EventKey == ImGuiKey_DownArrow) {
              data->DeleteChars(0, data->BufTextLen);
              data->InsertChars(0, "Pressed Down!");
              data->SelectAll();
          }
      } else if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
          int i = 0;
          if (data->Buf[0] == '\0')
              while (i < shlen(assets))
                  assets[i].value->searched = true;
          else
              while (i < shlen(assets))
                  assets[i].value->searched =
                      (strstr(assets[i].value->filename, data->Buf) ==
                       NULL) ? false : true;

      }
  */
  return 0;
}

static int TextEditCallback() // ImGuiInputTextCallbackData * data)
{
  /*
      static int dirty = 0;

      if (data->EventChar == '\n') {
          dirty = 1;
      } else if (data->EventChar == '(') {
          //data->EventChar = 245;
          dirty = 2;
      } else if (data->EventChar == ')') {
          dirty = 3;
      }

      if (data->EventFlag == ImGuiInputTextFlags_CallbackAlways) {
          if (dirty == 1) {
              dirty = 0;
              char *c = &data->Buf[data->CursorPos - 2];


              while (*c != '\n')
                  c--;

              c++;

              if (isblank(*c)) {
                  char *ce = c;

                  while (isblank(*ce))
                      ce++;

                  data->InsertChars(data->CursorPos, c, ce);
              }

          }
      }

  */
  return 0;
}

void editor_save() {
  FILE *feditor = fopen(editor_filename, "w+");
  fwrite(&editor, sizeof(editor), 1, feditor);
  fclose(feditor);
}

static void edit_input_cb(GLFWwindow *w, int key, int scancode, int action, int mods) {
  if (editor_wantkeyboard())
    return;

  if (action != GLFW_PRESS)
      return;

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
    NEGATE(editor.lighting.show);
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

void editor_init(struct mSDLWindow *window) {
  levels = vec_make(MAXNAME, 10);
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


}

// TODO: Implement
int editor_wantkeyboard() { return 0; }

const int nuk_std = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
                    NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE;

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

  static char text[3][64];
  static int text_len[3];
  static const char *items[] = {"Item 0", "item 1", "item 2"};
  static int selected_item = 0;
  static int check = 1;

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
      nk_layout_row_dynamic(ctx,25,3);
      if (nk_button_label(ctx, "New")) {
        new_level();
        current_level[0] = '\0';
      }

      if (nk_button_label(ctx, "Save")) {
        save_level(current_level);
        get_levels();
      }

      if (nk_button_label(ctx, "Save as")) {
        save_level(levelname);
        strcpy(current_level, levelname);
        levelname[0] = '\0';
        get_levels();
      }

      nk_edit_string_zero_terminated(ctx, NK_EDIT_SIMPLE, levelname, MAXNAME - 1, nk_filter_default);

    nk_layout_row_dynamic(ctx,25,1);
      vec_walk(levels, editor_level_btn);
    NK_MENU_END()

  if (editor.export.show) {
    nk_begin(ctx, "Export and Bake", editor.export.rect, nuk_std);

    nk_layout_row_dynamic(ctx, 25,2);
    if (nk_button_label(ctx, "Bake")) {
    }
    if (nk_button_label(ctx, "Build")) {
    }

    nk_end(ctx);
  }

  // Shadow map vars
  NK_MENU_START(lighting)

    nk_layout_row_dynamic(ctx, 25, 1);
    nk_label(ctx, "Directional shadow map", NK_TEXT_LEFT);

    nk_property_float(ctx, "Near plane", -200.f, &near_plane, 200.f, 1.f, 0.01f);
    nk_property_float(ctx, "Far plane", -200.f, &far_plane, 200.f, 1.f, 0.01f);
    nk_property_float(ctx, "Shadow lookahead", 0.f, &shadowLookahead, 100.f, 1.f, 0.01f);
    nk_property_float(ctx, "Plane size", 0.f, &plane_size, 100.f, 1.f, 0.01f);

  NK_MENU_END()

  NK_MENU_START(gamesettings)
    nk_layout_row_dynamic(ctx,25,1);

    // nk_edit_string_zero_terminated(ctx, NK_EDIT_SIMPLE, cur_project->name,
    // 126, nk_filter_default);

    if (nk_tree_push(ctx, NK_TREE_NODE, "Physics", NK_MINIMIZED)) {
      nk_property_float(ctx, "2d Gravity", -5000.f, &phys2d_gravity, 0.f, 1.f,
                        0.1f);
      phys2d_apply();
      nk_tree_pop(ctx);
    }

    if (nk_tree_push(ctx, NK_TREE_NODE, "Quality", NK_MINIMIZED)) {
      nk_tree_pop(ctx);
    }

  NK_MENU_END()

  NK_MENU_START(stats)
    nk_labelf(ctx, NK_TEXT_LEFT, "FPS: %2.4f", 1.f / deltaT);
    nk_labelf(ctx, NK_TEXT_LEFT, "Triangles rendered: %llu", triCount);
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

  NK_MENU_START(debug)

    nk_layout_row_dynamic(ctx, 25, 1);

    nk_property_float(ctx, "Camera FOV", 0.1f, &editorFOV, 90.f, 1.f, 0.1f);
    nk_property_float(ctx, "Camera Near Plane", 0.1f, &editorClose, 5.f, 0.1f, 0.01f);
    nk_property_float(ctx, "Camera Far Plane", 50.f, &editorFar, 10000.f, 1.f,1.f);

    if (nk_tree_push(ctx, NK_TREE_NODE, "Shading mode", NK_MINIMIZED)) {
      renderMode =
          nk_option_label(ctx, "Lit", renderMode == LIT) ? LIT : renderMode;
      renderMode = nk_option_label(ctx, "Unlit", renderMode == UNLIT)
                       ? UNLIT
                       : renderMode;
      renderMode = nk_option_label(ctx, "Wireframe", renderMode == WIREFRAME)
                       ? WIREFRAME
                       : renderMode;
      renderMode = nk_option_label(ctx, "Directional shadow map",
                                   renderMode == DIRSHADOWMAP)
                       ? DIRSHADOWMAP
                       : renderMode;
      nk_tree_pop(ctx);
    }

    if (nk_tree_push(ctx, NK_TREE_NODE, "Lighting", NK_MINIMIZED)) {
      nk_checkbox_label(ctx, "Shadows", &renderDynamicShadows);
      nk_checkbox_label(ctx, "Ambient Occlusion", &renderAO);
      nk_tree_pop(ctx);
    }

    if (nk_tree_push(ctx, NK_TREE_NODE, "Debug Draws", NK_MINIMIZED)) {
      nk_checkbox_label(ctx, "Gizmos", &renderGizmos);
      nk_checkbox_label(ctx, "Grid", &showGrid);
      nk_checkbox_label(ctx, "Physics", &debugDrawPhysics);
      nk_tree_pop(ctx);
    }

    NK_MENU_END()

  NK_MENU_START(hierarchy)
  nk_layout_row_dynamic(ctx, 25, 1);

    if (nk_button_label(ctx, "New Object")) {
      MakeGameobject();
    }

    obj_gui_hierarchy(selectedobject);

    NK_MENU_END()

  if (nk_begin(ctx, "Simulate", nk_rect_std, nuk_std)) {

  nk_layout_row_dynamic(ctx, 25, 2);
  if (physOn) {
    if (nk_button_label(ctx, "Pause"))
      game_pause();

    if (nk_button_label(ctx, "Stop"))
      game_stop();
  } else {
    if (nk_button_label(ctx, "Play"))
      game_start();
  }

  nk_end(ctx);
  }

  if (nk_begin(ctx, "Prefab Creator", nk_rect_std, nuk_std)) {
    nk_layout_row_dynamic(ctx, 25, 1);

    vec_walk(prefabs, editor_prefab_btn);
    nk_end(ctx);
  }

  NK_MENU_START(assets)
    nk_layout_row_dynamic(ctx,25,1);
    nk_edit_string_zero_terminated(ctx, NK_EDIT_SIMPLE, asset_search_buffer, 100, nk_filter_ascii);

    /*
if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion) {
data->InsertChars(data->CursorPos, "..");
} else if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
if (data->EventKey == ImGuiKey_UpArrow) {
data->DeleteChars(0, data->BufTextLen);
data->InsertChars(0, "Pressed Up!");
data->SelectAll();
} else if (data->EventKey == ImGuiKey_DownArrow) {
data->DeleteChars(0, data->BufTextLen);
data->InsertChars(0, "Pressed Down!");
data->SelectAll();
}
} else if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
int i = 0;
if (data->Buf[0] == '\0')
while (i < shlen(assets))
assets[i].value->searched = true;
else
while (i < shlen(assets))
assets[i].value->searched =
(strstr(assets[i].value->filename, data->Buf) ==
NULL) ? false : true;

}
*/

    if (nk_button_label(ctx, "Reload all files"))
      get_all_files();

    nk_group_begin(ctx, "##scrolling", NK_WINDOW_NO_SCROLLBAR);
    for (int i = 0; i < shlen(assets); i++) {
      if (!assets[i].value->searched)
        continue;

      if (nk_button_label(ctx, assets[i].value->filename + stemlen)) {
        editor_selectasset(assets[i].value);
      }
    }
    nk_group_end(ctx);

    NK_MENU_END()

  if (selected_asset)
    editor_asset_gui(selected_asset);

  NK_MENU_START(debug)

    if (nk_button_label(ctx, "Reload Shaders")) {
      shader_compile_all();
    }

    nk_property_int(ctx, "Grid 1 Span", 1, &grid1_span, 500, 1, 1);
    nk_checkbox_label(ctx, "Draw", &grid1_draw);

    nk_property_int(ctx, "Grid 2 Span", 10, &grid2_span, 1000, 1, 1);
    nk_checkbox_label(ctx, "Draw", &grid2_draw);

    nk_property_float(ctx, "Grid Opacity", 0.f, &gridOpacity, 1.f, 0.01f, 0.01f);
    nk_property_float(ctx, "Small unit", 0.5f, &smallGridUnit, 5.f, 0.1f, 0.1f);
    nk_property_float(ctx, "Big unit", 10.f, &bigGridUnit, 50.f, 1.f, 0.1f);
    nk_property_float(ctx, "Small thickness", 1.f, &gridSmallThickness, 10.f, 0.1f, 0.1f);
    nk_property_float(ctx, "Big thickness", 1.f, &gridBigThickness, 10.f, 0.1f, 0.1f);

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

    nk_layout_row_dynamic(ctx, 30, 3);

    if (nk_button_label(ctx, "Save"))
      gameobject_saveprefab(selectedobject);

    if (nk_button_label(ctx, "Del")) {
      gameobject_delete(selected_index);
      pickGameObject(-1);
      nk_end(ctx);
      goto startobjectgui;
    }

    if (selectedobject->editor.prefabSync && nk_button_label(ctx, "Revert"))
        gameobject_revertprefab(selectedobject);

    nk_label(ctx, "Name", NK_TEXT_LEFT);
    nk_edit_string_zero_terminated(ctx, NK_EDIT_SIMPLE, selectedobject->editor.mname, 50, nk_filter_ascii);

    nk_label(ctx, "Prefab", NK_TEXT_LEFT);
    nk_edit_string_zero_terminated(ctx, NK_EDIT_SIMPLE, selectedobject->editor.prefabName, 50, nk_filter_ascii);

    object_gui(selectedobject);

    NK_FORCE_END()

    NK_FORCE(components)
    nk_layout_row_dynamic(ctx,25,1);
    for (int i = 0; i < ncomponent; i++) {
      if (nk_button_label(ctx, components[i].name)) {
        gameobject_addcomponent(selectedobject, &components[i]);
      }
    }

   NK_FORCE_END()
   }
}

void editor_render() { editor_project_gui(); }

void pickGameObject(int pickID) {
  if (pickID >= 0 && pickID < gameobjects->len) {
    selected_index = pickID;
    selectedobject = (struct mGameObject *)vec_get(gameobjects, pickID);
  } else {
    selected_index = -1;
    selectedobject = NULL;
  }
}

int is_allowed_extension(const char *ext) {
  for (size_t i = 0;
       i < sizeof(allowed_extensions) / sizeof(allowed_extensions[0]); i++) {
    if (!strcmp(ext + 1, allowed_extensions[i]))
      return true;
  }

  return false;
}

void editor_level_btn(char *level) {

  if (nk_button_label(ctx, level)) {
    load_level(level);
    strcpy(current_level, level);
  }
}

void editor_selectasset(struct fileasset *asset) {
  const char *ext = get_extension(asset->filename);

  if (!strcmp(ext + 1, "png") || !strcmp(ext + 1, "jpg")) {
    asset->data = texture_loadfromfile(asset->filename);
    tex_gui_anim.tex = (struct Texture *)asset->data;
    asset->type = ASSET_TYPE_IMAGE;
    tex_anim_set(&tex_gui_anim);
    // float tex_scale = float((float) ASSET_WIN_SIZE /
    // tex_gui_anim.tex->width);
    if (tex_scale >= 10.f)
      tex_scale = 10.f;
  } else if (!strcmp(ext + 1, "glsl")) {
    asset->type = ASSET_TYPE_TEXT;

    FILE *fasset = fopen(asset->filename, "rb");

    fseek(fasset, 0, SEEK_END);
    long length = ftell(fasset);
    fseek(fasset, 0, SEEK_SET);
    asset->data = malloc(ASSET_TEXT_BUF);
    fread(asset->data, 1, length, fasset);
    fclose(fasset);
  }
  selected_asset = asset;
}

void editor_selectasset_str(char *path) {
  struct fileasset *asset = (struct fileasset *)shget(assets, path);

  if (asset)
    editor_selectasset(asset);
}

void editor_asset_tex_gui(struct Texture *tex) {
  /*
      ImGui::Text("%dx%d", tex->width, tex->height);

      ImGui::SliderFloat("Zoom", &tex_scale, 0.01f, 10.f);

      int old_sprite = tex->opts.sprite;
      ImGui::Checkbox("Sprite", (bool *) &tex->opts.sprite);

      if (old_sprite != tex->opts.sprite)
          tex_gpu_load(tex);


      ImGui::RadioButton("Raw", &tex_view, 0);
      ImGui::SameLine(); ImGui::RadioButton("View 1", &tex_view, 1);
      ImGui::SameLine(); ImGui::RadioButton("View 2", &tex_view, 2);


      ImGui::Checkbox("Animation", (bool *) &tex->opts.animation);

      if (tex->opts.animation) {
          int old_frames = tex->anim.frames;
          int old_ms = tex->anim.ms;
          ImGui::SliderInt("Frames", &tex->anim.frames, 1, 20);
          ImGui::SliderInt("FPS", &tex->anim.ms, 1, 24);




          if (tex_gui_anim.playing) {
              if (ImGui::Button("Pause"))
                  anim_pause(&tex_gui_anim);
              ImGui::SameLine();
              if (tex_gui_anim.playing && ImGui::Button("Stop"))
                  anim_stop(&tex_gui_anim);
          } else {
              if (ImGui::Button("Play"))
                  anim_play(&tex_gui_anim);

              ImGui::SameLine();
              if (ImGui::Button("Bkwd"))
                  anim_bkwd(&tex_gui_anim);


              ImGui::SameLine();
              if (ImGui::Button("Fwd"))
                  anim_fwd(&tex_gui_anim);
          }








          ImGui::SameLine();
          ImGui::Text("Frame %d/%d", tex_gui_anim.frame + 1,
                      tex_gui_anim.tex->anim.frames);





          if (old_frames != tex->anim.frames || old_ms != tex->anim.ms)
              tex_anim_set(&tex_gui_anim);

          ImVec2 uv0 = ImVec2(tex_gui_anim.uv.x, tex_gui_anim.uv.y);
          ImVec2 uv1 = ImVec2(tex_gui_anim.uv.x + tex_gui_anim.uv.w,
                              tex_gui_anim.uv.y + tex_gui_anim.uv.h);
          ImGui::Image((void *) (intptr_t) tex->id,
                       ImVec2(tex->width * tex_gui_anim.uv.w * tex_scale,
                              tex->height * tex_gui_anim.uv.h * tex_scale),
                       uv0, uv1);
      } else {
          ImGui::Image((void *) (intptr_t) tex->id,
                       ImVec2(tex->width * tex_scale,
                              tex->height * tex_scale));
      }
      */
}

void editor_asset_text_gui(char *text) {
  /*
      ImGui::InputTextMultiline("File edit", text, ASSET_TEXT_BUF,
                                ImVec2(600, 500),
                                ImGuiInputTextFlags_CallbackAlways |
                                ImGuiInputTextFlags_CallbackCharFilter,
                                TextEditCallback);
      if (ImGui::Button("Save")) {
          FILE *f = fopen(selected_asset->filename, "wd");
          size_t len = strlen(text);
          fwrite(text, len, 1, f);
          fclose(f);
      }
      */
}

void editor_asset_gui(struct fileasset *asset) {

  nk_begin(ctx, "Asset Viewer", nk_rect_std, nuk_std);

  nk_labelf(ctx, NK_TEXT_LEFT, "%s", selected_asset->filename);

  // ImGui::SameLine();
  if (nk_button_label(ctx, "Close"))
    selected_asset = NULL;

  switch (asset->type) {
  case ASSET_TYPE_NULL:
    break;

  case ASSET_TYPE_IMAGE:
    editor_asset_tex_gui((struct Texture *)asset->data);
    break;

  case ASSET_TYPE_TEXT:
    editor_asset_text_gui((char *)asset->data);
    break;
  }

  nk_end(ctx);
}

void editor_makenewobject() {}

int obj_gui_hierarchy(struct mGameObject *selected) {

  for (int i = 0; i < gameobjects->len; i++) {
    struct mGameObject *go = (struct mGameObject *)vec_get(gameobjects, i);

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
    gameobject_makefromprefab(prefab);
    /*GameObject* newprefab = (GameObject*)createPrefab(*prefab); */
    /*cam_inverse_goto(&camera, &newprefab->transform); */
  }
}

void game_start() { physOn = 1; }

void game_resume() { physOn = 1; }

void game_stop() { physOn = 0; }

void game_pause() { physOn = 0; }

void sprite_gui(struct mSprite *sprite) {
  nk_labelf(ctx, NK_TEXT_LEFT, "Path %s", sprite->tex->path);
  // ImGui::SameLine();

  if (nk_button_label(ctx, "Load texture") && selected_asset != NULL) {
    sprite_loadtex(sprite, selected_asset->filename);
  }

  if (sprite->tex != NULL) {
    nk_labelf(ctx, NK_TEXT_LEFT, "%s", sprite->tex->path);
    nk_labelf(ctx, NK_TEXT_LEFT, "%dx%d", sprite->tex->width,
              sprite->tex->height);
    if (nk_button_label(ctx, "Imgbutton"))
      editor_selectasset_str(sprite->tex->path);
    // if (ImGui::ImageButton ((void *) (intptr_t) sprite->tex->id, ImVec2(50,
    // 50))) {
  }
  nk_property_float2(ctx, "Sprite Position", -1.f, sprite->pos, 0.f, 0.01f,
                     0.01f);

  nk_layout_row_dynamic(ctx, 25, 3);
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