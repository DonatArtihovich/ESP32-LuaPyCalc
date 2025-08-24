#include "python-runner.h"
#include "esp_cpu.h"
#include "py/stackctrl.h"
#include "mpthreadport.h"
#include "unistd.h"

static const char *TAG = "PythonRunController";

#define HEAP_SIZE (16 * 1024) // 16 KB heap, adjust as needed
static char heap[HEAP_SIZE] = {0};

namespace CodeRunner
{

    esp_err_t PythonRunController::RunCodeString(const char *code, char *traceback, size_t traceback_len)
    {
        esp_err_t ret{ESP_OK};

        mp_thread_init(pxTaskGetStackStart(NULL), 10240 / sizeof(uintptr_t));
        volatile int stack_dummy;
        mp_stack_set_top((void *)&stack_dummy);
        mp_stack_set_limit(10240);
        mp_stack_ctrl_init();
        mp_stack_set_limit(10240);
        gc_init(heap, heap + HEAP_SIZE);
        mp_init();
        mp_stack_ctrl_init();

        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0)
        {
            mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, code, strlen(code), 0);
            qstr source_name = lex->source_name;
            mp_parse_tree_t parse_tree = mp_parse(lex, MP_PARSE_FILE_INPUT);
            mp_obj_t module_fun = mp_compile(&parse_tree, source_name, true);

            mp_call_function_0(module_fun);
            nlr_pop();
        }
        else
        {
            mp_obj_t exc = (mp_obj_t)nlr.ret_val;

            size_t n;
            size_t *items;
            mp_obj_exception_get_traceback(exc, &n, &items);

            if (n == 0 || items == NULL)
            {
                ESP_LOGE(TAG, "Python Error");
                snprintf(traceback, traceback_len, "%s", "Python Error");
                return ESP_FAIL;
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
        }

        mp_deinit();
        gc_sweep_all();

        ESP_LOGI(TAG, "Python Run FInished");
        return ret;
    }

    esp_err_t PythonRunController::RunCodeFile(const char *code, char *traceback, size_t traceback_len)
    {
        esp_err_t ret{ESP_OK};
        ESP_LOGI(TAG, "Run Python File: %s", code);

        return ret;
    }
}