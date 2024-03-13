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
LEAK ?= 0

INFO :=
LD = $(CC)

ifeq ($(CC), x86_64-w64-mingw32-gcc)
  AR = x86_64-w64-mingw32-ar
endif

ifdef NEDITOR
  CPPFLAGS += -DNO_EDITOR
endif

ifdef NFLAC
  CPPFLAGS += -DNFLAC
endif

ifdef NMP3
  CPPFLAGS += -DNMP3
endif

ifdef NSVG
  CPPFLAGS += -DNSVG
endif

ifdef NQOA
  CPPFLAGS += -DNQOA
endif

CPPFLAGS += -ffast-math

ifeq ($(CC), emcc)
  LDFLAGS += #--closure 1
  CPPFLAGS += -O0
  OPT = 0
  DBG = 0
  AR = emar
endif

ifeq ($(DBG),1)
  CPPFLAGS += -g
  INFO += _dbg
else
  CPPFLAGS += -DNDEBUG
  LDFLAGS += -s
endif

ifeq ($(LEAK),1)
  CPPFLAGS += -fsanitize=address
endif

CPPFLAGS += -DLEAK=$(LEAK)

ifeq ($(OPT),small)
  CPPFLAGS += -Oz -flto -fno-ident -fno-asynchronous-unwind-tables
  LDFLAGS += -flto

  ifeq ($(CC), emcc)
    LDFLAGS += --closure 1
  endif

  INFO := $(addsuffix _small,$(INFO))
else
  ifeq ($(OPT), 1)
    CPPFLAGS += -O2 -flto
    INFO := $(addsuffix _opt,$(INFO))
  else
    CPPFLAGS += -O0
  endif
endif

CPPFLAGS += -DHAVE_CEIL -DCP_USE_CGTYPES=0 -DCP_USE_DOUBLES=0 -DHAVE_FLOOR -DHAVE_FMOD -DHAVE_LRINT -DHAVE_LRINTF $(includeflag) -MD $(WARNING_FLAGS) -I. -DVER=\"$(VER)\" -DCOM=\"$(COM)\" -DINFO=\"$(INFO)\" #-DENABLE_SINC_MEDIUM_CONVERTER -DENABLE_SINC_FAST_CONVERTER -DCP_COLLISION_TYPE_TYPE=uintptr_t -DCP_BITMASK_TYPE=uintptr_t

CPPFLAGS += -D_FILE_OFFSET_BITS=64 # for tinycdb

# ENABLE_SINC_[BEST|FAST|MEDIUM]_CONVERTER
# default, fast and medium available in game at runtime; best available in editor

PKGCMD = tar --directory $(BIN) --exclude="./*.a" --exclude="./obj" -czf $(DISTDIR)/$(DIST) .
ZIP = .tar.gz
UNZIP = cp $(DISTDIR)/$(DIST) $(DESTDIR) && tar xzf $(DESTDIR)/$(DIST) -C $(DESTDIR) && rm $(DESTDIR)/$(DIST)

ifeq ($(ARCH),)
  ARCH != uname -m
endif

STEAMPATH = steam/sdk/redistributable_bin
DISCORDPATH = discord/lib

ifdef DISCORD
  LDPATHS += $(DISCORDPATH)/$(ARCH)
  LDLIBS += discord_game_sdk
  CPPFLAGS += -DDISCORD
endif

ifdef STEAM
  LDLIBS += steam_api
  LDPATHS += $(STEAMPATH)/$(ARCH)
endif

ifeq ($(OS), Windows_NT)
  LDFLAGS += -mwin32 -static
  CPPFLAGS += -mwin32
  LDLIBS += mingw32 kernel32 d3d11 user32 shell32 dxgi gdi32 ws2_32 ole32 winmm setupapi m pthread
  EXT = .exe
  ARCH := x86_64
  PKGCMD = cd $(BIN); zip -q -r $(MAKEDIR)/$(DISTDIR)/$(DIST) . -x \*.a ./obj/\*
  ZIP = .zip
  UNZIP = unzip -o -q $(DISTDIR)/$(DIST) -d $(DESTDIR)
else ifeq ($(CC), emcc)
  OS := Web
  LDFLAGS += -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -pthread -sTOTAL_MEMORY=128MB
  CPPFLAGS += -pthread
  LDLIBS +=  pthread quickjs GL openal c m dl
  EXT = .html
else 
  UNAME != uname -s
  ifeq ($(UNAME), Linux)
    OS := Linux
    LDFLAGS += -pthread -rdynamic
    LDLIBS += GL pthread c m dl X11 Xi Xcursor EGL asound
  endif

  ifeq ($(UNAME), Darwin)
    OS := macos
    CPPFLAGS += -arch $(ARCH)
    CFLAGS += -x objective-c
    CXXFLAGS += -std=c++11
    LDFLAGS += -framework Cocoa -framework QuartzCore -framework AudioToolbox -framework Metal -framework MetalKit
  endif
endif

BIN = bin/$(OS)/$(ARCH)$(INFO)

ifdef STEAM
  BIN := $(addsuffix /steam, $(BIN))
endif

OBJDIR = $(BIN)/obj

# All other sources
OBJS != find source/engine -type f -name '*.c' | grep -vE 'test|tool|example|fuzz|main'
CPPOBJS != find source/engine -type f -name '*.cpp' | grep -vE 'test|tool|example|fuzz|main'
OBJS += $(CPPOBJS)
OBJS += $(shell find source/engine -type f -name '*.m')
OBJS := $(patsubst %.cpp, %.o, $(OBJS))
OBJS := $(patsubst %.c, %.o,$(OBJS))
OBJS := $(patsubst %.m, %.o, $(OBJS))
OBJS := $(addprefix $(BIN)/obj/, $(OBJS))

engineincs != find source/engine -maxdepth 1 -type d
includeflag != find source -type d -name include
includeflag += $(engineincs) source/engine/thirdparty/tinycdb source/shaders source/engine/thirdparty/sokol source/engine/thirdparty/stb source/engine/thirdparty/cgltf source/engine/thirdparty/TinySoundFont source/engine/thirdparty/dr_libs
includeflag := $(addprefix -I, $(includeflag))

# Adding different SDKs
ifdef STEAM
  includeflag += -Isteam/sdk/public
  CPPFLAGS += -DSTEAM
#  BIN += /steam
endif

WARNING_FLAGS = -Wno-incompatible-function-pointer-types -Wno-incompatible-pointer-types

NAME = primum$(EXT)
SEM = 0.3.0
COM != fossil describe
VER = $(SEM)

LDLIBS := $(addprefix -l, $(LDLIBS))
LDPATHS := $(addprefix -L, $(LDPATHS))

DEPENDS = $(OBJS:.o=.d)
-include $(DEPENDS)

DIST = yugine-$(OS)$(ARCH)$(INFO)-$(COM)$(ZIP)
DISTDIR = ./dist

.DEFAULT_GOAL := all
primum: all
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
	$(LD) $^ $(CPPFLAGS) $(LDFLAGS) -L$(BIN) $(LDPATHS) $(LDLIBS) -o $@
	@echo Finished build

$(DISTDIR)/$(DIST): $(BIN)/$(NAME)
	@echo Creating distribution $(DIST)
	@mkdir -p $(DISTDIR)
	@$(PKGCMD)

$(BIN)/libengine.a: $(OBJS) 
	@$(AR) rcs $@ $(OBJS)

CDB_C != find $(CDB) -name *.c
CDB_O := $(patsubst %.c, %.o, $(CDB_C))
$(CDB)/libcdb.a:
	rm -f $(CDB)/libcdb.a
	make -C $(CDB) libcdb.a 

tools/libcdb.a: $(CDB)/libcdb.a
	cp $(CDB)/libcdb.a tools

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

$(BIN)/libquickjs.a: 
	make -C quickjs clean
	make -C quickjs SYSRT=$(SYSRT) TTARGET=$(TTARGET) ARCH=$(ARCH) DBG=$(DBG) OPT=$(OPT) AR=$(AR) OS=$(OS) libquickjs.a HOST_CC=$(CC) LEAK=$(LEAK)
	@mkdir -p $(BIN)
	cp -rf quickjs/libquickjs.* $(BIN)

$(OBJDIR)/%.o: %.c source/engine/core.cdb.h $(SHADERS)
	@mkdir -p $(@D)
	@echo Making C object $@
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(@D)
	@echo Making C++ object $@
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.m
	@mkdir -p $(@D)
	@echo Making Objective-C object $@
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

shaders: $(SHADERS)
	@echo Making shaders

%.sglsl.h:%.sglsl
	@echo Creating shader $^
	@./sokol-shdc --ifdef -i $^ --slang=glsl330:hlsl5:metal_macos:metal_ios:metal_sim:glsl300es -o $@

cdb: tools/cdb.c tools/libcdb.a
	cc $^ -I$(CDB) -o cdb

source/engine/core.cdb.h: core.cdb
	xxd -i $< > $@

SCRIPTS := $(shell ls scripts/*.js*)
SCRIPT_O := $(addsuffix o, $(SCRIPTS))
CORE != (ls icons/* fonts/*)
CORE := $(CORE) $(SCRIPTS)

core.cdb: packer $(CORE)
	./packer $(CORE)
	chmod 644 out.cdb
	mv out.cdb core.cdb

packer: tools/packer.c tools/libcdb.a
	cc $^ -I$(CDB) -o packer

jsc: tools/jso.c tools/libquickjs.a
	$(CC) $^ -lm -Iquickjs -o $@

tools/libquickjs.a: $(BIN)/libquickjs.a
	cp -f $(BIN)/libquickjs.a tools

WINCC = x86_64-w64-mingw32-gcc
#WINCC = i686-w64-mingw32-g++
.PHONY: crosswin
crosswin:
	make packer
	make CC=$(WINCC) OS=Windows_NT

crossmac:
	make ARCH=arm64
	mv primum primum_arm64
	make ARCH=x86_64
	mv primum primum_x86_64
	lipo primum_arm64 primum_x86_64 -create -output primum

crossweb:
	make packer
	make CC=emcc

clean:
	@echo Cleaning project
	rm -rf bin dist
	rm -f source/shaders/*.h core.cdb jso cdb packer TAGS source/engine/core.cdb.h tools/libcdb.a $(CDB)/libcdb.a
	rm -f $(CDB)/*.o
	@make -C quickjs clean

docs: doc/prosperon.org
	make -C doc
	mv doc/html .

test:
	@echo No tests yet ...

TAGINC != find . -name "*.[chj]"
tags: $(TAGINC)
	@echo Making tags.
	@etags $(TAGINC)
