#pragma once

#include "runner.h"

extern "C"
{
#include "python-bindings.h"
}

namespace CodeRunner
{
    class PythonRunController
    {
        // static int python_input_impl();
        // static void python_get_traceback(char *traceback, size_t traceback_len, size_t depth = 2);

        static void setup_python();

    public:
        static esp_err_t RunCodeString(const char *code, char *traceback, size_t traceback_len);
        static esp_err_t RunCodeFile(const char *path, char *traceback, size_t traceback_len);
        static const char *upy_obj_to_string(mp_obj_t obj);
    };
}