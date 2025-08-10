#pragma once

#include "display.h"
#include <map>
#include <vector>
#include <cstring>
#include <algorithm>
#include <functional>
#include <type_traits>
#include <memory>
#include "esp_log.h"

using LCD::Color, Display::DisplayController, Display::UiStringItem, Display::Position;

namespace Scene
{
    enum class SceneId
    {
        CurrentScene = 0,
        StartScene = 1,
        FilesScene = 2,
        CodeScene = 3,
        SettingsScene = 4,
    };

    enum class Direction
    {
        Up,
        Right,
        Left,
        Bottom,
    };

    struct Cursor
    {
        uint8_t x{0}, y{0},
            width{10}, height{15};
    };

    struct Modal
    {
        std::vector<UiStringItem> ui{};
        std::string data{};
        std::function<void()> Ok{},
            Cancel{},
            PreEnter{},
            PreLeave{};
        std::function<void(Direction)> Arrow{};
    };

    class Scene
    {
        bool is_cursor_controlling{};
        Cursor cursor{};
        uint8_t stage{};

    protected:
        DisplayController &display;
        std::vector<UiStringItem> main_ui{};
        std::vector<UiStringItem> *ui{&main_ui};
        std::map<uint8_t, Modal> modals{};
        const size_t default_line_length{37};
        const size_t max_lines_per_page{9};

        void SetCursorControlling(bool cursor);
        virtual void EnterModalControlling();
        virtual void LeaveModalControlling(uint8_t stage, bool rerender = true);
        void RenderLines(uint8_t first_line, uint8_t last_line, bool clear_line_after = false);
        void GetCursorXY(uint16_t *ret_x, uint16_t *ret_y, int16_t x = -1, int16_t y = -1);
        void ClearCursor(std::vector<UiStringItem>::iterator line, int16_t x = -1, int16_t y = -1);
        void RenderCursor();
        void SpawnCursor(int16_t cursor_x = -1, int16_t cursor_y = -1, bool clearing = true);
        size_t MoveCursor(Direction direction, bool rerender = true, size_t scrolling = 0);
        virtual uint8_t ScrollContent(Direction direction, bool rerender = true, uint8_t count = 1);

        bool IsCursorControlling();
        void CursorInit(Cursor *cursor);
        void CursorDeleteChars(size_t count, size_t scrolling = 0, int16_t initial_x = -1, int16_t initial_y = -1);
        void CursorInsertChars(std::string chars, size_t scrolling = 0);
        void CursorAppendLine(const char *label = "", Color color = Color::White);

        virtual void InitModals();
        void AddModalLabel(std::string modal_label, Modal &modal);

        template <typename StageType, typename = std::enable_if_t<std::is_enum_v<StageType>>>
        void OpenStageModal(StageType stage)
        {
            SetStage(stage);
            EnterModalControlling();
        }

        template <typename StageType, typename = std::enable_if_t<std::is_enum_v<StageType>>>
        void SetStage(StageType stage)
        {
            this->stage = static_cast<uint8_t>(stage);
        }

        void SetStage(uint8_t stage);

        template <typename StageType, typename = std::enable_if_t<std::is_enum_v<StageType>>>
        StageType GetStage()
        {
            return static_cast<StageType>(stage);
        }

        uint8_t GetStage();

        template <typename StageType, typename = std::enable_if_t<std::is_enum_v<StageType>>>
        bool IsStage(StageType stage)
        {
            return GetStage<StageType>() == stage;
        }

        bool IsStage(uint8_t stage);

        template <typename StageType, typename = std::enable_if_t<std::is_enum_v<StageType>>>
        void AddStageModal(StageType stage, Modal modal)
        {
            modals[(uint8_t)stage] = modal;
        }

        template <typename StageType, typename = std::enable_if_t<std::is_enum_v<StageType>>>
        Modal &GetStageModal(StageType stage)
        {
            return modals[(uint8_t)stage];
        }

        Modal &GetStageModal(uint8_t stage);

        Modal &GetStageModal()
        {
            return modals[stage];
        }

        bool IsModalStage();
        bool IsModalStage(uint8_t stage);
        virtual bool IsHomeStage(uint8_t stage);

        virtual size_t GetLinesPerPageCount();
        virtual size_t GetLinesPerPageCount(uint8_t stage) = 0;
        virtual size_t GetLinesScroll();
        virtual size_t GetLineLength();
        virtual std::vector<UiStringItem>::iterator GetContentUiStart();
        size_t GetContentUiStartIndex();
        virtual size_t GetContentUiStartIndex(uint8_t stage) = 0;

        void ChangeItemFocus(UiStringItem *item, bool focus, bool rerender = false);
        virtual void ChangeHeader(const char *header, bool rerender = false);
        virtual uint8_t Focus(Direction direction);
        void RenderUiListEnding(const char *end_label = "more items");

        virtual void RenderAll();
        virtual void RenderContent() = 0;
        virtual void RenderHeader() = 0;
        virtual void RenderModal();

        virtual void ClearHeader(Color color = Color::Black);
        virtual void ClearContent(Color color = Color::Black);

    public:
        Scene(DisplayController &display);
        virtual void Init() = 0;
        virtual void Arrow(Direction direction);
        virtual SceneId Enter();
        virtual SceneId Escape() = 0;
        virtual void Delete();
        virtual void Value(char value);

        virtual ~Scene() = default;
    };
}