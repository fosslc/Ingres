/*
**	Copyright (c) 1993, 2005 Ingres Corporation
**
*/
#ifndef SI_INCLUDE
#define SI_INCLUDE

#include    <sicl.h>
#ifdef CL_PROTOTYPED
/* get LOCATION type if prototyped */
#include    <lo.h>
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
**/

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
#ifdef CL_PROTOTYPED
	LOCATION	*in, 
	FILE		*out
#endif
);
#endif

#ifndef SIclose
FUNC_EXTERN STATUS  SIclose(
#ifdef CL_PROTOTYPED
	FILE		*close
#endif
);
#endif

#ifndef SIcopy
FUNC_EXTERN STATUS  SIcopy(
#ifdef CL_PROTOTYPED
        LOCATION        *in,
        LOCATION        *out
#endif
);
#endif

#ifndef SIeqinit
FUNC_EXTERN STATUS  SIeqinit(
#ifdef CL_PROTOTYPED
	void
#endif
);
#endif

#ifndef SIflush
FUNC_EXTERN STATUS  SIflush(
#ifdef CL_PROTOTYPED
	FILE		*s
#endif
);
#endif

#ifndef SIfopen
FUNC_EXTERN STATUS  SIfopen(
#ifdef CL_PROTOTYPED
	LOCATION	*loc, 
	char		*mode, 
	i4		type, 
	i4		length, 
	FILE		**desc
#endif
);
#endif

#ifndef SIfprintf
FUNC_EXTERN VOID SIfprintf(
#ifdef CL_PROTOTYPED
        FILE		*fp,
	const char	*fmt,
	...
#endif
);
#endif

#ifndef SIfseek
FUNC_EXTERN STATUS  SIfseek(
#ifdef CL_PROTOTYPED
	FILE		*fp, 
	OFFSET_TYPE	offset, 
	i4		mode
#endif
);
#endif

#ifndef SIftell
FUNC_EXTERN OFFSET_TYPE SIftell(
#ifdef CL_PROTOTYPED
	FILE		*fp
#endif
);
#endif

#ifndef SIgetattr
FUNC_EXTERN STATUS     SIgetattr(
#ifdef CL_PROTOTYPED
	FILE		*stream,
	i4		*type,
	i4		*length
#endif
);
#endif

#ifndef SIgetc
FUNC_EXTERN i4      SIgetc(
#ifdef CL_PROTOTYPED
	FILE		*stream
#endif
);
#endif

#ifndef SIgetrec
FUNC_EXTERN STATUS  SIgetrec(
#ifdef CL_PROTOTYPED
	char		*bug, 
	i4		n, 
	FILE		*stream
#endif
);
#endif

#ifndef SIisopen
FUNC_EXTERN bool    SIisopen(
#ifdef CL_PROTOTYPED
	FILE		*streamptr
#endif
);
#endif

#ifndef SIopen
FUNC_EXTERN STATUS  SIopen(
#ifdef CL_PROTOTYPED
	LOCATION	*loc, 
	char		*mode, 
	FILE		**desc
#endif
);
#endif

#ifndef SIprintf
FUNC_EXTERN VOID SIprintf(
#ifdef CL_PROTOTYPED
	const char		*fmt,
	...
#endif
);
#endif

#ifndef SIputc
FUNC_EXTERN i4      SIputc(
#ifdef CL_PROTOTYPED
	i4		c, 
	FILE		*stream
#endif
);
#endif

#ifndef SIputrec
FUNC_EXTERN STATUS  SIputrec(
#ifdef CL_PROTOTYPED
	char		*buf, 
	FILE		*stream
#endif
);
#endif

#ifndef SIread
FUNC_EXTERN STATUS  SIread(
#ifdef CL_PROTOTYPED
	FILE		*stream, 
	i4		numofbytes, 
	i4		*actual_count, 
	char		*pointer
#endif
);
#endif

#ifndef SIterminal
FUNC_EXTERN bool    SIterminal(
#ifdef CL_PROTOTYPED
	FILE		*s
#endif
);
#endif

#ifndef SIungetc
FUNC_EXTERN i4      SIungetc(
#ifdef CL_PROTOTYPED
	char		c, 
	FILE		*stream
#endif
);
#endif

#ifndef SIwrite
FUNC_EXTERN STATUS  SIwrite(
#ifdef CL_PROTOTYPED
	i4		typesize, 
	char		*pointer, 
	i4		*count, 
	FILE		*stream
#endif
);
#endif

#ifndef SIcreate
FUNC_EXTERN STATUS  SIcreate(
#ifdef CL_PROTOTYPED
        LOCATION        *lo
#endif
);
#endif

#ifndef SIhistgetrec
FUNC_EXTERN STATUS  SIhistgetrec(
#ifdef CL_PROTOTYPED
	char		*bug, 
	i4		n, 
	FILE		*stream
#endif
);
#endif

#ifndef SIclearhistory
FUNC_EXTERN void SIclearhistory();
#endif

#ifdef NT_GENERIC
FUNC_EXTERN VOID SIsaveDatabase(
	char 		*db
);
#endif /* NT_GENERIC */

FUNC_EXTERN STATUS SIstd_write(
#ifdef CL_PROTOTYPED
	i4		std_stream,
	char		*str
#endif
);

#endif /*SI_INCLUDE*/
