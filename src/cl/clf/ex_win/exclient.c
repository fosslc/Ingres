/*
**	Copyright (c) 1983 Ingres Corporation
*/
# include 	<compat.h>
# include	<excl.h>
# include	<ex.h>

/*
**  Set the default client type to be an user application.
*/
static i4       client_type = EX_USER_APPLICATION;
static bool     ex_initialized = FALSE;

/*{
** Name:	EXsetclient - Set client information for the EX subsystem
**
** Description:
**	This routine informs the EX subsystem as to the type of client
**	that it is dealing with.  The client type information is
**	important in that it allows EX to modify its behavior dependent
**	on the client
**
**	For user applications, EX will only intercept access violation and
**	floating point exceptions.  This becomes the default behavior for EX.
**	It is up to the user application to deal with any other exceptions.
**
**	If the client is the Ingres DBMS, then EX will need to intercept
**	any exceptions that will cause a process to exit (current behavior).
**
**	In the case of Ingres Tools, EX will also intercept user generated
**	exit exceptions (such as interrupt) in addition to the access
**	violation and floating point exceptions.
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
**		EX_OK			If everything succeeded.
**		EXSETCLIENT_LATE	If the EX subsystem has already
**					been initialized when this is called.
**		EXSETCLIENT_BADCLEINT	If an unknown client was passed in.
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
*/
STATUS
EXsetclient(i4  client)
{
	if (!ex_initialized)
	{
	    switch (client)
	    {
	      case EX_INGRES_DBMS:
	      case EX_INGRES_TOOL:
	      case EX_USER_APPLICATION:
	        client_type = client;
		break;

	      default:
		return(EXSETCLIENT_BADCLIENT);
		break;
	    }
	}
	else
	{
	    return(EXSETCLIENT_LATE);
	}
	return(EX_OK);
}
