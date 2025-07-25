#pragma once

#include "display.h"
#include <vector>
#include <cstring>
#include "esp_log.h"

using LCD::Color, Display::DisplayController;

namespace Scene
{
    enum class Direction
    {
        Up,
        Right,
        Left,
        Bottom,
    };

    class Scene
    {
    protected:
        DisplayController &display;
        std::vector<Display::UiStringItem> ui;

    public:
        Scene(DisplayController &display);
        virtual void Init();
        virtual void Arrow(Direction direction);
    };
}