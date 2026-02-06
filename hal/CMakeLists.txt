add_library(jstm_hal STATIC
    src/system_init.cpp
    src/log_backend.cpp
)

target_include_directories(jstm_hal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(jstm_hal PUBLIC
    jstm_core
    stm32_hal
)

target_link_options(jstm_hal PUBLIC
    -Wl,--whole-archive $<TARGET_FILE:stm32_startup> -Wl,--no-whole-archive
)
add_dependencies(jstm_hal stm32_startup)
