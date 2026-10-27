# ---- Perft regression tests ----------------------------------------------------
#
#  Each test runs the perft tool on a position and checks the total node count.
#  Expected counts are the standard ones from https://www.chessprogramming.org/Perft_Results
#  (cross-checked with python-chess). Depths are kept low so the whole suite runs in seconds,
#  even in Debug with sanitizers.

#  add_perft_test(<name> <depth> <expected nodes> <fen> [KNOWN_BUG <reason>])
#
#  KNOWN_BUG marks a position the engine currently gets wrong: the test is inverted (WILL_FAIL)
#  so the suite stays green, and it starts failing as soon as the bug is fixed, which is the
#  signal to remove the KNOWN_BUG flag.
function(add_perft_test name depth expected fen)
    cmake_parse_arguments(ARG "" "KNOWN_BUG" "" ${ARGN})

    set(test_name "perft.${name}.d${depth}")
    add_test(NAME ${test_name} COMMAND perft ${depth} ${fen})

    #  [^0-9] after the count so 197281 does not match 1972810
    set_tests_properties(${test_name} PROPERTIES
        PASS_REGULAR_EXPRESSION "Nodes searched: ${expected}[^0-9]"
        TIMEOUT 120
        LABELS "perft")

    if(ARG_KNOWN_BUG)
        set_tests_properties(${test_name} PROPERTIES
            WILL_FAIL TRUE
            LABELS "perft;known-bug")
        message(STATUS "Known bug, inverted test: ${test_name} (${ARG_KNOWN_BUG})")
    endif()
endfunction()


add_perft_test(startpos 4 197281
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")

add_perft_test(kiwipete 3 97862
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1")

add_perft_test(position3 4 43238
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1")

add_perft_test(position4 3 9467
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1")

add_perft_test(position5 3 62379
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8")

add_perft_test(position6 3 81467
    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P3/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10")


# ---- Game status tests ----------------------------------------------------------
#
#  Checkmate, stalemate and fifty move rule detection on hand-picked positions.

add_executable(game_status_test game_status_test.cpp)
target_link_libraries(game_status_test PRIVATE engine chess_options)
add_test(NAME game_status COMMAND game_status_test)


# ---- Unit tests (GoogleTest) --------------------------------------------------
#
#  GoogleTest is downloaded at configure time, pinned to a release archive.
#  gtest_discover_tests registers every TEST() as its own ctest entry (core.Fen.RoundTrip, ...).

include(FetchContent)
FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(core_tests
    core/fen_test.cpp
)
target_link_libraries(core_tests PRIVATE core chess_options GTest::gtest_main)
gtest_discover_tests(core_tests TEST_PREFIX "core.")
