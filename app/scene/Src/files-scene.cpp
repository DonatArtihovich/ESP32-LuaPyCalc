#include "files-scene.h"

namespace Scene
{
    FilesScene::FilesScene(DisplayController &display, SDCard &_sdcard)
        : Scene(display), sdcard{_sdcard} {}

    void FilesScene::Init()
    {
        ui.push_back(UiStringItem{"Files", Color::White, display.fx32L, false});
        ui.push_back(UiStringItem{"< Esc", Color::White, display.fx24G});

        ChangeItemFocus(&ui[1], true);

        std::vector<std::string> files{sdcard.ReadDirectory(SD("/"))};
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
        RenderAll();
    }

    void FilesScene::RenderAll()
    {
        display.Clear(Color::Black);
        display.DrawStringItem(&ui[0], Position::Center, Position::End);
        display.DrawStringItem(&ui[1], Position::Start, Position::End);
        display.DrawStringItems(ui.begin() + 2, ui.end(), 10, display.GetHeight() - 80, true);
    }

    void FilesScene::Arrow(Direction direction)
    {
        Scene::Arrow(direction);
        Scene::Focus(direction);
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
}