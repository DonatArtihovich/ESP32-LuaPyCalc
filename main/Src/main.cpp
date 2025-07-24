#include "main.h"
#include "keyboard.h"
#include "sdkconfig.h"

using Keyboard::KeyboardController;

extern "C" void app_main(void)
{
    KeyboardController keyboard{
        (gpio_num_t)CONFIG_GPIO_KEYBOARD_SIPO_CLK,
        (gpio_num_t)CONFIG_GPIO_KEYBOARD_SIPO_LH,
        (gpio_num_t)CONFIG_GPIO_KEYBOARD_SIPO_DS,
        (gpio_num_t)CONFIG_GPIO_KEYBOARD_PISO_CLK,
        (gpio_num_t)CONFIG_GPIO_KEYBOARD_PISO_LH,
        (gpio_num_t)CONFIG_GPIO_KEYBOARD_PISO_DS};

    keyboard.Init();
}
