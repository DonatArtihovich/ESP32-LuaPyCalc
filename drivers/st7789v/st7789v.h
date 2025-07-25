#ifndef MAIN_ST7789_H_
#define MAIN_ST7789_H_

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "fontx.h"

using Fontx::Font, Fontx::FontxFile;

#define rgb565(r, g, b) ~(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))

// #define RED rgb565(255, 0, 0)       // 0xf800
// #define GREEN rgb565(0, 255, 0)     // 0x07e0
// #define BLUE rgb565(0, 0, 255)      // 0x001f
// #define BLACK rgb565(0, 0, 0)       // 0x0000
// #define WHITE rgb565(255, 255, 255) // 0xffff
// #define GRAY rgb565(128, 128, 128)  // 0x8410
// #define YELLOW rgb565(255, 255, 0)  // 0xFFE0
// #define CYAN rgb565(0, 156, 209)    // 0x04FA
// #define PURPLE rgb565(128, 0, 128)  // 0x8010

namespace LCD
{

    enum class Color
    {
        Red = (uint16_t)rgb565(255, 0, 0),       // 0xf800
        Green = (uint16_t)rgb565(0, 255, 0),     // 0x07e0
        Blue = (uint16_t)rgb565(0, 0, 255),      // 0x001f
        Black = (uint16_t)rgb565(0, 0, 0),       // 0x0000
        White = (uint16_t)rgb565(255, 255, 255), // 0xffff
        Gray = (uint16_t)rgb565(128, 128, 128),  // 0x8410
        Yellow = (uint16_t)rgb565(255, 255, 0),  // 0xFFE0
        Cyan = (uint16_t)rgb565(0, 156, 209),    // 0x04FA
        Purple = (uint16_t)rgb565(128, 0, 128),  // 0x8010

    };

    typedef enum
    {
        DIRECTION0,
        DIRECTION90,
        DIRECTION180,
        DIRECTION270
    } DIRECTION;

    typedef enum
    {
        SCROLL_RIGHT = 1,
        SCROLL_LEFT = 2,
        SCROLL_DOWN = 3,
        SCROLL_UP = 4,
    } SCROLL_TYPE_t;

    typedef struct
    {
        uint16_t _width;
        uint16_t _height;
        uint16_t _offsetx;
        uint16_t _offsety;
        uint16_t _font_direction;
        uint16_t _font_fill;
        uint16_t _font_fill_color;
        uint16_t _font_underline;
        uint16_t _font_underline_color;
        gpio_num_t _dc;
        gpio_num_t _bl;
        spi_device_handle_t _SPIHandle;
        bool _use_frame_buffer;
        uint16_t *_frame_buffer;
    } TFT_t;

    class ST7789V
    {
        void spi_clock_speed(int speed);
        void spi_master_init(gpio_num_t mosi, gpio_num_t clk, gpio_num_t cs, gpio_num_t dc, gpio_num_t rst, gpio_num_t bl);
        bool spi_master_write_byte(spi_device_handle_t SPIHandle, const uint8_t *Data, size_t DataLength);
        bool spi_master_write_command(uint8_t cmd);
        bool spi_master_write_data_byte(uint8_t data);
        bool spi_master_write_data_word(uint16_t data);
        bool spi_master_write_addr(uint16_t addr1, uint16_t addr2);
        bool spi_master_write_color(Color color, uint16_t size);
        bool spi_master_write_colors(Color *colors, uint16_t size);

        TFT_t dev;
        uint16_t _width, _height;
        gpio_num_t _mosi, _clk, _cs, _dc, _rst, _bl;

    public:
        ST7789V(TFT_t _dev, gpio_num_t mosi, gpio_num_t clk, gpio_num_t cs, gpio_num_t dc, gpio_num_t rst, gpio_num_t bl);
        ST7789V(gpio_num_t mosi, gpio_num_t clk, gpio_num_t cs, gpio_num_t dc, gpio_num_t rst, gpio_num_t bl);
        ST7789V();

        void delayMS(int ms);
        void Init(uint16_t width, uint16_t height, int offsetx, int offsety);
        void DrawPixel(uint16_t x, uint16_t y, Color color);
        void DrawMultiPixels(uint16_t x, uint16_t y, uint16_t size, Color *colors);
        void DrawFillRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, Color color);
        void DrawFillSquare(uint16_t x0, uint16_t y0, uint16_t size, Color color);
        void DisplayOff();
        void DisplayOn();
        void FillScreen(Color color);
        void DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, Color color);
        void DrawRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, Color color);
        void DrawRectAngle(uint16_t xc, uint16_t yc, uint16_t w, uint16_t h, uint16_t angle, Color color);
        void DrawTriangle(uint16_t xc, uint16_t yc, uint16_t w, uint16_t h, uint16_t angle, Color color);
        void DrawRegularPolygon(uint16_t xc, uint16_t yc, uint16_t n, uint16_t r, uint16_t angle, Color color);
        void DrawCircle(uint16_t x0, uint16_t y0, uint16_t r, Color color);
        void DrawFillCircle(uint16_t x0, uint16_t y0, uint16_t r, Color color);
        void DrawRoundRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t r, Color color);
        void DrawArrow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t w, Color color);
        void DrawFillArrow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t w, Color color);
        int DrawChar(FontxFile *fx, uint16_t x, uint16_t y, uint8_t ascii, Color color);
        int DrawString(FontxFile *fx, uint16_t x, uint16_t y, uint8_t *ascii, Color color);
        int DrawCode(FontxFile *fx, uint16_t x, uint16_t y, uint8_t code, Color color);
        // int DrawUTF8Char(TFT_t * dev, FontxFile *fx, uint16_t x, uint16_t y, uint8_t *utf8, uint16_t color);
        // int DrawUTF8String(TFT_t * dev, FontxFile *fx, uint16_t x, uint16_t y, unsigned char *utfs, uint16_t color);
        void SetFontDirection(uint16_t);
        void SetFontFill(Color color);
        void UnsetFontFill();
        void SetFontUnderLine(Color color);
        void UnsetFontUnderLine();
        void BacklightOff();
        void BacklightOn();
        void InversionOff();
        void InversionOn();
        void WrapArround(SCROLL_TYPE_t scroll, int start, int end);
        void InversionArea(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t *save);
        void GetRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t *save);
        void SetRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t *save);
        void SetCursor(uint16_t x0, uint16_t y0, uint16_t r, Color color, uint16_t *save);
        void ResetCursor(uint16_t x0, uint16_t y0, uint16_t r, Color color, uint16_t *save);
        void DrawFinish();

        uint16_t GetWidth();
        uint16_t GetHeight();
    };
}

#endif /* MAIN_ST7789_H_ */
