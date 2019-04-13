/*
 * A Python binding for cracklib.
 *
 * Parts of this code are based on work Copyright (c) 2003 by Domenico
 * Andreoli.
 *
 * Copyright (c) 2008, 2009, 2012 Jan Dittberner <jan@dittberner.info>
 *
 * This file is part of cracklib.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef PYTHON_H
#include PYTHON_H
#else
#include <Python.h>
#endif
#if PY_MAJOR_VERSION >= 3
#define IS_PY3K
#endif
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#include <crack.h>
#include <locale.h>
#ifdef HAVE_LIBINTL_H
#include <libintl.h>
#endif

#ifdef HAVE_PTHREAD_H
static pthread_mutex_t cracklib_mutex = PTHREAD_MUTEX_INITIALIZER;
#define LOCK() pthread_mutex_lock(&cracklib_mutex)
#define UNLOCK() pthread_mutex_unlock(&cracklib_mutex)
#else
#define LOCK()
#define UNLOCK()
#endif

#define DICT_SUFFIX ".pwd"

static char _cracklib_FascistCheck_doc [] =
	"arguments: passwd, dictpath (optional)\n"
	"\n"
	"  passwd - password to be checked for weakness\n"
	"  dictpath - full path name to the cracklib dictionary database\n"
	"\n"
	"if dictpath is not specified the default dictionary database\n"
	"will be used.\n"
	"\n"
	"return value: the same password passed as first argument.\n"
	"\n"
	"if password is weak, exception ValueError is raised with argument\n"
	"set to the reason of weakness.\n"
;

static PyObject *
_cracklib_FascistCheck(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char *candidate, *dict;
    char *defaultdict = NULL;
    const char *result;
    struct stat st;
    char *keywords[] = {"pw", "dictpath", NULL};
    char *dictfile;

    self = NULL;
    candidate = NULL;
    dict = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|s", keywords,
                                     &candidate, &dict))
    {
        PyErr_SetString(PyExc_ValueError, "error parsing arguments");
        return NULL;
    }

    if (candidate == NULL)
    {
        PyErr_SetString(PyExc_ValueError, "first argument was not a string!");
        return NULL;
    }
    if (dict != NULL)
    {
        if (dict[0] != '/')
        {
            PyErr_SetString(PyExc_ValueError,
                            "second argument was not an absolute path!");
            return NULL;
        }
        dictfile = malloc(strlen(dict) + sizeof(DICT_SUFFIX));
        if (dictfile == NULL)
        {
            PyErr_SetFromErrnoWithFilename(PyExc_OSError, dict);
            return NULL;
        }
        sprintf(dictfile, "%s" DICT_SUFFIX, dict);
        if (lstat(dictfile, &st) == -1)
        {
            PyErr_SetFromErrnoWithFilename(PyExc_OSError, dictfile);
            free(dictfile);
            return NULL;
        }
        free(dictfile);
    } else
    {
        defaultdict = strdup(GetDefaultCracklibDict());
        if (errno == ENOMEM) {
            PyErr_SetFromErrno(PyExc_OSError);
            return NULL;
        }
        dictfile = malloc(strlen(defaultdict) + sizeof(DICT_SUFFIX));
        if (dictfile == NULL)
        {
            PyErr_SetFromErrnoWithFilename(PyExc_OSError, defaultdict);
            free(defaultdict);
            return NULL;
        }
        sprintf(dictfile, "%s" DICT_SUFFIX, defaultdict);
        if (lstat(dictfile, &st) == -1)
        {
            PyErr_SetFromErrnoWithFilename(PyExc_OSError, dictfile);
            free(defaultdict);
            free(dictfile);
            return NULL;
        }
        free(dictfile);
    }

	setlocale(LC_ALL, "");
#ifdef ENABLE_NLS
	textdomain("cracklib");
#endif

    LOCK();
    result = FascistCheck(candidate, dict ? dict : defaultdict);
    UNLOCK();

    if (defaultdict != NULL)
    {
        free(defaultdict);
    }

    if (result != NULL)
    {
    	PyErr_SetString(PyExc_ValueError, result);
        return NULL;
    }
    return Py_BuildValue("s", candidate);
}

static PyMethodDef
_cracklibmethods[] =
{
    {"FascistCheck", (PyCFunction) _cracklib_FascistCheck,
     METH_VARARGS | METH_KEYWORDS, _cracklib_FascistCheck_doc},
    {NULL, NULL},
};

static char _cracklib_doc[] =
    "Python bindings for cracklib.\n"
    "\n"
    "This module enables the use of cracklib features from within a Python\n"
    "program or interpreter.\n"
;

#ifdef IS_PY3K

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "_cracklib",
    _cracklib_doc,
    0,
    _cracklibmethods,
    NULL,
    NULL,
    NULL,
    NULL
};

#define INITERROR return NULL

PyObject *
PyInit__cracklib(void)

#else
#define INITERROR return

void
init_cracklib(void)
#endif
{
#ifdef IS_PY3K
    PyObject *module = PyModule_Create(&moduledef);
#else
    PyObject *module = Py_InitModule3("_cracklib", _cracklibmethods, _cracklib_doc);
#endif
    if (module == NULL)
        INITERROR;
#ifdef IS_PY3K
    return module;
#endif
}
