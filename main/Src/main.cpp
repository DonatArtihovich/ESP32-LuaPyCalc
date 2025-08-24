#include "../Inc/main.h"
#include "sdkconfig.h"

static const char *TAG = "App";

extern QueueHandle_t xQueueRunnerProcessing;
extern QueueHandle_t xQueueRunnerStdout;
extern QueueHandle_t xQueueRunnerStdin;
extern TaskHandle_t xTaskRunnerIO;
extern TaskHandle_t xTaskRunnerProcessing;
extern TaskHandle_t xTaskRunnerDisplaying;
extern TaskHandle_t xTaskRunnerWatchdogResetting;

extern SemaphoreHandle_t xIsRunningMutex;
extern SemaphoreHandle_t xIsWaitingInputMutex;
extern SemaphoreHandle_t xIsWaitingOutputMutex;

extern SemaphoreHandle_t xDisplayingSemaphore;

SemaphoreHandle_t xAppMutex = NULL;

namespace Main
{
    static void TaskRunnerIO(void *arg)
    {
        Main *App{(Main *)arg};

        char stdout_buffer[64] = {0};

        while (1)
        {
            if (xQueueReceive(xQueueRunnerStdout, stdout_buffer, portMAX_DELAY) == pdPASS)
            {
                while (xSemaphoreTake(xAppMutex, pdMS_TO_TICKS(100)) != pdPASS)
                {
                    vTaskDelay(pdMS_TO_TICKS(1));
                }

                App->SendCodeOutput(stdout_buffer);
                xSemaphoreGive(xAppMutex);

                if (!uxQueueMessagesWaiting(xQueueRunnerStdout))
                {
                    CodeRunController::SetIsWaitingOutput(false);
                }
                memset(stdout_buffer, 0, sizeof(stdout_buffer));
            }
        }
    }

    static void TaskRunnerProcessing(void *arg)
    {
        Main *App{(Main *)arg};

        static char traceback[300] = {0};
        CodeRunner::CodeProcess processing{};

        while (1)
        {
            if (xQueueReceive(xQueueRunnerProcessing, &processing, portMAX_DELAY) == pdPASS)
            {
                ESP_LOGI(TAG, "Code processing: %s, is file: %d", processing.data, processing.is_file);

                esp_err_t ret{ESP_OK};
                if (processing.is_file)
                {
                    ret = CodeRunController::RunCodeFile(
                        processing.data,
                        processing.language,
                        traceback, sizeof(traceback));
                }
                else
                {
                    ret = CodeRunController::RunCodeString(
                        processing.data,
                        processing.language,
                        traceback, sizeof(traceback));
                }

                ESP_LOGI(TAG, "Code run ret: %d", ret);

                if (ret != ESP_OK)
                {
                    while (xSemaphoreTake(xAppMutex, pdMS_TO_TICKS(100)) != pdPASS)
                    {
                        vTaskDelay(pdMS_TO_TICKS(1));
                    }

                    while (CodeRunController::IsWaitingOutput())
                    {
                        vTaskDelay(pdMS_TO_TICKS(1));
                    }

                    App->SendCodeError(traceback);
                    App->DisplayCodeLog();
                    xSemaphoreGive(xAppMutex);
                    memset(traceback, 0, sizeof(traceback));
                }
                else
                {
                    while (xSemaphoreTake(xAppMutex, pdMS_TO_TICKS(100)) != pdPASS)
                    {
                        vTaskDelay(pdMS_TO_TICKS(1));
                    }

                    while (CodeRunController::IsWaitingOutput())
                    {
                        vTaskDelay(pdMS_TO_TICKS(1));
                    }
                    ESP_LOGI(TAG, "Is not waiting output");

                    App->SendCodeSuccess();
                    App->DisplayCodeLog();
                    xSemaphoreGive(xAppMutex);
                }

                CodeRunController::SetIsRunning(false);
            }
        }

        vTaskDelete(NULL);
    }

    static void TaskRunnerDisplaying(void *arg)
    {
        Main *App{(Main *)arg};

        while (1)
        {
            if (xSemaphoreTake(xDisplayingSemaphore, portMAX_DELAY) == pdPASS)
            {
                while (CodeRunController::IsWaitingOutput())
                {
                    vTaskDelay(pdMS_TO_TICKS(1));
                }

                while (xSemaphoreTake(xAppMutex, pdMS_TO_TICKS(100)) != pdPASS)
                {
                    vTaskDelay(pdMS_TO_TICKS(1));
                }

                App->DisplayCodeLog(false);
                xSemaphoreGive(xAppMutex);
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
        std::vector<QueueHandle_t> queues{};

        auto queues_check = [&queues](QueueHandle_t queue)
        {
            if (queue == NULL)
            {
                for (auto q : queues)
                {
                    if (q != NULL)
                    {
                        vQueueDelete(q);
                    }
                }

                ESP_LOGE(TAG, "Error creating queue");
                vTaskDelete(NULL);
            }
            else
            {
                queues.push_back(queue);
            }
        };

        queues_check(xQueueRunnerStdout = xQueueCreate(32, sizeof(char[64])));
        queues_check(xQueueRunnerStdin = xQueueCreate(64, sizeof(char[1])));
        queues_check(xQueueRunnerProcessing = xQueueCreate(1, sizeof(CodeRunner::CodeProcess)));

        std::vector<SemaphoreHandle_t> semphrs{};

        auto sem_check = [&semphrs, &queues](SemaphoreHandle_t sem)
        {
            if (sem == NULL)
            {
                for (auto q : queues)
                {
                    if (q != NULL)
                    {
                        vQueueDelete(q);
                    }
                }

                for (auto s : semphrs)
                {
                    if (s != NULL)
                    {
                        vSemaphoreDelete(s);
                    }
                }

                ESP_LOGE(TAG, "Error creating semaphore");
                vTaskDelete(NULL);
            }
            else
            {
                semphrs.push_back(sem);
            }
        };

        sem_check(xAppMutex = xSemaphoreCreateMutex());
        sem_check(xIsWaitingOutputMutex = xSemaphoreCreateMutex());
        sem_check(xIsRunningMutex = xSemaphoreCreateMutex());
        sem_check(xIsWaitingInputMutex = xSemaphoreCreateMutex());
        sem_check(xDisplayingSemaphore = xSemaphoreCreateBinary());

        std::vector<TaskHandle_t> tasks{};

        auto tsk_check = [&tasks, &semphrs, &queues](TaskHandle_t tsk)
        {
            if (tsk == NULL)
            {
                for (auto q : queues)
                {
                    if (q != NULL)
                    {
                        vQueueDelete(q);
                    }
                }

                for (auto s : semphrs)
                {
                    if (s != NULL)
                    {
                        vSemaphoreDelete(s);
                    }
                }

                for (auto t : tasks)
                {
                    if (t != NULL)
                    {
                        vTaskDelete(t);
                    }
                }

                ESP_LOGE(TAG, "Error creating task");
                vTaskDelete(NULL);
            }
            else
            {
                tasks.push_back(tsk);
            }
        };

        xTaskCreate(TaskRunnerIO, "Code IO", 4096, this, 10, &xTaskRunnerIO);
        tsk_check(xTaskRunnerIO);

        xTaskCreate(TaskRunnerProcessing, "Code Processing", 10 * 1024, this, 9, &xTaskRunnerProcessing);
        tsk_check(xTaskRunnerProcessing);

        xTaskCreate(TaskRunnerDisplaying, "Code Log Displaying", 2048, this, 8, &xTaskRunnerDisplaying);
        tsk_check(xTaskRunnerDisplaying);

        ESP_LOGI(TAG, "Init FreeRTOS Runner objects.");
        return ESP_OK;
    }

    void Main::SendCodeOutput(const char *output)
    {
        scene->SendCodeOutput(output);
    }

    void Main::SendCodeError(const char *traceback)
    {
        scene->SendCodeError(traceback);
    }

    void Main::SendCodeSuccess()
    {
        scene->SendCodeSuccess();
    }

    void Main::DisplayCodeLog(bool is_end)
    {
        scene->DisplayCodeLog(is_end);
    }
}

extern "C" void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);
    Main::Main App{};
    App.Setup();

    while (1)
    {
        while (xSemaphoreTake(xAppMutex, pdMS_TO_TICKS(100)) != pdPASS)
        {
            vTaskDelay(pdMS_TO_TICKS(1));
        }

        App.Tick();
        xSemaphoreGive(xAppMutex);
    }
}
