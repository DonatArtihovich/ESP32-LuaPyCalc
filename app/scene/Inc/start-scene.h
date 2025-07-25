#pragma once

#include "scene.h"

namespace Scene
{
    class StartScene : public Scene
    {
        std::vector<const char *> menu{"Files", "Code"};

    public:
        StartScene(DisplayController &display);
        void Init() override;
        void Focus(Direction direction) override;
    };
}