#include "code-scene.h"

namespace Scene
{
    CodeScene::CodeScene(DisplayController &display)
        : Scene(display) {}

    void CodeScene::Init()
    {
        RenderAll();
    }

    SceneId CodeScene::Escape()
    {
        return SceneId::StartScene;
    }

    SceneId CodeScene::Enter()
    {
        return SceneId::CurrentScene;
    }

    void CodeScene::Arrow(Direction direction)
    {
        Scene::Arrow(direction);
    }

    void CodeScene::Delete()
    {
        return Scene::Delete();
    }

    void CodeScene::Value(char value)
    {
        Scene::Value(value);
    }

    size_t CodeScene::GetLinesPerPageCount(uint8_t stg)
    {
        return lines_per_page;
    }

    size_t CodeScene::GetContentUiStartIndex(uint8_t stage)
    {
        return content_ui_start;
    }

    void CodeScene::RenderHeader()
    {
        ClearHeader(Color::Red);
    }

    void CodeScene::RenderContent()
    {
        ClearContent(Color::Red);
    }
}