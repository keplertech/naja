// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLEquipotential.h"
#include "PySNLEquipotentialMode.h"
#include "PySNLEquipotentialEdgeKind.h"
#include "PySNLBitTerm.h"

#include "PyInterface.h"
#include "PySNLNetComponent.h"
#include "PySNLPath.h"
#include "PySNLOccurrence.h"
#include "PySNLOccurrences.h"
#include "PySNLBitTerms.h"

#include "SNLPath.h"
#include "SNLEquipotential.h"
#include "NetlistGraph.h"

namespace PYNAJA {

using namespace naja::NL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLEquipotential, function)

static int PySNLEquipotential_Init(PySNLEquipotential* self, PyObject* args, PyObject* kwargs) {
  SNLEquipotential* equipotential = nullptr;
  PyObject* arg0 = nullptr;
  PyObject* modeObject = nullptr;
  int keepGraph = 0;
  static const char* keywords[] = {"net_component", "mode", "keep_graph", nullptr};

  if (not PyArg_ParseTupleAndKeywords(
      args,
      kwargs,
      "O|Op:SNLEquipotential",
      const_cast<char**>(keywords),
      &arg0,
      &modeObject,
      &keepGraph)) {
    setError("malformed SNLEquipotential create method");
    return -1;
  }
  auto mode = SNLEquipotential::Mode::Standard;
  if (modeObject) {
    if (not PyLong_Check(modeObject)) {
      setError("SNLEquipotential mode must be an SNLEquipotential.Mode");
      return -1;
    }
    switch (PyLong_AsLong(modeObject)) {
      case static_cast<long>(SNLEquipotential::Mode::Standard):
        break;
      case static_cast<long>(SNLEquipotential::Mode::TraverseAssigns):
        mode = SNLEquipotential::Mode::TraverseAssigns;
        break;
      default:
        setError("invalid SNLEquipotential.Mode value");
        return -1;
    }
  }
  if (IsPySNLOccurrence(arg0)) {
    const auto occurrence = PYSNLOccurrence_O(arg0);
    if (not occurrence->isNetComponentOccurrence()) {
      setError("SNLOccurrence passed to SNLEquipotential constructor is not a SNLNetComponentOccurrence");
      return -1;
    }
    equipotential = new SNLEquipotential(*occurrence, SNLEquipotential::Options{mode, bool(keepGraph)});
  } else if (IsPySNLNetComponent(arg0)) {
    equipotential = new SNLEquipotential(PYSNLNetComponent_O(arg0), SNLEquipotential::Options{mode, bool(keepGraph)});
  } else {
    setError("SNLEquipotential create accepts SNLNetComponent or SNLNetComponentOccurrence as only argument");
    return -1;
  }
  self->object_ = equipotential;
  return 0;
}

static PyObject* PySNLEquipotential_dumpDotFile(PySNLEquipotential* self, PyObject* args) {
  char* path = NULL; 
  if (not PyArg_ParseTuple(args, "s:SELF_TYPE.METHOD", &path)) {
    setError("dumpDotFile expact a string as argument");
    return nullptr;
  }
  std::filesystem::path outputPath;
  if (path) {
    outputPath = std::filesystem::path(path);
  }
  if (self->object_->getGraph()) {
    try {
      self->object_->dumpDotFile(outputPath.string());
    } catch (const NLException& error) {
      setError(error.what());
      return nullptr;
    }
    Py_RETURN_NONE;
  }
  auto top = NLUniverse::get()->getTopDesign();
  std::string dotFileName(outputPath.string());
  naja::SnlVisualiser snl(top, true, self->object_);
  snl.process();
  snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
  Py_RETURN_NONE;
}

static PyObject* PySNLEquipotential_getGraph(PySNLEquipotential* self, PyObject*) {
  const auto* graph = self->object_->getGraph();
  if (not graph) { Py_RETURN_NONE; }
  PyObject* nodes = PyList_New(graph->nodes.size());
  if (not nodes) { return nullptr; }
  for (size_t i = 0; i < graph->nodes.size(); ++i) {
    const auto& node = graph->nodes[i];
    PyObject* leaves = PyList_New(0);
    PyObject* terms = PyList_New(0);
    if (not leaves or not terms) {
      Py_XDECREF(leaves); Py_XDECREF(terms); Py_DECREF(nodes);
      return nullptr;
    }
    for (const auto& leaf: node.leaves) {
      PyObject* item = PySNLOccurrence_Link(leaf);
      if (not item or PyList_Append(leaves, item) < 0) {
        Py_XDECREF(item); Py_DECREF(leaves); Py_DECREF(terms); Py_DECREF(nodes);
        return nullptr;
      }
      Py_DECREF(item);
    }
    for (auto term: node.terms) {
      PyObject* item = PySNLBitTerm_Link(term);
      if (not item or PyList_Append(terms, item) < 0) {
        Py_XDECREF(item); Py_DECREF(leaves); Py_DECREF(terms); Py_DECREF(nodes);
        return nullptr;
      }
      Py_DECREF(item);
    }
    PyObject* net = PySNLOccurrence_Link(node.net);
    PyObject* item = net ? Py_BuildValue("(OOO)", net, leaves, terms) : nullptr;
    Py_XDECREF(net); Py_DECREF(leaves); Py_DECREF(terms);
    if (not item) { Py_DECREF(nodes); return nullptr; }
    PyList_SET_ITEM(nodes, i, item);
  }
  PyObject* edges = PyList_New(graph->edges.size());
  if (not edges) { Py_DECREF(nodes); return nullptr; }
  for (size_t i = 0; i < graph->edges.size(); ++i) {
    const auto& edge = graph->edges[i];
    PyObject* crossing = PySNLOccurrence_Link(edge.crossing);
    PyObject* item = crossing ? Py_BuildValue("(iOnn)", static_cast<int>(edge.kind),
      crossing, static_cast<Py_ssize_t>(edge.from), static_cast<Py_ssize_t>(edge.to)) : nullptr;
    Py_XDECREF(crossing);
    if (not item) { Py_DECREF(nodes); Py_DECREF(edges); return nullptr; }
    PyList_SET_ITEM(edges, i, item);
  }
  return Py_BuildValue("(NN)", nodes, edges);
}

GetContainerMethod(SNLEquipotential, SNLBitTerm*, SNLBitTerms, Terms)
GetContainerMethod(SNLEquipotential, SNLOccurrence, SNLOccurrences, InstTermOccurrences)
GetBoolAttribute(SNLEquipotential, isConst0)
GetBoolAttribute(SNLEquipotential, isConst1)
GetBoolAttribute(SNLEquipotential, isConstX)
GetBoolAttribute(SNLEquipotential, isConstZ)

//LCOV_EXCL_START
ManagedTypeLinkCreateMethod(SNLEquipotential) 
//LCOV_EXCL_STOP
ManagedTypeDeallocMethod(SNLEquipotential)

PyMethodDef PySNLEquipotential_Methods[] = {
  { "getGraph", (PyCFunction)PySNLEquipotential_getGraph, METH_NOARGS,
    "Returns retained net nodes and crossings, or None."},
  {  "getTerms", (PyCFunction)PySNLEquipotential_getTerms, METH_NOARGS,
     "Returns the equi top terms."},
  {  "getInstTermOccurrences", (PyCFunction)PySNLEquipotential_getInstTermOccurrences, METH_NOARGS,
     "Returns the equi inst terms."},
  {  "isConst0", (PyCFunction)PySNLEquipotential_isConst0, METH_NOARGS,
     "Returns True if the equipotential is constant 0."},
  {  "isConst1", (PyCFunction)PySNLEquipotential_isConst1, METH_NOARGS,
     "Returns True if the equipotential is constant 1."},
  {  "isConstX", (PyCFunction)PySNLEquipotential_isConstX, METH_NOARGS,
     "Returns True if the equipotential is constant X."},
  {  "isConstZ", (PyCFunction)PySNLEquipotential_isConstZ, METH_NOARGS,
     "Returns True if the equipotential is constant Z."},
  {  "dumpDotFile", (PyCFunction)PySNLEquipotential_dumpDotFile, METH_VARARGS,
     "Dump the dot file."},
  {NULL, NULL, 0, NULL} /* sentinel */
};

PyTypeManagedNLObjectWithoutNLIDLinkPyType(SNLEquipotential)
PyTypeObjectDefinitions(SNLEquipotential)

void PySNLEquipotential_postModuleInit() {
  PySNLEquipotentialEdgeKind_postModuleInit();
  PyDict_SetItemString(
    PyTypeSNLEquipotential.tp_dict,
    "EdgeKind",
    reinterpret_cast<PyObject*>(&PyTypeSNLEquipotentialEdgeKind));
  PySNLEquipotentialMode_postModuleInit();
  PyDict_SetItemString(
    PyTypeSNLEquipotential.tp_dict,
    "Mode",
    reinterpret_cast<PyObject*>(&PyTypeSNLEquipotentialMode));
}

}  // namespace PYNAJA
