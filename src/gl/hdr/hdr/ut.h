/*
**	Copyright (c) 2004 Ingres Corporation
*/
# ifndef UT_HDR_INCLUDED
# define UT_HDR_INCLUDED 1

#ifdef CL_PROTOTYPED
/* needed for LOCATION definition */
#include    <lo.h>
#endif
#include    <utcl.h>

/**CL_SPEC
** Name:	UT.h	- Define UT function externs
**
** Specification:
**
** Description:
**	Contains UT function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**	21-oct-1996 (canor01)
**	    Add prototype for UTcompile_ex().
**	31-mar-1997 (canor01)
**	    Protect against multiple inclusion of ut.h.
**	07-apr-1997 (canor01)
**	    Change prototype for UTcompile_ex(): parms is an array
**	    of pointers.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      14-Jan-2004 (hanje04)
**          Change functions using old, single argument va_start (in
**          varargs.h), to use newer (ANSI) 2 argument version (in stdargs.h).
**          This is because the older version is no longer implemented in
**          version 3.3.1 of GCC.
**/
FUNC_EXTERN STATUS  UTcompile(
#ifdef CL_PROTOTYPED
	    LOCATION	*srcfile, 
	    LOCATION	*objfile, 
	    LOCATION	*outfile, 
	    bool	*pristine, 
	    CL_ERR_DESC *clerror
#endif
);

FUNC_EXTERN STATUS  UTcompile_ex(
#ifdef CL_PROTOTYPED
	    LOCATION	*srcfile, 
	    LOCATION	*objfile, 
	    LOCATION	*outfile, 
	    char	**parms,
		bool      append,
	    bool	*pristine, 
	    CL_ERR_DESC *clerror
#endif
);

FUNC_EXTERN STATUS  UTdeffile(
#ifdef CL_PROTOTYPED
	    char	*progname,
	    char	*buf,
	    LOCATION	*ploc,
	    CL_ERR_DESC *clerror
#endif
);

FUNC_EXTERN STATUS  UTedit(
#ifdef CL_PROTOTYPED
	    char	*e, 
	    LOCATION	*f
#endif
);

#ifndef UTexe
FUNC_EXTERN STATUS  UTexe(i4,
        LOCATION *,
        PTR,
        PTR,
        char *,
        CL_ERR_DESC *,
        char *,
        ...)
;
#endif

FUNC_EXTERN STATUS  UTlink(
#ifdef CL_PROTOTYPED
	    LOCATION	*objs[], 
	    char	*usrlist[], 
	    LOCATION	*exe, 
	    LOCATION	*outfile, 
	    bool	*pristine, 
	    CL_ERR_DESC *clerror
#endif
);

FUNC_EXTERN STATUS  UTprint(
#ifdef CL_PROTOTYPED
	    char	*printer_cmd, 
	    LOCATION	*filename, 
	    bool	delete,
	    i4		copies, 
	    char	*title, 
	    char	*destination
#endif
);

# endif /* ! UT_HDR_INCLUDED */
