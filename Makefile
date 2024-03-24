MAKEFLAGS = --jobs=4
UNAME != uname
MAKEDIR != pwd
# Options
# NDEBUG --- build with debugging symbols and logging

CXX:=$(CC)

# Temp to strip long emcc paths to just emcc
CC := $(notdir $(CC))

OPT ?= 0

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

ifeq ($(CC), emcc)
  LDFLAGS += #--closure 1 --emrun
  CPPFLAGS += -O0
  OPT = 0
  NDEBUG = 1
  AR = emar
endif

ifdef NDEBUG
  CPPFLAGS += -DNDEBUG
else
  CPPFLAGS += -g -DDUMP
  INFO :=$(INFO)_dbg
endif

ifdef LEAK
  CPPFLAGS += -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -DLEAK
endif

ifeq ($(OPT),small)
  CPPFLAGS += -Oz -flto -fno-ident -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections 

  LDFLAGS += -flto

  ifeq ($(CC), emcc)
    LDFLAGS += --closure 1
  endif

  INFO :=$(INFO)_small
else ifeq ($(OPT), 1)
  CPPFLAGS += -O2 -flto
  INFO :=$(INFO)_opt
else
	CPPFLAGS += -O2
endif

CPPFLAGS += -DHAVE_CEIL -DCP_USE_CGTYPES=0 -DCP_USE_DOUBLES=0 -DHAVE_FLOOR -DHAVE_FMOD -DHAVE_LRINT -DHAVE_LRINTF $(includeflag) -MD $(WARNING_FLAGS) -I. -DVER=\"$(SEM)\" -DCOM=\"$(COM)\" -DINFO=\"$(INFO)\" #-DENABLE_SINC_MEDIUM_CONVERTER -DENABLE_SINC_FAST_CONVERTER -DCP_COLLISION_TYPE_TYPE=uintptr_t -DCP_BITMASK_TYPE=uintptr_t 

CPPFLAGS += -D_FILE_OFFSET_BITS=64 # for tinycdb
CPPFLAGS += -DCONFIG_VERSION=\"2024-02-14\" -DCONFIG_BIGNUM #for quickjs

# ENABLE_SINC_[BEST|FAST|MEDIUM]_CONVERTER
# default, fast and medium available in game at runtime; best available in editor

PKGCMD = tar --directory --exclude="./*.a" --exclude="./obj" -czf $(DISTDIR)/$(DIST) .
ZIP = .tar.gz
UNZIP = cp $(DISTDIR)/$(DIST) $(DESTDIR) && tar xzf $(DESTDIR)/$(DIST) -C $(DESTDIR) && rm $(DESTDIR)/$(DIST)

ifeq ($(ARCH),)
  ARCH != uname -m
endif

INFO :=$(INFO)_$(ARCH)

ifeq ($(OS), Windows_NT) # then WINDOWS
  LDFLAGS += -mwin32 -static
  CPPFLAGS += -mwin32
  LDLIBS += mingw32 kernel32 d3d11 user32 shell32 dxgi gdi32 ws2_32 ole32 winmm setupapi m pthread
  EXT = .exe
  PKGCMD = zip -q -r $(MAKEDIR)/$(DISTDIR)/$(DIST) . -x \*.a ./obj/\*
  ZIP = .zip
  UNZIP = unzip -o -q $(DISTDIR)/$(DIST) -d $(DESTDIR)
	INFO :=$(INFO)_win
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
else ifeq ($(CC), emcc) # Then WEB
  OS := Web
  LDFLAGS += -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -pthread -sTOTAL_MEMORY=128MB
  CPPFLAGS += -pthread
  LDLIBS +=  pthread quickjs GL openal c m dl
  EXT = .html
else 
  UNAME != uname -s
  ifeq ($(UNAME), Linux) # then LINUX
    OS := Linux
    LDFLAGS += -pthread -rdynamic
    LDLIBS += GL pthread c m dl X11 Xi Xcursor EGL asound
		INFO :=$(INFO)_linux
  endif

  ifeq ($(UNAME), Darwin)
    OS := macos
    CPPFLAGS += -arch $(ARCH)
    CFLAGS += -x objective-c
    CXXFLAGS += -std=c++11
    LDFLAGS += -framework Cocoa -framework QuartzCore -framework AudioToolbox -framework Metal -framework MetalKit
		INFO :=$(INFO)_macos
  endif
endif

# All other sources
OBJS != find source/engine -type f -name '*.c' | grep -vE 'test|tool|example|fuzz|main' | grep -vE 'quickjs'
CPPOBJS != find source/engine -type f -name '*.cpp' | grep -vE 'test|tool|example|fuzz|main'
OBJS += $(CPPOBJS)
OBJS += $(shell find source/engine -type f -name '*.m')
OBJS := $(patsubst %.cpp, %$(INFO).o, $(OBJS))
OBJS := $(patsubst %.c, %$(INFO).o,$(OBJS))
OBJS := $(patsubst %.m, %$(INFO).o, $(OBJS))

engineincs != find source/engine -maxdepth 1 -type d
includeflag != find source -type d -name include
includeflag += $(engineincs) source/engine/thirdparty/tinycdb source/shaders source/engine/thirdparty/sokol source/engine/thirdparty/stb source/engine/thirdparty/cgltf source/engine/thirdparty/TinySoundFont source/engine/thirdparty/dr_libs
includeflag := $(addprefix -I, $(includeflag))

WARNING_FLAGS = -Wno-incompatible-function-pointer-types -Wno-incompatible-pointer-types

APP = prosperon
NAME = $(APP)$(INFO)$(EXT)
SEM != git describe --tags --abbrev=0
COM != git rev-parse --short HEAD

LDLIBS := $(addprefix -l, $(LDLIBS))
LDPATHS := $(addprefix -L, $(LDPATHS))

DEPENDS = $(OBJS:.o=.d)
-include $(DEPENDS)

.DEFAULT_GOAL := all
all: $(NAME)
	cp -f $(NAME) $(APP)

DESTDIR ?= ~/.bin

SHADERS = $(shell ls source/shaders/*.sglsl)
SHADERS := $(patsubst %.sglsl, %.sglsl.h, $(SHADERS))

install: $(NAME)
	@echo Copying to destination
	cp -f $(NAME) $(DESTDIR)/$(APP)

$(NAME): libengine$(INFO).a libquickjs$(INFO).a
	@echo Linking $(NAME)
	$(LD) $^ $(CPPFLAGS) $(LDFLAGS) -L. $(LDPATHS) $(LDLIBS) -o $@
	@echo Finished build

libengine$(INFO).a: $(OBJS)
	@echo Archiving $@
	$(AR) rcs $@ $(OBJS)

QUICKJS := source/engine/thirdparty/quickjs
libquickjs$(INFO).a: $(QUICKJS)/libregexp$(INFO).o $(QUICKJS)/quickjs$(INFO).o $(QUICKJS)/libunicode$(INFO).o $(QUICKJS)/cutils$(INFO).o $(QUICKJS)/libbf$(INFO).o
	$(AR) rcs $@ $^
	
%$(INFO).o: %.c $(SHADERS)
	@echo Making C object $@
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

%$(INFO).o: %.cpp
	@echo Making C++ object $@
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

%$(INFO).o: %.m
	@echo Making Objective-C object $@
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

shaders: $(SHADERS)
	@echo Making shaders

%.sglsl.h:%.sglsl
	@echo Creating shader $^
	./sokol-shdc --ifdef -i $^ --slang=glsl330:hlsl5:metal_macos:metal_ios:metal_sim:glsl300es -o $@

SCRIPTS := $(shell ls scripts/*.js*)
CORE != (ls icons/* fonts/*)
CORE := $(CORE) $(SCRIPTS)

CDB = source/engine/thirdparty/tinycdb
CDB_C != find $(CDB) -name *.c
CDB_O := $(patsubst %.c, %.o, $(CDB_C))
CDB_O := $(notdir $(CDB_O))
tools/libcdb.a: $(CDB_C)
	cc -c $^
	$(AR) rcs $@ $(CDB_O)

cdb: tools/cdb.c tools/libcdb.a
	@echo Making cdb
	cc $^ -I$(CDB) -o cdb
	
packer: tools/packer.c tools/libcdb.a
	@echo Making packer
	cc $^ -I$(CDB) -o packer

core.cdb: packer $(CORE)
	@echo Packing core.cdb
	./packer $(CORE)
	chmod 644 out.cdb
	mv out.cdb core.cdb

source/engine/core.cdb.h: core.cdb
	@echo Making $@
	xxd -i $< > $@

jsc: tools/jso.c tools/libquickjs.a
	$(CC) $^ -lm -Iquickjs -o $@

tools/libquickjs.a: $(BIN)/libquickjs.a
	cp -f $(BIN)/libquickjs.a tools

WINCC = x86_64-w64-mingw32-gcc
crosswin: packer
	make CC=$(WINCC) OS=Windows_NT ARCH=x86_64 DEBUG=$(DEBUG) OPT=$(OPT)
	
crossios:
	make OS=IOS ARCH=arm64 DEBUG=$(DEBUG) OPT=$(OPT)

ICNSIZE = 16 32 128 256 512 1024
Prosperon.icns: icons/moon.gif
	mkdir -p Prosperon.iconset
	for i in $(ICNSIZE); do magick icons/moon.gif -size $${i}x$${i} Prosperon.iconset/icon_$${i}x$${i}.png; done
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

crossweb:
	make CC=emcc
	
playweb:
	make crossweb
	emrun $(NAME).html

clean:
	@echo Cleaning project
	rm -f source/shaders/*.h core.cdb jso cdb packer TAGS source/engine/core.cdb.h tools/libcdb.a **.a **.o **.d $(APP)* *.icns
	rm -rf Prosperon.app 

docs: doc/prosperon.org
	make -C doc
	mv doc/html .

TAGINC != find . -name "*.[chj]"
tags: $(TAGINC)
	@echo Making tags.
	@etags $(TAGINC)
