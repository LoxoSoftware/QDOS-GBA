#---------------------------------------------------------------------------------
# Makefile for QDOS-GBA app development
#---------------------------------------------------------------------------------

TARGET		:= $(notdir $(CURDIR))

export LIBGBA := ${DEVKITPRO}/libgba

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif
ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPRO")
endif

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-marm

CFLAGS	:=	-g0 -Wall -Os\
		-mcpu=arm7tdmi -mtune=arm7tdmi\
 		-fomit-frame-pointer -Wno-builtin-declaration-mismatch\
		-ffast-math -fno-inline -fpie\
		$(ARCH) 
		#-fno-strict-aliasing
CFLAGS	+=	$(INCLUDE)
LDFLAGS	=	-g $(ARCH) -Wl,--entry=0,-n,--section-start=.start=0\
		-Wl,--error-unresolved-symbols

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS	:= 	-lgba

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:=	$(LIBGBA)

#---------------------------------------------------------------------------------

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-iquote $(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

#---------------------------------------------------------------------------------
build:
	@$(DEVKITARM)/bin/arm-none-eabi-gcc ./main.c -nostdlib \
	$(CFLAGS) $(ASFLAGS) $(LDFLAGS) $(INCLUDE) $(LIBPATHS) $(LIBS) \
	-o ./$(TARGET).elf
	@$(DEVKITARM)/bin/arm-none-eabi-strip --strip-section-headers --strip-debug $(TARGET).elf -o $(TARGET).qdos
	@echo "Binary usage:"
	@du $(TARGET).qdos --bytes -h

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm *.qdos *.elf
