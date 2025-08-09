#pragma once
#include "scene.h"
#include "sd.h"

using SD::SDCard;

namespace Scene
{
    enum class FilesSceneStage
    {
        DirectoryStage,
        FileOpenStage,
        DeleteModalStage,
        CreateChooseModalStage,
        CreateModalStage,
    };

    class FilesScene : public Scene
    {
        SDCard &sdcard;

        std::string curr_directory{SD("")};
        const size_t content_ui_start{4};
        const size_t directory_lines_per_page{9}; // including "more"
        const size_t file_lines_per_page{9};      // not including "more"
        const size_t directory_lines_scroll{directory_lines_per_page};
        const size_t file_lines_scroll{file_lines_per_page};
        const size_t file_line_length{37}; // without \0
        std::vector<UiStringItem> directory_backup{};

        void OpenDirectory(const char *relative_path);
        size_t ReadDirectory();
        void ReadFile(std::string path);
        void SaveDirectory();

        void OpenFile(const char *relative_path);
        void SaveFile();
        void CloseFile();

        void DeleteFile(std::string filename);

        size_t GetContentUiStartIndex() override;
        uint8_t GetLinesPerPageCount() override;
        size_t GetLineLength() override;

        uint8_t Focus(Direction direction) override;
        uint8_t ScrollContent(Direction direction, bool rerender = true, uint8_t count = 1) override;

        void LeaveModalControlling() override;
        void InitModals() override;

        void InitDeleteModal();
        void InitCreateChooseModal();

        void SetupDeleteModal();
        void SetupCreateChooseModal();

        void RenderAll() override;
        void RenderContent() override;
        void RenderHeader();

        void ToggleUpButton(bool mode, bool rerender = false);
        void ToggleSaveButton(bool mode, bool rerender = false);
        void ToggleCreateButton(bool mode, bool rerender = false);

        void ChangeHeader(const char *header, bool rerender = false) override;

    public:
        FilesScene(DisplayController &display, SDCard &_sdcard);
        void Init() override;
        void Arrow(Direction direction) override;
        SceneId Enter() override;
        SceneId Escape() override;
        void Delete() override;
        ~FilesScene() = default;
    };
}