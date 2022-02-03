UNAME != uname

ifeq ($(OS),Windows_NT)
	UNAME = Windows_NT
endif

UNAME_P != uname -m

#CC specifies which compiler we're using
CC = clang
CXX = clang++

ifeq ($(DEBUG), 1)
	DEFFALGS += -DDEBUG
	INFO = dbg
endif

objprefix = ./obj

DIRS = engine pinball editor brainstorm
ETP = ./source/engine/thirdparty/

define make_objs
	find $(1) -type f -name '*.c' -o -name '*.cpp' | sed 's|\.c.*|.o|' | sed 's|\.|./obj|1'
endef

define find_include
	find $(1) -type f -name '*.h'  | sed 's|\/[^\/]*\.h$$||' | sort | uniq
endef

define prefix
	echo $(1) | tr " " "\n" | sed 's|.*|$(2)&$(3)|'
endef

define rm
	tmp=$$(mktemp); \
	echo $(2) | tr " " "\n" > $${tmp}; \
	echo $(1) | tr " " "\n" | grep -v -f $${tmp}; \
	rm $${tmp}
endef

# All other sources
edirs != $(call find_include, ./source/engine)
eobjects != $(call make_objs, ./source/engine)
eobjects != $(call rm,$(eobjects),sqlite s7 pl_mpeg_extract_frames pl_mpeg_player)

imguisrcs = imgui imgui_draw imgui_widgets imgui_tables backends/imgui_impl_sdl backends/imgui_impl_opengl3
imguiobjs != $(call prefix,$(imguisrcs),./source/editor/imgui/,.o)

eddirs != find ./source/editor -type d
edobjects != $(call make_objs, ./source/editor)

bsdirs != find ./source/brainstorm -type d
bsobjects != $(call make_objs, ./source/brainstorm)

pindirs != find ./source/pinball -type d
pinobjects != $(call make_objs, ./source/pinball);

edirs += ./source/engine/thirdparty/Chipmunk2D/include ./source/engine/thirdparty/enet/include
includeflag != $(call prefix,$(edirs) $(eddirs) $(pindirs) $(bsdirs),-I)

#COMPILER_FLAGS specifies the additional compilation options we're using
WARNING_FLAGS = -w #-pedantic -Wall -Wextra -Wwrite-strings
COMPILER_FLAGS = -g -O0 $(WARNING_FLAGS)

LIBPATH = -L.

ifeq ($(UNAME), Windows_NT)
	LINKER_FLAGS = -static -DSDL_MAIN_HANDLED
	ELIBS = engine   mingw32 SDL2main SDL2 m dinput8 dxguid dxerr8 user32 gdi32 winmm imm32 ole32 oleaut32 shell32 version uuid setupapi opengl32 stdc++ winpthread
	ELIBS += SDL2_mixer FLAC vorbis vorbisenc vorbisfile mpg123 out123 syn123 opus opusurl opusfile ogg ssp shlwapi
	CLIBS = glew32
	EXT = .exe
else
	LINKER_FLAGS =
	ELIBS = engine
	CLIBS = SDL2 SDL2_mixer GLEW GL dl pthread
	EXT =
endif

ELIBS != $(call prefix, $(ELIBS), -l)
CLIBS != $(call prefix, $(CLIBS), -l)

LELIBS = -Wl,-Bstatic $(ELIBS) -Wl,-Bdynamic $(CLIBS)

objects = $(bsobjects) $(eobjects) $(pinobjects)
DEPENDS = $(objects:.o=.d)
-include $(DEPENDS)

yuginec = ./source/engine/yugine.c

engine: libengine.a
	Linking engine
	$(CXX) $(yuginec) -DGLEW_STATIC $(includeflag) $(LIBPATH) $(LINKER_FLAGS) $(LELIBS) -o yugine

libengine.a: $(eobjects)
	@echo Making library engine.a

	@ar -r libengine.a $(eobjects)

xbrainstorm: libengine.a $(bsobjects)
	@echo Making brainstorm
	@$(CXX) $(bsobjects) -DGLEW_STATIC $(includeflag) $(LIBPATH) $(LINKER_FLAGS) $(LELIBS) -o $@
	mv xbrainstorm brainstorm/brainstorm$(EXT)

pinball: libengine.a $(pinobjects)
	@echo Making pinball
	@$(CXX) $(pinobjects) -DGLEW_STATIC $(includeflag) $(LIBPATH) $(LINKER_FLAGS) $(LELIBS) -o $@
	mv pinball paladin/pinball

$(objprefix)/%.o:%.cpp
	@mkdir -p $(@D)
	@echo Making C++ object $@
	@$(CXX) $(includeflag) $(COMPILER_FLAGS) -c -MMD -MP $< -o $@

$(objprefix)/%.o:%.c
	@mkdir -p $(@D)
	@echo Making C object $@
	@$(CC) $(includeflag) $(COMPILER_FLAGS) -c -MMD -MP $< -o $@

clean:
	@echo Cleaning project
	@rm -f $(eobjects) $(bsobjects)