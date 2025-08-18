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

    esp_err_t LuaRunController::RunCodeString(const char *code, char *traceback, size_t traceback_len)
    {
        esp_err_t ret{ESP_OK};
        ESP_LOGI(TAG, "Run Lua code string...");

        lua_State *L{setup_lua()};
        int lua_ret = LUA_OK; // luaL_dostring(L, code);

        if ((lua_ret = luaL_loadstring(L, code)) != LUA_OK)
        {
            snprintf(traceback, traceback_len, lua_tostring(L, -1));
            lua_pop(L, 1); // delete err msg
        }
        else
        {
            lua_ret = lua_pcall(L, 0, LUA_MULTRET, 0);

            if (lua_ret != LUA_OK)
            {
                lua_getglobal(L, "debug"); // get traceback:
                lua_getfield(L, -1, "traceback");
                lua_pushvalue(L, -3);  // copy error msg
                lua_pushinteger(L, 2); // stack trace depth

                lua_call(L, 2, 1); // debug.traceback(err)

                const char *tr = lua_tostring(L, 1);
                if (tr != NULL)
                {
                    ESP_LOGI(TAG, "traceback_len: %d", traceback_len);
                    snprintf(traceback, traceback_len, tr);
                    lua_pop(L, 2); // delete err msg and traceback
                }
                else
                {
                    snprintf(traceback, traceback_len, "Runtime error.");
                    lua_pop(L, 2); // delete err msg and traceback
                }
            }
        }

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

            if (s == NULL)
                s = "nil";

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
            else
            {
                xQueueSend(xQueueRunnerStdout, "\n", portMAX_DELAY);
            }
        }

        return 0;
    }

    int LuaRunController::lua_io_read_impl(lua_State *L)
    {
        if (CodeRunController::SetIsWaitingInput(true) != ESP_OK)
        {
            lua_pushnil(L);
            return 1;
        }
        ESP_LOGI(TAG, "Is waiting input: true");

        std::string io_buff{};
        char ch = 0;
        while (xQueueReceive(xQueueRunnerStdin, &ch, portMAX_DELAY) == pdPASS &&
               ch != '\n')
        {
            io_buff.push_back(ch);
        }

        if (io_buff.size() || ch == '\n')
        {
            lua_pushstring(L, io_buff.c_str());
        }
        else
        {
            lua_pushnil(L);
        }

        ESP_LOGI(TAG, "Is waiting input: false");
        CodeRunController::SetIsWaitingInput(false);

        return 1;
    }
}