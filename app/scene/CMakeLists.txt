idf_component_register(
    SRCS "./Src/scene.cpp" "./Src/start-scene.cpp" "./Src/files-scene.cpp" "./Src/code-scene.cpp"
    INCLUDE_DIRS "." "./Inc"
    REQUIRES "display" "sdcard" "keyboard" "runner"
)