#include "start-scene.h"

static const char *TAG = "StartScene";

namespace Scene
{
    StartScene::StartScene(DisplayController &display) : Scene{display} {}

    void StartScene::Init()
    {
        display.Clear(Color::Black);
        display.DrawMenu(display.fx32L, "Menu", Color::White, display.fx24M, menu, Color::White);
    }

    void StartScene::Focus(Direction direction)
    {
        switch (direction)
        {
        case Direction::Up:
            ESP_LOGI(TAG, "Focus Up");
            break;
        case Direction::Right:
            ESP_LOGI(TAG, "Focus Right");
            break;
        case Direction::Bottom:
            ESP_LOGI(TAG, "Focus Bottom");
            break;
        case Direction::Left:
            ESP_LOGI(TAG, "Focus Left");
            break;
        }
    }
}