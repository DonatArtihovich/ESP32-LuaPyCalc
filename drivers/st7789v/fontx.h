#ifndef MAIN_FONTX_H_
#define MAIN_FONTX_H_

namespace Fontx
{
    typedef struct
    {
        const char *path;
        char fxname[10];
        bool opened;
        bool valid;
        bool is_ank;
        uint8_t w;
        uint8_t h;
        uint16_t fsz;
        uint8_t bc;
        FILE *file;
        unsigned char *fonts;
    } FontxFile;

    class Font
    {
    public:
        static void AddFontx(FontxFile *fx, const char *path);
        static void InitFontx(FontxFile *fxs, const char *f0, const char *f1);
        static bool OpenFontx(FontxFile *fx);
        static void CloseFontx(FontxFile *fx);
        static void DumpFontx(FontxFile *fxs);
        static uint8_t getFontWidth(FontxFile *fx);
        static uint8_t getFontHeight(FontxFile *fx);
        static bool GetFontx(FontxFile *fxs, uint8_t ascii, uint8_t *pw, uint8_t *ph);
        static void Font2Bitmap(uint8_t *fonts, uint8_t *line, uint8_t w, uint8_t h, uint8_t inverse);
        static void UnderlineBitmap(uint8_t *line, uint8_t w, uint8_t h);
        static void ReversBitmap(uint8_t *line, uint8_t w, uint8_t h);
        static void ShowFont(uint8_t *fonts, uint8_t pw, uint8_t ph);
        static void ShowBitmap(uint8_t *bitmap, uint8_t pw, uint8_t ph);
        static uint8_t RotateByte(uint8_t ch);
    };
}
// UTF8 to SJIS table
// https://www.mgo-tec.com/blog-entry-utf8sjis01.html
// #define Utf8Sjis "Utf8Sjis.tbl"
// uint16_t UTF2SJIS(spiffs_file fd, uint8_t *utf8);
// int String2SJIS(spiffs_file fd, unsigned char *str_in, size_t stlen, uint16_t *sjis, size_t ssize);
#endif /* MAIN_FONTX_H_ */
