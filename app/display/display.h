#pragma once
#include "st7789v.h"
#include "fontx.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_log.h"

using Fontx::Font, Fontx::FontxFile, LCD::Color;

namespace Display
{
    class DisplayController
    {
        LCD::ST7789V lcd;
        gpio_num_t _mosi, _clk, _cs, _dc, _rst, _bl;
        FontxFile fx16G[2], fx24G[2], fx32G[2], fx32L[2], fx16M[2], fx24M[2], fx32M[2];

        esp_err_t mountSPIFFS(const char *path, const char *label, size_t max_files);
        esp_err_t initFonts();

    public:
        DisplayController(
            gpio_num_t mosi,
            gpio_num_t clk,
            gpio_num_t cs,
            gpio_num_t dc,
            gpio_num_t rst,
            gpio_num_t bl);

        esp_err_t Init();
        void MainMenu();
    };
}