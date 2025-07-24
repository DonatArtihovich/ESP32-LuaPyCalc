#include "main.h"
#include "sdkconfig.h"
#include "keyboard.h"
#include "sd.h"

using Keyboard::KeyboardController, SD::SDCard;

static const char *TAG = "MAIN";

extern "C" void app_main(void)
{
    KeyboardController keyboard{
        (gpio_num_t)CONFIG_GPIO_KEYBOARD_SIPO_CLK,
        (gpio_num_t)CONFIG_GPIO_KEYBOARD_SIPO_LH,
        (gpio_num_t)CONFIG_GPIO_KEYBOARD_SIPO_DS,
        (gpio_num_t)CONFIG_GPIO_KEYBOARD_PISO_CLK,
        (gpio_num_t)CONFIG_GPIO_KEYBOARD_PISO_LH,
        (gpio_num_t)CONFIG_GPIO_KEYBOARD_PISO_DS};

    SDCard card{
        (gpio_num_t)CONFIG_GPIO_SD_MISO,
        (gpio_num_t)CONFIG_GPIO_SD_MOSI,
        (gpio_num_t)CONFIG_GPIO_SD_CLK,
        (gpio_num_t)CONFIG_GPIO_SD_CS,
    };

    keyboard.Init();

    if (card.Mount(CONFIG_MOUNT_POINT) == ESP_OK)
    {
        ESP_LOGI(TAG, "SD card mounted.");
    }
    else
    {
        ESP_LOGE(TAG, "SD card mount error.");
    }

    if (ESP_OK == card.Unmount())
    {
        ESP_LOGI(TAG, "SD card unmounted.");
    }
}
