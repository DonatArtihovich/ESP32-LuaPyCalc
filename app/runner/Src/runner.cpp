#include "runner.h"
#include "lua-runner.h"

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

    esp_err_t CodeRunController::RunCodeString(std::string code, CodeLanguage language, char *traceback, size_t traceback_len)
    {
        esp_err_t ret{ESP_OK};
        SetIsRunning(true);

        switch (language)
        {
        case CodeLanguage::Lua:
            ret |= LuaRunController::RunCodeString(code.c_str(), traceback, traceback_len);
            break;
        default:
            ESP_LOGI(TAG, "Language is not implemented yet");
        }

        SetIsRunning(false);
        return ret;
    }

    esp_err_t CodeRunController::RunCodeFile(std::string path, CodeLanguage language, char *traceback, size_t traceback_len)
    {
        esp_err_t ret{ESP_OK};
        SetIsRunning(true);

        switch (language)
        {
        case CodeLanguage::Lua:
            ret |= LuaRunController::RunCodeFile(path.c_str(), traceback, traceback_len);
            break;
        default:
            ESP_LOGI(TAG, "Language is not implemented yet");
        }

        SetIsRunning(false);
        return ret;
    }

    void CodeRunController::SetIsRunning(bool is_running)
    {
        while (xSemaphoreTake(xIsRunningMutex, portMAX_DELAY) != pdPASS)
        {
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        CodeRunController::is_running = is_running;
        xSemaphoreGive(xIsRunningMutex);
    }

    bool CodeRunController::IsRunning()
    {
        bool is_running{};

        while (xSemaphoreTake(xIsRunningMutex, portMAX_DELAY) != pdPASS)
        {
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        is_running = CodeRunController::is_running;
        xSemaphoreGive(xIsRunningMutex);

        return is_running;
    }

    void CodeRunController::SetIsWaitingInput(bool is_waiting_input)
    {
        while (xSemaphoreTake(xIsWaitingInputMutex, portMAX_DELAY) != pdPASS)
        {
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        CodeRunController::is_waiting_input = is_waiting_input;
        xSemaphoreGive(xIsWaitingInputMutex);
    }

    bool CodeRunController::IsWaitingInput()
    {
        bool is_waiting_input{};

        while (xSemaphoreTake(xIsWaitingInputMutex, portMAX_DELAY) != pdPASS)
        {
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        is_waiting_input = CodeRunController::is_waiting_input;
        xSemaphoreGive(xIsWaitingInputMutex);

        return is_waiting_input;
    }

    void CodeRunController::SetIsWaitingOutput(bool is_waiting_output)
    {
        while (xSemaphoreTake(xIsWaitingOutputMutex, portMAX_DELAY) != pdPASS)
        {
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        CodeRunController::is_waiting_output = is_waiting_output;
        xSemaphoreGive(xIsWaitingOutputMutex);
    }

    bool CodeRunController::IsWaitingOutput()
    {
        bool is_waiting_output{};

        while (xSemaphoreTake(xIsWaitingOutputMutex, portMAX_DELAY) != pdPASS)
        {
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        is_waiting_output = CodeRunController::is_waiting_output;
        xSemaphoreGive(xIsWaitingOutputMutex);

        return is_waiting_output;
    }
}