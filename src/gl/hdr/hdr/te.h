/*
**	Copyright (c) 2004 Ingres Corporation
*/
# ifndef TE_HDR_INCLUDED
# define TE_HDR_INCLUDED 1

#include    <tecl.h>

/**CL_SPEC
** Name:	TE.h	- Define TE function externs
**
** Specification:
**
** Description:
**	Contains TE function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**     15-may-1995 (harpa06)
**	    Cross Integration of bug fix for 49935 by prida01 - Add TE_ECHOON
**	    and TE_ECHOOFF for stdin in Unix when redirecting the 
**	    standard output.
**	31-mar-1997 (canor01)
**	    Protect against multiple inclusion of te.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      24-Feb-2002 (hanch04)
**          Moved TE_ECHOON and TE_ECHOOFF from te.h to tecl.h
**	31-mar-2004 (kodse01)
**		SIR 112068
**		Added prototypes for TEsettmattr() and TEresettmattr()
**/

FUNC_EXTERN STATUS TEclose(
#ifdef CL_PROTOTYPED
	void
#endif
);

FUNC_EXTERN VOID    TEcmdwrite(
#ifdef CL_PROTOTYPED
	char	    *buffer, 
	i4	    length
#endif
);

#ifndef TEflush
FUNC_EXTERN STATUS  TEflush(
#ifdef CL_PROTOTYPED
	void
#endif
);
#endif

FUNC_EXTERN i4      TEget(
#ifdef CL_PROTOTYPED
	i4	    seconds
#endif
);

FUNC_EXTERN VOID    TEinflush(
#ifdef CL_PROTOTYPED
	void
#endif
);

FUNC_EXTERN VOID    TEjobcntrl(
#ifdef CL_PROTOTYPED
	VOID	    (*reset)(), 
	VOID	    (*redraw)()
#endif
);

FUNC_EXTERN VOID    TElock(
#ifdef CL_PROTOTYPED
	bool	    lock, 
	i4	    row, 
	i4	    col
#endif
);

FUNC_EXTERN STATUS  TEname(
#ifdef CL_PROTOTYPED
	char	    *buffer
#endif
);

FUNC_EXTERN STATUS  TEopen(
#ifdef CL_PROTOTYPED
	TEENV_INFO  *envinfo
#endif
);

FUNC_EXTERN VOID    TEput(
#ifdef CL_PROTOTYPED
	char	    c
#endif
);

FUNC_EXTERN STATUS  TErestore(
#ifdef CL_PROTOTYPED
	i4	    which
#endif
);

FUNC_EXTERN STATUS  TEtest(
#ifdef CL_PROTOTYPED
	char	    *in_name,
	i4	    in_type,
	char	    *cut_name,
	i4	    out_type
#endif
);

FUNC_EXTERN bool    TEvalid(
#ifdef CL_PROTOTYPED
	char	    *termname
#endif
);

FUNC_EXTERN VOID    TEwrite(
#ifdef CL_PROTOTYPED
	char	    *buffer, 
	i4	    length
#endif
);

FUNC_EXTERN i4	TEsettmattr();

FUNC_EXTERN i4	TEresettmattr();

# endif /* ! TE_HDR_INCLUDED */
