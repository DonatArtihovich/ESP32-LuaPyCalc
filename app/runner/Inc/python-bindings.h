#pragma once

#include "py/compile.h"
#include "py/runtime.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/stackctrl.h"
#include "esp_cpu.h"
#include "py/stackctrl.h"
#include "mpthreadport.h"
#include "py/obj.h"

#ifdef __cplusplus
extern "C"
{
#endif
    mp_obj_t upy_print_impl(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);
    mp_obj_t upy_input_impl(size_t n_args, const mp_obj_t *pos_args);

    extern const mp_obj_fun_builtin_var_t micropython_print_obj;
    extern const mp_obj_fun_builtin_var_t micropython_input_obj;

#ifdef __cplusplus
}
#endif