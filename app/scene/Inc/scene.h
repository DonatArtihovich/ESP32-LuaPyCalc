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
    enum class SceneId
    {
        CurrentScene = 0,
        StartScene = 1,
        FilesScene = 2,
        CodeScene = 3,
        SettingsScene = 4,
    };

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
        virtual void Arrow(Direction direction);
        virtual SceneId Enter() = 0;
        virtual SceneId Escape() = 0;
        virtual void RenderAll() = 0;

        void ChangeItemFocus(UiStringItem *item, bool focus, bool rerender = false);
        virtual uint8_t Focus(Direction direction);

        virtual ~Scene() = default;
    };
}