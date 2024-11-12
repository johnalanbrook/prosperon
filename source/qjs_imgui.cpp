#include "imgui.h"
#include "implot.h"
#include "imnodes.h"
#include "quickjs.h"

#include "sokol_app.h"
#include "sokol_gfx.h"

#include "render.h"
#define SOKOL_IMPL
#include "util/sokol_imgui.h"
#include "util/sokol_gfx_imgui.h"

static sgimgui_t sgimgui;

#define JSC_CCALL(NAME, ...) static JSValue js_##NAME (JSContext *js, JSValue self, int argc, JSValue *argv) { \
  JSValue ret = JS_UNDEFINED; \
  __VA_ARGS__  ;\
  return ret; \
} \

#define JSC_SCALL(NAME, ...) static JSValue js_##NAME (JSContext *js, JSValue self, int argc, JSValue *argv) { \
  JSValue ret = JS_UNDEFINED; \
  const char *str = JS_ToCString(js, argv[0]); \
  __VA_ARGS__  ;\
  JS_FreeCString(js, str); \
  return ret; \
} \

#define JSC_SSALL(NAME, ...) static JSValue js_##NAME (JSContext *js, JSValue self, int argc, JSValue *argv) { \
  JSValue ret = JS_UNDEFINED; \
  const char *str = JS_ToCString(js, argv[0]); \
  const char *str2 = JS_ToCString(js, argv[1]); \
  __VA_ARGS__  ; \
  JS_FreeCString(js, str); \
  JS_FreeCString(js, str2); \
  return ret; \
} \

static inline int js_arrlen(JSContext *js, JSValue v)
{
  JSValue len = JS_GetPropertyStr(js, v, "length");
  int intlen;
  JS_ToInt32(js, &intlen, len);
  JS_FreeValue(js, len);
  return intlen;
}

static inline double js2number(JSContext *js, JSValue v)
{
  double n;
  JS_ToFloat64(js, &n, v);
  return n;
}

static inline ImVec2 js2vec2(JSContext *js, JSValue v)
{
  ImVec2 c;
  double dx, dy;
  JSValue x = JS_GetPropertyUint32(js, v, 0);
  JSValue y = JS_GetPropertyUint32(js, v, 1);
  JS_ToFloat64(js, &dx, x);
  JS_ToFloat64(js, &dy, y);
  JS_FreeValue(js, x);
  JS_FreeValue(js, y);
  c.x = dx;
  c.y = dy;
  return c;
}

static inline ImVec4 js2vec4(JSContext *js, JSValue v)
{
  ImVec4 c;
  double dx, dy, dz, dw;
  JSValue x = JS_GetPropertyUint32(js, v, 0);
  JSValue y = JS_GetPropertyUint32(js, v, 1);
  JSValue z = JS_GetPropertyUint32(js, v, 2);
  JSValue w = JS_GetPropertyUint32(js, v, 3);  
  JS_ToFloat64(js, &dx, x);
  JS_ToFloat64(js, &dy, y);
  JS_ToFloat64(js, &dz, z);
  JS_ToFloat64(js, &dw, w);  
  JS_FreeValue(js, x);
  JS_FreeValue(js, y);
  JS_FreeValue(js, z);
  JS_FreeValue(js, w);  
  c.x = dx;
  c.y = dy;
  c.z = dz;
  c.w = dw;
  return c;
}

static inline JSValue vec22js(JSContext *js, ImVec2 vec)
{
  JSValue v = JS_NewObject(js);
  JS_SetPropertyUint32(js, v, 0, JS_NewFloat64(js, vec.x));
  JS_SetPropertyUint32(js, v, 1, JS_NewFloat64(js, vec.y));  
  return v;
}

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

JSC_CCALL(imgui_window,
  const char *str = JS_ToCString(js,argv[0]);
  bool active = true;
  ImGui::Begin(str, &active);
  JS_Call(js, argv[1], JS_UNDEFINED, 0, NULL);
  ImGui::End();
  JS_FreeCString(js,str);
  return JS_NewBool(js,active);
)
JSC_SCALL(imgui_menu,
  if (ImGui::BeginMenu(str)) {
    JS_Call(js, argv[1], JS_UNDEFINED, 0, NULL);
    ImGui::EndMenu();
  }
)

JSC_CCALL(imgui_menubar,
  if (ImGui::BeginMenuBar()) {
    JS_Call(js, argv[0], JS_UNDEFINED, 0, NULL);  
    ImGui::EndMenuBar();
  }
)

JSC_CCALL(imgui_mainmenubar,
  if (ImGui::BeginMainMenuBar()) {
    JS_Call(js, argv[0], JS_UNDEFINED, 0, NULL);
    ImGui::EndMainMenuBar();
  }
)

JSC_CCALL(imgui_menuitem,
  const char *name = JS_ToCString(js,argv[0]);
  const char *keyfn = JS_IsUndefined(argv[1]) ? NULL : JS_ToCString(js,argv[1]);
  bool on = JS_IsUndefined(argv[3]) ? false : JS_ToBool(js,argv[3]);
  if (ImGui::MenuItem(JS_ToBool(js,argv[0]) ? name : "##empty" ,keyfn, &on))
    JS_Call(js, argv[2], JS_UNDEFINED, 0, NULL);

  if (!JS_IsNull(argv[0])) JS_FreeCString(js,name);
  if (keyfn) JS_FreeCString(js,keyfn);

  return JS_NewBool(js,on);
)

JSC_SCALL(imgui_plot,
  if (ImPlot::BeginPlot(str)) {
    JS_Call(js, argv[1], JS_UNDEFINED, 0, NULL);
    ImPlot::EndPlot();
  }
)

#define PLOT_FN(NAME, FN, ADD, SHADED) JSC_SCALL(imgui_##NAME, \
  fill_plotdata(js, argv[1], argv[3]); \
  bool shaded = JS_ToBool(js,argv[2]);\
  int flag = 0; \
) \

//if (shaded) flag = SHADED;
//  ImPlot::FN(str, &plotdata[0].x, &plotdata[0].y, arrlen(plotdata), ADD flag, 0, sizeof(ImVec2));

static ImVec2 *plotdata = NULL;

void fill_plotdata(JSContext *js, JSValue v, JSValue last)
{
/*  arrsetlen(plotdata, 0);

  if (JS_IsArray(js,js_getpropidx(v, 0))) {
    for (int i = 0; i < js_arrlen(js, v); i++)
      arrput(plotdata, js2vec2(js, js_getpropidx(v, i)));
  }
  else {
    // Fill it with the x axis being the array index
    for (int i = 0; i < js_arrlen(js, v); i++) {
      if (JS_IsUndefined(js_getpropidx(v,i))) continue;
      ImVec2 c;
      c.x = i;
      c.y = js2number(js, js_getpropidx(v,i));
      arrput(plotdata, c);
    }
  }

  if (!JS_IsUndefined(last)) {
    int frame = js2number(js, last);
    ImVec2 c = js2vec2(js,last);
    c.y = arrlast(plotdata).y;
    arrput(plotdata, c);
  }*/
}

PLOT_FN(lineplot, PlotLine,,ImPlotLineFlags_Shaded)
PLOT_FN(scatterplot, PlotScatter,,0)
PLOT_FN(stairplot, PlotStairs,,ImPlotStairsFlags_Shaded)
PLOT_FN(digitalplot, PlotDigital,,0)

JSC_SCALL(imgui_barplot,
  fill_plotdata(js, argv[1], JS_UNDEFINED);
//  ImPlot::PlotBars(str, &plotdata[0].x, &plotdata[0].y, js_arrlen(js, argv[1]), js2number(js, argv[2]), 0, 0, sizeof(ImVec2));
)

JSC_SCALL(imgui_histogramplot,
  size_t offset, len, per_e;
  JSValue typed = JS_GetTypedArrayBuffer(js, argv[1], &offset, &len, &per_e);
//  ImPlot::PlotHistogram(str, JS_GetArrayBuffer(js, NULL, typed), js_arrlen(js, argv[1]));
  JS_FreeValue(js, typed);
)

JSC_SCALL(imgui_heatplot,
  int rows = js2number(js, argv[2]);
  int cols = js2number(js, argv[3]);
//  if (rows*cols == (int)js_arrlen(js, argv[1]))
//    ImPlot::PlotHeatmap(str, histodata, rows, cols);
)

JSC_CCALL(imgui_pieplot,
/*  if (js_arrlen(js, argv[0]) != js_arrlen(js, argv[1])) return JS_UNDEFINED;
  
  const char *labels[js_arrlen(js, argv[0])];
  for (int i = 0; i < js_arrlen(js, argv[0]); i++)
    labels[i] = JS_ToCString(js, js_getpropidx(argv[0], i));

  fill_histodata(argv[1]);
  ImPlot::PlotPieChart(labels, histodata, js_arrlen(js, argv[1]), js2number(js, argv[2]), js2number(js, argv[3]), js2number(js, argv[4]));

  for (int i = 0; i < js_arrlen(js, argv[0]); i++)
    JS_FreeCString(js,labels[i]);*/
)

JSC_SCALL(imgui_textplot,
  ImVec2 c = js2vec2(js, argv[1]);
//  ImPlot::PlotText(str, c.x, c.y);
)

JSC_CCALL(imgui_inplot,
  ImVec2 v = js2vec2(js, argv[0]);
  ImPlotRect lm = ImPlot::GetPlotLimits();
  if (v.x > lm.X.Min && v.x < lm.X.Max && v.y > lm.Y.Min && v.y < lm.Y.Max)
    return JS_NewBool(js,true);

  return JS_NewBool(js,false);
)

JSC_CCALL(imgui_plothovered,
  return JS_NewBool(js,ImPlot::IsPlotHovered());
)

JSC_SSALL(imgui_plotaxes,
  ImPlot::SetupAxes(str,str2);
)

JSC_CCALL(imgui_plotmousepos,
  ImPlotPoint p = ImPlot::GetPlotMousePos();
  return vec22js(js, ImVec2{p.x,p.y});
)

JSC_CCALL(imgui_axeslimits,
  ImPlot::SetupAxesLimits(js2number(js, argv[0]), js2number(js, argv[1]), js2number(js, argv[2]), js2number(js, argv[3]));
)

JSC_CCALL(imgui_fitaxis,
  ImPlot::SetNextAxisToFit((js2number(js, argv[0]) == 0) ? ImAxis_X1 : ImAxis_Y1);
)

JSC_SSALL(imgui_textinput,
  char buffer[512];
  if (JS_IsUndefined(argv[1]))
    buffer[0] = 0;
  else
    strncpy(buffer, str2, 512);
    
  ImGui::InputText(str, buffer, sizeof(buffer));
  if (strcmp(buffer, str2))
    ret = JS_NewString(js,buffer);
  else
    ret = JS_DupValue(js,argv[1]);
)

JSC_SSALL(imgui_textbox,
  char buffer[512];
  if (JS_IsUndefined(argv[1]))
    buffer[0] = 0;
  else
    strncpy(buffer, str2, 512);
    
  ImGui::InputTextMultiline(str, buffer, sizeof(buffer));
  if (strcmp(buffer, str2))
    ret = JS_NewString(js,buffer);
  else
    ret = JS_DupValue(js,argv[1]);
)

JSC_SCALL(imgui_text, ImGui::Text(str) )

JSC_SCALL(imgui_button,
  if (ImGui::Button(str))
    JS_Call(js, argv[1], JS_UNDEFINED, 0, NULL);
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
/*  float low = JS_IsUndefined(argv[2]) ? 0.0 : js2number(js, argv[2]);
  float high = JS_IsUndefined(argv[3]) ? 1.0 : js2number(js, argv[3]);
  
  if (JS_IsArray(js, argv[1])) {
    int n = js_arrlen(js, argv[1]);
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
    float val = js2number(js, argv[1]);
    ImGui::SliderFloat(str, &val, low, high, "%.3f");
    ret = number2js(val);
  }*/
)

JSC_SCALL(imgui_intslider,
  int low = JS_IsUndefined(argv[2]) ? 0.0 : js2number(js, argv[2]);
  int high = JS_IsUndefined(argv[3]) ? 1.0 : js2number(js, argv[3]);
  
  JSValue arr = JS_NewTypedArray(js, 1, &argv[1], JS_TYPED_ARRAY_INT32);
  size_t len;
  JSValue buf = JS_GetTypedArrayBuffer(js, arr, NULL, &len, NULL);
  int *data = (int*)JS_GetArrayBuffer(js, NULL, buf);
    
  switch(len) {
    case 2: 
      ImGui::SliderInt2(str, data, low, high);
      break;
    case 3:
      ImGui::SliderInt3(str, data, low, high);
      break;
    case 4:
      ImGui::SliderInt3(str, data, low, high);
      break;
  }
//  ret = floatarr2js(n, a);
//  } else {
/*    int val = js2number(js, argv[1]);
    ImGui::SliderInt(str, &val, low, high);
    ret = JS_NewInt32(js,val);*/
//  }
)

JSC_SCALL(imgui_checkbox,
  bool val = JS_ToBool(js, argv[1]);
  ImGui::Checkbox(str, &val);
  ret = JS_NewBool(js,val);
)

JSC_CCALL(imgui_pushid,
  ImGui::PushID(js2number(js, argv[0]));
)

JSC_CCALL(imgui_popid, ImGui::PopID(); )

JSC_CCALL(imgui_image,
/*  texture *tex = js2texture(argv[0]);
  simgui_image_desc_t sg = {};
  sg.image = tex->id;
  sg.sampler = std_sampler;
  simgui_image_t ss = simgui_make_image(&sg);
  
  ImGui::Image(simgui_imtextureid(ss), ImVec2(tex->width, tex->height), ImVec2(0,0), ImVec2(1,1));
  
  simgui_destroy_image(ss);
*/
)

JSC_CCALL(imgui_imagebutton,
/*  texture *tex = js2texture(argv[1]);
  simgui_image_desc_t sg = {};
  sg.image = tex->id;
  sg.sampler = std_sampler;
  simgui_image_t ss = simgui_make_image(&sg);
  
  
  if (ImGui::ImageButton(str, simgui_imtextureid(ss), ImVec2(tex->width, tex->height)))
    JS_Call(js, argv[2], JS_UNDEFINED, 0, NULL);
    
  simgui_destroy_image(ss);*/
)

JSC_CCALL(imgui_sameline, ImGui::SameLine(js2number(js, argv[0])) )
JSC_CCALL(imgui_columns, ImGui::Columns(js2number(js, argv[0])) )
JSC_CCALL(imgui_nextcolumn, ImGui::NextColumn() )
JSC_SCALL(imgui_collapsingheader, ret = JS_NewBool(js,ImGui::CollapsingHeader(str)) )
JSC_SCALL(imgui_radio, ret = JS_NewBool(js,ImGui::RadioButton(str, JS_ToBool(js, argv[1]))))

JSC_SCALL(imgui_tree,
  if (ImGui::TreeNode(str)) {
    JS_Call(js, argv[1],JS_UNDEFINED, 0,NULL);
    ImGui::TreePop();
  }
)

JSC_SCALL(imgui_tabbar,
  if (ImGui::BeginTabBar(str)) {
    JS_Call(js, argv[1],JS_UNDEFINED, 0,NULL);  
    ImGui::EndTabBar();
  }
)

JSC_SCALL(imgui_tab,
  if (ImGui::BeginTabItem(str)) {
    JS_Call(js, argv[1],JS_UNDEFINED, 0,NULL);    
    ImGui::EndTabItem();
  }
)

JSC_SCALL(imgui_listbox,
/*  char **arr = js2strarr(argv[1]);
  int n = js_arrlen(js, argv[1]);
  int idx = js2number(js, argv[2]);
  ImGui::ListBox(str, &idx, arr, n, 4);
  for (int i = 0; i < n; i++)
    free(arr[i]);

//  arrfree(arr); // TODO: Doesn't this need freed?
  ret = JS_NewInt32(js,idx);*/
)

JSC_SCALL(imgui_int,
  int n = js2number(js, argv[1]);
  ImGui::InputInt(str, &n);
  ret = JS_NewInt32(js, n);
)

JSC_SCALL(imgui_open_popup,
  ImGui::OpenPopup(str);
)

JSC_SCALL(imgui_popup,
  if (ImGui::BeginPopup(str)) {
    JS_Call(js, argv[1],JS_UNDEFINED, 0,NULL);      
    ImGui::EndPopup();
  }
)

JSC_CCALL(imgui_close_popup,
  ImGui::CloseCurrentPopup();
)

JSC_SCALL(imgui_modal,
  if (ImGui::BeginPopupModal(str)) {
    JS_Call(js, argv[1],JS_UNDEFINED, 0,NULL);      
    ImGui::EndPopup();
  }
)

JSC_SCALL(imgui_context,
  if (ImGui::BeginPopupContextItem(str)) {
    JS_Call(js, argv[1],JS_UNDEFINED, 0,NULL);      
    ImGui::EndPopup();
  }
)

JSC_SCALL(imgui_table,
  int flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_SizingStretchProp;
  bool sort = false;  
  if (!JS_IsUndefined(argv[3])) sort = true;  

  if (sort) flags |= ImGuiTableFlags_Sortable;
  if (ImGui::BeginTable(str, js2number(js, argv[1]), flags)) {
    JS_Call(js, argv[2], JS_UNDEFINED, 0, NULL);

    if (sort) {
      ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs();
      if (sort_specs && sort_specs->SpecsDirty) {
        for (int i = 0; i < sort_specs->SpecsCount; i++)
        {
          const ImGuiTableColumnSortSpecs* spec = &sort_specs->Specs[i];
          JSValue send[2];
          send[0] = JS_NewInt32(js,spec->ColumnIndex);
          send[1] = JS_NewBool(js,spec->SortDirection == ImGuiSortDirection_Ascending);
          JS_Call(js, argv[3], JS_UNDEFINED, 2, send);          
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
JSC_CCALL(imgui_tableangledheadersrow, ImGui::TableAngledHeadersRow())

JSC_SCALL(imgui_dnd, 
  if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
    double n = js2number(js, argv[1]);
    ImGui::SetDragDropPayload(str, &n, sizeof(n));
//    JS_Call(js, argv[2], JS_UNDEFINED, 2, send);              
    ImGui::EndDragDropSource();
  }
)

JSC_SCALL(imgui_dndtarget,
  if (ImGui::BeginDragDropTarget()) {
    if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(str)) {
      JSValue n = JS_NewFloat64(js,*(double*)payload->Data);
      JS_Call(js, argv[1], JS_UNDEFINED, 1, &n);                    
    }
    ImGui::EndDragDropTarget();
  }
)

JSC_SCALL(imgui_color,
/*  int n = js_arrlen(js, argv[1]);
  float color[n];
  js2floatarr(argv[1],n,color);

  if (n == 3)
    ImGui::ColorEdit3(str, color);
  else if (n == 4)
    ImGui::ColorEdit4(str, color);

  ret = floatarr2js(n, color);*/
)

JSC_CCALL(imgui_startnode,
  ImNodes::BeginNodeEditor();
  JS_Call(js, argv[0], JS_UNDEFINED, 0,NULL);                     
  ImNodes::EndNodeEditor();
  int start_attr;
  int end_attr;
  if (ImNodes::IsLinkCreated(&start_attr, &end_attr))
  {
    JSValue ab[2];
    ab[0] = JS_NewInt32(js,start_attr);
    ab[1] = JS_NewInt32(js,end_attr);
    JS_Call(js,argv[1], JS_UNDEFINED, 2, ab);
    
    for (int i = 0; i < 2; i++) JS_FreeValue(js, ab[i]);
  }

  int node_id;
  if (ImNodes::IsNodeHovered(&node_id))
  {
    JSValue a = JS_NewInt32(js,node_id);
    JS_Call(js, argv[2], JS_UNDEFINED, 1,&a);                         
    JS_FreeValue(js,a);
  }

  int link_id;
  if (ImNodes::IsLinkHovered(&link_id))
  {
    JSValue a = JS_NewInt32(js,link_id);
    JS_Call(js, argv[3], JS_UNDEFINED, 1,&a);                             
    JS_FreeValue(js,a);
  }
)

JSC_CCALL(imgui_node,
  ImNodes::BeginNode(js2number(js, argv[0]));
  JS_Call(js, argv[1], JS_UNDEFINED, 0,NULL);                               
  ImNodes::EndNode();
)

JSC_CCALL(imgui_nodein,
  ImNodes::BeginInputAttribute(js2number(js, argv[0]));
  JS_Call(js, argv[1], JS_UNDEFINED, 0,NULL);                                 
  ImNodes::EndInputAttribute();
)

JSC_CCALL(imgui_nodeout,
  ImNodes::BeginOutputAttribute(js2number(js, argv[0]));
  JS_Call(js, argv[1], JS_UNDEFINED, 0,NULL);                                 
  ImNodes::EndOutputAttribute();
)

JSC_CCALL(imgui_nodelink,
  ImNodes::Link(js2number(js, argv[0]), js2number(js, argv[1]), js2number(js, argv[2]));
)

JSC_CCALL(imgui_nodemini, ImNodes::MiniMap(js2number(js, argv[0])))

ImPlotPoint js2plotvec2(JSContext *js, JSValue v)
{
  ImVec2 c = js2vec2(js, v);
  return ImPlotPoint(c);
}

JSValue plotvec22js(JSContext *js, ImPlotPoint v)
{
  return vec22js(js, ImVec2{v.x,v.y});
}

ImU32 js2imu32(JSContext *js, JSValue v)
{
  return ImGui::ColorConvertFloat4ToU32(js2vec4(js, v));
}

JSC_CCALL(imgui_rectfilled,
  ImGui::GetWindowDrawList()->AddRectFilled(js2vec2(js,argv[0]), js2vec2(js,argv[1]), js2imu32(js,argv[2]));
)

JSC_CCALL(imgui_line,
  ImGui::GetWindowDrawList()->AddLine(js2vec2(js,argv[0]), js2vec2(js,argv[1]),js2imu32(js, argv[2]));
)

JSC_CCALL(imgui_point,
  ImGui::GetWindowDrawList()->AddCircleFilled(js2vec2(js,argv[0]), js2number(js, argv[1]), js2imu32(js, argv[2]));
)

JSC_CCALL(imgui_cursorscreenpos,
  ImVec2 v = ImGui::GetCursorScreenPos();
  return vec22js(js, v);
)

JSC_CCALL(imgui_setcursorscreenpos,
  ImGui::SetCursorScreenPos(js2vec2(js,argv[0]));
)

JSC_CCALL(imgui_contentregionavail,
  return vec22js(js, ImGui::GetContentRegionAvail());
)

JSC_CCALL(imgui_beziercubic,
  ImGui::GetWindowDrawList()->AddBezierCubic(js2vec2(js,argv[0]), js2vec2(js,argv[1]), js2vec2(js,argv[2]), js2vec2(js,argv[3]), js2imu32(js, argv[4]), js2number(js, argv[5]));
)

JSC_CCALL(imgui_bezierquad,
  ImGui::GetWindowDrawList()->AddBezierQuadratic(js2vec2(js,argv[0]), js2vec2(js,argv[1]), js2vec2(js,argv[2]), js2imu32(js, argv[3]), js2number(js, argv[4]));
)

JSC_SCALL(imgui_drawtext,
  ImGui::GetWindowDrawList()->AddText(js2vec2(js,argv[1]), js2imu32(js,argv[2]), str);
)

JSC_CCALL(imgui_rect,
  ImGui::GetWindowDrawList()->AddRect(js2vec2(js,argv[0]), js2vec2(js,argv[1]), js2imu32(js,argv[2]));
)

JSC_CCALL(imgui_mousehoveringrect,
  return JS_NewBool(js,ImGui::IsMouseHoveringRect(js2vec2(js,argv[0]), js2vec2(js,argv[1])));
)

JSC_CCALL(imgui_mouseclicked,
  return JS_NewBool(js,ImGui::IsMouseClicked(js2number(js, argv[0])));
)

JSC_CCALL(imgui_mousedown,
  return JS_NewBool(js,ImGui::IsMouseDown(js2number(js, argv[0])));
)

JSC_CCALL(imgui_mousereleased,
  return JS_NewBool(js,ImGui::IsMouseReleased(js2number(js, argv[0])));
)

JSC_CCALL(imgui_mousedragging,
  return JS_NewBool(js,ImGui::IsMouseDragging(js2number(js, argv[0])));
)

JSC_CCALL(imgui_mousedelta,
  ImVec2 dm = ImGui::GetIO().MouseDelta;
  return vec22js(js, (ImVec2){dm.x,dm.y});
)

JSC_CCALL(imgui_dummy,
  ImGui::Dummy(js2vec2(js,argv[0]));
)

JSC_SCALL(imgui_invisiblebutton,
  ImGui::InvisibleButton(str, js2vec2(js,argv[1]));
)

JSC_CCALL(imgui_width,
  ImGui::PushItemWidth(js2number(js, argv[0]));
)

JSC_CCALL(imgui_windowpos,
  return vec22js(js,ImGui::GetWindowPos());
)

JSC_CCALL(imgui_plotpos,
  return plotvec22js(js, ImPlot::GetPlotPos());
)

JSC_CCALL(imgui_plot2pixels,
  return vec22js(js, ImPlot::PlotToPixels(js2plotvec2(js, argv[0])));
)

JSC_CCALL(imgui_plotlimits,
  ImPlotRect lim = ImPlot::GetPlotLimits();
  JSValue xlim = JS_NewObject(js);
  JS_SetPropertyStr(js,xlim, "min", JS_NewFloat64(js,lim.X.Min));
  JS_SetPropertyStr(js,xlim, "max", JS_NewFloat64(js,lim.X.Max));
  JSValue ylim = JS_NewObject(js);
  JS_SetPropertyStr(js,ylim, "min", JS_NewFloat64(js,lim.Y.Min));
  JS_SetPropertyStr(js,ylim, "max", JS_NewFloat64(js,lim.Y.Max));
  
  JSValue limits = JS_NewObject(js);
  JS_SetPropertyStr(js,limits, "x", xlim);
  JS_SetPropertyStr(js,limits, "y", ylim);
  
  return limits;
)

static JSValue axis_fmts[10];
void jsformatter(double value, char *buff, int size, JSValue *fmt)
{
/*  JSValue v = JS_NewFloat64(js,value);
  const char *str = JS_ToCString(js, JS_Call(js,*fmt, JS_UNDEFINED, 1, &v));
  strncpy(buff,str, size);
  JS_FreeCString(js, str);*/
}

JSC_CCALL(imgui_axisfmt,
  int y = num_to_yaxis(js2number(js, argv[0]));
  
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
/*  sg_frame_stats stats = sg_query_frame_stats();
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
  FSTAT(size_update_image)*/
  return v;
)

JSC_CCALL(imgui_setaxes,
  int x = num_to_xaxis(js2number(js, argv[0]));
  int y = num_to_yaxis(js2number(js, argv[1]));
  ImPlot::SetAxes(x,y);
)

JSC_CCALL(imgui_setupaxis,
  ImPlot::SetupAxis(num_to_yaxis(js2number(js, argv[0])));
)

JSC_SCALL(imgui_setclipboard,
  ImGui::SetClipboardText(str);
)

JSC_CCALL(imgui_newframe,
  simgui_frame_desc_t frame = {
    .width = js2number(js, argv[0]),
    .height = js2number(js,argv[1]),
    .delta_time = js2number(js,argv[2])
  };
  simgui_new_frame(&frame);
)

JSC_CCALL(imgui_endframe,
  simgui_render();
)

JSC_CCALL(imgui_wantmouse,
  return JS_NewBool(js, ImGui::GetIO().WantCaptureMouse);
)

JSC_CCALL(imgui_wantkeys,
  return JS_NewBool(js, ImGui::GetIO().WantCaptureKeyboard);
)

JSC_CCALL(imgui_init,
  simgui_desc_t sdesc = {
    .image_pool_size = 1024
  };
  simgui_setup(&sdesc);
  
  sgimgui_desc_t desc = {0};
  sgimgui_init(&sgimgui, &desc);

  sgimgui.frame_stats_window.disable_sokol_imgui_stats = true;

  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = ".prosperon/imgui.ini";
  ImGui::LoadIniSettingsFromDisk(".prosperon/imgui.ini");

  ImPlot::CreateContext();
  ImNodes::CreateContext();
  sg_enable_frame_stats();
)

#define MIST_FUNC_DEF(CLASS,NAME,AMT) JS_CFUNC_DEF(#NAME, AMT, js_##CLASS##_##NAME)
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
  MIST_FUNC_DEF(imgui, tableangledheadersrow, 0),
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
  MIST_FUNC_DEF(imgui, newframe, 3),
  MIST_FUNC_DEF(imgui, endframe, 0),
  MIST_FUNC_DEF(imgui, wantmouse, 0),
  MIST_FUNC_DEF(imgui, wantkeys, 0),
  MIST_FUNC_DEF(imgui, init, 0),
};

extern "C" {
void gui_input(sapp_event *e)
{
  simgui_handle_event(e);
}

JSValue js_imgui(JSContext *js)
{
  JSValue imgui = JS_NewObject(js);
  JS_SetPropertyFunctionList(js, imgui, js_imgui_funcs, sizeof(js_imgui_funcs)/sizeof(js_imgui_funcs[0]));

  return imgui;
}
}
static int js_init_imgui(JSContext *js, JSModuleDef *m) {
  JS_SetModuleExportList(js, m, js_imgui_funcs, sizeof(js_imgui_funcs)/sizeof(JSCFunctionListEntry));
  JS_SetModuleExport(js, m, "default", js_imgui(js));
  return 0;
}

#ifdef JS_SHARED_LIBRARY
#define JS_INIT_MODULE js_init_module
#else
#define JS_INIT_MODULE js_init_module_imgui
#endif

JSModuleDef *JS_INIT_MODULE(JSContext *js, const char *name) {
  JSModuleDef *m;
  m = JS_NewCModule(js, name, js_init_imgui);
  if (!m) return NULL;
  JS_AddModuleExport(js, m, "default");
  return m;
}
