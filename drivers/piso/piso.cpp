#include "piso.h"

namespace Piso
{
    PisoController::PisoController(
        gpio_num_t clk,
        gpio_num_t lh,
        gpio_num_t ds)
        : _lh{lh},
          _clk{clk},
          _ds{ds} {}

    void PisoController::Init()
    {
        gpio_set_direction(_clk, GPIO_MODE_OUTPUT);
        gpio_set_direction(_lh, GPIO_MODE_OUTPUT);
        gpio_set_direction(_ds, GPIO_MODE_INPUT);

        gpio_set_level(_lh, 1);
        gpio_set_level(_clk, 0);
    }

    uint8_t PisoController::ReadByte()
    {
        uint8_t data{};
        gpio_set_level(_lh, 0);
        esp_rom_delay_us(5);
        gpio_set_level(_lh, 1);
        esp_rom_delay_us(5);

        for (int i = 0; i < 8; i++)
        {
            data |= ((gpio_get_level(_ds) & 0x01) << (7 - i));
            if (i < 7)
            {
                gpio_set_level(_clk, 1);
                esp_rom_delay_us(5);
                gpio_set_level(_clk, 0);
                esp_rom_delay_us(5);
            }
        }

        return data;
    }
}