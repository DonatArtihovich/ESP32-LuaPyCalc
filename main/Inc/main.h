#pragma once

#include <memory>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "keyboard.h"
#include "sd.h"
#include "display.h"

#include "scene.h"
#include "start-scene.h"
#include "files-scene.h"
#include "code-scene.h"

#include "runner.h"

using Keyboard::KeyboardController, SD::SDCard, Display::DisplayController;

namespace Main
{
    class Main
    {
        KeyboardController keyboard{
            (gpio_num_t)CONFIG_GPIO_KEYBOARD_CLK,
            (gpio_num_t)CONFIG_GPIO_KEYBOARD_SIPO_LH,
            (gpio_num_t)CONFIG_GPIO_KEYBOARD_SIPO_DS,
            (gpio_num_t)CONFIG_GPIO_KEYBOARD_PISO_LH,
            (gpio_num_t)CONFIG_GPIO_KEYBOARD_PISO_DS,
        };

        SDCard sdcard{
            (gpio_num_t)CONFIG_GPIO_SD_MISO,
            (gpio_num_t)CONFIG_GPIO_SD_MOSI,
            (gpio_num_t)CONFIG_GPIO_SD_CLK,
            (gpio_num_t)CONFIG_GPIO_SD_CS,
        };

        DisplayController display{
            (gpio_num_t)CONFIG_GPIO_DISPLAY_MOSI,
            (gpio_num_t)CONFIG_GPIO_DISPLAY_SCK,
            (gpio_num_t)CONFIG_GPIO_DISPLAY_CS,
            (gpio_num_t)CONFIG_GPIO_DISPLAY_DC,
            (gpio_num_t)CONFIG_GPIO_DISPLAY_RST,
            (gpio_num_t)CONFIG_GPIO_DISPLAY_BL,
        };

        std::unique_ptr<Scene::Scene> scene;

        void SwitchScene(Scene::SceneId id);
        esp_err_t InitRunnerIO();

    public:
        Main();
        void Setup();
        void Tick();
        void SendCodeOutput(const char *output);
    };

}