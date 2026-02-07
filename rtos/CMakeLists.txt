add_library(jstm_rtos STATIC
    src/hal_timebase_tim.c
    src/rtos_hooks.cpp
)

target_include_directories(jstm_rtos PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/config
)

target_link_libraries(jstm_rtos PUBLIC
    jstm_core
    stm32_hal
    freertos_kernel
)

target_compile_definitions(jstm_rtos PUBLIC
    JSTM_USE_FREERTOS=1
)

# Force the linker to pull in FreeRTOS hook symbols from this static library.
# Without this, the linker discards them before seeing that freertos_kernel
# needs them, causing undefined-reference errors.
target_link_options(jstm_rtos INTERFACE
    "LINKER:--undefined=vApplicationMallocFailedHook"
    "LINKER:--undefined=vApplicationStackOverflowHook"
    "LINKER:--undefined=vApplicationGetIdleTaskMemory"
    "LINKER:--undefined=vApplicationGetTimerTaskMemory"
    "LINKER:--undefined=SysTick_Handler"
)
