#pragma once
#include "scene.h"
#include "sd.h"

using SD::SDCard;

namespace Scene
{
    struct Cursor
    {
        uint8_t x{0}, y{0},
            width{10}, height{15};
    };

    class FilesScene : public Scene
    {
        SDCard &sdcard;

        std::string curr_directory{SD("")};
        const size_t content_ui_start{2};
        std::vector<UiStringItem> directory_backup{};

        bool isFileOpened{};
        bool isCursorControlling{};
        Cursor cursor{};

        size_t ReadDirectory(int ui_start);

        void RenderContent(int ui_start, bool file = false);
        void RenderCursor();
        void GetCursorXY(uint16_t *ret_x, uint16_t *ret_y, int16_t x = -1, int16_t y = -1);
        void RenderHeader();

        void ClearCursor(std::vector<UiStringItem>::iterator line, int16_t x = -1, int16_t y = -1);
        void MoveCursor(Direction direction);
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