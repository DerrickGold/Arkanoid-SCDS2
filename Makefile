#folder of sdk root
DSTWOSDK :=/mnt/DSTwo/SDK
#libBAG root path
LIBBAG :=$(DSTWOSDK)/../libBAG



#folders with sources
#name of output binary file
BINARY      := $(shell basename $(CURDIR))
SOURCES     := src
INCLUDES    := include

#include the standard DSTwo SDK library
LIBS :=$(DSTWOSDK)/lib/libds2b.a -lc -lm -lgcc
#include libBAG
EXTLIBS :=$(LIBBAG)/lib/libbag.a $(DSTWOSDK)/lib/libds2a.a 


CFLAGS := -std=gnu99 -mips32 -O3 -Wall -Wno-char-subscripts -mno-abicalls -fno-pic -fno-builtin \
	   -fno-exceptions -ffunction-sections -mlong-calls\
	   -fomit-frame-pointer -msoft-float -G 4  

#DSTwo SDK  setup
# CROSS :=#
CROSS :=/opt/mipsel-4.1.2-nopic/bin/

CC =$(CROSS)mipsel-linux-gcc
AR =$(CROSS)mipsel-linux-ar rcsv
LD	=$(CROSS)mipsel-linux-ld
OBJCOPY	=$(CROSS)mipsel-linux-objcopy
NM	=$(CROSS)mipsel-linux-nm
OBJDUMP	=$(CROSS)mipsel-linux-objdump


FS_DIR =$(DSTWOSDK)/libsrc/fs
CONSOLE_DIR =$(DSTWOSDK)/libsrc/console
KEY_DIR =$(DSTWOSDK)/libsrc/key
ZLIB_DIR = $(DSTWOSDK)/libsrc/zlib



CSRC    :=  $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.c))
CPPSRC	:=	$(foreach dir,$(SOURCES),$(wildcard $(dir)/*.cpp))
SSRC    :=  $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.S))

INC := -I$(SOURCES) \
       -I$(DSTWOSDK)/include -I$(FS_DIR) -I$(CONSOLE_DIR) -I$(KEY_DIR) -I$(ZLIB_DIR) \
       -I$(LIBBAG)/libsrc/include \
		$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
		$(foreach dir,$(SOURCES),-I$(CURDIR)/$(dir)) \


LINKS  :=$(DSTWOSDK)/specs/link.xn
STARTS :=$(DSTWOSDK)/specs/start.S
STARTO :=start.o

SOBJS   := $(SSRC:.s=.o)
OBJS	:= $(CPPSRC:.cpp=.o) $(CSRC:.c=.o)

APP	:=$(BINARY).elf

all: $(APP)
	$(OBJCOPY) -O binary $(APP) $(BINARY).bin
	rm -fr *.elf
	$(DSTWOSDK)/tools/makeplug $(BINARY).bin $(BINARY).plg
	rm -fr *.bin
	rm depend

$(APP):	depend $(SOBJS) $(OBJS) $(STARTO) $(LINKS) $(EXTLIBS)
	$(CC) -nostdlib -static -T $(LINKS) -o $@ $(STARTO) $(OBJS) $(EXTLIBS) $(LIBS) 
	rm $(STARTO)
$(EXTLIBS): 
	make -C ../source/

$(STARTO):
	$(CC) $(CFLAGS) $(INC) -o $@ -c $(STARTS)

.c.o:
	$(CC) $(CFLAGS) $(INC) -o $@ -c $<
.cpp.o:
	$(CXX) $(CFLAGS) $(INC) -fno-rtti -fvtable-gc -o $@ -c $<
.S.o:
	$(CC) $(CFLAGS) $(INC) -D_ASSEMBLER_ -D__ASSEMBLY__ -o $@ -c $<
	
#---------------------------------------------------------------------------------
clean:
	rm -fr  $(OBJS) *.plg *.bin
	rm depend

depend:	Makefile
	$(CC) -MM $(CFLAGS) $(INC) $(SSRC) $(CSRC) $(CPPSRC) > $@ \
#    $(CXX) -MM $(CFLAGS) $(INC) $(CPPSRC) > $@

include depend


