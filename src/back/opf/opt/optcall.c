/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
/* beginning of optimizer header files */
#include    <opglobal.h>
#include    <opdstrib.h>
#include    <opfcb.h>
#include    <opgcb.h>
#include    <opscb.h>
#include    <ophisto.h>
#include    <opboolfact.h>
#include    <oplouter.h>
#include    <opeqclass.h>
#include    <opcotree.h>
#include    <opvariable.h>
#include    <opattr.h>
#include    <openum.h>
#include    <opagg.h>
#include    <opmflat.h>
#include    <opcsubqry.h>
#include    <opsubquery.h>
#include    <opcstate.h>
#include    <opstate.h>

/* external routine declarations definitions */
#define        OPT_CALL      TRUE
#include    <opxlint.h>


/**
**
**  Name: OPTCALL.C - SET A DEBUG TRACE FLAG
**
**  Description:
**      This file will contain all the procedures necessary to support the
**      opt_call entry point to the optimizer facility.
**
**  History:    
**      31-dec-85 (seputis)    
**          initial creation
**	18-dec-90 (kwatts)
**	    Modified optcall().
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      21-mar-95 (inkdo01)
**          Added support for op170 (parse tree dump)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name: opt_call - Call an Optimizer trace operation
**
** External call format:
**      status = (DB_STATUS) opt_call((DB_DEBUG_CB *) dbt_cb);
**
** Description:
**      This is the trace point entry to the Optimizer facility.
**      The control block is a special tracing control block type as
**      specified in the implementation conventions.  There is no opcode
**      since the operation is completely specified by the control block.
** Inputs:
**      debug_cb                             Pointer to the DB_DEBUG_CB
**                                           containing the parameters,
**        .db_trswitch                       What operation to perform
**	    DB_TR_NOCHANGE		       None
**	    DB_TR_ON			       Turn on a tracepoint
**	    DB_TR_OFF			       Turn off a tracepoint
**	  .db_trace_point		     The number of the tracepoint to be
**					     effected
**	  .db_vals[2]			     Optional values, to be interpreted
**					     differently for each tracepoint
**	  .db_value_count		     The number of values specified in
**					     the above array
**
**
** Outputs:
**      Returns:
**          E_DB_OK                          Function completed normally
**          E_DB_ERROR                       Function failed - error by caller
**                                           no error code available but
**                                           status returned if 
**                                           a) trace flag is not recognized
**                                           b) if value is required but not
**                                              supplied
**                                           c) if value is supplied but not
**                                              required
**                                           d) invalid parameter in debug
**                                              control block
**          E_DB_FATAL                       Function failed - internal error
**                                           - internal exception handler
**                                           was unexpectedly invoked
**                                           
**                                              
**      Exceptions:
** 
** Side Effects:
**      There may be session state information updated in the OPF session 
**      control block associated with this session.  This information will
**      persist between calls to the OPF.  If certain tracing features
**      are selected then some information will be directed towards specified
**      trace files or devices.  Global variables associated with the server
**      may be set prior to initialization of the server and will persist
**      subsequent to the shutdown.
** History:
**      31-dec-85 (seputis)
**          initial creation
**	27-dec-90 (seputis)
**	    add f050 trace flag
**	03-jan-90 (kwatts)
**	    Modified code to support OPT_F056_NOSMARTDISK,
**	    OPT_F057_WHYNOSMARTDISK, OPT_F058_SD_CONJUNCT_TRACE
**	    trace points.
**	18-mar-91 (seputis)
**	    OPT_F059_CARTPROD trace point added
**	29-mar-91 (seputis)
**	    OPT_F060_QEP trace point added
**	    OPT_F061_CORRELATION trace point added
**	    OPT_F063_CORRELATION trace point added
**	    OPT_F064_NOKEY trace point added
**      5-dec-91 (rickh)
**          Changed opt_f054_allplans to opt_f069_allplans as part of the
**          merge of the outer join path with 6.4.
**	14-may-92 (rickh)
**	    Added OP199.
**      11-oct-91 (seputis)
**          OPT_F065_8CHAR trace point added
**      11-nov-91 (seputis)
**          OPT_F066_GUESS trace point added
**      21-may-92 (seputis)
**          xDEBUG trace point op168 since it does not work in all cases
**      5-feb-93 (ed)
**          add trace point op201 which modifies default histogram processing
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	13-may-94 (ed)
**	    added OP130 to skip non-critical sections of code
**	19-may-97 (inkdo01)
**	    Activate F054, F055 as CNF transformation trace points, and F075
**	    to force more accuracy in QEP displays
**	29-jun-99 (hayke02)
**	    Activate F077 and F078 (trace points op205 and op206) for
**	    limiting tuple estimates for joins between table with stats
**	    and table without stats (dbms parameter opf_stats_nostats_max).
**	    This change fixes bug 97450.
**	1-feb-00 (inkdo01)
**	    Enable tp op207 to dump more QEP detail.
**	10-sep-01 (inkdo01)
**	    Redefine op167 as the "no key joins" tp.
**	18-oct-02 (inkdo01)
**	    Enable 153 to dump range table (not whole op156), 208 to enable 
**	    new enumeration and 209 to dump stuff for new enumeration.
**	26-jul-04 (hayke02)
**	    Activate F083 (trace point op211) for old reverse var ordering
**	    when adding useful secondary indices. This change fixes problem
**	    INGSRV 2919, bug 112737.
**	6-Aug-2007 (kschendel) SIR 122513
**	    Steal unused op131 for disabling join-time part qual.
**	    (note, op135 seems to be the next reuse candidate.)
**	1-feb-2008 (dougi)
**	    Redefine meaning of op248 (Kjoin cost factoring) and define
**	    op148 (turns off T/Kjoin presort) and op144 (write OPF session
**	    stats to DBMS log).
**	30-oct-08 (hayke02)
**	    Add OPT_F118_GREEDY_FACTOR for opf_greedy_factor. This change
**	    fixes bug 121159.
**	02-Dec-08 (wanfr01)
**	    SIR 121316
**	    Add OPT_F084_TBLAUTOSTRUCT to allowed trace points.
**	 5-dec-08 (hayke02)
**	    Add op213. This change addresses SIR 121322.
**	27-Oct-09 (smeke01) b122794
**	    Add OP147 (previously it could be switched on only by switching on  
**	    another OP tracepoint at the same time).
**	20-Nov-2009 (kschendel) SIR 122890
**	    Rename OP129.
**	22-mar-10 (smeke01) b123457
**	    Add op214 (OPT_F086_DUMP_QTREE2).
*/

DB_STATUS
opt_call(
	DB_DEBUG_CB        *debug_cb)
{
    i4             flag;		    /* Trace flag to be affected */
    i4		firstval;           /* first value of trace control
                                            ** block */
    i4		secondval;          /* second value of trace control
                                            ** block */
    OPS_CB              *ops_cb;	    /* ptr to session control block */

    /* FIXME - need to establish an exception handler here */

    ops_cb = ops_getcb();		    /* get session control block from
                                            ** SCF */
    /* Flags 0 - OPT_GMAX-1 are for the server; all others are for sessions */
    flag = debug_cb->db_trace_point;
    if (flag < 0)
	return (E_DB_ERROR);

    /* There can be UP TO two values, but maybe they weren't given */
    if (debug_cb->db_value_count > 0)
	firstval = debug_cb->db_vals[0];
    else
	firstval = 0L;

    if (debug_cb->db_value_count > 1)
	secondval = debug_cb->db_vals[1];
    else
	secondval = 0L;

    if (flag < OPT_GMAX)
    {	/* a server trace flag is found */
	switch (flag)
	{   /* check that a valid global trace flag is being affected */

	case OPT_F038_FAKE_GATEWAY:
	{
	    /* set some flags to change OPF to gateway */
 	    ops_cb->ops_server->opg_qeftype = OPF_GQEF;	/* make this a gateway QEF */
 	    ops_cb->ops_server->opg_smask = OPF_TIDDUPS | OPF_TIDUPDATES | OPF_INDEXSUB;
 	}
	case OPT_GSTARTUP:
        case OPT_F004_FLATTEN:
        case OPT_F005_PARSER:
	    break;
	default:
	    return( E_DB_ERROR );	    /* return if not a valid flag */
	}
	/*
	** Three possible actions: Turn on flag, turn it off, or do nothing.
	*/
	switch (debug_cb->db_trswitch)
	{
	case DB_TR_ON:
	    ult_set_macro(&ops_cb->ops_server->opg_tt, flag, firstval, secondval);
	    ops_cb->ops_server->opg_check = TRUE; /* indicates that at least one
                                            ** trace flag has been set
                                            ** - this probably could be made
                                            ** into a counter */
	    break;
    
	case DB_TR_OFF:
	    ult_clear_macro(&ops_cb->ops_server->opg_tt, flag);
	    break;
    
	case DB_TR_NOCHANGE:
	    break;
    
	default:
	    return (E_DB_ERROR);
	}
    }
    else
    {   /* a session level trace flag will be set */
	flag = flag - OPT_GMAX;		    /* get proper offset for session
                                            ** trace buffer */
	switch (flag)
	{
	case OPT_F002_SKIPCONSISTENCY:
	{
	    switch (debug_cb->db_trswitch)
	    {
	    case DB_TR_ON:
		ops_cb->ops_skipcheck = TRUE;	/* indicates non critical consistency
						** checks should be skipped */
		break;
	    case DB_TR_OFF:
		ops_cb->ops_skipcheck = FALSE;	/* resume consistency checks */
		break;
	    case DB_TR_NOCHANGE:
		break;
	    default:
		return (E_DB_ERROR);
	    }
	    return(E_DB_OK);
	}
        case OPT_F001_DUMP_QTREE:
        case OPT_T129_NO_LOAD:
        case OPT_T131_NO_JOINPQUAL:
        case OPT_F004_FLATTEN:
        case OPT_F005_PARSER:
        case OPT_F006_NONOTEX:
        case OPT_AGSNAME:
        case OPT_AGSIMPLE:
        case OPT_AGSUB:
        case OPT_MAKAVAR:
        case OPT_AGCANDIDATE:
        case OPT_AGREPLACE:
        case OPT_PROJECTION:
        case OPT_TIMER:
        case OPT_F016_OPFSTATS:
        case OPT_FCOTREE:
	case OPT_F019_AGGCOMPAT:
	case OPT_F020_NOPRESORT:
	case OPT_F021_QPTREE:
	case OPT_F022_FULL_QP:
	case OPT_F023_NOCOMP:
	case OPT_F025_RANGETAB:
	case OPT_F026_FULL_OPKEY:
	case OPT_F027_FUSED_OPKEY:
	case OPT_F028_JOINOP:
	case OPT_F029_HISTMAP:
	case OPT_F030_SQLNAMES:
	case OPT_F031_WARNINGS:
	case OPT_F032_NOEXECUTE:
	case OPT_F033_OPF_TO_OPC:
	case OPT_F034_NEWFEATS:
	case OPT_F035_NO_HISTJOIN:
	case OPT_F036_OPC_DISTRIBUTED:
	case OPT_F037_STATISTICS:
	case OPT_F039_NO_KEYJOINS:
#ifdef xDEBUG
        case OPT_F040_SQL_TEXT:
#endif
	case OPT_F041_MULTI_SITE:
	case OPT_F042_PTREEDUMP:
	case OPT_F043_CONSTANTS:
	case OPT_F044_NO_RTREE:
	case OPT_F045_QENPROW:
	case OPT_F046_CONTROLC:
	case OPT_F048_FLOAT:
	case OPT_F049_KEYING:
	case OPT_F050_COMPLETE:
	case OPT_F051_QPDUMP:  
	case OPT_F052_CARTPROD:
	case OPT_F053_FASTOPT:
	case OPT_F054_OLDCNF:
	case OPT_F055_NOCNF:
	case OPT_F056_NOSMARTDISK:
	case OPT_F057_WHYNOSMARTDISK:
	case OPT_F058_SD_CONJUNCT_TRACE:
	case OPT_F059_CARTPROD:
	case OPT_F060_QEP:
	case OPT_F061_CORRELATION:
	case OPT_F062_OLDJCARD:
	case OPT_F063_CORRELATION:
	case OPT_F064_NOKEY:
        case OPT_F065_8CHAR:
        case OPT_F066_GUESS:
        case OPT_F067_PQ:
	case OPT_F068_OLDFLATTEN:
        case OPT_F069_ALLPLANS:
        case OPT_F070_RIGHT:
	case OPT_F071_QEP_WITHOUT_COST:
        case OPT_F073_NODEFAULT:
	case OPT_F074_UVAGG:
	case OPT_F075_FLOATQEP:
	case 76: /* inkdo01 kludge */
	case OPT_F077_STATS_NOSTATS_MAX:
	case OPT_F078_NO_STATS_NOSTATS_MAX:
	case OPT_F081_DUMPENUM:
	case OPT_F082_QEPDUMP:
	case OPT_F083_OLDIDXORDER:
	case OPT_F084_TBLAUTOSTRUCT:
	case OPT_F085_MISSINGSTATS:
	case OPT_F086_DUMP_QTREE2:
            if (debug_cb->db_value_count > 0)
		return( E_DB_ERROR );	    /* these flags have no values */
	    break;			    /* Flag is defined */
	case OPT_F119_NEWENUM:
            if (debug_cb->db_value_count > 1)
		return( E_DB_ERROR );	    /* this flag may have 0 or 1 value */
	    break;			    /* Flag is defined */
	case OPT_F120_SORTCPU:
            if (debug_cb->db_value_count > 1)
		return( E_DB_ERROR );	    /* this flag may have 0 or 1 value */
	    break;			    /* Flag is defined */
	case OPT_F118_GREEDY_FACTOR:
	case OPT_F123_SCAN:
	case OPT_F124_BLOCKING:
	case OPT_F125_TIMEFACTOR:
	case OPT_F126_SITE_FACTOR:
	case OPT_F127_TIMEOUT:
	    if ((debug_cb->db_value_count < 1)
		&&
		(debug_cb->db_trswitch == DB_TR_ON) /* value only required
					    ** if trace flag is to be
					    ** turned on */
		)
		return (E_DB_ERROR);	    /* at least one value must be
					    ** specified */
	    break;
        case OPT_F024_NOTIMEOUT:
            if (debug_cb->db_value_count > 0)
                return( E_DB_ERROR );
            ops_cb->ops_alter.ops_timeout = FALSE; /* alternative way of 
                                            ** changing no timeout flag for 
                                            ** session */
	    return (E_DB_OK);
	case OPT_F072_LVCH_TO_VCH:
            if (debug_cb->db_value_count > 0)
		return( E_DB_ERROR );	    /* these flags have no values */
	    /*
	    ** This trace point instructs OPF\OPC to use set the tuple
	    ** descriptor for the query to state that long varchar elements
	    ** are varchar.  This is useful for testing the system before there
	    ** is large object support in the frontends.
	    **
	    ** To be useful, this action must be accompanied by an action which
	    ** tells the adu_redeem() code to format the beasts as varchar.
	    ** This is accomplished by setting adf trace point 11.
	    **
	    ** Note that this is server wide setting...
	    */

	    if (debug_cb->db_trswitch != DB_TR_NOCHANGE)
	    {
		DB_STATUS   status;
		DB_DEBUG_CB adf_dbg;

		STRUCT_ASSIGN_MACRO(*debug_cb, adf_dbg);

		/* {@fix_me@}  Need a #define for this... */
		
		adf_dbg.db_trace_point = 11;
		
		status = adb_trace(&adf_dbg);
		if (DB_FAILURE_MACRO(status))
		    return(status);
	    }

	    break;
	default:
	    return( E_DB_ERROR );	    /* unknown session trace flag */
	}
	/*
	** Three possible actions: Turn on flag, turn it off, or do nothing.
	*/
	switch (debug_cb->db_trswitch)
	{
	case DB_TR_ON:
	    ult_set_macro(&ops_cb->ops_tt, flag, firstval, secondval);
	    ops_cb->ops_check = TRUE;	    /* indicates that at least one
                                            ** trace flag has been set
                                            ** - this probably could be made
                                            ** into a counter */
	    break;
    
	case DB_TR_OFF:
	    ult_clear_macro(&ops_cb->ops_tt, flag);
	    break;
    
	case DB_TR_NOCHANGE:
	    break;
    
	default:
	    return (E_DB_ERROR);
	}
    }

    return (E_DB_OK);
}
