#pragma once

// python stuff
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h"

// pbc stuff
#include <pbc/pbc.h>

/*******************************************************************************
* pypbc.c                                                                      *
*                                                                              *
* Modifications by Jemtaly                                                     *
* Copyright (c) 2024                                                           *
*                                                                              *
* Originally written by Geremy Condra                                          *
* Licensed under GPLv3                                                         *
* Released 11 October 2009                                                     *
*                                                                              *
* This file contains the types and functions needed to use PBC from Python 3.  *
*******************************************************************************/

#define PBC_DEBUG

// used to see which group a given element is in
enum Group {G1, G2, GT, Zr};

// the param type
typedef struct {
    PyObject_HEAD
    int ready;
    pbc_param_t pbc_params;
} Parameters;

Parameters *Parameters_create();
PyObject *Parameters_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
void Parameters_dealloc(Parameters *parameter);

PyMemberDef Parameters_members[];
PyMethodDef Parameters_methods[];
PyTypeObject ParametersType;

// the pairing type
typedef struct {
    PyObject_HEAD
    int ready;
    pairing_t pbc_pairing;
} Pairing;

Pairing *Pairing_create();
PyObject *Pairing_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
void Pairing_dealloc(Pairing *pairing);

PyMemberDef Pairing_members[];
PyMethodDef Pairing_methods[];
PyTypeObject PairingType;

// the element type
typedef struct {
    PyObject_HEAD
    int ready;
    element_t pbc_element;
    Pairing *pairing;
} Element;

Element *Element_create();
PyObject *Element_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
void Element_dealloc(Element *element);

PyMemberDef Element_members[];
PyMethodDef Element_methods[];
PyTypeObject ElementType;
