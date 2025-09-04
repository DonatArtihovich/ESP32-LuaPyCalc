#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"

namespace Piso
{
    class PisoController
    {
        gpio_num_t _lh, _clk, _ds;

    public:
        PisoController(gpio_num_t lh, gpio_num_t clk, gpio_num_t ds);
        void Init();
        uint8_t ReadByte();
    };
}