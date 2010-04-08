#ifndef SICL_INCLUDED
#define SICL_INCLUDED

#include <lo.h>

/*
** Copyright (c) 1985, 2009 Ingres Corporation
**
*/

/**
** Name: SI.H - Global definitions for the SI compatibility library.
**
** Description:
**      The file contains the type used by SI and the definition of the
**      SI functions.  These are used for stream I/O.
**
** History:
**      01-oct-1985 (jennifer)
**          Updated to coding standard for 5.0.
**	13-dec-1985 (jeff)
**	    Moved extern of SI_iob to after definition of FILE.
**	15-sep-1986 (Joe)
**	    Added SIput_macro.
**	4/91 (Mike S)
**	    Replace the FILE array SI_iob with the FILE pointer SI_iobptr; this 
**	    allows shareable image vectoring of the data.
**	6/91 (Mike S)
**	    Increase available SI files to 127.
**	4-feb-93 (ed)
**	    fix illegal preprocessor directive
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**      04-nov-92 (walt)
**          Removed definitions of SYNC_EF and TTY_IO_EF for Alpha.
**          These are also in eventflag.h and the Alpha compiler
**          gives a warning for redefinitions, even if the value is
**          the same.  The VAX compilers process the redefinitions
**          silently.
**	28-jun-93 (rogerl)
**	    Define SI_MAX_TXT_REC as per CL committee.
**      8-jul-93 (ed)
**          move nested include protection to glhdr
**	05-aug-1997 (teresak)
**	    definition for SI_BIN was missing. 
**	09-mar-1998 (kinte01)
**          Integrated VAX CL change 275230 from ingres63p:
**              15-may-95 (mckba01)
**                  Changes for GCN optimization. New file type
**                  #def  GCN_RACC_FILE.
**                  Addition of new #def for _flags in FILE structure
**                  _SI_RACC_OPT.
**          Back out last change to _FILE structure per change 275491:
**                      11/95 (mckba01) - Remove buffered record flag _usebuf
**                      from _FILE struct, after disaster at JPM.  Also it's
**                      non-aligned on Alpha anyway.
**	11-mar-1998 (kinte01)
**          Additional changes from GCN optimization
**	30-dec-1999 (kinte01)
**	    Change typdef to quite ANSI compiler warnings
**	21-jun-2000 (bolke01)
**	    Increase the value of _NFILE 127 > 1024.  This is axpvms specific.
**	04-jun-2001 (kinte01)
**	    Redefine SI routines to their C variants per change 447330
**	04-jun-2001 (kinte01)
**	    Cross integrate Unix Change 449371 
**	    28-jul-2000 (somsa01)
**	    Added new arg defines to SIstd_write().
**	24-aug-2001 (kinte01)
**	   Change 447330 replaced 2 CL functions with their C code equivalents
**	   These CL functions that were replaced were defined as no opt 
**	   functions on VMS.  Currently modified to be the C code equivalent
**	   but due to the impact may need to revisit this
**	02-feb-2005 (abbjo03)
**	    Add #define for PUTC.
**	07-sep-2005 (abbjo03)
**	    Replace xxx_FILE constants by SI_xxx.
**	08-may-2008 (joea)
**	    Remove redefinition of NULL.
**      10-dec-2008 (horda03) Bug 120571
**          Added ifndef SICL_INCLUDED to prevent the header file from being included
**          multiple times. Also including lo.h here, just as it is the Unix version
**          of this file.
**      19-dec-2008 (stegr01)
**          Itanium VMS port. Removed obsolete VAX declarations.
**	16-apr-2009 (joea)
**	    Increase BUFSIZ to 16384.
**      30-Dec-2009 (horda03) Bug 123091/123092
**          Added flush_mutex to FILE.
**/


 
/* 
** Forward and/or External typedef/struct references.
*/


/*
**  Forward and/or External function references.
*/

FUNC_EXTERN VOID    SIsprintf();

/* 
** Defined Constants
*/

/* SI return status codes. */

#define                 SI_OK           0

/* The following symbols are machine dependent */

# define	WORDSIZE	32		/* 32 bit machine */
# define	LOG2WORDSIZE	5		/* base 2 log of wordsize */

/* 
** Even though TEgtty always returns false on VMS, 
** we still need STDIN, etc. so routines that call it will compile.
*/

# define	STDIN	0
# define 	STDOUT	1
# define 	STDERR	2

/* things to do with files. */

# define	MAX_F_NAME	64
# define	BUFSIZ		16384
# define	_NFILE		1024

# define	TTY		1
# define	MBX		2
/*
**      New file type flag to select
**      GCN optimized RACC file handling.
*/
# define        GCN_RACC_FILE   7

# define	SI_MAX_TXT_REC	32000

/*
** position arguments for SIfseek
*/
# define	SI_P_START	0
# define	SI_P_CURRENT	1
# define	SI_P_END	2

/* SIstd_write() arguments */
# define	SI_STD_IN	0	/* stdin */
# define	SI_STD_OUT	1	/* stdout */
# define	SI_STD_ERR	2	/* stderr */

# define PUTC           0

/* terminal type stuff */

# define 	CRMOD		01000
# define	RAW		0100


/*
** Type Definitions.
*/

/* FILE control struct. */

typedef	struct _FILE
{
	char	*_ptr;
	char	*_file;
	char	*_base;
	short	_cnt;
	char	_type;
	char	_flag;
#ifdef OS_THREADS_USED
        PTR     flush_mutex;
#endif
} FILE;


extern		FILE	*SI_iobptr;

# define	_IOREAD	1
# define	_IOWRT	2
# define	_IOPEN	4
# define	_IOEOF	8
# define	_IOERR	16
# define	_IOSTRG	32
# define	_IORACC 64
/*
**      New Flag for GCN RACC file access optimization
*/

# define        _SI_RACC_OPT 128

# define	EOF	(-1)
# define	stdin	(&SI_iobptr[0])
# define	stdout	(&SI_iobptr[1])
# define	stderr	(&SI_iobptr[2])


# define	SIgetchar()	SIgetc(stdin)
# define	SIputchar(c)	SIputc(c, stdout)
# define	SIgetc(f)	(--(f)->_cnt >= 0 ? *(f)->_ptr++ & 0377 : _filbuf(f))
/*
** NOTE:  IF THIS CHANGES, CHANGE SIput_macro too !!
*/
# define	SIputc(c, f)	(--(f)->_cnt >= 0 ? *(f)->_ptr++ = (c) : _flsbuf((c), f))
/*
** This is like SIputc, but takes the flush routine as a parameter
** as well.  It is called by SIdofrmt. It is put here so that
** any change in SIputc is done in it to.  I didn't have
** SIputc use this to make it simpler.
*/
# define	SIput_macro(p, c, f)	(--(f)->_cnt >= 0 ? *(f)->_ptr++ = (c) : (*(p))((c), f))
# define	SIungetc(c, f)	(++(f)->_cnt, *(--(f)->_ptr) = (c))

# define	SIfdopen	fdopen
# define	SIfileno	fileno
# define	SIunbuf(fp)	setbuf(fp, NULL)

#endif /* SICL_INCLUDED */
