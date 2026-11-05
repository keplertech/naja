// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <iostream>
#include <sstream>
#include <type_traits>

#define PY_SSIZE_T_CLEAN /* Make "s#" use Py_ssize_t rather than int. */
#include <Python.h>

#include "NLException.h"
#include "PyNetlistState.h"
#include "NLID.h"
#include "NajaPythonProperty.h"
#include "PyNLID.h"

namespace PYNAJA {

static inline void setError(const std::string& reason) {
  //Mabybe create a custom error type in the future ?
  PyErr_SetString(PyExc_RuntimeError, reason.c_str());
}

static inline int warnDeprecatedName(const char* oldName, const char* newName) {
  std::string message = std::string(oldName) + "() is deprecated and will be removed in next release, use " + newName + "() instead";
  return PyErr_WarnEx(PyExc_DeprecationWarning, message.c_str(), 2);
}

// Deprecated alias forwarding to IMPL after emitting a DeprecationWarning.
#define DEFINE_DEPRECATED_ALIAS(ALIAS, IMPL, OLD_NAME, NEW_NAME) \
  static PyObject* ALIAS(PyObject* self, PyObject* arg) { \
    if (warnDeprecatedName(OLD_NAME, NEW_NAME) < 0) { \
      return nullptr; \
    } \
    return IMPL(self, arg); \
  }

//LCOV_EXCL_START
//Can be used to debug the type of a PyObject
static inline std::string getStringForPyObject(PyObject* obj) {
  if (PyUnicode_Check(obj)) {
    return PyUnicode_AsUTF8(obj);
  }
  if (PyLong_Check(obj)) {
    return std::to_string(PyLong_AsLong(obj));
  }
  if (PyFloat_Check(obj)) {
    return std::to_string(PyFloat_AsDouble(obj));
  }
  if (PyBool_Check(obj)) {
    return PyLong_AsLong(obj) ? "True" : "False";
  }
  if (PyBytes_Check(obj)) {
    return PyBytes_AsString(obj);
  }
  if (PyByteArray_Check(obj)) {
    return PyByteArray_AsString(obj);
  }
  if (PyList_Check(obj)) {
    std::ostringstream oss;
    oss << "[";
    for (Py_ssize_t i = 0; i < PyList_Size(obj); ++i) {
      if (i > 0) oss << ", ";
      oss << getStringForPyObject(PyList_GetItem(obj, i));
    }
    oss << "]";
    return oss.str();
  }
  if (PyTuple_Check(obj)) {
    std::ostringstream oss;
    oss << "(";
    for (Py_ssize_t i = 0; i < PyTuple_Size(obj); ++i) {
      if (i > 0) oss << ", ";
      oss << getStringForPyObject(PyTuple_GetItem(obj, i));
    }
    oss << ")";
    return oss.str();
  }
  if (PyDict_Check(obj)) {
    std::ostringstream oss;
    oss << "{";
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(obj, &pos, &key, &value)) {
      if (pos > 0) oss << ", ";
      oss << getStringForPyObject(key) << ": " << getStringForPyObject(value);
    }
    oss << "}";
    return oss.str();
  }
  return "<unknown>";
}
//LCOV_EXCL_STOP

template <typename T>
PyObject* richCompare(T left, T right, int op) {
  if ((op == Py_LT) and (left <  right)) Py_RETURN_TRUE;
  if ((op == Py_LE) and (left <= right)) Py_RETURN_TRUE;
  if ((op == Py_EQ) and (left == right)) Py_RETURN_TRUE;
  if ((op == Py_NE) and (left != right)) Py_RETURN_TRUE;
  if ((op == Py_GT) and (left >  right)) Py_RETURN_TRUE;
  if ((op == Py_GE) and (left >= right)) Py_RETURN_TRUE;
  Py_RETURN_FALSE; 
}

template <typename T>
PyObject* toPyLong(T value) {
  if constexpr (std::is_enum_v<T>) {
    using Underlying = std::underlying_type_t<T>;
    return toPyLong(static_cast<Underlying>(value));
  } else if constexpr (std::is_signed_v<T>) {
    return PyLong_FromLongLong(static_cast<long long>(value));
  } else {
    return PyLong_FromUnsignedLongLong(static_cast<unsigned long long>(value));
  }
}

inline PyObject* toPyNLID(const naja::NL::NLID& id) {
  return PyNLID_Link(id);
}

inline void combinePyHash(Py_uhash_t& seed, Py_uhash_t value) {
  seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
}

inline Py_uhash_t hashString(const std::string& value) {
  Py_uhash_t hash = 1469598103934665603ULL;
  for (unsigned char c: value) {
    hash ^= c;
    hash *= 1099511628211ULL;
  }
  return hash;
}

inline Py_uhash_t hashNLID(const naja::NL::NLID& id) {
  Py_uhash_t seed = 0;
  combinePyHash(seed, static_cast<Py_uhash_t>(id.type_));
  combinePyHash(seed, static_cast<Py_uhash_t>(id.dbID_));
  combinePyHash(seed, static_cast<Py_uhash_t>(id.libraryID_));
  combinePyHash(seed, static_cast<Py_uhash_t>(id.designID_));
  combinePyHash(seed, static_cast<Py_uhash_t>(id.designObjectID_));
  combinePyHash(seed, static_cast<Py_uhash_t>(id.instanceID_));
  combinePyHash(seed, static_cast<Py_uhash_t>(id.bit_));
  return seed;
}

inline Py_hash_t finishPyHash(Py_uhash_t hash) {
  auto pyHash = static_cast<Py_hash_t>(hash);
  return pyHash == -1 ? -2 : pyHash;
}

}

// This macro must be redefined in derived classes.
// Example : baseOject_.object_
#define ACCESS_OBJECT object_
#define ACCESS_CLASS(_pyObject)  _pyObject

#define TRY try {

#define NLCATCH \
  } catch (const naja::NL::NLException& e) { \
    std::ostringstream error; \
    error << "Naja NL error: " << e.getReason(); \
    /* << "\n" << e.trace().to_string(); */ \
    setError(error.str()); \
    return nullptr; \
  } catch (const std::exception& e) {                   \
    setError("Exception " + std::string(e.what()));     \
    return nullptr;                                     \
  } catch (...) {                                       \
    setError("Unknown exception");                      \
    return nullptr;                                     \
  }

#define DirectReprMethod(PY_FUNC_NAME,PY_SELF_TYPE,SELF_TYPE)                                   \
  static PyObject* PY_FUNC_NAME(PY_SELF_TYPE *self ) {                                          \
    if (not self->ACCESS_OBJECT) {                                                              \
      std::ostringstream repr;                                                                  \
      repr << "<" #PY_SELF_TYPE " [" << (void*)self << " <-> nullptr] unbound>";                \
      return PyUnicode_FromString(repr.str().c_str());                                          \
    }                                                                                           \
    SELF_TYPE* object = dynamic_cast<SELF_TYPE*>(self->ACCESS_OBJECT);                          \
    if (not object) {                                                                           \
      return PyUnicode_FromString("<PyObject invalid dynamic_cast>");                           \
    }                                                                                           \
    std::ostringstream repr;                                                                    \
    repr << "[" << (void*)self << "<->" << (void*)object << " " << object->getString() << "]";  \
    return PyUnicode_FromString(repr.str().c_str() );                                           \
  }

#define DirectStrMethod(PY_FUNC_NAME,PY_SELF_TYPE,SELF_TYPE)                      \
  static PyObject* PY_FUNC_NAME(PY_SELF_TYPE *self) {                             \
    if (not self->ACCESS_OBJECT) {                                                \
      std::ostringstream repr;                                                    \
      repr << "<" #PY_SELF_TYPE " [" << (void*)self << " <-> nullptr] unbound>";  \
      return PyUnicode_FromString(repr.str().c_str());                            \
    }                                                                             \
    SELF_TYPE* object = dynamic_cast<SELF_TYPE*>(self->ACCESS_OBJECT);            \
    if (not object)                                                               \
      return PyUnicode_FromString("<PyObject invalid dynamic_cast>" );            \
                                                                                  \
    return PyUnicode_FromString(object->getString().c_str() );                    \
  }

#define DirectCmpByNLIDMethod(PY_FUNC_NAME, PY_SELF_TYPE) \
  static PyObject* PY_FUNC_NAME(PY_SELF_TYPE* self, PyObject* other, int op) { \
    if (not (PyObject_TypeCheck(self, Py_TYPE(other)) or PyObject_TypeCheck(other, Py_TYPE(self)))) Py_RETURN_FALSE; \
    PY_SELF_TYPE* otherPyObject = (PY_SELF_TYPE*)other; \
    auto id = self->ACCESS_OBJECT->getNLID(); \
    auto otherID = otherPyObject->ACCESS_OBJECT->getNLID(); \
    return richCompare(id, otherID, op); \
  }

#define DirectCmpByPtrMethod(PY_FUNC_NAME, PY_SELF_TYPE) \
  static PyObject* PY_FUNC_NAME(PY_SELF_TYPE* self, PyObject* other, int op) { \
    if (not (PyObject_TypeCheck(self, Py_TYPE(other)) or PyObject_TypeCheck(other, Py_TYPE(self)))) Py_RETURN_FALSE; \
    PY_SELF_TYPE* otherPyObject = (PY_SELF_TYPE*)other; \
    auto selfObject = self->ACCESS_OBJECT; \
    auto otherObject = otherPyObject->ACCESS_OBJECT; \
    return richCompare(selfObject, otherObject, op); \
  }

#define DirectCmpByObjectMethod(PY_FUNC_NAME, PY_SELF_TYPE) \
  static PyObject* PY_FUNC_NAME(PY_SELF_TYPE* self, PyObject* other, int op) { \
    if (not (PyObject_TypeCheck(self, Py_TYPE(other)) or PyObject_TypeCheck(other, Py_TYPE(self)))) Py_RETURN_FALSE; \
    PY_SELF_TYPE* otherPyObject = (PY_SELF_TYPE*)other; \
    auto selfObject = self->ACCESS_OBJECT; \
    auto otherObject = otherPyObject->ACCESS_OBJECT; \
    return richCompare(*selfObject, *otherObject, op); \
  }
     
#define DirectHashByNLIDMethod(PY_FUNC_NAME,PY_SELF_TYPE)                    \
  static Py_hash_t PY_FUNC_NAME(PY_SELF_TYPE *self) {                        \
    if (not self->ACCESS_OBJECT) {                                           \
      setError("Attempt to hash an unbound object");                         \
      return -1;                                                             \
    }                                                                        \
    return PYNAJA::finishPyHash(                                             \
      PYNAJA::hashNLID(self->ACCESS_OBJECT->getNLID()));                     \
  }

#define DirectGetNumericMethod(PY_FUNC_NAME, FUNC_NAME, PY_SELF_TYPE, SELF_TYPE) \
  static PyObject* PY_FUNC_NAME(PY_SELF_TYPE* self, PyObject *args) { \
    GENERIC_METHOD_HEAD(SELF_TYPE, #FUNC_NAME"()") \
    return PYNAJA::toPyLong(selfObject->FUNC_NAME()); \
  }

#define DirectGetNLIDMethod(PY_FUNC_NAME, PY_SELF_TYPE, SELF_TYPE) \
  static PyObject* PY_FUNC_NAME(PY_SELF_TYPE* self) { \
    GENERIC_METHOD_HEAD(SELF_TYPE, "getNLID()") \
    return PYNAJA::toPyNLID(selfObject->getNLID()); \
  }

// -------------------------------------------------------------------
// Attribute Macro For DBo Deletion.

# define  DBoDestroyAttribute(PY_FUNC_NAME,PY_SELF_TYPE)                                                     \
  static PyObject* PY_FUNC_NAME ( PY_SELF_TYPE *self )                                                       \
  {                                                                                                          \
    TRY                     \
    if (self->ACCESS_OBJECT == NULL) {                                                                       \
      std::ostringstream  message;                                                                           \
      message << "applying a destroy() to a Python object with no Hurricane object attached";                \
      PyErr_SetString( PyExc_RuntimeError, message.str().c_str() );                                                  \
      return NULL;                                                                                           \
    }                                                                                                        \
    naja::NajaPythonProperty* proxy = static_cast<naja::NajaPythonProperty*>                                     \
                           ( self->ACCESS_OBJECT->getProperty( naja::NajaPythonProperty::getPropertyName() ) );\
    if (proxy == NULL) {                                                                                     \
      std::ostringstream  message;                                                                           \
      message << "Trying to destroy() a Hurricane object of with no Proxy attached ";                        \
      PyErr_SetString( PyExc_RuntimeError, message.str().c_str() );                                                  \
      return NULL;                                                                                           \
    }                                                                                                        \
    self->ACCESS_OBJECT->destroy();                                                                          \
    self->ACCESS_OBJECT = NULL;                                                                              \
    NLCATCH                                                                                                 \
    Py_RETURN_NONE;                                                                                          \
  }

#define DirectDeallocMethod(TYPE) \
  static void Py##TYPE##_DeAlloc(Py##TYPE* self) { \
    if (self->ACCESS_OBJECT) { \
      delete self->ACCESS_OBJECT; \
    } \
    PyObject_DEL(self); \
  }

#define DBoDeallocMethod(SELF_TYPE)                                                         \
  static void Py##SELF_TYPE##_DeAlloc ( Py##SELF_TYPE *self )                               \
  {                                                                                         \
    if ( self->ACCESS_OBJECT != NULL ) {                                                    \
        naja::NajaPythonProperty* proxy = static_cast<naja::NajaPythonProperty*>                \
        ( self->ACCESS_OBJECT->getProperty ( naja::NajaPythonProperty::getPropertyName() ) ); \
        if (proxy == NULL) {                                                                \
          std::ostringstream  message;                                                      \
          message << "deleting a Python object with no Proxy attached ";                    \
          PyErr_SetString ( PyExc_RuntimeError, message.str().c_str() );                    \
        }                                                                                   \
        self->ACCESS_OBJECT->remove ( proxy );                                              \
    }                                                                                       \
    PyObject_DEL ( self );                                                                  \
  }

#define ManagedTypeDeallocMethod(SELF_TYPE) \
  static void Py##SELF_TYPE##_DeAlloc(Py##SELF_TYPE *self) { \
    if (self->ACCESS_OBJECT) { \
      delete self->ACCESS_OBJECT; \
    } \
    PyObject_DEL(self); \
  }

#define PyTypeNLAbstractObjectWithNLIDLinkPyType(SELF_TYPE) \
  DirectReprMethod(Py##SELF_TYPE##_Repr, Py##SELF_TYPE, SELF_TYPE) \
  DirectStrMethod(Py##SELF_TYPE##_Str, Py##SELF_TYPE, SELF_TYPE) \
  DirectCmpByNLIDMethod(Py##SELF_TYPE##_Cmp, Py##SELF_TYPE) \
  DirectHashByNLIDMethod(Py##SELF_TYPE##_Hash, Py##SELF_TYPE) \
  extern void Py##SELF_TYPE##_LinkPyType() { \
    PyType##SELF_TYPE.tp_richcompare = (richcmpfunc)Py##SELF_TYPE##_Cmp; \
    PyType##SELF_TYPE.tp_repr = (reprfunc)Py##SELF_TYPE##_Repr; \
    PyType##SELF_TYPE.tp_str = (reprfunc)Py##SELF_TYPE##_Str; \
    PyType##SELF_TYPE.tp_hash = (hashfunc)Py##SELF_TYPE##_Hash; \
    PyType##SELF_TYPE.tp_methods = Py##SELF_TYPE##_Methods; \
  }

#define PyTypeNLFinalObjectWithNLIDLinkPyType(SELF_TYPE) \
  DirectReprMethod(Py##SELF_TYPE##_Repr, Py##SELF_TYPE, SELF_TYPE) \
  DirectStrMethod(Py##SELF_TYPE##_Str, Py##SELF_TYPE, SELF_TYPE) \
  DirectCmpByNLIDMethod(Py##SELF_TYPE##_Cmp, Py##SELF_TYPE) \
  DirectHashByNLIDMethod(Py##SELF_TYPE##_Hash, Py##SELF_TYPE) \
  extern void Py##SELF_TYPE##_LinkPyType() { \
    PyType##SELF_TYPE.tp_dealloc = (destructor) Py##SELF_TYPE##_DeAlloc; \
    PyType##SELF_TYPE.tp_richcompare = (richcmpfunc)Py##SELF_TYPE##_Cmp; \
    PyType##SELF_TYPE.tp_repr = (reprfunc)Py##SELF_TYPE##_Repr; \
    PyType##SELF_TYPE.tp_str = (reprfunc)Py##SELF_TYPE##_Str; \
    PyType##SELF_TYPE.tp_hash = (hashfunc)Py##SELF_TYPE##_Hash; \
    PyType##SELF_TYPE.tp_methods = Py##SELF_TYPE##_Methods; \
  }

#define PyTypeNLObjectWithoutNLIDLinkPyType(SELF_TYPE) \
  DirectReprMethod(Py##SELF_TYPE##_Repr, Py##SELF_TYPE, SELF_TYPE) \
  DirectStrMethod (Py##SELF_TYPE##_Str, Py##SELF_TYPE, SELF_TYPE) \
  DirectCmpByPtrMethod (Py##SELF_TYPE##_Cmp,  Py##SELF_TYPE) \
  extern void  Py##SELF_TYPE##_LinkPyType() { \
    PyType##SELF_TYPE.tp_dealloc = (destructor) Py##SELF_TYPE##_DeAlloc; \
    PyType##SELF_TYPE.tp_richcompare = (richcmpfunc) Py##SELF_TYPE##_Cmp; \
    PyType##SELF_TYPE.tp_repr = (reprfunc)Py##SELF_TYPE##_Repr; \
    PyType##SELF_TYPE.tp_str = (reprfunc)Py##SELF_TYPE##_Str; \
    PyType##SELF_TYPE.tp_hash = PyObject_HashNotImplemented; \
    PyType##SELF_TYPE.tp_methods = Py##SELF_TYPE##_Methods; \
  }

#define PyTypeManagedNLObjectWithoutNLIDLinkPyType(SELF_TYPE) \
  DirectReprMethod(Py##SELF_TYPE##_Repr, Py##SELF_TYPE, SELF_TYPE) \
  DirectStrMethod (Py##SELF_TYPE##_Str, Py##SELF_TYPE, SELF_TYPE) \
  DirectCmpByObjectMethod (Py##SELF_TYPE##_Cmp,  Py##SELF_TYPE) \
  extern void  Py##SELF_TYPE##_LinkPyType() { \
    PyType##SELF_TYPE.tp_dealloc = (destructor) Py##SELF_TYPE##_DeAlloc; \
    PyType##SELF_TYPE.tp_richcompare = (richcmpfunc) Py##SELF_TYPE##_Cmp; \
    PyType##SELF_TYPE.tp_repr = (reprfunc)Py##SELF_TYPE##_Repr; \
    PyType##SELF_TYPE.tp_str = (reprfunc)Py##SELF_TYPE##_Str; \
    PyType##SELF_TYPE.tp_hash = PyObject_HashNotImplemented; \
    PyType##SELF_TYPE.tp_init = (initproc)Py##SELF_TYPE##_Init; \
    PyType##SELF_TYPE.tp_methods = Py##SELF_TYPE##_Methods; \
  }

#define PyTypeManagedNLObjectWithoutNLIDLinkPyTypeWithHash(SELF_TYPE, HASH_FUNC) \
  DirectReprMethod(Py##SELF_TYPE##_Repr, Py##SELF_TYPE, SELF_TYPE) \
  DirectStrMethod (Py##SELF_TYPE##_Str, Py##SELF_TYPE, SELF_TYPE) \
  DirectCmpByObjectMethod (Py##SELF_TYPE##_Cmp,  Py##SELF_TYPE) \
  extern void  Py##SELF_TYPE##_LinkPyType() { \
    PyType##SELF_TYPE.tp_dealloc = (destructor) Py##SELF_TYPE##_DeAlloc; \
    PyType##SELF_TYPE.tp_richcompare = (richcmpfunc) Py##SELF_TYPE##_Cmp; \
    PyType##SELF_TYPE.tp_repr = (reprfunc)Py##SELF_TYPE##_Repr; \
    PyType##SELF_TYPE.tp_str = (reprfunc)Py##SELF_TYPE##_Str; \
    PyType##SELF_TYPE.tp_hash = (hashfunc)HASH_FUNC; \
    PyType##SELF_TYPE.tp_init = (initproc)Py##SELF_TYPE##_Init; \
    PyType##SELF_TYPE.tp_methods = Py##SELF_TYPE##_Methods; \
  }

#define ManagedTypeLinkCreateMethod(SELF_TYPE) \
  PyObject* Py##SELF_TYPE##_Link(const SELF_TYPE& object) { \
    SELF_TYPE* newObject = new SELF_TYPE(object); \
    Py##SELF_TYPE* pyObject = PyObject_NEW(Py##SELF_TYPE, &PyType##SELF_TYPE); \
    pyObject->ACCESS_OBJECT = newObject; \
    return (PyObject*)pyObject; \
  }

#define DBoLinkCreateMethod(SELF_TYPE)                                                      \
  PyObject* Py##SELF_TYPE##_Link ( SELF_TYPE* object ) {                                    \
    if ( object == NULL ) {                                                                 \
      Py_RETURN_NONE;                                                                       \
    }                                                                                       \
    Py##SELF_TYPE* pyObject = NULL;                                                         \
    TRY                                                                                     \
    naja::NajaPythonProperty* proxy = static_cast<naja::NajaPythonProperty*>                    \
      ( object->getProperty ( naja::NajaPythonProperty::getPropertyName() ) );                \
    if ( proxy == NULL ) {                                                                  \
      pyObject = PyObject_NEW(Py##SELF_TYPE, &PyType##SELF_TYPE);                           \
      if (pyObject == NULL) { return NULL; }                                                \
      proxy = naja::NajaPythonProperty::create ( (void*)pyObject );                           \
      CHECK_OFFSET ( pyObject, SELF_TYPE )                                                  \
      pyObject->ACCESS_OBJECT = object;                                                     \
      object->put ( proxy );                                                                \
    naja::NajaPythonProperty* proxy = static_cast<naja::NajaPythonProperty*>                    \
    ( pyObject->ACCESS_OBJECT->getProperty ( naja::NajaPythonProperty::getPropertyName() ) ); \
    assert(proxy != nullptr);                                                               \
    } else {                                                                                \
      pyObject = (Py##SELF_TYPE*)proxy->getShadow ();                                       \
      Py_INCREF ( ACCESS_CLASS(pyObject) );                                                 \
    }                                                                                       \
    NLCATCH                                                                                \
    return ( (PyObject*)pyObject );                                                         \
  }

#define GetObjectMethod(SELF_TYPE, OBJECT_TYPE, METHOD) \
  static PyObject* Py##SELF_TYPE##_##METHOD(Py##SELF_TYPE* self) { \
    METHOD_HEAD("SELF_TYPE.##METHOD##()") \
    return Py##OBJECT_TYPE##_Link(selfObject->METHOD()); \
  }

#define GetObjectByName(SELF_TYPE, OBJECT_TYPE, METHOD) \
  static PyObject* Py##SELF_TYPE##_##METHOD(Py##SELF_TYPE* self, PyObject* args) { \
    OBJECT_TYPE* obj = nullptr; \
    METHOD_HEAD("SELF_TYPE.METHOD()") \
    char* name = NULL; \
    if (PyArg_ParseTuple(args, "s:SELF_TYPE.METHOD", &name)) { \
      TRY \
      obj = selfObject->METHOD(NLName(name)); \
      NLCATCH \
    } else { \
      setError("invalid number of parameters for METHOD."); \
      return nullptr; \
    } \
    return Py##OBJECT_TYPE##_Link(obj); \
  }

#define GetObjectByIndex(SELF_TYPE, OBJECT_TYPE, METHOD) \
  static PyObject* Py##SELF_TYPE##_get##METHOD(Py##SELF_TYPE* self, PyObject* args) { \
    OBJECT_TYPE* obj = nullptr; \
    METHOD_HEAD(#SELF_TYPE".get"#OBJECT_TYPE"()") \
    int index = 0; \
    if (PyArg_ParseTuple(args, "i:"#SELF_TYPE".get"#METHOD, &index)) { \
      TRY \
      obj = selfObject->get##METHOD(index); \
      NLCATCH \
    } else { \
      setError("invalid number of parameters for get"#METHOD"."); \
      return nullptr; \
    } \
    return Py##OBJECT_TYPE##_Link(obj); \
  }

#define GetNameMethod(SELF_TYPE) \
  static PyObject* Py##SELF_TYPE##_getName(Py##SELF_TYPE* self) { \
    METHOD_HEAD(#SELF_TYPE".getName()") \
    return PyUnicode_FromString(selfObject->getName().getString().c_str()); \
  }

#define SetNameMethod(SELF_TYPE) \
  static PyObject* Py##SELF_TYPE##_setName(Py##SELF_TYPE* self, PyObject* arg) { \
    METHOD_HEAD(#SELF_TYPE".setName()") \
    if (not PyUnicode_Check(arg)) { \
      setError(#SELF_TYPE".setName() expects a string as argument"); \
      return nullptr; \
    } \
    selfObject->setName(NLName(PyUnicode_AsUTF8(arg))); \
    Py_RETURN_NONE; \
  }

#define GetStringAttribute(SELF_TYPE, METHOD) \
  static PyObject* PySNL##SELF_TYPE##_##METHOD(PySNL##SELF_TYPE* self) { \
    METHOD_HEAD("SNL"#SELF_TYPE"."#METHOD"()") \
    TRY \
    return PyUnicode_FromString(selfObject->METHOD().c_str()); \
    NLCATCH \
    return nullptr; \
  }

#define GetBoolAttribute(SELF_TYPE, METHOD) \
  static PyObject* Py##SELF_TYPE##_##METHOD(Py##SELF_TYPE* self) { \
    METHOD_HEAD(#SELF_TYPE"."#METHOD"()") \
    TRY \
    if (selfObject->METHOD()) Py_RETURN_TRUE; \
    Py_RETURN_FALSE; \
    NLCATCH \
  }

#define GetSizetAttribute(SELF_TYPE, METHOD) \
  static PyObject* Py##SELF_TYPE##_##METHOD(Py##SELF_TYPE* self) { \
    METHOD_HEAD(#SELF_TYPE"."#METHOD"()") \
    return Py_BuildValue("n", selfObject->METHOD()); \
  }

#define GetBoolAttributeWithFunction(SELF_TYPE, METHOD, FUNCTION) \
  static PyObject* Py##SELF_TYPE##_##METHOD(Py##SELF_TYPE* self) { \
    METHOD_HEAD(#SELF_TYPE"."#METHOD"()") \
    TRY \
    if (FUNCTION(selfObject)) Py_RETURN_TRUE; \
    Py_RETURN_FALSE; \
    NLCATCH \
  }

#define LoadObjectConstant(DICTIONARY, CONSTANT_VALUE, CONSTANT_NAME)  \
 constant = PyLong_FromLong((long)CONSTANT_VALUE);                  \
 PyDict_SetItemString(DICTIONARY, CONSTANT_NAME, constant);         \
 Py_DECREF(constant);

#define PYTYPE_READY(TYPE) \
  if (trackNetlistMethods(&PyType##TYPE) < 0 || PyType_Ready(&PyType##TYPE) < 0) { \
    NAJA_LOG_ERROR("Failed to initialize <Py" #TYPE ">."); \
    return nullptr; \
  }

#define PYTYPE_READY_SUB(TYPE, TYPE_BASE) \
  PyType##TYPE.tp_base = &PyType##TYPE_BASE; \
  if (trackNetlistMethods(&PyType##TYPE) < 0 || PyType_Ready(&PyType##TYPE) < 0) { \
    NAJA_LOG_ERROR("Failed to initialize <Py" #TYPE ">."); \
    return nullptr; \
  }

#define PyTypeObjectDefinitions(SELF_TYPE) \
  PyTypeObject PyType##SELF_TYPE = { \
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0) \
    .tp_name = #SELF_TYPE, \
    .tp_basicsize = sizeof(Py##SELF_TYPE), \
    .tp_itemsize = 0, \
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, \
    .tp_doc = PyDoc_STR("#SELF_TYPE objects"), \
    .tp_new = PyType_GenericNew, \
  };

#define PyTypeInheritedObjectDefinitions(SELF_TYPE, SUPER_TYPE) \
  PyTypeObject PyType##SELF_TYPE = { \
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0) \
    .tp_name = #SELF_TYPE, \
    .tp_basicsize = sizeof(Py##SELF_TYPE), \
    .tp_itemsize = 0, \
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, \
    .tp_doc = PyDoc_STR("#SELF_TYPE objects"), \
    .tp_base = &PyType##SUPER_TYPE, \
    .tp_new = PyType_GenericNew, \
  };

#define PyContainerMethodsForNonPointers(TYPE, CONTAINER) \
  DirectDeallocMethod(CONTAINER) \
  static PyObject* Py##CONTAINER##IteratorNext(Py##CONTAINER##Iterator* pyIterator) { \
    auto iterator = pyIterator->object_; \
    if (iterator and pyIterator->container_ \
      and pyIterator->container_->object_ \
      and *iterator != pyIterator->container_->object_->end()) { \
      TYPE object = **iterator; \
      ++(*iterator); \
      return Py##TYPE##_Link(object); \
    } \
    return nullptr; \
  } \
  static PyObject* getIterator(Py##CONTAINER* pyContainer) { \
    auto pyIterator = \
      PyObject_New(Py##CONTAINER##Iterator, &PyType##CONTAINER##Iterator); \
    if (not pyIterator) return nullptr; \
    pyIterator->container_ = pyContainer; \
    pyIterator->object_ = new naja::NajaCollection<TYPE>::Iterator(pyContainer->object_->begin()); \
    Py_INCREF(pyContainer); \
    return (PyObject*)pyIterator; \
  } \
  static void Py##CONTAINER##IteratorDeAlloc(Py##CONTAINER##Iterator* pyIterator) { \
    Py_XDECREF(pyIterator->container_); \
    if (pyIterator->object_) delete pyIterator->object_; \
    PyObject_Del(pyIterator); \
  } \
  extern void Py##CONTAINER##_LinkPyType() { \
    PyType##CONTAINER.tp_iter               = (getiterfunc)getIterator; \
    PyType##CONTAINER.tp_dealloc            = (destructor)Py##CONTAINER##_DeAlloc; \
    PyType##CONTAINER##Iterator.tp_dealloc  = (destructor)Py##CONTAINER##IteratorDeAlloc; \
    PyType##CONTAINER##Iterator.tp_iter     = PyObject_SelfIter; \
    PyType##CONTAINER##Iterator.tp_iternext = (iternextfunc)Py##CONTAINER##IteratorNext; \
  }

#define PyContainerMethods(TYPE, CONTAINER) \
  DirectDeallocMethod(CONTAINER) \
  static PyObject* Py##CONTAINER##IteratorNext(Py##CONTAINER##Iterator* pyIterator) { \
    auto iterator = pyIterator->object_; \
    if (iterator and pyIterator->container_ \
      and pyIterator->container_->object_ \
      and *iterator != pyIterator->container_->object_->end()) { \
      auto object = **iterator; \
      ++(*iterator); \
      return Py##TYPE##_Link(object); \
    } \
    return nullptr; \
  } \
  static PyObject* getIterator(Py##CONTAINER* pyContainer) { \
    auto pyIterator = \
      PyObject_New(Py##CONTAINER##Iterator, &PyType##CONTAINER##Iterator); \
    if (not pyIterator) return nullptr; \
    pyIterator->container_ = pyContainer; \
    pyIterator->object_ = new naja::NajaCollection<TYPE*>::Iterator(pyContainer->object_->begin()); \
    Py_INCREF(pyContainer); \
    return (PyObject*)pyIterator; \
  } \
  static void Py##CONTAINER##IteratorDeAlloc(Py##CONTAINER##Iterator* pyIterator) { \
    Py_XDECREF(pyIterator->container_); \
    if (pyIterator->object_) delete pyIterator->object_; \
    PyObject_Del(pyIterator); \
  } \
  extern void Py##CONTAINER##_LinkPyType() { \
    PyType##CONTAINER.tp_iter               = (getiterfunc)getIterator; \
    PyType##CONTAINER.tp_dealloc            = (destructor)Py##CONTAINER##_DeAlloc; \
    PyType##CONTAINER##Iterator.tp_dealloc  = (destructor)Py##CONTAINER##IteratorDeAlloc; \
    PyType##CONTAINER##Iterator.tp_iter     = PyObject_SelfIter; \
    PyType##CONTAINER##Iterator.tp_iternext = (iternextfunc)Py##CONTAINER##IteratorNext; \
  }

#define PyTypeContainerObjectDefinitions(SELF_TYPE) \
  PyTypeObject PyType##SELF_TYPE = { \
      PyVarObject_HEAD_INIT(NULL,0) \
      #SELF_TYPE                      /* tp_name.          */           \
    , sizeof(Py##SELF_TYPE)           /* tp_basicsize.     */           \
    , 0                               /* tp_itemsize.      */           \
    /* methods. */                                                      \
    , 0                               /* tp_dealloc.       */           \
    , 0                               /* tp_print.         */           \
    , 0                               /* tp_getattr.       */           \
    , 0                               /* tp_setattr.       */           \
    , 0                               /* tp_compare.       */           \
    , 0                               /* tp_repr.          */           \
    , 0                               /* tp_as_number.     */           \
    , 0                               /* tp_as_sequence.   */           \
    , 0                               /* tp_as_mapping.    */           \
    , 0                               /* tp_hash.          */           \
    , 0                               /* tp_call.          */           \
    , 0                               /* tp_str            */           \
    , 0                               /* tp_getattro.      */           \
    , 0                               /* tp_setattro.      */           \
    , 0                               /* tp_as_buffer.     */           \
    , Py_TPFLAGS_DEFAULT              /* tp_flags.         */           \
    , "#SELF_TYPE objects"            /* tp_doc.           */           \
  };

#define GENERIC_METHOD_HEAD(SELF_TYPE, function) \
  if (not self->ACCESS_OBJECT) {                                                            \
    setError("Attempt to call " function " on an unbound object");                          \
    return nullptr;                                                                         \
  }                                                                                         \
  SELF_TYPE* selfObject = dynamic_cast<SELF_TYPE*>(self->ACCESS_OBJECT); \
  if (not selfObject) { \
    setError("Invalid dynamic_cast<> while calling " function "");                          \
    return nullptr;                                                                         \
  }

#define GetContainerMethod(TYPE, ITERATED, CONTAINER, GET_OBJECTS) \
  static PyObject* Py##TYPE##_get##GET_OBJECTS(Py##TYPE *self) { \
    METHOD_HEAD(#TYPE ".get" #GET_OBJECTS "()") \
    Py##CONTAINER* pyObjects = nullptr; \
    TRY \
    auto objects = new naja::NajaCollection<ITERATED>(selfObject->get##GET_OBJECTS()); \
    pyObjects = PyObject_NEW(Py##CONTAINER, &PyType##CONTAINER); \
    if (not pyObjects) return nullptr; \
    pyObjects->object_ = objects; \
    NLCATCH \
    return (PyObject*)pyObjects; \
  }

#define HasElementsMethod(TYPE, METHOD, CONTAINER) \
  static PyObject* Py##TYPE##_##METHOD(Py##TYPE *self) { \
    METHOD_HEAD(#TYPE "." #METHOD "()") \
    TRY \
    if (selfObject->CONTAINER().empty()) Py_RETURN_FALSE; \
    Py_RETURN_TRUE; \
    NLCATCH \
    return nullptr; \
  }

#define GetContainerMethodWithMethodName(TYPE, ITERATED, METHOD) \
  static PyObject* PySNL##TYPE##_##METHOD(PySNL##TYPE *self) { \
    METHOD_HEAD("SNL" #TYPE "." #METHOD "()") \
    PySNL##ITERATED##s* pyObjects = nullptr; \
    TRY \
    auto objects = new naja::NajaCollection<SNL##ITERATED*>(selfObject->METHOD()); \
    pyObjects = PyObject_NEW(PySNL##ITERATED##s, &PyTypeSNL##ITERATED##s); \
    if (not pyObjects) return nullptr; \
    pyObjects->object_ = objects; \
    NLCATCH \
    return (PyObject*)pyObjects; \
  }

#define GetDesignModelingRelatedObjects(TYPE, GETTER, OWNER_TYPE) \
  if (IsPy##TYPE(object)) { \
    auto object_o = PY##TYPE##_O(object); \
    TRY \
    auto objects = new naja::NajaCollection<TYPE*>(SNLDesignModeling::GETTER(object_o)); \
    auto pyObjects = PyObject_NEW(Py##TYPE##s, &PyType##TYPE##s); \
    if (not pyObjects) return nullptr; \
    pyObjects->object_ = objects; \
    return (PyObject*)pyObjects; \
    NLCATCH \
  } \
  setError("malformed " #OWNER_TYPE "." #GETTER " method"); \
  return nullptr;
