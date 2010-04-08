/*
** Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <nm.h>
#include    <er.h>
#include    <lo.h>
#include    <pm.h>

#include    <iicommon.h>
#include    <gca.h>
#include    <gcu.h>

/*
** Name: gcugtsym.c - GCF utility(s) to get environment symbols/parms
**
** Description:
**	gcu_get_tracesym ()	- Get tracing-related symbol
**
**	Essentially will look for things in order of: 
**	environment variable -> symbol.tbl -> config.dat
** History:
**	08-Jan-93 (edg)
**	     Written.
**	11-Mar-97 (gordy)
**	    Added gcu.h for utility function declarations.
*/



/*
** Name: gcu_get_tracesym() - get a GCF trace symbol.
**	
** Description:
**	Used to get GCF symbols for name of trace log file and trace
**	log level.  Supports getting 2 different "flavors" of the
**	symbol: NMgtAt and PMget variants.  NMgtAt supports front-end and
**	more generic setting.  PMget allows the specification of a more
**	specific server instance if so desired.  The order of precedence
**	is effectively: env variable, symbol.tbl variable, config.dat var.
**
**	We assume that PMget() will behave rationally in the case 
**	PMinit() hasn't been called.
** Inputs:
**	nmsym	- host name for logging, copied in
**	pmsym	- server type for logging, copied in
** Outputs:
**	sym_value - value for the symbol.
**
** History:
**	08-Jan-93 (edg)
**	    Written.
*/

VOID
gcu_get_tracesym( char *nmsym, char *pmsym, char **sym_value )
{
    if ( nmsym && *nmsym )  NMgtAt( nmsym, sym_value );

    if ( !( *sym_value && **sym_value )
	 && pmsym && *pmsym && PMget( pmsym, sym_value ) != OK )
    {
	*sym_value = (char *)NULL;
    }

    return;
}
