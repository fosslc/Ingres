/*
**Copyright (c) 2004 Ingres Corporation
*/
#include <compat.h>
#include <gl.h>
#include <clconfig.h>
#include <systypes.h>
#include <cs.h>
#include <pc.h>
#include <csinternal.h>
#include <er.h>
#include <lo.h>
#include <si.h>
#include <st.h>
#include <cv.h>
#include <rusage.h>
#include <clnfile.h>
#include <id.h>
#include <pm.h>

#include "cslocal.h"

#ifdef      xCL_006_FCNTL_H_EXISTS
#include    <fcntl.h>
#endif      /* xCL_006_FCNTL_H_EXISTS */
#ifdef      xCL_007_FILE_H_EXISTS
#include    <sys/file.h>
#endif	    /* xCL_007_FILE_H_EXISTS */

/*
** csoption.c - fetch and distribute CS options 
**
** History:
**	1-Sep-93 (seiwald)
**	    Whole file (CS_build_msg and CS_parse_option) replaced with
**	    new CSoptions().  Old history tossed.
**	19-oct-93 (seiwald)
**	    Mimic old iirundbms.c code to use RLIMIT_RSS only if it is
**	    defined.  Sorry, Tom.
**      31-jan-94 (mikem)
**          sir #57671
**	    Added to unix-cl specific server configuration options to control
**	    the transfer size of di-slave non-shared memory I/O: size_io_buf,
**	    num_io_buf.
**	31-jan-1994 (bryanp) B56917, B56918, B56921
**          CSoptions now returns STATUS, not VOID
**          Added check for PM name with null value, i.e., string of length 0.
**          Added explicit check for PM_NOT_FOUND as the only "ok"return code
**		from PMget. Other return codes are treated as errors.
**	    Added include of <pm.h>, and of <lo.h>, on which <pm.h> depends.
**	01-feb-1994 (lauraw)
**	    REmoved redundant includes of lo.h and pm.h.
**	29-oct-1997 (canor01)
**	    Add stack caching for os-threaded servers.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
[@history_template@]...
*/

# define	CSO_ACT_SESSION        	  1
# define	CSO_CON_SESSION        	  2
# define	CSO_CPUSTATS           	  3
# define	CSO_DEFINE             	  4
# define	CSO_ECHO               	  5
# define	CSO_MAXLDB_CON         	  6
# define	CSO_MAX_WS             	  7
# define	CSO_PRIORITY           	  8
# define	CSO_QUANTUM            	  9
# define	CSO_SERVER_CLASS       	 10
# define	CSO_SESS_ACCTNG        	 11
# define	CSO_STK_SIZE           	 12
# define	CSO_SIZE_IO_BUF        	 13
# define	CSO_NUM_IO_BUF         	 14
# define	CSO_STK_CACHE         	 15

static struct CSO_OPTIONS {
	i4	cso_index;	/* examined only in CSoptions() */
	char	cso_argtype;	/* meaningful only to CSoptions() */
	char	*cso_name;	/* parameter name for PMget() */
} 
cso_opttab[] = 
{
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
    CSO_STK_CACHE,	't',	"!.stack_caching",
    CSO_SERVER_CLASS,	'g',	"!.server_class",
    CSO_SIZE_IO_BUF,	'o',	"!.size_io_buf",
    CSO_NUM_IO_BUF,	'o',	"!.num_io_buf",
    0, 0, 0
};

/*
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
**
** Inputs:
**	cssb				Address of the CS_SYSTEM block.
**
** Outputs:
**	cssb				The CS_SYSTEM block is filled in.
**
** Returns:
**	STATUS
**
** History:
**	31-jan-1994 (bryanp) B56917, B56918, B56921
**          CSoptions now returns STATUS, not VOID
**          Added check for PM name with null value, i.e., string of length 0.
**          Added explicit check for PM_NOT_FOUND as the only "ok"return code
**		from PMget. Other return codes are treated as errors.
**	15-Sep-2005 (schka24)
**	    Round stack size to an 8K alignment, and add an extra 8K for
**	    guard pages or whatever.  Mostly because it feels good.
*/

STATUS
CSoptions( CS_SYSTEM *cssb )
{
	i4 echo = 1;
	struct CSO_OPTIONS *csopt;
	i4 failures = 0;
	STATUS	status = OK;

	for( csopt = cso_opttab;
	     status == OK && csopt->cso_name != 0;
	     csopt++ )
	{
	    char	*cs_svalue;
	    f8		cs_dvalue;
	    i4		cs_value = 0;

	    /* 
	     * Get option's value using PMget() 
	     */

	    status = PMget( csopt->cso_name, &cs_svalue );
	    if (status == PM_NOT_FOUND)
	    {
		status = OK;
		continue;
	    }
	    else if (status != OK)
		break;

	    /*
	    ** B56918: Strings of length 0. If the value for a particular
	    ** PM name is missing, PMget returns with a string of length 0.
	    ** Currently, all the PM parameters which are handled by this
	    ** routine treat a string of length 0 as identical in meaning to
	    ** PM_NOT_FOUND.
	    */
	    if (cs_svalue[0] == EOS)
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
		/* numeric option or the word "max" */

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

		if( STcasecmp( cs_svalue, "on"  ) )
		    continue;
		break;
		    
	    case 'z':
		/* clear a switch option */

		if( !STcasecmp( cs_svalue, "on" ) )
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
		cssb->cs_max_ldb_connections = cs_value;
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
		setpriority( PRIO_PROCESS, 0, cs_value );
# else
		nice( cs_value );
# endif
		break;

	    case CSO_QUANTUM:
		cssb->cs_quantum = cs_value;
		break;

	    case CSO_SESS_ACCTNG:
		cssb->cs_mask |= CS_ACCNTING_MASK;
		cssb->cs_mask |= CS_CPUSTAT_MASK;
		break;
		
	    case CSO_CPUSTATS:
		cssb->cs_mask |= CS_CPUSTAT_MASK;
		break;

	    case CSO_STK_SIZE:
		/* Add some extra for thread-local storage and/or guard
		** pages, and round to a nice looking boundary.  This is
		** not a critical operation and so we're not using any fancy
		** page definition, just go with 8K.
		*/
		cs_value = (cs_value + 8192 + 8191) & (~8191);
		cssb->cs_stksize = cs_value;
		break;

	    case CSO_STK_CACHE:
		cssb->cs_stkcache = TRUE;
		break;

	    case CSO_SERVER_CLASS:
		cssb->cs_isstar = !STcompare( cs_svalue, "STAR" ); /* cheesy */
		break;

	    case CSO_DEFINE:
		cssb->cs_define = TRUE;
		break;

	    case CSO_SIZE_IO_BUF:
		cssb->cs_size_io_buf = cs_value;
		break;

	    case CSO_NUM_IO_BUF:
		cssb->cs_num_io_buf = cs_value;
		break;
	    }
	}
    return (status);
}
