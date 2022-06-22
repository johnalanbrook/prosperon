procs != nproc
MAKEFLAGS = --jobs=$(procs)

UNAME != uname

ifeq ($(OS),Windows_NT)
	UNAME = Windows_NT
endif

UNAME_P != uname -m

#CC specifies which compiler we're using
CC = clang -std=c99

ifeq ($(DEBUG), 1)
	DEFFALGS += -DDEBUG
	INFO = dbg
endif

BIN = ./bin/
objprefix = $(BIN)obj

DIRS = engine pinball editor brainstorm
ETP = ./source/engine/thirdparty/

define make_objs
	find $(1) -type f -name '*.c' -o -name '*.cpp' | sed 's|\.c.*|.o|' | sed 's|\.|$(objprefix)|1'
endef

define make_obj
	echo $(1) | tr " " "\n" | sed 's|\.c.*|.o|' | sed 's|\.|$(objprefix)|1'
endef

define find_include
	find $(1) -type f -name '*.h'  | sed 's|\/[^\/]*\.h$$||' | sort | uniq
endef

define prefix
	echo $(1) | tr " " "\n" | sed 's|.*|$(2)&$(3)|'
endef

define rm
	tmp=$$(mktemp); \
	echo $(2) | tr " " "\n" > $${tmp}; \
	echo $(1) | tr " " "\n" | grep -v -f $${tmp}; \
	rm $${tmp}
endef

define findindir
	find $(1) -maxdepth 1 -type f -name '$(2)'
endef

# All other sources
edirs != find ./source/engine -type d -name include
edirs += ./source/engine
ehead != $(call findindir,./source/engine,*.h)
eobjects != $(call make_objs, ./source/engine)
eobjects != $(call rm,$(eobjects),sqlite pl_mpeg_extract_frames pl_mpeg_player yugine.c yugine.c)

eddirs != find ./source/editor -type d
eddirs += ./source/editor
edhead != $(call findindir,./source/editor,*.h)
edobjects != find ./source/editor -maxdepth 1 -type f  -name '*.c' -o -name '*.cpp'
edobjects != $(call make_obj,$(edobjects))

bsdirs != find ./source/brainstorm -type d
bsobjects != $(call make_objs, ./source/brainstorm)

pindirs != find ./source/pinball -type d
pinobjects != $(call make_objs, ./source/pinball);

edirs += ./source/engine/thirdparty/Chipmunk2D/include ./source/engine/thirdparty/enet/include
includeflag != $(call prefix,$(edirs) $(eddirs) $(pindirs) $(bsdirs),-I)
COMPINCLUDE = $(edirs) $(eddirs) $(pindirs) $(bsdirs)

#COMPILER_FLAGS specifies the additional compilation options we're using
WARNING_FLAGS = #-Wall -Wextra -Wwrite-strings -Wno-unused-parameter -Wno-unused-function -Wno-missing-braces -Wno-incompatible-function-pointer-types -Wno-gnu-statement-expression -Wno-complex-component-init -pedantic
#COMPILER_FLAGS = $(includeflag) -g -O0 $(WARNING_FLAGS) -DGLEW_STATIC -D_GLFW_X11 -D_POSIX_C_SOURCE=1993809L -c -MMD -MP $< -o $@
COMPILER_FLAGS = $(includeflag) -I/usr/local/include -g -O0 $(WARNING_FLAGS) -DGLEW_STATIC -D_GLFW_X11 -D_POSIX_C_SOURCE=1993809L -c $< -o $@

LIBPATH = -L./bin

ifeq ($(UNAME), Windows_NT)
	LINKER_FLAGS = -static -DSDL_MAIN_HANDLED
	ELIBS = engine editor mingw32 SDL2main SDL2 m dinput8 dxguid dxerr8 user32 gdi32 winmm imm32 ole32 oleaut32 shell32 version uuid setupapi opengl32 stdc++ winpthread
	ELIBS += SDL2_mixer FLAC vorbis vorbisenc vorbisfile mpg123 out123 syn123 opus opusurl opusfile ogg ssp shlwapi
	CLIBS = glew32
	EXT = .exe
else
	LINKER_FLAGS =
	ELIBS = editor engine
	CLIBS = glfw SDL2 SDL2_mixer m
	EXT =
endif

ELIBS != $(call prefix, $(ELIBS), -l)
CLIBS != $(call prefix, $(CLIBS), -l)

#LELIBS = -Wl,-Bstatic $(ELIBS) -Wl,-Bdynamic $(CLIBS)
LELIBS = $(ELIBS) $(CLIBS)

objects = $(bsobjects) $(eobjects) $(pinobjects)
DEPENDS = $(objects:.o=.d)
-include $(DEPENDS)

yuginec = ./source/engine/yugine.c

ENGINE = $(BIN)libengine.a
EDITOR = $(BIN)libeditor.a
INCLUDE = $(BIN)include

linkinclude = $(BIN)include

LINK = $(LIBPATH) $(LINKER_FLAGS) $(LELIBS) -o $@

engine: $(yuginec:.%.c=$(objprefix)%.o) $(ENGINE)
	@echo Linking engine
	@$(CC) $< $(LINK)
	@echo Finished build

editor: $(yuginec:.%.c=$(objprefix)%.o) $(EDITOR) $(ENGINE)
	@echo Linking editor
	$(CC) $< $(LINK)
	@echo Finished build

$(ENGINE): $(eobjects)
	@echo Making library engine.a
	@ar -r $(ENGINE) $(eobjects)
	@cp -u -r $(ehead) $(INCLUDE)

$(EDITOR): $(edobjects)
	@echo Making editor library
	@ar -r $(EDITOR) $(edobjects)
	@cp -u -r $(edhead) $(INCLUDE)

xbrainstorm: $(ENGINE) $(bsobjects)
	@echo Making brainstorm
	@$(CC) $(bsobjects) $(LINK) -o $@
	@mv xbrainstorm brainstorm/brainstorm$(EXT)

pinball: $(ENGINE) $(pinobjects)
	@echo Making pinball
	@$(CC) $(pinobjects) $(LINK) -o $@
	@mv pinball paladin/pinball

$(objprefix)/%.o:%.cpp
	@mkdir -p $(@D)
	@echo Making C++ object $@
	-@$(CC) $(COMPILER_FLAGS)

$(objprefix)/%.o:%.c
	@mkdir -p $(@D)
	@echo Making C object $@
	-@$(CC) $(COMPILER_FLAGS)

clean:
	@echo Cleaning project
	@find $(BIN) -type f -delete