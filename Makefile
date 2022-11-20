procs != nproc
MAKEFLAGS = --jobs=$(procs)

UNAME != uname

ifeq ($(OS),Windows_NT)
	UNAME = Windows_NT
endif

UNAME_P != uname -m

CCACHE = ccache

CC = $(CCACHE) clang

ifeq ($(DEBUG), 1)
	DEFFALGS += -DDEBUG
	INFO = dbg
endif

BIN = ./bin/
objprefix = $(BIN)obj

DIRS = engine pinball editor brainstorm
ETP = ./source/engine/thirdparty/

define make_objs
	find $(1) -type f -name '*.c' | sed 's|\.c.*|.o|' | sed 's|\.|$(objprefix)|1'
endef

define prefix
	echo $(1) | tr " " "\n" | sed 's/.*/$(2)&$(3)/'
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
eobjects != $(call rm,$(eobjects),sqlite pl_mpeg_extract_frames pl_mpeg_player yugine nuklear)

edirs += ./source/engine/thirdparty/Chipmunk2D/include ./source/engine/thirdparty/enet/include ./source/engine/thirdparty/Nuklear
includeflag != $(call prefix,$(edirs) $(eddirs),-I)
COMPINCLUDE = $(edirs) $(eddirs)

WARNING_FLAGS = -Wno-everything #-Wno-incompatible-function-pointer-types -Wall -Wwrite-strings -Wunsupported -Wall -Wextra -Wwrite-strings -Wno-unused-parameter -Wno-unused-function -Wno-missing-braces -Wno-incompatible-function-pointer-types -Wno-gnu-statement-expression -Wno-complex-component-init -pedantic

COMPILER_FLAGS = $(includeflag) -I/usr/local/include -g -O0 -MD $(WARNING_FLAGS) -c $< -o $@

LIBPATH = -L./bin -L/usr/local/lib

ALLFILES != find source/ -name '*.[ch]' -type f

ifeq ($(UNAME), Windows_NT)
	LINKER_FLAGS = -static -DSDL_MAIN_HANDLED
	ELIBS = engine editor mingw32 SDL2main SDL2 m dinput8 dxguid dxerr8 user32 gdi32 winmm imm32 ole32 oleaut32 shell32 version uuid setupapi opengl32 stdc++ winpthread
	ELIBS += SDL2_mixer FLAC vorbis vorbisenc vorbisfile mpg123 out123 syn123 opus opusurl opusfile ogg ssp shlwapi
	CLIBS = glew32
	EXT = .exe
else
	LINKER_FLAGS = -g
	ELIBS =  engine glfw3  pthread yughc mruby portaudio m
	CLIBS = asound jack  c
	EXT =
endif

CLIBS != $(call prefix, $(CLIBS), -l)
ELIBS != $(call prefix, $(ELIBS), -l)

LELIBS = -Wl,-Bdynamic $(ELIBS) -Wl,-Bdynamic $(CLIBS)

objects = $(eobjects)
DEPENDS = $(objects:.o=.d)
-include $(DEPENDS)

yuginec = ./source/engine/yugine.c

ENGINE = $(BIN)libengine.a
INCLUDE = $(BIN)include

LINK = $(LIBPATH) $(LINKER_FLAGS) $(LELIBS)

engine: $(yuginec:.%.c=$(objprefix)%.o) $(ENGINE) tags $(BIN)libportaudio.a $(BIN)libglfw3.a
	@echo Linking engine
	$(CC) $< $(LINK) -o $@
	@echo Finished build

bs: engine
	cp engine brainstorm

pin: engine
	cp -rf source/shaders pinball
	cp -rf assets/fonts pinball
	cp -f assets/scripts/* pinball/scripts
	cp engine pinball

pal: engine
	cp engine paladin

$(ENGINE): $(eobjects)
	@echo Making library engine.a
	@ar r $(ENGINE) $(eobjects)
	@mkdir -p $(INCLUDE)
	@cp -u -r $(ehead) $(INCLUDE)

bin/libglfw3.a: source/glfw/build/src/libglfw3.a
	@cp $< $@

source/glfw/build/src/libglfw3.a:
	@echo Making GLFW
	@mkdir -p source/glfw/build
	@cd source/glfw/build; cmake ..; make -j16
	@cp source/glfw/build/src/libglfw3.a bin

bin/libportaudio.a: source/portaudio/build/libportaudio.a
	@cp $< $@

source/portaudio/build/libportaudio.a:
	@echo Making Portaudio
	@mkdir -p source/portaudio/build
	@cd source/portaudio/build; cmake ..; make -j16
	@cp source/portaudio/build/libportaudio.a bin


$(objprefix)/%.o:%.c
	@mkdir -p $(@D)
	@echo Making C object $@
	@$(CC) $(COMPILER_FLAGS)

tags: $(ALLFILES)
	@echo Making tags
	@ctags -x -R source > tags

clean:
	@echo Cleaning project
	@find $(BIN) -type f -delete
	@rm -rf source/portaudio/build source/glfw/build
