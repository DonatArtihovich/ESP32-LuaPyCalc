#include "files-scene.h"

static const char *TAG = "FileScene";

namespace Scene
{
    FilesScene::FilesScene(DisplayController &display, SDCard &_sdcard)
        : Scene(display), sdcard{_sdcard} {}

    void FilesScene::Init()
    {
        InitModals();
        ui->push_back(UiStringItem{"  Files   ", Color::White, display.fx32L, false});
        ui->push_back(UiStringItem{"< Esc", Color::White, display.fx24G});

        ui->push_back(UiStringItem{"Save", Color::White, display.fx24G}); // [Save] button
        (ui->end() - 1)->displayable = false;

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

    void FilesScene::RenderAll()
    {
        RenderHeader();
        RenderContent();
        if (IsCursorControlling())
        {
            RenderCursor();
        }
    }

    void FilesScene::RenderContent()
    {
        display.Clear(Color::Black, 0, 0, 0, display.GetHeight() - 35);
        display.DrawStringItems(ui->begin() + content_ui_start,
                                ui->end(),
                                10,
                                display.GetHeight() - 60,
                                GetLinesPerPageCount());
        if (!(ui->end() - 1)->displayable)
        {
            RenderUiListEnding(IsStage(FilesSceneStage::FileOpenStage)
                                   ? "more lines"
                                   : "more items");
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

    void FilesScene::Arrow(Direction direction)
    {
        Scene::Arrow(direction);
        if (IsStage(FilesSceneStage::FileOpenStage) &&
            !IsCursorControlling() &&
            direction == Direction::Bottom)
        {
            SetCursorControlling(true);
            auto focused = std::find_if(ui->begin(), ui->end(), [this](auto &item)
                                        { return item.focused; });
            if (focused != ui->end())
            {
                ChangeItemFocus(&(*focused), false, true);
            }

            SpawnCursor(-1, -1, false);
            return;
        }

        if (IsCursorControlling())
        {
            MoveCursor(direction, true, file_lines_scroll);
            return;
        }

        Focus(direction);
    }

    SceneId FilesScene::Enter()
    {
        if (IsCursorControlling())
        {
            CursorInsertChars("\n", file_lines_scroll);
            return SceneId::CurrentScene;
        }

        auto focused = std::find_if(
            ui->begin(),
            ui->end(),
            [](auto &item) mutable
            { return item.focused; });

        if (focused == ui->end())
        {
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
        else if (focused->label.contains("^ Up"))
        {
            ScrollContent(Direction::Up, true, directory_lines_scroll);
        }
        else if (focused->label.contains("Save"))
        {
            SaveFile();
        }
        else if (focused->label.contains("< Esc"))
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
            LeaveModalControlling();
        }
        else
        {
            return SceneId::StartScene;
        }

        return SceneId::CurrentScene;
    }

    void FilesScene::Delete()
    {
        if (IsCursorControlling())
        {
            CursorDeleteChars(1, file_lines_scroll);
        }
        else if (!IsModalStage())
        {
            OpenStageModal(FilesSceneStage::DeleteModalStage);
        }
    }

    uint8_t FilesScene::Focus(Direction direction)
    {
        if (!Scene::Focus(direction))
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
        size_t up_index{3};
        if (mode)
        {
            (*ui)[up_index].label = "^ Up";
            (*ui)[up_index].focusable = true;
        }
        else
        {
            ChangeItemFocus(&(*ui)[up_index], false);
            (*ui)[up_index].label.clear();
            (*ui)[up_index].focusable = false;
            ChangeItemFocus(&(*GetContentUiStart()), true);
        }

        display.DrawStringItem(&*(ui->begin() + up_index));
    }

    void FilesScene::ToggleSaveButton(bool mode, bool rerender)
    {
        size_t save_index{2};
        (*ui)[save_index].displayable = mode;

        if (rerender)
        {
            display.DrawStringItem(&*(ui->begin() + save_index));
        }
    }

    void FilesScene::OpenFile(const char *relative_path)
    {
        SaveDirectory();
        ChangeHeader(relative_path, true);
        ChangeItemFocus(&(*ui)[1], true, true);
        SetStage(FilesSceneStage::FileOpenStage);
        ToggleSaveButton(true, true);
        ui->erase(GetContentUiStart(), ui->end());

        char buff[file_line_length + 1] = {0};
        uint32_t seek_pos{0};
        int read = 0;
        while ((read = sdcard.ReadFile(
                    (curr_directory + relative_path).c_str(),
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

        uint8_t fw, fh;
        Font::GetFontx(display.fx16G, 0, &fw, &fh);

        RenderContent();

        size_t lines_count = ui->size() - GetContentUiStartIndex();

        Cursor cursor{
            .x = static_cast<uint8_t>(file_line_length - 1),
            .y = static_cast<uint8_t>(lines_count > file_lines_per_page
                                          ? file_lines_per_page - 1
                                          : lines_count - 1),
            .width = fw,
            .height = fh,
        };
        CursorInit(&cursor);
    }

    void FilesScene::ChangeHeader(const char *header, bool rerender)
    {
        Scene::ChangeHeader(header);

        if (rerender)
        {
            RenderHeader();
        }
    }

    void FilesScene::RenderHeader()
    {
        display.Clear(Color::Black, 0, display.GetHeight() - 60, display.GetWidth(), display.GetHeight());
        display.DrawStringItem(&(*ui)[0], Position::Center, Position::End);
        display.DrawStringItem(&(*ui)[1], Position::Start, Position::End);
        display.DrawStringItem(&(*ui)[2], Position::End, Position::End);
    }

    void FilesScene::CloseFile()
    {
        ui->erase(GetContentUiStart(), ui->end());
        std::for_each(ui->begin(), ui->end(), [this](auto &item)
                      { if (item.focused) ChangeItemFocus(&item, false); });

        ui->insert(ui->end(), directory_backup.begin(), directory_backup.end());
        SetStage(FilesSceneStage::DirectoryStage);
        ChangeHeader("Files");
        ToggleSaveButton(false);
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

    size_t FilesScene::GetContentUiStartIndex()
    {
        return content_ui_start + !IsStage(FilesSceneStage::FileOpenStage);
    }

    uint8_t FilesScene::GetLinesPerPageCount()
    {
        return IsStage(FilesSceneStage::FileOpenStage)
                   ? file_lines_per_page
                   : directory_lines_per_page;
    }

    size_t FilesScene::GetLineLength()
    {
        return IsStage(FilesSceneStage::FileOpenStage)
                   ? file_line_length
                   : default_line_length;
    }

    uint8_t FilesScene::ScrollContent(Direction direction, bool rerender, uint8_t count)
    {
        size_t lines_per_page{GetLinesPerPageCount()};
        bool is_file_open_stage{IsStage(FilesSceneStage::FileOpenStage)};

        if (count > lines_per_page - 1 - !is_file_open_stage)
            count = lines_per_page - 1 - !is_file_open_stage;

        if (direction == Direction::Bottom || direction == Direction::Up)
        {
            count = Scene::ScrollContent(direction, rerender, count);

            if (!is_file_open_stage)
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

        ESP_LOGI(TAG, "File %s saved.", file_path.c_str());
    }

    void FilesScene::InitModals()
    {
        std::vector<UiStringItem> delete_modal_ui{};

        delete_modal_ui.push_back(UiStringItem{"Do you want to delete?", Color::White, display.fx24G, false});
        display.SetPosition(&*(delete_modal_ui.end() - 1), Position::Center, Position::End);

        delete_modal_ui.push_back(UiStringItem{"Ok", Color::White, display.fx24G});
        display.SetPosition(&*(delete_modal_ui.end() - 1), Position::Start, Position::Start);
        ChangeItemFocus(&*(delete_modal_ui.end() - 1), true);

        delete_modal_ui.push_back(UiStringItem{"Cancel", Color::White, display.fx24G});
        display.SetPosition(&*(delete_modal_ui.end() - 1), Position::End, Position::Start);

        modals_ui[(uint8_t)FilesSceneStage::DeleteModalStage] = delete_modal_ui;
    }

    bool FilesScene::IsModalStage()
    {
        FilesSceneStage stage{GetStage<FilesSceneStage>()};
        return stage == FilesSceneStage::CreateModalStage ||
               stage == FilesSceneStage::DeleteModalStage;
    }

    void FilesScene::EnterModalControlling()
    {
        Scene::EnterModalControlling();
    }

    void FilesScene::LeaveModalControlling()
    {
        Scene::LeaveModalControlling();
        SetStage(FilesSceneStage::DirectoryStage);
    }
}