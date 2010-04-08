/*
**	Copyright (c) 2004 Ingres Corporation
*/
# ifndef TR_HDR_INCLUDED
# define TR_HDR_INCLUDED 1

#include    <trcl.h>
# if !defined(__STDARG_H__)
#include    <stdarg.h>
# define __STDARG_H__
#endif

/**CL_SPEC
** Name:	TR.h	- Define TR function externs
**
** Specification:
**
** Description:
**	Contains TR function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**	31-mar-1997 (canor01)
**	    Protect against multiple inclusion of tr.h.
**	26-Oct-1998 (kosma01)
**	    Provide stdarg prototypes for sgi_us5, because
**	    sgi_us5 has a problem with floating point values
**	    on vararg functions that have no prototype.
**	09-apr-1999 (somsa01)
**	    Not all platforms define __STDARG_H__ in their stdarg.h header
**	    file. Therefore, manually define it.
**      21-Apr-1999 (kosma01)
**          I am restoring the FUNC_EXTERN I left off from the old
**          TRdisplay() and TRformat() function declares/prototypes.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	04-may-1999 (hanch04)
**	    Function for TRformat should pass a pointer.
**	01-feb-2000 (marro11)
**	    Relocating the recent introduction of __STDARG_H__ to trcl.h; its
**	    inclusion wreaks havoc in tr.c when compiled for dg8_us5, dgi_us5,
**	    hp8_us5, hpb_us5, ris_us5, rs4_us5, and su4_u42,   Since we don't
**	    want platform ifdef's in gl, it's inclusion was moved to trcl.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	05-sep-2002 (devjo01)
**	    Add declare for TRwrite.
**	25-May-2007 (jonj)
**	    Add prototype for TRformat_to_buffer().
**/
#ifndef TRdisplay
#if defined(__STDARG_H__)
FUNC_EXTERN i4  TRdisplay( char *, ... );
#else
FUNC_EXTERN i4  TRdisplay( );
#endif
#endif

#ifndef TRformat
#if defined( __STDARG_H__)
FUNC_EXTERN void TRformat( i4(*)(PTR, i4, char *), ...);
#else
FUNC_EXTERN void TRformat();
#endif
#endif

FUNC_EXTERN VOID TRformat_to_buffer(
VOID	(*fcn)(),
i4	*arg,
char	*buffer,
i4	l_buffer,
char	*fmt,
va_list	ap);

FUNC_EXTERN VOID TRflush(
#ifdef CL_PROTOTYPED
	    void
#endif
);

/*
FUNC_EXTERN 
bool TRgettrace(
#ifdef CL_PROTOTYPED
	    i4		word,
	    i4		bit
#endif
);
*/
bool
TRgettrace(i4 word,i4 bit);


FUNC_EXTERN STATUS TRmaketrace(
#ifdef CL_PROTOTYPED
	    char	**argv,
	    char	tflag,
	    i4	tsize,
	    i4		*tvect,
	    bool	tonoff
#endif
);

FUNC_EXTERN STATUS TRrequest(
#ifdef CL_PROTOTYPED
	    char	*record,
	    i4		record_size,
	    i4		*read_count,
	    CL_ERR_DESC *err_code,
	    char	*prompt
#endif
);

FUNC_EXTERN STATUS TRset_file(
#ifdef CL_PROTOTYPED
	    i4		flag,
	    char	*filename,
	    i4		name_length,
	    CL_ERR_DESC *err_code
#endif
);

FUNC_EXTERN STATUS TRsettrace(
#ifdef CL_PROTOTYPED
	    i4		word,
	    i4		bit,
	    bool	set
#endif
);

FUNC_EXTERN STATUS TRwrite(
#ifdef CL_PROTOTYPED
	    PTR		arg,
	    i4		buflen,
	    char       *buffer
#endif
);

# endif /* ! TR_HDR_INCLUDED */
