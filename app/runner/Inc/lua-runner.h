#pragma once

#include "runner.h"

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

namespace CodeRunner
{
    class LuaRunController
    {
        static int lua_print_impl(lua_State *L);
        static int lua_io_read_impl(lua_State *L);

    public:
        static esp_err_t RunCodeString(const char *code);
    };
}