cmake_minimum_required( VERSION 3.6 )

project( ktbs56a )

enable_language( ASM )
enable_language( C )
enable_language( CXX )

set( CMAKE_SYSTEM_NAME "Generic" CACHE INTERNAL "CMake system name" )
set( CMAKE_SYSTEM_PROCESSOR "ARM-cortex-m3" CACHE INTERNAL "CMake processor" )
set( CMAKE_VERBOSE_MAKEFILE OFF CACHE INTERNAL "CMake verbose makefile" )

set( CMAKE_ASM_COMPILER "/usr/bin/arm-none-eabi-as" CACHE INTERNAL "CMake asm compiler" )
set( CMAKE_C_COMPILER "/usr/bin/arm-none-eabi-gcc" CACHE INTERNAL "CMake c compiler" )
set( CMAKE_CXX_COMPILER "/usr/bin/arm-none-eabi-g++" CACHE INTERNAL "CMake cxx compiler" )
set( CMAKE_OBJCOPY_UTIL "/usr/bin/arm-none-eabi-objcopy" CACHE INTERNAL "CMake objcopy util" )
set( CMAKE_OBJDUMP_UTIL "/usr/bin/arm-none-eabi-objdump" CACHE INTERNAL "CMake objdump util" )
set( CMAKE_SIZE_UTIL "/usr/bin/arm-none-eabi-size" CACHE INTERNAL "CMake size util" )
set( STLINK_INFO_UTIL "/usr/local/bin/st-info" CACHE INTERNAL "Stlink info util" )
set( STLINK_FLASH_UTIL "/usr/local/bin/st-flash" CACHE INTERNAL "Stlink flash util" )
set( CMAKE_GDB_BACKEND "/usr/bin/arm-none-eabi-gdb" CACHE INTERNAL "CMake gdb backend" )
set( CMAKE_GDB_SERVER "/usr/local/bin/st-util" CACHE INTERNAL "CMake gdb server" )
set( CMAKE_GDB_FRONTEND "/usr/bin/cgdb" CACHE INTERNAL "CMake gdb frontend" )

message( "CMake ASM compiler is ${CMAKE_ASM_COMPILER}" )
message( "CMake C compiler is ${CMAKE_C_COMPILER}" )
message( "CMake CXX compiler is ${CMAKE_CXX_COMPILER}" )

set( CMAKE_ASM_FLAGS "-mthumb -mcpu=cortex-m3" CACHE INTERNAL "CMake asm flags" )
set( CMAKE_C_FLAGS "-O0 -pipe -mthumb -mcpu=cortex-m3 -std=gnu11 -ffunction-sections -fomit-frame-pointer -fdata-sections -fno-builtin -Wl,--gc-sections" CACHE INTERNAL "CMake c flags" )
set( CMAKE_CXX_FLAGS "-O0 -pipe -mthumb -mcpu=cortex-m3 -std=c++11 -flto -ffunction-sections -fomit-frame-pointer -fdata-sections -fno-builtin -Wl,--gc-sections" CACHE INTERNAL "CMake cxx flags" )
set( CMAKE_EXE_LINKER_FLAGS "-O0 -pipe -mthumb -mcpu=cortex-m3 -flto -ffunction-sections -fomit-frame-pointer -fdata-sections -fno-builtin -Wl,--gc-sections -Wl,--no-wchar-size-warning --specs=nano.specs --specs=nosys.specs -Wl,-Map,${CMAKE_PROJECT_NAME}.map -u _printf_float -u _scanf_float" CACHE INTERNAL "CMake exe linker flags" )

set( LINKER_SCRIPT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ldscripts )
set( LDMEM_SCRIPT ${LINKER_SCRIPT_DIR}/mem.ld )
set( LDLIB_SCRIPT ${LINKER_SCRIPT_DIR}/libs.ld )
set( LDSECTIONS_SCRIPT ${LINKER_SCRIPT_DIR}/sections.ld )
set( LDSCRIPT ${LINKER_SCRIPT_DIR}/stm32f103rg.ld )

set( HAL_DRIVER "STDPERIPH" CACHE INTERNAL "HAL driver name" )

set( SDK_PATH "/home/user/Documents/SDK/embedded/stm32/stm32f10x" CACHE INTERNAL "SDK path" )
set( SDK_CMSIS_PATH "${SDK_PATH}/cmsis" CACHE INTERNAL "SDK CMSIS path" )
set( SDK_SPL_PATH "${SDK_PATH}/spl" CACHE INTERNAL "SDK SPL path" )
set( SDK_HAL_PATH "${SDK_PATH}/hal" CACHE INTERNAL "SDK HAL path" )
set( SDK_RTOS_PATH "/home/user/Documents/SDK/freertos" CACHE INTERNAL "SDK RTOS path" )

set( SDK_CMSIS_INC_DIR "${SDK_CMSIS_PATH}/inc" CACHE INTERNAL "SDK CMSIS include path" )
set( SDK_SPL_INC_DIR "${SDK_SPL_PATH}/inc" CACHE INTERNAL "SDK CMSIS include path" )
set( SDK_HAL_INC_DIR "${SDK_HAL_PATH}/inc" CACHE INTERNAL "SDK CMSIS include path" )
set( SDK_RTOS_INC_DIR "${SDK_RTOS_PATH}/inc" CACHE INTERNAL "SDK RTOS include path" )

file( GLOB SDK_CMSIS_SOURCES "${SDK_CMSIS_PATH}/src/*.c" "${SDK_CMSIS_PATH}/src/*.cpp" "${SDK_CMSIS_PATH}/src/*.cxx" "${SDK_CMSIS_PATH}/src/*.s" CACHE INTERNAL "SDK CMSIS sources" )
file( GLOB SDK_SPL_SOURCES "${SDK_SPL_PATH}/src/*.c" "${SDK_SPL_PATH}/src/*.cpp" "${SDK_SPL_PATH}/src/*.cxx" "${SDK_SPL_PATH}/src/*.s" CACHE INTERNAL "SDK SPL sources" )
file( GLOB SDK_HAL_SOURCES "${SDK_HAL_PATH}/src/*.c" "${SDK_HAL_PATH}/src/*.cpp" "${SDK_HAL_PATH}/src/*.cxx" "${SDK_HAL_PATH}/src/*.s" CACHE INTERNAL "SDK HAL sources" )
file( GLOB SDK_RTOS_SOURCES "${SDK_RTOS_PATH}/src/*.c" "${SDK_RTOS_PATH}/src/*.cpp" "${SDK_RTOS_PATH}/src/*.cxx" "${SDK_RTOS_PATH}/src/*.s" CACHE INTERNAL "SDK RTOS sources" )

set( PROJECT_INC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/inc" CACHE INTERNAL "Project include path" )
set( PROJECT_SYS_INC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/system/inc" CACHE INTERNAL "Project system include path" )
file( GLOB PROJECT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.s" CACHE INTERNAL "Project sources" )
file( GLOB PROJECT_SYSTEM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/system/src/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/system/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/system/src/*.cxx" "${CMAKE_CURRENT_SOURCE_DIR}/system/src/*.s" CACHE INTERNAL "Project system sources" )

if(( HAL_DRIVER STREQUAL "HAL" ) OR ( HAL_DRIVER STREQUAL "FULL_LL" ))

  set( HSE_VALUE 8000000 )
  set( MCU_DEVICE STM32F103xG )
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D${MCU_DEVICE} -DHSE_VALUE=${HSE_VALUE} -DUSE_${HAL_DRIVER}_DRIVER" CACHE INTERNAL "CMake c flags" )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D${MCU_DEVICE} -DHSE_VALUE=${HSE_VALUE} -DUSE_${HAL_DRIVER}_DRIVER" CACHE INTERNAL "CMake cxx flags" )
  set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -D${MCU_DEVICE} -DHSE_VALUE=${HSE_VALUE} -DUSE_${HAL_DRIVER}_DRIVER -T${LDMEM_SCRIPT} -T${LDLIB_SCRIPT} -T${LDSECTIONS_SCRIPT}" CACHE INTERNAL "CMake exe linker flags" )
  include_directories( ${PROJECT_INC_DIR} ${PROJECT_SYS_INC_DIR} ${SDK_CMSIS_INC_DIR} ${SDK_HAL_INC_DIR} ${SDK_RTOS_INC_DIR} )
  add_executable( ${CMAKE_PROJECT_NAME}.elf ${PROJECT_SOURCES} ${PROJECT_SYSTEM_SOURCES} ${SDK_CMSIS_SOURCES} ${SDK_HAL_SOURCES} ${SDK_RTOS_SOURCES} )

elseif((( HAL_DRIVER STREQUAL "STDPERIPH" ) OR ( HAL_DRIVER STREQUAL "SPL" )))

  set( HSE_VALUE 8000000 )
  set( MCU_DEVICE STM32F10X_XL )
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D${MCU_DEVICE} -DHSE_VALUE=${HSE_VALUE} -DUSE_${HAL_DRIVER}_DRIVER" CACHE INTERNAL "CMake c flags" )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D${MCU_DEVICE} -DHSE_VALUE=${HSE_VALUE} -DUSE_${HAL_DRIVER}_DRIVER" CACHE INTERNAL "CMake cxx flags" )
  set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -D${MCU_DEVICE} -DHSE_VALUE=${HSE_VALUE} -DUSE_${HAL_DRIVER}_DRIVER -T${LDSCRIPT}" CACHE INTERNAL "CMake exe linker flags" )
  include_directories( ${PROJECT_INC_DIR} ${PROJECT_SYS_INC_DIR} ${SDK_CMSIS_INC_DIR} ${SDK_SPL_INC_DIR} ${SDK_RTOS_INC_DIR} )
  add_executable( ${CMAKE_PROJECT_NAME}.elf ${PROJECT_SOURCES} ${PROJECT_SYSTEM_SOURCES} ${SDK_CMSIS_SOURCES} ${SDK_SPL_SOURCES} ${SDK_RTOS_SOURCES} )

elseif( HAL_DRIVER STREQUAL "CMSIS" )

  set( HSE_VALUE 8000000 )
  set( MCU_DEVICE STM32F10X_XL )
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D${MCU_DEVICE} -DHSE_VALUE=${HSE_VALUE} -DUSE_${HAL_DRIVER}_DRIVER" CACHE INTERNAL "CMake c flags" )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D${MCU_DEVICE} -DHSE_VALUE=${HSE_VALUE} -DUSE_${HAL_DRIVER}_DRIVER" CACHE INTERNAL "CMake cxx flags" )
  set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -D${MCU_DEVICE} -DHSE_VALUE=${HSE_VALUE} -DUSE_${HAL_DRIVER}_DRIVER -T${LDMEM_SCRIPT} -T${LDLIB_SCRIPT} -T${LDSECTIONS_SCRIPT}" CACHE INTERNAL "CMake exe linker flags" )
  include_directories( ${PROJECT_INC_DIR} ${PROJECT_SYS_INC_DIR} ${SDK_CMSIS_INC_DIR} ${SDK_RTOS_INC_DIR} )
  add_executable( ${CMAKE_PROJECT_NAME}.elf ${PROJECT_SOURCES} ${PROJECT_SYSTEM_SOURCES} ${SDK_CMSIS_SOURCES} ${SDK_RTOS_SOURCES} )
  
endif(( HAL_DRIVER STREQUAL "HAL" ) OR ( HAL_DRIVER STREQUAL "FULL_LL" ))

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/cmake/clang_complete.in ${CMAKE_CURRENT_SOURCE_DIR}/.clang_complete )

add_custom_command( TARGET ${CMAKE_PROJECT_NAME}.elf POST_BUILD COMMAND ${CMAKE_OBJCOPY_UTIL} ARGS -Oihex ${CMAKE_PROJECT_NAME}.elf ${CMAKE_PROJECT_NAME}.hex )
add_custom_command( TARGET ${CMAKE_PROJECT_NAME}.elf POST_BUILD COMMAND ${CMAKE_OBJCOPY_UTIL} ARGS -Obinary ${CMAKE_PROJECT_NAME}.elf ${CMAKE_PROJECT_NAME}.bin )
add_custom_command( TARGET ${CMAKE_PROJECT_NAME}.elf POST_BUILD COMMAND ${CMAKE_SIZE_UTIL} ${CMAKE_PROJECT_NAME}.elf )
add_custom_command( TARGET ${CMAKE_PROJECT_NAME}.elf POST_BUILD COMMAND ${CMAKE_SIZE_UTIL} ${CMAKE_PROJECT_NAME}.hex )

add_custom_target( flash COMMAND ${STLINK_FLASH_UTIL} write ${CMAKE_PROJECT_NAME}.bin 0x08000000 )
add_custom_target( dump COMMAND ${CMAKE_OBJDUMP_UTIL} -t ${CMAKE_PROJECT_NAME}.elf )
add_custom_target( size COMMAND ${CMAKE_SIZE_UTIL} ${CMAKE_PROJECT_NAME}.elf )
