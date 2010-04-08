/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** wcsmo.h
**
** Description
**      Function prototypes for the wcsmo module.
**
** History:
**      16-Jul-98 (fanra01)
**          Created.
**
*/
#ifndef WCSMO_INCLUDED
#define WCSMO_INCLUDED

#include <wcsfile.h>

FUNC_EXTERN STATUS
wcs_floc_define (void);

FUNC_EXTERN STATUS
wcs_floc_attach (char * name, WCS_LOCATION* wcs_loc_info);

FUNC_EXTERN VOID
wcs_floc_detach (char * name);

#endif
