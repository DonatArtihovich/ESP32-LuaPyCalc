#include "runner.h"
#include "lua-runner.h"

static const char *TAG = "CodeRunner";

namespace CodeRunner
{

    esp_err_t CodeRunController::RunCodeString(std::string code, CodeLanguage language)
    {
        esp_err_t ret{ESP_OK};

        ESP_LOGI(TAG, "RunCodeString: \"%s\"", code.c_str());
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
}