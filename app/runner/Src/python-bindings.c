#include "python-bindings.h"

MP_DEFINE_CONST_FUN_OBJ_KW(micropython_print_obj, 0, upy_print_impl);
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(micropython_input_obj, 0, 1, upy_input_impl);
