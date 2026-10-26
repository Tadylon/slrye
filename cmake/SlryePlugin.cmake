function(slrye_add_plugin target plugin_id)
    add_library(${target} MODULE ${ARGN})

    target_link_libraries(${target} PRIVATE slrye_plugin_api)
    target_compile_definitions(${target} PRIVATE SLRYE_PLUGIN_BUILD)

    set_target_properties(${target} PROPERTIES
        PREFIX ""
        OUTPUT_NAME "${plugin_id}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/plugins/${plugin_id}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/plugins/${plugin_id}"
    )

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plugin.yaml")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_CURRENT_SOURCE_DIR}/plugin.yaml"
                    "$<TARGET_FILE_DIR:${target}>/plugin.yaml"
            COMMENT "Copying plugin manifest for ${plugin_id}"
        )
    endif()
endfunction()
