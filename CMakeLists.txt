# Music Player CMake Configuration
cmake_minimum_required(VERSION 3.15)
project(MusicPlayer VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Print configuration
message(STATUS "")
message(STATUS "Music Player Build Configuration:")
message(STATUS "  C++ Standard:      ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "")

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/inc
    ${CMAKE_SOURCE_DIR}/inc/interfaces
    ${CMAKE_SOURCE_DIR}/inc/utils
    ${CMAKE_SOURCE_DIR}/inc/app
    ${CMAKE_SOURCE_DIR}/inc/service
    ${CMAKE_SOURCE_DIR}/inc/hal
)

# Source files - Models
set(MODEL_SOURCES
    src/app/model/MediaFile.cpp
    src/app/model/Playlist.cpp
    src/app/model/Library.cpp
    src/app/model/PlaybackState.cpp
    src/app/model/History.cpp
    src/app/model/PlaylistManager.cpp
    src/app/model/MediaFileFactory.cpp
)

# Source files - Controllers
set(CONTROLLER_SOURCES
    src/app/controller/LibraryController.cpp
    src/app/controller/PlaylistController.cpp
    src/app/controller/PlaybackController.cpp
    src/app/controller/HistoryController.cpp
    src/app/controller/USBController.cpp
)

# Source files - Views
set(VIEW_SOURCES
    src/app/view/BaseView.cpp
    src/app/view/LibraryView.cpp
    src/app/view/PlaylistView.cpp
    src/app/view/NowPlayingView.cpp
    src/app/view/HistoryView.cpp
    src/app/view/FileBrowserView.cpp
    src/app/view/ViewFactory.cpp
    src/app/view/MainWindow.cpp
)

# Source files - Services
set(SERVICE_SOURCES
    src/service/LocalFileSystem.cpp
    src/service/TagLibMetadataReader.cpp
    src/service/SDL2PlaybackEngine.cpp
    src/service/JsonPersistence.cpp
)

# Source files - HAL
set(HAL_SOURCES
    src/hal/LinuxHardware.cpp
)

# Source files - Application
set(APP_SOURCES
    src/app/Application.cpp
)

# Main executable
add_executable(music_player
    src/main.cpp
    ${MODEL_SOURCES}
    ${CONTROLLER_SOURCES}
    ${VIEW_SOURCES}
    ${SERVICE_SOURCES}
    ${HAL_SOURCES}
    ${APP_SOURCES}
)

# Find and link TagLib
find_package(PkgConfig REQUIRED)
pkg_check_modules(TAGLIB REQUIRED taglib)
target_include_directories(music_player PRIVATE ${TAGLIB_INCLUDE_DIRS})
target_link_libraries(music_player PRIVATE ${TAGLIB_LIBRARIES})

# ImGui - Real Production Integration
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/external/imgui")
if(EXISTS "${IMGUI_DIR}/imgui.h")
    message(STATUS "  ImGui Support:     ON")
    
    # Create ImGui library
    add_library(imgui STATIC
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
    )
    
    # Check for SDL2 backends
    pkg_check_modules(SDL2 sdl2)
    if(SDL2_FOUND AND EXISTS "${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp")
        target_sources(imgui PRIVATE
            ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
            ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
        )
        target_include_directories(imgui PUBLIC ${SDL2_INCLUDE_DIRS})
        target_link_libraries(imgui PUBLIC ${SDL2_LIBRARIES} GL)
        message(STATUS "  SDL2 Backend:      ON")
    endif()
    
    target_include_directories(imgui PUBLIC 
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
    )
    
    target_link_libraries(music_player PRIVATE imgui)
    target_compile_definitions(music_player PRIVATE USE_IMGUI)
else()
    message(WARNING "  ImGui Support:     OFF (not found at ${IMGUI_DIR})")
    message(WARNING "Run './build_production.sh' to download and compile with ImGui")
endif()

# SDL2 for audio (optional but recommended)
# SDL2 for audio (optional but recommended)
pkg_check_modules(SDL2 sdl2)
pkg_check_modules(SDL2_MIXER SDL2_mixer)

if(SDL2_FOUND)
    target_include_directories(music_player PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(music_player PRIVATE ${SDL2_LIBRARIES})
    target_compile_definitions(music_player PRIVATE USE_SDL2)
    message(STATUS "  SDL2 Support:      ON")

    if(SDL2_MIXER_FOUND)
        target_include_directories(music_player PRIVATE ${SDL2_MIXER_INCLUDE_DIRS})
        target_link_libraries(music_player PRIVATE ${SDL2_MIXER_LIBRARIES})
        target_compile_definitions(music_player PRIVATE USE_SDL_MIXER)
        message(STATUS "  SDL2 Mixer:        ON")
    else()
        message(WARNING "  SDL2 Mixer:        OFF (Install libsdl2-mixer-dev for audio)")
    endif()
else()
    message(WARNING "  SDL2 Support:      OFF")
endif()

# Threading
find_package(Threads REQUIRED)
target_link_libraries(music_player PRIVATE Threads::Threads)

# Filesystem (for older GCC)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.1)
    target_link_libraries(music_player PRIVATE stdc++fs)
endif()

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(music_player PRIVATE 
        -Wall -Wextra -Wpedantic
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Debug>:-g -O0>
    )
endif()

message(STATUS "")

# Install target
install(TARGETS music_player DESTINATION bin)
