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
        };

        // sd_spi_host = SDSPI_HOST_DEFAULT();

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

    size_t SDCard::ReadFile(const char *path, char *buff, size_t len, uint32_t pos, uint8_t seek_point)
    {
        FILE *file = fopen(path, "r");
        if (file != NULL)
        {
            fseek(file, pos, seek_point);
            fgets(buff, len, file);
        }

        uint32_t current_pos = ftell(file);
        fseek(file, pos, seek_point);
        size_t result = current_pos - ftell(file);
        fclose(file);

        ESP_LOGI(TAG, "Read %d from %s: %s", result, path, buff);
        return result;
    }

    esp_err_t SDCard::WriteFile(const char *path, const char *buff, uint32_t pos, std::ios_base::seekdir seek_point)
    {
        esp_err_t ret = ESP_FAIL;

        std::ofstream file(path, std::ios::out);
        if (file.is_open())
        {
            file.seekp(pos, seek_point);
            file << buff;
            file.close();
            ret = ESP_OK;
        }

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
            ESP_LOGD(TAG, "SD Card read %s: %s, %d", path, curr->d_name, curr->d_type);
            std::string filename(curr->d_name);
            if (curr->d_type == 2)
            {
                filename.push_back('/');
            }

            files.push_back(filename);
        }

        closedir(dir);
        return files;
    }

    esp_err_t SDCard::Unmount()
    {
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_vfs_fat_sdcard_unmount(_mount_path, card));
        return spi_bus_free((spi_host_device_t)sd_spi_host.slot);
    }

    bool SDCard::IsDirectory(const char *path)
    {
        DIR *directory = opendir(path);

        if (directory != nullptr)
        {
            closedir(directory);
            return true;
        }

        return false;
    }

    esp_err_t SDCard::RemoveDirectory(const char *path)
    {
        namespace fs = std::filesystem;
        std::error_code ec{};

        if (fs::exists(path, ec))
        {
            if (ec)
                return ESP_FAIL;

            bool removed = fs::remove_all(path, ec);

            if (ec || !removed)
                return ESP_FAIL;
        }
        else
        {
            return ESP_FAIL;
        }

        return ESP_OK;
    }

    esp_err_t SDCard::RemoveFile(const char *path)
    {
        if (unlink(path) == 0)
        {
            return ESP_OK;
        }

        return ESP_FAIL;
    }

    esp_err_t SDCard::CreateDirectory(const char *path)
    {
        ESP_LOGI(TAG, "Creating directory at path %s", path);
        std::error_code ec{};
        if ((std::filesystem::exists(path, ec) ||
             std::filesystem::create_directory(path)) &&
            !ec)
        {
            return ESP_OK;
        }

        return ESP_FAIL;
    }

    esp_err_t SDCard::CreateFile(const char *path)
    {
        ESP_LOGI(TAG, "Creating file at path %s", path);
        std::error_code ec{};
        if (std::filesystem::exists(path, ec))
        {
            return ESP_OK;
        }
        else if (ec)
        {
            return ESP_FAIL;
        }

        std::fstream file{};
        file.open(path, std::ios::app);
        file.close();

        if (std::filesystem::exists(path, ec))
        {
            return ESP_OK;
        }
        else if (ec)
        {
            return ESP_FAIL;
        }

        return ESP_FAIL;
    }

    esp_err_t SDCard::RenameFile(const char *path, const char *new_path)
    {
        ESP_LOGI(TAG, "Rename file %s to %s", path, new_path);
        std::error_code ec{};
        std::filesystem::rename(path, new_path, ec);

        if (!ec)
        {
            return ESP_OK;
        }

        return ESP_FAIL;
    }
}