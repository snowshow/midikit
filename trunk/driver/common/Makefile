
PROJECTDIR=../..
SUBDIR=driver/common

include ../../config.mk

OBJS=$(OBJDIR)/rtp.o $(OBJDIR)/rtpmidi.o

.PHONY: all clean

all: $(OBJS)

clean:
	rm -f $(LIB)
	rm -f $(OBJS)

$(OBJDIR)/%.o:
	@$(MKDIR_P) $(OBJDIR)
	$(COMPILE_OBJ)

$(OBJDIR)/rtp.o: rtp.c rtp.h
$(OBJDIR)/rtpmidi.o: rtpmidi.c rtpmidi.h rtp.h
