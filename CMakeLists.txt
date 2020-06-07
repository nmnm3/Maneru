cmake_minimum_required(VERSION 3.10)

project(Maneru)

if(NOT COLD_CLEAR_ROOT)
    message(FATAL_ERROR "Missing COLD_CLEAR_ROOT variable, git clone from https://github.com/MinusKelvin/cold-clear")
endif()

if(NOT EXISTS ${COLD_CLEAR_ROOT}/c-api/coldclear.h)
    message(FATAL_ERROR "Missing ${COLD_CLEAR_ROOT}/c-api/coldclear.h")
endif()

set(COLD_CLEAR_RELEASE ${COLD_CLEAR_ROOT}/target/release)
if(NOT EXISTS ${COLD_CLEAR_RELEASE}/cold_clear.dll.lib OR NOT EXISTS ${COLD_CLEAR_RELEASE}/cold_clear.dll)
    message(FATAL_ERROR "Missing lib or dll, build cold-clear first")
endif()

if(NOT SDL2_ROOT)
    message(FATAL_ERROR "Missing SDL2_ROOT variable, download Development Libraries from https://www.libsdl.org/download-2.0.php")
endif()

include_directories(${COLD_CLEAR_ROOT}/c-api ${SDL2_ROOT}/include)
link_directories(${COLD_CLEAR_RELEASE} ${SDL2_ROOT}/lib/x64)
add_definitions("/D UNICODE /MP /wd4244 /wd4018 /wd4267")

file(GLOB MANERU_SRC_FILES
    "*.h"
    "*.cpp"
)
add_executable(Maneru WIN32
    ${MANERU_SRC_FILES}
    Maneru.rc)

add_custom_command(TARGET Maneru POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${COLD_CLEAR_RELEASE}/cold_clear.dll $<TARGET_FILE_DIR:Maneru>
)

add_custom_command(TARGET Maneru POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SDL2_ROOT}/lib/x64/SDL2.dll $<TARGET_FILE_DIR:Maneru>
)

add_custom_command(TARGET Maneru POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SDL2_ROOT}/README-SDL.txt $<TARGET_FILE_DIR:Maneru>
)

add_custom_command(TARGET Maneru POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/maneru.config $<TARGET_FILE_DIR:Maneru>
)