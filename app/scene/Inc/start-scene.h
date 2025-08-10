#pragma once

#include "scene.h"

namespace Scene
{
    class StartScene : public Scene
    {
    public:
        StartScene(DisplayController &display);

        void Init() override;
        void Arrow(Direction direction) override;
        SceneId Enter() override;

        ~StartScene() = default;
    };
}