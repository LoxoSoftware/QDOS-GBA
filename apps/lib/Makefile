#---------------------------------------------------------------------------------
# General makefile for QDOS-GBA app development
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

CFLAGS	:=	-mcpu=arm7tdmi -mtune=arm7tdmi\
 		-fomit-frame-pointer -Wno-builtin-declaration-mismatch\
		-ffast-math -fno-inline -fpie\
		$(ARCH)
		#-fno-strict-aliasing

LDFLAGS	=	-g $(ARCH) -Wl,--entry=0,-n,-N,--section-start=.start=0\
		-Wl,--error-unresolved-symbols,--no-warn-rwx-segments

#---------------------------------------------------------------------------------

build:
	@$(DEVKITARM)/bin/arm-none-eabi-gcc ./main.c -nostdlib \
	$(CFLAGS) $(ASFLAGS) $(LDFLAGS) $(INCLUDE) $(LIBPATHS) $(LIBS) \
	-o ./$(TARGET).elf
	@$(DEVKITARM)/bin/arm-none-eabi-strip --strip-section-headers --strip-debug $(TARGET).elf -o $(TARGET).qdos
	@echo "Binary size:"
	@du $(TARGET).qdos --bytes -h

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm *.qdos *.elf
