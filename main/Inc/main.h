#pragma once

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "keyboard.h"
#include "sd.h"

using Keyboard::KeyboardController, SD::SDCard;

namespace Main
{
    class Main
    {
        static KeyboardController keyboard;
        static SDCard sdcard;

    public:
        static void Setup();
    };
}