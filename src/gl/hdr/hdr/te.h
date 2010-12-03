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
**	04-Nov-2010 (miket) SIR 124685
**	    Prototype cleanup.
**/

FUNC_EXTERN STATUS TEclose(
	VOID
);

FUNC_EXTERN VOID    TEcmdwrite(
	char	    *buffer, 
	i4	    length
);

#ifndef TEflush
FUNC_EXTERN STATUS  TEflush(
	VOID
);
#endif

FUNC_EXTERN i4      TEget(
	i4	    seconds
);

FUNC_EXTERN VOID    TEinflush(
	VOID
);

FUNC_EXTERN VOID    TEjobcntrl(
	i4	    (*reset)(),
	i4	    (*redraw)()
);

FUNC_EXTERN VOID    TElock(
	bool	    lock, 
	i4	    row, 
	i4	    col
);

FUNC_EXTERN STATUS  TEname(
	char	    *buffer
);

FUNC_EXTERN STATUS  TEopen(
	TEENV_INFO  *envinfo
);

FUNC_EXTERN VOID    TEput(
	char	    c
);

FUNC_EXTERN STATUS  TErestore(
	i4	    which
);

FUNC_EXTERN STATUS  TEtest(
	char	    *in_name,
	i4	    in_type,
	char	    *cut_name,
	i4	    out_type
);

FUNC_EXTERN bool    TEvalid(
	char	    *termname
);

FUNC_EXTERN VOID    TEwrite(
	char	    *buffer, 
	i4	    length
);

FUNC_EXTERN STATUS  TEgbf(
	char	    *loc,
	i4	    *type
);

FUNC_EXTERN bool    TEgtty(
	i4	    fd,
	char	    *char_buf
);

FUNC_EXTERN i4	TEsettmattr(VOID);

FUNC_EXTERN i4	TEresettmattr(VOID);

FUNC_EXTERN VOID    TEread(VOID);

FUNC_EXTERN STATUS  TEsave(VOID);

FUNC_EXTERN STATUS  TEswitch(VOID);

FUNC_EXTERN STATUS  TEgbfclose(VOID);

# endif /* ! TE_HDR_INCLUDED */
