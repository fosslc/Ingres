/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** wcsmo.h
**
** Description
**      Function prototypes for the ddcmo module.
**
** History:
**      16-Jul-98 (fanra01)
**          Created.
**
*/
#ifndef DDCMO_INCLUDED
#define DDCMO_INCLUDED

FUNC_EXTERN STATUS
ddc_define (void);

FUNC_EXTERN STATUS
ddc_sess_attach (PDB_CACHED_SESSION  dbSession);

FUNC_EXTERN VOID
ddc_sess_detach (PDB_CACHED_SESSION dbSession);

#endif
