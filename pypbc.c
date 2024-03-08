#include "pypbc.h"
#include <stdio.h>

/*******************************************************************************
* pypbc.c                                                                      *
*                                                                              *
* Modifications by Jemtaly                                                     *
* Copyright (c) 2024                                                           *
*                                                                              *
* Modifications by Joseph deBlaquiere                                          *
* Copyright (c) 2017                                                           *
*                                                                              *
* Originally written by Geremy Condra                                          *
* Licensed under GPLv3                                                         *
* Released 11 October 2009                                                     *
*                                                                              *
* This file contains the types and functions needed to use PBC from Python 3.  *
*******************************************************************************/

// initialize a GMP integer from a Python number
void mpz_init_from_pynum(mpz_t mpz_n, PyObject *py_n) {
    // convert the Python number to a C string
    PyObject *py_n_unicode = PyNumber_ToBase(py_n, 10);
    PyObject *py_n_bytes = PyUnicode_AsASCIIString(py_n_unicode);
    char *str_n = PyBytes_AsString(py_n_bytes);
    // initialize the GMP integer from the ASCII string
    mpz_init_set_str(mpz_n, str_n, 10);
    // decrement the reference count on the objects
    Py_DECREF(py_n_unicode);
    Py_DECREF(py_n_bytes);
}

// get a Python number from a GMP integer
PyObject *mpz_to_pynum(mpz_t mpz_n) {
    // convert the GMP integer to a C string
    char *str_n = mpz_get_str(NULL, 10, mpz_n);
    // convert the C string to a Python number
    PyObject *py_n = PyLong_FromString(str_n, NULL, 10);
    // free the string
    free(str_n);
    return py_n;
}

/*******************************************************************************
*                                    Params                                    *
*******************************************************************************/

PyDoc_STRVAR(Parameters__doc__,
    "A representation of the parameters of an elliptic curve.\n"
    "\n"
    "There are three basic ways to instantiate a Parameters object:\n"
    "\n"
    "Parameters(string: str) -> Parameters\n"
    "\n"
    "These objects are essentially only used for creating Pairings.");

Parameters *Parameters_create(void) {
    // allocate the object
    Parameters *params = (Parameters *)ParametersType.tp_alloc(&ParametersType, 0);
    // check if the object was allocated
    if (!params) {
        PyErr_SetString(PyExc_TypeError, "could not create Parameters object");
        return NULL;
    }
    // set the ready flag to 0
    params->ready = 0;
    return params;
}

PyObject *Parameters_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    // required argument is the parameter string
    char *string = NULL;
    if (!PyArg_ParseTuple(args, "s", &string)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments, expected a string");
        return NULL;
    }
    // create the object
    Parameters *params = Parameters_create();
    // initialize the parameters from a string
    if (pbc_param_init_set_str(params->pbc_params, string)) {
        PyErr_SetString(PyExc_ValueError, "could not parse parameters from string");
        return NULL;
    }
    // set the ready flag
    params->ready = 1;
    return (PyObject *)params;
}

void Parameters_dealloc(Parameters *params) {
    // clear the parameters if they're ready
    if (params->ready) {
        pbc_param_clear(params->pbc_params);
    }
    // free the object
    Py_TYPE(params)->tp_free((PyObject *)params);
}

PyObject *Parameters_str(PyObject *py_params) {
    // cast the argument
    Parameters *params = (Parameters *)py_params;
    // declare a buffer
    char buffer[4096];
    // open a file in memory
    FILE *fp = fmemopen((void *)buffer, sizeof(buffer), "w+");
    // check if the file was opened
    if (fp == NULL) {
        PyErr_SetString(PyExc_IOError, "could not write parameters to buffer");
        return NULL;
    }
    // write the parameters to the buffer
    pbc_param_out_str(fp, params->pbc_params);
    // close the file
    fclose(fp);
    // return the buffer as a string
    return PyUnicode_FromString(buffer);
}

PyMemberDef Parameters_members[] = {
    {NULL},
};

PyMethodDef Parameters_methods[] = {
    {NULL},
};

PyTypeObject ParametersType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pypbc.Parameters",                       /* tp_name */
    sizeof(Parameters),                       /* tp_basicsize */
    0,                                        /* tp_itemsize */
    (destructor)Parameters_dealloc,           /* tp_dealloc */
    0,                                        /* tp_print */
    0,                                        /* tp_getattr */
    0,                                        /* tp_setattr */
    0,                                        /* tp_compare */
    Parameters_str,                           /* tp_repr */
    0,                                        /* tp_as_number */
    0,                                        /* tp_as_sequence */
    0,                                        /* tp_as_mapping */
    0,                                        /* tp_hash */
    0,                                        /* tp_call */
    Parameters_str,                           /* tp_str */
    0,                                        /* tp_getattro */
    0,                                        /* tp_setattro */
    0,                                        /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    Parameters__doc__,                        /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    0,                                        /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    0,                                        /* tp_iter */
    0,                                        /* tp_iternext */
    Parameters_methods,                       /* tp_methods */
    Parameters_members,                       /* tp_members */
    0,                                        /* tp_getset */
    0,                                        /* tp_base */
    0,                                        /* tp_dict */
    0,                                        /* tp_descr_get */
    0,                                        /* tp_descr_set */
    0,                                        /* tp_dictoffset */
    0,                                        /* tp_init */
    0,                                        /* tp_alloc */
    Parameters_new,                           /* tp_new */
};

/*******************************************************************************
*                                   Pairings                                   *
*******************************************************************************/

PyDoc_STRVAR(Pairing__doc__,
    "Represents a bilinear pairing, frequently referred to as e-hat.\n"
    "\n"
    "Basic usage:\n"
    "\n"
    "Pairing(params: Parameters) -> Pairing\n"
    "\n"
    "This object is used to apply the bilinear map to two elements.");

Pairing *Pairing_create(void) {
    // allocate the object
    Pairing *pairing = (Pairing *)PairingType.tp_alloc(&PairingType, 0);
    // check if the object was allocated
    if (!pairing) {
        PyErr_SetString(PyExc_TypeError, "could not create Pairing object");
        return NULL;
    }
    // set the ready flag to 0
    pairing->ready = 0;
    return pairing;
}

PyObject *Pairing_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    // required argument is the parameters
    PyObject *py_params;
    if (!PyArg_ParseTuple(args, "O!", &ParametersType, &py_params)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments, expected Parameters object");
        return NULL;
    }
    // cast the argument
    Parameters *params = (Parameters *)py_params;
    // create the object
    Pairing *pairing = Pairing_create();
    // initialize the pairing with the parameters
    pairing_init_pbc_param(pairing->pbc_pairing, params->pbc_params);
    // set the ready flag
    pairing->ready = 1;
    return (PyObject *)pairing;
}

void Pairing_dealloc(Pairing *pairing) {
    // clear the pairing if it's ready
    if (pairing->ready) {
        pairing_clear(pairing->pbc_pairing);
    }
    // free the object
    Py_TYPE(pairing)->tp_free((PyObject *)pairing);
}

PyObject *Pairing_apply(PyObject *py_pairing, PyObject *args) {
    // we expect two elements
    PyObject *py_lft;
    PyObject *py_rgt;
    if (!PyArg_ParseTuple(args, "O!O!", &ElementType, &py_lft, &ElementType, &py_rgt)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments, expected two Elements");
        return NULL;
    }
    // declare the result element
    Element *ele_res;
    // cast the arguments
    Pairing *pairing = (Pairing *)py_pairing;
    Element *ele_lft = (Element *)py_lft;
    Element *ele_rgt = (Element *)py_rgt;
    // check the groups of the elements
    if (ele_lft->pbc_element->field == pairing->pbc_pairing->G1 && ele_rgt->pbc_element->field == pairing->pbc_pairing->G2) {
        // build the result element and initialize it with the pairing and group
        ele_res = Element_create();
        element_init_GT(ele_res->pbc_element, pairing->pbc_pairing);
        ele_res->pairing = ele_lft->pairing;
        // apply the pairing
        pairing_apply(ele_res->pbc_element, ele_lft->pbc_element, ele_rgt->pbc_element, pairing->pbc_pairing);
    } else if (ele_lft->pbc_element->field == pairing->pbc_pairing->G2 && ele_rgt->pbc_element->field == pairing->pbc_pairing->G1) {
        // build the result element and initialize it with the pairing and group
        ele_res = Element_create();
        element_init_GT(ele_res->pbc_element, pairing->pbc_pairing);
        ele_res->pairing = ele_lft->pairing;
        // apply the pairing
        pairing_apply(ele_res->pbc_element, ele_rgt->pbc_element, ele_lft->pbc_element, pairing->pbc_pairing);
    } else {
        PyErr_SetString(PyExc_ValueError, "only Elements in G1 and G2 can be paired");
        return NULL;
    }
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(ele_res->pairing);
    ele_res->ready = 1;
    return (PyObject *)ele_res;
}

PyObject *Pairing_order(PyObject *py_pairing) {
    // cast the argument
    Pairing *pairing = (Pairing *)py_pairing;
    // return the r value
    return mpz_to_pynum(pairing->pbc_pairing->r);
}

PyObject *Pairing_is_symmetric(PyObject *py_pairing) {
    // cast the argument
    Pairing *pairing = (Pairing *)py_pairing;
    // return whether the pairing is symmetric
    if (pairing->pbc_pairing->G1 == pairing->pbc_pairing->G2) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

PyMemberDef Pairing_members[] = {
    {NULL},
};

PyMethodDef Pairing_methods[] = {
    {"apply", (PyCFunction)Pairing_apply, METH_VARARGS, "Applies the pairing."},
    {"order", (PyCFunction)Pairing_order, METH_NOARGS, "Returns the order of the pairing."},
    {"is_symmetric", (PyCFunction)Pairing_is_symmetric, METH_NOARGS, "Returns whether the pairing is symmetric."},
    {NULL},
};

PyTypeObject PairingType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pypbc.Pairing",                          /* tp_name */
    sizeof(Pairing),                          /* tp_basicsize */
    0,                                        /* tp_itemsize */
    (destructor)Pairing_dealloc,              /* tp_dealloc */
    0,                                        /* tp_print */
    0,                                        /* tp_getattr */
    0,                                        /* tp_setattr */
    0,                                        /* tp_reserved */
    0,                                        /* tp_repr */
    0,                                        /* tp_as_number */
    0,                                        /* tp_as_sequence */
    0,                                        /* tp_as_mapping */
    0,                                        /* tp_hash */
    0,                                        /* tp_call */
    0,                                        /* tp_str */
    0,                                        /* tp_getattro */
    0,                                        /* tp_setattro */
    0,                                        /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    Pairing__doc__,                           /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    0,                                        /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    0,                                        /* tp_iter */
    0,                                        /* tp_iternext */
    Pairing_methods,                          /* tp_methods */
    Pairing_members,                          /* tp_members */
    0,                                        /* tp_getset */
    0,                                        /* tp_base */
    0,                                        /* tp_dict */
    0,                                        /* tp_descr_get */
    0,                                        /* tp_descr_set */
    0,                                        /* tp_dictoffset */
    0,                                        /* tp_init */
    0,                                        /* tp_alloc */
    Pairing_new,                              /* tp_new */
};

/*******************************************************************************
*                                   Elements                                   *
*******************************************************************************/

PyDoc_STRVAR(Element__doc__,
    "Represents an element of a bilinear group.\n"
    "\n"
    "Basic usage:\n"
    "\n"
    "Element(pairing: Pairing, group: int, string: str) -> Element\n"
    "\n"
    "Most of the basic arithmetic operations apply. Please note that many of them\n"
    "do not make sense between groups, and that not all of these are checked for.");

Element *Element_create(void) {
    // allocate the object
    Element *element = (Element *)ElementType.tp_alloc(&ElementType, 0);
    // check if the object was allocated
    if (!element) {
        PyErr_SetString(PyExc_TypeError, "could not create Element object");
        return NULL;
    }
    // set the ready flag to 0
    element->ready = 0;
    return element;
}

PyObject *Element_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    // required arguments are the pairing and the group
    PyObject *py_pairing;
    enum Group group;
    char *string = NULL;
    if (!PyArg_ParseTuple(args, "O!is", &PairingType, &py_pairing, &group, &string)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments, expected Pairing object, group, and string");
        return NULL;
    }
    // cast the arguments
    Pairing *pairing = (Pairing *)py_pairing;
    // build the result element and initialize it with the pairing and group
    Element *element = Element_create();
    switch (group) {
    case G1: element_init_G1(element->pbc_element, pairing->pbc_pairing); break;
    case G2: element_init_G2(element->pbc_element, pairing->pbc_pairing); break;
    case GT: element_init_GT(element->pbc_element, pairing->pbc_pairing); break;
    case Zr: element_init_Zr(element->pbc_element, pairing->pbc_pairing); break;
    default: Py_DECREF(element); PyErr_SetString(PyExc_ValueError, "invalid group"); return NULL;
    }
    element->pairing = pairing;
    // set the element to the string
    if (element_set_str(element->pbc_element, string, 10) == 0) {
        PyErr_SetString(PyExc_ValueError, "could not parse element from string");
        return NULL;
    }
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(element->pairing);
    element->ready = 1;
    return (PyObject *)element;
}

void Element_dealloc(Element *element) {
    // clear the element and decrement the reference count on the pairing if it's ready
    if (element->ready){
        element_clear(element->pbc_element);
        Py_DECREF(element->pairing);
    }
    // free the object
    Py_TYPE(element)->tp_free((PyObject *)element);
}

PyObject *Element_from_int(PyObject *cls, PyObject *args) {
    // required arguments are the pairing and the group
    PyObject *py_pairing;
    PyObject *py_val;
    if (!PyArg_ParseTuple(args, "O!O!", &PairingType, &py_pairing, &PyLong_Type, &py_val)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments, expected Pairing object and number");
        return NULL;
    }
    // cast the arguments
    Pairing *pairing = (Pairing *)py_pairing;
    // convert the number to an mpz_t
    mpz_t mpz_val;
    mpz_init_from_pynum(mpz_val, py_val);
    // build the result element and initialize it with the pairing and group
    Element *element = Element_create();
    element_init_Zr(element->pbc_element, pairing->pbc_pairing);
    element->pairing = pairing;
    // set the element to the number
    element_set_mpz(element->pbc_element, mpz_val);
    // clear the mpz_t
    mpz_clear(mpz_val);
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(element->pairing);
    element->ready = 1;
    return (PyObject *)element;
}

PyObject *Element_zero(PyObject *cls, PyObject *args) {
    // required arguments are the pairing and the group
    PyObject *py_pairing;
    enum Group group;
    if (!PyArg_ParseTuple(args, "O!i", &PairingType, &py_pairing, &group)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments, expected Pairing object and group");
        return NULL;
    }
    // cast the arguments
    Pairing *pairing = (Pairing *)py_pairing;
    // build the result element and initialize it with the pairing and group
    Element *element = Element_create();
    switch (group) {
    case G1: element_init_G1(element->pbc_element, pairing->pbc_pairing); break;
    case G2: element_init_G2(element->pbc_element, pairing->pbc_pairing); break;
    case GT: element_init_GT(element->pbc_element, pairing->pbc_pairing); break;
    case Zr: element_init_Zr(element->pbc_element, pairing->pbc_pairing); break;
    default: Py_DECREF(element); PyErr_SetString(PyExc_ValueError, "invalid group"); return NULL;
    }
    element->pairing = pairing;
    // set the element to 0
    element_set0(element->pbc_element);
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(element->pairing);
    element->ready = 1;
    return (PyObject *)element;
}

PyObject *Element_one(PyObject *cls, PyObject *args) {
    // required arguments are the pairing and the group
    PyObject *py_pairing;
    enum Group group;
    if (!PyArg_ParseTuple(args, "O!i", &PairingType, &py_pairing, &group)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments, expected Pairing object and group");
        return NULL;
    }
    // cast the arguments
    Pairing *pairing = (Pairing *)py_pairing;
    // build the result element and initialize it with the pairing and group
    Element *element = Element_create();
    switch (group) {
    case G1: element_init_G1(element->pbc_element, pairing->pbc_pairing); break;
    case G2: element_init_G2(element->pbc_element, pairing->pbc_pairing); break;
    case GT: element_init_GT(element->pbc_element, pairing->pbc_pairing); break;
    case Zr: element_init_Zr(element->pbc_element, pairing->pbc_pairing); break;
    default: Py_DECREF(element); PyErr_SetString(PyExc_ValueError, "invalid group"); return NULL;
    }
    element->pairing = pairing;
    // set the element to 1
    element_set1(element->pbc_element);
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(element->pairing);
    element->ready = 1;
    return (PyObject *)element;
}

PyObject *Element_random(PyObject *cls, PyObject *args) {
    // required arguments are the pairing and the group
    PyObject *py_pairing;
    enum Group group;
    if (!PyArg_ParseTuple(args, "O!i", &PairingType, &py_pairing, &group)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments, expected Pairing object and group");
        return NULL;
    }
    // cast the arguments
    Pairing *pairing = (Pairing *)py_pairing;
    // build the result element and initialize it with the pairing and group
    Element *element = Element_create();
    switch (group) {
    case G1: element_init_G1(element->pbc_element, pairing->pbc_pairing); break;
    case G2: element_init_G2(element->pbc_element, pairing->pbc_pairing); break;
    case GT: element_init_GT(element->pbc_element, pairing->pbc_pairing); break;
    case Zr: element_init_Zr(element->pbc_element, pairing->pbc_pairing); break;
    default: Py_DECREF(element); PyErr_SetString(PyExc_ValueError, "invalid group"); return NULL;
    }
    element->pairing = pairing;
    // make the element random
    element_random(element->pbc_element);
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(element->pairing);
    element->ready = 1;
    return (PyObject *)element;
}

PyObject *Element_from_hash(PyObject *cls, PyObject *args) {
    // required arguments are the pairing and the group
    PyObject *py_pairing;
    enum Group group;
    PyObject *py_bytes;
    if (!PyArg_ParseTuple(args, "O!iO!", &PairingType, &py_pairing, &group, &PyBytes_Type, &py_bytes)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments, expected Pairing object, group, and bytes");
        return NULL;
    }
    // cast the arguments
    Pairing *pairing = (Pairing *)py_pairing;
    // build the result element and initialize it with the pairing and group
    Element *element = Element_create();
    switch (group) {
    case G1: element_init_G1(element->pbc_element, pairing->pbc_pairing); break;
    case G2: element_init_G2(element->pbc_element, pairing->pbc_pairing); break;
    case GT: element_init_GT(element->pbc_element, pairing->pbc_pairing); break;
    case Zr: element_init_Zr(element->pbc_element, pairing->pbc_pairing); break;
    default: Py_DECREF(element); PyErr_SetString(PyExc_ValueError, "invalid group"); return NULL;
    }
    element->pairing = pairing;
    // convert the bytes to an element
    int size = PyBytes_Size(py_bytes);
    unsigned char *bytes = (unsigned char *)PyBytes_AsString(py_bytes);
    element_from_hash(element->pbc_element, bytes, size);
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(element->pairing);
    element->ready = 1;
    return (PyObject *)element;
}

PyObject *Element_from_bytes(PyObject *cls, PyObject *args) {
    // required arguments are the pairing and the group
    PyObject *py_pairing;
    enum Group group;
    PyObject *py_bytes;
    if (!PyArg_ParseTuple(args, "O!iO!", &PairingType, &py_pairing, &group, &PyBytes_Type, &py_bytes)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments, expected Pairing object, group, and bytes");
        return NULL;
    }
    // cast the arguments
    Pairing *pairing = (Pairing *)py_pairing;
    // build the result element and initialize it with the pairing and group
    Element *element = Element_create();
    switch (group) {
    case G1: element_init_G1(element->pbc_element, pairing->pbc_pairing); break;
    case G2: element_init_G2(element->pbc_element, pairing->pbc_pairing); break;
    case GT: element_init_GT(element->pbc_element, pairing->pbc_pairing); break;
    case Zr: element_init_Zr(element->pbc_element, pairing->pbc_pairing); break;
    default: Py_DECREF(element); PyErr_SetString(PyExc_ValueError, "invalid group"); return NULL;
    }
    element->pairing = pairing;
    // check the size of the bytes
    if (PyBytes_Size(py_bytes) != element_length_in_bytes(element->pbc_element)) {
        Py_DECREF(element);
        PyErr_SetString(PyExc_ValueError, "invalid number of bytes");
        return NULL;
    }
    // convert the bytes to an element
    unsigned char *bytes = (unsigned char *)PyBytes_AsString(py_bytes);
    element_from_bytes(element->pbc_element, bytes);
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(element->pairing);
    element->ready = 1;
    return (PyObject *)element;
}

PyObject *Element_from_bytes_compressed(PyObject *cls, PyObject *args) {
    // required arguments are the pairing and the group
    PyObject *py_pairing;
    enum Group group;
    PyObject *py_bytes;
    if (!PyArg_ParseTuple(args, "O!iO!", &PairingType, &py_pairing, &group, &PyBytes_Type, &py_bytes)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments, expected Pairing object, group, and bytes");
        return NULL;
    }
    // cast the arguments
    Pairing *pairing = (Pairing *)py_pairing;
    // build the result element and initialize it with the pairing and group
    Element *element = Element_create();
    switch (group) {
    case G1: element_init_G1(element->pbc_element, pairing->pbc_pairing); break;
    case G2: element_init_G2(element->pbc_element, pairing->pbc_pairing); break;
    case GT:
    case Zr: Py_DECREF(element); PyErr_SetString(PyExc_ValueError, "only Elements in G1 or G2 can be created from compressed bytes"); return NULL;
    default: Py_DECREF(element); PyErr_SetString(PyExc_ValueError, "invalid group"); return NULL;
    }
    element->pairing = pairing;
    // check the size of the bytes
    if (PyBytes_Size(py_bytes) != element_length_in_bytes_compressed(element->pbc_element)) {
        Py_DECREF(element);
        PyErr_SetString(PyExc_ValueError, "invalid number of bytes");
        return NULL;
    }
    // convert the bytes to an element
    unsigned char *bytes = (unsigned char *)PyBytes_AsString(py_bytes);
    element_from_bytes_compressed(element->pbc_element, bytes);
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(element->pairing);
    element->ready = 1;
    return (PyObject *)element;
}

PyObject *Element_from_bytes_x_only(PyObject *cls, PyObject *args) {
    // required arguments are the pairing and the group
    PyObject *py_pairing;
    enum Group group;
    PyObject *py_bytes;
    if (!PyArg_ParseTuple(args, "O!iO!", &PairingType, &py_pairing, &group, &PyBytes_Type, &py_bytes)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments, expected Pairing object, group, and bytes");
        return NULL;
    }
    // cast the arguments
    Pairing *pairing = (Pairing *)py_pairing;
    // build the result element and initialize it with the pairing and group
    Element *element = Element_create();
    switch (group) {
    case G1: element_init_G1(element->pbc_element, pairing->pbc_pairing); break;
    case G2: element_init_G2(element->pbc_element, pairing->pbc_pairing); break;
    case GT:
    case Zr: Py_DECREF(element); PyErr_SetString(PyExc_ValueError, "only Elements in G1 or G2 can be created from x-only bytes"); return NULL;
    default: Py_DECREF(element); PyErr_SetString(PyExc_ValueError, "invalid group"); return NULL;
    }
    element->pairing = pairing;
    // check the size of the bytes
    if (PyBytes_Size(py_bytes) != element_length_in_bytes_x_only(element->pbc_element)) {
        Py_DECREF(element);
        PyErr_SetString(PyExc_ValueError, "invalid number of bytes");
        return NULL;
    }
    // convert the bytes to an element
    unsigned char *bytes = (unsigned char *)PyBytes_AsString(py_bytes);
    element_from_bytes_x_only(element->pbc_element, bytes);
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(element->pairing);
    element->ready = 1;
    return (PyObject *)element;
}

PyObject *Element_to_bytes(PyObject *py_element) {
    // cast the argument
    Element *element = (Element *)py_element;
    // get the size of the buffer and allocate it
    int size = element_length_in_bytes(element->pbc_element);
    unsigned char buffer[size];
    // convert the element to bytes
    element_to_bytes(buffer, element->pbc_element);
    // return the buffer as a bytes object
    return PyBytes_FromStringAndSize((char *)buffer, size);
}

Py_hash_t Element_hash(PyObject *py_element) {
    // cast the argument
    Element *element = (Element *)py_element;
    // get the size of the buffer and allocate it
    int size = element_length_in_bytes(element->pbc_element);
    unsigned char buffer[size];
    // convert the element to bytes
    element_to_bytes(buffer, element->pbc_element);
    // initialize the hash
    Py_uhash_t hash = 14695981039346656037U;
    // iterate over the string
    for (Py_ssize_t i = 0; i < size; i++) {
        hash ^= (Py_uhash_t)buffer[i];
        hash *= 1099511628211U;
    }
    // add the pointer to the hash
    Py_uhash_t ptr = (Py_uhash_t)element->pbc_element->field;
    hash ^= ptr >> 4 | ptr << (8 * sizeof(ptr) - 4);
    // check if the hash is invalid
    if (hash == (Py_uhash_t)-1) {
        hash = (Py_uhash_t)-2;
    }
    // return the hash
    return (Py_hash_t)hash;
}

PyObject *Element_to_bytes_compressed(PyObject *py_element) {
    // cast the argument
    Element *element = (Element *)py_element;
    // make sure the element is in G1 or G2
    if (element->pbc_element->field != element->pairing->pbc_pairing->G1 && element->pbc_element->field != element->pairing->pbc_pairing->G2) {
        PyErr_SetString(PyExc_TypeError, "only Elements in G1 or G2 can be converted to compressed bytes");
        return NULL;
    }
    // get the size of the buffer and allocate it
    int size = element_length_in_bytes_compressed(element->pbc_element);
    unsigned char buffer[size];
    // convert the element to compressed bytes
    element_to_bytes_compressed(buffer, element->pbc_element);
    // return the buffer as a bytes object
    return PyBytes_FromStringAndSize((char *)buffer, size);
}

PyObject *Element_to_bytes_x_only(PyObject *py_element) {
    // cast the argument
    Element *element = (Element *)py_element;
    // make sure the element is in G1 or G2
    if (element->pbc_element->field != element->pairing->pbc_pairing->G1 && element->pbc_element->field != element->pairing->pbc_pairing->G2) {
        PyErr_SetString(PyExc_TypeError, "only Elements in G1 or G2 can be converted to x-only bytes");
        return NULL;
    }
    // get the size of the buffer and allocate it
    int size = element_length_in_bytes_x_only(element->pbc_element);
    unsigned char buffer[size];
    // convert the element to x-only bytes
    element_to_bytes_x_only(buffer, element->pbc_element);
    // return the buffer as a bytes object
    return PyBytes_FromStringAndSize((char *)buffer, size);
}

PyObject *Element_add(PyObject *py_lft, PyObject *py_rgt) {
    // check the type of arguments
    if (!PyObject_TypeCheck(py_lft, &ElementType) || !PyObject_TypeCheck(py_rgt, &ElementType)) {
        PyErr_SetString(PyExc_TypeError, "operands must be Elements");
        return NULL;
    }
    // declare the result element
    Element *ele_res;
    // convert both objects to Elements
    Element *ele_lft = (Element *)py_lft;
    Element *ele_rgt = (Element *)py_rgt;
    // make sure they're in the same ring
    if (ele_lft->pbc_element->field != ele_rgt->pbc_element->field) {
        PyErr_SetString(PyExc_ValueError, "only Elements in the same group can be added");
        return NULL;
    }
    // build and initialize the result element to the same group as the left element
    ele_res = Element_create();
    element_init_same_as(ele_res->pbc_element, ele_lft->pbc_element);
    ele_res->pairing = ele_lft->pairing;
    // add the two elements
    element_add(ele_res->pbc_element, ele_lft->pbc_element, ele_rgt->pbc_element);
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(ele_res->pairing);
    ele_res->ready = 1;
    return (PyObject *)ele_res;
}

PyObject *Element_sub(PyObject *py_lft, PyObject *py_rgt) {
    // check the type of arguments
    if (!PyObject_TypeCheck(py_lft, &ElementType) || !PyObject_TypeCheck(py_rgt, &ElementType)) {
        PyErr_SetString(PyExc_TypeError, "operands must be Elements");
        return NULL;
    }
    // declare the result element
    Element *ele_res;
    // convert both objects to Elements
    Element *ele_lft = (Element *)py_lft;
    Element *ele_rgt = (Element *)py_rgt;
    // make sure they're in the same ring
    if (ele_lft->pbc_element->field != ele_rgt->pbc_element->field) {
        PyErr_SetString(PyExc_ValueError, "only Elements in the same group can be subtracted");
        return NULL;
    }
    // build and initialize the result element to the same group as the left element
    ele_res = Element_create();
    element_init_same_as(ele_res->pbc_element, ele_lft->pbc_element);
    ele_res->pairing = ele_lft->pairing;
    // subtract the two elements
    element_sub(ele_res->pbc_element, ele_lft->pbc_element, ele_rgt->pbc_element);
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(ele_res->pairing);
    ele_res->ready = 1;
    return (PyObject *)ele_res;
}

PyObject *Element_div(PyObject *py_lft, PyObject *py_rgt) {
    // check the type of arguments
    if (!PyObject_TypeCheck(py_lft, &ElementType) || !PyObject_TypeCheck(py_rgt, &ElementType)) {
        PyErr_SetString(PyExc_TypeError, "operands must be Elements");
        return NULL;
    }
    // declare the result element
    Element *ele_res;
    // convert both objects to Elements
    Element *ele_lft = (Element *)py_lft;
    Element *ele_rgt = (Element *)py_rgt;
    // make sure they're in the same ring or one is in Zr
    if (ele_lft->pbc_element->field != ele_rgt->pbc_element->field) {
        PyErr_SetString(PyExc_ValueError, "only Elements in the same group can be divided");
        return NULL;
    }
    // build and initialize the result element to the same group as the left element
    ele_res = Element_create();
    element_init_same_as(ele_res->pbc_element, ele_lft->pbc_element);
    ele_res->pairing = ele_lft->pairing;
    // divide the two elements
    element_div(ele_res->pbc_element, ele_lft->pbc_element, ele_rgt->pbc_element);
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(ele_res->pairing);
    ele_res->ready = 1;
    return (PyObject *)ele_res;
}

PyObject *Element_pow(PyObject *py_lft, PyObject *py_rgt, PyObject *py_mod) {
    // check the type of the first argument
    if (!PyObject_TypeCheck(py_lft, &ElementType)) {
        PyErr_SetString(PyExc_TypeError, "the base must be an Element");
        return NULL;
    }
    // declare the result element
    Element *ele_res;
    // convert the first argument to an Element
    Element *ele_lft = (Element *)py_lft;
    // check the type of the second argument
    if (PyObject_TypeCheck(py_rgt, &ElementType)) {
        // convert the second argument to an Element
        Element *ele_rgt = (Element *)py_rgt;
        // make sure the second element is in Zr
        if (ele_rgt->pbc_element->field == ele_lft->pairing->pbc_pairing->Zr && (ele_lft->pbc_element->field == ele_lft->pairing->pbc_pairing->Zr || ele_lft->pbc_element->field->pairing)) {
            // build and initialize the result element to the same group as the left element
            ele_res = Element_create();
            element_init_same_as(ele_res->pbc_element, ele_lft->pbc_element);
            ele_res->pairing = ele_lft->pairing;
            // raise the element to the power
            element_pow_zn(ele_res->pbc_element, ele_lft->pbc_element, ele_rgt->pbc_element);
        } else {
            PyErr_SetString(PyExc_TypeError, "if the exponent is an Element, it must be in Zr and the base must be in Zr, G1, G2, or GT");
            return NULL;
        }
    } else if (PyLong_Check(py_rgt)) {
        // convert it to an mpz
        mpz_t mpz_lft;
        mpz_init_from_pynum(mpz_lft, py_rgt);
        // build and initialize the result element to the same group as the left element
        ele_res = Element_create();
        element_init_same_as(ele_res->pbc_element, ele_lft->pbc_element);
        ele_res->pairing = ele_lft->pairing;
        // raise the element to the power
        element_pow_mpz(ele_res->pbc_element, ele_lft->pbc_element, mpz_lft);
        // clean up the mpz
        mpz_clear(mpz_lft);
    } else {
        PyErr_SetString(PyExc_TypeError, "the exponent must be an Element or an integer");
        return NULL;
    }
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(ele_res->pairing);
    ele_res->ready = 1;
    return (PyObject *)ele_res;
}

PyObject *Element_mul(PyObject *py_lft, PyObject *py_rgt) {
    // declare the result element
    Element *ele_res;
    // check the type of arguments
    if (PyObject_TypeCheck(py_lft, &ElementType) && PyObject_TypeCheck(py_rgt, &ElementType)) {
        // convert both objects to Elements
        Element *ele_lft = (Element *)py_lft;
        Element *ele_rgt = (Element *)py_rgt;
        // make sure they're in the same ring or one is in Zr
        if (ele_lft->pbc_element->field == ele_rgt->pbc_element->field) {
            // build and initialize the result element to the same group as the left element
            ele_res = Element_create();
            element_init_same_as(ele_res->pbc_element, ele_lft->pbc_element);
            ele_res->pairing = ele_lft->pairing;
            // multiply the two elements
            element_mul(ele_res->pbc_element, ele_lft->pbc_element, ele_rgt->pbc_element);
        } else if (ele_rgt->pbc_element->field == ele_lft->pairing->pbc_pairing->Zr && ele_lft->pbc_element->field->pairing) {
            // build and initialize the result element to the same group as the left element
            ele_res = Element_create();
            element_init_same_as(ele_res->pbc_element, ele_lft->pbc_element);
            ele_res->pairing = ele_lft->pairing;
            // multiply the two elements
            element_mul_zn(ele_res->pbc_element, ele_lft->pbc_element, ele_rgt->pbc_element);
        } else if (ele_lft->pbc_element->field == ele_rgt->pairing->pbc_pairing->Zr && ele_rgt->pbc_element->field->pairing) {
            // build and initialize the result element to the same group as the right element
            ele_res = Element_create();
            element_init_same_as(ele_res->pbc_element, ele_rgt->pbc_element);
            ele_res->pairing = ele_rgt->pairing;
            // multiply the two elements
            element_mul_zn(ele_res->pbc_element, ele_rgt->pbc_element, ele_lft->pbc_element);
        } else {
            PyErr_SetString(PyExc_ValueError, "only Elements in the same group can be multiplied, or one must be in Zr and the other in G1, G2, or GT");
            return NULL;
        }
    } else if (PyLong_Check(py_rgt)) {
        // convert the left object to an Element
        Element *ele_lft = (Element *)py_lft;
        // convert the right object to an mpz
        mpz_t mpz_rgt;
        mpz_init_from_pynum(mpz_rgt, py_rgt);
        // build and initialize the result element to the same group as the left element
        ele_res = Element_create();
        element_init_same_as(ele_res->pbc_element, ele_lft->pbc_element);
        ele_res->pairing = ele_lft->pairing;
        // multiply the two elements
        element_mul_mpz(ele_res->pbc_element, ele_lft->pbc_element, mpz_rgt);
        // clean up the mpz
        mpz_clear(mpz_rgt);
    } else if (PyLong_Check(py_lft)) {
        // convert the right object to an Element
        Element *ele_rgt = (Element *)py_rgt;
        // convert the left object to an mpz
        mpz_t mpz_lft;
        mpz_init_from_pynum(mpz_lft, py_lft);
        // build and initialize the result element to the same group as the right element
        ele_res = Element_create();
        element_init_same_as(ele_res->pbc_element, ele_rgt->pbc_element);
        ele_res->pairing = ele_rgt->pairing;
        // multiply the two elements
        element_mul_mpz(ele_res->pbc_element, ele_rgt->pbc_element, mpz_lft);
        // clean up the mpz
        mpz_clear(mpz_lft);
    } else {
        PyErr_SetString(PyExc_TypeError, "operands must be Elements or integers");
        return NULL;
    }
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(ele_res->pairing);
    ele_res->ready = 1;
    return (PyObject *)ele_res;
}

PyObject *Element_neg(PyObject *py_arg) {
    // declare the result element
    Element *ele_res;
    // cast the argument
    Element *ele_arg = (Element *)py_arg;
    // build and initialize the result element to the same group as the argument
    ele_res = Element_create();
    element_init_same_as(ele_res->pbc_element, ele_arg->pbc_element);
    ele_res->pairing = ele_arg->pairing;
    // negate the element
    element_neg(ele_res->pbc_element, ele_arg->pbc_element);
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(ele_res->pairing);
    ele_res->ready = 1;
    return (PyObject *)ele_res;
}

PyObject *Element_invert(PyObject *py_arg) {
    // declare the result element
    Element *ele_res;
    // cast the argument
    Element *ele_arg = (Element *)py_arg;
    // build and initialize the result element to the same group as the argument
    ele_res = Element_create();
    element_init_same_as(ele_res->pbc_element, ele_arg->pbc_element);
    ele_res->pairing = ele_arg->pairing;
    // invert the element
    element_invert(ele_res->pbc_element, ele_arg->pbc_element);
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(ele_res->pairing);
    ele_res->ready = 1;
    return (PyObject *)ele_res;
}

PyObject *Element_cmp(PyObject *py_lft, PyObject *py_rgt, int op) {
    // check the type of arguments
    if (!PyObject_TypeCheck(py_lft, &ElementType) || !PyObject_TypeCheck(py_rgt, &ElementType)) {
        PyErr_SetString(PyExc_TypeError, "operands must be Elements");
        return NULL;
    }
    // convert both objects to Elements
    Element *ele_lft = (Element *)py_lft;
    Element *ele_rgt = (Element *)py_rgt;
    // make sure they're in the same ring
    if (ele_lft->pbc_element->field != ele_rgt->pbc_element->field) {
        PyErr_SetString(PyExc_ValueError, "only Elements in the same group can be compared");
        return NULL;
    }
    // compare the two elements
    switch (op) {
    case Py_EQ:
        if (element_cmp(ele_lft->pbc_element, ele_rgt->pbc_element)) {
            Py_RETURN_FALSE;
        } else {
            Py_RETURN_TRUE;
        }
    case Py_NE:
        if (element_cmp(ele_lft->pbc_element, ele_rgt->pbc_element)) {
            Py_RETURN_TRUE;
        } else {
            Py_RETURN_FALSE;
        }
    default:
        PyErr_SetString(PyExc_ValueError, "only == and != comparisons are supported");
        return NULL;
    }
}

PyObject *Element_is0(PyObject *py_element) {
    // cast the argument
    Element *element = (Element *)py_element;
    // check if the element is 0
    if (element_is0(element->pbc_element)) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

PyObject *Element_is1(PyObject *py_element) {
    // cast the argument
    Element *element = (Element *)py_element;
    // check if the element is 1
    if (element_is1(element->pbc_element)) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

Py_ssize_t Element_len(PyObject *py_element) {
    // cast the argument
    Element *element = (Element *)py_element;
    // check the size of the element
    Py_ssize_t sz_c = element_item_count(element->pbc_element);
    // check if the element is in Zr
    if (sz_c == 0) {
        PyErr_SetString(PyExc_TypeError, "Element is not dimensioned");
        return -1;
    }
    // return the length of the element
    return sz_c;
}

PyObject *Element_item(PyObject *py_element, Py_ssize_t sz_i) {
    // cast the argument
    Element *element = (Element *)py_element;
    // check the size of the element
    Py_ssize_t sz_c = element_item_count(element->pbc_element);
    // check if the element is in Zr
    if (sz_c == 0) {
        PyErr_SetString(PyExc_ValueError, "Element is not dimensioned");
        return NULL;
    }
    // check if the index is in range
    if (sz_i < 0 || sz_i >= sz_c) {
        PyErr_SetString(PyExc_IndexError, "index out of range");
        return NULL;
    }
    // get the item
    element_ptr item = element_item(element->pbc_element, sz_i);
    // build the result element
    Element *ele_res = Element_create();
    element_init_same_as(ele_res->pbc_element, item);
    ele_res->pairing = element->pairing;
    // set the item
    element_set(ele_res->pbc_element, item);
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(ele_res->pairing);
    ele_res->ready = 1;
    return (PyObject *)ele_res;
}

PyObject *Element_int(PyObject *py_element) {
    // cast the argument
    Element *element = (Element *)py_element;
    // check the size of the element
    Py_ssize_t sz_c = element_item_count(element->pbc_element);
    // check if the element is in Zr
    if (sz_c != 0) {
        PyErr_SetString(PyExc_ValueError, "cannot convert multidimensional point to int");
        return NULL;
    }
    // allocate an mpz
    mpz_t mpz_int;
    mpz_init(mpz_int);
    // convert the element to an mpz
    element_to_mpz(mpz_int, element->pbc_element);
    // convert the mpz to a Python integer
    PyObject *py_int = mpz_to_pynum(mpz_int);
    // clean up the mpz
    mpz_clear(mpz_int);
    return py_int;
}

PyObject *Element_str(PyObject *py_element) {
    // cast the argument
    Element *element = (Element *)py_element;
    // allocate a buffer
    char buffer[4096];
    // convert the element to a string
    int size = element_snprint(buffer, sizeof(buffer), element->pbc_element);
    // return the string
    return PyUnicode_FromStringAndSize(buffer, size);
}

PyObject *Element_order(PyObject *py_element) {
    // cast the argument
    Element *element = (Element *)py_element;
    // convert the mpz to a Python integer
    return mpz_to_pynum(element->pbc_element->field->order);
}

PyMemberDef Element_members[] = {
    {NULL},
};

PyMethodDef Element_methods[] = {
    {"zero", (PyCFunction)Element_zero, METH_VARARGS | METH_CLASS, "Creates an element representing the additive identity for its group."},
    {"one", (PyCFunction)Element_one, METH_VARARGS | METH_CLASS, "Creates an element representing the multiplicative identity for its group."},
    {"random", (PyCFunction)Element_random, METH_VARARGS | METH_CLASS, "Creates a random element from the given group."},
    {"from_int", (PyCFunction)Element_from_int, METH_VARARGS | METH_CLASS, "Creates an element in Zr from the given integer."},
    {"from_hash", (PyCFunction)Element_from_hash, METH_VARARGS | METH_CLASS, "Creates an Element from the given hash value."},
    {"from_bytes", (PyCFunction)Element_from_bytes, METH_VARARGS | METH_CLASS, "Creates an element from a byte string."},
    {"from_bytes_compressed", (PyCFunction)Element_from_bytes_compressed, METH_VARARGS | METH_CLASS, "Creates an element from a byte string using the compressed format."},
    {"from_bytes_x_only", (PyCFunction)Element_from_bytes_x_only, METH_VARARGS | METH_CLASS, "Creates an element from a byte string using the x-only format."},
    {"to_bytes", (PyCFunction)Element_to_bytes, METH_NOARGS, "Converts the element to a byte string."},
    {"to_bytes_x_only", (PyCFunction)Element_to_bytes_x_only, METH_NOARGS, "Converts the element to a byte string using the x-only format."},
    {"to_bytes_compressed", (PyCFunction)Element_to_bytes_compressed, METH_NOARGS, "Converts the element to a byte string using the compressed format."},
    {"is0", (PyCFunction)Element_is0, METH_NOARGS, "Returns True if the element is additive identity."},
    {"is1", (PyCFunction)Element_is1, METH_NOARGS, "Returns True if the element is multiplicative identity."},
    {"order", (PyCFunction)Element_order, METH_NOARGS, "Returns the order of the element."},
    {NULL},
};

PyNumberMethods Element_num_meths = {
    Element_add,    // binaryfunc nb_add;
    Element_sub,    // binaryfunc nb_subtract;
    Element_mul,    // binaryfunc nb_multiply;
    0,              // binaryfunc nb_remainder;
    0,              // binaryfunc nb_divmod;
    Element_pow,    // ternaryfunc nb_power;
    Element_neg,    // unaryfunc nb_negative;
    0,              // unaryfunc nb_positive;
    0,              // unaryfunc nb_absolute;
    0,              // inquiry nb_bool;
    Element_invert, // unaryfunc nb_invert;
    0,              // binaryfunc nb_lshift;
    0,              // binaryfunc nb_rshift;
    0,              // binaryfunc nb_and;
    0,              // binaryfunc nb_xor;
    0,              // binaryfunc nb_or;
    Element_int,    // unaryfunc nb_int;
    0,              // void *nb_reserved;
    0,              // unaryfunc nb_float;
    0,              // binaryfunc nb_inplace_add;
    0,              // binaryfunc nb_inplace_subtract;
    0,              // binaryfunc nb_inplace_multiply;
    0,              // binaryfunc nb_inplace_remainder;
    0,              // ternaryfunc nb_inplace_power;
    0,              // binaryfunc nb_inplace_lshift;
    0,              // binaryfunc nb_inplace_rshift;
    0,              // binaryfunc nb_inplace_and;
    0,              // binaryfunc nb_inplace_xor;
    0,              // binaryfunc nb_inplace_or;
    0,              // binaryfunc nb_floor_divide;
    Element_div,    // binaryfunc nb_true_divide;
    0,              // binaryfunc nb_inplace_floor_divide;
    0,              // binaryfunc nb_inplace_true_divide;
};

PySequenceMethods Element_sq_meths = {
    Element_len,    // inquiry sq_length;
    0,              // binaryfunc sq_concat;
    0,              // intargfunc sq_repeat;
    Element_item,   // intargfunc sq_item;
    0,              // intintargfunc sq_slice;
    0,              // intobjargproc sq_ass_item;
    0,              // intintobjargproc sq_ass_slice
};

PyTypeObject ElementType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pypbc.Element",                          /* tp_name */
    sizeof(Element),                          /* tp_basicsize */
    0,                                        /* tp_itemsize */
    (destructor)Element_dealloc,              /* tp_dealloc */
    0,                                        /* tp_print */
    0,                                        /* tp_getattr */
    0,                                        /* tp_setattr */
    0,                                        /* tp_reserved */
    Element_str,                              /* tp_repr */
    &Element_num_meths,                       /* tp_as_number */
    &Element_sq_meths,                        /* tp_as_sequence */
    0,                                        /* tp_as_mapping */
    Element_hash,                             /* tp_hash */
    0,                                        /* tp_call */
    Element_str,                              /* tp_str */
    0,                                        /* tp_getattro */
    0,                                        /* tp_setattro */
    0,                                        /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    Element__doc__,                           /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    Element_cmp,                              /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    0,                                        /* tp_iter */
    0,                                        /* tp_iternext */
    Element_methods,                          /* tp_methods */
    Element_members,                          /* tp_members */
    0,                                        /* tp_getset */
    0,                                        /* tp_base */
    0,                                        /* tp_dict */
    0,                                        /* tp_descr_get */
    0,                                        /* tp_descr_set */
    0,                                        /* tp_dictoffset */
    0,                                        /* tp_init */
    0,                                        /* tp_alloc */
    Element_new,                              /* tp_new */
};

/*******************************************************************************
*                                    Module                                    *
*******************************************************************************/

PyDoc_STRVAR(pypbc__doc__,
    "A Python wrapper for the PBC library.\n"
    "\n"
    "This module provides a Python interface to the PBC library, which is a\n"
    "library for pairing-based cryptography. It provides a Pythonic interface\n"
    "to the PBC library, allowing for the creation of pairings, elements, and\n"
    "parameters, as well as operations on these objects.\n");

PyMethodDef pypbc_methods[] = {
    {NULL},
};

PyModuleDef pypbc_module = {
    PyModuleDef_HEAD_INIT,
    "pypbc",
    pypbc__doc__,
    -1,
    pypbc_methods,
};

PyMODINIT_FUNC PyInit_pypbc(void) {
    // initialize the types
    if (PyType_Ready(&ParametersType) < 0) {
        return NULL;
    }
    if (PyType_Ready(&PairingType) < 0) {
        return NULL;
    }
    if (PyType_Ready(&ElementType) < 0) {
        return NULL;
    }
    // create the module
    PyObject *module = PyModule_Create(&pypbc_module);
    if (module == NULL) {
        return NULL;
    }
    // increment the reference count for the types
    Py_INCREF(&PairingType);
    Py_INCREF(&ParametersType);
    Py_INCREF(&ElementType);
    // add the types to the module
    PyModule_AddObject(module, "Parameters", (PyObject *)&ParametersType);
    PyModule_AddObject(module, "Pairing", (PyObject *)&PairingType);
    PyModule_AddObject(module, "Element", (PyObject *)&ElementType);
    // add the group constants
    PyModule_AddObject(module, "G1", PyLong_FromLong(G1));
    PyModule_AddObject(module, "G2", PyLong_FromLong(G2));
    PyModule_AddObject(module, "GT", PyLong_FromLong(GT));
    PyModule_AddObject(module, "Zr", PyLong_FromLong(Zr));
    return module;
}
