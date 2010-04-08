/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** History:
**      07-Sep-1998 (fanra01)
**          Corrected incomplete last line.
*/

#ifndef LOGIN_INCLUDED
#define LOGIN_INCLUDED

#include <ddfcom.h>

GSTATUS 
AllocUserPass(
	char	*message, 
	char	**user, 
	char	**password);

#endif
