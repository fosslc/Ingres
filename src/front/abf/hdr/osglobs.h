/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    osglobs.h -	OSL Parser Global Declarations File.
**
** Description:
**	Contains the declarations for the OSL parser global variables.
**
** History:
**	Revision 7.0  89/08  wong
**	Added 'osUser'.
**
**	Revision 6.2  89/05  wong
**	Added 'osChkSQL'.
**
**	Revision 6.0  87/06  wong
**	Added 'osWarning' and 'osCompat' flags.  05/87  wong
**	Added 'osAppid'; removed "OL.c" constants.  03/87  wong
**	Moved global constants to "oslconf.h" and file variables
**	to "osfiles.h."  02/86 wong
**
**	Revision 5.1  86/11/04  23:42:26  wong
**	Removed 'isidch()' macro and declaration for 'yychar'.
**	Added translator arguments, 'osFid' and 'osDebugIL'.
**	(Removed 'osAggflag'; unused.)  09/86  wong
**
**      15-nov-95 (toumi01; from 1.1 axp_osf port)
**         added dkhor's change for axp_osf
**         20-jan-93 (dkhor)
**         For axp_osf declare osAppid & osFid as long (8 bytes).
**      23-mar-1999 (hweho01)
**         Extended axp_osf changes to ris_u64 (AIX 64-bit platform).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      08-Sep-2000 (hanje04)
**         Extended axp_osf changes to axp_lnx (Alpha Linux)
**	06-Mar-2003 (hanje04)
**	   Extend axp_osf changes to all 64bit platforms (LP64)
**	24-apr-2003 (somsa01)
**	    Use i8 instead of long for LP64 case.
**/

/*
** FRAMES
*/
GLOBALREF char	*osFrm;		/* frame or procedure name */
GLOBALREF char	*osForm;	/* form name */
#if defined(LP64)
GLOBALREF i8  osAppid;        /* application ID */
GLOBALREF i8  osFid;
#else /* LP64 */
GLOBALREF i4	osAppid;	/* application ID */
GLOBALREF i4	osFid;
#endif /* LP64 */
GLOBALREF PTR	osIGframe;	/* in-core frame representation */

/*
** ENVIRONMENTS
*/
GLOBALREF bool	osUser;		/* user-written 4GL */
GLOBALREF bool	osChkSQL;	/* non Open/SQL warnings */
GLOBALREF bool	osWarning;	/* Output warnings */
GLOBALREF i4	osCompat;	/* Compatiblity Flag */
GLOBALREF bool	osList;		/* Produce a listing ? */
GLOBALREF bool	osDebugIL;	/* IL debug flag */
GLOBALREF bool  osStatic;	/* 'static' frame */

/*
** CODEGEN
*/
GLOBALREF char 	*osSymbol;		/* symbol name */

/*
** PARSER
*/
GLOBALREF i4	yydebug;

/*
** DATABASE
*/
GLOBALREF char	*osDatabase;
GLOBALREF char	*osXflag;
GLOBALREF char	*osUflag;

/* ERROR COUNT */
GLOBALREF i4	osErrcnt;
GLOBALREF i4	osWarncnt;
