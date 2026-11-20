# SPDX-License-Identifier: Apache-2.0

add_library(naja_snl_hdl STATIC SNLRTLPrimitives.cpp SNLHDLSpecialization.cpp)
set_property(TARGET naja_snl_hdl PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(naja_snl_hdl PUBLIC naja_nl)
target_include_directories(naja_snl_hdl PRIVATE ${PROJECT_SOURCE_DIR}/thirdparty/sha256)
target_include_directories(naja_snl_hdl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT BUILD_NAJA_PYTHON)
  set_target_properties(naja_snl_hdl PROPERTIES PUBLIC_HEADER "SNLRTLPrimitives.h;SNLHDLLibrary.h;SNLHDLSpecialization.h")
  install(TARGETS naja_snl_hdl
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
