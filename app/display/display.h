#pragma once
#include "st7789v.h"
#include "fontx.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include <map>
#include <vector>

using Fontx::Font, Fontx::FontxFile, LCD::Color;

namespace Display
{
    struct UiStringItem
    {
        uint32_t id;
        const char *label;
        uint16_t x, y;
        Color color;
        Color backgroundColor;
        FontxFile *font;
        bool focusable;
        bool focused = false;

        static uint32_t lastId;

        UiStringItem(const char *label,
                     Color color,
                     FontxFile *font,
                     bool focusable = true,
                     Color backgroundColor = Color::None,
                     uint16_t x = 0,
                     uint16_t y = 0);
    };

    enum class Position
    {
        Start,
        Center,
        End,
        NotSpecified,
    };

    class DisplayController
    {
        LCD::ST7789V lcd;
        gpio_num_t _mosi, _clk, _cs, _dc, _rst, _bl;

        esp_err_t mountSPIFFS(const char *path, const char *label, size_t max_files);
        esp_err_t initFonts();

    public:
        FontxFile fx16G[2], fx24G[2], fx32G[2], fx32L[2], fx16M[2], fx24M[2], fx32M[2];

        DisplayController(
            gpio_num_t mosi,
            gpio_num_t clk,
            gpio_num_t cs,
            gpio_num_t dc,
            gpio_num_t rst,
            gpio_num_t bl);

        esp_err_t Init();
        void Clear(Color color);
        void DrawStringItem(UiStringItem *item,
                            Position hp = Position::NotSpecified,
                            Position vp = Position::NotSpecified);
        void DrawStringItems(
            std::vector<UiStringItem>::iterator start,
            std::vector<UiStringItem>::iterator end,
            uint16_t x, uint16_t y, bool direction = false);
        uint16_t GetWidth();
        uint16_t GetHeight();
    };
}