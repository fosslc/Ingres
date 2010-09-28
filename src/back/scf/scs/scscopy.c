/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cv.h>
#include    <er.h>
#include    <ex.h>
#include    <er.h>
#include    <pc.h>
#include    <cs.h>
#include    <me.h>
#include    <tm.h>
#include    <st.h>
#include    <sl.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <ade.h>
#include    <adp.h>
#include    <qsf.h>
#include    <scf.h>
#include    <gca.h>
#include    <dmf.h>
#include    <dmrcb.h>
#include    <copy.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <qefqeu.h>
#include    <qefcopy.h>
#include    <opfcb.h>
#include    <psfparse.h>

#include <dudbms.h>
#include <dmccb.h>

#include    <sc.h>
#include    <sc0e.h>
#include    <sc0m.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <scfcontrol.h>
#include    <sxf.h>
#include    <sc0a.h>

/*
**
**  Name: SCSCOPY.C - Copy related sub-routines.
**
**  Description:
**	This file contains various sub-routines that are used during
**	copy processing. They will usually be called by the sequencer
**	during a copy.
**
**	  scs_copy_begin()	- Start a COPY statement
**	  scs_copy_from()	- Sequencer continuation for COPY FROM
**	  scs_copy_into()	- Sequencer continuation for COPY INTO
**	  scs_copy_bad()	- Sequencer rundown for errored COPY
**	  scs_scopy()		- Send GCA_C_{INTO,FROM} block to FE
**	  scs_copy_error()	- Process a COPY error.
**	  scs_cpsave_err()	- Stash a COPY error code.
**	  scs_cpxmit_errors()	- Transmit a batch of copy errors
**	  scs_cpinterrupt()	- Send interrupt to the FE.
**        scs_cp_cpnify()       - Prepare read copy blocks containing
**                                  large objects
**        scs_cp_redeem()       - Expand large objects in copy blocks
**
**  History:
**	25-jun-1992 (fpang)
**	    Created this source file header description as part of Sybil merge.
**	13-jul-92 (rog)
**	    Included ddb.h for Sybil, and er.h because of a change to scs.h.
**	21-jul-92 (rog)
**	    Fix compilation error: control-L was a carat-L.
**	10-nov-92 (robf)
**	    Make sure SXF audit data is flushed prior to returning anything
**	    to the user. 
**  	23-Feb-1993 (fred)
**  	    Added large object support.  Added scs_cp_cpnify() and
**          scs_cp_redeem() to enable the management of large object
**          values during a COPY statement.
**	12-Mar-1993 (daveb)
**	    Add include <st.h> <me.h>
**      23-Jun-1993 (fred)
**          Fixed bug in copy null indicator value handling.  See
**          scs_scopy(). 
**	2-Jul-1993 (daveb)
**	    prototyped, remove func extern refs from headers.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	7-jul-93 (rickh)
**	    Prototyped qef_call invocations.
**	08-sep-93 (swm)
**	    Cast completion id. parameter to IIGCa_call() to PTR as it is
**	    no longer a i4, to accomodate pointer completion ids.
**	24-jan-1994 (gmanning)
**	    Change errno to local_errno because of NT macro problems.
**      05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with
**	    DM0M_OBJECT. This results in owner within sc0m_allocate having to
**	    be PTR.
**          In addition calls to sc0m_allocate within this module had the
**          3rd and 4th parameters transposed, this has been remedied.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**	19-nov-1996 (kch)
**	    In the function scs_cp_redeem(), we now calculate the number of
**	    non LO bytes in the row and decrement this by the number of non
**	    LO bytes copied. When we come to an LO column, we call scs_redeem()
**	    with the remaining length of the copy buffer minus the number of
**	    non LO bytes still to copy, thus preventing scs_redeem() from
**	    leaving an insufficient amount of space in the buffer for the
**	    remaining non LO data. This change fixes bug 79287.
**	14-jan-1999 (nanpr01)
**	    Changed the sc0m_deallocate to pass pointer to a pointer.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-Dec-2001 (gordy)
**	    GCA_COL_ATT no longer defined with DB_DATA_VALUE.
**      25-mar-2004 (stial01)
**          scs_redeem: restrict blob output to MAXI2
**          Fixed incorrect casting of length arguments.
**      20-nov-2008 (stial01)
**          DON'T assume GCA_MAXNAME > DB_MAXNAME
**	10-Mar-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
**	25-Mar-2010 (kschendel) SIR 123485
**	    Chop COPY bits out of sequencer, move here.  Prune out
**	    some obsolete leftovers from the COPY INTO code.  No functional
**	    change, this is just for readability and to start shrinking
**	    the incredibly gigantic sequencer mainline.
**	30-Mar-2010 (kschendel) SIR 123485
**	    Re-type some ptr's as the proper struct pointer.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
*/

/*
**  Forward and/or External function/variable references.
*/
static DB_STATUS scs_scopy(SCD_SCB *scb, QEU_COPY *qe_copy );

static void scs_cpsave_err(QEU_COPY *qe_copy, i4 local_errno );

static void scs_cpxmit_errors(SCD_SCB *scb, i4  end_stream );

static DB_STATUS scs_cp_cpnify(SCD_SCB *scb, 
				char *inbuffer,
				i4  insize, 
				QEU_COPY *qe_copy);

static DB_STATUS scs_cp_redeem(SCD_SCB *scb,
				QEU_COPY *qe_copy,
				char *outbuffer, 
				i4  outsize);

static DB_STATUS scs_redeem(SCD_SCB   	 *scb,
			    DB_BLOB_WKSP *wksp,
			    i4           continuation,
			    DB_DT_ID     datatype,
			    i4           prec,
			    i4      out_size,
			    PTR          cpn_ptr,
			    i4           *res_prec,
			    i4           *res_len,
			    PTR          out_area);
GLOBALREF	SC_MAIN_CB	*Sc_main_cb;

/*
** Name: scs_copy_begin - Sequencer startup for COPY statement
**
** Description:
**
**	A new query has just been parsed and it is a COPY.  The
**	Front End must be notified to start copy processing and
**	QEF must be initialized to begin the COPY.
** 
**	After exclusive-locking the parse result in QSF, we
**	initialize QEF by calling QEU_B_COPY.  This begins a
**	transaction and opens the copy relation for reading or
**	writing, depending on the direction of the COPY.
** 
**	The copy map is built and queued for sending to the front-end.
**	Then, we return with sscb_state set to SCS_CP_FROM or SCS_CP_INTO
**	as appropriate.  The next time into the sequencer, one
**	of the scs_copy_from / scs_copy_into routines will be called
**	to stream tuples.
**
**	The "next operation" for CS is set to:
**       - CS_OUTPUT if COPY INTO
**       - CS_EXCHANGE if COPY FROM (or error)
**
**	Note that if all goes well, the QP QSF object (which is a
**	QEF_RCB which points to a QEU_COPY and associated data structures)
**	will remain locked.  If something goes wrong during COPY startup,
**	the parser result is destroyed from QSF.
**
** Inputs:
**	scb		Sequencer session control block
**	next_op		An output
**	ret_val		An output
**
** Outputs:
**	scb		sscb_state updated
**	next_op		Set to CS_OUTPUT or CS_EXCHANGE as above
**	ret_val		Set to return value for caller return
**
** Returns a sequencer-action code (continue, break, or return(ret_val)).
**
** History:
**	25-Mar-2010 (kschendel) SIR 123485
**	    Hatchet COPY code out of sequencer mainline.
*/

sequencer_action_enum
scs_copy_begin(SCD_SCB *scb, i4 *next_op, DB_STATUS *ret_val)

{
    DB_STATUS status;
    PSQ_CB *ps_ccb;		/* Query parse control block */
    QEF_RCB *qe_ccb;		/* QEF request block */
    QEU_COPY *qe_copy;		/* COPY control block */
    QSF_RCB *qs_ccb;		/* QSF request block */
    SCS_CURRENT *cquery;	/* Pointer to scb->scb_sscb.sscb_cquery */
    SCS_SSCB *sscb;		/* Pointer to scb->scb_sscb */

    /* Load up convenience pointers */

    sscb = &scb->scb_sscb;
    cquery = &sscb->sscb_cquery;
    ps_ccb = sscb->sscb_psccb;

    *ret_val = E_DB_OK;

    /*
    ** Lock the copy control structure just created by PSF.
    ** Since the call to get the lock returns the root id, there
    ** there is no need to do an info here.
    **
    ** For COPY, PSF creates a "QP" consisting of a QEF_RCB which
    ** points to a QEU_COPY, which may point to additional data
    ** structures that we don't care about here.
    */
    qs_ccb = sscb->sscb_qsccb;
    qs_ccb->qsf_obj_id.qso_handle = ps_ccb->psq_result.qso_handle;
    qs_ccb->qsf_lk_state = QSO_EXLOCK;

    status = qsf_call(QSO_LOCK, qs_ccb);
    if (DB_FAILURE_MACRO(status))
    {
	scs_qsf_error(status, qs_ccb->qsf_error.err_code,
	    E_SC0210_SCS_SQNCR_ERROR);
	sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
	sscb->sscb_state = SCS_INPUT;
	*next_op = CS_EXCHANGE;
	*ret_val = E_DB_WARN;
	return (SEQUENCER_BREAK);
    }
    STRUCT_ASSIGN_MACRO(ps_ccb->psq_result, cquery->cur_qp);
    cquery->cur_lk_id = qs_ccb->qsf_lk_id;
    cquery->cur_row_count = 0;

    /*
    ** Initialize the QEF control block.
    ** Set QEF error flag to INTERNAL so user messages won't
    ** be sent directly to the Front End.  The system will hang
    ** if we try to send a message while the FE is sending
    ** blocks of tuples to us in a COPY FROM.
    */

    qe_ccb = (QEF_RCB *) qs_ccb->qsf_root;
    qe_copy = qe_ccb->qeu_copy;
    qe_ccb->qef_cb = sscb->sscb_qescb;
    qe_ccb->qef_sess_id = scb->cs_scb.cs_self;
    qe_ccb->qef_db_id = sscb->sscb_ics.ics_opendb_id;
    qe_ccb->qef_eflag = QEF_INTERNAL;

    /*
    ** Initialize some of the copy control block fields.
    */
    qe_copy->qeu_rowspace = 0;
    qe_copy->qeu_sbuf = NULL;
    qe_copy->qeu_sptr = NULL;
    qe_copy->qeu_ssize = 0;
    qe_copy->qeu_sused = 0;
    qe_copy->qeu_partial_tup = FALSE;
    qe_copy->qeu_error = FALSE;

    qe_copy->qeu_uputerr[0] = 0;
    qe_copy->qeu_uputerr[1] = 0;
    qe_copy->qeu_uputerr[2] = 0;
    qe_copy->qeu_uputerr[3] = 0;
    qe_copy->qeu_uputerr[4] = 0;

    /*
    ** Call QEF to initialize COPY processing.  This will begin
    ** a single statement transaction and prepare for reading
    ** or writing of tuples.
    */
    status = qef_call(QEU_B_COPY, qe_ccb);
    if (DB_FAILURE_MACRO(status))
    {
	i4	qeu_b_copy_err;
	DB_STATUS local_status;
	/*
	** Error in COPY startup.  Since the Front End has not
	** yet started sending COPY data blocks, there is no
	** need to send an interrupt block, we can just signal
	** an error.
	**
	** The QE0023 error is special, it indicates that the table
	** timestamp is newer than the QP timestamp, and the query
	** should be retried.
	*/

	/* qs_ccb is set up above */
	qeu_b_copy_err = qe_ccb->error.err_code;
	local_status = qsf_call(QSO_DESTROY, qs_ccb);
	if (DB_FAILURE_MACRO(local_status))
	{
	    scs_qsf_error(local_status, qs_ccb->qsf_error.err_code,
		E_SC0210_SCS_SQNCR_ERROR);
	}
	cquery->cur_qp.qso_handle = 0;

	if (qeu_b_copy_err == E_QE0023_INVALID_QUERY)
	    return (SEQUENCER_CONTINUE);
	if (sscb->sscb_interrupt)
	    return (SEQUENCER_BREAK);
	scs_qef_error(status, qeu_b_copy_err,
			    E_SC0210_SCS_SQNCR_ERROR, scb);

	sscb->sscb_state = SCS_INPUT;
	*next_op = CS_EXCHANGE;
	*ret_val = E_DB_ERROR;
	return (SEQUENCER_BREAK);
    }
    sscb->sscb_noretries = 0;
    sscb->sscb_nostatements += 1;

    cquery->cur_qe_cb = (PTR) qe_ccb;

    /*
    ** Build and queue copy map message for front-end.
    */
    {
	GCA_TD_DATA *tdesc;
	tdesc = (GCA_TD_DATA *) qe_copy->qeu_tdesc;

	/*
	**  If tuple has a blob, don't set compression bit.
	**  Otherwise, if compression bit is set in the
	**  scb control block, pass it to the tuple
	**  descriptor.  (but only for the INTO direction)
	*/
	tdesc->gca_result_modifier &= ~GCA_COMP_MASK;
	if (qe_copy->qeu_direction == CPY_INTO
	  && (sscb->sscb_opccb->opf_names & GCA_COMP_MASK)
	  && (tdesc->gca_result_modifier & GCA_LO_MASK) == 0
	  && (Sc_main_cb->sc_capabilities & SC_VCH_COMPRESS) )
	{
	    tdesc->gca_result_modifier |= GCA_COMP_MASK;
	}
    }
    status = scs_scopy(scb, qe_copy);
    if (DB_FAILURE_MACRO(status))
    {
	DB_STATUS local_status;

	/* scs_scopy issues error messages as needed. */
	/* First, shut down whatever QEF started */
	qe_copy->qeu_stat |= CPY_FAIL;
	local_status = qef_call(QEU_E_COPY, qe_ccb);
	if (DB_FAILURE_MACRO(local_status))
	{
	    scs_qef_error(local_status, qe_ccb->error.err_code,
		E_SC0210_SCS_SQNCR_ERROR, scb);
	}

	/* qs_ccb is set up above */
	local_status = qsf_call(QSO_DESTROY, qs_ccb);
	if (DB_FAILURE_MACRO(local_status))
	{
	    scs_qsf_error(local_status, qs_ccb->qsf_error.err_code,
		E_SC0210_SCS_SQNCR_ERROR);
	}
	cquery->cur_qe_cb = (PTR) 0;
	cquery->cur_qp.qso_handle = 0;

	sscb->sscb_state = SCS_INPUT;
	*ret_val = status;
	return (SEQUENCER_BREAK);
    }
    if (qe_copy->qeu_direction == CPY_FROM)
    {
	sscb->sscb_state = SCS_CP_FROM;
    }
    else
    {
	sscb->sscb_state = SCS_CP_INTO;
    }
    *next_op = CS_EXCHANGE;

    sscb->sscb_cfac = DB_CLF_ID;
    return (SEQUENCER_RETURN);		/* with ret_val E_DB_OK */

} /* scs_copy_begin */

/*
** Name: scs_copy_from - Sequencer control for in-progress COPY FROM
**
** Description:
**
**	We are in the middle of processing a "COPY FROM".  We
**	have just received an input block from the Front End
**	to be processed.
**
**	If the input block is a GCA_CDATA block, then we want
**	to append them to the copy table.  Form a list of
**	QEF_DATA structs pointing to the tuples just received
**	and call QEU_R_COPY.  Some care has to be taken here, as
**	CDATA messages are streamed, and not necessarily aligned with
**	GCA buffering.  So we might need to remember a partial tuple
**	for the next time thru.  Also, LOB data arrives in the midst
**	of the CDATA stream (in the LOB column's proper position, of
**	course), and the LOB value has to be squirreled away (couponified).
**	Some variables (qeu_cp_xxx) in the QEU_COPY block are used to
**	save state when we're scanning the input stream for LOB processing.
**
**	If we received an IACK block, then the FE is ready to
**	read errors we are going to send.  We will send the
**	error blocks followed by a response block.  If the
**	errors are not fatal (we are not in SCS_CP_ERROR
**	state), then the response block will have the
**	GCA_CONTINUE_MASK bit set to indicate that the
**	serves merely to cause a protocol turnaround.
**
**	There are two ways the Sequencer can be told to
**	terminate the copy.  The Front End can send a
**	GCA_RESPONSE block telling us to halt, or the FE can
**	send an interrupt.  If either of these events occur,
**	then we want to end COPY.  Call QEU_E_COPY to close
**	the copy table and complete the transaction.  If the
**	COPY has ended prematurely by an interrupt or an error
**	then QEU_COPY will backout the copy unless the user
**	specified ROLLBACK = DISABLED.  After QEU_E_COPY,
**	clean up memory allocated by SCF and PSF for copy
**	processing.
**
**	If the copy is not finished, then return with next
**	state SCS_CP_FROM in order to process the next block
**	of tuples.  If the copy is finished, then set next
**	state to SCS_INPUT.  In this case, SCF will send a
**	GCA_RESPONSE block to the FE giving the number of
**	rows inserted.
**
**	If the copy was terminated due to an error, we have
**	to read in the outstanding data blocks from the front
**	end before we send an error block.  Save the error
**	and set next state to SCS_CP_ERROR to read in the data
**	blocks until we get the IACK block.  Then send the
**	error blocks and a response block indicating an error.
**
**	If the we encounter an error and the front end is
**	already waiting for a status block from us (we are at
**	a sync point), just go ahead and send the error.
**
**	The sequencer state (sscb_state) is set to:
**      - SCS_CP_FROM if more GCA_CDATA blocks are to follow
**      - SCS_CP_ERROR if a copy-fatal error occurred
**      - SCS_INPUT if the COPY is finished.
**	The CS next-operation is set to:
**      - CS_EXCHANGE if an error was encountered and we
**	      wish to send the interrupt
**      - CS_INPUT otherwise.
**
** Inputs:
**	scb		Sequencer session control block
**	next_op		An output
**	ret_val		An output
**
** Outputs:
**	scb		sscb_state updated
**	next_op		Set to CS_INPUT or CS_EXCHANGE as above
**	ret_val		Set to return value for caller return
**
** Returns a sequencer-action code (break, or return(ret_val); continue
** is not allowed).
**
** History:
**	25-Mar-2010 (kschendel) SIR 123485
**	    Hatchet COPY code out of sequencer mainline.
*/

sequencer_action_enum
scs_copy_from(SCD_SCB *scb, i4 *next_op, DB_STATUS *ret_val)

{
    bool cpi_endcopy;		/* TRUE if ending (RESPONSE or interrupt) */
    bool cpi_saved_tup;
    bool cpi_had_error;
    bool cpi_more_data;
    char *cpi_data;
    DB_STATUS status;
    i4 cpi_input_tups;
    i4 cpi_new_rows;
    i4 cpi_dt_size;
    i4 cpi_block_type;
    i4 cpi_amount_left;
    i4 cpi_leftover;
    QEF_RCB *qe_ccb;		/* QEF request block */
    QEU_COPY *qe_copy;		/* COPY control block */
    QSF_RCB *qs_ccb;		/* QSF request block */
    SCC_CSCB *cscb;		/* Pointer to scb->scb_cscb */
    SCS_CURRENT *cquery;	/* Pointer to scb->scb_sscb.sscb_cquery */
    SCS_SSCB *sscb;		/* Pointer to scb->scb_sscb */

    /* Load up convenience pointers */

    cscb = &scb->scb_cscb;
    sscb = &scb->scb_sscb;
    cquery = &sscb->sscb_cquery;

    *ret_val = E_DB_OK;

    *next_op = CS_INPUT;
    qe_ccb = (QEF_RCB *) cquery->cur_qe_cb;
    qe_copy = qe_ccb->qeu_copy;

    if ((cscb->cscb_gci.gca_status == E_GC0001_ASSOC_FAIL)
	|| (cscb->cscb_gci.gca_status == E_GC0023_ASSOC_RLSED))
    {
	/* Fail the COPY, ignore error */
	qe_copy->qeu_stat |= CPY_FAIL;
	(void) qef_call(QEU_E_COPY, qe_ccb);
	*next_op = CS_TERMINATE;
	sscb->sscb_state = SCS_TERMINATE;
	sscb->sscb_cfac = DB_CLF_ID;
	return (SEQUENCER_RETURN);
    }
    else if (cscb->cscb_gci.gca_status != E_GC0000_OK)
    {
	sc0e_0_put(cscb->cscb_gci.gca_status,
		    &cscb->cscb_gci.gca_os_status);
	sc0e_0_put(E_SC022F_BAD_GCA_READ, 0);
	sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
	/* Fail the COPY, ignore error */
	qe_copy->qeu_stat |= CPY_FAIL;
	(void) qef_call(QEU_E_COPY, qe_ccb);
	sscb->sscb_state = SCS_INPUT;
	sscb->sscb_qmode = 0;
	sscb->sscb_cfac = DB_CLF_ID;
	*next_op = CS_EXCHANGE;
	*ret_val = E_DB_SEVERE;
	return (SEQUENCER_RETURN);
    }	
    cscb->cscb_gci.gca_status = E_SC1000_PROCESSED;

    /*
    ** Process the message from the front-end.  Valid messages are:
    ** CDATA - this is the normal case, a stream of tuples.
    ** RESPONSE (or interrupt) - FE is finished, end the copy.
    ** ACK or empty message - ignore
    ** IACK (or obsolete Q_STATUS) - FE has noticed an interrupt sent to
    **		it, and has temporarily halted the tuple stream so that
    **		the server can send some errors.
    ** Anything else is a protocol error.
    */

    cpi_block_type = cscb->cscb_gci.gca_message_type;
    cpi_dt_size = cscb->cscb_gci.gca_d_length;
    cpi_data = (char *) cscb->cscb_gci.gca_data_area;

    cpi_endcopy = FALSE;
    if (cpi_block_type == GCA_RESPONSE || sscb->sscb_interrupt)
    {
	cpi_endcopy = TRUE;
    }
    else if ((cpi_block_type == GCA_ACK) || (cpi_dt_size == 0))
    {
	/* Fine, send ack's all day */
	sscb->sscb_cfac = DB_CLF_ID;
	*next_op = CS_INPUT;
	return (SEQUENCER_RETURN);
    }
    else if (cpi_block_type == GCA_Q_STATUS || cpi_block_type == GCA_IACK)
    {
	/*
	** We sent the front-end an interrupt and it has responded with
	** IACK, interrupting the tuple stream so that we can send
	** an error.  In this case, we will queue up the messages
	** to be sent to the FE, and place a response block at the end
	** (with GCA_CONTINUE_MASK set) to tell the FE to 
	** continue after it gets the messages.  The FE will either
	** resume the tuple stream, or end the copy.
	*/
	scs_cpxmit_errors(scb, TRUE);
	sscb->sscb_cfac = DB_CLF_ID;
	*next_op = CS_EXCHANGE;
	return (SEQUENCER_RETURN);
    }
    else if (cpi_block_type != GCA_CDATA)
    {
	i4   should_be = GCA_CDATA;

	sc0e_put(E_SC022E_WRONG_BLOCK_TYPE, 0, 2,
		sizeof(cpi_block_type), (PTR)&cpi_block_type,
		sizeof(i4), (PTR)&should_be,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0);
	sc0e_0_put(E_SC0250_COPY_OUT_OF_SEQUENCE, 0);
	sc0e_0_uput(E_SC0250_COPY_OUT_OF_SEQUENCE, 0);
	scs_copy_error(scb, qe_ccb, 1);
	scd_note(E_DB_ERROR, DB_SCF_ID);
	sscb->sscb_state = SCS_INPUT;
	*next_op = CS_EXCHANGE;
	*ret_val = E_DB_ERROR;
	return (SEQUENCER_BREAK);
    }

    /*
    ** The loop below exists ONLY for the case where a
    ** copy includes a large object.  In all other
    ** cases, the loop control variable (cpi_more_data)
    ** will be set to FALSE in the block below where the
    ** number of tuples sent is calculated.
    **
    ** For the case where the copy into contains large
    ** objects, the scs_cp_cpnify() routine is called
    ** to convert the inline objects to coupons for
    ** insertion by QEF.  Since there is only enough sequencer state
    ** in the QEU_COPY for one row, after finding the end of a
    ** row, we have to send it off to QEF.  Then, we might loop
    ** to exhaust all data sent by the frontend before asking for
    ** another communication block.
    */

    cpi_input_tups = 0;
    cpi_saved_tup = FALSE;
    cpi_more_data = TRUE;
    cpi_had_error = FALSE;

    do
    {
	/*
	** If we received part of a tuple in the last
	** communication block, we have it stored in
	** qe_copy->qeu_sbuf.  Read the rest of the
	** tuple there too.  If the rest of the tuples
	** didn't all fit in this data block, just
	** read in what did and wait for the next
	** block.
	*/

	if (qe_copy->qeu_partial_tup && (!cpi_endcopy)
	    && ((cquery->cur_rdesc.rd_modifier & GCA_LO_MASK) == 0))
	{
	    cpi_amount_left = qe_copy->qeu_ext_length - qe_copy->qeu_sused;
	    if (cpi_amount_left > cpi_dt_size)
	    {
		MEcopy((PTR)cpi_data, cpi_dt_size, (PTR)qe_copy->qeu_sptr);
		qe_copy->qeu_sptr += cpi_dt_size;
		qe_copy->qeu_sused += cpi_dt_size;
		cpi_dt_size = 0;
	    }
	    else
	    {
		/* Entire remainder of tuple has arrived */
		MEcopy((PTR)cpi_data, cpi_amount_left, (PTR)qe_copy->qeu_sptr);
		cpi_dt_size -= cpi_amount_left;
		cpi_data += cpi_amount_left;
		/* Reset "save" pointer to start, for next time */
		qe_copy->qeu_sptr = (char *)qe_copy->qeu_sbuf + sizeof(SC0M_OBJECT);
		qe_copy->qeu_sused = 0;
		cpi_saved_tup = TRUE;
		cpi_input_tups = 1;
		qe_copy->qeu_partial_tup = FALSE;
	    }
	}

	/*
	** Figure out how many tuples there are in
	** this block.  If we have a tuple that
	** spanned block boundaries in qeu_sbuf, then
	** cpi_input_tups is already 1.
	**
	** Remember if there is a partial tuple at
	** the end that did not fit completely in this
	** communication block.  If so, we will save
	** it in qeu_sbuf below.  If there are no
	** peripheral columns involved in the copy,
	** then determining the number of tuples (and
	** amount of leftover space) is a matter of
	** simple division, since the tuples will
	** arrived packed in the communication block.
	** On the other hand, if there are peripheral
	** objects (large objects) involved, we will
	** have to walk the communication block.  The
	** process of walking the block will also
	** replace the inline peripheral objects with
	** coupons, to be processed below by QEF and
	** DMF.
	*/


	if ((cquery->cur_rdesc.rd_modifier & GCA_LO_MASK) == 0)
	{
	    cpi_more_data = FALSE;
	    cpi_input_tups += cpi_dt_size / qe_copy->qeu_ext_length;
	    cpi_leftover = cpi_dt_size % qe_copy->qeu_ext_length;
	}
	else if (cpi_endcopy)
	{
	    cpi_more_data = FALSE;
	    cpi_input_tups = 0;
	}
	else
	{
	    cpi_leftover = 0;
	    if (qe_copy->qeu_sbuf == NULL)
	    {
		status = sc0m_allocate(SCU_MZERO_MASK,
			sizeof(SC0M_OBJECT) + qe_copy->qeu_ext_length,
			DB_SCF_ID,
			(PTR)SCS_MEM,
			CV_C_CONST_MACRO('C','P','Y','T'),
			&qe_copy->qeu_sbuf);
		if (DB_FAILURE_MACRO(status))
		{
		    sc0e_0_put(status, 0);
		    scs_cpinterrupt(scb);
		    scs_cpsave_err(qe_copy, status);
		    scs_cpsave_err(qe_copy, E_SC0206_CANNOT_PROCESS);
		    scs_copy_error(scb, qe_ccb, 0);
		    sscb->sscb_state = SCS_CP_ERROR;
		    qe_copy->qeu_sused = cpi_leftover;
		    qe_copy->qeu_partial_tup = TRUE;
		    sscb->sscb_cfac = DB_CLF_ID;
		    *next_op = CS_INPUT;
		    return (SEQUENCER_RETURN);
		}
		qe_copy->qeu_sptr = (char *)qe_copy->qeu_sbuf +
		    sizeof(SC0M_OBJECT);
		qe_copy->qeu_ssize = qe_copy->qeu_ext_length;
	    }
	    /* Only blank out the buffer if starting a new tuple. */
	    else if (qe_copy->qeu_cp_cur_att < 0)
	    {
		MEfill(qe_copy->qeu_ext_length, 0, (PTR) qe_copy->qeu_sptr);
	    }
	    cpi_more_data = TRUE; /* By default... */
	    status = scs_cp_cpnify(scb,
				cpi_data, /* input */
				cpi_dt_size, /*sizeof(input) */
				qe_copy);

	    if (status)
	    {
		if (DB_FAILURE_MACRO(status))
		{
		    sc0e_0_put(status, 0);
		    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		    scs_copy_error(scb, qe_ccb, 1);
		    sscb->sscb_state = SCS_CP_ERROR;
		    *next_op = CS_INPUT;
		    cpi_had_error = TRUE;
		    break;
		}
		else
		{
		    /*
		    ** E_DB_INFO says the couponify
		    ** returned properly, but has not yet
		    ** finished with a tuple.  Here, we
		    ** just leave the number of tuples as
		    ** zero, and fall thru the code,
		    ** eventually returning here with
		    ** another copy block to process.
		    */
		    cpi_input_tups = 0;
		    cpi_saved_tup = FALSE;
		    if (qe_copy->qeu_cp_in_offset < 0)
		    {
			cpi_more_data = FALSE;	/* expected */
		    }
		}
	    }
	    else
	    {
		/* We found the end of a row, and the entire row
		** with coupons is in the sptr buffer.
		*/
		cpi_input_tups = 1;
		cpi_saved_tup = TRUE;
		if (qe_copy->qeu_cp_in_offset < 0)
		{
		    cpi_more_data = FALSE;
		}
	    }
	}

	if ((cpi_input_tups > 0) && (!cpi_endcopy))
	{
	    char *row_buf;
	    i4 i;
	    QEF_DATA *data_buffer;

	    /*
	     ** QEF expects a list of QEF_DATA structs
	     ** which point to the tuples to be appended.
	     ** We must allocate space for the structs,
	     ** format them into a QEF_DATA list, and point
	     ** to the tuples just read.  If we already
	     ** have a formatted QEF_DATA list from the
	     ** last set of tuples read, we can just change
	     ** the data pointers to point to the new
	     ** tuples.
	     */
	    if (cpi_input_tups > qe_copy->qeu_rowspace)
	    {
		/*
		** We need a list of QEF_DATA structs
		** to call QEF.  Allocate one extra
		** one since we will need it on the
		** next call if tuples cross block
		** boundaries.
		*/

		cpi_new_rows = cpi_input_tups + 1;

		/*
		** Allocate storage to format into
		** QEF_DATA structs.  These structs
		** will be passed to QEF and will
		** point to the buffers to get/put
		** rows.  If we have previously
		** allocated storage for this purpose,
		** but it is not enough, we have to
		** allocate a larger block.
		*/

		if (cquery->cur_amem)
		{
		    sc0m_deallocate(0, &cquery->cur_amem);
		    cquery->cur_qef_data = NULL;
		}
		status = sc0m_allocate(0,
			   sizeof(SC0M_OBJECT) +
				       (cpi_new_rows * sizeof(QEF_DATA)),
			   DB_SCF_ID,
			   (PTR)SCS_MEM,
			   CV_C_CONST_MACRO('C','P','Y','D'),
			   &cquery->cur_amem);
		if (DB_FAILURE_MACRO(status))
		{
		    sc0e_0_put(status, 0);
		    if (cpi_endcopy)
		    {
			sc0e_0_uput(status, 0);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS,
				    0);
			scs_copy_error(scb, qe_ccb, 1);
			sscb->sscb_state = SCS_INPUT;
			*next_op = CS_INPUT;
			cpi_had_error = TRUE;
			break;
		    }
		    else
		    {
			scs_cpinterrupt(scb);
			scs_cpsave_err(qe_copy, status);
			scs_cpsave_err(qe_copy, 
				       E_SC0206_CANNOT_PROCESS);
			scs_copy_error(scb, qe_ccb, 0);
			sscb->sscb_state = SCS_CP_ERROR;
			if (cpi_leftover != 0)
			{
			    qe_copy->qeu_sused = cpi_leftover;
			    qe_copy->qeu_partial_tup = TRUE;
			}
			*next_op = CS_INPUT; 
			    /* block sent via NO_ASYNC_EXIT */
			sscb->sscb_cfac = DB_CLF_ID;
			return (SEQUENCER_RETURN);
		    }
		}

		/*
		 ** Format the QEF_DATA structs into a list.
		 */
		cquery->cur_qef_data =
		    (QEF_DATA *)((char *)cquery->cur_amem + sizeof(SC0M_OBJECT));
		data_buffer = cquery->cur_qef_data;

		for (i = 0; i < cpi_new_rows; i++)
		{
		    data_buffer->dt_size = qe_copy->qeu_ext_length;
		    data_buffer->dt_next = data_buffer + 1;
		    data_buffer->dt_data = NULL;
		    data_buffer++;
		}
		data_buffer--;
		data_buffer->dt_next = NULL;

		qe_copy->qeu_rowspace = cpi_new_rows;
	    }

	    /*
	    ** Set data pointers in QEF_DATA list to
	    ** point at tuples just read.
	    ** If we have a tuple saved in
	    ** qeu_sbuf, then the first QEF_DATA
	    ** struct points there.
	    */
	    row_buf = cpi_data;
	    qe_copy->qeu_input = data_buffer = cquery->cur_qef_data;
	    qe_copy->qeu_cur_tups = cpi_input_tups;

	    if (cpi_saved_tup)
	    {
		data_buffer->dt_data = qe_copy->qeu_sptr;
		data_buffer = data_buffer->dt_next;
		--cpi_input_tups;
	    }

	    while (--cpi_input_tups >= 0)
	    {
		data_buffer->dt_data = (PTR) row_buf;

		row_buf += qe_copy->qeu_ext_length;
		data_buffer = data_buffer->dt_next;
	    }

	    /*
	    ** Call QEF to append the tuples to the relation
	    */
	    status = qef_call(QEU_R_COPY, qe_ccb);
	    if (DB_FAILURE_MACRO(status))
	    {
		if (cpi_endcopy)
		{
		    scs_qef_error(status,
				  qe_ccb->error.err_code,
				  E_SC0210_SCS_SQNCR_ERROR,
				  scb);
		    qe_copy->qeu_stat |= CPY_FAIL;
		    scs_copy_error(scb, qe_ccb, 0);
		    sscb->sscb_state = SCS_INPUT;
		    *next_op = CS_EXCHANGE;
		    cpi_had_error = TRUE;
		    break;
		}
		else
		{
		    /*
		    ** B39166: Suppress scs_cpinterrupt() under
		    ** FORCE_ABORT condition, scs_cpinterrupt() is called
		    ** again later.
		    */
		    /*
		    ** Ignore excess QEF "USER ERROR" 
		    ** messages, real error will have already
		    ** been queued.
		    */
		    /*
		    ** We also suppress the cpinterrupt in
		    ** this case currently to work around
		    ** syncronization problems where the
		    ** interrupt goes out at an inappropriate
		    ** time for the FE to retrieve. This may
		    ** delay the detection of the error
		    ** but lowers the likelihood of the
		    ** frontend hanging on error. Longterm
		    ** the interrupt handling should be
		    ** looked at and this code reworked.
		    */
		    if(qe_ccb->error.err_code!=E_QE0025_USER_ERROR)
		    {
			scs_cpsave_err(qe_copy, qe_ccb->error.err_code);
			if (sscb->sscb_force_abort != SCS_FAPENDING)
				scs_cpinterrupt(scb);
		    }

		    qe_copy->qeu_stat |= CPY_FAIL;
		    scs_copy_error(scb, qe_ccb, 0);
		    sscb->sscb_state = SCS_CP_ERROR;
		    if (cpi_leftover != 0)
		    {
			qe_copy->qeu_sused = cpi_leftover;
			qe_copy->qeu_partial_tup = TRUE;
		    }
		    *next_op = CS_INPUT;
			/* block sent via NO_ASYNC_EXIT */
		    sscb->sscb_cfac = DB_CLF_ID;
		    return (SEQUENCER_RETURN);
		}
	    }

	}

	if (cpi_endcopy)
	{
	    /*
	    ** End of the Copy, call QEF to close the
	    ** relation.  We also have to send a
	    ** response to the Front End.
	    */
	    status = qef_call(QEU_E_COPY, qe_ccb);
	    if (DB_FAILURE_MACRO(status))
	    {
		scs_qef_error(status, qe_ccb->error.err_code,
			      E_SC0210_SCS_SQNCR_ERROR, scb);
		sc0e_0_put(E_SC0251_COPY_CLOSE_ERROR, 0);
		*ret_val = E_DB_ERROR;
	    }

	    cquery->cur_row_count = qe_copy->qeu_count;

	    /*
	    ** Free up the memory allocated for COPY
	    ** processing and destroy the control
	    ** structures allocated by PSF.
	    */
	    if (qe_copy->qeu_sbuf != NULL)
	    {
		sc0m_deallocate(0, &qe_copy->qeu_sbuf);
	    }

	    if (cquery->cur_amem)
	    {
		sc0m_deallocate(0, &cquery->cur_amem);
		cquery->cur_qef_data = NULL;
	    }

	    qs_ccb = sscb->sscb_qsccb;
	    qs_ccb->qsf_obj_id.qso_handle = cquery->cur_qp.qso_handle;
	    qs_ccb->qsf_lk_id = cquery->cur_lk_id;
	    status = qsf_call(QSO_DESTROY, qs_ccb);
	    if (DB_FAILURE_MACRO(status))
	    {
		scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			      E_SC0210_SCS_SQNCR_ERROR);
	    }
	    cquery->cur_qp.qso_handle = 0;
	    cquery->cur_qe_cb = NULL;

	    *next_op = CS_INPUT;
	    sscb->sscb_state = SCS_INPUT;
	    break;
	}

	/* This batch of rows was processed, if there is leftover then
	** save it for next time.  (Allocate the save area if it hasn't
	** been allocated yet, or is too small.  Too small really shouldn't
	** happen.)
	*/
	if (cpi_leftover != 0)
	{
	    if ((qe_copy->qeu_sbuf != NULL) &&
		(qe_copy->qeu_ssize < qe_copy->qeu_ext_length))
	    {
		sc0m_deallocate(0, &qe_copy->qeu_sbuf);
		qe_copy->qeu_ssize = 0;
	    }

	    if (qe_copy->qeu_sbuf == NULL)
	    {
		status = sc0m_allocate(0,
				sizeof(SC0M_OBJECT) + qe_copy->qeu_ext_length,
				DB_SCF_ID,
				(PTR)SCS_MEM,
				CV_C_CONST_MACRO('C','P','Y','T'),
				&qe_copy->qeu_sbuf);
		if (DB_FAILURE_MACRO(status))
		{
		    sc0e_0_put(status, 0);
		    scs_cpinterrupt(scb);
		    scs_cpsave_err(qe_copy, status);
		    scs_cpsave_err(qe_copy,
				   E_SC0206_CANNOT_PROCESS);
		    scs_copy_error(scb, qe_ccb, 0);
		    sscb->sscb_state = SCS_CP_ERROR;
		    qe_copy->qeu_sused = cpi_leftover;
		    qe_copy->qeu_partial_tup = TRUE;
		    sscb->sscb_cfac = DB_CLF_ID;
		    *next_op = CS_INPUT;
		    return (SEQUENCER_RETURN);
		}
		qe_copy->qeu_sptr = (char *)qe_copy->qeu_sbuf + sizeof(SC0M_OBJECT);
		qe_copy->qeu_ssize = qe_copy->qeu_ext_length;
	    }
	    if ((cquery->cur_rdesc.rd_modifier & GCA_LO_MASK) == 0)
	    {
		MEcopy( (PTR) (cpi_data + cpi_dt_size - cpi_leftover),
			cpi_leftover, (PTR)qe_copy->qeu_sptr);
		qe_copy->qeu_sptr += cpi_leftover;
		qe_copy->qeu_sused = cpi_leftover;
		qe_copy->qeu_partial_tup = TRUE;
	    }
	}
    } while (cpi_more_data);	/* Only loops if LOB exists */

    if (cpi_had_error || cpi_endcopy)
    {
	if (cpi_had_error)
	    *ret_val = E_DB_WARN;
	return (SEQUENCER_BREAK);
    }
    sscb->sscb_cfac = DB_CLF_ID;
    return (SEQUENCER_RETURN);

} /* scs_copy_from */

/*
** Name: scs_copy_into - Sequencer control for in-progress COPY INTO
**
** Description:
**
**	We are in the middle of processing a "COPY INTO".  We
**	want to get the next block of tuples to send to the
**	Front End.
**
**	If we receive a GCA_RESPONSE block telling us to end
**	the copy, or if we receive a user interrupt, then terminate
**	the COPY.  Call QEU_E_COPY to end the copy and clean
**	things up at the QEF end of things.  Then, we
**	clean up memory allocated by SCF and PSF for copy
**	processing.
**
**	If all is well, and we are to continue COPY processing,
**	then form a list of QEF_DATA structs and send to
**	QEU_W_COPY to fill.  In the general case, the QEF_DATA structs
**	will point into the communication block, allowing QEF
**	to retrieve the data directly into the GCA_CDATA block.
**
**	If QEU_W_COPY returns that we have exhausted the rows
**	in the table, then end the query.  This is done by 
**	queuing any remaining CDATA's, and then having the main
**	sequencer follow up with a RESPONSE message.
**
**	If copy is not finished, the state is left at SCS_CP_INTO
**	so that we return here to send more data.
**	If the copy is finished, then set next state to SCS_INPUT.
**
**	Next State:
**	   - SCS_CP_INTO if still more rows to send.
**	   - SCS_INPUT if COPY is finished
**	Next CS Operation:
**	   - CS_EXCHANGE if COPY is finished.
**	   - CS_OUTPUT otherwise.
** Inputs:
**	scb		Sequencer session control block
**	op_code		The CS operation at entry to the sequencer
**	next_op		An output
**	ret_val		An output
**
** Outputs:
**	scb		sscb_state updated
**	next_op		Set to CS_OUTPUT or CS_EXCHANGE as above
**	ret_val		Set to return value for caller return
**
** Returns a sequencer-action code (break, or return(ret_val); continue is
** not allowed here).
**
** History:
**	25-Mar-2010 (kschendel) SIR 123485
**	    Hatchet COPY code out of sequencer mainline.
**	    Prune a bunch of dead "leftover" code that was never executed.
*/

sequencer_action_enum
scs_copy_into(SCD_SCB *scb, i4 op_code, i4 *next_op, DB_STATUS *ret_val)

{
    bool cpo_endcopy;
    char *cpo_data;
    char *row_buf;
    DB_STATUS status;
    i4 cpo_dt_size;
    i4 cpo_outbuf_size;
    i4 cpo_new_rows;
    i4 cpo_send_size;
    i4 cpo_out_tups;
    i4 cpo_gca_size;
    i4 i;
    QEF_DATA *data_buffer;
    QEF_RCB *qe_ccb;		/* QEF request block */
    QEU_COPY *qe_copy;		/* COPY control block */
    QSF_RCB *qs_ccb;		/* QSF request block */
    SCC_CSCB *cscb;		/* Pointer to scb->scb_cscb */
    SCC_GCMSG *message;		/* Message we're building to send */
    SCS_CURRENT *cquery;	/* Pointer to scb->scb_sscb.sscb_cquery */
    SCS_SSCB *sscb;		/* Pointer to scb->scb_sscb */

    /* Load up convenience pointers */

    cscb = &scb->scb_cscb;
    sscb = &scb->scb_sscb;
    cquery = &sscb->sscb_cquery;

    *ret_val = E_DB_OK;

    *next_op = CS_INPUT;
    qe_ccb = (QEF_RCB *) cquery->cur_qe_cb;
    qe_copy = qe_ccb->qeu_copy;

    if (op_code == CS_INPUT)
    {
	/* Input implies that the FE sent an attention / interrupt.
	** See what the FE sent to us.  Ignore ACK, RESPONSE or
	** interrupt ends the copy.  Anything else just sort of
	** gets ignored (which is probably wrong).
	*/
	if ((cscb->cscb_gci.gca_status == E_GC0001_ASSOC_FAIL)
	    || (cscb->cscb_gci.gca_status == E_GC0023_ASSOC_RLSED))
	{
	    /* Fail the COPY, ignore error */
	    qe_copy->qeu_stat |= CPY_FAIL;
	    (void) qef_call(QEU_E_COPY, qe_ccb);
	    *next_op = CS_TERMINATE;
	    sscb->sscb_state = SCS_TERMINATE;
	    sscb->sscb_cfac = DB_CLF_ID;
	    return (SEQUENCER_RETURN);
	}
	else if (cscb->cscb_gci.gca_status != E_GC0000_OK)
	{
	    sc0e_0_put(cscb->cscb_gci.gca_status,
		    &cscb->cscb_gci.gca_os_status);
	    sc0e_0_put(E_SC022F_BAD_GCA_READ, 0);
	    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
	    /* Fail the COPY, ignore error */
	    qe_copy->qeu_stat |= CPY_FAIL;
	    (void) qef_call(QEU_E_COPY, qe_ccb);
	    sscb->sscb_state = SCS_INPUT;
	    sscb->sscb_qmode = 0;
	    sscb->sscb_cfac = DB_CLF_ID;
	    *next_op = CS_EXCHANGE;
	    *ret_val = E_DB_SEVERE;
	    return (SEQUENCER_RETURN);
	}	
	cscb->cscb_gci.gca_status = E_SC1000_PROCESSED;

	/*
	** See what the front end wanted to tell us.
	*/

	if ((cscb->cscb_gci.gca_message_type == GCA_RESPONSE)
	    || (sscb->sscb_interrupt))
	{

	    qe_copy->qeu_stat |= CPY_FAIL;
	    status = qef_call(QEU_E_COPY, qe_ccb);
	    if (DB_FAILURE_MACRO(status))
	    {
		*ret_val = E_DB_ERROR;
		scs_qef_error(status, qe_ccb->error.err_code,
				E_SC0210_SCS_SQNCR_ERROR, scb);
		sc0e_0_put(E_SC0251_COPY_CLOSE_ERROR, 0);
	    }

	    cquery->cur_row_count = GCA_NO_ROW_COUNT;

	    /*
	    ** If return is OK, CS_EXCHANGE to send a RESPONSE with
	    ** the rowcount.  If copy-end fails, don't send anything.
	    */
	    if (*ret_val == E_DB_OK)
	    {
		*ret_val = E_DB_WARN;	/* So sequencer sets GCA_FAIL_MASK */
		*next_op = CS_EXCHANGE;
	    }
	    else
		*next_op = CS_INPUT;

	    /*
	    ** We're done with the copy, go back into input state
	    */

	    sscb->sscb_state = SCS_INPUT;
	    return (SEQUENCER_BREAK);
	}
	else if (cscb->cscb_gci.gca_message_type == GCA_ACK)
	{
	    /* Fine, send ack's all day */
	    *next_op = CS_OUTPUT;
	}
    }
    cpo_endcopy = FALSE;
    *next_op = CS_OUTPUT;

    /* Allocate a message buffer for the send.  Use the larger of
    ** the comm buffer size or the tuple size, unless the table contains
    ** LOB's, in which case use the max size to make LOB processing easier.
    **
    ** FIXME this message buffer really ought to be handled with
    ** NODEALLOC_MASK and maybe dealloc-on-query-done.  As it is, we're
    ** doing a butt-load of memory allocs and deallocs for large copies.
    */
    if ((cquery->cur_rdesc.rd_modifier & GCA_LO_MASK) == 0)
    {
	if (qe_copy->qeu_ext_length > cscb->cscb_comm_size)
	{
	    cpo_gca_size = qe_copy->qeu_ext_length;
	}
	else
	{
	    cpo_gca_size = cscb->cscb_comm_size;
	}
    }
    else
    {
	cpo_gca_size = Sc_main_cb->sc_maxtup;
    }
    status = sc0m_allocate(0,
		sizeof(SCC_GCMSG) + cpo_gca_size,
		SCS_MEM,
		(PTR)SCS_MEM,
		SCG_TAG,
		(PTR *) &message);
    if (status)
    {
	sc0e_0_put(status, 0);
	sc0e_0_uput(status, 0);
	scd_note(E_DB_SEVERE, DB_SCF_ID);
    }
    /* Link message onto the end of our to-send list */
    message->scg_next = (SCC_GCMSG *) &cscb->cscb_mnext;
    message->scg_prev = cscb->cscb_mnext.scg_prev;
    cscb->cscb_mnext.scg_prev->scg_next = message;
    cscb->cscb_mnext.scg_prev = message;
    if (cscb->cscb_different)
    {
	message->scg_mdescriptor = cquery->cur_rdesc.rd_tdesc;
    }
    else
    {
	message->scg_mdescriptor = NULL;
    }

    message->scg_marea = cpo_data = ((char *)message + sizeof(SCC_GCMSG));
    message->scg_mask = 0;
    message->scg_mtype = GCA_CDATA;
    cpo_outbuf_size = message->scg_msize = cpo_gca_size;
    cpo_dt_size = cpo_outbuf_size;
    cpo_send_size = 0;

    /*
    ** Figure out how many tuples to get on this call to QEF.
    ** "cpo_dt_size" is the length of the output buffer in whole rows.
    ** If there are LOB's, operate one row at a time.
    */
    qe_copy->qeu_cur_tups = 0;
    if ((cquery->cur_rdesc.rd_modifier & GCA_LO_MASK) == 0)
    {
	cpo_out_tups = cpo_dt_size / qe_copy->qeu_ext_length;
    }
    else
    {
	cpo_out_tups = 1;
    }

    if (cpo_out_tups > 0)
    {
	if (qe_copy->qeu_rowspace < cpo_out_tups)
	{
	    cpo_new_rows = cpo_out_tups;

	    /*
	    ** Allocate storage to format into QEF_DATA structs.
	    ** These structs will be passed to QEF and
	    ** will point to  the buffers to get/put rows.
	    */

	    status = sc0m_allocate(0, 
		sizeof(SC0M_OBJECT) + (cpo_new_rows * sizeof(QEF_DATA)),
		DB_SCF_ID, (PTR)SCS_MEM, 
		CV_C_CONST_MACRO('C','P','Y','D'),
		&cquery->cur_amem);
	    if (DB_FAILURE_MACRO(status))
	    {
		sc0e_0_put(status, 0);
		sc0e_0_uput(status, 0);
		sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		scs_copy_error(scb, qe_ccb, 0);
		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
		return (SEQUENCER_BREAK);
	    }

	    /*
	    ** Format the QEF_DATA structs into a linked list.
	    */
	    cquery->cur_qef_data =
		(QEF_DATA *)((char *)cquery->cur_amem + sizeof(SC0M_OBJECT));
	    data_buffer = cquery->cur_qef_data;

	    for (i = 0; i < cpo_new_rows; i++)
	    {
		data_buffer->dt_size = qe_copy->qeu_ext_length;
		data_buffer->dt_next = data_buffer + 1;
		data_buffer->dt_data = NULL;
		data_buffer++;
	    }
	    data_buffer--;
	    data_buffer->dt_next = NULL;

	    qe_copy->qeu_rowspace = cpo_new_rows;
	}

	/*
	** Set data pointers in QEF_DATA list to point at
	** positions for tuples in the output block.
	**
	** If there is leftover space at the end of the output
	** block then point the last QEF_DATA struct to
	** qeu_sbuf.  We can then write part of the tuple
	** this block and the rest in the next one.
	*/
	row_buf = cpo_data;
	data_buffer = cquery->cur_qef_data;
	qe_copy->qeu_cur_tups = cpo_out_tups;
	qe_copy->qeu_output = cquery->cur_qef_data;

	while (--cpo_out_tups >= 0)
	{
	    data_buffer->dt_data = row_buf;

	    row_buf += qe_copy->qeu_ext_length;
	    data_buffer = data_buffer->dt_next;
	}
	if (cquery->cur_rdesc.rd_modifier & GCA_LO_MASK)
	{
	    /* LOB is different, the row is put into the qeu_sbuf
	    ** save area, since the redeemed LOB data will get
	    ** interspersed with the non-LOB stuff.
	    */
	    if ((qe_copy->qeu_sbuf != NULL) &&
		(qe_copy->qeu_ssize < qe_copy->qeu_ext_length))
	    {
		sc0m_deallocate(0, &qe_copy->qeu_sbuf);
		qe_copy->qeu_ssize = 0;
	    }
	    if (qe_copy->qeu_sbuf == NULL)
	    {
		status = sc0m_allocate(0,
		    sizeof(SC0M_OBJECT) + qe_copy->qeu_ext_length,
		    DB_SCF_ID, (PTR)SCS_MEM,
		    CV_C_CONST_MACRO('C','P','Y','T'),
		    &qe_copy->qeu_sbuf);
		if (DB_FAILURE_MACRO(status))
		{
		    sc0e_0_put(status, 0);
		    sc0e_0_uput(status, 0);
		    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		    scs_copy_error(scb, qe_ccb, 0);
		    sscb->sscb_state = SCS_INPUT;
		    *next_op = CS_EXCHANGE;
		    return (SEQUENCER_BREAK);
		}
		qe_copy->qeu_sptr = (char *)qe_copy->qeu_sbuf + sizeof(SC0M_OBJECT);
		qe_copy->qeu_ssize = qe_copy->qeu_ext_length;
		qe_copy->qeu_sused = 0;
	    }

	    data_buffer = cquery->cur_qef_data;
	    data_buffer->dt_data = qe_copy->qeu_sptr;
	}

	/* If we're not in the middle of sending LOB stuff, or if there
	** are not LOB's, qeu_cp_in_offset will be -1.
	*/
	if (qe_copy->qeu_cp_in_offset < 0)
	{
	    /*
	    ** Call QEF to get the tuples.
	    */
	    status = qef_call(QEU_W_COPY, qe_ccb);
	    if (DB_FAILURE_MACRO(status))
	    {

		if ((qe_ccb->error.err_code == E_QE0015_NO_MORE_ROWS) ||
		    sscb->sscb_interrupt)
		{
		    cpo_endcopy = TRUE;
		}
		else if (qe_ccb->error.err_code == E_QE0025_USER_ERROR)
		{
		    /* who cares */
		}
		else
		{
		    scs_qef_error(status,
				  qe_ccb->error.err_code,
				  E_SC0210_SCS_SQNCR_ERROR,
				  scb);
		    qe_copy->qeu_stat |= CPY_FAIL;
		    scs_copy_error(scb, qe_ccb, 0);
		    sscb->sscb_state = SCS_INPUT;
		    *next_op = CS_EXCHANGE;
		    return (SEQUENCER_BREAK);
		}
	    }

	    /*
	    ** If QEF didn't give us as many tuples as
	    ** we asked for, then only send as many
	    ** tuples as we got.
	    */
	    cpo_send_size += (qe_copy->qeu_cur_tups *
			      qe_copy->qeu_ext_length);

	} /* if need to call QEF */
    }

    /*
    ** If dealing with large objects, install the
    ** objects inline for transmission to the FE.
    */

    if (   (!cpo_endcopy)
	&& (cquery->cur_rdesc.rd_modifier & GCA_LO_MASK) != 0)
    {

	/* Send non-LOB data out of qeu_sptr or redeem/send LOB data,
	** as instructed by the qe_copy state information.
	*/
	status = scs_cp_redeem(scb, qe_copy, cpo_data, cpo_dt_size);

	if (status)
	{
	    if (DB_FAILURE_MACRO(status))
	    {
		sc0e_0_put(status, 0);
		sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		scs_copy_error(scb, qe_ccb, 1);
		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_INPUT;
		return (SEQUENCER_BREAK);
	    }
	    else
	    {
		/*
		** E_DB_INFO says that no error occurred, but we're
		** not yet finished with the tuple.  Here, we
		** just leave the number of tuples as
		** zero, and fall thru the code,
		** eventually returning here with
		** another copy block to process.
		*/
		qe_copy->qeu_cur_tups = 0;
		message->scg_mask |= SCG_NOT_EOD_MASK;
	    }
	}
	else
	{
	    qe_copy->qeu_cur_tups = 1;
	}
	cpo_send_size = qe_copy->qeu_cp_out_offset;
    }

    /* 
    ** Increment the count of tuples copied out so far.
    */
    qe_copy->qeu_count += qe_copy->qeu_cur_tups;

    message->scg_msize = cpo_send_size;
    message->scg_mtype = GCA_CDATA;
    /*
    **  Compress the tuple, if it has any
    **  varchars.
    */
    {
	GCA_TD_DATA *tdesc;
	i4 total_dif;

	tdesc = (GCA_TD_DATA *) qe_copy->qeu_tdesc;

	if ((tdesc) &&
	    (tdesc->gca_result_modifier & GCA_COMP_MASK))
	{
	    if (status = scs_compress(tdesc,
			cpo_data,
			qe_copy->qeu_cur_tups,
			&total_dif))
	    {
		/* Bring data transfer for this file to a stop */
		cpo_endcopy = TRUE;

		/* Tell all cut threads to quit */
		qe_copy->qeu_stat |= CPY_FAIL;
	    }
	    message->scg_msize -= total_dif; 
	}
    }

    if (cpo_endcopy)
    {
	/*
	** Done with copy, have to call QEF to close the
	** copy, and must send response to FE.
	*/
	status = qef_call(QEU_E_COPY, qe_ccb);
	if (DB_FAILURE_MACRO(status))
	{
	    *ret_val = E_DB_ERROR;
	    scs_qef_error(status,
			    qe_ccb->error.err_code,
			    E_SC0210_SCS_SQNCR_ERROR,
			    scb);
	    sc0e_0_put(E_SC0251_COPY_CLOSE_ERROR, 0);
	}

	/* Correction for the fix for bug 2678: if there are
	** no rows to send then don't send the block. Turning
	** it into a response block creates too many response
	** blocks.
	*/
	if (message->scg_msize == 0)
	{
	    /* Take the message off of its queue to prevent
	    ** it's being sent. Then deallocate it.
	    */
	    message->scg_prev->scg_next = message->scg_next;
	    message->scg_next->scg_prev = message->scg_prev;
	    sc0m_deallocate(0, (PTR *)&message);
	}

	if (cquery->cur_amem)
	{
	    sc0m_deallocate(0, &cquery->cur_amem);
	    cquery->cur_qef_data = NULL;
	}
	if (qe_copy->qeu_sbuf != NULL)
	{
	    sc0m_deallocate(0, &qe_copy->qeu_sbuf);
	}
	cquery->cur_row_count = qe_copy->qeu_count;

	/* deallocate qsf memory */
	qs_ccb = sscb->sscb_qsccb;
	qs_ccb->qsf_obj_id.qso_handle = cquery->cur_qp.qso_handle;
	qs_ccb->qsf_lk_id = cquery->cur_lk_id;
	status = qsf_call(QSO_DESTROY, qs_ccb);
	if (DB_FAILURE_MACRO(status))
	{
	    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
		E_SC0210_SCS_SQNCR_ERROR);
	}
	cquery->cur_qp.qso_handle = 0;
	cquery->cur_qe_cb = NULL;

	/*
	** If return is OK and we still have a message to send
	** then CS_EXCHANGE to write current output
	** block and read the status block.  If not 
	** then just read the status block from the FE.
	*/
	if (*ret_val == E_DB_OK && message != NULL)
	    *next_op = CS_EXCHANGE;
	else
	    *next_op = CS_INPUT;

	/*
	** We have to get a status block from the Front End to
	** tell us how many rows it really put into the copy file.
	*/
	sscb->sscb_state = SCS_INPUT;
    }
    return (SEQUENCER_BREAK);

} /* scs_copy_into */

/*
** Name: scs_copy_bad - Sequencer control for COPY in an error state
**
**
** Description:
**	We have encountered an error during COPY FROM.  Presumably
**	an interrupt has been sent to the front-end.  However, the
**	front-end is streaming rows, and only checks for messages
**	from the server every now and then.  This routine tosses
**	CDATA messages (and ACK's), until an IACK arrives.  That
**	means that the FE has noticed the interrupt and has stopped
**	sending rows.  We can now send the queued errors and end
**	the copy.
**
**	During force-abort processing, this code may be
**	executed after code below has already deallocated
**	qs_ccb, hence checks are made here to ensure that
**	pointers are non-null.
**
**	If we get here for a state that isn't SCS_CP_ERROR, something
**	has gone astray in the sequencer, and we squawk loudly.
**
**	Next State:
**	  - SCS_CP_ERROR if haven't seen the IACK yet.
**	  - SCS_INPUT if FE terminates copy.
**	Next CS Operation:
**	  - CS_INPUT
**
** Inputs:
**	scb		Sequencer session control block
**	next_op		An output
**	ret_val		An output
**
** Outputs:
**	scb		sscb_state updated
**	next_op		Set to CS_INPUT as above
**	ret_val		Set to return value for caller return
**
** Returns a sequencer-action code (continue, break, or return(ret_val)).
**
** History:
**	25-Mar-2010 (kschendel) SIR 123485
**	    Hatchet COPY code out of sequencer mainline.
*/

sequencer_action_enum
scs_copy_bad(SCD_SCB *scb, i4 *next_op, DB_STATUS *ret_val)

{
    DB_STATUS status;
    QEF_RCB *qe_ccb;		/* QEF request block */
    QEU_COPY *qe_copy;		/* COPY control block */
    QSF_RCB *qs_ccb;		/* QSF request block */
    SCC_CSCB *cscb;		/* Pointer to scb->scb_cscb */
    SCC_GCMSG *message;		/* Message we're building to send */
    SCS_CURRENT *cquery;	/* Pointer to scb->scb_sscb.sscb_cquery */
    SCS_SSCB *sscb;		/* Pointer to scb->scb_sscb */

    /* Load up convenience pointers */

    cscb = &scb->scb_cscb;
    sscb = &scb->scb_sscb;
    cquery = &sscb->sscb_cquery;

    *ret_val = E_DB_OK;

    if (sscb->sscb_state == SCS_CP_ERROR)
    {
	i4 cpe_block_type;

	qe_ccb = (QEF_RCB *) cquery->cur_qe_cb;
	qe_copy = qe_ccb ? qe_ccb->qeu_copy : NULL;
	*next_op = CS_INPUT;

	/* If we got here because of some sequencer or memory
	** alloc error, the COPY is still running, and we need
	** to shut it down.  (If we got here because of interrupt
	** or force-abort etc, COPY end was already called.)
	*/
	if (qe_copy != NULL && qe_copy->qeu_copy_ctl != NULL)
	{
	    qe_copy->qeu_stat |= CPY_FAIL;
	    status = qef_call(QEU_E_COPY, qe_ccb);
	    /* We're already in error status, toss any
	    ** additional error from copy shutdown
	    */
	}

	if ((cscb->cscb_gci.gca_status == E_GC0001_ASSOC_FAIL)
	    || (cscb->cscb_gci.gca_status == E_GC0023_ASSOC_RLSED))
	{
	    *next_op = CS_TERMINATE;
	    sscb->sscb_state = SCS_TERMINATE;
	    sscb->sscb_cfac = DB_CLF_ID;
	    /* FIXME this leaks COPY memory, really ought to fix.
	    ** Memory not deallocated above because we want to
	    ** keep it around for normal case to report error,
	    ** rowcount
	    */
	    return (SEQUENCER_RETURN);
	}
	else if (cscb->cscb_gci.gca_status != E_GC0000_OK)
	{
	    sc0e_0_put(cscb->cscb_gci.gca_status,
		    &cscb->cscb_gci.gca_os_status);
	    sc0e_0_put(E_SC022F_BAD_GCA_READ, 0);
	    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
	    sscb->sscb_state = SCS_INPUT;
	    sscb->sscb_qmode = 0;
	    sscb->sscb_cfac = DB_CLF_ID;
	    *next_op = CS_EXCHANGE;
	    *ret_val = E_DB_SEVERE;
	    return (SEQUENCER_RETURN);
	}	
	cscb->cscb_gci.gca_status = E_SC1000_PROCESSED;
	cpe_block_type = cscb->cscb_gci.gca_message_type;

	if (cpe_block_type == GCA_CDATA)
	{
	    /* Just eat it */
	    sscb->sscb_cfac = DB_CLF_ID;
	    return (SEQUENCER_RETURN);
	}
	else if (cpe_block_type != GCA_RESPONSE && cpe_block_type != GCA_IACK)
	{
	    i4   should_be = GCA_RESPONSE;

	    sc0e_put(E_SC022E_WRONG_BLOCK_TYPE, 0, 2,
		     sizeof(cpe_block_type), (PTR)&cpe_block_type,
		     sizeof(i4), (PTR) &should_be,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
	    sc0e_0_put(E_SC0250_COPY_OUT_OF_SEQUENCE, 0);
	    sc0e_0_uput(E_SC0250_COPY_OUT_OF_SEQUENCE, 0);
	    scd_note(E_DB_ERROR, DB_SCF_ID);

	    *ret_val = E_DB_ERROR;

	    /* fall through to clean up */
	}

	/*
	** In force_abort case, qe_copy may not be set up.
	*/
	cquery->cur_row_count = qe_copy ? qe_copy->qeu_count : GCA_NO_ROW_COUNT;

	/*
	** Before returning, give error or warning message if any
	** errors or warnings were found during copy.
	*/
	if (qe_copy && qe_copy->qeu_error)
	{
	    scs_cpxmit_errors(scb, FALSE);
	    if (*ret_val == E_DB_OK)
		*ret_val = E_DB_WARN;	/* So we get GCA_FAIL_MASK */
	    if (qe_copy->qeu_stat & CPY_FAIL)
	    {
		sc0e_0_uput(5842L /* user error */, 0);
	    }
	    else
	    {
		char	    buf[16];
		/* COPY: Copy terminated abnormally.  %d rows were
		**       successfully copied. */
		CVla(qe_copy->qeu_count, buf);

		sc0e_uput(5841L /* user error */, 0, 1,
		    STlength(buf), (PTR) buf,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
	    }
	}

	/*
	** If done, then clean up memory used for COPY and the
	** control blocks allocated by PSF for COPY processing.
	*/
	if (qe_copy && (qe_copy->qeu_sbuf != NULL))
	{
	    sc0m_deallocate(0, &qe_copy->qeu_sbuf);
	}
	if (cquery->cur_amem) 
	{
	    sc0m_deallocate(0, &cquery->cur_amem);
	    cquery->cur_qef_data = NULL;
	}

	qs_ccb = sscb->sscb_qsccb;
	qs_ccb->qsf_obj_id.qso_handle = cquery->cur_qp.qso_handle;
	qs_ccb->qsf_lk_id = cquery->cur_lk_id;
	if (qs_ccb->qsf_obj_id.qso_handle)
	{
	    status = qsf_call(QSO_DESTROY, qs_ccb);
	    if (DB_FAILURE_MACRO(status))
	    {
		scs_qsf_error(status, qs_ccb->qsf_error.err_code,
		    E_SC0210_SCS_SQNCR_ERROR);
	    }
	}
	cquery->cur_qp.qso_handle = 0;
	cquery->cur_qe_cb = NULL;

	sscb->sscb_state = SCS_INPUT;
	return (SEQUENCER_BREAK);
    }
    else
    {
	/* Invalid state for COPY, complain */
	sc0e_0_put(E_SC021B_BAD_COPY_STATE, 0);
	sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
	scd_note(E_DB_ERROR, DB_SCF_ID);

	if (qe_ccb = (QEF_RCB *) cquery->cur_qe_cb)
	{
	    scs_copy_error(scb, qe_ccb, 0);
	    sscb->sscb_state = SCS_CP_ERROR;
	    sscb->sscb_cfac = DB_CLF_ID;
	    *next_op = CS_INPUT;
	    *ret_val = E_DB_ERROR;
	    return (SEQUENCER_RETURN);
	}

	*ret_val = E_DB_ERROR;
	return (SEQUENCER_BREAK);
    }
} /* scs_copy_bad */

/*}
** Name: scs_scopy	- send GCA_C_{INTO,FROM} block to FE
**
** Description:
**
** Inputs:
**      scb                             session control block for this session.
**	qe_copy				copy control block
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    Copy map message is created.
**
** History:
**      21-Jul-1987 (fred)
**          GCA/GCF Support introduced.
**      18-sep-89 (jrb)
**          Added code to put prec*256+scale into length fields of GCA_CPRDD
**          message for attribute descriptors of DECIMAL values.
**	07-nov-91 (ralph)
**	    6.4->6.5 merge:
**		22-jul-91 (stevet)
**		    Added support for new copy map message for
**		    GCA_PROTOCOL_LEVEL_50.  This new map supports
**		    GCA_DATA_VALUE format for NULL value description
**		    and added precision type to column description.
**	6-jan-1993 (stevet)
**	    Remove code to put prec*256+scale into length field because 
**	    we have added new precision fields to GCA_PROTOCOL_LEVEL_50.
**	    Also start passing the precisions to the FE.
**      26-Feb-1993 (fred)
**  	    Added large object support.  Initialized new sscb_copy
**          fields which are used to keep information across copy
**          block transmissions when large objects are in use.  Also,
**          fixed latent bug made evident in this code, wherein the
**          fact that the gca_result_modifer was assumed to always be
**          zero (it isn't with large objects).
**      23-Jun-1993 (fred)
**          Fixed bug in copy null indicator value handling.  When an
**          indicator value was provided, the parser converts it into
**          a value of the type being sent back.  When this value is a
**          large type, then a coupon is build.  However, this routine
**          was not cognizant of the fact that there was a coupon
**          there;  therefore it didn't redeem it, causing the coupon
**          to be sent as is to the frontend, where it was really
**          pretty worthless.  This routine now keeps an eye out for
**          coupons, redeeming them for transmission when necessary.
**	2-Jul-1993 (daveb)
**	    prototyped, use proper PTR bor allocation.
**      05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**          This results in owner within sc0m_allocate having to be PTR.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**	21-Dec-2001 (gordy)
**	    GCA_COL_ATT no longer defined with DB_DATA_VALUE.
**	19-Mar-2010 (kschendel) SIR 123448
**	    Kill unused qeu_moving_data.
*/

static DB_STATUS
scs_scopy(SCD_SCB *scb,
	  QEU_COPY  *qe_copy )

{
    char	    *data;
    char            *data_start;
    QEU_CPDOMD	    *cpdom_desc;
    GCA_TD_DATA	    *sqlda;
    GCA_DATA_VALUE  gdv;
    SCC_GCMSG	    *message;
    i4		    filename_length;
    i4		    logname_length;
    i4		    data_size, buffer_size;
    i4		    status, i, error;
    i4	    ln_val;
    i4	    cp_num_attr;
    i4		    n_val;
    i4              data_size_suspect = 0;
    PTR             block;
    char            *msg_start;

    qe_copy->qeu_cp_cur_att = -1;
    qe_copy->qeu_cp_att_need = -1;
    qe_copy->qeu_cp_in_offset = -1;
    qe_copy->qeu_cp_out_offset = -1;
    qe_copy->qeu_cp_in_lobj = 0;
    qe_copy->qeu_cp_att_count = -1;

    if (qe_copy->qeu_fdesc->cp_filename)
	filename_length = STlength(qe_copy->qeu_fdesc->cp_filename) + 1;
    else
	filename_length = 0;

    if (qe_copy->qeu_logname)
	logname_length = STlength(qe_copy->qeu_logname) + 1;
    else
	logname_length = 0;

    sqlda = (GCA_TD_DATA *) qe_copy->qeu_tdesc;

    /* 
    ** Make sure ULC_COPY struct will fit in the output buffer.
    */

    /*
    **  Add new copy map support for GCA_PROTOCOL_LEVEL_50 
    */

    cp_num_attr = (i4)qe_copy->qeu_fdesc->cp_filedoms;

    if (scb->scb_cscb.cscb_version >= GCA_PROTOCOL_LEVEL_50)
    {
	data_size = (sizeof(i4)	+	/* cp_status */
		    sizeof(i4)		+	/* cp_direction */
		    sizeof(i4)		+	/* cp_maxerrs */
		    sizeof(i4)		+	/* cp_fname length */
		    sizeof(i4)		+	/* cp_logname length */
		    sizeof(i4)		+	/* cp_filedoms */
		    sizeof(i4)		+	/* da_tup_size */
		    sizeof(i4)		+	/* gca_result */
		    sizeof(u_i4)	+	/* gca_id_tdscr */  
		    sizeof(i4))	+	/* da_num_atts */
		    (sqlda->gca_l_col_desc	*	/* da_attribute */
		     (sizeof(sqlda->gca_col_desc[0].gca_attdbv) +
		      sizeof(sqlda->gca_col_desc[0].gca_l_attname))) +
		    (filename_length)	+	/* cp_fname */
		    (logname_length)	+	/* cp_logname */
		    (cp_num_attr		*	/* cp_fdesc */
			(GCA_MAXNAME	+	/* cp_domname */
			sizeof(i4)		+	/* cp_type */
			sizeof(i4)		+	/* cp_precision */
			sizeof(i4)	+	/* cp_length */
			sizeof(i4)		+	/* cp_user_delim */
			sizeof(i4)		+	/* size of ... */
			CPY_MAX_DELIM	+	/* cp_delimiter */
			sizeof(i4)	+	/* cp_tupmap */
			sizeof(i4)		+	/* cp_cvid */
			sizeof(i4)		+	/* cp_cvprec */
			sizeof(i4)		+	/* cp_cvlen */
			sizeof(i4)		+	/* cp_withnull */
			sizeof(i4)));		/* cp_nulldata_value */
	for (i = 0, cpdom_desc = qe_copy->qeu_fdesc->cp_fdom_desc;
	    i < cp_num_attr;
	    i++, cpdom_desc = cpdom_desc->cp_next)
	{
	    if( cpdom_desc->cp_nullen)
	    {
		data_size += sizeof(gdv.gca_type) + 
			     sizeof(gdv.gca_precision) +
			     sizeof(gdv.gca_l_value);
		if ((sqlda->gca_result_modifier & GCA_LO_MASK)
					   == 0)
		{
		    /*
		    ** In this case, there can be no large object
		    ** types in the copy, so we needn't worry
		    ** about them individually.
		    */

		    data_size += cpdom_desc->cp_nullen;
		}
		else
		{
		    /*
		    ** if this att is a large object,
		    ** then
		    **     add MAXTUP
		    ** else
		    **     add length as above
		    */

		    
		    status = adi_dtinfo(scb->scb_sscb.sscb_adscb,
					cpdom_desc->cp_type,
					&n_val);
		    if (DB_FAILURE_MACRO(status))
		    {
			sc0e_0_put(scb->scb_sscb.sscb_adscb->adf_errcb.ad_errcode,
				 0);
			sc0e_0_uput(scb->scb_sscb.sscb_adscb->adf_errcb.ad_errcode,
				 0);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			return (E_DB_ERROR);
		    }
		    
		    if (n_val & AD_PERIPHERAL)
		    {
			data_size += DB_MAXTUP;
			data_size_suspect = 1;
		    }
		    else
		    {
			data_size += cpdom_desc->cp_nullen;
		    }
		}
	    }
	}
    }
    else
    {
	data_size = (sizeof(i4)	+	/* cp_status */
		    sizeof(i4)		+	/* cp_direction */
		    sizeof(i4)		+	/* cp_maxerrs */
		    sizeof(i4)		+	/* cp_fname length */
		    sizeof(i4)		+	/* cp_logname length */
		    sizeof(i4)		+	/* cp_filedoms */
		    sizeof(i4)		+	/* da_tup_size */
		    sizeof(i4)		+	/* gca_result */
		    sizeof(u_i4)	+	/* gca_id_tdscr */  
		    sizeof(i4))	+	/* da_num_atts */
		    (sqlda->gca_l_col_desc	*	/* da_attribute */
		     (sizeof(sqlda->gca_col_desc[0].gca_attdbv) +
		      sizeof(sqlda->gca_col_desc[0].gca_l_attname))) +
		    (filename_length)	+	/* cp_fname */
		    (logname_length)	+	/* cp_logname */
		    (cp_num_attr		*	/* cp_fdesc */
			(GCA_MAXNAME	+	/* cp_domname */
			sizeof(i4)		+	/* cp_type */
			sizeof(i4)	+	/* cp_length */
			sizeof(i4)		+	/* cp_user_delim */
			sizeof(i4)		+	/* size of ... */
			CPY_MAX_DELIM	+	/* cp_delimiter */
			sizeof(i4)	+	/* cp_tupmap */
			sizeof(i4)		+	/* cp_cvid */
			sizeof(i4)		+	/* cp_cvlen */
			sizeof(i4)		+	/* cp_withnull */
			sizeof(i4)));		/* cp_nullen */

	for (i = 0, cpdom_desc = qe_copy->qeu_fdesc->cp_fdom_desc;
	    i < cp_num_attr;
	    i++, cpdom_desc = cpdom_desc->cp_next)
	{
	    data_size += cpdom_desc->cp_nullen;
	}
    }

    if (qe_copy->qeu_sbuf != NULL)
	sc0m_deallocate(0, &qe_copy->qeu_sbuf);
    status = sc0m_allocate(0,
			(i4)(sizeof(SCC_GCMSG)
			+ data_size),
			DB_SCF_ID,
			(PTR) SCS_MEM,
			CV_C_CONST_MACRO('S','C','P','Y'),
			&block);
    message = (SCC_GCMSG *)block;
    if (DB_FAILURE_MACRO(status))
    {
	sc0e_0_put(status, 0);
	sc0e_0_uput(status, 0);
	sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
	return (E_DB_ERROR);
    }
    message->scg_next = (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext;
    message->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
    scb->scb_cscb.cscb_mnext.scg_prev->scg_next = message;
    scb->scb_cscb.cscb_mnext.scg_prev = message;
    message->scg_msize = data_size;

    /*
    **  New message type for GCA_PROTOCOL_LEVEL_50
    */
    if (scb->scb_cscb.cscb_version >= GCA_PROTOCOL_LEVEL_50)
    {
	if (qe_copy->qeu_direction == CPY_FROM)
	{
	    message->scg_mtype = GCA1_C_FROM;
	}
	else
	{
	    message->scg_mtype = GCA1_C_INTO;
	}
    }
    else
    {
	if (qe_copy->qeu_direction == CPY_FROM)
	{
	    message->scg_mtype = GCA_C_FROM;
	}
	else
	{
	    message->scg_mtype = GCA_C_INTO;
	}
    }

    if (	(scb->scb_cscb.cscb_different == TRUE)
	 || (sqlda->gca_result_modifier & GCA_LO_MASK))
    {
	message->scg_mask =
	    (SCG_NODEALLOC_MASK |SCG_QDONE_DEALLOC_MASK);
    }
    else
    {
	message->scg_mask = 0;
    }

    message->scg_marea = msg_start = data = ((char *) message + sizeof(SCC_GCMSG));
    buffer_size = data_size;

    ln_val = (i4)qe_copy->qeu_stat;
    MEcopy((PTR)&ln_val, sizeof(i4), (PTR)data);
    data += sizeof(i4);

    ln_val = (i4)qe_copy->qeu_direction;
    MEcopy((PTR)&ln_val, sizeof(i4), (PTR)data);
    data += sizeof(i4);

    ln_val = (i4)qe_copy->qeu_maxerrs;
    MEcopy((PTR)&ln_val, sizeof(i4), (PTR)data);
    data += sizeof(i4);

    n_val = (i4)filename_length;
    MEcopy((PTR)&n_val, sizeof(i4), (PTR)data);
    data += sizeof(i4);

    if (filename_length)
    {
	MEcopy((PTR)qe_copy->qeu_fdesc->cp_filename, filename_length,
	    (PTR)data);
	data += filename_length;
    }

    n_val = (i4)logname_length;
    MEcopy((PTR)&n_val, sizeof(i4), (PTR)data);
    data += sizeof(i4);

    if (logname_length)
    {
	MEcopy((PTR)qe_copy->qeu_logname, logname_length, (PTR)data);
	data += logname_length;
    }

    /* Now let's copy the tuple descriptor */
    if (	(scb->scb_cscb.cscb_different == TRUE)
	 || (sqlda->gca_result_modifier & GCA_LO_MASK))
    {
	scb->scb_sscb.sscb_cquery.cur_rdesc.rd_tdesc = data;
    }
    else
    {
	scb->scb_sscb.sscb_cquery.cur_rdesc.rd_tdesc = NULL;
    }
    ln_val = (i4)sqlda->gca_tsize;
    MEcopy((PTR)&ln_val, sizeof(i4), (PTR)data);
    data += sizeof(i4);

    n_val = sqlda->gca_result_modifier;  /* gca_result_modifier */
    scb->scb_sscb.sscb_cquery.cur_rdesc.rd_modifier =
	sqlda->gca_result_modifier;
    MEcopy((PTR)&n_val, sizeof(i4), (PTR)data);
    data += sizeof(i4);

    ln_val = scb->scb_sscb.sscb_nostatements;   /* gca_id_tdscr */
    MEcopy((PTR) &ln_val, sizeof(u_i4), (PTR) data);
    data += sizeof (u_i4);

    ln_val = (i4)sqlda->gca_l_col_desc;
    qe_copy->qeu_cp_att_count = ln_val;
    MEcopy((PTR)&ln_val, sizeof(i4), (PTR)data);
    data += sizeof(i4);

    for (i = 0; i < sqlda->gca_l_col_desc; i++)
    {
	MEcopy((PTR)&sqlda->gca_col_desc[i].gca_attdbv,
	       sizeof(sqlda->gca_col_desc[0].gca_attdbv),
	       (PTR)data);
	data += sizeof(sqlda->gca_col_desc[0].gca_attdbv);

	/* no names are sent */
	MEfill( sizeof(sqlda->gca_col_desc[0].gca_l_attname), 
		'\0', (PTR)data);
	data += sizeof(sqlda->gca_col_desc[0].gca_l_attname);
    }

    ln_val = cp_num_attr;
    MEcopy((PTR)&ln_val, sizeof(i4), (PTR)data);
    data += sizeof(i4);

    for (i = 0, cpdom_desc = qe_copy->qeu_fdesc->cp_fdom_desc;
	 i < cp_num_attr;
	 i++, cpdom_desc = cpdom_desc->cp_next)
    {
	/*
	** Build the gca_row_desc for the COPY code in libq (iicopy.c).
	** Copy up to GCA_MAXNAME bytes of the cp_domname.
	** 
	** If DB_ATT_MXNAME > GCA_MAXNAME, cp_domname is truncated.
	** This is ok because the truncated cp_domname is mostly
	** used in iicopy.c for error notifications.
	** 
	** The only exception is cp_domname for dummy columns.
	** In this case the dummy column name cannot be truncated,
	** so we need to limit the size of dummy column names to
	** GCA_MAXNAME in pslcopy.c
	*/
	MEmove(DB_ATT_MAXNAME, (PTR) cpdom_desc->cp_domname, ' ', 
	    GCA_MAXNAME, (PTR)data);

	data += GCA_MAXNAME;

	n_val = (i4)cpdom_desc->cp_type;
	MEcopy((PTR)&n_val, sizeof(i4), (PTR)data);
	data += sizeof(i4);
	
	/*
	**  Add precision for GCA_PROTOCOL_LEVEL_50
	*/
	if (scb->scb_cscb.cscb_version >= GCA_PROTOCOL_LEVEL_50)
	{
	    n_val = (i4)cpdom_desc->cp_prec;	
	    MEcopy((PTR)&n_val, sizeof(i4), (PTR)data);
	    data += sizeof(i4);
	}		

	ln_val = (i4)cpdom_desc->cp_length;
	MEcopy((PTR)&ln_val, sizeof(i4), (PTR)data);
	data += sizeof(i4);

	n_val = (i4)cpdom_desc->cp_user_delim;
	MEcopy((PTR)&n_val, sizeof(i4), (PTR)data);
	data += sizeof(i4);

	n_val = CPY_MAX_DELIM;
	MEcopy((PTR)&n_val, sizeof(i4), (PTR)data);
	data += sizeof(i4);
	
	MEcopy((PTR)cpdom_desc->cp_delimiter,CPY_MAX_DELIM,(PTR)data);
	data += CPY_MAX_DELIM;

	ln_val = (i4)cpdom_desc->cp_tupmap;
	MEcopy((PTR)&ln_val, sizeof(i4), (PTR)data);
	data += sizeof(i4);

	n_val = (i4)cpdom_desc->cp_cvid;
	MEcopy((PTR)&n_val, sizeof(i4), (PTR)data);
	data += sizeof(i4);

	/*
	**  Add cp_cvprec for GCA_PROTOCOL_LEVEL_50
	*/
	if (scb->scb_cscb.cscb_version >= GCA_PROTOCOL_LEVEL_50)
	{
	    n_val = (i4)cpdom_desc->cp_cvprec;
	    MEcopy((PTR)&n_val, sizeof(i4), (PTR)data);
	    data += sizeof(i4);
	}		

	ln_val = (i4)cpdom_desc->cp_cvlen;
	MEcopy((PTR)&ln_val, sizeof(i4), (PTR)data);
	data += sizeof(i4);

	n_val = (i4)cpdom_desc->cp_withnull;
	MEcopy((PTR)&n_val, sizeof(i4), (PTR)data);
	data += sizeof(i4);

	if (scb->scb_cscb.cscb_version >= GCA_PROTOCOL_LEVEL_50)
	{
	    /* if cp_nullen > 0 then null value descriptor is given */

	    n_val = (cpdom_desc->cp_nullen ? (i4)TRUE : (i4)FALSE);
	    MEcopy((PTR)&n_val, sizeof(i4), (PTR)data);
	    data += sizeof(i4);
		
	    if( cpdom_desc->cp_nullen)
	    {
		/* 
		** The data type for the NULL descriptor is the
		** same as the row descriptor.
		*/

		gdv.gca_type = cpdom_desc->cp_type; 
		gdv.gca_precision = (i4)cpdom_desc->cp_nulprec; 
		gdv.gca_l_value = cpdom_desc->cp_nullen;

		if ((sqlda->gca_result_modifier & GCA_LO_MASK) != 0) 
		{
		    status = adi_dtinfo(scb->scb_sscb.sscb_adscb,
					gdv.gca_type,
					&n_val);
		    if (DB_FAILURE_MACRO(status))
		    {
			sc0e_0_put(scb->scb_sscb.sscb_adscb->adf_errcb.ad_errcode,
				 0);
			sc0e_0_uput(scb->scb_sscb.sscb_adscb->adf_errcb.ad_errcode,
				 0);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			return (E_DB_ERROR);
		    }

		    if (n_val & AD_PERIPHERAL)
		    {
			i4       res_len;
			i4       res_prec;

			status = scs_redeem(scb,
					    NULL,
					    0,
					    gdv.gca_type,
					    gdv.gca_precision,
					    DB_MAXTUP,
					    cpdom_desc->cp_nuldata,
					    &res_prec,
					    &res_len,
					    data
						+ sizeof(gdv.gca_type)
						+ sizeof(gdv.gca_precision)
						+ sizeof(gdv.gca_l_value));
			if (status)
			{
			    /*
			    ** Error -- null value too long.
			    */

			    sc0e_0_put(scb->scb_sscb.sscb_adscb->adf_errcb.ad_errcode,
				     0);
			    sc0e_0_uput(scb->scb_sscb.sscb_adscb->adf_errcb.ad_errcode,
				      0);
			    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			    return (E_DB_ERROR);
			}
			gdv.gca_l_value = res_len;
			gdv.gca_precision = res_prec;
		    }
		}

		MEcopy((PTR)&gdv.gca_type, 
		       sizeof(gdv.gca_type), (PTR)data);
		data += sizeof(gdv.gca_type);

		MEcopy((PTR)&gdv.gca_precision, 
		       sizeof(gdv.gca_precision), (PTR)data);
		data += sizeof(gdv.gca_precision);

		MEcopy((PTR)&gdv.gca_l_value, 
		       sizeof(gdv.gca_l_value), (PTR)data);
		data += sizeof(gdv.gca_l_value);

		if ((n_val & AD_PERIPHERAL) == 0)

		{
		    /*
		    ** If a large object, then it was materialized
		    ** above.  It's length was placed in the
		    ** gca_l_value area already.
		    */

		    MEcopy((PTR)cpdom_desc->cp_nuldata, 
			   gdv.gca_l_value, (PTR)data);
		}
		data += gdv.gca_l_value;
	    }
	}
	else
	{
	    ln_val = (i4)cpdom_desc->cp_nullen;
	    MEcopy((PTR)&ln_val, sizeof(i4), (PTR)data);
	    data += sizeof(i4);

	    if (cpdom_desc->cp_nullen)
	    {
		MEcopy((PTR)cpdom_desc->cp_nuldata, cpdom_desc->cp_nullen,
		       (PTR)data);
		data += cpdom_desc->cp_nullen;
	    }
	} 
    }
    if (data_size_suspect)
	message->scg_msize = data - msg_start;

    return (E_DB_OK);
}

/*
** History:
**	2-Jul-1993 (daveb)
**	    prototyped.
**	17-jun-2010 (stephenb)
**	    Check if copy has ended already before ending it, otherwise
**	    we may try to reference null values. (Bug 123939)
*/
VOID
scs_copy_error(SCD_SCB	*scb,
                QEF_RCB	*qe_ccb,
                i4	copy_done )
{
    QEU_COPY	*qe_copy;
    QSF_RCB	*qs_ccb;
    i4		status = E_DB_OK;
    i4		i;

    qe_copy = qe_ccb->qeu_copy;
    qe_copy->qeu_error = TRUE;

    /*
    ** If ROLLBACK = DISABLED was specified, don't backout copy statement.
    */
    if ((qe_copy->qeu_stat & CPY_NOABORT) == 0)
	qe_copy->qeu_stat |= CPY_FAIL;

    /*
    ** Call QEF to end copy statement if not ended already.
    */
    if (qe_copy != NULL && qe_copy->qeu_copy_ctl != NULL)
    {
	status = qef_call(QEU_E_COPY, qe_ccb);
	if (DB_FAILURE_MACRO(status))
	{
	    if ((qe_ccb->error.err_code == E_QE0002_INTERNAL_ERROR)
		|| (qe_ccb->error.err_code == E_QE001D_FAIL))
	    {
		sc0e_0_put(E_SC0251_COPY_CLOSE_ERROR, 0);
		sc0e_0_put(qe_ccb->error.err_code, 0);
		scs_cpsave_err(qe_copy, E_SC0251_COPY_CLOSE_ERROR);
	    }
	    else /* rest are scs' fault */
	    {
		sc0e_0_put(E_SC0251_COPY_CLOSE_ERROR, 0);
		sc0e_0_put(E_SC0024_INTERNAL_ERROR, 0);
		sc0e_0_put(qe_ccb->error.err_code, 0);
		sc0e_0_put(E_SC0210_SCS_SQNCR_ERROR, 0);
		scs_cpsave_err(qe_copy, E_SC0251_COPY_CLOSE_ERROR);
	    }
	}
    }

    scb->scb_sscb.sscb_cquery.cur_row_count = qe_copy->qeu_count;

    /*
    ** If COPY is over, write appropriate error message depending on whether
    ** the COPY has been aborted or not.  Clean up memory allocated for COPY
    ** sequencing and destroy control blocks allocated in PSF.
    */
    if (copy_done)
    {
	for (i = 0; i < 5; i++)
	{
	    if (qe_copy->qeu_uputerr[i] != 0)
		sc0e_0_uput(qe_copy->qeu_uputerr[i], 0);
	}

	if (qe_copy->qeu_stat & CPY_FAIL)
	{
	    /* COPY: Copy has been aborted. */
	    sc0e_0_uput(5842L /* user error */, 0);
	}
	else
	{
	    char	buf[16];
	    /* COPY: Copy terminated abnormally.  %d rows successfully copied */

	    CVla(qe_copy->qeu_count, buf);
	    sc0e_uput(5841L /* user error */, 0, 1,
		      STlength(buf), (PTR) buf,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0);		
	}

	if (qe_copy->qeu_sbuf != NULL)
	{
	    sc0m_deallocate(0, &qe_copy->qeu_sbuf);
	}
	if (scb->scb_sscb.sscb_cquery.cur_amem != NULL)
	{
	    sc0m_deallocate(0, &scb->scb_sscb.sscb_cquery.cur_amem);
	}

	qs_ccb = scb->scb_sscb.sscb_qsccb;
	qs_ccb->qsf_obj_id.qso_handle =
	    scb->scb_sscb.sscb_cquery.cur_qp.qso_handle;
	qs_ccb->qsf_lk_id =
	    scb->scb_sscb.sscb_cquery.cur_lk_id;
	status = qsf_call(QSO_DESTROY, qs_ccb);
	if (DB_FAILURE_MACRO(status))
	{
	    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
		E_SC0210_SCS_SQNCR_ERROR);
	}
	scb->scb_sscb.sscb_cquery.cur_qe_cb = (PTR) 0;
	scb->scb_sscb.sscb_cquery.cur_qp.qso_handle = 0;
    }
}

/*
** History:  
**	2-Jul-1993 (daveb)
**	    prototyped.
**	24-jan-1994 (gmanning)
**	    Change errno to local_errno because of NT macro problems.
*/
static void
scs_cpsave_err(QEU_COPY	*qe_copy,
                i4	local_errno )
{
    i4		i;

    for (i = 0; i < 5; i++)
    {
	if (qe_copy->qeu_uputerr[i] == 0)
	{
	    qe_copy->qeu_uputerr[i] = local_errno;
	    return;
	}
    }
}

/*{
** Name: scs_cpxmit_errors	- Transmit a batch of copy errors
**
** Description:
**      This routine is called from COPY FROM processing after 
**      the FE has indicated (by responding to the DBMS originated 
**      interrupt) that it is ready to hear about errors.  The error 
**      blocks are sent, followed by a response block which indicates 
**      that the copy is to continue.  If the copy is to be terminated, 
**      the response block is not sent, as it will be sent from the 
**      sequencer.
**
** Inputs:
**      scb                             Session control block
**	end_stream			!FALSE if this routine should
**					terminate the error stream with
**					a response block.
**
** Outputs:
**      None explicitly.   Errors are sent to the FE via GCF
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      01-Sep-1987 (fred)
**          Created.
**	2-Jul-1993 (daveb)
**	    prototyped, use proper PTR for allocation.
**      05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**          This results in owner within sc0m_allocate having to be PTR.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
[@history_template@]...
*/
static void
scs_cpxmit_errors(SCD_SCB *scb,
		  i4  end_stream )
{
    i4		i;
    DB_STATUS	status;
    DB_STATUS	error;
    SCC_GCMSG	*message;
    QEU_COPY	*qe_copy =
		((QEF_RCB *) scb->scb_sscb.sscb_cquery.cur_qe_cb)->qeu_copy;
    PTR		block;

    for (i = 0; i < 5; i++)
    {
	if (qe_copy->qeu_uputerr[i] != 0)
	    sc0e_0_uput(qe_copy->qeu_uputerr[i], 0);
    }
    if (end_stream)
    {
	status = sc0m_allocate(0,
		    (i4)(sizeof(SCC_GCMSG) + sizeof(GCA_RE_DATA)),
 		    DB_SCF_ID,
		    (PTR)SCS_MEM,
		    SCG_TAG,
		    &block);
	message = (SCC_GCMSG *)block;
	if (status)
	{
	    sc0e_0_put(status, 0);
	    sc0e_0_uput(status, 0);
	    scd_note(E_DB_SEVERE, DB_SCF_ID);
	}
	message->scg_next = (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext;
	message->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
	scb->scb_cscb.cscb_mnext.scg_prev->scg_next = message;
	scb->scb_cscb.cscb_mnext.scg_prev = message;
	message->scg_mask = 0;
	message->scg_mtype = GCA_RESPONSE;
	message->scg_msize = sizeof(GCA_RE_DATA);
	message->scg_marea = ((char *) message + sizeof(SCC_GCMSG));

	((GCA_RE_DATA *)message->scg_marea)->gca_rqstatus =
		scb->scb_sscb.sscb_state != SCS_CP_ERROR
		? GCA_CONTINUE_MASK
		: GCA_FAIL_MASK;
    }
}

/*{
** Name: scs_cpinterrupt()	- Send interrupt to the FE.
**
** Description:
**      This routine sends a COPY_ERROR interrupt to the FE 
**      to inform the FE that some errors have been encountered 
**      and that the protocol should be "turned around" so 
**      that the errors can be sent.
**
** Inputs:
**      scb                             Session control block
**
** Outputs:
**      None
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      01-Sep-1987 (fred)
**          Created.
**	2-Jul-1993 (daveb)
**	    prototyped.
*/
VOID
scs_cpinterrupt(SCD_SCB *scb )
{
    DB_STATUS           status;
    DB_STATUS		error;
    GCA_AT_DATA		b;
    i4		cur_error;

    b.gca_itype = GCA_COPY_ERROR;
    scb->scb_cscb.cscb_gco.gca_association_id =
	    scb->scb_cscb.cscb_assoc_id;
    scb->scb_cscb.cscb_gco.gca_buffer = (char*)&b;
    scb->scb_cscb.cscb_gco.gca_msg_length = sizeof(b);
    scb->scb_cscb.cscb_gco.gca_message_type = GCA_NP_INTERRUPT;
    scb->scb_cscb.cscb_gco.gca_end_of_data = TRUE;
    scb->scb_cscb.cscb_gco.gca_modifiers = 0;
    /*
    **	Flush SXF audit data prior to communicating with the user
    */
    status = sc0a_flush_audit(scb, &cur_error);

    if (status != E_DB_OK)
    {
	/*
	**	Write an error
	*/
	sc0e_0_put(status = cur_error, 0);
	scd_note(E_DB_SEVERE, DB_SXF_ID);
	return ;
    }

    status = IIGCa_call(GCA_SEND,
			(GCA_PARMLIST *)&scb->scb_cscb.cscb_gco,
			GCA_NO_ASYNC_EXIT,
			(PTR)0,
			-1,
			&error);
    scb->scb_cscb.cscb_gco.gca_status = OK;	/* Don't really care about  */
						/* error		    */
    if (status)
    {
	sc0e_0_put(status = error, 0);
    }
}

/*{
** Name:	scs_cp_cpnify - Turn large objects into coupons during copy
**
** Description:
**	The name of this routine is a bit misleading.  It's called
**	for COPY FROM when there are any large-object columns in the
**	row.  LOB columns are inherently variable length, and so we
**	have to scan the row column by column to determine the end
**	of the row.  As each column is looked at, it is copied to an
**	output buffer.  If a LOB column is encountered, it is
**	couponified:  the data is saved off to someplace out-of-line,
**	and the coupon is copied to the output buffer.
**
**	If the input stream runs out before we reach the end of the
**	row, E_DB_INFO is returned, and the qe_copy state is updated
**	to reflect the current position.  Presumably the caller will
**	fetch more input data and return here to continue the scan.
**
**	Minor FIXME:  the looping could be reduced by operating on
**	copy groups similar to the way the front-end code works: a
**	group would be one LOB or N non-lob columns.  Since the
**	processing for a non-LOB column is minimal, this would only
**	be a small win.
**
** Re-entrancy:
**	Yes.
**
** Inputs:
**	scb		Session's sequencer control block
**	inbuffer	Pointer to base (not current position) of input buffer
**	insize		Total size of input buffer
**	qe_copy		QEU_COPY copy control block
**			NOTE:  "at the start of a new row" is indicated by
**			qeu_cp_cur_att == -1 (*not* zero).
**
** Outputs:
**	qe_copy state is updated to reflect new row position
**
** Returns:
**	Returns E_DB_OK if a row was completed;
**	E_DB_INFO if a row is incomplete, needs more input data;
**	E_DB_WARN/ERROR if some other (couponify) error.
**
** Exceptions: (if any)
**	None.
**
** Side Effects:
**  	None.
**
** History:
**	23-Feb-1993 (fred)
**  	    Created.
**	21-Dec-2001 (gordy)
**	    GCA_COL_ATT no longer defined with DB_DATA_VALUE: use DB_COPY macro.
**       6-Jul-2007 (hanal04) Bug 117209
**          Whilst crossing the fix for 117209 to main I noticed the same
**          buffer overrun is possible in the couponify.
**	23-Feb-2010 (kschendel) SIR 122974
**	    Return INFO, not OK, if input runs out on the first column.
**	    (Symptom was bogus rows created occasionally.)  Buffering changes
**	    in the copy front-end exposed this long-standing bug.
**	    Improve header comments.
**	25-Mar-2010 (kschendel) SIR 123485
**	    Pass qe_copy to get copy state out of sequencer sscb.
**	31-Mar-2010 (kschendel) SIR 123485
**	    Pass the qe-copy to blob fetch so it can do blob optimization.
**	    Issue manual end-of-row's to DMF as we see them.
*/
static DB_STATUS
scs_cp_cpnify(SCD_SCB *scb,
	      char    *inbuffer,
	      i4       insize,
	      QEU_COPY *qe_copy)
{
    DB_STATUS	    status = E_DB_OK;
    char    	    *ibp;
    char            *obp;
    DB_DATA_VALUE   att;
    GCA_TD_DATA	    *sqlda =
	(GCA_TD_DATA *) scb->scb_sscb.sscb_cquery.cur_rdesc.rd_tdesc;
    i4              current_get;
    i4              old_insize;
    DB_DATA_VALUE   local_dv;
    char            *att_ptr;
    union {
        ADP_PERIPHERAL	    periph;
	char		    buf[sizeof(ADP_PERIPHERAL) + 1];
    }		    pdv;

    /* If at beginning of tuple, say it correctly */
    if (qe_copy->qeu_cp_cur_att < 0)
    {
	qe_copy->qeu_cp_cur_att = 0;
	qe_copy->qeu_cp_out_offset = 0;
	DB_COPY_ATT_TO_DV( &sqlda->gca_col_desc[0], &att );
	qe_copy->qeu_cp_att_need = att.db_length;
    }
    obp = qe_copy->qeu_sptr + qe_copy->qeu_cp_out_offset;

    if (qe_copy->qeu_cp_in_offset < 0)
    {
	qe_copy->qeu_cp_in_offset = 0;
    }
    ibp = inbuffer + qe_copy->qeu_cp_in_offset;
    insize -= qe_copy->qeu_cp_in_offset;

    /*
    ** Note that this MEcopy() is necessary since the sqlda part of
    ** the actual GCA message may not be aligned.  It depends on
    ** things like the copyfile name, log name, etc.
    */

    att_ptr = (char *) sqlda->gca_col_desc;
    att_ptr += (qe_copy->qeu_cp_cur_att *
		(sizeof(sqlda->gca_col_desc[0].gca_attdbv)
		 + sizeof(sqlda->gca_col_desc[0].gca_l_attname)));
    
    DB_COPY_ATT_TO_DV( att_ptr, &att );

    while ((status == E_DB_OK) && (insize > 0))
    {
	if (att.db_length != ADE_LEN_UNKNOWN)
	{
	    current_get = min(insize, qe_copy->qeu_cp_att_need);
	    if (current_get > qe_copy->qeu_ext_length)
		current_get = qe_copy->qeu_ext_length;
	    MEcopy((PTR) ibp, current_get, (PTR) obp);
	    insize -= current_get;
	    ibp += current_get;
	    obp += current_get;
	    qe_copy->qeu_cp_in_offset += current_get;
	    qe_copy->qeu_cp_out_offset += current_get;
	    qe_copy->qeu_cp_att_need -= current_get;
	}
	else
	{
	    STRUCT_ASSIGN_MACRO(att, local_dv);
	    current_get = sizeof(ADP_PERIPHERAL);
	    if (local_dv.db_datatype < 0)
		current_get += 1;

	    /* Pick up in-progress coupon plus null indicator.  If first
	    ** time thru for this lob, make sure the DMF part of the coupon
	    ** is initially cleared.
	    */
	    MEcopy((PTR) obp, current_get, (PTR) pdv.buf);
	    if (!qe_copy->qeu_cp_in_lobj)
	    {
		MEfill(sizeof(pdv.periph.per_value.val_coupon), 0,
			(PTR) &pdv.periph.per_value.val_coupon);
	    }
	    local_dv.db_length = current_get;
	    local_dv.db_data = pdv.buf;

	    scb->scb_sscb.sscb_gcadv.gca_type = local_dv.db_datatype;
	    scb->scb_sscb.sscb_gcadv.gca_precision = local_dv.db_prec;
	    scb->scb_sscb.sscb_dmm = qe_copy->qeu_cp_in_lobj;
	    old_insize = insize;
	    status = scs_blob_fetch(scb,
				    &local_dv,
				    &insize,
				    &ibp,
				    qe_copy);
	    MEcopy((PTR) pdv.buf, current_get, (PTR) obp);
	    qe_copy->qeu_cp_in_lobj = scb->scb_sscb.sscb_dmm;
	    scb->scb_sscb.sscb_dmm = 0;
	    qe_copy->qeu_cp_in_offset += (old_insize - insize);
	    if (status == E_DB_OK)
	    {
		qe_copy->qeu_cp_in_lobj = 0;
		qe_copy->qeu_cp_att_need = 0;
		obp += current_get;
		qe_copy->qeu_cp_out_offset += current_get;
	    }
	    else
	    {
		if (DB_FAILURE_MACRO(status))
		{
		    qe_copy->qeu_cp_in_lobj = 0;
		    /* reset after faking out scs_blob_fetch() */
		}
		else
		{
		    /*
		    ** The couponify worked, but consumed all the
		    ** input.  Mark the buffer used so our caller
		    ** will get us some more...
		    */

		    qe_copy->qeu_cp_in_offset = -1;
		}
		break;
	    }
	}
	if (qe_copy->qeu_cp_att_need == 0)
	{
	    /* Finished this att, move to next, see if done.
	    ** Tell DMF (dmpe) that this row is done and it should reset
	    ** any row-by-row data, namely the logical key generator.
	    **
	    ** (and why is this done here in the sequencer, instead of in
	    ** QEF, you ask?  It's because incoming blobs are (we hope!)
	    ** being couponified directly into the copy-table's etabs, so
	    ** the couponifier has to stay in sync with the base row bounds
	    ** or coupons from different rows will clash.  We might
	    ** couponify any number of rows in the buffer before handing
	    ** the lot off to QEF.  So, the "end row" call has to be here.)
	    */
	    if (++qe_copy->qeu_cp_cur_att >= qe_copy->qeu_cp_att_count)
	    {
		DMR_CB fakedmr;		/* Just to get access ID to DMF */

		fakedmr.type = DMR_RECORD_CB;
		fakedmr.length = sizeof(DMR_CB);
		fakedmr.dmr_access_id = qe_copy->qeu_access_id;
		(void) dmf_call(DMPE_END_ROW, &fakedmr);
		qe_copy->qeu_cp_cur_att = -1;
		break;
	    }

	    att_ptr += sizeof(sqlda->gca_col_desc[0].gca_attdbv)
			    + sizeof(sqlda->gca_col_desc[0].gca_l_attname);

	    DB_COPY_ATT_TO_DV( att_ptr, &att );
	    qe_copy->qeu_cp_att_need = att.db_length;
	}
    }
    if (insize == 0)
	qe_copy->qeu_cp_in_offset = -1;

    if (qe_copy->qeu_cp_cur_att >= 0 && status == E_DB_OK)
    {
	status = E_DB_INFO;
    }
    return(status);
}

/*{
** Name:	scs_cp_redeem - Expand large objects during copy
**
** Description:
**	This routine is used during a copy into a file from a table (a
**	COPY INTO).  Basically it moves a tuple into an output message.
**	As tuple attributes are processed, if a large object is
**	encountered (in coupon form), the coupon is "redeemed",
**	replacing the inline coupon with the peripheral data itself.
**
**	As the tuple is processed, we keep track of available space in
**	the output message.  If/when the buffer fills, it is sent, and
**	if the end of the tuple hadn't been reached, E_DB_INFO is returned
**	to the caller as a signal that this row is not completely
**	sent yet.  The current state information about where we are
**	in the row is saved in the qeu_copy area.
**
** Re-entrancy:
**	Yes.
**
** Inputs:
**	scb		Session's sequencer control block
**	qe_copy		QEU_COPY copy control block
**			NOTE:  "at the start of a new row" is indicated by
**			qeu_cp_cur_att == -1 (*not* zero).
**	outbuffer	Pointer to base of output buffer
**	outsize		Total size of output buffer
**
** Outputs:
**	qe_copy sequencer state is updated to reflect new row position
**
** Returns:
**	Returns E_DB_OK if a row was completed;
**	E_DB_INFO if a row is incomplete, still more to send;
**	E_DB_WARN/ERROR if some other (redeem) error.
**
** Exceptions: (if any)
**	None.
**
** Side Effects:
**  	None.
**
** History:
**	24-Feb-1993 (fred)
**  	    Created.
**	19-nov-1996 (kch)
**	    We now calculate the number of non LO bytes in the row and
**	    decrement this total by the number of non LO bytes copied.
**	    When we come to an LO column, we call scs_redeem() with the
**	    remaining length of the copy buffer minus the number of non LO
**	    bytes still to copy, thus preventing scs_redeem() from leaving
**	    an insufficient amount of space in the buffer for the remaining
**	    non LO data. This change fixes bug 79287.
**	21-Dec-2001 (gordy)
**	    GCA_COL_ATT no longer defined with DB_DATA_VALUE: use DB_COPY macro.
**       5-Jul-2007 (hanal04) Bug 117209
**          Rewrite the fix for Bug 79287. When we are writing a non-LO
**          column we need to make sure current_get is not greater than
**          the remaining space in the output buffer.
**	25-Mar-2010 (kschendel)
**	    Move state variables to qeu-copy. Fix up the comments.
**	20-Apr-2010 (kschendel) SIR 123485
**	    Pass redeem a blob-wksp with the open base table and column
**	    number, makes DMPE's life a lot easier.
*/
static DB_STATUS
scs_cp_redeem(SCD_SCB *scb,
	      QEU_COPY *qe_copy,
	      char    *outbuffer,
	      i4       outsize)
{
    DB_STATUS	    status = E_DB_OK;
    char    	    *ibp;
    char            *obp;
    DB_DATA_VALUE   att;
    DB_BLOB_WKSP    wksp;
    i4              current_get;
    i4		    insize;
    i4              res_prec;
    i4              res_len;
    char            *att_ptr;
    GCA_TD_DATA	    *sqlda =
	(GCA_TD_DATA *) scb->scb_sscb.sscb_cquery.cur_rdesc.rd_tdesc;
    i4		    i;
    i4		    outsize_save;

    insize = qe_copy->qeu_ssize;

    /* If at beginning of tuple, say it correctly */

    if (qe_copy->qeu_cp_cur_att < 0)
    {
	qe_copy->qeu_cp_cur_att = 0;
	DB_COPY_ATT_TO_DV( &sqlda->gca_col_desc[0], &att );
	qe_copy->qeu_cp_att_need = att.db_length;
    }
    qe_copy->qeu_cp_out_offset = 0;
    obp = outbuffer;

    if (qe_copy->qeu_cp_in_offset < 0)
    {
	qe_copy->qeu_cp_in_offset = 0;
    }
    ibp = qe_copy->qeu_sptr + qe_copy->qeu_cp_in_offset;

    /*
    ** Note that this MEcopy() is necessary since the sqlda part of
    ** the actual GCA message may not be aligned.  It depends on
    ** things like the copyfile name, log name, etc.
    */

    att_ptr = (char *) sqlda->gca_col_desc;
    att_ptr += (qe_copy->qeu_cp_cur_att *
		(sizeof(sqlda->gca_col_desc[0].gca_attdbv)
		 + sizeof(sqlda->gca_col_desc[0].gca_l_attname)));
    
    DB_COPY_ATT_TO_DV( att_ptr, &att );

    while ((status == E_DB_OK) && (outsize > 0))
    {
	if (att.db_length != ADE_LEN_UNKNOWN)
	{
	    current_get = min(insize, qe_copy->qeu_cp_att_need);
            current_get = min(outsize, current_get);
	    MEcopy((PTR) ibp, current_get, (PTR) obp);
	    insize -= current_get;
	    outsize -= current_get;
	    ibp += current_get;
	    obp += current_get;
	    qe_copy->qeu_cp_in_offset += current_get;
	    qe_copy->qeu_cp_out_offset += current_get;
	    qe_copy->qeu_cp_att_need -= current_get;
	    /* Calculate number of non LO bytes remaining */
	}
	else
	{
	    current_get = sizeof(ADP_PERIPHERAL);
	    if (att.db_datatype < 0)
		current_get += 1;

	    /*
	    ** Bug 79287 - save outsize and call scs_redeem() with the
	    ** number of bytes left in the copy buffer after we have
	    ** accounted for the remaining non LO bytes.
	    */
	    wksp.access_id = qe_copy->qeu_access_id;
	    wksp.base_attid = qe_copy->qeu_cp_cur_att+1;
	    wksp.flags = BLOBWKSP_ACCESSID | BLOBWKSP_ATTID;
	    status = scs_redeem(scb,
				&wksp,
				qe_copy->qeu_cp_in_lobj,
				att.db_datatype,
				att.db_prec,
				outsize,
				(PTR) ibp,
				&res_prec,
				&res_len,
				(PTR) obp);

	    qe_copy->qeu_cp_out_offset += res_len;
	    if (status == E_DB_OK)
	    {
		qe_copy->qeu_cp_in_lobj = 0;
		qe_copy->qeu_cp_att_need = 0;

		ibp += current_get;
		qe_copy->qeu_cp_in_offset += current_get;
		insize -= current_get;

		outsize -= res_len;
		obp += res_len;
	    }
	    else
	    {
		qe_copy->qeu_cp_in_lobj = 1;
		outsize = 0;
		if (DB_FAILURE_MACRO(status))
		{
		    qe_copy->qeu_cp_in_lobj = 0;
		    /* No longer in object since we'll bail out now */
		}
		break;
	    }
	}
	if (qe_copy->qeu_cp_att_need == 0)
	{
	    /* Finished this att, move to next, see if done */
	    if (++qe_copy->qeu_cp_cur_att >= qe_copy->qeu_cp_att_count)
	    {
		qe_copy->qeu_cp_cur_att = -1;
		qe_copy->qeu_cp_in_offset = -1;
		status = E_DB_OK;
		break;
	    }
	    /*
	    ** Note that this MEcopy() is necessary since the sqlda part of
	    ** the actual GCA message may not be aligned.  It depends on
	    ** things like the copyfile name, log name, etc.
	    */

	    att_ptr += sizeof(sqlda->gca_col_desc[0].gca_attdbv)
			    + sizeof(sqlda->gca_col_desc[0].gca_l_attname);

	    DB_COPY_ATT_TO_DV( att_ptr, &att );
	    qe_copy->qeu_cp_att_need = att.db_length;
	}
    }

    if (qe_copy->qeu_cp_cur_att >= 0 && status == E_DB_OK)
    {
	status = E_DB_INFO;
    }

    return(status);
}

/*
** {
** Name:	scs_redeem - Expand large objects
**
** Description:
**	This routine is a utility used during a copy to expand
**      It is used both to expand coupons when doing a copy 
**      into a file from a table (a COPY INTO) as well as during copy
**      map processing to expand large object null indicator values
**      (copy...(... colx = long varchar(0) with null ('faceplant'))).
**      
**      This routine simply sets up to call adu_redeem() to do all the
**      work, returning its status without comment.  It is up to the
**      caller of this routine to correctly interpret and log said
**      status if necessary.
**
** Re-entrancy:
**	Yes.
**
** Inputs:
**	<param name>	<input value description>
**
** Outputs:
**	<param name>	<output value description>
**
** Returns:
**	<function return values>
**
** Exceptions: (if any)
**	None.
**
** Side Effects:
**  	None.
**
** History:
**	23-Jun-1993 (fred)
**  	    Created.
**      10-Nov-1993 (fred)
**          Added parameter to scs_lowksp_alloc() routine to indicate
**          that the call is from copy processing.
**	11-May-2004 (schka24)
**	    Remove same.
**	20-Apr-2010 (kschendel) SIR 123485
**	    Pass a db-blob-wksp from caller so that DMPE has an easier
**	    time figuring out what is going on.
*/
static DB_STATUS
scs_redeem(SCD_SCB  	*scb,
	   DB_BLOB_WKSP *wksp,
	   i4           continuation,
	   DB_DT_ID     datatype,
	   i4           prec,
	   i4      out_size,
	   PTR          cpn_ptr,
	   i4           *res_prec,
	   i4           *res_len,
	   PTR          out_area)
{
    DB_STATUS       status;
    ADP_LO_WKSP	    *lowksp;
    DB_DATA_VALUE   coupon_dv;
    DB_DATA_VALUE   result_dv;
    DB_DATA_VALUE   wksp_dv;
    union {
        ADP_PERIPHERAL	    periph;
	char		    buf[sizeof(ADP_PERIPHERAL) + 1];
    }		    pdv;

    for (;;)
    {
	coupon_dv.db_datatype = datatype;
	coupon_dv.db_prec = prec;
	coupon_dv.db_length = sizeof(ADP_PERIPHERAL);
	
	if (coupon_dv.db_datatype < 0)
	    coupon_dv.db_length += 1;
	
	STRUCT_ASSIGN_MACRO(coupon_dv, result_dv);
	
	coupon_dv.db_data = (char *) pdv.buf;
	
	/*
	 ** Must correctly set the length here, since in the tuple
	 ** descriptor (from which we are constructing the DBDV),
	 ** the length is specified as unknown.
	 */
	
	MEcopy((PTR) cpn_ptr, coupon_dv.db_length, (PTR) pdv.buf);
	result_dv.db_data = out_area;
	result_dv.db_length = out_size;

	if (result_dv.db_length > MAXI2)
	{
	    result_dv.db_length = MAXI2;
	}
	
	status = scs_lowksp_alloc(scb, (PTR *) &lowksp);
	if (status)
	    break;
	
	wksp_dv.db_data = (char *) lowksp;
	wksp_dv.db_length = sizeof(*lowksp);

	if (!continuation)
	{
	    /* If caller passed workspace info, pass it along. */
	    ((DB_BLOB_WKSP *) lowksp->adw_fip.fip_pop_cb.pop_info)->flags = 0;
	    lowksp->adw_fip.fip_pop_cb.pop_temporary = ADP_POP_TEMPORARY;
	    if (wksp != NULL)
		*((DB_BLOB_WKSP *) lowksp->adw_fip.fip_pop_cb.pop_info) = *wksp;
	}
	
	status = adu_redeem(scb->scb_sscb.sscb_adscb,
			    &result_dv,
			    &coupon_dv,
			    &wksp_dv,
			    continuation);
	MEcopy((PTR) pdv.buf, coupon_dv.db_length, (PTR) cpn_ptr);
	*res_len = result_dv.db_length;
	*res_prec = result_dv.db_prec;
	break;
    }
    return(status);
}
