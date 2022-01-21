MAKEFLAGS := --jobs=$(shell nproc)

UNAME := $(shell uname)

ifeq ($(OS),Windows_NT)
	UNAME := Windows_NT
endif

UNAME_P := $(shell uname -m)

#CC specifies which compiler we're using
CC = clang
CXX = clang++

ifeq ($(DEBUG), 1)
	DEFFALGS += -DDEBUG
	INFO = dbg
endif

BINDIR := ./bin
BUILDDIR := ./obj

objprefix = ./obj

DIRS := engine pinball editor brainstorm
ETP := ./source/engine/thirdparty/

# All other sources
esrcs := $(shell find ./source/engine -name '*.c*')
esrcs := $(filter-out %sqlite3.c %shell.c, $(esrcs))
eheaders := $(shell find ./source/engine -name '*.h')
edirs := $(addprefix $(ETP), Chipmunk2D/include bitmap-outliner cgltf enet/include pl_mpeg s7 sqlite3 stb) ./source/engine # $(shell find ./source/engine -type d)
edirs := $(filter-out %docs %doc %include% %src %examples  , $(edirs))

eobjects := $(sort $(patsubst .%.cpp, $(objprefix)%.o, $(filter %.cpp, $(esrcs))) $(patsubst .%.c, $(objprefix)%.o, $(filter %.c, $(esrcs))))

imguidir := ./source/editor/imgui/
imguisrcs := $(addprefix $(imguidir), imgui imgui_draw imgui_widgets imgui_tables backends/imgui_impl_sdl backends/imgui_impl_opengl3)
imguisrcs := $(addsuffix .cpp, $(imguisrcs))
imguiobjs := $(sort $(patsubst .%.cpp, $(objprefix)%.o, $(filter %.cpp, $(imguisrcs))) $(patsubst .%.c, $(objprefix)%.o, $(filter %.c, $(imguisrcs))))

edsrcs := $(wildcard ./source/editor/*)
edheaders := $(shell find ./source/editor -name '*.h')
eddirs := $(shell find ./source/editor -type d)

edobjects := $(sort $(patsubst .%.cpp, $(objprefix)%.o, $(filter %.cpp, $(edsrcs))) $(patsubst .%.c, $(objprefix)%.o, $(filter %.c, $(edsrcs)))) $(imguiobjs)

eobjects := $(filter-out %yugine.o %pl_mpeg_extract_frames.o %pl_mpeg_player.o, $(eobjects)) $(edobjects)

edirs := $(edirs) $(eddirs)


bssrcs := $(shell find ./source/brainstorm -name '*.c*')
bsheaders := $(shell find ./source/brainstorm -name '*.h')
eddirs := $(shell find ./source/brainstorm -type d)

bsobjects := $(sort $(patsubst .%.cpp, $(objprefix)%.o, $(filter %.cpp, $(bssrcs))) $(patsubst .%.c, $(objprefix)%.o, $(filter %.c, $(bssrcs))))

pinsrcs := $(shell find ./source/editor -name '*.c*')
pinheaders := $(shell find ./source/editor -name '*.h')
pindirs := $(shell find ./source/pinball -type d)

edobjects := $(sort $(patsubst .%.cpp, $(objprefix)%.o, $(filter %.cpp, $(edsrcs))) $(patsubst .%.c, $(objprefix)%.o, $(filter %.c, $(edsrcs))))

includeflag := $(addprefix -I, $(edirs) $(eddirs) $(pindirs) $(bsdirs) ./source/editor ./source/editor/imgui ./source/editor/imgui/backends)

#COMPILER_FLAGS specifies the additional compilation options we're using
WARNING_FLAGS := -w #-pedantic -Wall -Wextra -Wwrite-strings
COMPILER_FLAGS := -g -O0 $(WARNING_FLAGS)

LIBPATH := $(addprefix -L, .)

ifeq ($(UNAME), Windows_NT)
	LINKER_FLAGS:= -static
	# DYNAMIC LIBS: SDL2 opengl32
	ELIBS := glew32 mingw32 SDL2main SDL2 m dinput8 dxguid dxerr8 user32 gdi32 winmm imm32 ole32 oleaut32 shell32 version uuid setupapi opengl32 stdc++ winpthread
	CLIBS :=
	EXT := .exe
else
	LINKER_FLAGS :=
	ELIBS := engine
	CLIBS := SDL2 SDL2_mixer GLEW GL dl pthread
	EXT :=
endif

LELIBS := -Wl,-Bstatic $(addprefix -l, ${ELIBS}) -Wl,-Bdynamic $(addprefix -l, $(CLIBS))



yuginec := ./source/engine/yugine.c

.SECONARY: $(eobjects)

engine: libengine.a
	@echo Linking engine
	$(CXX) $(yuginec) -DGLEW_STATIC $(includeflag) $(LIBPATH) $(LINKER_FLAGS) $(LELIBS) -o yugine

libengine.a: $(eobjects)
	@echo Making library engine.a
	@ar -r libengine.a $(eobjects)

xbrainstorm: libengine.a $(bsobjects)
	@echo Making brainstorm
	$(CXX) $(bsobjects) -DGLEW_STATIC $(includeflag) $(LIBPATH) $(LINKER_FLAGS) $(LELIBS) -o $@
	mv xbrainstorm brainstorm/xbrainstorm

$(objprefix)/%.o:%.cpp
	@mkdir -p $(@D)
	@echo Making C++ object $@
	@$(CXX) $(includeflag) $(COMPILER_FLAGS) -c $< -o $@

$(objprefix)/%.o:%.c
	@mkdir -p $(@D)
	@echo Making C object $@
	@$(CC) $(includeflag) $(COMPILER_FLAGS) -c $< -o $@

clean:
	@echo Cleaning project
	@rm -f $(eobjects)

