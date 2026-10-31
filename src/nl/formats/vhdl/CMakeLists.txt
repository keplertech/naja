# SPDX-License-Identifier: Apache-2.0

add_library(naja_snl_vhdl STATIC VHDLConstructor.cpp VHDLRTLConstructor.cpp VHDLIntent.cpp)
set_property(TARGET naja_snl_vhdl PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(naja_snl_vhdl PUBLIC naja_snl_hdl vhdl_frontend)
target_include_directories(naja_snl_vhdl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT BUILD_NAJA_PYTHON)
  set_target_properties(naja_snl_vhdl PROPERTIES PUBLIC_HEADER "VHDLConstructor.h;VHDLIntent.h")
  install(TARGETS naja_snl_vhdl
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
