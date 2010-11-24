/*
**Copyright (c) 2004, 2010 Ingres Corporation
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
#include    <ade.h>
#include    <adfops.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefqeu.h>
#include    <me.h>
#include    <st.h>
#include    <ex.h>
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
#include    <opckey.h>
#include    <opcd.h>
#include    <opxlint.h>
#include    <opclint.h>
/**
**
**  Name: OPCDQP.C - Routines for filling in the non-QEF_AHD part of 
**			the QEF_QP_CB for distributed. From opcqp.c.
**
**  Description:
**      This file contains the routines required to initialize and/or fill 
**	in a query plan.
**
**	External Routines (visible outside this file):
**          opc_idqp_init() - Begin building a QEF_QP_CB
**
**	Internal Routines:
**
**
**  History:
**      02-sep-88 (robin)
**          Created from opcqp.c
**      21-oct-88 (robin)
**          Modified for QEF hdr changes
**	24-jan-91 (stec)
**	    General cleanup of spacing, identation, lines shortened to 80 chars.
**	    Changed code to make certain that BT*() routines use initialized
**	    size of bitmaps, when applicable.
**      26-mar-91 (fpang)
**          Renamed opdistributed.h to opdstrib.h
**	24-mar-92 (barbara)
**	    Fixed bug 42625 in opc_idqp_init().  Store tablids along with
**	    repeat query plan so that PSF may fully validate shareability
**	    of query plans.
**	17-dec-92 (ed)
**          remove common code with local OPC, as part of merging
**          of local and distributed OPC
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**      06-mar-96 (nanpr01)
**          initialize qeq_q14_page_size field.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
void opc_dahd(
	OPS_STATE *global,
	QEF_AHD **daction);
void opc_qrybld(
	OPS_STATE *global,
	DB_LANG *qlang,
	bool retflag,
	QEQ_TXT_SEG *qtext,
	DD_LDB_DESC *qldb,
	QEQ_D1_QRY *qryptr);
i4 opc_tstsingle(
	OPS_STATE *global);

/*{
** Name: opc_dahd   	- Allocate a distributed action header
**
** Description:
**      Allocate a distributed action header, add it to the 
**	action list, and return a pointer to it so that the caller
**	can fill in details of the action type.
[@comment_line@]...
**
** Inputs:
**      global                          Ptr to global state variable
[@PARAM_DESCR@]...
**
** Outputs:
**      daction	                        Updated with address of new distributed
**					action header.
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    [@description_or_none@]
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
[@history_template@]...
*/
VOID
opc_dahd(
	OPS_STATE          *global,
	QEF_AHD		    **daction )
{
    QEF_AHD	    *actptr;
    QEF_AHD	    *newact;

    actptr = global->ops_cstate.opc_qp->qp_ahd;
    if ( actptr != NULL )
    {
	for	( ;actptr->ahd_next != NULL; actptr = actptr->ahd_next );
    }

	/*  Allocate a new distributed action header */

    newact = (QEF_AHD *) opu_qsfmem( global, sizeof (QEF_AHD));
    newact->ahd_next = NULL;    

    *daction = newact;
    global->ops_cstate.opc_qp->qp_ahd_cnt += 1;

    if (actptr != NULL )
	actptr->ahd_next = newact;
    else
	global->ops_cstate.opc_qp->qp_ahd = newact;
    return;

}

/*{
** Name: opc_qrybld	- Fill in a QEQ_D1_QRY data structure
**
** Description:
[@comment_line@]...
**
** Inputs:
**      global                          Ptr to global state variable
**      qlang                           Query language
**      retflag				TRUE if query is retrieval
**      qtext                           Query text string(s)
**      qldb                            LDB where query will execute
**	qryaddr				QEQ_D1_QRY to fill in
[@PARAM_DESCR@]...
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
**      21-sep-88 (robin)
**          Created.
**      21-oct-88 (robin)
**          Modified for QEF hdr changes
**      06-mar-96 (nanpr01)
**          initialize qeq_q14_page_size field.
[@history_template@]...
*/
VOID
opc_qrybld(
	OPS_STATE   *global,
	DB_LANG	    *qlang,
	bool	    retflag,
	QEQ_TXT_SEG *qtext,
	DD_LDB_DESC *qldb,
	QEQ_D1_QRY  *qryptr )
{


    qryptr->qeq_q1_lang = *qlang;
    qryptr->qeq_q2_quantum = 0;
    qryptr->qeq_q3_ctl_info = 0;
    qryptr->qeq_q3_read_b = retflag;
    qryptr->qeq_q4_seg_p = qtext;
    qryptr->qeq_q5_ldb_p = qldb;
    qryptr->qeq_q6_col_cnt = 0;
    qryptr->qeq_q7_col_pp = NULL;
    qryptr->qeq_q8_dv_cnt = 0;
    qryptr->qeq_q9_dv_p = (DB_DATA_VALUE *)NULL;
    qryptr->qeq_q10_agg_size = 0;
    qryptr->qeq_q11_dv_offset = 0;
    qryptr->qeq_q12_qid_first = TRUE;
    qryptr->qeq_q13_csr_handle = (PTR) NULL; 
    qryptr->qeq_q14_page_size = DB_MIN_PAGESIZE; 
    return;	

}

/*{
** Name:	- opc_tstsingle
**
** Description:
**      Test whether a query is single site or not.  Return TRUE if single
**	site, FALSE otherwise. This is abstracted into a routine because it
**	is used in several places.
** Inputs:
**      global	    Global query state
**
** Outputs:
**	Returns:
**	    A i4  set to TRUE if single site, FALSE otherwise
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      17-jan-89 (robin)
**	    created
[@history_template@]...
*/
i4
opc_tstsingle(
	OPS_STATE   *global )
{
    if (global->ops_statement != NULL)
    {
    	if ( 
    	    (( global->ops_qheader->pst_distr->pst_stype == PST_0SITE ) 
    	    || ( global->ops_qheader->pst_distr->pst_stype == PST_1SITE )) &&
    	    (( global->ops_qheader->pst_distr->pst_qtext != NULL )
    	    &&  (global->ops_qheader->pst_distr->pst_ldb_desc != NULL ))
    	   )
    	{ 
	    return ((i4) TRUE);
	}
    }
    return ((i4) FALSE);
}
