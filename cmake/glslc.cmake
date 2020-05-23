find_program(GLSLC glslc)

function(add_shader TARGET_NAME SHADER DESTINATION)
    add_custom_target(${TARGET_NAME} ALL
        COMMAND ${GLSLC} -o ${CMAKE_CURRENT_BINARY_DIR}/${DESTINATION} ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}
        VERBATIM
    )
endfunction()