/*
**	Copyright (c) 1989, 2005 Ingres Corporation
**
*/
/*
** si.h -- include file for the SI module of the compatibility library
**
** History:
**	epoch -- (unknown)
**		created.
**	08-may-89 (daveb)
**		remove BSD41c references and inclusions of param.h.
**		This was for _NFILE/NOFILE, which should not be visible
**		outside the CL.  Also, param.h ended up including
**		<sys/types.h>, which blew up since it didn't have the
**		wrapping provided by our systypes.h.
**
**		Finally, once and for all, fix the NULL redefined
**		message the often comes from including si.h by 
**		undefining it, including stdio, and then redefining
**		it bo be 0 if needed.
**	23-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Comment out string following #endif.
**	22-mar-91 (seng)
**	    Add <systypes.h> to clear <types.h> for AIX 3.1
**	    Undefine NULL and redefine it as in compat.h
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	28-jun-93 (rogerl)
**	    Define SI_MAX_TXT_REC as per CL committee.
**      8-jul-93 (ed)
**          move nested include protection to glhdr
**	30-may-95 (tutto01, canor01)
**	    Added PUTC and CMCPYCHAR to tell SIdofrmt how to format buffer.
**    22-may-95 (emmag)
**        Desktop platforms use SIfopen to open files in binary mode.
**	04-jan-95 (wadag01)
**	    Removed redefinition of FILE structure members for SCO 
**	    (# ifdef CHANGE_TO_XPG4_PLUS) added by morayf for sos_us5.
**	    They were needed to fix the __ptr syntax errors.
**	    Instead "-a ods30" was added to CCMACH in mkdefault.sh.
**	    This option causes on SCO to include ods_30_compat/stdio.h instead
**	    of the default stdio.h (workaround for __ptr syntax errors on SCO).
**	20-nov-1996 (canor01)
**	    Add SIfscanf, SIfsync for compatibility.
**	19-nov-1996 (donc)
**	    Integrate OpenROAD function prototypes and Fixup vector routine.
**	    I isolated these definitions with an ifdef for WIN32 and WIN32S
**	18-jan-97 (mcgem01)
**	    Add missing #endif
**	27-jan-97 (hanch04)
**	    Changed LO_INCLUDE to be CL_PROTOTYPED
**	10-feb-1997 (canor01)
**	    fsync() is not ANSI, so Windows NT does not support it.  It does
**	    allow a way to emulate it by opening file with 'c' flag and 
**	    calling fflush().
**	16-feb-97 (mcgem01)
**	    We had two different SIfsync macros.  Orlan's version as
**	    described above, is the correct one.
**	16-may-97 (mcgem01)
**	    Should not prototype functions that haven't been #define'ed
**	21-nov-2000 (crido01)
**	    Added FUSEDLL conditional defines for SIgetc and SIungetc.
**	17-jan-2001(crido01)
**	    Added FUSEDLL conditional defines for SIclose.
**	01-apr-1998 (canor01)
**          Correct Unix definition of SIfsync() to take a FILE * parameter.
**	25-may-1999 (somsa01)
**	    SIterminal() is different on NT, and it cannot be aliased by
**	    one OS function call.
**	28-jul-2000 (somsa01)
**	    Added new arg defines to SIstd_write().
**	03-Apr-2002 (yeema01) Bug 107276, 105960
**	    The Trace Window defines are needed in order to support Trace
**	    Window on Unix under Mainwin (xde).
**	25-Aug-2005 (abbjo03)
**	    Make SI_VAR valid on all platforms.
**	07-sep-2005 (abbjo03)
**	    Replace xxx_FILE constants by SI_xxx and move them to si.h.
**	    Change #ifndef DESKTOP for SIfopen to #ifndef NT_GENERIC.
**	08-Sep-2005 (thaju02) B115176
**	    Add #define SIftell, SIfseek for LARGEFILE64.
**	26-Sep-2005 (thaju02)
**	    Do not use ftello64/fseeko64 for NT_GENERIC. Once we move to
**	    building on Visual Studio 2005, we'll need to add SIfseek/SIftell
**	    defines for _fseeki64/_ftelli64.
**	21-Nov-2006 (kschendel)
**	    Remove insane foolishness with NULL definition.
**	23-Jan-2007 (wanfr01)
**	    Bug 117499
**	    Reduce SI_MAX_TXT_REC to 16000 to avoid stack corruption on AIX 
**	12-Feb-2007 (wridu01)
**          Added definition of TRACE_LOGONLY so that OpenROAD can be built
**          against the native Ingres libraries.  
**	23-oct-2008 (zhayu01) SIR 120921
**	    Added FUSEDLL conditional defines for SIfseek, SIgetc and 
**	    SIungetc.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/

# include 	<systypes.h>

/* Once upon a time, stdio.h defined its own NULL, causing annoying
** redefinition warnings.  These days one would hope that stdio.h is
** rather better behaved!
** If some platform turns up where stdio does unconditionally redefine
** NULL, you'll probably have to add an #undef and re-#define NULL
** around it.  Make sure you use the proper definition!  which is
** almost certainly ((void *)0).
*/

# include	<stdio.h>

#ifdef CL_PROTOTYPED
/* get LOCATION type if prototyped */
#include    <lo.h>
#endif

#if defined(WIN32) || defined(WIN32S)
#ifndef _INC_IO
#include <io.h>
#endif /* _INC_IO */

#endif /* WIN32 || WIN32S */


/* MACRO DEFINITIONS FOR SI ROUTINES */

/*SIclose
**	
**	Close stream.
**
**	Close the stream s: SIclose(s)
*/

#ifndef FUSEDLL
#define		SIclose(A)	fclose(A)
#else
#define		SIclose(A)	SIclose(A)
#endif

/*SIsscanf
**
**	C sscanf
**
**	The sscanf function of the C language.
**
*/
#define		SIsscanf	sscanf

/*SIfscanf
**
**	C fscanf
**
**	The sscanf function of the C language.
**
*/
#define		SIfscanf	fscanf

#if defined(LARGEFILE64) && !defined(NT_GENERIC)
#define		SIfseek			fseeko64
#define		SIftell			ftello64
#else
#ifndef FUSEDLL
#define         SIfseek			fseek
#else
#define         SIfseek			SIfseek
#endif  /* not FUSEDLL */
#define         SIftell			ftell
#endif  /* LARGEFILE64  and not NT_GENERIC */

#define         SIputrec		fputs

#ifndef NT_GENERIC
#define         SIterminal(f)		isatty(fileno(f))		
#endif

#define         SIfdopen		fdopen
#define         SIfileno		fileno
#define         SIunbuf(fp)		setbuf(fp, NULL )

/*SIflush
**
**	Flush stream
**
**	Flush the stream s:SIflush(s)
**
*/
#define		SIflush(A)	fflush(A)

/*SIfsync
**
**	Sync stream to disk
**
**	Sync the stream s: SIfsync(s)
**
*/
# ifdef NT_GENERIC
/* fsync() emulated on fflush() when file opened with 'c' flag */
#define		SIfsync(s)	fflush(s)
# else /* NT_GENERIC */
#define		SIfsync(s)	fsync(fileno(s))
# endif /* NT_GENERIC */

/*SIgetc
**
**	SIgetc returns the next character from the named input 
**	stream.
**
*/
#ifndef FUSEDLL
# define	SIgetc(p)	getc(p)
#else
# define	SIgetc(p)	SIgetc(p)
#endif


/*SIputc
**
**	SIputc appends the character c to the named output
**	stream.  It returns the character written.
**
**
**	i4 SIputc(c, stream)
**	char	c;
**	FILE	*stream;
*/
# define	SIputc(x, p)	putc(x,p)

/*SIungetc
**
**	SIungetc pushes the character c back on an input stream
**	That character will be returned by the next getc call on that 
**	stream. Ungetc returns c. Returns EOF if it can't push a character
**	back.
**
**
**	char SIungetc(c, stream)
**	char	c;
**	FILE	*stream;
*/
#ifndef FUSEDLL
#define		SIungetc(A, B)	ungetc(A, B)
#else
#define		SIungetc(A, B)	SIungetc(A, B)
#endif

# define	STDIN		0
# define	STDOUT		1
# define	STDERR		2
# define	STDIN2		3
# define	STDOUT2		4

/* args to new SIdofrmt */
# define        SI_SPRINTF      0
# define        SI_FPRINTF      1
# define        SI_PRINTF       2

#if defined(any_aix)
# define 	SI_MAX_TXT_REC	16000
#else
# define 	SI_MAX_TXT_REC	32000
#endif

/* fseek arguments.  UNIX / DOS these match the system fseek encodes */
# define	SI_P_START	0
# define	SI_P_END	2
# define	SI_P_CURRENT	1

/* SIstd_write() arguments */
# define	SI_STD_IN	0	/* stdin */
# define	SI_STD_OUT	1	/* stdout */
# define	SI_STD_ERR	2	/* stderr */

# ifndef NT_GENERIC
# define SIfopen(loc,mod,typ,len,des)	SIopen(loc,mod,des)
# endif /* NT_GENERIC */

/* Tell SIdofrmt() which function to use to format buffer */
# define PUTC 		0
# define CMCPYCHAR 	1


#if defined(WIN32) || defined(WIN32S) || defined(xde)
# define TRACEWIN	215

# define TRACE_NORMAL  		0
# define TRACE_MINACTIVE	1
# define TRACE_MINIMIZED	2
# define TRACE_MAXIMIZED	3
# define TRACE_LOGONLY          4
#endif


#if defined(CL_PROTOTYPED)
/* FUNCTION PROTOTYPES */
FUNC_EXTERN char *	SIgetBufQLine(void);
FUNC_EXTERN void	SIpinit(short docreate,void *hInstance,i2 hWnd,i2 TraceFlg,char *LogFile,i2 AppendFlg,char *TrcTitle,bool bFontFlag);
FUNC_EXTERN void	SIsetupFixups(int (*p_fclose)(),FILE * (*p_fdopen)(),int (*p_fflush)(),int (*p__commit)(),char * (*p_fgets)(),int (*p_fileno)(),int (*p__fileno)(),FILE * (*p_fopen)(),int (*p_fputc)(),int (*p_fputs)(),size_t (*p_fread)(),FILE * (*p_freopen)(),int (*p_fseek)(),long (*p_ftell)(),size_t (*p_fwrite)(),int (*p_getc)(),int (*p_putc)(),void (*p_setbuf)(),int (*p_ungetc)(),int (*p_dup)(),int (*p_dup2)(),int (*p_isatty)(),char * (*p_mktemp)(),void (*p_exit)(),int *(*p_geterrno)());
# endif /* CL_PROTOTYPED */
