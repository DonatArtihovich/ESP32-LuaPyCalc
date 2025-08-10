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

    void StartScene::RenderHeader()
    {
        ClearHeader();
        display.DrawStringItem(&(*ui)[0], Display::Position::Center, Display::Position::End);
    }

    void StartScene::RenderContent()
    {
        display.Clear(Color::Black, 0, 0, 0, display.GetHeight() - 35);
        display.DrawStringItems(GetContentUiStart(), ui->end(), 0, display.GetHeight() - 80, 3);
    }

    SceneId StartScene::Enter()
    {
        auto focused = std::find_if(
            ui->cbegin(),
            ui->cend(),
            [](auto &item)
            { return item.focused; });

        if (focused != ui->cend())
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

    SceneId StartScene::Escape()
    {
        return SceneId::CurrentScene;
    }

    size_t StartScene::GetContentUiStartIndex(uint8_t stage)
    {
        return 1;
    }

    size_t StartScene::GetLinesPerPageCount(uint8_t stage)
    {
        return lines_per_page;
    }
}