/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <pc.h>
#include    <adf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <cs.h>
#include    <cv.h>
#include    <me.h>
#include    <nm.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>
#include    <scf.h>
#include    <tpf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rqf.h>
#include    <tpfcat.h>
#include    <tpfddb.h>
#include    <tpfqry.h>
#include    <tpfproto.h>

/**
**
**  Name: TPFUTIL.C - Utility functions for the entire TPF
**
**  Description:
**      The routines defined in this file provide auxiliary facility functions.
**
**	tpf_u0_printf	    - send string to FE
**	tpf_u1_p_sema4	    - call CSp_semaphore
**	tpf_u2_v_sema4	    - call CSv_semaphore
**	tpf_u3_msg_to_ldb   - send an LDB a commit/abort message
**
**  History:    $Log-for RCS$
**      24-jan-90 (carl)
**          created
**	24-jun-90 (carl)
**	    added tpf_u3_msg_to_ldb   
**	19-jan-92 (fpang)
**	    Included qefrcb.h, scf.h and adf.h
**	22-Jul-1993 (fpang)
**	    Include missing header files.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Include tpfqry.h, tpfproto.h to get prototypes.
**/

GLOBALREF   char                *IITP_09_3dots_p;

GLOBALREF   char                *IITP_51_tpf_call_tab[];

GLOBALREF   char                *IITP_52_dx_state_tab[];


/*{
** Name: tpf_u0_printf - send string to FE
**
** Description:
**      This routine performs the printf function by returning the info to 
**      be displayed through SCC_TRACE. 
**
** Inputs:
**	v_tpf_p -
**	    The current request control block to TPF. This is here for future
**	    expansion,
**	cbufsize -
**	    The formatted buffer size.
**	cbuf -
**	    The formatted buffer.
**
** Outputs:
**	none
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    The string in the formatted buffer goes to the FE.
**
** History:
**      05-oct-90 (carl)
**          adapted
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	08-sep-93 (swm)
**	    Cast v_tpr_p->tpr_19_qef_sess to SCF_SESSION rather than
**	    i4 since SCF_SESSION is now no longer defined as i4.
**	08-oct-93 (swm)
**	    Bug #56448
**	    Made tpf_u0_tprintf portable by eliminating p1 .. p2 PTR paramters,
**	    instead requiring caller to pass a string already formatted by
**	    STprintf. This is necessary because STprintf is a varargs function
**	    whose parameters can vary in size; the current code breaks on
**	    any platform where the STprintf size types are not the same (these
**	    are usually integers and pointers).
**	    This approach has been taken because varargs functions outside CL
**	    have been outlawed and using STprintf in client code is less
**	    cumbersome than trying to emulate varargs with extra size or
**	    struct parameters.
*/


VOID
tpf_u0_tprintf(
	TPR_CB		*v_tpr_p,
	i4		cbufsize,
	char		*cbuf)
{
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) & v_tpr_p->tpr_session;
    SCF_CB	scf_cb;
    DB_STATUS   scf_status;


    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_QEF_ID;
    scf_cb.scf_len_union.scf_blength = STnlength(cbufsize, cbuf);
    scf_cb.scf_ptr_union.scf_buffer = cbuf;
    scf_cb.scf_session = (SCF_SESSION) v_tpr_p->tpr_19_qef_sess;
					/* borrow QEF's session id for now */
    scf_status = scf_call(SCC_TRACE, &scf_cb);
    if (scf_status != E_DB_OK)
    {
	TRdisplay("SCF error displaying trace data to user.\n");
	TRdisplay("TPF trace message is:\n%s", cbuf);
    }
}


/*{
** Name: tpf_u1_p_sema4 - call CSp_semaphore
**
** Description:
**      This routine provides a central point for calling CSp_semaphore.
**
** Inputs:
**	i1_mode			TPD_0SEM_SHARE or TPD_1SEM_EXCLUSIVE
**	i2_sema4_p		ptr to CS_SEMAPHORE
**  
** Outputs:
**	none
**
**	Returns:
**	    void
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-jan-90 (carl)
**          created
*/

VOID
tpf_u1_p_sema4(
	i4		 i1_mode,
	CS_SEMAPHORE	*i2_sema4_p)
{

    CSp_semaphore(i1_mode, i2_sema4_p);
}


/*{
** Name: tpf_u2_v_sema4 - call CSv_semaphore
**
** Description:
**      This routine provides a central point for calling CSv_semaphore.
**
** Inputs:
**	i1_sema4_p		ptr to CS_SEMAPHORE
**  
** Outputs:
**	none
**
**	Returns:
**	    void
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-jan-90 (carl)
**          created
*/

VOID
tpf_u2_v_sema4(
	CS_SEMAPHORE	*i1_sema4_p)
{

    CSv_semaphore(i1_sema4_p);
}


/*{
** Name: tpf_u3_msg_to_ldb - Send an LDB a commit/abort message.
**
** Description:
**
** Inputs:
**	    i1_ldb_p			ptr to DD_LDB_DESC for LDB
**	    i2_commit_b			TRUE if committing, FALSE if aborting
**	    v_rcb_p			TPF_RCB
** Outputs:
**	none
**          Returns:
**		E_DB_OK                 
**		E_DB_ERROR              caller error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	23-jun-90 (carl)
**          created
*/


DB_STATUS
tpf_u3_msg_to_ldb(
	DD_LDB_DESC	*i1_ldb_p,
	bool		i2_commit_b,
	TPR_CB		*v_rcb_p)
{
    DB_STATUS	    status;
    TPD_SS_CB	    *sscb_p = (TPD_SS_CB *) & v_rcb_p->tpr_session;
    i4		    rqf_op;
    RQR_CB	    rq_cb,
		    *rq_p = & rq_cb;


    MEfill(sizeof(RQR_CB), 0, (PTR) rq_p);

    v_rcb_p->tpr_error.err_code = E_DB_OK;

    rq_p->rqr_session = sscb_p->tss_rqf;  /* RQF session cb ptr */
    rq_p->rqr_timeout = 0;
    rq_p->rqr_1_site = i1_ldb_p;
    rq_p->rqr_q_language = DB_SQL;

    rq_p->rqr_msg.dd_p1_len = 0;
    rq_p->rqr_msg.dd_p2_pkt_p = (char *) NULL;
    rq_p->rqr_msg.dd_p3_nxt_p = (DD_PACKET *) NULL;

    if (i2_commit_b)
	rqf_op = RQR_COMMIT;
    else
	rqf_op = RQR_ABORT;
    status = tpd_u4_rqf_call(rqf_op, rq_p, v_rcb_p);

    return(status);
}

