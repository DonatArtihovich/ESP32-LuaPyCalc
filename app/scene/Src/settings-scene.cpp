#include "settings-scene.h"

static const char *TAG = "SettingsScene";

namespace Scene
{
    SettingsScene::SettingsScene(DisplayController &display) : Scene{display} {}

    enum SettingsSceneStage
    {
        SettingsStage,
        ThemeSettingsModalStage,
        FilesSotringModalStage,
    };

    void SettingsScene::Init()
    {
        content_ui_start = 2;
        SetStage(SettingsSceneStage::SettingsStage);
        InitModals();
        auto &theme{Settings::Settings::GetTheme()};

        ui->push_back(
            Display::UiStringItem{"Settings", theme.Colors.MainTextColor, display.fx32L, false});
        display.SetPosition(&(*ui)[0], Position::Center, Position::End);

        ui->push_back(UiStringItem{"< Esc", theme.Colors.MainTextColor, display.fx24G});
        display.SetPosition(&*(ui->end() - 1), Position::Start, Position::End);

        ui->push_back(
            Display::UiStringItem{"Theme         ", theme.Colors.MainTextColor, display.fx24G});
        ui->push_back(
            Display::UiStringItem{"Files Sorting ", theme.Colors.MainTextColor, display.fx24G});

        ChangeItemFocus(&(*ui)[1], true);
        RenderAll();
    }

    SceneId SettingsScene::Escape()
    {
        if (!IsModalStage())
        {
            return SceneId::StartScene;
        }

        LeaveModalControlling((uint8_t)SettingsSceneStage::SettingsStage);
        return SceneId::CurrentScene;
    }

    SceneId SettingsScene::Enter()
    {
        auto focused{GetFocused()};

        if (focused == ui->end())
        {
            ESP_LOGE(TAG, "Focused item not found");
            return SceneId::CurrentScene;
        }

        if (focused->label.find("< Esc") != std::string::npos)
        {
            return Escape();
        }

        if (IsModalStage())
        {
            if (IsStage(SettingsSceneStage::ThemeSettingsModalStage))
            {
                ESP_LOGI(TAG, "New theme: %s", focused->label.c_str());
            }
        }

        if (focused->label.find("Theme") != std::string::npos)
        {
            OpenStageModal(SettingsSceneStage::ThemeSettingsModalStage);
        }

        return SceneId::CurrentScene;
    }

    void SettingsScene::InitModals()
    {
        InitThemeSettingsModal();
    }

    void SettingsScene::InitThemeSettingsModal()
    {
        Modal modal{};
        auto &theme{Settings::Settings::GetTheme()};

        modal.ui.push_back(Display::UiStringItem{"Theme", theme.Colors.MainTextColor, display.fx32L, false});
        display.SetPosition(&modal.ui[0], Position::Center, Position::End);
        modal.ui.push_back(Display::UiStringItem{"< Esc", theme.Colors.MainTextColor, display.fx24G});
        display.SetPosition(&modal.ui[1], Position::Start, Position::End);

        modal.ui.push_back(Display::UiStringItem{"Default", theme.Colors.MainTextColor, display.fx24G});
        ChangeItemFocus(&*(modal.ui.end() - 1), true);
        modal.ui.push_back(Display::UiStringItem{"Light", theme.Colors.MainTextColor, display.fx24G});
        modal.ui.push_back(Display::UiStringItem{"Green", theme.Colors.MainTextColor, display.fx24G});

        display.SetListPositions(modal.ui.begin() + 2, modal.ui.end(), 10, display.GetHeight() - 60, 3);

        modal.PreEnter = [this]()
        {
            Modal &modal{GetStageModal(SettingsSceneStage::ThemeSettingsModalStage)};
            ChangeItemFocus(&modal.ui[2], true);
        };

        modal.PreLeave = [this]()
        {
            Modal &modal{GetStageModal(SettingsSceneStage::ThemeSettingsModalStage)};
            auto focused{GetFocused(modal.ui.begin())};

            if (focused != modal.ui.end())
            {
                ChangeItemFocus(&*focused, false);
            }
        };

        AddStageModal((uint8_t)SettingsSceneStage::ThemeSettingsModalStage, modal);
    }
}