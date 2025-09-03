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

        ESP_LOGD(TAG, "Read %d from %s: %s", result, path, buff);
        return result;
    }

    esp_err_t SDCard::WriteFile(
        const char *path,
        const char *buff,
        uint32_t pos,
        std::ios_base::seekdir seek_point,
        std::ios_base::openmode mode)
    {
        esp_err_t ret = ESP_FAIL;

        std::ofstream file(path, mode);

        if (file.is_open())
        {
            file.seekp(pos, seek_point);
            file << buff;
            file.close();
            ret = ESP_OK;
        }

        return ret;
    }

    esp_err_t SDCard::ReadDirectory(const char *path, std::vector<std::string> &files)
    {
        DIR *dir = opendir(path);

        if (!dir)
        {
            ESP_LOGE(TAG, "Failed to read directory \"%s\"", path);
            closedir(dir);
            return ESP_FAIL;
        }

        dirent *curr = nullptr;
        while ((curr = readdir(dir)) != nullptr)
        {
            ESP_LOGI(TAG, "SD Card read %s: %s, %d", path, curr->d_name, curr->d_type);
            std::string filename(curr->d_name);
            if (curr->d_type == 2)
            {
                filename.push_back('/');
            }

            files.push_back(filename);
        }

        closedir(dir);
        return ESP_OK;
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
        ESP_LOGI(TAG, "RemoveDirectory");
        DIR *dir = opendir(path);
        if (!dir)
        {
            return ESP_FAIL;
        }

        struct dirent *entry;
        char full_path[512] = {0};

        while ((entry = readdir(dir)) != nullptr)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

            if (IsDirectory(full_path))
            {
                RemoveDirectory(full_path);
            }
            else
            {
                RemoveFile(full_path);
            }
        }
        closedir(dir);

        if (rmdir(path) != 0)
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
        if ((Exists(path) ||
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
        if (Exists(path))
        {
            return ESP_OK;
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

    esp_err_t SDCard::CopyFile(const char *path, const char *new_path)
    {
        if (!Exists(path) || Exists(new_path))
        {
            return ESP_FAIL;
        }

        ESP_LOGI(TAG, "Copy file %s to %s", path, new_path);
        if (!IsDirectory(path))
        {
            size_t pos{};
            char buff[250] = {0};

            size_t curr_read{};
            while ((curr_read = ReadFile(path, buff, sizeof(buff), pos)) > 0)
            {
                if (WriteFile(new_path, buff, pos, std::ios::beg, std::ios::app) != ESP_OK)
                {
                    return ESP_FAIL;
                }
                memset(buff, 0, sizeof(buff));
                pos += curr_read;
            }
        }
        else
        {
            std::vector<std::string> files{};
            esp_err_t ret{ReadDirectory(path, files)};

            if (ESP_OK != ret)
                return ret;

            CreateDirectory(new_path);
            ESP_LOGI(TAG, "Create directory %s", new_path);
            for (std::string &file : files)
            {
                std::string old_path{std::string(path) + "/" + file};
                std::string copy_path{std::string(new_path) + "/" + file};
                esp_err_t ret{IsDirectory(old_path.c_str())
                                  ? CreateDirectory(copy_path.c_str())
                                  : CopyFile(old_path.c_str(), copy_path.c_str())};

                if (ret != ESP_OK)
                {
                    return ESP_FAIL;
                }
                ESP_LOGI(TAG, "Copy Dir File %s to %s", (std::string(path) + "/" + file).c_str(), (std::string(new_path) + "/" + file).c_str());
            }
        }

        return ESP_OK;
    }

    bool SDCard::Exists(const char *path)
    {
        std::error_code ec{};
        if (std::filesystem::exists(path, ec) && !ec)
        {
            ESP_LOGI(TAG, "Path %s exists", path);
            return true;
        }

        ESP_LOGI(TAG, "Path %s not exists", path);
        return false;
    }
}