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
        const size_t directory_lines_per_page{9}; // including "more"
        const size_t file_lines_per_page{9};      // not including "more"
        const size_t directory_lines_scroll{directory_lines_per_page};
        const size_t file_lines_scroll{file_lines_per_page};
        const size_t file_line_length{37}; // without \0
        std::vector<UiStringItem> directory_backup{};

        bool isFileOpened{};
        bool isCursorControlling{};
        Cursor cursor{};

        void RenderContent(int ui_start, bool file = false);
        void RenderCursor();
        void RenderHeader();
        void RenderLines(uint8_t first_line = 0, uint8_t last_line = 10, bool file = false);

        void GetCursorXY(uint16_t *ret_x, uint16_t *ret_y, int16_t x = -1, int16_t y = -1);
        void ClearCursor(std::vector<UiStringItem>::iterator line, int16_t x = -1, int16_t y = -1);
        void MoveCursor(Direction direction, bool rerender = true, bool with_scrolling = true);
        uint8_t ScrollContent(Direction direction, bool rerender = true, uint8_t count = 1);
        void ToggleUpButton(bool mode, bool rerender = false);
        void SpawnCursor(uint8_t cursor_x, uint8_t cursor_y, bool clearing = true);

        void OpenDirectory(const char *relative_path);
        size_t ReadDirectory(int ui_start);
        void SaveDirectory();

        void OpenFile(const char *relative_path);
        void InsertChars(std::string chars);
        void CloseFile();

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