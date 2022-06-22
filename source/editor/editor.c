#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION
#define NK_KEYSTATE_BASED_INPUT

#include "nuklear.h"
#include "nuklear_glfw_gl3.h"

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

#include "openglrender.h"
#include "editor.h"
#include "window.h"
#include "resources.h"
#include "registry.h"
#include "datastream.h"
#include "gameobject.h"
#include "camera.h"
#include "shader.h"
#include <dirent.h>
#include <stdio.h>
#include "editorstate.h"
#include <stdlib.h>
#include "input.h"
#include "2dphysics.h"
#include "debugdraw.h"
#include "level.h"
#include "texture.h"
#include "sprite.h"
#include <chipmunk/chipmunk.h>
#include "math.h"
#include <ftw.h>
#include <ctype.h>
#include "pinball.h"
#include "config.h"
#include "vec.h"
#include "debug.h"
#include "script.h"


#include <stb_ds.h>
#define ASSET_TEXT_BUF 1024*1024	/* 1 MB buffer for editing text files */

struct gameproject *cur_project;
struct vec *projects;
static char setpath[MAXPATH];


// Menus
// TODO: Pack this into a bitfield
static struct editorVars editor = { 0 };

bool flashlightOn = false;

// Lighting effect flags
static bool renderAO = true;
static bool renderDynamicShadows = true;

// Debug render modes
static bool renderGizmos = false;
static bool showGrid = true;
static bool debugDrawPhysics = false;

const char *allowed_extensions[] = { "jpg", "png", "gltf", "glsl" };

static const char *editor_filename = "editor.ini";

//static ImGuiIO *io = NULL;

struct asset {
    char *key;
    struct fileasset *value;
};

static struct asset *assets = NULL;

static char asset_search_buffer[100] = { 0 };

struct fileasset *selected_asset;

static int selected_index = -1;

static int grid1_width = 1;
static int grid1_span = 100;
static int grid2_width = 3;
static int grid2_span = 1000;
static bool grid1_draw = true;
static bool grid2_draw = true;

static float tex_scale = 1.f;
static struct TexAnimation tex_gui_anim = { 0 };

char current_level[MAXNAME] = { '\0' };
char levelname[MAXNAME] = { '\0' };

static struct vec *levels = NULL;

static const int ASSET_WIN_SIZE = 512;

static const char *get_extension(const char *filepath)
{
    return strrchr(filepath, '.');
}

static int
check_if_resource(const char *fpath, const struct stat *sb, int typeflag,
		  struct FTW *ftwbuf)
{
    if (typeflag == FTW_F) {
	const char *ext = get_extension(fpath);
	if (ext && is_allowed_extension(ext)) {
	    struct fileasset *newasset =
		(struct fileasset *) calloc(1, sizeof(struct fileasset));
	    newasset->filename =
		(char *) malloc(sizeof(char) * strlen(fpath) + 1);
	    strcpy(newasset->filename, fpath);
	    newasset->extension_len = strlen(ext);
	    newasset->searched = true;
	    shput(assets, newasset->filename, newasset);
	}
    }

    return 0;
}

static void print_files_in_directory(const char *dirpath)
{
    int nflags = 0;
    shfree(assets);
    nftw(dirpath, &check_if_resource, 10, nflags);
}

static void get_all_files()
{
    print_files_in_directory(DATA_PATH);
}

static int *compute_prefix_function(const char *str)
{
    int str_len = strlen(str);
    int *pi = (int *) malloc(sizeof(int) * str_len);
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

static bool kmp_match(const char *search, const char *text, int *pi)
{
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



static int MyCallback()//ImGuiInputTextCallbackData * data)
{
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

static int TextEditCallback()//ImGuiInputTextCallbackData * data)
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

void editor_save()
{
    FILE *feditor = fopen(editor_filename, "w+");
    fwrite(&editor, sizeof(editor), 1, feditor);
    fclose(feditor);
}

static void edit_input_cb(GLFWwindow *w, int key, int scancode, int action, int mods)
{
    if (editor_wantkeyboard()) return;

    switch (key) {
        case GLFW_KEY_ESCAPE:
	    quit = true;
	    editor_save_projects();
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
	    editor.showAssetMenu = !editor.showAssetMenu;
	    break;

	case GLFW_KEY_F3:
	    editor.showStats = !editor.showStats;
	    break;

	case GLFW_KEY_F4:
	    editor.showHierarchy = !editor.showHierarchy;
	    break;

	case GLFW_KEY_F5:
	    editor.showLighting = !editor.showLighting;
	    break;

	case GLFW_KEY_F6:
	    editor.showGameSettings = !editor.showGameSettings;
	    break;

	case GLFW_KEY_F7:
	    editor.showViewmode = !editor.showViewmode;
	    break;

	case GLFW_KEY_F8:
	    editor.showDebugMenu = !editor.showDebugMenu;
	    break;

	case GLFW_KEY_F9:
	    editor.showExport = !editor.showExport;
	    break;

	case GLFW_KEY_F10:
	    editor.showLevel = !editor.showLevel;
	    break;

	case GLFW_KEY_F11:
	    window_togglefullscreen(mainwin);
	    break;

	case GLFW_KEY_GRAVE_ACCENT:
	    editor.showREPL = !editor.showREPL;
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

static void edit_mouse_cb(GLFWwindow *w, int button, int action, int mods)
{
    if (editor_wantkeyboard()) return;

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

struct nk_context *ctx;
struct nk_glfw nkglfw = {0};

void editor_init(struct mSDLWindow *window)
{
    projects = vec_make(sizeof(struct gameproject), 5);
    levels = vec_make(MAXNAME, 10);
    editor_load_projects();

    FILE *feditor = fopen(editor_filename, "r");
    if (feditor == NULL) {
	editor_save();
    } else {
	fread(&editor, sizeof(editor), 1, feditor);
	fclose(feditor);
    }

    ctx = nk_glfw3_init(&nkglfw, window->window, NK_GLFW3_INSTALL_CALLBACKS);


   struct nk_font_atlas *atlas;
    nk_glfw3_font_stash_begin(&nkglfw, &atlas);
    struct nk_font *droid = nk_font_atlas_add_from_file(atlas, "fonts/notosans.tff", 14, 0);
    nk_glfw3_font_stash_end(&nkglfw);

    glfwSetKeyCallback(window->window, edit_input_cb);
    glfwSetMouseButtonCallback(window->window, edit_mouse_cb);
}

void editor_input()
{
}

int editor_wantkeyboard()
{
    return 0;
}

const int nuk_std = NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
            NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE;

const struct nk_rect nk_rect_std = {250, 250, 250, 250};

void editor_project_gui()
{
/* Grid, etc */
    if (grid1_draw)
	draw_grid(grid1_width, grid1_span);
    if (grid2_draw)
	draw_grid(grid2_width, grid2_span);



    if (debugDrawPhysics) {
	/*
	   for (int i = 0; i < number_of_gameobjects(); i++)
	   phys2d_dbgdrawcircle(objects[i]->circle);
	 */
    }

    static char text[3][64];
    static int text_len[3];
    static const char *items[] = {"Item 0","item 1","item 2"};
    static int selected_item = 0;
    static int check = 1;

    int i;

    if (nk_begin(ctx, "Grid Demo", nk_rect(600, 350, 275, 250),
        NK_WINDOW_TITLE|NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|
        NK_WINDOW_NO_SCROLLBAR))
    {

        nk_layout_row_dynamic(ctx, 30, 2);
        nk_label(ctx, "Floating point:", NK_TEXT_RIGHT);
        nk_edit_string(ctx, NK_EDIT_FIELD, text[0], &text_len[0], 64, nk_filter_float);
        nk_label(ctx, "Hexadecimal:", NK_TEXT_RIGHT);
        nk_edit_string(ctx, NK_EDIT_FIELD, text[1], &text_len[1], 64, nk_filter_hex);
        nk_label(ctx, "Binary:", NK_TEXT_RIGHT);
        nk_edit_string(ctx, NK_EDIT_FIELD, text[2], &text_len[2], 64, nk_filter_binary);
        nk_label(ctx, "Checkbox:", NK_TEXT_RIGHT);
        nk_checkbox_label(ctx, "Check me", &check);
        nk_label(ctx, "Combobox:", NK_TEXT_RIGHT);
        if (nk_combo_begin_label(ctx, items[selected_item], nk_vec2(nk_widget_width(ctx), 200))) {
            nk_layout_row_dynamic(ctx, 25, 1);
            for (i = 0; i < 3; ++i)
                if (nk_combo_item_label(ctx, items[i], NK_TEXT_LEFT))
                    selected_item = i;
            nk_combo_end(ctx);
        }
    }
nk_end(ctx);

if (nk_begin(ctx, "Menu Demo", nk_rect(600, 350, 275, 250), nuk_std)) {
     nk_menubar_begin(ctx);

     nk_layout_row_dynamic(ctx, 30, 4);

     char bbbuf[256];
     snprintf(bbbuf, 256, "Current level: %s", current_level[0] == '\0' ? "Level not saved!" : current_level);
     nk_label(ctx, bbbuf, NK_TEXT_LEFT);

     if (nk_menu_begin_text(ctx, "Windows", 7, NK_TEXT_LEFT, nk_vec2(100, 30))) {
         nk_layout_row_dynamic(ctx, 30, 1);

          if (nk_button_label(ctx, "Resources")) editor.showAssetMenu = !editor.showAssetMenu;
          if (nk_button_label(ctx, "Hierarchy")) editor.showHierarchy = !editor.showHierarchy;

          /*
          	    ImGui::MenuItem("Lighting", "F5", &editor.showLighting);
	    ImGui::MenuItem("Game Settings", "F6",
			    &editor.showGameSettings);
	    ImGui::MenuItem("View", "F7", &editor.showViewmode);
	    ImGui::MenuItem("Debug", "F8", &editor.showDebugMenu);
	    ImGui::MenuItem("Export", "F9", &editor.showExport);
	    ImGui::MenuItem("Level", "F10", &editor.showLevel);
	    ImGui::MenuItem("REPL", "`", &editor.showREPL);

          */

     }

     if (nk_menu_begin_text(ctx, "Levels", 100, 0, nk_vec2(100, 50))) {

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

	   // ImGui::InputText("", levelname, MAXNAME);

	   vec_walk(levels, (void (*)(void *)) &editor_level_btn);

     }

    nk_menubar_end(ctx);
}
nk_end(ctx);


    if (editor.showExport && nk_begin(ctx, "Export and Bake", nk_rect_std, nuk_std)) {

	if (nk_button_label(ctx, "Bake")) {
	}
	if (nk_button_label(ctx, "Build")) {
	}

	nk_end(ctx);
    }


    // Shadow map vars
    if (editor.showLighting && nk_begin(ctx, "Lighting options", nk_rect_std, nuk_std)) {
        nk_layout_row_dynamic(ctx, 25, 1);
	if (nk_combo_begin_text(ctx, "Directional shadow map", 50, nk_vec2(nk_widget_width(ctx), 400))) {
	    nk_slider_float(ctx, -200.f, &near_plane, 200.f, 1.f);
	    nk_slider_float(ctx, -200.f, &far_plane, 200.f, 1.f);
	    nk_slider_float(ctx, 0.f, &shadowLookahead, 100.f, 1.f);
	    nk_slider_float(ctx, 0.f, &plane_size, 100.f, 1.f);
	}

	nk_end(ctx);
    }

    /*

    if (editor.showGameSettings) {
	ImGui::Begin("Game settings", &editor.showGameSettings);

	ImGui::InputText("Game name", cur_project->name, 127);

	if (ImGui::CollapsingHeader("Physics")) {
	    ImGui::DragFloat("2d Gravity", &phys2d_gravity, 1.f, -5000.f,
			     0.f, "%.3f");
	    phys2d_apply();
	}

	if (ImGui::CollapsingHeader("Quality")) {

	}

	ImGui::End();
    }


    if (editor.showStats) {
	ImGui::Begin("Stats", &editor.showStats);
	ImGui::Text("FPS: %2.4f", 1.f / deltaT);
	ImGui::Text("Triangles rendered: %llu", triCount);
	ImGui::End();
    }


    if (editor.showREPL) {
	ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
	ImGui::Begin("REPL", &editor.showREPL);
	static char buffer[512] = { '\0' };
	if (ImGui::InputText("", buffer, 512, flags)) {
	    //scheme_load_string(sc, buffer);
	    script_run(buffer);
	    buffer[0] = { '\0' };
	}
	ImGui::End();

    }

    if (editor.showViewmode) {
	ImGui::Begin("View options", &editor.showViewmode);

	ImGui::SliderFloat("Camera FOV", &editorFOV, 0.1f, 90.f);
	ImGui::SliderFloat("Camera Near Plane", &editorClose, 0.1f, 5.f);
	ImGui::SliderFloat("Camera Far Plane", &editorFar, 50.f, 10000.f);

	if (ImGui::CollapsingHeader("Shading mode")) {
	    ImGui::RadioButton("Lit", &renderMode, RenderMode::LIT);
	    ImGui::RadioButton("Unlit", &renderMode, RenderMode::UNLIT);
	    ImGui::RadioButton("Wireframe", &renderMode,
			       RenderMode::WIREFRAME);
	    ImGui::RadioButton("Directional shadow map", &renderMode,
			       RenderMode::DIRSHADOWMAP);
	}

	if (ImGui::CollapsingHeader("Lighting")) {
	    ImGui::Checkbox("Shadows", &renderDynamicShadows);
	    ImGui::Checkbox("Ambient Occlusion", &renderAO);
	}

	if (ImGui::CollapsingHeader("Debug Draws")) {
	    ImGui::Checkbox("Gizmos", &renderGizmos);
	    ImGui::Checkbox("Grid", &showGrid);
	    ImGui::Checkbox("Physics", &debugDrawPhysics);
	}

	ImGui::End();
    }

    if (editor.showHierarchy) {
	ImGui::Begin("Objects", &editor.showHierarchy);

	if (ImGui::Button("New Object")) {
	    MakeGameobject();
	}

	obj_gui_hierarchy(selectedobject);

	ImGui::End();
    }

    ImGui::Begin("Simulate");

    if (physOn) {
	if (ImGui::Button("Pause"))
	    game_pause();

	ImGui::SameLine();
	if (ImGui::Button("Stop"))
	    game_stop();
    } else {
	if (ImGui::Button("Play"))
	    game_start();
    }

    ImGui::End();


    ImGui::Begin("Prefab Creator");

    vec_walk(prefabs, (void (*)(void *)) &editor_prefab_btn);

    ImGui::End();


    if (editor.showAssetMenu) {
	ImGui::Begin("Asset Menu", &editor.showAssetMenu);

	ImGui::InputText("Search", asset_search_buffer, 100,
			 ImGuiInputTextFlags_CallbackEdit, MyCallback);

	if (ImGui::Button("Reload all files"))
	    get_all_files();

	ImGui::BeginChild("##scrolling");
	for (int i = 0; i < shlen(assets); i++) {
	    if (!assets[i].value->searched)
		continue;

	    if (ImGui::Button(assets[i].value->filename + stemlen)) {
		editor_selectasset(assets[i].value);
	    }
	}
	ImGui::EndChild();

	ImGui::End();
    }

    if (selected_asset)
	editor_asset_gui(selected_asset);




    if (editor.showDebugMenu) {
	ImGui::Begin("Debug Menu", &editor.showDebugMenu);
	if (ImGui::Button("Reload Shaders")) {
	    shader_compile_all();
	}
	//ImGui::SliderFloat("Grid scale", &gridScale, 100.f, 500.f, "%1.f");

	ImGui::SliderInt("Grid 1 Span", &grid1_span, 1, 500);
	ImGui::SameLine();
	ImGui::Checkbox("Draw", &grid1_draw);

	ImGui::SliderInt("Grid 2 Span", &grid2_span, 10, 1000);
	ImGui::SameLine();
	ImGui::Checkbox("Draw", &grid2_draw);


	   ImGui::SliderFloat("Grid Opacity", &gridOpacity, 0.f, 1.f);
	   ImGui::SliderFloat("Small unit", &smallGridUnit, 0.5f, 5.f);
	   ImGui::SliderFloat("Big unit", &bigGridUnit, 10.f, 50.f);
	   ImGui::SliderFloat("Small thickness", &gridSmallThickness, 1.f, 10.f, "%1.f");
	   ImGui::SliderFloat("Big thickness", &gridBigThickness, 1.f, 10.f, "%1.f");
	   ImGui::ColorEdit3("1 pt grid color", (float*)&gridSmallColor);
	   ImGui::ColorEdit3("10 pt grid color", (float*)&gridBigColor);

	//ImGui::SliderInt("MSAA", &msaaSamples, 0, 4);
	ImGui::End();
    }

  startobjectgui:

    if (selectedobject) {
	draw_point(selectedobject->transform.position[0],
		   selectedobject->transform.position[1], 5);


	ImGui::Begin("Object Parameters");

	if (ImGui::Button("Save"))
	    gameobject_saveprefab(selectedobject);

	ImGui::SameLine();
	if (ImGui::Button("Del")) {
	    gameobject_delete(selected_index);
	    pickGameObject(-1);
	    ImGui::End();
	    goto startobjectgui;
	}

	ImGui::SameLine();
	if (selectedobject->editor.prefabSync) {
	    if (ImGui::Button("Revert"))
		gameobject_revertprefab(selectedobject);

	}


	ImGui::InputText("Name", selectedobject->editor.mname, 50);

	ImGui::InputText("Prefab", selectedobject->editor.prefabName, 50,
			 selectedobject->editor.
			 prefabSync ? ImGuiInputTextFlags_ReadOnly : 0);

	object_gui(selectedobject);


	ImGui::End();


	ImGui::Begin("Components");

	for (int i = 0; i < ncomponent; i++) {
	    if (ImGui::Button(components[i].name)) {
		gameobject_addcomponent(selectedobject, &components[i]);
	    }
	}


	ImGui::End();

    }

*/
}

void editor_render()
{
    nk_glfw3_new_frame(&nkglfw);

    struct nk_colorf bg;
    bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;

    if (cur_project)
	editor_project_gui();
    else
	editor_proj_select_gui();

	editor_project_gui();

       if (nk_begin(ctx, "Demo", nk_rect(50, 50, 230, 250),
            NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
            NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
        {
            enum {EASY, HARD};
            static int op = EASY;
            static int property = 20;
            nk_layout_row_static(ctx, 30, 80, 1);
            if (nk_button_label(ctx, "button"))
                fprintf(stdout, "button pressed\n");

            nk_layout_row_dynamic(ctx, 30, 2);
            if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
            if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;

            nk_layout_row_dynamic(ctx, 25, 1);
            nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);

            nk_layout_row_dynamic(ctx, 20, 1);
            nk_label(ctx, "background:", NK_TEXT_LEFT);
            nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_combo_begin_color(ctx, nk_rgb_cf(bg), nk_vec2(nk_widget_width(ctx),400))) {
                nk_layout_row_dynamic(ctx, 120, 1);
                bg = nk_color_picker(ctx, bg, NK_RGBA);
                nk_layout_row_dynamic(ctx, 25, 1);
                bg.r = nk_propertyf(ctx, "#R:", 0, bg.r, 1.0f, 0.01f,0.005f);
                bg.g = nk_propertyf(ctx, "#G:", 0, bg.g, 1.0f, 0.01f,0.005f);
                bg.b = nk_propertyf(ctx, "#B:", 0, bg.b, 1.0f, 0.01f,0.005f);
                bg.a = nk_propertyf(ctx, "#A:", 0, bg.a, 1.0f, 0.01f,0.005f);
                nk_combo_end(ctx);
            }
        }
nk_end(ctx);

    nk_glfw3_render(&nkglfw, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
}


void pickGameObject(int pickID)
{
    if (pickID >= 0 && pickID < gameobjects->len) {
	selected_index = pickID;
	selectedobject =
	    (struct mGameObject *) vec_get(gameobjects, pickID);
    } else {
	selected_index = -1;
	selectedobject = NULL;
    }
}

int is_allowed_extension(const char *ext)
{
    for (size_t i = 0;
	 i < sizeof(allowed_extensions) / sizeof(allowed_extensions[0]);
	 i++) {
	if (!strcmp(ext + 1, allowed_extensions[i]))
	    return true;
    }

    return false;
}

void editor_level_btn(char *level)
{

    if (nk_button_label(ctx, level)) {
	load_level(level);
	strcpy(current_level, level);
    }

}

void editor_selectasset(struct fileasset *asset)
{
    const char *ext = get_extension(asset->filename);

    if (!strcmp(ext + 1, "png") || !strcmp(ext + 1, "jpg")) {
	asset->data = texture_loadfromfile(asset->filename);
	tex_gui_anim.tex = (struct Texture *) asset->data;
	asset->type = ASSET_TYPE_IMAGE;
	tex_anim_set(&tex_gui_anim);
	//float tex_scale = float((float) ASSET_WIN_SIZE / tex_gui_anim.tex->width);
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

void editor_selectasset_str(char *path)
{
    struct fileasset *asset = (struct fileasset*)shget(assets, path);

    if (asset)
	editor_selectasset(asset);
}

void editor_asset_tex_gui(struct Texture *tex)
{
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

void editor_asset_text_gui(char *text)
{
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

void editor_asset_gui(struct fileasset *asset)
{
/*
    ImGui::Begin("Asset Viewer");

    ImGui::Text("%s", selected_asset->filename);

    ImGui::SameLine();
    if (ImGui::Button("Close"))
        selected_asset = NULL;

    switch (asset->type) {
    case ASSET_TYPE_NULL:
	break;

    case ASSET_TYPE_IMAGE:
	editor_asset_tex_gui((struct Texture *) asset->data);
	break;

    case ASSET_TYPE_TEXT:
	editor_asset_text_gui((char *) asset->data);
	break;
    }

    ImGui::End();
    */
}

void editor_load_projects()
{
    FILE *f = fopen("projects.yugh", "r");
    if (!f)
	return;

    vec_load(projects, f);
    fclose(f);
}

void editor_save_projects()
{
    FILE *f = fopen("projects.yugh", "w");
    vec_store(projects, f);
    fclose(f);
}

void editor_project_btn_gui(struct gameproject *gp)
{
/*
    if (ImGui::Button(gp->name))
	editor_init_project(gp);


    ImGui::SameLine();
    ImGui::Text("%s", gp->path);
    */
}

void editor_proj_select_gui()
{
/*
    ImGui::Begin("Project Select");

    vec_walk(projects, (void (*)(void *)) &editor_project_btn_gui);

    ImGui::InputText("Project import path", setpath, MAXPATH);
    ImGui::SameLine();
    if (ImGui::Button("Create")) {
	editor_make_project(setpath);
    }
    ImGui::SameLine();
    if (ImGui::Button("Import")) {
	editor_import_project(setpath);
    }

    ImGui::End();
    */
}

void editor_init_project(struct gameproject *gp)
{
    cur_project = gp;
    DATA_PATH = strdup(gp->path);
    stemlen = strlen(DATA_PATH);
    findPrefabs();
    get_levels();
    get_all_files();
}

void editor_make_project(char *path)
{
    FILE *f = path_open("w", "%s%s", path, "/project.yugh");
    cur_project =
	(struct gameproject *) malloc(sizeof(struct gameproject));
    strncpy(cur_project->name, "New Game", 127);
    strncpy(cur_project->path, path, 2048);
    vec_add(projects, cur_project);
    fwrite(cur_project, sizeof(*cur_project), 1, f);
    fclose(f);

    editor_init_project(cur_project);

    editor_save_projects();
}

void editor_import_project(char *path)
{
    FILE *f = path_open("r", "%s%s", path, "/project.yugh");
    if (!f)
	return;

    struct gameproject *gp = (struct gameproject *) malloc(sizeof(*gp));
    fread(gp, sizeof(*gp), 1, f);
    fclose(f);

    vec_add(projects, gp);
}


/////// Object GUIs
#include "light.h"
#include "transform.h"
#include "static_actor.h"

/*
void light_gui(struct mLight *light)
{
    object_gui(&light->obj);

    if (ImGui::CollapsingHeader("Light")) {
	ImGui::DragFloat("Strength", &light->strength, 0.001f, 0.f, 1.f);
	ImGui::ColorEdit3("Color", &light->color[0]);
	ImGui::Checkbox("Dynamic", (bool *) &light->dynamic);
    }
}


void pointlight_gui(struct mPointLight *light)
{
    light_gui(&light->light);

    if (ImGui::CollapsingHeader("Point Light")) {
	ImGui::DragFloat("Constant", &light->constant, 0.001f, 0.f, 1.f);
	ImGui::DragFloat("Linear", &light->linear, 0.001f, 0.f, 0.3f);
	ImGui::DragFloat("Quadratic", &light->quadratic, 0.001f, 0.f,
			 0.3f);
    }

}

void spotlight_gui(struct mSpotLight *spot)
{
    light_gui(&spot->light);

    if (ImGui::CollapsingHeader("Spotlight")) {
	ImGui::DragFloat("Linear", &spot->linear, 0.001f, 0.f, 1.f);
	ImGui::DragFloat("Quadratic", &spot->quadratic, 0.001f, 0.f, 1.f);
	ImGui::DragFloat("Distance", &spot->distance, 0.1f, 0.f, 200.f);
	ImGui::DragFloat("Cutoff Degrees", &spot->cutoff, 0.01f, 0.f,
			 0.7f);
	ImGui::DragFloat("Outer Cutoff Degrees", &spot->outerCutoff, 0.01f,
			 0.f, 0.7f);
    }
}
*/

void staticactor_gui(struct mStaticActor *sa)
{
    object_gui(&sa->obj);
    /*
    if (ImGui::CollapsingHeader("Model")) {
	ImGui::Checkbox("Cast Shadows", &sa->castShadows);
	ImGui::Text("Model path: %s", sa->currentModelPath);

	ImGui::SameLine();
	if (ImGui::Button("Load model")) {
	    //asset_command = set_new_model;
	    curActor = sa;
	}

    }
    */
}

void trans_drawgui(struct mTransform *T)
{
    /*ImGui::DragFloat3("Position", (float *) &T->position, 0.01f, -1000.f,
       1000.f, "%4.2f");
       ImGui::DragFloat("Rotation", (float *) &T->rotation[0]   , 0.5f, 0.f,
       360.f, "%4.2f");

    ImGui::DragFloat("Scale", (float *) &T->scale, 0.001f, 0.f, 1000.f,
		     "%3.1f");
		     */
}

void object_gui(struct mGameObject *go)
{
    float temp_pos[2];
    temp_pos[0] = cpBodyGetPosition(go->body).x;
    temp_pos[1] = cpBodyGetPosition(go->body).y;

    draw_point(temp_pos[0], temp_pos[1], 3);

    //ImGui::DragFloat2("Position", (float *) temp_pos, 1.f, 0.f, 0.f,
		   //   "%.0f");

    cpVect tvect = { temp_pos[0], temp_pos[1] };
    cpBodySetPosition(go->body, tvect);

    float mtry = cpBodyGetAngle(go->body);
    float modtry = fmodf(mtry * RAD2DEGS, 360.f);
    float modtry2 = modtry;
    //ImGui::DragFloat("Angle", &modtry, 0.5f, -1000.f, 1000.f, "%3.1f");
    modtry -= modtry2;
    cpBodySetAngle(go->body, mtry + (modtry * DEG2RADS));

   // ImGui::DragFloat("Scale", &go->scale, 0.001f, 0.f, 1000.f, "%3.3f");

  /*  if (ImGui::Button("Start")) {

    }

    ImGui::SameLine();
    if (ImGui::Button("Update")) {

    }

    ImGui::SameLine();
    if (ImGui::Button("Fixed Update")) {

    }

    ImGui::SameLine();
    if (ImGui::Button("End")) {

    }

    ImGui::RadioButton("Static", (int *) &go->bodytype,
		       (int) CP_BODY_TYPE_STATIC);
    ImGui::SameLine();
    ImGui::RadioButton("Dynamic", (int *) &go->bodytype,
		       (int) CP_BODY_TYPE_DYNAMIC);
    ImGui::SameLine();
    ImGui::RadioButton("Kinematic", (int *) &go->bodytype,
		       (int) CP_BODY_TYPE_KINEMATIC);

    cpBodySetType(go->body, go->bodytype);

    if (go->bodytype == CP_BODY_TYPE_DYNAMIC) {
	ImGui::DragFloat("Mass", &go->mass, 0.01f, 0.01f, 1000.f);
	cpBodySetMass(go->body, go->mass);
    }

    ImGui::DragFloat("Friction", &go->f, 0.01f, 0.f, 10.f);
    ImGui::DragFloat("Elasticity", &go->e, 0.01f, 0.f, 2.f);

    int n = -1;

    for (int i = 0; i < go->components->len; i++) {
	ImGui::PushID(i);

	struct component *c =
	    (struct component *) vec_get(go->components, i);

	if (c->draw_debug)
	    c->draw_debug(c->data);

	if (ImGui::CollapsingHeader(c->name)) {
	    if (ImGui::Button("Del")) {
		n = i;
	    }

	    c->draw_gui(c->data);


	}

	ImGui::PopID();
    }

    if (n >= 0)
	gameobject_delcomponent(go, n);
*/
}


void sprite_gui(struct mSprite *sprite)
{

    //ImGui::Text("Path", sprite->tex->path);
    //ImGui::SameLine();
    /*
    if (ImGui::Button("Load texture") && selected_asset != NULL) {
	sprite_loadtex(sprite, selected_asset->filename);
    }

    if (sprite->tex != NULL) {
	ImGui::Text("%s", sprite->tex->path);
	ImGui::Text("%dx%d", sprite->tex->width, sprite->tex->height);
	if (ImGui::ImageButton
	    ((void *) (intptr_t) sprite->tex->id, ImVec2(50, 50))) {
	    editor_selectasset_str(sprite->tex->path);
	}
    }

    ImGui::DragFloat2("Sprite Position", (float *) sprite->pos, 0.01f,
		      -1.f, 0.f);

    if (ImGui::Button("C")) {
	sprite->pos[0] = -0.5f;
	sprite->pos[1] = -0.5f;
    }

    ImGui::SameLine();
    if (ImGui::Button("U")) {
	sprite->pos[0] = -0.5f;
	sprite->pos[1] = -1.f;
    }

    ImGui::SameLine();
    if (ImGui::Button("D")) {
	sprite->pos[0] = -0.5f;
	sprite->pos[1] = 0.f;
    }
*/
}

void circle_gui(struct phys2d_circle *circle)
{

  //  ImGui::DragFloat("Radius", &circle->radius, 1.f, 1.f, 10000.f);
//    ImGui::DragFloat2("Offset", circle->offset, 1.f, 0.f, 0.f);

    phys2d_applycircle(circle);



}

void segment_gui(struct phys2d_segment *seg)
{

//    ImGui::DragFloat2("a", seg->a, 1.f, 0.f, 0.f);
 //   ImGui::DragFloat2("b", seg->b, 1.f, 0.f, 0.f);

    phys2d_applyseg(seg);



}

void box_gui(struct phys2d_box *box)
{
/*
    ImGui::DragFloat("Width", &box->w, 1.f, 0.f, 1000.f);
    ImGui::DragFloat("Height", &box->h, 1.f, 0.f, 1000.f);
    ImGui::DragFloat2("Offset", box->offset, 1.f, 0.f, 0.f);
    ImGui::DragFloat("Radius", &box->r, 1.f, 0.f, 100.f);
*/
    phys2d_applybox(box);



}

void poly_gui(struct phys2d_poly *poly)
{
/*
    if (ImGui::Button("Add Poly Vertex"))
	phys2d_polyaddvert(poly);

    for (int i = 0; i < poly->n; i++) {
	ImGui::PushID(i);
	ImGui::DragFloat2("P", &poly->points[i * 2], 1.f, 0.f, 0.f);
	ImGui::PopID();
    }

    ImGui::DragFloat("Radius", &poly->radius);

    phys2d_applypoly(poly);

*/


}

void edge_gui(struct phys2d_edge *edge)
{
/*
    if (ImGui::Button("Add Edge Vertex"))
	phys2d_edgeaddvert(edge);

    for (int i = 0; i < edge->n; i++) {
	ImGui::PushID(i);
	ImGui::DragFloat2("E", &edge->points[i * 2], 1.f, 0.f, 0.f);
	ImGui::PopID();
    }

    ImGui::DragFloat("Thickness", &edge->thickness);

    phys2d_applyedge(edge);


*/
}

void editor_makenewobject()
{

}

int obj_gui_hierarchy(struct mGameObject *selected)
{
/*
    for (int i = 0; i < gameobjects->len; i++) {
	struct mGameObject *go =
	    (struct mGameObject *) vec_get(gameobjects, i);
	if (ImGui::Selectable(go->editor.mname, go == selected, 1 << 22)) {
	    if (go != selected)
		pickGameObject(i);
	}
    }
*/
    return 0;
}

void get_levels()
{
    fill_extensions(levels, DATA_PATH, EXT_LEVEL);
}

void editor_prefab_btn(char *prefab)
{
   // if (ImGui::Button(prefab)) {
//	gameobject_makefromprefab(prefab);
	/*GameObject* newprefab = (GameObject*)createPrefab(*prefab); */
	/*cam_inverse_goto(&camera, &newprefab->transform); */
//    }

}

void game_start()
{
    physOn = 1;
}

void game_resume()
{
    physOn = 1;
}

void game_stop()
{
    physOn = 0;
}

void game_pause()
{
    physOn = 0;
}

void pinball_flipper_gui(struct flipper *flip)
{
/*
    ImGui::DragFloat("Angle start", &flip->angle1, 0, 360);
    ImGui::DragFloat("Angle end", &flip->angle2, 0, 360);
    ImGui::DragFloat("Flipper speed", &flip->flipspeed, 0, 100);
    */
}
