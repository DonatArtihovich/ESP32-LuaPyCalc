#include "code-scene.h"

namespace Scene
{
    CodeScene::CodeScene(DisplayController &display)
        : Scene(display) {}

    void CodeScene::Init()
    {
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
        ClearHeader(Color::Black);
        std::for_each(ui->begin(), ui->begin() + 3,
                      [this](auto &item)
                      { display.DrawStringItem(&item); });
    }

    void CodeScene::RenderContent()
    {
        ClearContent(Color::Black);
        display.DrawStringItems(ui->begin() + content_ui_start,
                                ui->end(),
                                10,
                                display.GetHeight() - 60,
                                Scene::GetLinesPerPageCount());

        if (!(ui->end() - 1)->displayable)
        {
            RenderUiListEnding();
        }
    }
}