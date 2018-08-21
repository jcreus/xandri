#include <Python.h>
#include "structmember.h"
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include "numpy/arrayobject.h"

#include "../blob.h"

static PyTypeObject WriterType;

typedef struct {
    PyObject_HEAD
    PyObject *name;
} Writer;

static PyObject *Writer_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    Writer *self = (Writer*)WriterType.tp_new(type, args, kwargs);
    if (!self) {
        return 0;
    }
    return (PyObject*)self;
}

static int Writer_init(Writer *self, PyObject *args, PyObject *kwargs) {
    const char *c_name = NULL;

    static char *kwlist[] = {"name", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s:__init__", kwlist, &c_name)) {
        return -1;
    }

    if (c_name) {
        PyObject *name = PyUnicode_FromString(c_name);
        Py_XINCREF(name);
        self->name = name;

        int ret = blob_create((char*)c_name);
        if (ret != 0) { PyErr_SetString(PyExc_ValueError, "Could not create blob."); }
    }
    return 0;
}

static PyObject *Writer_add_index(Writer *self, PyObject *args, PyObject *kwargs) {
    const char *c_name = NULL;
    PyObject *arr_arg = NULL;
    PyArrayObject *arr_obj = NULL;
    int width;

    static char *kwlist[] = {"name", "array", "width", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sOi", kwlist, &c_name, &arr_arg, &width)) {
        PyErr_SetString(PyExc_ValueError, "Invalid arguments, or something.");
        return 0;
    }
    arr_obj = PyArray_GETCONTIGUOUS((PyArrayObject*)arr_arg);
    uint64_t *data = (uint64_t*)PyArray_DATA(arr_obj);

    PyObject *ascii = PyUnicode_AsASCIIString(self->name);

    blob_index_from_memory((char*)PyBytes_AsString(ascii),
                            (char*)c_name,
                            data,
                            PyArray_DIM(arr_obj, 0),
                            width,
                            1);
    Py_DECREF(arr_obj);
    Py_DECREF(ascii);

    return Py_BuildValue("");
}

static void Writer_dealloc(Writer* self) {
    Py_TYPE(self)->tp_free((PyObject *)self);
}


static PyMemberDef Writer_members[] = {
    {"name", T_OBJECT_EX, offsetof(Writer, name), 0, "Name of the frame"},
    {NULL}
};

static PyMethodDef Writer_methods[] = {
    {"add_index", (PyCFunction)Writer_add_index, METH_VARARGS | METH_KEYWORDS, NULL},
    {"add_key", (PyCFunction)Writer_add_index, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}
};

static PyTypeObject WriterType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_xandri.Writer",                           /*tp_name*/
    sizeof(Writer),                             /*tp_basicsize*/
    0,                                          /*tp_itemsize*/
    (destructor)Writer_dealloc,                 /*tp_dealloc*/
    0,                                          /*tp_print*/
    0,                                          /*tp_getattr*/
    0,                                          /*tp_setattr*/
    0,                                          /*tp_compare*/
    0,                                          /*tp_repr*/
    0,                                          /*tp_as_number*/
    0,                                          /*tp_as_sequence*/
    0,                                          /*tp_as_mapping*/
    0,                                          /*tp_hash */
    0,                                          /*tp_call*/
    0,                                          /*tp_str*/
    0,                                          /*tp_getattro*/
    0,                                          /*tp_setattro*/
    0,                                          /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,                         /*tp_flags*/
    0,                                          /*tp_doc*/
    0,                                          /*tp_traverse*/
    0,                                          /*tp_clear*/
    0,                                          /*tp_richcompare*/
    0,                                          /*tp_weaklistoffset*/
    0,                                          /*tp_iter*/
    0,                                          /*tp_iternext*/
    Writer_methods,                             /*tp_methods*/
    Writer_members,                             /*tp_members*/
    0,                                          /*tp_getsets*/
    0,                                          /*tp_base*/
    0,                                          /*tp_dict*/
    0,                                          /*tp_descr_get*/
    0,                                          /*tp_descr_set*/
    0,                                          /*tp_dictoffset*/
    (initproc)Writer_init,                      /*tp_init*/
    0,                                          /*tp_alloc*/
    Writer_new,                                 /*tp_new*/
};


static PyObject* _xandri_write(PyObject* self) {
    return Py_BuildValue("s", "uniqueCombinations() return value (is of type 'string')");
}

static PyObject* _xandri_row_writer(PyObject* self) {
    return Py_BuildValue("s", "uniqueCombinations() return value (is of type 'string')");
}

/*static PyObject* _xandri_remove(PyObject* self) {
    return Py_BuildValue("s", "uniqueCombinations() return value (is of type 'string')");
}*/

static PyMethodDef _xandri_methods[] = {
    {"write", (PyCFunction) _xandri_write, METH_VARARGS, NULL},
    {"row_writer", (PyCFunction) _xandri_row_writer, METH_VARARGS, NULL},
    {NULL}
};

static struct PyModuleDef _xandri = {
    PyModuleDef_HEAD_INIT,
    "_xandri",
    "Documentation goes here",
    -1,
    _xandri_methods
};

void register_type(PyObject *module, const char *name, PyTypeObject *type) {
    if (!PyType_Ready(type)) {
        Py_INCREF(type);
        if (PyModule_AddObject(module, name, (PyObject*)type)) {
            Py_DECREF(type);
        }
    }
}

PyMODINIT_FUNC PyInit__xandri(void) {
    PyObject *obj = PyModule_Create(&_xandri);
    WriterType.tp_new = PyType_GenericNew;
    register_type(obj, "Writer", &WriterType);
    return obj;
}
