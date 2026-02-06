add_library(jstm_graphics INTERFACE)

target_include_directories(jstm_graphics INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(jstm_graphics INTERFACE
    jstm_core
)
