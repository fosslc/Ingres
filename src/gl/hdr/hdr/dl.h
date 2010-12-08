/*
**	Copyright (c) 2004 Ingres Corporation
*/
#include    <dlcl.h>
#include    <lo.h>

/**CL_SPEC
** Name:	DL.h	- Define DL function externs
**
** Specification:
**
** Description:
**	Contains DL function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**	11-aug-93 (ed)
**	    changed DLprepare to PTR * since routine was written
**	    to return a PTR not pass a PTR
**	21-oct-1996 (canor01)
**	    Add prototypes for DLcreate_loc() and DLprepare_loc().
**	24-mar-1997 (canor01)
**	    Add prototype for DLdelete_loc().
**	07-may-1997 (canor01)
**	    Add prototype for DLconstructloc().
**	03-jun-97 (leighb)
**	    If you are going to reference the LOCATION structure, you have
**	    to include the header that defines it.
**	13-jun-1997 (canor01)
**	    Add extra parameter to DLcreate_loc().
**	16-jun-1997 (canor01)
**	    Correct prototype for DLbind().  addr is a PTR*.
**	22-jun-97 (mcgem01)
**	    Reintroduce Leigh's change of 6/3/97 which had been backed
**	    out in error.
**	13-aug-1997 (canor01)
**	    Added 'flags' parameter to DLprepare_loc().
**	08-sep-1997 (canor01)
**	    Add 'append' parameter to DLcreate_loc().
**	29-dec-1997 (canor01)
**	    Add 'miscparms' parameter to DLcreate_loc().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      19-apr-2004 (loera01)
**          Added new DLload() routine.
**	1-Dec-2010 (kschendel) SIR 124685
**	    Kill CL_PROTOTYPED (always on now).
**/

FUNC_EXTERN STATUS DLbind(
	    PTR		handle, 
	    char	*sym, 
	    PTR		*addr, 
	    CL_ERR_DESC *errp
);

FUNC_EXTERN STATUS DLcreate(
	    char	*exename,
	    char	*vers,
	    char	*dlmodname,
	    char	*in_objs[],
	    char	*in_libs[],
	    char	*exp_fcns[],
	    CL_ERR_DESC	*err
);

FUNC_EXTERN STATUS DLcreate_loc(
            char        *exename,
            char        *vers,
            char        *dlmodname,
            char        *in_objs[],
            char        *in_libs[],
            char        *exp_fcns[],
	    LOCATION    *dlloc,
	    char	*parms,
	    LOCATION    *errloc,
	    bool	append,
	    char	*miscparms,
            CL_ERR_DESC *err
);

FUNC_EXTERN STATUS DLdelete(
	    char	*dlmodname, 
	    CL_ERR_DESC *err
);

FUNC_EXTERN STATUS DLdelete_loc(
	    char	*dlmodname, 
	    LOCATION    *dlloc,
	    CL_ERR_DESC *err
);

FUNC_EXTERN STATUS DLnameavail(
	    char	    *dlmodname, 
	    CL_ERR_DESC	    *err
);

FUNC_EXTERN STATUS DLload(
            LOCATION *ploc, 
            char *dlmodname, 
            char *syms[], 
            PTR *handle,
            CL_ERR_DESC *errp
);

FUNC_EXTERN STATUS DLprepare(
	    char	    *vers, 
	    char	    *dlmodname, 
	    char	    *syms[], 
	    PTR		    *handle,
	    CL_ERR_DESC	    *errp
);

FUNC_EXTERN STATUS DLprepare_loc(
            char            *vers,
            char            *dlmodname,
            char            *syms[],
	    LOCATION	    *dlloc,
	    i4		    flags,
            PTR             *handle,
            CL_ERR_DESC     *errp
);

FUNC_EXTERN STATUS DLunload(
	    PTR		    handle, 
	    CL_ERR_DESC	    *errp
);

FUNC_EXTERN STATUS DLconstructloc(
	    LOCATION	    *inloc,
	    char	    *buffer,
	    LOCATION	    *outloc,
	    CL_ERR_DESC	    *errp
);
