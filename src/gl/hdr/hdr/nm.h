/*
**  Copyright (c) 2004 Ingres Corporation
*/
# ifndef NM_HDR_INCLUDED
# define NM_HDR_INCLUDED 1

/* needed for the FILE definition */
#include    <si.h>
#include    <nmcl.h>

/**CL_SPEC
** Name:	NM.h	- Define NM function externs
**
** Specification:
**
** Description:
**	Contains NM function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**	24-jul-1995 (duursma)
**	    replaced the non-existent (typo?) NMgtInstAt() with NMgtIngAt(),
**	    which was recently added to VMS (and already existed on UNIX).
**	16-aug-95 (hanch04)
**	    change NMgtIngAt to return STATUS
**	31-mar-1997 (canor01)
**	    Protect against multiple inclusion of nm.h.
**	14-mar-2003 (somsa01)
**	    Added NMgtTLSCleanup() for Windows.
**	17-dec-2003 (somsa01)
**	    Added NMlockSymbolData(), NMunlockSymbolData() and
**	    NMlogOperation().
**	03-feb-2004 (somsa01)
**	    Backed out last change for now.
**	29-mar-2004 (somsa01)
**	    Added prototype for NMlogOperation().
**	1-Dec-2010 (kschendel) SIR 124685
**	    Kill CL_PROTOTYPED (always on now).
**/

FUNC_EXTERN STATUS  NMf_open(
	    char	*mode, 
	    char	*fname, 
	    FILE	**fptr
);

FUNC_EXTERN char *  NMgetpart(
	    char	*buf
);

FUNC_EXTERN VOID    NMgtAt(
	    char	*name, 
	    char	**ret_val
);

FUNC_EXTERN STATUS  NMgtIngAt(
	    char	*name, 
	    char	**ret_val
);

FUNC_EXTERN STATUS  NMloc(
	    char	which, 
	    LOCTYPE	what, 
	    char	*fname, 
	    LOCATION	*ploc
);

FUNC_EXTERN VOID    NMlogOperation(
	    char	*oper,
	    char	*name,
	    char	*value,
	    char	*new_value,
	    STATUS	status
);

FUNC_EXTERN STATUS  NMpathIng(
	    char	**path
);

FUNC_EXTERN STATUS  NMsetpart(
	    char	*partname
);

FUNC_EXTERN STATUS  NMt_open(
	    char	*mode, 
	    char	*fname, 
	    char	*suffix,
	    LOCATION	*t_loc, 
	    FILE	**fptr
);

#ifdef NT_GENERIC
FUNC_EXTERN VOID    NMgtTLSCleanup();
#endif  /* NT_GENERIC */


# endif /* NM_HDR_INCLUDED */
