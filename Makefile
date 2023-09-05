PROCS != nproc
MAKEFLAGS = --jobs=$(PROCS)

UNAME != uname
MAKEDIR != pwd

# Options
# DBG --- build with debugging symbols and logging

# Temp to strip long emcc paths to just emcc
CC := $(notdir $(CC))

DBG ?= 1
OPT ?= 0

INFO :=
LD = $(CC)

ifeq ($(DBG),1)
  CFLAGS += -g
  INFO += dbg
else
  CFLAGS += -DNDEBUG
  LDFLAGS += -s
  INFO += rel
endif

ifeq ($(OPT),small)
  CFLAGS += -Oz -flto -fno-ident -fno-asynchronous-unwind-tables 
  LDFLAGS += -flto
  INFO := $(addsuffix _small,$(INFO))
else
  ifeq ($(OPT), 1)
    CFLAGS += -O2 -flto
    LDFLAGS += -flto
    INFO := $(addsuffix _opt,$(INFO))
  else
    CFLAGS += -O0
  endif
endif

PTYPE != uname -m

CFLAGS += -DHAVE_CEIL -DHAVE_FLOOR -DHAVE_FMOD -DHAVE_LRINT -DHAVE_LRINTF $(includeflag) -MD $(WARNING_FLAGS) -I. -DCP_USE_DOUBLES=0 -DTINYSPLINE_FLOAT_PRECISION -DVER=\"$(VER)\" -DINFO=\"$(INFO)\"

PKGCMD = tar --directory $(BIN) --exclude="./*.a" --exclude="./obj" -czf $(DISTDIR)/$(DIST) .
ZIP = .tar.gz
UNZIP = cp $(DISTDIR)/$(DIST) $(DESTDIR) && tar xzf $(DESTDIR)/$(DIST) -C $(DESTDIR) && rm $(DESTDIR)/$(DIST)

ARCH = x64

ifeq ($(OS), Windows_NT)
  LDFLAGS += -mwin32 -static
  CFLAGS += -mwin32
  LDLIBS += mingw32 kernel32 user32 shell32 dxgi gdi32 ws2_32 ole32 winmm setupapi m
  EXT = .exe
  PLAT = w64
  PKGCMD = cd $(BIN); zip -q -r $(MAKEDIR)/$(DISTDIR)/$(DIST) . -x \*.a ./obj/\*
  ZIP = .zip
  UNZIP = unzip -o -q $(DISTDIR)/$(DIST) -d $(DESTDIR)
else ifeq ($(CC), emcc)
  LDFLAGS += -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -pthread -s ALLOW_MEMORY_GROWTH=1
  CFLAGS += -pthread
  LDLIBS +=  pthread quickjs GL openal c m dl
  CC = emcc
  EXT = .html
  PLAT = html5
else
  UNAME != uname -s
  ifeq ($(UNAME), Linux)
    LDFLAGS += -pthread -rdynamic
    LDLIBS += GL pthread c m dl X11 Xi Xcursor EGL asound
    PLAT = linux-$(ARCH)
  endif

  ifeq ($(UNAME), Darwin)
    ifeq ($(PLATFORM), macosx)
      LDLIBS += Coca QuartzCore OpenGL
      PLAT = mac-$(ARCH)
    else ifeq ($(PLATFORM), iphoneos)
      LDLIBS += Foundation UIKit OpenGLES GLKit
    endif
  endif
endif

BIN = bin/$(CC)/$(INFO)

OBJDIR = $(BIN)/obj

# All other sources
OBJS != find source/engine -type f -name '*.c'
OBJS := $(patsubst %.c, %.o,$(OBJS))
OBJS := $(addprefix $(BIN)/obj/, $(OBJS))

engineincs != find source/engine -maxdepth 1 -type d
includeflag != find source -type d -name include
includeflag += $(engineincs) source/engine/thirdparty/Nuklear
includeflag := $(addprefix -I, $(includeflag))

WARNING_FLAGS = -Wno-incompatible-function-pointer-types -Wno-incompatible-pointer-types #-Wall -Wno-incompatible-function-pointer-types -Wno-unused-function# -pedantic -Wextra -Wwrite-strings -Wno-incompatible-function-pointer-types -Wno-incompatible-pointer-types -Wno-unused-function -Wno-int-conversion

NAME = yugine$(EXT)
SEM = 0.0.1
COM != fossil describe
VER = $(SEM)-$(COM)

LDLIBS := $(addprefix -l, $(LDLIBS))

DEPENDS = $(OBJS:.o=.d)
-include $(DEPENDS)

SCRIPTS = $(shell ls source/scripts/*.js)

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

$(BIN)/$(NAME): $(BIN)/libengine.a $(BIN)/libquickjs.lto.a
	@echo Linking $(NAME)
	$(LD) $^ $(LDFLAGS) $(LDLIBS) -o $@
	@echo Finished build

$(DISTDIR)/$(DIST): $(BIN)/$(NAME) source/shaders/* $(SCRIPTS) assets/*
	@echo Creating distribution $(DIST)
	@mkdir -p $(DISTDIR)
	@cp -rf assets/* $(BIN)
	@cp -rf source/shaders $(BIN)
	@cp -r source/scripts $(BIN)
	@$(PKGCMD)

$(BIN)/libengine.a: $(OBJS)
	@$(AR) r $@ $^

$(BIN)/libquickjs.lto.a:
	make -C quickjs clean
	make -C quickjs libquickjs.a libquickjs.lto.a CC=$(CC)
	cp quickjs/libquickjs.* $(BIN)

$(OBJDIR)/%.o:%.c
	@mkdir -p $(@D)
	@echo Making C object $@
	@$(CC) $(CFLAGS) -c $^ -o $@

clean:
	@echo Cleaning project
	@rm -rf bin/*
	@rm -f *.gz
	@rm -rf dist/*
