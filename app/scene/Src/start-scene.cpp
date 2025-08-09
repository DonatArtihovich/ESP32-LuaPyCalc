#include "start-scene.h"

#include <string>

static const char *TAG = "StartScene";

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

    void StartScene::RenderAll()
    {
        display.Clear(Color::Black);
        display.DrawStringItem(&(*ui)[0], Display::Position::Center, Display::Position::End);
        RenderContent();
    }

    void StartScene::RenderContent()
    {
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
            if (focused->label.contains("Files"))
            {
                display.Clear(Color::Black);
                return SceneId::FilesScene;
            }
        }

        return SceneId::CurrentScene;
    }

    SceneId StartScene::Escape()
    {
        return SceneId::CurrentScene;
    }
    // StartScene::
    size_t StartScene::GetContentUiStartIndex()
    {
        return 1;
    }

    uint8_t StartScene::GetLinesPerPageCount()
    {
        return lines_per_page;
    }
}