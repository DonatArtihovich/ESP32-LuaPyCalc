#include "files-scene.h"

namespace Scene
{
    FilesScene::FilesScene(DisplayController &display, SDCard &_sdcard)
        : Scene(display), sdcard{_sdcard} {}

    void FilesScene::Init()
    {
        RenderAll();
    }

    void FilesScene::RenderAll()
    {
        display.Clear(Color::Green);
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
}