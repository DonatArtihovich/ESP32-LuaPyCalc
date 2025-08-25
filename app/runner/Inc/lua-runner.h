#pragma once

#include "runner.h"

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "lmem.h"
}

namespace CodeRunner
{
    class LuaRunController
    {
        static int lua_print_impl(lua_State *L);
        static int lua_io_read_impl(lua_State *L);
        static void lua_get_traceback(lua_State *L, char *traceback, size_t traceback_len, size_t depth = 2);

        static lua_State *setup_lua();

    public:
        static esp_err_t RunCodeString(const char *code, char *traceback, size_t traceback_len);
        static esp_err_t RunCodeFile(const char *path, char *traceback, size_t traceback_len);
    };
}