project('prosperon', ['c', 'cpp'], default_options : [ 'cpp_std=c++11'])

libtype = get_option('default_library')

link = []

if not get_option('editor')
  add_project_arguments('-DNEDITOR', language:'c')
endif

add_project_arguments('-Wno-implicit-function-declaration', language: 'c')
add_project_arguments('-Wno-incompatible-function-pointer-types', language: 'c')
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
  link += '-static' # Required to pack in mingw dlls on cross compilation
endif

if host_machine.system() == 'emscripten'
  link += '-sUSE_WEBGPU'
endif

deps += dependency('quickjs', static:true)
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

storefront = get_option('storefront')
if storefront == 'steam'
  deps += dependency('qjs-steam',static:false)
endif

sources = []
src = ['anim.c', 'config.c', 'datastream.c','font.c','gameobject.c','HandmadeMath.c','input.c', 'jsffi.c','log.c','miniz.c','model.c','render.c','script.c','resources.c','simplex.c','spline.c','texture.c', 'timer.c', 'transform.c','warp.c','window.c','yugine.c']

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
  command: ['zip', '-r', '@OUTPUT@'] + zip_paths,
  build_by_default: true,
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