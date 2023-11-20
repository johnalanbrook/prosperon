MAKEFLAGS = --jobs=4
UNAME != uname
MAKEDIR != pwd

# Options
# DBG --- build with debugging symbols and logging

CXX:=$(CC)

ifeq ($(CXX), tcc)
  CXX=clang
endif

ifeq ($(CC),cc)
  CC=clang
endif
# Temp to strip long emcc paths to just emcc
CC := $(notdir $(CC))

DBG ?= 1
OPT ?= 0

INFO :=
LD = $(CC)

#ifeq ($(CC), clang)
#  AR = llvm-ar
#endif
ifeq ($(CC), x86_64-w64-mingw32-gcc)
  AR = x86_64-w64-mingw32-ar
endif

ifdef NEDITOR
  CFLAGS += -DNO_EDITOR
endif

ifdef NFLAC
  CFLAGS += -DNFLAC
endif

ifdef NMP3
  CFLAGS += -DNMP3
endif

ifdef NSVG
  CFLAGS += -DNSVG
endif

ifdef NQOA
  CFLAGS += -DNQOA
endif

ifeq ($(DBG),1)
  CFLAGS += -g
  INFO += _dbg
  LDFLAGS += -g
else
  CFLAGS += -DNDEBUG
  LDFLAGS += -s
endif

ifeq ($(OPT),small)
  CFLAGS += -Oz -flto -fno-ident -fno-asynchronous-unwind-tables
  LDFLAGS += -flto

  ifeq ($(CC), emcc)
    LDFLAGS += --closure 1
  endif

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

CFLAGS += -DHAVE_CEIL -DCP_USE_CGTYPES=0 -DCP_USE_DOUBLES=0 -DTINYSPLINE_FLOAT_PRECISION -DHAVE_FLOOR -DHAVE_FMOD -DHAVE_LRINT -DHAVE_LRINTF $(includeflag) -MD $(WARNING_FLAGS) -I. -DVER=\"$(VER)\" -DINFO=\"$(INFO)\"

PKGCMD = tar --directory $(BIN) --exclude="./*.a" --exclude="./obj" -czf $(DISTDIR)/$(DIST) .
ZIP = .tar.gz
UNZIP = cp $(DISTDIR)/$(DIST) $(DESTDIR) && tar xzf $(DESTDIR)/$(DIST) -C $(DESTDIR) && rm $(DESTDIR)/$(DIST)

ifeq ($(ARCH),)
  ARCH != uname -m
endif

STEAMPATH = steam/sdk/redistributable_bin

ifeq ($(OS), Windows_NT)
  LDFLAGS += -mwin32 -static -g
  CFLAGS += -mwin32 -g
  LDLIBS += mingw32 kernel32 d3d11 user32 shell32 dxgi gdi32 ws2_32 ole32 winmm setupapi m
  EXT = .exe
  ARCH := x86_64
  PKGCMD = cd $(BIN); zip -q -r $(MAKEDIR)/$(DISTDIR)/$(DIST) . -x \*.a ./obj/\*
  ZIP = .zip
  UNZIP = unzip -o -q $(DISTDIR)/$(DIST) -d $(DESTDIR)

  ifdef STEAM
    LDPATHS += $(STEAMPATH)/win64
    LDLIBS += steam_api64
  endif

else ifeq ($(OS), ios)
  TTARGET = arm64-apple-ios13.1 
  SYSRT := /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk
  CFLAGS += --target=$(TTARGET) -isysroot $(SYSRT) -DTARGET_OS_IPHONE -x objective-c
  LDFLAGS += --target=$(TTARGET) -isysroot $(SYSRT) -framework Foundation -framework UIKit -framework Metal -framework MetalKit -framework AudioToolbox -framework AVFoundation

else ifeq ($(CC), emcc)
  OS := Web
  LDFLAGS += -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -pthread -sTOTAL_MEMORY=450MB
  CFLAGS += -pthread
  LDLIBS +=  pthread quickjs GL openal c m dl
  CC = emcc
  EXT = .html

else 
  UNAME != uname -s
  ifeq ($(UNAME), Linux)
    OS := Linux
    LDFLAGS += -pthread -rdynamic
    LDLIBS += GL pthread c m dl X11 Xi Xcursor EGL asound

    ifdef STEAM
      LDLIBS += steam_api
      LDPATHS += $(STEAMPATH)/linux64
    endif
  endif

  ifeq ($(UNAME), Darwin)
    OS := macos
    CFLAGS += -arch $(ARCH) -x objective-c
    LDFLAGS += -arch $(ARCH) -framework Cocoa -framework QuartzCore -framework AudioToolbox -framework Metal -framework MetalKit
    ifdef STEAM
      LDPATHS += $(STEAMPATH)/osx
      LDLIBS += steam_api
    endif
  endif
endif

BIN = bin/$(OS)/$(ARCH)$(INFO)

ifdef STEAM
  BIN := $(addsuffix /steam, $(BIN))
endif

OBJDIR = $(BIN)/obj

# All other sources
OBJS != find source/engine -type f -name '*.c'
OBJS += $(shell find source/engine -type f -name '*.cpp')
OBJS += $(shell find source/engine -type f -name '*.m')
OBJS := $(patsubst %.cpp, %.o, $(OBJS))
OBJS := $(patsubst %.c, %.o,$(OBJS))
OBJS := $(patsubst %.m, %.o, $(OBJS))
OBJS := $(addprefix $(BIN)/obj/, $(OBJS))

engineincs != find source/engine -maxdepth 1 -type d
includeflag != find source -type d -name include
includeflag += $(engineincs) source/engine/thirdparty/tinycdb source/shaders
includeflag := $(addprefix -I, $(includeflag))

# Adding different SDKs
ifdef STEAM
  includeflag += -Isteam/sdk/public
  CFLAGS += -DSTEAM
#  BIN += /steam
endif

WARNING_FLAGS = -Wno-incompatible-function-pointer-types -Wno-incompatible-pointer-types -Wno-unused-function -Wno-unused-const-variable -Wno-address-of-temporary

NAME = primum$(EXT)
SEM = 0.0.1
COM != fossil describe
VER = $(SEM)-$(COM)

LDLIBS := $(addprefix -l, $(LDLIBS))
LDPATHS := $(addprefix -L, $(LDPATHS))

DEPENDS = $(OBJS:.o=.d)
-include $(DEPENDS)

DIST = yugine-$(OS)$(ARCH)$(INFO)-$(COM)$(ZIP)
DISTDIR = ./dist

.DEFAULT_GOAL := all
all: $(BIN)/$(NAME)
	cp $(BIN)/$(NAME) .

DESTDIR ?= ~/.bin

CDB = source/engine/thirdparty/tinycdb

SHADERS = $(shell ls source/shaders/*.sglsl)
SHADERS := $(patsubst %.sglsl, %.sglsl.h, $(SHADERS))

install: $(BIN)/$(NAME)
	cp -f $(BIN)/$(NAME) $(DESTDIR)

$(BIN)/$(NAME): $(BIN)/libengine.a $(BIN)/libquickjs.a
	@echo Linking $(NAME)
	$(LD) $^ $(LDFLAGS) -L$(BIN) $(LDPATHS) $(LDLIBS) -o $@
	@echo Finished build

$(DISTDIR)/$(DIST): $(BIN)/$(NAME)
	@echo Creating distribution $(DIST)
	@mkdir -p $(DISTDIR)
	@$(PKGCMD)

$(BIN)/libengine.a: source/engine/core.cdb.h $(OBJS)
	@$(AR) rcs $@ $(OBJS)

$(BIN)/libcdb.a:
	mkdir -p $(BIN)
	rm -f $(CDB)/libcdb.a
	make -C $(CDB) CC=$(CC) AR=$(AR) libcdb.a
	cp $(CDB)/libcdb.a $(BIN)

tools/libcdb.a:
	make -C $(CDB) libcdb.a
	mv $(CDB)/libcdb.a tools


DOCOS = Sound gameobject Game Window physics Profile Time Player Mouse IO Log ColorMap sprite SpriteAnim Render Geometry
DOCHTML := $(addsuffix .api.html, $(DOCOS))
DOCMD := $(addsuffix .api.md, $(DOCOS))

api.md: $(DOCMD)
	@(echo "# API"; cat $^) > $@
	@rm $^

INPUT = editor DebugControls component.sprite component.polygon2d component.edge2d component.circle2d
INPUTMD := $(addsuffix .input.md, $(INPUT))
input.md: $(INPUTMD)
	@(echo "# Input"; cat $^) > $@
	@rm $^

%.input.md: primum $(SCRIPTS)
	@echo Printing controls for $*
	@./primum -e $* > $@

%.api.md: primum $(SCRIPTS)
	@echo Printing api for $*
	@./primum -d $* > $@

$(BIN)/libquickjs.a: $(QUICKJS_O)
	make -C quickjs clean
	make -C quickjs SYSRT=$(SYSRT) TTARGET=$(TTARGET) ARCH=$(ARCH) DBG=$(DBG) OPT=$(OPT) AR=$(AR) OS=$(OS) libquickjs.a libquickjs.lto.a HOST_CC=$(CC)
	@mkdir -p $(BIN)
	cp -rf quickjs/libquickjs.* $(BIN)

$(OBJDIR)/%.o: %.c 
	@mkdir -p $(@D)
	@echo Making C object $@
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(@D)
	@echo Making C++ object $@ with $(CXX)
	@$(CXX) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.m
	@mkdir -p $(@D)
	@echo Making Objective-C object $@
	@$(CC) $(CFLAGS) -c $< -o $@

shaders: $(SHADERS)
	@echo Making shaders

%.sglsl.h:%.sglsl
	@echo Creating shader $^
	@./sokol-shdc --ifdef -i $^ --slang=glsl330:hlsl5:metal_macos:metal_ios:metal_sim:glsl300es -o $@

cdb: tools/cdb.c tools/libcdb.a
	cc $^ -I$(CDB) -o cdb

source/engine/core.cdb.h: core.cdb
	xxd -i $< > $@

SCRIPTS := $(shell ls scripts/*.js)
SCRIPT_O := $(addsuffix o, $(SCRIPTS))
CORE != (ls icons/* fonts/*)
CORE := $(CORE) $(SCRIPTS)

core.cdb: packer $(CORE)
	./packer $(CORE)
	chmod 644 out.cdb
	mv out.cdb core.cdb

CDB_C != find $(CDB) -name *.c
packer: tools/packer.c tools/libcdb.a
	cc $^ -I$(CDB) -o packer

jso: tools/jso.c tools/libquickjs.a
	$(CC) $^ -lm -Iquickjs -o $@

tools/libquickjs.a:
	make -C quickjs clean
	make -C quickjs OPT=$(OPT) AR=$(AR) libquickjs.a
	cp -f quickjs/libquickjs.a tools

%.jso: %.js jso
	@echo Making $@ from $<
	./jso $< > $@

WINCC = x86_64-w64-mingw32-gcc
#WINCC = i686-w64-mingw32-g++
.PHONY: crosswin
crosswin:
	make CC=$(WINCC) OS=Windows_NT

crossmac:
	make ARCH=arm64
	mv primum primum_arm64
	make ARCH=x86_64
	mv primum primum_x86_64
	lipo primum_arm64 primum_x86_64 -create -output primum

clean:
	@echo Cleaning project
	@rm -rf bin dist
	@rm -f shaders/*.sglsl.h shaders/*.metal core.cdb jso cdb packer scripts/*.jso
	@make -C quickjs clean

TAGINC != find . -name "*.[chj]"
tags: $(TAGINC)
	@echo Making tags.
	@etags $(TAGINC)
