#pragma once
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

namespace Sipo
{
    class SipoController
    {
        gpio_num_t _clk, _lh, _ds;
        static void shift_out(gpio_num_t clk, gpio_num_t ds, uint8_t data, bool lsb);

    public:
        SipoController(gpio_num_t clk, gpio_num_t lh, gpio_num_t ds);
        void Init();
        void SendByte(uint8_t data, bool lsb = true);
    };
}