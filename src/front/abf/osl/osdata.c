/*
** Copyright (c) 1984, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<si.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<oslconf.h>
#include	<oserr.h>
#include	<osglobs.h>
#include	<osfiles.h>
#include	<ossym.h>
#include	<ostree.h>
#include	<osquery.h>

/**
** Name:    osdata.c - OSL Parser Global Data Definitions Module.
**
** Description:
**	Contains the definitions of the OSL parser globaldef variables and their
**	initializations.
**
** History:
**	Revision 5.1  86/11/04  15:02:00  wong
**	Removed most configuration variables ('osparser', etc.)
**	Parser state flags no longer globaldef; query state structures not used.
**	Added translator arguments, 'osFid' and 'osDebugIL'.
**	(Removed 'osAggflag'; unused.)
**
**	Revision 6.0  87/06  wong
**	Added 'osAppid', 'osWarning' and 'osCompat'.  Removed 'osRfilename'
**	and 'osPfilename'.  Added 'osErrfile'.
**	Added 'osmem', memory tag.  88/03
**
**	Revision 6.2  89/05  wong
**	Added 'osChkSQL' to flag checking for Open/SQL compliance.
**
**	Revision 6.3/03/00  89/08  wong
**	Added 'osUser' to flag user-written 4GL.
**
**	Revision 6.4
**	03/13/91 (emerson)
**		Added GLOBALDEF for osDecimal to store the value
**		specifed by the logical symbol II_4GL_DECIMAL
**		(which now will take the place of II_DECIMAL in 
**		the 4GL compiler).
**	08/04/91 (emerson)
**		Added GLOBALDEF for osAppname (so that we can
**		accept an application name in lieu of application ID
**		on the -a flag of the osl or oslsql command line).
**
**	Revision 6.5
**	16-jun-92 (davel)
**		added GLOBALDEF for bool osFloatLiteral to store the value
**		specified by the logical II_NUMERIC_LITERAL.  If
**		the value of II_NUMERIC_LITERAL is "float", then
**		osFloatLiteral will be set to TRUE, indicating that numeric 
**		literals are to be treated as floats if they are not integers.
**      11-jan-1996 (toumi01; from 1.1 axp_osf port)
**              added kchin's change for axp_osf
**              20-jan-93 (kchin)
**              On axp_osf, pointers and long are 8 bytes.  osAppid and
**              osFid must be declared as long to tally with usage.
**  25-July-1997 (rodjo04)
**      Declared  new GLOBALDEF boolian variable  chk_join.
**      This is used with macro Check_join. If this is set to true,
**      then a JOIN was encountered.
**  23-mar-1999 (hweho01)
**      Extended the change for axp_osf to ris_u64 (AIX 64-bit
**      platform).
**  10-mar-2000 (rodjo04) BUG 98775.
**      Removed above change (bug 82837). 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**  08-Sep-2000 (hanje04)
**      Extended the change for axp_osf to axp_lnx (Alpha Linux)
**      06-Mar-2003 (hanje04)
**              Extend axp_osf changes to all 64bit platforms (LP64).
**  24-apr-2003 (somsa01)
**	Use i8 instead of long for LP64 case.
**/

/*
** FILES
*/
GLOBALDEF char	*osIfilname = NULL;	/* The name of the input file */
GLOBALDEF FILE	*osIfile = NULL;
GLOBALDEF char	*osLfilname = NULL;	/* The name of the error listing file */
GLOBALDEF FILE	*osLisfile = NULL;
GLOBALDEF FILE	*osErrfile = NULL;

GLOBALDEF char	*osOfilname = NULL;	/* name of output file, to pass to CG */

/* 
** CODEGEN
*/
GLOBALDEF char	*osSymbol = NULL;	/* module entry symbol, to pass to CG */


/*
** FRAMES
*/
GLOBALDEF char	*osFrm = ERx("");	/* The name of the frame or procedure */
GLOBALDEF char	*osForm = ERx("");	/* The name of the form */
GLOBALDEF char	*osAppname = (char *)NULL; /* The name of the application;
					** set only if -a flag is followed by
					** something that's not a decimal number
					*/
#if defined(LP64)
GLOBALDEF i8	osAppid = 0;		/* The ID of the application (-a flag)*/
GLOBALDEF i8	osFid = 0;
#else /* LP64 */
GLOBALDEF i4	osAppid = 0;		/* The ID of the application (-a flag)*/
GLOBALDEF i4	osFid = 0;
#endif /* LP64 */
GLOBALDEF PTR	osIGframe = (PTR) NULL; /* ptr to in-core frame */

/*
** ENVIRONMENTS
*/
GLOBALDEF bool	osUser = TRUE;
GLOBALDEF bool	osChkSQL = FALSE;
GLOBALDEF bool	osWarning = TRUE;
GLOBALDEF i4	osCompat = 8;
GLOBALDEF bool	osList = FALSE;
GLOBALDEF bool	osDebugIL = FALSE;
GLOBALDEF bool	osStatic = FALSE;
GLOBALDEF char	osDecimal = EOS;
GLOBALDEF bool	osFloatLiteral = FALSE;

GLOBALDEF FILE	*osOfile = NULL;	/* File pointer for the debug file */

/*
** DATABASE
*/
GLOBALDEF char	*osDatabase = ERx("");
GLOBALDEF char	*osXflag = ERx("");
GLOBALDEF char	*osUflag = ERx("");

/*
** union used for passing pointer arguments of different types
** to node construction routines
*/
GLOBALDEF U_ARG	u_ptr[3] ZERO_FILL;

/*
** ERRORS
*/
GLOBALDEF i4	osErrcnt = 0;
GLOBALDEF i4	osWarncnt = 0;

/* Memory Allocation Tag */
GLOBALDEF u_i4	osmem = 0;



