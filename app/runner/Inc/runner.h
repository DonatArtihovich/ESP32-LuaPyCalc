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
        Text,
        Lua,
        Python,
        Ruby
    };

    struct CodeProcess
    {
        CodeLanguage language;
        std::string data;
        bool is_file{};
    };

    class CodeRunController
    {
        static bool is_running;
        static bool is_waiting_input;

    public:
        static esp_err_t RunCodeString(std::string code, CodeLanguage language);
        static esp_err_t RunCodeFile(std::string path, CodeLanguage language);

        static esp_err_t SetIsRunning(bool is_running);
        static bool IsRunning();

        static esp_err_t SetIsWaitingInput(bool is_waiting_input);
        static bool IsWaitingInput();
    };
}