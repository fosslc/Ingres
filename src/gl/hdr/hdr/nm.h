/*
**  Copyright (c) 2004 Ingres Corporation
*/
# ifndef NM_HDR_INCLUDED
# define NM_HDR_INCLUDED 1

#ifdef CL_PROTOTYPED
/* needed for the FILE definition */
#include    <si.h>
#endif
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
**/

FUNC_EXTERN STATUS  NMf_open(
#ifdef CL_PROTOTYPED
	    char	*mode, 
	    char	*fname, 
	    FILE	**fptr
#endif
);

FUNC_EXTERN char *  NMgetpart(
#ifdef CL_PROTOTYPED
	    char	*buf
#endif
);

FUNC_EXTERN VOID    NMgtAt(
#ifdef CL_PROTOTYPED
	    char	*name, 
	    char	**ret_val
#endif
);

FUNC_EXTERN STATUS  NMgtIngAt(
#ifdef CL_PROTOTYPED
	    char	*name, 
	    char	**ret_val
#endif
);

FUNC_EXTERN STATUS  NMloc(
#ifdef CL_PROTOTYPED
	    char	which, 
	    LOCTYPE	what, 
	    char	*fname, 
	    LOCATION	*ploc
#endif
);

FUNC_EXTERN VOID    NMlogOperation(
#ifdef CL_PROTOTYPED
	    char	*oper,
	    char	*name,
	    char	*value,
	    char	*new_value,
	    STATUS	status
#endif
);

FUNC_EXTERN STATUS  NMpathIng(
#ifdef CL_PROTOTYPED
	    char	**path
#endif
);

FUNC_EXTERN STATUS  NMsetpart(
#ifdef CL_PROTOTYPED
	    char	*partname
#endif
);

FUNC_EXTERN STATUS  NMt_open(
#ifdef CL_PROTOTYPED
	    char	*mode, 
	    char	*fname, 
	    char	*suffix,
	    LOCATION	*t_loc, 
	    FILE	**fptr
#endif
);

#ifdef NT_GENERIC
FUNC_EXTERN VOID    NMgtTLSCleanup();
#endif  /* NT_GENERIC */


# endif /* NM_HDR_INCLUDED */
