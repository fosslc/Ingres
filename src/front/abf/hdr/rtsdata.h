/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	rtsdata.h -	ABF RunTime System Data Declaration File.
**
** Description:
**	Contains declarations for the shared ABF RTS data.  These are used by
**	the ABF RTS.  ABF and ABFIMAGE use IIar_Dbname and IIar_Status.
*/

/*
** The database
*/
GLOBALREF char		*IIarDbname;

GLOBALREF STATUS	IIarStatus;	/* exit status */
