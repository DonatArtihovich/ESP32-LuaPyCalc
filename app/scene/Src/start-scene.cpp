#include "start-scene.h"

namespace Scene
{
    StartScene::StartScene(DisplayController &display) : Scene{display} {}

    void StartScene::Init()
    {
        ui->push_back(
            Display::UiStringItem{"Menu", Color::White, display.fx32L, false});
        ui->push_back(
            Display::UiStringItem{"> Files     ", Color::White, display.fx24G});
        ui->push_back(
            Display::UiStringItem{"> Code      ", Color::White, display.fx24G});
        ui->push_back(
            Display::UiStringItem{"> Settings  ", Color::White, display.fx24G});

        ChangeItemFocus(&(*ui)[1], true);
        RenderAll();
    }

    void StartScene::Arrow(Direction direction)
    {
        Scene::Arrow(direction);
        Scene::Focus(direction);
    }

    SceneId StartScene::Enter()
    {
        auto focused = GetFocused();

        if (focused != ui->end())
        {
            display.Clear(Color::Black);
            if (focused->label.contains("Files"))
            {
                return SceneId::FilesScene;
            }
            else if (focused->label.contains("Code"))
            {
                return SceneId::CodeScene;
            }
            else if (focused->label.contains("Settings"))
            {
                return SceneId::SettingsScene;
            }
        }

        return SceneId::CurrentScene;
    }
}