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

set(_img_convert "")

if(WIN32)
  find_program(_img_magick NAMES magick.exe magick
    DOC "ImageMagick magick for icon generation")
  if(_img_magick)
    set(_img_convert "${_img_magick}")
  else()
    message(STATUS "Icon: ImageMagick 'magick' not found on Windows; using existing .ico/.icns")
    return()
  endif()
else()
  find_program(_img_convert NAMES magick convert
    DOC "ImageMagick convert for icon generation")
  if(NOT _img_convert)
    message(STATUS "Icon: ${PNG_ICON_SOURCE} found but ImageMagick not found; using existing .ico/.icns")
    return()
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
set(USE_GENERATED_ICONS FALSE)
if(WIN32)
  set(USE_GENERATED_ICONS TRUE)
endif()
set(GENERATED_ICO_FILENAME "${_ico_stem}.ico")
if(WIN32)
  message(STATUS "Windows icon will be generated from ${PNG_ICON_SOURCE} (.ico)")
endif()
