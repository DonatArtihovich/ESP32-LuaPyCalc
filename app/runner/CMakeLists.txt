idf_component_register(
    SRCS "./Src/runner.cpp" "./Src/lua-runner.cpp" "./Src/python-runner.cpp"
    INCLUDE_DIRS "Inc"
    REQUIRES "lua" "micropython-helper"
)