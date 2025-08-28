#include "scene.h"

static const char *TAG = "Scene";

extern QueueHandle_t xQueueRunnerStdin;

namespace Scene
{
    Scene::Scene(DisplayController &_display) : display{_display} {}

    void Scene::Value(char value)
    {
        bool is_ctrl_pressed{KeyboardController::IsKeyPressed(Keyboard::Key::Ctrl)};
        ESP_LOGI(TAG, "Entered value: %c, Ctrl pressed: %d", value, is_ctrl_pressed);

        if (IsModalStage())
        {
            Modal &modal{GetStageModal()};
            if (modal.Value)
            {
                modal.Value(value, is_ctrl_pressed);
                if (IsCodeRunning() && value != '\b')
                {
                    stdin_entered++;
                }
            }

            return;
        }

        if (IsCursorControlling())
        {
            if (!is_ctrl_pressed && value != '\b')
            {
                CursorInsertChars(std::string(1, value), GetLinesScroll());
            }
            else if (value == 'v' || value == 'V')
            {
                CursorPaste();
            }
        }
    }

    uint8_t Scene::Focus(Direction direction,
                         std::function<bool(UiStringItem *last_f, UiStringItem *new_f)> add_cond)
    {
        auto last_focused = GetFocused();

        if (last_focused == ui->end())
        {
            return -1;
        }

        std::function<void(UiStringItem &)> fe_cb = nullptr;
        UiStringItem *new_focused = nullptr;

        switch (direction)
        {
        case Direction::Up:
            fe_cb = [&new_focused, &last_focused, &add_cond](auto &item)
            {
                bool main_cond{item.y > last_focused->y &&
                               item.focusable &&
                               item.displayable};

                if ((!new_focused && main_cond) || (main_cond && item.y < new_focused->y))
                {
                    if (add_cond && !add_cond(&item, &*last_focused))
                    {
                        return;
                    }
                    new_focused = &item;
                }
            };
            break;
        case Direction::Right:
            fe_cb = [&new_focused, &last_focused, &add_cond](auto &item)
            {
                uint8_t fw;
                Font::GetFontx(last_focused->font, 0, &fw, 0);
                bool main_cond{item.x > (last_focused->x + last_focused->label.size() * fw) &&
                               item.focusable &&
                               item.displayable};

                if ((!new_focused && main_cond) || (main_cond && item.x < new_focused->x))
                {
                    if (add_cond && !add_cond(&item, &*last_focused))
                    {
                        return;
                    }
                    new_focused = &item;
                }
            };
            break;
        case Direction::Bottom:
            fe_cb = [&new_focused, &last_focused, &add_cond](auto &item)
            {
                bool main_cond{item.y < last_focused->y &&
                               item.focusable &&
                               item.displayable};

                if ((!new_focused && main_cond) || (main_cond && item.y > new_focused->y))
                {
                    if (add_cond && !add_cond(&item, &*last_focused))
                    {
                        return;
                    }
                    new_focused = &item;
                }
            };
            break;
        case Direction::Left:
            fe_cb = [&new_focused, &last_focused, &add_cond](auto &item)
            {
                bool main_cond{item.x < last_focused->x &&
                               item.focusable &&
                               item.displayable};

                if ((!new_focused && main_cond) || (main_cond && item.x > new_focused->x))
                {
                    if (add_cond && !add_cond(&item, &*last_focused))
                    {
                        return;
                    }
                    new_focused = &item;
                }
            };
            break;
        }

        std::for_each(ui->begin(), ui->end(), fe_cb);

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

        if (IsModalStage())
        {
            Modal &modal{GetStageModal()};
            if (modal.Arrow != nullptr)
            {
                GetStageModal().Arrow(direction);
                return;
            }
        }

        if (IsCursorControlling())
        {
            MoveCursor(direction, true, GetLinesScroll());
            return;
        }

        Focus(direction);
    }

    void Scene::ChangeHeader(std::string header, bool rerender)
    {
        UiStringItem &header_item{main_ui[0]};
        header_item.label = header;
        display.SetPosition(&header_item, Position::Center, Position::End);
        ESP_LOGI(TAG, "Change header to \"%s\"", header_item.label.c_str());

        if (rerender)
        {
            RenderHeader();
        }
    }

    void Scene::RenderUiListEnding(const char *end_label)
    {
        auto last_displayable{
            std::find_if(ui->rbegin(),
                         ui->rend() - GetContentUiStartIndex(),
                         [](auto &item)
                         { return item.displayable; })
                .base() -
            1};

        if (last_displayable != ui->end())
        {
            display.DrawListEndingLabel(last_displayable,
                                        ui->end() - last_displayable - 1,
                                        end_label);
        }
    }

    SceneId Scene::Enter()
    {
        if (IsCursorControlling())
        {
            Value('\n');
        }

        return SceneId::CurrentScene;
    }

    void Scene::Delete()
    {
        ESP_LOGI(TAG, "Delete pressed.");
        if (IsCodeRunning())
        {
            if (CodeRunController::IsWaitingInput() && stdin_entered > 0)
            {
                Value('\b');
                CursorDeleteChars(1, GetLinesScroll());
                stdin_entered--;
            }
        }
        else if (IsCursorControlling())
        {
            CursorDeleteChars(1, GetLinesScroll());
        }
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
        return ui->begin() + GetContentUiStartIndex();
    }

    size_t Scene::GetContentUiStartIndex()
    {
        return GetContentUiStartIndex(stage);
    }

    size_t Scene::GetLinesPerPageCount()
    {
        return GetLinesPerPageCount(stage);
    }

    void Scene::RenderAll()
    {
        RenderHeader();
        RenderContent();

        if (IsCursorControlling())
        {
            RenderCursor();
        }
    }

    void Scene::RenderHeader()
    {
        ClearHeader();
        std::for_each(ui->begin(),
                      ui->begin() + content_ui_start,
                      [this](auto item)
                      { display.DrawStringItem(&item); });
    }

    void Scene::RenderContent()
    {
        ClearContent(Color::Black);
        display.DrawStringItems(ui->begin() + content_ui_start,
                                ui->end(),
                                10,
                                display.GetHeight() - 60,
                                GetLinesPerPageCount());
    }

    void Scene::RenderLines(uint8_t first_line, uint8_t last_line, bool clear_line_after, uint8_t start_x)
    {
        uint8_t fw, fh;
        size_t lines_per_page{GetLinesPerPageCount()};
        auto first_displaying = std::find_if(
            GetContentUiStart(),
            ui->end(),
            [](auto &item)
            { return item.displayable; });
        Font::GetFontx(first_displaying->font, 0, &fw, &fh);

        uint16_t lines_start_x{first_displaying->x},
            lines_start_y(first_displaying->y - first_line * fh),
            clear_start_y(first_displaying->y - (first_line * fh) - (last_line - first_line) * fh),
            clear_end_y(clear_start_y + (last_line - first_line + 1) * fh);

        if (clear_line_after && clear_start_y - fh >= 0)
        {
            clear_start_y -= fh;
        }

        if (start_x != 0)
        {
            lines_start_y -= fh;
            clear_end_y -= fh;

            UiStringItem &first_line_item{*(first_displaying + first_line)};

            display.Clear(Color::Black,
                          lines_start_x + start_x * fw,
                          clear_end_y,
                          display.GetWidth(),
                          clear_end_y + fh);

            UiStringItem item{first_line_item};
            item.label.erase(item.label.begin(), item.label.begin() + start_x);
            item.x += start_x * fw;

            display.DrawStringItem(&item);
            if (last_line == first_line)
            {
                return;
            }

            first_line++;
        }

        display.Clear(Color::Black, lines_start_x, clear_start_y, display.GetWidth(), clear_end_y);

        auto first_render_line{first_displaying + first_line};
        auto render_end{first_displaying + last_line + 1};
        if (render_end > ui->end())
        {
            render_end = ui->end();
        }

        display.DrawStringItems(first_render_line, render_end,
                                lines_start_x, lines_start_y,
                                lines_per_page - first_line);

        if (!(ui->end() - 1)->displayable)
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

        auto first_line{GetContentUiStart()};
        *ret_x = static_cast<uint16_t>(x * cursor.width + first_line->x);
        *ret_y = static_cast<uint16_t>(first_line->y - y * cursor.height + 2);
    }

    void Scene::ClearCursor(UiStringItem *line,
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

        ESP_LOGD(TAG, "Clear cursor: x %d, y %d", cursor_x, cursor_y);

        uint16_t x, y;
        GetCursorXY(&x, &y, cursor_x, cursor_y);

        display.Clear(Color::Black, x, y, x + cursor.width, y + cursor.height);
        y -= 2;

        if (line != nullptr)
        {
            char sym = ' ';
            if (cursor_x < line->label.size())
            {
                sym = line->label[cursor_x];
            }
            UiStringItem previous_cursor_pos{
                std::string(1, sym).c_str(),
                line->color,
                line->font,
                false,
                Color::None,
                x, y};

            display.DrawStringItem(&previous_cursor_pos);
        }
    }

    void Scene::RenderCursor()
    {
        if (!IsCursorControlling())
            return;

        uint16_t cursor_x{}, cursor_y{};
        GetCursorXY(&cursor_x, &cursor_y);
        ESP_LOGD(TAG, "Render cursor: x: %d, y: %d", cursor.x, cursor.y);

        display.DrawCursor(
            cursor_x,
            cursor_y,
            cursor.width,
            cursor.height);
    }

    void Scene::SpawnCursor(int16_t cursor_x, int16_t cursor_y, bool clearing, bool rerender)
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
            ui->end(),
            [](auto &item)
            { return item.displayable; }))};

        if (cursor_y > displayable_count - 1)
        {
            cursor_y = displayable_count - 1;
            cursor_x = GetLineLength();
        }

        auto first_displayable{std::find_if(
            GetContentUiStart(),
            ui->end(),
            [](auto &item)
            { return item.displayable; })};

        auto line{first_displayable};

        if (line != ui->end() && line + cursor_y < ui->end())
        {
            line += cursor_y;
        }
        else
            return;

        uint8_t max_cursor_x = (line == ui->end() - 1 &&
                                line->label.size() < GetLineLength())
                                   ? line->label.size()
                                   : line->label.size() - 1;

        if (cursor_x > max_cursor_x)
        {
            cursor_x = max_cursor_x;
        }

        if (line == ui->end() - 1 && cursor_x == line->label.size() &&
            line->label[line->label.size() - 1] == '\n')
        {
            CursorAppendLine();
            cursor_x = 0;
            cursor_y++;
        }

        if (clearing && rerender)
        {
            ClearCursor(&*(first_displayable + cursor.y));
        }
        cursor.x = cursor_x;
        cursor.y = cursor_y;

        ESP_LOGD(TAG, "Spawn cursor X: %d, Cursor Y: %d", cursor.x, cursor.y);
        if (rerender)
        {
            RenderCursor();
        }
    }

    size_t Scene::GetLineLength()
    {
        return default_line_length;
    }

    size_t Scene::MoveCursor(Direction direction, bool rerender, size_t scrolling)
    {
        if (!IsCursorControlling())
            return 0;

        auto line = std::find_if(GetContentUiStart(), ui->end(), [](auto &item)
                                 { return item.displayable; });

        if (line == ui->end())
            return 0;

        line += cursor.y;

        uint16_t cursor_x{cursor.x}, cursor_y{cursor.y};

        uint8_t scrolled_count{};
        bool cursor_changed = true;

        switch (direction)
        {
        case Direction::Up:
        {
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
        }
        case Direction::Right:
        {
            if ((cursor_x < (line->label.size() < GetLineLength() + 1
                                 ? line->label.size() - 1
                                 : GetLineLength())) ||
                line == ui->cend() - 1)
            {
                cursor_x++;
            }
            else if ((line + 1) != ui->end())
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
                                       ui->end(),
                                       [](auto &item)
                                       { return item.displayable; }) -
                                   scrolled_count;
                }
                cursor_x = 0;
            }
            else
                cursor_changed = false;
            break;
        }
        case Direction::Bottom:
        {
            if ((line + 1) != ui->end())
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
                                       ui->end(),
                                       [](auto &item)
                                       { return item.displayable; }) -
                                   scrolled_count;
                }
                else
                    cursor_changed = false;
            }
            break;
        }
        case Direction::Left:
        {
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
        }

        if (cursor_changed)
        {
            SpawnCursor(cursor_x, cursor_y, scrolled_count == 0, rerender);
        }

        return scrolled_count;
    }

    uint8_t Scene::ScrollContent(Direction direction, bool rerender, uint8_t count)
    {
        size_t lines_per_page{GetLinesPerPageCount()};

        if (direction == Direction::Bottom)
        {
            auto first_displayable = std::find_if(
                GetContentUiStart(),
                ui->end(),
                [](auto &item)
                { return item.displayable; });

            auto last_displayable = std::find_if(
                                        ui->rbegin(),
                                        ui->rend() - GetContentUiStartIndex(),
                                        [](auto &item)
                                        { return item.displayable; })
                                        .base() -
                                    1;

            if (last_displayable >= ui->end() - 1 || first_displayable == ui->end())
            {
                return 0;
            }

            if (count > ui->end() - last_displayable)
            {
                count = ui->end() - last_displayable;
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
                 it < ui->end() && it < last_displayable + count + 1;
                 it++)
            {
                it->displayable = true;
            }

            size_t displayable_count = std::count_if(
                GetContentUiStart(),
                ui->end(),
                [](auto &item)
                { return item.displayable; });

            if (displayable_count < lines_per_page - 1)
            {
                auto end{it + lines_per_page - displayable_count - 1};

                for (; it < ui->end() && it < end; it++)
                {
                    it->displayable = true;
                    displayable_count++;
                }

                for (it = ui->end() - displayable_count - 1;
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
                ui->rbegin(),
                ui->rend() - GetContentUiStartIndex(),
                [](auto &item)
                { return item.displayable; });

            auto last_displayable = std::reverse_iterator(std::find_if(
                                        GetContentUiStart(),
                                        ui->end(),
                                        [](auto &item)
                                        { return item.displayable; })) -
                                    1;

            if (last_displayable >= ui->rend() - 1 || first_displayable == ui->rend() - GetContentUiStartIndex())
                return 0;

            if (count > (last_displayable.base() - 1) - GetContentUiStart())
            {
                count = (last_displayable.base() - 1) - GetContentUiStart();
            }

            std::vector<UiStringItem>::reverse_iterator it{};
            for (it = ui->rbegin();
                 it < last_displayable &&
                 it < first_displayable + count;
                 it++)
            {
                it->displayable = false;
            }

            for (it = last_displayable + 1;
                 it < ui->rend() && it < last_displayable + count + 1;
                 it++)
            {
                it->displayable = true;
            }

            size_t displayable_count = std::count_if(
                GetContentUiStart(),
                ui->end(),
                [](auto &item)
                { return item.displayable; });

            if (displayable_count < lines_per_page - 1)
            {
                auto end{it + lines_per_page - displayable_count - 1};

                for (; it < ui->rend() - GetContentUiStartIndex() && it < end; it++)
                {
                    it->displayable = true;
                    displayable_count++;
                }

                for (it = ui->rend() - GetContentUiStartIndex() - displayable_count - 1;
                     displayable_count < lines_per_page - 1 &&
                     it > ui->rbegin();
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

        auto first_displaying = std::find_if(
            GetContentUiStart(),
            ui->end(),
            [](auto &item)
            { return item.displayable; });

        if (initial_x < initial_count &&
            first_displaying + initial_y == GetContentUiStart())
        {
            return;
        }
        size_t first_displaying_index{static_cast<size_t>(first_displaying - ui->begin())};

        size_t count{initial_count};
        size_t start_line_index{first_displaying_index + initial_y};

        if (start_line_index == ui->size() - 1 && (*ui)[start_line_index].label.size() == 0)
        {
            ClearCursor(&(*ui)[start_line_index]);
        }

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

                if (count > 0 && start_line_index != GetContentUiStartIndex())
                {
                    initial_x = (*ui)[--start_line_index].label.size() - 1;

                    count--;
                    initial_y--;
                }
            }
        }
        else
        {
            initial_x -= count;
        }

        size_t delete_x{static_cast<size_t>(initial_x)};
        uint8_t lines_per_page = GetLinesPerPageCount();

        size_t line_index{start_line_index};
        while (count > 0)
        {
            UiStringItem &line{(*ui)[line_index]};
            auto begin{line.label.begin() + delete_x};
            auto end{begin + count > line.label.end() ? line.label.end() : begin + count};
            size_t curr_line_count{static_cast<size_t>(end - begin)};

            line.label.erase(begin, end);
            count -= curr_line_count;

            if (count)
            {
                delete_x = 0;
                line_index++;
            }
        }

        size_t last_changed_index{start_line_index};
        for (size_t i{start_line_index}; i < ui->size() - 1; i++)
        {
            UiStringItem &line{(*ui)[i]};

            if (!(line.label.size() < GetLineLength() &&
                  (!line.label.size() ||
                   line.label[line.label.size() - 1] != '\n')))
            {
                break;
            }

            size_t next_line_index{i + 1};
            UiStringItem *next_line{&(*ui)[next_line_index]};
            while (line.label.size() < GetLineLength())
            {
                while (next_line->label.size() == 0 &&
                       line.label.size() < GetLineLength() &&
                       next_line_index < ui->size() - 1)
                {
                    next_line_index++;
                    next_line = &(*ui)[next_line_index];
                }

                if (next_line->label.size() == 0)
                    break;

                char curr = next_line->label[0];
                line.label.push_back(curr);
                next_line->label.erase(next_line->label.begin());

                if (curr == '\n')
                    break;
            }

            if (next_line->label.size())
            {
                last_changed_index = next_line_index;
            }
            else if (line.label.size())
            {
                last_changed_index = i;
            }
        }

        int erased_count{};
        for (auto it{ui->rbegin()};
             it < ui->rend() - GetContentUiStartIndex() - 1 &&
             it->label.size() == 0 && !((it + 1)->label.ends_with('\n'));
             it++)
        {
            erased_count++;
            ui->erase(it.base() - 1);
        }

        size_t displayable_count = std::count_if(
            GetContentUiStart(),
            ui->end(),
            [](auto &item)
            { return item.displayable; });

        auto last_displayable_find_end{ui->rend() - GetContentUiStartIndex()};
        std::vector<UiStringItem>::iterator last_displayable{
            std::find_if(
                ui->rbegin(),
                last_displayable_find_end,
                [](auto &item)
                { return item.displayable; })
                .base() -
            1};

        if (last_displayable != last_displayable_find_end.base() - 1)
        {
            for (auto it{last_displayable + 1};
                 it < ui->end() &&
                 displayable_count < lines_per_page;
                 it++, displayable_count++)
            {
                it->displayable = true;
            }
        }

        last_changed_index -= first_displaying_index;

        size_t scrolled{};
        while (initial_y < 0 && scrolling > 0)
        {
            scrolled = ScrollContent(Direction::Up, false, scrolling);
            initial_y += scrolled;
        }

        if (initial_y > displayable_count - 1 && initial_y < lines_per_page)
        {
            CursorAppendLine();
        }

        auto first_rerender_line_index{scrolled > 0 ? 0 : initial_y};
        auto last_rerender_line_index{erased_count
                                          ? lines_per_page - 1
                                      : (last_changed_index > lines_per_page - 1)
                                          ? lines_per_page - 1
                                          : last_changed_index};

        if (first_displaying + last_rerender_line_index > ui->end() - 1)
        {
            last_rerender_line_index = ui->end() - 1 - first_displaying;
        }

        if (scrolled > 0)
        {
            RenderContent();
        }
        else
        {
            RenderLines(
                first_rerender_line_index,
                last_rerender_line_index,
                first_displaying + last_rerender_line_index == ui->end() - 1,
                initial_x);
        }

        SpawnCursor(initial_x, initial_y, first_displaying + cursor.y < ui->end());
    }

    void Scene::CursorAppendLine(const char *label, Color color)
    {
        UiStringItem last_line{label, color, (ui->end() - 1)->font, false};
        uint8_t fw, fh;
        Font::GetFontx((ui->end() - 1)->font, 0, &fw, &fh);

        last_line.x = 10;
        last_line.y = (ui->end() - 1)->y - fh;
        ui->push_back(last_line);
    }

    void Scene::CursorInsertLine(std::vector<UiStringItem>::iterator line_before, const char *label, Color color, bool displayable)
    {
        UiStringItem last_line{label, color, line_before->font, false};
        uint8_t fw, fh;
        Font::GetFontx(line_before->font, 0, &fw, &fh);

        last_line.x = line_before->x;
        last_line.y = line_before->y - fh;

        if (!displayable)
        {
            last_line.displayable = false;
        }

        ui->insert(line_before + 1, last_line);
    }

    void Scene::CursorInsertChars(std::string chars, size_t scrolling, bool rerender)
    {
        if (!chars.size())
            return;

        ESP_LOGD(TAG, "Inserting %s", chars.c_str());

        size_t inserting_len = chars.size();
        uint8_t insert_x{cursor.x}, last_insert_y{cursor.y};
        const uint8_t insert_x_initial{cursor.x}, first_insert_y{cursor.y},
            initial_cursor_x{cursor.x},
            initial_cursor_y{cursor.y};

        auto first_displaying = std::find_if(
            GetContentUiStart(),
            ui->end(),
            [](auto &item)
            { return item.displayable; });

        if (first_displaying == ui->end())
        {
            ESP_LOGE(TAG, "First displaying not found");
            return;
        }

        size_t first_displaying_index{static_cast<size_t>(first_displaying - ui->begin())};
        auto start_line_index{first_displaying_index + first_insert_y};

        size_t scrolled_count{0};

        for (size_t line_index{start_line_index}; line_index < ui->size(); line_index++)
        {
            if (!chars.size())
                break;

            if (line_index > start_line_index)
            {
                last_insert_y++;
            }

            int size = chars.size();

            UiStringItem &line{(*ui)[line_index]};

            size_t line_length{GetLineLength()};

            if (line.label.size() + size > line_length)
            {
                size = line_length - line.label.size();
            }

            if (size == 0)
            {
                size_t count{line.label.size() - insert_x};
                chars.append(line.label, line.label.size() - count);
                line.label.erase(line.label.size() - count);

                size = line_length - line.label.size();
            }

            for (int i = 0; i < size; i++)
            {
                line.label.insert(line.label.begin() + insert_x, chars[0]);
                chars.erase(chars.begin());
                insert_x++;
            }

            if (chars.size())
            {
                insert_x = 0;
            }

            size_t find_index = std::string::npos;
            if ((find_index = line.label.find('\n')) != std::string::npos)
            {
                for (auto it{line.label.rbegin()};
                     it < line.label.rend() - find_index - 1; it++)
                {
                    chars.insert(chars.begin(), *it);
                }

                line.label.erase(
                    line.label.begin() + find_index + 1,
                    line.label.end());

                insert_x = 0;
            }

            if (line_index == (ui->size() - 1) &&
                (chars.size() ||
                 line.label[line.label.size() - 1] == '\n' ||
                 (line.label.size() == GetLineLength() && cursor.x == line.label.size() - 1)))
            {
                bool is_new_line_displayable{!(ui->size() - first_displaying_index >= GetLinesPerPageCount())};
                bool is_cursor_moving{cursor.y == line_index - first_displaying_index &&
                                      cursor.x == line.label.size() - 1 &&
                                      !chars.size()};

                CursorAppendLine();

                if (!is_new_line_displayable)
                {
                    (ui->end() - 1)->displayable = false;
                }

                if (is_cursor_moving)
                {
                    scrolled_count += MoveCursor(Direction::Right, false, 1);
                }
            }
        }

        size_t cursor_scrolling_count{0};
        if (scrolling > 0)
        {
            cursor_scrolling_count = last_insert_y - first_insert_y <= scrolling
                                         ? scrolling
                                         : last_insert_y - first_insert_y;
        }

        for (int i{}; i < inserting_len; i++)
        {
            scrolled_count += MoveCursor(Direction::Right, false, cursor_scrolling_count);
        }

        if (rerender)
        {
            if (scrolled_count > 0)
            {
                RenderContent();
            }
            else
            {
                if (last_insert_y > GetLinesPerPageCount() - 1)
                {
                    last_insert_y = GetLinesPerPageCount() - 1;
                }

                RenderLines(first_insert_y, last_insert_y, false, insert_x_initial);
            }
        }
        else
        {
            display.SetListPositions(GetContentUiStart(),
                                     ui->end(),
                                     10,
                                     display.GetHeight() - 60,
                                     GetLinesPerPageCount());
        }

        if (scrolled_count == 0 && rerender)
        {
            ClearCursor(&(*ui)[start_line_index], initial_cursor_x, initial_cursor_y);
        }

        SpawnCursor(-1, -1, false, rerender);
    }

    void Scene::CursorInit(FontxFile *font, uint8_t x, uint8_t y)
    {
        uint8_t fw, fh;
        Font::GetFontx(font, 0, &fw, &fh);
        cursor.x = x;
        cursor.y = y;
        cursor.width = fw;
        cursor.height = fh;
    }

    void Scene::SetStage(uint8_t stage)
    {
        this->stage = stage;
    }

    uint8_t Scene::GetStage()
    {
        return this->stage;
    }

    bool Scene::IsStage(uint8_t stage)
    {
        return this->stage == stage;
    }

    Modal &Scene::GetStageModal(uint8_t stage)
    {
        return modals[stage];
    }

    void Scene::RenderModal()
    {
        display.Clear(Color::Black);
        std::for_each(
            ui->begin(),
            ui->end(),
            [this](auto &item)
            { display.DrawStringItem(&item); });

        if (IsCursorControlling() && !IsCodeRunning())
        {
            RenderCursor();
        }
    }

    void Scene::RenderModalContent()
    {
        display.Clear(Color::Black, 10, 0, display.GetWidth(), display.GetHeight() - 35);
        display.DrawStringItems(GetContentUiStart(),
                                ui->end(),
                                10,
                                display.GetHeight() - 60,
                                GetLinesPerPageCount());
    }

    void Scene::EnterModalControlling()
    {
        if (!IsModalStage())
            return;

        ESP_LOGI(TAG, "Enter %d Stage", GetStage());
        Modal &modal{GetStageModal()};

        if (modal.PreEnter)
        {
            modal.PreEnter();
        }

        ui = &modal.ui;
        RenderModal();
    }

    void Scene::LeaveModalControlling(uint8_t stage, bool rerender)
    {
        if (!IsModalStage())
            return;

        Modal &modal = GetStageModal();
        if (modal.PreLeave)
        {
            modal.PreLeave();
        }

        if (IsHomeStage(stage))
        {
            ui = &main_ui;
        }
        else if (IsModalStage(stage))
        {
            ui = &GetStageModal(stage).ui;
        }

        SetStage(stage);

        if (rerender)
        {
            if (IsHomeStage(stage))
            {
                RenderAll();
            }
            else if (IsModalStage(stage))
            {
                RenderModal();
            }
        }
    }

    void Scene::InitModals() {}

    bool Scene::IsModalStage()
    {
        return modals.count(stage) != 0;
    }

    bool Scene::IsModalStage(uint8_t stage)
    {
        return modals.count(stage) != 0;
    }

    void Scene::AddStageModal(uint8_t stage, Modal modal)
    {
        modals[stage] = modal;
    }

    void Scene::AddModalLabel(std::string modal_label, Modal &modal)
    {
        UiStringItem label_item{"", Color::White, display.fx24G, false};
        display.SetPosition(&label_item, Position::NotSpecified, Position::End);

        uint8_t fw{}, fh{};
        Font::GetFontx(label_item.font, 0, &fw, &fh);

        std::vector<std::string> label_words{};

        std::string word{};
        for (int i{}; i < modal_label.size(); i++)
        {
            if (modal_label[i] != ' ')
            {
                word += modal_label[i];
            }

            if (modal_label[i] == ' ' || i == modal_label.size() - 1)
            {
                label_words.push_back(word);
                word.clear();
            }
        }

        std::for_each(
            label_words.begin(),
            label_words.end(),
            [](auto &word)
            { printf("%s\n", word.c_str()); });

        size_t line_num{0};
        for (int i{}; i < label_words.size(); i++)
        {
            label_item.label += label_words[i] + ' ';

            if (i == label_words.size() - 1 || (label_item.label + label_words[i + 1]).size() > 22)
            {
                if (line_num > 0)
                {
                    label_item.y -= fh;
                }

                display.SetPosition(&label_item, Position::Center);

                modal.ui.insert(modal.ui.begin() + line_num, label_item);

                label_item.label = "";
                line_num++;
            }
        }
    }

    bool Scene::IsHomeStage(uint8_t stage)
    {
        return true;
    }

    size_t Scene::GetLinesScroll()
    {
        return max_lines_per_page;
    }

    void Scene::ClearHeader(Color color)
    {
        display.Clear(color, 0, display.GetHeight() - 60, display.GetWidth(), display.GetHeight());
    }

    void Scene::ClearContent(Color color)
    {
        display.Clear(color, 0, 0, 0, display.GetHeight() - 35);
    }

    size_t Scene::GetContentUiStartIndex(uint8_t stage)
    {
        return content_ui_start;
    }

    size_t Scene::GetLinesPerPageCount(uint8_t stage)
    {
        return max_lines_per_page;
    }

    SceneId Scene::Escape()
    {
        return SceneId::CurrentScene;
    }

    std::vector<UiStringItem>::iterator Scene::GetFocused()
    {
        return GetFocused(ui->begin(), ui->end());
    }

    std::vector<UiStringItem>::iterator Scene::GetFocused(std::vector<UiStringItem>::iterator begin)
    {
        return GetFocused(begin, ui->end());
    }

    std::vector<UiStringItem>::iterator Scene::GetFocused(std::vector<UiStringItem>::iterator begin,
                                                          std::vector<UiStringItem>::iterator end)
    {
        auto focused{std::find_if(begin, end,
                                  [](auto &item)
                                  { return item.focused; })};

        if (focused == end)
        {
            ESP_LOGE(TAG, "Focused item not found");
        }

        return focused;
    }

    void Scene::InitCodeRunModal(uint8_t code_run_stage)
    {
        Modal modal{};

        modal.ui.push_back(UiStringItem{"Running...", Color::White, display.fx32L, false});
        display.SetPosition(&modal.ui[0], Position::Center, Position::End);

        modal.PreEnter = [this, code_run_stage]()
        {
            Modal &modal{GetStageModal(code_run_stage)};

            char cursor_xy[8] = {0};
            snprintf(cursor_xy, sizeof(cursor_xy), "%hhd;%hhd;%hhd", cursor.x, cursor.y, (uint8_t)IsCursorControlling());
            modal.data = cursor_xy;

            CursorInit(display.fx16G);

            modal.ui.push_back(UiStringItem{"", Color::White, display.fx16G, false});
            modal.ui[1].x = 10;
            modal.ui[1].y = display.GetHeight() - 60;

            SetCursorControlling(true);
        };

        modal.PreLeave = [this]()
        {
            Modal &modal{GetStageModal()};
            modal.ui.erase(modal.ui.begin() + 1, modal.ui.end());
            ESP_LOGI(TAG, "CodeRunModal.data: %s", modal.data.c_str());

            if (modal.data.size())
            {
                uint8_t x{}, y{}, is_cursor_controlling{};
                sscanf(
                    modal.data.c_str(),
                    "%hhd;%hhd;%hhd", &x, &y,
                    &is_cursor_controlling);

                CursorInit(display.fx16G, x, y);
                SetCursorControlling(is_cursor_controlling);
                modal.data.clear();
            }
            else
            {
                ESP_LOGE(TAG, "Code Run modal data not found");
            }
        };

        modal.Arrow = [this](Direction direction)
        {
            if (CodeRunController::IsRunning())
                return;
            ESP_LOGI(TAG, "direction %d", (int)direction);
            if (direction == Direction::Up || direction == Direction::Bottom)
            {
                if (ScrollContent(direction, false, GetLinesScroll()) > 0)
                {
                    RenderModalContent();
                    if (!(ui->end() - 1)->displayable)
                    {
                        RenderUiListEnding("more log lines...");
                    }
                }
            }
        };

        modal.Value = [this](char value, bool is_ctrl)
        {
            if (CodeRunController::IsWaitingInput())
            {
                std::string not_displaying{"\b\4"};
                if ((value == 'd' || value == 'D') && is_ctrl)
                {
                    value = '\4';
                }

                if (xQueueSend(xQueueRunnerStdin, &value, portMAX_DELAY) == pdPASS)
                {
                    if (not_displaying.find(value) == std::string::npos)
                    {
                        CursorInsertChars(std::string(1, value));
                    }
                    else if (value == '\4')
                    {
                        CursorInsertChars(std::string(1, '\n'));
                    }
                }
            }
        };

        AddStageModal(code_run_stage, modal);
    }

    void Scene::CursorPaste()
    {
        if (clipboard.size())
        {
            CursorInsertChars(clipboard, GetLinesScroll());
        }
    }

    void Scene::ScrollToEnd()
    {
        for (auto it{GetContentUiStart()}; it < ui->end(); it++)
        {
            if (ui->end() - it > GetLinesPerPageCount())
            {
                it->displayable = false;
            }
            else
            {
                it->displayable = true;
            }
        }
    }

    void Scene::SendCodeOutput(const char *output)
    {
        if (!IsCodeRunning())
            return;

        CursorInsertChars(output, 1, false);
    }

    void Scene::SendCodeError(const char *traceback)
    {
        if (!IsCodeRunning())
            return;

        ESP_LOGE(TAG, "Code error: %s", traceback);

        std::string traceback_str{traceback};
        size_t max_line_len = GetLineLength();
        UiStringItem error_line{"", Color::Red, GetContentUiStart()->font, false};

        size_t index = 0;
        size_t find_n{std::string::npos};

        while (index < traceback_str.length())
        {
            find_n = traceback_str.find('\n', index);
            if (find_n != std::string::npos && find_n - index < max_line_len)
            {
                error_line.label = traceback_str.substr(index, find_n - index + 1);
                ui->push_back(error_line);
                index = find_n + 1;
            }
            else
            {
                error_line.label = traceback_str.substr(index, max_line_len);
                ui->push_back(error_line);
                index += max_line_len;
            }
        }

        ScrollToEnd();
        display.SetListPositions(GetContentUiStart(), ui->end(), 10, display.GetHeight() - 60, GetLinesPerPageCount());
    }

    void Scene::SendCodeSuccess()
    {
        if (!IsCodeRunning())
            return;

        ESP_LOGI(TAG, "Code Successfully executed.");

        UiStringItem end_item{"Successfully executed.", Color::Green, GetContentUiStart()->font, false};
        ui->push_back(end_item);

        ScrollToEnd();
        display.SetListPositions(GetContentUiStart(), ui->end(), 10, display.GetHeight() - 60, GetLinesPerPageCount());
    }

    void Scene::DisplayCodeLog(bool code_end)
    {
        if (!IsCodeRunning())
            return;

        ESP_LOGI(TAG, "Display code log...");

        RenderModalContent();
        if (!code_end)
        {
            RenderCursor();
        }

        // for (auto it{GetContentUiStart()}; it < ui->end(); it++)
        // {
        //     ESP_LOGW(TAG, "After Display: %s, displayable: %d, x: %d, y: %d",
        //              it->label.c_str(),
        //              it->displayable, it->x, it->y);
        // }
    }

    bool Scene::IsCodeRunning()
    {
        return false;
    }

    void Scene::Tab()
    {
        ESP_LOGI(TAG, "Tab pressed.");

        if (IsCursorControlling())
        {
            Value('\t');
            return;
        }

        std::function<bool(UiStringItem *, UiStringItem *)> right_add_cond{
            [](UiStringItem *last_f, UiStringItem *new_f)
            {
                uint8_t fh{};
                Font::GetFontx(last_f->font, 0, 0, &fh);
                return new_f->y >= last_f->y - fh;
            }};

        if (!Scene::Focus(Direction::Right, right_add_cond))
        {
            Scene::Focus(Direction::Bottom);
        }
    }
}