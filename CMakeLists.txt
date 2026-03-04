cmake_minimum_required(VERSION 3.22)

# ── Version ────────────────────────────────────────────────────────────────────
# Single source of truth for the app version:
# - installer_bridge.iss and Inno Setup read it via version.iss
# - CMake reads the same version.iss here and passes it to project().
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/version.iss" _eb_version_line REGEX "AppVersion")
if(NOT _eb_version_line)
  message(FATAL_ERROR "version.iss not found or AppVersion not defined")
endif()
string(REGEX REPLACE ".*\"([0-9]+\\.[0-9]+\\.[0-9]+)\".*" "\\1" APP_VERSION "${_eb_version_line}")

project(MTC_Bridge_JUCE VERSION ${APP_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── Centralized version ────────────────────────────────────────────────────
# Version is defined ONLY in the project() call above.
# BridgeVersion.h is auto-generated from BridgeVersion.h.in at configure time.
# To release a new version: change the VERSION field in project() and re-run cmake.
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/core/BridgeVersion.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/core/BridgeVersion.h"
    @ONLY
)

include(FetchContent)

FetchContent_Declare(
  juce
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.4
)
FetchContent_MakeAvailable(juce)

juce_add_gui_app(EasyBridge
  PRODUCT_NAME "Easy Bridge"
  MICROPHONE_PERMISSION_ENABLED TRUE
  MICROPHONE_PERMISSION_TEXT "Easy Bridge requires audio input access for LTC input."
)
juce_generate_juce_header(EasyBridge)

# Icons: one PNG → generate .ico (Windows) and .icns (macOS) at build time if ImageMagick is available
set(PNG_ICON_SOURCE "Icons/Icon Bridge.png")
set(ICON_OUTPUT_BASE "Icon Bridge")
set(ICON_BUNDLE_NAME "Icon Bridge")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateIconsFromPng.cmake")

if(USE_GENERATED_ICONS)
  set(ICO_FILENAME "${GENERATED_ICO_FILENAME}")
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/AppIcon.rc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/AppIcon.rc"
    @ONLY
  )
  add_custom_target(icon_gen ALL DEPENDS ${GENERATED_ICO_PATH} ${GENERATED_ICNS_PATH})
  add_dependencies(EasyBridge icon_gen)
  set(_appicon_rc "${CMAKE_CURRENT_BINARY_DIR}/generated/AppIcon.rc")
else()
  set(_appicon_rc "${CMAKE_CURRENT_SOURCE_DIR}/Source/AppIcon.rc")
endif()

# macOS app bundle icon — copy .icns into Resources and set CFBundleIconFile
if(APPLE)
  set(_bridge_icns "${CMAKE_CURRENT_SOURCE_DIR}/Icons/Icon Bridge.icns")
  target_sources(EasyBridge PRIVATE "${_bridge_icns}")
  set_source_files_properties("${_bridge_icns}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  set_target_properties(EasyBridge PROPERTIES MACOSX_BUNDLE_ICON_FILE "${ICON_BUNDLE_NAME}")
  add_custom_command(TARGET EasyBridge POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
      "-DBUNDLE_CONTENT=$<TARGET_BUNDLE_CONTENT_DIR:EasyBridge>"
      "-DICNS_SRC=${_bridge_icns}"
      "-DICON_NAME=${ICON_BUNDLE_NAME}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/patch_macos_icon.cmake"
    COMMENT "Patching macOS bundle icon"
    VERBATIM
  )
  add_custom_command(TARGET EasyBridge POST_BUILD
    COMMAND /bin/rm -rf
      "$<TARGET_BUNDLE_CONTENT_DIR:EasyBridge>/Icon"
    COMMAND /bin/rm -rf
      "$<TARGET_BUNDLE_CONTENT_DIR:EasyBridge>/Icons"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/Fonts"
      "$<TARGET_BUNDLE_CONTENT_DIR:EasyBridge>/Fonts"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/Help"
      "$<TARGET_BUNDLE_CONTENT_DIR:EasyBridge>/Help"
    COMMENT "Copying runtime assets into macOS bundle (Contents/)"
    VERBATIM
  )
endif()

target_sources(EasyBridge PRIVATE
  Source/Main.cpp
  Source/App.cpp
  Source/App.h
  Source/MainWindow.cpp
  Source/MainWindow.h
  ${_appicon_rc}
  Source/engine/BridgeEngine.cpp
  Source/engine/BridgeEngine.h
  Source/engine/OscInput.cpp
  Source/engine/inputs/OscInput.h
  Source/core/BridgeVersion.h
  Source/core/Timecode.cpp
  Source/core/Timecode.h
  Source/core/ClockState.cpp
  Source/core/ClockState.h
  Source/core/ConfigStore.cpp
  Source/core/ConfigStore.h
  # ── Extracted UI & platform helpers ────────────────────────────────────────
  Source/platform/WindowUtils.cpp
  Source/platform/WindowUtils.h
  Source/ui/style/BridgeColours.h
  Source/ui/style/BridgeLookAndFeel.h
  Source/ui/style/StyleHelpers.h
  Source/ui/widgets/CircleButtons.h
  Source/ui/widgets/DotToggle.h
  Source/ui/widgets/MacSwitch.h
  Source/ui/widgets/LevelMeter.h
  Source/ui/DarkDialog.h
  Source/ui/StatusMonitorWindow.h
)

target_compile_definitions(EasyBridge PRIVATE
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
)

target_link_libraries(EasyBridge PRIVATE
  juce::juce_gui_extra
  juce::juce_audio_utils
  juce::juce_osc
)

if(WIN32)
  set(ASIO_SDK_DIR "$ENV{ASIO_SDK_DIR}" CACHE PATH "Path to Steinberg ASIO SDK root")
  if(NOT ASIO_SDK_DIR)
    set(_asio_candidates
      "${CMAKE_CURRENT_SOURCE_DIR}/ASIOSDK"
      "${CMAKE_CURRENT_SOURCE_DIR}/asiosdk"
      "${CMAKE_CURRENT_SOURCE_DIR}/third_party/asiosdk"
      "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/asiosdk"
      "C:/ASIOSDK"
      "C:/SDK/ASIOSDK"
      "C:/SDK/asiosdk"
      "$ENV{USERPROFILE}/Downloads/asiosdk"
      "$ENV{USERPROFILE}/Downloads/ASIOSDK"
      "$ENV{USERPROFILE}/Desktop/asiosdk"
      "$ENV{USERPROFILE}/Desktop/ASIOSDK"
    )
    foreach(_cand IN LISTS _asio_candidates)
      if(EXISTS "${_cand}/common/asio.h" AND EXISTS "${_cand}/host/asiodrivers.h")
        set(ASIO_SDK_DIR "${_cand}" CACHE PATH "Path to Steinberg ASIO SDK root" FORCE)
        break()
      endif()
    endforeach()
  endif()
  if(ASIO_SDK_DIR)
    if(EXISTS "${ASIO_SDK_DIR}/common/asio.h" AND EXISTS "${ASIO_SDK_DIR}/host/asiodrivers.h")
      target_compile_definitions(EasyBridge PRIVATE JUCE_ASIO=1)
      target_include_directories(EasyBridge PRIVATE
        "${ASIO_SDK_DIR}/common"
        "${ASIO_SDK_DIR}/host"
      )
      message(STATUS "ASIO enabled for EasyBridge from: ${ASIO_SDK_DIR}")
    else()
      message(WARNING "ASIO_SDK_DIR is set but required headers were not found in common/ and host/. ASIO disabled.")
    endif()
  else()
    message(STATUS "ASIO SDK not found for EasyBridge. Build will not include ASIO/ReaRoute devices.")
  endif()
  target_link_libraries(EasyBridge PRIVATE ws2_32)
endif()
