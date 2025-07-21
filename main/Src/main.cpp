#include "main.h"

extern "C" void app_main(void)
{
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
