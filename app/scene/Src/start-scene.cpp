#include "start-scene.h"

static const char *TAG = "StartScene";

namespace Scene
{
    StartScene::StartScene(DisplayController &display) : Scene{display} {}

    void StartScene::Init()
    {
        ui.push_back(
            Display::UiStringItem{"Menu", Color::White, display.fx32L, Color::None, false});
        ui.push_back(
            Display::UiStringItem{"> Files     ", Color::White, display.fx24G, Color::Blue});
        ui.push_back(
            Display::UiStringItem{"> Code      ", Color::White, display.fx24G});

        display.Clear(Color::Black);
        display.DrawStringItem(&ui[0], Display::Position::Center, Display::Position::End);
        display.DrawStringItems(ui.begin() + 1, 2, 0, display.GetHeight() - 80, true);
    }

    void StartScene::Arrow(Direction direction)
    {
        switch (direction)
        {
        case Direction::Up:
            ESP_LOGI(TAG, "Arrow Up");
            break;
        case Direction::Right:
            ESP_LOGI(TAG, "Arrow Right");
            break;
        case Direction::Bottom:
            ESP_LOGI(TAG, "Arrow Bottom");
            break;
        case Direction::Left:
            ESP_LOGI(TAG, "Arrow Left");
            break;
        }
    }
}