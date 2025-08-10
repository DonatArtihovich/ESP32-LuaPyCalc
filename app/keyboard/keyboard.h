#pragma once
#include "sipo.h"
#include "piso.h"
#include <map>
#include <array>
#include <cctype>

namespace Keyboard
{
    enum class Key
    {
        // -----Controls-----
        Enter, // (o0,i0)
        Top,
        Right,
        Bottom,
        Left,
        Escape,
        Delete,
        Ctrl,
        Shift, // (o1, i0)
        Caps,
        Space,
        // -----Numbers-----
        NumOne,       // 1 !
        NumTwo,       // 2 @
        NumThree,     // 3 #
        NumFour,      // 4 $
        NumFive,      // 5 %
        NumSix,       // 6 ^ (o2, i0)
        NumSeven,     // 7 &
        NumEight,     // 8 *
        NumNine,      // 9 (
        NumZero,      // 0 )
                      // -----Signs-----
        Minus,        // -
        Plus,         // +=
        LeftBracket,  // [{
        RightBracket, // ]} (o3, i0)
        Slash,        // \|
        Semicolon,    // ;:
        Quote,        // '"
        Question,     // /?
        Point,        // .>
        Comma,        // ,<
        Backticks,    // `~
                      // -----Letters-----
        LetterQ,      // qQ (o4, i0)
        LetterW,      // wW
        LetterE,      // eE
        LetterR,      // rR
        LetterT,      // tT
        LetterY,      // yY
        LetterU,      // uU
        LetterI,      // iI
        LetterO,      // oO (o5, i0)
        LetterP,      // pP
        LetterA,      // aA
        LetterS,      // sS
        LetterD,      // dD
        LetterF,      // fF
        LetterG,      // gG
        LetterH,      // hH
        LetterJ,      // jJ (o6, i0)
        LetterK,      // kK
        LetterL,      // lL
        LetterZ,      // zZ
        LetterX,      // xX
        LetterC,      // cC
        LetterV,      // vV
        LetterB,      // bB
        LetterN,      // nN (o7, i0)
        LetterM,      // mM
    };

    static volatile uint64_t KeyState = 0xFFFFFFFF;

    class KeyboardController
    {
        gpio_num_t _clk, _sipo_lh, _sipo_ds, _piso_lh, _piso_ds;
        static const std::map<Key, std::array<char, 2>> symbol_keys;
        static const std::map<Key, char> letter_keys;

    public:
        KeyboardController(
            gpio_num_t _clk,
            gpio_num_t sipo_lh,
            gpio_num_t sipo_ds,
            gpio_num_t piso_lh,
            gpio_num_t piso_ds);

        esp_err_t Init();
        static bool IsKeyPressed(Key key);
        static char GetKeyValue(Key value_key);
    };
}