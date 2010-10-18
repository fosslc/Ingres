/*
**Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include	<iicommon.h>
#include	<dbdbms.h>
#include	<me.h>
#include	<st.h>
#include	<ddb.h>
#include	<generr.h>
#include	<sqlstate.h>
#include	<ulf.h>
#include	<cs.h>
#include	<scf.h>
#include	<ulm.h>
#include	<ulh.h>
#include	<adf.h>
#include	<dmf.h>
#include	<dmtcb.h>
#include	<dmrcb.h>
#include	<qefmain.h>
#include	<qsf.h>
#include	<qefrcb.h>
#include	<qefqeu.h>
#include	<rdf.h>
#include	<rqf.h>
#include	<rdfddb.h>
#include	<ex.h>
#include	<ulx.h>
#include	<tr.h>
#include	<psfparse.h>
#include	<rdftrace.h>
#include	<tm.h>
#include	<rdfint.h>

/**
**
**  Name: RDFUTIL.C - Utility routines for RDF.
**
**  Description:
**	This file contains the utility routines for RDF.
**
**	rdu_init_info()	- Initilaize table information block.
**	rdu_chk_status() - RDF error handling routine
**	rdu_info_dump - RDF table information block dump.
**	rdu_rel_dump - RDF relation informatin dump.
**	rdu_his_dump - RDF histogram information dump.
**	rdu_ituple_dump - RDF integrity tuples dump.
**	rdu_ptuple_dump - RDF protection tuples dump.
**	rdu_attr_dump - RDF attributes informatin dump.
**	rdu_indx_dump - RDF index informatin dump.
**	rdu_keys_dump - RDF primary key informatin dump.
**	rdu_atthsh_dump - RDF hash table of attributes informatin dump.
**	rdu_treenode_to_tuple - PSF  treenode to tuples conversion routine.
**	rdu_qrytext_to_tuple - PSF querytext to tuples conversion routine.
**	rdu_qttuple_to_text - PSF iiqrytext tuples to query text conversion
**	rdu_ttuple_to_node - PSF iitree tuples to query tree nodes conversion
**	rdu_malloc      RDF allocate memory
**
**
**  History:
**      10-apr-86 (ac)
**          written
**	25-feb-87 (puree)
**	    modify rdu_chk_status() for ULH and SCF errors.
**      11-may-87 (puree)
**	    add rdu_malloc for memory allocation.
**	06-feb-89 (ralph)
**	    change rdu_[ip]tuple_dump to use DB_COL_WORDS when dumping
**	    domset arrays from iiintegrity/iiprotect catalogs
**	10-may-89 (neil)
**	    Initialized rdr_rtuples as well.
**	20-dec-91 (teresa)
**	    added include of header files ddb.h, rqf.h and rdfddb.h for sybil
**	16-jul-92 (teresa)
**	    added prototype support.
**	31-jul-92 (rickh)
**	    FIPS CONSTRAINTS:  Removed reference to dbr_column.
**	09-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Use rds_cat_owner rather than "$ingres" in rdu_master_infodump()
**	06-jul-93 (teresa)
**	    change include of dbms.h to include of gl.h, sl.h, iicommon.h &
**	    dbdbms.h.
**	26-jul-1993 (rog)
**	    Removed definition of EX_MAX_SYS_REP.
**	11-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in rdu_info_dump(),
**	    rdu_view_dump().
**	28-Oct-1996 (hanch04)
**	    Changed SS50000_MISC_ING_ERRORS to SS50000_MISC_ERRORS
**	28-may-1997 (shero03)
**	    Reverse the meaning for RDF_CHECK_SUM
**	05-Jun-1997 (shero03)
**	    Use ulm_palloc only when the object is private,
**	    otherwise use ulm_spalloc.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	15-Aug-1997 (jenjo02)
** 	    New field, ulm_flags, added to ULM_RCB in which to specify the
**	    type of stream to be opened, shared or private.
**	    Semaphore calls are now made directly to the CL instead of going
**	    thru SCF. Handle errors from the CL in rdu_ferror().
**      16-mar-1998 (rigka01)
**	    Cross integrate change 434809 from OI 1.2 to OI 2.0
**          bug 89413 - When threads are locked out of using shared stream,
**          private stream is allocated but use of private stream causes
**          access violation.
**          The exception occurs when releasing the shared ulh object semaphore
**          when memory is low.  rdu_private was obtaining the semaphore
**          before attempting to get the memory for the private ulh_object
**          and memory allocation was failing.  The fix is to move the get
**          for the semaphore after all gets of memory needed by this routine.
**	22-jul-1998 (shust01)
** 	    Backed out change 434936 (2.0) for bug 89413.  Memory allocations
** 	    done via rdu_malloc() should be done after getting semaphore.
** 	    Can potentially have caused SEGVs in PSF (among other areas). bug 92059.
**      09-may-00 (wanya01)
**          Bug 99982: Server crash with SEGV in rdf_unfix() and/or psl_make
**          _user_default().
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
**      21-feb-2001 (rigka01)
**          Bug 103486: more server crashes in rdf_unfix() and/or psl_make_
**          user_default(). info block indicates default information is
**          available but pointer to the info is null.
**	21-Mar-2003 (jenjo02)
**	    Cleaned up rdf_state "&" tests; rdf_state is an integral
**	    value, not a bit mask.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value DB_ERROR structure.
**	    Pass pointer to facilities DB_ERROR instead of just its err_code
**	    in rdu_ferror().
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**      20-aug-2010 (stial01)
**          Fixed E_UL0005_NOMEM/rdu_mrecover() err handling.
**          rdu_mrecover() don't assume error is in global->rdf_ulmcb.
**      01-oct-2010 (stial01) (SIR 121123 Long Ids)
**          Store blank trimmed names in DMT_ATT_ENTRY
**/

/* Static Prototypes */
static DB_ERROR * rdu_gerrblk(RDF_GLOBAL    *global);

/* debug only dump routines */
#ifdef xDEV_TEST
static VOID rdu_ldbdesc_dump(RDR_INFO	*info);
static VOID ldbdesc_dmp(DD_LDB_DESC *ldbdesc);
static VOID ldbplus_dmp(DD_0LDB_PLUS *ldbplus);
static VOID rdu_col_dump(i4 col_cnt, DD_COLUMN_DESC **cols);
#endif

/* Trace message length */
#define	RDF_MSGLENGTH		80

/* Global References */
GLOBALREF    i4 RDF_privctr;	    /* used with RD0030 trace*/

GLOBALREF const RDF_CALL_INFO Rdf_call_info[];	/* RDF operation info */


/*{
** Name: rdu_currtime	- get current time
**
** Description:
**      This routine calls TMnow for current time
**
** Inputs:
**
** Outputs:
**      time_stamp                      current time
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      05-apr-89 (mings)
**          initial creation
[@history_template@]...
*/
VOID
rdu_currtime(
    DB_TAB_TIMESTAMP    *time_stamp)
{
    SYSTIME		sys_time;
    FUNC_EXTERN VOID	TMnow();
    /* call TMnow */

    TMnow(&sys_time);

    time_stamp->db_tab_high_time = sys_time.TM_secs;
    time_stamp->db_tab_low_time = sys_time.TM_msecs;
}

/*{
** Name: rdu_setname	- create a name for the ULH hash object
**
** Description:
**      This routine creates a name for the ULH hash object
**
** Inputs:
**      global                          ptr to RDF global state variable
**      table_ptr                       ptr to the table name
**
** Outputs:
**      global->ulh_namelen             length of the ULH hash object name
**      global->ulh_objname             ULH object name
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-oct-87 (seputis)
**          initial creation
**	09-apr-90 (teg)
**	    synonyms support by checking to see if object already built
**	    before building it.
**	6-Jul-2006 (kschendel)
**	    dmf db id's and unique db id's aren't the same!  Just use the
**	    unique db id.
[@history_template@]...
*/
VOID
rdu_setname(
    RDF_GLOBAL         *global,
    DB_TAB_ID          *table_id)
{
    char	    *obj_name;	    /* ptr to ULH object name to be built */

    if ( !(global->rdf_init & RDF_INAME) )
    {
	obj_name = (char *)&global->rdf_objname[0];
	MEcopy((PTR)table_id, sizeof(DB_TAB_ID), (PTR)obj_name);
	obj_name += sizeof(DB_TAB_ID);
	MEcopy((PTR)&global->rdfcb->rdf_rb.rdr_unique_dbid,
	    sizeof(i4),
	    (PTR)obj_name);
	global->rdf_namelen = sizeof(DB_TAB_ID) + sizeof(i4);
	global->rdf_init |= RDF_INAME;	    /* indicates that the ULH name
						** has been initialized */
    }
}

/*{
** Name: rdu_init_info - Initilaize table information block.
**
**	Internal call format:	rdu_init_info(rdf_cb->rdf_info_blk);
**
** Description:
**      This function initializes the table information block.
**
** Inputs:
**
**      rdf_cb
**		.rdf_info_blk		pointer to the table information block.
**
** Outputs:
**      rdf_cb
**		.rdf_info_blk		pointer to the table information block.
**
**	Returns:
**		none
**
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	10-mar-86 (ac)
**          written
**	10-may-89 (neil)
**	    Initialized rdr_rtuples as well.
**	27-may-92 (teresa)
**	    initialize star-specific fields and new sybil field rdr_star_obj
**	29-jul-92 (teresa)
**	    get DDBID from RDF_SESS_CB instead of expecting a ptr to RDF_CB in
**	    global.
**	01-mar-93 (teresa)
**	    MEfill infoblk to take care of initing various fields that must
**	    be set to zero.  [Note: I'm still setting the ptrs to null (rather
**	    than zerofilling) because I've been told that on some platforms
**	    NULL is -1 instead of zero.]
**	01-nov-1998 (nanpr01)
**	    Initialize all fields and donot do MEFill .
**	29-Dec-2003 (schka24)
**	    Init the partitioned table stuff.
**	15-Mar-2003 (schka24)
**	    Really init the partitioned table stuff (!), put mefill back.
*/
VOID
rdu_init_info(  RDF_GLOBAL	*global,
		RDR_INFO	*rdf_info_blk,
		PTR             streamid)
{
    RDF_SESS_CB	*sess_cb;

    MEfill(sizeof(RDR_INFO), 0, rdf_info_blk);

    rdu_currtime(&rdf_info_blk->rdr_timestamp);
    rdf_info_blk->stream_id = streamid;

    /* get the RDF session control block to obtain distributed ddb id, in
    ** case this is a distributed session.
    */
    sess_cb = global->rdf_sess_cb;

    if (sess_cb->rds_distrib)
	rdf_info_blk->rdr_ddb_id = sess_cb->rds_ddb_id;
    else
	rdf_info_blk->rdr_ddb_id = NULL;

}

/*{
** Name: rdu_gerrblk	- get ptr to proper error block for RDF
**
** Description:
**      Since rdf_call can be entered with two types of control blocks
**      (unfortunately) the routine will determine which type has be used
**      and return a ptr to the DB_ERROR struct to update
**
** Inputs:
**      global                          ptr to RDF state variable
**
** Outputs:
**	Returns:
**	    ptr to DB_ERROR to update
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-nov-87 (seputis)
**          initial creation
**	02-oct-92 (teresa)
**	    added handling for RDF_END_SESSION and RDF_DDB
**	15-feb-93 (teresa)
**	    add RDF_READTUPLES operation
**	25-Mar-2004 (schka24)
**	    Forgot to update magic switch, get rid of it instead.
[@history_template@]...
*/
static DB_ERROR *
rdu_gerrblk( RDF_GLOBAL	*global)
{
    DB_ERROR	*rdf_error;

    if (Rdf_call_info[global->rdf_operation].cb_type == RDF_CCB_TYPE)
	rdf_error = &global->rdfccb->rdf_error;
    else if (Rdf_call_info[global->rdf_operation].cb_type == RDF_CB_TYPE)
	rdf_error = &global->rdfcb->rdf_error;
    else
    {

	TRdisplay("invalid operation code detected in RDF\n ");
	rdf_error = NULL;	    /* we'll segv soon enough... */
    }
    return(rdf_error);
}

/*{
** Name: rdu_sccerror	- report error to user
**
** Description:
**      Report a message to the user
**
** Inputs:
**      status                          status of error
**      msg_buffer                      ptr to message
**      error                           ingres error number
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    fatal status will cause query to be aborted, and previous text sent
**          to user to be flushed
**
** History:
**      15-apr-87 (seputis)
**          initial creation
**	21-may-89 (jrb)
**	    renamed "error" parm to "local_error" and added "generic_error" parm
**	24-oct-92 (andre)
**	    changed interface to receive sqlstate instead of generic error +
**	    in SCF_CB, generic_error has been replaced with sqlstate
**	13-nov-92 (andre)
**	    In rdu_sccerror(), if the caller has not passed sqlstate,
**	    set scf_sqlstate to MISC_ING_ERROR.
**	3-Jan-2004 (schka24)
**	    make global

[@history_template@]...
*/
DB_STATUS
rdu_sccerror(	DB_STATUS	   status,
		DB_SQLSTATE	   *sqlstate,
		RDF_ERROR          local_error,
		char               *msg_buffer,
		i4                msg_length)
{
    SCF_CB                 scf_cb;
    DB_STATUS              scf_status;

    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_RDF_ID;
    scf_cb.scf_nbr_union.scf_local_error = local_error;
    if (sqlstate)
	STRUCT_ASSIGN_MACRO((*sqlstate), scf_cb.scf_aux_union.scf_sqlstate);
    else
	MEcopy((PTR) SS50000_MISC_ERRORS, DB_SQLSTATE_STRING_LEN,
	    (PTR) scf_cb.scf_aux_union.scf_sqlstate.db_sqlstate);
    scf_cb.scf_len_union.scf_blength = msg_length;
    scf_cb.scf_ptr_union.scf_buffer = msg_buffer;
    scf_cb.scf_session = DB_NOSESSION;
    if (DB_SUCCESS_MACRO(status))
	scf_status = scf_call(SCC_TRACE, &scf_cb);
    else
	scf_status = scf_call(SCC_ERROR, &scf_cb);
    if (scf_status != E_DB_OK)
    {
	TRdisplay("SCF error displaying RDF message to user\n");
	TRdisplay("RDF message is :%s",msg_buffer);
    }
    return (scf_status);
}

/*{
** Name: rduWarnFcn	- this routine will report an RDF warning.
**
** Description:
**      This routine will log and/or report an RDF warning.  The RDF
**	warning means that performance has been degraded, but processing
**	can continue to a successful completion.
**
** Inputs:
**      global				ptr to RDF state variable
**      warn				RDF warning code to log
**	FileName			Filename of function caller.
**	LineNumber			Line number within that file.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	03-may-90 (teg)
**	    created for SYNONYMS.
**	24-oct-92 (andre)
**	    ule_format() expects a (DB_SQLSTATE *) as does rdu_sccerror()
**	10-Sep-2008 (jonj)
**	    Renamed to rduWarnFcn, called from rdu_warn macro.
[@history_line@]...
*/
VOID
rduWarnFcn( RDF_GLOBAL	*global,
	    RDF_ERROR	warn,
	    PTR		FileName,
	    i4		LineNumber)
{
    DB_STATUS	    ule_status;		/* return status from ule_format */
    i4         ule_error;          /* ule cannot format message */
    char	    msg_buffer[DB_ERR_SIZE]; /* error message buffer */
    i4         msg_buf_length;     /* length of message returned */
    DB_SQLSTATE	    sqlstate;
    DB_ERROR	localDBerror;

    localDBerror.err_code = warn;
    localDBerror.err_data = 0;
    localDBerror.err_file = FileName;
    localDBerror.err_line = LineNumber;


    ule_status = uleFormat( &localDBerror, 0, NULL, ULE_LOG, &sqlstate, msg_buffer,
	(i4) (sizeof(msg_buffer)-1), &msg_buf_length, &ule_error, 0);
    if (ule_status != E_DB_OK)
    {
        STprintf(msg_buffer,
	    "ULE error = %x, RDF warning message cannot be found:  %x\n",
	    ule_error, warn);
	TRdisplay("%s", msg_buffer);
    }
    else
	msg_buffer[msg_buf_length] = 0;	/* null terminate */

    (VOID) rdu_sccerror(E_DB_WARN, &sqlstate, warn,
			msg_buffer, (i4)msg_buf_length);
}

/*{
** Name: rduIerrorFcn	- this routine will report the RDF error
**
** Description:
**      This routine will log and/or report a RDF error.
**
** Inputs:
**      global				ptr to RDF state variable
**      error                           RDF error code to log
**	FileName			Filename of function caller.
**	LineNumber			Line number within that file.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	3-jul-86 (seputis)
**          initial creation
**	21-may-89 (jrb)
**	    Now passes generic error to rdu_sccerror as well as local error.
**	24-oct-92 (andre)
**	    ule_format() expects a (DB_SQLSTATE *) as does rdu_sccerror()
**	09-Sep-2008 (jonj)
**	    Name changed to rduIerrorFcn, invoked by rdu_ierror macro to
**	    provide __FILE__ and __LINE__ of error source.
[@history_line@]...
*/
VOID
rduIerrorFcn( RDF_GLOBAL         *global,
	    DB_STATUS		status,
	    RDF_ERROR		local_error,
	    PTR			FileName,
	    i4			LineNumber)
{
    DB_STATUS	    ule_status;		/* return status from ule_format */
    i4         ule_error;          /* ule cannot format message */
    char	    msg_buffer[DB_ERR_SIZE]; /* error message buffer */
    i4         msg_buf_length;     /* length of message returned */
    i4         log;                /* should this error be logged */
    DB_ERROR	    *rdf_error;
    DB_SQLSTATE	    sqlstate;

    rdf_error = rdu_gerrblk(global);
    if (!rdf_error)
	return;				/* return if bad parameter since we
					** do not know the control block type */
    /* Populate error source information */
    rdf_error->err_code = local_error;
    rdf_error->err_data = 0;
    rdf_error->err_file = FileName;
    rdf_error->err_line = LineNumber;

    if (DB_SUCCESS_MACRO(status) || (local_error > E_RD01FF_RECOVERABLE_ERRORS))
	return;				/* return if warning or info is
					** found , or if a recoverable
                                        ** error is found */
    if (local_error < E_RD0100_LOG_ERRORS)
	log = ULE_LOOKUP;
    else
	log = ULE_LOG;

    ule_status = uleFormat( rdf_error, 0, NULL, log, &sqlstate, msg_buffer,
	(i4) (sizeof(msg_buffer)-1), &msg_buf_length, &ule_error, 0);
    if (ule_status != E_DB_OK)
    {
        STprintf(msg_buffer,
	    "ULE error = %x, RDF message from %s:%d cannot be found - error no. = %x\n",
	    ule_error, FileName, LineNumber, local_error );
	TRdisplay("%s", msg_buffer);
    }
    else
	msg_buffer[msg_buf_length] = 0;	/* null terminate */
    if (local_error != E_RD000C_USER_INTR)  /* do not report control C errors
					    ** to the user */
    {
	(VOID) rdu_sccerror(E_DB_ERROR,
				&sqlstate,
				local_error,
				msg_buffer,
				(i4)msg_buf_length);
    }
}

/*{
** Name: rdu_ferror - RDF error handling routine
**
**	Internal call format:	rdu_ferror(global, status, err_code from
**	callee, rdf_error)
**
** Description:
**
**      This function handle the errors returned from the services routines
**	called by RDF.
**
**	It maps the external facility error code to an RDF error code.  It
**	copies the external facility error to rdf_error->err_data.  Then it
**	calls ule_format to format/log the RDF error.  Then it calls
**	rdu_scerror to cause the formatted error message to be output to the
**	user terminal.  After reporting the RDF mapped error, it reports the
**	original facility error via ule_format and then rdu_scerror.
**
**	There is a mechanism to supress printing an expected error to the
**	user terminal and to the log file:  To suppress an external facility
**	message and the corresponding RDF message, specify input value
**	'suppress' as the value of the expected error message.
**
**	    EXAMPLE:  when dropping a permit the user may specify a
**		      nonexistent permit number.  This will cause QEF
**		      to output E_QE0015.  RDF wil translate this to
**		      E_RD0013.  It will log both errors and print both
**		      errors to the user terminal.  Finally PSF will
**		      translate E_RD0013 into user error E_US1454.
**		      Bug 4390 was generated, requesting that E_QE0015 and
**		      E_RD0013 be supressed, and only E_US1454 be reported.
**
**		      These messages can be suppressed by calling rdu_ferror
**		      with supress=E_QE0015.
**
**		      Now RDF will put E_QE0015 and E_RD0013 into the
**		      appropriate locations of the RDF error control block,
**		      but will skip logging these messages to
**		      errlog.log, and will NOT call rdu_scerror to report
**		      the error to the user terminal.  The user will see
**		      only the E_US1454 message.
**
**		      However, if rdu_ferror were called with suppress = 0,
**		      rdu_ferror would cause both E_QE0015 and E_RD0013 to be
**		      logged and printed to the user terminal.
**
**	There is some logic that is if-deffed out.  That logic would suppress
**	printing error messages less that RD0128 to the error log file, but
**	would cause the error to be printed to the user terminal.
**	Unfortunately, the RDF error messages would need to be renumbered for
**	this to work correctly -- some of the messages less than 128 should not
**	be suppressed.  Changing the error message numbers is fairly non-trivial
**	so this remains ifdeffed out.
**
** Inputs:
**      global				RDF local varibles for this session
**	status				Error Status value returned from
**					    the external facility call
**					    (E_DB_OK,E_DB_WARN,E_DB_ERROR,etc)
**	ferror				DB_ERROR pointer of error returned from
**					    external facility called.
**	interror			Internal Error Code (or RDF error) that
**					    will be returned if rdu_ferror is
**					    unable to map err_code.
**	suppress			Expected err_code where output of msg
**					    to user terminal should be
**					    suppressed.  IF NULL, output is
**					    not suppressed.
**
** Outputs:
**	none
**	Returns:
**		none
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	10-apr-86 (ac)
**          written
**	10-apr-87 (puree)
**	    add ULH_CALL and SCF_CALL error codes.
**	11-nov-87 (seputis)
**          rewritten for resource handling
**	11-aug-88 (seputis)
**          added RD002A for deadlock errors
**	7-nov-88 (seputis)
**          change RDF error message
**	21-may-89 (jrb)
**	    Now gets generic error code from ule_format and passes to
**	    rdu_sccerror.
**      17-apr-90 (teg)
**          added logic to suppress specified messages.
**	11-nov-91 (teresa)
**	    Picked up INGRES63p changes 262778 and 260973:
**	    03-feb-91 (teresa)
**		pre-integrate changes seputis made to implement OPF counting
**		semaphores (ie, lock_timeout messages)
**	16-apr-92 (teresa)
**	    Pick up bryanp change from 6.4 line:
**	    13-apr-1992 (bryanp)
**		B38326: Handle E_DM0042_DEADLOCK error from dmf_call(DMT_SHOW).
**	27-jul-92 (andre)
**	    handle E_QE025[0-2]
**	28-sep-92 (andre)
**	    having translated E_QE025[0-2] into E_RD0210, return, since QEF has
**	    already logged the message and PSF will display its own message for
**	    the user
**	24-oct-92 (andre)
**	    ule_format() expects a (DB_SQLSTATE *) as does rdu_sccerror()
**	20-feb-93 (andre)
**	    added code to handle E_QE0255 and E_QE0256
**	28-mar-93 (andre)
**	    will handle E_QE0257_ABANDONED_CONSTRAINT in the same way as msgs
**	    E_QE025[0-2]
**	22-oct-93 (andre)
**	    add code to translate E_QE011C_NONEXISTENT_SYNONYM into
**	    E_RD014A_NONEXISTENT_SYNONYM and return (QEF has already logged the
**	    message and PSF will provide user with an informative message)
**	15-Aug-1997 (jenjo02)
**	    Semaphore calls are now made directly to the CL instead of going
**	    thru SCF. Handle errors from the CL.
**	17-Jan-2003 (jenjo02)
**	    Handle E_DM006B_NOWAIT_LOCK_BUSY the same as
**	    E_DM004D_LOCK_TIMER_EXPIRED.
**	12-Feb-2003 (jenjo02)
**	    Delete DM006B, E_DM004D_LOCK_TIMER_EXPIRED is now
**	    returned instead.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Caller supplies pointer to facilities DB_ERROR rather
**	    than just its err_code.
*/
VOID
rdu_ferror( RDF_GLOBAL	*global,
	    DB_STATUS	status,
	    DB_ERROR	*ferror,
	    RDF_ERROR	interror,
	    RDF_ERROR   suppress)
{
    DB_ERROR	    *rdf_error;

    rdf_error = rdu_gerrblk(global);
    /* need to break up the error code by facility for compilers which
    ** cannot handle sparse switch statements */
    SETDBERR(rdf_error, 0, interror);
    if ((ferror->err_code >> 16) == DB_ULF_ID)
    {
	switch (ferror->err_code)
	{
	    case E_UL0004_CORRUPT:
		SETDBERR(rdf_error, 0, E_RD0006_MEM_CORRUPT);
		break;

	    case E_UL0005_NOMEM:
		SETDBERR(rdf_error, 0, E_RD0001_NO_MORE_MEM);
		break;

	    case E_UL0009_MEM_SEMWAIT:
		SETDBERR(rdf_error, 0, E_RD0008_MEM_SEMWAIT);
		break;

	    case E_UL000A_MEM_SEMRELEASE:
		SETDBERR(rdf_error, 0, E_RD0009_MEM_SEMRELEASE);
		break;

	    case E_UL000B_NOT_ALLOCATED:
		SETDBERR(rdf_error, 0, E_RD0007_MEM_NOT_OWNED);
		break;

	    case E_UL0006_BADPTR:
	    case E_UL0007_CANTFIND:
		SETDBERR(rdf_error, 0, E_RD000A_MEM_NOT_FREE);
		break;

	    case E_UL0101_TABLE_ID:
		SETDBERR(rdf_error, 0, E_RD0041_BAD_FCB);
		break;

	    case E_UL0103_NOT_EMPTY:
		SETDBERR(rdf_error, 0, E_RD0042_CACHE_NOT_EMPTY);
		break;

	    case E_UL0104_FULL_TABLE:
	    case E_UL0110_CHDR_MEM:
	    case E_UL0111_OBJ_MEM:
		SETDBERR(rdf_error, 0, E_RD0043_CACHE_FULL);
		break;

	    case E_UL0106_OBJECT_ID:
		SETDBERR(rdf_error, 0, E_RD0044_BAD_INFO_BLK);
		break;

	    case E_UL0119_ALIAS_MEM:
		/* this is caused because the RDF startup parameters for
		** number of tables or number of alias per table.  PSF will
		** print a nice user message explaining how to reconfigure
		** RDF at server startup.  So, just return */
		SETDBERR(rdf_error, 0, E_RD0145_ALIAS_MEM);
		return;

	    default:
		break;
	}
    }
    else if ((ferror->err_code >> 16) == DB_DMF_ID)
    {
	switch (ferror->err_code)
	{
	    case E_DM0042_DEADLOCK:
		SETDBERR(rdf_error, 0, E_RD002A_DEADLOCK);
		break;

	    case E_DM0065_USER_INTR:
		SETDBERR(rdf_error, 0, E_RD000C_USER_INTR);
		break;

	    case E_DM0064_USER_ABORT:
		SETDBERR(rdf_error, 0, E_RD000D_USER_ABORT);
		break;

	    case E_DM0054_NONEXISTENT_TABLE:
		SETDBERR(rdf_error, 0, E_RD0002_UNKNOWN_TBL);
		return;			    /* this is an expected error
					    ** which can be recovered from
					    ** by the caller so do not
					    ** log or send message to user */
	    case E_DM0112_RESOURCE_QUOTA_EXCEED:
		SETDBERR(rdf_error, 0, E_RD001A_RESOURCE_ERROR);
		return;			    /* exceeded some internal DMF
					    ** resource */
	    case E_DM0114_FILE_NOT_FOUND:
		SETDBERR(rdf_error, 0, E_RD0021_FILE_NOT_FOUND);
		break;

	    case E_DM004D_LOCK_TIMER_EXPIRED:
		SETDBERR(rdf_error, 0, E_RD002B_LOCK_TIMER_EXPIRED);
		if (global->rdfcb->rdf_rb.rdr_2types_mask & RDR2_TIMEOUT)
		    return;		/* this error was due to time out
					** specification by the caller, so
					** that retry by caller could be
					** attempted */
		else
		    break;

	    default:
		break;
	}
    }
    else if ((ferror->err_code >> 16) == DB_QEF_ID)
    {
	switch (ferror->err_code)
	{
	    case E_QE0002_INTERNAL_ERROR:
		SETDBERR(rdf_error, 0, E_RD0010_QEF_ERROR);
		break;

	    case E_QE0011_AMBIGUOUS_REPLACE:
		SETDBERR(rdf_error, 0, E_RD0015_AMBIGUOUS_REPLACE);
		break;

	    case E_QE0015_NO_MORE_ROWS:
		SETDBERR(rdf_error, 0, E_RD0013_NO_TUPLE_FOUND);
		break;

	    case E_QE0017_BAD_CB:
	    case E_QE0018_BAD_PARAM_IN_CB:
		SETDBERR(rdf_error, 0, E_RD0020_INTERNAL_ERR);
		break;

	    case E_QE0022_QUERY_ABORTED:
		SETDBERR(rdf_error, 0, E_RD000D_USER_ABORT);
		break;

	    case E_QE0024_TRANSACTION_ABORTED:
		SETDBERR(rdf_error, 0, E_RD0050_TRANSACTION_ABORTED);
		break;

	    case E_QE0025_USER_ERROR:
		SETDBERR(rdf_error, 0, E_RD0025_USER_ERROR);
		return;			/* QEF has already reported back
                                        ** to user, this will typically be
                                        ** a deadlock, or transaction
                                        ** aborted , do not log or send
                                        ** other messages to user to override
                                        ** QEF message */

	    case E_QE002A_DEADLOCK:
		SETDBERR(rdf_error, 0, E_RD002A_DEADLOCK);
		break;

	    case E_QE002B_TABLE_EXISTS:
		SETDBERR(rdf_error, 0, E_RD0017_TABLE_EXISTS);
		break;

	    case E_QE0027_BAD_QUERY_ID:
		SETDBERR(rdf_error, 0, E_RD0014_BAD_QUERYTEXT_ID);
		break;

	    case E_QE0031_NONEXISTENT_TABLE:
		SETDBERR(rdf_error, 0, E_RD0018_NONEXISTENT_TABLE);
		break;

	    case E_QE0036_LOCK_TIMER_EXPIRED:
		SETDBERR(rdf_error, 0, E_RD002B_LOCK_TIMER_EXPIRED);
		if (global->rdfcb->rdf_rb.rdr_2types_mask & RDR2_TIMEOUT)
		    return;		/* this error was due to time out
					** specification by the caller, so
					** that retry by caller could be
					** attempted */
		else
		    break;

	    case E_QE004A_KEY_SEQ:
		SETDBERR(rdf_error, 0, E_RD0016_KEY_SEQ);
		break;

	    case E_QE0051_TABLE_ACCESS_CONFLICT:
		SETDBERR(rdf_error, 0, E_RD0019_TABLE_ACCESS_CONFLICT);
		break;

	    case E_QE0250_ABANDONED_PERMIT:
	    case E_QE0251_ABANDONED_VIEW:
	    case E_QE0252_ABANDONED_DBPROC:
	    case E_QE0257_ABANDONED_CONSTRAINT:
		SETDBERR(rdf_error, 0, E_RD0210_ABANDONED_OBJECTS);
		return;

	    case E_QE0255_NO_SCHEMA_TO_DROP:
		SETDBERR(rdf_error, 0, E_RD0211_NONEXISTENT_SCHEMA);
		return;

	    case E_QE0256_NONEMPTY_SCHEMA:
		SETDBERR(rdf_error, 0, E_RD0212_NONEMPTY_SCHEMA);
		return;

	    case E_QE011C_NONEXISTENT_SYNONYM:
		SETDBERR(rdf_error, 0, E_RD014A_NONEXISTENT_SYNONYM);
		return;

	    default:
		break;
	}
    }
    else if ((ferror->err_code >> 16) == DB_CLF_ID)
    {
	switch (ferror->err_code)
	{
	    case E_CS0004_BAD_PARAMETER:
	    case E_CS000A_NO_SEMAPHORE:
		SETDBERR(rdf_error, 0, E_RD0031_SEM_INIT_ERR);
		break;

	    case E_CS0017_SMPR_DEADLOCK:
		SETDBERR(rdf_error, 0, E_RD0032_NO_SEMAPHORE);
		break;
	    default:
		break;
	}
    }
    {
	DB_STATUS	ule_status;	    /* return status from ule_format */
	i4         ule_error;          /* ule cannot format message */
	char		msg_buffer[DB_ERR_SIZE]; /* error message buffer */
	i4         msg_buf_length;     /* length of message returned */
	i4         log;                /* logging state */
	DB_SQLSTATE	sqlstate;	    /* SQLSTATE status code */

	/* see if the calling routine has requested this message be suppressed*/
	if (ferror->err_code == suppress)
	    return;

#if 0
log all messages for now so that DMT_SHOW errors etc. are reported.
	if ((interror % 256) < 128)
	    log = ULE_LOOKUP;		/* lookup only user errors with range
					    ** of 0-127 */
	else
#endif
	    log = ULE_LOG;			/* log errors with range 128-255 */

	ule_status = uleFormat( rdf_error, 0, NULL, log, &sqlstate,
	    msg_buffer, (i4) (sizeof(msg_buffer)-1), &msg_buf_length,
	    &ule_error, 0);
	if (ule_status != E_DB_OK)
	{
	    STprintf(msg_buffer,
    "ULE error = %x\nRDF cannot find error no = %x Status = %x Facility error = %x \n",
		ule_error, rdf_error->err_code , status, ferror->err_code);
	}
	else
	    msg_buffer[msg_buf_length] = 0;	/* null terminate */
	/* FIXME - should check possible return status and generate a severe error
	** unless it is simple (like cannot find message) */
	{   /* report any errors in the range 0 to 127 to the user */
	    DB_STATUS		scf_status;
	    scf_status = rdu_sccerror(status, &sqlstate,
		    ferror->err_code,  msg_buffer, (i4)msg_buf_length);
	    if (scf_status != E_DB_OK)
	    {
		TRdisplay(
		    "RDF error = %x Status = %x Facility error = %x \n",
		    rdf_error->err_code , status, ferror->err_code);
	    }
	}
	ule_status = uleFormat( ferror, 0, NULL, log, &sqlstate,
	    msg_buffer, (i4) (sizeof(msg_buffer)-1), &msg_buf_length,
	    &ule_error, 0);
	if (ule_status != E_DB_OK)
	{
	    STprintf(msg_buffer,
    "ULE error = %x\nRDF error = %x Status = %x Facility error cannot be found - error no = %x \n",
		ule_error,  rdf_error->err_code, status, ferror->err_code);
	}
	else
	    msg_buffer[msg_buf_length] = 0;	/* null terminate */
	{
	    DB_STATUS		scf1_status;
	    scf1_status = rdu_sccerror(status, &sqlstate,
		ferror->err_code, msg_buffer, (i4)msg_buf_length);
	    if (scf1_status != E_DB_OK)
	    {
		TRdisplay(
		    "RDF error = %x Status = %x Facility error = %x \n",
		    rdf_error->err_code , status, ferror->err_code);
	    }
	}
    }
}

/*{
** Name: rdu_mrecover	- recover from a memory error
**
** Description:
**      This routine will attempt to recover from an out of memory error by
**      call the garbage collection routines
**
** Inputs:
**      global                          ptr to RDF global state variable
**
** Outputs:
**	Returns:
**	    DB_STATUS
**		error if original error wasn't "out of memory",
**		or if nothing found to free, or reclaim error.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      11-nov-87 (seputis)
**          initial creation
**      17-apr-90 (teresa)
**	    added new parameter to rdu_ferror() for bug 4390.
**	01-feb-91 (teresa)
**	    fix uninitialized "status" bug when called by OPF to reclaim memory.
**	    Also, add statistics about memory reclaim.
**	27-may-92 (teresa)
**	    Added support for LDBdesc cache for Sybil.
**	21-may-93 (teresa)
**	    Typecast parameter of ulh_destroy() to match prototype.
**	12-feb-99 (stephenb)
**	    Propagate natjo01 6.4 change to init status
**	1-Jan-2004 (schka24)
**	    Make global so rdf_part can use it.
[@history_template@]...
*/
DB_STATUS
rdu_mrecover(	RDF_GLOBAL         *global,
		ULM_RCB		   *ulm_rcb,
		DB_STATUS           orig_status,
		RDF_ERROR	    errcode)
{
    DB_STATUS	    status = E_DB_ERROR;
    RDI_FCB	    *rdi_fcb;	    /* ptr to facility control block
				    ** initialized at server startup time
				    */
    ULH_RCB	    ulh_rcb;	    /* the reclaimation routine for
				    ** ulh requires this control block*/

    rdi_fcb = (RDI_FCB *)(global->rdfcb->rdf_rb.rdr_fcb);

    if (ulm_rcb->ulm_error.err_code == E_UL0005_NOMEM)
    {
	do	/* control loop */
	{

	    /* Is RDF building the LDBdesc cache ? */
	    if (global->rdf_resources & RDF_BLD_LDBDESC)
	    {
		/* RDF was attempting to allocate LDBdesc cache memory, so try
		** to free up some unfixed object there first
		*/
		ulh_rcb.ulh_hashid = Rdi_svcb->rdv_dist_hashid;
		ulh_rcb.ulh_object = (ULH_OBJECT *) NULL;
		status = ulh_destroy(&ulh_rcb, (unsigned char *)NULL, (i4) 0);

		/* stop if we succeed or if we fail with any error other than
		** no memory to reclaim
		*/
		if (DB_SUCCESS_MACRO(status))
		{
		    /* update statistics */
		    Rdi_svcb->rdvstat.rds_n11_LDBmem_claim++;
		    break;
		}
		else if (ulh_rcb.ulh_error.err_code != E_UL0109_NFND)
		    break;
	    }
	    /* Is is reasonable to reclaim memory from the QTREE cache ? */
	    if (rdi_fcb->rdi_fac_id == DB_PSF_ID)
	    {
		/*
		** Either the memory error is NOT from the LDBdesc cache or
		** we were unable to reclaim the desired memory from that cache.
		** In either case, only PSF uses query trees, so  ignore this
		** hash table unless called by PSF.
		*/
		ulh_rcb.ulh_hashid = Rdi_svcb->rdv_qtree_hashid;
		ulh_rcb.ulh_object = (ULH_OBJECT *) NULL;
		status = ulh_destroy(&ulh_rcb, (unsigned char *)NULL, (i4) 0);
		    /* reclaim memory occupied by unfixed parse trees prior to
		    ** reclaiming relation descriptors
		    */

		/* stop if we succeed or if we fail with any error other
		** than no memory to reclaim
		*/
		if (DB_SUCCESS_MACRO(status))
		{
		    /* update statistics */
		    Rdi_svcb->rdvstat.rds_n8_QTmem_claim++;
		    break;
		}
		else if (ulh_rcb.ulh_error.err_code != E_UL0109_NFND)
		    break;
	    }
	    /* Attempt to reclaim memory from the Relation Cache
	    ** i.e., reclaim memory from unfixed relation descriptors
	    */
	    ulh_rcb.ulh_hashid = Rdi_svcb->rdv_rdesc_hashid;
	    ulh_rcb.ulh_object = (ULH_OBJECT *) NULL;
	    status = ulh_destroy(&ulh_rcb, (unsigned char *)NULL, (i4) 0);
	    if (DB_SUCCESS_MACRO(status))
		/* report statistics */
		Rdi_svcb->rdvstat.rds_n9_relmem_claim++;

	} while (0);	/* end of control loop */
	if (DB_FAILURE_MACRO(status))
	{
	    /* report statistics */
	    Rdi_svcb->rdvstat.rds_n10_nomem++;

	    /* we could not get the memory back from anywhere,
	    ** so return an error */

	    if (ulh_rcb.ulh_error.err_code == E_UL0109_NFND)
		rdu_ierror(global, status, E_RD0001_NO_MORE_MEM);
	    else
		rdu_ferror(global, status,
		    &ulh_rcb.ulh_error,
		    E_RD0040_ULH_ERROR,0);
	}
    }
    else
    {
	/* it was not a memory reclaim error, so handle the error */
	status = orig_status;
	rdu_ferror(global, status, &ulm_rcb->ulm_error, errcode, 0);
    }
    return(status);
}

/*{
** Name: rdu_cstream - create a new private memory stream
**
**	Internal call format:	rdu_cstream(global);
**
** Description:
**      This function initializes the table information block.
**
** Inputs:
**
**	svcb
**		.rdv_rdesc_hashid   pointer to hash table
**	rdfcb			    pointer to request block.
**		.rdr_fcb	    pointer to facility control block.
**	ulm_cb
**		.ulm_poolid
**
** Outputs:
**	ulm_cb
**		.ulm_streamid
**
**	Returns:
**		DB_STATUS
**
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**      11-nov-87 (seputis)
**          rewritten for resource management
**	17-Oct-89 (teg)
**	    change logic to get memleft from Server Control Block, since RDF
**	    now allocates its own memory.  This is part of the fix for bug 5446.
**	23-jan-92 (teresa)
**	    SYBIL: rdi_poolid->rdv_poolid; rdv_c1_memleft->rdv_memleft;
**	31-Dec-2003 (schka24)
**	    Log whenever we garbage collect memory
*/
DB_STATUS
rdu_cstream(RDF_GLOBAL	*global)
{
    ULM_RCB	    *ulm_rcb;		/* ptr to ULM control block being
					** initialized */
    i4		    mrecover_cnt = 0;
    DB_STATUS	    status = E_DB_OK;
    RDI_FCB	    *rdi_fcb;		/* ptr to facility control block
				    ** initialized at server startup time
				    */
    if (global->rdf_init & RDF_IULM)
    {	/* not expecting ULM to be initialized yet, assumptions are made
        ** that this is called prior to any rdu_malloc call */
	status = E_DB_SEVERE;
	rdu_ierror(global, status, E_RD0118_ULMOPEN);
	return(status);
    }
    ulm_rcb = &global->rdf_ulmcb;
    rdi_fcb = (RDI_FCB *)(global->rdfcb->rdf_rb.rdr_fcb);
    ulm_rcb->ulm_poolid = Rdi_svcb->rdv_poolid;
    ulm_rcb->ulm_memleft = &Rdi_svcb->rdv_memleft;
    ulm_rcb->ulm_facility = DB_RDF_ID;
    ulm_rcb->ulm_blocksize = 0;	/* No prefered block size */
    ulm_rcb->ulm_streamid_p = &ulm_rcb->ulm_streamid;
    /* Get a private stream, visible only to this thread */
    ulm_rcb->ulm_flags = ULM_PRIVATE_STREAM;
    do
    {	/* FIXME - this could be an infinite loop since some other thread
        ** may immediately use the memory freed by this thread */
	status = ulm_openstream(ulm_rcb);
	if(DB_FAILURE_MACRO(status))
	{
	    mrecover_cnt++;
	    TRdisplay("%@ [%x] Garbage-collecting RDF: cstream err %d (poolid %x, memleft %d try %d)\n",
		global->rdf_sess_id, ulm_rcb->ulm_error.err_code,
		Rdi_svcb->rdv_poolid, Rdi_svcb->rdv_memleft, mrecover_cnt);
	    status = rdu_mrecover(global, ulm_rcb, status, E_RD0118_ULMOPEN);
	}
	else
	{
	    global->rdf_init |= RDF_IULM;
	    global->rdf_resources |= RDF_RPRIVATE; /* a private
				    ** stream has been created which
				    ** should be deleted if any error
				    ** condition is found */
	    break;
	}
    } while (DB_SUCCESS_MACRO(status));
    return(status);
}

/*{
** Name: rdu_dstream	- delete a memory stream
**
** Description:
**      This routine will delete a memory stream
**
** Inputs:
**      global                          routine to delete a private
**                                      memory stream
**	stream_id			ID of stream to be deleted
**	state				ptr to state of object contained
**					within the stream, NULL if
**					default rdf_infoblk stream is
**					to be used
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-nov-87 (seputis)
**          initial creation
**      11-jan-89 (seputis)
**          added state parameter to determine correct consistency check
**	    for call from UNFIX.
**	17-Oct-89 (teg)
**	    change logic to get memleft from Server Control Block, since RDF
**	    now allocates its own memory.  This is part of the fix for bug 5446.
**      17-apr-90 (teg)
**          new parameter added to rdu_ferror.
**	23-jan-92 (teresa)
**	    SYBIL: rdi_poolid->rdv_poolid; rdv_c1_memleft->rdv_memleft;
**	26-feb-93 (teresa)
**	    add suport to unfix a defaults or ldbdesc cache item.
**      09-may-00 (wanya01)
**          Bug 99982: Server crash with SEGV in rdf_unfix() and/or psl_make
**          _user_default().
**      21-feb-2001 (rigka01)
**          Bug 103486: more server crashes in rdf_unfix() and/or psl_make_
**          user_default(). info block indicates default information is
**          available but pointer to the info is null.
**          If attr is nulled for private info block, then unset
**          RDR_ATTRIBUTES; also ensure defaults information is only
**	    destroyed when rdf_shared_sessions is 0
**	19-oct-2009 (wanfr01) Bug 122755
**	    Need to pass infoblk to the deulh routines
[@history_template@]...
*/
DB_STATUS
rdu_dstream(	RDF_GLOBAL         *global,
		PTR		   stream_id,
		RDF_STATE	   *state)
{
    ULM_RCB	    *ulm_rcb;		/* ptr to ULM control block being
					** initialized */
    DB_STATUS	    status = E_DB_OK;
    DB_STATUS	    t_status = E_DB_OK;
    RDR_INFO	    *infoblk=global->rdfcb->rdf_info_blk;

    ulm_rcb = &global->rdf_ulmcb;
    if (!(global->rdf_init & RDF_IULM))
    {	/* if ULM is not initialized then this is a call from RDF_UNFIX
        ** in which case the streamid is obtained from the usr_infoblk
        ** - if ULM is initialized then the ULM control block should contain
        ** the stream id to delete */
	RDI_FCB	    *rdi_fcb;		/* ptr to facility control block
					** initialized at server startup time
					*/
	rdi_fcb = (RDI_FCB *)(global->rdfcb->rdf_rb.rdr_fcb);
	ulm_rcb->ulm_poolid = Rdi_svcb->rdv_poolid;
	ulm_rcb->ulm_memleft = &Rdi_svcb->rdv_memleft;
	ulm_rcb->ulm_facility = DB_RDF_ID;
	ulm_rcb->ulm_blocksize = 0;	/* No prefered block size */
	if ((state && (*state != RDF_SPRIVATE)) /* this would occur
					** for an UNFIX mis-match */
	    ||
	    (   !state
		&&
		(   ((RDF_ULHOBJECT *)(global->rdfcb->rdf_info_blk->rdr_object))
			->rdf_state
		    !=
		    RDF_SPRIVATE
		)
	    )
	    )
	{   /* expecting this to be a private memory stream to be deleted */
	    status = E_DB_SEVERE;
	    rdu_ierror(global, status, E_RD011B_PRIVATE);
	    return(status);
	}
	ulm_rcb->ulm_streamid_p = &stream_id;
    }

    /* not all paths through this will have an infoblk. But if one exists, then
    ** verify that there is no LDBdesc or DEFAULTS cache object tied to this
    ** private cache object before destroying it.
    */
    if (infoblk)
    {
	/*
	**  if the infoblk still has a fixed LDBdesc cache object,
	**  release it now
	*/
	if (infoblk->rdr_types & RDR_DST_INFO)
	{
	    if ( infoblk->rdr_obj_desc
	       &&
		 infoblk->rdr_obj_desc->dd_o9_tab_info.dd_t9_ldb_p
	       )
	    {
		/* The LDBdesc cache object is still fixed, so release it */
		if (global->rdf_d_ulhobject->rdf_state == RDF_SBAD)
		{
		    /* destroy the distributed object */
		    status = rdu_i_dulh(global);
		    if (DB_FAILURE_MACRO(status))
			return status;
		}
		else
		{
		    /* release the distributed object */
		    status = rdu_r_dulh(global);
		    if (DB_FAILURE_MACRO(status))
			return status;
		}
	    }	/* end loop */
	}   /* end LDBdesc cache cleanup */
	/*
	**  if the infoblk still has a fixed DEFAULTS cache object,
	**  release it now
	*/
	if (infoblk->rdr_2_types & RDR2_DEFAULT)
	{
	    i4		    i;
	    DMT_ATT_ENTRY   *attr;	    /* ptr to DMT_ATT_ENTRY ptr array */
	    RDD_DEFAULT	    *rdf_defobj;    /* default cache object */

	    /* the 1st entry in the attr array is for the TID, so start with
	    ** the second element, which is the first attribute.
	    */
          if ((global->rdf_ulhobject->rdf_state == RDF_SPRIVATE)
               &&
              (global->rdf_ulhobject->rdf_sysinfoblk)
               &&
              (infoblk->rdr_attr == global->rdf_ulhobject->rdf_sysinfoblk->rdr_attr)
             )
           {
              /* if using private cache, do not destroy shared default cache object */
              infoblk->rdr_types &= (~RDR_ATTRIBUTES);
              infoblk->rdr_attr = NULL;
              infoblk->rdr_attr_names = NULL;
              infoblk->rdr_2_types &= (~RDR2_DEFAULT);
           }
           else
           {
           if (global->rdf_ulhobject->rdf_shared_sessions < 1)
	    for (i=1; i <= infoblk->rdr_no_attr; i++)
	    {
		attr = infoblk->rdr_attr[i];
		if ( attr && attr->att_default)
		{
		    /* a default cache object exists, so release it */
		    global->rdf_init &= ~RDF_DE_IULH;
		    rdf_defobj = (RDD_DEFAULT*) (attr->att_default);
		    global->rdf_de_ulhobject = (RDF_DE_ULHOBJECT *)
			    rdf_defobj->rdf_def_object;
		    if (global->rdf_de_ulhobject->rdf_state == RDF_SBAD)
			/* destroy the default cache object */
			t_status = rdu_i_deulh(global, infoblk, attr);
		    else
			/* release the default cache object */
			t_status = rdu_r_deulh(global, infoblk, attr);
		    global->rdf_de_ulhobject = 0;

		}
	    }	/* end loop */
           }
	}   /* end defaults cache cleanup */
    } /* end if there is an infoblk */

    status = ulm_closestream(ulm_rcb);
    if (DB_FAILURE_MACRO(status))
    {
	rdu_ferror(global, status,
	    &ulm_rcb->ulm_error, E_RD011C_ULM_CLOSE,0);
    }
    else
    {
	/* see if there was a failure releasing the LDBdesc or DEFAULTS cache
	** objects.  If so, report that error.
	*/
	if (DB_FAILURE_MACRO(t_status))
	    status = t_status;
	global->rdf_resources &= (~RDF_RPRIVATE);
	global->rdf_init &= (~RDF_IULM);
    }

    return(status);
}

/*{
** Name: rdu_private	- create private sys_infoblk
**
** Description:
**	Create a private info block, making a copy of the master info
**	block.  This happens whenever a session wants to update something
**	in the info block but other sessions are using it.  A copy of
**	the ULH object is made as well, from the same stream;  that
**	way, the entire private object set can be deallocated at once.
**	The private object is linked to the master one via the sysinfoblk
**	pointer in the ULH object copy.
**
**	(schka24) This business with a private info block is done (I think!)
**	because many RDF operations can take system catalog locks.
**	If we were simply to sit on a master condition variable, which
**	would seem to be the obvious solution, we might get into an
**	undetectable deadlock (the session(s) using the master might
**	need to take locks we're holding).  So we make a private copy
**	so that this session can continue onwards.
**
**	An ultimately better solution might be to integrate the locking
**	and mutexing systems so that deadlocks can be detected;  or
**	perhaps, take some kind of regular lock on the master info.
**
** Inputs:
**      global                          ptr to RDF state variable
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	Releases the RDF object semaphore.
**
** History:
**      27-oct-87 (seputis)
**          initial creation
**      26-apr-89 (mings)
**          check RDR_REFRESH for register with refresh
**      23-jun-89 (mings)
**          add rdf_shared_sessions in RDF_ULHOBJECT
**	21-may-92 (teresa)
**	    Merged titan changes into mainline AND
**	    change rdf_shared_sessions logic to only set this to non-zero if
**	    the thread will be fixing the LDBdesc object (i.e., if the relation
**	    object does not already exist.)  Also, when copying the relation
**	    object, also copy the dd_o9_tab_info element.  This work is for
**	    sybil.
**	16-feb-94 (teresa)
**	    Added support to  calcuate checksum on newly created private object.
**	23-jan-97 (cohmi01)
**	    Remove checksum calls from here, now done before releasing mutex
**	    to ensure that checksum is recomputed during same mutex interval
**	    that the infoblock was altered in. (Bug 80245).
**	    Restore setting of RDF_RPRIVATE bit, was removed inadvertently
**	    by teresa's 16-feb-94 fix.
**	    Remove checksum code from rdu_xlock() that had long been '#if 0'.
**      16-mar-1998 (rigka01)
**	    Cross integrate change 434809 from OI 1.2 to OI 2.0
**          bug 89413 - When threads are locked out of using shared stream,
**          private stream is allocated but use of private stream causes
**          access violation.
**          The exception occurs when releasing the shared ulh object semaphore
**          when memory is low.  rdu_private was obtaining the semaphore
**          before attempting to get the memory for the private ulh_object
**          and memory allocation was failing.  The fix is to move the get
**          for the semaphore after all gets of memory needed by this routine.
**	22-jul-1998 (shust01)
** 	    Backed out change 434936 (2.0) for bug 89413.  Memory allocations
** 	    done via rdu_malloc() should be done after getting semaphore.
** 	    Can potentially have caused SEGVs in PSF (among other areas).
**	    bug 92059.
**	29-Dec-2003 (schka24)
**	    copy partition stuff to private infoblk.
**	15-Mar-2004 (schka24)
**	    Trust flags, not pointers, when copying stuff;  fix comments.
**	27-Jul-2004 (schka24)
**	    Need to include pparray with attribute info.
**
[@history_template@]...
*/
DB_STATUS
rdu_private(	RDF_GLOBAL         *global,
		RDR_INFO	   **infoblkpp)
{
    DB_STATUS	    status;
    RDR_INFO	    *usr_infoblk;
    RDR_INFO	    *sys_infoblk;
    RDF_ULHOBJECT   *rdf_ulhobject;
    RDF_CB	    *rdfcb = global->rdfcb;

    /* Copy data from the system info block to user's copy */

    status = rdu_gsemaphore(global);	/* get a semaphore on the object to
					** be copied */
    sys_infoblk = global->rdf_ulhobject->rdf_sysinfoblk; /* FIXME - check if
					** sys_infoblk is valid here */
    if (DB_FAILURE_MACRO(status))
	return(status);

    status = rdu_cstream(global);	/* create a temporary memory
					** stream, to be used only by
					** this thread and to be deleted
					** upon unfix by the user */
    if (DB_FAILURE_MACRO(status))
	return(status);
    status = rdu_malloc(global, (i4)sizeof(*rdf_ulhobject),
	(PTR *)&rdf_ulhobject);
    if (DB_FAILURE_MACRO(status))
	return(status);
    rdf_ulhobject->rdf_state = RDF_SPRIVATE; 	/* indicate that this
						** object is being updated */
    rdf_ulhobject->rdf_sintegrity = RDF_SNOTINIT;
    rdf_ulhobject->rdf_sprotection = RDF_SNOTINIT;
    rdf_ulhobject->rdf_ssecalarm = RDF_SNOTINIT;
    rdf_ulhobject->rdf_srule = RDF_SNOTINIT;
    rdf_ulhobject->rdf_skeys = RDF_SNOTINIT;
    rdf_ulhobject->rdf_sprocedure_parameter = RDF_SNOTINIT;
    rdf_ulhobject->rdf_procedure = NULL;
    rdf_ulhobject->rdf_defaultptr = NULL;

    rdf_ulhobject->rdf_sysinfoblk = sys_infoblk;
    status = rdu_malloc(global, (i4)sizeof(*usr_infoblk),
	(PTR *)&usr_infoblk);
    if (DB_FAILURE_MACRO(status))
	return(status);
    rdu_init_info(global, usr_infoblk, global->rdf_ulmcb.ulm_streamid);
    rdf_ulhobject->rdf_shared_sessions = 1;
    rdf_ulhobject->rdf_starptr = NULL;

    /* If distributed, allocate memory for object descriptor. */
    if (global->rdfcb->rdf_rb.rdr_r1_distrib & DB_3_DDB_SESS)
    {
	status = rdu_malloc(global,
		    (i4)sizeof(*usr_infoblk->rdr_obj_desc),
		    (PTR *)&usr_infoblk->rdr_obj_desc);
	if (DB_FAILURE_MACRO(status))
	    return(status);
    }

    usr_infoblk->rdr_object = (PTR)rdf_ulhobject;
			/* have this object point back to itself
			** initially, so it can be found if the user
			** calls RDF with this object again */
    global->rdf_ulhobject = rdf_ulhobject; /* save ptr
			** to the resource which is to be updated */
    global->rdfcb->rdf_info_blk = usr_infoblk;
    *infoblkpp = usr_infoblk;

    /* If RDR_REFRESH is set, mark rdf_resources as RDF_REFRESH.
    ** This setup will cause RDF to bring information automatically */

    if (rdfcb->rdf_rb.rdr_types_mask & RDR_REFRESH)
	global->rdf_resources |= RDF_REFRESH;


    {
	RDF_TYPES	    types;

	/* Copy those resources that the sysinfo block knows about.
	** Use the sysinfo block flags, not the pointers.  This is
	** under mutex protection so the flags are safe.
	** (If Ingres knew more about memory store barriers, we could
	** just make sure that the flags were set after the pointers
	** and a membar, but it doesn't.) 
	** FIXME this business of copying sysinfo pointers into a
	** private info block is wrong and dangerous, but I don't have
	** time to fix now.  A problem could arise if the master is
	** unfixed before the private copy, and memory pressure or
	** invalidation destroys the master data while the private
	** copy is still pointing to it.
	*/
	types = 0;
	if (sys_infoblk->rdr_types & RDR_RELATION)
	{
	    usr_infoblk->rdr_rel = sys_infoblk->rdr_rel;
	    usr_infoblk->rdr_no_attr = sys_infoblk->rdr_no_attr;
	    usr_infoblk->rdr_attnametot = sys_infoblk->rdr_attnametot;
	    usr_infoblk->rdr_no_index = sys_infoblk->rdr_no_index;
	    types |= RDR_RELATION;
	    if (sys_infoblk->rdr_types & RDR_DST_INFO)
	    {
		if (sys_infoblk->rdr_obj_desc)
		     STRUCT_ASSIGN_MACRO(*sys_infoblk->rdr_obj_desc,
					 *usr_infoblk->rdr_obj_desc);
		else
		     STRUCT_ASSIGN_MACRO(global->rdf_tobj,
					 *usr_infoblk->rdr_obj_desc);
		types |= RDR_DST_INFO;
	    }
	}
	if (sys_infoblk->rdr_types & RDR_ATTRIBUTES)
	{
	    usr_infoblk->rdr_attr = sys_infoblk->rdr_attr;
	    usr_infoblk->rdr_attr_names = sys_infoblk->rdr_attr_names;
	    usr_infoblk->rdr_pp_array = sys_infoblk->rdr_pp_array;
	    types |= RDR_ATTRIBUTES;
	}
	if (sys_infoblk->rdr_types & RDR_BLD_PHYS)
	{
	    usr_infoblk->rdr_keys = sys_infoblk->rdr_keys;
	    usr_infoblk->rdr_parts = sys_infoblk->rdr_parts;
	    types |= RDR_BLD_PHYS;
	}
	if (sys_infoblk->rdr_types & RDR_INDEXES)
	{
	    usr_infoblk->rdr_indx = sys_infoblk->rdr_indx;
	    types |= RDR_INDEXES;
	}
	if (sys_infoblk->rdr_types & RDR_VIEW)
	{
	    usr_infoblk->rdr_view = sys_infoblk->rdr_view;
	    types |= RDR_VIEW;
	}
	if (sys_infoblk->rdr_types & RDR_STATISTICS)
	{
	    usr_infoblk->rdr_histogram = sys_infoblk->rdr_histogram;
	    types |= RDR_STATISTICS;
	}
	if (sys_infoblk->rdr_types & RDR_HASH_ATT)
	{
	    usr_infoblk->rdr_atthsh = sys_infoblk->rdr_atthsh;
	    types |= RDR_HASH_ATT;
	}

	/* If distributed, make sure attach the distributed info to it. */
	if (sys_infoblk->rdr_types & RDR_DST_INFO)
	{
	    types |= RDR_DST_INFO;
	    usr_infoblk->rdr_obj_desc = sys_infoblk->rdr_obj_desc;
	}
	usr_infoblk->rdr_types = types;
    }
    /* set resources bit to indicate that a private stream has been created
    ** which should be deleted if any error condition is found
    */
    global->rdf_resources |= RDF_RPRIVATE;
    status = rdu_rsemaphore(global);

    return(status);
}

/*{
** Name: rdf_exception	- handle unknown exception for RDF
**
** Description:
**      This routine will handle unknown exceptions for RDF
**
** Inputs:
**      ex_args                         argument list from exception handler
**
** Outputs:
**	Returns:
**	    STATUS - appropriate CL return code for exception handler
**	Exceptions:
**
**
** Side Effects:
**
**
** History:
**      15-nov-87 (seputis)
**	    created
**	21-may-89 (jrb)
**	    Now passes generic error code to rdu_sccerror.
**	22-jan-90 (teg)
**	    remove VMS specific code and replace with call to EXsys_report.
**	07-nov-91 (teresa)
**	    picked  up ingres63p change 262778:
**	    18-jul-91 (teresa)
**		replace "256" with constant "EX_MAX_SYS_REP"
**	24-oct-92 (andre)
**	    ule_format() expects a (DB_SQLSTATE *) +
**	    in ADF_ERROR, generic_error has been replaced with sqlstate
**	02-jul-1993 (rog)
**	    Call ulx_exception() to take care of reporting functions.
**	    Remove call to adx_handler() because RDF does not need to have
**	    ADF handle math exceptions.
[@history_template@]...
*/
STATUS
rdf_exception(EX_ARGS *args)
{
    /* unexpected exception so record the error and return to
    ** deallocate resources */

    ulx_exception( args, DB_RDF_ID, E_RD0125_EXCEPTION, TRUE );

    return (EXDECLARE);        /* return to exception handler declaration pt */
}

/*{
** Name: rdu_drop_attachments
**
** Description:
**      Drop attached objects from an infoblock
**
** History:
**      6-Feb-08 (kibro01)
**	    created
**	1-Sep-2009 (wanfr01)
**	    Bug 122549 - Need mutex protection to avoid concurrency crash.
**	19-oct-2009 (wanfr01) Bug 122755
**	    Expand mutex protection to protect shared session count.
**	    The infoblock you want to use is not necessarily the global infoblk.
**	    Use the infoblk as passed in by the caller.
*/
STATUS
rdu_drop_attachments(RDF_GLOBAL *global, RDR_INFO *infoblk, i4 session_reqd)
{
    i4 status = 0;

    status = rdu_gsemaphore(global);
    if (DB_FAILURE_MACRO(status))
	return(status);
    if (global->rdf_ulhobject &&
	global->rdf_ulhobject->rdf_shared_sessions <= session_reqd)
    {
        /* this is a relation cache object, so see if it currently has any
        ** other cache objects (LDBdesc/DEFAULTS) attached to it.  If so,
        ** then release them.
        */
	
        if (infoblk->rdr_types & RDR_DST_INFO)
        {
            /* this is a distributed thread, so release the LDBdesc object */
            if ( infoblk->rdr_obj_desc &&
                 infoblk->rdr_obj_desc->dd_o9_tab_info.dd_t9_ldb_p)
            {
                RDF_ULHOBJECT *rdf_ulhobj=(RDF_ULHOBJECT *) infoblk->rdr_object;

                if ( !global->rdf_d_ulhobject && rdf_ulhobj->rdf_starptr)
                    global->rdf_d_ulhobject = rdf_ulhobj->rdf_starptr;

                /* The LDBdesc cache object is still fixed, so release it */
		if (global->rdf_d_ulhobject)
		{
                    if (global->rdf_d_ulhobject->rdf_state == RDF_SBAD)
                        /* destroy the distributed object */
                        status = rdu_i_dulh(global);
                    else
                        /* release the distributed object */
                        status = rdu_r_dulh(global);
		}
            }
        }
        if (infoblk->rdr_2_types & RDR2_DEFAULT )
        {
            /* this is a local cache,
            ** so see if there are any referenced default objects to release
            */
            i4          i;
            DMT_ATT_ENTRY   *attr;          /* ptr to DMT_ATT_ENTRY ptr array */
            RDD_DEFAULT     *rdf_defobj;    /* default cache object */

           if ((global->rdf_ulhobject->rdf_state == RDF_SPRIVATE) &&
               (global->rdf_ulhobject->rdf_sysinfoblk) &&
               (infoblk->rdr_attr == global->rdf_ulhobject->rdf_sysinfoblk->rdr_attr)
             )
           {
             /* if using private cache, do not destroy shared default cache object */
            infoblk->rdr_types &= (~RDR_ATTRIBUTES);
            infoblk->rdr_attr = NULL;
            infoblk->rdr_attr_names = NULL;
            infoblk->rdr_2_types &= (~RDR2_DEFAULT);
           }
           else
           {
            /* the 1st entry in the attr array is for the TID, so start with
            ** the second element, which is the first attribute.
            */
            for (i=1; i <= infoblk->rdr_no_attr; i++)
            {

                attr = infoblk->rdr_attr[i];
                if ( attr && attr->att_default)
                {
                    /* a default cache object exists, so release it */
                    global->rdf_init &= ~RDF_DE_IULH;
                    rdf_defobj = (RDD_DEFAULT*) (attr->att_default);
                    global->rdf_de_ulhobject = (RDF_DE_ULHOBJECT *)
                            rdf_defobj->rdf_def_object;
                    if (global->rdf_de_ulhobject->rdf_state == RDF_SBAD)
                        /* destroy the default cache object */
                        status = rdu_i_deulh(global, infoblk, attr);
                    else
                        /* release the default cache object */
                        status = rdu_r_deulh(global, infoblk, attr);
                    global->rdf_de_ulhobject = 0;
                } /* endif there are defaults to release */
            }  /* end loop to walk each attribute and look for defaults */
           }
        }
    }
    status = rdu_rsemaphore(global);   
    return (status);
}

/*{
** Name: rdu_iulh	- invalidate a ulh object
**
** Description:
**
**      This routine will invalidate a ULH object
**
**	If the object is a relation object and contains a reference to a
**	fixed LDBdesc or DEFAULTS cache object, then unfix that object.
**	Since we are invalidating the relation cache object and do not know
**	if other threads still hold the infoblk we are invalidating, we do
**	NOT clear the pointer to the DEFAULTS/LDBdesc cache object.  When the
**	last session unfixes this object, it will be destroyed and we do not
**	want to zero a pointer out from under another thread that may be using
**	it.
**
** Inputs:
**      global                          ptr to RDF global state variable
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-nov-87 (mings)
**          created
**	20-jun-90 (teresa)
**	    added new parameter to rdu_ferror() for bug 4390.
**	26-feb-93 (teresa)
**	    Added suport for defaults cache
**	11-mar-93 (teresa)
**	    fix AV releasing infoblks with LDB descriptor attached and
**	    global->rdf_d_ulhobject was not initialized.
**	08-dec-93 (teresa)
**	    Fix memory leak bug 57687 where we tested for qtree cache object
**	    (instead of RELcache object) to unfix LDBdesc or defaults cache
**	    object.
**	08-sep-1998 (nanpr01)
**	    Check for the null pointer before dereferencing it. Also
**	    rdf_shared_sessions is incremented and decremented without
**	    semaphore and can be incorrect. At least if it becomes less
**	    than zero (by loosing add), we should free the memory rather
**	    than holding it.
**      09-may-00 (wanya01)
**          Bug 99982: Server crash with SEGV in rdf_unfix() and/or psl_make
**          _user_default().
**      21-feb-2001 (rigka01)
**          Bug 103486: more server crashes in rdf_unfix() and/or psl_make_
**          user_default(). info block indicates default information is
**          available but pointer to the info is null.
**          If attr is nulled for private info block, then unset
**          RDR_ATTRIBUTES; also ensure defaults information is only
**	    destroyed when rdf_shared_sessions is 0
**	6-Feb-2008 (kibro01) b114324
**	    Added rdu_drop_attachments
[@history_template@]...
*/
DB_STATUS
rdu_iulh(RDF_GLOBAL *global)
{
    /* call ULH to release lock on the system info block */
    ULH_RCB	    *ulh_rcb;
    DB_STATUS	    status= E_DB_OK;
    DB_STATUS	    ulh_status;
    RDR_INFO	    *infoblk = global->rdfcb->rdf_info_blk;

    ulh_rcb = &global->rdf_ulhcb;

    if (!(global->rdf_init & RDF_IULH))
    {	/* This will not be initialized during an RDF_UNFIX call but it will
        ** be initialized during internal error recovery calls, to either the
        ** query tree, or relation, ulh hash ids.
	** Note that master ulhptr is reliable, private one isn't, so
	** make sure we have a good ulh object pointer in the ulhcb.
	*/

	RDR_INFO	    *sys_infoblk;

	sys_infoblk = ((RDF_ULHOBJECT *)(global->rdfcb->rdf_info_blk->
		  rdr_object))->rdf_sysinfoblk;
	if (global->rdfcb->rdf_rb.rdr_types_mask & RDR_PROCEDURE)
	    ulh_rcb->ulh_hashid = Rdi_svcb->rdv_qtree_hashid;
	else
	    ulh_rcb->ulh_hashid = Rdi_svcb->rdv_rdesc_hashid;

	ulh_rcb->ulh_object =
	    ((RDF_ULHOBJECT *)(sys_infoblk->rdr_object))->rdf_ulhptr;
    }
    /* fix bug 57687, where the code used to incorrectly read rdv_qtree_hashid
    ** instead of rdv_rdesc_hashid -- this fixes a memory leak.
    **
    ** If this is the last thread to access the relation cache object and it
    ** has pointer to either the LDBDESC or DEFAULTS cache, then release it
    ** before destroying th is e object.
    */
    if ( global->rdf_ulhobject &&
	 ulh_rcb->ulh_hashid == Rdi_svcb->rdv_rdesc_hashid
       )
    {
	status = rdu_drop_attachments(global, infoblk, 0);
    }
    ulh_status = ulh_destroy(ulh_rcb, (unsigned char *)NULL, (i4)0);
    if (DB_FAILURE_MACRO(ulh_status))
    {
	rdu_ferror(global, ulh_status, &ulh_rcb->ulh_error,
	    E_RD0040_ULH_ERROR,0);
	status = ulh_status;
    }
    return(status);
}

/*{
** Name: rdu_rulh	- unfix a ulh object
**
** Description:
**
**      This routine will unfix a ULH object.  If that object contains pointers
**	to other cache objects (LDBdesc or DEFAULTS) and this thread is the last
**	thread to unfix it, then it will unfix the object and clear the pointer
**	to it.
**
** Inputs:
**      global                          ptr to RDF global state variable
[@PARAM_DESCR@]...
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-nov-87 (seputis)
**          initial creation
**      17-apr-90 (teg)
**          new parameter added to rdu_ferror.
**	23-jan-92 (teresa)
**	    SYBIL: rdi_qthashid->rdv_qtree_hashid;
**	26-feb-93 (teresa)
**	    Add support for defaults cache.
**	11-mar-93 (teresa)
**	    fix AV releasing infoblks with LDB descriptor attached and
**	    global->rdf_d_ulhobject was not initialized.
**	28-sep-93 (robf)
**          Only dereference sysinfo_blk if not null (may be empty during
**	    error recovery)
**	08-sep-1998 (nanpr01)
**	    Check for the null pointer before dereferencing it. Also
**	    rdf_shared_sessions is incremented and decremented without
**	    semaphore and can be incorrect. At least if it becomes less
**	    than zero (by loosing add), we should free the memory rather
**	    than holding it.
**      09-may-00 (wanya01)
**          Bug 99982: Server crash with SEGV in rdf_unfix() and/or psl_make
**          _user_default().
**      21-feb-2001 (rigka01)
**          Bug 103486: more server crashes in rdf_unfix() and/or psl_make_
**          user_default(). info block indicates default information is
**          available but pointer to the info is null.
**          If attr is nulled for private info block, then unset
**          RDR_ATTRIBUTES; also ensure defaults information is only
**	    destroyed when rdf_shared_sessions is 0
**	20-Aug-2007 (kibro01) b118595
**	    If the remote connection was lost, the object was not fully
**	    created.  Check for NULL before attempting to destroy it.
**      1-Sep-2009 (wanfr01)
**          Bug 122549 - Need mutex protection to avoid concurrency crash.
**	19-oct-2009 (wanfr01) Bug 122755
**	    Expand mutex protection to protect shared session count.
**	    Need to pass infoblk pointer down to deulh routines
[@history_template@]...
*/
DB_STATUS
rdu_rulh(RDF_GLOBAL *global)
{
    /* call ULH to release lock on the system info block */
    ULH_RCB	    *ulh_rcb;
    DB_STATUS	    status = E_DB_OK;
    DB_STATUS	    ulh_status;
    RDR_INFO	    *infoblk = global->rdfcb->rdf_info_blk;

    ulh_rcb = &global->rdf_ulhcb;
    if (!(global->rdf_init & RDF_IULH))
    {	/* This will not be initialized during an RDF_UNFIX call but it will
        ** be initialized during internal error recovery calls, to either the
        ** query tree, or relation, ulh hash ids */
	RDR_INFO	*sys_infoblk;
	sys_infoblk = ((RDF_ULHOBJECT *)(global->rdfcb->rdf_info_blk->
	    rdr_object))->rdf_sysinfoblk;
	if ( (global->rdfcb->rdf_rb.rdr_types_mask & RDR_PROCEDURE) ||
	     (global->rdfcb->rdf_rb.rdr_types_mask & RDR_QTREE ) )
	    ulh_rcb->ulh_hashid = Rdi_svcb->rdv_qtree_hashid;
	else
	    ulh_rcb->ulh_hashid = Rdi_svcb->rdv_rdesc_hashid;
	if(sys_infoblk)
	{
	    ulh_rcb->ulh_object =
		((RDF_ULHOBJECT *)(sys_infoblk->rdr_object))->rdf_ulhptr;
	}
	else
	{
	    ulh_rcb->ulh_object=NULL;
	}
    }

    /* this is the last thread to unfix a relation cache object.  If this object
    ** has pointer to either the LDBDESC or DEFAULTS cache, then release it.
    */

    status = rdu_gsemaphore(global);
    if (DB_FAILURE_MACRO(status))
	return(status);
    if ( global->rdf_ulhobject &&
	 global->rdf_ulhobject->rdf_shared_sessions <= 0 &&
	 ulh_rcb->ulh_hashid == Rdi_svcb->rdv_rdesc_hashid
       )
    {
	if (infoblk->rdr_types & RDR_DST_INFO)
	{
	    if ( infoblk->rdr_obj_desc
	       &&
		 infoblk->rdr_obj_desc->dd_o9_tab_info.dd_t9_ldb_p
	       )
	    {
		RDF_ULHOBJECT *rdf_ulhobj=(RDF_ULHOBJECT *) infoblk->rdr_object;

		/* The LDBdesc cache object is still fixed, so release it */
		if ( !global->rdf_d_ulhobject
		    &&
		     rdf_ulhobj->rdf_starptr
		   )
		    global->rdf_d_ulhobject = rdf_ulhobj->rdf_starptr;

		/* If the rdf_d_ulhobject is null, that means we haven't
		** managed to get the description from the remote server,
		** so don't try to destroy it (kibro01) b118595
		*/
		if (global->rdf_d_ulhobject)
		{
		    if (global->rdf_d_ulhobject->rdf_state == RDF_SBAD)
		        /* destroy the distributed object */
		        status = rdu_i_dulh(global);
		    else
		        /* release the distributed object */
		        status = rdu_r_dulh(global);
		}
	    }
	}
	if (infoblk->rdr_2_types & RDR2_DEFAULT)
	{
	    i4		i;
	    DMT_ATT_ENTRY   *attr;	    /* ptr to DMT_ATT_ENTRY ptr array */
	    RDD_DEFAULT	    *rdf_defobj;    /* default cache object */

	    /* the 1st entry in the attr array is for the TID, so start with
	    ** the second element, which is the first attribute.
	    */

           if ((global->rdf_ulhobject->rdf_state == RDF_SPRIVATE)
               &&
               (global->rdf_ulhobject->rdf_sysinfoblk)
               &&
               (infoblk->rdr_attr == global->rdf_ulhobject->rdf_sysinfoblk->rdr_attr)
              )
           {
              /* if using private cache, do not destroy shared default cache object */
              infoblk->rdr_types &= (~RDR_ATTRIBUTES);
              infoblk->rdr_attr = NULL;
              infoblk->rdr_attr_names = NULL;
              infoblk->rdr_2_types &= (~RDR2_DEFAULT);
           }
           else
           {
           if (global->rdf_ulhobject->rdf_shared_sessions < 1)
	    for (i=1; i <= infoblk->rdr_no_attr; i++)
	    {

		attr = infoblk->rdr_attr[i];
		if ( attr && attr->att_default)
		{
		    /* a default cache object exists, so release it */
		    global->rdf_init &= ~RDF_DE_IULH;
		    rdf_defobj = (RDD_DEFAULT*) (attr->att_default);
		    global->rdf_de_ulhobject = (RDF_DE_ULHOBJECT *)
			    rdf_defobj->rdf_def_object;
		    if (global->rdf_de_ulhobject->rdf_state == RDF_SBAD)
			/* destroy the default cache object */
			status = rdu_i_deulh(global, global->rdfcb->rdf_info_blk, attr);
		    else
			/* release the default cache object */
			status = rdu_r_deulh(global, global->rdfcb->rdf_info_blk, attr);
		    global->rdf_de_ulhobject = 0;
		} /* endif there are defaults to release */
	    }  /* end loop to walk each attribute and look for defaults */
           }
	}
    }
    status = rdu_rsemaphore(global);
    if (DB_FAILURE_MACRO(status))
	return(status);

    ulh_status = ulh_release(ulh_rcb);
    if (DB_FAILURE_MACRO(ulh_status))
    {
	status = ulh_status;
	rdu_ferror(global, ulh_status, &ulh_rcb->ulh_error,
	    E_RD011A_ULH_RELEASE,0);
    }
    return(status);
}

/*{
** Name: rdu_r_dulh	- unfix a distributed ulh object
**
** Description:
**      This routine will unfix a distributed ULH object (from LDBdesc cache)
**
** Inputs:
**      global                          ptr to RDF global state variable
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-may-92 (teresa)
**	    Created for sybil
**	18-Feb-2008 (kibro01) b119744
**	    Partially created star objects can have only some of their
**	    attachments filled in, so check we have an object to release.
[@history_template@]...
*/
DB_STATUS
rdu_r_dulh(RDF_GLOBAL *global)
{
    /* call ULH to release lock on the system info block */
    ULH_RCB	    *ulh_rcb;
    DB_STATUS	    ulh_status = E_DB_OK;
    RDR_INFO	    *infoblk = global->rdfcb->rdf_info_blk; /* start with usr_infoblk */

    ulh_rcb = &global->rdf_dist_ulhcb;

    if (!(global->rdf_init & RDF_D_IULH))
    {
	/* This will not be initialized during an RDF_UNFIX call but it will
        ** be initialized during internal error recovery calls
	*/
	RDF_DULHOBJECT	*obj;

	ulh_rcb->ulh_object = global->rdf_d_ulhobject->rdf_ulhptr;
	if (!ulh_rcb->ulh_object)
	    ulh_rcb->ulh_object = global->rdf_dist_ulhcb.ulh_object;
	ulh_rcb->ulh_hashid = Rdi_svcb->rdv_dist_hashid;
    }

    if (ulh_rcb->ulh_object)
    {
        ulh_status = ulh_release(ulh_rcb);
    }
    if (DB_FAILURE_MACRO(ulh_status))
    {
	rdu_ferror(global, ulh_status, &ulh_rcb->ulh_error,
	    E_RD011A_ULH_RELEASE,0);
    }
    else
    {
	/* clear the ptr to the unfixed LDBdesc object from relation cache */
	if (global->rdf_ulhobject)
	{
	    global->rdf_ulhobject->rdf_starptr = NULL;

	    /*
	    ** clear ptr to unfixed LDBdesc object from appropriate infoblk
	    ** if user infoblk is undefined, then use system infoblk
	    */
	    if ( (!infoblk) || (infoblk == NULL) )
		infoblk = global->rdf_ulhobject->rdf_sysinfoblk;
	    infoblk->rdr_star_obj=NULL;
	}
    }
    return(ulh_status);
}

/*{
** Name: rdu_i_dulh	- invalidate a ulh distributed object
**
** Description:
**      This routine will invalidate a distributed ULH object
**
** Inputs:
**      global                          ptr to RDF global state variable
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-may-92 (teresa)
**	    Initial creation for Sybil.
[@history_template@]...
*/
DB_STATUS
rdu_i_dulh(RDF_GLOBAL *global)
{
    /* call ULH to release lock on the system info block */
    ULH_RCB	    *ulh_rcb;
    DB_STATUS	    ulh_status;
    ulh_rcb = &global->rdf_dist_ulhcb;

    if (!(global->rdf_init & RDF_D_IULH))
    {
	/* This will not be initialized during an RDF_UNFIX call but it will
        ** be initialized during internal error recovery calls
	*/
	ulh_rcb->ulh_object = global->rdf_d_ulhobject->rdf_ulhptr;
	if (!ulh_rcb->ulh_object)
	    ulh_rcb->ulh_object = global->rdf_dist_ulhcb.ulh_object;
	ulh_rcb->ulh_hashid = Rdi_svcb->rdv_dist_hashid;
    }
    ulh_status = ulh_destroy(ulh_rcb, (unsigned char *)NULL, (i4)0);
    if (DB_FAILURE_MACRO(ulh_status))
    {
	rdu_ferror(global, ulh_status, &ulh_rcb->ulh_error,
	    E_RD0040_ULH_ERROR,0);
    }
    else
    {
	/* clear ptr to the invalidated LDBdesc object from relation cache */
	if (global->rdf_ulhobject)
	    global->rdf_ulhobject->rdf_starptr = NULL;
    }
    return(ulh_status);
}

/*{
** Name: rdu_r_deulh	- unfix a defaults ulh object
**
** Description:
**      This routine will unfix a default ULH object (from Defaults cache)
**
** Inputs:
**      global                          ptr to RDF global state variable
**	att_ptr				ptr to DMF_ATTR_ENTRY
** Outputs:
**	att_ptr
**	    att_default			Ptr to attribute default is cleared.
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	23-feb-93 (teresa)
**	    Created for constraints
**	20-may-93 (teresa)
**	    Renamed rdf_de_ulhcb to rdf_def_ulhcb so it remains unique to 8
**	    characters with rdf_de_ulhobject.
**	19-oct-2009 (wanfr01)
**	    The infoblock you want to use is not necessarily the global infoblk.
**	    Use the infoblk as passed in by the caller.
[@history_template@]...
*/

DB_STATUS
rdu_r_deulh(RDF_GLOBAL *global, RDR_INFO *infoblk, DMT_ATT_ENTRY *att_ptr)
{
    /* call ULH to release lock on the system info block */
    ULH_RCB	    *ulh_rcb;
    DB_STATUS	    ulh_status;
    RDF_DULHOBJECT  *obj;

    /* init ULH_RCB */
    ulh_rcb = &global->rdf_dist_ulhcb;
    if (global->rdf_de_ulhobject)
	ulh_rcb->ulh_object = global->rdf_de_ulhobject->rdf_ulhptr;
    if (!ulh_rcb->ulh_object)
	ulh_rcb->ulh_object = global->rdf_def_ulhcb.ulh_object;
    ulh_rcb->ulh_hashid = Rdi_svcb->rdv_def_hashid;

    ulh_status = ulh_release(ulh_rcb);
    if (DB_FAILURE_MACRO(ulh_status))
    {
	rdu_ferror(global, ulh_status, &ulh_rcb->ulh_error,
	    E_RD011A_ULH_RELEASE,0);
    }
    else
    {
	/* clear the ptr to the unfixed Default object from attribute info */
	if (att_ptr)
	{
	    att_ptr->att_default = NULL;
	    infoblk->rdr_2_types &= ~RDR2_DEFAULT;
	}
    }

    return(ulh_status);
}

/*{
** Name: rdu_i_deulh	- invalidate a ulh defaults object
**
** Description:
**      This routine will invalidate a defaults cache ULH object
**
** Inputs:
**      global                          ptr to RDF global state variable
**	att_ptr				ptr to DMF_ATTR_ENTRY
** Outputs:
**	att_ptr
**	    att_default			Ptr to attribute default is cleared.
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-feb-93 (teresa)
**	    Initial creation for constraints.
**	20-may-93 (teresa)
**	    Renamed rdf_de_ulhcb to rdf_def_ulhcb so it remains unique to 8
**	    characters with rdf_de_ulhobject.
**	19-oct-2009 (wanfr01) Bug 122755
**	    The infoblock you want to use is not necessarily the global infoblk.
**	    Use the infoblk as passed in by the caller.
[@history_template@]...
*/
DB_STATUS
rdu_i_deulh(RDF_GLOBAL *global, RDR_INFO *infoblk, DMT_ATT_ENTRY *att_ptr)
{
    /* call ULH to release lock on the system info block */
    ULH_RCB	    *ulh_rcb;
    DB_STATUS	    ulh_status;

    /* init ULH_RCB */
    ulh_rcb = &global->rdf_dist_ulhcb;
    if (global->rdf_de_ulhobject)
	ulh_rcb->ulh_object = global->rdf_de_ulhobject->rdf_ulhptr;
    if (!ulh_rcb->ulh_object)
	ulh_rcb->ulh_object = global->rdf_def_ulhcb.ulh_object;
    ulh_rcb->ulh_hashid = Rdi_svcb->rdv_def_hashid;

    ulh_status = ulh_destroy(ulh_rcb, (unsigned char *)NULL, (i4)0);
    if (DB_FAILURE_MACRO(ulh_status))
    {
	rdu_ferror(global, ulh_status, &ulh_rcb->ulh_error,
	    E_RD0040_ULH_ERROR,0);
    }
    else
    {
	/* clear the ptr to the unfixed Default object from attribute info */
	if (att_ptr)
	{
	    att_ptr->att_default = NULL;
	    infoblk ->rdr_2_types &= ~RDR2_DEFAULT;
	}
    }
    return(ulh_status);
}

/*{
** Name: rdu_xlock	- mark object as locked or create new private object
**
** Description:
**      Used to mark an object for update by a single thread
**
** Inputs:
**      global                          ptr to RDF state variable
**	infoblkpp			Ptr to infoblk to lock
**	first_lock			flag: true -> called by rdf_gdesc.
**
** Outputs:
**	global				ptr to RDF state variable
**	    .rdf_resources		resources held by this thread
**					May set RDF_RUPDATE (update sysinfoblk)
**	    .rdf_ulhobject		ULH descpiptor for infoblk
**		.rdf_state		state of infoblk:
**					    may be set to RDF_SUPDATE
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      28-oct-87 (seputis)
**          initial creation
**      26-apr-89 (mings)
**          check RDR_REFRESH mask for register with refresh
**      23-jun-89 (mings)
**          add rdf_shared_sessions in RDF_ULHOBJECT
**	20-jun-90 (teresa)
**	    added new parameter to rdu_ferror() for bug 4390.
**	27-may-92 (teresa)
**	    Picked up changes from Star code plus added new parameter first_lock
**	17-sep-92 (teresa)
**	    changed RD0021 -> RD0030 (tracepoint to simulate concurrency)
**	12-mar-93 (teresa)
**	    removed logic to increment rdf_shared_sessions from this routine.
**	23-feb-94 (teresa)
**	    moved checksum validation to rdu_cache() in rdfgetdesc.c.  This is
**	    part of the fix to bug 59336.
**	23-jan-97 (cohmi01)
**	    Remove checksum code that had long been '#if 0'.
**	31-oct-98 (nanpr01)
**	    Was checking the wrong variable for rule, protect, etc. This
**	    caused sysinfoblk's ptuples etc to be set rather than usrinfo
**	    block's ptuples etc.
**	30-Jul-2006 (kschendel)
**	    Don't set RUPDATE if this session already has a private infoblk!
**	    We can't update the system infoblk if we have a private one.
**	    This little gem caused random segv's, usually in psy_view,
**	    thanks to callers thinking that they had the system infoblk
**	    and not returning info into the user one.
**	    (Note that this replaces kibro01's fix for bug 116147.)
[@history_template@]...
*/
DB_STATUS
rdu_xlock(  RDF_GLOBAL		*global,
	    RDR_INFO           **infoblkpp)
{
    DB_STATUS	    status;
    RDF_ULHOBJECT   *rdf_ulhobject;
    RDR_INFO	    *sys_infoblk;
    RDF_CB	    *rdfcb = global->rdfcb;
    i4	    firstval,
		    secondval;
    bool	    priv_flag=FALSE;
    RDF_STATE	    *rdf_state;

    if (global->rdf_resources & RDF_RUPDATE)
	return(E_DB_OK);	    /* check if this thread already has an
				    ** update lock on the table descriptor */

    /* If we already have a private infoblk, just return -- we'll
    ** continue updating it.
    */
    sys_infoblk = global->rdf_ulhobject->rdf_sysinfoblk;
    if (global->rdfcb->rdf_info_blk != sys_infoblk)
    {
	*infoblkpp = global->rdfcb->rdf_info_blk;  /* Just in case! */
	return (E_DB_OK);
    }

    status = rdu_gsemaphore(global); /* protect the update flag
				    ** for reading by this thread */
    if (DB_FAILURE_MACRO(status))
	return(status);

    rdf_ulhobject = (RDF_ULHOBJECT *) sys_infoblk->rdr_object;

    if (global->rdf_resources & (RDF_RINTEGRITY | RDF_RRULE | RDF_RPROTECTION |
			RDF_RPROCEDURE_PARAMETER | RDF_RKEYS | RDF_RSECALARM))
    {
	if (global->rdf_resources & RDF_RINTEGRITY)
	    rdf_state = &rdf_ulhobject->rdf_sintegrity;
	else if (global->rdf_resources & RDF_RRULE)
	    rdf_state = &rdf_ulhobject->rdf_srule;
	else if (global->rdf_resources & RDF_RPROTECTION)
	    rdf_state = &rdf_ulhobject->rdf_sprotection;
	else if (global->rdf_resources & RDF_RPROCEDURE_PARAMETER)
	    rdf_state = &rdf_ulhobject->rdf_sprocedure_parameter;
	else if (global->rdf_resources & RDF_RKEYS)
	    rdf_state = &rdf_ulhobject->rdf_skeys;
	else if (global->rdf_resources & RDF_RSECALARM)
	    rdf_state = &rdf_ulhobject->rdf_ssecalarm;
    }
    else
	    rdf_state = &rdf_ulhobject->rdf_state;

    /* We will create a private info block in the following situations:
    ** 1) the system copy is locked by another thread.
    ** 2) we need to go to cdb and ldb for partial information;
    **	    ex: register with refresh.
    ** 3) the system copy may have been corrputed in memory (RDF_BAD_CHECKSUM)
    ** 4) A trace point is set to request RDF use private infoblks for testing
    **    (every other time rdu_xlock is called, it will force a private infoblk
    **	  to be build if trace is turned on).
    ** (schka24) It would seem to make more sense to simply wait for
    ** whoever has the master locked, if that's the problem.  However
    ** you can't do that blindly because this session may be holding
    ** catalog locks that the master-locking session will need.
    ** So using a private infoblk so that both can proceed may actually
    ** be the simplest approach, as much as I hate to admit it.
    */
    if ( ult_check_macro(&global->rdf_sess_cb->rds_strace, RD0030,
			 &firstval, &secondval)
       )
	priv_flag = (RDF_privctr++ % 2 ) ? TRUE : FALSE;

    if (*rdf_state == RDF_SUPDATE
	||
	rdfcb->rdf_rb.rdr_types_mask & RDR_REFRESH
	||
	global->rdf_resources & RDF_BAD_CHECKSUM
        ||
	priv_flag
       )
    {
	/* another thread is updating the ULH object so
	** create another stream to be used by this thread
	** for allocating the cached object, ... this is
	** necessary since semaphores cannot be held across
	** DMF calls, so several threads may update the
	** same object,... create a new stream which is
	** private to the thread which is attempting the
	** concurrent update
	*/

	/* create a private infoblk */
	status = rdu_private(global, infoblkpp);
	/* report statistics */
	if (DB_SUCCESS_MACRO(status))
	{
	    /* update statistics */
	    Rdi_svcb->rdvstat.rds_l0_private++;
	    if (Rdi_svcb->rdvstat.rds_l0_private == 0)
		Rdi_svcb->rdvstat.rds_l1_private++;

	}
    }
    else
    {
	/* reserve this shared object as being updated by this
	** thread */
	global->rdf_resources |= RDF_RUPDATE; /* mark this
			    ** thread has having update status
			    */
	*rdf_state = RDF_SUPDATE; /* mark the
			    ** object as being in the process
			    ** of being updated */

	/* report statistics */
	Rdi_svcb->rdvstat.rds_l2_supdate++;
	if (Rdi_svcb->rdvstat.rds_l2_supdate== 0)
	    Rdi_svcb->rdvstat.rds_l3_supdate++;

    }
    return(status);
}

/*{
** Name: rdu_malloc - Allocate memory
**
** Description:
**
**      This function allocates memory for RDF in one of the following ways:
**	    1.  RDF LDBdesc cache memory - from RDF's LDBdesc memory pool
**	    2.  Caller supplied memory - via an initialized ULM_CB.
**	    3.  RDF RELcache or QTREEcache memory - from RDF's memory pool
**
**	When allocating memory from the RDF cache (LDBdesc, REL or QTREE),
**	rdu_malloc() will automatically try to recover from out-of-memory
**	errors.  It does this by removing unfixed objects from the appropriate
**	cache until it reclaims enough memory or until there are no more unfixed
**	objects to destroy.  Rdu_malloc() accomplishes this automatic memory
**	recovery by calling subroutine rdu_mrecover().
**
**	When RDF is asked to allocate memory from another facility's memory
**	stream (i.e., global.rdf_fulmcb is NOT NULL), then rdu_malloc does not
**	attempt to recover from an out-of-memory error.
**
** Inputs:
**	svcb		    	    Server Control Block
**	    .rdv_poolid	    	    Memory Pool ID
**	    .rdv_memleft	    Amount of memory (bytes) available in pool
**	Global			    RDF Global variables
**	    .rdf_resources	    Bitmask of resources held by current session
**	    .rdf_init		    Bitmask indicating if certain control blocks
**				     have already been initialized
**	    .rdfcb		    RDF Control Block
**		.rdf_info_blk	    Fixed Cache Object
**		    .stream_id	    Memory Stream Id
**		.rdf_rb		    RDF Request Block
**		    .rdr_fcb	    Facility Control Block
**	    .rdf_d_ulhobject	    ULH header for a LDBDESC cache object
**		.rdf_pstream_id	    Memory Stream id
**	    .rdf_dist_ulhcb	    ULH_CB for LDBDESC cache
**		.ulh_object	    ULH header for a LDBDESC  cache object
**		    .ulh_streamid   Memory Stream id
**	    .rdf_fulmcb		    Initialized Faciltiy ULM_CB.
**	    .rdf_ulhcb		    ULH_CB for REL/QTREE cache
**		.ulh_object	    ULH header for a REL/QTREE cache object
**		    .ulh_streamid   Memory Stream id
**	    .rdf_ulhobject	    ULH header for a REL/QTREE cache object
**		.rdf_sysinfoblk	    Cache Object for REL/QTREE cache
**		    .stream_id	    Memory Stream Id
**	psize			    Number of bytes to allocate
**
** Outputs:
**	pptr			    Pointer to initialized memory
**	Global			    RDF Global variables
**	    .rdfcb		    RDF Control Block
**		.rdf_error	    RDF Error Block
**		    .err_code	    Error code, could be a ULM error forwarded,
**				     or one of the following RDF errors:
**					E_RD0117_STREAMID
**					E_RD000B_MEM_ERROR
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**		none
**
** Side Effects:
**	Unfixed cache objects may be removed from the cache so that the memory
**	is available to honor this request.
**
** History:
**	11-may-87 (puree)
**          written
**      11-nov-87 (seputis)
**          rewritten for resource management
**	17-Oct-89 (teg)
**	    change logic to get memleft from Server Control Block, since RDF
**	    now allocates its own memory.  This is part of the fix for bug 5446.
**	23-jan-92 (teresa)
**	    SYBIL: rdi_hashid->rdv_rdesc_hashid; rdi_poolid->rdv_poolid;
**		   rdv_c1_memleft->rdv_memleft;
**	18-may-92 (teresa)
**	    add support for LDBcache allocation.
**	07-jan-93 (teresa)
**	    add support to use initialized ULM_CB supplied by another facility.
**	    (Also redo the description, inputs and outputs sections since they
**	    do not even vagely resemble what is going on in this routine.)
**	20-may-93 (teresa)
**	    Add support to allocate memory for defaults cache objects.
**	14-sep-93 (swm)
**	    Change casts in pointer comparison to char *.
**	05-Jun-1997 (shero03)
**	    If the object is private call palloc otherwise call spalloc.
**	30-Nov-2000 (jenjo02)
**	    If facility_supplied_ulmcb and ULM fails, return ulm_error
**	    in caller's rdfcb->rdf_error.
**	21-Mar-2003 (jenjo02)
**	    Removed use of RDF_D_IULM and RDF_DE_IULM flags. Once set,
**	    they're never reset and cause memory to be allocated from
**	    another object's stream; if that DESTROYABLE object gets
**	    LRU'd out, away goes its memory stream and the memory
**	    for -this- object!
**	31-Dec-2003 (schka24)
**	    Log whenever we garbage collect.
*/
DB_STATUS
rdu_malloc( RDF_GLOBAL	*global,
	    i4		psize,
	    PTR		*pptr)
{
    ULM_RCB	    *ulm_rcb;		/* ptr to ULM control block being
					** initialized */
    RDI_FCB	    *rdi_fcb;		/* ptr to facility control block
					** initialized at server startup time */
    i4		    mrecover_cnt = 0;
    DB_STATUS	    status;
    bool	    facility_supplied_ulmcb = FALSE; /* set true if RDF is
					** allocating memory from another
					** facility's memory stream.
					*/

    if (global->rdf_resources & RDF_BLD_LDBDESC)
    {
	ulm_rcb = &global->rdf_dulmcb;
	/* init the ULM control block for memory allocation */
	rdi_fcb = (RDI_FCB *)(global->rdfcb->rdf_rb.rdr_fcb);
	ulm_rcb->ulm_poolid = Rdi_svcb->rdv_poolid;
	ulm_rcb->ulm_memleft = &Rdi_svcb->rdv_memleft;
	ulm_rcb->ulm_facility = DB_RDF_ID;
	ulm_rcb->ulm_blocksize = 0;	/* No prefered block size */
	ulm_rcb->ulm_streamid_p = &ulm_rcb->ulm_streamid;
	if (global->rdf_resources & RDF_D_RUPDATE)
	{   /* if an exclusive lock has been obtained on the LDB descriptor
	    ** then use the system memory stream */
	    if (!global->rdf_d_ulhobject)
	    {   /* this must be the first allocation for this obj since
		** the ulhobject is not defined */
		if (global->rdf_init & RDF_D_IULH)
		{   /* get the stream id from the ulh descriptor */
		    ulm_rcb->ulm_streamid_p
			= &global->rdf_dist_ulhcb.ulh_object->ulh_streamid;
		}
		else
		{   /* cannot find where to get streamid so report error */
		    status = E_DB_SEVERE;
		    rdu_ierror(global, status, E_RD0117_STREAMID);
		    return(status);
		}
	    }
	    else
	    {	/* allocate memory from system descriptor */
		ulm_rcb->ulm_streamid_p
		    = &global->rdf_d_ulhobject->rdf_pstream_id;
	    }
	}
    }
    else if (global->rdf_resources & RDF_DEF_DESC)
    {
	ulm_rcb = &global->rdf_deulmcb;
	/* init the ULM control block for memory allocation */
	rdi_fcb = (RDI_FCB *)(global->rdfcb->rdf_rb.rdr_fcb);
	ulm_rcb->ulm_poolid = Rdi_svcb->rdv_poolid;
	ulm_rcb->ulm_memleft = &Rdi_svcb->rdv_memleft;
	ulm_rcb->ulm_facility = DB_RDF_ID;
	ulm_rcb->ulm_blocksize = 0;	/* No prefered block size */
	ulm_rcb->ulm_streamid_p = &ulm_rcb->ulm_streamid;
	if (global->rdf_resources & RDF_DE_RUPDATE)
	{   /* if an exclusive lock has been obtained on the default
	    ** then use the system memory stream */
	    if (!global->rdf_de_ulhobject)
	    {   /* this must be the first allocation for this obj since
		** the ulhobject is not defined */
		if (global->rdf_init & RDF_DE_IULH)
		{   /* get the stream id from the ulh descriptor */
		    ulm_rcb->ulm_streamid_p
			= &global->rdf_def_ulhcb.ulh_object->ulh_streamid;
		}
		else
		{   /* cannot find where to get streamid so report error */
		    status = E_DB_SEVERE;
		    rdu_ierror(global, status, E_RD0117_STREAMID);
		    return(status);
		}
	    }
	    else
	    {	/* allocate memory from system descriptor */
		ulm_rcb->ulm_streamid_p
		    = &global->rdf_de_ulhobject->rdf_pstream_id;
	    }
	}
    }
    else if (global->rdf_fulmcb)
    {
	/*
	** the calling facility has supplied an initialized ULM_CB and
	** requests that RDF use memory from it rather than from RDF's
	** memory stream.
	*/
	ulm_rcb = global->rdf_fulmcb;
	facility_supplied_ulmcb	= TRUE;
    }
    else /* must be allocating memory for a local cache. */
    {
	ulm_rcb = &global->rdf_ulmcb;
	if (!(global->rdf_init & RDF_IULM))
	{
	    /* init the ULM control block for memory allocation */
	    rdi_fcb = (RDI_FCB *)(global->rdfcb->rdf_rb.rdr_fcb);
	    ulm_rcb->ulm_poolid = Rdi_svcb->rdv_poolid;
	    ulm_rcb->ulm_memleft = &Rdi_svcb->rdv_memleft;
	    ulm_rcb->ulm_facility = DB_RDF_ID;
	    ulm_rcb->ulm_blocksize = 0;	/* No prefered block size */
	    if (global->rdf_resources & RDF_RUPDATE)
	    {
		/*
		** if an exclusive lock has been obtained on the system
		** descriptor then use the system memory stream
		*/
		if (!global->rdf_ulhobject)
		{
		    /*
		    ** this must be the first allocation for this table since
		    ** the ulhobject is not defined
		    */
		    if (global->rdf_init & RDF_IULH)
		    {
			/* get the stream id from the ulh descriptor */
			ulm_rcb->ulm_streamid_p
			    = &global->rdf_ulhcb.ulh_object->ulh_streamid;
		    }
		    else
		    {
			/* cannot find where to get streamid so report error */
			status = E_DB_SEVERE;
			rdu_ierror(global, status, E_RD0117_STREAMID);
			return(status);
		    }
		}
		else
		{
		    /* allocate memory from system descriptor */
		    ulm_rcb->ulm_streamid_p
			= &global->rdf_ulhobject->rdf_sysinfoblk->stream_id;
		}
	    }
	    else
	    {   /* need to get this out of the user private memory stream */
		ulm_rcb->ulm_streamid_p =
		    &global->rdfcb->rdf_info_blk->stream_id;
		if (global->rdfcb->rdf_info_blk ==
		    global->rdf_ulhobject->rdf_sysinfoblk)
		{
		    /*
		    ** this should be a private stream so report an error
		    ** if not
		    */
		    status = E_DB_SEVERE;
		    rdu_ierror(global, status, E_RD0117_STREAMID);
		    return(status);
		}
	    }
	    global->rdf_init |= RDF_IULM;
	} /* end of initializing relation or qtree ULM control block */
    } /* end of relation or qtree or LDBdesc cache processing */
    ulm_rcb->ulm_psize = psize;

    /*
    ** jenjo02: removed check of streamid. ULM now does this validation,
    **		so the check here is redundant.
    **		Also removed use of ulm_spalloc(). ULM streams are now
    **	        defined as PRIVATE or SHARED. SHARED streams will be
    **	        semaphore protected by ULM; PRIVATE streams will not.
    **		The stream creator (ulm_openstream()) has already
    **	        established this stream characteristic, so we needn't
    **	   	check for RDF_SPRIVATE here.
    */

    do
    {
	status = ulm_palloc(ulm_rcb);

	if ( DB_SUCCESS_MACRO(status) )
	{
	    /* either we got the memory we requested or RDF was allocating
	    ** memory from another facility and got an error.  In either
	    ** case, RDF does not attempt to recover any memory from it's
	    ** own cache.
	    */
	    *pptr = ulm_rcb->ulm_pptr;
	    break;
	}
	else if ( facility_supplied_ulmcb )
	{
	    /* Return ULM failure reason in caller's rdfcb */
	    STRUCT_ASSIGN_MACRO(ulm_rcb->ulm_error, global->rdfcb->rdf_error);
	}
	else
	{
	    /* attempt to recover from memory error by garbage collection */
	    mrecover_cnt++;
	    TRdisplay("%@ [%x] Garbage-collecting RDF: err %d psize %d (poolid %x, memleft %d try %d)\n",
		global->rdf_sess_id, ulm_rcb->ulm_error.err_code, psize,
		ulm_rcb->ulm_poolid, *ulm_rcb->ulm_memleft, mrecover_cnt);
	    status = rdu_mrecover(global, ulm_rcb, status, E_RD000B_MEM_ERROR);
	}

    } while (DB_SUCCESS_MACRO(status));

    return(status);
}

/*{
** Name: rdu_master_infodump - Figure if there are any requests to dump
**                             RDF table information then do so.
**
**      Internal call format:   rdu_master_infodump(info,svcb);
**
** Description:
**      This function sees if any dump RDR_INFO trace requests have been set.
**	If so, it calls the appropriate routine to dump the rdf information.
**	Traces are as follows:	   (any combination may be set)
**	111   -- Dump all RDR_INFO information (ie,do dumps 2 thru 12)
**      112   -- Dump the relation information.
**      113   -- Dump the attributes information.
**      114   -- Dump the indexes information.
**      115   -- Dump integrity tuples.
**      116   -- Dump protection tuples.
**      117   -- Dump the statistics information.
**      118   -- Dump attribute hash table information.
**      119   -- Dump primary key information.
**	120   -- Dump iirule tuples.
**	121   -- Dump view information.
**      122   -- Dump RDR_INFO strcuture (high level RDF info)
**	123   -- Dump LDBdesc cache object
**
**	If any non-zero parameter is supplied with the trace point, then
**	trace info on INGRES owned (or system) catalogs is supressed.  If
**	no parameter is specified or a value of 0 is specified, then
**	system catalog trace info is also dumped.  Examples:
**
**	Example 1:  Supress system catalog info
**	    set trace terminal\g
**	    set trace point rd111 1\g
**
**	Example 2:  Do not supress system catalog info
**	    set trace terminal\g
**	    set trace point rd111\g
**
**	(See <rdftrace.h> for a complete list of RDF traces.)
**
**	NOTE: these trace points are session specific, not server wide.
**
** Inputs:
**      info		Pointer to the table information block [RDR_INFO]
**	fcb		Pointer to RDF Facility control block
**	    .rdi_fac_id	    id of requesting facility (OPF, PSF or SCF)
**
** Outputs:
**      none
**      Returns:
**              none
**      Exceptions:
**              none
** Side Effects:
**              none
** History:
**      08-dec-1989 (teg)
**          written
**	17-sep-92 (teresa)
**	    udpated to support sybil traces.
**	09-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Use rds_cat_owner rather than "$ingres" in rdu_master_infodump()
**	17-Nov-2005 (schka24)
**	    No dumps if no session (e.g. invalidate dbevent).
*/
VOID
rdu_master_infodump(	RDR_INFO        *info,
			RDF_GLOBAL	*global,
			RDF_CALLER	caller,
			RDF_SVAR	request)
{
#ifdef xDEV_TEST
    RDI_FCB		*fcb = (RDI_FCB *)global->rdfcb->rdf_rb.rdr_fcb;

    i4             v1=0;
    i4             v2=0;
    i4                  traceflag;
    bool		rd111=FALSE;
    bool		rd112=FALSE;
    bool		rd113=FALSE;
    bool		rd114=FALSE;
    bool		rd115=FALSE;
    bool		rd116=FALSE;
    bool		rd117=FALSE;
    bool		rd118=FALSE;
    bool		rd119=FALSE;
    bool		rd120=FALSE;
    bool		rd121=FALSE;
    bool		rd122=FALSE;
    bool		rd123=FALSE;
    bool		rd124=FALSE;
    bool		rd125=FALSE;
    bool		skip_ingres=FALSE;

    /* if designated block to dump is null, then return */
    if (info == NULL || global->rdf_sess_cb == NULL)
	return;

    /* first see which trace flags are set */
    if ((caller == RDFGETDESC) || (caller == RDFGETINFO))
    {
	rd111 = ult_check_macro(&global->rdf_sess_cb->rds_strace, RD0111,
				&v1, &v2);
	if (v1 & rd111)
	    skip_ingres = TRUE;
    }
    else if (caller == RDFUNFIX)
    {
	rd124 = ult_check_macro(&global->rdf_sess_cb->rds_strace, RD0124,
			        &v1, &v2);
        if (v1 & rd124)
	    skip_ingres = TRUE;
	/* do not continue processing if called from rdf_unfix and trace
	** point rd124 is not set
	*/
	if (!rd124)
	    return;
    }
    else if (caller == RDFINVALID)
    {
	rd125 = ult_check_macro(&global->rdf_sess_cb->rds_strace, RD0125,
			        &v1, &v2);
	if (v1 & rd125)
	    skip_ingres = TRUE;
	/* do not continue processing if called from rdf_invalidate and trace
	** point rd125 is not set
	*/
	if (!rd125)
	    return;
    }
    else
	return;	/* return if called by an unexpected source */

    if (rd111)
    {
	/* called to dump infoblk at create time */

	/* this automatically includes trace points RD112 to RD122, so dont
	** waste the CPU cycles of ult_check_macro.  However, trace RD113 calls
	** RD118 and RD119, so dont set those here.
	*/
	if (caller == RDFGETDESC)
	{
	    rd112 = TRUE;
	    rd113 = TRUE;
	    rd114 = TRUE;
	    rd122 = TRUE;
	    rd123 = TRUE;
	}
	else
	{
	    if (request & RDR_INTEGRITIES)
		rd115 = TRUE;
	    if (request & RDR_PROTECT)
		rd116 = TRUE;
	    if (request & RDR_STATISTICS)
		rd117 = TRUE;
	    if (request & RDR_RULE)
		rd120 = TRUE;
	    if (request & RDR_VIEW)
		rd121 = TRUE;
	}
    }
    else if (rd124 || rd125)
    {
	/* called to dump infoblk at unfix or invalidate time */
	rd112 = TRUE;
	rd113 = TRUE;
	rd114 = TRUE;
	rd115 = TRUE;
	rd115 = TRUE;
	rd117 = TRUE;
	rd120 = TRUE;
	rd121 = TRUE;
	rd122 = TRUE;
	rd123 = TRUE;
    }
    else
    {
	/* check for any combination of RDF RDI_INFO trace flags */

	rd112 = ult_check_macro(&global->rdf_sess_cb->rds_strace, RD0112,
				&v1, &v2);
	if ( (v1 != NULL) && rd112)
	    skip_ingres = TRUE;
	rd113 = ult_check_macro(&global->rdf_sess_cb->rds_strace, RD0113,
				&v1, &v2);
	if ((v1 != NULL) &&  rd113)
	    skip_ingres = TRUE;
	rd114 = ult_check_macro(&global->rdf_sess_cb->rds_strace, RD0114,
				&v1, &v2);
	if ((v1 != NULL) &&  rd114)
	    skip_ingres = TRUE;
	rd115 = ult_check_macro(&global->rdf_sess_cb->rds_strace, RD0115,
				&v1, &v2);
	if ((v1 != NULL) &&  rd115)
	    skip_ingres = TRUE;
	rd116 = ult_check_macro(&global->rdf_sess_cb->rds_strace, RD0116,
				&v1, &v2);
	if ((v1 != NULL) &&  rd116)
	    skip_ingres = TRUE;
	rd117 = ult_check_macro(&global->rdf_sess_cb->rds_strace, RD0117,
				&v1, &v2);
	if ((v1 != NULL) &&  rd117)
	    skip_ingres = TRUE;
	rd118 = ult_check_macro(&global->rdf_sess_cb->rds_strace, RD0118,
				&v1, &v2);
	if ((v1 != NULL) &&  rd118)
	    skip_ingres = TRUE;
	rd119 = ult_check_macro(&global->rdf_sess_cb->rds_strace, RD0119,
				&v1, &v2);
	if ((v1 != NULL) &&  rd119)
	    skip_ingres = TRUE;
	rd120 = ult_check_macro(&global->rdf_sess_cb->rds_strace, RD0120,
				&v1, &v2);
	if ((v1 != NULL) &&  rd120)
	    skip_ingres = TRUE;
	rd121 = ult_check_macro(&global->rdf_sess_cb->rds_strace, RD0121,
				&v1, &v2);
	if ((v1 != NULL) &&  rd121)
	    skip_ingres = TRUE;
	rd122 = ult_check_macro(&global->rdf_sess_cb->rds_strace, RD0122,
				&v1, &v2);
	if ((v1 != NULL) &&  rd122)
	    skip_ingres = TRUE;
	rd123 = ult_check_macro(&global->rdf_sess_cb->rds_strace, RD0123,
				&v1, &v2);
	if ((v1 != NULL) &&  rd123)
	    skip_ingres = TRUE;
    }

    /*
    ** Proceed only if there is something in the infoblk to dump.
    **
    ** (if skip_ingres flag is set and owner is '$ingres' or 'ingres', then dont
    ** bother dumping the contents.  This means that only owner objects will get
    ** dumped, not system objects)
    */
    if (((rd112 && (info->rdr_rel != NULL))    ||
	 (rd113 && (info->rdr_attr != NULL))   ||
	 (rd114 && (info->rdr_indx != NULL))   ||
	 (rd115 && (info->rdr_ituples != NULL))||
	 (rd116 && (info->rdr_ptuples != NULL))||
	 (rd117 && (info->rdr_histogram != NULL)) ||
	 (rd118 && (info->rdr_atthsh != NULL)) ||
	 (rd119 && (info->rdr_keys != NULL))   ||
	 (rd120 && (info->rdr_rtuples != NULL))||
	 (rd121 && (info->rdr_view != NULL))   ||
	 (rd123 && (info->rdr_obj_desc != NULL))  ) == FALSE)
		return;
    if (skip_ingres)
    {
	char	*ingres_name = global->rdf_sess_cb->rds_cat_owner;

	/* Check to see if table owned by $ingres */
	if ( MEcmp( (PTR) &info->rdr_rel->tbl_owner,
		    (PTR) ingres_name, sizeof(DB_OWN_NAME)) == 0)
	    return;
	++ingres_name;
	/* Check to see if table owned by ingres */
	if ( MEcmp( (PTR) &info->rdr_rel->tbl_owner,
		    (PTR) ingres_name, sizeof(DB_OWN_NAME)-1) == 0)
	    return;
    }

    if (fcb->rdi_fac_id == DB_PSF_ID)
    {
	TRdisplay("\n\n--------------------------------------------------------\n");
	TRdisplay("-- Object Name: %#s     Calling Facility: PSF\n",
		 DB_TAB_MAXNAME, &info->rdr_rel->tbl_name);
    }
    if (fcb->rdi_fac_id == DB_OPF_ID)
    {
	TRdisplay("\n\n--------------------------------------------------------\n");
	TRdisplay("-- Object Name: %#s     Calling Facility: OPF\n",
		  DB_TAB_MAXNAME, &info->rdr_rel->tbl_name);
    }
    if (fcb->rdi_fac_id == DB_SCF_ID)
    {
	TRdisplay("\n\n--------------------------------------------------------\n");
	TRdisplay("-- Object Name: %#s     Calling Facility: SCF\n",
		 DB_TAB_MAXNAME, &info->rdr_rel->tbl_name);
    }

    switch (caller)
    {
	case RDFGETDESC:
	case RDFGETINFO:
	    TRdisplay("  TRACE OF CREATION OF CACHE OBJECT\n");
	    break;
	case RDFINVALID:
	    TRdisplay("  TRACE OF INVALIDATION OF CACHE OBJECT\n");
	    break;
	case RDFUNFIX:
	    TRdisplay("  TRACE OF CACHE OBJECT UNFIX\n");
	    break;
    }
    TRdisplay("--------------------------------------------------------\n");

    /*
    **  check for individual dump requests
    */

    /* check request to dump RDI_INFO contents */
    if (rd122)
    {
	rdu_info_dump(info);
    }

    /* check request to dump the relation information */
    if (rd112)
    {
	rdu_rel_dump(info);
    }

    /* check request to dump the attributes information */
    if (rd113)
    {
	rdu_attr_dump(info);
    }

    /* check request to dump the indexes information */
    if (rd114)
    {
	rdu_indx_dump(info);
    }

    /* check request to dump the integrity tuples */
    if (rd115)
    {
	rdu_ituple_dump(info);
    }

    /* check request to dump the protection tuples */
    if (rd116)
    {
	rdu_ptuple_dump(info);
    }

    /* check request to dump the statistics information */
    if (rd117)
    {
	i4	att_start;
	i4	att_end;

	/* if building infoblk, dump histogram for attribute just built
	** else dump all attributes
	*/
	if (caller == RDFGETINFO)
	    att_start=att_end= global->rdfcb->rdf_rb.rdr_hattr_no;
	else
	{
	    att_start = 1;
	    att_end = info->rdr_no_attr;
	}
	rdu_hist_dump(info,att_start,att_end);
    }

    /* check request to dump the hash table attribute information */
    if (rd118)
    {
	rdu_atthsh_dump(info);
    }

    /* check request to dump primary key information */
    if (rd119)
    {
	rdu_keys_dump(info);
    }

    /* check request to dump iirule tuple information */
    if (rd120)
    {
	rdu_rtuple_dump(info);
    }

    /* check request to dump view information */
    if (rd121)
    {
	rdu_view_dump(info);
    }

    if (rd123)
    {
	rdu_ldbdesc_dump(info);
    }
#endif
}

#ifdef xDEV_TEST
/*{
** Name: rdu_info_dump - RDF table information block dump.
**
**	Internal call format:	rdu_info_dump(rdf_info_blk);
**
** Description:
**      This function dump the table information block.
**
** Inputs:
**	rdf_info_blk	    Pointer to the table information block.
**
** Outputs:
**	none
**	Returns:
**		none
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	10-apr-86 (ac)
**          written
**	11-dec-89 (teg)
**	    modified to dump every element in RDR_INFO, many of which are
**	    pointers to other memory segments.  Removed dump of iirelation,
**	    iiattribute and iiindex info.  To dump those, user must set
**	    RD0112, RD0113 and/or RD0114 (respectively).
**	17-sep-92 (teresa)
**	    udpated to print rdr_object and rdr_star_obj.
**	11-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
*/
VOID
rdu_info_dump(RDR_INFO	*info)
{

	if (info == NULL)
	    return;

	TRdisplay("\n---------------------- rdr_info ------------------------\n");
	TRdisplay("address of relation entry %p\n", info->rdr_rel);
	TRdisplay("address of attributes entry %p\n", info->rdr_attr);
	TRdisplay("address of index entry %p\n", info->rdr_indx);
	TRdisplay("address of integrity tuples entry %p\n", info->rdr_ituples);
	TRdisplay("address of protection tuples entry %p\n", info->rdr_ptuples);
	TRdisplay("address of rule tuples entry %p\n", info->rdr_rtuples);
	TRdisplay("address of view entry %p\n", info->rdr_view);
	TRdisplay("address of histogram entry %p\n", info->rdr_histogram);
	TRdisplay("address of key array %p\n", info->rdr_keys);
	TRdisplay("address of hash table of attributes %p\n", info->rdr_atthsh);
	TRdisplay("no of attributes %d\n", info->rdr_no_attr);
	TRdisplay("no of indexes %d\n", info->rdr_no_index);
	TRdisplay("valid status %d\n", info->rdr_invalid);
	TRdisplay("rdr_types %x\n", info->rdr_types);
	TRdisplay("time stamp high %x\n", info->rdr_timestamp.db_tab_high_time);
	TRdisplay("time stamp low %x\n", info->rdr_timestamp.db_tab_low_time);
	TRdisplay("stream id ptr %p\n", info->stream_id);
	TRdisplay("address of db procedure tuples entry %0p\n", info->rdr_dbp);
	TRdisplay("address of STAR object descriptor %0p\n",
		    info->rdr_obj_desc);
	TRdisplay("ldb id %x\n",info->rdr_ldb_id);
	TRdisplay("ddb id %x\n",info->rdr_ddb_id);
	TRdisplay("local table schema mask %x\n", info->rdr_local_info);
	TRdisplay("checksum %d\n", info->rdr_checksum);
	TRdisplay("RDF ULH Object Hdr %p\n", info->rdr_object);
	TRdisplay("RDF Distributed ULH Object Hdr %p\n", info->rdr_star_obj);
}

/*{
** Name: rdu_rel_dump - RDF relation informatin dump.
**
**	Internal call format:	rdu_rel_dump(rdf_info_blk);
**
** Description:
**      This function dump the relation information.
**
** Inputs:
**	rdf_info_blk	    Pointer to the table information block.
**
** Outputs:
**	none
**	Returns:
**		none
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	10-apr-86 (ac)
**          written
**	11-dec-89 (teg)
**	    updated table names to 24 char, added table location count, fixed
**	    typos.
*/
VOID
rdu_rel_dump(RDR_INFO	*info)
{
	if (info == NULL)
	    return;

	if (info->rdr_rel == NULL)
	{
	    TRdisplay("WARNING:  relation info is missing from this object!");
	    return;
	}

	TRdisplay("\n--------------------- rdr_rel ----------------------------\n");
	TRdisplay("table id :  base : %d  index : %d\n",
	 info->rdr_rel->tbl_id.db_tab_base, info->rdr_rel->tbl_id.db_tab_index);
	TRdisplay("table name :  %#s\n", 
	    DB_TAB_MAXNAME, &info->rdr_rel->tbl_name);
	TRdisplay("table owner :  %#s\n", 
	    DB_OWN_MAXNAME, &info->rdr_rel->tbl_owner);
	TRdisplay("table location count: %d\n",
	    info->rdr_rel->tbl_loc_count);
	TRdisplay("table default location name :  %#s\n",
	     DB_LOC_MAXNAME, &info->rdr_rel->tbl_location);
	TRdisplay("table file name :  %12s\n", &info->rdr_rel->tbl_filename);
	TRdisplay("table attribute count :  %d\n",
	    info->rdr_rel->tbl_attr_count);
	TRdisplay("table indexes count :  %d\n",
	    info->rdr_rel->tbl_index_count);
	TRdisplay("table record width :  %d\n", info->rdr_rel->tbl_width);
	TRdisplay("table storage type :  %d\n",
	    info->rdr_rel->tbl_storage_type);
	TRdisplay("table status mask :  %x\n", info->rdr_rel->tbl_status_mask);
	TRdisplay("table estimated record count :  %d\n",
	    info->rdr_rel->tbl_record_count);
	TRdisplay("table estimated page count :  %d\n",
	    info->rdr_rel->tbl_page_count);
	TRdisplay("table estimated data page count :  %d\n",
	    info->rdr_rel->tbl_dpage_count);
	TRdisplay("table estimated index page count :  %d\n",
	    info->rdr_rel->tbl_ipage_count);
	TRdisplay("table expiration date :  %d\n",
	    info->rdr_rel->tbl_expiration);
	TRdisplay("table date_modified :  high_time:  %d low_time %d\n",
	    info->rdr_rel->tbl_date_modified.db_tab_high_time,
	    info->rdr_rel->tbl_date_modified.db_tab_low_time);
	TRdisplay("table create time:  :  %d \n",
	    info->rdr_rel->tbl_create_time);
	TRdisplay("table query text id of the view:  :  high %d  low %d\n",
	    info->rdr_rel->tbl_qryid.db_qry_high_time,
	    info->rdr_rel->tbl_qryid.db_qry_low_time);
	TRdisplay("table struct_mod_date :   %d \n",
	    info->rdr_rel->tbl_struct_mod_date);
	TRdisplay("table fill factor of index page :  %d \n",
	    info->rdr_rel->tbl_i_fill_factor);
	TRdisplay("table fill factor of data page :  %d \n",
	    info->rdr_rel->tbl_d_fill_factor);
	TRdisplay("table fill factor of btree leaf page :  %d \n",
	    info->rdr_rel->tbl_l_fill_factor);
	TRdisplay("table minimum number of pages specified for modify:  %d \n",
	    info->rdr_rel->tbl_min_page);
	TRdisplay("table maximum number of pages specified for modify:  %d \n",
	    info->rdr_rel->tbl_max_page);
	TRdisplay("gateway id : %d \n",
	    info->rdr_rel->tbl_relgwid);
	TRdisplay("gateway specific info : %x \n",
	    info->rdr_rel->tbl_relgwother);
	TRdisplay("Smart disk type : %x \n",
	    info->rdr_rel->tbl_sdtype);
	TRdisplay("Temporary table : %x \n",
	    info->rdr_rel->tbl_temporary);
}

/*{
** Name: rdu_his_dump - RDF histogram information dump.
**
**	Internal call format:	rdu_hist_dump(rdf_info_blk,att_no);
**
** Description:
**      This function dump the histogram information.
**
** Inputs:
**	rdf_info_blk	    Pointer to the table information block.
**	att_start	    first attribute number to dump statistics for.
**	att_end		    last attribute number to dump statistics for.
**
** Outputs:
**	none
**	Returns:
**		none
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	10-apr-86 (ac)
**          written
**	11-dec-89 (teg)
**	    modified to dujp snumcells+1 elements of datavalue.
**	29-aug-2002 (bolke01)
**	    modified to include output of per cell data f4count, f4repf).
*/
VOID
rdu_hist_dump(RDR_INFO	*info,i4 att_start, i4  att_end)
{
	i4	    i, j;

	if (info == NULL)
	    return;

	if (info->rdr_histogram == NULL)
	    return;

	TRdisplay("\n-------------------- histogram --------------------------\n");

	for (i=att_start; i <= att_end; i++)
	{
	    if (info->rdr_histogram[i] != NULL)
	    {
		TRdisplay("\nHistogram for attribute number %d\n", i);
		TRdisplay("number of unique values :  %7.3f\n",
		    info->rdr_histogram[i]->snunique);
		TRdisplay("repition factor :  %7.3f\n",
		    info->rdr_histogram[i]->sreptfact);
		TRdisplay("number of cells :  %d\n",
		    info->rdr_histogram[i]->snumcells);
		TRdisplay("all values for the column are unique ?:  %d\n",
		    info->rdr_histogram[i]->sunique);
		TRdisplay("complete ?      :  %d\n",
		    info->rdr_histogram[i]->scomplete);
		TRdisplay("domain          :  %d\n",
		    info->rdr_histogram[i]->sdomain);
		TRdisplay("stat version    :  %s\n",
		    info->rdr_histogram[i]->sversion);
		    TRdisplay("cell      f4count        f4repf    \n");
		for (j=0; j < info->rdr_histogram[i]->snumcells; j++)
		{
		    TRdisplay("[%4d] :  %15.6f, %15.6\n", j+1,
			info->rdr_histogram[i]->f4count[j], info->rdr_histogram[i]->f4repf[j]);

		}
		for (j=0; j <= info->rdr_histogram[i]->snumcells; j++)
		{
		    TRdisplay("datavalue[%d] :  %x\n", j,
			info->rdr_histogram[i]->datavalue[j]);
		}
	    }

	}
}


/*{
** Name: rdu_ituple_dump - RDF integrity tuples dump.
**
**	Internal call format:	rdu_ituple_dump(rdf_info_blk);
**
** Description:
**      This function dump the integrity tuples in the table information block.
**
** Inputs:
**	rdf_info_blk	    Pointer to the table information block.
**
** Outputs:
**	none
**	Returns:
**		none
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	10-apr-86 (ac)
**          written
**	06-feb-89 (ralph)
**	    changed to use DB_COL_WORDS when dumping
**	    dbi_domset array from iiintegrity catalog
**	11-dec-89 (teg)
**	    fixed AV by checking rdr_ituples for null before using, added
**	    dump of dbi_seq
**	15-oct-92 (rblumer)
**	    print out new fields in DB_INTEGRITY tuple.
**          Also, change dbi_domset to dbi_columns.
*/
VOID
rdu_ituple_dump(RDR_INFO *info)
{
	i4	    i = 0, j;
	QEF_DATA    *data;

	if (info == NULL)
	    return;

	if(info->rdr_ituples == NULL)
	    return;

	TRdisplay("\n--------------------- Integrity --------------------------\n");
	TRdisplay("Number of integrity tuples retrieved:  %d\n",
	    info->rdr_ituples->qrym_cnt);
	data = info->rdr_ituples->qrym_data;

	while(data != NULL && i < info->rdr_ituples->qrym_cnt)
	{
	    DB_INTEGRITY    *tuple;

	    tuple  = (DB_INTEGRITY *)(data->dt_data);
	    TRdisplay ("\nIntegrity Tuple Number: %d\n",i);
	    TRdisplay("table id :  base : %d  index : %d\n",
		tuple->dbi_tabid.db_tab_base, tuple->dbi_tabid.db_tab_index);
	    TRdisplay("query text id:  high : %d  low : %d\n",
		tuple->dbi_txtid.db_qry_high_time,
		tuple->dbi_txtid.db_qry_low_time);
	    TRdisplay("tree id:  high : %d  low : %d\n",
		tuple->dbi_tree.db_tre_high_time,
		tuple->dbi_tree.db_tre_low_time);
	    TRdisplay("number of result variable :  %d\n", tuple->dbi_resvar);
	    TRdisplay("integrity number:  %d\n", tuple->dbi_number);
	    TRdisplay("sequence number: %d\n", tuple->dbi_seq);
	    for (j = 0; j < DB_COL_WORDS; j++)
		TRdisplay("bit map of domains :  %x\n",
			  	tuple->dbi_columns.db_domset[j]);

	    TRdisplay("constraint name: %#.s\n",
		      DB_CONS_MAXNAME, tuple->dbi_consname);
	    TRdisplay("constraint id :  id1 : %d  id2 : %d\n",
		      tuple->dbi_cons_id.db_tab_base,
		      tuple->dbi_cons_id.db_tab_index);
	    TRdisplay("constraint schema id :  id1 : %d  id2 : %d\n",
		      tuple->dbi_consschema.db_tab_base,
		      tuple->dbi_consschema.db_tab_index);
	    TRdisplay("constraint flags :  %x\n", tuple->dbi_consflags);

	    i++;
	    data = data->dt_next;
	}
}


/*{
** Name: rdu_ptuple_dump - RDF protection tuples dump.
**
**	Internal call format:	rdu_ptuple_dump(rdf_info_blk);
**
** Description:
**      This function dump the protection tuples in the table information block.
**
** Inputs:
**	rdf_info_blk	    Pointer to the table information block.
**
** Outputs:
**	none
**	Returns:
**		none
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	10-apr-86 (ac)
**          written
**	06-feb-89 (ralph)
**	    changed to use DB_COL_WORDS when dumping
**	    dbp_domset array from iiprotect catalog
**	11-dec-89 (teg)
**	    modified dump to dump all protions of DB_PROCTECTION struct as of
**	    today.
*/
VOID
rdu_ptuple_dump(RDR_INFO *info)
{
	i4	    i = 0, j;
	QEF_DATA    *data;

	if (info == NULL)
	    return;

	if (info->rdr_ptuples == NULL)
	    return;

	TRdisplay("\n--------------------- Permit --------------------------\n");
	TRdisplay("Number of protection tuples retrieved:  %d\n",
	    info->rdr_ptuples->qrym_cnt);
	data = info->rdr_ptuples->qrym_data;

	while (data != NULL && i < info->rdr_ptuples->qrym_cnt)
	{
	    DB_PROTECTION *tuple;

	    tuple  = (DB_PROTECTION *)(data->dt_data);

	    TRdisplay ("\nPermit Tuple Number: %d\n",i);
	    TRdisplay("table id :  base : %d  index : %d\n",
		tuple->dbp_tabid.db_tab_base, tuple->dbp_tabid.db_tab_index);
	    TRdisplay("permit number :  %d\n", tuple->dbp_permno);
	    TRdisplay("bit map of permit operation :  %x\n", tuple->dbp_popset);
	    TRdisplay("time of the day permit begins :  %d\n",
		tuple->dbp_pdbgn);
	    TRdisplay("time of the day permit ends :  %d\n", tuple->dbp_pdend);
	    TRdisplay("time of the week permit begins :  %d\n",
		tuple->dbp_pwbgn);
	    TRdisplay("time of the week permit ends :  %d\n", tuple->dbp_pwend);
	    TRdisplay("query text id:  high : %d  low : %d\n",
		tuple->dbp_txtid.db_qry_high_time,
		tuple->dbp_txtid.db_qry_low_time);
	    TRdisplay("tree id:  high : %d  low : %d\n",
		tuple->dbp_treeid.db_tre_high_time,
		tuple->dbp_treeid.db_tre_low_time);
	    TRdisplay("user name :  %#s\n", DB_OWN_MAXNAME, &tuple->dbp_owner);
	    TRdisplay("terminal name :  %16s\n", &tuple->dbp_term);
	    TRdisplay("grantee type: %d\n", tuple->dbp_gtype);
	    TRdisplay("sequence number: %d\n", tuple->dbp_seq);
	    for (j = 0; j < DB_COL_WORDS; j++)
		TRdisplay("bit map of permitted domains :  %x\n",
		    tuple->dbp_domset[j]);
	    i++;
	    data = data->dt_next;
	}
}


/*{
** Name: rdu_attr_dump - RDF attributes informatin dump.
**
**	Internal call format:	rdu_attr_dump(rdf_info_blk);
**
** Description:
**      This function dump the attributes information.
**
** Inputs:
**	rdf_info_blk	    Pointer to the table information block.
**
** Outputs:
**	none
**	Returns:
**		none
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	10-apr-86 (ac)
**          written
**	11-dec-89 (teg)
**	    modified to dump 24 chars for attribute name.
*/
VOID
rdu_attr_dump(RDR_INFO	*info)
{
	i4	i;
	DMT_ATT_ENTRY	**attr;
	VOID	rdu_keys_dump();
	VOID	rdu_atthsh_dump();

	if (info == NULL)
	    return;

	if (info->rdr_attr == NULL)
	{
	    TRdisplay("WARNING:  attribute info is missing from this object!");
	    return;
	}

	TRdisplay("\n--------------------- Attributes --------------------------\n");
	for (i=1, attr = info->rdr_attr; i <= info->rdr_no_attr; i++)
	{

		TRdisplay("\nATTRIBUTE  NO. %d\n", i);
		TRdisplay("attribute name : %#s\n",
			DB_ATT_MAXNAME,  &attr[i]->att_name);
		TRdisplay("attribute number : %d\n", attr[i]->att_number);
		TRdisplay("attribute offset : %d\n", attr[i]->att_offset);
		TRdisplay("attribute type : %d\n", attr[i]->att_type);
		TRdisplay("attribute width : %d\n", attr[i]->att_width);
		TRdisplay("attribute precision : %d\n", attr[i]->att_prec);
		TRdisplay("attribute flags : %d\n", attr[i]->att_flags);
		TRdisplay("attribute key sequence number : %d\n",
		    attr[i]->att_key_seq_number);
	}

	if (info->rdr_keys != NULL)
		rdu_keys_dump(info);

	if (info->rdr_atthsh != NULL)
		rdu_atthsh_dump(info);
}

/*{
** Name: rdu_indx_dump - RDF index informatin dump.
**
**	Internal call format:	rdu_indx_dump(rdf_info_blk);
**
** Description:
**      This function dump the index information.
**
** Inputs:
**	rdf_info_blk	    Pointer to the table information block.
**
** Outputs:
**	none
**	Returns:
**		none
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	10-apr-86 (ac)
**          written
**	11-dec-89 (teg)
**	    fixed AV by checking for nonzero info->rdr_indx before using it,
**	    changed idx_name to dump 24 char, assured all elements in struct
**	    as of today are dumped.
**	30-May-2006 (jenjo02)
**	    idx_attr_id elements changed from DB_MAXKEYS to DB_MAXIXATTS.
*/
VOID
rdu_indx_dump(RDR_INFO	*info)
{
	i4	i;
	i4	j;
	DMT_IDX_ENTRY	**indx;

	if (info == NULL )
	    return;

	if (info->rdr_indx == NULL)
	    return;

	TRdisplay("\n--------------------- Index  --------------------------\n");
	for (i=1, indx = info->rdr_indx; i <= info->rdr_no_index; i++)
	{
		TRdisplay("\nINDEX NO. %d\n", i);
		TRdisplay("index name %#s\n", 
			DB_TAB_MAXNAME, &indx[i-1]->idx_name);
		TRdisplay("index table id :  base : %d  index : %d\n",
		    indx[i-1]->idx_id.db_tab_base,
		    indx[i-1]->idx_id.db_tab_index);
		for (j = 0; j < DB_MAXIXATTS; j++)
		    if (indx[i-1]->idx_attr_id[j])
			TRdisplay("ordinal number of attribute %d\n",
				   indx[i-1]->idx_attr_id[j]);
		TRdisplay("number of attributes in index %d\n",
						 indx[i-1]->idx_array_count);
		TRdisplay("number of keys in index %d\n",
						 indx[i-1]->idx_key_count);
		TRdisplay("number of data pages in index %d\n",
						 indx[i-1]->idx_dpage_count);
		TRdisplay("number of index pages in index %d\n",
						 indx[i-1]->idx_ipage_count);
		TRdisplay("status %d\n", indx[i-1]->idx_status);
		TRdisplay("storage structure %d\n",
					 indx[i-1]->idx_storage_type);
	}
}

/*{
** Name: rdu_keys_dump - RDF primary key informatin dump.
**
**	Internal call format:	rdu_keys_dump(rdf_info_blk);
**
** Description:
**      This function dump the primary key information.
**
** Inputs:
**	rdf_info_blk	    Pointer to the table information block.
**
** Outputs:
**	none
**	Returns:
**		none
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	10-apr-86 (ac)
**          written
*/
VOID
rdu_keys_dump(RDR_INFO	*info)
{
	i4	    key_no = 1;
	i4	    *key_ptr;

	if (info == NULL)
	    return;

	if (info->rdr_keys == NULL)
	    return;

	TRdisplay("--------------------- Primary Keys --------------------------\n");
	TRdisplay("No of attributes in the primary key is %d\n",
	    info->rdr_keys->key_count);
	key_ptr = info->rdr_keys->key_array;
	while (*key_ptr != 0)
	{
		TRdisplay("  key no  %d is attribute no %d\n",
		    key_no, *key_ptr);
		key_no++;
		key_ptr++;
	}
}

/*{
** Name: rdu_atthsh_dump - RDF hash table of attributes information dump.
**
**	Internal call format:	rdu_atthsh_dump(rdf_info_blk);
**
** Description:
**      This function dump the hash table of attributes information.
**
** Inputs:
**	rdf_info_blk	    Pointer to the table information block.
**
** Outputs:
**	none
**	Returns:
**		none
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	10-apr-86 (ac)
**          written
*/
VOID
rdu_atthsh_dump(RDR_INFO *info)
{
	i4	i;
	RDD_BUCKET_ATT	*hsh_element;

	if (info == NULL)
	    return;

	if (info->rdr_atthsh == NULL)
	{
	    TRdisplay("WARNING:  attribute hash info is missing from this object!");
	    return;
	}

	TRdisplay("----------------- Attribute Hash Table ---------------------\n");
	for (i=0; i < RDD_SIZ_ATTHSH; i++)
	{
	    if ((hsh_element = info->rdr_atthsh->rdd_atthsh_tbl[i]) != NULL)
	    {
		TRdisplay("\n bucket no %d\n", i);
		do {

		    TRdisplay("attribute name : %#s\n",
			DB_ATT_MAXNAME, &hsh_element->attr->att_name);

		} while ((hsh_element = hsh_element->next_attr) != NULL);
	    }
	}

}

/*{
** Name: rdu_rtuple_dump - RDF rules tuple dump.
**
**	Internal call format:	rdu_rtuple_dump(rdf_info_blk);
**
** Description:
**      This function dumps the iirules tuples in the table information block.
**
** Inputs:
**	rdf_info_blk	    Pointer to the table information block.
**
** Outputs:
**	none
**	Returns:
**		none
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	11-dec-89 (teg)
**	    written.
**	31-jul-92 (rickh)
**	    FIPS CONSTRAINTS:  Removed reference to dbr_column.
*/
VOID
rdu_rtuple_dump(RDR_INFO *info)
{
	i4	    i = 0, j, k;
	QEF_DATA    *data;

	if (info == NULL)
	    return;

	if (info->rdr_rtuples == NULL)
	    return;

	TRdisplay("\n--------------------- Rules --------------------------\n");
	TRdisplay("Number of rule tuples retrieved:  %d\n",
	    info->rdr_rtuples->qrym_cnt);
	data = info->rdr_rtuples->qrym_data;

	while (data != NULL && i < info->rdr_rtuples->qrym_cnt)
	{
	    DB_IIRULE *tuple;

	    tuple  = (DB_IIRULE *)(data->dt_data);
	    TRdisplay ("\nRule Tuple Number: %d\n",i);
	    TRdisplay("name of rule: %#s\n", 
		DB_RULE_MAXNAME, &tuple->dbr_name);
	    TRdisplay("owner of rule: %#s\n", 
		DB_OWN_MAXNAME, &tuple->dbr_owner);
	    TRdisplay("rule type: %d\n", tuple->dbr_type);
	    TRdisplay("rule modifier flag bitmap: %x\n", tuple->dbr_flags);
	    TRdisplay("table id (0 for time-type rules)  base : %d  index : %d\n",
		tuple->dbr_tabid.db_tab_base, tuple->dbr_tabid.db_tab_index);
	    TRdisplay("query text id:  high : %d  low : %d\n",
		tuple->dbr_txtid.db_qry_high_time,
		tuple->dbr_txtid.db_qry_low_time);
	    TRdisplay("tree id:  high : %d  low : %d\n",
		tuple->dbr_treeid.db_tre_high_time,
		tuple->dbr_treeid.db_tre_low_time);
	    TRdisplay("statement action: %d\n", tuple->dbr_statement);
	    for ( k = 0; k < DB_COL_WORDS; k++ )
	    {
	        TRdisplay("\nupdate specific column bitmap longword %d:  %0d",
		    tuple->dbr_columns.db_domset[ k ] );
	    }
	    TRdisplay("\n");
	    TRdisplay("number procedure parameters for rule: %d\n",
		tuple->dbr_dbpparam);
	    TRdisplay("procedure name to evoke: %#s\n",
		DB_DBP_MAXNAME, &tuple->dbr_dbpname);
	    TRdisplay("procedure owner: %#s\n",
		DB_OWN_MAXNAME, &tuple->dbr_dbpowner);
	    TRdisplay("date to fire time rule: %12s\n",
		&tuple->dbr_tm_date);
	    TRdisplay("repeat interval for time rules: %12s\n",
		&tuple->dbr_tm_int);
	    i++;
	    data = data->dt_next;
	}
}

/*{
** Name: rdu_view_dump - RDF view information dump.
**
**	Internal call format:	rdu_view_dump(rdf_info_blk);
**
** Description:
**      This function dumps view information in the table information block.
**
** Inputs:
**	rdf_info_blk	    Pointer to the table information block.
**
** Outputs:
**	none
**	Returns:
**		none
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	11-dec-89 (teg)
**	    written.
**	11-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values
*/
VOID
rdu_view_dump(RDR_INFO *info)
{

	PST_PROCEDURE	*node;

	if (info == NULL)
	    return;

	if ( info->rdr_view == NULL)
	    return;

	node = info->rdr_view->qry_root_node;

	TRdisplay("\n--------------------- View Tree  --------------------------\n");
	TRdisplay("Tree Version Number: %d\n", node->pst_vsn);
	TRdisplay("DB Procedure Flag: %d\n", node->pst_isdbp);
	TRdisplay("Address of 1st PST_STATEMENT: %0x\n", node->pst_stmts);
	TRdisplay("Address of procedure parameter: %0x\n", node->pst_parms);
	TRdisplay("Cursor id:  high value %d,  low value %d,  name %#s\n",
		node->pst_dbpid.db_cursor_id[0],node->pst_dbpid.db_cursor_id[1],
		DB_CURSOR_MAXNAME, node->pst_dbpid.db_cur_name);
	if (node->pst_stmts)
	{
	    TRdisplay("\nFIRST STATEMENT\n Statement Mode: %d\n",
		    node->pst_stmts->pst_mode);
	    TRdisplay(" Ptr to next statement in list: %p\n",
		    node->pst_stmts->pst_next);
	    TRdisplay(" Ptr to rule chain (pst_after_stmn): %p\n",
		    node->pst_stmts->pst_after_stmt);
	    TRdisplay(" Ptr to OPF internal information: %p\n",
		    node->pst_stmts->pst_opf);
	    TRdisplay(" Statement type: %d\n",
		    node->pst_stmts->pst_type);
	    TRdisplay(" Statement line number: %d\n",
		    node->pst_stmts->pst_lineno);
	    TRdisplay(" Ptr to procedure next statement: %p\n",
		    node->pst_stmts->pst_link);
	}
	if (node->pst_parms)
	{

	    i4	    i,j;


	    TRdisplay("\nPROCEDURE PARAMETERS/VARIABLES\n");
	    TRdisplay(" Number of local variables: %d\n",
		    node->pst_parms->pst_nvars );

	    for (i=0,j=node->pst_parms->pst_first_varno;
		i<node->pst_parms->pst_nvars; i++,j++)
	    {
		TRdisplay(" Variable Number: %d,  DV ptr: %x,  name %#s\n",
			 j, node->pst_parms->pst_vardef[i],
			 DB_PARM_MAXNAME, &node->pst_parms->pst_varname[i]);
	    }
	}
}

/*{
** Name: rdu_ldbdesc_dump - RDF DDB and LDB descriptor information dump.
**
**	Internal call format:	rdu_ldbdesc_dump(rdf_info_blk);
**
** Description:
**      This function dumps the DDB and information for Star.
**
** Inputs:
**	rdf_info_blk	    Pointer to the table information block.
**
** Outputs:
**	none
**	Returns:
**		none
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	17-sep-92 (teresa)
**	    initial creation
*/
static VOID
rdu_ldbdesc_dump(RDR_INFO	*info)
{
	i4	i;

	if (info == NULL)
	    return;

	if (info->rdr_obj_desc == NULL)
	    return;

	TRdisplay("\n-------------------- LDB Descriptor -----------------------\n");
	TRdisplay("Distributed Object Name: %#s\n",
			DB_OBJ_MAXNAME, info->rdr_obj_desc->dd_o1_objname);
	TRdisplay("Distributed Object Owner: %#s\n",
			DB_OWN_MAXNAME, info->rdr_obj_desc->dd_o2_objowner);
	TRdisplay("Distributed Object Id    base : %d  index : %d\n",
			info->rdr_obj_desc->dd_o3_objid.db_tab_base,
			info->rdr_obj_desc->dd_o3_objid.db_tab_index);
	TRdisplay("Query Id   high : %d  low : %d\n",
			info->rdr_obj_desc->dd_o4_qryid.db_qry_high_time,
			info->rdr_obj_desc->dd_o4_qryid.db_qry_low_time);
	TRdisplay("Create date: %25s\n",
			info->rdr_obj_desc->dd_o5_cre_date);
	switch (info->rdr_obj_desc->dd_o6_objtype)
	{
	    case DD_0OBJ_NONE:
		TRdisplay("Object type: NOT SPECIFIED\n");
		break;
	    case DD_1OBJ_LINK:
		TRdisplay("Object type: LINK\n");
		break;
	    case DD_2OBJ_TABLE:
		TRdisplay("Object type: TABLE\n");
		break;
	    case DD_3OBJ_VIEW:
		TRdisplay("Object type: VIEW\n");
		break;
	    case DD_4OBJ_INDEX:
		TRdisplay("Object type: INDEX\n");
		break;
	}
	TRdisplay("Alter date: %25s\n",
			info->rdr_obj_desc->dd_o7_alt_date);
	if (info->rdr_obj_desc->dd_o8_sysobj_b)
	    TRdisplay("System Object:	TRUE\n");
	else
	    TRdisplay("System Object:	FALSE\n");
	TRdisplay("contents of dd_o9_tab_info:\n");
	TRdisplay("  Local Name: %#s\n",
	    DB_TAB_MAXNAME, info->rdr_obj_desc->dd_o9_tab_info.dd_t1_tab_name);
	TRdisplay("  Local Owner: %#s\n",
	    DB_OWN_MAXNAME, info->rdr_obj_desc->dd_o9_tab_info.dd_t2_tab_owner);
	switch (info->rdr_obj_desc->dd_o9_tab_info.dd_t3_tab_type)
	{
	    case DD_0OBJ_NONE:
		TRdisplay("  Local Object type: NOT SPECIFIED\n");
		break;
	    case DD_1OBJ_LINK:
		TRdisplay("  Local Object type: LINK\n");
		break;
	    case DD_2OBJ_TABLE:
		TRdisplay("  Local Object type: TABLE\n");
		break;
	    case DD_3OBJ_VIEW:
		TRdisplay("  Local Object type: VIEW\n");
		break;
	    case DD_4OBJ_INDEX:
		TRdisplay("  Local Object type: INDEX\n");
		break;
	}
	TRdisplay("  Local Create date: %25s\n",
			info->rdr_obj_desc->dd_o9_tab_info.dd_t4_cre_date);
	TRdisplay("  Local Alter date: %25s\n",
			info->rdr_obj_desc->dd_o9_tab_info.dd_t5_alt_date);
	if (info->rdr_obj_desc->dd_o9_tab_info.dd_t6_mapped_b)
	    TRdisplay("  Local Column Names Are Mapped\n");
	else
	    TRdisplay("  Local Column Names Are NOT Mapped\n");
	TRdisplay("  Number of Columns in Local Table:  %d\n",
			info->rdr_obj_desc->dd_o9_tab_info.dd_t7_col_cnt);
	/* dump the mapped attribute names */
	rdu_col_dump(info->rdr_obj_desc->dd_o9_tab_info.dd_t7_col_cnt,
		     info->rdr_obj_desc->dd_o9_tab_info.dd_t8_cols_pp);
	TRdisplay("  LDB DESCRIPTOR DUMP: (address :%p)\n",
			info->rdr_obj_desc->dd_o9_tab_info.dd_t9_ldb_p);
	TRdisplay("     IDENTITY INFORMATION: (address %p)\n",
		info->rdr_obj_desc->dd_o9_tab_info.dd_t9_ldb_p->dd_i1_ldb_desc);
	ldbdesc_dmp(
	       &info->rdr_obj_desc->dd_o9_tab_info.dd_t9_ldb_p->dd_i1_ldb_desc);
	TRdisplay("     DBA AND CAPABILITIES INFORMATION: %p\n",
		info->rdr_obj_desc->dd_o9_tab_info.dd_t9_ldb_p->dd_i2_ldb_plus);
	ldbplus_dmp(
	       &info->rdr_obj_desc->dd_o9_tab_info.dd_t9_ldb_p->dd_i2_ldb_plus);
}

/*{
** Name: ldbdesc_dmp - Dump DD_LDB_DESC structure
**
** Description:
**      This function dumps the DD_LDB_DESC structure if populated.
**
** Inputs:
**	ldbdesc		    Ptr to DD_LDB_DESC to dump
**
** Outputs:
**	none
**	Returns:
**		none
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	17-sep-92 (teresa)
**	    initial creation
*/
static VOID
ldbdesc_dmp(DD_LDB_DESC *ldbdesc)
{
    if (!ldbdesc)
	return;

    if (ldbdesc->dd_l1_ingres_b)
	TRdisplay("     Requires Priveleged Association ($ingres Status)\n");
    else
	TRdisplay("     Does not Require Priveleged Association ($ingres Status)\n");
    TRdisplay("     Node Name: %#s\n", 
	DB_NODE_MAXNAME, ldbdesc->dd_l2_node_name);
    TRdisplay("     LDB Name : %60s\n", ldbdesc->dd_l3_ldb_name);
    TRdisplay("     DBMS Type: %#s\n",
	DB_TYPE_MAXLEN, ldbdesc->dd_l4_dbms_name);
    TRdisplay("     LDB id   : %d\n",   ldbdesc->dd_l5_ldb_id);
    /* the following bitmasks may only be set between TPF and QEF and be
    ** unused while RDF can see them:
    */
    if (ldbdesc->dd_l6_flags & DD_01FLAG_1PC)
        TRdisplay("     Supports 1PC, but not 2PC\n");
    if (ldbdesc->dd_l6_flags & DD_02FLAG_2PC)
        TRdisplay("     This is a CDB association for 2PC\n");
    if (ldbdesc->dd_l6_flags & DD_03FLAG_SLAVE2PC)
        TRdisplay("     LDB Supports Slave 2PC protocol\n");
}

/*{
** Name: ldbplus_dmp - Dump DD_0LDB_PLUS structure
**
** Description:
**      This function dumps the DD_0LDB_PLUS structure if populated.
**
** Inputs:
**	ldbplus		    Ptr to DD_0LDB_PLUS to dump
**
** Outputs:
**	none
**	Returns:
**		none
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	17-sep-92 (teresa)
**	    initial creation
**	02-sep-92 (barbara)
**	    Updated to dump DELIMITED_ID bit.
*/
static VOID
ldbplus_dmp(DD_0LDB_PLUS *ldbplus)
{
    if (! ldbplus)
	return;

    if (ldbplus->dd_p1_character && DD_1CHR_DBA_NAME)
    {
	TRdisplay("     Concept of DBA Name is supported\n");
	TRdisplay("     DBA Name: %#s\n", DB_OWN_MAXNAME, ldbplus->dd_p2_dba_name);
    }
    else
	TRdisplay("     Concept of DBA Name NOT supported\n");

    if (ldbplus->dd_p1_character && DD_2CHR_USR_NAME)
    {
	TRdisplay("     Concept of User Name is supported\n");
        TRdisplay("     User Name: %#s\n", DB_OWN_MAXNAME, ldbplus->dd_p5_usr_name);
    }
    else
	TRdisplay("     Concept of User Name NOT supported\n");

    if (ldbplus->dd_p1_character && DD_3CHR_SYS_NAME)
    {
	TRdisplay("     Concept of System Name is supported\n");
        TRdisplay("     System Name: %#s\n",
	      sizeof(ldbplus->dd_p6_sys_name), ldbplus->dd_p6_sys_name);
    }
    else
	TRdisplay("     Concept of System Name NOT supported\n");

    TRdisplay("     STAR Alias for DB Name: %#s\n",
	      DB_DB_MAXNAME, ldbplus->dd_p4_ldb_alias);

    if (ldbplus->dd_p3_ldb_caps.dd_c1_ldb_caps && DD_1CAP_DISTRIBUTED)
	TRdisplay("     DD_1CAP_DISTRIBUTED is ON\n");
    else
	TRdisplay("     DD_1CAP_DISTRIBUTED is OFF\n");
    if (ldbplus->dd_p3_ldb_caps.dd_c1_ldb_caps && DD_2CAP_INGRES)
	TRdisplay("     DD_2CAP_INGRES is ON\n");
    else
	TRdisplay("     DD_2CAP_INGRES is OFF\n");
    if (ldbplus->dd_p3_ldb_caps.dd_c1_ldb_caps && DD_3CAP_SAVEPOINTS)
	TRdisplay("     DD_3CAP_SAVEPOINTS is ON\n");
    else
	TRdisplay("     DD_3CAP_SAVEPOINTS is OFF\n");
    if (ldbplus->dd_p3_ldb_caps.dd_c1_ldb_caps && DD_4CAP_SLAVE2PC)
	TRdisplay("     DD_4CAP_SLAVE2PC is ON\n");
    else
	TRdisplay("     DD_4CAP_SLAVE2PC is OFF\n");
    if (ldbplus->dd_p3_ldb_caps.dd_c1_ldb_caps && DD_5CAP_TIDS)
	TRdisplay("     DD_5CAP_TIDS is ON\n");
    else
	TRdisplay("     DD_5CAP_TIDS is OFF\n");
    if (ldbplus->dd_p3_ldb_caps.dd_c1_ldb_caps && DD_6CAP_UNIQUE_KEY_REQ)
	TRdisplay("     DD_6CAP_UNIQUE_KEY_REQ is ON\n");
    else
	TRdisplay("     DD_6CAP_UNIQUE_KEY_REQ is OFF\n");
    if (ldbplus->dd_p3_ldb_caps.dd_c1_ldb_caps && DD_7CAP_USE_PHYSICAL_SOURCE)
	TRdisplay("     DD_7CAP_USE_PHYSICAL_SOURCE is ON\n");
    else
	TRdisplay("     DD_7CAP_USE_PHYSICAL_SOURCE is OFF\n");
    if (ldbplus->dd_p3_ldb_caps.dd_c1_ldb_caps && DD_8CAP_DELIMITED_IDS)
	TRdisplay("     DD_8CAP_DELIMITED_IDS is ON\n");
    else
	TRdisplay("     DD_8CAP_DELIMITED_IDS is OFF\n");
    if (ldbplus->dd_p3_ldb_caps.dd_c2_ldb_caps && DD_0CAP_PRE_602_ING_SQL)
	TRdisplay("     DD_0CAP_PRE_602_ING_SQL is ON\n");
    else
	TRdisplay("     DD_0CAP_PRE_602_ING_SQL is OFF\n");
    if (ldbplus->dd_p3_ldb_caps.dd_c2_ldb_caps && DD_201CAP_DIFF_ARCH)
	TRdisplay("     DD_201CAP_DIFF_ARCH is ON\n");
    else
	TRdisplay("     DD_201CAP_DIFF_ARCH is OFF\n");
    TRdisplay("     OPEN/SQL Level : %d\n",
	      ldbplus->dd_p3_ldb_caps.dd_c3_comsql_lvl);
    TRdisplay("     INGRES/SQL Level : %d\n",
	      ldbplus->dd_p3_ldb_caps.dd_c4_ingsql_lvl);
    TRdisplay("     INGRES/QUEL Level : %d\n",
	      ldbplus->dd_p3_ldb_caps.dd_c5_ingquel_lvl);
    if (ldbplus->dd_p3_ldb_caps.dd_c6_name_case && DD_0CASE_LOWER)
        TRdisplay("     Name Case:  LOWER CASE\n");
    else if (ldbplus->dd_p3_ldb_caps.dd_c6_name_case && DD_1CASE_MIXED)
	TRdisplay("     Name Case:  MIXED CASE\n");
    else if (ldbplus->dd_p3_ldb_caps.dd_c6_name_case && DD_2CASE_UPPER)
	TRdisplay("     Name Case:  UPPER CASE\n");

    TRdisplay("     DBMS Type: %#s\n",
	  DB_TYPE_MAXLEN, ldbplus->dd_p3_ldb_caps.dd_c7_dbms_type);

    switch (ldbplus->dd_p3_ldb_caps.dd_c8_owner_name)
    {
    case DD_0OWN_NO:
        TRdisplay("     <owner>.<table> NOT allowed\n");
	break;
    case DD_1OWN_YES:
        TRdisplay("     <owner>.<table> allowed\n");
	break;
    case DD_2OWN_QUOTED:
        TRdisplay("     '<owner>'.<table> allowed\n");
	break;
    }
}

/*{
** Name: rdu_col_dump - RDF Local attributes dump.
**
**	Internal call format:	 rdu_col_dump(rdf_info_blk);
**
** Description:
**      This function dumps the LDB column info (name and ordinal position)
**
** Inputs:
**	col_cnt		number of columns in cols array
**	cols		array of DD_COLUMN_DESC ptrs, one for each column
**
** Outputs:
**	none
**	Returns:
**		none
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	17-sep-92 (teresa)
**	    initial creation
*/
static VOID rdu_col_dump(i4 col_cnt, DD_COLUMN_DESC **cols)
{
    i4		    i;
    DD_COLUMN_DESC  *this_col;

    if ( !cols)
	return;

    for (i=0; i<col_cnt; i++)
    {
	this_col = cols[i];
	if (this_col)
	{
	    TRdisplay("     Column Name: %#s\n",
		  DB_ATT_MAXNAME, this_col->dd_c1_col_name);
	    TRdisplay("     Column Ordinal Position: %d\n",
		  this_col->dd_c2_col_ord);
	}
    }
}
#endif
/* end of xDEV_TEST if */

/*{
** Name: rdu_xor - compute checksum for a structure
**
**	Internal call format:	rdu_xor(&chksum, iptr, isize);
**
** Description:
**      This function execute checksum on infoblk.
**
** Inputs:
**	chksum	    pointer to checksum
**	iptr	    pointer to start of memory
**	isize       size
**
** Outputs:
**	none
**	Returns:
**		none
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	21-apr-89 (mings)
**          created for Titan
*/
VOID
rdu_xor(    i4	    *chksum,
	    char	    *iptr,
	    i4	    isize)
{
    i4	    i;
    for (i = 0; i < isize && (isize - i) > 16; i += 16)
    {
	*chksum ^= *iptr;
	*chksum ^= *(iptr++);
	*chksum ^= *(iptr++);
	*chksum ^= *(iptr++);
	*chksum ^= *(iptr++);
	*chksum ^= *(iptr++);
	*chksum ^= *(iptr++);
	*chksum ^= *(iptr++);
	*chksum ^= *(iptr++);
	*chksum ^= *(iptr++);
	*chksum ^= *(iptr++);
	*chksum ^= *(iptr++);
	*chksum ^= *(iptr++);
	*chksum ^= *(iptr++);
	*chksum ^= *(iptr++);
	*chksum ^= *(iptr++);
    }

    for (; i < isize; i++)
    	*chksum ^= *(iptr++);
}

/*{
** Name: rdu_com_checksum - execute checksum on infoblock
**
**	Internal call format:	rdu_com_checksum(infoblk);
**
** Description:
**      This function execute checksum on infoblk.
**
** Inputs:
**	infoblk	    Pointer to info block
**
** Outputs:
**
**	chksum      value of checksum for the infoblk
**	Returns:
**		none
**	Exceptions:
**		none
** Side Effects:
**	Because all structures in RDR_INFO are not continuously, we will only
**      consider the RDR_INFO, DMT_TAB_ENTRY, DMT_ATT_ENTRY, and DMT_IDX_ENTRY
**      for the calculation of checksum.
** History:
**	27-apr-89 (mings)
**          created for Titan
**	25-feb-94 (teresa)
**	    updated checksum to include RDR_VIEW and statistics fields.
*/
i4
rdu_com_checksum(RDR_INFO *infoblk)
{
    char	    *iptr;
    i4	    isize;
    i4	    chksum = 0;
    i4		    i;

    if (infoblk->rdr_invalid)
	return(0);

    isize = sizeof(RDR_INFO) - sizeof(infoblk->rdr_checksum);
    iptr = (char *)infoblk;

    rdu_xor(&chksum, iptr, isize);

    if (infoblk->rdr_types & RDR_RELATION)
    {
	isize = sizeof(DMT_TBL_ENTRY);
	iptr = (char *)infoblk->rdr_rel;
	rdu_xor(&chksum, iptr, isize);
    }

    if (infoblk->rdr_types & RDR_ATTRIBUTES)
    {
	isize = sizeof(DMT_ATT_ENTRY);

	for (i = 1; i <= infoblk->rdr_no_attr; i++)
	{
	    iptr = (char *)infoblk->rdr_attr[i];
	    rdu_xor(&chksum, iptr, isize);
	}
    }

    if (infoblk->rdr_types & RDR_INDEXES)
    {
	isize = sizeof(DMT_IDX_ENTRY);

	for (i = 0; i < infoblk->rdr_no_index; i++)
	{
	    iptr = (char *)infoblk->rdr_indx[i];
	    rdu_xor(&chksum, iptr, isize);
	}
    }

    if (infoblk->rdr_types & RDR_DST_INFO
	&&
	infoblk->rdr_types & RDR_RELATION
       )
    {
	isize = sizeof(DD_OBJ_DESC);
	iptr = (char *)infoblk->rdr_obj_desc;
	rdu_xor(&chksum, iptr, isize);

	if (iptr = (char *)infoblk->rdr_obj_desc->dd_o9_tab_info.dd_t9_ldb_p)
	{
	    isize = sizeof(DD_1LDB_INFO);
	    rdu_xor(&chksum, iptr, isize);
	}

    }

    if (infoblk->rdr_types & RDR_DST_INFO
	&&
	infoblk->rdr_types & RDR_ATTRIBUTES
	&&
	infoblk->rdr_obj_desc->dd_o9_tab_info.dd_t6_mapped_b
       )
    {
	isize = sizeof(DD_COLUMN_DESC);
	for (i = 1; i <= infoblk->rdr_obj_desc->dd_o9_tab_info.dd_t7_col_cnt; i++)
	{
	    iptr = (char *)infoblk->rdr_obj_desc->dd_o9_tab_info.dd_t8_cols_pp[i];
	    rdu_xor(&chksum, iptr, isize);
	}
    }

    if (infoblk->rdr_types & RDR_STATISTICS)
    {
	isize = sizeof(RDD_HISTO);

	for (i = 0; i < infoblk->rdr_no_attr; i++)
	{
	    if (infoblk->rdr_histogram[i])
	    {
		iptr = (char *)infoblk->rdr_histogram[i];
		rdu_xor(&chksum, iptr, isize);
	    }
	}
    }

    if (infoblk->rdr_types & RDR_VIEW)
    {
	/* the checksum routine does not understand how to walk a tree.  The
	** rdr_view entry is a pointer to the top node of a tree.  Since RDF
	** is not smart enough to walk the whole tree, it simply uses the ptr
	** to the tree as part of the checksum.
	*/
	isize = sizeof (RDD_VIEW);
	iptr = (char *)infoblk->rdr_view;
	rdu_xor(&chksum, iptr, isize);
    }


    return(chksum);
}

/*{
** Name: rdd_setsem	- get a semaphore on ldb descriptor, or on a col default
**
** Description:
**      This routine gets a semaphore for ldb if sem_type is
**	RDF_BLD_LDBDESC.  However, if sem_type is RDF_DEF_DESC, it will
**	call SCF to get a column default semaphore.
**
**	The Distributed seamphore (for a ldb descriptor or other star object)
**	either resides in the ULHOBJECT.rdf_ulhptr->ulh_usem of an RDF cache
**	object or resides in the svcb (when fixing objects that are not managed
**	by ulh).  It can be found as:
**
**	1.  Use ULHOBJECT's ulh_sem when object is just created/fixed by a
**	    ulh_create()/ulh_getalias() call.  The ULHOBJECT is in:
**		global->rdf_d_ulhobject
**	    or
**		global->rdf_dist_ulhcb.ulh_object
**	2.  Use generic semaphore descriptor for those distributed objects that
**	    are not managed by ULH.  The semaphore is in:
**		Rdi_svcb->rdv_global_sem
**
**	The Defaults semaphore must reside in the ULHOBJECT for that particular
**	default.  It is found as either:
**	    global->rdf_de_ulhobject
**	or as
**	    global->rdf_def_ulhcb.ulh_object
**
** Inputs:
**	svcb				RDF Server control Block address
**	    .rdv_global_sem		Semaphore structure for LDBdesc cache
**      global                          ptr to RDF state descriptor
**	    .rdf_d_ulhobject		PTR to RDF_DULHOBJECT for fixed LDB obj
**	    .rdf_dist_ulhcb		PTR to ULH_RCB for fixed LBD obj.
**		.ulh_object		ULH_OBJECT containing semaphore.
**	    .rdf_d_ulhobject		PTR to RDF_DE_ULHOBJECT for fixed default
**	    .rdf_def_ulhcb		PTR to ULH_RCB for fixed Default obj.
**		.ulh_object		ULH_OBJECT containing semaphore.
**	sem_type			Type of semaphore:
**					    RDF_BLD_LDBDESC - ldbdesc semaphore
**					    RDF_DEF_DESC - Defaults semaphore
** Outputs:
**	global				ptr to RDF state descriptor
**	    .rdf_ddrequests		Distributed processing requests
**		.rdd_ddstatus		Status bitflags: sets RDD_SEMAPHORE
**	    .rdf_resources		RDF resources held.  Sets RDF_RSEMAPHORE
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-aug-88 (mings)
**         initial creation
**	20-jun-90 (teresa)
**	   added new parameter to rdu_ferror() for bug 4390.
**	20-may-92 (teresa)
**	    rewrote to stop using semaphore address and scf_cb address as
**	    input parameters.
**	16-jul-92 (teresa)
**	    prototypes
**	17-feb-93 (teresa)
**	    genericized to handle defaults semaphores along with ldbdesc
**	    semaphores
**	1-apr-93 (teresa)
**	    use default semaphore if we dont have a cache object.
**	18-may-93 (teresa)
**	    Fix bug 51564 -- rdd_setsem setting wrong internal semaphore flag
**			     for defaults semaphore.
**	20-may-93 (teresa)
**	    Renamed rdf_de_ulhcb to rdf_def_ulhcb so it remains unique to 8
**	    characters with rdf_de_ulhobject.
**	15-Aug-1997 (jenjo02)
**	    Call CL semaphore functions directly instead of going thru SCF.
*/
DB_STATUS
rdd_setsem(RDF_GLOBAL *global, i4 sem_type)
{

    DB_STATUS		    status;
    CS_SEMAPHORE	    *sem;

    if (sem_type == RDF_BLD_LDBDESC)
    {
	if (global->rdf_d_ulhobject)
	    sem = &global->rdf_d_ulhobject->rdf_ulhptr->ulh_usem;
	else if (global->rdf_dist_ulhcb.ulh_object)
	    sem = &global->rdf_dist_ulhcb.ulh_object->ulh_usem;
	else
	    sem = &Rdi_svcb->rdv_global_sem;
    }
    else
    {
	/* sem_type must be RDF_DEF_DESC */
	if (global->rdf_def_ulhcb.ulh_object)
	    sem = &global->rdf_def_ulhcb.ulh_object->ulh_usem;
	else if (global->rdf_de_ulhobject)
	    sem = &global->rdf_de_ulhobject->rdf_ulhptr->ulh_usem;
	else
	    sem = &Rdi_svcb->rdv_global_sem;
    }

    /* get the semaphore */
    status = CSp_semaphore(TRUE, sem);

    if (DB_FAILURE_MACRO(status))
    {
	DB_ERROR	localDBerror;

	SETDBERR(&localDBerror, 0, status);
	rdu_ferror(global, E_DB_ERROR, &localDBerror, E_RD0031_SEM_INIT_ERR,0);
	status = E_DB_ERROR;
    }
    else
    {
	if (sem_type == RDF_BLD_LDBDESC)
	    global->rdf_ddrequests.rdd_ddstatus |= RDD_SEMAPHORE;
	else
	    /* sem_tye must be RDF_DEF_DESC */
	    global->rdf_resources |= RDF_DSEMAPHORE;
    }
    return(status);
}

/*{
** Name: rdd_resetsem	- release semaphore.
**
** Description:
**      This routine releases a semaphore.
**
** Inputs:
**	sem_type			Type of semaphore:
**					    RDF_BLD_LDBDESC - ldbdesc semaphore
**					    RDF_DEF_DESC - Defaults semaphore
**	svcb				RDF Server control Block address
**	    .rdv_global_sem		Semaphore structure for LDBdesc cache
**      global                          ptr to RDF state descriptor
**	    .rdf_d_ulhobject		PTR to RDF_DULHOBJECT for fixed LDB obj
**	    .rdf_dist_ulhcb		PTR to ULH_RCB for fixed LBD obj.
**		.ulh_object		ULH_OBJECT containing semaphore.
**	    .rdf_d_ulhobject		PTR to RDF_DE_ULHOBJECT for fixed default
**	    .rdf_def_ulhcb		PTR to ULH_RCB for fixed Default obj.
**		.ulh_object		ULH_OBJECT containing semaphore.
** Outputs:
**	global				ptr to RDF state descriptor
**	    .rdf_ddrequests		Distributed processing requests
**		.rdd_ddstatus		Status bitflags: clears RDD_SEMAPHORE
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-sep-88 (mings)
**          initial creation
**	20-jun-90 (teresa)
**	   added new parameter to rdu_ferror() for bug 4390.
**	19-may-92 (teresa)
**	    fix a bug where RDD_SEMAPHORE was being reset even if the
**	    semaphore release failed.
**	20-may-92 (teresa)
**	    rewrote to stop using semaphore address and scf_cb address as
**	    input parameters.
**	16-jul-92 (teresa)
**	    prototypes
**	17-feb-93 (teresa)
**	    genericized to handle defaults semaphores along with ldbdesc
**	    semaphores
**	1-apr-93 (teresa)
**	    use default semaphore if we dont have a cache object.
**	18-may-93 (teresa)
**	    Fix bug 51564 -- rdd_resetsem setting wrong internal semaphore flag
**			     for defaults semaphore.
**	20-may-93 (teresa)
**	    Renamed rdf_de_ulhcb to rdf_def_ulhcb so it remains unique to 8
**	    characters with rdf_de_ulhobject.
**	15-Aug-1997 (jenjo02)
**	    Call CL semaphore functions directly instead of going thru SCF.
*/
DB_STATUS
rdd_resetsem(RDF_GLOBAL *global, i4 sem_type)
{

    DB_STATUS	    status;
    CS_SEMAPHORE    *sem;

    if (sem_type == RDF_BLD_LDBDESC)
    {
	if (global->rdf_d_ulhobject)
	    sem = &global->rdf_d_ulhobject->rdf_ulhptr->ulh_usem;
	else if (global->rdf_dist_ulhcb.ulh_object)
	    sem = &global->rdf_dist_ulhcb.ulh_object->ulh_usem;
	else
	    sem = &Rdi_svcb->rdv_global_sem;
    }
    else
    {
	/* sem_type must be RDF_DEF_DESC */
	if (global->rdf_def_ulhcb.ulh_object)
	    sem = &global->rdf_def_ulhcb.ulh_object->ulh_usem;
	else if (global->rdf_de_ulhobject)
	    sem = &global->rdf_de_ulhobject->rdf_ulhptr->ulh_usem;
	else
	    sem = &Rdi_svcb->rdv_global_sem;
    }

    /* release the semaphore */
    status = CSv_semaphore(sem);

    if (DB_FAILURE_MACRO(status))
    {
	DB_ERROR	localDBerror;

	SETDBERR(&localDBerror, 0, status);
	rdu_ferror(global, E_DB_ERROR, &localDBerror, E_RD0034_RELEASE_SEMAPHORE,0);
	status = E_DB_ERROR;
    }
    else
    {
	if (sem_type == RDF_BLD_LDBDESC)
	    global->rdf_ddrequests.rdd_ddstatus &= (~RDD_SEMAPHORE);
	else
	    /* sem_tye must be RDF_DEF_DESC */
	    global->rdf_resources &= (~RDF_DSEMAPHORE);
    }

    return(status);
}

/* the following routines are not used now but may be useful later, they are
** lifted from the TITAN codeline (6.4 star)
*/

/*{
** Name: rdu_scftrace - call scc_trace to display trace information
**
**	Internal call format:	rdu_scftrace(where_p, length, msg_buf);
**
** Description:
**      This function call scc_trace to display trace information.
**
** Inputs:
**	where_p		0 send to log file only,
**			1 send to screen,
**			> 1 send to both log and screen
**	length		message lenghth
**	msg_buf		message
**
** Outputs:
**	none
**	Returns:
**		none
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	21-apr-89 (mings)
**          created for Titan
*/
#ifdef xDEV_TEST
VOID
rdu_scftrace(	i4		*where_p,
		u_i4		length,
		char		*msg_buf)
{
    DB_STATUS	status = E_DB_OK;
    SCF_CB	scf_cb;
    char	buf[5];

    /* if length == 0, then send a line feed */

    if (length == 0)
    {
	/* send a line feed to log file */

	if (*where_p == 0 || *where_p > 1)
	    TRdisplay("\n ");

	msg_buf = &buf[0];
	STprintf(msg_buf, " \n");
	length = STlength(msg_buf);
    }
    else
    {
	/* send to log file */
	if (length < RDF_MSGLENGTH)
	    msg_buf[length] = EOS;

	if (*where_p == 0 || *where_p > 1)
	    TRdisplay("%s", msg_buf);

    }

    if (*where_p > 0)
    {
	scf_cb.scf_length = sizeof(scf_cb);
	scf_cb.scf_type = SCF_CB_TYPE;
	scf_cb.scf_facility = DB_RDF_ID;
	scf_cb.scf_nbr_union.scf_local_error = 0;

	scf_cb.scf_len_union.scf_blength = (u_i4)length;
	scf_cb.scf_ptr_union.scf_buffer = msg_buf;
	scf_cb.scf_session = DB_NOSESSION;

	status = scf_call(SCC_TRACE, &scf_cb);

	if (DB_FAILURE_MACRO(status))
	{
	    TRdisplay("SCF error %d when displaying RDF info to user\n",
		       scf_cb.scf_error.err_code);
	}
    }

}
#endif

#ifdef xDEV_TEST
/*{
** Name: rdu_object_trace - print relation descriptor info
**
**	Internal call format:	rdu_object_trace(rdf_info_blk);
**
** Description:
**      This function dump the object information.
**
** Inputs:
**	rdf_info_blk	    Pointer to the information block.
**	where_to_print
**
** Outputs:
**	none
**	Returns:
**		none
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	21-apr-89 (mings)
**          created for Titan
*/
VOID
rdu_object_trace(   RDF_CCB	*scb_p,
		    RDR_INFO	*rdf_info_blk,
		    i4	where_to_print,
		    i4	tr_type)
{
    char	msg_buf[RDF_MSGLENGTH];

    if (tr_type == RDU_DELOBJ_DUMP)
    {
	TRformat(rdu_scftrace,&where_to_print, msg_buf,
		(i4)RDF_MSGLENGTH, "\n\nDESTROYING OBJECT:\n");
    }
    else if (tr_type == RDU_ADDOBJ_DUMP)
    {
	TRformat(rdu_scftrace,&where_to_print, msg_buf,
		(i4)RDF_MSGLENGTH, "\n\nRELATION OBJECT:\n");
    }
    rdu_info_dump(rdf_info_blk);
/*    rdu_desc_trace(scb_p, rdf_info_blk, where_to_print); */
}
#endif

/*{
** Name: rdu_help - print trace help information
**
**	Internal call format:	rdu_help(where_to_print);
**
** Description:
**      This function prints trace help information.
**
** Inputs:
**	where_to_print    Where to print trace info
**
** Outputs:
**	none
**	Returns:
**		none
**	Exceptions:
**		none
** Side Effects:
**		none
** History:
**	21-apr-89 (mings)
**          created for Titan
**	17-jul-90 (teresa)
**	    added info for trace point rd96
*/
#ifdef xDEV_TEST
VOID
rdu_help(i4 where_to_print)
{
    char    msg_buf[RDF_MSGLENGTH];

    TRformat(rdu_scftrace, &where_to_print, msg_buf,
	     (i4)RDF_MSGLENGTH, "\n\nHELP - HOW TO USE RDF TRACE\n");
    TRformat(rdu_scftrace, &where_to_print, msg_buf,
	     (i4)RDF_MSGLENGTH, "\n===========================\n");
    TRformat(rdu_scftrace, &where_to_print, msg_buf,
	     (i4)RDF_MSGLENGTH, "\n\n\tSyntax:   SET TRACE POINT RDxxx [y]\n");
    TRformat(rdu_scftrace, &where_to_print, msg_buf, (i4)RDF_MSGLENGTH,
	     "\n\n\t    RDxxx is trace number\n");
    TRformat(rdu_scftrace, &where_to_print, msg_buf, (i4)RDF_MSGLENGTH,
	     "\n\t    y is an optional numeric number, if specified,\n");
    TRformat(rdu_scftrace, &where_to_print, msg_buf, (i4)RDF_MSGLENGTH,
	     "\n\t    trace messages will be sent to log file and screen,\n");
    TRformat(rdu_scftrace, &where_to_print, msg_buf, (i4)RDF_MSGLENGTH,
	     "\n\t    otherwise, trace messages will be sent to log file only\n");

    TRformat(rdu_scftrace, &where_to_print, msg_buf, (i4)RDF_MSGLENGTH,
	    "\n\n\tTrace_Number    Internal              Description\n");
    TRformat(rdu_scftrace, &where_to_print, msg_buf, (i4)RDF_MSGLENGTH,
	    "\n\t=================================================\n");
    TRformat(rdu_scftrace, &where_to_print, msg_buf, (i4)RDF_MSGLENGTH,
	    "\n\tRD050           RDU_INFO_DUMP          Info Block\n");
    TRformat(rdu_scftrace, &where_to_print, msg_buf, (i4)RDF_MSGLENGTH,
	    "\n\tRD051           RDU_REL_DUMP        Relation Desc\n");
    TRformat(rdu_scftrace, &where_to_print, msg_buf, (i4)RDF_MSGLENGTH,
            "\n\tRD052           RDU_ATTR_DUMP      Attribute Info\n");
    TRformat(rdu_scftrace, &where_to_print, msg_buf, (i4)RDF_MSGLENGTH,
	    "\n\tRD053           RDU_INDX_DUMP          Index Info\n");
    TRformat(rdu_scftrace, &where_to_print, msg_buf, (i4)RDF_MSGLENGTH,
	    "\n\tRD054           RDU_ATTHSH_DUMP    Attr Hash Info\n");
    TRformat(rdu_scftrace, &where_to_print, msg_buf, (i4)RDF_MSGLENGTH,
	    "\n\tRD055           RDU_KEYS_DUMP            Key Info\n");
    TRformat(rdu_scftrace, &where_to_print, msg_buf, (i4)RDF_MSGLENGTH,
	    "\n\tRD056           RDU_HIST_DUMP      Histogram Info\n");
    TRformat(rdu_scftrace, &where_to_print, msg_buf, (i4)RDF_MSGLENGTH,
	    "\n\tRD057           RDU_ITUPLE_DUMP    Integrity Info\n");
    TRformat(rdu_scftrace, &where_to_print, msg_buf, (i4)RDF_MSGLENGTH,
	    "\n\tRD058           RDU_PTUPLE_DUMP   Protection Info\n");
    TRformat(rdu_scftrace, &where_to_print, msg_buf, (i4)RDF_MSGLENGTH,
	    "\n\tRD059           RDU_ADDOBJ_DUMP        Add Object\n");
    TRformat(rdu_scftrace, &where_to_print, msg_buf, (i4)RDF_MSGLENGTH,
	    "\n\tRD060           RDU_DELOBJ_DUMP     Delete Object\n");
    TRformat(rdu_scftrace, &where_to_print, msg_buf, (i4)RDF_MSGLENGTH,
	    "\n\tRD090           RDU_DIAGNOSIS          Cache Info\n");
    TRformat(rdu_scftrace, &where_to_print, msg_buf, (i4)RDF_MSGLENGTH,
	    "\n\tRD091           RDU_NO_UPDATE          No Updates\n");
    TRformat(rdu_scftrace, &where_to_print, msg_buf, (i4)RDF_MSGLENGTH,
	    "\n\tRD092           RDU_FLUSH_ALL           Flush all\n");
    TRformat(rdu_scftrace, &where_to_print, msg_buf, (i4)RDF_MSGLENGTH,
	    "\n\tRD093           RDU_CHECKSUM       	Checksum\n");
    TRformat(rdu_scftrace, &where_to_print, msg_buf, (i4)RDF_MSGLENGTH,
	    "\n\tRD094           RDU_LDB_DESCRIPTOR    No Checksum\n");
    TRformat(rdu_scftrace, &where_to_print, msg_buf, (i4)RDF_MSGLENGTH,
	    "\n\tRD095           RDU_SUMMARY          Summary only\n");
    TRformat(rdu_scftrace, &where_to_print, msg_buf, (i4)RDF_MSGLENGTH,
	    "\n\tRD096           RDU_QRYTXT	Print RDF queries\n");
    TRformat(rdu_scftrace, &where_to_print, msg_buf, (i4)RDF_MSGLENGTH,
	    "\n\tRD099           RDU_HELP                     Help\n\n");

}
#endif
