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

        scene->Init();
    }

    void Main::Tick()
    {
        using Keyboard::Key, Scene::Direction, Keyboard::KeyboardController;
        Key controllers[]{Key::Enter, Key::Escape, Key::Delete, Key::Top, Key::Right, Key::Bottom, Key::Left};
        Scene::SceneId sceneId = Scene::SceneId::CurrentScene;

        for (auto key : controllers)
        {
            if (KeyboardController::IsKeyPressed(key))
            {
                switch (key)
                {
                case Key::Enter:
                    ESP_LOGD(TAG, "Enter pressed.");
                    sceneId = scene->Enter();
                    break;
                case Key::Escape:
                    ESP_LOGD(TAG, "Escape pressed.");
                    sceneId = scene->Escape();
                    break;
                case Key::Delete:
                    ESP_LOGD(TAG, "Delete pressed.");
                    scene->Delete();
                    break;
                case Key::Top:
                    ESP_LOGD(TAG, "Top pressed.");
                    scene->Arrow(Direction::Up);
                    break;
                case Key::Right:
                    ESP_LOGD(TAG, "Right pressed.");
                    scene->Arrow(Direction::Right);
                    break;
                case Key::Bottom:
                    ESP_LOGD(TAG, "Bottom pressed.");
                    scene->Arrow(Direction::Bottom);
                    break;
                case Key::Left:
                    ESP_LOGD(TAG, "Left pressed.");
                    scene->Arrow(Direction::Left);
                    break;
                default:
                    break;
                }
            }
        }

        if (sceneId != Scene::SceneId::CurrentScene)
        {
            SwitchScene(sceneId);
        }

        for (int i{(int)Key::NumOne}; i <= (int)Key::LetterM; i++)
        {
            Key key{static_cast<Key>(i)};

            if (KeyboardController::IsKeyPressed(key))
            {
                scene->Value(KeyboardController::GetKeyValue(key));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(150));
    }

    void Main::SwitchScene(Scene::SceneId id)
    {
        using Scene::SceneId;

        switch (id)
        {
        case SceneId::CurrentScene:
            return;
        case SceneId::StartScene:
            ESP_LOGI(TAG, "Switch to Start Scene");
            scene = std::make_unique<Scene::StartScene>(display);
            break;
        case SceneId::FilesScene:
            ESP_LOGI(TAG, "Switch to Files Scene");
            scene = std::make_unique<Scene::FilesScene>(display, sdcard);
            break;
        case SceneId::CodeScene:
            ESP_LOGI(TAG, "Switch to Code Scene");
            break;
        case SceneId::SettingsScene:
            ESP_LOGI(TAG, "Switch to Settings Scene");
            break;
        }

        scene->Init();
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
