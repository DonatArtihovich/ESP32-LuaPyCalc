#pragma once

#include <string>

#include "esp_log.h"
#include "esp_err.h"

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
    };
}