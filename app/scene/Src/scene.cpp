#include "scene.h"

static const char *TAG = "Scene";

namespace Scene
{
    Scene::Scene(DisplayController &_display) : display{_display} {}

    void Scene::Focus(Direction direction)
    {
        auto last_focused = std::find_if(
            ui.begin(),
            ui.end(),
            [](auto item)
            { return item.focused == true; });

        if (last_focused == ui.end())
        {
            ESP_LOGE(TAG, "Last focus not found");
            return;
        }

        std::function<void(UiStringItem &)> fe_cb = nullptr;
        UiStringItem *new_focused = nullptr;

        switch (direction)
        {
        case Direction::Up:
            fe_cb = [&new_focused, &last_focused](auto &item)
            {
                if (
                    (!new_focused && item.y > last_focused->y && item.focusable) ||
                    (item.y > last_focused->y && item.focusable && item.y < new_focused->y))
                {
                    new_focused = &item;
                }
            };
            break;
        case Direction::Right:
            fe_cb = [&new_focused, &last_focused](auto &item)
            {
                if (
                    (!new_focused && item.x > last_focused->x && item.focusable) ||
                    (item.x > last_focused->x && item.focusable && item.x < new_focused->x))
                {
                    new_focused = &item;
                }
            };
            break;
        case Direction::Bottom:
            fe_cb = [&new_focused, &last_focused](auto &item)
            {
                if (
                    (!new_focused && item.y < last_focused->y && item.focusable) ||
                    (item.y < last_focused->y && item.focusable && item.y > new_focused->y))
                {
                    new_focused = &item;
                }
            };
            break;
        case Direction::Left:
            fe_cb = [&new_focused, &last_focused](auto &item)
            {
                if (
                    (!new_focused && item.x < last_focused->x && item.focusable) ||
                    (item.x < last_focused->x && item.focusable && item.x > new_focused->x))
                {
                    new_focused = &item;
                }
            };
            break;
        }

        std::for_each(ui.begin(), ui.end(), fe_cb);

        if (new_focused != nullptr)
        {
            ChangeItemFocus(&(*last_focused), false);
            ChangeItemFocus(&(*new_focused), true);

            ESP_LOGI(TAG, "Last focused %lu: %s", last_focused->id, last_focused->label);
            ESP_LOGI(TAG, "New focused %lu: %s", new_focused->id, new_focused->label);
        }
        else
        {
            ESP_LOGI(TAG, "New focus not found.");
        }
    }

    void Scene::ChangeItemFocus(UiStringItem *item, bool focus)
    {
        if (!item->focusable || item->focused == focus)
            return;

        item->focused = focus;
        if (focus)
        {
            item->backgroundColor = Color::Blue;
        }
        else
        {
            item->backgroundColor = Color::Black;
        }

        display.DrawStringItem(item, Position::NotSpecified, Position::NotSpecified);
    }
}