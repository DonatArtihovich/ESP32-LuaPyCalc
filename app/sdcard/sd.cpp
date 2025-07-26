#include "sd.h"

static const char *TAG = "SD";

namespace SD
{
    SDCard::SDCard(gpio_num_t sd_miso, gpio_num_t sd_mosi, gpio_num_t sd_clk, gpio_num_t sd_cs)
        : _miso{sd_miso}, _mosi{sd_mosi}, _clk{sd_clk}, _cs{sd_cs} {}

    esp_err_t SDCard::Mount(const char path[])
    {
        strncpy(_mount_path, path, sizeof(_mount_path) - 1);
        const esp_vfs_fat_sdmmc_mount_config_t fat_mount_cfg = {
            .format_if_mount_failed = true,
            .max_files = 4,
            .allocation_unit_size = 16 * 1024,
            .use_one_fat = false,
        };

        sd_spi_host = SDSPI_HOST_DEFAULT();

        spi_bus_config_t bus_cfg = {
            .mosi_io_num = _mosi,
            .miso_io_num = _miso,
            .sclk_io_num = _clk,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 4000,
        };
        esp_err_t ret = spi_bus_initialize((spi_host_device_t)sd_spi_host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
        if (ESP_OK != ret)
        {
            ESP_LOGE(TAG, "SPI BUS initialisation failed.");
            return ret;
        }

        sdspi_device_config_t sd_spi_dev_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
        sd_spi_dev_cfg.gpio_cs = _cs;
        sd_spi_dev_cfg.host_id = (spi_host_device_t)sd_spi_host.slot;

        ret = esp_vfs_fat_sdspi_mount(_mount_path, &sd_spi_host, &sd_spi_dev_cfg, &fat_mount_cfg, &card);

        if (ESP_OK != ret)
        {
            if (ESP_FAIL == ret)
            {
                ESP_LOGE(TAG, "SD card mounting failed.");
            }
            else
            {
                ESP_LOGE(TAG, "SD card mounting error: (%i)", ret);
            }
        }

        return ret;
    }

    esp_err_t SDCard::ReadFile(const char *path, char *buff, size_t len)
    {
        esp_err_t ret = ESP_FAIL;
        FILE *file = fopen(path, "r");
        if (file != NULL)
        {
            fgets(buff, len, file);
            ret = ESP_OK;
        }

        fclose(file);
        return ret;
    }

    esp_err_t SDCard::WriteFile(const char *path, char *buff)
    {
        esp_err_t ret = ESP_FAIL;
        FILE *file = fopen(path, "w");
        if (file != NULL)
        {
            fputs(buff, file);
            ret = ESP_OK;
        }

        fclose(file);
        return ret;
    }

    std::vector<std::string> SDCard::ReadDirectory(const char *path)
    {
        std::vector<std::string> files{};
        DIR *dir = opendir(path);

        if (!dir)
        {
            ESP_LOGE(TAG, "Failed to read directory \"%s\"", path);
            closedir(dir);
            return files;
        }

        dirent *curr = nullptr;
        while ((curr = readdir(dir)) != nullptr)
        {
            ESP_LOGI(TAG, "SD Card read %s: %s, %d", path, curr->d_name, curr->d_type);
            files.push_back(std::string(curr->d_name));
        }

        closedir(dir);
        return files;
    }

    esp_err_t SDCard::Unmount()
    {
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_vfs_fat_sdcard_unmount(_mount_path, card));
        return spi_bus_free((spi_host_device_t)sd_spi_host.slot);
    }
}