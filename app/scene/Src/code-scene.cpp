#include "code-scene.h"

static const char *TAG = "CodeScene";

extern QueueHandle_t xQueueRunnerProcessing;

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

        OpenStageModal(CodeSceneStage::LanguageChooseModalStage);
    }

    SceneId CodeScene::Escape()
    {
        if (IsCursorControlling())
        {
            SetCursorControlling(false);
            ChangeItemFocus(&*(ui->begin() + 1), true, true);
            return SceneId::CurrentScene;
        }
        else if (IsModalStage())
        {
            if (IsStage(CodeSceneStage::LanguageChooseModalStage))
            {
                return SceneId::StartScene;
            }

            LeaveModalControlling();
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
        else
        {
            auto focused{std::find_if(
                ui->begin(),
                ui->end(),
                [](auto &item)
                { return item.focused; })};

            if (focused == ui->end())
            {
                ESP_LOGE(TAG, "Focused item not found");
                return SceneId::CurrentScene;
            }

            if (IsModalStage())
            {
                if (IsStage(CodeSceneStage::LanguageChooseModalStage))
                {

                    for (auto &[key, language] : runner_languages)
                    {
                        if (focused->label == key)
                        {
                            runner_language = language;
                            ChangeHeader(key);
                            LeaveModalControlling();
                        }
                    }
                }
            }
            else if (focused->label.contains("< Esc"))
            {
                return Escape();
            }
            else if (focused->label.contains("Run"))
            {
                RunCode();
            }
        }

        return SceneId::CurrentScene;
    }

    void CodeScene::Arrow(Direction direction)
    {
        Scene::Arrow(direction);

        if (IsModalStage() && GetStageModal().Arrow != nullptr)
        {
            GetStageModal().Arrow(direction);
            return;
        }

        if (!IsCursorControlling() && direction == Direction::Bottom)
        {
            SetCursorControlling(true);
            auto focused{std::find_if(ui->begin(),
                                      ui->end(),
                                      [](auto &item)
                                      { return item.focused; })};
            ChangeItemFocus(&*focused, false, true);
            SpawnCursor();
            return;
        }

        if (IsCursorControlling())
        {
            MoveCursor(direction, true, GetLinesScroll());
            return;
        }

        Focus(direction);
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

    uint8_t CodeScene::ScrollContent(Direction direction, bool rerender, uint8_t count)
    {
        size_t lines_per_page{Scene::GetLinesPerPageCount()};

        if (count > lines_per_page - 1)
            count = lines_per_page - 1;

        if (direction == Direction::Bottom || direction == Direction::Up)
        {
            count = Scene::ScrollContent(direction, rerender, count);

            if (rerender && count)
            {
                RenderContent();
            }

            return count;
        }

        return 0;
    }

    void CodeScene::InitModals()
    {
        InitLanguageChooseModal();
        InitCodeRunModal();
    }

    void CodeScene::InitLanguageChooseModal()
    {
        Modal modal;

        modal.ui.push_back(UiStringItem{"Choose language:", Color::White, display.fx24G, false});
        display.SetPosition(&*(modal.ui.end() - 1), Position::Center, Position::End);

        uint8_t fw, fh;
        Font::GetFontx(display.fx24G, 0, &fw, &fh);

        for (auto &[key, _] : runner_languages)
        {
            modal.ui.push_back(UiStringItem{key, Color::White, display.fx24G});
        }

        size_t langs_start_index{GetContentUiStartIndex((uint8_t)CodeSceneStage::LanguageChooseModalStage)};

        display.SetPosition(&modal.ui[langs_start_index], Position::Center, Position::Center);
        ChangeItemFocus(&modal.ui[langs_start_index], true);

        uint8_t start_y{static_cast<uint8_t>(modal.ui[langs_start_index].y + fh * (runner_languages.size() / 2))};

        display.SetListPositions(modal.ui.begin() + langs_start_index, modal.ui.end(), 10, start_y, 9);
        std::for_each(
            modal.ui.begin() + langs_start_index,
            modal.ui.end(),
            [this](auto &item)
            { display.SetPosition(&item, Position::Center); });

        modal.PreEnter = [this]()
        {
            ChangeItemFocus(&(*ui)[1], true);
        };

        modal.PreLeave = [this]()
        {
            auto focused{std::find_if(
                ui->begin(),
                ui->end(),
                [](auto &item)
                { return item.focused; })};

            if (focused != ui->end())
            {
                ChangeItemFocus(&*focused, false);
            }
        };

        modal.Arrow = [this](Direction direction)
        {
            Focus(direction);
        };

        AddStageModal(CodeSceneStage::LanguageChooseModalStage, modal);
    }

    void CodeScene::InitCodeRunModal()
    {
        Modal modal{};

        modal.ui.push_back(UiStringItem{"Running...", Color::White, display.fx32L, false});
        display.SetPosition(&modal.ui[0], Position::Center, Position::End);

        modal.PreEnter = [this]()
        {
            uint8_t fw, fh;
            Font::GetFontx(display.fx16G, 0, &fw, &fh);
            Cursor cursor{
                .x = 0,
                .y = 0,
                .width = fw,
                .height = fh,
            };
            CursorInit(&cursor);

            Modal &modal{GetStageModal(CodeSceneStage::CodeRunModalStage)};
            modal.ui.push_back(UiStringItem{"", Color::White, display.fx16G, false});
            modal.ui[1].x = 10;
            modal.ui[1].y = display.GetHeight() - 60;

            SetCursorControlling(true);
        };

        modal.PreLeave = [this]()
        {
            Modal &modal{GetStageModal()};
            modal.ui.erase(modal.ui.begin() + 1, modal.ui.end());
        };

        AddStageModal(CodeSceneStage::CodeRunModalStage, modal);
    }

    void CodeScene::LeaveModalControlling(uint8_t stage, bool rerender)
    {
        Scene::LeaveModalControlling(stage, rerender);
    }

    size_t CodeScene::GetContentUiStartIndex(uint8_t stage)
    {
        if (stage == (uint8_t)CodeSceneStage::LanguageChooseModalStage ||
            stage == (uint8_t)CodeSceneStage::CodeRunModalStage)
        {
            return 1;
        }

        return content_ui_start;
    }

    void CodeScene::RunCode()
    {
        std::string code{};
        std::for_each(GetContentUiStart(), ui->end(), [&code](auto &item)
                      { code += item.label; });

        OpenStageModal(CodeSceneStage::CodeRunModalStage);

        CodeRunner::CodeProcess process{
            .language = runner_language,
            .data = code,
            .is_file = false,
        };

        xQueueSend(xQueueRunnerProcessing, &process, portMAX_DELAY);
        ESP_LOGI(TAG, "Send processing code item");
    }

    void CodeScene::SendCodeOutput(const char *output)
    {
        if (IsStage(CodeSceneStage::CodeRunModalStage))
        {
            ESP_LOGI(TAG, "Code output: %s", output);
            CursorInsertChars(output, 1);
        }
    }
}