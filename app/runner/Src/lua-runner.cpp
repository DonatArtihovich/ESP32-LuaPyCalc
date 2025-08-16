#include "lua-runner.h"

static const char *TAG = "LuaRunController";

namespace CodeRunner
{
    esp_err_t LuaRunController::RunCodeString(const char *code)
    {
        esp_err_t ret{ESP_OK};
        ESP_LOGI(TAG, "Run Lua Code: %s", code);

        lua_State *L = luaL_newstate();
        luaL_openlibs(L);

        int lua_ret = luaL_dostring(L, code);

        lua_close(L);

        if (lua_ret != LUA_OK)
        {
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
}