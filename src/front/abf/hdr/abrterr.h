/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<erar.h>

/**
** Name:    abrterr.h -	ABF Run-Time System Error Definitions File.
**
** Description:
**	Defines the human-readable names for the ABF RTS error numbers.
**
** History:
**	Revision 6.0  87/08  wong
**	Initial revision.
**
**	13-aug-1987 (arthur)
**		Added additional error numbers, formerly in abfconsts.h.
**	18-aug-92 (davel)
**		added POSTOSQL (for positional parameter passed to db proc).
*/

#define	NOSUCHDB	E_AR0000_NOSUCHDB	/* 1 (db) */
#define	OUTOFMEM	E_AR0001_OUTOFMEM	/* 1 (routine) */
#define	ALLOCFRM	E_AR0002_ALLOCFRM	/* 1 frame name */
#define PRMTYPE		E_AR0003_PRMTYPE	/* Parameter type mismatch */
						/*
						** (called object name)
						** (called object kind)
						** (parameter name)
						** (formal type)
						** (actual type)
						*/
#define POSTOOSL	E_AR0004_POSTOOSL	/* OSL proc with positional */
						/*
						** (procedure name)
						*/
#define POSTOSQL	E_AR0028_POSTOSQL	/* SQL proc with positional */
						/*
						** (procedure name)
						*/
#define	KEYTOHOST	E_AR0005_KEYTOHOST	/* Host proc with keyword */
						/*
						** (procedure name)
						*/
#define	RETNOTYPE	E_AR0006_RETNOTYPE	/* return from notype */
						/*
						** (name of object)
						** (object type)
						** (expected type)
						*/
#define ABNULLPRM	E_AR0007_ABNULLPRM	/* Nullable parameter passed to
						** non-nullable field. */
						/*
						** (called object name)
						** (called object kind)
						** (parameter name)
						*/
#define ABNULLBYREF	E_AR0008_ABNULLBYREF	/* Nullable field returned byref
						** to non-nullable field. */
						/*
						** (name of nullable field)
						** (called object kind)
						*/
#define NOTBLFLD	E_AR0009_NOTBLFLD
#define	DBGFILE		E_AR000A_DBGFILE	/* 0 */
#define	NOINIFRM	E_AR000B_NOINIFRM	/* Unable to initialize form */
						/*
						** (frame's name)
						** (form's name)
						*/
#define	ABNXTYPE	E_AR000C_ABNXTYPE
#define	ABRETFRM	E_AR000D_ABRETFRM	/* Return from non-OSL frame */
						/*
						** (frame's name)
						** (frame's type)
						*/
#define	ABNULLOBJ	E_AR000E_ABNULLOBJ	/* (object's name) Tried to run
						** a object with a NULL ptr.
						*/
#define	RETTYPE		E_AR000F_RETTYPE	/* Return type not compatible*/
						/*
						** (called obj name)
						** (called obj kind)
						** (type returned)
						** (type expected)
						*/
#define	ABEXPREVAL	E_AR0010_ABEXPREVAL	/* Error evaluating ADF expr */
#define	ABNULLNAME	E_AR0011_ABNULLNAME	/* NULL used where an OSL name
						   or other string required */
#define	ABNOFIELD	E_AR0012_ABNOFIELD	/* 2 (field, form) Bad field specified
							in passed query */
#define	ABNOTFCOL	E_AR0013_ABNOTFCOL	/* 3 (column, tblfld, form) Bad column */
#define	NOQRYFRM	E_AR0014_NOQRYFRM	/*
						** Mismatch between passed
						** query's form and frame's
						** form name.
						*/
						/*
						** (frame's name)
						** (query's form name)
						** (frame's form name)
						*/
#define	NOQRYTBL	E_AR0015_NOQRYTBL	/*
						** table field given in passed
						** query does not exist.
						*/
						/*
						** (frame's name)
						** (frame's form name)
						** (table name)
						*/
#define	DOMOVER		E_AR0016_DOMOVER	/* Too many domains in query */
#define	ABBADQUAL	E_AR0017_ABBADQUAL	/* Bad qualification type in
						** ABRT_QUAL structure.
						*/
#define	PARAMSPACE	E_AR0018_PARAMSPACE	/* Not enough space in query
						** passed as argument for
						** size value.
						*/
#define	NOPARAM		E_AR0019_NOPARAM	/* Bad parameter name. */
						/*
						** (parameter name)
						** (called object name)
						** (called object kind)
						*/
/*
** The following set of errors (ABPROGERR through ABNOARG)
** are for errors coming from UTexe.
** They all take the same set of parameters, which are (in order):
**	program error error_in_hex a0, a1, a2
**
** For ABPROGERR, a0 is any system message returned by ERreport.
** For all others, a0-2 are from the errv returned by UTexe.
** The text of the error message determines which arguments to use.
*/
#define	ABPROGERR	E_AR001A_ABPROGERR
#define	ABBIGCOM	E_AR001B_ABBIGCOM	/* Maps to UTEBIGCOM */
#define	ABARGLIST	E_AR001C_ABARGLIST	/* Maps to UTENOEQUAL,
						   UTENOPERCENT, UTENOSPEC
						   or UTEBADARG */
#define	ABBADSPEC	E_AR001D_ABBADSPEC	/* UTEBADSPEC */
#define	ABBADTYPE	E_AR001E_ABBADTYPE	/* UTEBADTYPE */
#define	ABNOSYM		E_AR001F_ABNOSYM	/* UTENOSYM */
#define	ABNOPROG	E_AR0020_ABNOPROG	/* UTENOPROG */
#define	ABMODTYPE	E_AR0021_ABMODTYPE	/* UTENoModuleType */
#define	ABNOARG		E_AR0022_ABNOARG	/* UTENOARG */
/*
** end of errors from UTexe.
*/
#define ABTRCFILE	E_AR004F_ABTRCFILE	/* Can't open trace file. */
						/*
						** (name of file)
						*/

/*
** moved from ABF for use of abfimage
*/
#define APPNAME		E_AR0051
#define NOSUCHAPP	E_AR0050
#define ABTMPDIR	E_AR0052

#define GLOBUNDEF	E_AR0053_glo_undef
#define GLOBBADTYPE	E_AR0054_glo_mismatch
#define BADCLASS	E_AR0055_bad_type
#define NOATTR		E_AR0056_no_attribute
#define BADINDEX	E_AR0057_bad_index
#define NOTARRAY	E_AR0059_notarray
#define CLASSFAIL	E_AR0060_class_fail
#define NOTSCALAR	E_AR0062_not_scalar
#define INTOATTRTYPE	E_AR0063_bad_convert_into
#define FROMATTRTYPE	E_AR0064_bad_convert_from

