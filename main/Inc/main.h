#pragma once

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "keyboard.h"
#include "sd.h"
#include "display.h"

using Keyboard::KeyboardController, SD::SDCard, Display::DisplayController;

namespace Main
{
    class Main
    {
        static KeyboardController keyboard;
        static SDCard sdcard;
        static DisplayController display;

    public:
        static void Setup();
        static void Tick();
    };
}