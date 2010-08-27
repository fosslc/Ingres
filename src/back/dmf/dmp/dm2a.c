/*
** Copyright (c) 1985, 2009 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <bt.h>
#include    <cm.h>
#include    <cs.h>
#include    <ex.h>
#include    <di.h>
#include    <me.h>
#include    <pc.h>
#include    <sd.h>
#include    <sr.h>
#include    <tr.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <ulf.h>
#include    <sxf.h>
#include    <adf.h>
#include    <ade.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmccb.h>
#include    <dmucb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dml.h>
#include    <dmpp.h>
#include    <dm1c.h>
#include    <dm1b.h>
#include    <dm1m.h>
#include    <dm0m.h>
#include    <dm0p.h>
#include    <dm1r.h>
#include    <dm2t.h>
#include    <dm2r.h>
#include    <dm2a.h>
#include    <dmftrace.h>

/**
**
**  Name: DM2A.C - Aggregate Request Processor.
**
**  Description:
**      This file contains the routines that process those simple
**	aggregate functions that are best done by one call to DMF
**	rather than multiple calls to DMF from QEF. Prior to the addition
**	of this DMF-based Aggregate Request Processor, all aggregates were
**	processed within QEF, with QEF getting one record at a time from
**	DMF in order to perform some aggregation logic, which, in the most
**	degenerate cases would simply add 1 to a (very expensive ADE) counter
**	to process a COUNT agg, or scan/compare every record of a btree table,
**	just to find the minimum or maximum value of the key field.
**
**	This Aggregate Request Processor removes these performance problems
**	by applying knowledge of individual storage structures to satisfy the
**	requested aggregates. DMR_AGGREGATE may be used to compute COUNT, MIN,
**	MAX, SUM and AVG aggregates, in any combination, and against any 
**	storage structure. Even when used in the 'worst case', eg all 
**	aggregates against a heap table, use of DMR_AGGREGATE reduces CPU 
**	and system paging, as QEF only invokes DMF once, rather than once 
**	for each qualifying tuple.
**
**	The addition of this Aggregate Request Processor was done in 
**	conjunction with changes to QEF, to invoke this processor, and with
**	OPF, to detect certain special cases (limited aggregates) described
**	below. The Aggregate Request Processor is invoked via the
**	DMR_AGGREGATE request type to dmfcall(), and processes multiple
**	aggregates in one pass, with the result(s) available to QEF upon
**	return from a single DMR_AGGREGATE request. With the exception
**	of a simple COUNT aggregate, the DMF Aggregate Request Processor will
**	invoke ADE to 'compute' the aggregate results, using an ADE_EXCB
**	provided by QEF upon issuing the DMR_AGGREGATE request. Qualifications
**	are handled as usual in DMF, if the dmr_q_fcn is provided with the
**	DMR_AGGREGATE request.  
**
**	Prior to issueing a DMR_AGGREGATE request, QEF must execute
**	a DMR_POSITION request, as in the past, using the DMR_CB that is
**	subsequently passed in with the DMR_AGGREGATE request.
**	
**	If the table is a gateway, no optimizations are done, and each
**	record (as per positioning) is read and aggregated, as before.
**
**	AGGREGATE LIMIT INDICATORS
**	--------------------------
**	As an optimization, DMR_AGGREGATE may be passed  bits referred
**	to as 'aggregate limit indicators', which indicate that the request 
**	conforms to certain limits that  allow extensive optimizations within
**	DMF. These bits and the optimizations performed are listed below. 
**	In general the absence of any aggregate limit indicators means
**	that any combination of aggregates may be present in the  ADE_EXCB, and
**	that DMF must call ade_execute_cx() for each tuple that passes the  
**	DMR_POSITION and optional qualification function criteria. The presence
**	of 1 or more aggregate limit indicators means that ONLY the aggregates
**	indicated are present in the ADE_EXCB and that DMF may optimize
**	the data path, as described below.
** 
**	The aggregate limit indicators, DMR_AGLIM_COUNT, DMR_AGLIM_MIN and
**	DMR_AGLIM_MAX indicate that a combination of COUNT, MIN or MAX
**	aggregates are present, in conformance with the following
**	restrictions, presumably enforced by OPF:
**
**	1. No other aggregates, other than those indicated by the
**	   combination of bits supplied, are being executed.
**
**	2. In the case of MIN and MAX, the aggregation is being done
**	   on the (major field of the) key of a BTREE or ISAM table.
**	   (DMR_AGGREGATE may be used for a non-keyed MIN/MAX, however
**	    the limit bits may not be used, and a full, albeit efficient
**	    scan will be done).
**
**
** CATEGORIES OF AGGREGATES & LIMIT BITS
**
**	For the purposes of the effects of the DMR_AGGREGATE request,
**	the possible combinations of different aggregates and the optimizations
**	performed are listed below, in decreasing order of potential 
**	optimization.  The term 'Qual' indicates either the existence of a 
**	qualification OR the presence of C2 security in the system, either
**	of which require additional work to be done for a COUNT request - 
**	namely the actual referencing and evaluation of tuples 
**	whose presence would otherwise be accounted for based on other 
**	structural information in the table.
**	Each 'DMF ACTIVITY' listed below corresponds to an 'action block'
**	of type DM2A_AGGACT that is used in the code to represent the
**	steps needed and their context of execution.
**
**	The numbers in parens are referred to in the code, such as CASE(1).
**
**	AGGREGATE
**	FUNCTION(S)	DMF ACTIVITY/OPTIMIZATION		'LIMIT' BIT	
**	----------	----------------------------------	-----------
**	(1)
**	COUNT alone	Varies by storage structure. 		DMR_AGLIM_COUNT
**	Without Qual    BTREE - only read leaf pages, add
**			up all bt_kids. Other structs - read
**			data pages, add up # tups based on
**			linetab accessor. 
**	            	Tuples themselves not accessed.
**			ade_execute_cx() never called. 
**	
**	(2)
**	MIN and/or MAX	Varies by storage structure. BTREE	DMR_AGLIM_MIN
**	without		or ISAM full or qualified scan is 	    and/or
**	any others.	reduced, to start/end probes, using	DMR_AGLIM_MAX
**	w/wo Qual.	knowledge of index structure.			
**			All qualifying records from reduced 
**			scan are be passed to ade_execute_cx().
**			There is a separate DM2A_AGGACT action
**			block, and a separate scan, for each
**			of MIN and MAX if both present.
**
**			BTREE/min - read till 1st qualifying
**			tuple, must be min since data is sorted.
**
**			BTREE/max - Read backwards from end of
**			qual, if any, or position to EOT and
**			read backwards. Stop at 1st qualifying
**			tuple since data is sorted.
**
**			ISAM/min - read mainpgs and ovflo pages
**			via dm2r_get until at least one qualifying
**			tuple has been found, AND until the end of
**			the entire mainpg/ovflo chain in which the
**			qualifying tuple has been found. This ensures
**			that any key with a lower value that is in
**			that mainpg range but is out of sort will
**			still be evaluated by ade_execute_cx().
**
**			ISAM/max - Start reading the last mainpg of
**			the positioned scan, and its overflows, until
**			the entire chain is scanned. If a qualifying
**			tup is found, thats it. Else, locate the 
**			previous mainpg (by brute force) and scan its 
**			entire chain continuing as above until reaching
**			the beginning of the positioned scan.
**	
**	(3)
**	MIN/MAX		Varies by storage structure.           	DMR_AGLIM_COUNT
**	with COUNT.	BTREE - Execute the    	   	    	    and
**	Without Qual.	union of (1) and (2) listed above,	DMR_AGLIM_MIN
**			ie. start/end probes for MIN/MAX,	OR DMR_AGLIM_MAX
**			followed by one pass over leaf pages 
**			to calculate COUNT.
**			For other structures, since data pages
**			must be read, treat like CASE(6)
**	
**	
**	(4)
**	COUNT		                               		DMR_AGLIM_COUNT
**	With Qual	All records within scan are accessed, 
**			qualified and counted.
**			No call to ade_execute_cx() for result.   
**			 
**	(5)
**	MIN/MAX		Like COUNT With QUAL(4), except that    DMR_AGLIM_COUNT
**	with COUNT.	ade_execute_cx() is called for each  	    and
**	With Qual.	qualifying tuple to compute result.     DMR_AGLIM_MIN
**								or DMR_AGLIM_MAX
**	
**	(6)
**	SUM, AVG	DMF need only be called once. All 	    none
**	with/without 	qualifying tups within scan range 
**	any others, 	must be passed to ade_execute_cx().
**	OR non-keyed
**	MIN/MAX.
**	w/wo Qual.
**	
**	
**
**
**	External functions:
**
**          dm2a_simpagg   		- Process simple aggregate functions
**
**	Internal functions:
**	    build_sagg_actions 		- compile list of required actions.
**	    dm2a_get_isam_backwards	- read ISAM pseudo-backwards
**	    
**
**  History:
**      21-aug-1995 (cohmi01)  
**          Created for the aggregate optimization project.
**	12-feb-1996 (nick)
**	    Fix #74636 and some other problems I found whilst here. V.F.M. !
**	18-mar-96 (nick)
**	    Don't treat E_DM0074 as an error when in DM2A_TUPSCAN. #74662
**	06-may-1996 (thaju02)
**	    New page format support: change page header references to use
**	    macros.
**	10-jul-96 (nick)
**	    Multiple aggregates not run in certain circumstances resulting
**	    in server failure due to fixed pages being orphaned.
**	15-jul-96 (sarjo01)
**	    Bug 76243:  Merge ADF warnings from temporary RCB(s) to passed
**	    in RCB so that correct warning counts are not lost.
**      16-jul-96 (sarjo01)
**          Added dm2a_clearwarn() and dm2a_copywarn() for use by qen_origagg()
**          to manage warning counters. 
**	30-jul-96 (nick)
**	    Remove above functions added by sarjo01 - direct calls to
**	    DMF from outside the facility are illegal.
**	23-Oct-1996 (wonst02)
**	    Added call to dm1m_count() for Rtree.
**	22-Jan-1997 (jenjo02)
**	    Removed DM0M_ZERO flag from dm0m_allocate() calls for
**	    readonly buffers. There's no reason to do this expensive
**	    operation. Also fixed what looked like a bug: the
**	    allocation for these buffers was using
**	    sizeof(DMPP_PAGE) instead of tcb_rel.relpgsize.
**	11-Feb-1997 (nanpr01)
**	    Bug 80636 : ADF control block needs to be initialized.
**      29-sep-1997 (stial01)
**          build_sagg_actions() B86295: Init position information in rcb 
**          after btree search.
**      29-Jan-1998 (hanal04)
**          RCB's are initialised with access_mode = WRITE, in the call
**          to dm2r_rcb_allocate(). Set access_mode to READ for simple
**          aggregates. b84245. Cross Integration of 432418.
**      12-Apr-1999 (stial01)
**          Distinguish leaf/index info for BTREE/RTREE indexes
**      12-nov-1999 (stial01)
**          Changes to how btree key info is stored in tcb
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-may-2001 (stial01)
**          Init adf_ucollation
**	31-Dec-2002 (hanch04)
**	    Fix cross integration, replace longnat with i4
**	18-nov-03 (inkdo01)
**	    Init adf_base_array.
**      29-sep-04 (stial01)
**          Allocate extra rnl buffer if DMPP_VPT_PAGE_HAS_SEGMENTS
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here.
**	    Execute CX in DMF context, like we do for qualification, now
**	    that DMF is smarter about ADF exceptions and errors.  Various
**	    changes for error handling in RCB's adfcb context.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2a_?, dm2r_? functions converted to DB_ERROR *,
**	    use new form uleFormat.
**	02-Dec-2008 (jonj)
**	    SIR 120874: dm1b_? functions converted to DB_ERROR *
**	04-Dec-2008 (jonj)
**	    SIR 120874: dmf_adf_error function converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm0p_? functions converted to DB_ERROR *
**      05-feb-2009 (joea)
**          Rename CM_ischarsetUTF8 to CMischarset_utf8.
**	19-Aug-2009 (kschendel) 121804
**	    Need cm.h for proper CM declarations (gcc 4.3).
**      24-nov-2009 (stial01)
**          Check if table has segmented rows (instead of if pagetype supports)
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace DMPP_PAGE* with DMP_PINFO* where needed.
*/

static DB_STATUS	build_sagg_actions(
	DMP_RCB		*rcb,
	i4		agglims,
	DM2A_AGGACT	act[DM2A_MAXAGGACT],
	i4		*nacts,
	i4		*countloc,
	DB_ERROR     	*dberr);

static DB_STATUS 	dm2a_get_isam_backwards(
	DMP_RCB     *r,
	DM_TID	*tid,
	i4     opflag,
	char	*record,
	DB_ERROR	*dberr);


/*{
** Name: dm2a_simpagg - Process simple aggregate functions
**
** Description:
**	This function executes simple aggregate functions using optimizations
**	available to DMF based on knowledge of storage structures. This
**	function is intended to be called by a 'dml' layer wrapper, invoked
**	from QEF for the 'SAGG' nodes. In addition to providing the
**	ADE_EXCB information needed for DMF to drive the aggregate computation,
** 	QEF provides information as to whether this is a 'limited' aggregate
**	request, and if so, which 'limited' aggregates are requested.
**	The following aggregates are considered 'limited' aggregates:
**	MIN
**	MAX
**	COUNT
**
**	A 'limited' aggregate request is one which contains only combinations
**	of limited aggregates, without any non-limited aggregates such as
**	SUM included in the request. The DMF aggregate processor is able
**	to apply additional optimizations in these cases.
**
**
** Inputs:
**	rcb                             The RCB to get a record from.
**	rec   	 			Pointer to location to place record.
**
** Outputs:
**	err_code                        The reason for error status.
**					    E_DM003C_BAD_TID
**					    E_DM0044_DELETED_TUPLE
**					    E_DM0042_DEADLOCK
**					    E_DM0074_NOT_POSITIONED
**					    E_DM0073_RECORD_ACCESS_CONFLICT
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** Exceptions:
**	none
**
** Side Effects:
**	none
**
** Assumptions:
**
** Related Functions:
**
** History:
**	21-Aug-1995 (cohmi01)
**	    Created, for aggregate optimizations.
**	12-feb-1996 (nick)
**	    Deallocate the READNOLOCK buffer we may have allocated in the
**	    local RCB.
**	18-mar-96 (nick)
**	    Ignore E_DM0074 on scans - we typically get this if a key doesn't
**	    exist.
**	06-may-1996 (thaju02)
**	    New page format support: change page header references to use
**	    macros.
**	10-jul-96 (nick)
**	    Ensure any remaining actions are processed when we hit E_DM0055
**	    and also ensure that all locally allocated RCB's are deallocated 
**	    if we do hit a fatal error which causes us to short-circuit the
**	    remaining actions. #77515, #77651
**      15-jul-96 (sarjo01)
**          Bug 76243:  Merge ADF warnings from temporary RCB(s) to passed
**          in RCB so that correct warning counts are not lost.
**      16-jul-96 (sarjo01)
**          Added dm2a_clearwarn() and dm2a_copywarn() for use by qen_origagg()
**          to manage warning counters.
**	10-jun-1998(angusm)
**	    bug 89411 (again): EX handler set below is not unset
**	    if we longjump back on error.
**      27-jul-98 (nicph02)
**          E_DM004D don't need to be logged as E_DM9043 is previously logged.
**          (part of bug 88649)
**	27-jul-98 (nicph02)
**	    E_DM0042 doesn't need to be logged as it is previously logged.
**          (part of bug 88755).
**	27-Jul-1998 (carsu07)
**	    Cross integrate fix for bug 90713 from OI1.2. Reports correct
**	    error message when date or money formats are set.
**	15-jan-1999 (nanpr01)
**	    pass ptr to ptr in dm0m_deallocate routine.
**	30-Apr-02 (wanfr01)
**	    Bug 79456, INGSRV 1765
**	    Unfix iirelation pages during aggregate processing to avoid 
**          server crash/inconsistant database if aggregate involves iitables
**     28-Jul-2003 (horda03) Bug 94525/INGSRV642
**          When processing max(varchar_col) with numeric and non-numeric values 
**          e.g. '5','600','ABC'. The min() function works OK but the max() 
**          function SEGV's the dbms. If an error is returned, then if the 
**          current RCB was locally allocated, it gets deallocated twice.
**          The second time in the cleanup, causes the SIGSEGV.
**      20-jan-2004 (huazh01)
**          Do not release the RCBs if they are null. 
**          This prevents SEGV in dm2a_simpagg() and fixes 
**          b111510, INGSRV2646.
**	31-Aug-2004 (schka24)
**	    adf_base_array removed, fix here.
**	05-Nov-2004 (drivi01)
**	    Removal of adf_base_array didn't propagate correctly from main.
**	21-feb-05 (inkdo01)
**	    Init Unicode normalization flag.
**	12-Feb-2007 (kschendel)
**	    Add occasional CScancelCheck to read cancels from the outside.
**	11-may-07 (gupsh01)
**	    Init adf_utf8_flag.
*/
DB_STATUS
dm2a_simpagg(
DMP_RCB		*rcb,
i4		agglims,
ADE_EXCB	*excb,
ADF_CB		*qef_adf_cb,
char        	*rec,
i4		*count,
DB_ERROR     	*dberr )
{
    DM2A_AGGACT	act[DM2A_MAXAGGACT];	/* aggregate action descriptors */
    DML_SCB	*scb;
    i4		a;			/* indices into 'act' array	*/
    i4		nacts;			/* number of actions compiled   */
    i4	p_count, rec_cnt;	/* record counters		*/
    DB_STATUS	status, local_status;
    bool	null_skipped = FALSE;	/* TRUE if ADE encountered null */
    i4	local_err = 0;
    DM_TID	flagtid;
    i4		countloc;		/* where to obtain count, if at all */
    void	*feedback;
    DB_ERROR	local_dberr;
    i4		*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    scb = NULL;
    if (rcb->rcb_xcb_ptr)
	scb = rcb->rcb_xcb_ptr->xcb_scb_ptr;	/* Should be non-null */

    /* Step 1.	Input validation.					*/	
    /* check for illegal combinations */
    if ((! agglims) && ! excb)
    {
	SETDBERR(dberr, 0, E_DM0165_AGG_BAD_OPTS);
    	return (E_DB_ERROR);
    }

    /* Step 2.	Compile a list of actions needed to process this request */
    status = build_sagg_actions(rcb, agglims, act, &nacts, &countloc, dberr);

    if (status != E_DB_OK)
	return (status);

    /* Step 3. Execution phase. Execute actions compiled above.		*/
    for (a = 0; a < nacts; a++)
    {
    	DMP_RCB	*r;
	ADF_CB  *adf_cb;

	/* action initialization, if any */
	r = act[a].rcb;		/* may be different than rcb passed in 	*/
	flagtid.tid_tid.tid_line = FALSE; /* for dm2a_get_isam_backwards flag */
	feedback = (act[a].operation == DM2A_TUPSCAN) ? 
	    (void*)&flagtid : &p_count;
	    
	/* Initialize the rcb adf cb from qef_adf_cb */
	adf_cb = r->rcb_adf_cb;
	adf_cb->adf_srv_cb = qef_adf_cb->adf_srv_cb;
	adf_cb->adf_dfmt = qef_adf_cb->adf_dfmt;
	adf_cb->adf_mfmt = qef_adf_cb->adf_mfmt;
	adf_cb->adf_decimal = qef_adf_cb->adf_decimal;
	adf_cb->adf_outarg = qef_adf_cb->adf_outarg;
	adf_cb->adf_exmathopt = qef_adf_cb->adf_exmathopt;
	adf_cb->adf_qlang = qef_adf_cb->adf_qlang;
	adf_cb->adf_nullstr = qef_adf_cb->adf_nullstr;
	adf_cb->adf_constants = qef_adf_cb->adf_constants;
	adf_cb->adf_slang = qef_adf_cb->adf_slang;
	adf_cb->adf_collation = qef_adf_cb->adf_collation;
	adf_cb->adf_ucollation = qef_adf_cb->adf_ucollation;
	adf_cb->adf_uninorm_flag = qef_adf_cb->adf_uninorm_flag;
	adf_cb->adf_rms_context = qef_adf_cb->adf_rms_context;
	adf_cb->adf_proto_level = qef_adf_cb->adf_proto_level;
	adf_cb->adf_lo_context = qef_adf_cb->adf_lo_context;
	adf_cb->adf_strtrunc_opt = qef_adf_cb->adf_strtrunc_opt;
	adf_cb->adf_year_cutoff = qef_adf_cb->adf_year_cutoff;

        if (CMischarset_utf8())
	   adf_cb->adf_utf8_flag = AD_UTF8_ENABLED;
	else
	   adf_cb->adf_utf8_flag = 0;

	/* action loop, based on scan(s) generated by build_sagg_actions */
	for (rec_cnt = 0; ;rec_cnt++)
	{
	    /* may call dm2r_get, dm1X_count, or dm2a_get_isam_backwards */

	    /* all now expect DB_ERROR *dberr instead of i4 *err_code */
	    
	    status = (*act[a].scanner)(r, feedback, act[a].direction, rec, dberr);

	    /* Bug 79456: Unfix page if table is iirelation, or you risk a
            fatal deadlock if the ade control block requests a control lock of
            a usre table */
	    if (r->rcb_tcb_ptr->tcb_rel.reltid.db_tab_base == DM_B_RELATION_TAB_ID)
	    {
		local_status = dm2r_unfix_pages(r, &local_dberr);
		if (local_status != E_DB_OK)
		{
		    if (status == E_DB_OK)
		    {
			status = local_status;
			*dberr = local_dberr;
		    }
			else
		    {
			uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL,
			       ULE_LOG, NULL, (char * )NULL, (i4)0,
			       (i4 *)NULL, &local_err, 0);
		    }
		}
	    }

	    if (status != E_DB_OK)
		    break;

	    /* troll for external interrupts */
	    CScancelCheck(r->rcb_sid);

	    /* except for count, we generally call ADE to do computation */
	    if (act[a].call_ade)
	    {

		if (scb != NULL)
		    scb->scb_qfun_adfcb = adf_cb;
		status = ade_execute_cx(adf_cb, excb);
		if (scb != NULL)
		    scb->scb_qfun_adfcb = NULL;

		if (status != E_DB_OK)
	    	{
		    status = dmf_adf_error(&adf_cb->adf_errcb, status,
					scb, dberr);
		    if (status != E_DB_OK)
		    {
			if (dberr->err_code != E_DM0023_USER_ERROR)
			    dberr->err_code = E_DM0166_AGG_ADE_FAILED;
			STRUCT_ASSIGN_MACRO(adf_cb->adf_errcb, qef_adf_cb->adf_errcb);
			break;
		    }
	    	}
		/* ADE was successful, if data was null, keep going.	*/
		if (excb->excb_nullskip)
		{
		    null_skipped = TRUE;	
		    rec_cnt--;	/* this one doesn't count */
		    continue;	/* next record */
	  	}
	    }

	    /* We've got a qualified, non-null tuple */
	    if (act[a].break_on == DM2A_BRK_FIRST) 
		break;	/* got first tup, so we are done */	    
	    else if (act[a].break_on == DM2A_BRK_NEWMAINPG)
	    {
		/* 
		** Since entire main-page range must be read, save mainpg of
		** 1st good tuple, keep going till next tup is on new mainpg
		*/
		if (rec_cnt == 0)
		    act[a].orig_mainpg = 
			DMPP_VPT_GET_PAGE_MAIN_MACRO(r->rcb_tcb_ptr->tcb_rel.relpgtype,
			r->rcb_data.page);
		else
		    if (DMPP_VPT_GET_PAGE_MAIN_MACRO(r->rcb_tcb_ptr->tcb_rel.relpgtype,
			r->rcb_data.page)
			!= act[a].orig_mainpg)
			break;	/* we have read all we need */
	    }	
	    else if (act[a].break_on == DM2A_BRK_NONEXT)
	    {
		/* Tell dm2a_get_isam_backwards it's OK to stop at chain-end */
		flagtid.tid_tid.tid_line = TRUE; 
		continue;
	    }
	}
	
	/* if this act uses a locally allocated allocated RCB, free it up */
	if (act[a].rcb != rcb)
	{
	    i4	local_err2;
            ADF_CB	*adf_cb1; 

	    adf_cb1 = act[a].rcb->rcb_adf_cb;
	    if (adf_cb1->adf_warncb.ad_anywarn_cnt > 0)
	    {
		/* Roll into rcb adfcb */
		dmr_adfwarn_rollup(rcb->rcb_adf_cb, adf_cb1);
	    }

	    if (act[a].rcb->rcb_rnl_cb)
		dm0m_deallocate((DM_OBJECT **)&act[a].rcb->rcb_rnl_cb);
	    local_status = dm2r_release_rcb(&act[a].rcb, &local_dberr);
	    if (local_status)
	    {
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
			NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			&local_err2, (i4)0);
	    }
	}
 
	if (status != E_DB_OK)
	{
	    if (act[a].operation == DM2A_TUPSCAN &&
	    	dberr->err_code == E_DM0055_NONEXT)
	    {
		status = E_DB_OK;
		CLRDBERR(dberr);
	    }
	    else if (dberr->err_code != E_DM0074_NOT_POSITIONED &&
                     dberr->err_code != E_DM004D_LOCK_TIMER_EXPIRED &&
                     dberr->err_code != E_DM0042_DEADLOCK)
	    {
		/* Log error unless already dealt with by dmf-adf-error */
		if (dberr->err_code != E_DM0023_USER_ERROR)
		{
		    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
		}

                /* To get here means that an error has occurred, and if the
                ** act[a].rcb was locally allocated, it has already been
                ** released, so increment a, so the cleanup section below
                ** won't try to deallocate the RCB again.
                */
                a++;

	    	break;
	    }
	}
    }	/* END - for each action */

    /* 
    ** ensure that we release all locally allocated RCBs as we may have
    ** bailed out early due to some unexpected error
    */
    for (;a < nacts; a++)
    {
	if ( act[a].rcb && act[a].rcb != rcb)
	{
	    i4	local_err2;

	    if (act[a].rcb->rcb_rnl_cb)
		dm0m_deallocate((DM_OBJECT **)&act[a].rcb->rcb_rnl_cb);
	    local_status = dm2r_release_rcb(&act[a].rcb, &local_dberr);
	    if (local_status)
	    {
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
			NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
			&local_err2, (i4)0);
	    }
	}
    }

    /* return the proper record count, if any */
    switch (countloc)
    {
	case	DM2A_COUNT_REC:
	    *count = rec_cnt;
	    break;
	case	DM2A_COUNT_PAG:
	    *count = p_count;
	    break;
	case	DM2A_COUNT_NONE:
	    *count = -1;	/* illegal count value */		
    }	

    /* If any null ever found by ADE, reset indicator so QEF will know.	*/
    excb->excb_nullskip = null_skipped;

    return (status);
}

/*{
** Name: build_sagg_actions - Build action blocks to process simple aggregates
**
** Description:
**	This function fills in one or more 'DMF aggregate action blocks',
**	(DM2A_AGGACT) that describe the steps needed to process the requested
**	aggregates. Each block will represent a scan of all or part of the
**	table, depending on the aggregates present and the storage structure.
**	SEE BLOCK COMMENT AT TOP OF FILE FOR POSSIBLE COMBINATIONS OF
**	AGGS and STRUCTURES, AND THE RESULTING ACTIONS THAT ARE TAKEN.
**
**
** Inputs:
**	rcb                             The RCB to get a record from.
**	agglims				bitmap of 'limiting aggregates'
**	act				ptr to mem in which to place actions
**
** Outputs:
**	nacts				Number of actions placed in 'act'
**	countloc			Indicates where to obtain 'count' from.
**	err_code                        The reason for error status.
**					    E_DM0165_AGG_BAD_OPTS
**					    E_DM0073_RECORD_ACCESS_CONFLICT
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** Exceptions:
**	none
**
** Side Effects:
**	none
**
** Assumptions:
**	'act' memory has been allocated for max number of actions.
**	'rcb' passed in has been positioned based on qualification,
**	or for all records, if no qualification.
**
** Related Functions:
**
** History:
**	21-Aug-1995 (cohmi01)
**	    Created, for aggregate optimizations.
**	12-feb-1996 (nick)
**	    READNOLOCK queries stash the page(s) in the RCB ; since we
**	    may allocate local RCBs here, we must ensure that they have
**	    the storage allocated for these pages else we trash something when
**	    attempting to fault in a page. #74636
**      29-sep-1997 (stial01)
**          build_sagg_actions() B86295: Init position information in rcb 
**          after btree search.
**      29-Jan-1998 (hanal04)
**          RCB's are initialised with access_mode = WRITE, in the call
**          to dm2r_rcb_allocate(). Set access_mode to READ for simple
**          aggregates. b84245. Cross Integration of 432418.
**	07-Dec-1999 (i4jo01)
**	    Make sure we copy over key qualification flag from original
**	    rcb. (b99581)
**	20-Jan-2005 (schka24)
**	    Only force unqual row-fetch if we're row auditing, not for
**	    all C2 servers.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
**	22-Jul-2010 (kschendel) b124116
**	    Fix problems if we allocate more RCB's here and the table
**	    contains LOB's.  If we're not qualifying rows, tag the RCB
**	    with "no coupon" since we can't possibly care about any
**	    LOB columns.  If we're qualifying, there is some remote chance
**	    that the where-clause will involve LOBs, so copy the BQCB
**	    pointer from the calling RCB into our temporary ones.
*/
static DB_STATUS
build_sagg_actions(
DMP_RCB		*rcb,
i4		agglims,
DM2A_AGGACT	act[DM2A_MAXAGGACT],
i4		*nacts,
i4		*countloc,
DB_ERROR     	*dberr )
{
    DMP_TCB	*t = rcb->rcb_tcb_ptr;
    DMP_RCB	*arcb;
    bool	getrows;		/* true if need to access rows	*/
    i4		na;			/* indices into 'act' array	*/
    DB_STATUS	status;
    DB_ERROR	local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);
    
				
    /* 
    ** If there is C2 security in use, or there is a qualification
    ** provided, we must access each row separately via a get routine,
    ** so that each row can be checked. This is still better than
    ** having QEF do the gets.
    ** Even with C2 security, we only need row-fetches if the table is
    ** being row audited.
    */
    getrows = rcb->rcb_f_qual != NULL ||
    	( (dmf_svcb->svcb_status & SVCB_C2SECURE)
	  && (t->tcb_rel.relstat2 & TCB_ROW_AUDIT) );

    /* 'compile' the action blocks needed to execute the requested aggs */
    status = E_DB_OK;
    na = 0;
    do
    {
	/* 
	** If no lims (CASE 6), or its a gateway table (no shortcuts),
	** or COUNT with qual, with/without MIN and/or MAX (CASE 5)
	** then set up one scan, tuple-level, with ADE.
	** As a similar case, if just COUNT with qual (CASE 4), set up
	** one similar scan, tuple-level without ADE.
	*/
	if ((! agglims) || (t->tcb_table_type == TCB_TAB_GATEWAY) ||
	    (getrows && (agglims & DM2A_AGLIM_COUNT)))
	{
	    /* Case 4,5 or 6. One block only. tuple-level scan.	*/

	    act[0].operation = DM2A_TUPSCAN; 	/* dm2r_get scan	*/	
	    act[0].scanner = dm2r_get;
	    act[0].direction = DM2R_GETNEXT;	/* read forward only	*/
	    act[0].break_on = DM2A_BRK_NONEXT;	/* read recs until end  */
	    act[0].rcb = rcb;                 	/* use rcb QEF passed   */
	    act[0].call_ade = (agglims == DM2A_AGLIM_COUNT) ?
		FALSE :	/* No ADE needed if only COUNT, do count here	*/
		TRUE;	/* ADE needed for aggs besides count 		*/

	    /* 
	    ** That's all for this request, just one action, no need to
	    ** test remaining cases, as these are self contained.
	    */
	    na = 1;
	    *countloc = DM2A_COUNT_REC;	/* use record count    	*/
	    break;
	}


	/* 
	** The following cases are cumulative, any combination of MIN,
	** MAX or count may appear, for a total of 3 action blocks.
	** If we are here, it is known that:
	**
	** 1) there must be at least one limit bit set, and ...
	** 2) if there is a COUNT, there can't be a qual.
	*/

	if (agglims & DM2A_AGLIM_COUNT)
	{
	    /* 
	    ** This must be an unqualified COUNT (CASE 1). Even if 
	    ** there may be other limited aggs (MIN/MAX), create an
	    ** action for a page scan, only use ADE if other aggs.
	    */

	    act[na].operation = DM2A_PAGESCAN; 	/* only read pages.	*/
	    act[na].scanner = 
	    	(t->tcb_table_type == TCB_BTREE) ? dm1b_count :
	    	(t->tcb_table_type == TCB_RTREE) ? dm1m_count : dm1r_count;
	    act[na].break_on = DM2A_BRK_FIRST;	/* one dmxxx call       */

	    /* 
	    ** Choose rcb, can use callers unless unless other aggs are
	    ** present, in which case get our own as user's may be
	    ** positioned for other aggs scan
	    */
	    if (agglims != DM2A_AGLIM_COUNT)
	    {
		status = dm2r_rcb_allocate(t, rcb, &rcb->rcb_tran_id,
		    rcb->rcb_lk_id, rcb->rcb_log_id, &arcb, dberr);

		if ( status != E_DB_OK )
		    break;
		act[na].rcb = arcb;
		/* b84245 - simple aggregates do not require WRITE mode */
		arcb->rcb_lk_mode = RCB_K_IS;
		arcb->rcb_k_mode = RCB_K_IS;
		arcb->rcb_access_mode = RCB_A_READ;
		arcb->rcb_uiptr = rcb->rcb_uiptr;
		/* won't need any lobs in the row */
		arcb->rcb_state |= RCB_NO_CPN;

		/* passed-in RCB should point to a valid message area if
		** it was opened in the normal way by a user (dmtopen).
		*/
		STRUCT_ASSIGN_MACRO(rcb->rcb_adf_cb->adf_errcb,
			act[na].rcb->rcb_adf_cb->adf_errcb);

		if (rcb->rcb_k_duration == RCB_K_READ)
		{
		    i4	rnl_bufs;
			
		    if (t->tcb_seg_rows)
			rnl_bufs = 3;
		    else
			rnl_bufs = 2;
		    status = dm0m_allocate(
				sizeof(DMP_MISC) + t->tcb_rel.relpgsize * rnl_bufs,
				(i4)0, (i4)MISC_CB,
				(i4)MISC_ASCII_ID, (char *)&act[na].rcb,
				(DM_OBJECT **)&arcb->rcb_rnl_cb,
				dberr);
		    if (status != E_DB_OK)
			break;
		    arcb->rcb_rnl_cb->misc_data =
			(char*)arcb->rcb_rnl_cb + sizeof(DMP_MISC);
		}
		status = dm2r_position(arcb, DM2R_ALL, NULL, 0,
				(DM_TID *)NULL, dberr);
		if ( status != E_DB_OK )
		    break;
	    }
	    else
	    {
		act[na].rcb = rcb;
	    }

	    act[na].call_ade = FALSE;

	    /* 
	    ** That's all for this request, increment action counter 
	    ** and fall through to test remaining cases for additional 
	    ** actions
	    */
	    na++;
	    *countloc = DM2A_COUNT_PAG;   	/* use page counter's count */
	}
	else
	{
	    *countloc = DM2A_COUNT_NONE;   	/* No record count available */
	}

	if (agglims & DM2A_AGLIM_MIN)   
	{
	    /* This is a min, (CASE 2 or CASE 3).                  	*/

	    act[na].operation = DM2A_TUPSCAN; 	/* dm2r_get scan	*/
	    act[na].scanner = dm2r_get;
	    act[na].direction = DM2R_GETNEXT;	/* read forward 	*/
	    /* 
	    ** 1st tup is the min for btree. for ISAM we must read
	    ** until we detect that we have moved to a new mainpage.
	    */
	    if (t->tcb_table_type == TCB_BTREE)
	    {
	    	act[na].break_on = DM2A_BRK_FIRST;
	    }
	    else
	    {
		act[na].break_on = DM2A_BRK_NEWMAINPG;
		act[na].orig_mainpg = rcb->rcb_p_lowtid.tid_tid.tid_page;
	    }
	    act[na].call_ade = TRUE;	/* to accumulate min value.	*/
	    act[na].rcb = rcb;

	    /* 
	    ** That's all for this request, increment action counter and
	    ** fall through to test remaining cases for additional actions.
	    */
	    na++;
	}

	if (agglims & DM2A_AGLIM_MAX)   
	{
	    /* This is a max, (CASE 2 or CASE 3).                  	*/

	    act[na].operation = DM2A_TUPSCAN; 	/* dm2r_get scan	*/

	    /* 
	    ** For now, assume we always need a separate rcb for MAX.
	    ** We might be able to avoid, if no other aggs.
	    */
	    status = dm2r_rcb_allocate(t, rcb, &rcb->rcb_tran_id,
		rcb->rcb_lk_id, rcb->rcb_log_id, &arcb, dberr);

	    if ( status )
		break;
            /* b84245 - simple aggregates do not require WRITE mode */
            arcb->rcb_lk_mode = RCB_K_IS;
            arcb->rcb_k_mode = RCB_K_IS;
            arcb->rcb_access_mode = RCB_A_READ;
	    arcb->rcb_uiptr = rcb->rcb_uiptr;
	    STRUCT_ASSIGN_MACRO(rcb->rcb_adf_cb->adf_errcb,
			arcb->rcb_adf_cb->adf_errcb);

	    if (rcb->rcb_k_duration == RCB_K_READ)
	    {
		i4	rnl_bufs;
		    
		if (t->tcb_seg_rows)
		    rnl_bufs = 3;
		else
		    rnl_bufs = 2;

	    	status = dm0m_allocate(
			    sizeof(DMP_MISC) + t->tcb_rel.relpgsize * rnl_bufs,
		    	    (i4)0, (i4)MISC_CB, 
			    (i4)MISC_ASCII_ID, (char *)&arcb,
			    (DM_OBJECT **)&arcb->rcb_rnl_cb, dberr);
		if (status != E_DB_OK)
		    break;
		arcb->rcb_rnl_cb->misc_data = (char*)arcb->rcb_rnl_cb +
				sizeof(DMP_MISC);
	    }

	    /*
	    ** Copy over position criteria in the RCB
	    */
	    arcb->rcb_f_rowaddr = rcb->rcb_f_rowaddr;
	    arcb->rcb_f_qual = rcb->rcb_f_qual;
	    arcb->rcb_f_arg = rcb->rcb_f_arg;
	    arcb->rcb_f_retval = rcb->rcb_f_retval;
	    arcb->rcb_hl_given = rcb->rcb_hl_given;
	    arcb->rcb_ll_given = rcb->rcb_ll_given;
	    MEcopy(rcb->rcb_hl_ptr, t->tcb_klen, arcb->rcb_hl_ptr);
	    MEcopy(rcb->rcb_ll_ptr, t->tcb_klen, arcb->rcb_ll_ptr);
	    /* Copy over LOB access stuff, but if we aren't qualifying,
	    ** we don't need to process LOB coupons at all.
	    */
	    arcb->rcb_bqcb_ptr = rcb->rcb_bqcb_ptr;
	    if (arcb->rcb_f_qual == NULL)
		arcb->rcb_state |= RCB_NO_CPN;
	    act[na].rcb = arcb;

	    if (! rcb->rcb_hl_given)
	    {
		/* No hi range given, position to end of table */
		status = dm2r_position(act[na].rcb, DM2R_LAST, NULL, 0,
				(DM_TID *)NULL, dberr);

		if ( status )
		    break;
	    }
	    if (t->tcb_table_type == TCB_BTREE)
	    {
		/*
		** Indicate that btree will be read backwards, starting
		** from the end of the table OR the end of the pages
		** that satisfy the hi key qualification, if any. Once
		** a record is returned from dm2r_get(), we know that
		** since dm2r_get() rejected any nonqualified rows, and
		** since DM2R_GETPREV returns data in key-decreasing
		** order, that 'MAX' has been found, and can quit scan.
		*/
	    	act[na].direction = DM2R_GETPREV;
	    	act[na].scanner = dm2r_get;
	    	act[na].break_on = DM2A_BRK_FIRST;
	  	if (rcb->rcb_hl_given)
		{
		    /* position new rcb based on hl keys saved in old rcb */
		    status = dm1b_search(arcb, rcb->rcb_hl_ptr, 
			rcb->rcb_hl_given, DM1C_RANGE, DM1C_NEXT,
                    	&arcb->rcb_other, &arcb->rcb_lowtid,
                    	&arcb->rcb_currenttid, dberr);
		    if ( status != E_DB_OK )
			break;

		    /* Save positioned information */
		    arcb->rcb_p_lowtid.tid_i4 = arcb->rcb_lowtid.tid_i4;
		    /* FIX ME this shouldnt get cleared in dm2r_position
		    or it causes a re-position always
		    arcb->rcb_pos_clean_count = DM1B_CC_INVALID;
		    */
		    arcb->rcb_p_flag = RCB_P_QUAL_NEXT;
		    arcb->rcb_state |= RCB_POSITIONED;
		}
	    }
	    else
	    {
		/* ISAM  */
	    	act[na].direction = DM2R_GETNEXT;
	    	act[na].scanner = dm2a_get_isam_backwards;
	    	act[na].break_on = DM2A_BRK_NONEXT;
	  	if (rcb->rcb_hl_given)
		{
		    /* position new rcb based on tids saved in old rcb */
		    STRUCT_ASSIGN_MACRO(rcb->rcb_hightid, arcb->rcb_hightid);
		    arcb->rcb_state |= RCB_POSITIONED;
		    arcb->rcb_hl_op_type = rcb->rcb_hl_op_type;
		}
		/* position the (starting) lowtid of new rcb based on hi */
		STRUCT_ASSIGN_MACRO(arcb->rcb_hightid, arcb->rcb_lowtid);
		/* save original start, which is now our end  */
		STRUCT_ASSIGN_MACRO(rcb->rcb_p_lowtid, arcb->rcb_p_lowtid);
	    }

	    act[na].call_ade = TRUE;	/* to accumulate max value.	*/

	    /* 
	    ** That's all for this request, increment action counter and
	    ** fall through.
	    */
	    na++;
	}
    } while (FALSE);

    if (status != E_DB_OK)
    {
	/* clean up anything here. */
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	return (status);
    } 
    else
    {
	if (DMZ_REC_MACRO(4))
	{
	    int i;

	    TRdisplay("%@\nDMF_AGG_PROC: rcb: %p  struct: %w \n",
		rcb, "0,1,2,HEAP,4,ISAM,6,HASH,8,9,10,BTREE,12,GATEWAY",
		t->tcb_table_type);

	    TRdisplay(
	    	"DMF_AGG_PROC: actions: %d  getrows: %w agglims: %v\n\n",
		na, "NO,YES", getrows, "COUNT,MAX,MIN", agglims);
	    for (i = 0; i < na; i++)
	    {
		TRdisplay("   op: %w  dir: %s  brk: %w ade: %w\n",
		    "?,TUPSCAN,PAGESCAN", act[i].operation,
		    act[i].direction == DM2R_GETNEXT ? "NEXT" : 
			act[i].direction == DM2R_GETPREV ? "PREV" : "NONE",
		    "?,NONEXT,FIRST,NEWMAINPG", act[i].break_on,
		    "NO,YES", act[i].call_ade);
		TRdisplay("   scanfunc: %s  origmainpg: %u  thisrcb: %p\n\n",
		    act[i].scanner == dm2r_get ? "r_get" :
			act[i].scanner == dm1b_count ? "b_count" :
			  act[i].scanner == dm1m_count ? "m_count" :
			    act[i].scanner == dm1r_count ? "r_count" : "???",
		    act[i].orig_mainpg, act[i].rcb);
	    }
	}  /* END TRACE POINT */
	*nacts = na;
	return (E_DB_OK);
    }
}

/*{
** Name: dm2a_get_isam_backwards - Read ISAM table in pseudo-backwards fashion.
**
** Description:
**	This function serves as a wrapper to dm2r_get() and takes the same
**	parameters as dm2r_get(), but will cause records from an ISAM
**	table to be returned in a pseudo-backwards order - that is -
**	while the data returned is not in reverse-sorted order, the data
**	is returned from succesively decreasing main-pages and their
**	overflow chains. This is used by the MAX on ISAM processing
**	to ensure that the highest range of data is seen first, with 
**	sucessively lower ranges seen. A flag passed in indicates that
**	EOF should be returned when reaching the end of the current chain,
**	to optimize the situation where a qualifying tuple has been identified,
**	and we must simply exhaust its key range for any other contenders.
**
**
** Inputs:
**		MUST BE TYPED SAME AS DM2R_GET - called interchangeably.
**
**      rcb                             The RCB to get a record from.
**      tid                             ptr to tid which is just used as a
**					indicator of whether to return EOS
**					upon end of chain - TRUE/FALSE.
**      opflag                          must be DM2R_GETNEXT
**
** Outputs:
**      record                          Pointer to location to return record.
**      err_code                        The reason for error status.

** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** Exceptions:
**	none
**
** Side Effects:
**	Tid values in the RCB are changed.
**
** Assumptions:
**	Relies on ISAM storage format, in particular, that all mainpages
**	are arranged in ascending order in the file.
**
** Related Functions:
**	Called by function ptr that is also used for dm2r_get().
**
** History:
**	05-Sep-1995 (cohmi01)
**	    Created, for aggregate optimizations.
**	06-may-1996 (thaju02)
**	    New page format support: change page header references to use
**	    macros.
*/
DB_STATUS
dm2a_get_isam_backwards(
    DMP_RCB     *r,
    DM_TID	*flagtid,
    i4     opflag,
    char	*record,
    DB_ERROR	*dberr )
{
    DM_PAGENO		pagno;
    DB_STATUS		s;		/* Variable used for return status. */
    DMP_PINFO		pinfo;
    i4		fix_action;	/* Type of access to page. */
    DM_TID		localtid;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    for (;;)
    {
	/* 
	** Try to get a qualifying tuple. May come here more than once 
	** if E_DM0055_NONEXT is returned.
	*/
    	s = dm2r_get(r, &localtid, opflag, record, dberr);

    	if (s == E_DB_OK || dberr->err_code != E_DM0055_NONEXT)
	    return (s);

	/* 
	** IF WE ARE HERE, then we must have received E_DM0055_NONEXT.
	** A E_DM0055_NONEXT return code here means that we reached the end
	** of our current ISAM main/ovflo chain. If the pseudo-tid passed
	** in is non-zero, that means our caller (dm2a_simpagg) has already
	** received 1 satisfactory tuple, so we may return E_DM0055_NONEXT.
	** Likewise, if hightid (which we have been moving backwards) is
	** equal to the saved lotid, we have exhausted all previous
	** main/ovflo chains within scan limits, and are done.
	** Otherwise, we must locate the previous main/ovflo chain and
	** start reading it. Last data page s.h.b. unfixed by dm1r_get().
	*/

	if ((flagtid->tid_tid.tid_line == TRUE) || 
	(r->rcb_hightid.tid_tid.tid_page == r->rcb_p_lowtid.tid_tid.tid_page))
	    return (s);
	    
    	fix_action = r->rcb_access_mode == RCB_A_READ ? DM0P_READ : DM0P_WRITE;

	/* Read pages backwards till previous mainpg is found.  */
	for (pagno = r->rcb_hightid.tid_tid.tid_page - 1;
	     pagno >= r->rcb_p_lowtid.tid_tid.tid_page; pagno--)
	{
	    s = dm0p_fix_page(r, pagno, fix_action, &r->rcb_data, dberr); 
	    if (s != E_DB_OK)
		return (s);

	    /* break if its a real mainpage, not an intervening ovflo page */
	    if ((DMPP_VPT_GET_PAGE_STAT_MACRO(r->rcb_tcb_ptr->tcb_rel.relpgtype,
		r->rcb_data.page) & (DMPP_DATA | DMPP_OVFL)) == DMPP_DATA)
		break;	/* got the previous main page */

	    s = dm0p_unfix_page(r, DM0P_UNFIX, &r->rcb_data, dberr);

	    if ( s != E_DB_OK )
		return (s);
	}

	if (pagno >= r->rcb_p_lowtid.tid_tid.tid_page)
	{
	    /* 
	    ** Loop ended on 'break' - got new mainpg. Set up to start
	    ** scan from that page till end of its ovflo pages.
	    */
	    r->rcb_lowtid.tid_tid.tid_page = r->rcb_hightid.tid_tid.tid_page =
		pagno;
	    r->rcb_lowtid.tid_tid.tid_line = DM_TIDEOF;
	    r->rcb_state |= (RCB_POSITIONED | RCB_WAS_POSITIONED);

	    continue;
	}
	else
	{
	    /* Loop ended when end (beginning) of scan limit was reached.  */
	    SETDBERR(dberr, 0, E_DM0055_NONEXT);
	    return (E_DB_ERROR);
	}
    }
}
