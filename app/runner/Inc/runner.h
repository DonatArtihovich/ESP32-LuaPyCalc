#pragma once

#include <string>
#include <cstring>

#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

namespace CodeRunner
{
    enum class CodeLanguage
    {
        Lua,
        Python,
        Ruby
    };

    class CodeRunController
    {
    public:
        static esp_err_t RunCodeString(std::string code, CodeLanguage language);
        static esp_err_t Init();
    };
}