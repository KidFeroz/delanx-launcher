#---------------------------------------------------------------------------------
# Delanx — autotest de dispositivo (NRO de consola, sin SDL ni red)
#
#   export DEVKITPRO=/opt/devkitpro
#   make -C tests/device            # -> tests/device/DelanxSelfTest.nro
#   make -C tests/device run        # lo envia por red con nxlink -s
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

TARGET		:=	DelanxSelfTest
BUILD		:=	build
# Se excluye a proposito ../../source (contiene el main de la aplicacion): aqui manda
# el main.cpp del autotest.
SOURCES		:=	. ../../source/core ../../source/model ../../source/services ../../source/platform
DATA		:=	data
INCLUDES	:=	. ../../source

APP_TITLE	:=	Delanx SelfTest
APP_AUTHOR	:=	KidFeroz
APP_VERSION	:=	0.1.0
ICON		:=	../../icon.jpg

#---------------------------------------------------------------------------------
ARCH	:=	-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

CFLAGS	:=	-g -Wall -Wextra -Wno-unused-parameter -O2 -ffunction-sections -fdata-sections \
			$(ARCH) $(DEFINES)

CFLAGS	+=	$(INCLUDE) -D__SWITCH__ -DDELANX_VERSION=\"$(APP_VERSION)\"

CXXFLAGS	:= $(CFLAGS) -std=c++20 -fno-rtti -fno-exceptions

ASFLAGS	:=	-g $(ARCH)
LDFLAGS		=	-specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map) -Wl,--gc-sections

PKGCONFIG	:=	$(DEVKITPRO)/portlibs/switch/bin/aarch64-none-elf-pkg-config
export PKG_CONFIG_PATH := $(DEVKITPRO)/portlibs/switch/lib/pkgconfig

PKG_MODULES	:=	libcurl jansson libarchive freetype2
PKG_CFLAGS	:=	$(shell $(PKGCONFIG) --cflags $(PKG_MODULES) 2>/dev/null)
PKG_LIBS	:=	$(shell $(PKGCONFIG) --libs $(PKG_MODULES) 2>/dev/null)

CFLAGS		+=	$(PKG_CFLAGS)
LIBS		:=	$(PKG_LIBS) -lnx -lm

LIBDIRS	:= $(PORTLIBS) $(LIBNX)

#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SRC	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES		:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export APP_ICON := $(CURDIR)/$(ICON)
export NROFLAGS += --icon=$(APP_ICON)

ifeq ($(strip $(NO_NACP)),)
	export NROFLAGS += --nacp=$(CURDIR)/$(TARGET).nacp
endif

.PHONY: $(BUILD) clean all run

all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

# Envia el NRO a la consola por red (nxlink del toolchain) y muestra su salida.
run: all
	nxlink -s $(CURDIR)/$(TARGET).nro

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).nro $(TARGET).nacp $(TARGET).elf $(TARGET).map

#---------------------------------------------------------------------------------
else
.PHONY:	all

DEPENDS	:=	$(OFILES:.o=.d)

all	:	$(OUTPUT).nro

ifeq ($(strip $(NO_NACP)),)
$(OUTPUT).nro	:	$(OUTPUT).elf $(OUTPUT).nacp
else
$(OUTPUT).nro	:	$(OUTPUT).elf
endif

$(OUTPUT).elf	:	$(OFILES)

$(OFILES_SRC)	: $(HFILES_BIN)

%.bin.o	%_bin.h :	%.bin
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
