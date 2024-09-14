#include "gui.h"

#include "render.h"
#include "sokol/sokol_app.h"
#include "imgui.h"
#include "implot.h"
#include "imnodes.h"
#include "imgui_neo_sequencer.h"
#include "ImSequencer.h"

#include "stb_ds.h"

#define SOKOL_IMPL
#include "sokol/util/sokol_imgui.h"
#include "sokol/util/sokol_gfx_imgui.h"
#include <stdlib.h>

static sgimgui_t sgimgui;

#include "jsffi.h"

static int wantkeys = 0;
static int wantmouse = 0;

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

JSC_CCALL(imgui_menuitem,
  char *name = js2strdup(argv[0]);
  char *keyfn = JS_IsUndefined(argv[1]) ? NULL : js2strdup(argv[1]);
  bool on = JS_IsUndefined(argv[3]) ? false : js2boolean(argv[3]);
  if (ImGui::MenuItem(JS_Is(argv[0]) ? name : "##empty" ,keyfn, &on))
    script_call_sym(argv[2], 0, NULL);

  if (JS_Is(argv[0])) free(name);
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

JSC_SSCALL(imgui_textbox,
  char buffer[512];
  strncpy(buffer, str2, 512);
  ImGui::InputTextMultiline(str, buffer, sizeof(buffer));
  ret = str2js(buffer);
)

JSC_SCALL(imgui_text, ImGui::Text(str) )

JSC_SCALL(imgui_button,
  if (ImGui::Button(str))
    script_call_sym(argv[1], 1, argv);
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

JSC_SCALL(imgui_slider,
  float low = JS_IsUndefined(argv[2]) ? 0.0 : js2number(argv[2]);
  float high = JS_IsUndefined(argv[3]) ? 1.0 : js2number(argv[3]);
  
  if (JS_IsArray(js, argv[1])) {
    int n = js_arrlen(argv[1]);
    float a[n];
    js2floatarr(argv[1], n, a);
    
    switch(n) {
      case 2: 
        ImGui::SliderFloat2(str, a, low, high);
        break;
      case 3:
        ImGui::SliderFloat3(str, a, low, high);
        break;
      case 4:
        ImGui::SliderFloat3(str, a, low, high);
        break;
    }
    
    ret = floatarr2js(n, a);
  } else {
    float val = js2number(argv[1]);
    ImGui::SliderFloat(str, &val, low, high, "%.3f");
    ret = number2js(val);
  }
)

JSC_SCALL(imgui_checkbox,
  bool val = js2boolean(argv[1]);
  ImGui::Checkbox(str, &val);
  ret = boolean2js(val);
)

JSC_CCALL(imgui_pushid,
  ImGui::PushID(js2number(argv[0]));
)

JSC_CCALL(imgui_popid, ImGui::PopID(); )

JSC_CCALL(imgui_image,
  texture *tex = js2texture(argv[0]);
  simgui_image_desc_t simgd;
  simgd.image = tex->id;
  simgd.sampler = std_sampler;
  simgui_image_t simgui_img = simgui_make_image(&simgd);
  ImTextureID tex_id = simgui_imtextureid(simgui_img);
  ImGui::Image(tex_id, ImVec2(tex->width, tex->height), ImVec2(0,0), ImVec2(1,1));
  simgui_destroy_image(simgui_img);
)

JSC_CCALL(imgui_sameline, ImGui::SameLine(js2number(argv[0])) )
JSC_CCALL(imgui_columns, ImGui::Columns(js2number(argv[0])) )
JSC_CCALL(imgui_nextcolumn, ImGui::NextColumn() )
JSC_SCALL(imgui_collapsingheader, ret = boolean2js(ImGui::CollapsingHeader(str)) )
JSC_SCALL(imgui_radio, ret = boolean2js(ImGui::RadioButton(str, js2boolean(argv[1]))))

JSC_SCALL(imgui_tree,
  if (ImGui::TreeNode(str)) {
    script_call_sym(argv[1],0,NULL);
    ImGui::TreePop();
  }
)

JSC_SCALL(imgui_tabbar,
  if (ImGui::BeginTabBar(str)) {
    script_call_sym(argv[1],0,NULL);
    ImGui::EndTabBar();
  }
)

JSC_SCALL(imgui_tab,
  if (ImGui::BeginTabItem(str)) {
    script_call_sym(argv[1],0,NULL);
    ImGui::EndTabItem();
  }
)

JSC_SCALL(imgui_listbox,
  char **arr = js2strarr(argv[1]);
  int n = js_arrlen(argv[1]);
  int idx = js2number(argv[2]);
  ImGui::ListBox(str, &idx, arr, n, 4);
  for (int i = 0; i < n; i++)
    free(arr[i]);

//  arrfree(arr); // TODO: Doesn't this need freed?
  ret = number2js(idx);
)

JSC_SCALL(imgui_int,
  int n = js2number(argv[1]);
  ImGui::InputInt(str, &n);
  ret = number2js(n);
)

JSC_SCALL(imgui_open_popup,
  ImGui::OpenPopup(str);
)

JSC_SCALL(imgui_popup,
  if (ImGui::BeginPopup(str)) {
    script_call_sym(argv[1],0,NULL);
    ImGui::EndPopup();
  }
)

JSC_CCALL(imgui_close_popup,
  ImGui::CloseCurrentPopup();
)

JSC_SCALL(imgui_modal,
  if (ImGui::BeginPopupModal(str)) {
    script_call_sym(argv[1],0,NULL);
    ImGui::EndPopup();
  }
)

JSC_SCALL(imgui_context,
  if (ImGui::BeginPopupContextItem(str)) {
    script_call_sym(argv[1],0,NULL);
    ImGui::EndPopup();
  }
)

JSC_SCALL(imgui_table,
  if (ImGui::BeginTable(str, js2number(argv[1]))) {
    script_call_sym(argv[2],0,NULL);
    ImGui::EndTable();
  }
)

JSC_CCALL(imgui_tablenextrow, ImGui::TableNextRow())
JSC_CCALL(imgui_tablenextcolumn, ImGui::TableNextColumn())

JSC_SCALL(imgui_dnd, 
  if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
    double n = js2number(argv[1]);
    ImGui::SetDragDropPayload(str, &n, sizeof(n));
    script_call_sym(argv[2],0,NULL);
    ImGui::EndDragDropSource();
  }
)

JSC_SCALL(imgui_dndtarget,
  if (ImGui::BeginDragDropTarget()) {
    if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(str)) {
      JSValue n = number2js(*(double*)payload->Data);
      script_call_sym(argv[1], 1, &n);
    }
    ImGui::EndDragDropTarget();
  }
)

JSC_SCALL(imgui_color,
  int n = js_arrlen(argv[1]);
  float color[n];
  js2floatarr(argv[1],n,color);

  if (n == 3)
    ImGui::ColorEdit3(str, color);
  else if (n == 4)
    ImGui::ColorEdit4(str, color);

  ret = floatarr2js(n, color);
)

JSC_CCALL(imgui_startnode,
  ImNodes::BeginNodeEditor();
  script_call_sym(argv[0],0,NULL);
  ImNodes::EndNodeEditor();
  int start_attr;
  int end_attr;
  if (ImNodes::IsLinkCreated(&start_attr, &end_attr))
  {
    JSValue ab[2];
    ab[0] = number2js(start_attr);
    ab[1] = number2js(end_attr);
    script_call_sym(argv[1], 2, ab);
    
    for (int i = 0; i < 2; i++) JS_FreeValue(js, ab[i]);
  }

  int node_id;
  if (ImNodes::IsNodeHovered(&node_id))
  {
    JSValue a = number2js(node_id);
    script_call_sym(argv[2],1,&a);
    JS_FreeValue(js,a);
  }

  int link_id;
  if (ImNodes::IsLinkHovered(&link_id))
  {
    JSValue a = number2js(link_id);
    script_call_sym(argv[3],1,&a);
    JS_FreeValue(js,a);
  }
)

JSC_CCALL(imgui_node,
  ImNodes::BeginNode(js2number(argv[0]));
  script_call_sym(argv[1],0,NULL);
  ImNodes::EndNode();
)

JSC_CCALL(imgui_nodein,
  ImNodes::BeginInputAttribute(js2number(argv[0]));
  script_call_sym(argv[1],0,NULL);
  ImNodes::EndInputAttribute();
)

JSC_CCALL(imgui_nodeout,
  ImNodes::BeginOutputAttribute(js2number(argv[0]));
  script_call_sym(argv[1],0,NULL);
  ImNodes::EndOutputAttribute();
)

JSC_CCALL(imgui_nodelink,
  ImNodes::Link(js2number(argv[0]), js2number(argv[1]), js2number(argv[2]));
)

JSC_CCALL(imgui_nodemini, ImNodes::MiniMap(js2number(argv[0])))

struct MySequence : public ImSequencer::SequenceInterface {
  int GetFrameMin() const { return 0; }
  int GetFrameMax() const { return 100; }
  int GetItemCount() const { return 0; }
  virtual void Get(int index, int **start, int **end, int *type, unsigned int *color) {
  }
};

JSC_SCALL(imgui_seq,
  struct MySequence mseq = {};
  
  int selected = -1;
  int first = 0;
  int current = 100;
  bool expanded = true;
  Sequencer(&mseq, &current, &expanded, &selected, &first, ImSequencer::SEQUENCER_EDIT_STARTEND | ImSequencer::SEQUENCER_ADD | ImSequencer::SEQUENCER_DEL | ImSequencer::SEQUENCER_COPYPASTE | ImSequencer::SEQUENCER_CHANGE_FRAME);

)

JSC_SCALL(imgui_sequencer,
  int32_t current = js2number(js_getpropstr(argv[1], "current"));
  int32_t start = js2number(js_getpropstr(argv[1], "start"));
  int32_t end = js2number(js_getpropstr(argv[1], "end"));
  if(ImGui::BeginNeoSequencer(str, &current, &start, &end, {700,200},
  ImGuiNeoSequencerFlags_AllowLengthChanging |
  ImGuiNeoSequencerFlags_EnableSelection | 
  ImGuiNeoSequencerFlags_Selection_EnableDragging |
  ImGuiNeoSequencerFlags_Selection_EnableDeletion)) {
    script_call_sym(argv[2], 0, NULL);
    ImGui::EndNeoSequencer();
   }
   
  js_setpropstr(argv[1], "current", number2js(current));
  js_setpropstr(argv[1], "start", number2js(start));
  js_setpropstr(argv[1], "end", number2js(end));    
)

JSC_SCALL(imgui_timeline,
  float *k = js2newfloatarr(argv[1]);
  int n = arrlen(k);
  int32_t *keys = (int32_t*)malloc(n*sizeof(*keys));
  
  for (int i = 0; i < n; i++) {
    keys[i] = k[i];
  }
    
  arrfree(k);
  
  if (ImGui::BeginNeoTimelineEx(str)) {
    for (int i = 0; i < n; i++)
      ImGui::NeoKeyframe(keys+i);
    ImGui::EndNeoTimeLine();
  }
  
  JSValue arr = JS_NewArray(js);
  for (int i = 0; i < n; i++)
    js_setprop_num(arr, i, number2js(keys[i]));
    
  free(keys);
  ret = arr;
)

JSC_SCALL(imgui_tlgroup,
  if (ImGui::BeginNeoGroup(str)) {
    script_call_sym(argv[1], 0, NULL);
    ImGui::EndNeoGroup();
  }
)

static const JSCFunctionListEntry js_imgui_funcs[] = {
  MIST_FUNC_DEF(imgui, window, 2),
  MIST_FUNC_DEF(imgui, menu, 2),
  MIST_FUNC_DEF(imgui, sameline, 1),
  MIST_FUNC_DEF(imgui, int, 2),
  MIST_FUNC_DEF(imgui, pushid, 1),
  MIST_FUNC_DEF(imgui, popid, 0),
  MIST_FUNC_DEF(imgui, slider, 4),
  MIST_FUNC_DEF(imgui, menubar, 1),
  MIST_FUNC_DEF(imgui, mainmenubar, 1),
  MIST_FUNC_DEF(imgui, menuitem, 3),
  MIST_FUNC_DEF(imgui, radio, 2),
  MIST_FUNC_DEF(imgui, image, 1),
  MIST_FUNC_DEF(imgui, textinput, 2),
  MIST_FUNC_DEF(imgui, textbox, 2),
  MIST_FUNC_DEF(imgui, button, 2),
  MIST_FUNC_DEF(imgui, checkbox, 2),
  MIST_FUNC_DEF(imgui, text, 1),
  MIST_FUNC_DEF(imgui, plot,1),
  MIST_FUNC_DEF(imgui, lineplot,2),
  MIST_FUNC_DEF(imgui, sokol_gfx, 0),
  MIST_FUNC_DEF(imgui, columns, 1),
  MIST_FUNC_DEF(imgui, nextcolumn, 0),
  MIST_FUNC_DEF(imgui, collapsingheader, 1),
  MIST_FUNC_DEF(imgui, tree, 2),
  MIST_FUNC_DEF(imgui, listbox, 3),
  MIST_FUNC_DEF(imgui, tabbar, 2),
  MIST_FUNC_DEF(imgui, tab, 2),
  MIST_FUNC_DEF(imgui, open_popup, 1),
  MIST_FUNC_DEF(imgui, modal, 2),
  MIST_FUNC_DEF(imgui, popup, 2),
  MIST_FUNC_DEF(imgui, close_popup,0),
  MIST_FUNC_DEF(imgui, context,2),
  MIST_FUNC_DEF(imgui, table, 3),
  MIST_FUNC_DEF(imgui, tablenextcolumn,0),
  MIST_FUNC_DEF(imgui, tablenextrow,0),
  MIST_FUNC_DEF(imgui, dnd, 3),
  MIST_FUNC_DEF(imgui, dndtarget, 2),
  MIST_FUNC_DEF(imgui, color, 2),
  MIST_FUNC_DEF(imgui, startnode, 1),
  MIST_FUNC_DEF(imgui, node, 2),
  MIST_FUNC_DEF(imgui, nodein, 2),
  MIST_FUNC_DEF(imgui, nodeout, 2),
  MIST_FUNC_DEF(imgui, nodelink, 3),
  MIST_FUNC_DEF(imgui, nodemini, 1),
  MIST_FUNC_DEF(imgui, sequencer, 3),
  MIST_FUNC_DEF(imgui, timeline, 3),
  MIST_FUNC_DEF(imgui, tlgroup, 2),
  MIST_FUNC_DEF(imgui, seq, 3),
};

static int started = 0;

JSValue gui_init(JSContext *js)
{
  simgui_desc_t sdesc = {0};
  simgui_setup(&sdesc);
  
  sgimgui_desc_t desc = {0};
  sgimgui_init(&sgimgui, &desc);

  ImPlot::CreateContext();
  ImNodes::CreateContext();

  JSValue imgui = JS_NewObject(js);
  JS_SetPropertyFunctionList(js, imgui, js_imgui_funcs, countof(js_imgui_funcs));
  started = 1;
  return imgui;
}

void gui_input(sapp_event *e)
{
  if (!started) return;
  simgui_handle_event(e);
  ImGuiIO io = ImGui::GetIO();
  wantkeys = io.WantCaptureKeyboard;
  wantmouse = io.WantCaptureMouse;
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

int gui_wantmouse() { return wantmouse; }
int gui_wantkeys() { return wantkeys; }
