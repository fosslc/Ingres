/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <ade.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmucb.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <psfparse.h>
#include    <qefmain.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <qefqeu.h>
#include    <scf.h>
#include    <qefscb.h>

#include    <dmmcb.h>
#include    <ex.h>
#include    <tm.h>

#include    <dudbms.h>
#include    <qeuqcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <sxf.h>
#include    <qefprotos.h>
/*
**
** Name: QEAEXECIMM.C - routine to execute the QEA_EXEC_IMM action header.
**
** Description:
** 	The routine in this module executes the QEA_EXEC_IMM action header.
**	qea_scontext() and qea_cobj() routines also process the
**      QEA_CREATE_INTEGRITY & QEA_CREATE_VIEW (with check option) actions. The
**      query text and PST_INFO will be passed in by qea_createIntegrity() &
**      qea_createview(). The text for the procedure will be processed first
**      and then the the text for the rule will be processed.
**
**	It saves the execution context and returns to SCF so that the 
**	query text for the action can be parsed.
**
**	qea_xcimm() - execute the QEA_EXEC_IMM/the rules and procedures
**			for the QEA_CREATE_INTEGRITY/QEA_CREATE_VIEW actions
**
**	qea_sontext() - save the current execution context
**
**      qea_cobj()- create a QSF object and set its root to point to
**                     DB_TEXT_STRING for the query text in QEA_EXEC_IMM/
**                     QEA_CREATE_INTEGRITY/QEA_CREATE_VIEW (WCO) action
**
**      qea_chkerror() - Check for errors returned by SCF. SCF will call QEF
**                      in case of error to clean up resources knowing that
**                      QEF has a saved environment.
**	
**	qea_dobj() - Destroy the QSF E.I. query text object of CREATE 
**		     TABLE/CREATE VIEW/GRANT in CREATE SCHEMA and the
**	             query text objects of rules/procedures/indexes for
**		     constraints and WCO.
**
**	qea_dsh() - Find the DSH for the following actions: 
**		    QEA_CREATE_TABLE/QEA_CREATE_VIEW in CREATE SCHEMA and
**		    QEA_GET for Alter Table Add Constraint.
**
** History:
**	30-nov-92 (anitap)
**	    Written.
**      18-dec-92 (anitap)
**          Reorganised qea_execimm() to make it more modular. Added three new
**          routines qea_savecontext(), qea_chkerror() and qea_cqsfobj().
**	03-feb-93 (anitap)
**	    Casting in qea_cqsfobj() to get rid of compiler warnings. Also
**	    set qsf_root to qsf_piece in qea_cqsfobj().
**	16-feb-93 (rickh)
**	    Added extra argument to qeq_dsh to agree with its new calling
**	    sequence.
**	04-mar-93 (anitap)
**	    Changed names of the functions to conform to standards. Fixed 
**	    compiler warning in qea_cobj().
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	24-jun-93 (anitap)
**	    Made changes in qea_cobj(). The query text object was being 
**	    unlocked before it was destroyed.
**	07-jul-93 (anitap)
**	    Added comment in qea_cobj().
**	    Moved call to qea_chkerror() from qea_scontext() to qea_xcimm() as
**	    it will be called only the first time.
**	    Added new arguments to qea_cobj() and qea_chkerror().
**	    Added new routine qea_dobj() to destroy E.I. query text objects
**	    after the E.I. objects get created.
**      13-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	06-aug-93 (anitap)
**	    Modularised some code in qea_xcimm() into qea_dsh() so that
**	    it can be called by the FIPS constraints project for Alter
**	    Table Add Constraint.
**	25-aug-93 (rickh)
**	    Added flags argument to qea_chkerror.
**	28-sep-93 (anitap)
**	    Added declaration for static function qea_dobj().
**      04-apr-94 (anitap)
**          Remember to close the ULM memory stream that we opened for E.I.
**          query text. Changes in qea_cobj() and qea_chkerror(). If we do not
**          close the memory stream, we will run out of QEF memory.
**	    Also return status on error in qea_dobj() and qea_xcimm() -- error
**	    code cleanup.
**	    Changed error message in qea_dsh().
**	13-jun-96 (nick)
**	    LINT directed changes.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	19-Aug-1997 (jenjo02)
**	    Distinguish between private and shared ULM streams.
**	    Piggyback ulm_palloc() calls with ulm_openstream() where appropriate.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Supply session's SID to QSF in qsf_sid.
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety.
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
**      28-May-2008 (gefei01)
**          Prototype change for qeq_dsh().
*/

/* static functions */

static DB_STATUS
qea_dobj(
        QEF_RCB         *qef_rcb,
        QSF_RCB         *qsf_rb);

/*{
** Name: QEA_XCIMM		- execute QEA_EXEC_IMM type of action
**
** Description:
**	qea_xcimm() will save the current context of the query plan like
**	the action header and QEF_RCB. It will then do qsf_call() to create a
**	QSF object for the query text. It will then call qsf_root() to
**	set the root of the object. QEF will have to pass the handle to the
**	QSF object (query text) to SCF. PSF expects a QSF handle to the 
**	object. 
**	
**	qea_xcimm() will set qef_rcb->qef_intstate and the error code in 
**	QEF_RCB to indicate to SCF that PSF needs to be called to parse the 
**	query text.
**
**	If an error occurs outside QEF while processing the QEA_EXEC_IMM action,
**	qea_xcimm() will be called back with the QEF_RCB intact. If an error
**	occurs while processing rules/procedures for the QEA_CREATE_INTEGRITY
**	action, qea_createIntegrity() will call back qea_scontext() with the
**	QEF_RCB intact. qea_scontext() will give error. Similarly for 
**	QEA_CREATE_VIEW (WCO).
**
**	After the first QEA_EXEC_IMM action is executed, qea_xcimm() will 
**	start processing the next QEA_EXEC_IMM action.
**
** Inputs:
**	action			Current action:
**	   .ahd_type		The action type:
**			    	   QEA_EXEC_IMM
**	.ahd_text		Pointer to query text to be parsed
**	.ahd_info		Pointer to PST_INFO structure having info for
**				accurate error reporting
**
** Outputs:
**	qef_rcb
**	    .error.err_code	one of the following
**				E_QE0303_PARSE_EXECIMM_QTEXT - Parse the qtext
**					for QEA_EXEC_IMM type of action
**				E_QE0000_OK
**	Returns:
**	    E_DB_{OK,WARN,ERROR,FATAL}
** 	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	30-nov-92 (anitap)
**	    Written for CREATE SCHEMA project.
**      18-dec-92 (anitap)
**          Reorganised the code to make the functions qea_savecontext(),
**          qea_chkerror() and qea_cqsfobj() accessible to qea_createIntegrity()
**          and the CREATE VIEW (WCO) project.
**	16-feb-93 (rickh)
**	    Added extra argument to qeq_dsh to agree with its new calling
**	    sequence.
**	04-mar-93 (anitap)
**	    Changed name of function to qea_xcimm() from qea_execimm() to 
**	    conform to coding standards.
**	07-jul-93 (anitap)
**	    Call qea_scontext() only for the first time and qea_chkerror() on 
**	    subsequent passes.
**	    Added new arguments to qea_chkerror() and qea_cobj().
**	    Added missing parenthesis for qea_chkerror() call.
**	06-aug-93 (anitap)
**	    Added call to new routine qea_dsh().
**	04-apr-94 (anitap)
**	    Return status instead of breaking after return from qea_chkerror().
*/
DB_STATUS
qea_xcimm(
	QEF_AHD		*act,
	QEF_RCB		*qef_rcb,
	QEE_DSH		*dsh,
	DB_TEXT_STRING	*text,
	PST_INFO	*qea_info)
{
    DB_STATUS		status = E_DB_OK;
    PST_INFO		*pst_info;
    QSF_RCB		qsf_rb;	

    for (;;)
    {
	/*
        ** Call qea_scontext() to check if we got an error while processing
        ** QEA_EXEC_IMM action. We will also save the context of the current
        ** execution.
        */
	/* Call qea_scontext() only on first pass to save context.
	** Call qea_chkerror() only on the subsequent pass to check for
        ** errors.
        */

	if ((qef_rcb->qef_intstate & QEF_EXEIMM_PROC) == 0)
           qea_scontext(qef_rcb, dsh, act);
	else 
	{  
	   if ( (status = qea_chkerror(dsh, &qsf_rb,
		 QEA_CHK_DESTROY_TEXT_OBJECT | QEA_CHK_LOOK_FOR_ERRORS )
	        ) != E_DB_OK )
	      return(status);
	}

        /* Check that this is the first time (not called back after the      */
	/* query text has been parsed for the DDL statement in CREATE        */
	/* SCHEMA). If first time, we continue executing and return to SCF to */		        
	/* parse the query text				     		     */
	/* The text for the QEA_EXEC_IMM type of action will not be in QSF.  */

	if ((qef_rcb->qef_intstate & QEF_EXEIMM_PROC) == 0)
	{
	   /* Get the id of the query text to pass on to SCF */

           /* Create a QSF object for the query text in QEA_EXEC_IMM action/ */
           /* query text for rule/procedure for constriants and views(WCO)   */

           status = qea_cobj(qef_rcb, dsh, qea_info, text, &qsf_rb);

           if (status != E_DB_OK)
           {
              return(status);
           }
	   break;
	}
	else
	{
	   /* Will be called back for two cases:			    */

	   /* 1. This code will be re-entered after the QEA_EXEC_IMM   	    */	
	   /*    action has been executed and the QP for the CREATE TABLE/  */
	   /*    ALTER TABLE/CREATE VIEW has been built. For this case,     */
	   /*    we need to continue processing the QP for the ALTER TABLE/ */
	   /*	 CREATE TABLE/CREATE VIEW.							    */

	   /* 2. Called back from SCF after a GRANT has been    	    */
	   /*    processed. At this point, we need to continue with the     */
	   /*    next action in the main QP of CREATE SCHEMA.     	    */

	   /* FIXME: For post 6.5, when the qrymod statements are put into  */
	   /* query plans, they will also be treated the same way as CREATE */
	   /* TABLE.							    */

	   /* We need to continue with the next QEA_EXEC_IMM action in the  */
	   /* QP if the previous QEA_EXEC_IMM action header was compiled    */ 
	   /* into a list of QEA_EXEC_IMM action headers.		    */

	   /* The query plan was found. Make sure the "EXEIMM_PROC" bit is  */
	   /* not on for the scope of this execution. It may get turned     */
	   /* back on if the QP has QEA_EXEC_IMM actions. But then we will  */
	   /* re-enter here (to try again) and turn it off.		    */
	   qef_rcb->qef_intstate &= ~QEF_EXEIMM_PROC;

	   pst_info = (PST_INFO *)qef_rcb->qef_info;	

           if (pst_info->pst_basemode == PSQ_GRANT)
	      break;
	}
	status = qea_dsh(qef_rcb, dsh, qef_rcb->qef_cb);
        if (status != E_DB_OK)
        {
           return(status);
        }
	break;
    }
    return(status);
}

/*{
** Name: QEA_SCONTEXT                - save the current execution context
**
** Description:
**      This routine is called back if a request to parse a QEA_EXEC_IMM type of
**      action fails. In this case we just contiue with error processing.
**      No need for another error as EXEIMM_CLEAN is only set if a client on
**      the outside issued an error knowing we have a saved environment.
**
**      This routine will be called by qea_xcimm(), qea_createIntegrity() and
**      qea_createview() for saving the context of current execution of
**      the query plan.
**
**      qea_scontext() is called by qea_createIntegrity() to save the current
**      execution context before returning to SCF so that the query text for
**      rule/procedure for constraint is parsed. qea_scontext()is called by
**      qea_xcimm() to save the current execution context of the QP for
**      CREATE SCHEMA before returning to SCF so that PSF can parse the query
**      text for the QEA_EXEC_IMM action.
**
**      The processing for qea_cview() is similar to qea_createIntegrity().
**
** Inputs:
**      qef_cb                 Session Control Block having
**                             the current execution context
**      action                 Current action:
**         .ahd_atype           The action type:
**                               QEA_EXEC_IMM
**                               QEA_CREATE_INTEGRITY
**                               QEA_CREATE_VIEW
**         .ahd_text           Pointer to query text to be parsed
**         .ahd_info           Pointer to PST_INFO structure having info for
**                             accurate error reporting
**
** Outputs:
**     qef_rcb
**         .error.err_code     E_QE0000_OK
**      Returns:
**         E_DB_{OK,WARN,ERROR,FATAL}
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      18-dec-92 (anitap)
**          Written for CREATE SCHEMA, FIPS consraints and CREATE VIEW (WCO)
**          projects.
**	07-jul-93 (anitap)
**	    Moved call to qea_chkerror() to qea_xcimm() as it will be called
**	    only the first time.
*/
void
qea_scontext(
        QEF_RCB         *qef_rcb,
        QEE_DSH         *dsh,
        QEF_AHD         *action)
{
       /* Actually execute a QEA_EXEC_IMM action. */
       /* We save the current execution context, and continue with the */
       /* next QEA_EXEC_IMM action. */

        /* Save the current execution context as represented by */
        /* QEF_RCB and the current DSH. */
        STRUCT_ASSIGN_MACRO(*qef_rcb, *dsh->dsh_exeimm_rcb);
        dsh->dsh_act_ptr = action;
}

/*{
** Name: QEA_CHKERROR                   - check for any errors encountered
**                                        while processing Execute Immediate
**                                        type of actions.
**
** Description:
**      This routine is called by qea_xcimm() to check for any errors
**      which occured with the previous E.I. processing i.e., if a request to
**      parse a QEA_EXEC_IMM type of action fails. In this case we just
**      continue with error processing. No need for another error as
**      EXEIMM_CLEAN is only set if a client on the outside issued an error
**      knowing we have a saved environment.
**
**      It is also called by qea_createIntegrity() after it finishes processing
**      the rules and prcedures via E.I. logic. This routine is called before
**      qea_createntegrity() starts appending tuples into IIDBDEPENDS catalog.
**
** Input:
**      qef_rcb                 QEF Request Control Block
**         .qef_intstate        QEF_EXEIMM_CLEAN
**         .error               will be set to E_QE0025_USER_ERROR
**
** Outputs:
**      qef_rcb
**         .error.err_code      E_QE0025_USER_ERROR
**      Returns:
**         E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
*          none
**
** History:
**      31-dec-92 (anitap)
**          Written for CREATE SCHEMA, FIPS constraints and CREATE VIEW (WCO)
**	    projects.	
**	22-jul-93 (anitap)
**	    Added call to new routine qea_dobj() to destroy E.I. query text 
**	    objects after the E.I. objects are created.
**	25-aug-93 (rickh)
**	    Added flags argument to qea_chkerror.
**	04-apr-94 (anitap)
**          We have remembered the streamid, etc. of the ULM stream that we
**          opened for the E.I. query text. Close the stream after destroying
**          the query text. Return proper error code in qef_rcb if error in
**	    qea_dobj().	
*/
DB_STATUS
qea_chkerror(
        QEE_DSH         *dsh,
	QSF_RCB		*qsf_rb,
	i4		flags )
{
        GLOBALREF QEF_S_CB  *Qef_s_cb;
	QEF_CB		*qef_cb = dsh->dsh_qefcb;
	QEF_RCB		*qef_rcb = qef_cb->qef_rcb;
        DB_STATUS           status = E_DB_OK;
	ULM_RCB		    ulm_rcb;

	for(;;)
	{

	   if ( flags & QEA_CHK_DESTROY_TEXT_OBJECT )
	   {
	      /* Destroy query text object */	
	      status = qea_dobj(qef_rcb, qsf_rb);
	      if (status != E_DB_OK)
	      {
		 dsh->dsh_error.err_code = qsf_rb->qsf_error.err_code;
		 return(status);
	      }

              if (qef_rcb->ulm_streamid != (PTR) NULL)
              {
      	         /* release the allocated memory */
		 STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm_rcb);		
		 ulm_rcb.ulm_streamid_p = &qef_rcb->ulm_streamid;
	
	         ulm_rcb.ulm_facility = DB_QEF_ID;
	         ulm_rcb.ulm_memleft = qef_rcb->ulm_memleft;

      	         status = ulm_closestream(&ulm_rcb);
		 if (status != E_DB_OK)
		 {
		    return(status);
		 }	
		 /* ULM has nullified qef_rcb->ulm_streamid */
              }
	   }

	   if ( flags & QEA_CHK_LOOK_FOR_ERRORS )
	   {
               if ((qef_rcb->qef_intstate & QEF_EXEIMM_CLEAN) != 0)
               {
                  dsh->dsh_error.err_code = E_QE0025_USER_ERROR;
	          qef_rcb->qef_intstate &= ~QEF_EXEIMM_CLEAN;
                  status = E_DB_ERROR;
		  return(status);
               }
	   }

	   break;
	}
        return (status);
}

/*{
** Name: QEA_COBJ	       - Create the QSF object for the query text
**                               in QEA_EXEC_IMM header/query text for rules/
**                               procedures/indexes for constraints and views 
**			         (WCO).
**
** Description:
**     This routine creates a QSF object for the query text in the QEA_EXEC_IMM
**     action header/query text for rules/procedures/indexes in the 
**     QEA_CREATE_INTEGRITY action header and QEA_CREATE_VIEW (WCO).
**
**     The query text needs to be parsed. PSF expects the query text to reside
**     in QSF.
**
**     This routine will create a QSF object for the query text and set the
**     root of the object to DB_TEXT_STRING. The PST_INFO is also copied over
**     to QEF_RCB to pass on to PSF. It also sets qef_instate to
**     QEF_EXEIMM_PROC so that we can be called back to clean up in case of
**     errors.
**
**     This routine will be called by qea_xcimm() for CREATE SCHEMA project,
**     qea_createIntegrity() for FIPS constraints projct and qea_createview()
**     for CREATE VIEW (WCO) project.
**
** Inputs:
**      qef_rcb                Request Control Block
**        .qef_qsf_handle      needs to be set to qso_handle in QSO_OBID
**        .qef_info            PST_INFO structure needs to be copied over
**        .qef_intstate        Should be set to QEF_EXEIMM_PROC
**        .error               err_code will be set to
**                             E_QE0303_PARSE_EXECIMM_QTEXT
**      act                    Current action:
**                             QEA_EXEC_IMM
**                             QEA_CREATE_INTEGRITY
**                             QEA_CREATE_VIEW
**      qea_info               PST_INFO struct to be copied into QEF_RCB
**      text                   which needs to be stored in QSF
**      qef_cb                 QEF control block
**        .qef_dsh              DSH stack for Execute Immediate
** Outputs:
**      qef_rcb
**          .error.err_code     one of the following
**                             E_QE0303_PARSE_EXECIMM_QTEXT - Parse the qtext
**                                     for QEA_EXEC_IMM type of action
**                             E_QE0000_OK
**      Returns:
**          E_DB_{OK,WARN,ERROR,FATAL}
**      Exceptions:
**         none
**
** Side Effects:
**         none
**
** History:
**      18-dec-92 (anitap)
**         Written for CREATE SCHEMA, FIPS constraints and CREATE VIEW (WCO)
**         projects.
**	03-feb-93 (anitap)
**	    Casting to get rid of compiler warnings. Set root of query text to
**	    qsf_piece.
**      03-mar-93 (andre)
**          we were not allocating enough memory - replace
**          sizeof(DB_TEXT_STRING) with sizeof(PSQ_QDESC) + 2 (to account for
**          that trailing blank and EOS)
**	04-mar-93 (anitap)
**	    Added return statement at the end. Fixed compiler warning. 
**	24-jun-93 (anitap)
**	    Removed call to qsf for unlocking the query text object. Was getting
**	    the following error in errlog.log: "E_QS0012_OBJ_NOT_LOCKED   
**	    QSF object is not locked by you."
**	    Also made changes to call ulm_closestream() on error.
**	07-jul-93 (anitap)
**	    Added comment about pointer arithmetic.
**	    Added new argument QSF_RCB so that it can be passed on back to
**	    QEF after E.I. processing for destroying the E.I. query text 
**	    object.
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for qsf_owner which has changed type to PTR.
**      04-apr-94 (anitap)
**          We want to close the ULM stream that we allocate here. So, remember
**          the streamid, amount of memory left, etc. in QEF_RCB through E.I.
**          processing.
**	10-Jan-2001 (jenjo02)
**	    Init ulm_streamid to null so we don't inadvertantly
**	    close whatever stream may be in Qef_s_cb->qef_d_ulmcb!
*/
DB_STATUS
qea_cobj(
        QEF_RCB         *qef_rcb,
	QEE_DSH		*dsh,
        PST_INFO        *qea_info,
        DB_TEXT_STRING  *text,
	QSF_RCB		*qsf_rb)
{
    DB_STATUS           status = E_DB_OK;
    i4             error;
    GLOBALREF QEF_S_CB  *Qef_s_cb;
    PSQ_QDESC		*qdesc;
    ULM_RCB		ulm;

    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm);
    ulm.ulm_streamid = (PTR) NULL;

    for (;;)
    {
	/* Open stream and allocate QSF_RCB with one action */
	ulm.ulm_flags = ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;
        ulm.ulm_psize = sizeof(QSF_RCB);
	
        if ((status = qec_mopen(&ulm)) != E_DB_OK)
        {
           status = E_DB_ERROR;
           error = E_QE001E_NO_MEM;
           qef_error( error, 0L, status, &error, &dsh->dsh_error, 0 );
           break;
        }

	qsf_rb = (QSF_RCB *) ulm.ulm_pptr;

        /* Initialize the QSF control block */
        qsf_rb->qsf_type = QSFRB_CB;
        qsf_rb->qsf_ascii_id = QSFRB_ASCII_ID;
        qsf_rb->qsf_length = sizeof(QSF_RCB);
        qsf_rb->qsf_owner = (PTR)DB_QEF_ID;
	qsf_rb->qsf_sid = qef_rcb->qef_cb->qef_ses_id;
        qsf_rb->qsf_obj_id.qso_type = QSO_QTEXT_OBJ;
        qsf_rb->qsf_obj_id.qso_lname = 0;

        /* Get the id of the query text to pass on to SCF */

        /* Place the text in QSF bcause the parser expects it to be there */
        status = qsf_call(QSO_CREATE, qsf_rb);

        if (DB_FAILURE_MACRO(status))
        {
           qef_error(qsf_rb->qsf_error.err_code, 0L, E_DB_ERROR,
                       &error, &dsh->dsh_error, 0) ;
	   break;
        }

        /* Set the id of the text in QEF_RCB so that we can call PSF  */
        /* to parse the query text.                                   */
	qef_rcb->qsf_rcb = qsf_rb;
        qef_rcb->qef_qsf_handle.qso_handle = qsf_rb->qsf_obj_id.qso_handle;
        qef_rcb->qef_qsf_handle.qso_type = QSO_QTEXT_OBJ;
        qef_rcb->qef_qsf_handle.qso_lname = 0;
	qef_rcb->qef_lk_id = qsf_rb->qsf_lk_id;

	/* copy ulm.ulm_streamid into qef_rcb->ulm_streamid
        ** in QEF_RCB so that we can deallocate the memory stream 
	** after we are done with E.I. processing.
	*/

	qef_rcb->ulm_streamid = ulm.ulm_streamid;
	qef_rcb->ulm_memleft = ulm.ulm_memleft;

        /* Allocate enough memory for the query descriptor plus the
        ** length of the text. 
        */
        qsf_rb->qsf_sz_piece = sizeof(PSQ_QDESC) + text->db_t_count + 2;

        status = qsf_call(QSO_PALLOC, qsf_rb);

        if (DB_FAILURE_MACRO(status))
        {
           qef_error(qsf_rb->qsf_error.err_code, 0L, E_DB_ERROR,
                     &error, &dsh->dsh_error, 0);
	   break;
        }

        qdesc = (PSQ_QDESC *) qsf_rb->qsf_piece;

        /* initialize PSQ_QDESC */
        qdesc->psq_qrysize = text->db_t_count + 1; /* 1 for trailing blank */
        qdesc->psq_datasize = 0;
        qdesc->psq_dnum = 0;

	/* Pointer Arithmetic */
        qdesc->psq_qrytext = (char *)(qdesc + 1);

        qdesc->psq_qrydata = (DB_DATA_VALUE **) NULL; 

        MEcopy((PTR) text->db_t_text, text->db_t_count,
		(PTR) qdesc->psq_qrytext);

        /* null terminate the string */
        {
	  char *p;
	  p = qdesc->psq_qrytext;
	  p += text->db_t_count;
          *p++ = ' ';
	  *p = '\0';
        }

        /* set the root */
        qsf_rb->qsf_root = qsf_rb->qsf_piece;

        status = qsf_call(QSO_SETROOT, qsf_rb);

        if (DB_FAILURE_MACRO(status))
        {
           qef_error(qsf_rb->qsf_error.err_code, 0L, E_DB_ERROR,
                     &error, &dsh->dsh_error, 0);
	   break;
        }

        /* copy over the PST_INFO structure to QEF_RCB */
        qef_rcb->qef_info = (PTR) qea_info;

        /* Ask SCF to parse the query text for this QEA_EXEC_IMM action
        ** header. Tell SCF to call us back even if the text cannot be
        ** parsed. We will need to clean up.
        */
        qef_rcb->qef_intstate |= QEF_EXEIMM_PROC;
        dsh->dsh_error.err_code = E_QE0303_PARSE_EXECIMM_QTEXT;

	break;

   } /* end of dummy for loop */	

   /* if we got an error while processing, we want to release
   ** the allocated memory.
   */

   if (status != E_DB_OK && ulm.ulm_streamid != (PTR) NULL)
   {
      /* release the allocated memory */

      ulm_closestream(&ulm);
   }

   return(status);

}

/*{
** Name: QEA_DOBJ		- Destroy the QSF object for the E.I. query text
**				  for QEA_EXEC_IMM header/query text for rules/
**				  procedures/indexes for constraints and views
**				  (WCO).
**
** Description:
**	This routine destroys the QSF object created by qea_cobj(). This is the
**	E.I. query text object created for QEA_EXEC_IMM action header/rules/
**	indexes/procedures in the QEA_CREATE_INTEGRITY action header and 
**	QEA_CREATE_VIEW (WCO).
**
**	The query text has been parsed by PSF and the E.I. objects created.
**
** 	This routine will be called by qea_xcimm() in QEAEXIMM.C for CREATE 
**	SCHEMA project, qea_createIntegrity() and qea_cview() in QEADDL.C for
**	FIPS constraints and CREATE VIEW (WCO) projects respectively.
**
** Inputs:
**	qef_rcb			Request Control Block
**	 .qef_qsf_handle	 qso_handle to the object which needs to be 
**				 destroyed.
**	 .qef_lk_id		 lock id of the query text object to be
**				 destroyed.
**	 .qsf_rcb		 pointer to the QSF_RCB
**	qsf_rb			 QSF_RCB copied from qsf_rcb.
**	 .qsf_lk_id		  qef_lk_id copied over to qsf_lk_id.
**	 .qso_handle		  qso_handle from qef_qsf_handle.  		   S
** Outputs:
**	Returns:
**	    E_DB_{OK,WARN,ERROR,FATAL}
**      Exceptions:
**	   none.
**
** Side Effects:
**	   none. 			
**
** History:
**	19-jul-93 (anitap)
**	    Written for CREATE SCHEMA, FIPS constraints and CREATE VIEW (WCO)
**	    projects.
**	04-apr-94 (anitap)
**	    Return the status instead of breaking on error.
*/
static DB_STATUS
qea_dobj(
	QEF_RCB		*qef_rcb,
	QSF_RCB	        *qsf_rb)
{
   DB_STATUS 		       status = E_DB_OK;

   for (;;)		/* dummy for loop to break for errors */
   {		
       qsf_rb = qef_rcb->qsf_rcb;	
       qsf_rb->qsf_lk_id = qef_rcb->qef_lk_id;
       qsf_rb->qsf_obj_id.qso_handle = qef_rcb->qef_qsf_handle.qso_handle;
       status = qsf_call(QSO_DESTROY, qsf_rb);
       if (DB_FAILURE_MACRO(status))
       {
	  return(status);
       }
       qef_rcb->qef_qsf_handle.qso_handle = 0;
	
       break;	
   } 
   return (status);
}		 

/*{
** Name: QEA_DSH	- Find DSH structure for the action resulting from
**			  E.I. parsing.
**
** Description: 
**	This routine finds the DSH structure for the following actions:
**	QEA_CREATE_TABLE and QEA_CREATE_VIEW in CREATE SCHEMA and QEA_GET
**	in QEA_CREATE_INTEGRITY (for Alter Table Add Constraint).
**
**	CREATE RULE and CREATE PROCEDURE not being actions do not require
**	DSH in QEA_CREATE_INTEGRITY. But this will change when the creation of
**	rules and procedures are routed through the server like DML statements.
**	This routine will need to be called.
**
**	This routine also takes care of stacking the DSH of the previous action,
**	i.e., QEA_EXEC_IMM for CREATE TABLE/CREATE VIEW and QEA_CREATE_INTEGRITY
**	for Alter Table Add Constraint.
**
** Inputs:
**	qef_rcb		Request Control block
**	dsh		Data segment header for the new action
**	qef_cb		Session control block
**
** Outputs:
**     qef_rcb
**         .error.err_code     E_QE0000_OK
**      Returns:
**         E_DB_{OK,WARN,ERROR,FATAL}
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	06-aug-93 (anitap)
**	    Created for CREATE SCHEMA and FIPS constraints projects.	
**	13-aug-93 (rickh)
**	    Added concluding return statement.
**	04-apr-94 (anitap)
**	    Changed error message to E_QE000D_NO_MEMORY_LEFT.
**      29-May-2008 (gefei01)
**          Prototype change for qeq_dsh().
**	21-Jun-2010 (kschendel) b123775
**	    Combine in-progress and is-tproc args to qeq-dsh.
*/
DB_STATUS
qea_dsh(
	QEF_RCB		*qef_rcb,
	QEE_DSH		*dsh,
	QEF_CB		*qef_cb)
{
     QEE_DSH		*tmp_dsh;
     DB_STATUS		status = E_DB_OK;
     i4		error;

     for(;;)		/* something to break out of */
     {
        tmp_dsh = dsh;

	/* Find the current DSH structure */
	status = qeq_dsh(qef_rcb, 0 , &dsh, QEQDSH_IN_PROGRESS, (i4) -1);
        if (DB_FAILURE_MACRO(status))
	{
	   /*
           ** The QP DSH is invalid for some reason. Generate error and return.
           ** If any allocation error then change to useful error message.
           */
            if (dsh->dsh_error.err_code == E_UL0005_NOMEM
                || dsh->dsh_error.err_code == E_QS0001_NOMEM           
                || dsh->dsh_error.err_code == E_QE001E_NO_MEM
                || dsh->dsh_error.err_code == E_QE000D_NO_MEMORY_LEFT)
	    {
	       qef_error(E_QE000D_NO_MEMORY_LEFT, 0L, status, &error,
                    &dsh->dsh_error, 0);
	       dsh->dsh_error.err_code = E_QE0122_ALREADY_REPORTED;	
	    }	

	    /*
            ** Now clean up and restore to state before routine entry.
            ** Pass in FALSE for release as if the DSH is NULL we do NOT want
            ** to cause cleanup to release all DSH's for this session
            ** (qee_cleanup).  If the DSH is not NULL it will be cleaned up at
            ** the end of the query.
            */
            status = qeq_cleanup(qef_rcb, status, FALSE);
	    if (status != E_DB_OK)
	       return (status);	
            dsh = tmp_dsh;
            qef_cb->qef_dsh = (PTR) dsh;

            qef_rcb->qef_pcount = dsh->dsh_exeimm_rcb->qef_pcount;
            qef_rcb->qef_usr_param = dsh->dsh_exeimm_rcb->qef_usr_param;

            qef_rcb->qef_qp = dsh->dsh_exeimm_rcb->qef_qp;
            qef_rcb->qef_qso_handle = dsh->dsh_exeimm_rcb->qef_qso_handle;
            qef_rcb->qef_rowcount = dsh->dsh_exeimm_rcb->qef_rowcount;
            qef_rcb->qef_targcount = dsh->dsh_exeimm_rcb->qef_targcount;
            qef_rcb->qef_output = dsh->dsh_exeimm_rcb->qef_output;
            qef_rcb->qef_count = dsh->dsh_exeimm_rcb->qef_count;
	    return(status);
         }

	 /* Found QP and DSH, stack old context */
         dsh->dsh_exeimm = tmp_dsh;
         dsh->dsh_stmt_no = qef_cb->qef_stmt++;
         qef_cb->qef_dsh = (PTR) dsh;

	 break;
     } /* end of dummy for loop */

     return( status );
}
