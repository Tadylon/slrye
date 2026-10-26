add_library(slrye_plugin_api INTERFACE)
target_include_directories(slrye_plugin_api INTERFACE
    ${PROJECT_SOURCE_DIR}/slrye
)
target_compile_features(slrye_plugin_api INTERFACE cxx_std_20)

add_subdirectory(demo_plugin)
