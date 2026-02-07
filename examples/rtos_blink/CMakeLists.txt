add_executable(example_rtos_blink main.cpp)

target_link_libraries(example_rtos_blink PRIVATE
    jstm_hal
    jstm_rtos
)

set_target_properties(example_rtos_blink PROPERTIES
    SUFFIX ".elf"
    LINK_DEPENDS "${JSTM_LINKER_SCRIPT}"
)

add_custom_command(TARGET example_rtos_blink POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:example_rtos_blink>
)

add_custom_command(TARGET example_rtos_blink POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:example_rtos_blink>
            ${CMAKE_CURRENT_BINARY_DIR}/example_rtos_blink.bin
)
