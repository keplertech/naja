// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <Python.h>
#include "NajaCollection.h"
#include "NajaPythonExport.h"

namespace naja::NL {
  class NLReset;
}

namespace PYNAJA {

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::NLReset*>* object_;
} PyNLResets;

typedef struct {
  PyObject_HEAD
  naja::NajaCollection<naja::NL::NLReset*>::Iterator* object_;
  PyNLResets* container_;
} PyNLResetsIterator;

NAJA_PY_EXPORT extern PyTypeObject PyTypeNLResets;
NAJA_PY_EXPORT extern PyTypeObject PyTypeNLResetsIterator;

extern void PyNLResets_LinkPyType();

} /* PYNAJA namespace */
