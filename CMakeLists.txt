cmake_minimum_required(VERSION 3.22)

project(MTC_Bridge_JUCE VERSION 2.6.0 LANGUAGES CXX)

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

# macOS app bundle icon — copy .icns into Resources and set CFBundleIconFile
if(APPLE)
  set(_bridge_icns "${CMAKE_CURRENT_SOURCE_DIR}/Icons/Icon Bridge.icns")
  target_sources(EasyBridge PRIVATE "${_bridge_icns}")
  set_source_files_properties("${_bridge_icns}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  # CFBundleIconFile must NOT include the extension (Apple requirement)
  set_target_properties(EasyBridge PROPERTIES MACOSX_BUNDLE_ICON_FILE "Icon Bridge")
  # Post-build: guarantee icon lands in bundle and Info.plist is correct
  # (JUCE/Xcode sometimes does not substitute MACOSX_BUNDLE_ICON_FILE reliably)
  add_custom_command(TARGET EasyBridge POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
      "-DBUNDLE_CONTENT=$<TARGET_BUNDLE_CONTENT_DIR:EasyBridge>"
      "-DICNS_SRC=${_bridge_icns}"
      "-DICON_NAME=Icon Bridge"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/patch_macos_icon.cmake"
    COMMENT "Patching macOS bundle icon"
    VERBATIM
  )
  # Copy runtime assets into Contents/ (one level up from MacOS/) so the
  # walk-up search in findBridgeBaseDir() finds Fonts/, Help/, Icons/ at
  # Contents/ level — without placing non-code files in Contents/MacOS/
  # (which would make codesign reject them as unsigned code objects).
  add_custom_command(TARGET EasyBridge POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/Fonts"
      "$<TARGET_BUNDLE_CONTENT_DIR:EasyBridge>/Fonts"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/Help"
      "$<TARGET_BUNDLE_CONTENT_DIR:EasyBridge>/Help"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/Icons"
      "$<TARGET_BUNDLE_CONTENT_DIR:EasyBridge>/Icons"
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
  Source/AppIcon.rc
  Source/engine/BridgeEngine.cpp
  Source/engine/BridgeEngine.h
  Source/engine/OscInput.cpp
  Source/engine/OscInput.h
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
