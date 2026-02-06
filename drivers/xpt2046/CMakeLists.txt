add_library(jstm_xpt2046 STATIC
    src/xpt2046.cpp
)

target_include_directories(jstm_xpt2046 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(jstm_xpt2046 PUBLIC
    jstm_hal
)
