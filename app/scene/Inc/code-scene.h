#pragma once

#include "scene.h"
#include "runner.h"

#include <cmath>

using CodeRunner::CodeLanguage, CodeRunner::CodeRunController;

namespace Scene
{
    enum class CodeSceneStage
    {
        CodeEnterStage,
        LanguageChooseModalStage,
        CodeRunModalStage
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
        void InitCodeRunModal();
        size_t GetContentUiStartIndex(uint8_t stage);

        void LeaveModalControlling(
            uint8_t stage = (uint8_t)CodeSceneStage::CodeEnterStage,
            bool rerender = true) override;

        void RunCode();

    public:
        CodeScene(DisplayController &display);

        void Init() override;
        void Arrow(Direction direction) override;
        SceneId Enter() override;
        SceneId Escape() override;
        void Delete() override;
        void Value(char value) override;

        void SendCodeOutput(const char *output) override;
        void SendCodeError(const char *traceback) override;
        void SendCodeSuccess() override;

        ~CodeScene() = default;
    };
}