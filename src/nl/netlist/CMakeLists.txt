# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(serialization)
add_subdirectory(visual)

set(NL_SOURCES
  ${PROJECT_SOURCE_DIR}/thirdparty/sha256/sha-256.c
  core/NLContentHash.cpp
  core/NLName.cpp
  core/NLObject.cpp
  core/NLUniverse.cpp
  core/NLDB.cpp core/NLDB0.cpp core/NLClock.cpp core/NLReset.cpp
  core/NLLibrary.cpp
  core/NLDesign.cpp
)

set(SNL_SOURCES
  snl/SNLAttributes.cpp
  snl/SNLRTLInfos.cpp
  snl/SNLDesignObject.cpp
  snl/SNLNet.cpp snl/SNLBusNet.cpp
  snl/SNLBitNet.cpp snl/SNLScalarNet.cpp snl/SNLBusNetBit.cpp
  snl/SNLNetComponent.cpp
  snl/SNLTerm.cpp snl/SNLBitTerm.cpp 
  snl/SNLBusTerm.cpp snl/SNLBundleTerm.cpp snl/SNLScalarTerm.cpp
  snl/SNLBusTermBit.cpp
  snl/SNLInstance.cpp snl/SNLInstTerm.cpp
  snl/SNLDesign.cpp
  snl/SNLSharedPath.cpp snl/SNLPath.cpp
  snl/SNLParameter.cpp snl/SNLInstParameter.cpp
  snl/SNLClockGating.cpp
  snl/SNLUtils.cpp
  snl/SNLOccurrence.cpp
  snl/SNLEquipotential.cpp
  snl/SNLUniquifier.cpp
)

set(PNL_SOURCES
  pnl/PNLDesign.cpp
  pnl/PNLInstance.cpp
  pnl/PNLDesignObject.cpp
  pnl/PNLNet.cpp
  pnl/PNLNetComponent.cpp
  pnl/PNLTerm.cpp
  pnl/PNLBitTerm.cpp
  pnl/PNLBitNet.cpp
  pnl/PNLInstTerm.cpp
  pnl/PNLScalarNet.cpp
  pnl/PNLScalarTerm.cpp
  pnl/PNLOrientation.cpp
  pnl/PNLSite.cpp
  pnl/PNLTechnology.cpp
)

set(DECORATOR_SOURCES
  decorators/NLLibraryTruthTables.cpp
  decorators/SNLDesignModeling.cpp
  decorators/NLBitDependencies.cpp
)

set(HEADERS
  core/NLContentHash.h
  core/NLName.h
  core/NLID.h
  core/NLObject.h
  core/NLLibrary.h
  core/NLUniverse.h core/NLDB.h
  core/NLClock.h core/NLReset.h
  core/NLDesign.h
  core/NLException.h
  decorators/NLLibraryTruthTables.h
  snl/SNLAttributes.h
  snl/SNLRTLInfos.h
  snl/SNLDesign.h
  snl/SNLDesignObject.h
  snl/SNLNet.h snl/SNLBusNet.h
  snl/SNLBitNet.h snl/SNLScalarNet.h snl/SNLBusNetBit.h
  snl/SNLNetComponent.h
  snl/SNLTerm.h snl/SNLBitTerm.h 
  snl/SNLBusTerm.h snl/SNLBundleTerm.h snl/SNLScalarTerm.h
  snl/SNLBusTermBit.h
  snl/SNLInstance.h snl/SNLInstTerm.h
  snl/SNLSharedPath.h snl/SNLPath.h
  snl/SNLParameter.h snl/SNLInstParameter.h
  snl/SNLClockGating.h
  snl/SNLUtils.h
  snl/SNLOccurrence.h
  snl/SNLEquipotential.h
  snl/SNLExceptions.h
  pnl/PNLPoint.h
  pnl/PNLDesign.h
  pnl/PNLInstance.h
  pnl/PNLDesignObject.h
  pnl/PNLNet.h
  pnl/PNLNetComponent.h
  pnl/PNLTerm.h
  pnl/PNLBitTerm.h
  pnl/PNLBitNet.h
  pnl/PNLInstTerm.h
  pnl/PNLScalarNet.h
  pnl/PNLScalarTerm.h
  pnl/PNLOrientation.h
  pnl/PNLTransform.h
  pnl/PNLBox.h
  pnl/PNLSite.h
  pnl/PNLTechnology.h
  decorators/SNLDesignModeling.h
  decorators/SNLTruthTable.h
  decorators/NLBitVecDynamic.h
  decorators/NLBitDependencies.h
)

add_library(naja_nl SHARED
  ${NL_SOURCES} ${SNL_SOURCES} ${PNL_SOURCES}
  ${DECORATOR_SOURCES}
)

target_link_libraries(naja_nl PUBLIC naja_core)
target_include_directories(naja_nl PRIVATE ${PROJECT_SOURCE_DIR}/thirdparty/sha256)
target_compile_options(naja_nl PRIVATE ${NAJA_CXX_WARNINGS}) 

target_include_directories(naja_nl SYSTEM BEFORE PUBLIC ${Boost_INCLUDE_DIR})
target_include_directories(naja_nl PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/core
  ${CMAKE_CURRENT_SOURCE_DIR}/snl
  ${CMAKE_CURRENT_SOURCE_DIR}/pnl
  ${CMAKE_CURRENT_SOURCE_DIR}/decorators
)
target_include_directories(naja_nl PUBLIC ${SPDLOG_DIR})

if(BUILD_NAJA_PYTHON)
  install(TARGETS naja_nl
    LIBRARY DESTINATION najaeda
    RUNTIME DESTINATION najaeda
  )
else(BUILD_NAJA_PYTHON)
  set_target_properties(naja_nl PROPERTIES PUBLIC_HEADER "${HEADERS}")
  install(TARGETS naja_nl
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif(BUILD_NAJA_PYTHON)
