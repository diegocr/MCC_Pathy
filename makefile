CC	= m68k-amigaos-gcc
STRIP	= m68k-amigaos-strip
RM	= rm -f
RMDIR	= rm -rf
MKDIR	= mkdir
CP	= cp -fup

# Directories
OBJDIR = .obj_os3
BINDIR = .

# Compiler/Linker flags
CPU      = -m68020-60
CPUFLAGS = -msoft-float
CDEFS    = 
WARN     = -W -Wall -Winline -Wpointer-arith -Wsign-compare \
	-Wbad-function-cast -Wunused \
	-Wmissing-prototypes -Wmissing-declarations
OPTFLAGS = -O3 -fomit-frame-pointer -funroll-loops
DEBUG    = -DDEBUG -g -O0
REDEFINE =
CFLAGS   = -noixemul -I. -I../includes $(CPU) $(CPUFLAGS) $(WARN) \
           $(OPTFLAGS) $(DEBUG) $(REDEFINE) -c
LDFLAGS  = $(CPU) $(CPUFLAGS) -noixemul -nostartfiles -nostdlib
LDLIBS   = -Wl,-Map,$@.map,--cref -lmui -ldebug -lamiga

# CPU and DEBUG can be defined outside, defaults to above
# using e.g. "make DEBUG= CPU=-m68060" produces optimized non-debug 68060 version
#
TARGET	= $(BINDIR)/Pathy.mcc
OBJS	= \
	$(OBJDIR)/Pathy_mcc.o	\
	$(OBJDIR)/util.o	\
	$(OBJDIR)/debug.o

all: $(BINDIR) $(OBJDIR) $(TARGET)

# make the object directories
$(OBJDIR):
#	@printf '\033[33mGenerating $@ directory\033[0m\n'
	@$(MKDIR) $(OBJDIR)

# make the binary directories
$(BINDIR):
#	@printf '\033[33mGenerating $@ directory\033[0m\n'
	@$(MKDIR) $(BINDIR)

#

$(OBJDIR)/%.o: %.c
	@printf '\033[32mCompiling $<\033[0m\n'
	@$(CC) $(CFLAGS) $< -o $@ -DNO_INLINE_STDARG

#

$(TARGET): $(OBJS)
	@printf '\033[32mLinking \033[1m$@\033[0m\n'
	@$(CC) $(LDFLAGS) -o $@.debug $(OBJS) $(LDLIBS)
	@$(CC) $(LDFLAGS) -o $@ -s $(OBJS) $(LDLIBS)

#

.PHONY: clean
clean:
	-$(RM) $(TARGET) $(TARGET).debug $(OBJS)

.PHONY: distclean
distclean: clean
	-$(RMDIR) $(OBJDIR) $(BINDIR)

.PHONY: demo
demo:
	$(CC) -noixemul -I. $(CPU) $(CPUFLAGS) $(WARN) \
           $(OPTFLAGS) $@.c -o $@ -s $(LDLIBS)

