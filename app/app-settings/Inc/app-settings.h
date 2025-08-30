#pragma once

#include <map>

#include "st7789v.h"
#include "fontx.h"

using LCD::Color;

namespace Settings
{
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
        ThemeColors Colors;
    };

    enum Themes
    {
        Default,
        Light,
        Green,
    };

    class Settings
    {
        static const std::map<Themes, Theme> themes;
        static Themes current_theme;

    public:
        static const Theme &GetTheme();
        static void SetTheme(Themes theme);
    };
}