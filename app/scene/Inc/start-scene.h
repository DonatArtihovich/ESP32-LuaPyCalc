#pragma once

#include "scene.h"

namespace Scene
{
    class StartScene : public Scene
    {
        const size_t lines_per_page{max_lines_per_page};

        size_t GetContentUiStartIndex(uint8_t stage) override;
        size_t GetLinesPerPageCount(uint8_t stage) override;

        void RenderAll() override;
        void RenderContent() override;

    public:
        StartScene(DisplayController &display);
        void Init() override;
        void Arrow(Direction direction) override;
        SceneId Enter() override;
        SceneId Escape() override;
        ~StartScene() = default;
    };
}