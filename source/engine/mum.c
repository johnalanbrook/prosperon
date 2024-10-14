#include "mum.h"

#include "jsffi.h"

#include "clay.h"
#include "script.h"

///// CLAY STUFF
// Primary ideas are CLAY_CONTAINER, CLAY_TEXT, CLAY_IMAGE, CLAY_SCROLL_CONTAINER, CLAY_BORDER_CONTAINER, and CLAY_FLOATING_CONTAINER

/*
  Clay_container -> layout
  clay_text -> text
  clay_image -> layout, image
  scorll_container -> layout, scroll
  floating_container -> layout, floating
  border_container -> layout, border
  
  Layout config
  {
    sizing: {
      width: min/max:percent, type grow:fill:percent:fixed
      height: min/max:percent
    }
    padding: [x,y],
    childGap: number,
    layoutirection: horizontal:vertical
    childAlignment: {
      x: left/right/center,
      y: top/bottom/center
    }
  }
  
  text config
  {
    fontSize: (height in pixels)
    letterSpacing: pixels,
    lineSpacing: pixels,
    wrapMode: words:newlines:none
  }
  
  image config
  {
    sourceDimensions
  }
  
  floating config
  {
    offset: [x,y],
    expand: [width, height],
    zindex: number,
    attachment: {
      element: left_top:left_center:left_bottom:center_top:center_center:center_bottom:right_top:right_center:right_button,
      parent: ""
    },
  }
  
  scroll config
  {
    horizontal: boolean,
    vertical: boolean
  }
  
  border config
  {
    left: {
      width,
      color
    }
    right: '',
    top: '',
    bottom:'',
    betweenchildren: '',
    corner_radius: {
      topleft: number,
      topright:'',
      bottomleft:'',
      bottomright:''
    }
  }
  
  
  and then render the commands
  rencercmd: {
    boundingbox: {x, y, width, height},
    config: [config from element type],
    text: text [this can probably be kept in the JS object as a ref]
    commandType: none/rectangle/border/text/image/scissor_start/scissor_end
  }
*/
/*
static Clay_ChildAlignment js2childAlignment(JSValue v)
{
  Clay_ChildAlignment clay = {};
  clay.x = (Clay_LayoutAlignmentX)js2number(js_getpropstr(v, "x"));
  clay.y = (Clay_LayoutAlignmentY)js2number(js_getpropstr(v, "y"));
  return clay;
}

static Clay_SizingAxis js2claysizingaxis(JSValue v)
{
  Clay_SizingAxis clay = {0};
  clay.type = (Clay__SizingType)js2number(js_getpropstr(v, "type"));
  if (clay.type == CLAY__SIZING_TYPE_PERCENT)
    clay.sizePercent = js2number(js_getpropstr(v, "percent"));
  else {
    HMM_Vec2 minmax = js2vec2(js_getpropstr(v, "minmax"));
    clay.sizeMinMax.min = minmax.x;
    clay.sizeMinMax.max = minmax.y;
  }
  return clay;
}

static Clay_Sizing js2claysizing(JSValue v)
{
  Clay_Sizing clay = {0};
  clay.width = js2claysizingaxis(js_getpropstr(v, "width"));
  clay.height = js2claysizingaxis(js_getpropstr(v, "height"));  
  return clay;
}

static Clay_LayoutConfig js2layout(JSValue v)
{
  Clay_LayoutConfig config = {0};
  config.sizing =js2claysizing(js_getpropstr(v, "sizing"));
  HMM_Vec2 padding = js2vec2(js_getpropstr(v, "padding"));
  config.padding.x = padding.x;
  config.padding.y = padding.y;
  config.childGap = js2number(js_getpropstr(v, "child_gap"));
  config.layoutDirection = (Clay_LayoutDirection)js2number(js_getpropstr(v, "layout_direction"));
  config.childAlignment = js2childAlignment(js_getpropstr(v, "child_alignment"));
  
  return config;
}

JSC_CCALL(clay_dimensions,
  HMM_Vec2 dim = js2vec2(argv[0]);
  Clay_SetLayoutDimensions((Clay_Dimensions) { dim.x, dim.y });
)

JSC_CCALL(clay_draw,
  Clay_BeginLayout();
  script_call_sym(argv[1], 0, NULL);
  Clay_RenderCommandArray cmd = Clay_EndLayout();
  
  ret = JS_NewArray(js);
  for (int i = 0; i < cmd.length; i++) {
  }
)

JSC_CCALL(clay_pointer,
  HMM_Vec2 pos = js2vec2(argv[0]);
  int down = js2boolean(argv[1]);
  Clay_SetPointerState((Clay_Vector2) {pos.x,pos.y }, down);
)

JSC_CCALL(clay_updatescroll,
  int drag = js2boolean(argv[0]);
  HMM_Vec2 delta = js2vec2(argv[1]);
  float dt = js2number(argv[2]);
  Clay_UpdateScrollContainers(drag, (Clay_Vector2){delta.x,delta.y}, dt);
)

JSC_SCALL(clay_container,
  Clay_LayoutConfig config = js2layout(argv[1]);
  printf("got here\n");
  Clay__OpenContainerElement(CLAY_ID(str), &config);
  script_call_sym(argv[2], 0, NULL);
  Clay__CloseElementWithChildren();
)

JSC_SCALL(clay_image,
  Clay_LayoutConfig config = js2layout(argv[1]);
  Clay_ImageElementConfig image = {0};
  HMM_Vec2 dim = js2vec2(argv[2]);
  image.sourceDimensions.width = dim.x;
  image.sourceDimensions.height = dim.y;
  Clay__OpenImageElement(CLAY_ID(str), &config, &image);
  script_call_sym(argv[3], 0, NULL);  
  Clay__CloseElementWithChildren();
)

JSC_SSCALL(clay_text,
  Clay_TextElementConfig text = {0};
  text.fontSize = js2number(js_getpropstr(argv[2], "font_size"));
  text.letterSpacing = js2number(js_getpropstr(argv[2], "letter_spacing"));
  text.lineHeight = js2number(js_getpropstr(argv[2], "line_spacing"));
  text.wrapMode = (Clay_TextElementConfigWrapMode)js2number(js_getpropstr(argv[2], "wrap"));
  Clay__OpenTextElement(CLAY_ID(str), CLAY_STRING(str2), &text);
//  CLAY_TEXT(CLAY_ID(str), CLAY_STRING(str2), text)
)

JSC_SCALL(clay_scroll,
  Clay_LayoutConfig config = js2layout(argv[1]);
  Clay_ScrollElementConfig scroll = {0};
  scroll.horizontal = js2boolean(js_getpropstr(argv[1], "scroll_horizontal"));
  scroll.vertical = js2boolean(js_getpropstr(argv[1], "scroll_vertical"));
  Clay__OpenScrollElement(CLAY_ID(str), &config, &scroll);
  script_call_sym(argv[2], 0, NULL);
  Clay__CloseElementWithChildren();  
)

static Clay_FloatingElementConfig js2floating(JSValue v)
{
  Clay_FloatingElementConfig clay = {0};
  HMM_Vec2 offset = js2vec2(js_getpropstr(v, "offset"));
  clay.offset.x = offset.x;
  clay.offset.y = offset.y;
  clay.zIndex = js2number(js_getpropstr(v, "zindex"));
  HMM_Vec2 expand = js2vec2(js_getpropstr(v, "expand"));
  clay.expand.width = expand.x;
  clay.expand.height = expand.y;
  clay.attachment.element = (Clay_FloatingAttachPointType)js2number(js_getpropstr(v, "element"));
  clay.attachment.parent = (Clay_FloatingAttachPointType)js2number(js_getpropstr(v, "parent"));  
  return clay;
}

JSC_SCALL(clay_floating,
  Clay_LayoutConfig config = js2layout(argv[1]);
  Clay_FloatingElementConfig floating = js2floating(argv[1]);
  Clay__OpenFloatingElement(CLAY_ID(str), &config, &floating);
  script_call_sym(argv[2], 0, NULL);
  Clay__CloseElementWithChildren();  
)

static const JSCFunctionListEntry js_clay_funcs[] = {
  MIST_FUNC_DEF(clay, dimensions, 1),
  MIST_FUNC_DEF(clay, draw, 2),
  MIST_FUNC_DEF(clay, pointer, 2),
  MIST_FUNC_DEF(clay, updatescroll, 3),
  MIST_FUNC_DEF(clay, container, 3),
  MIST_FUNC_DEF(clay, image, 4),
  MIST_FUNC_DEF(clay, text, 3),
  MIST_FUNC_DEF(clay, scroll, 3),
  MIST_FUNC_DEF(clay, floating, 3)
};

// Example measure text function
static inline Clay_Dimensions MeasureText(Clay_String *text, Clay_TextElementConfig *config) {
    // Clay_TextElementConfig contains members such as fontId, fontSize, letterSpacing etc
    // Note: Clay_String->chars is not guaranteed to be null terminated
}

JSValue clay_init(JSContext *js)
{
  uint64_t totalMemorySize = Clay_MinMemorySize();
  Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));
  Clay_Initialize(arena, (Clay_Dimensions) { 1920, 1080 });
  Clay_SetMeasureTextFunction(MeasureText);
  
  JSValue clay = JS_NewObject(js);
  JS_SetPropertyFunctionList(js, clay, js_clay_funcs, countof(js_clay_funcs));
  
  return clay;
}
*/