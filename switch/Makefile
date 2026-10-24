.SUFFIXES:

ifeq ($(strip $(DEVKITPRO)),)
$(error Please set DEVKITPRO in your environment)
endif

TOPDIR ?= $(CURDIR)
TARGET		:=	pksm-switch
BUILD		:=	build
APP_TITLE	:=	PKSM Switch
APP_AUTHOR	:=	FlagBrew
APP_VERSION	:=	0.1.0
ROMFS		:=	romfs

include $(DEVKITPRO)/libnx/switch_rules

SOURCES		:=	source \
				../core/memecrypto \
				../core/source/i18n \
				../core/source/personal \
				../core/source/pkx \
				../core/source/sav \
				../core/source/utils \
				../core/source/wcx
INCLUDES	:=	include \
				../core/include \
				../core/include/enums \
				../core/include/personal \
				../core/include/pkx \
				../core/include/sav \
				../core/include/utils \
				../core/include/wcx \
				../core/memecrypto \
				../core/pcg-cpp/include

ARCH		:=	-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CFLAGS		:=	-g -Wall -Wextra -Werror -Wno-unused-parameter \
				-Wno-missing-field-initializers -O2 \
				-ffunction-sections $(ARCH) -D__SWITCH__ -D_GNU_SOURCE=1
CXXFLAGS	:=	$(CFLAGS) -fno-rtti -std=gnu++20
LDFLAGS		=	-specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) \
				-Wl,-Map,$(notdir $*.map)
SDL_PKG_CONFIG_PATH := $(PORTLIBS)/lib/pkgconfig
SDL_CFLAGS	:=	$(shell PKG_CONFIG_PATH=$(SDL_PKG_CONFIG_PATH) pkg-config --cflags sdl2 SDL2_ttf)
SDL_LIBS	:=	$(shell PKG_CONFIG_PATH=$(SDL_PKG_CONFIG_PATH) pkg-config --static --libs sdl2 SDL2_ttf)
CFLAGS		+=	$(SDL_CFLAGS)
CXXFLAGS	+=	$(SDL_CFLAGS)
LIBS		:=	$(SDL_LIBS) -lnx

ifeq ($(strip $(SDL_CFLAGS)),)
$(error SDL2 and SDL2_ttf development packages are required)
endif

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)
export ROMFS_PATH	:=	$(CURDIR)/$(ROMFS)
export NROFLAGS	+=	--icon=$(TOPDIR)/icon.jpg \
				--nacp=$(OUTPUT).nacp \
				--romfsdir=$(ROMFS_PATH)
export LD	:=	$(CXX)
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
export OFILES	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o)
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
				-I$(CURDIR)/$(BUILD) \
				-I$(DEVKITPRO)/libnx/include
export CPPFLAGS	:=	$(INCLUDE)
export LIBPATHS	:=	-L$(DEVKITPRO)/libnx/lib -L$(PORTLIBS)/lib

CORE_STRING_FILES	:=	$(shell find $(TOPDIR)/../core/strings -type f)
CORE_PERSONAL_FILES	:=	$(wildcard $(TOPDIR)/../core/personals/*)
export PKM_SPRITESHEET	:=	$(ROMFS_PATH)/sprites/pkm_spritesheet.t3x
export PKM_SPRITESHEET_URL	:=	https://cdn.sigkill.tech/assets/pkm_spritesheet.t3x
export PKM_SPRITESHEET_SHA256	:=	c54b464de9e56f5b04c7d679bdf0b9b6c84dbea5555bb7ae62862b1862081032

.PHONY: all clean $(BUILD)

all: $(BUILD)

$(BUILD):
	@mkdir -p $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@rm -rf $(BUILD) $(ROMFS) $(TARGET).nro $(TARGET).nacp $(TARGET).elf

else

.PHONY: all

all: $(OUTPUT).nro

$(OUTPUT).nro: $(OUTPUT).elf $(OUTPUT).nacp $(TOPDIR)/icon.jpg $(ROMFS_PATH)/i18n \
		$(ROMFS_PATH)/personal $(ROMFS_PATH)/fonts/Roboto.ttf \
		$(ROMFS_PATH)/fonts/OFL-Roboto.txt $(PKM_SPRITESHEET)

$(ROMFS_PATH)/i18n: $(CORE_STRING_FILES)
	@mkdir -p $@
	@cp -R $(TOPDIR)/../core/strings/* $@

$(ROMFS_PATH)/personal: $(CORE_PERSONAL_FILES)
	@mkdir -p $@
	@cp $(TOPDIR)/../core/personals/* $@

$(ROMFS_PATH)/fonts/Roboto.ttf: $(TOPDIR)/../assets/gfx/ui/Roboto.ttf
	@mkdir -p $(dir $@)
	@cp $< $@

$(ROMFS_PATH)/fonts/OFL-Roboto.txt: $(TOPDIR)/../assets/gfx/ui/OFL-Roboto.txt
	@mkdir -p $(dir $@)
	@cp $< $@

$(PKM_SPRITESHEET):
	@mkdir -p $(dir $@)
	@curl --fail --location --silent --show-error "$(PKM_SPRITESHEET_URL)" --output "$@.tmp"
	@echo "$(PKM_SPRITESHEET_SHA256)  $@.tmp" | sha256sum --check
	@mv "$@.tmp" "$@"

$(OFILES): | $(DEPSDIR)

$(OUTPUT).elf: $(OFILES)

$(DEPSDIR):
	@mkdir -p $@

-include $(wildcard $(DEPSDIR)/*.d)

endif
