/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <pc.h>
#include    <di.h>
#include    <lo.h>
#include    <tm.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <cs.h>
#include    <cv.h>
#include    <me.h>
#include    <nm.h>
#include    <st.h>
#include    <tr.h>
#include    <scf.h>
#include    <qsf.h>
#include    <adf.h>
#include    <qefrcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <tpfcat.h>
#include    <tpfqry.h>
#include    <tpfddb.h>
#include    <tpfproto.h>


/**
**
** Name: TP2DBGS.C - TPF's debugging utilities
** 
** Description: 
**	tp2_d0_crash	- routine to provide control to crash session/server
**	tp2_d1_suspend	- routine to provide control to suspend session
**
**  History:    $Log-for RCS$
**      13-oct-90 (carl)
**          split off tp2util.c
**	19-jan-92 (fpang)
**	    Included qefrcb.h, scf.h and adf.h
**	22-Jul-1993 (fpang)
**	    Include missing header files.
**	11-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in tp2_d0_crash(),
**	    tp2_d1_suspend().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

GLOBALREF   char            *IITP_00_tpf_p;
 

/*{
** Name: tp2_d0_crash - routine to provide control to crash session (server)
** 
**  Description:    
**	Provide crash point to kill the session (server).
**
** Inputs:
**	v_rcb_p->		TPR_CB
**	
** Outputs:
**	v_rcb_p->
**	    tpr_error
**		.err_code	E_TP0022_SECURE_FAILED or
**				E_TP0023_COMMIT_FAILED
**	Returns:
**	    E_DB_FATAL		to simulate fatal error to shut down session
**	    
**	Exceptions:
**	    None
**
** Side Effects:
**	    
**
** History:
**      19-dec-89 (carl)
**          created
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	11-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
*/


DB_STATUS
tp2_d0_crash(
	TPR_CB		*v_rcb_p)
{
    DB_STATUS	status = E_DB_FATAL;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_rcb_p->tpr_session;
    TPD_DX_CB   *dxcb_p = & sscb_p->tss_dx_cb;


    TRdisplay("%s %p: Crashing the server ",
        IITP_00_tpf_p,
        sscb_p);

    switch (sscb_p->tss_4_dbg_2pc)
    {
    case TSS_60_CRASH_BEF_SECURE:
	TRdisplay("%s %p:   before SECURE state of DX %x %x is logged...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	v_rcb_p->tpr_error.err_code = E_TP0022_SECURE_FAILED;
	break;

    case TSS_61_CRASH_IN_SECURE:
	TRdisplay("%s %p:   after SECURE state of DX %x %x is logged...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	v_rcb_p->tpr_error.err_code = E_TP0022_SECURE_FAILED;
	break;

    case TSS_62_CRASH_AFT_SECURE:
	TRdisplay("%s %p:   after SECURE state of DX %x %x is committed...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	v_rcb_p->tpr_error.err_code = E_TP0022_SECURE_FAILED;
	break;

    case TSS_63_CRASH_IN_POLLING:
	TRdisplay("%s %p:   while polling last site of DX %x %x...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	v_rcb_p->tpr_error.err_code = E_TP0022_SECURE_FAILED;
	break;

    case TSS_70_CRASH_BEF_COMMIT:
	TRdisplay("%s %p:   before COMMIT state of %d %d is logged...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	v_rcb_p->tpr_error.err_code = E_TP0023_COMMIT_FAILED;
	break;

    case TSS_71_CRASH_IN_COMMIT:
	TRdisplay("%s %p:   after COMMIT state of DX %x %x is logged...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	v_rcb_p->tpr_error.err_code = E_TP0023_COMMIT_FAILED;
	break;

    case TSS_72_CRASH_AFT_COMMIT:
	TRdisplay("%s %p:   after COMMIT state of DX %x %x is committed...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	v_rcb_p->tpr_error.err_code = E_TP0023_COMMIT_FAILED;
	break;

    case TSS_73_CRASH_LX_COMMIT:
	TRdisplay("%s %p:   while committing LXs of DX %x %x...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	v_rcb_p->tpr_error.err_code = E_TP0023_COMMIT_FAILED;
	break;

    case TSS_80_CRASH_BEF_ABORT:
	TRdisplay("%s %p:   before ABORT state of %d %d is logged...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	v_rcb_p->tpr_error.err_code = E_TP0023_COMMIT_FAILED;
	break;

    case TSS_81_CRASH_IN_ABORT:
	TRdisplay("%s %p:   after ABORT state of DX %x %x is logged...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	v_rcb_p->tpr_error.err_code = E_TP0023_COMMIT_FAILED;
	break;

    case TSS_82_CRASH_AFT_ABORT:
	TRdisplay("%s %p:   after ABORT state of DX %x %x is committed...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	v_rcb_p->tpr_error.err_code = E_TP0023_COMMIT_FAILED;
	break;

    case TSS_83_CRASH_LX_ABORT:
	TRdisplay("%s %p:   while aborting LXs of DX %x %x...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	v_rcb_p->tpr_error.err_code = E_TP0023_COMMIT_FAILED;
	break;

    case TSS_90_CRASH_STATE_LOOP:
	TRdisplay("%s %p:   before executing loop to log and commit state of\n",
		IITP_00_tpf_p,
		sscb_p);
	TRdisplay("%s %p:   of DX %x %x...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	sscb_p->tss_4_dbg_2pc = TSS_90_CRASH_STATE_LOOP;
	break;	    

    case TSS_91_CRASH_DX_DEL_LOOP:
	TRdisplay("%s %p:   before executing loop to delete DX %x %x...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	sscb_p->tss_4_dbg_2pc = TSS_91_CRASH_DX_DEL_LOOP;
	break;	    

    default:
	TRdisplay("%s %p:   in unknown state of DX %x %x...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	if (v_rcb_p->tpr_error.err_code == 0)
	    v_rcb_p->tpr_error.err_code = E_TP0020_INTERNAL;
	break;
    }
    return(status);
}


/*{
** Name: tp2_d1_suspend - routine to provide control to suspend session 
** 
**  Description:    
**	Provide control points to suspend the session to allow the CDBMS or
**	an LDBMS server to be brought down (and later up again) to test the 
**	various 2PC protocols.
**
** Inputs:
**	v_rcb_p->		TPR_CB
**	    .tpr_session	TPR ssession context
**		.tss_dx_cb	DX CB
**		    .tss_4_dbg_2pc
**				suspend point	
**		    .tss_5_dbg_timer	
**				timer in seconds
** Outputs:
**	none
**
**	Returns:
**	    nothing
**	    
**	Exceptions:
**	    None
**
** Side Effects:
**	    
**
** History:
**      13-may-90 (carl)
**          created
*/


VOID
tp2_d1_suspend(
	TPR_CB		*v_rcb_p)
{
    DB_STATUS	ignore;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_rcb_p->tpr_session;
    TPD_DX_CB   *dxcb_p = & sscb_p->tss_dx_cb;
    i4		cs_mask = CS_TIMEOUT_MASK;


    TRdisplay("%s %p: Suspending session for %d seconds\n", 
	IITP_00_tpf_p,
	sscb_p,
	sscb_p->tss_5_dbg_timer);

    switch (sscb_p->tss_4_dbg_2pc)
    {
    case TSS_65_SUSPEND_BEF_SECURE:
	TRdisplay("%s %p:    before SECURE state of DX %x %x is logged...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	break;

    case TSS_66_SUSPEND_IN_SECURE:
	TRdisplay("%s %p:    after SECURE state of DX %x %x is logged...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	break;

    case TSS_67_SUSPEND_AFT_SECURE:
	TRdisplay("%s %p:    after SECURE state of DX %x %x is committed...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	break;

    case TSS_68_SUSPEND_IN_POLLING:
	TRdisplay("%s %p:    before polling the third/second last site of\n",
		IITP_00_tpf_p,
		sscb_p);
	TRdisplay("%s %p:    DX %x %x...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	break;

    case TSS_75_SUSPEND_BEF_COMMIT:
	TRdisplay("%s %p:    before COMMIT state of %d %d is logged...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	break;

    case TSS_76_SUSPEND_IN_COMMIT:
	TRdisplay("%s %p:    after COMMIT state of DX %x %x is logged...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	break;

    case TSS_77_SUSPEND_AFT_COMMIT:
	TRdisplay("%s %p:    after COMMIT state of DX %x %x is committed...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	break;

    case TSS_78_SUSPEND_LX_COMMIT:
	TRdisplay("%s %p:    before committing the third/second last site of\n",
		IITP_00_tpf_p,
		sscb_p);
	TRdisplay("%s %p:    DX %x %x...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	break;

    case TSS_85_SUSPEND_BEF_ABORT:
	TRdisplay("%s %p:    before ABORT state of %d %d is logged...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	break;

    case TSS_86_SUSPEND_IN_ABORT:
	TRdisplay("%s %p:    after ABORT state of DX %x %x is logged...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	break;

    case TSS_87_SUSPEND_AFT_ABORT:
	TRdisplay("%s %p:    after ABORT state of DX %x %x is committed...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	break;

    case TSS_88_SUSPEND_LX_ABORT:
	TRdisplay("%s %p:    before aborting the third/second last site of\n",
		IITP_00_tpf_p,
		sscb_p);
	TRdisplay("%s %p:    DX %x %x...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	break;

    case TSS_95_SUSPEND_STATE_LOOP:
	TRdisplay("%s %p:    for DX %x %x before executing loop\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	TRdisplay("%s %p:    to log and commit state.\n",
		IITP_00_tpf_p,
		sscb_p);
	break;	    

    case TSS_96_SUSPEND_DX_DEL_LOOP:
	TRdisplay("%s %p:    for DX %x %x before executing loop\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	TRdisplay("%s %p:    to delete the DX\n",
		IITP_00_tpf_p,
		sscb_p);
	break;	    

    default:
	TRdisplay("%s %p:    in unknown state of DX %x %x...\n",
		IITP_00_tpf_p,
		sscb_p,
		dxcb_p->tdx_id.dd_dx_id1, 
		dxcb_p->tdx_id.dd_dx_id2);
	break;
    }
    ignore = CSsuspend(cs_mask, sscb_p->tss_5_dbg_timer, (PTR) NULL);

    TRdisplay("%s %p: Resumed after suspending session for %d seconds\n", 
		IITP_00_tpf_p,
		sscb_p,
		sscb_p->tss_5_dbg_timer);
}
