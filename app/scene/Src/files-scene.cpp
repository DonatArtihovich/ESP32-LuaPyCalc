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

        if (ReadDirectory())
        {
            ChangeItemFocus(&(*GetContentUiStart()), true);
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
        if (IsCursorControlling())
        {
            RenderCursor();
        }
    }

    void FilesScene::RenderContent(uint8_t ui_start_index)
    {
        display.Clear(Color::Black, 10, 0, 0, display.GetHeight() - 35);
        display.DrawStringItems(ui.begin() + ui_start_index,
                                ui.end(),
                                10,
                                display.GetHeight() - 60,
                                GetLinesPerPageCount());
        if (!(ui.end() - 1)->displayable)
        {
            RenderUiListEnding(isFileOpened ? "more lines" : "more items");
        }
    }

    size_t FilesScene::ReadDirectory()
    {
        std::vector<std::string> files{sdcard.ReadDirectory(curr_directory.c_str())};
        ui.erase(GetContentUiStart(), ui.end());

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
        if (isFileOpened && !IsCursorControlling() && direction == Direction::Bottom)
        {
            SetCursorControlling(true);
            auto focused = std::find_if(ui.begin(), ui.end(), [this](auto &item)
                                        { return item.focused; });
            if (focused != ui.end())
            {
                ChangeItemFocus(&(*focused), false, true);
            }

            SpawnCursor(-1, -1, false);
            return;
        }

        if (IsCursorControlling())
        {
            MoveCursor(direction, true, file_lines_scroll);
            return;
        }

        Focus(direction);
    }

    SceneId FilesScene::Enter()
    {
        if (IsCursorControlling())
        {
            CursorInsertChars("\n", file_lines_scroll);
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
            if (IsCursorControlling())
            {
                ChangeItemFocus(&ui[1], true, true);
                SetCursorControlling(false);
            }
            else
            {
                CloseFile();
            }
            return SceneId::CurrentScene;
        }

        return SceneId::StartScene;
    }

    void FilesScene::Delete()
    {
        if (IsCursorControlling())
        {
            CursorDeleteChars(3, file_lines_scroll);
        }
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
        }

        return true;
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
        ReadDirectory();
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
        ui.erase(GetContentUiStart(), ui.end());

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

        RenderContent(GetContentUiStartIndex());

        size_t lines_count = ui.size() - GetContentUiStartIndex();

        Cursor cursor{
            .x = static_cast<uint8_t>(file_line_length - 1),
            .y = static_cast<uint8_t>(lines_count > file_lines_per_page
                                          ? file_lines_per_page - 1
                                          : lines_count - 1),
            .width = fw,
            .height = fh,
        };
        CursorInit(&cursor);
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
        ui.erase(GetContentUiStart(), ui.end());
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

    size_t FilesScene::GetContentUiStartIndex()
    {
        return content_ui_start + !isFileOpened;
    }

    uint8_t FilesScene::GetLinesPerPageCount()
    {
        return isFileOpened ? file_lines_per_page : directory_lines_per_page;
    }

    size_t FilesScene::GetLineLength()
    {
        return isFileOpened ? file_line_length : default_line_length;
    }

    uint8_t FilesScene::ScrollContent(Direction direction, bool rerender, uint8_t count)
    {
        size_t lines_per_page{GetLinesPerPageCount()};

        if (count > lines_per_page - 1 - !isFileOpened)
            count = lines_per_page - 1 - !isFileOpened;

        if (direction == Direction::Bottom)
        {
            count = Scene::ScrollContent(direction, rerender, count);
            if (!ui[content_ui_start].focusable &&
                !isFileOpened)
            {
                ToggleUpButton(true);
            }

            if (rerender)
            {
                RenderContent(content_ui_start);
            }

            return count;
        }

        if (direction == Direction::Up)
        {
            count = Scene::ScrollContent(direction, rerender, count);
            if (ui[content_ui_start + 1].displayable &&
                !isFileOpened)
            {
                ToggleUpButton(false);
            }

            if (rerender)
            {
                RenderContent(content_ui_start);
            }

            return count;
        }

        return 0;
    }
}