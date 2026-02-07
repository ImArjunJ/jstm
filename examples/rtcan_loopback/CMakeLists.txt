add_executable(example_rtcan_loopback main.cpp)

target_link_libraries(example_rtcan_loopback PRIVATE
    jstm_hal
    jstm_rtos
    jstm_rtcan
)

set_target_properties(example_rtcan_loopback PROPERTIES
    SUFFIX ".elf"
    LINK_DEPENDS "${JSTM_LINKER_SCRIPT}"
)

add_custom_command(TARGET example_rtcan_loopback POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:example_rtcan_loopback>
)

add_custom_command(TARGET example_rtcan_loopback POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:example_rtcan_loopback>
            ${CMAKE_CURRENT_BINARY_DIR}/example_rtcan_loopback.bin
)
