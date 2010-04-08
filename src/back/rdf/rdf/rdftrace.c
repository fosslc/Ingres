/*
**Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include	<sl.h>
#include	<iicommon.h>
#include	<dbdbms.h>
#include	<ddb.h>
#include	<ulf.h>
#include	<ulm.h>
#include	<cs.h>
#include	<tr.h>
#include	<scf.h>
#include	<ulh.h>
#include	<adf.h>
#include	<dmf.h>
#include	<dmtcb.h>
#include	<dmrcb.h>
#include	<qefmain.h>
#include        <qsf.h>
#include	<qefrcb.h>
#include	<qefqeu.h>
#include	<rdf.h>
#include	<rqf.h>
#include	<rdfddb.h>
#include	<ex.h>
#include	<rdfint.h>
#include	<rdftrace.h>

/* forward references */
static	i4	    trace_type(i4  trace_id);  /* sess or server trace? */
#define			SVR_WIDE_TRACE	    1
#define			SESS_ONLY_TRACE	    2

/**
**
**  Name: RDFTRACE.C - Call RDF trace operation.
**
**  Description:
**	This file contains the trace entry point to RDF.
**
**	rdf_trace - Call RDF trace operation.
**
**
**  History:
**      15-apr-86 (ac)
**          written
**	02-mar-87 (puree)
**	    replace global server control block with global pointer.
**	14-dec-1989 (teg)
**	    modify to go get svcb from SCF instead of using a global pointer
**	    to it.
**	20-dec-91 (teresa)
**	    added include of header files ddb.h, rqf.h and rdfddb.h for sybil
**	08-dec-92 (anitap)
**	    added include of header file qsf.h for CREATE SCHEMA.
**	30-jun-93 (shailaja)
**	    Fixed compiler warnings
**	06-jul-93 (teresa)
**	    change include of dbms.h to include of gl.h, sl.h, iicommon.h &
**	    dbdbms.h.  Also added include of tr.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    RDI_SVCB *svcb is now a GLOBALREF, added macros 
**	    GET_RDF_SESSCB macro to extract *RDF_SESSCB directly
**	    from SCF's SCB, added (ADF_CB*)rds_adf_cb,
**	    deprecating rdu_gcb(), rdu_adfcb(), rdu_gsvcb()
**	    functions.
**/

/*{
** Name: rdf_trace - Call RDF trace operation.
**
**	External call format:	status = rdf_trace(&db_debug_cb)
**
** Description:
**      This function is the standard entry point to RDF for setting and
**	clearing tracepoints(the "set trace point" command). Because RDF 
**	is a service facility, trace point for RDF can only be set on 
**	the server basis. There is no session level trace point. 
**	Db_debug_cb is the tracing control block that contains the trace 
**	flag information.
**
**	See file <rdftrace.h> for a description of all possible
**	RDF trace points.
**	
** Inputs:
**      debug_cb		    Pointer to a DB_DEBUG_CB
**        .db_trswitch              What operation to perform
**	    DB_TR_NOCHANGE	    None
**	    DB_TR_ON		    Turn on a tracepoint
**	    DB_TR_OFF		    Turn off a tracepoint
**	  .db_trace_point	    Trace point ID(the flag number)
**	  .db_value_count	    The number of values specified in
**				    the value array
**	  .db_vals[2]		    Optional values, to be interpreted
**				    differently for each tracepoint
** Outputs:
**	none
**
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_ERROR			Function failed due to error by caller;
**	    E_DB_FATAL			Function failed due to some internal
**					problem; 
**	Exceptions:
**		none
**
** Side Effects:
**	The trace vector in the server control block of RDF will be updated
**	to contain the trace information. The trace information will be persist
**	throughout the life of the server.
**
** History:
**	15-apr-86 (ac)
**          written
**	02-mar-87 (puree)
**	    replace global server control block with global pointer.
**	14-dec-1989 (teg)
**	    modify to go get svcb from SCF instead of using a global pointer
**	    to it.
**	16-jul-92 (teresa)
**	    prototypes
**	16-sep-92 (teresa)
**	    implement trace points rd3 and rd4 to clear ldbdesc cache.
**	22-apr-94 (teresa)
**	    added trace points rd11 and rd12 to dump memory statistics or all
**	    statistics.  This is an action trace point -- the dump occurs when 
**	    the trace point is selected rather than during query execution.
**	20-nov-2007 (thaju02)
**	    If trace point RD0022/RDU_CHECKSUM specified, invalidate 
**	    relcache. Entries need rdr_checksum to be calc/init'd 
**	    otherwise E_RD010D errors may be reported. (B119499)
*/
DB_STATUS
rdf_trace(DB_DEBUG_CB *debug_cb)
{
    i4		flag;
    i4		firstval;
    i4		secondval;
    DB_STATUS		status;
    i4		trace_scope;

    /* assure flag is legal */
    flag = debug_cb->db_trace_point;
    if (flag >= RDF_NB)
    {
	return (E_DB_ERROR);
    }

    /* There can be UP TO two values, but maybe they weren't given */
    firstval = (debug_cb->db_value_count > 0) ? debug_cb->db_vals[0] : 0L;
    secondval = (debug_cb->db_value_count > 1) ? debug_cb->db_vals[1] : 0L;

    /* see if this is an action trace.  Action traces require an immediate
    ** action rather than turning trace flags on/off/etc.
    */
    if ( (debug_cb->db_trswitch==DB_TR_ON) && (flag <= RD_ACT_MAX) )
    {
	/* see which action is requested.  Not all actions are implemented
	** yet, so its possible that this call may  become a no-opt 
	*/
	switch (flag)
	{
	case RD0001:
	case RD0002:
	case RD0003:
	case RD0004:
	case RD0005:
	case RD0010:
	    status=rdt_clear_cache(flag);
	    if (DB_FAILURE_MACRO(status))
	    {
		return(E_DB_ERROR);
	    }
	    break;
	case RD0011:
	    /* dump memory info.  This trace is used by tech support when
	    ** debugging out of memory errors. */
	    TRdisplay("\n...............................................\n");
	    TRdisplay("RDF Cache Memory Available:   %d\n",Rdi_svcb->rdv_memleft);
	    TRdisplay("RDF memory cache size     :   %d\n", 
			Rdi_svcb->rdv_pool_size);
	    TRdisplay("Max number of objects allowed on Cache:\n");
	    TRdisplay("   RELcache: %d,	    QTREE Cache: %d, \n",
			Rdi_svcb->rdv_cnt0_rdesc, Rdi_svcb->rdv_cnt1_qtree);
	    TRdisplay("   LDBDesc Cache %d,   DEFAULTS cache: %d\n",
			Rdi_svcb->rdv_cnt2_dist, Rdi_svcb->rdv_cnt3_default);
	    TRdisplay("Hashids:\n");
	    TRdisplay("   RELcache: %d,	    QTREE Cache: %d, \n",
			Rdi_svcb->rdv_rdesc_hashid, Rdi_svcb->rdv_qtree_hashid);
	    TRdisplay("   LDBDesc Cache %d,   DEFAULTS cache: %d\n",
			Rdi_svcb->rdv_dist_hashid, Rdi_svcb->rdv_def_hashid);
	    TRdisplay("...............................................\n");
	    break;
	case RD0012:
	    /* dump all of the RDF statistics */
	    rdf_report ( &Rdi_svcb->rdvstat );
	    break;

	    default:
		break;

	}
    }
    else
    {

	/*
	** determine if this is a session wide trace or a server wide trace
	** and process accordingly
	*/
	trace_scope = trace_type(flag);
	if (trace_scope == SVR_WIDE_TRACE)
	{   
	    /* turn trace on in svcb
	    **
	    ** Three possible actions: Turn on flag, turn it off, or do nothing.
	    */
	    switch (debug_cb->db_trswitch)
	    {
	    case DB_TR_ON:
		    if ((flag == RD0022) && 
			!(ult_check_macro(&Rdi_svcb->rdf_trace, 
					flag, &firstval, &secondval)))
		    {
			    /* setting RDU_CHECKSUM */
			    RDF_GLOBAL      global;
			    RDF_CB          rdfcb;
			    RDI_FCB         fcb;

			    global.rdfcb = &rdfcb;
			    rdfcb.rdf_rb.rdr_fcb = (PTR)&fcb;
			    rdfcb.rdf_info_blk = NULL;
			    rdfcb.rdf_rb.rdr_db_id = NULL;
			    rdfcb.rdf_rb.rdr_types_mask = 0;
			    rdfcb.rdf_rb.rdr_2types_mask = 0;
			    CSget_sid(&rdfcb.rdf_rb.rdr_session_id);
			    fcb.rdi_fac_id = DB_RDF_ID;
			    status = rdf_invalidate(&global, &rdfcb);
			    if (DB_FAILURE_MACRO(status))
				return(E_DB_ERROR);
		    }
		    ult_set_macro(&Rdi_svcb->rdf_trace, flag, firstval, secondval);
		break;

	    case DB_TR_OFF:
		    ult_clear_macro(&Rdi_svcb->rdf_trace, flag);
		break;

	    case DB_TR_NOCHANGE:
		/* Do nothing */
		break;

	    default:
		return (E_DB_ERROR);
	    };
	}
	else
	{
	    CS_SID	    sid;
	    RDF_SESS_CB	    *rdf_sess_cb;

	    /* 
	    ** this trace point is session specific, so use the session control
	    ** block for this trace point.
	    */

	    CSget_sid(&sid);
	    rdf_sess_cb = GET_RDF_SESSCB(sid);

	    /*
	    ** Three possible actions: Turn on flag, turn it off, or do nothing.
	    */

	    switch (debug_cb->db_trswitch)
	    {
	    case DB_TR_ON:
		    ult_set_macro(&rdf_sess_cb->rds_strace, 
				  flag, firstval, secondval);
		break;

	    case DB_TR_OFF:
		    ult_clear_macro(&rdf_sess_cb->rds_strace, flag);
		break;

	    case DB_TR_NOCHANGE:
		/* Do nothing */
		break;

	    default:
		return (E_DB_ERROR);
	    }
	}
    }
    return (E_DB_OK);
}

/*{
** Name  trace_type - Determine trace type
**
** Description:
**	Determine if the specified trace point is server wide (and goes into
**	the SVCB) or session specific (and goes into the session control block).
**	Most trace points are session specific.
**
**	The following trace points are neither, they demmand immediate action:
**	    cache flush traces		    (rd0001 to rd0005, rd0010)
**	    Unimplemented Action traces	    (rd0006 to rd0009)
**	    memory dump trace		    (rd0011)
**	    statistics dump trace	    (rd0012)
**	    Unimplemented Action traces	    (rd0013 to rd0019)
**
**	The following trace points are server wide:
**	    No Checksum			    (rd0022)
**
** Inputs:
**	trace_id	    trace point numeric identifier
**
** Outputs:
**	none
**
**	Returns:
**	    SVR_WIDE_TRACE  - applies to all sessions
**	    SESS_ONLY_TRACE - applies only to this session
**	    
**	Exceptions:
**		none
**
** Side Effects:
**	none.
**
** History:
**	22-sep-92 (teresa)
**	    initial creation for Sybil
*/
static	i4	    
trace_type(i4  trace_id)
{
    i4	ret_value;

    switch (trace_id)
    {
	case RD0022:	
	    ret_value = SVR_WIDE_TRACE;
	    break;
	default:
	    ret_value = SESS_ONLY_TRACE;
	    break;
    }
    return (ret_value);
}

/*{
** Name: rdt_clear_cache - Clear RDf cache
**
** Description:
**      This function causes one or more RDF cache(s) to be cleared, as follows:
**	    Trace Point:	    Caches Cleared:
**	        rd1		    RELcache
**		rd2		    RELcache, QTREE cache
**		rd3		    LDBdesc cache
**		rd4		    LDBdesc cache, RELcache, QTREE cache
**		rd5		    Defaults Cache
**		rd10		    ALL RDF CACHES
**
** Inputs:
**	trace_pt			value of trace point.
** Outputs:
**	none
**
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_ERROR			Function failed due to error by caller;
**	    E_DB_FATAL			Function failed due to some internal
**					problem; 
**	Exceptions:
**		none
**
** Side Effects:
**	none.
**
** History:
**	 (teresa)
**	    Initial creation -- the original subroutine header section seems
**	    to have gotten lost, so recreated today (7-jan-91)
**	07-jan-91 (teresa)
**	    Fix AV caused by uninitialized rdr_types_mask and rdr_2types_mask.
**	    (bug 41956)
**	23-Jan-92 (teresa)
**	    SYBIL:  change criter for QTREE invalidation from rdi_qthash (which
**		    is no longer in the structure to setting facility ID to
**		    PSF to invalidate QTREE hash.
**	16-jul-92 (teresa)
**	    prototypes
**	16-sep-92 (teresa)
**	    modify to take trace_pt as argument and handle trace points 3 and 4.
**	07-Mar-2001 (jenjo02)
**	    Supply session id to rdf_invalidate in RDF_CB.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/
DB_STATUS
rdt_clear_cache(i4	    trace_pt)
{
    RDF_GLOBAL	    global;
    RDF_CB	    rdfcb;
    RDI_FCB	    fcb;
    DB_STATUS	    status;

    global.rdfcb = &rdfcb;

    rdfcb.rdf_rb.rdr_fcb = (PTR ) &fcb;
    rdfcb.rdf_info_blk = NULL;
    rdfcb.rdf_rb.rdr_db_id = NULL;
    rdfcb.rdf_rb.rdr_types_mask = 0;
    rdfcb.rdf_rb.rdr_2types_mask = 0;
    CSget_sid(&rdfcb.rdf_rb.rdr_session_id);

/* trace
** point   cache	    action (call rdf_invalidate with )
** ------  ---------    ---------------------
** RD0001  RELcache	rdi_fac_id = DB_RDF_ID, rdr_2types_mask = 0
** RD0002  RELcache &	
**          QTREE	rdi_fac_id = DB_PSF_ID, rdr_2types_mask = 0
** RD0003  LDBdesc	rdr_2types_mask = RDR2_INVL_LDBDESC
** RD0004  rel,QTREE,   rdi_fac_id=DB_PSF_ID, then call a second time with
**	    LDBdesc	rdr_2types_mask=RDR2_INVL_LDBDESC
** RD0005  Defaults	rdi_fac_id=DB_RDF_ID, rdr_2types_mask=RDR2_INVL_DEFAULT
** RD0010  ALL		Three calls:  
**			1 rdi_fac_id=DB_RDF_ID,rdr_2types_mask=RDR2_INVL_LDBDESC
**			2 rdi_fac_id=DB_RDF_ID,rdr_2types_mask=RDR2_INVL_DEFAULT
**			3 rdi_fac_id=DB_PSF_ID,rdr_2types_mask = 0
*/

    switch (trace_pt)
    {
    case RD0001:
	fcb.rdi_fac_id = fcb.rdi_fac_id = DB_RDF_ID;
	status = rdf_invalidate(&global, &rdfcb);
	break;
    case RD0002:
	fcb.rdi_fac_id = fcb.rdi_fac_id = DB_PSF_ID;
	status = rdf_invalidate(&global, &rdfcb);
	break;
    case RD0003:
	fcb.rdi_fac_id = fcb.rdi_fac_id = DB_RDF_ID;
	rdfcb.rdf_rb.rdr_2types_mask = RDR2_INVL_LDBDESC;
	status = rdf_invalidate(&global, &rdfcb);
	break;
    case RD0004:
	/* invalidate LDBdesc cache first, then RELcache and QTREE cache */
	fcb.rdi_fac_id = fcb.rdi_fac_id = DB_RDF_ID;
	rdfcb.rdf_rb.rdr_2types_mask = RDR2_INVL_LDBDESC;
	status = rdf_invalidate(&global, &rdfcb);	    /* LDBdesc cache */
	if (status != E_DB_OK)
	    break;
	rdfcb.rdf_rb.rdr_2types_mask = 0;
	fcb.rdi_fac_id = fcb.rdi_fac_id = DB_PSF_ID;
	status = rdf_invalidate(&global, &rdfcb);	    /* RELcache, QTREE*/
	break;
    case RD0005:
	fcb.rdi_fac_id = fcb.rdi_fac_id = DB_RDF_ID;
	rdfcb.rdf_rb.rdr_2types_mask = RDR2_INVL_DEFAULT;
	status = rdf_invalidate(&global, &rdfcb);
	break;
    case RD0010:
	/* invalidate LDBdesc cache first, then default cache, then
	** RELcache and QTREE cache 
	*/
	fcb.rdi_fac_id = fcb.rdi_fac_id = DB_RDF_ID;
	rdfcb.rdf_rb.rdr_2types_mask = RDR2_INVL_LDBDESC;
	status = rdf_invalidate(&global, &rdfcb);	    /* LDBdesc cache */
	if (status != E_DB_OK)
	    break;
	fcb.rdi_fac_id = fcb.rdi_fac_id = DB_RDF_ID;
	rdfcb.rdf_rb.rdr_2types_mask = RDR2_INVL_DEFAULT;
	status = rdf_invalidate(&global, &rdfcb);	    /* Default cache */
	if (status != E_DB_OK)
	    break;
	fcb.rdi_fac_id = fcb.rdi_fac_id = DB_PSF_ID;
	rdfcb.rdf_rb.rdr_2types_mask = 0;
	status = rdf_invalidate(&global, &rdfcb);	    /* RELcache, QTREE*/
	break;
    default:
	/* do nothing if trace_pt is an unexpected value */
	break;
    }    
    return status;
}
