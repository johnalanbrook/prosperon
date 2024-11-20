project('prosperon', ['c', 'cpp'], default_options : [ 'cpp_std=c++11'])

libtype = get_option('default_library')

link = []
src = []

if not get_option('editor')
  add_project_arguments('-DNEDITOR', language:'c')
endif

add_project_arguments('-Wno-incompatible-pointer-types', language: 'c')
add_project_arguments('-Wno-narrowing', language: 'cpp')

deps = []

if host_machine.system() == 'darwin'
  add_project_arguments('-x', 'objective-c', language: 'c')
  fworks = ['foundation', 'metal', 'audiotoolbox', 'metalkit', 'avfoundation', 'quartzcore', 'cocoa']
  foreach fkit : fworks
    deps += dependency('appleframeworks', modules: fkit)
  endforeach
endif

cc = meson.get_compiler('c')

if host_machine.system() == 'linux'
  deps += cc.find_library('asound', required:true)
  deps += [dependency('x11'), dependency('xi'), dependency('xcursor'), dependency('egl'), dependency('gl')]
  link += '-fuse-ld=mold' # use mold, which is very fast, for debug builds
endif

if host_machine.system() == 'windows'
  deps += cc.find_library('d3d11')
  # these are for tracy
  deps += cc.find_library('ws2_32')
  deps += cc.find_library('dbghelp')
  #end
  link += '-static' # Required to pack in mingw dlls on cross compilation
endif

if host_machine.system() == 'emscripten'
  link += '-sUSE_WEBGPU'
endif


tracy_opts = ['fibers=true', 'on_demand=true']
quickjs_opts = []

src += 'qjs_tracy.c'
add_project_arguments('-DTRACY_ENABLE', language:['c','cpp'])
deps += dependency('tracy', static:true, default_options:tracy_opts)

quickjs_opts += 'default_library=static'

deps += dependency('quickjs', static:true, default_options:quickjs_opts)

storefront = get_option('storefront')
if storefront == 'steam'
  deps += dependency('qjs-steam',static:false)
endif

deps += dependency('qjs-layout',static:true)
deps += dependency('qjs-nota',static:true)
deps += dependency('qjs-miniz',static:true)
deps += dependency('qjs-soloud',static:true)

deps += dependency('threads')

if get_option('chipmunk')
  deps += dependency('qjs-chipmunk',static:true)
endif

if get_option('enet')
  deps += dependency('qjs-enet',static:true)
endif

sources = []
src += ['anim.c', 'config.c', 'datastream.c','font.c','gameobject.c','HandmadeMath.c','jsffi.c','model.c','render.c','render_trace.cpp','script.c','simplex.c','spline.c','texture.c', 'timer.c', 'transform.c','warp.c','yugine.c', 'glad.c', 'wildmatch.c']

imsrc = ['GraphEditor.cpp','ImCurveEdit.cpp','ImGradient.cpp','imgui_draw.cpp','imgui_tables.cpp','imgui_widgets.cpp','imgui.cpp','ImGuizmo.cpp','imnodes.cpp','implot_items.cpp','implot.cpp']

srceng = 'source'
tp = srceng / 'thirdparty'

includes = [srceng,tp / 'cgltf',tp / 'imgui',tp / 'par',tp / 'sokol',tp / 'stb',tp,tp / 'pl_mpeg/include']

foreach file : src
    full_path = join_paths('source', file)
    sources += files(full_path)
endforeach

if get_option('editor')
  sources += 'source/qjs_imgui.cpp'
  foreach imgui : imsrc
    sources += tp / 'imgui' / imgui
  endforeach
  deps += dependency('qjs-dmon',static:true)
endif

includers = []
foreach inc : includes
  includers += include_directories(inc)
endforeach

zip_folders = ['scripts', 'fonts', 'icons', 'shaders']
zip_paths = []
foreach folder: zip_folders
  zip_paths += meson.project_source_root() / folder
endforeach

core = custom_target('core.zip',
  output:'core.zip',
  command: ['zip', '-r', '-j', '@OUTPUT@'] + zip_paths,
)

prosperon = executable('prosperon', sources,
  dependencies: deps,
  include_directories: includers,
  link_args: link
)

prosperon_dep = declare_dependency(
  link_with:prosperon
)

test('sanity', prosperon)