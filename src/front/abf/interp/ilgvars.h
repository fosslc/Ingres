/*
**Copyright (c) 1986, 2004 Ingres Corporation
** All rights reserved.
*/

/**
** Name:	ILGVARS.h	-	Globals used by OSL interpreter
**
** Description:
**	Declares global variables used by OSL interpreter.
**
** History:
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		change type of IIOframeptr and add IIOcontrolptr.
**
**	03/09/91 (emerson)
**		Integrated DESKTOP porting changes
**		(Changed IIITvtnValToNat to return i4 instead of nat).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*
** Global variables
*/
extern char		*IIOapplname;	/* name of the application */
extern IFFRAME		*IIOframeptr;	/* pointer to frame stack */
extern IFCONTROL	*IIOcontrolptr;	/* pointer to control stack */
extern IL		*IIOstmt;	/* pointer to current IL statement */
extern bool		IIOfrmErr;	/* if TRUE, exit frame due to error */
extern bool		IIOdebug;	/* if TRUE, IL statements are printed */

/*
** IRBLK for use by ILRF.
*/
extern IRBLK	il_irblk;

/*
** Table mapping IL op codes to execution routines.
*/
extern IL_OPTAB	IIILtab[];

/*
** Return types of commonly used routines
*/
IL		*IIOgiGetIl();
PTR		IIOgvGetVal();
DB_DATA_VALUE	*IIOFdgDbdvGet();
i4		IIITvtnValToNat();
