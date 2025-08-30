#include "app-settings.h"

static const char *TAG = "Settings";

namespace Settings
{
    const std::map<Themes, Theme> Settings::themes{
        {Themes::Default,
         {Themes::Default, {
                               Color::Black, // MainBackgroundColor
                               Color::White, // SecondaryBackgroundColor
                               Color::Blue,  // FocusedBackgroundColor
                               Color::White, // MainTextColor
                               Color::Black, // SecondaryTextColor
                               Color::White, // FocusedTextColor
                               Color::White, // CursorColor
                               Color::Red,   // CodeErrorColor
                               Color::Green, // CodeSuccessColor
                           }}},
        {Themes::Light, {Themes::Light, {
                                            Color::White, // MainBackgroundColor
                                            Color::Black, // SecondaryBackgroundColor
                                            Color::Blue,  // FocusedBackgroundColor
                                            Color::Black, // MainTextColor
                                            Color::White, // SecondaryTextColor
                                            Color::White, // FocusedTextColor
                                            Color::Black, // CursorColor
                                            Color::Red,   // CodeErrorColor
                                            Color::Green, // CodeSuccessColor
                                        }}},
        {Themes::Green, {Themes::Green, {
                                            Color::DarkGreen,  // MainBackgroundColor
                                            Color::White,      // SecondaryBackgroundColor
                                            Color::DarkYellow, // FocusedBackgroundColor
                                            Color::White,      // MainTextColor
                                            Color::Black,      // SecondaryTextColor
                                            Color::White,      // FocusedTextColor
                                            Color::White,      // CursorColor
                                            Color::Red,        // CodeErrorColor
                                            Color::Green,      // CodeSuccessColor
                                        }}},
    };

    Themes Settings::current_theme{Themes::Default};
    FilesSortingModes Settings::current_files_sorting{FilesSortingModes::AlphabetAscending};
    nvs_handle_t Settings::nvs_handle{};

    esp_err_t Settings::Init()
    {
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }

        ret |= RestoreSettings();

        return ret;
    }

    const Theme &Settings::GetTheme()
    {
        if (themes.count(current_theme) == 0)
        {
            SetTheme(Themes::Default);
        }

        return themes.at(current_theme);
    }

    void Settings::SetTheme(Themes theme)
    {
        current_theme = theme;
        SaveTheme();
    }

    void Settings::SetFilesSortingMode(FilesSortingModes mode)
    {
        current_files_sorting = mode;
        SaveFilesSortingMode();
    }

    esp_err_t Settings::RestoreSettings()
    {
        esp_err_t ret{nvs_open("settings", NVS_READONLY, &nvs_handle)};
        if (ret == ESP_OK)
        {
            int8_t buffer{};
            ret = nvs_get_i8(nvs_handle, "theme", &buffer);
            if (ret == ESP_OK)
            {
                ESP_LOGI(TAG, "Read theme from NVS: %d", buffer);
                current_theme = (Themes)buffer;
            }
            else if (ret == ESP_ERR_NVS_NOT_FOUND)
            {
                SaveTheme();
            }

            ret = nvs_get_i8(nvs_handle, "fsort", &buffer);
            if (ret == ESP_OK)
            {
                ESP_LOGI(TAG, "Read files sorting from NVS: %d", buffer);
                current_files_sorting = (FilesSortingModes)buffer;
            }
            else if (ret == ESP_ERR_NVS_NOT_FOUND)
            {
                SaveFilesSortingMode();
            }

            nvs_close(nvs_handle);
        }
        else if (ret == ESP_ERR_NVS_NOT_FOUND)
        {
            ret = SaveTheme() | SaveFilesSortingMode();
        }

        return ret;
    }

    esp_err_t Settings::SaveTheme()
    {
        esp_err_t ret{nvs_open("settings", NVS_READWRITE, &nvs_handle)};
        if (ret == ESP_OK)
        {
            ret = nvs_set_i8(nvs_handle, "theme", (int8_t)current_theme);
            if (ret == ESP_OK)
            {
                ESP_LOGI(TAG, "Save theme into NVS: %d", (int8_t)current_theme);
            }

            nvs_close(nvs_handle);
        }

        return ret;
    }

    esp_err_t Settings::SaveFilesSortingMode()
    {
        esp_err_t ret{nvs_open("settings", NVS_READWRITE, &nvs_handle)};
        if (ret == ESP_OK)
        {
            ret = nvs_set_i8(nvs_handle, "fsort", (int8_t)current_files_sorting);
            if (ret == ESP_OK)
            {
                ESP_LOGI(TAG, "Save files sorting mode into NVS: %d", (int8_t)current_files_sorting);
            }

            nvs_close(nvs_handle);
        }

        return ret;
    }

    FilesSortingModes Settings::GetFilesSortingMode()
    {
        return current_files_sorting;
    }
}