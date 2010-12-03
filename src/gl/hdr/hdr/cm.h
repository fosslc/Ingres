/*
** Copyright (c) 1993, 2009 Ingres Corporation
*/
# ifndef CM_HDR_INCLUDED
# define CM_HDR_INCLUDED 1
#include    <cmcl.h>

/**CL_SPEC
** Name:	CM.h	- Define CM function externs
**
** Specification:
**
** Description:
**	Contains CM function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**	31-mar-1997 (canor01)
**	    Protect against multiple inclusion of cm.h.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	14-may-2002 (somsa01)
**	    Added CMset_locale().
**      25-Jun-2002 (fanra01)
**          Sir 108122
**          Add prototypes
**	14-Jan-2004 (gupsh01)
**	    Added the file location in input parameter, it can now
**	    be CM_COLLATION_LOC for $II_SYSTEM/ingres/files/collation
**	    or CM_UCHARMAPS_LOC for $II_SYSTEM/ingres/filese/ucharmaps
**	    for unicode and local encoding mapping files.
**	    Added prototype for CM_getcharset() function.
**      22-Mar-2004 (hweho01)
**          1) Added prototype CMread_attr(), 
**          2) Enabled CMvalidusername() and CMvalidhostname() to Unix.
**	14-Jun-2004 (schka24)
**	    Added prototypes for charset functions.
**      18-Nov-2004 (Gordon.Thorpe@ca.com and Ralph.Loen@ca.com)
**          Added new CMgetDefCS() function to simplify export of the
**          CMdefcs global reference in Windows.
**	26-Apr-2007 (gupsh01)
**	    Added CM_ischarsetUTF8() to identify if the II_CHARSETxx
**	    is set to UTF8.
**	03-Nov-2007 (gupsh01)
**	    Added CM_UTF32toUTF16(), CM_UTF8toUTF32 and other unicode
**	    utf coercion routines.
**	11-Nov-2008 (gupsh01)
**	    Added CM_UTF8_twidth() to compute approximately the terminal 
**	    width of a UTF8 character. In reality it may vary from 
**	    terminal to terminal, however gives an approximate measure of
**	    how wide a character may be seen on  a fixed font terminal. 
**      05-feb-2009 (joea)
**          Rename CM_ischarsetUTF8 to CMischarset_utf8.  Add prototype
**          for CMischarset_doublebyte.
**	19-Aug-2009 (stephenb/kschendel) 121804
**	    Add protos for CMvalidinstancecode, CMccopy.
**      25-Nov-2009 (hanal04) Bug 122311
**          Ingres Janitor's project. Fix compiler warnings by making
**          CM_UTF8toUTF32() FUNC_EXTERN consistent with its declaration
**          and usage in cmutf8.c files.
**      29-Nov-2010 (frima01) SIR 124685
**          Add argument void to CMgetDefCS and CMset_locale declaration.
**	1-Dec-2010 (kschendel) SIR 124685
**	    Kill CL_PROTOTYPED (always on now).
**/

FUNC_EXTERN STATUS      CMclose_col(
	CL_ERR_DESC *	syserr,
	i4		loctype		
);
FUNC_EXTERN STATUS      CMdump_col(
	char		*colname, 
	PTR		tablep, 
	i4		tablen, 
	CL_ERR_DESC	*syserr,
	i4		loctype		
);
FUNC_EXTERN STATUS      CMopen_col(
	char		*colname, 
	CL_ERR_DESC	*syserr,
	i4		loctype		
);
FUNC_EXTERN STATUS      CMread_col(
	char		*bufp, 
	CL_ERR_DESC	*syserr
);
FUNC_EXTERN STATUS      CMset_attr(
	char		*name, 
	CL_ERR_DESC	*err
);
FUNC_EXTERN STATUS      CMwrite_attr(
	char		*name, 
	CMATTR		*attr, 
	CL_ERR_DESC	*syserr
);
FUNC_EXTERN char *      CMunctrl(
	char		*str
);
FUNC_EXTERN STATUS      CMset_locale(void);

FUNC_EXTERN STATUS      CM_getcharset(
	char		*pcs
);

#ifndef CMoper
FUNC_EXTERN u_char *    CMoper(
	char		*str
);
#endif

# ifndef CMread_attr
FUNC_EXTERN STATUS      CMread_attr(
	char		*name, 
	char		*filespec, 
	CL_ERR_DESC	*err
);
# endif /* CMread_attr */

FUNC_EXTERN void	CMget_charset_name(
	char		*charset
);

FUNC_EXTERN STATUS	CMset_charset(
	CL_ERR_DESC	*cl_err
);

# ifndef CMvalidhostname
FUNC_EXTERN bool
CMvalidhostname(
	char* hostname
);
# endif /* CMvalidhostname */

# ifndef CMvalidusername
FUNC_EXTERN bool
CMvalidusername(
	char* username
);
# endif /* CMvalidusername */

# if defined(NT_GENERIC)
# ifndef CMvalidpath
FUNC_EXTERN bool CMvalidpath(
	char* path
);
# endif /* CMvalidpath */

# ifndef CMvalidpathname
FUNC_EXTERN bool CMvalidpathname(
	char* path
);
# endif /* CMvalidpathname */

# endif /* NT_GENERIC */

FUNC_EXTERN i4 
CMgetDefCS(void);

FUNC_EXTERN bool
CMischarset_doublebyte(void);

FUNC_EXTERN bool
CMischarset_utf8(void);

FUNC_EXTERN u_i2 
CM_getUTF8property();

FUNC_EXTERN i4 
CM_UTF32toUTF8(int, char *);

FUNC_EXTERN i4 
CM_UTF8toUTF32(u_char *, i4, u_i2 *);

FUNC_EXTERN bool
CM_isLegalUTF8Sequence(const u_char *, const u_char *);

FUNC_EXTERN i4 
CM_UTF32toUTF16(i4, u_i2 *, u_i2 *, i4 *);

FUNC_EXTERN bool
CMvalidinstancecode( char* ii_install );

FUNC_EXTERN i4 
CM_UTF8_twidth(char *, char *);

FUNC_EXTERN SIZE_TYPE
CMccopy(u_char *source, SIZE_TYPE len, u_char *dest);

# endif /* ! CM_HDR_INCLUDED */
