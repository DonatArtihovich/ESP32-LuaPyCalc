#include "display.h"

static const char *TAG = "Display";

namespace Display
{
    uint32_t UiStringItem::lastId = 0;
    UiStringItem::UiStringItem(const char *label,
                               Color color,
                               FontxFile *font,
                               bool focusable,
                               Color backgroundColor,
                               uint16_t x,
                               uint16_t y)
    {
        id = UiStringItem::lastId++;
        this->label = label;
        this->x = x;
        this->y = y;
        this->color = color;
        this->backgroundColor = backgroundColor;
        this->font = font;
        this->focusable = focusable;
    }

    DisplayController::DisplayController(
        gpio_num_t mosi,
        gpio_num_t clk,
        gpio_num_t cs,
        gpio_num_t dc,
        gpio_num_t rst,
        gpio_num_t bl)
        : _mosi{mosi},
          _clk{clk},
          _cs{cs},
          _dc{dc},
          _rst{rst},
          _bl{bl}
    {
        lcd = LCD::ST7789V{LCD::TFT_t{}, _mosi, _clk, _cs, _dc, _rst, _bl};
    }

    esp_err_t DisplayController::Init()
    {
        esp_err_t ret = initFonts();
        if (ret != ESP_OK)
        {
            return ret;
        }

        lcd.Init(240, 320);

        return ESP_OK;
    }

    esp_err_t DisplayController::mountSPIFFS(const char *path, const char *label, size_t max_files)
    {
        esp_vfs_spiffs_conf_t conf = {
            .base_path = path,
            .partition_label = label,
            .max_files = max_files,
            .format_if_mount_failed = true,
        };
        esp_err_t ret = esp_vfs_spiffs_register(&conf);

        if (ret != ESP_OK)
        {
            if (ret == ESP_FAIL)
            {
                ESP_LOGE(TAG, "Failed to mount SPIFFS.");
            }
            else if (ret == ESP_ERR_NOT_FOUND)
            {
                ESP_LOGE(TAG, "Failed to find SPIFFS partition");
            }
            else
            {
                ESP_LOGE(TAG, "SPIFFS Error (%s)", esp_err_to_name(ret));
            }
        }

        size_t total = 0, used = 0;
        ret = esp_spiffs_info(conf.partition_label, &total, &used);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        }
        else
        {
            ESP_LOGI(TAG, "Mount %s to %s success", path, label);
            ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
        }

        return ret;
    }

    esp_err_t DisplayController::initFonts()
    {
        if (ESP_OK != mountSPIFFS("/fonts", "storage1", 4))
        {
            return ESP_FAIL;
        }

        Font::InitFontx(fx16G, "/fonts/ILGH16XB.FNT", "");
        Font::InitFontx(fx24G, "/fonts/ILGH24XB.FNT", "");
        Font::InitFontx(fx32G, "/fonts/ILGH32XB.FNT", "");
        Font::InitFontx(fx32L, "/fonts/LATIN32B.FNT", "");
        Font::InitFontx(fx16M, "/fonts/ILMH16XB.FNT", "");
        Font::InitFontx(fx24M, "/fonts/ILMH24XB.FNT", "");
        Font::InitFontx(fx32M, "/fonts/ILMH32XB.FNT", "");

        return ESP_OK;
    }

    void DisplayController::Clear(Color color, uint16_t x, uint16_t y, uint16_t x2, uint16_t y2)
    {
        if (x2 <= x || x2 > GetWidth())
        {
            x2 = GetWidth();
        }

        if (y2 <= y || y2 > GetHeight())
        {
            y2 = GetHeight();
        }

        lcd.DrawFillRect(y, x, y2, x2, color);
    }

    void DisplayController::DrawStringItem(UiStringItem *item, Position hp, Position vp)
    {
        if (!item->displayable)
        {
            return;
        }

        uint8_t fw, fh;
        Font::GetFontx(item->font, 0, &fw, &fh);
        uint16_t label_width = fw * item->label.size();

        switch (hp)
        {
        case Position::Start:
            item->x = 0;
            break;
        case Position::Center:
            item->x = (GetWidth() - label_width) / 2;
            break;
        case Position::End:
            item->x = GetWidth() - label_width;
            break;
        case Position::NotSpecified:
            break;
        }

        switch (vp)
        {
        case Position::Start:
            item->y = 0;
            break;
        case Position::Center:
            item->y = (GetHeight() - fh) / 2;
            break;
        case Position::End:
            item->y = GetHeight() - fh;
            break;
        case Position::NotSpecified:
            break;
        }

        if (item->backgroundColor != Color::None)
        {
            lcd.DrawFillRect(
                item->y,
                item->x,
                item->y + fh,
                item->x + label_width,
                item->backgroundColor);
        }

        lcd.SetFontDirection(1);
        lcd.DrawString(item->font, item->y, item->x, (uint8_t *)item->label.c_str(), item->color);
    }

    void DisplayController::DrawStringItems(
        std::vector<UiStringItem>::iterator start,
        std::vector<UiStringItem>::iterator end,
        int16_t x,
        int16_t y,
        bool direction)
    {
        lcd.SetFontDirection(1);
        uint8_t fw, fh;

        for (auto it = start; it < end; it++)
        {
            if (!it->displayable)
            {
                ESP_LOGI(TAG, "Item not displayable, %s", it->label.c_str());
                continue;
            }
            Font::GetFontx(it->font, 0, &fw, &fh);
            uint16_t width = it->label.size() * fw;

            it->x = x;
            it->y = y;

            if (direction)
            {
                y -= fh;
            }
            else
            {
                x += width;
            }

            if ((x > GetWidth() || y < 0 || y > GetHeight() || x < 0) && it < (end - 1))
            {
                it->displayable = false;
                (it + 1)->displayable = false;
            }

            if (!it->displayable)
            {
                ESP_LOGI(TAG, "Item not displayable, %s", it->label.c_str());
                continue;
            }
            else
            {
                ESP_LOGI(TAG, "Displaying item %s, x: %d, y: %d", it->label.c_str(), x, y);
            }

            if (it->backgroundColor != Color::None)
            {
                lcd.DrawFillRect(it->y, it->x, it->y + fh, it->x + width, it->backgroundColor);
            }
            lcd.DrawString(it->font, it->y, it->x, (uint8_t *)it->label.c_str(), it->color);
        }

        if (!(end - 1)->displayable)
        {
            for (auto it = end - 1; it >= start; it--)
            {
                if (it->displayable)
                {
                    Font::GetFontx(it->font, 0, &fw, &fh);
                    uint8_t label[30]{0};
                    snprintf((char *)label, 29, "%d More items", end - it - 1);
                    lcd.DrawString(
                        it->font,
                        direction ? it->y - fh : it->y,
                        direction ? it->x : it->x + it->label.size() * fw,
                        label,
                        Color::White);
                    break;
                }
            }
        }
    }

    uint16_t DisplayController::GetWidth()
    {
        return lcd.height;
    }

    uint16_t DisplayController::GetHeight()
    {
        return lcd.width;
    }
}