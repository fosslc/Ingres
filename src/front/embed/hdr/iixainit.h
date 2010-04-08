
/*--------------------------- iixainit.h -----------------------------------*/

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
**  Name: iixainit.h - LIBQXA entrypoints, called at startup and shutdown
**                     of LIBQXA.
**
*/

#ifndef  IIXAINIT_H
#define  IIXAINIT_H

/*
**   Name: IIxa_startup()
**
**   Description:
**       This function initializes all global structures and fields. It is
**       called when the *first* xa_open() call is received in LIBQXA.
**
**   Current Definition:
**       We do not allow co-existence of connections/gca associations that
**       are in XA and non-XA mode in an Ingres client program operating in
**       an XA environment.
**
**   Inputs:
**       None.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successful initialization/startup.
**
**	History:
**	  ?	Initial version
**	12-dec-1996 (donc)
**	   At least give the startup and shutdown flags a datatype so
**	   that things compile for NT.
**	13-oct-1998 (thoda04)
**	   Startup flag is a i4  datatype.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

FUNC_EXTERN   DB_STATUS     IIxa_startup();



/*
**   Name: IIxa_shutdown()
**
**   Description:
**       This function cleans up all global structures and fields. It is called
**       either when the *last* xa_close() call has been received and
**       processed in LIBQXA, or when the appln server (AS) needs to be
**       shutdown for any other reason. 
**
**   Inputs:
**       None.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successful shutdown.
*/

FUNC_EXTERN  DB_STATUS     IIxa_shutdown();


/* Global Definition, checked in IIxa_open() in iixamain.c */

#ifdef NT_GENERIC
GLOBALREF  i4  IIxa_startup_done;
GLOBALREF  i4  IIxa_shutdown_done;
#else
GLOBALREF   IIxa_startup_done;
GLOBALREF   IIxa_shutdown_done;
#endif

#endif  /* ifndef IIXAINIT_H */

/*--------------------------- end of iixainit.h ----------------------------*/

