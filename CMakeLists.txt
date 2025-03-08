cmake_minimum_required(VERSION 3.10)
project(CubeShooter)

set(CMAKE_CXX_STANDARD 17)

set(STEAM_SDK_DIR "C:/Users/dunca/Desktop/sdk")
set(SFML_DIR "C:/Users/dunca/Desktop/SFML-2.6.1")
# Include directories
include_directories(
    ${STEAM_SDK_DIR}/public
    ${SFML_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/Core
    ${CMAKE_SOURCE_DIR}/src/Entities
    ${CMAKE_SOURCE_DIR}/src/Networking
    ${CMAKE_SOURCE_DIR}/src/States
    ${CMAKE_SOURCE_DIR}/src/Utils
)

# Libraries
set(STEAM_LIB "${STEAM_SDK_DIR}/redistributable_bin/win64/steam_api64.lib")
link_directories(${SFML_DIR}/lib)

# Source files
add_executable(CubeShooter
    src/main.cpp
    src/Core/CubeGame.cpp
    src/Entities/Player.cpp
    src/Entities/Enemy.cpp
    src/Entities/Bullet.cpp
    src/Entities/EntityManager.cpp
    src/Hud/Hud.cpp
    src/Networking/SteamManager.cpp
    src/Networking/NetworkManager.cpp
    src/States/MainMenuState.cpp
    src/States/LobbyState.cpp
    src/States/GameplayState.cpp
    src/States/GameOverState.cpp
    src/States/LobbyCreationState.cpp
    src/States/LobbySearchState.cpp
)
# Link libraries
target_link_libraries(CubeShooter
    ${STEAM_LIB}
    sfml-graphics
    sfml-window
    sfml-system
)

# MSVC-specific settings
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
endif()

# Function to copy files only if they exist
function(copy_if_exists SRC DEST)
    if(EXISTS ${SRC})
        add_custom_command(TARGET CubeShooter POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SRC} ${DEST}
        )
    else()
        message(WARNING "Missing file: ${SRC}")
    endif()
endfunction()

# Copy Steam API and steam_appid.txt
copy_if_exists("${STEAM_SDK_DIR}/redistributable_bin/win64/steam_api64.dll" "$<TARGET_FILE_DIR:CubeShooter>")
copy_if_exists("${CMAKE_SOURCE_DIR}/steam_appid.txt" "$<TARGET_FILE_DIR:CubeShooter>")

# Copy SFML DLLs and font
set(SFML_BIN_DIR "${SFML_DIR}/bin")
copy_if_exists("C:/Users/dunca/Desktop/Projects/CubeGame/Roboto-Regular.ttf" "$<TARGET_FILE_DIR:CubeShooter>")
copy_if_exists("${SFML_BIN_DIR}/sfml-graphics-2.dll" "$<TARGET_FILE_DIR:CubeShooter>")
copy_if_exists("${SFML_BIN_DIR}/sfml-graphics-d-2.dll" "$<TARGET_FILE_DIR:CubeShooter>")
copy_if_exists("${SFML_BIN_DIR}/sfml-system-2.dll" "$<TARGET_FILE_DIR:CubeShooter>")
copy_if_exists("${SFML_BIN_DIR}/sfml-system-d-2.dll" "$<TARGET_FILE_DIR:CubeShooter>")
copy_if_exists("${SFML_BIN_DIR}/sfml-window-2.dll" "$<TARGET_FILE_DIR:CubeShooter>")
copy_if_exists("${SFML_BIN_DIR}/sfml-window-d-2.dll" "$<TARGET_FILE_DIR:CubeShooter>")
# Removed unused SFML audio/network DLLs since they aren't needed