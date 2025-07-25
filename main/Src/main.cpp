#include "main.h"
#include "sdkconfig.h"

static const char *TAG = "MAIN";

namespace Main
{
    KeyboardController Main::keyboard{
        (gpio_num_t)CONFIG_GPIO_KEYBOARD_CLK,
        (gpio_num_t)CONFIG_GPIO_KEYBOARD_SIPO_LH,
        (gpio_num_t)CONFIG_GPIO_KEYBOARD_SIPO_DS,
        (gpio_num_t)CONFIG_GPIO_KEYBOARD_PISO_LH,
        (gpio_num_t)CONFIG_GPIO_KEYBOARD_PISO_DS,
    };

    SDCard Main::sdcard{
        (gpio_num_t)CONFIG_GPIO_SD_MISO,
        (gpio_num_t)CONFIG_GPIO_SD_MOSI,
        (gpio_num_t)CONFIG_GPIO_SD_CLK,
        (gpio_num_t)CONFIG_GPIO_SD_CS,
    };

    DisplayController Main::display{
        (gpio_num_t)CONFIG_GPIO_DISPLAY_MOSI,
        (gpio_num_t)CONFIG_GPIO_DISPLAY_SCK,
        (gpio_num_t)CONFIG_GPIO_DISPLAY_CS,
        (gpio_num_t)CONFIG_GPIO_DISPLAY_DC,
        (gpio_num_t)CONFIG_GPIO_DISPLAY_RST,
        (gpio_num_t)CONFIG_GPIO_DISPLAY_BL,
    };

    void Main::Setup()
    {
        ESP_ERROR_CHECK(keyboard.Init());
        ESP_ERROR_CHECK(display.Init());

        if (ESP_OK == sdcard.Mount(CONFIG_MOUNT_POINT))
        {
            ESP_LOGI(TAG, "SD card mounted.");
        }
        else
        {
            ESP_LOGE(TAG, "SD card mount error.");
        }

        display.MainMenu();
    }
}

extern "C" void app_main(void)
{
    Main::Main::Setup();
}
