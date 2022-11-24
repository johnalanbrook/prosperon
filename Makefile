procs != nproc
MAKEFLAGS = --jobs=$(procs)

UNAME != uname

ifeq ($(OS),Windows_NT)
	UNAME = Windows_NT
endif

UNAME_P != uname -m

CCACHE = ccache

CC = $(CCACHE) cc

ifeq ($(DEBUG), 1)
	DEFFALGS += -DDEBUG
	INFO = dbg
endif

BIN = ./bin/
objprefix = $(BIN)obj

DIRS = engine editor
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
edirs != find ./source -type d -name include
edirs += ./source/engine
ehead != $(call findindir,./source/engine,*.h)
eobjects != $(call make_objs, ./source/engine)
eobjects != $(call rm,$(eobjects),sqlite pl_mpeg_extract_frames pl_mpeg_player yugine nuklear)

edirs += ./source/engine/thirdparty/Chipmunk2D/include ./source/engine/thirdparty/enet/include ./source/engine/thirdparty/Nuklear
includeflag != $(call prefix,$(edirs),-I)
COMPINCLUDE = $(edirs)

WARNING_FLAGS = -Wno-everything #-Wno-incompatible-function-pointer-types -Wall -Wwrite-strings -Wunsupported -Wall -Wextra -Wwrite-strings -Wno-unused-parameter -Wno-unused-function -Wno-missing-braces -Wno-incompatible-function-pointer-types -Wno-gnu-statement-expression -Wno-complex-component-init -pedantic


SEM = 0.0.1
COM != git rev-parse --short HEAD
VER = $(SEM)-$(COM)

COMPILER_FLAGS = $(includeflag) -I/usr/local/include -g -rdynamic -O0 -MD $(WARNING_FLAGS) -DDBG=1 -DVER=\"$(VER)\" -c $< -o $@

LIBPATH = -L./bin -L/usr/local/lib

ALLFILES != find source/ -name '*.[ch]' -type f

ifeq ($(UNAME), Windows_NT)
	LINKER_FLAGS = -DSDL_MAIN_HANDLED
	ELIBS = engine editor mingw32 SDL2main SDL2 m dinput8 dxguid dxerr8 user32 gdi32 winmm imm32 ole32 oleaut32 shell32 version uuid setupapi opengl32 stdc++ winpthread
	ELIBS += SDL2_mixer FLAC vorbis vorbisenc vorbisfile mpg123 out123 syn123 opus opusurl opusfile ogg ssp shlwapi
	CLIBS = glew32
	EXT = .exe
else
	LINKER_FLAGS = -g -rdynamic
	ELIBS =  engine pthread yughc mruby c m dl
	CLIBS =
	EXT =
endif

CLIBS != $(call prefix, $(CLIBS), -l)
ELIBS != $(call prefix, $(ELIBS), -l)

LELIBS = $(ELIBS) `pkg-config --libs ./source/glfw/build/src/glfw3.pc` `pkg-config --libs ./source/portaudio/build/portaudio-2.0.pc`

objects = $(eobjects)
DEPENDS = $(objects:.o=.d)
-include $(DEPENDS)

yuginec = ./source/engine/yugine.c

ENGINE = $(BIN)libengine.a
INCLUDE = $(BIN)include

LINK = $(LIBPATH) $(LINKER_FLAGS) $(LELIBS)




.PHONY: yugine

yugine: $(yuginec:.%.c=$(objprefix)%.o) $(ENGINE) $(BIN)libportaudio.a $(BIN)libglfw3.a
	@echo Linking yugine
	$(CC) $< $(LINK) -o yugine
	@echo Finished build

dist: yugine
	mkdir -p bin/dist
	cp yugine bin/dist
	cp -rf assets/fonts bin/dist
	cp -rf source/scripts bin/dist
	cp -rf source/shaders bin/dist
	tar -czf yugine-$(VER).tar.gz --directory bin/dist .

install: yugine
	cp yugine ~/.local/bin

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

clean:
	@echo Cleaning project
	@find $(BIN) -type f -delete
	@rm -rf source/portaudio/build source/glfw/build
