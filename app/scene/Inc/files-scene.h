#pragma once
#include "scene.h"
#include "sd.h"

#include "runner.h"

using SD::SDCard, CodeRunner::CodeLanguage, CodeRunner::CodeRunController;

namespace Scene
{
    enum class FilesSceneStage
    {
        DirectoryStage,
        FileOpenStage,
        DeleteModalStage,
        CreateChooseModalStage,
        CreateModalStage,
        CodeRunModalStage,
    };

    class FilesScene : public Scene
    {
        SDCard &sdcard;

        std::string curr_directory{SD("")};
        const size_t directory_lines_per_page{max_lines_per_page}; // including "more"
        const size_t file_lines_per_page{max_lines_per_page};      // not including "more"
        const size_t directory_lines_scroll{directory_lines_per_page};
        const size_t file_lines_scroll{file_lines_per_page};
        const size_t file_line_length{default_line_length}; // without \0
        const size_t max_filename_size{8};
        const size_t max_filename_ext_size{3};
        std::vector<UiStringItem> directory_backup{};
        CodeLanguage runner_language{CodeLanguage::Text};

        void OpenDirectory(const char *relative_path);
        size_t ReadDirectory();
        void ReadFile(std::string path);
        void SaveDirectory();

        void OpenFile(const char *relative_path);
        void DetectLanguage(std::string filename);
        void SaveFile();
        void CloseFile();

        void RunFile();

        void DeleteFile(std::string filename);
        bool CreateFile(std::string filename, bool is_directory);

        bool IsHomeStage(uint8_t stage) override;

        size_t GetLinesScroll() override;
        size_t GetLinesPerPageCount(uint8_t stage);
        size_t GetContentUiStartIndex(uint8_t stage) override;
        size_t GetLineLength() override;

        uint8_t Focus(Direction direction, std::function<bool(UiStringItem *, UiStringItem *)> = nullptr) override;
        uint8_t ScrollContent(Direction direction, bool rerender = true, uint8_t count = 1) override;

        void LeaveModalControlling(
            uint8_t stage = (uint8_t)FilesSceneStage::DirectoryStage,
            bool rerender = true) override;

        void InitModals() override;

        void InitDeleteModal();
        void InitCreateChooseModal();
        void InitCreateModal();

        void SetupDeleteModal();
        void SetupCreateChooseModal();
        void SetupCreateModal();

        void RenderContent() override;

        void ToggleUpButton(bool mode, bool rerender = false);
        void ToggleSaveButton(bool mode, bool rerender = false);
        void ToggleCreateButton(bool mode, bool rerender = false);
        bool IsCodeRunning() override;

        void SortFiles();

    public:
        FilesScene(DisplayController &display, SDCard &_sdcard);
        void Init() override;
        void Arrow(Direction direction) override;
        void Value(char value) override;
        SceneId Enter() override;
        SceneId Escape() override;
        void Delete() override;

        ~FilesScene() = default;
    };
}