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
    }

    void FilesScene::RenderContent(int ui_start, bool file)
    {
        display.Clear(Color::Black, 10, 0, 0, display.GetHeight() - 35);
        display.DrawStringItems(ui.begin() + ui_start,
                                ui.end(),
                                10,
                                display.GetHeight() - 60,
                                true,
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
        Focus(direction);
    }

    SceneId FilesScene::Enter()
    {
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
            ScrollContent(Direction::Up);
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
            CloseFile();
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
                ScrollContent(direction);
                Scene::Focus(direction);
            }
        }

        return true;
    }

    void FilesScene::ScrollContent(Direction direction)
    {
        // auto last_focused = std::find_if(
        //     ui.begin(),
        //     ui.end(),
        //     [](auto &item)
        //     { return item.focused; });

        if (direction == Direction::Bottom)
        {
            auto first_displayable = std::find_if(
                ui.begin() + content_ui_start + !isFileOpened,
                ui.end(),
                [](auto &item)
                { return item.displayable; });

            if (first_displayable != ui.end())
            {
                auto next_displayable = std::find_if(
                    first_displayable + 1, ui.end(),
                    [](auto &item)
                    { return !item.displayable; });

                if (next_displayable != ui.end())
                {
                    first_displayable->displayable = false;
                    next_displayable->displayable = true;

                    if (!ui[content_ui_start].focusable)
                    {
                        ToggleUpButton(true);
                    }

                    // ChangeItemFocus(&(*last_focused), false);
                    RenderContent(content_ui_start, isFileOpened);
                    // ChangeItemFocus(&(*last_focused), true);
                }
            }
        }
        else if (direction == Direction::Up)
        {
            auto first_displayable = std::find_if(
                ui.rbegin(),
                ui.rend() - 2,
                [](auto &item)
                { return item.displayable; });

            if (first_displayable != ui.rend() - 2)
            {
                auto next_displayable = std::find_if(
                    first_displayable + 1,
                    ui.rend() - 2,
                    [](auto &item)
                    { return !item.displayable; });

                if (next_displayable != ui.rend() - 2)
                {
                    first_displayable->displayable = false;
                    next_displayable->displayable = true;

                    if (next_displayable == ui.rend() - content_ui_start - 2)
                    {
                        ToggleUpButton(false);
                    }

                    RenderContent(content_ui_start, isFileOpened);
                }
            }
        }
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

    void FilesScene::ToggleUpButton(bool mode)
    {
        if (mode)
        {
            ui[content_ui_start].label = "^ Up";
            ui[content_ui_start].focusable = true;
        }
        else
        {
            ChangeItemFocus(&ui[content_ui_start], false);
            ui[content_ui_start].label.clear();
            ui[content_ui_start].focusable = false;
            ChangeItemFocus(&ui[content_ui_start + 1], true);
        }
    }

    void FilesScene::OpenFile(const char *relative_path)
    {
        SaveDirectory();
        ChangeHeader(relative_path, true);
        ChangeItemFocus(&ui[1], true, true);
        isFileOpened = true;
        ui.erase(ui.begin() + content_ui_start, ui.end());

        char buff[38] = {0};
        uint32_t seek_pos{0};
        int read = 0;
        while ((read = sdcard.ReadFile((curr_directory + relative_path).c_str(), buff, 38, seek_pos)) != 0)
        {
            ui.push_back(UiStringItem{
                buff,
                Color::White,
                display.fx16G,
            });
            seek_pos += read;
        }

        ESP_LOGI(TAG, "Seek pos = %lu", seek_pos);

        cursor.y = ui.size() - 1 - content_ui_start;
        RenderContent(content_ui_start, true);
        cursor.x = std::find_if(ui.crbegin(), ui.crend() - content_ui_start, [](auto &item)
                                { return item.displayable; })
                       ->label.size() -
                   1;

        RenderCursor();
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
        uint16_t cursor_x{static_cast<uint16_t>(cursor.x * cursor.width + 10)},
            cursor_y{static_cast<uint16_t>(display.GetHeight() - 60 - cursor.y * cursor.height + 2)};

        ESP_LOGI(TAG, "Cursor X: %d", cursor_x);
        ESP_LOGI(TAG, "Cursor Y: %d", cursor_y);

        display.DrawCursor(
            cursor_x,
            cursor_y,
            cursor.width,
            cursor.height);
    }
}