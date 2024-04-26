MAKEFLAGS = --jobs=8
UNAME != uname
MAKEDIR != pwd
# Options
# NDEBUG --- build with debugging symbols and logging

ifeq ($(ARCH),)
	ARCH != uname -m
endif

CXX:=$(CC)

OPT ?= 0
LD = $(CC)

STEAM = steam/sdk
STEAMAPI = 

ifeq ($(CROSS)$(CC), emcc)
	LDFLAGS += --shell-file shell.html --closure 1
	CPPFLAGS += -Wbad-function-cast -Wcast-function-type -sSTACK_SIZE=1MB -sALLOW_MEMORY_GROWTH -sINITIAL_MEMORY=128MB
	NDEBUG = 1
	ARCH:= wasm
	OPT=small
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

ifdef NDEBUG
  CPPFLAGS += -DNDEBUG
else
  CPPFLAGS += -g -DDUMP
  INFO :=$(INFO)_dbg
endif

ifdef LEAK
  CPPFLAGS += -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -DLEAK
	INFO := $(INFO)_leak
endif

ifeq ($(OPT),small)
  CPPFLAGS += -Os -flto -fno-ident -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections

  LDFLAGS += -flto

  INFO :=$(INFO)_small
else ifeq ($(OPT), 1)
  CPPFLAGS += -O3 -flto
  INFO :=$(INFO)_opt
else
	CPPFLAGS += -O0
endif

CPPFLAGS += -DHAVE_CEIL -DCP_USE_CGTYPES=0 -DCP_USE_DOUBLES=0 -DHAVE_FLOOR -DHAVE_FMOD -DHAVE_LRINT -DHAVE_LRINTF $(includeflag) -MD $(WARNING_FLAGS) -I. -DVER=\"$(SEM)\" -DCOM=\"$(COM)\" -DINFO=\"$(INFO)\" #-DENABLE_SINC_MEDIUM_CONVERTER -DENABLE_SINC_FAST_CONVERTER -DCP_COLLISION_TYPE_TYPE=uintptr_t -DCP_BITMASK_TYPE=uintptr_t 
CPPFLAGS += -DCONFIG_VERSION=\"2024-02-14\" -DCONFIG_BIGNUM #for quickjs

# ENABLE_SINC_[BEST|FAST|MEDIUM]_CONVERTER
# default, fast and medium available in game at runtime; best available in editor

PKGCMD = tar --directory --exclude="./*.a" --exclude="./obj" -czf $(DISTDIR)/$(DIST) .
ZIP = .tar.gz
UNZIP = cp $(DISTDIR)/$(DIST) $(DESTDIR) && tar xzf $(DESTDIR)/$(DIST) -C $(DESTDIR) && rm $(DESTDIR)/$(DIST)

INFO := $(INFO)_$(ARCH)

ifeq ($(OS), Windows_NT) # then WINDOWS
	PLATFORM := win64
  DEPS += resource.o
	STEAMAPI := steam_api64
  LDFLAGS += -mwin32 -static
  CPPFLAGS += -mwin32
  LDLIBS += mingw32 kernel32 d3d11 user32 shell32 dxgi gdi32 ws2_32 ole32 winmm setupapi m pthread
  PKGCMD = zip -q -r $(MAKEDIR)/$(DISTDIR)/$(DIST) . -x \*.a ./obj/\*
  ZIP = .zip
  UNZIP = unzip -o -q $(DISTDIR)/$(DIST) -d $(DESTDIR)
	INFO :=$(INFO)_win
	EXT = .exe
else ifeq ($(OS), IOS)
  CC = /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
	SDK = iphoneos
	SDK_PATH = /Applications/Xcode.app/Contents/Developer/Platforms/$(SDK).platform/Developer/SDKs/$(SDK).sdk
	CFLAGS += -isysroot $(SDK_PATH) -miphoneos-version-min=13.0
	LDFLAGS += -isysroot $(SDK_PATH) -miphoneos-version-min=13.0
	LDFLAGS += -framework Foundation -framework UIKit -framework AudioToolbox -framework Metal -framework MetalKit -framework AVFoundation
	CXXFLAGS += -std=c++11
	CFLAGS += -x objective-c
	INFO :=$(INFO)_ios
else ifeq ($(OS), wasm) # Then WEB
  OS := Web
  LDFLAGS += -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2
  CPPFLAGS += -DNSTEAM
  LDLIBS += GL openal c m dl
	STEAMAPI := 
  EXT = .html
else 
  UNAME != uname -s
  ifeq ($(UNAME), Linux) # then LINUX
    OS := Linux
		PLATFORM := linux64
    LDFLAGS += -pthread -rdynamic
    LDLIBS += GL pthread c m dl X11 Xi Xcursor EGL asound
		INFO :=$(INFO)_linux
  endif

  ifeq ($(UNAME), Darwin)
    OS := macos
		PLATFORM := osx
    CPPFLAGS += -arch $(ARCH)
    CFLAGS += -x objective-c
    CXXFLAGS += -std=c++11
    LDFLAGS += -framework Cocoa -framework QuartzCore -framework AudioToolbox -framework Metal -framework MetalKit
		INFO :=$(INFO)_macos
  endif
endif

# All other sources
OBJS != find source -type f -name '*.c' | grep -vE 'test|tool|example|fuzz|main' | grep -vE 'quickjs'
CPPOBJS != find source -type f -name '*.cpp' | grep -vE 'test|tool|example|fuzz|main'
OBJS += $(CPPOBJS)
OBJS += source/engine/yugine.c
OBJS += $(shell find source/engine -type f -name '*.m')
QUICKJS := source/engine/thirdparty/quickjs
OBJS += $(addprefix $(QUICKJS)/, libregexp.c quickjs.c libunicode.c cutils.c libbf.c)
OBJS := $(patsubst %.cpp, %$(INFO).o, $(OBJS))
OBJS := $(patsubst %.c, %$(INFO).o,$(OBJS))
OBJS := $(patsubst %.m, %$(INFO).o, $(OBJS))

engineincs != find source/engine -maxdepth 1 -type d
includeflag != find source -type d -name include
includeflag += $(engineincs) source/shaders source/engine/thirdparty/sokol source/engine/thirdparty/stb source/engine/thirdparty/cgltf source/engine/thirdparty/TinySoundFont source/engine/thirdparty/dr_libs
includeflag += $(STEAM)/public
includeflag += source
includeflag := $(addprefix -I, $(includeflag))

WARNING_FLAGS = -Wno-incompatible-function-pointer-types -Wno-incompatible-pointer-types

ifeq ($(INFO),_)
	INFO := 
endif
APP = prosperon
NAME = $(APP)$(INFO)$(EXT)
SEM != git describe --tags --abbrev=0
COM != git rev-parse --short HEAD

LDLIBS += $(STEAMAPI)
LDLIBS := $(addprefix -l, $(LDLIBS))
LDPATHS := $(STEAM)/redistributable_bin/$(PLATFORM)
LDPATHS := $(addprefix -L, $(LDPATHS))

DEPENDS = $(OBJS:.o=.d)
-include $(DEPENDS)

.DEFAULT_GOAL := all
all: $(NAME)
	cp -f $(NAME) $(APP)$(EXT)

SHADERS = $(shell ls source/shaders/*.sglsl)
SHADERS := $(patsubst %.sglsl, %.sglsl.h, $(SHADERS))

prereqs: $(SHADERS) source/engine/core.cdb.h

DESTDIR ?= ~/.bin
install: $(NAME)
	@echo Copying to destination
	cp -f $(NAME) $(DESTDIR)/$(APP)

$(NAME): $(OBJS) $(DEPS)
	@echo Linking $(NAME)
	$(CROSS)$(LD) $^ $(CPPFLAGS) $(LDFLAGS) -L. $(LDPATHS) $(LDLIBS) -o $@
	@echo Finished build

%$(INFO).o: %.c 
	@echo Making C object $@
	$(CROSS)$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

%$(INFO).o: %.cpp
	@echo Making C++ object $@
	$(CROSS)$(CXX) $(CPPFLAGS) $(CXXFLAGS) -fpermissive -c $< -o $@

%$(INFO).o: %.m
	@echo Making Objective-C object $@
	$(CROSS)$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

shaders: $(SHADERS)
	@echo Making shaders

%.sglsl.h:%.sglsl
	@echo Creating shader $^
	./sokol-shdc -r --ifdef -i $^ --slang=glsl330:hlsl5:metal_macos:metal_ios:metal_sim:glsl300es -o $@

SCRIPTS := $(shell ls scripts/*.js*)
CORE != (ls icons/* fonts/*)
CORE := $(CORE) $(SCRIPTS)

packer: tools/packer.c source/engine/miniz.c
	@echo Making packer
	$(CC) -O2 $^ -Isource/engine -o packer

core.cdb: packer $(CORE)
	@echo Packing core.cdb
	./packer $@ $(CORE)

source/engine/core.cdb.h: core.cdb
	@echo Making $@
	xxd -i $< > $@

ICNSIZE = 16 32 128 256 512 1024
ICNNAME := $(addsuffix .png, $(ICNSIZE))
ICON = icons/moon.gif
icon.ico: $(ICON)
	for i in $(ICNSIZE); do convert $^ -thumbnail $${i}x$${i} $${i}.png; done
	convert $(ICNNAME) icon.ico
	rm $(ICNNAME)

resource.o: resource.rc resource.manifest icon.ico
	$(CROSS)windres -i $< -o $@

crossios:
	make OS=IOS ARCH=arm64 DEBUG=$(DEBUG) OPT=$(OPT)

Prosperon.icns: $(ICON)
	mkdir -p Prosperon.iconset
	for i in $(ICNSIZE); do magick $^ -size $${i}x$${i} Prosperon.iconset/icon_$${i}x$${i}.png; done
	iconutil -c icns Prosperon.iconset

crossmac: Prosperon.icns
	make ARCH=arm64 DEBUG=$(DEBUG) OPT=$(OPT)
	mv $(APP) mac_arm64
	make ARCH=x86_64 DEBUG=$(DEBUG) OPT=$(OPT)
	mv $(APP) mac_x86_64
	lipo mac_arm64 mac_x86_64 -create -output $(APP)_mac
	rm mac_arm64 mac_x86_64
	rm -rf Prosperon.app
	mkdir Prosperon.app
	mkdir Prosperon.app/Contents
	mkdir Prosperon.app/Contents/MacOS
	mkdir Prosperon.app/Contents/Resources
	mv $(NAME) Prosperon.app/Contents/MacOS/Prosperon
	cp Info.plist Prosperon.app/Contents
	cp Prosperon.icns Prosperon.app/Contents/Resources
	
crosswin:
	make CROSS=x86_64-w64-mingw32- OS=Windows_NT CC=gcc

crossweb:
	make CROSS=em OS=wasm
	mv $(APP).html index.html
	
clean:
	@echo Cleaning project
	rm -f source/shaders/*.h core.cdb jso cdb packer TAGS source/engine/core.cdb.h tools/libcdb.a $(APP)* *.icns *.ico
	find . -type f -name "*.[oad]" -delete
	rm -rf Prosperon.app 

docs: doc/prosperon.org
	make -C doc
	mv doc/html .

TAGINC != find . -name "*.[chj]"
tags: $(TAGINC)
	@echo Making tags.
	@etags $(TAGINC)
