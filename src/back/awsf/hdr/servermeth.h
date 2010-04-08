/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** History:
**	07-Sep-1998 (fanra01)
**	    Changed case of actsession to match file in piccolo.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
#ifndef SERVER_METH_INCLUDED
#define SERVER_METH_INCLUDED

#include <actsession.h>
#include <htmlform.h>

typedef struct __SERVER_DLL_METHOD 
{
	char*	name;
	char**	params;
} SERVER_DLL_METHOD , *PSERVER_DLL_METHOD;

typedef struct __SERVER_FUNCTION 
{
	char*	name;
	char*	*params;
	u_i4 params_size;
	PTR fct;
} SERVER_FUNCTION, *PSERVER_FUNCTION;

GSTATUS 
WSFAddServerFunction (	
	DDFHASHTABLE	*tab,
	char			*name, 
	PTR				entry,
	char*			params[]);

GSTATUS WSFGetServerFunction (
	char			*dll,
	char			*name,
	PSERVER_FUNCTION *func);

#endif
