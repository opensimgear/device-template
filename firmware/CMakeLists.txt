cmake_minimum_required(VERSION 3.24)
project(osg_hello LANGUAGES C ASM)
set(OSG_BOARD "starter_g0b1cb" CACHE STRING "Board target")
if(NOT OSG_BOARD STREQUAL "starter_g0b1cb")
  message(FATAL_ERROR "Unknown board: ${OSG_BOARD}; only starter_g0b1cb is implemented")
endif()
set(OSG_USB_VID "" CACHE STRING "Authorized development USB VID, decimal or 0x hex")
set(OSG_USB_PID "" CACHE STRING "Authorized development USB PID, decimal or 0x hex")
foreach(id OSG_USB_VID OSG_USB_PID)
  if(NOT "${${id}}" MATCHES "^(0[xX][0-9a-fA-F]+|[0-9]+)$")
    message(FATAL_ERROR "Set ${id} to an identifier you are authorized to use; no public allocation exists yet")
  endif()
  math(EXPR id_value "${${id}}")
  if(id_value LESS 1 OR id_value GREATER 65535)
    message(FATAL_ERROR "${id} must be in 1..65535")
  endif()
endforeach()
include(cmake/dependencies.cmake)
set(HAL ${OSG_STM32_HAL_DIR})
set(CMSIS ${OSG_STM32_CMSIS_DIR})
add_library(osg_hal STATIC
  ${HAL}/Src/stm32g0xx_hal.c ${HAL}/Src/stm32g0xx_hal_cortex.c
  ${HAL}/Src/stm32g0xx_hal_rcc.c ${HAL}/Src/stm32g0xx_hal_rcc_ex.c
  ${HAL}/Src/stm32g0xx_hal_pwr.c ${HAL}/Src/stm32g0xx_hal_pwr_ex.c
  ${HAL}/Src/stm32g0xx_hal_dma.c
  ${HAL}/Src/stm32g0xx_hal_adc.c ${HAL}/Src/stm32g0xx_hal_adc_ex.c
  ${HAL}/Src/stm32g0xx_hal_gpio.c ${HAL}/Src/stm32g0xx_hal_fdcan.c
  ${CMSIS}/Source/Templates/system_stm32g0xx.c)
target_include_directories(osg_hal PUBLIC include ${HAL}/Inc ${CMSIS}/Include ${OSG_CMSIS_CORE_DIR}/CMSIS/Core/Include)
target_compile_definitions(osg_hal PUBLIC STM32G0B1xx USE_HAL_DRIVER)
add_executable(osg_hello src/main.c src/profile.c src/usb_descriptors.c boards/starter_g0b1cb/board.c
  ${CMSIS}/Source/Templates/gcc/startup_stm32g0b1xx.s)
target_include_directories(osg_hello PRIVATE include boards/starter_g0b1cb)
target_compile_definitions(osg_hello PRIVATE OSG_USB_VID=${OSG_USB_VID} OSG_USB_PID=${OSG_USB_PID})
target_link_libraries(osg_hello PRIVATE osg_core osg_usb osg_hal osg_canard)
target_compile_options(osg_hello PRIVATE -Wall -Wextra -Werror)
foreach(target osg_hello osg_hal osg_core osg_usb osg_tinyusb osg_canard)
  target_compile_options(${target} PRIVATE -Os -ffunction-sections -fdata-sections)
endforeach()
target_link_options(osg_hello PRIVATE -T${CMAKE_CURRENT_SOURCE_DIR}/boards/starter_g0b1cb/flash.ld
  -nostartfiles --specs=nano.specs --specs=nosys.specs
  -Wl,--gc-sections,-Map=${CMAKE_CURRENT_BINARY_DIR}/osg_hello.map -Wl,--print-memory-usage)
set_target_properties(osg_hello PROPERTIES SUFFIX .elf)
add_custom_command(TARGET osg_hello POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:osg_hello> ${CMAKE_CURRENT_BINARY_DIR}/osg_hello.bin
  COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:osg_hello> ${CMAKE_CURRENT_BINARY_DIR}/osg_hello.hex
  COMMAND ${CMAKE_SIZE} $<TARGET_FILE:osg_hello>)
