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
        const size_t content_ui_start{2};
        std::vector<UiStringItem> directory_backup{};
        bool isFileOpened{false};

        size_t ReadDirectory(int ui_start);

        void RenderContent(int ui_start, bool file = false);
        void RenderHeader();
        void ScrollContent(Direction direction);
        void ToggleUpButton(bool mode);

        void OpenDirectory(const char *relative_path);
        void OpenFile(const char *relative_path);
        void CloseFile();
        void SaveDirectory();

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