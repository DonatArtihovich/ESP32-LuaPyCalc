#include "settings-scene.h"

static const char *TAG = "SettingsScene";

namespace Scene
{
    static const char *fsort_labels[]{"Alphabet (asc.)",
                                      "Alphabet (desc.)",
                                      "Files first (alph. asc.)",
                                      "Files first (alph. desc.)",
                                      "Directories first (alph. asc.)",
                                      "Directories first (alph. desc.)"};

    static const char *theme_labels[]{"Default", "Light", "Green"};

    enum SettingsSceneStage
    {
        SettingsStage,
        ThemeSettingsModalStage,
        FilesSortingModalStage,
    };

    SettingsScene::SettingsScene(DisplayController &display) : Scene{display} {}

    void SettingsScene::Init()
    {
        content_ui_start = 2;
        SetStage(SettingsSceneStage::SettingsStage);
        InitModals();
        InitUI();
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
                ESP_LOGD(TAG, "New theme: %s", focused->label.c_str());

                for (size_t i{}; i < sizeof(theme_labels) / sizeof(*theme_labels); i++)
                {
                    if (focused->label.find(theme_labels[i]) != std::string::npos)
                    {
                        SetTheme((Settings::Themes)i);
                        break;
                    }
                }

                return SceneId::CurrentScene;
            }

            if (IsStage(SettingsSceneStage::FilesSortingModalStage))
            {
                ESP_LOGD(TAG, "New files sorting: %s", focused->label.c_str());

                for (size_t i{}; i < sizeof(fsort_labels) / sizeof(*fsort_labels); i++)
                {
                    if (focused->label.find(fsort_labels[i]) != std::string::npos)
                    {
                        Settings::Settings::SetFilesSortingMode((Settings::FilesSortingModes)i);
                        break;
                    }
                }

                return SceneId::CurrentScene;
            }
        }

        if (focused->label.find("Theme") != std::string::npos)
        {
            OpenStageModal(SettingsSceneStage::ThemeSettingsModalStage);
        }
        else if (focused->label.find("Files Sorting") != std::string::npos)
        {
            OpenStageModal(SettingsSceneStage::FilesSortingModalStage);
        }

        return SceneId::CurrentScene;
    }

    void SettingsScene::InitModals()
    {
        InitThemeSettingsModal();
        InitFilesSortingSettingsModal();
    }

    void SettingsScene::InitThemeSettingsModal()
    {
        Modal modal{};
        auto &theme{Settings::Settings::GetTheme()};

        modal.ui.push_back(Display::UiStringItem{"Theme", theme.Colors.MainTextColor, display.fx32L, false});
        display.SetPosition(&modal.ui[0], Position::Center, Position::End);
        modal.ui.push_back(Display::UiStringItem{"< Esc", theme.Colors.MainTextColor, display.fx24G});
        display.SetPosition(&modal.ui[1], Position::Start, Position::End);

        for (const char *theme_label : theme_labels)
        {
            modal.ui.push_back(Display::UiStringItem{theme_label, theme.Colors.MainTextColor, display.fx24G});
        }
        display.SetListPositions(modal.ui.begin() + 2, modal.ui.end(), 10, display.GetHeight() - 60, 3);

        modal.PreEnter = [this]()
        {
            Modal &modal{GetStageModal(SettingsSceneStage::ThemeSettingsModalStage)};
            ChangeItemFocus(&modal.ui[2 + (int)Settings::Settings::GetTheme().key], true);
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

    void SettingsScene::InitFilesSortingSettingsModal()
    {
        Modal modal{};
        auto &theme{Settings::Settings::GetTheme()};

        modal.ui.push_back(Display::UiStringItem{"Sorting Mode", theme.Colors.MainTextColor, display.fx24G, false});
        display.SetPosition(&modal.ui[0], Position::Center, Position::End);
        modal.ui.push_back(Display::UiStringItem{"< Esc", theme.Colors.MainTextColor, display.fx24G});
        display.SetPosition(&modal.ui[1], Position::Start, Position::End);

        for (const char *fsort_label : fsort_labels)
        {
            modal.ui.push_back(Display::UiStringItem{fsort_label, theme.Colors.MainTextColor, display.fx16G});
        }
        display.SetListPositions(modal.ui.begin() + 2, modal.ui.end(), 10, display.GetHeight() - 60, 6);

        modal.PreEnter = [this]()
        {
            Modal &modal{GetStageModal(SettingsSceneStage::FilesSortingModalStage)};
            ChangeItemFocus(&modal.ui[2 + (int)Settings::Settings::GetFilesSortingMode()], true);
        };

        modal.PreLeave = [this]()
        {
            Modal &modal{GetStageModal(SettingsSceneStage::FilesSortingModalStage)};
            auto focused{GetFocused(modal.ui.begin())};

            if (focused != modal.ui.end())
            {
                ChangeItemFocus(&*focused, false);
            }
        };

        AddStageModal((uint8_t)SettingsSceneStage::FilesSortingModalStage, modal);
    }

    void SettingsScene::InitUI()
    {
        auto &theme{Settings::Settings::GetTheme()};

        ui->push_back(
            Display::UiStringItem{"Settings", theme.Colors.MainTextColor, display.fx32L, false});
        display.SetPosition(&(*ui)[0], Position::Center, Position::End);

        ui->push_back(UiStringItem{"< Esc", theme.Colors.MainTextColor, display.fx24G});
        display.SetPosition(&*(ui->end() - 1), Position::Start, Position::End);

        ui->push_back(
            Display::UiStringItem{"Theme         ", theme.Colors.MainTextColor, display.fx24M});
        ui->push_back(
            Display::UiStringItem{"Files Sorting ", theme.Colors.MainTextColor, display.fx24M});

        ChangeItemFocus(&(*ui)[2], true);
    }

    void SettingsScene::SetTheme(Settings::Themes theme)
    {
        Settings::Settings::SetTheme(theme);
        LeaveModalControlling(SettingsSceneStage::SettingsStage, false);
        ui->erase(ui->begin(), ui->end());
        InitModals();
        InitUI();
        OpenStageModal(SettingsSceneStage::ThemeSettingsModalStage);
    }
}