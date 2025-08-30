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
    };

    const Theme &Settings::GetTheme()
    {
        return themes.at(Themes::Default);
    }
}