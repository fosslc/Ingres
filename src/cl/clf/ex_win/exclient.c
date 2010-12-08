/*
**	Copyright (c) 1983 Ingres Corporation
*/
# include 	<compat.h>
# include	<excl.h>
# include	<ex.h>


/*{
** Name:	EXsetclient - Set client information for the EX subsystem
**
** Description:
**	This routine informs the EX subsystem as to the type of client
**	that it is dealing with.  The client type information is
**	important in that it allows EX to modify its behavior dependent
**	on the client
**
**	Windows doesn't in fact care, so EXsetclient is a stub here.
**
** Inputs:
**	EX_INGRES_DBMS		Client is the Ingres DBMS or any application
**				that desires the trap all behavior.
**
**	EX_INGRES_TOOL		Client is one of the Ingres Tools.
**
**	EX_USER_APPLICATION	Client is a user written application.  This
**				is actually unnecessary since this is the
**				default.  It is defined here just for
**				completeness.
**
** Outputs:
**
**	Returns:
**		EX_OK			always.
**
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	21-mar-94 (dave) - Initial version.
**	25-mar-94 (dave) - Added error return status if the subsystem
**			  has already been initialized.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	17-Nov-2010 (kschendel) SIR 124685
**	    JoeA pointed out that this routine is a no-op, make it
**	    look like one.
*/
STATUS
EXsetclient(i4  client)
{
    /* Nothing to do here */
    return(EX_OK);
}
