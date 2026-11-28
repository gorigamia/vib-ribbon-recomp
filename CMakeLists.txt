cmake_minimum_required(VERSION 3.24)
project(VibRibbonPC LANGUAGES C CXX)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

set(PSXRECOMP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/tools/psxrecomp-src" CACHE PATH "PSXRecomp checkout")
set(PSX_RECOMP_UI OFF CACHE BOOL "No launcher needed for initial validation")
set(PSX_NETPLAY OFF CACHE BOOL "Single-player validation")
set(PSX_ENABLE_VULKAN OFF CACHE BOOL "Use OpenGL for initial validation")
set(PSX_REWIND OFF CACHE BOOL "Disable rewind for the baseline")
set(PSX_PGXP_VARIANT OFF CACHE BOOL "Do not build an enhanced second executable")
set(PSX_DEBUG_TOOLS ON CACHE BOOL "Enable diagnostic counters and debug interface")
set(BUILD_TESTING OFF CACHE BOOL "Only build the title experiment")
set(PSXRECOMP_REQUIRE_GAME_C ON CACHE BOOL "Refuse a setup-only binary")

include("${PSXRECOMP_ROOT}/runtime/runtime.cmake")
psxrecomp_add_game_runtime(psx-runtime
    WINDOW_TITLE "Vib-Ribbon PC - runtime experiment"
    EXE_NAME "VibRibbonPC"
    DEFAULT_GAME_CONFIG_PATH "game.toml"
    PRELOADED_MODS_DIR "NONE"
    EXTRAS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/vib_settings.cpp"
    MAX_PLAYERS 1
    GAME_OVERLAY_STATIC_C "${CMAKE_CURRENT_SOURCE_DIR}/.local/generated/overlays/overlays_static.c"
    GEN_MARKER ".local/generated/boot/SCES_028.73_dispatch.c"
    GEN_FULL_GLOB ".local/generated/boot/SCES_028.73_full_*.c"
)
target_include_directories(psx-runtime PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
