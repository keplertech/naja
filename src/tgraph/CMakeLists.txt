# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

set(GRAPH_SOURCES
  TClock.cpp
  TClockPropagator.cpp
  TEdge.cpp
  TGraph.cpp
  TLoopBreaker.cpp
  TNode.cpp
  TSNLGraphConstructor.cpp
)

set(GRAPH_HEADERS
  TClock.h
  TClockPropagator.h
  TCaptureEdge.h
  TEdge.h
  TEdgeProxy.h
  TGraph.h
  TLoopBreaker.h
  TNode.h
  TNodeProxy.h
  TSNLGraphConstructor.h
)

add_library(naja_tgraph SHARED ${GRAPH_SOURCES})

# TClock retains SNL terminal identity; the constructor consumes DNL.
target_link_libraries(naja_tgraph PUBLIC naja_nl naja_dnl)

target_compile_options(naja_tgraph BEFORE PRIVATE ${NAJA_CXX_WARNINGS})

target_include_directories(naja_tgraph PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(BUILD_NAJA_PYTHON)
  install(TARGETS naja_tgraph
    LIBRARY DESTINATION najaeda
    RUNTIME DESTINATION najaeda
  )
else(BUILD_NAJA_PYTHON)
  set_target_properties(naja_tgraph PROPERTIES PUBLIC_HEADER "${GRAPH_HEADERS}")
  install(TARGETS naja_tgraph
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif(BUILD_NAJA_PYTHON)
