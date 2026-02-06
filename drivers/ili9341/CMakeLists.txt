add_library(jstm_ili9341 STATIC
    src/ili9341.cpp
)

target_include_directories(jstm_ili9341 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(jstm_ili9341 PUBLIC
    jstm_hal
)
