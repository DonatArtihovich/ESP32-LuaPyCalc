#pragma once

#include <cstdio>
#include <dirent.h>
#include <cstring>
#include <vector>
#include <string>
#include <cerrno>

#include "esp_log.h"
#include "driver/spi_master.h"
#include "sdkconfig.h"

#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

#define SD(path) CONFIG_MOUNT_POINT path

namespace SD
{
    class SDCard
    {
        gpio_num_t _miso, _mosi, _clk, _cs;
        sdmmc_host_t sd_spi_host;
        char _mount_path[50] = {0};

    public:
        sdmmc_card_t *card;

        SDCard(gpio_num_t sd_miso, gpio_num_t sd_mosi, gpio_num_t sd_clk, gpio_num_t sd_cs);
        esp_err_t Mount(const char path[]);
        esp_err_t Unmount();

        size_t ReadFile(const char *path, char *buff, size_t len, uint32_t pos = 0, uint8_t seek_point = SEEK_SET);
        esp_err_t WriteFile(const char *path, const char *buff, uint32_t pos = 0, uint8_t seek_point = SEEK_SET);
        std::vector<std::string> ReadDirectory(const char *path);
        bool IsDirectory(const char *path);
    };
}