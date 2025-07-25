#pragma once
#include "sipo.h"
#include "piso.h"

namespace Keyboard
{
    static volatile uint64_t KeyState = 0xFFFFFFFF;

    class KeyboardController
    {
        gpio_num_t _clk, _sipo_lh, _sipo_ds, _piso_lh, _piso_ds;

    public:
        KeyboardController(
            gpio_num_t _clk,
            gpio_num_t sipo_lh,
            gpio_num_t sipo_ds,
            gpio_num_t piso_lh,
            gpio_num_t piso_ds);

        esp_err_t Init();
    };

    enum class Key
    {
        Top = 9,
        Right = 16,
        Enter = 25,
        Left = 32,
        Bottom = 49,
    };
}