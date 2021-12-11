# Project output name
PROJNAME	= 5.0-movegen

# Project directory structure
SRCDIR		= ./src/
INCDIR		= ./src/
RELOBJDIR	= ./build/
DBGOBJDIR	= ./build/debug/
PRFOBJDIR	= ./build/gprof/
OUTDIR		= ./out/
LSTDIR		= ./lst/
ARCHDIR		= ../arch/src/
LOGDIR		= ./test/log/

MODULES		= build commands evaluate fen history io log moves movegen options search state ui

#	Build tools
TOOLCHAIN	= 
CC     		= $(TOOLCHAIN)gcc
LD      	= $(TOOLCHAIN)gcc
RM		= rm -rf
MKDIR		= mkdir

# 	Flags for build tools
#	General
INCFLAGS	= -I$(INCDIR)
CFLAGS  	= -c -std=c99 -Wfatal-errors -Wall -Werror
LFLAGS  	= -lrt
#	Configuration-specific
RELCFLAGS	= -g -O3 -msse4 -DLOGGING=NO
RELLFLAGS	= -g
DBGCFLAGS	= -g -O0 -DLOGGING=YES
DBGLFLAGS	= -rdynamic
PRFCFLAGS	= -pg -g -Wa,-adhln $(RELCFLAGS)
PRFLFLAGS	= -pg $(DGBCFLAGS)

#	OS-specific stuff
ifeq ($(OS),Windows_NT)
CFLAGS		+= -D OS=WIN
MODULES		+= win
else
CFLAGS		+= -D OS=POSIX
MODULES		+= posix
endif

COMPNAME	:= $(shell gcc --version | grep 'gcc')
TARGETNAME	:= $(shell gcc -dumpmachine)
BUILDFLAGS	:= -DCOMPILER_NAME="\"$(COMPNAME)\"" -DTARGET_NAME="\"$(TARGETNAME)\"" \
			-DOS_NAME="\"$(OS)\"" -DPROGRAM_NAME="\"$(PROJNAME)\"" \
			-DLOG_DIR="\"$(LOGDIR)\""

# 	Output file names
RELOUTPATH	= $(OUTDIR)$(PROJNAME)
DBGOUTPATH	= $(addsuffix -debug, $(RELOUTPATH))
PRFOUTPATH	= $(addsuffix -gprof, $(RELOUTPATH))

#	Calculate paths based on project directories and list of module names
RELOBJPATHS	= $(addsuffix .o, $(addprefix $(RELOBJDIR),$(MODULES)))
DBGOBJPATHS	= $(addsuffix .o, $(addprefix $(DBGOBJDIR),$(MODULES)))
PRFOBJPATHS	= $(addsuffix .o, $(addprefix $(PRFOBJDIR),$(MODULES)))
HDRPATHS	= $(addsuffix .h, $(addprefix $(SRCDIR),$(MODULES)))

.PHONY: clean release debug gprof arch opts

#	Make All rule
all:	release
release: $(RELOUTPATH)
debug:	 $(DBGOUTPATH)
gprof:   $(PRFOUTPATH)

#	Print usage options
opts:
	$(info ) $(info Build Targets)
	$(info make         - standard build)
	$(info make clean   - remove all object and executables)
	$(info make opts    - display this message)
	$(info make arch    - archive the source)
	$(info make objdump - display object info)
			   			   
#	Clean rule - Remove build and output dirs
clean:
	$(RM) $(DBGOUTPATH)
	$(RM) $(RELOUTPATH)
	$(RM) $(PRFOUTPATH)
	$(RM) $(OUTDIR)
	$(RM) $(DBGOBJDIR)
	$(RM) $(PRFOBJDIR)
	$(RM) $(RELOBJDIR)

arch: clean
	find . -name "*~" -delete
	find . -name "#*" -delete
	tar -czf $(ARCHDIR)$(PROJNAME)-$(shell date +%Y%m%d%H%M)-src.tar.gz .

#	Order-only rules to make sure dirs are created before object files that go inside them
$(RELOBJPATHS): | $(RELOBJDIR)
$(DBGOBJPATHS): | $(DBGOBJDIR)
$(PRFOBJPATHS): | $(PRFOBJDIR)
$(DBGOBJDIR) $(PRFOBJDIR): | $(RELOBJDIR)
$(RELOUTPATH) $(DBGOUTPATH) $(PRFOUTPATH):  | $(OUTDIR)
#	Rules to create directories if they don't exist
$(DBGOBJDIR) $(RELOBJDIR) $(PRFOBJDIR) $(OUTDIR):
	$(MKDIR) $@

# Build rules
$(RELOUTPATH): $(RELOBJPATHS) 
	$(LD) -o $@ $(RELOBJPATHS) $(RELLFLAGS)
$(RELOBJDIR)%.o: $(SRCDIR)%.c
	$(CC) $(INCFLAGS) $(CFLAGS) $(RELCFLAGS) $(BUILDFLAGS) -o $@ $<
# Output
$(DBGOUTPATH): $(DBGOBJPATHS) 
	$(LD) -o $@ $(DBGOBJPATHS) $(DBGLFLAGS)
$(DBGOBJDIR)%.o: $(SRCDIR)%.c
	$(CC) $(INCFLAGS) $(CFLAGS) $(DBGCFLAGS) $(BUILDFLAGS) -o $@ $<
# Output
$(PRFOUTPATH): $(PRFOBJPATHS) 
	$(LD) -o $@ $(PRFOBJPATHS) $(PRFLFLAGS)
$(PRFOBJDIR)%.o: $(SRCDIR)%.c
	$(CC) $(INCFLAGS) $(CFLAGS) $(PRFCFLAGS) $(BUILDFLAGS) -o $@ $< >> $(@:.o=.lst)
