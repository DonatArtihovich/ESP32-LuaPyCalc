#include "scene.h"

static const char *TAG = "Scene";

namespace Scene
{
    Scene::Scene(DisplayController &_display) : display{_display} {}

    uint8_t Scene::Focus(Direction direction)
    {
        auto last_focused = std::find_if(
            ui.begin(),
            ui.end(),
            [](auto item)
            { return item.focused == true; });

        if (last_focused == ui.end())
        {
            ESP_LOGE(TAG, "Last focus not found");
            return -1;
        }

        std::function<void(UiStringItem &)> fe_cb = nullptr;
        UiStringItem *new_focused = nullptr;

        switch (direction)
        {
        case Direction::Up:
            fe_cb = [&new_focused, &last_focused](auto &item)
            {
                if (
                    (!new_focused && item.y > last_focused->y && item.focusable && item.displayable) ||
                    (item.y > last_focused->y && item.focusable && item.displayable && item.y < new_focused->y))
                {
                    new_focused = &item;
                }
            };
            break;
        case Direction::Right:
            fe_cb = [&new_focused, &last_focused](auto &item)
            {
                if (
                    (!new_focused && item.x > last_focused->x && item.focusable && item.displayable) ||
                    (item.x > last_focused->x && item.focusable && item.displayable && item.x < new_focused->x))
                {
                    new_focused = &item;
                }
            };
            break;
        case Direction::Bottom:
            fe_cb = [&new_focused, &last_focused](auto &item)
            {
                if (
                    (!new_focused && item.y < last_focused->y && item.focusable && item.displayable) ||
                    (item.y < last_focused->y && item.focusable && item.displayable && item.y > new_focused->y))
                {
                    new_focused = &item;
                }
            };
            break;
        case Direction::Left:
            fe_cb = [&new_focused, &last_focused](auto &item)
            {
                if (
                    (!new_focused && item.x < last_focused->x && item.focusable && item.displayable) ||
                    (item.x < last_focused->x && item.focusable && item.displayable && item.x > new_focused->x))
                {
                    new_focused = &item;
                }
            };
            break;
        }

        std::for_each(ui.begin(), ui.end(), fe_cb);

        if (new_focused != nullptr)
        {
            ChangeItemFocus(&(*last_focused), false, true);
            ChangeItemFocus(&(*new_focused), true, true);

            ESP_LOGI(TAG, "Last focused %lu: %s", last_focused->id, last_focused->label.c_str());
            ESP_LOGI(TAG, "New focused %lu: %s", new_focused->id, new_focused->label.c_str());
            return 1;
        }

        ESP_LOGI(TAG, "New focus not found.");
        return 0;
    }

    void Scene::ChangeItemFocus(UiStringItem *item, bool focus, bool rerender)
    {
        if (!item->focusable || item->focused == focus)
            return;

        ESP_LOGI(TAG, "Item focus changed to %d", focus);
        item->focused = focus;
        if (focus)
        {
            item->backgroundColor = Color::Blue;
        }
        else if (rerender)
        {
            item->backgroundColor = Color::Black;
        }
        else
        {
            item->backgroundColor = Color::None;
        }

        if (rerender)
        {
            display.DrawStringItem(item, Position::NotSpecified, Position::NotSpecified);
            if (!focus)
                item->backgroundColor = Color::None;
        }
    }

    void Scene::Arrow(Direction direction)
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

    void Scene::ChangeHeader(const char *header, bool rerender)
    {
        ui[0].label = header;
        ESP_LOGI(TAG, "Change header to \"%s\"", ui[0].label.c_str());

        if (rerender)
        {
            ui[0].backgroundColor = Color::Black;
            display.DrawStringItem(&ui[0], Position::Center, Position::End);
            ui[0].backgroundColor = Color::None;
        }
    }

    void Scene::RenderUiListEnding(size_t ui_start, const char *end_label)
    {
        auto last_displayable{
            std::find_if(ui.rbegin(),
                         ui.rend() - ui_start,
                         [](auto &item)
                         { return item.displayable; })
                .base() -
            1};

        if (last_displayable != ui.end())
        {
            display.DrawListEndingLabel(last_displayable,
                                        ui.end() - last_displayable - 1,
                                        end_label);
        }
    }

    void Scene::Delete()
    {
        ESP_LOGI(TAG, "Delete pressed.");
    }
}