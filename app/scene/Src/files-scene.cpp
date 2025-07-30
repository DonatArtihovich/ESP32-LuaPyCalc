#include "files-scene.h"

static const char *TAG = "FileScene";

namespace Scene
{
    FilesScene::FilesScene(DisplayController &display, SDCard &_sdcard)
        : Scene(display), sdcard{_sdcard} {}

    void FilesScene::Init()
    {
        ui.push_back(UiStringItem{"  Files   ", Color::White, display.fx32L, false});
        ui.push_back(UiStringItem{"< Esc", Color::White, display.fx24G});
        ui.push_back(UiStringItem{"", Color::White, display.fx24M, false});

        if (ReadDirectory(content_ui_start + 1))
        {
            ChangeItemFocus(&ui[content_ui_start + 1], true);
        }
        else
        {
            ChangeItemFocus(&ui[1], true);
        }

        RenderAll();
    }

    void FilesScene::RenderAll()
    {
        RenderHeader();
        RenderContent(content_ui_start);
        if (isCursorControlling)
        {
            RenderCursor();
        }
    }

    void FilesScene::RenderContent(int ui_start, bool file)
    {
        display.Clear(Color::Black, 10, 0, 0, display.GetHeight() - 35);
        display.DrawStringItems(ui.begin() + ui_start,
                                ui.end(),
                                10,
                                display.GetHeight() - 60,
                                file ? file_lines_per_page : directory_lines_per_page,
                                file ? "more lines..." : "more items...");
    }

    size_t FilesScene::ReadDirectory(int ui_start)
    {
        std::vector<std::string> files{sdcard.ReadDirectory(curr_directory.c_str())};
        ui.erase(ui.begin() + ui_start, ui.end());

        if (!curr_directory.ends_with("sdcard"))
        {
            ui.push_back(UiStringItem{"..", Color::White, display.fx16G});
        }
        if (!files.size())
        {
            ui.push_back(UiStringItem{"No files found", Color::White, display.fx16G, false});
        }
        else
        {
            for (auto &file : files)
            {
                ui.push_back(UiStringItem{file.c_str(), Color::White, display.fx16G});
            }
        }

        return files.size();
    }

    void FilesScene::Arrow(Direction direction)
    {
        Scene::Arrow(direction);
        if (isFileOpened && !isCursorControlling && direction == Direction::Bottom)
        {
            isCursorControlling = true;
            auto focused = std::find_if(ui.begin(), ui.end(), [this](auto &item)
                                        { return item.focused; });
            if (focused != ui.end())
            {
                ChangeItemFocus(&(*focused), false, true);
            }

            SpawnCursor(cursor.x, cursor.y, false);
            return;
        }

        if (isCursorControlling)
        {
            MoveCursor(direction);
            return;
        }

        Focus(direction);
    }

    SceneId FilesScene::Enter()
    {
        if (isCursorControlling)
        {
            InsertChars("Q");
            return SceneId::CurrentScene;
        }
        size_t index = -1;
        auto focused = std::find_if(
            ui.begin(),
            ui.end(),
            [&index](auto &item) mutable
            { index++; return item.focused; });

        if (focused == ui.end())
        {
            return SceneId::CurrentScene;
        }

        if (index > content_ui_start)
        {
            if (focused->label == "..")
            {
                OpenDirectory(focused->label.c_str());
                return SceneId::CurrentScene;
            }

            char filename[30] = {0};
            snprintf(filename, 29, "/%s", focused->label.c_str());

            if (sdcard.IsDirectory((curr_directory + filename).c_str()))
            {
                ESP_LOGI(TAG, "%s is directory", filename);
                OpenDirectory(filename);
            }
            else
            {
                ESP_LOGI(TAG, "%s isn't directory", filename);
                OpenFile(filename);
            }
        }
        else if (focused->label.contains("^ Up"))
        {
            ScrollContent(Direction::Up, true, directory_lines_scroll);
        }
        else if (focused->label.contains("< Esc"))
        {
            return Escape();
        }

        return SceneId::CurrentScene;
    }

    SceneId FilesScene::Escape()
    {
        if (isFileOpened)
        {
            if (isCursorControlling)
            {
                ChangeItemFocus(&ui[1], true, true);
                isCursorControlling = false;
            }
            else
            {
                CloseFile();
            }
            return SceneId::CurrentScene;
        }

        return SceneId::StartScene;
    }

    uint8_t FilesScene::Focus(Direction direction)
    {
        if (!Scene::Focus(direction))
        {
            ESP_LOGI(TAG, "Basic focus not found");
            if (direction == Direction::Bottom)
            {
                ScrollContent(direction, true, directory_lines_scroll);
            }
            // else if (direction == Direction::Up)
            // {
            //     ScrollContent(direction, true, 5);
            // }
        }

        return true;
    }

    uint8_t FilesScene::ScrollContent(Direction direction, bool rerender, uint8_t count)
    {
        size_t lines_per_page{isFileOpened ? file_lines_per_page : directory_lines_per_page};

        if (count > lines_per_page - 1 - !isFileOpened)
            count = lines_per_page - 1 - !isFileOpened;
        size_t content_ui_gap{content_ui_start + !isFileOpened};

        if (direction == Direction::Bottom)
        {
            auto first_displayable = std::find_if(
                ui.begin() + content_ui_gap,
                ui.end(),
                [](auto &item)
                { return item.displayable; });

            auto last_displayable = std::find_if(
                                        ui.rbegin(),
                                        ui.rend() - content_ui_gap,
                                        [](auto &item)
                                        { return item.displayable; })
                                        .base() -
                                    1;

            if (last_displayable >= ui.end() - 1 || first_displayable == ui.end())
                return 0;

            ESP_LOGI(TAG, "LAST DISPLAYABLE \"%s\"", last_displayable->label.c_str());
            ESP_LOGI(TAG, "FIRST DISPLAYABLE \"%s\"", first_displayable->label.c_str());
            ESP_LOGI(TAG, "COUNT \"%d\"", count);

            std::vector<UiStringItem>::iterator it{};
            for (it = ui.begin() + content_ui_gap;
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
                ui.begin() + content_ui_gap,
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
                     it > ui.begin() + content_ui_gap;
                     it--)
                {
                    it->displayable = true;
                    displayable_count++;
                }
            }

            if (!ui[content_ui_start].focusable &&
                !isFileOpened)
            {
                ToggleUpButton(true);
            }

            if (rerender)
            {
                RenderContent(content_ui_start, isFileOpened);
            }

            return count;
        }

        if (direction == Direction::Up)
        {
            auto first_displayable = std::find_if(
                ui.rbegin(),
                ui.rend() - content_ui_gap,
                [](auto &item)
                { return item.displayable; });

            auto last_displayable = std::reverse_iterator(std::find_if(
                                        ui.begin() + content_ui_gap,
                                        ui.end(),
                                        [](auto &item)
                                        { return item.displayable; })) -
                                    1;

            if (last_displayable >= ui.rend() - 1 || first_displayable == ui.rend())
                return 0;

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
                ui.begin() + content_ui_gap,
                ui.end(),
                [](auto &item)
                { return item.displayable; });

            if (displayable_count < lines_per_page - 1)
            {
                auto end{it + lines_per_page - displayable_count - 1};

                for (; it < ui.rend() - content_ui_gap && it < end; it++)
                {
                    it->displayable = true;
                    displayable_count++;
                }

                for (it = ui.rend() - content_ui_gap - displayable_count - 1;
                     displayable_count < lines_per_page - 1 &&
                     it > ui.rbegin();
                     it--)
                {
                    it->displayable = true;
                    displayable_count++;
                }
            }

            if (ui[content_ui_start + 1].displayable &&
                !isFileOpened)
            {
                ToggleUpButton(false);
            }

            if (rerender)
            {
                RenderContent(content_ui_start, isFileOpened);
            }

            return count;
        }

        return 0;
    }

    void FilesScene::OpenDirectory(const char *relative_path)
    {
        if (!strcmp(relative_path, ".."))
        {
            curr_directory.erase(curr_directory.rfind('/'));
        }
        else
        {
            curr_directory += relative_path;
        }

        if (curr_directory.ends_with('/'))
        {
            curr_directory.erase(curr_directory.rfind('/'));
        }

        ESP_LOGI(TAG, "Opening directory %s", curr_directory.c_str());
        ReadDirectory(content_ui_start + 1);
        ToggleUpButton(false);
        RenderContent(content_ui_start);
    }

    void FilesScene::ToggleUpButton(bool mode, bool rerender)
    {
        if (mode)
        {
            ui[content_ui_start].label = "^ Up";
            ui[content_ui_start].focusable = true;
        }
        else
        {
            ChangeItemFocus(&ui[content_ui_start], false, rerender);
            ui[content_ui_start].label.clear();
            ui[content_ui_start].focusable = false;
            ChangeItemFocus(&ui[content_ui_start + 1], true, rerender);
        }
    }

    void FilesScene::OpenFile(const char *relative_path)
    {
        SaveDirectory();
        ChangeHeader(relative_path, true);
        ChangeItemFocus(&ui[1], true, true);
        isFileOpened = true;
        ui.erase(ui.begin() + content_ui_start, ui.end());

        char buff[file_line_length + 1] = {0};
        uint32_t seek_pos{0};
        int read = 0;
        while ((read = sdcard.ReadFile(
                    (curr_directory + relative_path).c_str(),
                    buff,
                    file_line_length + 1,
                    seek_pos)) != 0)
        {
            ui.push_back(UiStringItem{
                buff,
                Color::White,
                display.fx16G,
                false,
            });
            seek_pos += read;
        }

        uint8_t fw, fh;
        Font::GetFontx(display.fx16G, 0, &fw, &fh);

        RenderContent(content_ui_start, true);

        cursor.height = fh;
        cursor.width = fw;

        size_t lines_count = ui.size() - content_ui_start;
        cursor.y = lines_count > file_lines_per_page ? file_lines_per_page - 1 : lines_count - 1;
        cursor.x = file_line_length - 1;
    }

    void FilesScene::ChangeHeader(const char *header, bool rerender)
    {
        Scene::ChangeHeader(header);

        if (rerender)
        {
            RenderHeader();
        }
    }

    void FilesScene::RenderHeader()
    {
        display.Clear(Color::Black, 0, display.GetHeight() - 60, display.GetWidth(), display.GetHeight());
        display.DrawStringItem(&ui[0], Position::Center, Position::End);
        display.DrawStringItem(&ui[1], Position::Start, Position::End);
    }

    void FilesScene::CloseFile()
    {
        ui.erase(ui.begin() + content_ui_start, ui.end());
        std::for_each(ui.begin(), ui.end(), [this](auto &item)
                      { if (item.focused) ChangeItemFocus(&item, false); });

        ui.insert(ui.end(), directory_backup.begin(), directory_backup.end());
        isFileOpened = false;
        isCursorControlling = false;
        ChangeHeader("Files");
        RenderAll();
        directory_backup.clear();
    }

    void FilesScene::SaveDirectory()
    {
        directory_backup.insert(
            directory_backup.end(),
            ui.cbegin() + content_ui_start,
            ui.cend());
    }

    void FilesScene::RenderCursor()
    {
        if (!isCursorControlling)
            return;

        uint16_t cursor_x{}, cursor_y{};
        GetCursorXY(&cursor_x, &cursor_y);

        ESP_LOGI(TAG, "Cursor X: %d", cursor_x);
        ESP_LOGI(TAG, "Cursor Y: %d", cursor_y);

        display.DrawCursor(
            cursor_x,
            cursor_y,
            cursor.width,
            cursor.height);
    }

    void FilesScene::SpawnCursor(uint8_t cursor_x, uint8_t cursor_y, bool clearing)
    {
        if (cursor_y > file_lines_per_page - 1)
        {
            cursor_y = file_lines_per_page - 1;
        }

        auto line{std::find_if(
            ui.begin() + content_ui_start + !isFileOpened,
            ui.end(),
            [](auto &item)
            { return item.displayable; })};

        if (line != ui.end() && line + cursor_y < ui.end())
        {
            line += cursor_y;
        }
        else
            return;

        if (cursor_x > line->label.size() - 1)
        {
            if (line->label[line->label.size() - 1] == '\n')
            {
                cursor_x = line->label.size() - 2;
            }
            else
            {
                cursor_x = (line == ui.end() - 1 &&
                            line->label.size() < file_line_length)
                               ? line->label.size()
                               : line->label.size() - 1;
            }
        }

        if (clearing)
        {
            ClearCursor(line - cursor_y + cursor.y);
        }
        cursor.x = cursor_x;
        cursor.y = cursor_y;

        ESP_LOGI(TAG, "Cursor X: %d, Cursor Y: %d", cursor.x, cursor.y);
        RenderCursor();
    }

    void FilesScene::MoveCursor(Direction direction, bool rerender, bool with_scrolling)
    {
        if (!isCursorControlling || !isFileOpened)
            return;

        auto line = std::find_if(ui.begin() + content_ui_start, ui.end(), [](auto &item)
                                 { return item.displayable; });

        if (line == ui.end())
            return;

        line += cursor.y;
        ESP_LOGI(TAG, "Current line: %s", line->label.c_str());

        uint16_t cursor_x{cursor.x}, cursor_y{cursor.y};

        uint8_t scrolled_count{};
        uint8_t scrolled_line_y{};

        switch (direction)
        {
        case Direction::Up:
            if (cursor_y > 0 || line > ui.begin() + content_ui_start)
            {
                if (cursor_y)
                {
                    cursor_y--;
                }
                else if (with_scrolling)
                {
                    scrolled_count = ScrollContent(Direction::Up, rerender, file_lines_scroll);
                    if (scrolled_count)
                        cursor_y = scrolled_count - 1;
                }
            }
            break;
        case Direction::Right:
            if ((cursor_x < (line->label.size() < file_line_length + 1
                                 ? line->label.size() - 1
                                 : file_line_length) &&
                 line->label[cursor.x + 1] != '\n') ||
                line == ui.cend() - 1)
            {
                cursor_x++;
            }
            else if ((line + 1) != ui.end())
            {
                if (cursor_y < file_lines_per_page - 1)
                {
                    cursor_y++;
                }
                else if (with_scrolling)
                {
                    scrolled_count = ScrollContent(Direction::Bottom, rerender, file_lines_scroll);
                    if (scrolled_count)
                        cursor_y = file_lines_per_page - scrolled_count;
                }
                cursor_x = 0;
            }
            break;
        case Direction::Bottom:
            if ((line + 1) != ui.end())
            {
                if (cursor_y < file_lines_per_page - 1)
                {
                    cursor_y++;
                }
                else if (with_scrolling)
                {
                    scrolled_count = ScrollContent(Direction::Bottom, rerender, file_lines_scroll);
                    if (scrolled_count)
                        cursor_y = file_lines_per_page - scrolled_count;
                }
            }
            break;
        case Direction::Left:
            if (cursor_x > 0)
            {
                cursor_x--;
            }
            else if (cursor_y > 0 || line > ui.begin() + content_ui_start)
            {
                if (cursor_y > 0)
                {
                    cursor_y--;
                }
                else if (with_scrolling)
                {
                    scrolled_count = ScrollContent(Direction::Up, rerender, file_lines_scroll);

                    if (scrolled_count)
                        cursor_y = scrolled_count - 1;
                }

                cursor_x = file_line_length;
            }
            break;
        }

        if (cursor_x != cursor.x || cursor_y != cursor.y)
        {
            SpawnCursor(cursor_x, cursor_y, scrolled_count == 0);
        }
    }

    void FilesScene::ClearCursor(std::vector<UiStringItem>::iterator line,
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

    void FilesScene::GetCursorXY(uint16_t *ret_x, uint16_t *ret_y, int16_t x, int16_t y)
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

    void FilesScene::InsertChars(std::string chars)
    {
        if (!chars.size())
            return;

        uint8_t insert_x{cursor.x}, insert_y{cursor.y};
        auto first_displaying = std::find_if(ui.begin() + content_ui_start, ui.end(), [](auto &item)
                                             { return item.displayable; });
        auto start_line{first_displaying + insert_y};
        uint8_t last_rendering_y{insert_y};

        for (auto line{start_line}; line < ui.end(); line++)
        {
            if (!chars.size())
                break;

            if (last_rendering_y < 10 && line > start_line)
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

            if (line->label.size() > file_line_length)
            {
                std::for_each(
                    line->label.begin() + file_line_length,
                    line->label.end(),
                    [&chars](auto &item)
                    { chars.push_back(item); });

                line->label.erase(
                    line->label.end() - (line->label.size() - file_line_length),
                    line->label.end());

                insert_x = 0;
            }

            if (line == ui.end() - 1 && chars.size())
            {
                ui.push_back(UiStringItem{std::string(""), line->color, line->font, false});
            }
        }

        RenderLines(insert_y, last_rendering_y, true);
        MoveCursor(Direction::Right);
    }

    void FilesScene::RenderLines(uint8_t first_line, uint8_t last_line, bool file)
    {
        uint8_t fw, fh;
        auto first_displaying = std::find_if(ui.begin() + content_ui_start, ui.end(), [](auto &item)
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
                                5,
                                file ? "more lines..." : "more items...");
    }
}