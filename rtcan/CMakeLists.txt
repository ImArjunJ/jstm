add_library(jstm_rtcan STATIC
    src/rtcan.cpp
)

target_include_directories(jstm_rtcan PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(jstm_rtcan PUBLIC
    jstm_core
    stm32_hal
    jstm_rtos
)
