add_executable(example_can_parallel_test main.cpp)

target_link_libraries(example_can_parallel_test PRIVATE
    jstm_hal
    jstm_rtos
    jstm_rtcan
)

set_target_properties(example_can_parallel_test PROPERTIES
    SUFFIX ".elf"
    LINK_DEPENDS "${JSTM_LINKER_SCRIPT}"
)

add_custom_command(TARGET example_can_parallel_test POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:example_can_parallel_test>
)

add_custom_command(TARGET example_can_parallel_test POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:example_can_parallel_test>
            ${CMAKE_CURRENT_BINARY_DIR}/example_can_parallel_test.bin
)
