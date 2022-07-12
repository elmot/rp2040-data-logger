set(FREERTOS_KERNEL_PATH ${CMAKE_SOURCE_DIR}/FreeRTOS-Kernel CACHE PATH "")
set(FREERTOS_RP2040_PATH ${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040 CACHE PATH "")
set(FREERTOS_CONFIG_FILE_DIRECTORY ${CMAKE_SOURCE_DIR}/src CACHE PATH "Path to FreeRTOSConfig.h directory")
add_library(FreeRTOS-RP2040-Kernel INTERFACE)
target_sources(FreeRTOS-RP2040-Kernel INTERFACE
        ${FREERTOS_KERNEL_PATH}/croutine.c
        ${FREERTOS_KERNEL_PATH}/event_groups.c
        ${FREERTOS_KERNEL_PATH}/list.c
        ${FREERTOS_KERNEL_PATH}/queue.c
        ${FREERTOS_KERNEL_PATH}/stream_buffer.c
        ${FREERTOS_KERNEL_PATH}/tasks.c
        ${FREERTOS_KERNEL_PATH}/timers.c
        ${FREERTOS_RP2040_PATH}/port.c
        ${FREERTOS_RP2040_PATH}/idle_task_static_memory.c
        )
target_include_directories(FreeRTOS-RP2040-Kernel INTERFACE
        ${FREERTOS_KERNEL_PATH}/include
        ${FREERTOS_RP2040_PATH}/include
        )
target_link_libraries(
        FreeRTOS-RP2040-Kernel INTERFACE
        hardware_exception
)
