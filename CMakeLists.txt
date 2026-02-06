cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(jstm C CXX ASM)

  set(JSTM_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/STM32F746ZGTx_FLASH.ld"
      CACHE FILEPATH "")

  add_link_options(-T${JSTM_LINKER_SCRIPT} -Wl,-Map=output.map -Wl,--gc-sections)
endif()

include(FetchContent)

if(NOT TARGET stm32_hal)
  FetchContent_Declare(
    cmsis_core
    GIT_REPOSITORY https://github.com/STMicroelectronics/cmsis_core.git
    GIT_TAG        v5.6.0
    GIT_SHALLOW    TRUE
  )

  FetchContent_Declare(
    cmsis_device_f7
    GIT_REPOSITORY https://github.com/STMicroelectronics/cmsis_device_f7.git
    GIT_TAG        v1.2.8
    GIT_SHALLOW    TRUE
  )

  FetchContent_Declare(
    stm32f7xx_hal_driver
    GIT_REPOSITORY https://github.com/STMicroelectronics/stm32f7xx_hal_driver.git
    GIT_TAG        v1.3.1
    GIT_SHALLOW    TRUE
  )

  FetchContent_MakeAvailable(cmsis_core cmsis_device_f7 stm32f7xx_hal_driver)

  add_library(cmsis_f7 INTERFACE)
  target_include_directories(cmsis_f7 INTERFACE
    ${cmsis_core_SOURCE_DIR}/Include
    ${cmsis_device_f7_SOURCE_DIR}/Include
  )
  target_compile_definitions(cmsis_f7 INTERFACE
    STM32F746xx
    USE_HAL_DRIVER
  )

  file(GLOB HAL_SOURCES "${stm32f7xx_hal_driver_SOURCE_DIR}/Src/*.c")
  list(FILTER HAL_SOURCES EXCLUDE REGEX ".*_template\\.c$")

  add_library(stm32_hal STATIC ${HAL_SOURCES})
  target_include_directories(stm32_hal PUBLIC
    ${stm32f7xx_hal_driver_SOURCE_DIR}/Inc
  )
  target_link_libraries(stm32_hal PUBLIC cmsis_f7)
  target_include_directories(stm32_hal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/hal/config
  )

  add_library(stm32_startup STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/hal/startup/startup_stm32f746xx.s
    ${CMAKE_CURRENT_SOURCE_DIR}/hal/startup/system_stm32f7xx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hal/startup/syscalls.c
    ${CMAKE_CURRENT_SOURCE_DIR}/hal/startup/stm32f7xx_it.c
  )
  target_link_libraries(stm32_startup PUBLIC stm32_hal)
  target_include_directories(stm32_startup PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/hal/startup
  )
endif()

option(JSTM_ENABLE_EXAMPLES "Build example programs" OFF)

add_subdirectory(core)
add_subdirectory(hal)
add_subdirectory(graphics)
add_subdirectory(drivers)

if(JSTM_ENABLE_EXAMPLES)
  add_subdirectory(examples)
endif()
