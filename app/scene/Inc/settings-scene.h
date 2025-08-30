#pragma once
#include "scene.h"
#include "app-settings.h"

namespace Scene
{
    class SettingsScene : public Scene
    {
        void InitModals() override;
        void InitThemeSettingsModal();

    public:
        SettingsScene(DisplayController &display);

        void Init() override;
        SceneId Escape() override;
        SceneId Enter() override;

        ~SettingsScene() = default;
    };
}