#include "scene.h"

namespace Scene
{
    class CodeScene : public Scene
    {
        const size_t lines_per_page{max_lines_per_page};
        const size_t content_ui_start{3};

        size_t GetLinesPerPageCount(uint8_t stage) override;
        size_t GetContentUiStartIndex(uint8_t stage) override;

        void RenderHeader() override;
        void RenderContent() override;

    public:
        CodeScene(DisplayController &display);

        void Init() override;
        void Arrow(Direction direction) override;
        SceneId Enter() override;
        SceneId Escape() override;
        void Delete() override;
        void Value(char value) override;

        ~CodeScene() = default;
    };
}