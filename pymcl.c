#include "pymcl.h"
#include <stdio.h>

/*******************************************************************************
* pymcl.c                                                                      *
*                                                                              *
* Originally written by Jemtaly                                                *
* Licensed under GPLv3                                                         *
* Released 5 March 2024                                                        *
*                                                                              *
* This file contains the types and functions needed to use mcl from Python 3.  *
*******************************************************************************/

Py_hash_t hash_bytes(const char *string, Py_ssize_t size) {
    // initialize the hash
    Py_uhash_t hash = 14695981039346656037U;
    // iterate over the string
    for (Py_ssize_t i = 0; i < size; i++) {
        hash ^= (Py_uhash_t)string[i];
        hash *= 1099511628211U;
    }
    // check if the hash is invalid
    if (hash == (Py_uhash_t)-1) {
        hash = (Py_uhash_t)-2;
    }
    // return the hash
    return (Py_hash_t)hash;
}

/*******************************************************************************
*                                      G1                                      *
*******************************************************************************/

PyDoc_STRVAR(G1__doc__,
    "Represents an element of the G1 group.\n"
    "\n"
    "Basic usage:\n"
    "\n"
    "G1(s: str) -> G1\n"
    "G1.__str__(self: G1) -> str\n"
    "G1.__add__(self: G1, other: G1) -> G1\n"
    "G1.__sub__(self: G1, other: G1) -> G1\n"
    "G1.__neg__(self: G1) -> G1\n"
    "G1.__mul__(self: G1, other: Fr) -> G1\n"
    "G1.__eq__(self: G1, other: G1) -> bool\n"
    "G1.__ne__(self: G1, other: G1) -> bool\n"
    "G1.__hash__(self: G1) -> int\n"
    "G1.serialize(self: G1) -> bytes\n"
    "G1.deserialize(b: bytes) -> G1\n"
    "G1.hash(b: bytes) -> G1\n"
    "G1.isZero(self: G1) -> bool\n"
    "\n"
    "Most of the basic arithmetic operations apply. Please note that many of them\n"
    "do not make sense between groups, and that not all of these are checked for.");

G1 *G1_create(void) {
    // allocate the object
    G1 *g1_res = (G1 *)G1Type.tp_alloc(&G1Type, 0);
    // check if the object was allocated
    if (!g1_res) {
        PyErr_SetString(PyExc_TypeError, "could not create G1 object");
        return NULL;
    }
    // return the object
    return g1_res;
}

PyObject *G1_new(PyTypeObject *cls, PyObject *args, PyObject *kwds) {
    // required a string
    char *string = NULL;
    Py_ssize_t length;
    if (!PyArg_ParseTuple(args, "|s#", &string, &length)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments, should be a string");
        return NULL;
    }
    // build the result
    G1 *g1_res = G1_create();
    // initialize the G1 with the string
    if (string == NULL) {
        mclBnG1_clear(&g1_res->mcl_g1);
    } else if (mclBnG1_setStr(&g1_res->mcl_g1, string, length, 10)) {
        PyErr_SetString(PyExc_ValueError, "could not parse the string as a G1 element");
        return NULL;
    }
    // return the element
    return (PyObject *)g1_res;
}

void G1_dealloc(PyObject *self) {
    // free the object
    Py_TYPE(self)->tp_free(self);
}

PyObject *G1_from_bytes(PyObject *type, PyObject *args) {
    // required a byte string
    PyObject *bytes;
    if (!PyArg_ParseTuple(args, "O!", &PyBytes_Type, &bytes)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments, should be a bytes object");
        return NULL;
    }
    // build the result
    G1 *g1_res = G1_create();
    // convert the bytes to an element
    Py_ssize_t size = PyBytes_Size(bytes);
    char *string = PyBytes_AsString(bytes);
    mclBnG1_hashAndMapTo(&g1_res->mcl_g1, string, size);
    // return the element
    return (PyObject *)g1_res;
}

PyObject *G1_str(PyObject *self) {
    // cast the argument
    G1 *g1_self = (G1 *)self;
    // get the string from the G1
    char buffer[240];
    Py_ssize_t length = mclBnG1_getStr(buffer, sizeof(buffer), &g1_self->mcl_g1, 10);
    // return the string
    return PyUnicode_FromStringAndSize(buffer, length);
}

PyObject *G1_serialize(PyObject *self) {
    // convert the object to G1
    G1 *g1_self = (G1 *)self;
    // build the buffer
    char buffer[48];
    // convert the G1 to bytes
    Py_ssize_t size = mclBnG1_serialize(buffer, sizeof(buffer), &g1_self->mcl_g1);
    // return the bytes
    return PyBytes_FromStringAndSize(buffer, size);
}

Py_hash_t G1_hash(PyObject *self) {
    // convert the object to G1
    G1 *g1_self = (G1 *)self;
    // build the buffer
    char buffer[48];
    // convert the G1 to bytes
    Py_ssize_t size = mclBnG1_serialize(buffer, sizeof(buffer), &g1_self->mcl_g1);
    // return the hash
    return hash_bytes(buffer, size);
}

PyObject *G1_deserialize(PyObject *type, PyObject *args) {
    // required a byte string
    PyObject *bytes;
    if (!PyArg_ParseTuple(args, "O!", &PyBytes_Type, &bytes)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments, should be a bytes object");
        return NULL;
    }
    // build the result
    G1 *g1_res = G1_create();
    // convert the bytes to an element
    Py_ssize_t size = PyBytes_Size(bytes);
    char *string = PyBytes_AsString(bytes);
    if (!mclBnG1_deserialize(&g1_res->mcl_g1, string, size)) {
        PyErr_SetString(PyExc_ValueError, "could not deserialize the bytes as a G1 element");
        return NULL;
    }
    // return the element
    return (PyObject *)g1_res;
}

PyObject *G1_add(PyObject *lft, PyObject *rgt) {
    // check the type of arguments
    if (!PyObject_TypeCheck(lft, &G1Type) || !PyObject_TypeCheck(rgt, &G1Type)) {
        PyErr_SetString(PyExc_TypeError, "operands must be both G1 elements");
        return NULL;
    }
    // convert both objects to G1
    G1 *g1_lft = (G1 *)lft;
    G1 *g1_rgt = (G1 *)rgt;
    // build the result element and initialize it to the same group as the left element
    G1 *g1_res = G1_create();
    mclBnG1_add(&g1_res->mcl_g1, &g1_lft->mcl_g1, &g1_rgt->mcl_g1);
    return (PyObject *)g1_res;
}

PyObject *G1_sub(PyObject *lft, PyObject *rgt) {
    // check the type of arguments
    if (!PyObject_TypeCheck(lft, &G1Type) || !PyObject_TypeCheck(rgt, &G1Type)) {
        PyErr_SetString(PyExc_TypeError, "operands must be both G1 elements");
        return NULL;
    }
    // convert both objects to G1
    G1 *g1_lft = (G1 *)lft;
    G1 *g1_rgt = (G1 *)rgt;
    // build the result element and initialize it to the same group as the left element
    G1 *g1_res = G1_create();
    mclBnG1_sub(&g1_res->mcl_g1, &g1_lft->mcl_g1, &g1_rgt->mcl_g1);
    return (PyObject *)g1_res;
}

PyObject *G1_neg(PyObject *arg) {
    // convert the object to G1
    G1 *g1_arg = (G1 *)arg;
    // build the result element and initialize it to the same group as the argument
    G1 *g1_res = G1_create();
    mclBnG1_neg(&g1_res->mcl_g1, &g1_arg->mcl_g1);
    return (PyObject *)g1_res;
}

PyObject *G1_mul(PyObject *lft, PyObject *rgt, PyObject *mod) {
    // check the type of arguments
    if (!PyObject_TypeCheck(lft, &G1Type) || !PyObject_TypeCheck(rgt, &FrType)) {
        PyErr_SetString(PyExc_TypeError, "operands must be a G1 element and an Fr element");
        return NULL;
    }
    // convert the objects to G1 and Element
    G1 *g1_lft = (G1 *)lft;
    Fr *fr_rgt = (Fr *)rgt;
    // build the result element and initialize it to the same group as the left element
    G1 *g1_res = G1_create();
    mclBnG1_mul(&g1_res->mcl_g1, &g1_lft->mcl_g1, &fr_rgt->mcl_fr);
    return (PyObject *)g1_res;
}

PyObject *G1_cmp(PyObject *lft, PyObject *rgt, int op) {
    // check the type of arguments
    if (!PyObject_TypeCheck(lft, &G1Type) || !PyObject_TypeCheck(rgt, &G1Type)) {
        PyErr_SetString(PyExc_TypeError, "operands must be both G1 elements");
        return NULL;
    }
    // convert both objects to G1
    G1 *g1_lft = (G1 *)lft;
    G1 *g1_rgt = (G1 *)rgt;
    // compare the elements
    int result = mclBnG1_isEqual(&g1_lft->mcl_g1, &g1_rgt->mcl_g1);
    // return the result
    switch (op) {
    case Py_EQ: if (result) { Py_RETURN_TRUE; } else { Py_RETURN_FALSE; }
    case Py_NE: if (result) { Py_RETURN_FALSE; } else { Py_RETURN_TRUE; }
    default: PyErr_SetString(PyExc_TypeError, "operation not supported"); return NULL;
    }
}

PyObject *G1_isZero(PyObject *arg) {
    // convert the object to G1
    G1 *g1_arg = (G1 *)arg;
    // check if the element is zero
    if (mclBnG1_isZero(&g1_arg->mcl_g1)) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

PyMemberDef G1_members[] = {
    {NULL},
};

PyMethodDef G1_methods[] = {
    {"serialize", (PyCFunction)G1_serialize, METH_NOARGS, "Serializes the element to a byte string."},
    {"deserialize", (PyCFunction)G1_deserialize, METH_VARARGS | METH_CLASS, "Deserializes the element from a byte string."},
    {"isZero", (PyCFunction)G1_isZero, METH_NOARGS, "Checks if the element is the zero element."},
    {"hash", (PyCFunction)G1_from_bytes, METH_VARARGS | METH_CLASS, "Hashes a byte string to a G1 element."},
    {NULL},
};

PyNumberMethods G1_num_meths = {
    G1_add, // binaryfunc nb_add;
    G1_sub, // binaryfunc nb_subtract;
    G1_mul, // binaryfunc nb_multiply;
    0,      // binaryfunc nb_remainder;
    0,      // binaryfunc nb_divmod;
    0,      // ternaryfunc nb_power;
    G1_neg, // unaryfunc nb_negative;
    0,      // unaryfunc nb_positive;
    0,      // unaryfunc nb_absolute;
    0,      // inquiry nb_bool;
    0,      // unaryfunc nb_invert;
    0,      // binaryfunc nb_lshift;
    0,      // binaryfunc nb_rshift;
    0,      // binaryfunc nb_and;
    0,      // binaryfunc nb_xor;
    0,      // binaryfunc nb_or;
    0,      // unaryfunc nb_int;
    0,      // void *nb_reserved;
    0,      // unaryfunc nb_float;
    0,      // binaryfunc nb_inplace_add;
    0,      // binaryfunc nb_inplace_subtract;
    0,      // binaryfunc nb_inplace_multiply;
    0,      // binaryfunc nb_inplace_remainder;
    0,      // ternaryfunc nb_inplace_power;
    0,      // binaryfunc nb_inplace_lshift;
    0,      // binaryfunc nb_inplace_rshift;
    0,      // binaryfunc nb_inplace_and;
    0,      // binaryfunc nb_inplace_xor;
    0,      // binaryfunc nb_inplace_or;
    0,      // binaryfunc nb_floor_divide;
    0,      // binaryfunc nb_true_divide;
    0,      // binaryfunc nb_inplace_floor_divide;
    0,      // binaryfunc nb_inplace_true_divide;
};

PyTypeObject G1Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pymcl.G1",                               /* tp_name */
    sizeof(G1),                               /* tp_basicsize */
    0,                                        /* tp_itemsize */
    G1_dealloc,                               /* tp_dealloc */
    0,                                        /* tp_print */
    0,                                        /* tp_getattr */
    0,                                        /* tp_setattr */
    0,                                        /* tp_compare */
    G1_str,                                   /* tp_repr */
    &G1_num_meths,                            /* tp_as_number */
    0,                                        /* tp_as_sequence */
    0,                                        /* tp_as_mapping */
    G1_hash,                                  /* tp_hash */
    0,                                        /* tp_call */
    G1_str,                                   /* tp_str */
    0,                                        /* tp_getattro */
    0,                                        /* tp_setattro */
    0,                                        /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    G1__doc__,                                /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    G1_cmp,                                   /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    0,                                        /* tp_iter */
    0,                                        /* tp_iternext */
    G1_methods,                               /* tp_methods */
    G1_members,                               /* tp_members */
    0,                                        /* tp_getset */
    0,                                        /* tp_base */
    0,                                        /* tp_dict */
    0,                                        /* tp_descr_get */
    0,                                        /* tp_descr_set */
    0,                                        /* tp_dictoffset */
    0,                                        /* tp_init */
    0,                                        /* tp_alloc */
    G1_new,                                   /* tp_new */
};

/*******************************************************************************
*                                      G2                                      *
*******************************************************************************/

PyDoc_STRVAR(G2__doc__,
    "Represents an element of the G2 group.\n"
    "\n"
    "Basic usage:\n"
    "\n"
    "G2(s: str) -> G2\n"
    "G2.__str__(self: G2) -> str\n"
    "G2.__add__(self: G2, other: G2) -> G2\n"
    "G2.__sub__(self: G2, other: G2) -> G2\n"
    "G2.__neg__(self: G2) -> G2\n"
    "G2.__mul__(self: G2, other: Fr) -> G2\n"
    "G2.__eq__(self: G2, other: G2) -> bool\n"
    "G2.__ne__(self: G2, other: G2) -> bool\n"
    "G2.__hash__(self: G2) -> int\n"
    "G2.serialize(self: G2) -> bytes\n"
    "G2.deserialize(b: bytes) -> G2\n"
    "G2.hash(b: bytes) -> G2\n"
    "G2.isZero(self: G2) -> bool\n"
    "\n"
    "Most of the basic arithmetic operations apply. Please note that many of them\n"
    "do not make sense between groups, and that not all of these are checked for.");

G2 *G2_create(void) {
    // allocate the object
    G2 *g2_res = (G2 *)G2Type.tp_alloc(&G2Type, 0);
    // check if the object was allocated
    if (!g2_res) {
        PyErr_SetString(PyExc_TypeError, "could not create G2 object");
        return NULL;
    }
    // return the object
    return g2_res;
}

PyObject *G2_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    // required a string
    char *string = NULL;
    Py_ssize_t length;
    if (!PyArg_ParseTuple(args, "|s#", &string, &length)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments, should be a string");
        return NULL;
    }
    // build the result
    G2 *g2_res = G2_create();
    // initialize the G2 with the string
    if (string == NULL) {
        mclBnG2_clear(&g2_res->mcl_g2);
    } else if (mclBnG2_setStr(&g2_res->mcl_g2, string, length, 10)) {
        PyErr_SetString(PyExc_ValueError, "could not parse the string as a G2 element");
        return NULL;
    }
    // return the element
    return (PyObject *)g2_res;
}

void G2_dealloc(PyObject *self) {
    // free the object
    Py_TYPE(self)->tp_free(self);
}

PyObject *G2_from_bytes(PyObject *type, PyObject *args) {
    // required a byte string
    PyObject *bytes;
    if (!PyArg_ParseTuple(args, "O!", &PyBytes_Type, &bytes)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments, should be a bytes object");
        return NULL;
    }
    // build the result
    G2 *g2_res = G2_create();
    // convert the bytes to an element
    Py_ssize_t size = PyBytes_Size(bytes);
    char *string = PyBytes_AsString(bytes);
    mclBnG2_hashAndMapTo(&g2_res->mcl_g2, string, size);
    // return the element
    return (PyObject *)g2_res;
}

PyObject *G2_str(PyObject *self) {
    // cast the argument
    G2 *g2_self = (G2 *)self;
    // get the string from the G2
    char buffer[480];
    Py_ssize_t length = mclBnG2_getStr(buffer, sizeof(buffer), &g2_self->mcl_g2, 10);
    // return the string
    return PyUnicode_FromStringAndSize(buffer, length);
}

PyObject *G2_serialize(PyObject *self) {
    // convert the object to G2
    G2 *g2_self = (G2 *)self;
    // build the buffer
    char buffer[96];
    // convert the G2 to bytes
    Py_ssize_t size = mclBnG2_serialize(buffer, sizeof(buffer), &g2_self->mcl_g2);
    // return the bytes
    return PyBytes_FromStringAndSize(buffer, size);
}

Py_hash_t G2_hash(PyObject *self) {
    // convert the object to G2
    G2 *g2_self = (G2 *)self;
    // build the buffer
    char buffer[96];
    // convert the G2 to bytes
    Py_ssize_t size = mclBnG2_serialize(buffer, sizeof(buffer), &g2_self->mcl_g2);
    // return the hash
    return hash_bytes(buffer, size);
}

PyObject *G2_deserialize(PyObject *type, PyObject *args) {
    // required a byte string
    PyObject *bytes;
    if (!PyArg_ParseTuple(args, "O!", &PyBytes_Type, &bytes)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments, should be a bytes object");
        return NULL;
    }
    // build the result
    G2 *g2_res = G2_create();
    // convert the bytes to an element
    Py_ssize_t size = PyBytes_Size(bytes);
    char *string = PyBytes_AsString(bytes);
    if (!mclBnG2_deserialize(&g2_res->mcl_g2, string, size)) {
        PyErr_SetString(PyExc_ValueError, "could not deserialize the bytes as a G2 element");
        return NULL;
    }
    // return the element
    return (PyObject *)g2_res;
}

PyObject *G2_add(PyObject *lft, PyObject *rgt) {
    // check the type of arguments
    if (!PyObject_TypeCheck(lft, &G2Type) || !PyObject_TypeCheck(rgt, &G2Type)) {
        PyErr_SetString(PyExc_TypeError, "operands must be both G2 elements");
        return NULL;
    }
    // convert both objects to G2
    G2 *g2_lft = (G2 *)lft;
    G2 *g2_rgt = (G2 *)rgt;
    // build the result element and initialize it to the same group as the left element
    G2 *g2_res = G2_create();
    mclBnG2_add(&g2_res->mcl_g2, &g2_lft->mcl_g2, &g2_rgt->mcl_g2);
    return (PyObject *)g2_res;
}

PyObject *G2_sub(PyObject *lft, PyObject *rgt) {
    // check the type of arguments
    if (!PyObject_TypeCheck(lft, &G2Type) || !PyObject_TypeCheck(rgt, &G2Type)) {
        PyErr_SetString(PyExc_TypeError, "operands must be both G2 elements");
        return NULL;
    }
    // convert both objects to G2
    G2 *g2_lft = (G2 *)lft;
    G2 *g2_rgt = (G2 *)rgt;
    // build the result element and initialize it to the same group as the left element
    G2 *g2_res = G2_create();
    mclBnG2_sub(&g2_res->mcl_g2, &g2_lft->mcl_g2, &g2_rgt->mcl_g2);
    return (PyObject *)g2_res;
}

PyObject *G2_neg(PyObject *arg) {
    // convert the object to G2
    G2 *g2_arg = (G2 *)arg;
    // build the result element and initialize it to the same group as the argument
    G2 *g2_res = G2_create();
    mclBnG2_neg(&g2_res->mcl_g2, &g2_arg->mcl_g2);
    return (PyObject *)g2_res;
}

PyObject *G2_mul(PyObject *lft, PyObject *rgt, PyObject *mod) {
    // check the type of arguments
    if (!PyObject_TypeCheck(lft, &G2Type) || !PyObject_TypeCheck(rgt, &FrType)) {
        PyErr_SetString(PyExc_TypeError, "operands must be a G2 element and an Fr element");
        return NULL;
    }
    // convert the objects to G2 and Element
    G2 *g2_lft = (G2 *)lft;
    Fr *fr_rgt = (Fr *)rgt;
    // build the result element and initialize it to the same group as the left element
    G2 *g2_res = G2_create();
    mclBnG2_mul(&g2_res->mcl_g2, &g2_lft->mcl_g2, &fr_rgt->mcl_fr);
    return (PyObject *)g2_res;
}

PyObject *G2_cmp(PyObject *lft, PyObject *rgt, int op) {
    // check the type of arguments
    if (!PyObject_TypeCheck(lft, &G2Type) || !PyObject_TypeCheck(rgt, &G2Type)) {
        PyErr_SetString(PyExc_TypeError, "operands must be both G2 elements");
        return NULL;
    }
    // convert both objects to G2
    G2 *g2_lft = (G2 *)lft;
    G2 *g2_rgt = (G2 *)rgt;
    // compare the elements
    int result = mclBnG2_isEqual(&g2_lft->mcl_g2, &g2_rgt->mcl_g2);
    // return the result
    switch (op) {
    case Py_EQ: if (result) { Py_RETURN_TRUE; } else { Py_RETURN_FALSE; }
    case Py_NE: if (result) { Py_RETURN_FALSE; } else { Py_RETURN_TRUE; }
    default: PyErr_SetString(PyExc_TypeError, "operation not supported"); return NULL;
    }
}

PyObject *G2_isZero(PyObject *arg) {
    // convert the object to G2
    G2 *g2_arg = (G2 *)arg;
    // check if the element is zero
    if (mclBnG2_isZero(&g2_arg->mcl_g2)) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

PyMemberDef G2_members[] = {
    {NULL},
};

PyMethodDef G2_methods[] = {
    {"serialize", (PyCFunction)G2_serialize, METH_NOARGS, "Serializes the element to a byte string."},
    {"deserialize", (PyCFunction)G2_deserialize, METH_VARARGS | METH_CLASS, "Deserializes the element from a byte string."},
    {"isZero", (PyCFunction)G2_isZero, METH_NOARGS, "Checks if the element is the zero element."},
    {"hash", (PyCFunction)G2_from_bytes, METH_VARARGS | METH_CLASS, "Hashes a byte string to a G2 element."},
    {NULL},
};

PyNumberMethods G2_num_meths = {
    G2_add, // binaryfunc nb_add;
    G2_sub, // binaryfunc nb_subtract;
    G2_mul, // binaryfunc nb_multiply;
    0,      // binaryfunc nb_remainder;
    0,      // binaryfunc nb_divmod;
    0,      // ternaryfunc nb_power;
    G2_neg, // unaryfunc nb_negative;
    0,      // unaryfunc nb_positive;
    0,      // unaryfunc nb_absolute;
    0,      // inquiry nb_bool;
    0,      // unaryfunc nb_invert;
    0,      // binaryfunc nb_lshift;
    0,      // binaryfunc nb_rshift;
    0,      // binaryfunc nb_and;
    0,      // binaryfunc nb_xor;
    0,      // binaryfunc nb_or;
    0,      // unaryfunc nb_int;
    0,      // void *nb_reserved;
    0,      // unaryfunc nb_float;
    0,      // binaryfunc nb_inplace_add;
    0,      // binaryfunc nb_inplace_subtract;
    0,      // binaryfunc nb_inplace_multiply;
    0,      // binaryfunc nb_inplace_remainder;
    0,      // ternaryfunc nb_inplace_power;
    0,      // binaryfunc nb_inplace_lshift;
    0,      // binaryfunc nb_inplace_rshift;
    0,      // binaryfunc nb_inplace_and;
    0,      // binaryfunc nb_inplace_xor;
    0,      // binaryfunc nb_inplace_or;
    0,      // binaryfunc nb_floor_divide;
    0,      // binaryfunc nb_true_divide;
    0,      // binaryfunc nb_inplace_floor_divide;
    0,      // binaryfunc nb_inplace_true_divide;
};

PyTypeObject G2Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pymcl.G2",                               /* tp_name */
    sizeof(G2),                               /* tp_basicsize */
    0,                                        /* tp_itemsize */
    G2_dealloc,                               /* tp_dealloc */
    0,                                        /* tp_print */
    0,                                        /* tp_getattr */
    0,                                        /* tp_setattr */
    0,                                        /* tp_compare */
    G2_str,                                   /* tp_repr */
    &G2_num_meths,                            /* tp_as_number */
    0,                                        /* tp_as_sequence */
    0,                                        /* tp_as_mapping */
    G2_hash,                                  /* tp_hash */
    0,                                        /* tp_call */
    G2_str,                                   /* tp_str */
    0,                                        /* tp_getattro */
    0,                                        /* tp_setattro */
    0,                                        /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    G2__doc__,                                /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    G2_cmp,                                   /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    0,                                        /* tp_iter */
    0,                                        /* tp_iternext */
    G2_methods,                               /* tp_methods */
    G2_members,                               /* tp_members */
    0,                                        /* tp_getset */
    0,                                        /* tp_base */
    0,                                        /* tp_dict */
    0,                                        /* tp_descr_get */
    0,                                        /* tp_descr_set */
    0,                                        /* tp_dictoffset */
    0,                                        /* tp_init */
    0,                                        /* tp_alloc */
    G2_new,                                   /* tp_new */
};

/*******************************************************************************
*                                      GT                                      *
*******************************************************************************/

PyDoc_STRVAR(GT__doc__,
    "Represents an element of the GT group.\n"
    "\n"
    "Basic usage:\n"
    "\n"
    "GT(s: str) -> GT\n"
    "GT.__str__(self: GT) -> str\n"
    "GT.__mul__(self: GT, other: GT) -> GT\n"
    "GT.__truediv__(self: GT, other: GT) -> GT\n"
    "GT.__invert__(self: GT) -> GT\n"
    "GT.__pow__(self: GT, other: Fr) -> GT\n"
    "GT.__eq__(self: GT, other: GT) -> bool\n"
    "GT.__ne__(self: GT, other: GT) -> bool\n"
    "GT.__hash__(self: GT) -> int\n"
    "GT.serialize(self: GT) -> bytes\n"
    "GT.deserialize(b: bytes) -> GT\n"
    "GT.isZero(self: GT) -> bool\n"
    "GT.isOne(self: GT) -> bool\n"
    "\n"
    "Most of the basic arithmetic operations apply. Please note that many of them\n"
    "do not make sense between groups, and that not all of these are checked for.");

GT *GT_create(void) {
    // allocate the object
    GT *gt_res = (GT *)GTType.tp_alloc(&GTType, 0);
    // check if the object was allocated
    if (!gt_res) {
        PyErr_SetString(PyExc_TypeError, "could not create GT object");
        return NULL;
    }
    // return the object
    return gt_res;
}

PyObject *GT_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    // required a string
    char *string = NULL;
    Py_ssize_t length;
    if (!PyArg_ParseTuple(args, "|s#", &string, &length)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments, should be a string");
        return NULL;
    }
    // build the result
    GT *gt_res = GT_create();
    // initialize the GT with the string
    if (string == NULL) {
        mclBnGT_clear(&gt_res->mcl_gt);
    } else if (mclBnGT_setStr(&gt_res->mcl_gt, string, length, 10)) {
        PyErr_SetString(PyExc_ValueError, "could not parse the string as a GT element");
        return NULL;
    }
    // return the element
    return (PyObject *)gt_res;
}

void GT_dealloc(PyObject *self) {
    // free the object
    Py_TYPE(self)->tp_free(self);
}

PyObject *GT_str(PyObject *self) {
    // cast the argument
    GT *gt_self = (GT *)self;
    // get the string from the GT
    char buffer[1440];
    Py_ssize_t length = mclBnGT_getStr(buffer, sizeof(buffer), &gt_self->mcl_gt, 10);
    // return the string
    return PyUnicode_FromStringAndSize(buffer, length);
}

PyObject *GT_serialize(PyObject *self) {
    // convert the object to GT
    GT *gt_self = (GT *)self;
    // build the buffer
    char buffer[576];
    // convert the GT to bytes
    Py_ssize_t size = mclBnGT_serialize(buffer, sizeof(buffer), &gt_self->mcl_gt);
    // return the bytes
    return PyBytes_FromStringAndSize(buffer, size);
}

Py_hash_t GT_hash(PyObject *self) {
    // convert the object to GT
    GT *gt_self = (GT *)self;
    // build the buffer
    char buffer[576];
    // convert the GT to bytes
    Py_ssize_t size = mclBnGT_serialize(buffer, sizeof(buffer), &gt_self->mcl_gt);
    // return the hash
    return hash_bytes(buffer, size);
}

PyObject *GT_deserialize(PyObject *type, PyObject *args) {
    // required a byte string
    PyObject *bytes;
    if (!PyArg_ParseTuple(args, "O!", &PyBytes_Type, &bytes)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments, should be a bytes object");
        return NULL;
    }
    // build the result
    GT *gt_res = GT_create();
    // convert the bytes to an element
    Py_ssize_t size = PyBytes_Size(bytes);
    char *string = PyBytes_AsString(bytes);
    if (!mclBnGT_deserialize(&gt_res->mcl_gt, string, size)) {
        PyErr_SetString(PyExc_ValueError, "could not deserialize the bytes as a GT element");
        return NULL;
    }
    // return the element
    return (PyObject *)gt_res;
}

PyObject *GT_mul(PyObject *lft, PyObject *rgt) {
    // check the type of arguments
    if (!PyObject_TypeCheck(lft, &GTType) || !PyObject_TypeCheck(rgt, &GTType)) {
        PyErr_SetString(PyExc_TypeError, "operands must be both GT elements");
        return NULL;
    }
    // convert both objects to GT
    GT *gt_lft = (GT *)lft;
    GT *gt_rgt = (GT *)rgt;
    // build the result element and initialize it to the same group as the left element
    GT *gt_res = GT_create();
    mclBnGT_mul(&gt_res->mcl_gt, &gt_lft->mcl_gt, &gt_rgt->mcl_gt);
    return (PyObject *)gt_res;
}

PyObject *GT_div(PyObject *lft, PyObject *rgt) {
    // check the type of arguments
    if (!PyObject_TypeCheck(lft, &GTType) || !PyObject_TypeCheck(rgt, &GTType)) {
        PyErr_SetString(PyExc_TypeError, "operands must be both GT elements");
        return NULL;
    }
    // convert both objects to GT
    GT *gt_lft = (GT *)lft;
    GT *gt_rgt = (GT *)rgt;
    // build the result element and initialize it to the same group as the left element
    GT *gt_res = GT_create();
    mclBnGT_div(&gt_res->mcl_gt, &gt_lft->mcl_gt, &gt_rgt->mcl_gt);
    return (PyObject *)gt_res;
}

PyObject *GT_inv(PyObject *arg) {
    // convert the object to GT
    GT *gt_arg = (GT *)arg;
    // build the result element and initialize it to the same group as the argument
    GT *gt_res = GT_create();
    mclBnGT_inv(&gt_res->mcl_gt, &gt_arg->mcl_gt);
    return (PyObject *)gt_res;
}

PyObject *GT_pow(PyObject *lft, PyObject *rgt, PyObject *mod) {
    // check the type of arguments
    if (!PyObject_TypeCheck(lft, &GTType) || !PyObject_TypeCheck(rgt, &FrType)) {
        PyErr_SetString(PyExc_TypeError, "operands must be a GT element and an Fr element");
        return NULL;
    }
    // convert the objects to GT and Element
    GT *gt_lft = (GT *)lft;
    Fr *fr_rgt = (Fr *)rgt;
    // build the result element and initialize it to the same group as the left element
    GT *gt_res = GT_create();
    mclBnGT_pow(&gt_res->mcl_gt, &gt_lft->mcl_gt, &fr_rgt->mcl_fr);
    return (PyObject *)gt_res;
}

PyObject *GT_cmp(PyObject *lft, PyObject *rgt, int op) {
    // check the type of arguments
    if (!PyObject_TypeCheck(lft, &GTType) || !PyObject_TypeCheck(rgt, &GTType)) {
        PyErr_SetString(PyExc_TypeError, "operands must be both GT elements");
        return NULL;
    }
    // convert both objects to GT
    GT *gt_lft = (GT *)lft;
    GT *gt_rgt = (GT *)rgt;
    // compare the elements
    int result = mclBnGT_isEqual(&gt_lft->mcl_gt, &gt_rgt->mcl_gt);
    // return the result
    switch (op) {
    case Py_EQ: if (result) { Py_RETURN_TRUE; } else { Py_RETURN_FALSE; }
    case Py_NE: if (result) { Py_RETURN_FALSE; } else { Py_RETURN_TRUE; }
    default: PyErr_SetString(PyExc_TypeError, "operation not supported"); return NULL;
    }
}

PyObject *GT_isZero(PyObject *arg) {
    // convert the object to GT
    GT *gt_arg = (GT *)arg;
    // check if the element is zero
    if (mclBnGT_isZero(&gt_arg->mcl_gt)) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

PyObject *GT_isOne(PyObject *arg) {
    // convert the object to GT
    GT *gt_arg = (GT *)arg;
    // check if the element is one
    if (mclBnGT_isOne(&gt_arg->mcl_gt)) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

PyMemberDef GT_members[] = {
    {NULL},
};

PyMethodDef GT_methods[] = {
    {"serialize", (PyCFunction)GT_serialize, METH_NOARGS, "Serializes the element to a byte string."},
    {"deserialize", (PyCFunction)GT_deserialize, METH_VARARGS | METH_CLASS, "Deserializes the element from a byte string."},
    {"isZero", (PyCFunction)GT_isZero, METH_NOARGS, "Checks if the element is the zero element."},
    {"isOne", (PyCFunction)GT_isOne, METH_NOARGS, "Checks if the element is the one element."},
    {NULL},
};

PyNumberMethods GT_num_meths = {
    0,      // binaryfunc nb_add;
    0,      // binaryfunc nb_subtract;
    GT_mul, // binaryfunc nb_multiply;
    0,      // binaryfunc nb_remainder;
    0,      // binaryfunc nb_divmod;
    GT_pow, // ternaryfunc nb_power;
    0,      // unaryfunc nb_negative;
    0,      // unaryfunc nb_positive;
    0,      // unaryfunc nb_absolute;
    0,      // inquiry nb_bool;
    GT_inv, // unaryfunc nb_invert;
    0,      // binaryfunc nb_lshift;
    0,      // binaryfunc nb_rshift;
    0,      // binaryfunc nb_and;
    0,      // binaryfunc nb_xor;
    0,      // binaryfunc nb_or;
    0,      // unaryfunc nb_int;
    0,      // void *nb_reserved;
    0,      // unaryfunc nb_float;
    0,      // binaryfunc nb_inplace_add;
    0,      // binaryfunc nb_inplace_subtract;
    0,      // binaryfunc nb_inplace_multiply;
    0,      // binaryfunc nb_inplace_remainder;
    0,      // ternaryfunc nb_inplace_power;
    0,      // binaryfunc nb_inplace_lshift;
    0,      // binaryfunc nb_inplace_rshift;
    0,      // binaryfunc nb_inplace_and;
    0,      // binaryfunc nb_inplace_xor;
    0,      // binaryfunc nb_inplace_or;
    0,      // binaryfunc nb_floor_divide;
    GT_div, // binaryfunc nb_true_divide;
    0,      // binaryfunc nb_inplace_floor_divide;
    0,      // binaryfunc nb_inplace_true_divide;
};

PyTypeObject GTType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pymcl.GT",                               /* tp_name */
    sizeof(GT),                               /* tp_basicsize */
    0,                                        /* tp_itemsize */
    GT_dealloc,                               /* tp_dealloc */
    0,                                        /* tp_print */
    0,                                        /* tp_getattr */
    0,                                        /* tp_setattr */
    0,                                        /* tp_compare */
    GT_str,                                   /* tp_repr */
    &GT_num_meths,                            /* tp_as_number */
    0,                                        /* tp_as_sequence */
    0,                                        /* tp_as_mapping */
    GT_hash,                                  /* tp_hash */
    0,                                        /* tp_call */
    GT_str,                                   /* tp_str */
    0,                                        /* tp_getattro */
    0,                                        /* tp_setattro */
    0,                                        /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    GT__doc__,                                /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    GT_cmp,                                   /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    0,                                        /* tp_iter */
    0,                                        /* tp_iternext */
    GT_methods,                               /* tp_methods */
    GT_members,                               /* tp_members */
    0,                                        /* tp_getset */
    0,                                        /* tp_base */
    0,                                        /* tp_dict */
    0,                                        /* tp_descr_get */
    0,                                        /* tp_descr_set */
    0,                                        /* tp_dictoffset */
    0,                                        /* tp_init */
    0,                                        /* tp_alloc */
    GT_new,                                   /* tp_new */
};


/*******************************************************************************
*                                      Fr                                      *
*******************************************************************************/

PyDoc_STRVAR(Fr__doc__,
    "Represents an element of the Fr group.\n"
    "\n"
    "Basic usage:\n"
    "\n"
    "Fr(s: str) -> Fr\n"
    "Fr.__str__(self: Fr) -> str\n"
    "Fr.__add__(self: Fr, other: Fr) -> Fr\n"
    "Fr.__sub__(self: Fr, other: Fr) -> Fr\n"
    "Fr.__neg__(self: Fr) -> Fr\n"
    "Fr.__mul__(self: Fr, other: Fr) -> Fr\n"
    "Fr.__truediv__(self: Fr, other: Fr) -> Fr\n"
    "Fr.__invert__(self: Fr) -> Fr\n"
    "Fr.__eq__(self: Fr, other: Fr) -> bool\n"
    "Fr.__ne__(self: Fr, other: Fr) -> bool\n"
    "Fr.__hash__(self: Fr) -> int\n"
    "Fr.serialize(self: Fr) -> bytes\n"
    "Fr.deserialize(b: bytes) -> Fr\n"
    "Fr.random() -> Fr\n"
    "Fr.isZero(self: Fr) -> bool\n"
    "Fr.isOne(self: Fr) -> bool\n"
    "\n"
    "Most of the basic arithmetic operations apply. Please note that many of them\n"
    "do not make sense between groups, and that not all of these are checked for.");

Fr *Fr_create(void) {
    // allocate the object
    Fr *fr_res = (Fr *)FrType.tp_alloc(&FrType, 0);
    // check if the object was allocated
    if (!fr_res) {
        PyErr_SetString(PyExc_TypeError, "could not create Fr object");
        return NULL;
    }
    // return the object
    return fr_res;
}

PyObject *Fr_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    // required a string
    char *string = NULL;
    Py_ssize_t length;
    if (!PyArg_ParseTuple(args, "|s#", &string, &length)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments, should be a string");
        return NULL;
    }
    // build the result
    Fr *fr_res = Fr_create();
    // initialize the Fr with the string
    if (string == NULL) {
        mclBnFr_clear(&fr_res->mcl_fr);
    } else if (mclBnFr_setStr(&fr_res->mcl_fr, string, length, 10)) {
        PyErr_SetString(PyExc_ValueError, "could not parse the string as a Fr element");
        return NULL;
    }
    // return the element
    return (PyObject *)fr_res;
}

void Fr_dealloc(PyObject *self) {
    // free the object
    Py_TYPE(self)->tp_free(self);
}

PyObject *Fr_str(PyObject *self) {
    // cast the argument
    Fr *fr_self = (Fr *)self;
    // get the string from the Fr
    char buffer[80];
    Py_ssize_t length = mclBnFr_getStr(buffer, sizeof(buffer), &fr_self->mcl_fr, 10);
    // return the string
    return PyUnicode_FromStringAndSize(buffer, length);
}

PyObject *Fr_serialize(PyObject *self) {
    // convert the object to Fr
    Fr *fr_self = (Fr *)self;
    // build the buffer
    char buffer[32];
    // convert the Fr to bytes
    Py_ssize_t size = mclBnFr_serialize(buffer, sizeof(buffer), &fr_self->mcl_fr);
    // return the bytes
    return PyBytes_FromStringAndSize(buffer, size);
}

Py_hash_t Fr_hash(PyObject *self) {
    // convert the object to Fr
    Fr *fr_self = (Fr *)self;
    // build the buffer
    char buffer[32];
    // convert the Fr to bytes
    Py_ssize_t size = mclBnFr_serialize(buffer, sizeof(buffer), &fr_self->mcl_fr);
    // return the hash
    return hash_bytes(buffer, size);
}

PyObject *Fr_deserialize(PyObject *type, PyObject *args) {
    // required a byte string
    PyObject *bytes;
    if (!PyArg_ParseTuple(args, "O!", &PyBytes_Type, &bytes)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments, should be a bytes object");
        return NULL;
    }
    // build the result
    Fr *fr_res = Fr_create();
    // convert the bytes to an element
    Py_ssize_t size = PyBytes_Size(bytes);
    char *string = PyBytes_AsString(bytes);
    if (!mclBnFr_deserialize(&fr_res->mcl_fr, string, size)) {
        PyErr_SetString(PyExc_ValueError, "could not deserialize the bytes as a Fr element");
        return NULL;
    }
    // return the element
    return (PyObject *)fr_res;
}

PyObject *Fr_random(PyObject *type, PyObject *args) {
    // build the result
    Fr *fr_res = Fr_create();
    // set the result to a random element
    mclBnFr_setByCSPRNG(&fr_res->mcl_fr);
    // return the element
    return (PyObject *)fr_res;
}

PyObject *Fr_add(PyObject *lft, PyObject *rgt) {
    // check the type of arguments
    if (!PyObject_TypeCheck(lft, &FrType) || !PyObject_TypeCheck(rgt, &FrType)) {
        PyErr_SetString(PyExc_TypeError, "operands must be both Fr elements");
        return NULL;
    }
    // convert both objects to Fr
    Fr *fr_lft = (Fr *)lft;
    Fr *fr_rgt = (Fr *)rgt;
    // build the result element and initialize it to the same group as the left element
    Fr *fr_res = Fr_create();
    mclBnFr_add(&fr_res->mcl_fr, &fr_lft->mcl_fr, &fr_rgt->mcl_fr);
    return (PyObject *)fr_res;
}

PyObject *Fr_sub(PyObject *lft, PyObject *rgt) {
    // check the type of arguments
    if (!PyObject_TypeCheck(lft, &FrType) || !PyObject_TypeCheck(rgt, &FrType)) {
        PyErr_SetString(PyExc_TypeError, "operands must be both Fr elements");
        return NULL;
    }
    // convert both objects to Fr
    Fr *fr_lft = (Fr *)lft;
    Fr *fr_rgt = (Fr *)rgt;
    // build the result element and initialize it to the same group as the left element
    Fr *fr_res = Fr_create();
    mclBnFr_sub(&fr_res->mcl_fr, &fr_lft->mcl_fr, &fr_rgt->mcl_fr);
    return (PyObject *)fr_res;
}

PyObject *Fr_neg(PyObject *arg) {
    // convert the object to Fr
    Fr *fr_arg = (Fr *)arg;
    // build the result element and initialize it to the same group as the argument
    Fr *fr_res = Fr_create();
    mclBnFr_neg(&fr_res->mcl_fr, &fr_arg->mcl_fr);
    return (PyObject *)fr_res;
}

PyObject *Fr_mul(PyObject *lft, PyObject *rgt) {
    // check the type of arguments
    if (!PyObject_TypeCheck(lft, &FrType) || !PyObject_TypeCheck(rgt, &FrType)) {
        PyErr_SetString(PyExc_TypeError, "operands must be both Fr elements");
        return NULL;
    }
    // convert both objects to Fr
    Fr *fr_lft = (Fr *)lft;
    Fr *fr_rgt = (Fr *)rgt;
    // build the result element and initialize it to the same group as the left element
    Fr *fr_res = Fr_create();
    mclBnFr_mul(&fr_res->mcl_fr, &fr_lft->mcl_fr, &fr_rgt->mcl_fr);
    return (PyObject *)fr_res;
}

PyObject *Fr_div(PyObject *lft, PyObject *rgt) {
    // check the type of arguments
    if (!PyObject_TypeCheck(lft, &FrType) || !PyObject_TypeCheck(rgt, &FrType)) {
        PyErr_SetString(PyExc_TypeError, "operands must be both Fr elements");
        return NULL;
    }
    // convert both objects to Fr
    Fr *fr_lft = (Fr *)lft;
    Fr *fr_rgt = (Fr *)rgt;
    // build the result element and initialize it to the same group as the left element
    Fr *fr_res = Fr_create();
    mclBnFr_div(&fr_res->mcl_fr, &fr_lft->mcl_fr, &fr_rgt->mcl_fr);
    return (PyObject *)fr_res;
}

PyObject *Fr_inv(PyObject *arg) {
    // convert the object to Fr
    Fr *fr_arg = (Fr *)arg;
    // build the result element and initialize it to the same group as the argument
    Fr *fr_res = Fr_create();
    mclBnFr_inv(&fr_res->mcl_fr, &fr_arg->mcl_fr);
    return (PyObject *)fr_res;
}

PyObject *Fr_cmp(PyObject *lft, PyObject *rgt, int op) {
    // check the type of arguments
    if (!PyObject_TypeCheck(lft, &FrType) || !PyObject_TypeCheck(rgt, &FrType)) {
        PyErr_SetString(PyExc_TypeError, "operands must be both Fr elements");
        return NULL;
    }
    // convert both objects to Fr
    Fr *fr_lft = (Fr *)lft;
    Fr *fr_rgt = (Fr *)rgt;
    // compare the elements
    int result = mclBnFr_isEqual(&fr_lft->mcl_fr, &fr_rgt->mcl_fr);
    // return the result
    switch (op) {
    case Py_EQ: if (result) { Py_RETURN_TRUE; } else { Py_RETURN_FALSE; }
    case Py_NE: if (result) { Py_RETURN_FALSE; } else { Py_RETURN_TRUE; }
    default: PyErr_SetString(PyExc_TypeError, "operation not supported"); return NULL;
    }
}

PyObject *Fr_isZero(PyObject *arg) {
    // convert the object to Fr
    Fr *fr_arg = (Fr *)arg;
    // check if the element is zero
    if (mclBnFr_isZero(&fr_arg->mcl_fr)) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

PyObject *Fr_isOne(PyObject *arg) {
    // convert the object to Fr
    Fr *fr_arg = (Fr *)arg;
    // check if the element is one
    if (mclBnFr_isOne(&fr_arg->mcl_fr)) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

PyMemberDef Fr_members[] = {
    {NULL},
};

PyMethodDef Fr_methods[] = {
    {"serialize", (PyCFunction)Fr_serialize, METH_NOARGS, "Serializes the element to a byte string."},
    {"deserialize", (PyCFunction)Fr_deserialize, METH_VARARGS | METH_CLASS, "Deserializes the element from a byte string."},
    {"random", (PyCFunction)Fr_random, METH_NOARGS | METH_CLASS, "Generates a random Fr element."},
    {"isZero", (PyCFunction)Fr_isZero, METH_NOARGS, "Checks if the element is the zero element."},
    {"isOne", (PyCFunction)Fr_isOne, METH_NOARGS, "Checks if the element is the one element."},
    {NULL},
};

PyNumberMethods Fr_num_meths = {
    Fr_add, // binaryfunc nb_add;
    Fr_sub, // binaryfunc nb_subtract;
    Fr_mul, // binaryfunc nb_multiply;
    0,      // binaryfunc nb_remainder;
    0,      // binaryfunc nb_divmod;
    0,      // ternaryfunc nb_power;
    Fr_neg, // unaryfunc nb_negative;
    0,      // unaryfunc nb_positive;
    0,      // unaryfunc nb_absolute;
    0,      // inquiry nb_bool;
    Fr_inv, // unaryfunc nb_invert;
    0,      // binaryfunc nb_lshift;
    0,      // binaryfunc nb_rshift;
    0,      // binaryfunc nb_and;
    0,      // binaryfunc nb_xor;
    0,      // binaryfunc nb_or;
    0,      // unaryfunc nb_int;
    0,      // void *nb_reserved;
    0,      // unaryfunc nb_float;
    0,      // binaryfunc nb_inplace_add;
    0,      // binaryfunc nb_inplace_subtract;
    0,      // binaryfunc nb_inplace_multiply;
    0,      // binaryfunc nb_inplace_remainder;
    0,      // ternaryfunc nb_inplace_power;
    0,      // binaryfunc nb_inplace_lshift;
    0,      // binaryfunc nb_inplace_rshift;
    0,      // binaryfunc nb_inplace_and;
    0,      // binaryfunc nb_inplace_xor;
    0,      // binaryfunc nb_inplace_or;
    0,      // binaryfunc nb_floor_divide;
    Fr_div, // binaryfunc nb_true_divide;
    0,      // binaryfunc nb_inplace_floor_divide;
    0,      // binaryfunc nb_inplace_true_divide;
};

PyTypeObject FrType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pymcl.Fr",                               /* tp_name */
    sizeof(Fr),                               /* tp_basicsize */
    0,                                        /* tp_itemsize */
    Fr_dealloc,                               /* tp_dealloc */
    0,                                        /* tp_print */
    0,                                        /* tp_getattr */
    0,                                        /* tp_setattr */
    0,                                        /* tp_compare */
    Fr_str,                                   /* tp_repr */
    &Fr_num_meths,                            /* tp_as_number */
    0,                                        /* tp_as_sequence */
    0,                                        /* tp_as_mapping */
    Fr_hash,                                  /* tp_hash */
    0,                                        /* tp_call */
    Fr_str,                                   /* tp_str */
    0,                                        /* tp_getattro */
    0,                                        /* tp_setattro */
    0,                                        /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    Fr__doc__,                                /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    Fr_cmp,                                   /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    0,                                        /* tp_iter */
    0,                                        /* tp_iternext */
    Fr_methods,                               /* tp_methods */
    Fr_members,                               /* tp_members */
    0,                                        /* tp_getset */
    0,                                        /* tp_base */
    0,                                        /* tp_dict */
    0,                                        /* tp_descr_get */
    0,                                        /* tp_descr_set */
    0,                                        /* tp_dictoffset */
    0,                                        /* tp_init */
    0,                                        /* tp_alloc */
    Fr_new,                                   /* tp_new */
};

/*******************************************************************************
*                                    Module                                    *
*******************************************************************************/

PyDoc_STRVAR(pymcl__doc__,
    "A Python 3 wrapper for the mcl library.\n"
    "\n"
    "This library provides a Python 3 interface to the mcl library, which is a\n"
    "C++ library for pairing-based cryptography. It provides a Pythonic interface\n"
    "to the mcl library, allowing for the use of bilinear groups and pairings in\n"
    "Python 3.");

PyObject *pairing(PyObject *self, PyObject *args) {
    // parse the arguments
    PyObject *lft, *rgt;
    if (!PyArg_ParseTuple(args, "O!O!", &G1Type, &lft, &G2Type, &rgt)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments, should be a G1 and a G2 element");
        return NULL;
    }
    // convert the objects to G1 and G2
    G1 *g1_lft = (G1 *)lft;
    G2 *g2_rgt = (G2 *)rgt;
    // build the result element
    GT *gt_res = GT_create();
    // compute the pairing
    mclBn_pairing(&gt_res->mcl_gt, &g1_lft->mcl_g1, &g2_rgt->mcl_g2);
    // return the result
    return (PyObject *)gt_res;
}

PyMethodDef pymcl_methods[] = {
    {"pairing", (PyCFunction)pairing, METH_VARARGS, "Computes the pairing between a G1 and a G2 element."},
    {NULL},
};

PyModuleDef pymcl_module = {
    PyModuleDef_HEAD_INIT,
    "pymcl",
    pymcl__doc__,
    -1,
    pymcl_methods,
};

PyMODINIT_FUNC PyInit_pymcl(void) {
    // initialize the library
    if (mclBn_init(MCL_BLS12_381, MCLBN_COMPILED_TIME_VAR)) {
        return NULL;
    }
    // initialize the types
    if (PyType_Ready(&G1Type) < 0) {
        return NULL;
    }
    if (PyType_Ready(&G2Type) < 0) {
        return NULL;
    }
    if (PyType_Ready(&GTType) < 0) {
        return NULL;
    }
    if (PyType_Ready(&FrType) < 0) {
        return NULL;
    }
    // create the module
    PyObject *module = PyModule_Create(&pymcl_module);
    if (module == NULL) {
        return NULL;
    }
    // increment the reference count for the types
    Py_INCREF(&G1Type);
    Py_INCREF(&G2Type);
    Py_INCREF(&GTType);
    Py_INCREF(&FrType);
    // add the types to the module
    PyModule_AddObject(module, "G1", (PyObject *)&G1Type);
    PyModule_AddObject(module, "G2", (PyObject *)&G2Type);
    PyModule_AddObject(module, "GT", (PyObject *)&GTType);
    PyModule_AddObject(module, "Fr", (PyObject *)&FrType);
    // initialize the constants
    const char *rStr = "0x73eda753299d7d483339d80809a1d80553bda402fffe5bfeffffffff00000001";
    PyObject *r = PyLong_FromString(rStr, NULL, 0);
    G1* g1 = G1_create();
    const char *g1Str = "1 0x17f1d3a73197d7942695638c4fa9ac0fc3688c4f9774b905a14e3a3f171bac586c55e83ff97a1aeffb3af00adb22c6bb 0x08b3f481e3aaa0f1a09e30ed741d8ae4fcf5e095d5d00af600db18cb2c04b3edd03cc744a2888ae40caa232946c5e7e1";
    mclBnG1_setStr(&g1->mcl_g1, g1Str, strlen(g1Str), 16);
    G2* g2 = G2_create();
    const char *g2Str = "1 0x24aa2b2f08f0a91260805272dc51051c6e47ad4fa403b02b4510b647ae3d1770bac0326a805bbefd48056c8c121bdb8 0x13e02b6052719f607dacd3a088274f65596bd0d09920b61ab5da61bbdc7f5049334cf11213945d57e5ac7d055d042b7e 0x0ce5d527727d6e118cc9cdc6da2e351aadfd9baa8cbdd3a76d429a695160d12c923ac9cc3baca289e193548608b82801 0x0606c4a02ea734cc32acd2b02bc28b99cb3e287e85a763af267492ab572e99ab3f370d275cec1da1aaa9075ff05f79be";
    mclBnG2_setStr(&g2->mcl_g2, g2Str, strlen(g2Str), 16);
    // add the constants to the module, no need to increment the reference count
    PyModule_AddObject(module, "r", r);
    PyModule_AddObject(module, "g1", (PyObject *)g1);
    PyModule_AddObject(module, "g2", (PyObject *)g2);
    return module;
}
