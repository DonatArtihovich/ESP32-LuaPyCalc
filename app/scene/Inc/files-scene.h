#pragma once
#include "scene.h"
#include "sd.h"

using SD::SDCard;

namespace Scene
{
    class FilesScene : public Scene
    {
        SDCard &sdcard;

        std::string curr_directory{SD("/")};
        const size_t directory_ui_start{2};

        void RenderDirectory(int ui_start);
        size_t ReadDirectory(int ui_start);

        void ScrollDirectoryFiles(Direction direction);

    public:
        FilesScene(DisplayController &display, SDCard &_sdcard);
        void Init() override;
        void Arrow(Direction direction) override;
        void RenderAll() override;
        SceneId Enter() override;
        SceneId Escape() override;
        uint8_t Focus(Direction direction) override;
    };
}