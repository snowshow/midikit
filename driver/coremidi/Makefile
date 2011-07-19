
PROJECTDIR=../..
SUBDIR=driver/coremidi

include ../../config.mk

OBJS=$(OBJDIR)/coremidi.o

.PHONY: all clean

all: $(OBJS)

clean:
	rm -f $(LIB)
	rm -f $(OBJS)

$(OBJDIR)/%.o:
	@$(MKDIR_P) $(OBJDIR)
	$(COMPILE_OBJ)

$(OBJDIR)/coremidi.o: coremidi.c coremidi.h
