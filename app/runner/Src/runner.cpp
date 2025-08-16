#include "runner.h"
#include "lua-runner.h"

static const char *TAG = "CodeRunner";

QueueHandle_t xQueueRunnerStdout = NULL;
QueueHandle_t xQueueRunnerStdin = NULL;
TaskHandle_t xTaskRunnerIO = NULL;

namespace CodeRunner
{
    esp_err_t CodeRunController::RunCodeString(std::string code, CodeLanguage language)
    {
        esp_err_t ret{ESP_OK};

        switch (language)
        {
        case CodeLanguage::Lua:
            ret = LuaRunController::RunCodeString(code.c_str());
            break;
        default:
            ESP_LOGI(TAG, "Language is not implemented yet");
        }

        return ret;
    }

    esp_err_t CodeRunController::RunCodeFile(std::string path, CodeLanguage language)
    {
        esp_err_t ret{ESP_OK};

        switch (language)
        {
        case CodeLanguage::Lua:
            ret = LuaRunController::RunCodeFile(path.c_str());
            break;
        default:
            ESP_LOGI(TAG, "Language is not implemented yet");
        }

        return ret;
    }
}