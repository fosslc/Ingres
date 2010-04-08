/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** History:
**      10-Sep-1998 (fanra01)
**          Corrected incomplete line.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
#ifndef WSF_VAR_INCLUDED
#define WSF_VAR_INCLUDED

#include <ddginit.h>
#include <ddfhash.h>
#include <wsf.h>

typedef struct __WSF_VAR 
{
	char				*name;
	char				*value;
} WSF_VAR, *WSF_PVAR;

GSTATUS 
WSFVARInitialize ();

GSTATUS 
WSFVARTerminate ();

GSTATUS 
WSFDestroyVariable (
	WSF_PVAR *var);

GSTATUS 
WSFCreateVariable (
	u_i2 tag,
	char *name, 
	u_i4 nlen, 
	char *value, 
	u_i4 vlen,
	WSF_PVAR *var);

GSTATUS 
WSFAddVariable (	
	char *name, 
	u_i4 nlen, 
	char *value, 
	u_i4 vlen);

GSTATUS 
WSFGetVariable (	
	char *name, 
	u_i4 nlen, 
	char **value);

GSTATUS 
WSFDelVariable (	
	char *name, 
	u_i4 nlen);

GSTATUS
WSFCleanVariable (	
	DDFHASHTABLE vars);

#endif
