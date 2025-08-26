#include "runner.h"
#include "lua-runner.h"
#include "python-runner.h"

static const char *TAG = "CodeRunner";

QueueHandle_t xQueueRunnerProcessing = NULL;
QueueHandle_t xQueueRunnerStdout = NULL;
QueueHandle_t xQueueRunnerStdin = NULL;
TaskHandle_t xTaskRunnerIO = NULL;
TaskHandle_t xTaskRunnerProcessing = NULL;
TaskHandle_t xTaskRunnerDisplaying = NULL;
TaskHandle_t xTaskRunnerWatchdogResetting = NULL;

SemaphoreHandle_t xIsRunningMutex = NULL;
SemaphoreHandle_t xIsWaitingInputMutex = NULL;
SemaphoreHandle_t xIsWaitingOutputMutex = NULL;
SemaphoreHandle_t xDisplayingSemaphore = NULL;

namespace CodeRunner
{
    bool CodeRunController::is_running{};
    bool CodeRunController::is_waiting_input{};
    bool CodeRunController::is_waiting_output{};

    esp_err_t CodeRunController::RunCodeString(const char *code, CodeLanguage language, char *traceback, size_t traceback_len)
    {
        esp_err_t ret{ESP_OK};
        SetIsRunning(true);
        esp_task_wdt_deinit();

        switch (language)
        {
        case CodeLanguage::Lua:
            ret |= LuaRunController::RunCodeString(code, traceback, traceback_len);
            break;
        case CodeLanguage::Python:
            ret |= PythonRunController::RunCodeString(code, traceback, traceback_len);
            break;
        default:
            ESP_LOGI(TAG, "Language is not implemented yet");
        }

        esp_task_wdt_config_t wdt_conf{
            .timeout_ms = 5,
            .trigger_panic = true,
        };
        esp_task_wdt_init(&wdt_conf);

        return ret;
    }

    esp_err_t CodeRunController::RunCodeFile(const char *path, CodeLanguage language, char *traceback, size_t traceback_len)
    {
        esp_err_t ret{ESP_OK};
        SetIsRunning(true);

        switch (language)
        {
        case CodeLanguage::Lua:
            ret |= LuaRunController::RunCodeFile(path, traceback, traceback_len);
            break;
        case CodeLanguage::Python:
            ret |= PythonRunController::RunCodeFile(path, traceback, traceback_len);
            break;
        default:
            ESP_LOGI(TAG, "Language is not implemented yet");
        }

        return ret;
    }

    void CodeRunController::SetIsRunning(bool is_running)
    {
        if (xSemaphoreTake(xIsRunningMutex, portMAX_DELAY) == pdPASS)
        {
            CodeRunController::is_running = is_running;
            xSemaphoreGive(xIsRunningMutex);
        }
        else
        {
            ESP_LOGE(TAG, "Error taking xIsRunningMutex");
        }
    }

    bool CodeRunController::IsRunning()
    {
        bool is_running{};

        if (xSemaphoreTake(xIsRunningMutex, portMAX_DELAY) == pdPASS)
        {
            is_running = CodeRunController::is_running;
            xSemaphoreGive(xIsRunningMutex);
        }
        else
        {
            ESP_LOGE(TAG, "Error taking xIsRunningMutex");
        }

        return is_running;
    }

    void CodeRunController::SetIsWaitingInput(bool is_waiting_input)
    {
        if (xSemaphoreTake(xIsWaitingInputMutex, portMAX_DELAY) == pdPASS)
        {
            CodeRunController::is_waiting_input = is_waiting_input;
            xSemaphoreGive(xIsWaitingInputMutex);
        }
        else
        {
            ESP_LOGE(TAG, "Error taking xIsWaitingInputMutex");
        }
    }

    bool CodeRunController::IsWaitingInput()
    {
        bool is_waiting_input{};

        if (xSemaphoreTake(xIsWaitingInputMutex, portMAX_DELAY) == pdPASS)
        {
            is_waiting_input = CodeRunController::is_waiting_input;
            xSemaphoreGive(xIsWaitingInputMutex);
        }
        else
        {
            ESP_LOGE(TAG, "Error taking xIsWaitingInputMutex");
        }

        return is_waiting_input;
    }

    void CodeRunController::SetIsWaitingOutput(bool is_waiting_output)
    {
        if (xSemaphoreTake(xIsWaitingOutputMutex, portMAX_DELAY) == pdPASS)
        {
            CodeRunController::is_waiting_output = is_waiting_output;
            xSemaphoreGive(xIsWaitingOutputMutex);
        }
        else
        {
            ESP_LOGE(TAG, "Error taking xIsWaitingOutputMutex");
        }
    }

    bool CodeRunController::IsWaitingOutput()
    {
        bool is_waiting_output{};

        if (xSemaphoreTake(xIsWaitingOutputMutex, portMAX_DELAY) == pdPASS)
        {
            is_waiting_output = CodeRunController::is_waiting_output;
            xSemaphoreGive(xIsWaitingOutputMutex);
        }
        else
        {
            ESP_LOGE(TAG, "Error taking xIsWaitingOutputMutex");
        }

        return is_waiting_output;
    }
}