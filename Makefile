PROCS != nproc
MAKEFLAGS = --jobs=$(PROCS)

UNAME != uname
MAKEDIR != pwd

# Options
# DBG --- build with debugging symbols and logging

# Temp to strip long emcc paths to just emcc
CC := $(notdir $(CC))

DBG ?= 1

QFLAGS :=

INFO :=

ifeq ($(DBG),1)
  QFLAGS += -g
  INFO = dbg

  ifeq ($(CC),tcc)
    QFLAGS += 
  endif
else
  QFLAGS += -DNDEBUG -s
  INFO = rel
endif

QFLAGS += -DHAVE_CEIL -DHAVE_FLOOR -DHAVE_FMOD -DHAVE_LRINT -DHAVE_LRINTF

# Uncomment for smallest binary
# QFLAGS += -ffunction-sections -fdata-sections -Wl,--gc-sections -fno-stack-protector -fomit-frame-pointer -fno-math-errno -fno-unroll-loops -fmerge-all-constants -fno-ident -mfpmath=387 -fsingle-precision-constant -ffast-math

PTYPE != uname -m

LINKER_FLAGS = $(QFLAGS)

ELIBS = engine quickjs

PKGCMD = tar --directory $(BIN) --exclude="./*.a" --exclude="./obj" --exclude="./include" -czf $(DISTDIR)/$(DIST) .
ZIP = .tar.gz
UNZIP = cp $(DISTDIR)/$(DIST) $(DESTDIR) && tar xzf $(DESTDIR)/$(DIST) -C $(DESTDIR) && rm $(DESTDIR)/$(DIST)

ARCH = x64

COMPILER_FLAGS = $(includeflag) $(QFLAGS) -MD $(WARNING_FLAGS) -I. -DCP_USE_DOUBLES=0 -DTINYSPLINE_FLOAT_PRECISION -DVER=\"$(VER)\" -DINFO=\"$(INFO)\" -c $< -o $@


ifeq ($(OS), Windows_NT)
  LINKER_FLAGS += -mwin32 -static
  COMPILER_FLAGS += -mwin32
  ELIBS += mingw32 kernel32 user32 shell32 dxgi gdi32 ws2_32 ole32 winmm setupapi m
  EXT = .exe
  PLAT = w64
  PKGCMD = cd $(BIN); zip -q -r $(MAKEDIR)/$(DISTDIR)/$(DIST) . -x \*.a ./obj/\*
  ZIP = .zip
  UNZIP = unzip -o -q $(DISTDIR)/$(DIST) -d $(DESTDIR)
else ifeq ($(CC), emcc)
  LINKER_FLAGS += -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -pthread
  COMPILER_FLAGS += -pthread
  ELIBS +=  pthread quickjs GL openal c m dl
  CC = emcc
  EXT = .html
  PLAT = html5
else
  UNAME != uname -s
  ifeq ($(UNAME), Linux)
    LINKER_FLAGS += -pthread -rdynamic
    ELIBS += GL pthread c m dl X11 Xi Xcursor EGL asound
    PLAT = linux-$(ARCH)
  endif

  ifeq ($(UNAME), Darwin)
    ifeq ($(PLATFORM), macosx)
      ELIBS = Coca QuartzCore OpenGL
      PLAT = mac-$(ARCH)
    else ifeq ($(PLATFORM), iphoneos)
      ELIBS = Foundation UIKit OpenGLES GLKit
    endif
  endif
endif

BIN = bin/$(CC)/$(INFO)/

objprefix = $(BIN)obj

define prefix
	echo $(1) | tr " " "\n" | sed 's/.*/$(2)&$(3)/'
endef

define rm
	tmp=$$(mktemp); \
	echo $(2) | tr " " "\n" > $${tmp}; \
	echo $(1) | tr " " "\n" | grep -v -f $${tmp}; \
	rm $${tmp}
endef

# All other sources
edirs != find source -type d -name include
subengs = sound 3d

ifeq ($(DBG), 1)
	subengs += debug
endif

edirs += source/engine $(addprefix source/engine/, $(subengs)) source/engine/thirdparty/Nuklear
ehead != find source/engine source/engine/sound source/engine/debug -maxdepth 1 -type f -name *.h
eobjects != find source/engine -type f -name '*.c' | sed -r 's|^(.*)\.c|$(objprefix)/\1.o|'  # Gets all .c files and makes .o refs

engincs != find source/engine -maxdepth 1 -type d
includeflag != find source -type d -name include
includeflag += $(engincs) source/engine/thirdparty/Nuklear
includeflag := $(addprefix -I, $(includeflag))

WARNING_FLAGS = -Wno-incompatible-function-pointer-types -Wno-incompatible-pointer-types #-Wall -Wno-incompatible-function-pointer-types -Wno-unused-function# -pedantic -Wextra -Wwrite-strings -Wno-incompatible-function-pointer-types -Wno-incompatible-pointer-types -Wno-unused-function -Wno-int-conversion

SEM = 0.0.1
COM != fossil describe
VER = $(SEM)-$(COM)


LIBPATH = -L$(BIN)

NAME = yugine$(EXT)

ELIBS != $(call prefix, $(ELIBS), -l)

objects = $(eobjects)
DEPENDS = $(objects:.o=.d)
-include $(DEPENDS)

ENGINE = $(BIN)libengine.a

SCRIPTS = $(shell ls source/scripts/*.js)

LINK = $(LIBPATH) $(LINKER_FLAGS) $(ELIBS)

MYTAG = $(VER)_$(PTYPE)_$(INFO)

DIST = yugine-$(PLAT)-$(COM)$(ZIP)
DISTDIR = ./dist

.DEFAULT_GOAL := all
all: $(DISTDIR)/$(DIST)

DESTDIR ?= ~/.bin

install: $(DISTDIR)/$(DIST)
	@echo Unpacking $(DIST) in $(DESTDIR)
#	@unzip $(DISTDIR)/$(DIST) -d $(DESTDIR)
	@$(UNZIP)

$(BIN)$(NAME): $(objprefix)/source/engine/yugine.o $(ENGINE) $(BIN)libquickjs.a
	@echo Linking $(NAME)
	$(CC) $< $(LINK) -o $(BIN)$(NAME)
	@echo Finished build

$(DISTDIR)/$(DIST): $(BIN)$(NAME) source/shaders/* $(SCRIPTS) assets/*
	@echo Creating distribution $(DIST)
	@mkdir -p $(DISTDIR)
	@cp -rf assets/* $(BIN)
	@cp -rf source/shaders $(BIN)
	@cp -r source/scripts $(BIN)
	@$(PKGCMD)

$(BIN)libquickjs.a:
	make -C quickjs clean
	make -C quickjs libquickjs.a libquickjs.lto.a CC=$(CC)
	cp quickjs/libquickjs.* $(BIN)

$(ENGINE): $(eobjects)
	@echo Making library engine.a
	@ar r $(ENGINE) $(eobjects)

$(objprefix)/%.o:%.c
	@mkdir -p $(@D)
	@echo Making C object $@ OS $(OS)
	@$(CC) $(COMPILER_FLAGS)

clean:
	@echo Cleaning project
	@rm -rf bin/*
	@rm -f *.gz
	@rm -rf dist/*
