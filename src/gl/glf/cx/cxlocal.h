/*
** Copyright (c) 2010 Ingres Corporation
*/
#ifndef CXLOCAL_H
#define CXLOCAL_H

#include <compat.h>

/**
** Name: cxlocal.h - CX compatibility library definitions.
**
** Description:
**      This header contains the description of the types, constants and
**      functions used internally in CX ('C'luster e'X'tension).
**
** History:
**      03-nov-2010 (joea)
**          Created.
**/

STATUS cx_dl_load_lib(char *libname, void **plibhandle, void *func_array[],
                      char *name_array[], int numentries);

#endif
