#include "keyboard.h"

using Sipo::SipoController, Piso::PisoController;
static const char *TAG = "Keyboard";

void TaskReadKeyboard(void *args);

namespace Keyboard
{
    TaskHandle_t TaskReadKeyboardHandler = NULL;

    KeyboardController::KeyboardController(
        gpio_num_t _clk,
        gpio_num_t sipo_lh,
        gpio_num_t sipo_ds,
        gpio_num_t piso_lh,
        gpio_num_t piso_ds)
        : _clk{_clk},
          _sipo_lh{sipo_lh},
          _sipo_ds{sipo_ds},
          _piso_lh{piso_lh},
          _piso_ds{piso_ds} {}

    esp_err_t KeyboardController::Init()
    {
        gpio_num_t pins[6] = {_clk, _sipo_lh, _sipo_ds, _piso_lh, _piso_ds};
        if (pdPASS != xTaskCreate(TaskReadKeyboard, "TaskReadKeyboard", 4096, pins, 10, &TaskReadKeyboardHandler))
        {
            return ESP_FAIL;
        };

        return ESP_OK;
    }
}

void TaskReadKeyboard(void *args)
{
    gpio_num_t *pins{(gpio_num_t *)args};
    SipoController sipo{*pins, *(pins + 1), *(pins + 2)};
    PisoController piso{*pins, *(pins + 3), *(pins + 4)};

    sipo.Init();
    piso.Init();

    while (1)
    {
        Keyboard::KeyState = 0;

        for (uint8_t i = 0; i < 8; i++)
        {
            sipo.SendByte(~(1 << i));
            uint8_t read = piso.ReadByte();
            // ESP_LOGI(TAG, "Row %i: 0x%02X", i, read);
            Keyboard::KeyState = (Keyboard::KeyState << 8) | read;
        }

        ESP_LOGD(TAG, "KeyState 0x%016llx", ~Keyboard::KeyState);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}