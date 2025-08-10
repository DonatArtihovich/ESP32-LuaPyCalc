#include "code-scene.h"

namespace Scene
{
    CodeScene::CodeScene(DisplayController &display)
        : Scene(display) {}

    void CodeScene::Init()
    {
        content_ui_start = 3;
        InitModals();

        ui->push_back(UiStringItem{"Code", Color::White, display.fx32L, false});
        display.SetPosition(&*(ui->end() - 1), Position::Center, Position::End);

        ui->push_back(UiStringItem{"< Esc", Color::White, display.fx24G});
        display.SetPosition(&*(ui->end() - 1), Position::Start, Position::End);
        ChangeItemFocus(&*(ui->end() - 1), true);

        ui->push_back(UiStringItem{"Run", Color::White, display.fx24G});
        display.SetPosition(&*(ui->end() - 1), Position::End, Position::End);

        ui->push_back(UiStringItem{"", Color::White, display.fx16G, false});

        uint8_t fw, fh;
        Font::GetFontx(display.fx16G, 0, &fw, &fh);
        Cursor cursor{
            .x = 0,
            .y = 0,
            .width = fw,
            .height = fh,
        };
        CursorInit(&cursor);

        RenderAll();
    }

    SceneId CodeScene::Escape()
    {
        if (IsCursorControlling())
        {
            SetCursorControlling(false);
            ChangeItemFocus(&*(ui->begin() + 1), true, true);
            return SceneId::CurrentScene;
        }

        return SceneId::StartScene;
    }

    SceneId CodeScene::Enter()
    {
        if (IsCursorControlling())
        {
            Scene::Enter();
        }

        return SceneId::CurrentScene;
    }

    void CodeScene::Arrow(Direction direction)
    {
        Scene::Arrow(direction);

        if (!IsCursorControlling() && direction == Direction::Bottom)
        {
            SetCursorControlling(true);
            auto focused{std::find_if(ui->begin(),
                                      ui->end(),
                                      [](auto &item)
                                      { return item.focused; })};
            ChangeItemFocus(&*focused, false, true);
            SpawnCursor();
        }
        else if (IsCursorControlling())
        {
            MoveCursor(direction, true, GetLinesScroll());
        }
        else
        {
            Focus(direction);
        }
    }

    void CodeScene::Delete()
    {
        Scene::Delete();
    }

    void CodeScene::Value(char value)
    {
        Scene::Value(value);
    }

    void CodeScene::RenderContent()
    {
        Scene::RenderContent();

        if (!(ui->end() - 1)->displayable)
        {
            RenderUiListEnding();
        }
    }
}