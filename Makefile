#---------------------------------------------------------------------------------
# Delanx — Decomp Launcher NX (Nintendo Switch homebrew)
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

#---------------------------------------------------------------------------------
# TARGET   nombre del NRO resultante
# BUILD    directorio de objetos
# SOURCES  directorios con codigo fuente (no recursivo: hay que listarlos)
# DATA     directorios con datos binarios empaquetados como .o
# INCLUDES directorios de cabeceras propias
# ROMFS    contenido que se empaqueta en romfs:/
#---------------------------------------------------------------------------------
TARGET		:=	Delanx
BUILD		:=	build
SOURCES		:=	source source/core source/model source/services source/ui source/platform
DATA		:=	data
INCLUDES	:=	source
ROMFS		:=	romfs

APP_TITLE	:=	Delanx
APP_AUTHOR	:=	KidFeroz
APP_VERSION	:=	0.1.0
ICON		:=	icon.jpg

#---------------------------------------------------------------------------------
# opciones de generacion de codigo
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

CFLAGS	:=	-g -Wall -Wextra -Wno-unused-parameter -O2 -ffunction-sections -fdata-sections \
			$(ARCH) $(DEFINES)

CFLAGS	+=	$(INCLUDE) -D__SWITCH__ -DDELANX_VERSION=\"$(APP_VERSION)\"

CXXFLAGS	:= $(CFLAGS) -std=c++20 -fno-rtti -fno-exceptions

ASFLAGS	:=	-g $(ARCH)
LDFLAGS		=	-specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map) -Wl,--gc-sections

#---------------------------------------------------------------------------------
# librerias: pkg-config de devkitPro resuelve las dependencias transitivas
# (libarchive -> zstd/lzma/bz2, curl -> mbedtls, SDL2 -> EGL/glapi, etc.)
#---------------------------------------------------------------------------------
PKGCONFIG	:=	$(DEVKITPRO)/portlibs/switch/bin/aarch64-none-elf-pkg-config
export PKG_CONFIG_PATH := $(DEVKITPRO)/portlibs/switch/lib/pkgconfig

PKG_MODULES	:=	sdl2 SDL2_image SDL2_ttf libcurl jansson libarchive freetype2

PKG_CFLAGS	:=	$(shell $(PKGCONFIG) --cflags $(PKG_MODULES) 2>/dev/null)
PKG_LIBS	:=	$(shell $(PKGCONFIG) --libs $(PKG_MODULES) 2>/dev/null)

CFLAGS		+=	$(PKG_CFLAGS)
LIBS		:=	$(PKG_LIBS) -lnx -lm

#---------------------------------------------------------------------------------
LIBDIRS	:= $(PORTLIBS) $(LIBNX)

#---------------------------------------------------------------------------------
# a partir de aqui no hace falta editar nada salvo que anadas tipos de fichero
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

#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif
#---------------------------------------------------------------------------------

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SRC	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES		:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(PORTLIBS)/include/SDL2 \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

ifeq ($(strip $(ICON)),)
	icons := $(wildcard *.jpg)
	ifneq (,$(findstring $(TARGET).jpg,$(icons)))
		export APP_ICON := $(TOPDIR)/$(TARGET).jpg
	else
		ifneq (,$(findstring icon.jpg,$(icons)))
			export APP_ICON := $(TOPDIR)/icon.jpg
		endif
	endif
else
	export APP_ICON := $(TOPDIR)/$(ICON)
endif

ifeq ($(strip $(NO_ICON)),)
	export NROFLAGS += --icon=$(APP_ICON)
endif

ifeq ($(strip $(NO_NACP)),)
	export NROFLAGS += --nacp=$(CURDIR)/$(TARGET).nacp
endif

ifneq ($(APP_TITLEID),)
	export NACPFLAGS += --titleid=$(APP_TITLEID)
endif

ifneq ($(ROMFS),)
	export NROFLAGS += --romfsdir=$(CURDIR)/$(ROMFS)
endif

.PHONY: $(BUILD) clean all package

#---------------------------------------------------------------------------------
all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
# Paquete listo para copiar a la SD: sdmc:/switch/Delanx/
#---------------------------------------------------------------------------------
package: all
	@rm -rf $(CURDIR)/dist
	@mkdir -p $(CURDIR)/dist/switch/Delanx
	@cp $(CURDIR)/$(TARGET).nro $(CURDIR)/dist/switch/Delanx/
	@cp $(CURDIR)/README.md $(CURDIR)/dist/switch/Delanx/README.md 2>/dev/null || true
	@cd $(CURDIR)/dist && zip -qr ../$(TARGET)-$(APP_VERSION)-sdcard.zip switch
	@echo "paquete: $(TARGET)-$(APP_VERSION)-sdcard.zip"

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).nro $(TARGET).nacp $(TARGET).elf $(TARGET)-*-sdcard.zip dist

#---------------------------------------------------------------------------------
else
.PHONY:	all

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
all	:	$(OUTPUT).nro

ifeq ($(strip $(NO_NACP)),)
$(OUTPUT).nro	:	$(OUTPUT).elf $(OUTPUT).nacp
else
$(OUTPUT).nro	:	$(OUTPUT).elf
endif

$(OUTPUT).elf	:	$(OFILES)

$(OFILES_SRC)	: $(HFILES_BIN)

#---------------------------------------------------------------------------------
%.bin.o	%_bin.h :	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
