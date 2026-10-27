#include "ui/Icons.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <map>
#include <vector>

// nanosvg is third-party C code; keep its warnings out of the -Werror build.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wsign-compare"
#define NANOSVG_IMPLEMENTATION
#include "nanosvg/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvg/nanosvgrast.h"
#pragma GCC diagnostic pop

namespace pksm::switchui::icons
{
    namespace
    {
        constexpr size_t ICON_COUNT = static_cast<size_t>(Icon::Count);

        // The file of each icon, in the order of Icon.
        constexpr std::array<const char*, ICON_COUNT> PATHS{{
            nullptr,
            "romfs:/icons/boxes.svg",
            "romfs:/icons/bank.svg",
            "romfs:/icons/bag.svg",
            "romfs:/icons/gift.svg",
            "romfs:/icons/trainer.svg",
            "romfs:/icons/more.svg",
            "romfs:/icons/backup.svg",
            "romfs:/icons/edit.svg",
        }};

        // On the canvas, icons are 26 pixels wide in a 48 pixel box.
        constexpr float GLYPH_SHARE = 26.0f / 48.0f;

        SDL_Renderer* renderer = nullptr;
        NSVGrasterizer* rasterizer = nullptr;
        std::array<NSVGimage*, ICON_COUNT> images{};
        // The textures made so far, by icon and pixel size.
        std::map<std::pair<size_t, int>, SDL_Texture*> textures;

        SDL_Texture* textureFor(size_t icon, int pixels)
        {
            const auto key   = std::make_pair(icon, pixels);
            const auto found = textures.find(key);
            if (found != textures.end())
            {
                return found->second;
            }

            SDL_Texture* texture  = nullptr;
            NSVGimage* image      = images[icon];
            if (image && image->width > 0.0f && rasterizer && renderer)
            {
                std::vector<unsigned char> pixelsData(static_cast<size_t>(pixels) * pixels * 4);
                nsvgRasterize(rasterizer, image, 0.0f, 0.0f, pixels / image->width,
                    pixelsData.data(), pixels, pixels, pixels * 4);
                texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                    SDL_TEXTUREACCESS_STATIC, pixels, pixels);
                if (texture &&
                    (SDL_UpdateTexture(texture, nullptr, pixelsData.data(), pixels * 4) != 0 ||
                        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND) != 0))
                {
                    SDL_DestroyTexture(texture);
                    texture = nullptr;
                }
            }
            // Remember failures too, so a broken icon is not rasterized every frame.
            textures.emplace(key, texture);
            return texture;
        }
    }

    bool load(SDL_Renderer* sdlRenderer, const char*& failedPath)
    {
        unload();
        renderer   = sdlRenderer;
        rasterizer = nsvgCreateRasterizer();
        for (size_t i = 0; i < ICON_COUNT; ++i)
        {
            if (!PATHS[i])
            {
                continue;
            }
            images[i] = nsvgParseFromFile(PATHS[i], "px", 96.0f);
            if (!images[i] || images[i]->width <= 0.0f || !images[i]->shapes)
            {
                failedPath = PATHS[i];
                unload();
                return false;
            }
        }
        if (!rasterizer)
        {
            failedPath = PATHS[1];
            unload();
            return false;
        }
        return true;
    }

    void unload()
    {
        for (auto& [key, texture] : textures)
        {
            if (texture)
            {
                SDL_DestroyTexture(texture);
            }
        }
        textures.clear();
        for (NSVGimage*& image : images)
        {
            if (image)
            {
                nsvgDelete(image);
                image = nullptr;
            }
        }
        if (rasterizer)
        {
            nsvgDeleteRasterizer(rasterizer);
            rasterizer = nullptr;
        }
        renderer = nullptr;
    }

    void draw(ImDrawList* drawList, Icon icon, const ImVec2& min, float boxSize, ImU32 color)
    {
        const int pixels   = static_cast<int>(std::lround(boxSize * GLYPH_SHARE));
        const float offset = std::floor((boxSize - static_cast<float>(pixels)) * 0.5f);
        drawSized(drawList, icon, ImVec2(min.x + offset, min.y + offset), pixels, color);
    }

    void drawSized(ImDrawList* drawList, Icon icon, const ImVec2& min, int pixels, ImU32 color)
    {
        const size_t index = static_cast<size_t>(icon);
        if (icon == Icon::None || index >= ICON_COUNT || pixels <= 0)
        {
            return;
        }
        SDL_Texture* texture = textureFor(index, pixels);
        if (!texture)
        {
            return;
        }
        // Whole pixels, placed on whole pixels, so the texture is drawn 1:1.
        const ImVec2 topLeft(std::floor(min.x), std::floor(min.y));
        drawList->AddImage(ImTextureRef(static_cast<void*>(texture)), topLeft,
            ImVec2(topLeft.x + static_cast<float>(pixels), topLeft.y + static_cast<float>(pixels)),
            ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), color);
    }
}
