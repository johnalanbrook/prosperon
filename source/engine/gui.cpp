#include "gui.h"

#include "render.h"
#include "sokol/sokol_app.h"
#include "imgui.h"
#include "implot.h"
#include "imnodes.h"

#include "stb_ds.h"

#define SOKOL_IMPL
#include "sokol/util/sokol_imgui.h"
#include "sokol/util/sokol_gfx_imgui.h"
#include <stdlib.h>

static sgimgui_t sgimgui;

#include "jsffi.h"

static int wantkeys = 0;
static int wantmouse = 0;

int num_to_yaxis(int y)
{
  switch(y) {
    case 0:
      return ImAxis_Y1;
    case 1:
      return ImAxis_Y2;
    case 2:
      return ImAxis_Y3;
  }
  return ImAxis_Y1;
}

int num_to_xaxis(int x)
{
  switch(x) {
    case 0:
      return ImAxis_X1;
    case 1:
      return ImAxis_X2;
    case 2:
      return ImAxis_X3;
  }

  return ImAxis_X1;
}

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
  if (ImPlot::BeginPlot(str)) {
    script_call_sym(argv[1], 0, NULL);
    ImPlot::EndPlot();
  }
)

#define PLOT_FN(NAME, FN, ADD, SHADED) JSC_SCALL(imgui_##NAME, \
  fill_plotdata(argv[1], argv[3]); \
  bool shaded = js2boolean(argv[2]);\
  int flag = 0; \
  if (shaded) flag = SHADED; \
  ImPlot::FN(str, &plotdata[0].x, &plotdata[0].y, arrlen(plotdata), ADD flag, 0, sizeof(HMM_Vec2)); \
) \

static HMM_Vec2 *plotdata = NULL;

void fill_plotdata(JSValue v, JSValue last)
{
  arrsetlen(plotdata, 0);

  if (JS_IsArray(js,js_getpropidx(v, 0))) {
    for (int i = 0; i < js_arrlen(v); i++)
      arrput(plotdata, js2vec2(js_getpropidx(v, i)));
  }
  else {
    // Fill it with the x axis being the array index
    for (int i = 0; i < js_arrlen(v); i++) {
      if (JS_IsUndefined(js_getpropidx(v,i))) continue;
      HMM_Vec2 c = (HMM_Vec2){i, js2number(js_getpropidx(v,i))};
      arrput(plotdata, c);
    }
  }

  if (!JS_IsUndefined(last)) {
    int frame = js2number(last);
    HMM_Vec2 c = (HMM_Vec2){frame, arrlast(plotdata).y};
    arrput(plotdata, c);
  }
}

PLOT_FN(lineplot, PlotLine,,ImPlotLineFlags_Shaded)
PLOT_FN(scatterplot, PlotScatter,,0)
PLOT_FN(stairplot, PlotStairs,,ImPlotStairsFlags_Shaded)
PLOT_FN(digitalplot, PlotDigital,,0)
static HMM_Vec3 *shadedata = NULL;

JSC_SCALL(imgui_shadedplot,
  arrsetlen(plotdata,js_arrlen(argv[1]));
  for (int i = 0; i < js_arrlen(argv[1]); i++) {
    HMM_Vec3 c;
    c.x = i;
    c.y = js2number(js_getpropidx(argv[1],i));
    c.z = js2number(js_getpropidx(argv[2],i));
    arrput(shadedata, c);
  }
  ImPlot::PlotShaded(str, &shadedata[0].x, &shadedata[0].y, &shadedata[0].z, arrlen(shadedata), 0, 0, sizeof(HMM_Vec3));
)

JSC_SCALL(imgui_barplot,
  fill_plotdata(argv[1], JS_UNDEFINED);
  ImPlot::PlotBars(str, &plotdata[0].x, &plotdata[0].y, js_arrlen(argv[1]), js2number(argv[2]), 0, 0, sizeof(HMM_Vec2));
)

static double *histodata = NULL;
void fill_histodata(JSValue v)
{
  arrsetlen(histodata, js_arrlen(v));
  for (int i = 0; i < js_arrlen(v); i++)
    histodata[i] = js2number(js_getpropidx(v, i));
}
JSC_SCALL(imgui_histogramplot,
  fill_histodata(argv[1]);    
  ImPlot::PlotHistogram(str, histodata, js_arrlen(argv[1]));
)

JSC_SCALL(imgui_heatplot,
  fill_histodata(argv[1]);
  int rows = js2number(argv[2]);
  int cols = js2number(argv[3]);
  if (rows*cols == (int)js_arrlen(argv[1]))
    ImPlot::PlotHeatmap(str, histodata, rows, cols);
)

JSC_CCALL(imgui_pieplot,
  if (js_arrlen(argv[0]) != js_arrlen(argv[1])) return JS_UNDEFINED;
  
  const char *labels[js_arrlen(argv[0])];
  for (int i = 0; i < js_arrlen(argv[0]); i++)
    labels[i] = js2str(js_getpropidx(argv[0], i));

  fill_histodata(argv[1]);
  ImPlot::PlotPieChart(labels, histodata, js_arrlen(argv[1]), js2number(argv[2]), js2number(argv[3]), js2number(argv[4]));

  for (int i = 0; i < js_arrlen(argv[0]); i++)
    JS_FreeCString(js,labels[i]);
)

JSC_SCALL(imgui_textplot,
  HMM_Vec2 v = js2vec2(argv[1]);
  ImPlot::PlotText(str, v.x, v.y);
)

JSC_CCALL(imgui_inplot,
  HMM_Vec2 v = js2vec2(argv[0]);
  ImPlotRect lm = ImPlot::GetPlotLimits();
  if (v.x > lm.X.Min && v.x < lm.X.Max && v.y > lm.Y.Min && v.y < lm.Y.Max)
    return boolean2js(true);

  return boolean2js(false);
)

JSC_CCALL(imgui_plothovered,
  return boolean2js(ImPlot::IsPlotHovered());
)

JSC_SSCALL(imgui_plotaxes,
  ImPlot::SetupAxes(str,str2);
)

JSC_CCALL(imgui_plotmousepos,
  ImPlotPoint p = ImPlot::GetPlotMousePos();
  return vec22js(HMM_Vec2{p.x,p.y});
)

JSC_CCALL(imgui_axeslimits,
  ImPlot::SetupAxesLimits(js2number(argv[0]), js2number(argv[1]), js2number(argv[2]), js2number(argv[3]));
)

JSC_CCALL(imgui_fitaxis,
  ImPlot::SetNextAxisToFit((js2number(argv[0]) == 0) ? ImAxis_X1 : ImAxis_Y1);
)

JSC_SSCALL(imgui_textinput,
  char buffer[512];
  if (JS_IsUndefined(argv[1]))
    buffer[0] = 0;
  else
    strncpy(buffer, str2, 512);
    
  ImGui::InputText(str, buffer, sizeof(buffer));
  if (strcmp(buffer, str2))
    ret = str2js(buffer);
  else
    ret = JS_DupValue(js,argv[1]);
)

JSC_SSCALL(imgui_textbox,
  char buffer[512];
  if (JS_IsUndefined(argv[1]))
    buffer[0] = 0;
  else
    strncpy(buffer, str2, 512);
    
  ImGui::InputTextMultiline(str, buffer, sizeof(buffer));
  if (strcmp(buffer, str2))
    ret = str2js(buffer);
  else
    ret = JS_DupValue(js,argv[1]);
)

JSC_SCALL(imgui_text, ImGui::Text(str) )

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

JSC_SCALL(imgui_intslider,
  int low = JS_IsUndefined(argv[2]) ? 0.0 : js2number(argv[2]);
  int high = JS_IsUndefined(argv[3]) ? 1.0 : js2number(argv[3]);
  
  if (JS_IsArray(js, argv[1])) {
    int n = js_arrlen(argv[1]);
    float a[n];
    js2floatarr(argv[1], n, a);
    int b[n];
    for (int i = 0; i < n; i++)
      b[i] = a[i];
    
    switch(n) {
      case 2: 
        ImGui::SliderInt2(str, b, low, high);
        break;
      case 3:
        ImGui::SliderInt3(str, b, low, high);
        break;
      case 4:
        ImGui::SliderInt3(str, b, low, high);
        break;
    }
    
    for (int i = 0; i < n; i++)
      a[i] = b[i];
    
    ret = floatarr2js(n, a);
  } else {
    int val = js2number(argv[1]);
    ImGui::SliderInt(str, &val, low, high);
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
  if (!tex->simgui.id) {
    simgui_image_desc_t simgd;
    simgd.image = tex->id;
    simgd.sampler = std_sampler;
    tex->simgui = simgui_make_image(&simgd);
  }
  
  ImGui::Image(simgui_imtextureid(tex->simgui), ImVec2(tex->width, tex->height), ImVec2(0,0), ImVec2(1,1));
)

JSC_SCALL(imgui_imagebutton,
  texture *tex = js2texture(argv[1]);
  if (!tex->simgui.id) {
    simgui_image_desc_t simgd;
    simgd.image = tex->id;
    simgd.sampler = std_sampler;
    tex->simgui = simgui_make_image(&simgd);
  }
  
  if (ImGui::ImageButton(str, simgui_imtextureid(tex->simgui), ImVec2(tex->width, tex->height)))
    script_call_sym(argv[2], 0, NULL);
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
  int flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_SizingStretchProp;
  bool sort = false;  
  if (!JS_IsUndefined(argv[3])) sort = true;  

  if (sort) flags |= ImGuiTableFlags_Sortable;
  if (ImGui::BeginTable(str, js2number(argv[1]), flags)) {
    script_call_sym(argv[2],0,NULL);

    if (sort) {
      ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs();
      if (sort_specs && sort_specs->SpecsDirty) {
        for (int i = 0; i < sort_specs->SpecsCount; i++)
        {
          const ImGuiTableColumnSortSpecs* spec = &sort_specs->Specs[i];
          JSValue send[2];
          send[0] = number2js(spec->ColumnIndex);
          send[1] = boolean2js(spec->SortDirection == ImGuiSortDirection_Ascending);
          script_call_sym(argv[3], 2, send);
          JS_FreeValue(js, send[0]);
          JS_FreeValue(js, send[1]);
        }
        sort_specs->SpecsDirty = false;
      }
    }

    ImGui::EndTable();
  }
)

JSC_CCALL(imgui_tablenextrow, ImGui::TableNextRow())
JSC_CCALL(imgui_tablenextcolumn, ImGui::TableNextColumn())
JSC_SCALL(imgui_tablesetupcolumn, ImGui::TableSetupColumn(str))
JSC_CCALL(imgui_tableheadersrow, ImGui::TableHeadersRow())

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

ImVec2 js2imvec2(JSValue v)
{
  HMM_Vec2 va = js2vec2(v);
  return ImVec2(va.x,va.y);
}

JSValue imvec22js(ImVec2 v)
{
  return vec22js(HMM_Vec2({v.x,v.y}));
}

ImPlotPoint js2plotvec2(JSValue v)
{
  HMM_Vec2 va = js2vec2(v);
  return ImPlotPoint(va.x,va.y);
}

JSValue plotvec22js(ImPlotPoint v)
{
  return vec22js(HMM_Vec2{v.x,v.y});
}

ImVec4 js2imvec4(JSValue v)
{
  HMM_Vec4 va = js2vec4(v);
  return ImVec4(va.x, va.y, va.z, va.w);
}

ImU32 js2imu32(JSValue v)
{
  return ImGui::ColorConvertFloat4ToU32(js2imvec4(v));
}

JSC_CCALL(imgui_rectfilled,
  ImGui::GetWindowDrawList()->AddRectFilled(js2imvec2(argv[0]), js2imvec2(argv[1]), js2imu32(argv[2]));
)

JSC_CCALL(imgui_line,
  ImGui::GetWindowDrawList()->AddLine(js2imvec2(argv[0]), js2imvec2(argv[1]),js2imu32(argv[2]));
)

JSC_CCALL(imgui_point,
  ImGui::GetWindowDrawList()->AddCircleFilled(js2imvec2(argv[0]), js2number(argv[1]), js2imu32(argv[2]));
)

JSC_CCALL(imgui_cursorscreenpos,
  ImVec2 v = ImGui::GetCursorScreenPos();
  HMM_Vec2 va;
  va.x = v.x;
  va.y = v.y;
  return vec22js(va);
)

JSC_CCALL(imgui_setcursorscreenpos,
  ImGui::SetCursorScreenPos(js2imvec2(argv[0]));
)

JSC_CCALL(imgui_contentregionavail,
  ImVec2 v = ImGui::GetContentRegionAvail();
  HMM_Vec2 va;
  va.x = v.x;
  va.y = v.y;
  return vec22js(va);
)

JSC_CCALL(imgui_beziercubic,
  ImGui::GetWindowDrawList()->AddBezierCubic(js2imvec2(argv[0]), js2imvec2(argv[1]), js2imvec2(argv[2]), js2imvec2(argv[3]), js2imu32(argv[4]), js2number(argv[5]));
)

JSC_CCALL(imgui_bezierquad,
  ImGui::GetWindowDrawList()->AddBezierQuadratic(js2imvec2(argv[0]), js2imvec2(argv[1]), js2imvec2(argv[2]), js2imu32(argv[3]), js2number(argv[4]));
)

JSC_SCALL(imgui_drawtext,
  ImGui::GetWindowDrawList()->AddText(js2imvec2(argv[1]), js2imu32(argv[2]), str);
)

JSC_CCALL(imgui_rect,
  ImGui::GetWindowDrawList()->AddRect(js2imvec2(argv[0]), js2imvec2(argv[1]), js2imu32(argv[2]));
)

JSC_CCALL(imgui_mousehoveringrect,
  return boolean2js(ImGui::IsMouseHoveringRect(js2imvec2(argv[0]), js2imvec2(argv[1])));
)

JSC_CCALL(imgui_mouseclicked,
  return boolean2js(ImGui::IsMouseClicked(js2number(argv[0])));
)

JSC_CCALL(imgui_mousedown,
  return boolean2js(ImGui::IsMouseDown(js2number(argv[0])));
)

JSC_CCALL(imgui_mousereleased,
  return boolean2js(ImGui::IsMouseReleased(js2number(argv[0])));
)

JSC_CCALL(imgui_mousedragging,
  return boolean2js(ImGui::IsMouseDragging(js2number(argv[0])));
)

JSC_CCALL(imgui_mousedelta,
  ImVec2 dm = ImGui::GetIO().MouseDelta;
  return vec22js((HMM_Vec2){dm.x,dm.y});
)

JSC_CCALL(imgui_dummy,
  ImGui::Dummy(js2imvec2(argv[0]));
)

JSC_SCALL(imgui_invisiblebutton,
  ImGui::InvisibleButton(str, js2imvec2(argv[1]));
)

JSC_CCALL(imgui_width,
  ImGui::PushItemWidth(js2number(argv[0]));
)

JSC_CCALL(imgui_windowpos,
  return imvec22js(ImGui::GetWindowPos());
)

JSC_CCALL(imgui_plotpos,
  return plotvec22js(ImPlot::GetPlotPos());
)

JSC_CCALL(imgui_plot2pixels,
  return imvec22js(ImPlot::PlotToPixels(js2plotvec2(argv[0])));
)

JSC_CCALL(imgui_plotlimits,
  ImPlotRect lim = ImPlot::GetPlotLimits();
  JSValue xlim = JS_NewObject(js);
  js_setpropstr(xlim, "min", number2js(lim.X.Min));
  js_setpropstr(xlim, "max", number2js(lim.X.Max));
  JSValue ylim = JS_NewObject(js);
  js_setpropstr(ylim, "min", number2js(lim.Y.Min));
  js_setpropstr(ylim, "max", number2js(lim.Y.Max));
  
  JSValue limits = JS_NewObject(js);
  js_setpropstr(limits, "x", xlim);
  js_setpropstr(limits, "y", ylim);
  
  return limits;
)

static JSValue axis_formatter = JS_UNDEFINED;

static JSValue axis_fmts[10];

void jsformatter(double value, char *buff, int size, JSValue *fmt)
{
  JSValue v = number2js(value);
  const char *str = js2str(script_call_sym_ret(*fmt, 1, &v));
  strncpy(buff,str, size);
  JS_FreeCString(js, str);
}

JSC_CCALL(imgui_axisfmt,
  int y = num_to_yaxis(js2number(argv[0]));
  
  if (!JS_IsUndefined(axis_fmts[y])) {
    JS_FreeValue(js, axis_fmts[y]);
    axis_fmts[y] = JS_UNDEFINED;
  }
  
  axis_fmts[y] = JS_DupValue(js,argv[1]);
  
  ImPlot::SetupAxisFormat(y, (ImPlotFormatter)jsformatter, (void*)(axis_fmts+y));
)

#define FSTAT(KEY) js_setpropstr(v, #KEY, number2js(stats.KEY));
JSC_CCALL(imgui_framestats,
  JSValue v = JS_NewObject(js);
  sg_frame_stats stats = sg_query_frame_stats();
  FSTAT(num_passes)
  FSTAT(num_apply_viewport)
  FSTAT(num_apply_scissor_rect)
  FSTAT(num_apply_pipeline)
  FSTAT(num_apply_bindings)
  FSTAT(num_apply_uniforms)
  FSTAT(num_draw)
  FSTAT(num_update_buffer)
  FSTAT(num_append_buffer)
  FSTAT(num_update_image)
  FSTAT(size_apply_uniforms)
  FSTAT(size_update_buffer)
  FSTAT(size_append_buffer)
  FSTAT(size_update_image)
  return v;
)

JSC_CCALL(imgui_setaxes,
  int x = num_to_xaxis(js2number(argv[0]));
  int y = num_to_yaxis(js2number(argv[1]));
  ImPlot::SetAxes(x,y);
)

JSC_CCALL(imgui_setupaxis,
  ImPlot::SetupAxis(num_to_yaxis(js2number(argv[0])));
)

JSC_SCALL(imgui_setclipboard,
  ImGui::SetClipboardText(str);
)

static const JSCFunctionListEntry js_imgui_funcs[] = {
  MIST_FUNC_DEF(imgui, windowpos, 0),
  MIST_FUNC_DEF(imgui, plot2pixels, 1),
  MIST_FUNC_DEF(imgui, plotpos, 0),
  MIST_FUNC_DEF(imgui, plotlimits, 0),
  MIST_FUNC_DEF(imgui, setaxes, 2),
  MIST_FUNC_DEF(imgui, setupaxis, 1),
  MIST_FUNC_DEF(imgui, framestats, 0),
  MIST_FUNC_DEF(imgui, inplot, 1),
  MIST_FUNC_DEF(imgui, window, 2),
  MIST_FUNC_DEF(imgui, menu, 2),
  MIST_FUNC_DEF(imgui, sameline, 1),
  MIST_FUNC_DEF(imgui, int, 2),
  MIST_FUNC_DEF(imgui, pushid, 1),
  MIST_FUNC_DEF(imgui, popid, 0),
  MIST_FUNC_DEF(imgui, slider, 4),
  MIST_FUNC_DEF(imgui, intslider, 4),
  MIST_FUNC_DEF(imgui, menubar, 1),
  MIST_FUNC_DEF(imgui, mainmenubar, 1),
  MIST_FUNC_DEF(imgui, menuitem, 3),
  MIST_FUNC_DEF(imgui, radio, 2),
  MIST_FUNC_DEF(imgui, image, 1),
  MIST_FUNC_DEF(imgui, imagebutton, 2),
  MIST_FUNC_DEF(imgui, textinput, 2),
  MIST_FUNC_DEF(imgui, textbox, 2),
  MIST_FUNC_DEF(imgui, button, 2),
  MIST_FUNC_DEF(imgui, checkbox, 2),
  MIST_FUNC_DEF(imgui, text, 1),
  MIST_FUNC_DEF(imgui, plot, 1),
  MIST_FUNC_DEF(imgui, lineplot, 4),
  MIST_FUNC_DEF(imgui, scatterplot, 4),
  MIST_FUNC_DEF(imgui, stairplot, 4),
  MIST_FUNC_DEF(imgui, digitalplot, 4),
  MIST_FUNC_DEF(imgui, shadedplot, 4),  
  MIST_FUNC_DEF(imgui, barplot, 3),
  MIST_FUNC_DEF(imgui, pieplot, 5),
  MIST_FUNC_DEF(imgui, textplot, 2),
  MIST_FUNC_DEF(imgui, histogramplot, 2),
  MIST_FUNC_DEF(imgui, plotaxes, 2),
  MIST_FUNC_DEF(imgui, plotmousepos, 0),
  MIST_FUNC_DEF(imgui, plothovered, 0),
  MIST_FUNC_DEF(imgui, axeslimits, 4),
  MIST_FUNC_DEF(imgui, fitaxis, 1),
  MIST_FUNC_DEF(imgui, sokol_gfx, 0),
  MIST_FUNC_DEF(imgui, columns, 1),
  MIST_FUNC_DEF(imgui, nextcolumn, 0),
  MIST_FUNC_DEF(imgui, collapsingheader, 1),
  MIST_FUNC_DEF(imgui, tree, 2),
  MIST_FUNC_DEF(imgui, listbox, 3),
  MIST_FUNC_DEF(imgui, axisfmt, 2),
  MIST_FUNC_DEF(imgui, tabbar, 2),
  MIST_FUNC_DEF(imgui, tab, 2),
  MIST_FUNC_DEF(imgui, open_popup, 1),
  MIST_FUNC_DEF(imgui, modal, 2),
  MIST_FUNC_DEF(imgui, popup, 2),
  MIST_FUNC_DEF(imgui, close_popup,0),
  MIST_FUNC_DEF(imgui, context,2),
  MIST_FUNC_DEF(imgui, table, 4),
  MIST_FUNC_DEF(imgui, tablenextcolumn,0),
  MIST_FUNC_DEF(imgui, tablenextrow,0),
  MIST_FUNC_DEF(imgui, tableheadersrow, 0),
  MIST_FUNC_DEF(imgui, tablesetupcolumn, 1),
  MIST_FUNC_DEF(imgui, dnd, 3),
  MIST_FUNC_DEF(imgui, dndtarget, 2),
  MIST_FUNC_DEF(imgui, color, 2),
  MIST_FUNC_DEF(imgui, startnode, 1),
  MIST_FUNC_DEF(imgui, node, 2),
  MIST_FUNC_DEF(imgui, nodein, 2),
  MIST_FUNC_DEF(imgui, nodeout, 2),
  MIST_FUNC_DEF(imgui, nodelink, 3),
  MIST_FUNC_DEF(imgui, nodemini, 1),
  MIST_FUNC_DEF(imgui, mousehoveringrect, 2),
  MIST_FUNC_DEF(imgui, mouseclicked, 1),
  MIST_FUNC_DEF(imgui, mousedown, 1),
  MIST_FUNC_DEF(imgui, mousereleased, 1),
  MIST_FUNC_DEF(imgui, mousedragging, 1),
  MIST_FUNC_DEF(imgui, mousedelta, 0),
  MIST_FUNC_DEF(imgui, rect, 3),
  MIST_FUNC_DEF(imgui, rectfilled, 3),
  MIST_FUNC_DEF(imgui, line, 3),
  MIST_FUNC_DEF(imgui, bezierquad, 5),
  MIST_FUNC_DEF(imgui, beziercubic, 6),
  MIST_FUNC_DEF(imgui, point, 3),
  MIST_FUNC_DEF(imgui, drawtext, 3),
  MIST_FUNC_DEF(imgui, cursorscreenpos, 0),
  MIST_FUNC_DEF(imgui, setcursorscreenpos, 1),
  MIST_FUNC_DEF(imgui, contentregionavail, 0),
  MIST_FUNC_DEF(imgui, dummy, 1),
  MIST_FUNC_DEF(imgui, invisiblebutton, 2),
  MIST_FUNC_DEF(imgui, width, 1),
  MIST_FUNC_DEF(imgui, setclipboard, 1),
};

static int started = 0;

JSValue gui_init(JSContext *js)
{
  simgui_desc_t sdesc = {};
  simgui_setup(&sdesc);

  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = ".prosperon/imgui.ini";
  ImGui::LoadIniSettingsFromDisk(".prosperon/imgui.ini");
  
  sgimgui_desc_t desc = {0};
  sgimgui_init(&sgimgui, &desc);

  sgimgui.frame_stats_window.disable_sokol_imgui_stats = true;

  ImPlot::CreateContext();
  ImNodes::CreateContext();

  JSValue imgui = JS_NewObject(js);
  JS_SetPropertyFunctionList(js, imgui, js_imgui_funcs, countof(js_imgui_funcs));
  started = 1;

  sg_enable_frame_stats();

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
