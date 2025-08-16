#include "scene.h"
#include "runner.h"

using CodeRunner::CodeLanguage;

namespace Scene
{
    enum CodeSceneStage
    {
        CodeEnterStage,
        LanguageChooseModalStage,
    };

    class CodeScene : public Scene
    {
        CodeLanguage runner_language;

        std::map<std::string, CodeRunner::CodeLanguage> runner_languages{
            {"Lua", CodeLanguage::Lua},
            {"Python", CodeLanguage::Python},
            {"Ruby", CodeLanguage::Ruby},
        };

        void RenderContent() override;
        uint8_t ScrollContent(Direction direction, bool rerender = true, uint8_t count = 1) override;
        void InitModals() override;
        void InitLanguageChooseModal();
        size_t GetContentUiStartIndex(uint8_t stage);

        void LeaveModalControlling(
            uint8_t stage = (uint8_t)CodeSceneStage::CodeEnterStage,
            bool rerender = true) override;

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