/*
** Copyright (c) 1987, 2003 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <er.h>
#include    <ex.h>
#include    <lo.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>
#include    <sp.h>
#include    <mo.h>
#include    <cv.h>

#include    <exhdef.h>

#include    <csinternal.h>

#include    <pm.h>

/*
** csoption.c - fetch and distribute CS options 
**
** History:
**	1-Sep-93 (seiwald)
**	    Written.
**
**	18-OCT-93 (seiwald)
**	    Whole file (CS_build_msg and CS_parse_option) replaced with
**	    new CSoptions().  Old history tossed.
**	19-Oct-93 (seiwald)
**	    No more csdbms.h.
**	20-oct-93 (lauraw)
**	    Define CSO_ECHO, as per seiwald.
**	22-nov-1993 (bryanp) B56813.
**	    For CSO_QUANTUM, use cs_svalue, not cs_value.
**	31-jan-1994 (bryanp) B56917, B56918, B56921
**	    CSoptions now returns STATUS, not VOID
**	    Added check for PM name with null value, i.e., string of length 0.
**	    Added explicit check for PM_NOT_FOUND as the only "ok" return code
**		from PMget. Other return codes are treated as errors.
**	    Added include of <pm.h>, and of <lo.h>, on which <pm.h> depends.
**      06-feb-98 (kinte01)
**          Cross integrate change from VAX VMS CL to Alpha CL
**          17-jan-1996 (nick)
**            quantum_size should be vms_quantum.
**	21-jan-1999 (canor01)
**	    Remove erglf.h.
**	19-jul-2000 (kinte01)
**	    Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**		Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**		from VMS CL as the use is no longer allowed
**	28-aug-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
*/

# define CSO_ACT_SESSION	1
# define CSO_CON_SESSION	2
# define CSO_CPUSTATS		3
# define CSO_DEFINE		4
# define CSO_GOVERNED		5
# define CSO_GPRIORITY		6
# define CSO_MAXLDB_CON		7
# define CSO_QUANTUM		8
# define CSO_SESS_ACCTNG	9
# define CSO_STK_SIZE		10
# define CSO_ECHO		11

static struct CSO_OPTIONS {
	i4	cso_index;	/* examined only in CSoptions() */
	i4	cso_argtype;	/* meaningful only to CSoptions() */
	char	*cso_name;	/* parameter name for PMget() */
} 
cso_opttab[] = 
{
    CSO_ECHO,		'z',	"!.echo",		/* first! */

    CSO_ACT_SESSION,	'o',	"!.active_limit",
    CSO_CON_SESSION,	'o',	"!.connect_limit",
    CSO_CPUSTATS,	't',	"!.cpu_statistics",
    CSO_DEFINE,		't',	"!.define_address",
    CSO_GOVERNED,	'o',	"!.cpu.usage",
    CSO_GPRIORITY,	'o',	"!.cpu.repent_at",
    CSO_MAXLDB_CON,	'o',	"!.outbound_limit",
    CSO_QUANTUM,	'g',	"!.vms_quantum",
    CSO_SESS_ACCTNG,	't',	"!.session_accounting",
    CSO_STK_SIZE,	'o',	"!.stack_size",
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
** Inputs:
**	cssb			    Address of the CS_SYSTEM block
**
** Outputs:
**	cssb			    The CS_SYSTEM block is filled in.
**
** Returns:
**	STATUS
**
** History:
**	22-nov-1993 (bryanp) B56813.
**	    For CSO_QUANTUM, use cs_svalue, not cs_value.
**	31-jan-1994 (bryanp) B56917, B56918, B56921
**	    CSoptions now returns STATUS, not VOID
**	    Added check for PM name with null value, i.e., string of length 0.
**	    Added explicit check for PM_NOT_FOUND as the only "ok" return code
**		from PMget. Other return codes are treated as errors.
**      06-feb-98 (kinte01)
**          Cross integrate change from VAX VMS CL to Alpha CL
**          17-jan-1996 (nick)
**             Don't choke on old, incorrect values for quantum which may still
**             be out there.
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
	    ** routine treat a string of length zero as identical in meaning
	    ** to PM_NOT_FOUND.
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
		    /* status = E_CL2525_CS_NUMERIC_ARG; */
		    status = E_CS0025_NUMERIC_ARG;
		}
		else if( csopt->cso_argtype == 'o' && cs_value < 0 )
		{
		    /* status = E_CL2526_CS_POSITIVE_VALUE; */
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

	    case CSO_QUANTUM:
		cssb->cs_aq_length = STlength( cs_svalue );
                /*
                ** Previous releases shipped with this ( incorrectly ) as
                ** a boolean ( which was ignored ).  Make sure we still
                ** ignore it else the server will barf on the value.
                */
                if ((STbcompare(cs_svalue, cssb->cs_aq_length, ERx("ON"), 2,
                        TRUE) != 0) &&
                    (STbcompare(cs_svalue, cssb->cs_aq_length, ERx("OFF"), 3,
                        TRUE) != 0))
                        MEcopy( cs_svalue, cssb->cs_aq_length,
                            cssb->cs_aquantum );
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

            case CSO_GOVERNED:
                cssb->cs_gpercent = cs_value;
                break;

            case CSO_GPRIORITY:
                cssb->cs_gpriority = cs_value;
                break;

	    case CSO_DEFINE:
		cssb->cs_define = TRUE;
		break;
	    }
	}

    return (status);
}
