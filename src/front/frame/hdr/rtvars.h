/*
**	rtvars.h
**	
**      Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	rtvars.h - Local runtime variables include header file.
**
** Description:
**	This file contains variable declarations local to the
**	runtime/tbacc directories.  It is assumed that frame.h
**	and runtime.h etc. are included before this file.
**
** History:
**	12/09/89 (dkh) - Initial version.
**	12/10/89 (Mike S) - Remove renamed globals
**	25-apr-1996 (chech02)
**	    	Added FAR to IIfrscb for windows 3.1 port.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# ifdef ATTCURSES
GLOBALREF       i2      IIvalcomms[];
# else
GLOBALREF       u_char  IIvalcomms[];
# endif
GLOBALREF       RUNFRM          *IIrootfrm;
GLOBALREF       RUNFRM          *IIstkfrm;
GLOBALREF       RUNFRM          *IIfrmio;
GLOBALREF       FRAME           *IIprev;
GLOBALREF       i4              IIresult;
GLOBALREF       i4              IIfromfnames;
#ifdef WIN16
GLOBALREF	FRS_CB		*FAR IIfrscb;
#else
GLOBALREF	FRS_CB		*IIfrscb;
#endif
