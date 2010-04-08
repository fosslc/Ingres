/*
**    Copyright (c) 1991, 2008 Ingres Corporation
*/
#ifndef DLINT_H

/*
** dlint.h -- Internal header file that DL internal routines might need.
**
** History:
**	25-jul-91 (jab)
**		Written
**	28-jan-92 (jab)
**		Modified (heavily) for CL-spec conformance.
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**      13-mar-2002 (loera01)
**          Removed Unix "#ifdef's".  Changed DLint structure to DLhandle,
**          and changed contents according to new design of DL.
**  20-nov-07 (Ralph Loen) Bug 119497
**          Added location field to DLhandle structure.
**	13-nov-2008 (joea)
**	    Use compat.h definition of SETCLERR.
*/

#define	DLINT_H


# define	DL_EXT_NAME	".exe"

#define	DL_TXT_NAME	".dsc"

GLOBALREF int DLdebugset;
#define	DLdebug	if(DLdebugset == 1) SIprintf

#define	DL_valid	0x1234
#define	DL_invalid	0x4321

typedef struct 
{
    i4 func;
    char *logname;
    char *sym;
    PTR  location;
} DLhandle;

#if CL_HAS_PROTOS
STATUS DLosprepare(char *vers, LOCATION *locp, char *syms[],
	struct DLint *localareap, CL_ERR_DESC *errp);
STATUS DLosunload(PTR cookie, CL_ERR_DESC *errp);
STATUS DLparsedesc(LOCATION *locp, CL_ERR_DESC *errp);
#else	/* CL_HAS_PROTOS */
STATUS DLosprepare();
STATUS DLosunload();
STATUS DLparsedesc();
#endif	/* CL_HAS_PROTOS */
#endif	/* DLINT_H */
