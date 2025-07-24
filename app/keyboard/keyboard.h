#pragma once
#include "sipo.h"
#include "piso.h"

namespace Keyboard
{
    static volatile uint64_t KeyState = 0xFFFFFFFF;

    class KeyboardController
    {
        gpio_num_t _sipo_clk, _sipo_lh, _sipo_ds, _piso_clk, _piso_lh, _piso_ds;

    public:
        KeyboardController(
            gpio_num_t sipo_clk,
            gpio_num_t sipo_lh,
            gpio_num_t sipo_ds,
            gpio_num_t piso_clk,
            gpio_num_t piso_lh,
            gpio_num_t piso_ds);

        esp_err_t Init();
    };
}