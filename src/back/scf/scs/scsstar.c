/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <ex.h>
#include    <st.h>
#include    <er.h>
#include    <cv.h>
#include    <cs.h>
#include    <tm.h>
#include    <me.h>
#include    <pc.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <gca.h>
#include    <scf.h>
#include    <qsf.h>
#include    <dmf.h>	
#include    <qefrcb.h>
#include    <qefmain.h>
#include    <qefcb.h>
#include    <psfparse.h>
#include    <sqlstate.h>

/* added for scs.h prototypes, ugh! */
#include <dudbms.h>
#include <dmccb.h>
#include <dmrcb.h>
#include <copy.h>
#include <qefqeu.h>
#include <qefcopy.h>

#include    <sc.h>
#include    <sc0e.h>
#include    <sc0m.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>

#include    <scfcontrol.h>

/**
**
**  Name: SCSSTAR.C - STAR routines
**
**  Description:
**      This file contains functions used exclusively by STAR.
**
**	    scs_dctdesc - Read and align downstream tuple descriptor.
**	    scs_dccpmap - Read and align downstream copymap descriptor.
**	    scs_dcinput - Fetch client message for direct connect.
**	    scs_dcsend  - Queue a message to be sent to the client.
**	    scs_dcmsg   - Detect a user's 'direct disconnect' message. 
**	    scs_dcinterrupt - Interrupt a downstream LDB.
**	    scs_dccont  - Interface to qef_call(QED_C2_CONT).
**	    scs_dcendcpy- Perform distributed copy termination.
**	    scs_dccnct  - Sequence 'DIRECT CONNECT'
**	    scs_dccopy  - Sequence distributed 'COPY FROM?INTO'
**	    scs_dcexec  - Sequence 'DIRECT DESCUTE IMMEDIATE'
**
**  History:    $Log-for RCS$
**	03-apr-90 (carl)
**	    created
**	04-apr-90 (carl)
**	    moved scs_dconnect here from scsqncr.c
**	13-aug-91 (fpang)
**	    1. scs_dconnect(), support for GCA_SECURE, GCA_INVPROC, GCA_CREPROC,
**	       GCA_DRPPROC, GCA_ERROR, GCA_TRACE.
**	    2. scs_sdctrace() written to support trace messages in 
**	       direct connect.
**	25-jun-1992 (fpang)
**	    Major rewrite of direct connect, direct execute immediate, and
**	    distributed copy. As part of Sybil merge, PSQ_DIRCON, PSQ_DIREXEC,
**	    and PSQ_DDCOPY processing has been rewritten to follow existing
**	    sequencer processing. The most significant change is that client
**	    communication will use existing SCF services instead of separate
**	    Star routines. The second major change is that the sequencing
**	    of direct connect, direct execute immediate, and distributed copy,
**	    will not be done in line within the sequencer, but in sub-routines
**	    instead. 
**	04-sep-1992 (fpang)
**	    Fixed a variaty of bugs discovered during distributed testing.
**	       - scs_dctdesc() - local variable 'it' was uninitialized.
**	       - scs_dccpmap() - sizeof calculations used the wrong fields in
**			         some cases.
**	       - scs_dccnct()  - fixed interrupt handling.
**	       - scs_dccopy()  - support GCA1_C_FROM/INTO.
**	18-Sep-1992 (fpang)
**	    Modified scs_dccpmap() to convert copy maps if LDB and FE are at 
**	    different protocols.
**	01-oct-1992 (fpang)
**	    In scs_dcexec(), set qed_c5_tran_ok_b because DEI is ok in a 
**	    transaction. Also fixed DEI error handling.
**	27-oct-1992 (fpang)
**	    Support GCA_NEW_EFF_USER_MASK after direct connect and direct
**	    disconnect. Also fixed copy termination. Also don't set
**	    qed_c5_tran_ok_b tp FALSE when reading from LDB.
**	18-jan-1993 (fpang)
**	    1. New function scs_dcxproc() for dynamic execute procedure.
**	    2. Conversion issue. Convert LDB error from local error, generic
**	       error, or sqlstate, to FE protocol, if protocols are different.
**	    3. Fixed some cases where scd_note() was missing parameters.
**	    4. Changes also to process protocol 60, retproc protocol.
**	21-jan-1993 (fpang)
**	    Interop issues WRT gca_retproc when FE and LDB are at different
**	    protocols.
**	12-Mar-1993 (daveb)
**	    Add include <st.h>
**	31-Mar-1993 (fpang)
**	    Various portability modifications.
**	    1. Cast MEcopy() and MEfill() 'nbytes' parameter to u_i2.
**	    2. In scs_dccpmap() change types of nleft and save from i4 
**	       to u_i2, and type of flsize from i4  to u_i2.
**	    3. When comparing between types explicitly cast operand.
**	2-Jul-1993 (daveb)
**	    prototyped, removed func externs from headers.
**	7-jul-93 (rickh)
**	    Prototyped qef_call invocations.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	22-jul-1993 (fpang)
**	    In scs_dccnct(), set the GCA_FLUSH_QRY_IDS_MASK bit in the
**	    response block after direct connect/disconnect to force the
**	    FE to drop procedure/query /cursor ids.
**	08-sep-93 (swm)
**	    Cast completion id. parameter to IIGCa_call() to PTR as it is
**	    no longer a i4, to accomodate pointer completion ids.
**	13-sep-93 (swm)
**	    Include cv.h to pickup CV_HEX_PTR_SIZE define which is now
**	    needed by scs.h.
**	09-Sep-1993 (fpang)
**	    In scs_dcinterrupt() and scs_dcendcpy(), if 
**	    scb->scb_sscb.sscb_dccb is NULL, DC control block has already
**	    been released, so don't continue. This usually happens if an
**	    interrupt occurs after COPY/DIRCON/DEI has already completed.
**	15-Sep-1993 (fpang)
**	    In scs_dccpmap(), when calculating size of new copy map, take
**	    into account any partial outstanding reads, to prevent buffer
**	    overflow.
**      05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**          This results in owner within sc0m_allocate having to be PTR.
**          In addition calls to sc0m_allocate within this module had the
**          3rd and 4th parameters transposed, this has been remedied.
**	01-Dec-1993 (fpang)
**	    In scs_dctdesc() and scs_dccpmap(), set 
**	    scs_dccb.dc_dcblk.sdc_td2_ldesc to scs_dccb.dc_tdesc because
**	    RQF needs to set the gca_descriptor field in the GCA_SD_PARMS.
**	    Fixes B57191, Star copy causes commsvr to crash.
**	18-apr-1994 (fpang)
**	    Modified scs_dcendcpy() to call QEF with QED_A_COPY if interrupted,
**	    and QED_E_COPY otherwise.
**	    Fixes B62553, Star Copy. Conversion errors cause subsequent
**	    SQL statements to fail or act eratic.
**      15-nov-1994 (rudtr01)
**          modified scs_dccopy() to properly set the name, and node
**          of the LDB before attempting the COPY TABLE
**	14-jan-1999 (nanpr01)
**	    Changed the sc0m_deallocate to pass pointer to a pointer.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	30-May-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
**	30-Mar-2010 (kschendel) SIR 123485
**	    Re-type some ptr's as the proper struct pointer.
**/

GLOBALREF SC_MAIN_CB *Sc_main_cb; /* server control block */


/*{
** Name: ldb_fetch - Fetch bytes from a bytestream
**
** Description:
**	This static routine copies bytes from one buffer to a second bufferr.
**	If the requested number of bytes is not available, copy whatever is
**	available, set 'save' to the amount still needed, and 'saveptr' to
**	the place to put the bytes. The subsequent call will take copy the
**	bytes owed.
**	This routine is a customized rountine to be used by scs_dctdesc() and 
**	scs_dccpmap() only.
**
** Inputs:
**	from		- buffer to copy from
**	size		- how many to copy
**	nleft		- how much is left in the source
**	save		- how many are owed
**	saveptr		- where to put owed bytes
**	dest		- buffer to copy to
**
** Outputs:
**	from		- Incremented past copied bytes
**	nleft		- how many left over
**	save		- if not enough to copy, how many are owed.
**	saveptr		- where to put owed bytes
**	dest		- what got copied
**
**    Returns:
**	DB_STATUS
**	  - E_DB_ERRROR - Made partial copy.
**	  - E_DB_OK	- Copied amount requested.
**    Exceptions:
**	None
** Side Effects:
**	None
**
** History:
**	20-Jan-1991 (fpang)
**	    Created for SYBIL.
[@history_template@]...
*/
DB_STATUS
ldb_fetch(char **from,		/* Source */
	  u_i2 size,		/* How many requested */
	  u_i2 *nleft,		/* How many are left in source */
	  u_i2 *save,		/* How many are owed from prevous call */
	  char **saveptr,	/* Where to put owed bytes */
	  char *dest )		/* Destination */
{
    if (*save != 0)
    {
	/* We owe some bytes */
	size = *save; *save = 0;  dest = (*saveptr);
    }
    if (*nleft < size)
    {
	/* Not enough to copy, just copy what's there.
	** Set save to what we owe
	** and saveptr to where they should go.
	*/
	(*save) = size - (*nleft);
	if (dest != NULL)
	{
	    MEcopy(*from, *nleft, dest);
	    (*saveptr) = (char *)(dest + (*nleft));
	}
	(*nleft) = 0;
	return (E_DB_ERROR);
    }
    else
    {
	/* Enough to copy, just copy. */
	if (dest != NULL)
	    MEcopy(*from, size, dest);
	(*from) += size;
	(*nleft) -= size;
    }
    return (E_DB_OK);
}

/*{
** Name: scs_dctdesc - Read, convert and queue an LDB tuple descriptor.
**
** Description:
**	This routine reads a possibly fragmented downstream LDB GCA_TDESCRs,
**	and constructs a contigious tuple descriptor, suitable to be sent to the
**	client. The constructed tuple descriptor is queued to be sent to the
**	client.
**
** Inputs:
**	scb		- session control block for this session
**
** Outputs:
**
**	scb->scb_sscb.sscb_dccb->dc_tdescmsg
**			- Allocated SCC_GCMSG containing the tuple desc.
**	scb->scb_sscb.sscb_dccb->dc_tdesc
**			- Pointer to tuple descriptor.
**
**    Returns:
**	DB_STATUS	
**
**    Exceptions:
**	none
**
** Side Effects:
**	Will allocate memory.
**
** History:
**	20-Jan-1991 (fpang)
**	    Created for SYBIL.
**	04-Sep-1992 (fpang)
**	    Initialized local variable 'it'. 
**	2-Jul-1993 (daveb)
**	    prototyped.  Use proper PTR for allocation.
**	01-Dec-1993 (fpang)
**	    Set scs_dccb.dc_dcblk.sdc_td2_ldesc to scs_dccb.dc_tdesc because
**	    RQF needs to set the gca_descriptor field in the GCA_SD_PARMS.
**	    Fixes B57191, Star copy causes commsvr to crash.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
[@history_template@]...
*/
DB_STATUS
scs_dctdesc(SCD_SCB *scb )
{
    u_i2	 size = 0;
    PTR		 darea = NULL;
    SCC_GCMSG	 *msg = NULL;
    GCA_RV_PARMS *rv;
    SCC_SDC_CB	 **dc;
    DB_STATUS    status;
    DB_ERROR     error;
    QEF_RCB      *qefrcb = scb->scb_sscb.sscb_qeccb;
    GCA_TD_DATA	 *td;
    i4	 ncols;
    PTR		block;

    dc = (SCC_SDC_CB **)
	            &qefrcb->qef_r3_ddb_req.qer_d6_conn_info.qed_c4_exchange_p;
    rv = (GCA_RV_PARMS *)scb->scb_sscb.sscb_dccb->dc_dcblk.sdc_rv;    
    /* Get offset to gca_l_col_desc */
    size = sizeof(td->gca_tsize)  
	 + sizeof(td->gca_result_modifier)
	 + sizeof(td->gca_id_tdscr);

    /* Extract gca_l_col_desc */
    MEcopy((PTR)(rv->gca_data_area + size), sizeof(td->gca_l_col_desc), 
	   &ncols);

    /* Compute maximum possible size of tuple descriptor using widest
    ** possible column name.
    */
    size = sizeof(GCA_TD_DATA)
	 - sizeof(GCA_COL_ATT)
	 + (ncols * (DB_GW1_MAXNAME + sizeof(GCA_COL_ATT)));

    /* Allocate the tuple descriptor */
    status = sc0m_allocate(SCU_MZERO_MASK,
			   (i4)(sizeof(SCC_GCMSG) +
			   size),
			   DB_SCF_ID, (PTR)SCS_MEM, SCG_TAG, &block);
    msg = (SCC_GCMSG *)block;

    if (DB_FAILURE_MACRO(status))
    {
	sc0e_put(status, 0, 0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0);
	sc0e_uput(status, 0, 0,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0);
	scd_note(E_DB_SEVERE, DB_SCF_ID);
	scb->scb_sscb.sscb_dccb->dc_tdescmsg = NULL;
	scb->scb_sscb.sscb_dccb->dc_tdesc = NULL;
	return (E_DB_ERROR);
    }

    msg->scg_marea = darea = ((char*)msg + sizeof(SCC_GCMSG));
	
    size = 0;

    do
    {
	/* Copy in the descriptor, one message at a time */
	rv = (GCA_RV_PARMS *)(*dc)->sdc_rv;
	MEcopy(rv->gca_data_area, rv->gca_d_length, (PTR)(darea + size));
	size += rv->gca_d_length;
	if (rv->gca_end_of_data == TRUE)
	    break;
        status = scs_dccont(scb, SDC_READ, 0, NULL);
    } while ((DB_SUCCESS_MACRO(status)) &&
	     (scb->scb_sscb.sscb_interrupt == 0));

    if ((DB_FAILURE_MACRO(status)) ||
	(scb->scb_sscb.sscb_interrupt != 0))
    {
	if (DB_FAILURE_MACRO(status))
	{
	    sc0e_put(status, 0, 0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
	    sc0e_uput(status, 0, 0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0);
	    scd_note(E_DB_SEVERE, DB_SCF_ID);
	}

	status = sc0m_deallocate(0, (PTR *)&msg);

	scb->scb_sscb.sscb_dccb->dc_tdescmsg = NULL;
	scb->scb_sscb.sscb_dccb->dc_tdesc = NULL;
	return (E_DB_ERROR);
    }

    /* Queue it to be sent to FE */
    msg->scg_mask = SCG_NODEALLOC_MASK;
    msg->scg_mdescriptor = NULL;
    msg->scg_msize = size;
    msg->scg_mtype = GCA_TDESCR;
    msg->scg_next = (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext;
    msg->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
    scb->scb_cscb.cscb_mnext.scg_prev->scg_next = msg;
    scb->scb_cscb.cscb_mnext.scg_prev = msg;

    /* Save ptrs to message, and aligned tuple descriptor. */
    scb->scb_sscb.sscb_dccb->dc_tdescmsg = (PTR)msg;
    scb->scb_sscb.sscb_dccb->dc_tdesc = 
	scb->scb_sscb.sscb_dccb->dc_dcblk.sdc_td2_ldesc = darea; 

    return (E_DB_OK);
}


/*{
** Name: scs_dccpmap - Read, convert and queue downstream LDB copymap
**
** Description:
**	This routine reads a possibly fragmented downstream LDB GCA_CPMAP
**	and (1) queues each message fragment to be sent to the FE, and (2) 
**	allocates a single non-fragmented aligned tuple descriptor. For both 
**	cases, new storage is allocated to store the block/descriptor. The 
**	aligned descriptor should be used by the caller for the gca_descriptor 
**	fields of GCA_TUDATA messages. 
**	This routine is intended to be used within direct-connect processing 
**	only. 
**
** Inputs:
**	scb		- session control block for this session
**
** Outputs:
**
**	scb->scb_sscb.sscb_dccb->dc_tdescmsg
**			- Allocated SCC_GCMSG containing the tuple desc.
**	scb->scb_sscb.sscb_dccb->dc_tdesc
**			- Pointer to tuple descriptor within the mesg.
**    Returns:
**	DB_STATUS	
**
**    Exceptions:
**	none
**
** Side Effects:
**	Will allocate memory.
**	
**
** History:
**	20-Jan-1991 (fpang)
**	    Created for SYBIL.
**	04-Sep-1992 (fpang)
**	    Fixed sizeof calculation. In some cases scs_dccpmap()
**	    was taking the sizeof the wrong field.
**	18-Sep-1992 (fpang)
**	    Convert copy maps if LDB and FE are at different protocols.
**	31-Mar-1993 (fpang)
**	    Change types of nleft and save from i4 to u_i2, and
**	    type of flsize from i4  to u_i2.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	15-Sep-1993 (fpang)
**	    When calculating size of new copy map, take into account any 
**	    partial outstanding reads, to prevent buffer overflow.
**	01-Dec-1993 (fpang)
**	    Set scs_dccb.dc_dcblk.sdc_td2_ldesc to scs_dccb.dc_tdesc because
**	    RQF needs to set the gca_descriptor field in the GCA_SD_PARMS.
**	    Fixes B57191, Star copy causes commsvr to crash.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
[@history_template@]...
*/
DB_STATUS
scs_dccpmap(SCD_SCB  *scb )
{
    u_i2	 size = 0;
    SCC_GCMSG	 *msg = NULL, *tmpmsg = NULL;
    GCA_RV_PARMS *rv;
    GCA_CP_MAP   *cp = NULL;
    DB_ERROR	 error;
    SCC_SDC_CB	 **dc;
    DB_STATUS    status = E_DB_OK;
    QEF_RCB	 *qefrcb = scb->scb_sscb.sscb_qeccb;
    PTR		 data;
    u_i2	 nleft = 0, save = 0;
    PTR		 saveptr = NULL;
    i4		 state;
    GCA_TD_DATA	 td;
    PTR		 tdptr = NULL;
    i4	 l_lnat = 0;
    i4		 l_nat = 0;
    PTR		 cpmap = NULL, cpptr = NULL;
    i4		 cpsiz = 0;
    i4	 l_rowdesc = 0;
    i4		 fe_version = scb->scb_cscb.cscb_version;
    i4		 be_version;
    i4		 cpd_type;
    PTR		 flname = NULL, flptr = NULL;
    u_i2	 flsize = 0;
    i4		 l_nullv = 0;
    i4		 nv_type = 0;
    i4		 nv_prec = 0;
    i4	 nv_len  = 0;
    i4		 nullen  = 0;
    PTR		block;

/* States for state machine that reads copy map */
#define SSTATUS_CP   0	/* Read gca_status_cp,    Next state: SDIRECTN_CP */
#define SDIRECTN_CP  1  /* Read gca_direction_cp, Next state: SMAX_ERRS_CP */
#define SMAX_ERRS_CP 2	/* Read gca_status_cp,    Next state : SLFNAME_CP */
#define SLFNAME_CP   3  /* Read gca_l_fname_cp,   Next state : SFNAME_CP */
#define SFNAME_CP    4  /* Read gca_fname_cp,     Next state : SLLOGNAME_CP */
#define SLLOGNAME_CP 5  /* Read gca_l_logname_cp, Next state : SLOGNAME_CP */
#define SLOGNAME_CP  6  /* Read gca_logname_cp,   Next state : SL_COL_DESC */
#define SL_COL_DESC  7  /* Read gca_tsize, gca_result_modifier,
			**      gca_id_tdscr, and gca_l_col_desc, 
			**                        Next state : SALLOC_TDSC */
#define SALLOC_TDSC  8  /* Allocate a GCA_TD_DATA,Next state : SATTDBV */
#define SATTDBV	     9  /* Read gca_attdbv,       Next state : SL_ATTNAME */
#define SL_ATTNAME   10 /* Read gca_l_attname,    Next state : SATTNAME */
#define SATTNAME     11 /* Read gca_attname, 
			** Next state : SL_L_ROWDSC if td.gca_l_col_desc == 0
			**              SATTDBV     otherwise */
#define SL_L_ROWDSC  12 /* Read gca_l_row_desc_cp,Next state : SCP_DOMNAM */
#define SCP_DOMNAM   13 /* Read gca_domname_cp,   Next state : SCP_TYPE */
#define SCP_TYPE     14 /* Read gca_type_cp,      Next state : SCP_PREC */
#define SCP_PREC     15 /* Read gca_precision_cp, Next state : SCP_LENG */
#define SCP_LENG     16 /* Read gca_length_cp,    Next state : SCP_USRDELM */
#define SCP_USRDELM  17 /* Read gca_user_delim_cp,Next state : SCP_L_DELIM */
#define SCP_L_DELIM  18 /* Read gca_l_delim_cp,   Next state : SCP_DELIM */
#define SCP_DELIM    19 /* Read gca_delim_cp,     Next state : SCP_TUPMAP */
#define SCP_TUPMAP   20 /* Read gca_tupmap_cp,    Next state : SCP_CVID */
#define SCP_CVID     21 /* Read gca_cvid_cp,      Next state : SCP_CVPREC */
#define SCP_CVPREC   22 /* Read gca_cvprec_cp,    Next state : SCP_CVLENG */
#define SCP_CVLENG   23 /* Read gca_cvlen_cp,     Next state : SCP_W_NULL */
#define SCP_W_NULL   24 /* Read gca_withnull_cp,  Next state : SCP_L_NULLV */
#define SCP_L_NULLV  25 /* Read gca_l_nullvalue_cp,
			**			  Next state : SCP_NV_TYP */
#define SCP_NV_TYP   26 /* Read gca_nullvalue_cp.gca_type, 
			**                        Next state : SCP_NV_PRC */
#define SCP_NV_PRC   27 /* Read gca_nullvalue_cp.gca_precision,
		        **                        Next state : SCP_NV_LEN */
#define SCP_NV_LEN   28 /* Read gca_nullvalue_cp.gca_length, or gca_nullen_cp
			**                        Next state : SCP_NV_VAL */
#define SCP_NV_VAL   29 /* Read gca_nullvalue_cp.gca_value, or null data value.
			** Next state : SCP_DONE   if l_rowdesc == 0
			**		SCP_DOMNAM otherwise. */
#define SCP_DONE     30 /* Done reading copy map, Next state : None */

    dc = (SCC_SDC_CB **)
		&qefrcb->qef_r3_ddb_req.qer_d6_conn_info.qed_c4_exchange_p;
    state = SSTATUS_CP;
    td.gca_l_col_desc = 0;


    if(( ((GCA_RV_PARMS *)((*dc)->sdc_rv))->gca_message_type == GCA1_C_FROM ) ||
       ( ((GCA_RV_PARMS *)((*dc)->sdc_rv))->gca_message_type == GCA1_C_INTO ))
	be_version = GCA_PROTOCOL_LEVEL_50;
    else
	be_version = GCA_PROTOCOL_LEVEL_40;


/*
** Conversion issue :
** When converting up (GCA_C_FROM/INTO to GCA1_C_FROM/INTO),
** add the following fields and values:
**     sizeof(i4) [ gca_precision_cp = 0 ]
**   + sizeof(i4) [ gca_cvprec_cp    = 0 ]
**   + sizeof(i4) [ gca_l_nullvalue_cp = null_len ? 1 : 0 ]
**   + ( num_coldescs  * 
**		(  sizeof(i4) [ gca_nullvalue_cp.gca_type = cp_type ]
**		 + sizeof(i4) [ gca_nullvalue_cp.gca_precision = 0 ]
**		 + sizeof(i4) [ gca_nullvalue_cp.gca_l_value = nullen ]
**		)
**     ) 
** and set [ gca_nullvalue_cp.gca_value = gca_nulldata ]
**
** When converting down (GCA1_C_FROM/INTO to GCA_C_FROM/INTO,
**     disgard gca_precision_cp, 
**	       gca_cvprec_cp,
**	       gca_l_nullvalue_cp,
**             gca_nullvalue_cp.gca_type,
**	       gca_nullvalue_cp.gca_precision.
**     assign  gca_nullen_cp = gca_nullvalue_cp.gca_l_value 
**        and  gca_nulldata  = gca_nullvalue_cp.gca_value.
*/
    
    /*
    ** The following is a state machine to read a fragmented copy map.
    ** Each 'case' of the 'switch' statement corresponds to a state.
    ** For each state, if the corresponding field(s) can be read, a
    ** state transition occurs by setting the next state, and 'falling
    ** through' to the next state. If the corresponding field(s) cannot
    ** be read, the state does not change, and we break from the switch
    ** to get more downstream data. The state machine will resume with the
    ** same state.
    */
    do
    {
	rv = (GCA_RV_PARMS *)(*dc)->sdc_rv;
	data = rv->gca_data_area;
	nleft = rv->gca_d_length;

	/* Allocate space for copy map message.
	** If one already exists, copy it
	** Add half of the current block in case conversion is needed.
	** Account for partial field copies if save != 0..
	*/
	tmpmsg = msg;
	cpsiz  = cpptr - cpmap;
	status = sc0m_allocate(SCU_MZERO_MASK,
			       (i4)(sizeof(SCC_GCMSG) 
			       + cpsiz 
			       + ((save > 0) ? size - save : 0)
			       + nleft + (nleft / 2)),
			       DB_SCF_ID, (PTR)SCS_MEM, SCG_TAG, &block);
	msg = (SCC_GCMSG *)block;
	if (status)
	{
	    sc0e_put(status, 0, 0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
	    scd_note(E_DB_SEVERE, DB_SCF_ID);
	    return(E_DB_SEVERE);
	}

	msg->scg_mask = 0;	/* Deallocate */
	msg->scg_mtype = rv->gca_message_type;
	if ((fe_version >= GCA_PROTOCOL_LEVEL_50) &&
	    (rv->gca_message_type == GCA_C_FROM))
		    msg->scg_mtype = GCA1_C_FROM;
	else if ((fe_version >= GCA_PROTOCOL_LEVEL_50) &&
	    (rv->gca_message_type == GCA_C_INTO))
		    msg->scg_mtype = GCA1_C_INTO;
	else if ((fe_version < GCA_PROTOCOL_LEVEL_50) &&
	     (rv->gca_message_type == GCA1_C_FROM))
		    msg->scg_mtype = GCA_C_FROM;
	else if ((fe_version < GCA_PROTOCOL_LEVEL_50) &&
	     (rv->gca_message_type == GCA1_C_INTO))
		    msg->scg_mtype = GCA_C_INTO;

	msg->scg_msize = cpsiz + nleft + (nleft / 2);
	msg->scg_mdescriptor = NULL;
	msg->scg_marea = (PTR)msg + sizeof(SCC_GCMSG);
	
	if (cpsiz)
	    MEcopy(cpmap, cpsiz, msg->scg_marea);

	cpmap = msg->scg_marea;
	cpptr = cpmap + cpsiz;

	/* Deallocate the previous message. */
	if ( tmpmsg != NULL )
	{
	    status = sc0m_deallocate( 0, (PTR *)&tmpmsg );
	    if (DB_FAILURE_MACRO(status))
	        break;
	}

	while (1)
	{
	    switch (state)
	    {
	        case SSTATUS_CP  : 
		    size = sizeof(cp->gca_status_cp);
		    if (ldb_fetch(&data, size, &nleft, 
			          &save, &saveptr,
				  (char *)&l_lnat) == E_DB_ERROR)
		    {
		        break; /* Need more data, no state transition. */
		    }
		    MEcopy( &l_lnat, size, cpptr );
		    cpptr += size;
		    state = SLFNAME_CP;  /* Fall through */
	        case SDIRECTN_CP  : 
		    size = sizeof(cp->gca_direction_cp);
		    if (ldb_fetch(&data, size, &nleft, 
			          &save, &saveptr,
				  (char *)&l_lnat) == E_DB_ERROR)
		    {
		        break; /* Need more data, no state transition. */
		    }
		    MEcopy( &l_lnat, size, cpptr );
		    cpptr += size;
		    state = SLFNAME_CP;  /* Fall through */
	        case SMAX_ERRS_CP  : 
		    size = sizeof(cp->gca_maxerrs_cp);
		    if (ldb_fetch(&data, size, &nleft, 
			          &save, &saveptr,
				  (char *)&l_lnat) == E_DB_ERROR)
		    {
		        break; /* Need more data, no state transition. */
		    }
		    MEcopy( &l_lnat, size, cpptr );
		    cpptr += size;
		    state = SLFNAME_CP;  /* Fall through */
	        case SLFNAME_CP : 
		    size = sizeof(cp->gca_l_fname_cp);
		    if (ldb_fetch(&data, size, &nleft, 
			          &save, &saveptr,
				  (char *)&l_nat) == E_DB_ERROR)
		    {
		        break; /* Need more data, no state transition. */
		    }
		    MEcopy( &l_nat, size, cpptr );
		    cpptr += size;
		    state = SFNAME_CP;  /* Fall through */
	        case SFNAME_CP : 
		    size = l_nat;	/* gca_fname_cp */
		    if (size && ((flptr == NULL) || (size > flsize)))
		    {
			if (flptr != NULL) 
			{
			    status = sc0m_deallocate( 0, &flptr);
			    if (DB_FAILURE_MACRO(status))
				break;
			    flsize = 0;
			    flptr = flname = NULL;
			}
		        status = sc0m_allocate(SCU_MZERO_MASK,
					(i4)(sizeof(SC0M_OBJECT) + size),
					       DB_SCF_ID, (PTR)SCS_MEM,
					       SCG_TAG, &flptr);
			if (DB_FAILURE_MACRO(status))
			    break;
			flsize = size;
			flname = flptr + sizeof(SC0M_OBJECT);
		    }
		    if (size)
		    {
		        if (ldb_fetch(&data, size, &nleft, 
			              &save, &saveptr, flname) == E_DB_ERROR)
		        {
		            break; /* Need more data, no state transition. */
		        }
		        MEcopy( flname, size, cpptr );
		        cpptr += size;
		    }
		    state = SLLOGNAME_CP;  /* Fall through */
	        case SLLOGNAME_CP : 
		    size = sizeof(cp->gca_l_logname_cp);
		    if (ldb_fetch(&data, size, &nleft, 
			          &save, &saveptr,
				  (char *)&l_nat) == E_DB_ERROR)
		    {
		        break; /* Need more data, no state transition. */
		    }
		    MEcopy( &l_nat, size, cpptr );
		    cpptr += size;
		    state = SLOGNAME_CP;  /* Fall through */
	        case SLOGNAME_CP : 
		    size = l_nat;	  /* gca_logname_cp */
		    if (size && ((flptr == NULL) || (size > flsize)))
		    {
			if (flptr != NULL)
			{
			    status = sc0m_deallocate( 0, &flptr);
			    if (DB_FAILURE_MACRO(status))
				break;
			    flsize = 0;
			    flptr = flname = NULL;
			}
		        status = sc0m_allocate(SCU_MZERO_MASK,
				       (i4)(sizeof(SC0M_OBJECT) + size),
					       DB_SCF_ID, (PTR)SCS_MEM,
					       SCG_TAG, &flptr);
			if (DB_FAILURE_MACRO(status))
			    break;
			flname = flptr + sizeof(SC0M_OBJECT);
			flsize = size;
		    }
		    if (ldb_fetch(&data, size, &nleft, 
			          &save, &saveptr, flname) == E_DB_ERROR)
		    {
		        break; /* Need more data, no state transition. */
		    }
		    MEcopy( flname, size, cpptr );
		    cpptr += size;
		    state = SL_COL_DESC;  /* Fall through */
	        case  SL_COL_DESC : 
		    size = sizeof(td.gca_tsize)
		         + sizeof(td.gca_result_modifier)
		         + sizeof(td.gca_id_tdscr)
		         + sizeof(td.gca_l_col_desc);
		    if (ldb_fetch(&data, size, &nleft, 
			          &save, &saveptr, (PTR)&td) == E_DB_ERROR)
		    {
		        break; /* Need more data, no state transition. */
		    }
		    state = SALLOC_TDSC;  /* Fall through */
	        case  SALLOC_TDSC : /* Allocate tuple descriptor */
		    /* Allocate the maximum tuple descriptor, assuming
		    ** the widest column name.
		    */
	            size = sizeof(GCA_TD_DATA) - sizeof(GCA_COL_ATT)
		 	 + (td.gca_l_col_desc * 
			    (DB_GW1_MAXNAME + sizeof(GCA_COL_ATT)));

	            status = sc0m_allocate(SCU_MZERO_MASK,
			   	   (i4)(sizeof(SC0M_OBJECT) + size),
			       	           DB_SCF_ID, (PTR)SCS_MEM, SCG_TAG, 
					   &tdptr);

    		    if (DB_FAILURE_MACRO(status))
			break;
		
	            scb->scb_sscb.sscb_dccb->dc_tdescmsg = tdptr;
		    tdptr += sizeof(SC0M_OBJECT);
	            scb->scb_sscb.sscb_dccb->dc_tdesc = tdptr;

		    size = sizeof(td.gca_tsize) + sizeof(td.gca_result_modifier)
		         + sizeof(td.gca_id_tdscr) + sizeof(td.gca_l_col_desc);

		    MEcopy((PTR)&td, size, tdptr);
		    tdptr += size;
		    MEcopy((PTR)&td, size, cpptr );
		    cpptr += size;
		    state = SATTDBV; /* Fall through */
	        case  SATTDBV : 
		    size = sizeof(td.gca_col_desc[0].gca_attdbv);
		    if (ldb_fetch(&data, size, &nleft, 
		        &save, &saveptr, tdptr) == E_DB_ERROR)
		    {
		        break; /* Need more data, no state transition. */
		    }
		    MEcopy( tdptr, size, cpptr );
		    cpptr += size;
		    tdptr += size;
		    state = SL_ATTNAME;  /* Fall through */
	        case  SL_ATTNAME : 
		    size = sizeof(td.gca_col_desc[0].gca_l_attname);
		    if (ldb_fetch(&data, size, &nleft, &save, &saveptr, 
			(char *)&td.gca_col_desc[0].gca_l_attname) == E_DB_ERROR)
		    {
		        break; /* Need more data, no state transition. */
		    }
		    MEcopy(&td.gca_col_desc[0].gca_l_attname, size, tdptr);
		    tdptr += size;
		    MEcopy(&td.gca_col_desc[0].gca_l_attname, size, cpptr);
		    cpptr += size;
		    state = SATTNAME;  /* Fall through */
	        case  SATTNAME : 
		    size = td.gca_col_desc[0].gca_l_attname;
		    if (ldb_fetch(&data, size, &nleft, 
		        &save, &saveptr, tdptr) == E_DB_ERROR)
		    {
		        break; /* Need more data, no state transition. */
		    }
		    MEcopy( tdptr, size, cpptr );
		    cpptr += size;
		    tdptr += size;

	            td.gca_l_col_desc--;
		    if (td.gca_l_col_desc != 0)
		    {
			state = SATTDBV;
			continue;	/* Go read next column attribute */
		    }
		    else
		    {
			state = SL_L_ROWDSC; 	/* Fall through */
		    }
	        case  SL_L_ROWDSC : 
		    size = sizeof(cp->gca_l_row_desc_cp);
		    if (ldb_fetch(&data, size, &nleft, 
		        &save, &saveptr, (char *)&l_rowdesc) == E_DB_ERROR)
		    {
		        break; /* Need more data, no state transition. */
		    }
		    MEcopy( (PTR)&l_rowdesc, size, cpptr );
		    cpptr += size;
		    state = SCP_DOMNAM;	/* Fall through */
		case  SCP_DOMNAM :
		    size = GCA_MAXNAME;		/* gca_domname_cp */
		    if (size && ((flptr == NULL) || (size > flsize)))
		    {
			if (flptr != NULL)
			{
			    status = sc0m_deallocate( 0, &flptr );
			    if (DB_FAILURE_MACRO(status))
				break;
			    flsize = 0;
			    flptr = flname = NULL;
			}
			status = sc0m_allocate(SCU_MZERO_MASK,
				       (i4)(sizeof(SC0M_OBJECT) + size),
					       DB_SCF_ID, (PTR)SCS_MEM,
					       SCG_TAG, &flptr);
			if (DB_FAILURE_MACRO(status))
			    break;
			flname = flptr + sizeof(SC0M_OBJECT);
			flsize = size;
		    }
		    if (ldb_fetch(&data, size, &nleft,
			&save, &saveptr, flname) == E_DB_ERROR)
		    {
			break; /* Need more data, no state transition. */
		    }
		    MEcopy( flname, size, cpptr );
		    cpptr += size;
		    state = SCP_TYPE;		/* Fall through */
		case  SCP_TYPE :
		    size = sizeof(i4);		/* gca_type_cp */
		    if (ldb_fetch(&data, size, &nleft,
			&save, &saveptr, (char *)&cpd_type) == E_DB_ERROR)
		    {
			break; /* Need more data, no state transition. */
		    }
		    MEcopy( (PTR)&cpd_type, size, cpptr );
		    cpptr += size;
		    state = SCP_PREC;		/* Fall through */
		case  SCP_PREC :
		    size = sizeof(i4);		/* gca_precision_cp */
		    if( be_version >= GCA_PROTOCOL_LEVEL_50 )
		    {
			if (ldb_fetch(&data, size, &nleft,
			    &save, &saveptr, (char *)&l_nat) == E_DB_ERROR)
		    	{
			    break; /* Need more data, not state transition. */
		    	}
		    }
		    else
		    	l_nat = 0;	/* if converting 40->50 */
		    if( fe_version >= GCA_PROTOCOL_LEVEL_50 )
		    {
		    	MEcopy( (PTR)&l_nat, size, cpptr );
		    	cpptr += size;
		    }
		    state = SCP_LENG;		/* Fall through */
		case SCP_LENG :
		    size = sizeof(i4);	/* gca_length_cp */
		    if (ldb_fetch(&data, size, &nleft,
			&save, &saveptr, (char *)&l_lnat) == E_DB_ERROR)
		    {
			break; /* Need more data, no state transition. */
		    }
		    MEcopy( (PTR)&l_lnat, size, cpptr );
		    cpptr += size;
		    state = SCP_USRDELM;	/* Fall through */
		case SCP_USRDELM :
		    size = sizeof(i4);		/* gca_user_delim_cp */
		    if (ldb_fetch(&data, size, &nleft,
			&save, &saveptr, (char *)&l_nat) == E_DB_ERROR)
		    {
			break; /* Need more data, no state transition. */
		    }
		    MEcopy( (PTR)&l_nat, size, cpptr );
		    cpptr += size;
		    state = SCP_L_DELIM;	/* Fall through */
		case SCP_L_DELIM :
		    size = sizeof(i4);		/* gca_l_delim_cp */
		    if (ldb_fetch(&data, size, &nleft,
			&save, &saveptr, (char *)&l_nat) == E_DB_ERROR)
		    {
			break; /* Need more data, no state transition. */
		    }
		    MEcopy( (PTR)&l_nat, size, cpptr );
		    cpptr += size;
		    state = SCP_DELIM;		/* Fall through */
		case SCP_DELIM :
		    size = l_nat;		/* gca_delim_cp */
		    if (size && ((flptr == NULL) || (size > flsize)))
		    {
			if (flptr != NULL)
			{
			    status = sc0m_deallocate( 0, &flptr );
			    if (DB_FAILURE_MACRO(status))
				break;
			    flsize = 0;
			    flname = flptr = NULL;
			}
		        status = sc0m_allocate(SCU_MZERO_MASK,
				       (i4)(sizeof(SC0M_OBJECT) + size),
					       DB_SCF_ID, (PTR)SCS_MEM,
					       SCG_TAG, &flptr);
			if (DB_FAILURE_MACRO(status))
			    break;
			flsize = size;
			flname = flptr + sizeof(SC0M_OBJECT);
		    }
		    if (ldb_fetch(&data, size, &nleft,
			&save, &saveptr, flname) == E_DB_ERROR)
		    {
			break; /* Need more data, no state transition. */
		    }
		    MEcopy( flname, size, cpptr );
		    cpptr += size;
		    state = SCP_TUPMAP;		/* Fall through */
		case SCP_TUPMAP :
		    size = sizeof(i4);	/* gca_tupmap_cp */
		    if (ldb_fetch(&data, size, &nleft,
			&save, &saveptr, (char *)&l_lnat) == E_DB_ERROR)
		    {
			break; /* Need more data, no state transition. */
		    }
		    MEcopy( (PTR)&l_lnat, size, cpptr );
		    cpptr += size;
		    state = SCP_CVID;		/* Fall through */
		case SCP_CVID :
		    size = sizeof(i4);		/* gca_cvid_cp */
		    if (ldb_fetch(&data, size, &nleft,
			&save, &saveptr, (char *)&l_nat) == E_DB_ERROR)
		    {
			break; /* Need more data, no state transition. */
		    }
		    MEcopy( (PTR)&l_nat, size, cpptr );
		    cpptr += size;
		    state = SCP_CVPREC; 	/* Fall through */
		case SCP_CVPREC :
		    size = sizeof(i4);		/* gca_cvprec_cp */
		    if( be_version >= GCA_PROTOCOL_LEVEL_50 )
		    {
		    	if (ldb_fetch(&data, size, &nleft,
			    &save, &saveptr, (char *)&l_nat) == E_DB_ERROR)
		    	{
			    break; /* Need more data, no state transition. */
		    	}
		    }
		    else
		    	l_nat = 0;	/* if converting 40->50 */
		    if( fe_version >= GCA_PROTOCOL_LEVEL_50 )
		    {
		    	MEcopy( (PTR)&l_nat, size, cpptr );
		    	cpptr += size;
		    }
		    state = SCP_CVLENG;		/* Fall through */
		case SCP_CVLENG :
		    size = sizeof(i4);		/* gca_cvlen_cp */
		    if (ldb_fetch(&data, size, &nleft,
			&save, &saveptr, (char *)&l_nat) == E_DB_ERROR)
		    {
			break; /* Need more data, no state transition. */
		    }
		    MEcopy( (PTR)&l_nat, size, cpptr );
		    cpptr += size;
		    state = SCP_W_NULL;		/* Fall through */
		case SCP_W_NULL :
		    size = sizeof(i4);		/* gca_withnull_cp */
		    if (ldb_fetch(&data, size, &nleft,
			&save, &saveptr, (char *)&l_nat) == E_DB_ERROR)
		    {
			break; /* Need more data, no state transition. */
		    }
		    MEcopy( (PTR)&l_nat, size, cpptr );
		    cpptr += size;
		    state = SCP_L_NULLV; 	/* Fall through */
		case SCP_L_NULLV :
		    size = sizeof(i4);		/* gca_l_nullvalue_cp */
		    if( be_version >= GCA_PROTOCOL_LEVEL_50 )
		    {
		        if (ldb_fetch(&data, size, &nleft,
			    &save, &saveptr, (char *)&l_nullv) == E_DB_ERROR)
		        {
			    break; /* Need more data, no state transition. */
			}
		    }
		    else
			l_nullv = 0;	/* if converting 40->50 */
		    state = SCP_NV_TYP;		/* Fall through */
		case SCP_NV_TYP :
		    size = sizeof(i4);	      /* gca_nullvalue_cp[0].gca_type */
		    if ((be_version >= GCA_PROTOCOL_LEVEL_50) && (l_nullv != 0))
		    {
			if (ldb_fetch(&data, size, &nleft,
				      &save, &saveptr,
				      (char *)&nv_type) == E_DB_ERROR)
		    	{
			    break; /* Need more data, no state transition. */
		    	}
		    }
		    else
		    	nv_type = 0;
		    state = SCP_NV_PRC;		/* Fall through */
		case SCP_NV_PRC :
		    size = sizeof(i4);	 /* gca_nullvalue_cp[0].gca_precision */
		    if ((be_version >= GCA_PROTOCOL_LEVEL_50) && (l_nullv != 0))
		    {
		        if (ldb_fetch(&data, size, &nleft,
			    &save, &saveptr, (char *)&nv_prec) == E_DB_ERROR)
		        {
			    break; /* Need more data, no state transition. */
		        }
		    }
		    else
			nv_prec = 0;
		    state = SCP_NV_LEN;		/* Fall through */
		case SCP_NV_LEN :
		    if ((be_version >= GCA_PROTOCOL_LEVEL_50) && (l_nullv != 0))
		    {
			/* gca_nullvalue_cp[0].gca_l_value */
		        size = sizeof(i4);	
			if (ldb_fetch(&data, size, &nleft,
			    &save, &saveptr, (char *)&nv_len) == E_DB_ERROR)
			{
			    break; /* Need more data, no state transition. */
			}
		    }
		    else if (be_version < GCA_PROTOCOL_LEVEL_50)
		    {
			size = sizeof(i4); 	/* gca_nullen_cp */
		        if (ldb_fetch(&data, size, &nleft,
			    &save, &saveptr, (char *)&nullen) == E_DB_ERROR)
		        {
			    break; /* Need more data, no state transition. */
		        }
		    }
		    else
		        nv_len = 0;
		    state = SCP_NV_VAL;		/* Fall through */
		case SCP_NV_VAL :
		    if (be_version >= GCA_PROTOCOL_LEVEL_50)
		        size = nv_len;	/* gca_nullvalue_cp[0].gca_value */
		    else
			size = nullen;	/* gca_nullen_cp */
		    if (size && ((flptr == NULL) || (size > flsize)))
		    {
			if (flptr != NULL)
			{
			    status = sc0m_deallocate( 0, &flptr );
			    if (DB_FAILURE_MACRO(status))
				break;
			    flsize = 0;
			    flptr = flname = NULL;
			}
			status = sc0m_allocate(SCU_MZERO_MASK,
				       (i4)(sizeof(SC0M_OBJECT) + size),
					       DB_SCF_ID, (PTR)SCS_MEM,
					       SCG_TAG, &flptr);
			if (DB_FAILURE_MACRO(status))
			    break;
			flsize = size;
			flname = flptr + sizeof(SC0M_OBJECT);
		    }
		    if (size)
		    {
		        if (ldb_fetch(&data, size, &nleft,
			    &save, &saveptr, flname) == E_DB_ERROR)
		        {
			    break; /* Need more data, no state transition. */
		        }
		    }
		    if(( fe_version >= GCA_PROTOCOL_LEVEL_50 ) &&
		       ( be_version <  GCA_PROTOCOL_LEVEL_50 ))
		    {
			/* Converting GCA_C_FROM/INTO to GCA1_C_FROM/INTO. */
			l_nullv = (nullen == 0) ? FALSE : TRUE;
			nv_len  = nullen;
			nv_type = cpd_type;
			nv_prec = 0;
		    }
		    else if(( fe_version < GCA_PROTOCOL_LEVEL_50 ) &&
			    ( be_version >=  GCA_PROTOCOL_LEVEL_50 ))
		    {
			/* Converting GCA1_C_FROM/INTO to GCA_C_FROM/INTO. */
			nullen = nv_len;
		    }

		    if( fe_version >= GCA_PROTOCOL_LEVEL_50 )
		    {
			/* gca_l_nullvalue_cp */
			MEcopy( (PTR)&l_nullv, sizeof(i4), cpptr );
			cpptr += sizeof( i4  );
			if (l_nullv == TRUE)
			{
			    /* gca_nullvalue_cp.gca_type */
			    MEcopy( (PTR)&nv_type, sizeof(i4), cpptr );
			    cpptr += sizeof( i4  );
			    /* gca_nullvalue_cp.gca_precision */
			    MEcopy( (PTR)&nv_prec, sizeof(i4), cpptr );
			    cpptr += sizeof( i4  );
			    /* gca_nullvalue_cp.gca_l_value */
			    MEcopy( (PTR)&nv_len, sizeof(i4), cpptr);
			    cpptr += sizeof( i4 );
			    /* gca_nullvalue_cp.gca_value */
			    MEcopy( flname, nv_len, cpptr );
			    cpptr += nv_len;
		        }
		    }
		    if( fe_version < GCA_PROTOCOL_LEVEL_50 )
		    {
			/* gca_nullen_cp */
			MEcopy( (PTR)&nullen, sizeof(i4), cpptr );
			cpptr += sizeof( i4  );
			if (nullen > 0)
			{
			    /* Null data */
			    MEcopy( flname, nullen, cpptr );
			    cpptr += nullen;
			} 
		    }
		    l_rowdesc--;
		    if (l_rowdesc != 0)
		    {
		        state = SCP_DOMNAM;	/* Go get next row domain */
		        continue;
		    }
		    else
		    {
			state = SCP_DONE;	/* Done reading CP map */
			break;
		    }
	    } /* switch */
	    break;	/* Unconditional. Done, or need to get more data */
	} /* while */
		
	if ((rv->gca_end_of_data == TRUE) || (DB_FAILURE_MACRO(status)))
	    break;

        status = scs_dccont(scb, SDC_READ, 0, NULL);  /* Read next ldb msg */

    } while ((DB_SUCCESS_MACRO(status)) && (scb->scb_sscb.sscb_interrupt == 0));

    if (flptr != NULL)
    {
	status = sc0m_deallocate( 0, &flptr );
    }

    if ((DB_FAILURE_MACRO(status)) ||
	(scb->scb_sscb.sscb_interrupt != 0) ||
	(state != SCP_DONE))
    {
	if (DB_FAILURE_MACRO(status))
	{
	    sc0e_put(status, 0, 0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
	    sc0e_uput(status, 0, 0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0);
	    scd_note(E_DB_SEVERE, DB_SCF_ID);
	}

	scc_dispose(scb);

	if (scb->scb_sscb.sscb_dccb->dc_tdescmsg != NULL)
	    status = sc0m_deallocate(0, &scb->scb_sscb.sscb_dccb->dc_tdescmsg);

	scb->scb_sscb.sscb_dccb->dc_tdesc = NULL;

	if (msg != NULL)
	    status = sc0m_deallocate(0, (PTR *)&msg);

	return (E_DB_ERROR);
    }

    msg->scg_msize = cpptr - cpmap;
    msg->scg_next = (SCC_GCMSG *)&scb->scb_cscb.cscb_mnext;
    msg->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
    scb->scb_cscb.cscb_mnext.scg_prev->scg_next = msg;
    scb->scb_cscb.cscb_mnext.scg_prev = msg;
    scb->scb_sscb.sscb_dccb->dc_dcblk.sdc_td2_ldesc = 
					    scb->scb_sscb.sscb_dccb->dc_tdesc;

    return (E_DB_OK);
}


/*{
** Name: scs_dcinput - Get client message block in direct connect.
**
** Description:
**	This routine will process client input. The GCA message is formatted
**	and returned to the caller. If an interrupt occurs, this routine
**	will return an error. This is a specialzed routine to be used within
**	direct connect processing only.
**
** Inputs:
**	scb		Session control block of session that wants input.
**	rv		Address to return address of GCA message.
**
** Outputs:
**	rv		Addres of GCA message.
**
**    Returns:
**	E_DB_ERROR	If interrupted
**	GC status	If exists
**	FAIL		If FORMAT fails
**	OK		If successful
**
**    Exceptions:
**	None
**
** Side Effects:
**	None
*
** History:
**	20-Jan-1991 (fpang)
**	    Created for SYBIL.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	7-Jul-1993 (daveb)
**	    pass correct err_code to sc0e_put instead of the whole
**	    DB_ERROR structure.
[@history_template@]...
*/
STATUS
scs_dcinput(SCD_SCB   *scb,
	    GCA_RV_PARMS **rv )
{
    DB_ERROR	error;
    DB_STATUS   status;

    if (scb->scb_sscb.sscb_interrupt != 0)
	return(E_DB_ERROR);

    if (scb->scb_cscb.cscb_gci.gca_status != E_GC0000_OK)
    {
	if (scb->scb_cscb.cscb_gci.gca_status == E_GC0027_RQST_PURGED)
	{
	    CScancelled(0);
            scb->scb_cscb.cscb_gci.gca_status = E_SC1000_PROCESSED;
	    return (E_GC0027_RQST_PURGED);
	}
	else if ((scb->scb_cscb.cscb_gci.gca_status == E_GC0001_ASSOC_FAIL) ||
	         (scb->scb_cscb.cscb_gci.gca_status == E_GC0023_ASSOC_RLSED))
	{
	    return(E_GC0001_ASSOC_FAIL);
	}
    	sc0e_put(scb->scb_cscb.cscb_gci.gca_status,
                 &scb->scb_cscb.cscb_gci.gca_os_status, 0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0);
    	sc0e_put(E_SC022F_BAD_GCA_READ, 0, 0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0);
	return(E_DB_ERROR);
    }

    scb->scb_cscb.cscb_gci.gca_status = E_SC1000_PROCESSED;
    *rv = &scb->scb_cscb.cscb_gci;
    return (E_DB_OK);
}

/*{
** Name: scs_dcsend - Queue a GCA message block to be sent to client.
**
** Description:
**	This routine performs the necesary processing to queue an unformated
**	GCA message destined to the FE client. If requested, the message will
**	be copied to allocated storage, otherwise, the original buffer is used.
**	The message will be formatted and then queued to be sent to the 
**	FE client.
**
** Inputs:
**	scb	- Session control block
**	rv	- Pointer to a GCA message block (GCA_RV_PARM)
**	message - Address to return address of message
**	copy	- Allocate a new buffer?
**
** Outputs:
**	message - Address of message.
**
**    Returns:
**	E_DB_SEVERE, E_DB_ERROR	- If failure
**	E_DB_OK			- If success
**
**    Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	20-Jan-1991 (fpang)
**	    Created for SYBIL.
**	18-Jan-1992 (fpang)
**	    If LDB and FE protocol are different, and message is a GCA_ERROR,
**	    convert from LDB local error, generic error, or sqlstate to what
**	    the FE protocol expects.
**	2-Jul-1993 (daveb)
**	    prototyped.  Use proper PTR for allocation.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**	12-Oct-2001 (jenjo02)
**	    Added some useful comments to explain error reporting.
**	    Don't try converting a user-defined MESSAGE number to
**	    an sqlstate, let those pass through unmolested.
**	    We only need to "look up" sqlstate if the LDB server
**	    is < PROTOCOL_LEVEL_60, otherwise sqlstate is already
**	    known.
**	    Don't ule_format() non-error error (zero) which won't
**	    be found and will produce an error if we try, obfuscating
**	    success.
**	    Check status of ule_format() and supply an appropriate
**	    SQLSTATE rather than using uninitialized local sqlstate
**	    variable.
[@history_template@]...
*/
DB_STATUS
scs_dcsend(SCD_SCB   *scb,
	   GCA_RV_PARMS *rv,
	   SCC_GCMSG  **message,
	   bool   copy )
{
    u_i2      size;
    DB_ERROR  error;
    DB_STATUS status;
    PTR		block;
    
    size = sizeof(SCC_GCMSG);

    if (copy)
	size += rv->gca_d_length;

    status = sc0m_allocate(0,
			   (i4)size,
			   DB_SCF_ID,
			   (PTR)SCS_MEM,
			   SCG_TAG,
			   &block);
    *message = (SCC_GCMSG *)block;
    if (status)
    {
	sc0e_put(status, 0, 0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0);
	scd_note(E_DB_SEVERE, DB_SCF_ID);
	return (E_DB_SEVERE);
    }

    (*message)->scg_mask = 0;		/* Deallocate */
    (*message)->scg_mtype = rv->gca_message_type;
    (*message)->scg_msize = rv->gca_d_length;
    if ((rv->gca_message_type == GCA_TUPLES) ||
	(rv->gca_message_type == GCA_CDATA))
        (*message)->scg_mdescriptor = scb->scb_sscb.sscb_dccb->dc_tdesc;
    else
        (*message)->scg_mdescriptor = NULL;	
    if (rv->gca_end_of_data == FALSE)
        (*message)->scg_mask |= SCG_NOT_EOD_MASK;
    
    if (copy)
    {
	(*message)->scg_marea = (char*)(*message) + sizeof(SCC_GCMSG);
	MEcopy(rv->gca_data_area, rv->gca_d_length, (*message)->scg_marea);
    }
    else
    {
	(*message)->scg_marea = rv->gca_buffer;
    }

    (*message)->scg_next = (SCC_GCMSG *)&scb->scb_cscb.cscb_mnext;
    (*message)->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
    scb->scb_cscb.cscb_mnext.scg_prev->scg_next = (*message);
    scb->scb_cscb.cscb_mnext.scg_prev = (*message);

    /*
    ** Conversion issues..
    ** All messages bound for the FE come through here, so do conversions
    ** here.
    ** 1. SQLSTATE -> GE -> Local error
    */
    
    if ((*message)->scg_mtype == GCA_ERROR)
    {
	GCA_ER_DATA	*erdata;
	GCA_E_ELEMENT	*elem;
	i4		nelem, nparm, ldbv;
	i4		peer;
	PTR		dbv;

	/*
	** - If the client has a partner protocol level of 2 or less,
	**   then he is not expecting generic errors, so we send errors
	**   the old way (local error in the gca_id_error slot and 0 in
	**   the gca_local_error slot).
	** - If the client protocol level is in (2,60) we send generic
	**   error in the gca_id_error slot and local error in the
	**   gca_local_error slot.
	** - Finally, if the protocol level >= 60, we send local error
	**   in gca_local_error and encoded sqlstate in gca_id_error
	**
	** Note: cscb_version will be correctly set ONLY IF client is going
	** through the name server.  Direct connections (by using the
	** II_DBMS_SERVER logical) will cause cscb_version to be 0.
	*/
	
	peer   = scb->scb_sscb.sscb_dccb->dc_dcblk.sdc_peer_protocol;
	erdata = (GCA_ER_DATA *)(*message)->scg_marea;

	for ( nelem = erdata->gca_l_e_element,
	      elem = &erdata->gca_e_element[0]; nelem > 0; nelem-- )
	{ 
	    /*
	    ** MESSAGE numbers from db procs "...do not correspond to
	    ** DBMS error codes...", so leave them alone.
	    **
	    ** For all others, if either client or peer doesn't know
	    ** about SQLSTATE, we've more work to do:
	    */
	    if ( elem->gca_severity != GCA_ERMESSAGE &&
	        (scb->scb_cscb.cscb_version < GCA_PROTOCOL_LEVEL_60 ||
		 peer < GCA_PROTOCOL_LEVEL_60) )
	    {
		i4		msglen, ulecode;
		i4		local_error, id_error;
		char		errbuf[DB_ERR_SIZE];
		DB_SQLSTATE 	sqlstate;

		if (peer < GCA_PROTOCOL_LEVEL_2)
		    MEcopy(&elem->gca_id_error, 
			   sizeof(elem->gca_id_error), 
			   &local_error);
		else
		    MEcopy(&elem->gca_local_error, 
			   sizeof(elem->gca_local_error),
			   &local_error);

		if (scb->scb_cscb.cscb_version <= GCA_PROTOCOL_LEVEL_2)
		{
    		    id_error = local_error;
		    local_error = 0;			
		}
		else 
		{
		    /*
		    ** If the LDB server does not "know" about SQLSTATEs, 
		    ** look up the SQLSTATE corresponding to this error
		    ** code, otherwise gca_id_error already contains
		    ** encoded SQLSTATE placed there by the LDB server.
		    */
		    if ( peer < GCA_PROTOCOL_LEVEL_60 )
		    {
			/* local_error == 0 is not an "error" */
			if ( local_error )
			    status = ule_format(local_error, NULL, ULE_LOOKUP, 
				&sqlstate, errbuf,  (i4)sizeof(errbuf), 
				    &msglen, &ulecode,(i4)12,
				0, NULL, 0, NULL,
				0, NULL, 0, NULL,
				0, NULL, 0, NULL,
				0, NULL, 0, NULL,
				0, NULL, 0, NULL,
				0, NULL, 0, NULL);
			if ( status || local_error == 0 )
			{
			    char	*misc_err_sqlstate;
			    i4		i;

			    if ( local_error )
				misc_err_sqlstate = SS50000_MISC_ERRORS;
			    else
				misc_err_sqlstate = SS00000_SUCCESS;

			    for ( i = 0; i < DB_SQLSTATE_STRING_LEN; i++ )
				sqlstate.db_sqlstate[i] = misc_err_sqlstate[i];
			}
		    }
		    else
			ss_decode(sqlstate.db_sqlstate, elem->gca_id_error);

		    if (scb->scb_cscb.cscb_version < GCA_PROTOCOL_LEVEL_60)
			id_error = ss_2_ge(sqlstate.db_sqlstate, local_error);
		    else
			id_error = ss_encode(sqlstate.db_sqlstate);
		}
		MEcopy(&id_error, sizeof(elem->gca_id_error),
		       &elem->gca_id_error);
		MEcopy(&local_error, sizeof(elem->gca_local_error),
		       &elem->gca_local_error);
	    } /* (cscb_version || peer) < GCA_PROTOCOL_LEVEL_60 */
	    
	    MEcopy(&elem->gca_l_error_parm, 
		   sizeof(elem->gca_l_error_parm),
		   &nparm);
	    for ( dbv = (PTR)&elem->gca_error_parm[0]; nparm != 0; nparm-- )
	    {
		dbv += 2 * sizeof(i4);          /* gca_type, gca_precision */
		MEcopy(dbv, sizeof(i4), &ldbv); /* gca_l_value */
		dbv += sizeof(i4) + ldbv;	 /* gca_l_value, gca_value */
	    }	    
	    
	} /* each error element */
    } /* GCA_ERROR */
    
    return (E_DB_OK);
}


/*{
** Name: scs_dcmsg - DIRECT DISCONNECT message detection.
**
** Description: This routine does a quick check of DIRECT DISCONNECT.
**    White spaces ( space, tab, new line, carriage return) are
**    ignored. It is expected such string is terminated by a
**    white space or a semicolon. A tacit assumption is that a direct
**    disconnect string does not span GCA message buffers.
**
** Inputs:
**      rv	GCA_RV_PARMS, GCA message block from FE.
**
** Outputs:
**	None
**
**	Returns:
**	    bool
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-Sep-1988 (alexh)
**          Created for TITAN DIRECT CONNECT
**	24-Jan-1992 (fpang)
**	    Changed input parameter to a GCA_IT_PARM.
**	2-Jul-1993 (daveb)
**	    prototyped.
[@history_template@]...
*/
bool
scs_dcmsg(GCA_RV_PARMS *rv )
{
    char	*str;
    char	*eos;
    i4		len;
    i4		compare_len;
    GCA_DATA_VALUE *qptr;

    if (rv->gca_message_type != GCA_QUERY)
	return (FALSE);

    if (rv->gca_end_of_data == FALSE)
	return (FALSE);

    qptr = ((GCA_Q_DATA *)rv->gca_data_area)->gca_qdata;

    str  = qptr->gca_value;
    len  = qptr->gca_l_value;
    eos  = str + len;

    /*
    ** skip white chars 
    */
    while (*str == '\n' || *str == '\t' || *str == ' ' || *str == '\r')
    {
    	++str;
    }

    /*
    ** check for "direct" keyword 
    */
    compare_len = STlength("direct");
    if (STncasecmp("direct", str, compare_len ) == 0)
    {
	char	*eot;/* end of token */

	str += compare_len;
	eot = str;

	/*
	** skip white chars 
	*/
	while (*str == '\n' || *str == '\t' || *str == ' ' || *str == '\r')
	    ++str;

	if (eot == str)
	{
	    /*
	    ** no separator after direct keyword 
	    */
	    return(FALSE);
	}

	/*
	** check for "disconnect" keyword 
	*/
	compare_len = STlength("disconnect");
	if (STncasecmp("disconnect", str, compare_len) == 0)
	{
	    str += compare_len;
	    /* skip white chars */
	    while (*str == '\n' || *str == '\t' || *str == ' ' || *str == '\r')
	    {
		++str;
	    }
	    if (*str == 0 && str <= eos)
	    {
		return(TRUE);
	    }
	}
    }

    return(FALSE);
}

/*{
** Name: scs_dcinterrupt - Interrupt a downstream LDB association.
**
** Description:
**	This routine will interrupt a downstream LDB association, and
**	return control to the caller when the LDB asociation has acknowldeged
**	the interrupt. This routined should be used within direct connect
**	processing only.
**
** Inputs:
**      scb             Session control block of session that wants input.
**      rv              Address to return address of GCA message.
**
** Outputs:
**	rv		Addres of GCA message.
**
**    Returns:
**	DB_STATUS
**
**    Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	20-Jan-1991 (fpang)
**	    Created for SYBIL.
**	7-Jul-1993 (daveb)
**	    pass correct err_code to sc0e_put instead of the whole
**	    DB_ERROR structure.
**	09-Sep-1993 (fpang)
**	    If scb->scb_sscb.sscb_dccb is NULL, DC control block has already
**	    been released, so don't continue. This usually happens if an
**	    interrupt occurs after COPY/DIRCON/DEI has already completed.
**	07-Oct-1993 (fpang)
**	    Logic for reading LDB IACK was not correct. LDB messages should
**	    be read and discarded while the message is NOT an IACK message 
**	    OR NOT end_of_data.
**      15-May-2002 (hanal04) Bug 106640 INGSRV 1629
**          If the interrupt is interrupted we must ensure scd_dccont() 
**          returns an error or we will loop indefinitely.
**          SCS_STAR_INTERRUPT is used to flag scd_dccont() so that
**          it knows the caller is scs_dcinterrupt().
**          
[@history_template@]...
*/
DB_STATUS
scs_dcinterrupt(SCD_SCB  *scb,
		GCA_RV_PARMS **rv )
{
    DB_STATUS    status;
    DB_ERROR     error;
    SCC_SDC_CB   *dc;
    QEF_RCB	 *qef_rcb = scb->scb_sscb.sscb_qeccb;
    GCA_RV_PARMS tmp_rv = scb->scb_cscb.cscb_gce;
    if (scb->scb_sscb.sscb_interrupt == 0)
	return (E_DB_OK);

    scb->scb_cscb.cscb_gce.gca_status = E_SC1000_PROCESSED;

    *rv = NULL;
    if (scb->scb_sscb.sscb_dccb == NULL)
    {
	return (E_DB_OK);
    }

    dc = &scb->scb_sscb.sscb_dccb->dc_dcblk;
    *rv = (GCA_RV_PARMS *)dc->sdc_rv;

    qef_rcb->qef_r3_ddb_req.qer_d6_conn_info.qed_c4_exchange_p = (PTR)dc;

    scb->scb_sscb.sscb_flags |= SCS_STAR_INTERRUPT;
    status = scs_dccont(scb, SDC_WRITE, GCA_ATTENTION, (PTR)&tmp_rv);
    if (DB_FAILURE_MACRO(status))
    {
        scb->scb_sscb.sscb_flags &= ~SCS_STAR_INTERRUPT;
	return(status);
    }

    do
    {
        status = scs_dccont(scb, SDC_READ, 0, NULL);
        if (DB_FAILURE_MACRO(status))
        {
            scb->scb_sscb.sscb_flags &= ~SCS_STAR_INTERRUPT;
            return(status);
        }
        *rv = (GCA_RV_PARMS *)dc->sdc_rv;
    } while (((*rv)->gca_message_type != GCA_IACK) ||
         ((*rv)->gca_end_of_data != TRUE));

    scb->scb_sscb.sscb_flags &= ~SCS_STAR_INTERRUPT;
    return (E_DB_OK);
}

/*{
** Name: scs_dccont - Interface to qef_call(QED_C2_CONT).
**
** Description:
**	This routine provides an interface to qef_call(QED_C2_CONT). It should
**	be used exclusively in direct-connect processing. 
**
** Inputs:
**	scb	- Session control block
**	sdmode  - Direct Connect direction/mode
**	msgtype - Type of message to be sent, 0 if reading.
**	msg	- Pointer to message to be sent, NULL if reading.
**
** Outputs:
**
**    Returns:
**	E_DB_ERROR	- If error.
**	E_DB_OK		- If successful.
**
**    Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	20-Jan-1991 (fpang)
**	    Created for SYBIL.
**	2-Jul-1993 (daveb)
**	    prototyped.  Remove unused vars 'it' and 'end_status'.
**	12-Oct-2001 (jenjo02)
**	    Return status from qef_call() rather than E_DB_OK
**	    to prevent looping between scs_dcinterrupt() and
**	    here if the connection is lost, in which case
**	    GCA_IACK will never be received.
**      15-May-2002 (hanal04) Bug 106640 INGSRV 1629
**          If qef_call() fails while processing an interrupt
**          (SCS_STAR_INTERRUPT) we must return the failure status.
[@history_template@]...
*/
DB_STATUS
scs_dccont(SCD_SCB *scb,
            i4    sdmode,
            i4    msgtype,
            PTR   msg )
{
    SCC_SDC_CB   *dc;
    QEF_RCB	 *qe_ccb = scb->scb_sscb.sscb_qeccb;
    DB_STATUS	status;

    dc = &scb->scb_sscb.sscb_dccb->dc_dcblk;
    qe_ccb->qef_r3_ddb_req.qer_d6_conn_info.qed_c4_exchange_p = (PTR)dc;
    dc->sdc_mode = sdmode;
    dc->sdc_msg_type = msgtype;
    dc->sdc_client_msg = msg;
    scb->scb_sscb.sscb_cfac = DB_QEF_ID;
    status = qef_call(QED_C2_CONT, qe_ccb);
    scb->scb_sscb.sscb_cfac = 0;

    if ((DB_FAILURE_MACRO(status)) &&
            ((scb->scb_sscb.sscb_interrupt == 0) || 
	     (scb->scb_sscb.sscb_flags & SCS_STAR_INTERRUPT)))
    {
	/* QEF error, w/o interrupt */
        scs_qef_error( status, qe_ccb->error.err_code, DB_SCF_ID, scb );
    }
    return(status);
}

/*{
** Name: scs_dcendcpy - Terminate distributed copy processing.
**
** Description:
**	This routine provides distributed copy termination processing.
**	It first calls qef() to terminate the copy, then it frees up
**	any storage allocated during copy processing.
**
** Inputs:
**	scb	- Session control block
**
** Outputs:
**
**    Returns:
**	DB_STATUS
**
**    Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	20-Jan-1991 (fpang)
**	    Created for SYBIL.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	09-Sep-1993 (fpang)
**	    If scb->scb_sscb.sscb_dccb is NULL, DC control block has already
**	    been released, so don't continue. This usually happens if an
**	    interrupt occurs after COPY/DIRCON/DEI has already completed.
**	18-apr-1994 (fpang)
**	    Modified scs_dcendcpy() to call QEF with QED_A_COPY if interrupted,
**	    and QED_E_COPY otherwise.
**	    Fixes B62553, Star Copy. Conversion errors cause subsequent
**	    SQL statements to fail or act eratic.
[@history_template@]...
*/
DB_STATUS
scs_dcendcpy( SCD_SCB *scb )
{
    DB_STATUS status1;
    DB_STATUS status2;
    DB_STATUS status3;
    QEF_RCB	 *qe_ccb = scb->scb_sscb.sscb_qeccb;
    i4		 qef_op;

    if ( scb->scb_sscb.sscb_dccb == (SCS_DCCB *)NULL )
	return (E_DB_OK);

    scb->scb_sscb.sscb_cfac = DB_QEF_ID;
    qef_op = (scb->scb_sscb.sscb_interrupt != 0) ? QED_A_COPY : QED_E_COPY;
    status1 = qef_call( qef_op, qe_ccb );
    scb->scb_sscb.sscb_cfac = 0;
    if( DB_FAILURE_MACRO( status1 ))
    {
	scs_qef_error( status1, qe_ccb->error.err_code, DB_SCF_ID, scb );
    }

    if( scb->scb_sscb.sscb_dccb->dc_tdescmsg != NULL )
    {
	status2 = sc0m_deallocate(0, &scb->scb_sscb.sscb_dccb->dc_tdescmsg);
	if( status2 )
	{
	    sc0e_put( status2, 0, 0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
	    scd_note( E_DB_SEVERE, DB_SCF_ID );
	}
	scb->scb_sscb.sscb_dccb->dc_tdesc = NULL;
    }

    status3 = sc0m_deallocate(0, (PTR *)&scb->scb_sscb.sscb_dccb);
    if (status3)
    {
	sc0e_put(status2, 0, 0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0);
	scd_note( E_DB_SEVERE, DB_SCF_ID );
    }

    if( DB_FAILURE_MACRO( status1 ))
	return( status1 );

    if(  status2 || status3 )
	return( E_DB_SEVERE );

    return (E_DB_OK);
}

/*{
** Name: scs_dccnct - Sequence 'Direct Connect'
**
** Description:
**	This routine sequences Direct Connect. 
**	The basic algorithm is to either read or write to/from the LDB
**	and then return to the sequencer to send or receive client data.
**	This routine also takes care of Direct Connect initialization and
**	termination.
**
** Inputs:
**	scb		- session control block for this session
**	opcode		- sequencer opcode
**
** Outputs:
**
**	qry_status	- query status
**      next_op		- what the next operation should be
**	scb->scb_sscb.sscb_state 
**			- sequencer state,
**    Returns:
**	DB_STATUS	
**
**    Exceptions:
**	Will queue message to FE.
**
** Side Effects:
**	Will allocate memory.
**
** History:
**	20-Jan-1991 (fpang)
**	    Created for SYBIL.
**	04-Sep-1992 (fpang)
**	    Fixed interrupt handling.   
**	21-Oct-1992 (fpang)
**	    Set GCA_NEW_EFF_USER_MASK after direct connect and
**	    direct disconnect.
**	18-jan-1993 (fpang)
**	    1. Support new gca protocol 60 retproc protocol.
**	    2. Supplied some missing arguments to scd_note().
**	21-jan-1993 (fpang)
**	    Handle interop issues when FE and LDB are at different GCA
**	    protocol levels WRT gca_retproc. 
**	2-Jul-1993 (daveb)
**	    prototyped.  Use proper PTR for allocation
**	22-jul-1993 (fpang)
**	    Set the GCA_FLUSH_QRY_IDS_MASK bit in the response block after
**	    direct connect/disconnect to force the FE to drop procedure/query
**	    /cursor ids.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
[@history_template@]...
*/
DB_STATUS
scs_dccnct( SCD_SCB *scb,
            i4	*qry_status,
            i4	*next_op,
            i4	opcode )
{
    QEF_RCB	 *qe_ccb;
    DB_STATUS	 status = E_DB_OK;
    SCC_GCMSG	 *message;
    bool	 fe_released;
    GCA_RV_PARMS *rv;
    i4	 fe, peer;
    PTR		block;

    if (scb->scb_sscb.sscb_state == SCS_CONTINUE)
    {
	/* We just read 'direct connect'.
	**   1. Connect to LDB
	**   2. Set up reply with a GCA_RESPONSE
	**   3. sscb_state -> SCS_DCONNECT
	**   4. next_op    -> CS_EXCHANGE
	*/

	/* Set up qef_rcb for connection */
	qe_ccb = scb->scb_sscb.sscb_qeccb;
	qe_ccb->qef_r3_ddb_req.qer_d6_conn_info.qed_c5_tran_ok_b = FALSE;
	qe_ccb->qef_r3_ddb_req.qer_d4_qry_info.qed_q2_lang =
				      scb->scb_sscb.sscb_ics.ics_qlang;

	/* Don't continue if interrupted. */
	if (scb->scb_sscb.sscb_interrupt != 0)
	{
	    return( status );
	}

	/* Allocate a direct connect contol block */
	status = sc0m_allocate(SCU_MZERO_MASK,
			       (i4)sizeof(SCS_DCCB),
			       DB_SCF_ID, (PTR)SCS_MEM,
			       CV_C_CONST_MACRO('D','C','C','B'),
			       &block);
	scb->scb_sscb.sscb_dccb = (SCS_DCCB *)block;
	if (status)
	{
	    sc0e_put(status, 0, 0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
	    sc0e_uput(status, 0, 0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0);
	    scd_note(E_DB_SEVERE, DB_SCF_ID);
	    *qry_status = GCA_FAIL_MASK;
	    scb->scb_sscb.sscb_state = SCS_INPUT;
	    return( status );
	}

	/* 1. Connect to LDB */
	qe_ccb->qef_r3_ddb_req.qer_d6_conn_info.qed_c4_exchange_p = 
	    (PTR)&scb->scb_sscb.sscb_dccb->dc_dcblk;
	scb->scb_sscb.sscb_cfac = DB_QEF_ID;
	status = qef_call(QED_C1_CONN, qe_ccb);
	scb->scb_sscb.sscb_cfac = 0;
	if (status != E_DB_OK)
	{
	    /* Deallocate the direct connect control block */
	    status = sc0m_deallocate(0, (PTR *)&scb->scb_sscb.sscb_dccb);
	    if (status)
	    {
		sc0e_put(status, 0, 0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0);
		sc0e_uput(status, 0, 0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0);
	    }

	    *qry_status = GCA_FAIL_MASK;
	    scb->scb_sscb.sscb_state = SCS_INPUT;
	    return( E_DB_ERROR );
	}

	/* 2. Set up response block */
	scb->scb_sscb.sscb_cquery.cur_qprmcount = 0;
	scb->scb_sscb.sscb_cquery.cur_qparam = 0;
	scb->scb_sscb.sscb_cquery.cur_result = 
			GCA_NEW_EFF_USER_MASK | GCA_FLUSH_QRY_IDS_MASK;
	scb->scb_sscb.sscb_force_abort = 0; 
	message = &scb->scb_cscb.cscb_rspmsg;
	message->scg_mtype = scb->scb_sscb.sscb_rsptype;
	message->scg_next = (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext;
	message->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
	scb->scb_cscb.cscb_mnext.scg_prev->scg_next = message;
	scb->scb_cscb.cscb_mnext.scg_prev = message;
	message->scg_mask = SCG_NODEALLOC_MASK;
	message->scg_msize = sizeof(GCA_RE_DATA);
	scb->scb_cscb.cscb_rdata->gca_rqstatus = *qry_status;
	scb->scb_cscb.cscb_rdata->gca_rqstatus |= 
			GCA_NEW_EFF_USER_MASK | GCA_FLUSH_QRY_IDS_MASK;
	scb->scb_cscb.cscb_rdata->gca_rowcount =
	   scb->scb_sscb.sscb_cquery.cur_row_count;
	scb->scb_cscb.cscb_rdata->gca_rhistamp =
	   scb->scb_sscb.sscb_comstamp.db_tab_high_time;
	scb->scb_cscb.cscb_rdata->gca_rlostamp =
	   scb->scb_sscb.sscb_comstamp.db_tab_low_time;

	/* 3,4. Set next state and next_op. */
	scb->scb_sscb.sscb_state = SCS_DCONNECT;
	*next_op = CS_EXCHANGE;
	return( status );
    }
    /*
    ** We're in the middle of 'direct connect' processing.
    **   1.  Read from client.
    **   1.1 Check client message for 'direct disconnect'
    **	     or GCA_RELEASE. End direct connect processing if true.
    **   2.  Send message to LDB
    **   2.1 If partial message sent, get more input.
    **   3.  Read LDB message.
    **   3.1 If LDB message is a turn around message, read from
    **	     FE again.
    **   3.2 IF LDB message is a tuple descriptor or copy map,
    **       convert and save for tuples.
    **   4.  Queue LDB message to be sent to FE.
    **
    ** Interrupts :
    **   If FE interrupts, interrupt the LDB (scs_dcinterrupt())
    **   IACK back to FE will be handled below.
    **
    ** sscb_state -> SCS_TERMINATE, if FE sends GCA_RELEASE.
    **	          -> SCS_INPUT, if FE sends 'direct disconnect'
    **            -> SCS_DCONNECT, otherwise.
    **
    ** next_op    -> CS_TERMINATE, if FE SENDS GCA_RELEASE
    **	          -> CS_EXHANGE, if FE sends 'direct disconnect'
    **	             or LDB sends 'turn around' message
    **	          -> CS_INPUT, if partial FE message sent
    **	          -> CS_OUTPUT, if patial LDB message sent
    */

    /* ASSERT sscb_state = SCS_DCONNECT */
    if (scb->scb_sscb.sscb_state != SCS_DCONNECT)
    {
	sc0e_put(E_SC0024_INTERNAL_ERROR, 0, 0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0);
	scd_note(E_DB_SEVERE, DB_SCF_ID);
	return (E_DB_SEVERE);
    }

    /* Fill in qef_rcb */
    qe_ccb = scb->scb_sscb.sscb_qeccb;
    qe_ccb->qef_r3_ddb_req.qer_d4_qry_info.qed_q1_timeout = 0;
    qe_ccb->qef_r3_ddb_req.qer_d4_qry_info.qed_q2_lang =
	scb->scb_sscb.sscb_ics.ics_qlang;
    qe_ccb->qef_r3_ddb_req.qer_d6_conn_info.qed_c4_exchange_p = 
	(PTR)&scb->scb_sscb.sscb_dccb->dc_dcblk;
    qe_ccb->qef_r3_ddb_req.qer_d6_conn_info.qed_c5_tran_ok_b = FALSE;

    /* Check for interrupt. If interrupted, interrupt LDB, return
    ** to relay IACK to FE.
    */
    if (scb->scb_sscb.sscb_interrupt != 0)
    {
	status = scs_dcinterrupt(scb, &rv);
	if (DB_FAILURE_MACRO(status))
	{
	    sc0e_put( status, 0, 0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
	    scd_note( E_DB_SEVERE, DB_SCF_ID );
	    return( E_DB_SEVERE );
	}
	return( E_DB_OK );
    }

    if (opcode == CS_INPUT)
    {
	fe_released = FALSE;
	/* 1. Read message from FE. */
	status = scs_dcinput(scb, &rv);
	if (status != E_DB_OK)
	{
	    if (scb->scb_sscb.sscb_interrupt != 0)
	    {
		/*
		** Interrupted. Interrupt LDB.
		** IACK response will be sent below.
		*/
		status = scs_dcinterrupt(scb, &rv);
		if (DB_FAILURE_MACRO(status))
		{
		    sc0e_put( status, 0, 0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0 );
		    scd_note( E_DB_SEVERE, DB_SCF_ID );
		    return( E_DB_SEVERE );
		}
	    }
	    else if (status == E_GC0027_RQST_PURGED)
	    {
		*next_op = CS_INPUT;
		return( E_DB_OK );
	    }
	    else if (status == E_GC0001_ASSOC_FAIL)
	    {
		fe_released = TRUE;
	    }
	    else
	    {
		sc0e_put( status, 0, 0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0 );
		scd_note( E_DB_SEVERE, DB_SCF_ID );
		return( E_DB_SEVERE );
	    }
	}


	if ((scb->scb_sscb.sscb_dccb->dc_tdescmsg != NULL)
		&& ((rv->gca_message_type != GCA_ACK) &&
		    (rv->gca_message_type != GCA_CDATA) &&
		    (rv->gca_message_type != GCA_RESPONSE)))
	{
	    /* Dispose of tuple descriptor allocated in previous
	    ** communications.
	    */
	    status = sc0m_deallocate(0,
			   &scb->scb_sscb.sscb_dccb->dc_tdescmsg);
	    if (status)
	    {
		sc0e_put(status, 0, 0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0);
		scd_note(E_DB_SEVERE, DB_SCF_ID);
		return (E_DB_SEVERE);
	    }
	    scb->scb_sscb.sscb_dccb->dc_tdesc = NULL;
	}

	if (scb->scb_sscb.sscb_interrupt != 0)
	    return( E_DB_OK );

	if ((rv->gca_message_type == GCA_QUERY) ||
	    (rv->gca_message_type == GCA_RELEASE) ||
	    (fe_released == TRUE))
	{
	    /*
	    ** 1.1 Check for 'direct disconnect', and GCA_RELEASE.
	    */
	    if ((fe_released == TRUE) ||
		(rv->gca_message_type == GCA_RELEASE) ||
		(scs_dcmsg(rv)))
	    {
		/* FE sent 'direct disconnect' or RELEASE.
		** We are done. 
		*/
		scb->scb_sscb.sscb_cfac = DB_QEF_ID;
		status = qef_call(QED_C3_DISC, qe_ccb);

		scb->scb_sscb.sscb_cfac = 0;
		scb->scb_sscb.sscb_rsptype = GCA_RESPONSE;
		scb->scb_sscb.sscb_nostatements++;
		scb->scb_sscb.sscb_cquery.cur_result |= 
				GCA_NEW_EFF_USER_MASK | GCA_FLUSH_QRY_IDS_MASK;

		status = sc0m_deallocate(0, (PTR *)&scb->scb_sscb.sscb_dccb);
		if (status)
		{
		    sc0e_put(status, 0, 0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0);
		    sc0e_uput(status, 0, 0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0);
		    scd_note(E_DB_SEVERE, DB_SCF_ID);
		    return(E_DB_SEVERE);
		}

		if ((rv->gca_message_type == GCA_RELEASE) ||
		    (fe_released == TRUE))
		{
		    scb->scb_sscb.sscb_state = SCS_TERMINATE;
		    *next_op = CS_TERMINATE;
		}
		else
		{
		    scb->scb_sscb.sscb_state = SCS_INPUT;
		    *next_op = CS_EXCHANGE;
		}
		return( E_DB_OK );
	    }  
	}

	/* 2. Send message to ldb */
	status = scs_dccont(scb, SDC_WRITE, rv->gca_message_type, (PTR)rv);
	if ((DB_FAILURE_MACRO(status)) || (scb->scb_sscb.sscb_interrupt != 0))
	{
	    if (scb->scb_sscb.sscb_interrupt != 0)
		status = scs_dcinterrupt(scb, &rv);
	    if (DB_FAILURE_MACRO(status))
	    {
		sc0e_put( status, 0, 0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0);
		scd_note( E_DB_SEVERE, DB_SCF_ID );
		return( E_DB_SEVERE );
	    }
	    return( status );
	}

	if ( (rv->gca_end_of_data == FALSE) ||
	     ((rv->gca_message_type == GCA_ACK) &&
	      ((scb->scb_sscb.sscb_rsptype == GCA_C_FROM) ||
               (scb->scb_sscb.sscb_rsptype == GCA1_C_FROM))) ||
	     (rv->gca_message_type == GCA_CDATA))
	{
	    /* 2.1 Sent partial data, get more input from FE. */
	    scb->scb_sscb.sscb_state = SCS_DCONNECT;
	    *next_op = CS_INPUT;
	    return( E_DB_OK); /* Break to get more input */
	}
	/* Fall through to read from LDB */
    } /* CS_INPUT */

    /* 3. Read message from LDB */
    status = scs_dccont(scb, SDC_READ, 0, NULL);

    if ((DB_FAILURE_MACRO(status)) || (scb->scb_sscb.sscb_interrupt != 0))
    {
	if (scb->scb_sscb.sscb_interrupt != 0)
	    status = scs_dcinterrupt(scb, &rv);
	if (DB_FAILURE_MACRO(status))
	{
	    sc0e_put( status, 0, 0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
	    scd_note( E_DB_SEVERE, DB_SCF_ID );
	    return( E_DB_SEVERE );
	}
	return( E_DB_OK);
    }

    rv = (GCA_RV_PARMS *)scb->scb_sscb.sscb_dccb->dc_dcblk.sdc_rv;
    scb->scb_sscb.sscb_rsptype = rv->gca_message_type;
    fe = scb->scb_cscb.cscb_version;
    peer = scb->scb_sscb.sscb_dccb->dc_dcblk.sdc_peer_protocol;

    switch (rv->gca_message_type)
    {
	case GCA_RETPROC:
	    /*
	    **		    FE(<60)	   FE(>=60)
	    **		+-----------------+----------------+
	    ** LDB(<60)	| CS_EXCHANGE (1) | CS_EXCHANGE (2)|
	    **	        +-----------------+----------------+
	    ** LDB(>=60)| CS_EXCHANGE (3) | CS_OUTPUT   (4)|
	    **		+-----------------+----------------+
	    **
	    **  (1) - Return, INVPROC is done.
	    **  (2) - Add a fake RESPONSE to satisfy FE.
	    **	(3) - Read and discard LDB msgs until a RESPONSE.
	    **	(4) - Return RETPROC, and turn around on RESPONSE.
	    */
	    if ((fe == peer) && (fe >= GCA_PROTOCOL_LEVEL_60))
		*next_op = CS_OUTPUT;
	    else
		*next_op = CS_EXCHANGE;
	    break;
	case GCA_ACCEPT:
	case GCA_REJECT:
	case GCA_S_BTRAN:
	case GCA_S_ETRAN:
	case GCA_DONE:
	case GCA_REFUSE:
	case GCA_RESPONSE:
	    *next_op = CS_EXCHANGE;	/* Turn around message */
	    break;			/* Reverse direction */
	case GCA_C_FROM :
	case GCA_C_INTO :
	case GCA1_C_FROM :
	case GCA1_C_INTO :
	    *next_op = CS_EXCHANGE;	/* Get ACK from the FE */
	    break;
	case GCA_TDESCR :
	    *next_op = CS_OUTPUT;	/* Relay tuple desc to FE */
	    break;
	case GCA_MD_ASSOC:
	case GCA_QC_ID:
	case GCA_TUPLES:
	case GCA_CDATA:
	case GCA_ERROR:
	case GCA_TRACE:
	case GCA_EVENT:
	case GCA_ACK :
	    *next_op = CS_OUTPUT;	/* Allowable mid-stream msg */
	    break;			/* Relay to FE */
	case GCA_RELEASE :
	case GCA_NP_INTERRUPT:
	case GCA_ATTENTION:
	case GCA_IACK:
	default :
	    /* Protocol violation */
	    sc0e_put( E_SC0226_BAD_GCA_TYPE, 0, 1, 
		     sizeof( rv->gca_message_type ),
		     (PTR)&rv->gca_message_type,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
	    sc0e_uput( E_SC0226_BAD_GCA_TYPE, 0, 1, 
		       sizeof( rv->gca_message_type ),
		       (PTR)&rv->gca_message_type,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0);
	    sc0e_uput( E_SC0206_CANNOT_PROCESS, 0, 0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0);
	    return( E_DB_SEVERE );
    }

    if (rv->gca_message_type == GCA_TDESCR) 
	status = scs_dctdesc(scb);  /* Fetch LDB tuple descriptor */
    else if ((rv->gca_message_type == GCA_C_INTO) ||
	     (rv->gca_message_type == GCA_C_FROM) ||
	     (rv->gca_message_type == GCA1_C_INTO) ||
	     (rv->gca_message_type == GCA1_C_FROM))
	status = scs_dccpmap(scb);  /* Fetch LDB copy map */
    else
	status = scs_dcsend(scb, rv, &message, FALSE);  /* Relay msg */

    if ((rv->gca_message_type == GCA_RETPROC) && (peer != fe))
    {
	/* Convert between RETPROC protocols. 
	**
	** If ldb_protocol >= 60 and fe_protocol < 60
	**	Read and discard until RESPONSE (case 1)
	** elseif ldb_protocol < 60  and fe_protocol >= 60
	**	Queue fake RESPONSE (case 2)
	** 
	** See RETPROC comment above for more details.
	*/
	if (peer >= GCA_PROTOCOL_LEVEL_60)
	{
	    /* Read and discard until response. */
	    while (rv->gca_message_type != GCA_RESPONSE)
	    {
		status = scs_dccont(scb, SDC_READ, 0, NULL);
		if ((DB_FAILURE_MACRO(status)) || 
		    (scb->scb_sscb.sscb_interrupt != 0))
		{
		    if (scb->scb_sscb.sscb_interrupt != 0)
			break;
		    if (DB_FAILURE_MACRO(status))
		    {
			sc0e_put( status, 0, 0,
				 0, (PTR)0,
				 0, (PTR)0,
				 0, (PTR)0,
				 0, (PTR)0,
				 0, (PTR)0,
				 0, (PTR)0);
			scd_note( E_DB_SEVERE, DB_SCF_ID );
			return( E_DB_SEVERE );
		    }
		}
		rv = (GCA_RV_PARMS *)scb->scb_sscb.sscb_dccb->dc_dcblk.sdc_rv;
	    }
	}
	else
	{
	    /* Queue a fake response block to satisfy protocol */
	    GCA_RV_PARMS     itb;
	    GCA_RE_DATA	     res;

	    MEfill ( sizeof(GCA_RE_DATA), (unsigned char)0, &res );
	    res.gca_rqstatus  = GCA_OK_MASK;
	    res.gca_rowcount  = GCA_NO_ROW_COUNT;
	    res.gca_rhistamp  = scb->scb_sscb.sscb_comstamp.db_tab_high_time;
	    res.gca_rlostamp  = scb->scb_sscb.sscb_comstamp.db_tab_low_time;
	    itb.gca_message_type = GCA_RESPONSE;
	    itb.gca_data_area = (PTR)&res;
	    itb.gca_d_length = sizeof(GCA_RE_DATA);
	    itb.gca_end_of_data = TRUE;

	    status = scs_dcsend(scb, &itb, &message, TRUE);
	}
    }

    if (scb->scb_sscb.sscb_interrupt != 0)
	status = scs_dcinterrupt(scb, &rv);

    if (DB_FAILURE_MACRO(status)) 
    {
	 sc0e_put( status, 0, 0,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0);
	 sc0e_uput( status, 0, 0,
		   0, (PTR)0,
		   0, (PTR)0,
		   0, (PTR)0,
		   0, (PTR)0,
		   0, (PTR)0,
		   0, (PTR)0);
	 scd_note( E_DB_SEVERE, DB_SCF_ID );
	 return( E_DB_SEVERE );
    }

    return( status );   
}

/*{
** Name: scs_dccopy - Sequence distributed 'Copy Into/From;
**
** Description:
**	This routine sequences distributed Copy Into/From.
**	The basic algorithm is to either read or write to/from the LDB
**	and then return to the sequencer to send or receive client data.
**	This routine also takes care of distributed Copy initialization and
**	termination.
**
** Inputs:
**	scb		- session control block for this session
**	opcode		- sequencer opcode
**
** Outputs:
**
**	qry_status	- query status
**      next_op		- what the next operation should be
**	scb->scb_sscb.sscb_state 
**			- sequencer state,
**    Returns:
**	DB_STATUS	
**
**    Exceptions:
**	Will queue message to FE.
**
** Side Effects:
**	Will allocate memory.
**
** History:
**	20-Jan-1991 (fpang)
**	    Created for SYBIL.
**	04-sep-1992 (fpang)
**	    Support new messages GCA1_C_FROM/INTO.
**	27-oct-1992 (fpang)
**	    Make sure to check exit status of scs_dcendcpy(). Also,
**	    copy LDB response block before calling scs_dcendcpy() because
**	    ending COPY could potentially overwrite communication buffers.
**	2-Jul-1993 (daveb)
**	    prototyped, use proper PTR for allocation
**      15-nov-1994 (rudtr01)
**              modified scs_dccopy() to properly set the name, and node
**              of the LDB before attempting the COPY TABLE
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
[@history_template@]...
*/
DB_STATUS
scs_dccopy(SCD_SCB *scb,
	   i4  *qry_status,
	   i4  *next_op,
	   i4  opcode )
{
    QEF_RCB	 *qe_ccb;
    DB_STATUS	 status = E_DB_OK;
    SCC_GCMSG	 *message;
    GCA_RV_PARMS *rv;
    QSF_RCB	 *qs_ccb;
    PSQ_CB	 *ps_ccb = scb->scb_sscb.sscb_psccb;
    GCA_RE_DATA  *re;
    PTR		block;

    /* Fill in qef_rcb */
    qe_ccb = scb->scb_sscb.sscb_qeccb;
    qe_ccb->qef_modifier = QEF_SSTRAN;
    qe_ccb->qef_r3_ddb_req.qer_d4_qry_info.qed_q2_lang = DB_SQL;
    qe_ccb->qef_r3_ddb_req.qer_d4_qry_info.qed_q1_timeout = 0;
    qe_ccb->qef_r3_ddb_req.qer_d6_conn_info.qed_c5_tran_ok_b = FALSE;
    STRUCT_ASSIGN_MACRO(*ps_ccb->psq_ldbdesc,
			qe_ccb->qef_r3_ddb_req.qer_d6_conn_info.qed_c1_ldb);

    /* Copy is over if client sends an interrupt. */
    if (scb->scb_sscb.sscb_interrupt != 0)
    {
	status = scs_dcendcpy(scb);
	*qry_status = GCA_FAIL_MASK;
	*next_op = CS_INPUT;
	return( status );
    }

    if (scb->scb_sscb.sscb_state == SCS_CONTINUE)
    {
	/*
	** This is the beginning of distributed copy sequencing.
	** 1.  Retrieve the copy query from qsf.
	** 2.  Connect to the ldb.
	** 3.  Send query to ldb.
	** 3.1 Free up qsf space.
	** 4.  Read response from ldb 
	** 5.  Set sscb_state depending on ldb response.
	** 6.  Convert and save the copy map.
	**
	** LDB connection will/may be opened.
	** Memory allocated for copy map.
	**
	** Interrupts will be handled below with the rest of
	** the sequencer interrupt handling. Basically, interrupt
	** the LDB before responding with an IACK to FE.
	**
	** sscb_state : SCS_CP_INTO if 'copy into'
	**	        SCS_CP_FROM if 'copy from'
	**		SCS_CP_ERROR otherwise.
	**
	** next_op    : CS_EXCHANGE if internal error
	**	        CS_OUTPUT if sscb_state set to SCS_CP_ERROR.	
	*/

	/* 1. Get copy query from qsf. */
	scb->scb_sscb.sscb_nostatements++;
	qe_ccb->qef_r3_ddb_req.qer_d6_conn_info.qed_c5_tran_ok_b = TRUE;
	qs_ccb = scb->scb_sscb.sscb_qsccb;
	qs_ccb->qsf_obj_id.qso_handle = ps_ccb->psq_result.qso_handle;
	qs_ccb->qsf_type = QSFRB_CB;
	qs_ccb->qsf_ascii_id = QSFRB_ASCII_ID;
	qs_ccb->qsf_length = sizeof(QSF_RCB);
	qs_ccb->qsf_owner = (PTR)DB_PSF_ID;
	qs_ccb->qsf_lk_state = QSO_EXLOCK;
	status = qsf_call(QSO_LOCK, qs_ccb);
	if (DB_SUCCESS_MACRO(status))
	{
	    status = qsf_call(QSO_INFO, qs_ccb);
	}

	if (DB_FAILURE_MACRO(status))
	{
	    scs_qsf_error( status, qs_ccb->qsf_error.err_code,
			   E_SC0210_SCS_SQNCR_ERROR );
	    *qry_status = GCA_FAIL_MASK;
	    *next_op = CS_INPUT;
	    return( status );
	}

	/* Allocate a direct connect contol block */
	status = sc0m_allocate(SCU_MZERO_MASK,
			       (i4)sizeof(SCS_DCCB),
			       DB_SCF_ID, (PTR)SCS_MEM,
			       CV_C_CONST_MACRO('D','C','C','B'),
			       &block);
	scb->scb_sscb.sscb_dccb = (SCS_DCCB *)block;
	if (status)
	{
	    sc0e_put(status, 0, 0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
	    sc0e_uput(status, 0, 0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0);
	    scd_note(E_DB_SEVERE, DB_SCF_ID);
	    *qry_status = GCA_FAIL_MASK;
	    scb->scb_sscb.sscb_state = SCS_INPUT;
	    return( status );
	}

	qe_ccb->qef_r3_ddb_req.qer_d6_conn_info.qed_c4_exchange_p = 
	    (PTR)&scb->scb_sscb.sscb_dccb->dc_dcblk;

	/* 2. Connect to ldb. */
	scb->scb_sscb.sscb_cfac = DB_QEF_ID;
	status = qef_call(QED_B_COPY, qe_ccb);
	scb->scb_sscb.sscb_cfac = 0;
	if (DB_FAILURE_MACRO(status))
	{
	    scs_qef_error( status, qe_ccb->error.err_code, 
			   DB_SCF_ID, scb );
	    *qry_status = GCA_FAIL_MASK;
	    *next_op = CS_INPUT;
	    return( status );
	}

	/* 3. Send query to the LDB. */
	status = scs_dccont(scb, SDC_WRITE,
			    GCA_QUERY, (PTR)qs_ccb->qsf_root);
	if ((DB_FAILURE_MACRO(status)) ||
	    (scb->scb_sscb.sscb_interrupt != 0))
	{
	     status = scs_dcendcpy(scb);
	     *qry_status = GCA_FAIL_MASK;
	     *next_op = CS_INPUT;
	     return( status );
	}

	/* 3.1 Free up qsf space from copy query. */
	status = qsf_call(QSO_DESTROY, qs_ccb);
	if (DB_FAILURE_MACRO(status))
	{
	    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			  E_SC0210_SCS_SQNCR_ERROR);
	    *qry_status = GCA_FAIL_MASK;
	    *next_op = CS_INPUT;
	    return( status );
	}

	do
	{   /* 4. Read LDB response */
	    status = scs_dccont(scb, SDC_READ, 0, NULL);
	    if ((DB_FAILURE_MACRO(status)) ||
		(scb->scb_sscb.sscb_interrupt != 0))
	    {
		 break;
	    }

	    rv = (GCA_RV_PARMS *)scb->scb_sscb.sscb_dccb->dc_dcblk.sdc_rv;

	    if ((rv->gca_message_type != GCA_C_FROM) &&
		(rv->gca_message_type != GCA_C_INTO) &&
		(rv->gca_message_type != GCA1_C_INTO) &&
		(rv->gca_message_type != GCA1_C_FROM) &&
		(rv->gca_message_type != GCA_RESPONSE))
	    {
		status = scs_dcsend(scb, rv, &message, TRUE);
		if ((DB_FAILURE_MACRO(status)) ||
		    (scb->scb_sscb.sscb_interrupt != 0))
		{
		     break;
		}
	    }
	    else
	    {
		break;
	    }
	} while (1);
	
	if ((DB_FAILURE_MACRO(status)) ||
	    (scb->scb_sscb.sscb_interrupt != 0))
	{
	    if (DB_FAILURE_MACRO(status))
		status = scs_dcendcpy(scb);
	    *qry_status = GCA_FAIL_MASK;
	    *next_op = CS_INPUT;
	    return( status );
	}

	/* 5. Set the next state based on what LDB sent back. */
	if ((rv->gca_message_type == GCA_C_FROM) ||
	    (rv->gca_message_type == GCA1_C_FROM))
	    scb->scb_sscb.sscb_state = SCS_CP_FROM;
	else if ((rv->gca_message_type == GCA_C_INTO) ||
		 (rv->gca_message_type == GCA1_C_INTO))
	    scb->scb_sscb.sscb_state = SCS_CP_INTO;
	else 
	{
	    /* Build response block. LDB wont do copy. */
	    re = (GCA_RE_DATA *)rv->gca_data_area;
	    scb->scb_sscb.sscb_cquery.cur_row_count = re->gca_rowcount;
	    scb->scb_sscb.sscb_comstamp.db_tab_high_time = 
		re->gca_rlostamp;
	    scb->scb_sscb.sscb_comstamp.db_tab_low_time = 
		re->gca_rhistamp;
	    *qry_status = re->gca_rqstatus;
	    scb->scb_sscb.sscb_cquery.cur_result = 0;
	    scb->scb_sscb.sscb_rsptype = GCA_RESPONSE;
	    scb->scb_sscb.sscb_state = SCS_INPUT;
	    *next_op = CS_EXCHANGE;
	    status = scs_dcendcpy(scb);
	    return( status );
	}

	/* 6. Take care of CPMAP, and save tuple desriptor.  */
	status = scs_dccpmap(scb);

	if ((DB_FAILURE_MACRO(status)) ||
	    (scb->scb_sscb.sscb_interrupt != 0))
	{
	    if (DB_FAILURE_MACRO(status))
		status = scs_dcendcpy(scb);
	    *qry_status = GCA_FAIL_MASK;
	    *next_op = CS_INPUT;
	    return( status );
	}

	/* Set response block type, and next_op */
	scb->scb_sscb.sscb_rsptype = 
	    scb->scb_sscb.sscb_dccb->dc_dcblk.sdc_msg_type;
	*next_op = CS_EXCHANGE;
	return( status ); /* Unconditional */
    } /* (scb->scb_sscb.sscb_state == SCS_CONTINUE) */

    /* ASSERT */
    if ((scb->scb_sscb.sscb_state != SCS_CP_FROM) &&
	(scb->scb_sscb.sscb_state != SCS_CP_INTO) &&
	(scb->scb_sscb.sscb_state != SCS_CP_ERROR))
    {
	sc0e_put(E_SC0024_INTERNAL_ERROR, 0, 0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0);
	scd_note(E_DB_SEVERE, DB_SCF_ID);
	return (E_DB_SEVERE);
    }
    /*
    ** We are in the middle of distributed COPY processing.
    ** Actions are directed by sscb_state and opcode.
    ** The default processing is to read from the LDB, check
    ** for GCA_RESPONSE, and queue the message to be sent to the FE
    ** if the LDB message is not a GCA_RESPONSE. If the LDB message
    ** is a GCA_RESPONSE, end copy, and build a response block
    ** to end the copy with the FE.
    ** If the state is SCS_CP_FROM or SCS_CP_ERROR, the opcode
    ** will be CS_INPUT, which will cause input to be received
    ** from the FE, and relayed to the LDB. The FE message is
    ** monitored for GCA_RESPONSE, which turns the direction.
    ** Interrupts will be handled below with general interrupt
    ** processing, which has a special case to properly terminate
    ** a copy.
    **
    ** If opcode is CS_INPUT, 
    **     1.  Read FE message 
    **     1.1 Relay to LDB. 
    **     1.2 Sscb_state is SCS_CP_FROM, and FE did not send a
    **         a GCA_RESPONSE. Check if LDB is signalling an error.
    **     1.3 Sscb_state is SCS_CP_ERROR, and FE did not send a
    **	       GCA_RESPONSE, get more input.
    **
    **  sscb_state  -> SCS_CP_ERROR, if current state is SCS_CP_FROM
    **	               and LDB interrupts.
    **              -> no change otherwise.
    **
    **  next_op     -> CS_INPUT, if state is SCS_CP_FROM and 
    **                 FE did not send GCA_RESONSE.
    **              -> CS_OUTPUT, if state is SCS_CP_FROM and
    **                 FE sent a GCA_RESPONSE.
    **                 (this is the fall through case)
    **              -> CS_INPUT, in case of internal errors.
    **
    ** If opcode is not CS_INPUT (default)
    **    2.  Read from LDB
    **    3.  If LDB did not send a response, keep reading from LDB.
    **    4.  If LDB sends a response, copy is done.
    **    5.  Build a response block from LDB's to answer FE.
    **
    **  sscb_state  -> SCS_INPUT, when copy is finished.
    **		    -> no change otherwise
    **
    **  next_op     -> CS_OUPUT, if LDB does not send a RESPONSE.
    **	            -> CS_INPUT, if error, or copy is finished.
    **
    ** Interrupts will be handled below with the rest of
    ** the sequencer interrupt handling. Basically, interrupt
    ** the LDB before responding with an IACK to FE.
    */

    qe_ccb->qef_r3_ddb_req.qer_d6_conn_info.qed_c4_exchange_p = 
	    (PTR)&scb->scb_sscb.sscb_dccb->dc_dcblk;

    if (opcode == CS_INPUT)
    {
	/* 1. Get client message.  */
	status = scs_dcinput(scb, &rv);
	if (status != E_DB_OK)
	{
	    if (scb->scb_sscb.sscb_interrupt != 0)
	    {
		*qry_status = GCA_FAIL_MASK;
		*next_op = CS_INPUT;
		return( status );
	    }
	    else if (status == E_GC0001_ASSOC_FAIL)
	    {
		status = scs_dcendcpy(scb);
		*qry_status = GCA_FAIL_MASK;
		*next_op = CS_TERMINATE;
		scb->scb_sscb.sscb_state = SCS_TERMINATE;
		return( status );
	    }
	    else if (status != E_GC0027_RQST_PURGED)
	    {
		sc0e_put(status, 0, 0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0);
		sc0e_uput(status, 0, 0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0);
		scd_note(E_DB_SEVERE, DB_SCF_ID);
		status = scs_dcendcpy(scb);
		*qry_status = GCA_FAIL_MASK;
	    }
	    *next_op = CS_INPUT;
	    return( status );	/* Purged, break to get more FE input */
	}

	/* 1.1 Relay client message to LDB. */
	status = scs_dccont(scb, SDC_WRITE, rv->gca_message_type, (PTR)rv );
	if ((DB_FAILURE_MACRO(status)) ||
	    (scb->scb_sscb.sscb_interrupt != 0))
	{
	     status = scs_dcendcpy(scb);
	     *qry_status = GCA_FAIL_MASK;
	     *next_op = CS_INPUT;
	     return( status );
	}

	if ((scb->scb_sscb.sscb_state == SCS_CP_FROM) &&
	    (rv->gca_message_type != GCA_RESPONSE))
	{
	    /* 1.2 Check if LDB is signalling an error. */
	    status = scs_dccont(scb, SDC_CHK_INT, 0, NULL);
	    if ((DB_FAILURE_MACRO(status))  ||
		(scb->scb_sscb.sscb_interrupt != 0))
	    {
		status = scs_dcendcpy( scb);
		*qry_status = GCA_FAIL_MASK;
		*next_op = CS_INPUT;
		return( status );
	    }

	    if (scb->scb_sscb.sscb_dccb->dc_dcblk.sdc_ldb_status 
			== SDC_INTERRUPTED)
	    {
		/* LDB is signaling an error. */
		scs_cpinterrupt(scb);
		*qry_status = GCA_FAIL_MASK;
		scb->scb_sscb.sscb_state = SCS_CP_ERROR;
		*next_op = CS_INPUT;
		return( status );
	    }
	    *next_op = CS_INPUT;
	    return( status );  /* Break to get more input */
	}

	/* 1.3 If COPY error, turn around when FE sends a RESPONSE. */
	if ((scb->scb_sscb.sscb_state == SCS_CP_ERROR) &&
	    (rv->gca_message_type != GCA_RESPONSE))
	{
	    *next_op = CS_INPUT;
	    return( status );  /*Break to get more input */
	}
	/* Fall through to read from LDB */
    } /* (opcode == CS_INPUT) */

    /* 2. Read from LDB */
    status = scs_dccont(scb, SDC_READ, 0, NULL );
    if ((DB_FAILURE_MACRO(status)) ||
	(scb->scb_sscb.sscb_interrupt != 0))
    {
	 status = scs_dcendcpy(scb);
	 *qry_status = GCA_FAIL_MASK;
	 *next_op = CS_INPUT;
	 return( status );
    }

    rv = (GCA_RV_PARMS *)scb->scb_sscb.sscb_dccb->dc_dcblk.sdc_rv;

    if (rv->gca_message_type != GCA_RESPONSE) 
    {
	/* 3. LDB did not send a GCA_RESPONSE, keep reading. */
	status = scs_dcsend(scb, rv, &message, FALSE); /* Relay to FE */
	if (DB_FAILURE_MACRO(status))
	{
	    sc0e_put(status, 0, 0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
	    sc0e_uput(status, 0, 0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0);
	    scd_note(E_DB_SEVERE, DB_SCF_ID);
	    status = scs_dcendcpy(scb);
	    *qry_status = GCA_FAIL_MASK;
	    *next_op = CS_INPUT;
	    return( status );
	}
	scb->scb_sscb.sscb_rsptype = rv->gca_message_type;
	*next_op = CS_OUTPUT;
    } /* LDB reply not GCA_RESPONSE */
    else
    {
        /* 4. LDB replied with a GCA_RESPONSE, end COPY. */
	re = (GCA_RE_DATA *)rv->gca_data_area;	/* Build response block */
	scb->scb_sscb.sscb_cquery.cur_row_count = re->gca_rowcount;
	scb->scb_sscb.sscb_comstamp.db_tab_high_time = re->gca_rlostamp;
	scb->scb_sscb.sscb_comstamp.db_tab_low_time = re->gca_rhistamp;
	*qry_status = re->gca_rqstatus;
	scb->scb_sscb.sscb_cquery.cur_result = 0;
	scb->scb_sscb.sscb_rsptype = GCA_RESPONSE;
	scb->scb_sscb.sscb_state = SCS_INPUT;
	*next_op = CS_EXCHANGE;
	status = scs_dcendcpy(scb);
	if (DB_FAILURE_MACRO(status))
	{
	    *qry_status = GCA_FAIL_MASK;
	    *next_op = CS_INPUT;
	}
    }
    return( status );
}

/*{
** Name: scs_dcexec - Sequence 'Direct Execute Immediate'
**
** Description:
**	This routine sequences Direct Execute Immediate. 
**	The basic algorithm is to either read or write to/from the LDB
**	and then return to the sequencer to send or receive client data.
**	This routine also takes care of Direct Execute Immediate initialization
**	and termination.
**
** Inputs:
**	scb		- session control block for this session
**	opcode		- sequencer opcode
**
** Outputs:
**
**	qry_status	- query status
**      next_op		- what the next operation should be
**	scb->scb_sscb.sscb_state 
**			- sequencer state,
**    Returns:
**	DB_STATUS	
**
**    Exceptions:
**	Will queue message to FE.
**
** Side Effects:
**	Will allocate memory.
**
** History:
**	20-Jan-1991 (fpang)
**	    Created for SYBIL.
**	01-oct-1992 (fpang)
**	    Set qed_c5_tran_ok_b because DEI is ok in a transaction.
**	    Also fixed error handling.
**	27-oct-1992 (fpang)
**	    Don't set qed_c5_tran_ok_b to FALSE when reading from LDB.
**	18-jan-1993 (fpang)
**	    Support new gca protocol 60 retproc protocol.
**	20-jan-1993 (fpang)
**	    Undid previous. DEI is always dynamic execute procedure, never
**	    invproc, so DEI should never see a RETPROC,
**	2-Jul-1993 (daveb)
**	    prototyped, use proper PTR for allocation.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
[@history_template@]...
*/
DB_STATUS
scs_dcexec(SCD_SCB *scb,
	   i4  *qry_status,
	   i4  *next_op,
	   i4  opcode )
{
    QEF_RCB	 *qe_ccb;
    DB_STATUS	 status = E_DB_OK;
    SCC_GCMSG	 *message;
    GCA_RV_PARMS *rv;
    QSF_RCB	 *qs_ccb;
    PSQ_CB	 *ps_ccb = scb->scb_sscb.sscb_psccb;
    PTR		block;

    /*
    ** STAR Direct Execute Immediate
    **
    ** PSF has the ldb query in QSF memory as Query Text.
    ** Retrieve it and put it into a DD_PACKET. Send the query to
    ** the LDB. LDB may only return non-data type messages. 'Message'
    ** type messages are queued to be sent to the FE. 'Turn-around'
    ** type messages end DEI. If LDB returns 'data', interrupt the
    ** LDB. FE interrupts end DEI.
    **
    ** sscb_state -> SCS_INPUT;
    ** next_op    -> CS_EXCHANGE - if success
    **            -> CS_INPUT - if error.
    */

    scb->scb_sscb.sscb_nostatements++;
    *qry_status = 0;

    if (scb->scb_sscb.sscb_interrupt != 0)
	return( status );

    /* Fill in qef_rcb for connect. */
    qe_ccb = scb->scb_sscb.sscb_qeccb;
    qe_ccb->qef_modifier = QEF_SSTRAN;
    qe_ccb->qef_r3_ddb_req.qer_d6_conn_info.qed_c5_tran_ok_b = TRUE;
    qe_ccb->qef_r3_ddb_req.qer_d4_qry_info.qed_q2_lang = DB_SQL;
    qe_ccb->qef_r3_ddb_req.qer_d4_qry_info.qed_q1_timeout = 0;

    /* Extract the query from QSF handle. */
    qs_ccb = scb->scb_sscb.sscb_qsccb;
    qs_ccb->qsf_obj_id.qso_handle = ps_ccb->psq_result.qso_handle;
    qs_ccb->qsf_type = QSFRB_CB;
    qs_ccb->qsf_ascii_id = QSFRB_ASCII_ID;
    qs_ccb->qsf_length = sizeof(QSF_RCB);
    qs_ccb->qsf_owner = (PTR)DB_PSF_ID;
    qs_ccb->qsf_lk_state = QSO_EXLOCK;
    status = qsf_call(QSO_LOCK, qs_ccb);
    if (DB_SUCCESS_MACRO(status))
    {
	status = qsf_call(QSO_INFO, qs_ccb);
    }

    if (DB_FAILURE_MACRO(status))
    {
	scs_qsf_error( status, qs_ccb->qsf_error.err_code,
		       E_SC0210_SCS_SQNCR_ERROR );
	*qry_status = GCA_FAIL_MASK;
	*next_op = CS_INPUT;
	return( status );
    }

    /* Allocate a direct connect contol block */
    status = sc0m_allocate(SCU_MZERO_MASK,
			   (i4)sizeof(SCS_DCCB),
			   DB_SCF_ID, (PTR)SCS_MEM,  
			   CV_C_CONST_MACRO('D','C','C','B'),
			   &block);
    scb->scb_sscb.sscb_dccb = (SCS_DCCB *)block;
    if (status)
    {
	sc0e_put(status, 0, 0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0);
	sc0e_uput(status, 0, 0,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0);
	scd_note(E_DB_SEVERE, DB_SCF_ID);
	*qry_status = GCA_FAIL_MASK;
	scb->scb_sscb.sscb_state = SCS_INPUT;
	return( status );
    }

    qe_ccb->qef_r3_ddb_req.qer_d6_conn_info.qed_c4_exchange_p = 
	    (PTR)&scb->scb_sscb.sscb_dccb->dc_dcblk;

    /* Connect to ldb if we have to */
    scb->scb_sscb.sscb_cfac = DB_QEF_ID;
    status = qef_call(QED_C1_CONN, qe_ccb);
    scb->scb_sscb.sscb_cfac = 0;
    if (DB_FAILURE_MACRO(status))
    {
	/* So we don't try to disconnect below. */
	qe_ccb->qef_r3_ddb_req.qer_d6_conn_info.qed_c4_exchange_p =
	    NULL;
	*qry_status = GCA_FAIL_MASK;
	*next_op = CS_INPUT;
    }
    else
    {
	/* Connect succeeded. Send qry to ldb as a DD_PACKET. */
	DD_PACKET  ldb_qry;
	ldb_qry.dd_p1_len = 
	    ((PSQ_QDESC *)qs_ccb->qsf_root)->psq_qrysize;
	ldb_qry.dd_p2_pkt_p = 
	    ((PSQ_QDESC *)qs_ccb->qsf_root)->psq_qrytext;
	ldb_qry.dd_p3_nxt_p = NULL;
	qe_ccb->qef_r3_ddb_req.qer_d6_conn_info.qed_c5_tran_ok_b = TRUE;
	status = scs_dccont(scb, SDC_WRITE, GCA_QUERY, (PTR)&ldb_qry);

        if ( (DB_FAILURE_MACRO(status)) &&
	    (scb->scb_sscb.sscb_interrupt == 0))
        {
	    /* LDB failed query, find out why. */
	    scs_qef_error( status, qe_ccb->error.err_code, DB_SCF_ID, scb );
	    scb->scb_sscb.sscb_cfac = DB_QEF_ID;
	    status = qef_call( QED_C3_DISC, qe_ccb );
	    scb->scb_sscb.sscb_cfac = 0;
	    if( DB_FAILURE_MACRO(status) )
	    {
	        scs_qef_error( status, qe_ccb->error.err_code,
			       DB_SCF_ID, scb );
	    }
	    *qry_status = GCA_FAIL_MASK;
	    *next_op = CS_INPUT;
        }
        else if (scb->scb_sscb.sscb_interrupt != 0)
        {
	    /* Interrupted, interrupt LDB, end DEI, and let general
	    ** interrupt processing below handle the IACK.
	    */
	    status = scs_dcinterrupt(scb, &rv);
	    if (DB_FAILURE_MACRO(status))
	    {
	        sc0e_put(status, 0, 0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0);
	        sc0e_uput(status, 0, 0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0);
	        scd_note(E_DB_SEVERE, DB_SCF_ID);
	    }
	    *qry_status = GCA_FAIL_MASK;
	    *next_op = CS_INPUT;
        }
    }

    /* Free qsf space. */
    status = qsf_call(QSO_DESTROY, qs_ccb);
    if (DB_FAILURE_MACRO(status))
    {
	scs_qsf_error(status, qs_ccb->qsf_error.err_code,
		      E_SC0210_SCS_SQNCR_ERROR);
	*qry_status = GCA_FAIL_MASK;
    }

    if (*qry_status != 0)
    {
	/* Error occured above, disconnect, DEI is done. */
	status = scs_dcinterrupt(scb, &rv);
	if (DB_FAILURE_MACRO(status))
	{
	    sc0e_put(status, 0, 0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
	    sc0e_uput(status, 0, 0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0);
	    scd_note(E_DB_SEVERE, DB_SCF_ID);
	}

	if (qe_ccb->qef_r3_ddb_req.qer_d6_conn_info.qed_c4_exchange_p 
	    != (PTR) NULL)
	{
	    /* Need to disconnect from ldb */
	    scb->scb_sscb.sscb_cfac = DB_QEF_ID;
	    status = qef_call( QED_C3_DISC, qe_ccb );
	    scb->scb_sscb.sscb_cfac = 0;
	    if( DB_FAILURE_MACRO(status) )
	    {
		scs_qef_error( status, qe_ccb->error.err_code,
			       DB_SCF_ID, scb );
	    }
	}

	status = sc0m_deallocate(0, (PTR *)&scb->scb_sscb.sscb_dccb);
	if (status)
	{
	    sc0e_put(status, 0, 0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
	    sc0e_uput(status, 0, 0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0);
	    scd_note(E_DB_SEVERE, DB_SCF_ID);
	    return(E_DB_SEVERE);
	}

	return( E_DB_ERROR );	/* Unconditional, DEI has aborted */
    } /* (*qry_status != 0) */

    /* Get LDB reply. */
    *next_op = CS_OUTPUT;
    do {
	/* Read from LDB until a turn around. Abort if data returned */
	status = scs_dccont(scb, SDC_READ, 0, NULL);
	if ((DB_FAILURE_MACRO(status)) ||
	    (scb->scb_sscb.sscb_interrupt != 0))
	{
	    if (scb->scb_sscb.sscb_interrupt != 0)
	    {
		/* Interrupted. See comments above. */
		status = scs_dcinterrupt(scb, &rv);
		if (DB_FAILURE_MACRO(status))
		{
		    sc0e_put(status, 0, 0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0);
		    sc0e_uput(status, 0, 0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0);
		    scd_note(E_DB_SEVERE, DB_SCF_ID);
		}
	    }
	    *qry_status = GCA_FAIL_MASK;
	    *next_op = CS_INPUT;
	    return( status );
	}

	rv = (GCA_RV_PARMS *)scb->scb_sscb.sscb_dccb->dc_dcblk.sdc_rv;

	if ((rv->gca_message_type ==  GCA_S_ETRAN) ||
	    (rv->gca_message_type ==  GCA_DONE) ||
	    (rv->gca_message_type ==  GCA_REFUSE) ||
	    (rv->gca_message_type ==  GCA_RESPONSE))
	{
	    /* Turn around messages, DEI is done. */
	    /* Build a response block. */
	    GCA_RE_DATA *re;
	    re = (GCA_RE_DATA *)rv->gca_data_area;
	    scb->scb_sscb.sscb_cquery.cur_row_count =
		re->gca_rowcount;
	    scb->scb_sscb.sscb_comstamp.db_tab_high_time =
		re->gca_rlostamp;
	    scb->scb_sscb.sscb_comstamp.db_tab_low_time =
		re->gca_rhistamp;
	    *qry_status = re->gca_rqstatus;
	    scb->scb_sscb.sscb_cquery.cur_result = 0;
	    scb->scb_sscb.sscb_rsptype = rv->gca_message_type;
	    scb->scb_sscb.sscb_state = SCS_INPUT;
	    *next_op = CS_EXCHANGE;
	    continue;
	}
	else if ((rv->gca_message_type == GCA_TRACE) ||
		 (rv->gca_message_type == GCA_ERROR) ||
		 (rv->gca_message_type == GCA_EVENT))
	{
	    /* 
	    ** Allowable mid stream messages, queue them to be sent to
	    ** FE and get more from LDB
	    */
	    *next_op = CS_OUTPUT;
	}
	else
	{
	    /* These are illegal. DEI cannot return data. */
	    *qry_status = GCA_FAIL_MASK;
	    sc0e_put(E_SC0226_BAD_GCA_TYPE, 0, 0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
	    sc0e_uput(E_SC0226_BAD_GCA_TYPE, 0, 0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0);
	    /* Interrupt LDB to stop data. Ignore status */
	    status = scs_dcinterrupt(scb, &rv);
	    *next_op = CS_INPUT;
	    continue;
	} 

	/* Queue the message */
	status = scs_dcsend(scb, rv, &message, TRUE);
	if (DB_FAILURE_MACRO(status))
	{
	    sc0e_put(status, 0, 0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
	    sc0e_uput(status, 0, 0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0);
	    scd_note(E_DB_SEVERE, DB_SCF_ID);
	    *next_op = CS_INPUT;
	}
    } while (*next_op == CS_OUTPUT);

    /* Disconnect from ldb */
    scb->scb_sscb.sscb_cfac = DB_QEF_ID;
    status = qef_call(QED_C3_DISC, qe_ccb);
    scb->scb_sscb.sscb_cfac = 0;
    if (DB_FAILURE_MACRO(status))
    {
	scs_qef_error( status, qe_ccb->error.err_code, DB_SCF_ID, scb );
	*qry_status = GCA_FAIL_MASK;
    }

    status = sc0m_deallocate(0, (PTR *)&scb->scb_sscb.sscb_dccb);
    if (status)
    {
	sc0e_put(status, 0, 0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0);
	sc0e_uput(status, 0, 0,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0);
	scd_note(E_DB_SEVERE, DB_SCF_ID);
	return(E_DB_SEVERE);
    }
    return( status );	
}

/*{
** Name: scs_dcxproc - Sequence Dynamic 'Execute Procedure'
**
** Description:
**	This routine sequences Dynamic Execute Procedure.
**	The basic algorithm is to either read or write to/from the LDB
**	and then return to the sequencer to send or receive client data.
**	This routine also takes care of Dynamic Execute Procedure initialization
**	and termination.
**	Since Ingres 64 database procedures cannot be registered, there is 
**	no conversion situation with respect to RETPROC protocol.  
**
** Inputs:
**	scb		- session control block for this session
**	opcode		- sequencer opcode
**
** Outputs:
**
**	qry_status	- query status
**      next_op		- what the next operation should be
**	scb->scb_sscb.sscb_state 
**			- sequencer state,
**    Returns:
**	DB_STATUS	
**
**    Exceptions:
**	Will queue message to FE.
**
** Side Effects:
**	Will allocate memory.
**
** History:
**	08-Jan-1993 (fpang)
**	    Created for SYBIL.
**	20-Jan-1993 (fpang)
**	    1. Fixed some erroneous comments.
**	    2. Fixed message processing code. Removed some errors
**	       that could never have happend.
**	2-Jul-1993 (daveb)
**	    prototyped, use proper PTR for allocation.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
@history_template@]...
*/
DB_STATUS
scs_dcxproc(SCD_SCB *scb,
	    i4  *qry_status,
	    i4  *next_op,
	    i4  opcode )
{
    QEF_RCB	 *qe_ccb;
    DB_STATUS	 status = E_DB_OK;
    SCC_GCMSG	 *message;
    GCA_RV_PARMS *rv;
    QSF_RCB	 *qs_ccb;
    PSQ_CB	 *ps_ccb = scb->scb_sscb.sscb_psccb;
    PTR		block;
    /*
    ** STAR Direct Execute Procedure
    **
    ** PSF has put the execute procedure query in QSF memory as Query Text.
    ** Retrieve it and put it into a DD_PACKET. Send the query to
    ** the LDB. Queue LDB messages (user messages and raised errors).
    ** Execution is complete when the LDB returns a RESPONSE.
    ** FE interrupts terminate procedure execution.
    **
    ** sscb_state -> SCS_INPUT;
    ** next_op    -> CS_EXCHANGE - if success
    **            -> CS_INPUT - if error.
    */

    scb->scb_sscb.sscb_nostatements++;
    *qry_status = 0;

    if (scb->scb_sscb.sscb_interrupt != 0)
	return( status );

    /* Fill in qef_rcb for connect. */
    qe_ccb = scb->scb_sscb.sscb_qeccb;
    qe_ccb->qef_modifier = QEF_SSTRAN;
    qe_ccb->qef_r3_ddb_req.qer_d6_conn_info.qed_c5_tran_ok_b = TRUE;
    qe_ccb->qef_r3_ddb_req.qer_d4_qry_info.qed_q2_lang = DB_SQL;
    qe_ccb->qef_r3_ddb_req.qer_d4_qry_info.qed_q1_timeout = 0;

    /* Extract the query from QSF handle. */
    qs_ccb = scb->scb_sscb.sscb_qsccb;
    qs_ccb->qsf_obj_id.qso_handle = ps_ccb->psq_result.qso_handle;
    qs_ccb->qsf_type = QSFRB_CB;
    qs_ccb->qsf_ascii_id = QSFRB_ASCII_ID;
    qs_ccb->qsf_length = sizeof(QSF_RCB);
    qs_ccb->qsf_owner = (PTR)DB_PSF_ID;
    qs_ccb->qsf_lk_state = QSO_EXLOCK;
    status = qsf_call(QSO_LOCK, qs_ccb);
    if (DB_SUCCESS_MACRO(status))
    {
	status = qsf_call(QSO_INFO, qs_ccb);
    }

    if (DB_FAILURE_MACRO(status))
    {
	scs_qsf_error( status, qs_ccb->qsf_error.err_code,
		       E_SC0210_SCS_SQNCR_ERROR );
	*qry_status = GCA_FAIL_MASK;
	*next_op = CS_INPUT;
	return( status );
    }

    /* Allocate a direct connect contol block */
    status = sc0m_allocate(SCU_MZERO_MASK,
			   (i4)sizeof(SCS_DCCB),
			   DB_SCF_ID, (PTR)SCS_MEM,
			   CV_C_CONST_MACRO('D','C','C','B'),
			   &block);
    scb->scb_sscb.sscb_dccb = (SCS_DCCB *)block;
    if (status)
    {
	sc0e_put(status, 0, 0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0);
	sc0e_uput(status, 0, 0,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0);
	scd_note(E_DB_SEVERE, DB_SCF_ID);
	*qry_status = GCA_FAIL_MASK;
	scb->scb_sscb.sscb_state = SCS_INPUT;
	return( status );
    }

    qe_ccb->qef_r3_ddb_req.qer_d6_conn_info.qed_c4_exchange_p = 
	    (PTR)&scb->scb_sscb.sscb_dccb->dc_dcblk;

    /* Connect to ldb if we have to */
    scb->scb_sscb.sscb_cfac = DB_QEF_ID;
    status = qef_call(QED_C1_CONN, qe_ccb);
    scb->scb_sscb.sscb_cfac = 0;
    if (DB_FAILURE_MACRO(status))
    {
	/* So we don't try to disconnect below. */
	qe_ccb->qef_r3_ddb_req.qer_d6_conn_info.qed_c4_exchange_p =
	    NULL;
	*qry_status = GCA_FAIL_MASK;
	*next_op = CS_INPUT;
    }
    else
    {
	/* Connect succeeded. Send qry to ldb as a DD_PACKET. */
	qe_ccb->qef_r3_ddb_req.qer_d6_conn_info.qed_c5_tran_ok_b = TRUE;
	status = scs_dccont(scb, SDC_WRITE, GCA_QUERY, (PTR)qs_ccb->qsf_root);

        if ( (DB_FAILURE_MACRO(status)) &&
	    (scb->scb_sscb.sscb_interrupt == 0))
        {
	    /* LDB failed query, find out why. */
	    scs_qef_error( status, qe_ccb->error.err_code, DB_SCF_ID, scb );
	    scb->scb_sscb.sscb_cfac = DB_QEF_ID;
	    status = qef_call( QED_C3_DISC, qe_ccb );
	    scb->scb_sscb.sscb_cfac = 0;
	    if( DB_FAILURE_MACRO(status) )
	    {
	        scs_qef_error( status, qe_ccb->error.err_code,
			       DB_SCF_ID, scb );
	    }
	    *qry_status = GCA_FAIL_MASK;
	    *next_op = CS_INPUT;
        }
        else if (scb->scb_sscb.sscb_interrupt != 0)
        {
	    /* Interrupted. Interrupt LDB end execute procedure, 
	    ** and let general interrupt processing below handle the IACK.
	    */
	    status = scs_dcinterrupt(scb, &rv);
	    if (DB_FAILURE_MACRO(status))
	    {
	        sc0e_put(status, 0, 0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0);
	        sc0e_uput(status, 0, 0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0);
	        scd_note(E_DB_SEVERE, DB_SCF_ID);
	    }
	    *qry_status = GCA_FAIL_MASK;
	    *next_op = CS_INPUT;
        }
    }

    /* Free qsf space. */
    status = qsf_call(QSO_DESTROY, qs_ccb);
    if (DB_FAILURE_MACRO(status))
    {
	scs_qsf_error(status, qs_ccb->qsf_error.err_code,
		      E_SC0210_SCS_SQNCR_ERROR);
	*qry_status = GCA_FAIL_MASK;
    }

    if (*qry_status != 0)
    {
	/* Error occured above, disconnect, execute procedure is done. */
	status = scs_dcinterrupt(scb, &rv);
	if (DB_FAILURE_MACRO(status))
	{
	    sc0e_put(status, 0, 0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
	    sc0e_uput(status, 0, 0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0);
	    scd_note(E_DB_SEVERE, DB_SCF_ID);
	}

	if (qe_ccb->qef_r3_ddb_req.qer_d6_conn_info.qed_c4_exchange_p 
	    != (PTR) NULL)
	{
	    /* Need to disconnect from ldb */
	    scb->scb_sscb.sscb_cfac = DB_QEF_ID;
	    status = qef_call( QED_C3_DISC, qe_ccb );
	    scb->scb_sscb.sscb_cfac = 0;
	    if( DB_FAILURE_MACRO(status) )
	    {
		scs_qef_error( status, qe_ccb->error.err_code,
			       DB_SCF_ID, scb );
	    }
	}

	status = sc0m_deallocate(0, (PTR *)&scb->scb_sscb.sscb_dccb);
	if (status)
	{
	    sc0e_put(status, 0, 0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
	    sc0e_uput(status, 0, 0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0);
	    scd_note(E_DB_SEVERE, DB_SCF_ID);
	    return(E_DB_SEVERE);
	}

	return( E_DB_ERROR );	/* Unconditional, Execute procedure aborted. */
    } /* (*qry_status != 0) */


    /* Get LDB reply. */
    *next_op = CS_OUTPUT;
    do {
	/* Read from LDB until a RESPONSE. */
	status = scs_dccont(scb, SDC_READ, 0, NULL);
	if ((DB_FAILURE_MACRO(status)) ||
	    (scb->scb_sscb.sscb_interrupt != 0))
	{
	    if (scb->scb_sscb.sscb_interrupt != 0)
	    {
		/* Interrupted. See comments above. */
		status = scs_dcinterrupt(scb, &rv);
		if (DB_FAILURE_MACRO(status))
		{
		    sc0e_put(status, 0, 0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0);
		    sc0e_uput(status, 0, 0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0);
		    scd_note(E_DB_SEVERE, DB_SCF_ID);
		}
	    }
	    *qry_status = GCA_FAIL_MASK;
	    *next_op = CS_INPUT;
	    return( status );
	}

	rv = (GCA_RV_PARMS *)scb->scb_sscb.sscb_dccb->dc_dcblk.sdc_rv;

	if (rv->gca_message_type ==  GCA_RESPONSE) 
	{
	    /* Turn around, dynamic execution is complete. */
	    /* Fill in, so sequencer can return the response block. */
	    GCA_RE_DATA *re;
	    re = (GCA_RE_DATA *)rv->gca_data_area;
	    scb->scb_sscb.sscb_cquery.cur_row_count =
		re->gca_rowcount;
	    scb->scb_sscb.sscb_comstamp.db_tab_high_time =
		re->gca_rlostamp;
	    scb->scb_sscb.sscb_comstamp.db_tab_low_time =
		re->gca_rhistamp;
	    *qry_status = re->gca_rqstatus;
	    scb->scb_sscb.sscb_cquery.cur_result = 0;
	    scb->scb_sscb.sscb_rsptype = rv->gca_message_type;
	    scb->scb_sscb.sscb_state = SCS_INPUT;
	    *next_op = CS_EXCHANGE;
	}
	else if ((rv->gca_message_type == GCA_TRACE) ||
		 (rv->gca_message_type == GCA_ERROR) ||
		 (rv->gca_message_type == GCA_EVENT))
	{
	    /* User message or raised error. Just queue it. */
	    status = scs_dcsend(scb, rv, &message, TRUE);
	    if (DB_FAILURE_MACRO(status))
	    {
		sc0e_put(status, 0, 0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0);
		sc0e_uput(status, 0, 0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0);
		scd_note(E_DB_SEVERE, DB_SCF_ID);
		*next_op = CS_INPUT;
	    }
	    *next_op = CS_OUTPUT;
	}
	else
	{
	    /* These are illegal messages. */
	    *qry_status = GCA_FAIL_MASK;
	    sc0e_put(E_SC0226_BAD_GCA_TYPE, 0, 0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
	    sc0e_uput(E_SC0226_BAD_GCA_TYPE, 0, 0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0);
	    /* Interrupt LDB to stop data. Ignore status */
	    status = scs_dcinterrupt(scb, &rv);
	    *next_op = CS_INPUT;
	} 
    } while (*next_op == CS_OUTPUT);

    /* Disconnect from ldb */
    scb->scb_sscb.sscb_cfac = DB_QEF_ID;
    status = qef_call(QED_C3_DISC, qe_ccb);
    scb->scb_sscb.sscb_cfac = 0;
    if (DB_FAILURE_MACRO(status))
    {
	scs_qef_error( status, qe_ccb->error.err_code, DB_SCF_ID, scb );
	*qry_status = GCA_FAIL_MASK;
    }

    status = sc0m_deallocate(0, (PTR *)&scb->scb_sscb.sscb_dccb);
    if (status)
    {
	sc0e_put(status, 0, 0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0);
	sc0e_uput(status, 0, 0,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0);
	scd_note(E_DB_SEVERE, DB_SCF_ID);
	return(E_DB_SEVERE);
    }
    return( status );	
}
