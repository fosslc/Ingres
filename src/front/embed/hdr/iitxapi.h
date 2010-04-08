/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
**  Name: iitxapi.h - 
**
**  Description:
**
**  History:
**	17-nov-1993 (larrym)
**	    gleened from iixaswch.c
*/

/*
**  Name: IItux_open  - Call IIxa_open, then initialize tuxedo stuff.
**
**  Description: 
**
**  Inputs:
**      xa_info    - character string, contains the RMI open string.
**      rmid       - RMI identifier, assigned by the TP system, unique to the AS
**                   process w/which LIBQXA is linked in.
**      flags      - TP system-specified flags, one of TMASYNC/TMNOFLAGS.
**
**  Outputs:
**	Returns:
**          XA_OK          - normal execution.
**          XAER_ASYNC     - if TMASYNC was specified. Currently, we don't 
**			     support
**                           TMASYNC (post-Ingres65 feature).
**          XAER_RMERR     - the RMI string was ill-formed. 
**          XAER_INVAL     - illegal status argument.
**          XAER_PROTO     - if the RMI is in the wrong state. Enforces the 
**			     state tables in the XA spec. 
**	Errors:
**	    None.
-*
**  Side Effects:
**	    None.
**	
**  History:
**	17-nov-1998 (larrym)
**	    written.
*/
int
IItux_open( char      *xa_info,
	   int       rmid,
	   long      flags);
