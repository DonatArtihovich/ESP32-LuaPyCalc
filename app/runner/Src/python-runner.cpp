#include "python-runner.h"

static const char *TAG = "PythonRunController";

#define HEAP_SIZE (16 * 1024) // 16 KB heap, adjust as needed
static char heap[HEAP_SIZE] = {0};

using CodeRunner::PythonRunController, CodeRunner::CodeRunController;

extern "C" mp_obj_t upy_print_impl(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    const char *sep = " ";
    const char *end = "\n";

    if (kw_args != NULL)
    {
        for (size_t i{}; i < kw_args->used; i++)
        {
            mp_map_elem_t *elem = &kw_args->table[i];
            if (mp_obj_is_qstr(elem->key))
            {
                qstr key = mp_obj_str_get_qstr(elem->key);
                if (key == MP_QSTR_sep)
                {
                    sep = mp_obj_str_get_str(elem->value);
                }
                else if (key == MP_QSTR_end)
                {
                    end = mp_obj_str_get_str(elem->value);
                }
            }
        }
    }

    for (size_t i = 0; i < n_args; i++)
    {
        if (i > 0)
        {
            CodeRunController::SetIsWaitingOutput(true);
            xQueueSend(xQueueRunnerStdout, sep, portMAX_DELAY);
        }

        CodeRunController::SetIsWaitingOutput(true);
        if (mp_obj_is_str(pos_args[i]))
        {
            xQueueSend(xQueueRunnerStdout, mp_obj_str_get_str(pos_args[i]), portMAX_DELAY);
        }
        else
        {
            xQueueSend(xQueueRunnerStdout, PythonRunController::upy_obj_to_string(pos_args[i]), portMAX_DELAY);
        }
    }

    CodeRunController::SetIsWaitingOutput(true);
    xQueueSend(xQueueRunnerStdout, end, portMAX_DELAY);

    return mp_const_none;
}

extern "C" mp_obj_t upy_input_impl(size_t n_args, const mp_obj_t *pos_args)
{
    const char *prompt = "";

    if (n_args > 0)
    {
        if (!mp_obj_is_str(pos_args[0]))
        {
            mp_raise_TypeError("string expected");
        }

        prompt = mp_obj_str_get_str(pos_args[0]);
    }

    if (prompt[0] != '\0')
    {
        CodeRunController::SetIsWaitingOutput(true);
        xQueueSend(xQueueRunnerStdout, prompt, portMAX_DELAY);
    }

    while (CodeRunController::IsWaitingOutput())
    {
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    char *buf = m_new(char, 256);

    CodeRunController::SetIsWaitingInput(true);
    xSemaphoreGive(xDisplayingSemaphore);

    size_t index{};
    char c{};
    while (index < 256 && xQueueReceive(xQueueRunnerStdin, &c, portMAX_DELAY) == pdPASS && c != '\n')
    {
        if (c == '\b')
        {
            if (index > 0)
            {
                index--;
            }
        }
        else
        {
            buf[index++] = c;
        }
    }
    buf[index] = '\0';

    ESP_LOGI(TAG, "Upy input return \"%s\"", mp_obj_str_get_str(mp_obj_new_str(buf, index)));
    CodeRunController::SetIsWaitingInput(false);
    return mp_obj_new_str(buf, index);
}

namespace CodeRunner
{
    void PythonRunController::setup_python()
    {
        mp_thread_init(pxTaskGetStackStart(NULL), 10240 / sizeof(uintptr_t));
        volatile int stack_dummy;
        mp_stack_set_top((void *)&stack_dummy);
        mp_stack_ctrl_init();
        mp_stack_set_limit(10240);
        gc_init(heap, heap + HEAP_SIZE);
        mp_init();

        mp_obj_dict_t *mp_globals = mp_globals_get();
        mp_obj_dict_t *mp_locals = mp_locals_get();
        if (!mp_globals)
        {
            mp_globals = (mp_obj_dict_t *)MP_OBJ_TO_PTR(mp_obj_new_dict(0));
        }

        if (!mp_locals)
        {
            mp_locals = (mp_obj_dict_t *)MP_OBJ_TO_PTR(mp_obj_new_dict(0));
        }

        if (mp_locals)
        {
            mp_locals_set((mp_obj_dict_t *)MP_OBJ_TO_PTR(mp_locals));
        }
        else
        {
            ESP_LOGE(TAG, "mp_locals is null");
        }

        if (mp_globals)
        {
            mp_obj_dict_store(mp_globals, MP_OBJ_NEW_QSTR(MP_QSTR_print), (mp_obj_t)&micropython_print_obj);
            mp_obj_dict_store(mp_globals, MP_OBJ_NEW_QSTR(MP_QSTR_input), (mp_obj_t)&micropython_input_obj);
            mp_globals_set((mp_obj_dict_t *)MP_OBJ_TO_PTR(mp_globals));
        }
        else
        {
            ESP_LOGE(TAG, "mp_globals is null");
        }
    }

    esp_err_t PythonRunController::RunCodeString(const char *code, char *traceback, size_t traceback_len)
    {
        esp_err_t ret{ESP_OK};
        setup_python();

        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0)
        {
            mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, code, strlen(code), 0);
            mp_parse_tree_t parse_tree = mp_parse(lex, MP_PARSE_FILE_INPUT);
            mp_obj_t module_fun = mp_compile(&parse_tree, lex->source_name, true);

            mp_call_function_0(module_fun);
            nlr_pop();
        }
        else
        {
            upy_get_traceback(&nlr, traceback, traceback_len);
            ret = ESP_FAIL;
        }

        mp_deinit();
        gc_sweep_all();
        return ret;
    }

    esp_err_t PythonRunController::RunCodeFile(const char *code, char *traceback, size_t traceback_len)
    {
        esp_err_t ret{ESP_OK};
        ESP_LOGI(TAG, "Run Python File: %s", code);
        setup_python();

        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0)
        {
            mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, code, strlen(code), 0);
            mp_parse_tree_t parse_tree = mp_parse(lex, MP_PARSE_FILE_INPUT);
            mp_obj_t module_fun = mp_compile(&parse_tree, lex->source_name, true);

            mp_call_function_0(module_fun);
            nlr_pop();
        }
        else
        {
            upy_get_traceback(&nlr, traceback, traceback_len);
            ret = ESP_FAIL;
        }

        mp_deinit();
        gc_sweep_all();
        return ret;
    }

    struct TracebackBuffer
    {
        char *buffer;
        size_t max;
    };

    void PythonRunController::upy_get_traceback(nlr_buf_t *nlr, char *traceback, size_t traceback_len)
    {
        mp_obj_t exc = (mp_obj_t)nlr->ret_val;

        size_t n;
        size_t *items;
        mp_obj_exception_get_traceback(exc, &n, &items);

        if (n == 0 || items == NULL)
        {
            snprintf(traceback, traceback_len, "%s", "Python Error.\n");
            return;
        }

        snprintf(traceback, traceback_len, "%s", "Traceback (most recent call last):\n");

        char line_buf[36] = {0};
        for (size_t i = 0; i < n; i += 3)
        {
            const char *file = qstr_str(items[i]);
            size_t line = items[i + 1];

            snprintf(line_buf, sizeof(line_buf), "  File \"%s\", line %zu\n", file, line);
            strncat(traceback, line_buf, traceback_len);
            memset(line_buf, 0, sizeof(line_buf));
        }

        TracebackBuffer tb = {
            .buffer = traceback + strlen(traceback),
            .max = traceback_len,
        };

        mp_print_t print;
        print.data = &tb;
        print.print_strn = [](void *data, const char *str, size_t len)
        {
            TracebackBuffer *tb = static_cast<TracebackBuffer *>(data);
            strncat(tb->buffer, str, tb->max - strlen(tb->buffer));
        };

        mp_obj_exception_print(&print, exc, PRINT_EXC);
    }

    const char *PythonRunController::upy_obj_to_string(mp_obj_t obj)
    {
        static char buffer[128] = {0};

        mp_print_t print;
        print.data = buffer;
        print.print_strn = [](void *data, const char *str, size_t len)
        {
            char *buf = (char *)data;
            strncat(buf, str, 128);
        };

        memset(buffer, 0, sizeof(buffer));

        mp_obj_print_helper(&print, obj, PRINT_STR);
        return buffer;
    }
}