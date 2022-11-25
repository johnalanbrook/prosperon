PROCS != nproc
MAKEFLAGS = --jobs=$(PROCS)

UNAME != uname

CC := ccache $(CC)

FLAGS = -DALLOW_SMP_DANGERS

QFLAGS = -O3 -DDBG=0
INFO = rel
PTYPE != uname -m

ifeq ($(DBG), 1)
	QFLAGS = -O0 -g -DDBG=1
	INFO = dbg
endif

QFLAGS := $(QFLAGS) $(FLAGS)

BIN = bin/
objprefix = $(BIN)obj

DIRS = engine editor

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
edirs != find source -type d -name include
edirs += source/engine source/engine/thirdparty/Nuklear
ehead != $(call findindir, source/engine,*.h)
eobjects != find source/engine -type f -name '*.c' | sed -r 's|^(.*)\.c|$(objprefix)/\1.o|'  # Gets all .c files and makes .o refs
eobjects != $(call rm,$(eobjects),sqlite pl_mpeg_extract_frames pl_mpeg_player yugine nuklear)

includeflag != $(call prefix,$(edirs),-I)

WARNING_FLAGS = #-Wall -pedantic -Wunsupported -Wextra -Wwrite-strings 
NO_WARNING_FLAGS = -Wno-incompatible-function-pointer-types -Wno-unused-parameter -Wno-unused-function -Wno-missing-braces -Wno-incompatible-function-pointer-types -Wno-gnu-statement-expression -Wno-complex-component-init

SEM = 0.0.1
COM != git rev-parse --short HEAD
VER = $(SEM)-$(COM)

COMPILER_FLAGS = $(includeflag) -I/usr/local/include $(QFLAGS) -rdynamic -MD $(WARNING_FLAGS) -DVER=\"$(VER)\" -DINFO=\"$(INFO)\" -c $< -o $@

LIBPATH = -Lbin -L/usr/local/lib

ifeq ($(UNAME), Windows_NT)
	LINKER_FLAGS = -DSDL_MAIN_HANDLED
	ELIBS = engine editor mingw32 SDL2main SDL2 m dinput8 dxguid dxerr8 user32 gdi32 winmm imm32 ole32 oleaut32 shell32 version uuid setupapi opengl32 stdc++ winpthread
	ELIBS += SDL2_mixer FLAC vorbis vorbisenc vorbisfile mpg123 out123 syn123 opus opusurl opusfile ogg ssp shlwapi glew32
	EXT = .exe
else
	LINKER_FLAGS = $(QFLAGS) -rdynamic
	ELIBS =  engine pthread yughc mruby c m dl
	EXT =
endif

ELIBS != $(call prefix, $(ELIBS), -l)
ELIBS := $(ELIBS) `pkg-config --libs source/glfw/build/src/glfw3.pc` `pkg-config --libs source/portaudio/build/portaudio-2.0.pc`

objects = $(eobjects)
DEPENDS = $(objects:.o=.d)
-include $(DEPENDS)

yuginec = source/engine/yugine.c

ENGINE = $(BIN)libengine.a
INCLUDE = $(BIN)include

LINK = $(LIBPATH) $(LINKER_FLAGS) $(ELIBS)

MYTAG = $(VER)_$(PTYPE)_$(INFO)

.PHONY: yugine

yugine: bin/obj/source/engine/yugine.o $(ENGINE) $(BIN)libportaudio.a $(BIN)libglfw3.a
	@echo Linking yugine
	$(CC) $< $(LINK) -o yugine
	@echo Finished build

dist: yugine
	mkdir -p bin/dist
	cp yugine bin/dist
	cp -rf assets/fonts bin/dist
	cp -rf source/scripts bin/dist
	cp -rf source/shaders bin/dist
	tar -czf yugine-$(MYTAG).tar.gz --directory bin/dist .

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
	@cd source/glfw/build; cmake ..; make -j$(PROCS)
	@cp source/glfw/build/src/libglfw3.a bin

bin/libportaudio.a: source/portaudio/build/libportaudio.a
	@cp $< $@

source/portaudio/build/libportaudio.a:
	@echo Making Portaudio
	@mkdir -p source/portaudio/build
	@cd source/portaudio/build; cmake ..; make -j$(PROCS)
	@cp source/portaudio/build/libportaudio.a bin


$(objprefix)/%.o:%.c
	@mkdir -p $(@D)
	@echo Making C object $@
	@$(CC) $(COMPILER_FLAGS)

clean:
	@echo Cleaning project
	@find $(BIN) -type f -delete
	@rm -rf source/portaudio/build source/glfw/build
