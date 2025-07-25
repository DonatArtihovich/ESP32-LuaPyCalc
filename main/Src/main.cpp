#include "main.h"
#include "sdkconfig.h"

static const char *TAG = "MAIN";

namespace Main
{
    Main::Main() : scene{new Scene::StartScene{display}} {}

    void Main::Setup()
    {
        ESP_ERROR_CHECK(keyboard.Init());
        ESP_ERROR_CHECK(display.Init());

        if (ESP_OK == sdcard.Mount(CONFIG_MOUNT_POINT))
        {
            ESP_LOGI(TAG, "SD card mounted.");
        }
        else
        {
            ESP_LOGE(TAG, "SD card mount error.");
        }
    }

    void Main::Tick()
    {
        using Keyboard::Key, Scene::Direction;
        Key controllers[]{Key::Enter, Key::Top, Key::Right, Key::Bottom, Key::Left};

        for (auto key : controllers)
        {
            if (keyboard.IsKeyPressed(key))
            {
                switch (key)
                {
                case Key::Enter:
                    ESP_LOGD(TAG, "Enter pressed.");
                    break;
                case Key::Top:
                    ESP_LOGD(TAG, "Top pressed.");
                    scene->Focus(Direction::Up);
                    break;
                case Key::Right:
                    ESP_LOGD(TAG, "Right pressed.");
                    scene->Focus(Direction::Right);
                    break;
                case Key::Bottom:
                    ESP_LOGD(TAG, "Bottom pressed.");
                    scene->Focus(Direction::Bottom);
                    break;
                case Key::Left:
                    ESP_LOGD(TAG, "Left pressed.");
                    scene->Focus(Direction::Left);
                    break;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

extern "C" void app_main(void)
{
    Main::Main app{};
    app.Setup();

    while (1)
    {
        app.Tick();
    }
}
