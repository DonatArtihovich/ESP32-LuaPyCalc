#include "main.h"
#include "sdkconfig.h"

static const char *TAG = "App";

extern QueueHandle_t xQueueRunnerProcessing;
extern QueueHandle_t xQueueRunnerStdout;
extern QueueHandle_t xQueueRunnerStdin;
extern TaskHandle_t xTaskRunnerIO;
extern TaskHandle_t xTaskRunnerProcessing;

extern SemaphoreHandle_t xIsRunningMutex;
extern SemaphoreHandle_t xIsWaitingInputMutex;

SemaphoreHandle_t xAppMutex = NULL;

namespace Main
{
    static void TaskRunnerIO(void *arg)
    {
        Main *App{(Main *)arg};
        xQueueRunnerStdout = xQueueCreate(32, sizeof(char[64]));

        if (xQueueRunnerStdout == NULL)
        {
            ESP_LOGE(TAG, "Error creating Stdout Queue");
            vTaskDelete(NULL);
        }

        xQueueRunnerStdin = xQueueCreate(64, sizeof(char[1]));

        if (xQueueRunnerStdin == NULL)
        {
            ESP_LOGE(TAG, "Error creating Stdin Queue");
            vQueueDelete(xQueueRunnerStdout);
            vTaskDelete(NULL);
        }

        char stdout_buffer[64] = {0};

        while (1)
        {
            if (xQueueReceive(xQueueRunnerStdout, stdout_buffer, portMAX_DELAY) == pdPASS)
            {
                if (xSemaphoreTake(xAppMutex, portMAX_DELAY) == pdPASS)
                {
                    App->SendCodeOutput(stdout_buffer);
                    xSemaphoreGive(xAppMutex);
                }
                memset(stdout_buffer, 0, sizeof(stdout_buffer));
            }
        }
    }

    static void TaskRunnerProcessing(void *arg)
    {
        xQueueRunnerProcessing = xQueueCreate(1, sizeof(CodeRunner::CodeProcess));
        if (xQueueRunnerProcessing == NULL)
        {
            vTaskDelete(NULL);
        }

        xIsRunningMutex = xSemaphoreCreateMutex();
        if (xIsRunningMutex == NULL)
        {
            vQueueDelete(xQueueRunnerProcessing);
            vTaskDelete(NULL);
        }

        xIsWaitingInputMutex = xSemaphoreCreateMutex();
        if (xIsWaitingInputMutex == NULL)
        {
            vQueueDelete(xQueueRunnerProcessing);
            vSemaphoreDelete(xIsRunningMutex);
            vTaskDelete(NULL);
        }

        CodeRunner::CodeProcess processing{};
        while (1)
        {
            if (xQueueReceive(xQueueRunnerProcessing, &processing, portMAX_DELAY) == pdPASS)
            {
                ESP_LOGI(TAG, "Code processing: %s, is file: %d", processing.data.c_str(), processing.is_file);
                if (processing.is_file)
                {
                    CodeRunController::RunCodeFile(processing.data, processing.language);
                }
                else
                {
                    CodeRunController::RunCodeString(processing.data, processing.language);
                }
            }
        }

        vTaskDelete(NULL);
    }

    Main::Main() : scene{new Scene::StartScene{display}} {}

    void Main::Setup()
    {
        ESP_ERROR_CHECK(keyboard.Init());
        ESP_ERROR_CHECK(display.Init());
        ESP_ERROR_CHECK(InitCodeRunner());

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
            scene = std::make_unique<Scene::CodeScene>(display);
            break;
        case SceneId::SettingsScene:
            ESP_LOGI(TAG, "Switch to Settings Scene");
            break;
        }

        scene->Init();
    }

    esp_err_t Main::InitCodeRunner()
    {
        if (xTaskCreate(TaskRunnerIO, "Code IO", 4096, this, 10, &xTaskRunnerIO) != pdPASS)
        {
            return ESP_FAIL;
        }

        if (xTaskCreate(TaskRunnerProcessing, "Code Processing", 4096, NULL, 9, &xTaskRunnerProcessing) != pdPASS)
        {
            vTaskDelete(xTaskRunnerIO);
            return ESP_FAIL;
        }

        return ESP_OK;
    }

    void Main::SendCodeOutput(const char *output)
    {
        scene->SendCodeOutput(output);
    }

}

extern "C" void app_main(void)
{
    xAppMutex = xSemaphoreCreateMutex();
    if (xAppMutex == NULL)
    {
        ESP_LOGE(TAG, "Error creating app mutex");
        vTaskDelete(NULL);
    }

    Main::Main App{};
    if (xSemaphoreTake(xAppMutex, portMAX_DELAY) == pdPASS)
    {
        App.Setup();
        xSemaphoreGive(xAppMutex);
    }

    while (1)
    {
        if (xSemaphoreTake(xAppMutex, portMAX_DELAY) == pdPASS)
        {
            App.Tick();
            xSemaphoreGive(xAppMutex);
        }
    }
}
