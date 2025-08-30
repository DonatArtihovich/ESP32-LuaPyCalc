#include "app-settings.h"

namespace Settings
{
    const std::map<Themes, Theme> Settings::themes{
        {Themes::Default,
         {{
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
        {Themes::Light,
         {{
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
        {Themes::Green,
         {{
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
    }
}