#include "runner.h"

static const char *TAG = "CodeRunner";

namespace CodeRunner
{

    esp_err_t CodeRunController::RunCodeString(std::string code, CodeLanguage language)
    {
        esp_err_t ret{ESP_OK};

        ESP_LOGI(TAG, "RunCodeString: \"%s\"", code.c_str());

        return ret;
    }
}