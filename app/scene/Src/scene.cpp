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

    void Scene::RenderUiListEnding(const char *end_label)
    {
        auto last_displayable{
            std::find_if(ui.rbegin(),
                         ui.rend() - GetContentUiStartIndex(),
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

    bool Scene::IsCursorControlling()
    {
        return is_cursor_controlling;
    }

    void Scene::SetCursorControlling(bool cursor_controlling)
    {
        is_cursor_controlling = cursor_controlling;
    }

    std::vector<UiStringItem>::iterator Scene::GetContentUiStart()
    {
        return ui.begin() + GetContentUiStartIndex();
    }

    void Scene::RenderLines(uint8_t first_line, uint8_t last_line)
    {
        uint8_t fw, fh;
        uint8_t lines_per_page{GetLinesPerPageCount()};
        auto first_displaying = std::find_if(GetContentUiStart(), ui.end(), [](auto &item)
                                             { return item.displayable; });
        Font::GetFontx(first_displaying->font, 0, &fw, &fh);

        uint16_t lines_start_x{10},
            lines_start_y(display.GetHeight() - 60 - first_line * fh),
            clear_start_y(display.GetHeight() - 60 - (first_line * fh) - (last_line - first_line) * fh),
            clear_end_y(clear_start_y + (last_line - first_line + 1) * fh);

        display.Clear(Color::Black, lines_start_x, clear_start_y, display.GetWidth(), clear_end_y);
        display.DrawStringItems(first_displaying + first_line,
                                first_displaying + last_line + 1,
                                lines_start_x,
                                lines_start_y,
                                lines_per_page - first_line);

        if (!(ui.end() - 1)->displayable)
        {
            RenderUiListEnding("more lines");
        }
    }

    void Scene::GetCursorXY(uint16_t *ret_x, uint16_t *ret_y, int16_t x, int16_t y)
    {
        if (x < 0)
        {
            x = cursor.x;
        }

        if (y < 0)
        {
            y = cursor.y;
        }

        *ret_x = static_cast<uint16_t>(x * cursor.width + 10);
        *ret_y = static_cast<uint16_t>(display.GetHeight() - 60 - y * cursor.height + 2);
    }

    void Scene::ClearCursor(std::vector<UiStringItem>::iterator line,
                            int16_t cursor_x,
                            int16_t cursor_y)
    {
        if (cursor_x < 0)
        {
            cursor_x = cursor.x;
        }
        if (cursor_y < 0)
        {
            cursor_y = cursor.y;
        }

        ESP_LOGI(TAG, "Clear cursor: x %d, y %d", cursor_x, cursor_y);

        uint16_t x, y;
        GetCursorXY(&x, &y, cursor_x, cursor_y);

        display.Clear(Color::Black, x, y, x + cursor.width, y + cursor.height);
        y -= 2;

        UiStringItem previous_cursor_pos{
            std::string(1, line->label[cursor_x]).c_str(),
            line->color,
            line->font,
            false,
            Color::None,
            x, y};

        display.DrawStringItem(&previous_cursor_pos);
    }

    void Scene::RenderCursor()
    {
        if (!IsCursorControlling())
            return;

        uint16_t cursor_x{}, cursor_y{};
        GetCursorXY(&cursor_x, &cursor_y);

        display.DrawCursor(
            cursor_x,
            cursor_y,
            cursor.width,
            cursor.height);
    }

    void Scene::SpawnCursor(int16_t cursor_x, int16_t cursor_y, bool clearing)
    {
        if (cursor_x < 0)
        {
            cursor_x = cursor.x;
        }

        if (cursor_y < 0)
        {
            cursor_y = cursor.y;
        }

        size_t displayable_count{static_cast<size_t>(std::count_if(
            GetContentUiStart(),
            ui.end(),
            [](auto &item)
            { return item.displayable; }))};

        if (cursor_y > displayable_count - 1)
        {
            cursor_y = displayable_count - 1;
            cursor_x = GetLineLength();
        }

        auto line{std::find_if(
            GetContentUiStart(),
            ui.end(),
            [](auto &item)
            { return item.displayable; })};

        if (line != ui.end() && line + cursor_y < ui.end())
        {
            line += cursor_y;
        }
        else
            return;

        uint8_t max_cursor_x = (line == ui.end() - 1 &&
                                line->label.size() < GetLineLength())
                                   ? line->label.size()
                                   : line->label.size() - 1;

        if (cursor_x > max_cursor_x)
        {
            cursor_x = max_cursor_x;
        }

        if (clearing)
        {
            ClearCursor(line - cursor_y + cursor.y);
        }
        cursor.x = cursor_x;
        cursor.y = cursor_y;

        ESP_LOGI(TAG, "Spawn cursor X: %d, Cursor Y: %d", cursor.x, cursor.y);
        RenderCursor();
    }

    size_t Scene::GetLineLength()
    {
        return default_line_length;
    }

    void Scene::MoveCursor(Direction direction, bool rerender, size_t scrolling)
    {
        if (!IsCursorControlling())
            return;

        auto line = std::find_if(GetContentUiStart(), ui.end(), [](auto &item)
                                 { return item.displayable; });

        if (line == ui.end())
            return;

        line += cursor.y;
        ESP_LOGI(TAG, "Current line: %s", line->label.c_str());

        uint16_t cursor_x{cursor.x}, cursor_y{cursor.y};

        uint8_t scrolled_count{};
        bool cursor_changed = true;

        switch (direction)
        {
        case Direction::Up:
            if (cursor_y > 0 || line > GetContentUiStart())
            {
                if (cursor_y)
                {
                    cursor_y--;
                }
                else if (scrolling > 0)
                {
                    scrolled_count = ScrollContent(Direction::Up, rerender, scrolling);
                    if (scrolled_count)
                        cursor_y = scrolled_count - 1;
                }
            }
            else
                cursor_changed = false;
            break;
        case Direction::Right:
            if ((cursor_x < (line->label.size() < GetLineLength() + 1
                                 ? line->label.size() - 1
                                 : GetLineLength())) ||
                line == ui.cend() - 1)
            {
                cursor_x++;
            }
            else if ((line + 1) != ui.end())
            {
                if (cursor_y < GetLinesPerPageCount() - 1)
                {
                    cursor_y++;
                }
                else if (scrolling > 0)
                {
                    scrolled_count = ScrollContent(Direction::Bottom, rerender, scrolling);
                    if (scrolled_count)
                        cursor_y = std::count_if(
                                       GetContentUiStart(),
                                       ui.end(),
                                       [](auto &item)
                                       { return item.displayable; }) -
                                   scrolled_count;
                }
                cursor_x = 0;
            }
            else
                cursor_changed = false;
            break;
        case Direction::Bottom:
            if ((line + 1) != ui.end())
            {
                if (cursor_y < GetLinesPerPageCount() - 1)
                {
                    cursor_y++;
                }
                else if (scrolling > 0)
                {
                    scrolled_count = ScrollContent(Direction::Bottom, rerender, scrolling);
                    if (scrolled_count)
                        cursor_y = std::count_if(
                                       GetContentUiStart(),
                                       ui.end(),
                                       [](auto &item)
                                       { return item.displayable; }) -
                                   scrolled_count;
                }
                else
                    cursor_changed = false;
            }
            break;
        case Direction::Left:
            if (cursor_x > 0)
            {
                cursor_x--;
            }
            else if (cursor_y > 0 || line > GetContentUiStart())
            {
                if (cursor_y > 0)
                {
                    cursor_y--;
                }
                else if (scrolling > 0)
                {
                    scrolled_count = ScrollContent(Direction::Up, rerender, scrolling);

                    if (scrolled_count)
                        cursor_y = scrolled_count - 1;
                }

                cursor_x = GetLineLength();
            }
            else
                cursor_changed = false;
            break;
        }

        if (cursor_changed)
        {
            SpawnCursor(cursor_x, cursor_y, scrolled_count == 0);
        }
    }

    uint8_t Scene::ScrollContent(Direction direction, bool rerender, uint8_t count)
    {
        size_t lines_per_page{GetLinesPerPageCount()};

        if (direction == Direction::Bottom)
        {
            auto first_displayable = std::find_if(
                GetContentUiStart(),
                ui.end(),
                [](auto &item)
                { return item.displayable; });

            auto last_displayable = std::find_if(
                                        ui.rbegin(),
                                        ui.rend() - GetContentUiStartIndex(),
                                        [](auto &item)
                                        { return item.displayable; })
                                        .base() -
                                    1;

            if (last_displayable >= ui.end() - 1 || first_displayable == ui.end())
                return 0;

            ESP_LOGI(TAG, "LAST DISPLAYABLE \"%s\"", last_displayable->label.c_str());
            ESP_LOGI(TAG, "FIRST DISPLAYABLE \"%s\"", first_displayable->label.c_str());
            ESP_LOGI(TAG, "COUNT \"%d\"", count);

            if (count > ui.end() - last_displayable - 1)
            {
                count = ui.end() - last_displayable - 1;
                ESP_LOGI(TAG, "COUNT \"%d\"", count);
            }

            std::vector<UiStringItem>::iterator it{};
            for (it = GetContentUiStart();
                 it < last_displayable &&
                 it < first_displayable + count;
                 it++)
            {
                it->displayable = false;
            }

            for (it = last_displayable + 1;
                 it < ui.end() && it < last_displayable + count + 1;
                 it++)
            {
                it->displayable = true;
            }

            size_t displayable_count = std::count_if(
                GetContentUiStart(),
                ui.end(),
                [](auto &item)
                { return item.displayable; });

            if (displayable_count < lines_per_page - 1)
            {
                auto end{it + lines_per_page - displayable_count - 1};

                for (; it < ui.end() && it < end; it++)
                {
                    it->displayable = true;
                    displayable_count++;
                }

                for (it = ui.end() - displayable_count - 1;
                     displayable_count < lines_per_page - 1 &&
                     it > GetContentUiStart();
                     it--)
                {
                    it->displayable = true;
                    displayable_count++;
                }
            }

            return count;
        }

        if (direction == Direction::Up)
        {
            auto first_displayable = std::find_if(
                ui.rbegin(),
                ui.rend() - GetContentUiStartIndex(),
                [](auto &item)
                { return item.displayable; });

            auto last_displayable = std::reverse_iterator(std::find_if(
                                        GetContentUiStart(),
                                        ui.end(),
                                        [](auto &item)
                                        { return item.displayable; })) -
                                    1;

            if (last_displayable >= ui.rend() - 1 || first_displayable == ui.rend())
                return 0;

            if (count > ui.rend() - GetContentUiStartIndex() - last_displayable - 1)
            {
                count = ui.rend() - GetContentUiStartIndex() - last_displayable - 1;
                ESP_LOGI(TAG, "COUNT \"%d\"", count);
            }

            std::vector<UiStringItem>::reverse_iterator it{};
            for (it = ui.rbegin();
                 it < last_displayable &&
                 it < first_displayable + count;
                 it++)
            {
                it->displayable = false;
            }

            for (it = last_displayable + 1;
                 it < ui.rend() && it < last_displayable + count + 1;
                 it++)
            {
                it->displayable = true;
            }

            size_t displayable_count = std::count_if(
                GetContentUiStart(),
                ui.end(),
                [](auto &item)
                { return item.displayable; });

            if (displayable_count < lines_per_page - 1)
            {
                auto end{it + lines_per_page - displayable_count - 1};

                for (; it < ui.rend() - GetContentUiStartIndex() && it < end; it++)
                {
                    it->displayable = true;
                    displayable_count++;
                }

                for (it = ui.rend() - GetContentUiStartIndex() - displayable_count - 1;
                     displayable_count < lines_per_page - 1 &&
                     it > ui.rbegin();
                     it--)
                {
                    it->displayable = true;
                    displayable_count++;
                }
            }

            return count;
        }

        return 0;
    }

    void Scene::CursorDeleteChars(size_t initial_count, size_t scrolling, int16_t initial_x, int16_t initial_y)
    {
        if (!IsCursorControlling())
            return;

        if (initial_x < 0)
        {
            initial_x = cursor.x;
        }

        if (initial_y < 0)
        {
            initial_y = cursor.y;
        }

        if (!initial_count)
            return;

        size_t count{initial_count};

        auto first_displaying = std::find_if(
            GetContentUiStart(),
            ui.end(),
            [](auto &item)
            { return item.displayable; });
        auto start_line{first_displaying + initial_y};

        if (count > GetLineLength())
        {
            return;
        }
        else if (initial_x < initial_count)
        {
            int8_t count{static_cast<int8_t>(initial_count)};

            while (count > 0)
            {
                if (initial_x > count)
                {
                    initial_x -= count;
                    break;
                }
                else
                {
                    count -= initial_x;
                    initial_x -= count;
                }

                ESP_LOGI(TAG, "Count -= initial X: %d - %d = %d", count + initial_x, initial_x, count);
                if (count > 0 && start_line != GetContentUiStart())
                {
                    initial_x = (--start_line)->label.size() - 1;
                    count--;
                    initial_y--;
                    ESP_LOGI(TAG, "Count, initial X, initial Y: %d, %d, %d", count, initial_x, initial_y);
                }
            }
        }
        else
        {
            initial_x -= count;
        }

        ESP_LOGI(TAG, "Initial X: %d, Initial Y: %d", initial_x, initial_y);

        size_t delete_x{static_cast<size_t>(initial_x)};
        uint8_t lines_per_page = GetLinesPerPageCount();

        auto line{start_line};
        while (count > 0)
        {
            auto begin{line->label.begin() + delete_x};
            auto end{begin + count > line->label.end() ? line->label.end() : begin + count};
            size_t curr_line_count{static_cast<size_t>(end - begin)};

            std::string deleting{};
            std::for_each(begin, end, [&deleting](auto &item)
                          { deleting.push_back(item); });
            ESP_LOGI(TAG, "Deleting %d chars: \"%s\"", curr_line_count, deleting.c_str());

            line->label.erase(begin, end);
            count -= curr_line_count;

            if (count)
            {
                delete_x = 0;
                line++;
            }
        }

        auto last_changed{start_line};
        for (auto it{start_line}; it < ui.end() - 1 &&
                                  (it->label.size() < GetLineLength() &&
                                   (!it->label.size() ||
                                    it->label[it->label.size() - 1] != '\n'));
             it++)
        {
            auto next_line{it + 1};
            while (it->label.size() < GetLineLength())
            {
                while (next_line->label.size() == 0 &&
                       it->label.size() < GetLineLength() &&
                       next_line < ui.end() - 1)
                {
                    next_line++;
                }

                if (next_line->label.size() == 0)
                    break;

                char curr = next_line->label[0];
                it->label.push_back(curr);
                next_line->label.erase(next_line->label.begin());

                if (curr == '\n')
                    break;
            }

            if (next_line->label.size())
            {
                last_changed = next_line;
            }
            else if (it->label.size())
            {
                last_changed = it;
            }
        }

        int erased_count{};
        for (auto it{ui.rbegin()};
             it < ui.rend() - GetContentUiStartIndex() &&
             !it->label.size();
             it++)
        {
            if (it->label.size() == 0)
            {
                erased_count++;
                ui.erase(it.base() - 1);
            }
        }

        size_t displayable_diff = lines_per_page -
                                  std::count_if(
                                      GetContentUiStart(),
                                      ui.end(),
                                      [](auto &item)
                                      { return item.displayable; });

        std::vector<UiStringItem>::iterator last_displayable{
            std::find_if(
                ui.rbegin(),
                ui.rend() - GetContentUiStartIndex(),
                [](auto &item)
                { return item.displayable; })
                .base() -
            1};

        if (displayable_diff > 0)
        {
            for (; last_displayable < ui.end() && displayable_diff > 0;
                 last_displayable++, displayable_diff--)
            {
                last_displayable->displayable = true;
            }
        }

        size_t last_changed_index{static_cast<size_t>(last_changed - (last_displayable - lines_per_page) - 1)};

        size_t scrolled{};
        while (initial_y < 0 && scrolling > 0)
        {
            ESP_LOGI(TAG, "Initial Y < 0: %d", initial_y);
            scrolled = ScrollContent(Direction::Up, false, scrolling);
            initial_y += scrolled;
        }

        RenderLines(
            scrolled > 0 ? 0 : initial_y,
            erased_count
                ? lines_per_page - 1
            : (last_changed_index > lines_per_page - 1)
                ? lines_per_page - 1
                : last_changed_index);

        SpawnCursor(initial_x, initial_y);
    }

    void Scene::CursorInsertChars(std::string chars)
    {
        if (!chars.size())
            return;

        uint8_t insert_x{cursor.x}, insert_y{cursor.y};
        auto first_displaying = std::find_if(GetContentUiStart(), ui.end(), [](auto &item)
                                             { return item.displayable; });
        auto start_line{first_displaying + insert_y};
        uint8_t last_rendering_y{insert_y};

        for (auto line{start_line}; line < ui.end(); line++)
        {
            if (!chars.size())
                break;

            if (last_rendering_y < GetLinesPerPageCount() - 1 && line > start_line)
            {
                last_rendering_y++;
            }

            std::for_each(
                chars.begin(),
                chars.end(),
                [&chars, &line, &insert_x](auto &curr_char)
                {
                    line->label.insert(line->label.cbegin() + insert_x, chars[0]);
                    chars.erase(chars.begin());
                    insert_x++;
                });

            size_t start_index = 0;
            if (line->label.size() > GetLineLength())
            {
                start_index = GetLineLength();
                ESP_LOGI(TAG, "Start_index: %d", start_index);

                std::for_each(
                    line->label.begin() + start_index,
                    line->label.end(),
                    [&chars](auto &item)
                    { chars.push_back(item); });

                line->label.erase(
                    line->label.end() - (line->label.size() - start_index),
                    line->label.end());

                insert_x = 0;
            }

            size_t find_index = std::string::npos;
            if ((find_index = line->label.find('\n')) != std::string::npos)
            {
                ESP_LOGI(TAG, "\\n Find_index: %d", find_index);
                std::for_each(
                    line->label.rbegin(),
                    line->label.rend() - find_index - 1,
                    [&chars](auto &item)
                    { chars.insert(chars.begin(), item); });

                std::for_each(line, line + 4 > ui.end() ? ui.end() : line + 4, [](auto &item)
                              { printf("Line: \"%s\"\n", item.label.c_str()); });

                line->label.erase(
                    line->label.begin() + find_index + 1,
                    line->label.end());

                ESP_LOGI(TAG, "Line after erasing: \"%s\"", line->label.c_str());

                insert_x = 0;
            }

            if (line == ui.end() - 1 && chars.size())
            {
                ui.push_back(UiStringItem{std::string(""), line->color, line->font, false});
                if (ui.end() - 1 - start_line > GetLinesPerPageCount())
                {
                    (ui.end() - 1)->displayable = false;
                }
            }
        }

        ESP_LOGI(TAG, "Insert y: %d, last rendering y: %d", insert_y, last_rendering_y);
        RenderLines(insert_y, last_rendering_y);
        MoveCursor(Direction::Right);
    }

    void Scene::CursorInit(Cursor *c)
    {
        cursor.x = c->x;
        cursor.y = c->y;
        cursor.width = c->width;
        cursor.height = c->height;
    }
}