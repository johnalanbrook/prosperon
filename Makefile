MAKEFLAGS := --jobs=$(shell nproc)

UNAME := $(shell uname)

ifeq ($(OS),Windows_NT)
	UNAME := Windows_NT
endif

UNAME_P := $(shell uname -m)

#CC specifies which compiler we're using
CC = clang -x c --std=c99
CXX = clang++

ifeq ($(DEBUG), 1)
	DEFFALGS += -DDEBUG
	INFO = dbg
endif

BINDIR := ./bin
BUILDDIR := ./obj

objprefix = ./obj

DIRS := engine pinball editor brainstorm

# All other sources
esrcs := $(shell find ./source/engine -name '*.c*')
esrcs := $(filter-out %sqlite3.c %shell.c %s7.c, $(esrcs))
eheaders := $(shell find ./source/engine -name '*.h')
edirs := $(shell find ./source/engine -type d)
edirs := $(filter-out %docs %doc %include% %src %examples  , $(edirs))

eobjects := $(sort $(patsubst .%.cpp, $(objprefix)%.o, $(filter %.cpp, $(esrcs))) $(patsubst .%.c, $(objprefix)%.o, $(filter %.c, $(esrcs))))

edsrcs := $(shell find ./source/editor -name '*.c*')
edheaders := $(shell find ./source/editor -name '*.h')
eddirs := $(shell find ./source/editor -type d)

edobjects := $(sort $(patsubst .%.cpp, $(objprefix)%.o, $(filter %.cpp, $(edsrcs))) $(patsubst .%.c, $(objprefix)%.o, $(filter %.c, $(edsrcs))))

bssrcs := $(shell find ./source/brainstorm -name '*.c*')
bsheaders := $(shell find ./source/brainstorm -name '*.h')
eddirs := $(shell find ./source/brainstorm -type d)

bsobjects := $(sort $(patsubst .%.cpp, $(objprefix)%.o, $(filter %.cpp, $(bssrcs))) $(patsubst .%.c, $(objprefix)%.o, $(filter %.c, $(bssrcs))))

pinsrcs := $(shell find ./source/editor -name '*.c*')
pinheaders := $(shell find ./source/editor -name '*.h')
pindirs := $(shell find ./source/pinball -type d)

edobjects := $(sort $(patsubst .%.cpp, $(objprefix)%.o, $(filter %.cpp, $(edsrcs))) $(patsubst .%.c, $(objprefix)%.o, $(filter %.c, $(edsrcs))))

includeflag := $(addprefix -I, $(edirs) $(eddirs) $(pindirs) $(bsdirs))

#COMPILER_FLAGS specifies the additional compilation options we're using
WARNING_FLAGS := -w #-pedantic -Wall -Wextra -Wwrite-strings
COMPILER_FLAGS := -g -O0 $(WARNING_FLAGS)


ifeq ($(UNAME), Windows_NT)
	LINKER_FLAGS:= -static
	# DYNAMIC LIBS: SDL2 opengl32
	ELIBS := glew32 mingw32 SDL2main SDL2 m dinput8 dxguid dxerr8 user32 gdi32 winmm imm32 ole32 oleaut32 shell32 version uuid setupapi opengl32 stdc++ winpthread
	CLIBS :=
	EXT := .exe
else
	LINKER_FLAGS :=
	ELIBS :=
	CLIBS := SDL2 SDL2_mixer GLEW GL dl pthread
	EXT :=
endif

LELIBS := -Wl,-Bstatic $(addprefix -l, ${ELIBS}) -Wl,-Bdynamic $(addprefix -l, $(CLIBS))

dir_guard = @mkdir -p $(@D)

FILENAME = $(TARGET)$(INFO)_$(UNAME_P)$(EXT)

eobjects := $(filter-out %yugine.o, $(eobjects))

.SECONARY: $(eobjects)

engine: engine.a
	@echo Linking engine
	-$(CXX) $^ -DGLEW_STATIC $(LINKER_FLAGS) $(LELIBS) -o $@

engine.a: $(eobjects)
	@echo Making library engine.a
	-@ar -rv engine.a $(eobjects)

brainstorms: engine.a $(bsobjects)
	@echo Making brainstorm
	$(CXX) $^ -DGLEW_STATIC $(LINKER_FLAGS) $(LELIBS) -o $@

$(objprefix)/%.o:%.cpp
	$(dir_guard)
	@echo Making C++ object $@
	-@$(CXX) $(includeflag) $(COMPILER_FLAGS) -c $< -o $@

$(objprefix)/%.o:%.c
	$(dir_guard)
	@echo Making C object $@
	$(CC) $(includeflag) $(COMPILER_FLAGS) -c $< -o $@

clean:
	@echo Cleaning project
	@rm -f $(eobjects)

