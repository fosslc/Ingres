/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<menu.h>
# include	<runtime.h>
# include	<si.h>
# include	<er.h>
# include	<lqgdata.h>


/**
** Name:	iiendform.c	-	End the forms system
**
** Description:
**
**	Public routines defined:
**		II_fend
**		IIendforms
**
** History:
**	22-feb-1983  -  Extracted from original runtime.c (jen)
**	06-feb-1985  -  Added II_fend to hide FTrestore and
**			FT_NORMAL from  LIBQ (dkh)
**	21-oct-1985  -  Modified II_fend to accept argument. (ncg)
**	04-apr-1987 (peter)
**		Add call to iiugfrs_setting to turn off message system.
**	08/14/87 (dkh) - ER changes.
**	09/23/89 (dkh) - Porting changes integration.
**	11/02/90 (dkh) - Replaced IILIBQgdata with IILQgdata().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Update defn for gcc 4.3.
**/


/*{
** Procedure:	II_fend
** Purpose:	Backward call to forms system from LIBQ.
** Parameters:
**	normal	- bool	- Reset to NORMAL/FORMS
** Return Values:
**	None
** Notes:
**	This call is called from LIBQ in so that LIBQ does not have to bring
** in the forms system to turn it on/off.  The call is made via an indirect
** function pointer (IIf_end).  This call is made in 3 cases:
**	1.  LIBQ aborts we want to reset terminals to NORMAL.
**	2.  LIBQ spawns a process and the FRS is on then reset to NORMAL.
**	3.  LIBQ ends a spawned process and FRS is on then return to FORMS.
** In case 3 there may be a DISPLAY loop going on so we want to make sure that
** the screen will get redisplayed, so we call IIclrscr(). Note that IIclrscr
** won't harm anything if there is no current form displayed.(ncg)
*/
VOID
II_fend( bool normal )
{
	VOID	FTmessage();		/* Address of forms msg routine */
	i4	IIboxerr();		/* Address of forms err routine */

	if (normal)
	{
		iiugfrs_setting(NULL, NULL);	/* turn off frs messages */
		FTrestore( FT_NORMAL );
	}
	else
	{
		iiugfrs_setting(FTmessage, IIboxerr);
					/* turn on frs messages */
		FTrestore( FT_FORMS );
		IIclrscr();
	}
}

/*{
** Name:	IIendforms	-	End the forms system
**
** Description:
**	Closes testing procedures, issues a close call to FT, and
**	flushes stdout.
**
**	This routine is part of RUNTIME's external interface.
**
** Inputs:
**
** Outputs:
**
** Returns:
**
** Exceptions:
**	none
**
** Example and Code Generation:
**	## endforms
**
**	IIendforms();
**
** Side Effects:
**
** History:
**
**	10-oct-92 (leighb) DeskTop Porting Change:
**		Don't write "\n" to stdout in Tools for Windows.
**	9-nov-93 (robf)
**          Reset dbprompthandler to default.
*/
VOID
IIendforms()
{
	if (IILQgdata()->form_on)
	{
		IItestfrs((i4) FALSE);	/* Close testing procedures */
		IILQgdata()->form_on = FALSE;
		iiugfrs_setting(NULL, NULL);	/* Turn off frs messages */
		FTclose(NULL);
#ifndef	PMFEWIN3
		SIprintf(ERx("\n"));
#endif
		IILQshSetHandler((int)14,(int (*)())0);
	}
	SIflush(stdout);
	SIflush(stderr);
}
