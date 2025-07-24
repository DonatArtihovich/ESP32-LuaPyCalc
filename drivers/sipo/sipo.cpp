#include "sipo.h"

namespace Sipo
{
    SipoController::SipoController(gpio_num_t clk, gpio_num_t lh, gpio_num_t ds) : _clk{clk}, _lh{lh}, _ds{ds} {}

    void SipoController::Init()
    {
        gpio_config_t gpio_conf{
            .pin_bit_mask = ((1UL << _clk) | (1UL << _lh) | (1UL << _ds)),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };

        gpio_config(&gpio_conf);

        gpio_set_level(_clk, 0);
        gpio_set_level(_ds, 0);
        gpio_set_level(_lh, 1);
    }

    void SipoController::shift_out(gpio_num_t clk, gpio_num_t ds, uint8_t data, bool lsb)
    {
        for (uint8_t i = 0; i < 8; i++)
        {
            if (lsb)
            {
                gpio_set_level(ds, ((data >> (7 - i)) & 0x01));
            }
            else
            {
                gpio_set_level(ds, ((data >> i) & 0x01));
            }
            gpio_set_level(clk, 1);
            gpio_set_level(clk, 0);
        }
    }

    void SipoController::SendByte(uint8_t data, bool lsb)
    {
        gpio_set_level(_lh, 0);
        shift_out(_clk, _ds, data, lsb);
        gpio_set_level(_lh, 1);
    }
}