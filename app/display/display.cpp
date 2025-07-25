#include "display.h"

static const char *TAG = "Display";

namespace Display
{
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

        lcd.Init(240, 320, 0, 0);

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

    void DisplayController::MainMenu()
    {
        lcd.DrawFillRect(0, 0, lcd.GetWidth(), lcd.GetHeight(), Color::Black);
        drawMenu(fx32L, "Menu", Color::White, fx24M, menus[Menu::Main], Color::White);
    }

    void DisplayController::drawMenu(
        FontxFile *header_font,
        const char *header,
        Color header_color,
        FontxFile *item_font,
        std::vector<const char *> items,
        Color item_color)
    {
        lcd.SetFontDirection(1);

        uint8_t hfw, hfh;
        Font::GetFontx(header_font, 0, &hfw, &hfh);
        lcd.DrawString(header_font, lcd.GetWidth() - hfh, (lcd.GetHeight() - (strlen(header) * hfw)) / 2, (uint8_t *)header, Color::White);

        uint8_t ifw, ifh;
        for (int i = 0; i < items.size(); i++)
        {
            ESP_LOGI(TAG, "Menu item: %s", items[i]);
            Font::GetFontx(item_font, 0, &ifw, &ifh);
            lcd.DrawString(item_font, lcd.GetWidth() - hfh - (ifh - 1), 0, (uint8_t *)items[i], Color::White);
        }
    }
}