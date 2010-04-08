/*
** Copyright (c) 1985, 2005 Ingres Corporation
**
*/

/**
** Name: CP.H - Global definitions for the CP compatibility library.
**
** Description:
**      The file contains the type used by CP and the definition of the
**      CP functions.  These are used for copying files.
**
** History:
**      01-oct-1985 (jennifer)
**          Updated to codinng standard for 5.0.
**	01-dec-1986 (rogerk)
**	    Added filetype constants for 6.0 copy.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	11-may-89 (daveb)
**		Remove RCS log and useless old history.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	25-jun-95 (emmag)
**	    CP not currently supported on NT.  Work around for now.
**	26-jun-95 (canor01)
**	    update above workaround.
**	05-dec-1995 (canor01)
**	    Another update to above for Windows NT.
**	07-sep-2005 (abbjo03)
**	    Replace xxx_FILE constants by SI_xxx.
**/

/*
**  Forward and/or External function references.
*/


/* 
** Defined Constants
*/


/* 
** On VMS copy files are the same as regular files: 
** there are system calls to change the privileges temporarily.
*/
# ifdef NT_GENERIC
# define	CPopen			SIfopen
# endif /* NT_GENERIC */

# define	CPclose(s)		SIclose(*(s))
# define	CPgetc(p)		SIgetc(*(p))
# define	CPwrite(l, b, c, f)	SIwrite(l, b, c, *(f))
# define	CPread(f, l, c, b)	SIread(*(f), l, c, b)
# define	CPflush(p)		SIflush(*(p))

/* CP return status codes. */

#define                 CP_OK           0

/*
** When creating a file during a "COPY INTO", the type of file is needed
** for the CPopen call.  The filetype varies from system to system.
** 
** CP_DEF_FILETYPE gives the default filetype to use.
** CP_NOBIN_FILETYPE gives the default type to use if SI_BIN is not allowed.
** CP_NOTARG_FILETYPE gives the filetype to use if no target list is given.
** CP_USER_FILETYPE tells whether the user may specify a filetype to use.
*/

#ifdef VMS
#define			CP_USER_FILETYPE	1
#define			CP_DEF_FILETYPE		SI_BIN
#define			CP_NOTARG_FILETYPE	SI_BIN
#define			CP_NOBIN_FILETYPE	SI_VAR
#endif

#ifdef UNIX
#define			CP_USER_FILETYPE	0
#define			CP_DEF_FILETYPE		SI_BIN
#define			CP_NOTARG_FILETYPE	SI_BIN
#define			CP_NOBIN_FILETYPE	SI_VAR
#endif

#ifdef CMS
#define			CP_USER_FILETYPE	0
#define			CP_DEF_FILETYPE		SI_TXT
#define			CP_NOTARG_FILETYPE	SI_BIN
#define			CP_NOBIN_FILETYPE	SI_TXT
#endif


/*
** Type Definitions.
*/

/* Since we are using the SI routines, use the SI types. */

typedef	FILE			*CPFILE;

/* 
** This def is here as I don't want to add it to adf.h yet on UNIX.
** CP.h must be ported in order for copy to really work anyway, and then
** this line will be removed. (ncg - temp fx for compiling on unix).
*/
#define                 E_AD1014_BAD_VALUE_FOR_DT	(E_AD_MASK + 0x1014L)
