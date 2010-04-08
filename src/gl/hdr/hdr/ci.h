/*
**	Copyright (c) 2004 Ingres Corporation
*/
# ifndef CI_HDR_INCLUDED
# define CI_HDR_INCLUDED

#include    <cicl.h>

/**CL_SPEC
** Name:	CI.h	- Define CI function externs
**
** Specification:
**
** Description:
**	Contains CI function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**	30-sep-1996 (canor01)
**	    Protect against multiple inclusion of ci.h.
**	07-may-1998 (canor01)
**	    Add CI_set_usercnt_func().
**	29-jun-1999 (somsa01)
**	    Added CIvalidate().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**/

FUNC_EXTERN BITFLD   CIchksum(
#ifdef	CL_PROTOTYPED
	u_char	    *input,
	i4	    size,
	i4	    inicrc
#endif
);
FUNC_EXTERN VOID     CIdecode(
#ifdef	CL_PROTOTYPED
	PTR	    cipher_text,
	i4	    size,
	CI_KS	    ks,
	PTR	    plain_text
#endif
);
FUNC_EXTERN VOID     CIencode(
#ifdef	CL_PROTOTYPED
	PTR	    plain_text,
	i4	    size,
	CI_KS	    ks,
	PTR	    cipher_text
#endif
);
FUNC_EXTERN VOID     CItobin(
#ifdef	CL_PROTOTYPED
	PTR	    textbuf,
	i4	    *size,
	u_char	    *block
#endif
);
FUNC_EXTERN VOID     CItotext(
#ifdef	CL_PROTOTYPED
	u_char	    *block,
	i4	    size,
	PTR	    textbuf
#endif
);

# ifndef E_CL2662_CI_BADSITEID
# define E_CL2662_CI_BADSITEID	(E_CL_MASK + 0x2662)
# endif

# endif /* CI_HDR_INCLUDED */
