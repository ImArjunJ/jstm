add_library(jstm_core INTERFACE)

target_include_directories(jstm_core INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(jstm_core INTERFACE cxx_std_23)
