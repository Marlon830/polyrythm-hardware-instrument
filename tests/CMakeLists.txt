include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.0.zip
)

set(BUILD_GMOCK OFF)
set(INSTALL_GTEST OFF)
FetchContent_MakeAvailable(googletest)

set(MAIN_SOURCE "${CMAKE_SOURCE_DIR}/source/main.cpp")
list(REMOVE_ITEM ALL_SOURCES ${MAIN_SOURCE})

find_package(ALSA REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SNDFILE REQUIRED sndfile)

add_library(TinyFirmwareLib STATIC ${ALL_SOURCES})
target_include_directories(TinyFirmwareLib
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${SNDFILE_LIBRARIES}
)

target_compile_features(TinyFirmwareLib PUBLIC cxx_std_17)
set_target_properties(TinyFirmwareLib PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)


target_link_libraries(TinyFirmwareLib
    PUBLIC
        ALSA::ALSA
        ftxui::screen
        ftxui::dom
        ftxui::component
        nlohmann_json::nlohmann_json
        ${SNDFILE_LIBRARIES}
)

add_executable(unit_tests
    test_example.cpp
    engine/module/OscModule.tests.cpp
    engine/module/FilterModule.tests.cpp
    engine/module/InstrumentModule.tests.cpp
    test_audiograph.cpp
)

target_link_libraries(unit_tests
    PRIVATE
        TinyFirmwareLib
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(unit_tests)