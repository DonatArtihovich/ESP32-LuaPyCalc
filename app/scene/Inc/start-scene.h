#pragma once

#include "scene.h"

namespace Scene
{
    class StartScene : public Scene
    {
        uint8_t lines_per_page{9};

        size_t GetContentUiStartIndex() override;
        void RenderAll() override;
        void RenderContent() override;
        uint8_t GetLinesPerPageCount() override;

    public:
        StartScene(DisplayController &display);
        void Init() override;
        void Arrow(Direction direction) override;
        SceneId Enter() override;
        SceneId Escape() override;
        ~StartScene() = default;
    };
}