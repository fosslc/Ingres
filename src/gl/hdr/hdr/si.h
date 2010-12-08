/*
**	Copyright (c) 1993, 2005 Ingres Corporation
**
*/
#ifndef SI_INCLUDE
#define SI_INCLUDE

#include    <sicl.h>
/* get LOCATION type if prototyped */
#include    <lo.h>
#ifdef VMS
# include   <stdarg.h>
#endif

/**CL_SPEC
** Name:	SI.h	- Define SI function externs
**
** Specification:
**
** Description:
**	Contains SI function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**	22-jun-93 (vijay)
**	    Enclose SIclose, SIflush in ifndef.
**      13-jun-1993 (ed)
**          make safe for multiple inclusions
**	24-jan-1994 (gmanning)
**	    Enclose in ifndefs the following: STgetwords, STindex, STlcopy, 
**	    STmove, STnlength, STrindex, SIcat, SIeqinit, SIfseek, SIftell, 
**	    SIgetrec, SIisopen, SIopen, SIputrec, SIread, SIterminal, SIwrite.
**	18-aug-95 (tutto01)
**	    Added declaration for SIsaveDatabase.  Needed for Windows NT.
**      07-jan-1997 (wilan06)
**          Added declaration of new CL function SIcreate.
**      19-jun-1997 (easda01)
**          Added declaration of new CL function SIcopy.
**	08-oct-1997 (canor01)
**	    Add complete prototypes for SIprintf and SIfprintf.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      03-jun-1999 (hanch04)
**          Change SIfseek to use OFFSET_TYPE and LARGEFILE64 for files > 2gig
**	28-jul-2000 (somsa01)
**	    Added prototype for SIstd_write().
**	10-aug-2000 (somsa01)
**	    Slightly changed prototype for SIstd_write().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      18-sep-2000 (hanch04)
**          Update SIftell
**	31-mar-2004	(kodse01)
**		SIR 112068
**		Added prototypes for SIhistgetrec() and SIclearhistory().
**	05-jan-2005 (abbjo03)
**	    Remove obsolete functions.
**	07-sep-2005 (abbjo03)
**	    Move SIfopen()/CPopen() file format constants here from sicl.h.
**      07-may-2010 (coomi01)
**          Move SI result codes here from silocal.h
**          Add two new results, SI_CANT_OPEN_EACCES and SI_CANT_OPEN_EEXIST 
**	1-Dec-2010 (kschendel) SIR 124685
**	    Kill CL_PROTOTYPED (always on now).
**/

#ifndef  SI_RESULT_CODES /* Thwart multiple inclusions */
# define SI_RESULT_CODES PRESENT
# define SI_BAD_SYNTAX          (E_CL_MASK | E_SI_MASK | 1) /* SIopen() */
# define SI_CAT_DIR             (E_CL_MASK | E_SI_MASK | 2) /* SIcat()  */
# define SI_CAT_NONE            (E_CL_MASK | E_SI_MASK | 3) /* SIcat()  */
# define SI_CANT_OPEN           (E_CL_MASK | E_SI_MASK | 4) /* SIcat(), SIcopy(), SIcreate(), SIopen() */
# define SI_CANT_OPEN_EACCES    (E_CL_MASK | E_SI_MASK | 5) /* SIcreate() : Permission not granted     */
# define SI_CANT_OPEN_EEXIST    (E_CL_MASK | E_SI_MASK | 6) /* SIcreate() : File already exists        */
#endif

/* File formats for SIfopen() and CPopen() */
# define	SI_BIN		3	/* binary, semi-structured sequential
					   files that store blocks of bytes */
# define	SI_TXT		4	/* text, a sequence of lines followed
					   by an end-of-line delimiter */
# define	SI_RACC		5	/* record access, used for manipulation
					   of binary data in a stream file,
					   allows byte-level seek as well as
					   update mode */
# define	SI_VAR		6	/* variable, unstructured sequential
					   files that store blocks of bytes */


#ifndef SIcat
FUNC_EXTERN STATUS  SIcat(
	LOCATION	*in, 
	FILE		*out
);
#endif

#ifndef SIclose
FUNC_EXTERN STATUS  SIclose(
	FILE		*close
);
#endif

#ifndef SIcopy
FUNC_EXTERN STATUS  SIcopy(
        LOCATION        *in,
        LOCATION        *out
);
#endif

#ifndef SIeqinit
FUNC_EXTERN STATUS  SIeqinit(
	void
);
#endif

#ifndef SIflush
FUNC_EXTERN STATUS  SIflush(
	FILE		*s
);
#endif

#ifndef SIfopen
FUNC_EXTERN STATUS  SIfopen(
	LOCATION	*loc, 
	char		*mode, 
	i4		type, 
	i4		length, 
	FILE		**desc
);
#endif

#ifndef SIfprintf
FUNC_EXTERN VOID SIfprintf(
        FILE		*fp,
	const char	*fmt,
	...
);
#endif

#ifndef SIfseek
FUNC_EXTERN STATUS  SIfseek(
	FILE		*fp, 
	OFFSET_TYPE	offset, 
	i4		mode
);
#endif

#ifndef SIftell
FUNC_EXTERN OFFSET_TYPE SIftell(
	FILE		*fp
);
#endif

#ifndef SIgetattr
FUNC_EXTERN STATUS     SIgetattr(
	FILE		*stream,
	i4		*type,
	i4		*length
);
#endif

#ifndef SIgetc
FUNC_EXTERN i4      SIgetc(
	FILE		*stream
);
#endif

#ifndef SIgetrec
FUNC_EXTERN STATUS  SIgetrec(
	char		*bug, 
	i4		n, 
	FILE		*stream
);
#endif

#ifndef SIisopen
FUNC_EXTERN bool    SIisopen(
	FILE		*streamptr
);
#endif

#ifndef SIopen
FUNC_EXTERN STATUS  SIopen(
	LOCATION	*loc, 
	char		*mode, 
	FILE		**desc
);
#endif

#ifndef SIprintf
FUNC_EXTERN VOID SIprintf(
	const char		*fmt,
	...
);
#endif

#ifndef SIputc
FUNC_EXTERN i4      SIputc(
	i4		c, 
	FILE		*stream
);
#endif

#ifndef SIputrec
FUNC_EXTERN STATUS  SIputrec(
	char		*buf, 
	FILE		*stream
);
#endif

#ifndef SIread
FUNC_EXTERN STATUS  SIread(
	FILE		*stream, 
	i4		numofbytes, 
	i4		*actual_count, 
	char		*pointer
);
#endif

#ifndef SIterminal
FUNC_EXTERN bool    SIterminal(
	FILE		*s
);
#endif

#ifndef SIungetc
FUNC_EXTERN i4      SIungetc(
	char		c, 
	FILE		*stream
);
#endif

#ifndef SIwrite
FUNC_EXTERN STATUS  SIwrite(
	i4		typesize, 
	char		*pointer, 
	i4		*count, 
	FILE		*stream
);
#endif

#ifndef SIcreate
FUNC_EXTERN STATUS  SIcreate(
        LOCATION        *lo
);
#endif

#ifndef SIhistgetrec
FUNC_EXTERN STATUS  SIhistgetrec(
	char		*bug, 
	i4		n, 
	FILE		*stream
);
#endif

#ifndef SIclearhistory
FUNC_EXTERN void SIclearhistory(void);
#endif

#ifdef NT_GENERIC
FUNC_EXTERN VOID SIsaveDatabase(
	char 		*db
);
#endif /* NT_GENERIC */

FUNC_EXTERN STATUS SIstd_write(
	i4		std_stream,
	char		*str
);

#ifdef VMS
VOID
SIdofrmt(FILE *outarg, const char *fmt, STATUS (*flfunc)(), va_list ap);
#else
VOID
SIdofrmt(i4, void *, const char *, va_list);
#endif

#endif /*SI_INCLUDE*/
