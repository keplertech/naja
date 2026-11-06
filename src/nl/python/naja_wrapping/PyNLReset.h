// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <Python.h>
#include "NajaPythonExport.h"

namespace naja::NL {
  class NLReset;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NL::NLReset* object_;
} PyNLReset;

NAJA_PY_EXPORT extern PyTypeObject PyTypeNLReset;

extern PyObject* PyNLReset_Link(naja::NL::NLReset* u);
extern void PyNLReset_LinkPyType();
extern void PyNLReset_postModuleInit();

#define IsPyNLReset(v) (PyObject_TypeCheck(v, &PyTypeNLReset))
#define PYNLReset(v)   ((PyNLReset*)(v))
#define PYNLReset_O(v) (PYNLReset(v)->object_)

} /* PYNAJA namespace */
