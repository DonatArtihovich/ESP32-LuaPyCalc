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
    enum Themes
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
            CodeSuccessColor;
    };

    struct Theme
    {
        Themes key;
        ThemeColors Colors;
    };

    class Settings
    {
        static const std::map<Themes, Theme> themes;
        static Themes current_theme;
        static nvs_handle_t nvs_handle;
        static esp_err_t RestoreSettings();
        static esp_err_t SaveSettings();

    public:
        static esp_err_t Init();

        static const Theme &GetTheme();
        static void SetTheme(Themes theme);
    };
}