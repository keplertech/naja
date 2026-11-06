// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PyNLResets.h"

#include "PyInterface.h"
#include "PyNLReset.h"

namespace PYNAJA {

using namespace naja::NL;

PyTypeContainerObjectDefinitions(NLResets)
PyTypeContainerObjectDefinitions(NLResetsIterator)

PyContainerMethods(NLReset, NLResets)

}
