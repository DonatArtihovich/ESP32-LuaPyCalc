#pragma once

#include "display.h"
#include <vector>
#include <cstring>
#include <algorithm>
#include <functional>
#include "esp_log.h"

using LCD::Color, Display::DisplayController, Display::UiStringItem, Display::Position;

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
        std::vector<UiStringItem> ui;

    public:
        Scene(DisplayController &display);
        virtual void Init() = 0;
        virtual void Arrow(Direction direction) = 0;
        virtual void RenderAll() = 0;

        void ChangeItemFocus(UiStringItem *item, bool focus);
        void Focus(Direction direction);
    };
}