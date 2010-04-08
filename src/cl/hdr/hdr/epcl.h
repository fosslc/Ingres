/*
** Copyright (c) 1985, 2007 Ingres Corporation
**
*/

/**
** Name: EP.H - Global definitions for execution privileges for 
**		Compatibility library.
**
** Description:
**	The file contains macros and definitions necessary to determine 
**	execution privileges of the binaries within Ingres.
**	Initial purpose is Vista port.
**
** History:
**      13-aug-2007 (drivi01)
**          Created.
**      14-Dec-2007 (rapma01)
**          added new line at the end of the file
*/
#ifndef EPCL_INCLUDED
#define EPCL_INCLUDED 1
#include <pc.h>

/*
** Macro Definitions
*/
# ifdef NT_GENERIC 
# define ELEVATION_REQUIRED() \
	{ \
	 if (GVvista() && !PCisAdmin()) \
	 { \
		SIprintf( ERget(E_CL2D01_NO_PRIVILEGE) ); \
		PCexit(PC_ELEVATION_REQUIRED); \
	 } \
	}
# else
#define ELEVATION_REQUIRED()
# endif
	
# ifdef NT_GENERIC 
# define ELEVATION_REQUIRED_WARNING() \
	{ \
	 if (GVvista() && !PCisAdmin()) \
  	     SIprintf( ERget(E_CL2D01_NO_PRIVILEGE) ); \
	}
# else
#define ELEVATION_REQUIRED_WARNING()
# endif


/*
** Error messages
*/
# define ERROR_REQ_PRIVILEGE	ERx("Elevated privileges are required to run this command\r\n")
# define ERROR_DENIED_PRIVILEGE	ERx("\nAccess Denied as you do not have sufficient privileges\r\n")
# define ERROR_REQ_ELEVATION	ERx("You must invoke this utility running in elevated mode.\n")

#endif  /* EPCL_INCLUDED */

