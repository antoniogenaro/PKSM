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
				source/app \
				source/domain \
				source/platform \
				source/screens \
				source/ui \
				../core/memecrypto \
				../core/source/i18n \
				../core/source/personal \
				../core/source/pkx \
				../core/source/sav \
				../core/source/utils \
				../core/source/wcx \
				../external/imgui
IMGUI_BACKEND_SOURCES := ../external/imgui/backends
IMGUI_BACKEND_CPPFILES := imgui_impl_sdl2.cpp imgui_impl_sdlrenderer2.cpp
INCLUDES	:=	include \
				../core/include \
				../core/include/enums \
				../core/include/personal \
				../core/include/pkx \
				../core/include/sav \
				../core/include/utils \
				../core/include/wcx \
				../core/memecrypto \
				../core/pcg-cpp/include \
				../external \
				../external/imgui \
				../external/imgui/backends

ARCH		:=	-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CFLAGS		:=	-g -Wall -Wextra -Werror -Wno-unused-parameter \
				-Wno-missing-field-initializers -O2 \
				-ffunction-sections $(ARCH) -D__SWITCH__ -D_GNU_SOURCE=1 \
				-DIMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS
CXXFLAGS	:=	$(CFLAGS) -fno-rtti -std=gnu++20
LDFLAGS		=	-specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) \
				-Wl,-Map,$(notdir $*.map)
SDL_PKG_CONFIG_PATH := $(PORTLIBS)/lib/pkgconfig
SDL_CFLAGS	:=	$(shell PKG_CONFIG_PATH=$(SDL_PKG_CONFIG_PATH) pkg-config --cflags sdl2)
SDL_LIBS	:=	$(shell PKG_CONFIG_PATH=$(SDL_PKG_CONFIG_PATH) pkg-config --static --libs sdl2)
CFLAGS		+=	$(SDL_CFLAGS)
CXXFLAGS	+=	$(SDL_CFLAGS)
LIBS		:=	$(SDL_LIBS) -lnx

ifeq ($(strip $(SDL_CFLAGS)),)
$(error SDL2 development package is required)
endif

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)
export ROMFS_PATH	:=	$(CURDIR)/$(ROMFS)
export NROFLAGS	+=	--icon=$(TOPDIR)/icon.jpg \
				--nacp=$(OUTPUT).nacp \
				--romfsdir=$(ROMFS_PATH)
export LD	:=	$(CXX)
export VPATH	:=	$(foreach dir,$(SOURCES) $(IMGUI_BACKEND_SOURCES),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp))) \
				$(IMGUI_BACKEND_CPPFILES)
CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
export OFILES	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o)
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
				-I$(CURDIR)/$(BUILD) \
				-I$(DEVKITPRO)/libnx/include
export CPPFLAGS	:=	$(INCLUDE)
export LIBPATHS	:=	-L$(DEVKITPRO)/libnx/lib -L$(PORTLIBS)/lib

export CORE_STRING_FILES	:=	$(shell find $(TOPDIR)/../core/strings -type f)
export SWITCH_STRING_FILES	:=	$(wildcard $(TOPDIR)/strings/*.json)
export SWITCH_ICON_FILES	:=	$(wildcard $(TOPDIR)/icons/*.svg)
export CORE_PERSONAL_FILES	:=	$(wildcard $(TOPDIR)/../core/personals/*)
export UI_FONT_FILES	:=	$(TOPDIR)/../assets/gfx/ui/Roboto.ttf $(TOPDIR)/../assets/gfx/ui/OFL-Roboto.txt \
				$(TOPDIR)/../external/imgui/misc/fonts/Roboto-Medium.ttf
# Roboto Medium is under the Apache License 2.0; pcg-cpp ships its full text.
export UI_FONT_APACHE_LICENSE	:=	$(TOPDIR)/../core/pcg-cpp/LICENSE-APACHE.txt
export PKM_SPRITESHEET	:=	$(ROMFS_PATH)/sprites/pkm_spritesheet.t3x
export PKM_SPRITESHEET_URL	:=	https://cdn.sigkill.tech/assets/pkm_spritesheet.t3x
export PKM_SPRITESHEET_SHA256	:=	c54b464de9e56f5b04c7d679bdf0b9b6c84dbea5555bb7ae62862b1862081032
export BALL_SPRITESHEET	:=	$(ROMFS_PATH)/sprites/types_spritesheet.t3x
export BALL_SPRITESHEET_URL	:=	https://cdn.sigkill.tech/assets/types_spritesheet.t3x
export BALL_SPRITESHEET_SHA256	:=	9fbaa10fe205ce57cf8732c37f72420204f906d75c65ffaee8bf615a08e48685

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

$(OUTPUT).nro: $(OUTPUT).elf $(OUTPUT).nacp $(TOPDIR)/icon.jpg \
		$(DEPSDIR)/i18n.stamp $(DEPSDIR)/strings.stamp $(DEPSDIR)/personal.stamp \
		$(DEPSDIR)/fonts.stamp $(DEPSDIR)/icons.stamp \
		$(PKM_SPRITESHEET) $(BALL_SPRITESHEET)

$(DEPSDIR)/i18n.stamp: $(CORE_STRING_FILES) | $(DEPSDIR)
	@mkdir -p $(ROMFS_PATH)/i18n
	@cp -R $(TOPDIR)/../core/strings/* $(ROMFS_PATH)/i18n/
	@touch $@

$(DEPSDIR)/strings.stamp: $(SWITCH_STRING_FILES) | $(DEPSDIR)
	@rm -rf $(ROMFS_PATH)/strings
	@mkdir -p $(ROMFS_PATH)/strings
	@cp $(TOPDIR)/strings/*.json $(ROMFS_PATH)/strings/
	@touch $@

$(DEPSDIR)/icons.stamp: $(SWITCH_ICON_FILES) | $(DEPSDIR)
	@rm -rf $(ROMFS_PATH)/icons
	@mkdir -p $(ROMFS_PATH)/icons
	@cp $(TOPDIR)/icons/*.svg $(ROMFS_PATH)/icons/
	@touch $@

# Also after Makefile changes, so a font added to the list is copied even when it is older.
$(DEPSDIR)/fonts.stamp: $(UI_FONT_FILES) $(UI_FONT_APACHE_LICENSE) $(TOPDIR)/Makefile | $(DEPSDIR)
	@rm -rf $(ROMFS_PATH)/fonts
	@mkdir -p $(ROMFS_PATH)/fonts
	@cp $(UI_FONT_FILES) $(ROMFS_PATH)/fonts/
	@cp $(UI_FONT_APACHE_LICENSE) $(ROMFS_PATH)/fonts/Apache-Roboto-Medium.txt
	@touch $@

$(DEPSDIR)/personal.stamp: $(CORE_PERSONAL_FILES) | $(DEPSDIR)
	@mkdir -p $(ROMFS_PATH)/personal
	@cp $(TOPDIR)/../core/personals/* $(ROMFS_PATH)/personal/
	@touch $@

$(PKM_SPRITESHEET):
	@mkdir -p $(dir $@)
	@curl --fail --location --silent --show-error "$(PKM_SPRITESHEET_URL)" --output "$@.tmp"
	@echo "$(PKM_SPRITESHEET_SHA256)  $@.tmp" | sha256sum --check
	@mv "$@.tmp" "$@"

$(BALL_SPRITESHEET):
	@mkdir -p $(dir $@)
	@curl --fail --location --silent --show-error "$(BALL_SPRITESHEET_URL)" --output "$@.tmp"
	@echo "$(BALL_SPRITESHEET_SHA256)  $@.tmp" | sha256sum --check
	@mv "$@.tmp" "$@"

$(OFILES): | $(DEPSDIR)

$(OUTPUT).elf: $(OFILES)

$(DEPSDIR):
	@mkdir -p $@

-include $(wildcard $(DEPSDIR)/*.d)

endif
