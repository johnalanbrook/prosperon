procs != nproc
MAKEFLAGS = --jobs=$(procs)

UNAME != uname

ifeq ($(OS),Windows_NT)
	UNAME = Windows_NT
endif

UNAME_P != uname -m

CCACHE = ccache

CC = $(CCACHE) clang -DSDL_DISABLE_IMMINTRIN_H
CLINK = clang

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
eobjects != $(call rm,$(eobjects),sqlite pl_mpeg_extract_frames pl_mpeg_player yugine)

edirs += ./source/engine/thirdparty/Chipmunk2D/include ./source/engine/thirdparty/enet/include ./source/engine/thirdparty/Nuklear
includeflag != $(call prefix,$(edirs) $(eddirs),-I)
COMPINCLUDE = $(edirs) $(eddirs)

WARNING_FLAGS = -Wno-everything #-Wall -Wwrite-strings -Wunsupported -Wall -Wextra -Wwrite-strings -Wno-unused-parameter -Wno-unused-function -Wno-missing-braces -Wno-incompatible-function-pointer-types -Wno-gnu-statement-expression -Wno-complex-component-init -pedantic

COMPILER_FLAGS = $(includeflag) -I/usr/local/include -g -O0 -MD $(WARNING_FLAGS) -c $< -o $@

LIBPATH = -L./bin -L/usr/local/lib -L/usr/lib64/pipewire-0.3/jack

ALLFILES != find source/ -name '*.[ch]' -type f

ifeq ($(UNAME), Windows_NT)
	LINKER_FLAGS = -static -DSDL_MAIN_HANDLED
	ELIBS = engine editor mingw32 SDL2main SDL2 m dinput8 dxguid dxerr8 user32 gdi32 winmm imm32 ole32 oleaut32 shell32 version uuid setupapi opengl32 stdc++ winpthread
	ELIBS += SDL2_mixer FLAC vorbis vorbisenc vorbisfile mpg123 out123 syn123 opus opusurl opusfile ogg ssp shlwapi
	CLIBS = glew32
	EXT = .exe
else
	LINKER_FLAGS = -g /usr/lib64/pipewire-0.3/jack/libjack.so.0
	ELIBS = m c engine glfw3 portaudio rt asound pthread SDL2 yughc mruby
	CLIBS =
	EXT =
endif

ELIBS != $(call prefix, $(ELIBS), -l)

LELIBS = $(ELIBS) $(CLIBS)

objects = $(eobjects)
DEPENDS = $(objects:.o=.d)
-include $(DEPENDS)

yuginec = ./source/engine/yugine.c

ENGINE = $(BIN)libengine.a
INCLUDE = $(BIN)include

LINK = $(LIBPATH) $(LINKER_FLAGS) $(LELIBS)

engine: $(yuginec:.%.c=$(objprefix)%.o) $(ENGINE)
	@echo Linking engine
	$(CLINK) $< $(LINK) -o $@
	@echo Finished build

bs: engine
	cp engine brainstorm

pin: engine
	cp engine pinball

$(ENGINE): $(eobjects) bin/libglfw3.a
	@echo Making library engine.a
	@ar r $(ENGINE) $(eobjects)
	@cp -u -r $(ehead) $(INCLUDE)

bin/libglfw3.a:
	@echo Making GLFW
	@make glfw/build
	@cp glfw/build/src/libglfw3.a bin/libglfw3.a

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
