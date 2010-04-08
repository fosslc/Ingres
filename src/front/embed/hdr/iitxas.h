/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Function prototypes
*/

/*
**  Name:	IItux_as_open
**
*/
int
IItux_as_init();

/*
**  Name:	IItux_as_close
**
*/
int
IItux_as_shutdown( i4  deregister );

/*
**  Name:	IItux_as_register_SVN
**	Get's called on an XA_START and calls ICAS to validate XID/flags.
*/
int
IItux_as_register_SVN(  i4	xa_call_type,
			XID	*xid,
			int	rmid);
/*
**  Name:	IItux_as_do_2phase
**
*/
void
IItux_as_do_2phase( TPSVCINFO *iitux_info );

/*
**  Name:	IItux_as_return_xids
**
*/
void
IItux_as_suicide( TPSVCINFO *iitux_info );
