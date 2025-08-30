#pragma once
#include "scene.h"
#include "app-settings.h"

namespace Scene
{
    class SettingsScene : public Scene
    {
        void InitModals() override;
        void InitThemeSettingsModal();
        void InitUI();

    public:
        SettingsScene(DisplayController &display);

        void Init() override;
        SceneId Escape() override;
        SceneId Enter() override;

        ~SettingsScene() = default;
    };
}