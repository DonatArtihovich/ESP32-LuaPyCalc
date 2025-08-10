#include "scene.h"

namespace Scene
{
    class CodeScene : public Scene
    {
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