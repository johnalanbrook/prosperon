project('prosperon', ['c', 'cpp'], default_options : [ 'cpp_std=c++14', 'default_library=static'])

libtype = get_option('default_library')

if (libtype != 'static')
  add_global_arguments('-DJS_SHARED_LIBRARY', language:'c')
endif

add_global_arguments('-DCONFIG_BIGNUM', language : 'c')
add_global_arguments('-DCP_USE_DOUBLES=0', language : 'c') # option for chipmunk physics

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
endif

if host_machine.system() == 'windows'
  depstrs = ['d3d11']
  foreach str : depstrs
    deps += dependency(str)
  endforeach
endif

deps += dependency('quickjs', static:true)
deps += dependency('qjs-enet',static:true)
deps += dependency('qjs-layout',static:true)
deps += dependency('qjs-nota',static:true)
#deps += dependency('qjs-steam',static:true)
deps += dependency('qjs-chipmunk',static:true)
deps += dependency('qjs-miniz',static:true)
deps += dependency('qjs-soloud',static:true)

sources = []
src = ['anim.c', 'config.c', 'datastream.c','font.c','gameobject.c','qjs_imgui.cpp','HandmadeMath.c','input.c', 'jsffi.c','log.c','miniz.c','model.c','render.c','script.c','resources.c','simplex.c','spline.c','texture.c', 'timer.c', 'transform.c','warp.c','window.c','yugine.c']

imsrc = ['GraphEditor.cpp','ImCurveEdit.cpp','ImGradient.cpp','imgui_draw.cpp','imgui_tables.cpp','imgui_widgets.cpp','imgui.cpp','ImGuizmo.cpp','imnodes.cpp','implot_items.cpp','implot.cpp']

srceng = 'source/engine'
tp = srceng / 'thirdparty'

includes = [srceng,tp / 'cgltf',tp / 'imgui',tp / 'par',tp / 'sokol',tp / 'stb',tp,tp / 'pl_mpeg/include']

foreach file : src
    full_path = join_paths('source/engine', file)
    sources += files(full_path)
endforeach

foreach imgui : imsrc
  sources += tp / 'imgui' / imgui
endforeach

includers = []
foreach inc : includes
  includers += include_directories(inc)
endforeach

incc = include_directories('source/engine/thirdparty/sokol')

packer = executable('packer',
  'tools/packer.c', 'source/engine/miniz.c',
  include_directories: includers,
)

core_glob = run_command('globcore.sh')
core_files = core_glob.stdout().strip().split('\n')

prosperon = executable('prosperon', sources,
  dependencies: deps,
  include_directories: includers
)
