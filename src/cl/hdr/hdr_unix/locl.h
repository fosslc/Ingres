/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/
typedef	struct _LOCATION	LOCATION;   /* needed for si.h */
#include	<compat.h>
#include	<si.h>
#include	<tm.h>

/**
** Name:	lo.h -	Compatibility Library Location Module Interface Definition.
**
** Description:
**	Contains declarations and definitions that define the interface for
**  the locations (LO) module of the CL.
**
**
** History:
**	  01-oct-1985 (jennifer)
**		  Updated to coding standard for 5.0.
**	  11-Sep-86 (ericj)
**		  Added constants to define the maximum of parts used in an
**		LOdetail call.
**	  30-dec-86 (ericj)
**		  Added constant to define the maximun file name length.
**	  17-feb-89 (billc)
**		  Added LOINFORMATION struct and appurtenances.
**	23-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Remove DB, JNL, CKP, which duplicate documented LO_DB, etc.
**	9-nov-90 (pete)
**		Add definition of LO_EXE_EXT, approved by CL for 6.3/03.
**	28-feb-91 (mikem)
**	    Added LO_WORK definition to support 6.3/04 sort on multiple
**	    location project.
**	07-nov-92 (jrb)
**	    Removed definition of LO_SORT... no longer used.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	8-jul-93 (ed)
**	    move nested include protection to glhdr, add LOCATION forward reference
**	15-sep-1997 (canor01)
**	    Increase LO_NM_LEN to 128 (from 48) to match limit from 6.4.
**	23-jun-1998(angusm)
**	    Add prot for LOgetowner().
**	    Increase LO_NM_LEN to 256 (from 48) to match limit from 6.4.
**	28-feb-2000 (somsa01)
**	    To fully support LARGEFILE64, changed the type of li_size to
**	    OFFSET_TYPE in LOINFORMATION.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	20-Mar-2001 (jenjo02)
**	    Added LO_RAW usage modifier.
**	10-May-2001 (jenjo02)
**	    LO_RAW changed to special raw filename.
**	11-May-2001 (jenjo02)
**	    Added return codes so that LO callers can discriminate 
**	    actual error from blanket "FAIL" when needed.
**	17-Oct-2001 (jenjo02)
**	    Added LO_IS_RAW return code
**  05-Jul-2005 (fanra01)
**      Bug 114804
**      Moved LO status codes into lo.h for consistent values on all
**      platforms.
**	30-mar-2006 (toumi01) BUG 115913
**	    Introduce for LOinfo a new query type of LO_I_XTYPE that can,
**	    without breaking the ancient behavior of LO_I_TYPE, allow for
**	    query of more exact file attributes: LO_IS_LNK, LO_IS_CHR,
**	    LO_IS_BLK, LO_IS_FIFO, LO_IS_SOCK. Also add the ability to
**	    stat a link and not the referenced file.
**	08-Nov-2010 (kiria01) SIR 124685
**	    In cleaning up PSF errors, added LOCATION_INIT as a supported
**	    initial value for LOCATION.
**/


/* 
** Defined Constants.
*/


/* 
** Symbols used in arguments to LOfroms
** special values chosen so that and's (&'s) of them
** will produce different values.
*/
#define	NODE		06
#define	PATH		05
#define	FILENAME	03

/* type used by flags to LOfroms and LOwhat */

#define	LOCTYPE		i2

/* symbols returned in flag argument to LOlist */

#define	ISDIR		01
#define	ISFILE		02


/* maximum buffer length of location buffer */

# define MAX_LOC		256

/* maximum buffer length for parts made by an LOdetail() call */
#define			LO_DEVNAME_MAX		16
#define			LO_PATH_MAX			255
#define			LO_FSUFFIX_MAX		3
#define			LO_FVERSION_MAX		4
#define			LO_FPREFIX_MAX		12

/* Maximum filename length >= LO_FPREFIX_MAX + LO_FSUFFIX_MAX + LO_FSUFFIX_MAX */
#define			LO_FILENAME_MAX		20

/*
** used by LOingpath
*/

# define	LO_DB		"data/"
# define	LO_CKP		"ckp/"
# define	LO_JNL		"jnl/"
# define	LO_DMP		"dmp/"
# define 	LO_WORK		"work/"
# define        JNLACTIVE       "jnl/active/"
# define        JNLFULL         "jnl/full/"
# define        JNLEXPIRED      "jnl/expired/"

/*
** Special filename that defines a RAW location.
** Note that this must have the same value as DM2F_RAW_LINKNAME
** in back/dmf/hdr/dm2f.h
*/
# define	LO_RAW		"iirawdevice"

/*! symbol indicating support for concurent lo_delete */

# define	LO_CONCUR_DELETE	1

/*
**	OS filename parameters
*/
# define	LO_NM_LEN	256	/* "reasonable" length */
# define	LO_EXT_LEN	0
# define	LO_OBJ_EXT	"o"
# define	LO_EXE_EXT	NULL
# define	LO_NM_CASE	1	/* case sensitive */
# define	LO_NM_VERS	0	/* no versions */


/*
** Type Definitions.
*/

/* structure of a location */
struct _LOCATION
{
	char	*node;
	char	*path;
	char	*fname;
	char	*string;
	FILE	*fp;
	char	desc;
} ;
#define LOCATION_INIT {NULL,NULL,NULL,NULL,NULL,0}

/*
**	structure for LOinfo calls.
*/

typedef struct
{
	i4	li_type;
#define 	LO_IS_DIR	1
#define 	LO_IS_FILE	2
#define 	LO_IS_MEM	3	/* not used */
/* extended types for LO_I_XTYPE */
#define		LO_IS_LNK	4
#define		LO_IS_CHR	5
#define		LO_IS_BLK	6
#define		LO_IS_FIFO	7
#define		LO_IS_SOCK	8
#define		LO_IS_UNKNOWN	9

	i4	li_perms; 	/* the permissions, as bit flags */
#define 	LO_P_WRITE	0x01
#define 	LO_P_READ	0x02
#define 	LO_P_EXECUTE	0x04

	OFFSET_TYPE	li_size;
	SYSTIME	li_last;	/* time of last access */
} LOINFORMATION;

/* bits for LOinfo requests. */
#define	LO_I_TYPE	0x01
#define	LO_I_PERMS	0x02
#define	LO_I_SIZE	0x04
#define	LO_I_LAST	0x08
#define LO_I_XTYPE	0x10	/* extended file type query */
#define LO_I_LSTAT	0x20	/* stat links and not referenced files */
/* NOTE: LO_I_ALL requests LO_I_TYPE and not LO_I_XTYPE, so to do all
   queries and to do the extended type query, use (LO_I_ALL|LO_I_XTYPE) */
#define	LO_I_ALL	(LO_I_TYPE | LO_I_PERMS | LO_I_SIZE | LO_I_LAST)

/*
**	structure for LOwcard directory searches.  Caller allocates,
**	but does not touch contents.
*/
typedef struct
{
	char *path;		/* path */
	char *fn;		/* filename portion of path */
	char *prefix;
	char *suffix;
	PTR dirp;		/* really a DIR pointer */
	i4 magic;		/* magic number for validation */
	char bufr[MAX_LOC];	/* name storage */
} LO_DIR_CONTEXT;

/*
	macro LO routines
*/
/*
# define	LOpurge		OK	 comment out temporarily, because frontend
*/

/*
**  Forward and/or External function references.
*/
FUNC_EXTERN STATUS  LOgtfile(
#ifdef CL_PROTOTYPED
            LOCATION        *loc1,
            LOCATION        *loc2
#endif
);

FUNC_EXTERN STATUS  LOreset(
#ifdef CL_PROTOTYPED
            void
#endif
);

FUNC_EXTERN STATUS  LOsave(
#ifdef CL_PROTOTYPED
            void
#endif
);

FUNC_EXTERN STATUS  LOcurnode(
#ifdef CL_PROTOTYPED
            LOCATION        *loc
#endif
);

FUNC_EXTERN STATUS  LOdbname(
#ifdef CL_PROTOTYPED
            LOCATION        *loc,
            char            *name
#endif
);

FUNC_EXTERN STATUS  LOgetowner(
#ifdef CL_PROTOTYPED
            LOCATION        *loc,
            char            **buf
#endif
);
