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
        // ui.push_back(UiStringItem{"^", Color::White, display.fx24M});

        if (ReadDirectory(2))
        {
            ChangeItemFocus(&ui[2], true);
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
        display.Clear(Color::Black);
        display.DrawStringItem(&ui[0], Position::Center, Position::End);
        display.DrawStringItem(&ui[1], Position::Start, Position::End);
        RenderDirectory(2);
    }

    void FilesScene::RenderDirectory(int ui_start)
    {
        display.Clear(Color::Black, 10, 0, 0, display.GetHeight() - 40);
        display.DrawStringItems(ui.begin() + ui_start, ui.end(), 10, display.GetHeight() - 60, true);
    }

    size_t FilesScene::ReadDirectory(int ui_start)
    {
        std::vector<std::string> files{sdcard.ReadDirectory(curr_directory.c_str())};
        ui.erase(ui.begin() + ui_start, ui.end());
        if (!files.size())
        {
            ui.push_back(UiStringItem{"No files found", Color::White, display.fx24G, false});
        }
        else
        {
            for (auto &file : files)
            {
                ui.push_back(UiStringItem{file.c_str(), Color::White, display.fx16M});
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
        return SceneId::CurrentScene;
    }

    SceneId FilesScene::Escape()
    {
        display.Clear(Color::Black);
        return SceneId::StartScene;
    }

    uint8_t FilesScene::Focus(Direction direction)
    {
        if (!Scene::Focus(direction))
        {
            ESP_LOGI(TAG, "Basic focus not found");
            // auto last_focused = std::find_if(
            //     ui.begin(),
            //     ui.end(),
            //     [](auto &item)
            //     { return item.focused; });

            if (direction == Direction::Bottom)
            {
                auto first_displayable = std::find_if(ui.begin() + 2, ui.end(), [](auto &item)
                                                      { return item.displayable; });

                if (first_displayable != ui.end())
                {
                    auto next_displayable = std::find_if(first_displayable + 1, ui.end(), [](auto &item)
                                                         { return !item.displayable; });

                    if (next_displayable != ui.end())
                    {
                        first_displayable->displayable = false;
                        next_displayable->displayable = true;

                        // ChangeItemFocus(&(*last_focused), false);
                        RenderDirectory(2);
                        // ChangeItemFocus(&(*last_focused), true);
                        Scene::Focus(direction);
                    }
                }
            }
            else if (direction == Direction::Up)
            {
                auto prev = std::find_if(ui.rbegin(), ui.rend() - 2, [](auto &item)
                                         { return item.displayable; });

                if (prev != ui.rend() - 2)
                {
                    auto next = std::find_if(prev + 1, ui.rend() - 2, [](auto &item)
                                             { return !item.displayable; });

                    if (next != ui.rend() - 2)
                    {
                        prev->displayable = false;
                        next->displayable = true;

                        RenderAll();
                        Scene::Focus(direction);
                    }
                }
            }
        }

        return true;
    }
}