get_filename_component(_ParaViewExternalData_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(${_ParaViewExternalData_DIR}/ExternalData.cmake)

if(NOT PARAVIEW_DATA_STORE)
  # Select a default from the following.
  set(PARAVIEW_DATA_STORE_DEFAULT "")
  if(EXISTS "${ParaView_SOURCE_DIR}/.ExternalData/config/store")
    # Configuration left by developer setup script.
    file(STRINGS "${ParaView_SOURCE_DIR}/.ExternalData/config/store"
      PARAVIEW_DATA_STORE_DEFAULT LIMIT_COUNT 1 LIMIT_INPUT 1024)
  elseif(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/../ParaViewExternalData")
    # Adjacent directory created by user.
    get_filename_component(PARAVIEW_DATA_STORE_DEFAULT
      "${CMAKE_SOURCE_DIR}/../ParaViewExternalData" ABSOLUTE)
  elseif(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/../ExternalData")
    # Generic adjacent directory created by user.
    get_filename_component(PARAVIEW_DATA_STORE_DEFAULT
      "${CMAKE_SOURCE_DIR}/../ExternalData" ABSOLUTE)
  elseif(DEFINED "ENV{ParaViewExternalData_OBJECT_STORES}")
    # The ParaViewExternalData environment variable.
    file(TO_CMAKE_PATH "$ENV{ParaViewExternalData_OBJECT_STORES}" PARAVIEW_DATA_STORE_DEFAULT)
  elseif(DEFINED "ENV{ExternalData_OBJECT_STORES}")
    # Generic ExternalData environment variable.
    file(TO_CMAKE_PATH "$ENV{ExternalData_OBJECT_STORES}" PARAVIEW_DATA_STORE_DEFAULT)
  endif()
endif()

# Provide users with an option to select a local object store,
# starting with the above-selected default.
set(PARAVIEW_DATA_STORE "${PARAVIEW_DATA_STORE_DEFAULT}" CACHE PATH
  "Local directory holding ExternalData objects in the layout %(algo)/%(hash).")
mark_as_advanced(PARAVIEW_DATA_STORE)

# Use a store in the build tree if none is otherwise configured.
if(NOT PARAVIEW_DATA_STORE)
  if(ExternalData_OBJECT_STORES)
    set(PARAVIEW_DATA_STORE "")
  else()
    set(PARAVIEW_DATA_STORE "${CMAKE_BINARY_DIR}/ExternalData/Objects")
    file(MAKE_DIRECTORY "${PARAVIEW_DATA_STORE}")
  endif()
endif()

# Tell ExternalData module about selected object stores.
list(APPEND ExternalData_OBJECT_STORES
  # Store selected by ParaView-specific configuration above.
  ${PARAVIEW_DATA_STORE}

  # Local data store populated by the ParaView pre-commit hook
  "${CMAKE_SOURCE_DIR}/.ExternalData"
  )

set(ExternalData_BINARY_ROOT ${CMAKE_BINARY_DIR}/ExternalData)

set(ExternalData_URL_TEMPLATES "" CACHE STRING
  "Additional URL templates for the ExternalData CMake script to look for testing data. E.g.
file:///var/bigharddrive/%(algo)/%(hash)")
mark_as_advanced(ExternalData_URL_TEMPLATES)
list(APPEND ExternalData_URL_TEMPLATES
  # Data published by MIDAS
  # Provided by VTK
  "http://midas3.kitware.com/midas/api/rest?method=midas.bitstream.download&checksum=%(hash)&algorithm=%(algo)"

  # Data published by developers using git-gerrit-push.
  "http://www.paraview.org/files/ExternalData/%(algo)/%(hash)"
  )

# Tell ExternalData commands to transform raw files to content links.
# TODO: Condition this feature on presence of our pre-commit hook.
set(ExternalData_LINK_CONTENT MD5)

# Match series of the form <base>.<ext>, <base>_<n>.<ext> such that <base> may
# end in a (test) number that is not part of any series numbering.
set(ExternalData_SERIES_PARSE "()(\\.[^./]*)$")
set(ExternalData_SERIES_MATCH "(_[0-9]+)?")