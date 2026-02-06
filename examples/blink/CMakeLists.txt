add_executable(example_blink main.cpp)

target_link_libraries(example_blink PRIVATE
    jstm_hal
)

set_target_properties(example_blink PROPERTIES
    SUFFIX ".elf"
    LINK_DEPENDS "${JSTM_LINKER_SCRIPT}"
)

add_custom_command(TARGET example_blink POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:example_blink>
)

add_custom_command(TARGET example_blink POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:example_blink>
            ${CMAKE_CURRENT_BINARY_DIR}/example_blink.bin
)
