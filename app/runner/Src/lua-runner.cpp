#include "lua-runner.h"

static const char *TAG = "LuaRunController";

extern QueueHandle_t xQueueRunnerStdout;
extern QueueHandle_t xQueueRunnerStdin;

namespace CodeRunner
{

    lua_State *LuaRunController::setup_lua()
    {
        lua_State *L = luaL_newstate();
        luaL_openlibs(L);

        lua_pushcfunction(L, lua_print_impl);
        lua_setglobal(L, "print");

        lua_getglobal(L, "io");
        lua_pushcfunction(L, lua_io_read_impl);
        lua_setfield(L, -2, "read");

        return L;
    }

    esp_err_t LuaRunController::RunCodeString(const char *code)
    {
        esp_err_t ret{ESP_OK};
        ESP_LOGI(TAG, "Run Lua code string...");

        lua_State *L{setup_lua()};
        int lua_ret = luaL_dostring(L, code);
        lua_close(L);

        if (lua_ret != LUA_OK)
        {
            ESP_LOGE(TAG, "Lua error: %d", lua_ret);
            ret = ESP_FAIL;
        }

        // #define LUA_OK 0
        // #define LUA_YIELD 1
        // #define LUA_ERRRUN 2
        // #define LUA_ERRSYNTAX 3
        // #define LUA_ERRMEM 4
        // #define LUA_ERRERR 5

        return ret;
    }

    esp_err_t LuaRunController::RunCodeFile(const char *path)
    {
        esp_err_t ret{ESP_OK};
        ESP_LOGI(TAG, "Run Lua code file %s...", path);

        lua_State *L{setup_lua()};
        int lua_ret = luaL_dofile(L, path);
        lua_close(L);

        if (lua_ret != LUA_OK)
        {
            ESP_LOGE(TAG, "Lua error: %d", lua_ret);
            ret = ESP_FAIL;
        }

        return ret;
    }

    int LuaRunController::lua_print_impl(lua_State *L)
    {
        int n = lua_gettop(L);

        char log_buffer[64] = {0};
        for (int i = 1; i <= n; i++)
        {
            const char *s = lua_tostring(L, i);
            size_t s_len = strlen(s);

            for (const char *p = s; p < s + s_len; p += sizeof(log_buffer))
            {
                snprintf(log_buffer, sizeof(log_buffer), "%s", p);

                xQueueSend(xQueueRunnerStdout, log_buffer, portMAX_DELAY);
                memset(log_buffer, 0, sizeof(log_buffer));
            }

            if (i < n)
            {
                xQueueSend(xQueueRunnerStdout, "\t", portMAX_DELAY);
            }
        }

        return 0;
    }

    int LuaRunController::lua_io_read_impl(lua_State *L)
    {
        char io_buff[1] = {0};
        if (xQueueReceive(xQueueRunnerStdin, io_buff, portMAX_DELAY) == pdPASS)
        {
            lua_pushstring(L, io_buff);
        }
        else
        {
            lua_pushnil(L);
        }

        return 1;
    }
}