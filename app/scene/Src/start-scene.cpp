#include "start-scene.h"

namespace Scene
{
    StartScene::StartScene(DisplayController &display) : Scene{display} {}

    void StartScene::Init()
    {
        auto &theme{Settings::Settings::GetTheme()};

        ui->push_back(
            Display::UiStringItem{"Menu", theme.Colors.MainTextColor, display.fx32L, false});
        ui->push_back(
            Display::UiStringItem{"> Files     ", theme.Colors.MainTextColor, display.fx24G});
        ui->push_back(
            Display::UiStringItem{"> Code      ", theme.Colors.MainTextColor, display.fx24G});
        ui->push_back(
            Display::UiStringItem{"> Settings  ", theme.Colors.MainTextColor, display.fx24G});

        ChangeItemFocus(&(*ui)[1], true);
        RenderAll();
    }

    SceneId StartScene::Enter()
    {
        auto focused = GetFocused();

        if (focused != ui->end())
        {
            display.Clear(Settings::Settings::GetTheme().Colors.MainBackgroundColor);
            if (focused->label.find("Files") != std::string::npos)
            {
                return SceneId::FilesScene;
            }
            else if (focused->label.find("Code") != std::string::npos)
            {
                return SceneId::CodeScene;
            }
            else if (focused->label.find("Settings") != std::string::npos)
            {
                return SceneId::SettingsScene;
            }
        }

        return SceneId::CurrentScene;
    }
}