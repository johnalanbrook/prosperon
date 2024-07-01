#include "gui.h"

#include "render.h"
#include "sokol/sokol_app.h"
#include "imgui.h"
#define SOKOL_IMPL
#include "sokol/util/sokol_imgui.h"
#include "sokol/util/sokol_gfx_imgui.h"

static sgimgui_t sgimgui;

#include "jsffi.h"

JSC_CCALL(imgui_begin,
char *title = js2strdup(argv[0]);
bool active = true;
ImGui::Begin(title, &active, ImGuiWindowFlags_MenuBar);
free(title);
return boolean2js(active);
)

JSC_CCALL(imgui_end, ImGui::End())

JSC_CCALL(imgui_beginmenu,
  char *title = js2strdup(argv[0]);
  bool active = ImGui::BeginMenu(title);
  free(title);
  return boolean2js(active);
)

JSC_CCALL(imgui_menuitem,
  char *name = js2strdup(argv[0]);
  char *hotkey = js2strdup(argv[1]);
  if (ImGui::MenuItem(name,hotkey))
    script_call_sym(argv[2], 0, NULL);
  free(name);
  free(hotkey);
)

JSC_CCALL(imgui_beginmenubar, ImGui::BeginMenuBar())
JSC_CCALL(imgui_endmenubar, ImGui::EndMenuBar())

JSC_SSCALL(imgui_textinput,
  char buffer[512];
  strncpy(buffer, str2, 512);
  ImGui::InputText(str, buffer, sizeof(buffer));
  ret = str2js(buffer);
)

static const JSCFunctionListEntry js_imgui_funcs[] = {
  MIST_FUNC_DEF(imgui, begin, 1),
  MIST_FUNC_DEF(imgui, end,0),
  MIST_FUNC_DEF(imgui, beginmenu, 1),
  MIST_FUNC_DEF(imgui, menuitem, 3),
  MIST_FUNC_DEF(imgui, beginmenubar, 0),
  MIST_FUNC_DEF(imgui, endmenubar, 0),
  MIST_FUNC_DEF(imgui, textinput, 2),
};

static int started = 0;

JSValue gui_init(JSContext *js)
{
  simgui_desc_t sdesc = {0};
  simgui_setup(&sdesc);
  
  sgimgui_desc_t desc = {0};
  sgimgui_init(&sgimgui, &desc);

  JSValue imgui = JS_NewObject(js);
  JS_SetPropertyFunctionList(js, imgui, js_imgui_funcs, countof(js_imgui_funcs));
  started = 1;
  return imgui;
}

void gui_input(sapp_event *e)
{
  if (started)
    simgui_handle_event(e);
}

void gui_newframe(int x, int y, float dt)
{
  simgui_frame_desc_t frame = {
    .width = x,
    .height = y,
    .delta_time = dt
  };
  simgui_new_frame(&frame);
}

void gfx_gui()
{
  sgimgui_draw(&sgimgui);
  sgimgui_draw_menu(&sgimgui, "sokol-gfx");
}

void gui_endframe()
{
  simgui_render();
}

void gui_exit()
{
  sgimgui_discard(&sgimgui);
}
