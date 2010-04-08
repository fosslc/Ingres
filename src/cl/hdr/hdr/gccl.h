/*
** Copyright (c) 2009 Ingres Corporation
*/

/**
** Name: gccl.h - GC definitions.
**
** Description:
**	Contains platform specific GC definitions.
**
** History:
**      11-Aug-09 (gordy)
**          Created.
*/

#ifndef GCCL_INCLUDED
#define GCCL_INCLUDED 1

#ifdef NT_GENERIC

/*
** Note: The following symbols could be defined 
** directly from the system defined symbols, but 
** it would require including a number of system 
** header files, such as windows.h.
*/

/*
** Max host name length comes from WinInet.h
** and the symbol INTERNET_MAX_HOST_NAME_LENGTH.
*/
#define	GC_HOSTNAME_MAX		256

/*
** Max user name length comes from Lmcons.h
** and the symbol UNLEN.
*/
#define	GC_USERNAME_MAX		256

#elif defined(VMS)

/*
** GCusername() reportedly has a max of 15 characters.
** Provide the standard Ingres max symbol length.
*/
#define	GC_HOSTNAME_MAX		32

/*
** Documented max for SYS$GETUAI() is 12.
** Provide the standard Ingres max symbol length.
*/
#define	GC_USERNAME_MAX		32

#else	/* Unix */

#include <limits.h>

/*
** Posix provides HOST_NAME_MAX symbol as 
** maximum length returned by gethostname().
*/
#ifdef HOST_NAME_MAX
#define	GC_HOSTNAME_MAX		HOST_NAME_MAX
#endif

#endif	/* Unix */

/*
** Default maximum hostname length
*/
#ifndef GC_HOSTNAME_MAX
#define	GC_HOSTNAME_MAX		256
#endif


/*
** Default maximum username length
*/
#ifndef GC_USERNAME_MAX
#define	GC_USERNAME_MAX		256
#endif

#endif  /* GCCL_INCLUDED */

