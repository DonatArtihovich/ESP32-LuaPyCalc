idf_component_register(
    SRCS "display.cpp" "./display.cpp"
    INCLUDE_DIRS "."
    REQUIRES "st7789v" "spiffs" "app-settings"
)