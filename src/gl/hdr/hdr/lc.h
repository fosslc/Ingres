
# ifndef _LC_HDR
# define _LC_HDR 1


/*
**	Copyright (c) 2004 Ingres Corporation
*/

/* 
** Name:	LC.h	- location specific function externs
**
** Specification:
**	This file contains defines for local specific function externs.
**
** Description:
**	 
**
** History:
**	07-May-2002 (gupsh01)
**	    initial creation. 
**/

typedef struct _LOCALE_MAP
{
 char stdloc [GL_MAXNAME];
 char ingloc [GL_MAXNAME];
} LOCALE_MAP;

STATUS LC_getStdLocale ( char 		*charset, 
			 char 		*std_encoding, 
		         CL_ERR_DESC 	*err_stat);


/*
** STATUS values of various functions, from <erglf.h>
*/
# define E_GL_MASK              0xD50000
# define E_LC_MASK              0x7000

# define E_LC_LOCALE_NOT_FOUND	(E_GL_MASK + E_LC_MASK + 0x01)
# define E_LC_CHARSET_NOT_SET	(E_GL_MASK + E_LC_MASK + 0x02)
# define E_LC_FORMAT_INCORRECT	(E_GL_MASK + E_LC_MASK + 0x03)

#endif 
