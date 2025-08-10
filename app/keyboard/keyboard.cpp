#include "keyboard.h"

using Sipo::SipoController, Piso::PisoController;
static const char *TAG = "Keyboard";

void TaskReadKeyboard(void *args);

namespace Keyboard
{
    TaskHandle_t TaskReadKeyboardHandler = NULL;

    const std::map<Key, std::array<char, 2>> KeyboardController::symbol_keys{
        {Key::NumOne, {'1', '!'}},
        {Key::NumTwo, {'2', '@'}},
        {Key::NumThree, {'3', '#'}},
        {Key::NumFour, {'4', '$'}},
        {Key::NumFive, {'5', '%'}},
        {Key::NumSix, {'6', '^'}},
        {Key::NumSeven, {'7', '&'}},
        {Key::NumEight, {'8', '*'}},
        {Key::NumNine, {'9', '('}},
        {Key::NumZero, {'0', ')'}},
        {Key::Minus, {'-', '-'}},
        {Key::Plus, {'+', '='}},
        {Key::LeftBracket, {'[', '{'}},
        {Key::RightBracket, {']', '}'}},
        {Key::Slash, {'\\', '|'}},
        {Key::Semicolon, {';', ':'}},
        {Key::Quote, {'\'', '"'}},
        {Key::Question, {'/', '?'}},
        {Key::Point, {'.', '>'}},
        {Key::Comma, {',', '<'}},
        {Key::Backticks, {'`', '~'}},
    };

    const std::map<Key, char> KeyboardController::letter_keys{
        {Key::LetterQ, 'q'},
        {Key::LetterW, 'w'},
        {Key::LetterE, 'e'},
        {Key::LetterR, 'r'},
        {Key::LetterT, 't'},
        {Key::LetterY, 'y'},
        {Key::LetterU, 'u'},
        {Key::LetterI, 'i'},
        {Key::LetterO, 'o'},
        {Key::LetterP, 'p'},
        {Key::LetterA, 'a'},
        {Key::LetterS, 's'},
        {Key::LetterD, 'd'},
        {Key::LetterF, 'f'},
        {Key::LetterG, 'g'},
        {Key::LetterH, 'h'},
        {Key::LetterJ, 'j'},
        {Key::LetterK, 'k'},
        {Key::LetterL, 'l'},
        {Key::LetterZ, 'z'},
        {Key::LetterX, 'x'},
        {Key::LetterC, 'c'},
        {Key::LetterV, 'v'},
        {Key::LetterB, 'b'},
        {Key::LetterN, 'n'},
        {Key::LetterM, 'm'},
    };

    KeyboardController::KeyboardController(
        gpio_num_t _clk,
        gpio_num_t sipo_lh,
        gpio_num_t sipo_ds,
        gpio_num_t piso_lh,
        gpio_num_t piso_ds)
        : _clk{_clk},
          _sipo_lh{sipo_lh},
          _sipo_ds{sipo_ds},
          _piso_lh{piso_lh},
          _piso_ds{piso_ds} {}

    esp_err_t KeyboardController::Init()
    {
        gpio_num_t pins[6] = {_clk, _sipo_lh, _sipo_ds, _piso_lh, _piso_ds};
        if (pdPASS != xTaskCreate(TaskReadKeyboard, "TaskReadKeyboard", 4096, pins, 10, &TaskReadKeyboardHandler))
        {
            return ESP_FAIL;
        };

        return ESP_OK;
    }

    bool KeyboardController::IsKeyPressed(Key key)
    {
        return ((~KeyState & (1ULL << (int)key)) != 0);
    }

    char KeyboardController::GetKeyValue(Key value_key)
    {
        if (value_key < Key::NumOne || value_key > Key::LetterM)
        {
            return 0;
        }

        char ch{0};
        bool is_shift_pressed{IsKeyPressed(Key::Shift)};

        if (symbol_keys.count(value_key) > 0)
        {
            ch = symbol_keys.at(value_key)[is_shift_pressed];
        }
        else if (letter_keys.count(value_key) > 0)
        {
            ch = letter_keys.at(value_key);

            if (is_shift_pressed)
            {
                ch = toupper(ch);
            }
        }

        return ch;
    }
}

void TaskReadKeyboard(void *args)
{
    gpio_num_t *pins{(gpio_num_t *)args};
    SipoController sipo{*pins, *(pins + 1), *(pins + 2)};
    PisoController piso{*pins, *(pins + 3), *(pins + 4)};

    sipo.Init();
    piso.Init();

    while (1)
    {
        Keyboard::KeyState = 0;

        for (uint8_t i = 0; i < 8; i++)
        {
            sipo.SendByte(~(1 << i), false);
            uint8_t read = piso.ReadByte();
            // ESP_LOGD(TAG, "Row (SIPO) %i: 0x%02X", 7 - i, read);
            Keyboard::KeyState = (Keyboard::KeyState << 8) | read;
        }

        ESP_LOGD(TAG, "KeyState 0x%016llx", ~Keyboard::KeyState);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}