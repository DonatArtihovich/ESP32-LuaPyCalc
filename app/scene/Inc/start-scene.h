#pragma once

#include "scene.h"

namespace Scene
{
    class StartScene : public Scene
    {
    public:
        StartScene(DisplayController &display);

        void Init() override;
        SceneId Enter() override;

        void SendCodeOutput(const char *output) override;
        void SendCodeError(const char *traceback) override;
        void SendCodeSuccess() override;

        ~StartScene() = default;
    };
}