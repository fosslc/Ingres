/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** History:
**      07-Sep-1998 (fanra01)
**          Corrected case of actsession to match piccolo.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
#ifndef WSM_VAR_INCLUDED
#define WSM_VAR_INCLUDED

#include <actsession.h>

typedef struct __WSM_SCAN_VAR 
{
	ACT_PSESSION		session;
	u_i4				level;
	u_i4				current_level;
	DDFHASHSCAN			scanner;
} WSM_SCAN_VAR, *WSM_PSCAN_VAR;

GSTATUS 
WSMAddVariable (	
	ACT_PSESSION act_session, 
	char *name, 
	u_i4 nlen, 
	char *value, 
	u_i4 vlen, 
	u_i4 level);

GSTATUS 
WSMGetVariable (	
	ACT_PSESSION act_session, 
	char *name, 
	u_i4 nlen, 
	char **value, 
	u_i4 level);

GSTATUS 
WSMDelVariable (	
	ACT_PSESSION act_session, 
	char *name, 
	u_i4 nlen, 
	u_i4 level );

GSTATUS 
WSMOpenVariable (	
	WSM_PSCAN_VAR scanner,
	ACT_PSESSION	act_session,
	u_i4 level );

GSTATUS 
WSMScanVariable (	
	WSM_PSCAN_VAR scanner,
	char **name,
	char **value,
	u_i4 *level );

GSTATUS 
WSMCloseVariable (	
	WSM_PSCAN_VAR scanner);

#endif
