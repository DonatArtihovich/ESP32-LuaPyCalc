#pragma once

#include <map>

#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "st7789v.h"
#include "fontx.h"

using LCD::Color;

namespace Settings
{
    enum class Themes
    {
        Default,
        Light,
        Green,
    };

    struct ThemeColors
    {
        Color MainBackgroundColor,
            SecondaryBackgroundColor,
            FocusedBackgroundColor,
            MainTextColor,
            SecondaryTextColor,
            FocusedTextColor,
            CursorColor,
            CodeErrorColor,
            CodeSuccessColor,
            SelectingColor;
    };

    struct Theme
    {
        Themes key;
        ThemeColors Colors;
    };

    enum class FilesSortingModes
    {
        AlphabetAscending,
        AlphabetDescending,
        FilesFirstAlphabetAscending,
        FilesFirstAlphabetDescending,
        DirectoriesFirstAlphabetAscending,
        DirectoriesFirstAlphabetDescending,
    };

    class Settings
    {
        static const std::map<Themes, Theme> themes;
        static Themes current_theme;
        static FilesSortingModes current_files_sorting;
        static nvs_handle_t nvs_handle;
        static esp_err_t RestoreSettings();
        static esp_err_t SaveTheme();
        static esp_err_t SaveFilesSortingMode();

    public:
        static esp_err_t Init();

        static const Theme &GetTheme();
        static void SetTheme(Themes theme);

        static FilesSortingModes GetFilesSortingMode();
        static void SetFilesSortingMode(FilesSortingModes mode);
    };
}