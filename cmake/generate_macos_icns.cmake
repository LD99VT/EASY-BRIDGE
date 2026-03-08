cmake_minimum_required(VERSION 3.22)

# generate_macos_icns.cmake — build a full .icns from a 1024x1024 PNG
# Required -D variables:
#   PNG_SRC  — source PNG file
#   ICNS_OUT — destination .icns file

if(NOT DEFINED PNG_SRC OR NOT DEFINED ICNS_OUT)
  message(FATAL_ERROR "generate_macos_icns: PNG_SRC and ICNS_OUT are required")
endif()

if(NOT EXISTS "${PNG_SRC}")
  message(FATAL_ERROR "generate_macos_icns: source PNG not found: ${PNG_SRC}")
endif()

get_filename_component(_out_dir "${ICNS_OUT}" DIRECTORY)
get_filename_component(_out_stem "${ICNS_OUT}" NAME_WE)
set(_iconset_dir "${_out_dir}/${_out_stem}.iconset")

file(MAKE_DIRECTORY "${_out_dir}")
file(REMOVE_RECURSE "${_iconset_dir}")
file(MAKE_DIRECTORY "${_iconset_dir}")

function(_resize_icon size scale)
  math(EXPR pixels "${size} * ${scale}")
  if(scale EQUAL 1)
    set(_name "icon_${size}x${size}.png")
  else()
    set(_name "icon_${size}x${size}@2x.png")
  endif()

  execute_process(
    COMMAND sips -z "${pixels}" "${pixels}" "${PNG_SRC}" --out "${_iconset_dir}/${_name}"
    RESULT_VARIABLE _result
    OUTPUT_QUIET
    ERROR_VARIABLE _error
  )
  if(NOT _result EQUAL 0)
    message(FATAL_ERROR "generate_macos_icns: failed to create ${_name}: ${_error}")
  endif()
endfunction()

_resize_icon(16 1)
_resize_icon(16 2)
_resize_icon(32 1)
_resize_icon(32 2)
_resize_icon(128 1)
_resize_icon(128 2)
_resize_icon(256 1)
_resize_icon(256 2)
_resize_icon(512 1)
_resize_icon(512 2)

execute_process(
  COMMAND iconutil -c icns "${_iconset_dir}" -o "${ICNS_OUT}"
  RESULT_VARIABLE _result
  OUTPUT_QUIET
  ERROR_VARIABLE _error
)
if(NOT _result EQUAL 0)
  message(FATAL_ERROR "generate_macos_icns: iconutil failed: ${_error}")
endif()

file(REMOVE_RECURSE "${_iconset_dir}")
