PROCS != nproc
MAKEFLAGS = --jobs=$(PROCS)

UNAME != uname

QFLAGS = -O3 -DDBG=0 -DED=1
INFO = rel
PTYPE != uname -m

# Options
# DBG=0,1 --- build with debugging symbols and logging
# ED=0,1 --- build with or without editor


ifeq ($(DBG), 1)
	QFLAGS = -O0 -g -DDBG=1 -DED=1
	ifeq ($(CC), tcc)
		QFLAGS += -b -bt24
	endif
	INFO = dbg
endif

ifeq 	($(ED), 0)
	QFLAGS = -DED=0
	INFO = ed
endif

BIN = bin/$(CC)/
objprefix = $(BIN)obj/$(INFO)

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
subengs = sound debug editor 3d
edirs += source/engine $(addprefix source/engine/, $(subengs)) source/engine/thirdparty/Nuklear
ehead != find source/engine source/engine/sound source/engine/debug source/engine/editor -maxdepth 1 -type f -name *.h
eobjects != find source/engine -type f -name '*.c' | sed -r 's|^(.*)\.c|$(objprefix)/\1.o|'  # Gets all .c files and makes .o refs
eobjects != $(call rm,$(eobjects),sqlite pl_mpeg_extract_frames pl_mpeg_player yugine nuklear)

includeflag != $(call prefix,$(edirs),-I)

WARNING_FLAGS = -Wall# -pedantic -Wextra -Wwrite-strings  -Wno-incompatible-function-pointer-types -Wno-incompatible-pointer-types -Wno-unused-function

SEM = 0.0.1
COM != git rev-parse --short HEAD
VER = $(SEM)-$(COM)

COMPILER_FLAGS = $(includeflag) $(QFLAGS) -MD $(WARNING_FLAGS) -DVER=\"$(VER)\" -DINFO=\"$(INFO)\" -c $< -o $@

LIBPATH = -L$(BIN)

ifeq ($(OS), WIN32)
	LINKER_FLAGS = $(QFLAGS)
	ELIBS = engine ucrt pthread yughc portaudio glfw3 opengl32 gdi32 ws2_32 ole32 winmm setupapi m
	CLIBS =
	EXT = .exe
else
	LINKER_FLAGS = $(QFLAGS) -L/usr/local/lib
	ELIBS =  engine pthread yughc portaudio asound glfw3 c m dl
	CLIBS =
	EXT =
endif

ELIBS != $(call prefix, $(ELIBS), -l)
ELIBS := $(CLIBS) $(ELIBS)


objects = $(eobjects)
DEPENDS = $(objects:.o=.d)
-include $(DEPENDS)

yuginec = source/engine/yugine.c

ENGINE = $(BIN)libengine.a
INCLUDE = $(BIN)include

LINK = $(LIBPATH) $(LINKER_FLAGS) $(ELIBS)

MYTAG = $(VER)_$(PTYPE)_$(INFO)

DIST = yugine-$(MYTAG).tar.gz

.PHONY: yugine

yugine: $(BIN)yugine

$(BIN)yugine: $(objprefix)/source/engine/yugine.o $(ENGINE)
	@echo Linking yugine
	$(CC) $< $(LINK) -o $(BIN)yugine
	@echo Finished build

$(BIN)$(DIST): $(BIN)yugine
	@echo Creating distribution zip
	@mkdir -p $(BIN)dist
	@cp $(BIN)yugine $(BIN)dist
	@cp -rf assets/fonts $(BIN)dist
	@cp -rf source/scripts $(BIN)dist
	@cp -rf source/shaders $(BIN)dist
	@tar czf $(DIST) --directory $(BIN)dist .
	@mv $(DIST) $(BIN)


dist: $(BIN)$(DIST)

install: $(BIN)$(DIST)
	@echo Unpacking distribution in $(DESTDIR)
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
