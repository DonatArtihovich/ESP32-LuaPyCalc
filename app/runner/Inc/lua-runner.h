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
    public:
        static esp_err_t RunCodeString(const char *code);
    };
}