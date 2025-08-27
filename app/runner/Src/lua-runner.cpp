#include "lua-runner.h"

static const char *TAG = "LuaRunController";

namespace CodeRunner
{

    lua_State *LuaRunController::setup_lua(const char *module_dir)
    {
        lua_State *L = luaL_newstate();
        luaL_openlibs(L);

        lua_pushcfunction(L, lua_print_impl);
        lua_setglobal(L, "print");

        lua_getglobal(L, "io");
        lua_pushcfunction(L, lua_io_read_impl);
        lua_setfield(L, -2, "read");

        lua_getglobal(L, "package");
        lua_getfield(L, -1, "path");
        const char *current_path = lua_tostring(L, -1);

        lua_pushfstring(L, "%s;%s/?.lua;%s/?.lua", current_path, CONFIG_MOUNT_POINT, module_dir);
        lua_setfield(L, -3, "path");
        lua_pop(L, 2);

        return L;
    }

    esp_err_t LuaRunController::RunCodeString(const char *code, char *traceback, size_t traceback_len)
    {
        esp_err_t ret{ESP_OK};
        ESP_LOGI(TAG, "Run Lua code string...");

        lua_State *L{setup_lua()};
        int lua_ret = LUA_OK;

        if ((lua_ret = luaL_loadstring(L, code)) == LUA_OK)
        {
            lua_ret = lua_pcall(L, 0, LUA_MULTRET, 0);
        }

        if (lua_ret != LUA_OK)
        {
            lua_get_traceback(L, traceback, traceback_len);
        }

        lua_close(L);

        if (lua_ret != LUA_OK)
        {
            ret = ESP_FAIL;
        }

        return ret;
    }

    esp_err_t LuaRunController::RunCodeFile(const char *path, char *traceback, size_t traceback_len)
    {
        esp_err_t ret{ESP_OK};
        ESP_LOGI(TAG, "Run Lua code file %s...", path);

        std::string module_dir = std::string(path).substr(0, std::string(path).find_last_of('/'));
        lua_State *L{setup_lua(module_dir.c_str())};
        int lua_ret = LUA_OK;

        if ((lua_ret = luaL_loadfile(L, path)) == LUA_OK)
        {
            lua_ret = lua_pcall(L, 0, LUA_MULTRET, 0);
        }

        if (lua_ret != LUA_OK)
        {
            lua_get_traceback(L, traceback, traceback_len);
        }

        lua_close(L);

        if (lua_ret != LUA_OK)
        {
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

                size_t log_buffer_len = strlen(log_buffer);

                if (log_buffer_len < 63)
                {
                    log_buffer[log_buffer_len] = i < n ? '\t' : '\n';
                    CodeRunController::SetIsWaitingOutput(true);
                    xQueueSend(xQueueRunnerStdout, log_buffer, portMAX_DELAY);
                }
                else
                {
                    CodeRunController::SetIsWaitingOutput(true);
                    xQueueSend(xQueueRunnerStdout, log_buffer, portMAX_DELAY);
                    CodeRunController::SetIsWaitingOutput(true);
                    xQueueSend(xQueueRunnerStdout, i < n ? "\t" : "\n", portMAX_DELAY);
                }
                memset(log_buffer, 0, sizeof(log_buffer));
            }
        }

        return 0;
    }

    int LuaRunController::lua_io_read_impl(lua_State *L)
    {
        CodeRunController::SetIsWaitingInput(true);

        ESP_LOGI(TAG, "Is waiting input: true");
        xSemaphoreGive(xDisplayingSemaphore);

        int n = lua_gettop(L);
        bool is_num{};
        const char *prompt = "*l";
        int chars_count{255};

        if (n == 1)
        {
            if (lua_type(L, 1) == LUA_TSTRING)
            {
                prompt = lua_tostring(L, 1);
                ESP_LOGI(TAG, "Prompt: %s", prompt);
            }
            else if (lua_type(L, 1) == LUA_TNUMBER)
            {
                is_num = true;
                chars_count = lua_tonumber(L, 1);
                ESP_LOGI(TAG, "Chars number: %d", chars_count);
            }
            else
            {
                return luaL_error(L, "invalid argument type, string expected");
            }
        }

        size_t prompt_len = strlen(prompt);

        if (strncmp(prompt, "*l", prompt_len) &&
            strncmp(prompt, "*L", prompt_len) &&
            strncmp(prompt, "*n", prompt_len) &&
            strncmp(prompt, "*number", prompt_len) &&
            strncmp(prompt, "*a", prompt_len) &&
            strncmp(prompt, "*all", prompt_len))
        {
            return luaL_error(L, "invalid format string");
        }

        ESP_LOGI(TAG, "Lua input argument: %s", prompt);

        char *io_buff = (char *)luaM_malloc_(L, (chars_count + 1) * sizeof(char), LUA_TSTRING);
        memset(io_buff, 0, (chars_count + 1) * sizeof(char));
        char ch = 0;
        size_t index{};
        while (index < chars_count + 1 && xQueueReceive(xQueueRunnerStdin, &ch, portMAX_DELAY) == pdPASS)
        {
            if (!is_num)
            {
                if (strncmp(prompt, "*l", prompt_len) == 0 ||
                    strncmp(prompt, "*L", prompt_len) == 0)
                {
                    if (ch == '\n')
                    {
                        break;
                    }
                }
                else if (strncmp(prompt, "*n", prompt_len) == 0 ||
                         strncmp(prompt, "*number", prompt_len) == 0)
                {
                    if (ch == '\n')
                    {
                        bool is_enter_end{};
                        for (char *p{io_buff}; *p != '\0'; p++)
                        {
                            if (!isspace(*p))
                            {
                                is_enter_end = true;
                                break;
                            }
                        }

                        if (is_enter_end)
                        {
                            break;
                        }
                    }
                }
                else if (strncmp(prompt, "*a", prompt_len) == 0 ||
                         strncmp(prompt, "*all", prompt_len) == 0)
                {
                    if (ch == '\4')
                    {
                        break;
                    }
                }
            }

            if (ch == '\b')
            {
                if (index > 0)
                {
                    index--;
                }
            }
            else
            {
                io_buff[index++] = ch;
            }
        }
        io_buff[index] = '\0';

        if (index || ch == '\n')
        {
            if (strncmp(prompt, "*n", prompt_len) != 0)
            {
                lua_pushstring(L, io_buff);
            }
            else
            {
                for (char *p{io_buff}; *p != '\0'; p++)
                {
                    if (!isspace(*p))
                    {
                        if (isdigit(*p))
                        {
                            lua_pushnumber(L, atof(io_buff));
                        }
                        else
                        {
                            lua_pushnil(L);
                        }
                        break;
                    }
                }
            }
        }
        else
        {
            lua_pushnil(L);
        }

        ESP_LOGI(TAG, "Is waiting input: false");
        CodeRunController::SetIsWaitingInput(false);

        return 1;
    }

    void LuaRunController::lua_get_traceback(lua_State *L, char *traceback, size_t traceback_len)
    {
        const char *err = lua_tostring(L, -1);
        if (err && err[0] != '\0')
        {
            snprintf(traceback, traceback_len, err);
        }
        else
        {
            snprintf(traceback, traceback_len, "Undefined Error.");
        }

        lua_pop(L, 1);
    }
}