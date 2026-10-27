find_package(Catch2 3 CONFIG REQUIRED)
include(Catch)

function(slrye_add_test target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE
        slrye_runtime
        Catch2::Catch2WithMain
    )
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    catch_discover_tests(${target})
endfunction()

slrye_add_test(slrye_app_tests app/launch_options_test.cpp)
slrye_add_test(slrye_core_tests core/imgui_backend_test.cpp)
slrye_add_test(slrye_domain_tests domain/entity_id_test.cpp)
slrye_add_test(slrye_scene_tests domain/scene_test.cpp)
slrye_add_test(slrye_camera_tests domain/camera_test.cpp)
slrye_add_test(slrye_scene_transform_tests domain/scene_transform_test.cpp)
slrye_add_test(slrye_platform_tests platform/paths_test.cpp)
slrye_add_test(slrye_file_io_tests platform/file_io_test.cpp)
slrye_add_test(slrye_services_tests services/plugin_manifest_test.cpp)
slrye_add_test(slrye_project_file_tests services/project_file_test.cpp)
slrye_add_test(slrye_recent_projects_tests services/recent_projects_test.cpp)
slrye_add_test(slrye_editor_settings_tests services/editor_settings_test.cpp)
slrye_add_test(slrye_scene_serializer_tests services/scene_serializer_test.cpp)
slrye_add_test(slrye_picking_tests services/picking_test.cpp)
slrye_add_test(slrye_scene_geometry_tests services/scene_geometry_test.cpp)
slrye_add_test(slrye_scene_renderer_tests services/scene_renderer_test.cpp)
slrye_add_test(slrye_ui_tests ui/editor_ui_test.cpp)
slrye_add_test(slrye_camera_controller_tests views/camera_controller_test.cpp)
slrye_add_test(slrye_views_tests views/view_test.cpp)

if(TARGET slrye_demo_plugin)
    add_executable(slrye_plugin_integration_tests services/plugin_manager_test.cpp)
    target_link_libraries(slrye_plugin_integration_tests PRIVATE
        slrye_runtime
        Catch2::Catch2WithMain
    )
    target_include_directories(slrye_plugin_integration_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_definitions(slrye_plugin_integration_tests PRIVATE
        SLRYE_DEMO_PLUGIN_DIR="$<TARGET_FILE_DIR:slrye_demo_plugin>"
    )
    add_dependencies(slrye_plugin_integration_tests slrye_demo_plugin)
    catch_discover_tests(slrye_plugin_integration_tests)
endif()
