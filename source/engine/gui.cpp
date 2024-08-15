#include "gui.h"

#include "render.h"
#include "sokol/sokol_app.h"
#include "imgui.h"
#include "implot.h"
#define SOKOL_IMPL
#include "sokol/util/sokol_imgui.h"
#include "sokol/util/sokol_gfx_imgui.h"

static sgimgui_t sgimgui;

#include "jsffi.h"

JSC_SCALL(imgui_window,
  bool active = true;
  ImGui::Begin(str, &active);
  script_call_sym(argv[1], 0, NULL);
  ImGui::End();
  ret = boolean2js(active);
)
JSC_SCALL(imgui_menu,
  if (ImGui::BeginMenu(str)) {
    script_call_sym(argv[1], 0, NULL);
    ImGui::EndMenu();
  }
)

JSC_CCALL(imgui_menubar,
  if (ImGui::BeginMenuBar()) {
    script_call_sym(argv[0], 0, NULL);
    ImGui::EndMenuBar();
  }
)

JSC_CCALL(imgui_mainmenubar,
  if (ImGui::BeginMainMenuBar()) {
    script_call_sym(argv[0], 0, NULL);
    ImGui::EndMainMenuBar();
  }
)

const char* imempty = "##empty";

JSC_CCALL(imgui_menuitem,
  char *name = !JS_Is(argv[0]) ? imempty : js2strdup(argv[0]);
  char *keyfn = JS_IsUndefined(argv[1]) ? NULL : js2strdup(argv[1]);
  bool on = JS_IsUndefined(argv[3]) ? false : js2boolean(argv[3]);
  if (ImGui::MenuItem(name,keyfn, &on))
    script_call_sym(argv[2], 0, NULL);

  if (name != imempty) free(name);
  if (keyfn) free(keyfn);

  return boolean2js(on);
)

JSC_SCALL(imgui_plot,
  ImPlot::SetNextAxisToFit(ImAxis_X1);
  ImPlot::SetNextAxisToFit(ImAxis_Y1);  
  ImPlot::BeginPlot(str);
  script_call_sym(argv[1], 0, NULL);
  ImPlot::EndPlot();
)

JSC_SCALL(imgui_lineplot,
  double data[js_arrlen(argv[1])];
  for (int i = 0; i < js_arrlen(argv[1]); i++)
    data[i] = js2number(js_getpropidx(argv[1], i));

  ImPlot::PlotLine(str, data, js_arrlen(argv[1]));
)

JSC_SSCALL(imgui_textinput,
  char buffer[512];
  strncpy(buffer, str2, 512);
  ImGui::InputText(str, buffer, sizeof(buffer));
  ret = str2js(buffer);
)

JSC_SCALL(imgui_text,
  ImGui::Text(str);
)

JSC_SCALL(imgui_button,
  if (ImGui::Button(str))
    script_call_sym(argv[1], 0, NULL);
)

JSC_CCALL(imgui_sokol_gfx,
  sgimgui_draw(&sgimgui);
  if (ImGui::BeginMenu("sokol-gfx")) {
    ImGui::MenuItem("Capabilities", 0, &sgimgui.caps_window.open);
    ImGui::MenuItem("Frame Stats", 0, &sgimgui.frame_stats_window.open);
    ImGui::MenuItem("Buffers", 0, &sgimgui.buffer_window.open);
    ImGui::MenuItem("Images", 0, &sgimgui.image_window.open);
    ImGui::MenuItem("Samplers", 0, &sgimgui.sampler_window.open);
    ImGui::MenuItem("Shaders", 0, &sgimgui.shader_window.open);
    ImGui::MenuItem("Pipelines", 0, &sgimgui.pipeline_window.open);
    ImGui::MenuItem("Attachments", 0, &sgimgui.attachments_window.open);
    ImGui::MenuItem("Calls", 0, &sgimgui.capture_window.open);
    ImGui::EndMenu();
  }
)

static const JSCFunctionListEntry js_imgui_funcs[] = {
  MIST_FUNC_DEF(imgui, window, 2),
  MIST_FUNC_DEF(imgui, menu, 2),
  MIST_FUNC_DEF(imgui, menubar, 1),
  MIST_FUNC_DEF(imgui, mainmenubar, 1),
  MIST_FUNC_DEF(imgui, menuitem, 3),
  MIST_FUNC_DEF(imgui, textinput, 2),
  MIST_FUNC_DEF(imgui, button, 2),
  MIST_FUNC_DEF(imgui, text, 1),
  MIST_FUNC_DEF(imgui, plot,1),
  MIST_FUNC_DEF(imgui,lineplot,2),
  MIST_FUNC_DEF(imgui, sokol_gfx, 0),
};

static int started = 0;

JSValue gui_init(JSContext *js)
{
  simgui_desc_t sdesc = {0};
  simgui_setup(&sdesc);
  
  sgimgui_desc_t desc = {0};
  sgimgui_init(&sgimgui, &desc);

  ImPlot::CreateContext();

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

void gui_endframe()
{
  simgui_render();
}

void gui_exit()
{
  sgimgui_discard(&sgimgui);
}
