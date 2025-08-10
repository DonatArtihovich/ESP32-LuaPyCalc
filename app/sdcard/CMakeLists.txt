idf_component_register(
    SRCS "./sd.cpp"
    INCLUDE_DIRS "."
    REQUIRES "sdmmc" "fatfs" "driver"
)