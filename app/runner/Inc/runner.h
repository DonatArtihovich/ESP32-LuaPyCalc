#pragma once

#include <string>
#include <cstring>

#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

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
        static bool is_waiting_output;

    public:
        static esp_err_t RunCodeString(std::string code, CodeLanguage language, char *traceback, size_t traceback_len);
        static esp_err_t RunCodeFile(std::string path, CodeLanguage language, char *traceback, size_t traceback_len);

        static void SetIsRunning(bool is_running);
        static bool IsRunning();

        static void SetIsWaitingInput(bool is_waiting_input);
        static bool IsWaitingInput();

        static void SetIsWaitingOutput(bool is_waiting_output);
        static bool IsWaitingOutput();
    };
}