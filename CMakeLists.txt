cmake_minimum_required(VERSION 3.22)
project(TinyFirmware LANGUAGES C CXX)

enable_language(C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  GIT_TAG v6.1.9
)
FetchContent_MakeAvailable(ftxui)


if(WIN32)
    # === WINDOWS ===
    message(STATUS "Configuration Build : WINDOWS (WASAPI)")
    
    add_compile_definitions(__WINDOWS_WASAPI__)
    set(PLATFORM_LIBS 
        dsound 
        winmm 
        ksuser 
        uuid 
        ole32 
        advapi32
    )

    find_package(SndFile CONFIG)
    if(TARGET SndFile::sndfile)
        set(SNDFILE_LIBS SndFile::sndfile)
    else()
        message(WARNING "libsndfile non trouv�. Assurez-vous de ne pas utiliser SNDFILE sur Windows ou installez-le via vcpkg.")
    endif()

elseif(UNIX AND NOT APPLE)
    # === LINUX ===
    message(STATUS "Configuration Build : LINUX (ALSA)")

    add_compile_definitions(__LINUX_ALSA__)
    find_package(ALSA REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SNDFILE REQUIRED sndfile)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(LIBDRM REQUIRED libdrm)
    include_directories(${LIBDRM_INCLUDE_DIRS})

    set(PLATFORM_LIBS 
        ALSA::ALSA 
        pthread
        ${SDL2_LIBRARIES}
        ${LIBDRM_LIBRARIES}
    )
    set(SNDFILE_LIBS ${SNDFILE_LIBRARIES})

endif()
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# LVGL configuration
add_subdirectory(lvgl)

file(GLOB_RECURSE ALL_SOURCES
    "${CMAKE_SOURCE_DIR}/source/*.cpp"
    "${CMAKE_SOURCE_DIR}/source/*.c"
)

add_executable(${PROJECT_NAME} ${ALL_SOURCES})
target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/lvgl
        ${SNDFILE_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIRS}
        ${LIBDRM_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ftxui::screen
        ftxui::dom
        ftxui::component
        nlohmann_json::nlohmann_json
        lvgl
        ${PLATFORM_LIBS}
        ${SNDFILE_LIBS}
)

option(BUILD_DOC "Build documentation" ON)
option(BUILD_TESTS "Build tests" OFF)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_DOC)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        add_subdirectory(docs)
    else()
        message(WARNING "Doxygen not found. The documentation will not be generated.")
    endif()
endif()