cmake_minimum_required(VERSION 3.16)
project(Battleship LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BATTLESHIP_CORE_SOURCES
    src/application/FleetPlan.cpp
    src/application/Match.cpp
    src/domain/AiPlayer.cpp
    src/domain/AiStrategy.cpp
    src/domain/Board.cpp
    src/domain/Types.cpp
    src/infrastructure/FileMatchRepository.cpp
    src/infrastructure/MatchFactory.cpp
    src/infrastructure/StdRandomGenerator.cpp
)

add_executable(battleship_console
    ${BATTLESHIP_CORE_SOURCES}
    src/console/main.cpp
)

target_include_directories(battleship_console PRIVATE src)

add_executable(battleship_tests
    tests/domain_tests.cpp
    src/application/FleetPlan.cpp
    src/application/Match.cpp
    src/domain/AiPlayer.cpp
    src/domain/AiStrategy.cpp
    src/domain/Board.cpp
    src/domain/Types.cpp
)

target_include_directories(battleship_tests PRIVATE src)

if(APPLE)
    enable_language(OBJCXX)

    add_executable(battleship
        ${BATTLESHIP_CORE_SOURCES}
        src/ui/main.mm
    )

    target_include_directories(battleship PRIVATE src)

    target_link_libraries(battleship PRIVATE
        "-framework Cocoa"
    )

    set_source_files_properties(src/ui/main.mm PROPERTIES
        COMPILE_OPTIONS "-fobjc-arc"
    )
endif()

if(MSVC)
    target_compile_options(battleship_console PRIVATE /W4)
    target_compile_options(battleship_tests PRIVATE /W4)
    if(APPLE)
        target_compile_options(battleship PRIVATE /W4)
    endif()
else()
    target_compile_options(battleship_console PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(battleship_tests PRIVATE -Wall -Wextra -Wpedantic)
    if(APPLE)
        target_compile_options(battleship PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()
