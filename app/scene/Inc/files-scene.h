#pragma once
#include "scene.h"
#include "sd.h"

using SD::SDCard;

namespace Scene
{
    class FilesScene : public Scene
    {
        SDCard &sdcard;

    public:
        FilesScene(DisplayController &display, SDCard &_sdcard);
        void Init() override;
        void Arrow(Direction direction);
        void RenderAll() override;
        SceneId Enter() override;
    };
}