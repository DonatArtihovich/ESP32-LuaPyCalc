#pragma once
#include "scene.h"
#include "sd.h"

using SD::SDCard;

namespace Scene
{
    class FilesScene : public Scene
    {
        SDCard &sdcard;

        std::string curr_directory{SD("")};
        const size_t directory_ui_start{2};
        std::vector<UiStringItem> directory_backup{};
        bool isFileOpened{false};

        void RenderDirectory(int ui_start);
        size_t ReadDirectory(int ui_start);

        void RenderFile(int ui_start);
        void RenderHeader();
        void ScrollDirectoryFiles(Direction direction);
        void ToggleUpButton(bool mode);

        void OpenDirectory(const char *relative_path);
        void OpenFile(const char *relative_path);

    public:
        FilesScene(DisplayController &display, SDCard &_sdcard);
        void Init() override;
        void Arrow(Direction direction) override;
        void RenderAll() override;
        SceneId Enter() override;
        SceneId Escape() override;
        uint8_t Focus(Direction direction) override;
        void ChangeHeader(const char *header, bool rerender = false) override;
    };
}