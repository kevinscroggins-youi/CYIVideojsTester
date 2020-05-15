# =============================================================================
# © You i Labs Inc. 2000-2017. All rights reserved.
cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

if(__yi_custom_platform_included)
    return()
endif()

set(__yi_custom_platform_included 1)

include(${YouiEngine_DIR}/cmake/Platform/YiTizenNacl.cmake)
macro(yi_configure_platform)
  cmake_parse_arguments(_ARGS "" "PROJECT_TARGET" "" ${ARGN})

  if(NOT _ARGS_PROJECT_TARGET)
      message(FATAL_ERROR "'yi_configure_platform' requires the PROJECT_TARGET argument be set")
  endif()

  # Call the "super" configuration here that is in the engine, it respects any values you set previous to this
  _yi_configure_platform(PROJECT_TARGET ${_ARGS_PROJECT_TARGET})

  # Set Tizen platform variable overrides
  set(YI_AUTHOR_NAME "You.i TV" CACHE STRING "The name of the company that is publishing the widget." FORCE)
  set(YI_COMPANY_URL "https://www.youi.tv" CACHE STRING "The company URL that will be inserted into the 'config.xml' file." FORCE)
  set(YI_DESCRIPTION "You.i TV Tizen NaCl Video.js tester application." CACHE STRING "The description of the widget." FORCE)
  set(YI_METADATA_CATEGORY "tester" CACHE STRING "The category for the Tizen application." FORCE)
  set(YI_PACKAGE_ID "gWjgX0Xyna" CACHE STRING "The ID value of the package. This gets combined in 'config.xml' with the YI_APP_NAME to make the application ID." FORCE)
  set(YI_SMART_HUB_PREVIEW_URL "" CACHE STRING "Set the remote JSON file URL for Smart Hub public preview." FORCE)

endmacro(yi_configure_platform)
