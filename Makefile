PROCS != nproc
MAKEFLAGS = --jobs=$(PROCS)

UNAME != uname

# Options
# DBG=0,1 --- build with debugging symbols and logging
# ED=0,1 --- build with or without editor

ED ?= 1
DBG ?= 1

ifeq ($(DBG), 1)
	LVL = -O2 -g
	INFO = dbg

	ifeq ($(CC), tcc)
		LVL +=
	endif

else
	LVL = -O2 -DNDEBUG -flto
	INFO = rel
endif

# QuickJS options

QFLAGS = $(LVL) -DDBG=$(DBG) -DED=$(ED)

PTYPE != uname -m

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

ifeq ($(ED), 1)
	subengs += editor
endif

ifeq ($(DBG), 1)
	subengs += debug
endif


edirs += source/engine $(addprefix source/engine/, $(subengs)) source/engine/thirdparty/Nuklear
ehead != find source/engine source/engine/sound source/engine/debug source/engine/editor -maxdepth 1 -type f -name *.h
eobjects != find source/engine -type f -name '*.c' | sed -r 's|^(.*)\.c|$(objprefix)/\1.o|'  # Gets all .c files and makes .o refs
eobjects != $(call rm,$(eobjects),sqlite pl_mpeg_extract_frames pl_mpeg_player yugine nuklear)

engincs != find source/engine -maxdepth 1 -type d
includeflag != find source -type d -name include
includeflag += $(engincs) source/engine/thirdparty/Nuklear
includeflag := $(addprefix -I, $(includeflag))

WARNING_FLAGS = -Wall# -pedantic -Wextra -Wwrite-strings -Wno-incompatible-function-pointer-types -Wno-incompatible-pointer-types -Wno-unused-function

SEM = 0.0.1
COM != git rev-parse --short HEAD
VER = $(SEM)-$(COM)

COMPILER_FLAGS = $(includeflag) $(QFLAGS) -MD $(WARNING_FLAGS) -DCP_USE_DOUBLES=0 -DTINYSPLINE_FLOAT_PRECISION -DVER=\"$(VER)\" -DINFO=\"$(INFO)\" -march=native -std=c99 -c $< -o $@

LIBPATH = -L$(BIN)

ifeq ($(OS), WIN32)
	LINKER_FLAGS = $(QFLAGS) -static
	ELIBS = engine ucrt yughc glfw3 opengl32 gdi32 ws2_32 ole32 winmm setupapi m
	CLIBS =
	EXT = .exe
else
	LINKER_FLAGS = $(QFLAGS) -L/usr/local/lib -rdynamic
	ELIBS =  engine pthread yughc glfw3 quickjs tcc c m dl
	CLIBS =
endif

NAME = yugine$(EXT)

ELIBS != $(call prefix, $(ELIBS), -l)
CLIBS != $(call prefix, $(CLIBS), -l);

objects = $(eobjects)
DEPENDS = $(objects:.o=.d)
-include $(DEPENDS)

ENGINE = $(BIN)libengine.a
INCLUDE = $(BIN)include

LINK = $(LIBPATH) $(LINKER_FLAGS) $(ELIBS)

MYTAG = $(VER)_$(PTYPE)_$(INFO)

DIST = $(NAME)-$(MYTAG).tar.gz

yugine: $(BIN)yugine

$(NAME): $(BIN)$(NAME)

$(BIN)$(NAME): $(objprefix)/source/engine/yugine.o $(ENGINE) $(BIN)libquickjs.a
	@echo Linking $(NAME)
	$(CC) $< $(LINK) -o $(BIN)$(NAME)
	@echo Finished build

$(BIN)$(DIST): $(BIN)$(NAME) source/shaders/* source/scripts/* assets/*
	@echo Creating distribution $(DIST)
	@mkdir -p $(BIN)dist
	@cp $(BIN)$(NAME) $(BIN)dist
	@cp -rf assets/* $(BIN)dist
	@cp -rf source/shaders $(BIN)dist
	@cp -r source/scripts $(BIN)dist
	@tar czf $(DIST) --directory $(BIN)dist .
	@mv $(DIST) $(BIN)

$(BIN)libquickjs.a:
	make -C quickjs clean
	make -C quickjs libquickjs.a libquickjs.lto.a CC=$(CC)
	cp quickjs/libquickjs.* $(BIN)

dist: $(BIN)$(DIST)

install: $(BIN)$(DIST)
	@echo Unpacking $(DIST) in $(DESTDIR)
	@cp $(BIN)$(DIST) $(DESTDIR)
	@tar xzf $(DESTDIR)/$(DIST) -C $(DESTDIR)
	@rm $(DESTDIR)/$(DIST)

$(ENGINE): $(eobjects)
	@echo Making library engine.a
	@ar r $(ENGINE) $(eobjects)
	@mkdir -p $(INCLUDE)
	@cp -u -r $(ehead) $(INCLUDE)

$(objprefix)/%.o:%.c
	@mkdir -p $(@D)
	@echo Making C object $@
	@$(CC) $(COMPILER_FLAGS)

.PHONY: clean
clean:
	@echo Cleaning project
	@rm -rf bin/*
	@rm -f *.gz
