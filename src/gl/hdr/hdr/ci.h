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
**      29-Nov-2010 (frima01) SIR 124685
**          Added prototypes and removed CL_PROTOTYPED ifdefs.
**/

/* actually encrypt or decrypt some information. */
FUNC_EXTERN VOID     CIencrypt(
	CI_KS		KS, 
	bool 		decode_flag,
	char 		*block);

/* unpack bits to be crypted into an equal number of bytes, one bit per byte. */
FUNC_EXTERN VOID     CIexpand(
	PTR 		ate_bytes, 
	PTR 		b_map);

/* pack bits that have been crypted back into bytes. */
FUNC_EXTERN VOID     CIshrink(
	PTR 		b_map,
	PTR 		ate_bytes);


FUNC_EXTERN BITFLD   CIchksum(
	u_char	    *input,
	i4	    size,
	i4	    inicrc
);
FUNC_EXTERN VOID     CIdecode(
	PTR	    cipher_text,
	i4	    size,
	CI_KS	    ks,
	PTR	    plain_text
);
FUNC_EXTERN VOID     CIencode(
	PTR	    plain_text,
	i4	    size,
	CI_KS	    ks,
	PTR	    cipher_text
);
FUNC_EXTERN VOID     CItobin(
	PTR	    textbuf,
	i4	    *size,
	u_char	    *block
);
FUNC_EXTERN VOID     CItotext(
	u_char	    *block,
	i4	    size,
	PTR	    textbuf
);

FUNC_EXTERN VOID     CIsetkey(
	PTR	    key_str,
	CI_KS	    KS);

FUNC_EXTERN VOID
CIrm_logical(
	PTR     arg1_dummy,
	PTR     arg2_dummy);

FUNC_EXTERN VOID
CImk_logical(
	PTR     log_name,
	PTR     equiv_str,
	PTR     arg1_dummy);

# ifndef E_CL2662_CI_BADSITEID
# define E_CL2662_CI_BADSITEID	(E_CL_MASK + 0x2662)
# endif

# endif /* CI_HDR_INCLUDED */
