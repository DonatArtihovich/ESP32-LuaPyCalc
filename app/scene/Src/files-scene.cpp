#include "files-scene.h"

static const char *TAG = "FileScene";

namespace Scene
{
    FilesScene::FilesScene(DisplayController &display, SDCard &_sdcard)
        : Scene(display), sdcard{_sdcard} {}

    void FilesScene::Init()
    {
        ui.push_back(UiStringItem{"Files", Color::White, display.fx32L, false});
        ui.push_back(UiStringItem{"< Esc", Color::White, display.fx24G});
        ui.push_back(UiStringItem{"", Color::White, display.fx24M, false});

        if (ReadDirectory(directory_ui_start + 1))
        {
            ChangeItemFocus(&ui[directory_ui_start + 1], true);
        }
        else
        {
            ChangeItemFocus(&ui[1], true);
        }

        RenderAll();
    }

    void FilesScene::RenderAll()
    {
        display.Clear(Color::Black, 0, 0, 10, display.GetHeight() - 60);
        display.DrawStringItem(&ui[0], Position::Center, Position::End);
        display.DrawStringItem(&ui[1], Position::Start, Position::End);
        RenderDirectory(directory_ui_start);
    }

    void FilesScene::RenderDirectory(int ui_start)
    {
        display.Clear(Color::Black, 10, 0, 0, display.GetHeight() - 35);
        display.DrawStringItems(ui.begin() + ui_start, ui.end(), 10, display.GetHeight() - 60, true);
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

        if (index > directory_ui_start)
        {
            if (focused->label == "..")
            {
                OpenDirectory(focused->label.c_str());
                return SceneId::CurrentScene;
            }

            char filename[30] = {0};
            snprintf(filename, 29, "/%s", focused->label.c_str());

            int isDir{sdcard.IsDirectory((curr_directory + filename).c_str())};

            switch (isDir)
            {
            case 1:
                ESP_LOGI(TAG, "%s is directory", filename);
                OpenDirectory(filename);
                break;
            case 0:
                ESP_LOGI(TAG, "%s isn't directory", filename);
                break;
            default:
                ESP_LOGE(TAG, "Checking %s error", filename);
            }
        }
        else if (focused->label.contains("^ Up"))
        {
            ScrollDirectoryFiles(Direction::Up);
        }
        else if (focused->label.contains("< Esc"))
        {
            return Escape();
        }

        return SceneId::CurrentScene;
    }

    SceneId FilesScene::Escape()
    {
        return SceneId::StartScene;
    }

    uint8_t FilesScene::Focus(Direction direction)
    {
        if (!Scene::Focus(direction))
        {
            ESP_LOGI(TAG, "Basic focus not found");
            if (direction == Direction::Bottom)
            {
                ScrollDirectoryFiles(direction);
                Scene::Focus(direction);
            }
        }

        return true;
    }

    void FilesScene::ScrollDirectoryFiles(Direction direction)
    {
        // auto last_focused = std::find_if(
        //     ui.begin(),
        //     ui.end(),
        //     [](auto &item)
        //     { return item.focused; });

        if (direction == Direction::Bottom)
        {
            auto first_displayable = std::find_if(
                ui.begin() + directory_ui_start + 1,
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

                    if (!ui[directory_ui_start].focusable)
                    {
                        ToggleUpButton(true);
                    }

                    // ChangeItemFocus(&(*last_focused), false);
                    RenderDirectory(directory_ui_start);
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

                    if (next_displayable == ui.rend() - directory_ui_start - 2)
                    {
                        ToggleUpButton(false);
                    }

                    RenderDirectory(directory_ui_start);
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
        ReadDirectory(directory_ui_start + 1);
        ToggleUpButton(false);
        RenderDirectory(directory_ui_start);
    }

    void FilesScene::ToggleUpButton(bool mode)
    {
        if (mode)
        {
            ui[directory_ui_start].label = "^ Up";
            ui[directory_ui_start].focusable = true;
        }
        else
        {
            ChangeItemFocus(&ui[directory_ui_start], false);
            ui[directory_ui_start].label.clear();
            ui[directory_ui_start].focusable = false;
            ChangeItemFocus(&ui[directory_ui_start + 1], true);
        }
    }
}