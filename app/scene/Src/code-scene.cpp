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

        CursorInit(display.fx16G);
        OpenStageModal(CodeSceneStage::LanguageChooseModalStage);
    }

    SceneId CodeScene::Escape()
    {
        if (IsCursorControlling() && !IsStage(CodeSceneStage::CodeRunModalStage))
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

            if (IsStage(CodeSceneStage::CodeRunModalStage))
            {
                if (!CodeRunController::IsRunning())
                {
                    LeaveModalControlling();
                }

                return SceneId::CurrentScene;
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
            else if (focused->label.find("< Esc") != std::string::npos)
            {
                return Escape();
            }
            else if (focused->label.find("Run") != std::string::npos)
            {
                RunCode();
            }
        }

        return SceneId::CurrentScene;
    }

    void CodeScene::Arrow(Direction direction)
    {
        if (!IsModalStage() && !IsCursorControlling() && direction == Direction::Bottom)
        {
            SetCursorControlling(true);
            auto focused{GetFocused()};
            if (focused != ui->end())
            {
                ChangeItemFocus(&*focused, false, true);
            }

            SpawnCursor();
            return;
        }

        Scene::Arrow(direction);
    }

    void CodeScene::Delete()
    {
        if (IsStage(CodeSceneStage::CodeRunModalStage))
            return;

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
        InitCodeRunModal((uint8_t)CodeSceneStage::CodeRunModalStage);
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
            .data = new char[code.size() + 1]{0},
            .is_file = false,
        };

        strncpy(process.data, code.c_str(), code.size() + 1);

        xQueueSend(xQueueRunnerProcessing, &process, portMAX_DELAY);
        ESP_LOGI(TAG, "Send processing code item");
    }

    bool CodeScene::IsCodeRunning()
    {
        return IsStage(CodeSceneStage::CodeRunModalStage);
    }
}