# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(SOURCES
  PySNLTimingAnalysis.cpp PyNetlistState.cpp
  PyNLID.cpp
  PyNLUniverse.cpp
  PyNLDB.cpp PyNLLibrary.cpp
  PyNLDBs.cpp PyNLLibraries.cpp
  PySNLDesign.cpp
  PySNLSequentialModel.cpp
  PySNLMemoryInterface.cpp
  PySNLAttribute.cpp
  PySNLParameter.cpp
  PyNLClock.cpp PyNLClocks.cpp PyNLReset.cpp PyNLResets.cpp
  PySNLDesignObject.cpp
  PySNLNet.cpp PySNLBusNet.cpp PySNLBitNet.cpp PySNLScalarNet.cpp PySNLBusNetBit.cpp
  PySNLNetType.cpp
  PySNLTermRole.cpp
  PySNLActiveLevel.cpp
  PySNLNetComponent.cpp
  PySNLTerm.cpp
  PySNLTermDirection.cpp 
  PySNLBusTerm.cpp PySNLBundleTerm.cpp PySNLBitTerm.cpp PySNLScalarTerm.cpp PySNLBusTermBit.cpp
  PySNLInstance.cpp PySNLInstParameter.cpp PySNLInstTerm.cpp
  PySNLAttributes.cpp
  PySNLDesigns.cpp
  PySNLParameters.cpp PySNLInstParameters.cpp
  PySNLNetComponents.cpp
  PySNLTerms.cpp PySNLBitTerms.cpp PySNLScalarTerms.cpp PySNLBusTerms.cpp PySNLBundleTerms.cpp
  PySNLNets.cpp PySNLBitNets.cpp PySNLScalarNets.cpp PySNLBusNets.cpp
  PySNLInstances.cpp PySNLInstTerms.cpp PySNLPath.cpp PySNLOccurrence.cpp
  PySNLOccurrences.cpp
  PySNLEquipotential.cpp PySNLEquipotentialMode.cpp PyLogicCone.cpp PySNLUniquifier.cpp
  NajaPythonProperty.cpp
)

add_library(naja_python SHARED ${SOURCES})
if (BUILD_NAJA_PYTHON)
  target_link_libraries(naja_python
    PUBLIC
    naja_nl_dump
    naja_snl_verilog naja_snl_systemverilog naja_snl_vhdl naja_snl_liberty naja_snl_visual
    naja_opt
    naja_metrics
    naja_tgraph
    Python3::Module
  )
else(BUILD_NAJA_PYTHON)
  target_link_libraries(naja_python
    PUBLIC
    naja_nl_dump
    naja_snl_verilog naja_snl_systemverilog naja_snl_vhdl naja_snl_liberty naja_snl_visual
    naja_opt
    naja_metrics
    naja_tgraph
    Python3::Python
  )
endif(BUILD_NAJA_PYTHON)

target_compile_definitions(naja_python PRIVATE NAJA_PYTHON_EXPORTS)

target_include_directories(naja_python
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE ${Python3_INCLUDE_DIRS}
)

Python3_add_library(naja MODULE PyNaja.cpp)
if (MSVC)
  target_compile_options(naja PRIVATE /utf-8)
endif()
target_link_libraries(naja PRIVATE naja_python)

if(BUILD_NAJA_PYTHON)
  #install(TARGETS naja LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/najaeda)
  #install(TARGETS naja_python
  #  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/najaeda
  #  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/najaeda
  #)
  install(TARGETS naja LIBRARY DESTINATION najaeda)
  install(TARGETS naja_python
    LIBRARY DESTINATION najaeda
    RUNTIME DESTINATION najaeda
  )
else(BUILD_NAJA_PYTHON)
  install(TARGETS naja LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/python/naja)
  install(TARGETS naja LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/python/najaeda)
  install(TARGETS naja_python LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}) 
endif(BUILD_NAJA_PYTHON)
