cmake_minimum_required(VERSION 3.20)

project(ChessEngine
    VERSION 0.1.0
    DESCRIPTION "Chess engine and SDL2 GUI"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(CHESS_BUILD_GUI "Build the SDL2 GUI (needs SDL2, SDL2_image, SDL2_ttf)" ON)
option(CHESS_SANITIZERS "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(CHESS_BUILD_TESTS "Build the test suite (run it with ctest)" ON)


# ---- Shared compiler settings ------------------------------------------------

add_library(chess_options INTERFACE)

if(MSVC)
    target_compile_options(chess_options INTERFACE /W4 /permissive-)
else()
    target_compile_options(chess_options INTERFACE -Wall -Wextra -Wpedantic)
endif()

if(CHESS_SANITIZERS)
    if(MSVC OR MINGW)
        message(FATAL_ERROR "CHESS_SANITIZERS is only supported with GCC/Clang on Linux or macOS")
    endif()
    target_compile_options(chess_options INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(chess_options INTERFACE -fsanitize=address,undefined)
endif()


# ---- Core: board, rules, move generation, perft. No dependency on SDL --------
#
#  Headers are included with their directory ("core/board.h"), so the search can later sit
#  next to it in src/search without name clashes.

add_library(core STATIC
    src/core/board.cpp
    src/core/move.cpp
    src/core/movegen.cpp
    src/core/perft.cpp
    src/core/types.cpp
)
target_include_directories(core PUBLIC src)
target_link_libraries(core PRIVATE chess_options)


# ---- Perft command line tool --------------------------------------------------

add_executable(perft src/perft/main.cpp)
target_link_libraries(perft PRIVATE core chess_options)


# ---- Tests --------------------------------------------------------------------

if(CHESS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()


# ---- GUI ----------------------------------------------------------------------

if(CHESS_BUILD_GUI)
    find_package(SDL2 REQUIRED CONFIG)
    find_package(SDL2_image REQUIRED CONFIG)
    find_package(SDL2_ttf REQUIRED CONFIG)

    add_executable(chess
        src/gui/app.cpp
        src/gui/button.cpp
        src/gui/draw.cpp
        src/gui/game_screen.cpp
        src/gui/main.cpp
        src/gui/menu_screen.cpp
    )

    if(TARGET SDL2::SDL2main)
        target_link_libraries(chess PRIVATE SDL2::SDL2main)
    endif()

    target_link_libraries(chess PRIVATE
        core
        chess_options
        SDL2::SDL2
        SDL2_image::SDL2_image
        SDL2_ttf::SDL2_ttf
    )

    # The GUI loads "sprites/..." relative to where it runs, so ship them next to the executable
    add_custom_command(TARGET chess POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/sprites $<TARGET_FILE_DIR:chess>/sprites
        COMMENT "Copying sprites next to the executable")
endif()
