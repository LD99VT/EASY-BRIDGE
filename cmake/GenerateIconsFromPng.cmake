# GenerateIconsFromPng.cmake
# From a single PNG, generate .ico (Windows) and .icns (macOS) at build time.
#
# Usage:
#   set(PNG_ICON_SOURCE "Icons/Icon Bridge.png")
#   set(ICON_OUTPUT_BASE "Icon Bridge")
#   set(ICON_BUNDLE_NAME "Icon Bridge")
#   include(cmake/GenerateIconsFromPng.cmake)
#
# If PNG_ICON_SOURCE exists and ImageMagick is found:
#   USE_GENERATED_ICONS=TRUE, GENERATED_ICO_PATH / GENERATED_ICNS_PATH set.
# Otherwise USE_GENERATED_ICONS=FALSE; use existing .ico/.icns in source tree.

set(_png_abs "${CMAKE_CURRENT_SOURCE_DIR}/${PNG_ICON_SOURCE}")
set(USE_GENERATED_ICONS FALSE)

if(NOT EXISTS "${_png_abs}")
  return()
endif()

find_program(_img_convert NAMES magick convert convert.exe
  DOC "ImageMagick convert for icon generation")
if(NOT _img_convert)
  message(STATUS "Icon: ${PNG_ICON_SOURCE} found but ImageMagick not found; using existing .ico/.icns")
  return()
endif()

set(_icns_png_abs "${_png_abs}")
if(APPLE)
  set(_bridge_icon_prep "${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_bridge_macos_icon.swift")
  find_program(_xcrun NAMES xcrun)
  if(EXISTS "${_bridge_icon_prep}" AND _xcrun)
    get_filename_component(_png_name_we "${PNG_ICON_SOURCE}" NAME_WE)
    set(_icns_png_abs "${CMAKE_CURRENT_BINARY_DIR}/generated/${_png_name_we} macOS.png")
    add_custom_command(OUTPUT "${_icns_png_abs}"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated"
      COMMAND "${_xcrun}" swift "${_bridge_icon_prep}" "${_png_abs}" "${_icns_png_abs}"
      MAIN_DEPENDENCY "${_png_abs}"
      DEPENDS "${_bridge_icon_prep}"
      COMMENT "Preparing macOS app icon source from ${PNG_ICON_SOURCE}"
      VERBATIM
    )
  endif()
endif()

string(REPLACE " " "_" _ico_stem "${ICON_OUTPUT_BASE}")
set(_gen_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(_ico_out "${_gen_dir}/${_ico_stem}.ico")

set(GENERATED_ICO_PATH "")
if(WIN32)
  set(GENERATED_ICO_PATH "${_ico_out}")
  add_custom_command(OUTPUT "${_ico_out}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${_gen_dir}"
    COMMAND "${_img_convert}" "${_png_abs}"
      -define icon:auto-resize=256,128,64,48,32,16
      "${_ico_out}"
    MAIN_DEPENDENCY "${_png_abs}"
    COMMENT "Generating .ico from ${PNG_ICON_SOURCE}"
    VERBATIM
  )
endif()

set(GENERATED_ICNS_PATH "")
if(APPLE)
  set(_icns_stem "${ICON_BUNDLE_NAME}")
  if(NOT _icns_stem)
    set(_icns_stem "${ICON_OUTPUT_BASE}")
  endif()
  set(_icns_out "${_gen_dir}/${_icns_stem}.icns")
  set(GENERATED_ICNS_PATH "${_icns_out}")
  set(_iconset "${_gen_dir}/${_icns_stem}.iconset")
  set(_deplist "")
  foreach(_s 16 32 128 256 512)
    math(EXPR _s2 "${_s} * 2")
    set(_p1 "${_iconset}/icon_${_s}x${_s}.png")
    set(_p2 "${_iconset}/icon_${_s}x${_s}@2x.png")
    list(APPEND _deplist "${_p1}" "${_p2}")
    add_custom_command(OUTPUT "${_p1}"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${_iconset}"
      COMMAND "${_img_convert}" "${_icns_png_abs}" -resize "${_s}x${_s}" "${_p1}"
      MAIN_DEPENDENCY "${_icns_png_abs}"
      COMMENT "ICNS iconset ${_p1}"
      VERBATIM
    )
    add_custom_command(OUTPUT "${_p2}"
      COMMAND "${_img_convert}" "${_icns_png_abs}" -resize "${_s2}x${_s2}" "${_p2}"
      MAIN_DEPENDENCY "${_icns_png_abs}"
      COMMENT "ICNS iconset ${_p2}"
      VERBATIM
    )
  endforeach()
  add_custom_command(OUTPUT "${_icns_out}"
    COMMAND iconutil --convert icns --output "${_icns_out}" "${_iconset}"
    DEPENDS ${_deplist}
    MAIN_DEPENDENCY "${_icns_png_abs}"
    COMMENT "Generating .icns from ${PNG_ICON_SOURCE}"
    VERBATIM
  )
endif()

set(USE_GENERATED_ICONS TRUE)
set(GENERATED_ICO_FILENAME "${_ico_stem}.ico")
message(STATUS "Icons will be generated from ${PNG_ICON_SOURCE} (ico + icns on macOS)")
