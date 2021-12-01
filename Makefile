MAKEFLAGS := --jobs=$(shell nproc)

TARGET := game
INFO :=

EDITOR := 1
DEBUG := 1

UNAME := $(shell uname)

ifeq ($(OS),Windows_NT)
	UNAME := Windows_NT
endif

UNAME_P := $(shell uname -m)

#CC specifies which compiler we're using
CC = clang -x c --std=c99
CXX = clang++

DEFFLAGS :=

ifeq ($(EDITOR), 1)
	DEFFLAGS += -DEDITOR
	TARGET = editor
endif

ifeq ($(DEBUG), 1)
	DEFFALGS += -DDEBUG
	INFO = dbg
endif

BINDIR := ./bin
BUILDDIR := ./obj

THIRDPARTY_DIR := ./source/thirdparty
THIRDPARTY_DIRS := ${sort ${dir ${wildcard ${THIRDPARTY_DIR}/*/}}}
THIRDPARTY_I := $(addsuffix include, ${THIRDPARTY_DIRS})  $(addsuffix src, $(THIRDPARTY_DIRS)) $(THIRDPARTY_DIRS) $(addsuffix build/include, $(THIRDPARTY_DIRS)) $(addsuffix include/chipmunk, $(THIRDPARTY_DIRS))
THIRDPARTY_L := $(addsuffix build/lib, $(THIRDPARTY_DIRS)) $(addsuffix build, ${THIRDPARTY_DIRS}) $(addsuffix build/bin, $(THIRDPARTY_DIRS) $(addsuffix build/src, $(THIRDPARTY_DIRS)))

# imgui sources
IMGUI_DIR := ${THIRDPARTY_DIR}/imgui
imgui = $(addprefix ${IMGUI_DIR}/, imgui imgui_draw imgui_widgets imgui_tables)
imguibackends = $(addprefix ${IMGUI_DIR}/backends/, imgui_impl_sdl imgui_impl_opengl3)
imguiobjs := $(addsuffix .cpp, $(imgui) $(imguibackends))

plmpeg_objs := ${THIRDPARTY_DIR}/pl_mpeg/pl_mpeg_extract_frames.c
cpobjs := $(wildcard ${THIRDPARTY_DIR}/Chipmunk2D/src/*.c)
s7objs := ${THIRDPARTY_DIR}/s7/s7.c

includeflag := $(addprefix -I, $(THIRDPARTY_I) ./source/engine ./source/editor $(IMGUI_DIR) $(IMGUI_DIR)/backends)

#LIBRARY_PATHS specifies the additional library paths we'll need
LIB_PATHS := $(addprefix -L, $(THIRDPARTY_L))

# Engine sources
sources := $(wildcard ./source/engine/*.cpp ./source/engine/*.c ./source/editor/*.c ./source/editor/*.cpp) $(imguiobjs) $(s7objs) $(cpobjs)

#COMPILER_FLAGS specifies the additional compilation options we're using
WARNING_FLAGS := -w #-pedantic -Wall -Wextra -Wwrite-strings
COMPILER_FLAGS := -g -O0 $(WARNING_FLAGS)


ifeq ($(UNAME), Windows_NT)
	LINKER_FLAGS:= -static
	# DYNAMIC LIBS: SDL2 opengl32
	ELIBS := glew32 mingw32 SDL2main SDL2 m dinput8 dxguid dxerr8 user32 gdi32 winmm imm32 ole32 oleaut32 shell32 version uuid setupapi opengl32 stdc++ winpthread
	CLIBS :=
	EXT := .exe
else
	LINKER_FLAGS :=
	ELIBS :=
	CLIBS := SDL2 GLEW GL dl pthread
	EXT :=
endif

LELIBS := -Wl,-Bstatic $(addprefix -l, ${ELIBS}) -Wl,-Bdynamic $(addprefix -l, $(CLIBS))

BUILDD = -DGLEW_STATIC

dir_guard = @mkdir -p $(@D)

objprefix = ./obj/$(UNAME)/$(UNAME_P)/$(TARGET)$(INFO)

objects := $(patsubst .%.cpp, $(objprefix)%.o, $(filter %.cpp, $(sources))) $(patsubst .%.c, $(objprefix)%.o, $(filter %.c, $(sources)))
objects := $(sort $(objects))
depends := $(patsubst %.o, %.d, $(objects))

FILENAME = $(TARGET)$(INFO)_$(UNAME_P)$(EXT)

all: install

$(TARGET): $(objects)
	@echo Linking $(TARGET)
	@$(CXX) $^ -DGLEW_STATIC $(LINKER_FLAGS) $(LIB_PATHS) $(LELIBS) -o $@

install: $(TARGET)
	mkdir -p bin/$(UNAME) && cp $(TARGET) bin/$(UNAME)/$(FILENAME)
	cp $(TARGET) yugine/$(FILENAME)
	rm $(TARGET)

-include $(depends)

$(objprefix)/%.o:%.cpp
	$(dir_guard)
	@echo Making C++ object $(notdir $@)
	-@$(CXX) $(BUILDD) $(DEFFLAGS) $(includeflag) $(COMPILER_FLAGS) -MD -c $< -o $@

$(objprefix)/%.o:%.c
	$(dir_guard)
	@echo Making C object $(notdir $@)
	-@$(CC) $(BUILDD) $(DEFFLAGS) $(includeflag) $(COMPILER_FLAGS) -MD -c $< -o $@

clean:
	@echo Cleaning project
	@rm -f $(objects) $(depends)

bsclean:
	rm -f $(bsobjects)

gameclean:
	rm -f $(gameobjects)

TAGS: $(sources) $(edsources)
	@echo Generating TAGS file
	@ctags -eR $^
