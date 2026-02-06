add_executable(example_touch_paint main.cpp)

target_link_libraries(example_touch_paint PRIVATE
    jstm_hal
    jstm_graphics
    jstm_ili9341
    jstm_xpt2046
)

set_target_properties(example_touch_paint PROPERTIES
    SUFFIX ".elf"
    LINK_DEPENDS "${JSTM_LINKER_SCRIPT}"
)

add_custom_command(TARGET example_touch_paint POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:example_touch_paint>
)

add_custom_command(TARGET example_touch_paint POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:example_touch_paint>
            ${CMAKE_CURRENT_BINARY_DIR}/example_touch_paint.bin
)
