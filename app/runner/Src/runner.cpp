#include "runner.h"
#include "lua-runner.h"

static const char *TAG = "CodeRunner";

QueueHandle_t xQueueStdout = NULL;
QueueHandle_t xQueueStdin = NULL;
TaskHandle_t xCodeIOTask = NULL;

static void TaskCodeIO(void *arg);

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

    esp_err_t CodeRunController::Init()
    {
        if (xTaskCreate(TaskCodeIO, "Code IO", 4096, NULL, 10, &xCodeIOTask) != pdPASS)
        {
            return ESP_FAIL;
        }

        return ESP_OK;
    }
}

static void TaskCodeIO(void *arg)
{
    xQueueStdout = xQueueCreate(32, sizeof(char[64]));

    if (xQueueStdout == NULL)
    {
        ESP_LOGE(TAG, "Error creating Stdout Queue");
        vTaskDelete(NULL);
    }

    xQueueStdin = xQueueCreate(64, sizeof(char[1]));

    if (xQueueStdin == NULL)
    {
        ESP_LOGE(TAG, "Error creating Stdin Queue");
        vQueueDelete(xQueueStdout);
        vTaskDelete(NULL);
    }

    char stdout_buffer[64] = {0};

    while (1)
    {
        if (xQueueReceive(xQueueStdout, stdout_buffer, portMAX_DELAY) == pdPASS)
        {
            ESP_LOGI(TAG, "Send from script: %s", stdout_buffer);
            memset(stdout_buffer, 0, sizeof(stdout_buffer));
        }
    }
}