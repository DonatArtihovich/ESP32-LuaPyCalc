#include "files-scene.h"

static const char *TAG = "FileScene";

extern QueueHandle_t xQueueRunnerProcessing;

namespace Scene
{
    FilesScene::FilesScene(DisplayController &display, SDCard &_sdcard)
        : Scene(display), sdcard{_sdcard} {}

    void FilesScene::Init()
    {
        content_ui_start = 4;
        InitModals();

        ui->push_back(UiStringItem{"  Files   ", Color::White, display.fx32L, false});
        display.SetPosition(&*(ui->end() - 1), Position::Center, Position::End);

        ui->push_back(UiStringItem{"< Esc", Color::White, display.fx24G});
        display.SetPosition(&*(ui->end() - 1), Position::Start, Position::End);

        ui->push_back(UiStringItem{"Create", Color::White, display.fx24G}); // [Create] button
        display.SetPosition(&*(ui->end() - 1), Position::End, Position::End);

        ui->push_back(UiStringItem{"Save", Color::White, display.fx24G, false}); // [Save] button
        display.SetPosition(&*(ui->end() - 1), Position::End, Position::End);
        (ui->end() - 1)->label.clear();

        ui->push_back(UiStringItem{"", Color::White, display.fx24M, false}); // [^ Up] button

        if (ReadDirectory())
        {
            ChangeItemFocus(&(*GetContentUiStart()), true);
        }
        else
        {
            ChangeItemFocus(&(*ui)[1], true);
        }

        SetStage(FilesSceneStage::DirectoryStage);
        RenderAll();
    }

    void FilesScene::RenderContent()
    {
        Scene::RenderContent();

        if (!(ui->end() - 1)->displayable)
        {
            RenderUiListEnding(IsStage(FilesSceneStage::FileOpenStage)
                                   ? "more lines"
                                   : "more items");
        }
    }

    void FilesScene::Value(char value)
    {
        FilesSceneStage stage{GetStage<FilesSceneStage>()};
        bool enter{true}, is_ctrl_pressed{KeyboardController::IsKeyPressed(Keyboard::Key::Ctrl)};

        if (stage == FilesSceneStage::FileOpenStage)
        {
            if ((value == 'r' || value == 'R') && is_ctrl_pressed)
            {
                RunFile();
                enter = false;
            }
            else if ((value == 's' || value == 'S') && is_ctrl_pressed)
            {
                SaveFile();
                enter = false;
            }
            else if (!(*ui)[3].label.size() && IsCursorControlling())
            {
                ToggleSaveButton(true, true);
            }
        }

        if (enter)
        {
            Scene::Value(value);
        }
    }

    size_t FilesScene::ReadDirectory()
    {
        std::vector<std::string> files{sdcard.ReadDirectory(curr_directory.c_str())};
        ui->erase(GetContentUiStart(), ui->end());

        if (!curr_directory.ends_with("sdcard"))
        {
            ui->push_back(UiStringItem{"..", Color::White, display.fx16G});
        }
        if (!files.size())
        {
            ui->push_back(UiStringItem{"No files found", Color::White, display.fx16G, false});
        }
        else
        {
            for (auto &file : files)
            {
                ui->push_back(UiStringItem{file.c_str(), Color::White, display.fx16G});
            }
        }

        return files.size();
    }

    void FilesScene::ReadFile(std::string path)
    {
        char buff[file_line_length + 1] = {0};
        uint32_t seek_pos{0};
        int read = 0;
        while ((read = sdcard.ReadFile(
                    path.c_str(),
                    buff,
                    file_line_length + 1,
                    seek_pos)) != 0)
        {
            ui->push_back(UiStringItem{
                buff,
                Color::White,
                display.fx16G,
                false,
            });
            seek_pos += read;
        }

        if (GetContentUiStart() == ui->end() ||
            (ui->end() - 1)->label.size() == GetLineLength())
        {
            ui->push_back(UiStringItem{
                "",
                Color::White,
                display.fx16G,
                false,
            });
        }
    }

    void FilesScene::Arrow(Direction direction)
    {
        if (IsStage(FilesSceneStage::FileOpenStage) &&
            !IsCursorControlling() &&
            direction == Direction::Bottom)
        {
            SetCursorControlling(true);
            auto focused = GetFocused();
            if (focused != ui->end())
            {
                ChangeItemFocus(&(*focused), false, true);
            }

            SpawnCursor(-1, -1, false);
            return;
        }

        Scene::Arrow(direction);
    }

    SceneId FilesScene::Enter()
    {
        if (IsCursorControlling())
        {
            return Scene::Enter();
        }

        auto focused = GetFocused();

        if (focused == ui->end())
        {
            return SceneId::CurrentScene;
        }

        if (IsModalStage())
        {
            uint8_t stage = (uint8_t)GetStage<FilesSceneStage>();
            if (modals.count(stage) == 0)
                return SceneId::CurrentScene;

            auto &modal = modals[stage];

            if (focused->label.find("Cancel") != std::string::npos)
            {
                modal.Cancel();
            }
            else if (focused->label.find("Ok") != std::string::npos)
            {
                modal.Ok();
            }
            else if (IsStage(FilesSceneStage::CreateChooseModalStage))
            {
                if (focused->label.find("File") != std::string::npos ||
                    focused->label.find("Directory") != std::string::npos)
                {
                    GetStageModal(FilesSceneStage::CreateModalStage).data =
                        ((focused->label.find("File") != std::string::npos)
                             ? "file"
                             : "directory");

                    OpenStageModal(FilesSceneStage::CreateModalStage);
                }
            }

            return SceneId::CurrentScene;
        }

        if (focused >= GetContentUiStart())
        {
            if (focused->label == "..")
            {
                OpenDirectory(focused->label.c_str());
                return SceneId::CurrentScene;
            }

            std::string filename{std::string(1, '/') + focused->label};
            if (sdcard.IsDirectory((curr_directory + filename).c_str()))
            {
                ESP_LOGI(TAG, "%s is directory", filename.c_str());
                OpenDirectory(filename.c_str());
            }
            else
            {
                ESP_LOGI(TAG, "%s isn't directory", filename.c_str());
                OpenFile(filename.c_str());
            }
        }
        else if (focused->label.find("^ Up") != std::string::npos)
        {
            ScrollContent(Direction::Up, true, directory_lines_scroll);
        }
        else if (focused->label.find("Save") != std::string::npos)
        {
            SaveFile();
        }
        else if (focused->label.find("Create") != std::string::npos)
        {
            OpenStageModal(FilesSceneStage::CreateChooseModalStage);
        }
        else if (focused->label.find("< Esc") != std::string::npos)
        {
            return Escape();
        }

        return SceneId::CurrentScene;
    }

    SceneId FilesScene::Escape()
    {
        if (IsStage(FilesSceneStage::FileOpenStage))
        {
            if (IsCursorControlling())
            {
                ChangeItemFocus(&(*ui)[1], true, true);
                SetCursorControlling(false);
            }
            else
            {
                CloseFile();
            }
        }
        else if (IsModalStage())
        {

            FilesSceneStage stage{GetStage<FilesSceneStage>()};

            if (stage == FilesSceneStage::CreateModalStage)
            {
                if (IsCursorControlling())
                {
                    SetCursorControlling(false);
                    ChangeItemFocus(&*(ui->end() - 3), true, true);
                }
                else
                {
                    LeaveModalControlling((uint8_t)FilesSceneStage::CreateChooseModalStage);
                }
            }
            else if (stage == FilesSceneStage::CodeRunModalStage)
            {
                LeaveModalControlling((uint8_t)FilesSceneStage::FileOpenStage);
            }
            else
            {
                LeaveModalControlling();
            }
        }
        else
        {
            return SceneId::StartScene;
        }

        return SceneId::CurrentScene;
    }

    void FilesScene::Delete()
    {
        if (IsStage(FilesSceneStage::FileOpenStage) &&
            IsCursorControlling() &&
            !(*ui)[3].label.size())
        {
            ToggleSaveButton(true, true);
        }

        Scene::Delete();

        if (!IsModalStage() && IsStage(FilesSceneStage::DirectoryStage))
        {
            auto focused = GetFocused();

            if (focused != ui->end() &&
                focused >= GetContentUiStart() &&
                focused->label != "..")
            {
                GetStageModal(FilesSceneStage::DeleteModalStage).data = focused->label;
                OpenStageModal(FilesSceneStage::DeleteModalStage);
            }
        }
    }

    uint8_t FilesScene::Focus(Direction direction, std::function<bool(UiStringItem *, UiStringItem *)> add_cond)
    {
        if (!Scene::Focus(direction, add_cond))
        {
            ESP_LOGI(TAG, "Basic focus not found");
            if (direction == Direction::Bottom)
            {
                ScrollContent(direction, true, directory_lines_scroll);
            }
        }

        return true;
    }

    void FilesScene::OpenDirectory(const char *relative_path)
    {
        if (!strcmp(relative_path, ".."))
        {
            curr_directory.erase(curr_directory.rfind('/'));
        }
        else
        {
            curr_directory += relative_path;
        }

        if (curr_directory.ends_with('/'))
        {
            curr_directory.erase(curr_directory.rfind('/'));
        }

        ESP_LOGI(TAG, "Opening directory %s", curr_directory.c_str());
        ReadDirectory();
        ToggleUpButton(false);
        RenderContent();
    }

    void FilesScene::ToggleUpButton(bool mode, bool rerender)
    {
        size_t up_index{4};
        if (mode)
        {
            (*ui)[up_index].label = "^ Up";
            (*ui)[up_index].focusable = true;
        }
        else
        {
            ChangeItemFocus(&(*ui)[up_index], false, rerender);
            (*ui)[up_index].label.clear();
            (*ui)[up_index].focusable = false;
            ChangeItemFocus(&(*GetContentUiStart()), true, rerender);
        }

        if (rerender)
        {
            display.DrawStringItem(&(*ui)[up_index]);
        }
    }

    void FilesScene::ToggleSaveButton(bool mode, bool rerender)
    {
        size_t save_index{3};
        if (mode)
        {
            (*ui)[save_index].label = "Save";
            (*ui)[save_index].focusable = true;
        }
        else if ((*ui)[save_index].focused)
        {
            ChangeItemFocus(&(*ui)[save_index], false);
            (*ui)[save_index].focusable = false;
            ChangeItemFocus(&(*ui)[1], true, rerender);
        }

        if (rerender)
        {
            if (!mode)
            {
                (*ui)[save_index].backgroundColor = Color::Black;
                (*ui)[save_index].label = std::string((*ui)[save_index].label.size(), ' ');
                display.DrawStringItem(&(*ui)[save_index]);
                (*ui)[save_index].label.clear();
                (*ui)[save_index].backgroundColor = Color::None;
            }
            else
            {
                display.DrawStringItem(&(*ui)[save_index]);
            }
        }
    }

    void FilesScene::ToggleCreateButton(bool mode, bool rerender)
    {
        size_t create_index{2};
        auto item{ui->begin() + create_index};
        item->displayable = mode;

        if (rerender)
        {
            display.DrawStringItem(&(*item));
        }
    }

    void FilesScene::OpenFile(const char *relative_path)
    {
        SaveDirectory();
        SetStage(FilesSceneStage::FileOpenStage);
        ChangeHeader(relative_path);
        ChangeItemFocus(&(*ui)[1], true);
        ToggleCreateButton(false);
        (*ui)[3].displayable = true;
        ui->erase(GetContentUiStart(), ui->end());

        ReadFile(curr_directory + relative_path);
        DetectLanguage(relative_path);
        RenderAll();

        size_t lines_count = ui->size() - Scene::GetContentUiStartIndex();

        uint8_t cursor_x{static_cast<uint8_t>(file_line_length - 1)},
            cursor_y{static_cast<uint8_t>(lines_count > file_lines_per_page
                                              ? file_lines_per_page - 1
                                              : lines_count - 1)};
        CursorInit(display.fx16G, cursor_x, cursor_y);
    }

    void FilesScene::CloseFile()
    {
        ui->erase(GetContentUiStart(), ui->end());
        std::for_each(ui->begin(), ui->end(), [this](auto &item)
                      { if (item.focused) ChangeItemFocus(&item, false); });

        ui->insert(ui->end(), directory_backup.begin(), directory_backup.end());
        SetStage(FilesSceneStage::DirectoryStage);
        ChangeHeader("Files");
        (*ui)[3].displayable = false;
        ToggleCreateButton(true);
        RenderAll();
        directory_backup.clear();
    }

    void FilesScene::SaveDirectory()
    {
        directory_backup.insert(
            directory_backup.end(),
            ui->cbegin() + content_ui_start,
            ui->cend());
    }

    size_t FilesScene::GetContentUiStartIndex(uint8_t stg)
    {
        FilesSceneStage stage{stg};
        if (stage == FilesSceneStage::CreateModalStage)
        {
            return ui->end() - 1 - ui->begin();
        }

        if (stage == FilesSceneStage::CodeRunModalStage)
        {
            return 1;
        }

        return content_ui_start + (stage != FilesSceneStage::FileOpenStage);
    }

    size_t FilesScene::GetLinesPerPageCount(uint8_t stage)
    {
        switch ((FilesSceneStage)stage)
        {
        case FilesSceneStage::FileOpenStage:
            return file_lines_per_page;
        case FilesSceneStage::DirectoryStage:
            return directory_lines_per_page;
        case FilesSceneStage::CreateModalStage:
            return 1;
        default:
            return max_lines_per_page;
        }
    }

    size_t FilesScene::GetLineLength()
    {
        return IsStage(FilesSceneStage::FileOpenStage)
                   ? file_line_length
                   : default_line_length;
    }

    uint8_t FilesScene::ScrollContent(Direction direction, bool rerender, uint8_t count)
    {
        size_t lines_per_page{Scene::GetLinesPerPageCount()};
        bool is_directory_open_stage{IsStage(FilesSceneStage::DirectoryStage)};

        if (count > lines_per_page - 1 - is_directory_open_stage)
            count = lines_per_page - 1 - is_directory_open_stage;

        if (direction == Direction::Bottom || direction == Direction::Up)
        {
            count = Scene::ScrollContent(direction, rerender, count);

            if (is_directory_open_stage)
            {
                if (direction == Direction::Bottom &&
                    !(*ui)[content_ui_start].focusable &&
                    count > 0)
                {
                    ToggleUpButton(true);
                }
                else if (direction == Direction::Up &&
                         (*ui)[content_ui_start + 1].displayable)
                {
                    ToggleUpButton(false);
                }
            }

            if (rerender && count)
            {
                RenderContent();
            }

            return count;
        }

        return 0;
    }

    void FilesScene::SaveFile()
    {
        if (!IsStage(FilesSceneStage::FileOpenStage))
        {
            return;
        }

        std::string file_path{curr_directory + (*ui)[0].label};
        uint64_t pos{0};
        auto line{GetContentUiStart()};

        while (line != ui->end() && sdcard.WriteFile(file_path.c_str(), line->label.c_str(), pos) == ESP_OK)
        {
            pos += line->label.size();
            ESP_LOGI(TAG, "Writing \"%s\", pos %lld", line->label.c_str(), pos);
            line++;
        }

        ToggleSaveButton(false, true);
        ESP_LOGI(TAG, "File %s saved.", file_path.c_str());
    }

    void FilesScene::InitModals()
    {
        InitDeleteModal();
        InitCreateChooseModal();
        InitCreateModal();

        for (auto &[key, modal] : modals)
        {
            modal.Cancel = [this]()
            {
                Escape();
            };
        }

        InitCodeRunModal((uint8_t)FilesSceneStage::CodeRunModalStage);
    }

    void FilesScene::InitDeleteModal()
    {
        Modal modal{};

        modal.ui.push_back(UiStringItem{"Ok", Color::White, display.fx24G});
        display.SetPosition(&*(modal.ui.end() - 1), Position::Start, Position::Start);
        ChangeItemFocus(&*(modal.ui.end() - 1), true);

        modal.ui.push_back(UiStringItem{"Cancel", Color::White, display.fx24G});
        display.SetPosition(&*(modal.ui.end() - 1), Position::End, Position::Start);

        modal.PreEnter = [this]()
        {
            SetupDeleteModal();
        };

        modal.PreLeave = [this]()
        {
            ui->erase(ui->begin(), ui->end() - 2);
            auto focused{GetFocused()};

            if (focused != ui->begin())
                ChangeItemFocus(&(*focused), false);
        };

        modal.Ok = [this]()
        {
            DeleteFile(GetStageModal().data);
            Escape();
        };

        AddStageModal(FilesSceneStage::DeleteModalStage, modal);
    }

    void FilesScene::InitCreateChooseModal()
    {
        Modal modal{};
        modal.ui.push_back(UiStringItem{"Create", Color::White, display.fx24G, false});
        display.SetPosition(&*(modal.ui.end() - 1), Position::Center, Position::End);

        modal.ui.push_back(UiStringItem{"File", Color::White, display.fx24G});
        display.SetPosition(&*(modal.ui.end() - 1), Position::Center, Position::Center);

        uint8_t fw, fh;
        Font::GetFontx(display.fx24G, 0, &fw, &fh);

        modal.ui.push_back(UiStringItem{"Directory", Color::White, display.fx24G});
        (modal.ui.end() - 1)->y = (modal.ui.end() - 2)->y - fh;
        display.SetPosition(&*(modal.ui.end() - 1), Position::Center);

        modal.ui.push_back(UiStringItem{"Cancel", Color::White, display.fx24G});
        display.SetPosition(&*(modal.ui.end() - 1), Position::End, Position::Start);

        modal.PreEnter = [this]()
        {
            SetupCreateChooseModal();
        };

        modal.PreLeave = [this]()
        {
            auto focused{GetFocused()};

            if (focused != ui->end())
                ChangeItemFocus(&(*focused), false);
        };

        AddStageModal(FilesSceneStage::CreateChooseModalStage, modal);
    }

    void FilesScene::InitCreateModal()
    {
        Modal modal{};

        modal.ui.push_back(UiStringItem{"Ok", Color::White, display.fx24G});
        display.SetPosition(&*(modal.ui.end() - 1), Position::Start, Position::Start);

        modal.ui.push_back(UiStringItem{"Cancel", Color::White, display.fx24G});
        display.SetPosition(&*(modal.ui.end() - 1), Position::End, Position::Start);

        modal.ui.push_back(UiStringItem{"", Color::White, display.fx24G, false});
        (modal.ui.end() - 1)->x = 70;
        display.SetPosition(&*(modal.ui.end() - 1), Position::NotSpecified, Position::Center);

        modal.PreEnter = [this]()
        {
            SetupCreateModal();
        };

        modal.PreLeave = [this]()
        {
            ui->erase(ui->begin(), ui->end() - 3);
            auto focused{GetFocused()};

            if (focused != ui->end())
                ChangeItemFocus(&(*focused), false);

            Modal &modal{GetStageModal()};
            modal.data.clear();
            (modal.ui.end() - 1)->label.clear();
        };

        modal.Arrow = [this](Direction direction)
        {
            if (!IsCursorControlling() && direction == Direction::Up)
            {
                SetCursorControlling(true);
                auto focused{GetFocused()};

                if (focused != ui->end())
                {
                    ChangeItemFocus(&*focused, false, true);
                }
            }
            else if (IsCursorControlling())
            {

                if (direction == Direction::Left ||
                    direction == Direction::Right)
                {
                    MoveCursor(direction);
                }
                else if (direction == Direction::Bottom)
                {
                    SetCursorControlling(false);
                    ChangeItemFocus(&*(ui->end() - 3), true, true);
                }
            }
            else
            {
                Scene::Focus(direction);
            }
        };

        modal.Value = [this](char value, bool is_ctrl_pressed)
        {
            if (!IsCursorControlling())
                return;

            std::string allowed_digits{"_-"};
            auto &label{(GetStageModal().ui.end() - 1)->label};
            bool enter{};

            if (isalnum(value) || allowed_digits.find(value) != std::string::npos)
            {
                if (label.size() < max_filename_size ||
                    (label.size() < (max_filename_size + max_filename_ext_size + 1) &&
                     label.find('.') != std::string::npos))
                {
                    enter = true;
                }
            }
            else if (value == '.')
            {
                if (label.size() < (max_filename_size + max_filename_ext_size + 1))
                {
                    enter = true;
                }
            }

            if (enter)
            {
                CursorInsertChars(std::string(1, value), GetLinesScroll());
            }
        };

        modal.Ok = [this]()
        {
            Modal &modal = GetStageModal();
            if (CreateFile((modal.ui.end() - 1)->label, modal.data == "directory"))
            {
                LeaveModalControlling((uint8_t)FilesSceneStage::CreateChooseModalStage, false);
                LeaveModalControlling();
            }
        };

        AddStageModal(FilesSceneStage::CreateModalStage, modal);
    }

    bool FilesScene::IsHomeStage(uint8_t stage)
    {
        return FilesSceneStage::DirectoryStage == (FilesSceneStage)stage ||
               FilesSceneStage::FileOpenStage == (FilesSceneStage)stage;
    }

    void FilesScene::LeaveModalControlling(uint8_t stage, bool rerender)
    {
        Scene::LeaveModalControlling(stage, rerender);
    }

    void FilesScene::SetupDeleteModal()
    {
        auto &modal = GetStageModal(FilesSceneStage::DeleteModalStage);
        std::string filename{modal.data};

        if (filename.ends_with('/'))
        {
            filename.erase(filename.end() - 1);
        }

        std::string focused_file{curr_directory + "/" + filename};

        std::string modal_label{"Do you want to delete "};
        modal_label += sdcard.IsDirectory(focused_file.c_str())
                           ? (sdcard.ReadDirectory(focused_file.c_str()).size()
                                  ? ("directory " + filename + " and it's contents?")
                                  : ("directory " + filename + "?"))
                           : ("file " + filename + "?");

        ChangeItemFocus(&(*modal.ui.begin()), true);
        AddModalLabel(modal_label, modal);
    }

    void FilesScene::SetupCreateChooseModal()
    {
        if (modals.count((uint8_t)FilesSceneStage::CreateChooseModalStage) == 0)
            return;

        auto &modal = GetStageModal(FilesSceneStage::CreateChooseModalStage);
        ChangeItemFocus(&(*(modal.ui.begin() + 1)), true);
    }

    void FilesScene::SetupCreateModal()
    {
        auto &modal = GetStageModal(FilesSceneStage::CreateModalStage);
        if (modal.data == "file")
        {
            AddModalLabel("Enter file name:", modal);
        }
        else if (modal.data == "directory")
        {
            AddModalLabel("Enter directory name:", modal);
        }

        CursorInit(display.fx24G);
        SetCursorControlling(true);
    }

    void FilesScene::DeleteFile(std::string filename)
    {
        std::string path{curr_directory + "/" + filename};
        if (path.ends_with('/'))
        {
            path.erase(path.end() - 1);
        }

        esp_err_t ret{};
        if (sdcard.IsDirectory(path.c_str()))
        {
            ret = sdcard.RemoveDirectory(path.c_str());
        }
        else
        {
            ret = sdcard.RemoveFile(path.c_str());
        }

        if (ESP_OK != ret)
            return;

        auto file_item = std::find_if(
            main_ui.begin() + Scene::GetContentUiStartIndex(),
            main_ui.end(),
            [&filename](auto &item)
            {
                return item.label == filename;
            });

        if (file_item == main_ui.end())
            return;

        if (file_item != main_ui.end() - 1)
        {
            ChangeItemFocus(&(*(file_item + 1)), true);
        }
        else
        {
            ChangeItemFocus(&(*(file_item - 1)), true);
        }

        auto next_displaying = std::find_if(
            file_item,
            main_ui.end(),
            [](auto &item)
            { return !item.displayable; });

        if (next_displaying != main_ui.end())
        {
            next_displaying->displayable = true;
            ESP_LOGI(TAG, "Make %s displayable", next_displaying->label.c_str());
        }
        ESP_LOGI(TAG, "Erasing file item %s", file_item->label.c_str());
        main_ui.erase(file_item);
    }

    bool FilesScene::CreateFile(std::string filename, bool is_directory)
    {
        if (filename.size() == 0 || !isalnum(filename[filename.size() - 1]))
        {
            return false;
        }

        std::string path{curr_directory + "/" + filename};
        esp_err_t ret{ESP_FAIL};

        if (is_directory)
        {
            ret = sdcard.CreateDirectory(path.c_str());
            filename.push_back('/');
        }
        else
        {
            ret = sdcard.CreateFile(path.c_str());
        }

        if (ESP_OK != ret)
        {
            return false;
        }

        std::transform(filename.begin(), filename.end(), filename.begin(), toupper);
        UiStringItem new_line{filename, Color::White, display.fx16G};

        size_t displayable_count{static_cast<size_t>(
            std::count_if(
                main_ui.begin() + GetContentUiStartIndex((uint8_t)FilesSceneStage::DirectoryStage),
                main_ui.end(),
                [](auto &item)
                { return item.displayable; }))};

        if (displayable_count == GetLinesPerPageCount((uint8_t)FilesSceneStage::DirectoryStage) - 1)
        {
            new_line.displayable = false;
        }

        if ((main_ui.end() - 1)->displayable && !(main_ui.end() - 1)->focusable)
        {
            main_ui.erase(main_ui.end() - 1, main_ui.end());
        }

        main_ui.push_back(new_line);
        return true;
    }

    size_t FilesScene::GetLinesScroll()
    {
        FilesSceneStage stage{GetStage<FilesSceneStage>()};

        switch (stage)
        {
        case FilesSceneStage::FileOpenStage:
            return file_lines_scroll;
        case FilesSceneStage::DirectoryStage:
            return directory_lines_scroll;
        case FilesSceneStage::CreateModalStage:
            return 0;
        default:
            return 0;
        }
    }

    void FilesScene::DetectLanguage(std::string filename)
    {
        std::string extension{};
        size_t extension_start{filename.find_last_of('.')};
        if (extension_start != std::string::npos)
        {
            extension.append(filename, extension_start);
            std::transform(extension.begin(), extension.end(), extension.begin(), tolower);
        }

        ESP_LOGI(TAG, "File extension: %s", extension.c_str());

        if (extension == ".lua")
        {
            runner_language = CodeLanguage::Lua;
        }
        else if (extension == ".py")
        {
            runner_language = CodeLanguage::Python;
        }
        // else if (extension == ".rb")
        // {
        //     runner_language = CodeLanguage::Ruby;
        // }
        else
        {
            runner_language = CodeLanguage::Text;
        }

        const char *arr[]{"Text", "Lua", "Python" /*, "Ruby" */};
        ESP_LOGI(TAG, "Detected language: %s", arr[(int)runner_language]);
    }

    void FilesScene::RunFile()
    {
        if (!IsStage(FilesSceneStage::FileOpenStage) ||
            runner_language == CodeLanguage::Text)
            return;

        std::string file_path{curr_directory + (*ui)[0].label};
        CodeRunner::CodeProcess process{
            .language = runner_language,
            .data = new char[file_path.size() + 1]{0},
            .is_file = true,
        };

        strncpy(process.data, file_path.c_str(), file_path.size() + 1);

        OpenStageModal(FilesSceneStage::CodeRunModalStage);
        xQueueSend(xQueueRunnerProcessing, &process, portMAX_DELAY);
    }

    bool FilesScene::IsCodeRunning()
    {
        return IsStage(FilesSceneStage::CodeRunModalStage);
    }
}