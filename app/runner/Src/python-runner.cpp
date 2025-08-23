#include "python-runner.h"

static const char *TAG = "PythonRunController";

namespace CodeRunner
{
    esp_err_t PythonRunController::RunCodeString(const char *code, char *traceback, size_t traceback_len)
    {
        esp_err_t ret{ESP_OK};
        ESP_LOGI(TAG, "Run Python Code: %s", code);

        return ret;
    }

    esp_err_t PythonRunController::RunCodeFile(const char *code, char *traceback, size_t traceback_len)
    {
        esp_err_t ret{ESP_OK};
        ESP_LOGI(TAG, "Run Python File: %s", code);

        return ret;
    }
}