
PROJECTDIR=../..
SUBDIR=driver/osc

include ../../config.mk

OBJS=$(OBJDIR)/osc.o

.PHONY: all clean

all: $(OBJS)

clean:
	rm -f $(LIB)
	rm -f $(OBJS)

$(OBJDIR)/%.o:
	@$(MKDIR_P) $(OBJDIR)
	$(COMPILE_OBJ)

$(OBJDIR)/osc.o: osc.c osc.h
