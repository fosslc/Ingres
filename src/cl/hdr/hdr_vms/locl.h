/*
** Copyright (c) 1985, 2008 Ingres Corporation
**
*/

#include	<tm.h>

/**
** Name: LO.H - Global definitions for the LO compatibility library.
**
** Description:
**      The file contains the type used by LO and the definition of the
**      LO functions.  These are used for manipulating locations.
**
** History:
**      01-oct-1985 (jennifer)
**          Updated to coding standard for 5.0.
**      11-Sep-86 (ericj)
**          Added constants to define the maximum of parts used in an
**	    LOdetail call.
**      30-dec-86 (ericj)
**          Added constant to define the maximun file name length.
**	27-Jan-88   (ericj)
**	    Updated to support sort locations.
**      22-May-89 (cal)
**	    Merged Edhsu's addition of DMP with MikeS' addition
**	    of loinformation.	    
**	27-sep-1989 (Mike S)
**	   Add VMS filename structures to LOCATION
**	30-oct-92 (jrb)
**	    Changed "LO_SORT" to "LO_WORK" and changed definition from "data" to
**	    "work" for multi-location sorts project.
**      8-jul-93 (ed)
**          move nested include protection to glhdr
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	26-nov-2001 (kinte01)
**	    Added some lo status return calls plus lo_raw definition
**	21-jan-2005 (abbjo03)
**	    Remove ancient LOingpath constants since they cause conflicts with
**	    back/aswf/wmo code.
**	11-may-2005 (abbjo03)
**	    Change li_size member of LOINFORMATION to SIZE_TYPE since file
**	    size should be an unsigned quantity.
**  05-Jul-2005 (fanra01)
**      Bug 114804
**      Moved LO status codes into lo.h for consistent values on all
**      platforms.
**      20-Aug-2007 (horda03) Bug 118877
**          Added LO_I_XTYPE LO_I_LSTAT for LOinfo(). Though the flags are
**          not used by the VMS' LOinfo() as files can't be symbolic links.
**	31-jan-2008 (toumi01/abbjo03) BUG 115913
**	    Add LO_IS_UNKNOWN.
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

/* symbols returned in flag argument to LOlist */

#define	ISDIR		01
#define	ISFILE		02


/* maximum buffer length of location buffer */

# define MAX_LOC		250

/* maximum buffer length for parts made by an LOdetail() call */
#define			LO_DEVNAME_MAX	    16
#define			LO_PATH_MAX	    255
#define			LO_FSUFFIX_MAX	    3
#define			LO_FVERSION_MAX	    4
#define			LO_FPREFIX_MAX	    12

/* Maximum filename length >= LO_FPREFIX_MAX + LO_FSUFFIX_MAX + LO_FSUFFIX_MAX */
#define			LO_FILENAME_MAX	    20

/*
** used by LOingpath
*/

# define	JNLACTIVE	"jnl.active"	/* change to journal */
# define	JNLFULL		"jnl.full"	/* ditto */
# define	JNLEXPIRED	"jnl.expired"	/* ditto */

/*
** Special filename that defines a RAW location.
** Note that this must have the same value as DM2F_RAW_LINKNAME
** in back/dmf/hdr/dm2f.h
*/
# define        LO_RAW          "iirawdevice"

/*
**  The new improved constants to be used with LOingpath.
*/
# define	LO_DB		"data"
# define	LO_CKP		"ckp"
# define	LO_JNL		"jnl"
# define	LO_DMP		"dmp"
# define	LO_WORK		"work"

/*! symbol indicating support for concurent lo_delete */

# define	LO_CONCUR_DELETE	1

# define	LO_EXT_LEN	39
# define	LO_NM_LEN	39
# define	LO_OBJ_EXT	"obj"
# define	LO_EXE_EXT	"exe"
# define	LO_NM_CASE	0
# define	LO_NM_VERS	1

/*
** Type Definitions.
*/
/* type used by flags to LOfroms and LOwhat */

typedef i2	LOCTYPE;

/* Directory context used by wildcard routines */
typedef struct
{
	char	filename[MAX_LOC+1];	/* wildcard directory spec */
	u_i4	context; 		/* search context */
	bool	node;			/* was an explicit node specified */
} LO_DIR_CONTEXT;

/* structure of a location */

# define NAM_SIZE 0x060		/* This MUST be at least NAM$C_BLN */
typedef struct
{
	char	*string;	/* filename buffer */
	LO_DIR_CONTEXT	*wc;    /* wildcard context */
	char	*nodeptr;	/* Pointer to node part of filename */
	char	*devptr;	/* Pointer to device part of filename */
	char	*dirptr;	/* Pointer to directory part of filename */
	char	*nameptr;	/* Pointer to name part of filename */
	char	*typeptr;	/* Pointer to type part of filename */
	char	*verptr;	/* Pointer to version part of filename */
	/* 
	** If a part isn't present, its pointer points to where the
	** part would be inserted.  For instance, in "DEV:FILE.TYPE", 
	** nodeptr points to "D", devptr to "F", verptr to the trailing EOS.
	*/
  	LOCTYPE	desc;		/* What parts LOCATION contains */

	unsigned char nodelen;	/* Length of node part */
	unsigned char devlen;	/* Length of device part */
	unsigned char dirlen;	/* Length of directory part */
	unsigned char namelen;	/* Length of name part */
	unsigned char typelen;	/* Length of type part */
	unsigned char verlen;	/* Length of version part */
} LOCATION;
#define LOCATION_INIT {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,0,0,0,0,0,0,0}


/*
**      structure for LOinfo calls.
*/

typedef struct
{
	i4     li_type;
#define         LO_IS_DIR       1
#define         LO_IS_FILE      2
#define         LO_IS_MEM       3
/* extended types for LO_I_XTYPE */
/* values 4 through 8 are not used on VMS */
#define		LO_IS_UNKNOWN	9

	i4     li_perms;       /* the permissions, as bit flags */
#define         LO_P_WRITE      0x01
#define         LO_P_READ       0x02
#define         LO_P_EXECUTE    0x04

	SIZE_TYPE li_size;
	SYSTIME li_last;        /* time of last access */
} LOINFORMATION;

/* bits for LOinfo requests. */
#define LO_I_TYPE       0x01
#define LO_I_PERMS      0x02
#define LO_I_SIZE       0x04
#define LO_I_LAST       0x08
#define LO_I_XTYPE      0x10
#define LO_I_LSTAT      0x20
#define LO_I_ALL        (LO_I_TYPE | LO_I_PERMS | LO_I_SIZE | LO_I_LAST)


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
