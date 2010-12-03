/******************************************************************************
**
** Copyright (c) 1987, 1991 Ingres Corporation
**
******************************************************************************/

#include <compat.h>
#include <gl.h>
#include <clconfig.h>
#include <systypes.h>
#include <cs.h>
#include <pc.h>
#include <csinternal.h>
#include <er.h>
#include <si.h>
#include <st.h>
#include <cv.h>
#include <lo.h>
#include <pm.h>
// #include <rusage.h>
// #include <clnfile.h>
// #include <id.h>

#include "cslocal.h"
#include <fcntl.h>

/******************************************************************************
** csoption.c - fetch and distribute CS options 
**
** History:
**	1-Sep-93 (seiwald)
**	    Whole file (CS_build_msg and CS_parse_option) replaced with
**	    new CSoptions().  Old history tossed.
**	19-oct-93 (seiwald)
**	    Mimic old iirundbms.c code to use RLIMIT_RSS only if it is
**	    defined.  Sorry, Tom.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	12-Nov-2010 (kschendel) SIR 124685
**	    Prototype / include fixes.
**
******************************************************************************/

#define	CSO_ACT_SESSION        	  1
#define	CSO_CON_SESSION        	  2
#define	CSO_CPUSTATS           	  3
#define	CSO_DEFINE             	  4
#define	CSO_ECHO               	  5
#define	CSO_MAXLDB_CON         	  6
#define	CSO_MAX_WS             	  7
#define	CSO_PRIORITY           	  8
#define	CSO_QUANTUM            	  9
#define	CSO_SERVER_CLASS       	 10
#define	CSO_SESS_ACCTNG        	 11
#define	CSO_STK_SIZE           	 12

static struct CSO_OPTIONS {
	i4 	cso_index;	/* examined only in CSoptions() */
	char	cso_argtype;	/* meaningful only to CSoptions() */
	char	*cso_name;	/* parameter name for PMget() */
} 

cso_opttab[] = {
    CSO_ECHO,		'z',	"!.echo",		/* first! */
    CSO_ACT_SESSION,	'o',	"!.active_limit",
    CSO_CON_SESSION,	'o',	"!.connect_limit",
    CSO_CPUSTATS,	't',	"!.cpu_statistics",
    CSO_DEFINE,		't',	"!.define_address",
    CSO_MAXLDB_CON,	'o',	"!.outbound_limit",
    CSO_MAX_WS,		'm',	"!.unix_maximum_working_set",
    CSO_PRIORITY,	'p',	"!.unix_priority",
    CSO_QUANTUM,	'o',	"!.quantum_size",
    CSO_SESS_ACCTNG,	't',	"!.session_accounting",
    CSO_STK_SIZE,	'o',	"!.stack_size",
    CSO_SERVER_CLASS,	'g',	"!.server_class",
    0, 0, 0
};

/******************************************************************************
** Name: CSoptions() - fetch a pile of options using PMget()
**
** Description:
**      Calls PMget() for each option in the option table, converts the
**      value appropriately, and distributes the value into the right
**	spot in the CS_SYSTEM block.
**
**	Note special cruddiness surrounding CSO_SERVER_CLASS: the CL
**	fetches this only to set cs_isstar, used by CSinitiate() to 
**	calculate the file descriptor fluff factors: STAR uses more of 'em.
******************************************************************************/
STATUS
CSoptions( CS_SYSTEM *cssb )
{
	i4  echo = 1;
	struct CSO_OPTIONS *csopt;
	i4  failures = 0;

	for( csopt = cso_opttab; csopt->cso_name; csopt++ )
	{
	    char	*cs_svalue;
	    i4		cs_value = 0;
	    STATUS	status = OK;

	    /* 
	     * Get option's value using PMget() 
	     */

	    if( PMget( csopt->cso_name, &cs_svalue ) != OK )
		continue;

	    if( echo )
		ERoptlog( csopt->cso_name, cs_svalue );

	    /* 
	     * Convert & check the option's value, according to argtype 
	     */

	    switch( csopt->cso_argtype )
	    {
	    case 'g':
		/* String option */
		break;

	    case 'm':
		/* numeric option or the word "max"

		if( !STcompare( cs_svalue, "max" ) )
		{
		    cs_value = -1;
		    break;
		}
		/* fall through */

	    case 'o':
	    case 'n':
		/* set a numeric option */

		if( CVal( cs_svalue, &cs_value ) != OK )
		{
		    status = E_CS0025_NUMERIC_ARG;
		}
		else if( csopt->cso_argtype == 'o' && cs_value < 0 )
		{
		    status = E_CS0026_POSITIVE_VALUE;
		}
		break;

	    case 't':
		/* set a switch option: on = true */

		if( STbcompare( cs_svalue, 0, "on", 0, TRUE ) )
		    continue;
		break;
		    
	    case 'z':
		/* clear a switch option */

		if( !STbcompare( cs_svalue, 0, "on", 0, TRUE ) )
		    continue;
		break;
	    }

	    if( status )
	    {
		(*cssb->cs_elog)( status, (CL_ERR_DESC *)0, 1,
			    STlength( csopt->cso_name ), csopt->cso_name );
		failures++;
		break;
	    }

	    /*
	     * Distribute option value, according to index
	     */

	    switch( csopt->cso_index )
	    {
	    case CSO_ECHO:
		/* echo: off */
		echo = 0;
		break;

	    case CSO_ACT_SESSION:
		cssb->cs_max_active = cs_value;
		break;

	    case CSO_CON_SESSION:
		cssb->cs_max_sessions = cs_value;
		break;

	    case CSO_MAXLDB_CON:
//		cssb->cs_max_ldb_connections = cs_value;
		break;

	    case CSO_MAX_WS:

# if defined( xCL_034_GETRLIMIT_EXISTS) && defined( RLIMIT_RSS )
		{
		    struct rlimit rlim;
		    getrlimit( RLIMIT_RSS, &rlim );

		    rlim.rlim_cur = cs_value;

		    if( cs_value < 0 || cs_value > rlim.rlim_max )
			rlim.rlim_cur = rlim.rlim_max;

		    setrlimit( RLIMIT_RSS, &rlim );
		}
#endif
		break;

	    case CSO_PRIORITY:
		/* Process priority */

# ifdef xCL_033_SETPRIORITY_EXISTS
//		setpriority( PRIO_PROCESS, 0, cs_value );
# else
//		nice( cs_value );
# endif
		break;

	    case CSO_QUANTUM:
//		cssb->cs_quantum = cs_value;
		break;

	    case CSO_SESS_ACCTNG:
		cssb->cs_mask |= CS_ACCNTING_MASK;
		cssb->cs_mask |= CS_CPUSTAT_MASK;
		break;
		
	    case CSO_CPUSTATS:
		cssb->cs_mask |= CS_CPUSTAT_MASK;
		break;

	    case CSO_STK_SIZE:
		cssb->cs_stksize = cs_value;
		break;

	    case CSO_SERVER_CLASS:
//		cssb->cs_isstar = !STcompare( cs_svalue, "STAR" ); /* cheesy */
		break;

	    case CSO_DEFINE:
		cssb->cs_define = TRUE;
		break;
	    }
	}
    return (OK);
}
