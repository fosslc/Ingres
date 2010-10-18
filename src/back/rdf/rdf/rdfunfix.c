/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
NO_OPTIM=nc4_us5
*/

#include	<compat.h>
#include	<gl.h>
#include	<sl.h>
#include	<iicommon.h>
#include	<dbdbms.h>
#include	<ddb.h>
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
#include        <qsf.h>
#include	<qefrcb.h>
#include	<qefqeu.h>
#include	<rdf.h>
#include	<rqf.h>
#include	<rdfddb.h>
#include	<ex.h>
#include	<rdfint.h>
#include	<rdftrace.h>

/**
**
**  Name: RDFUNFIX.C - Release the memory used for storing a table information
**		       so that other table information can be stored.
**
**  Description:
**	This file contains the routine for releasing the memory used for
**	storing a table information.
**
**	rdf_unfix - Release the memory used for storing a table information
**		    so that other table information can be stored.
**
**
**  History:
**      28-mar-86 (ac)
**          written
**	12-jan-87 (puree)
**	    modify for cache version.
**	22-mar-89 (neil)
**	    added support for rule objects.
**	20-dec-91 (teresa)
**	    added include of header files ddb.h, rqf.h and rdfddb.h for sybil
**	21-may-92 (teresa)
**	    SYBIL:  unfix LDBdesc cache object if appropriate.
**	08-dec-92 (anitap)
**	    Added include of heder file qsf.h for CREATE SCHEMA.
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	06-jul-93 (teresa)
**	    change include of dbms.h to include of gl.h, sl.h, iicommon.h &
**	    dbdbms.h.
**      18-Jan-96 (ramra01)
**          Highly concurrent machines can lead to an AVs, due to checking
**          for the rdf_state of a recently released ULH_OBJECT or the
**	    object being referenced is non-existant.
**      23-jan-97 (cohmi01)
**          Remove checksum calls from here, now done before releasing mutex
**          to ensure that checksum is recomputed during same mutex interval
**          that the infoblock was altered in. (Bug 80245).
**	29-sep-98 (matbe01)
**	    Added NO_OPTIM for NCR (nc4_us5) to eliminate runtime error.
**      09-may-00 (wanya01)
**          Bug 99982: Server crash with SEGV in rdf_unfix() and/or psl_make
**          _user_default().
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
**      31-aug-2001 (rigka01) bug# 105248
**          X-int 451569 by stial01
**          rdf_unfix: Remove two lines of code added on 09-may-00, which
**          was updating sys_rdf_ulhobj->rdf_shared_sessions after the
**          memory was freed
**      31-aug-2001 (rigka01)
**          Bug 103486: more server crashes in rdf_unfix() and/or psl_make_
**          user_default(). info block indicates default information is
**          available but pointer to the info is null.
**          If attr is nulled for private info block, then unset
**          RDR_ATTRIBUTES; also ensure defaults information is not  
**          destroyed when info block is a private info block
**      31-aug-2001 (rigka01) bug# 105248
**          X-int 451569 by stial01
**          rdf_unfix: Remove two lines of code added on 09-may-00, which
**          was updating sys_rdf_ulhobj->rdf_shared_sessions after the
**          memory was freed
**      20-aug-2002 (huazh01)
**          On rdf_unfix(), request a semaphore before updating
**          rdf_ulhobject->rdf_shared_sessions. This fixes b107625.
**          INGSRV 1551.
**	21-Mar-2003 (jenjo02)
**	    Cleaned up rdf_state "&" tests; rdf_state is an integral
**	    value, not a bit mask.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value DB_ERROR structure.
**	    Pass pointer to facilities DB_ERROR instead of just its err_code
**	    in rdu_ferror().
**      01-oct-2010 (stial01) (SIR 121123 Long Ids)
**          Store blank trimmed names in DMT_ATT_ENTRY
**/

/*{
** Name: rdf_unfix - Release the memory used for storing a table information
**		     so that other table information can be stored.
**
**	External call format:	status = rdf_call(RDF_UNFIX, &rdf_cb);
**
** Description:
**	This function releases all the memory used for storing a table
**	information.
**
** Inputs:
**
**      rdf_cb
**		.rdf_rb
**			.rdr_fcb	Pointer to the internal structure
**					which contains the global memory
**					allocation and caching control
**					information.
**		.rdf_info_blk		Pointer to the table information
**					block to be released.
** Outputs:
**      rdf_cb
**
**		.rdf_error              Filled with error if encountered.
**
**			.err_code	One of the following error numbers.
**				E_RD0000_OK
**					Operation succeed.
**				E_RD0006_MEM_CORRUPT
**					Memory pool is corrupted.
**				E_RD0007_MEM_NOT_OWNED
**					Memory pool is not owned by the calling
**					facility.
**				E_RD0008_MEM_SEMWAIT
**					Error waiting for exclusive access of
**					memory.
**				E_RD0009_MEM_SEMRELEASE
**					Error releasing exclusive access of
**					memory.
**				E_RD000A_MEM_NOT_FREE
**					Error freeing memory.
**				E_RD000B_MEM_ERROR
**					Whatever memory error not mentioned
**					above.
**				E_RD0003_BAD_PARAMETER
**					Bad input parameters.
**				E_RD0020_INTERNAL_ERR
**					RDF internal error.
**
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_ERROR			Function failed due to error by caller;
**					error status in rdf_cb.rdf_error.
**	    E_DB_FATAL			Function failed due to some internal
**					problem; error status in
**					rdf_cb.rdf_error.
**	Exceptions:
**		none
**
** Side Effects:
**		.rdf_info_blk		Pointer to the table information
**					block is cleared to null.
** History:
**	28-mar-86 (ac)
**          written
**	12-jan-87 (puree)
**	    modify for cache version.
**      12-nov-87 (seputis)
**          rewritten for error/resource handling
**      12-aug-88 (seputis)
**          procedures now have an infoblk for grant support
**	22-mar-89 (neil)
**	    added support for rule objects.
**	15-dec-89 (teg)
**	    add logic to put ptr to RDF Server CB into GLOBAL for subordinate
**	    routines to use.
**	01-feb-90 (teresa)
**	    prevent potential (albeit unlikely) AV by assuring that
**	    rdi_fcb is non-zero before trying to dereference it.
**	23-jan-92 (teresa)
**	    SYBIL:  rdi_qthashid->rdv_qtree_hashid
**	21-may-92 (teresa)
**	    SYBIL:  unfix LDBdesc cache object if appropriate.
**	09-jul-92 (teresa)
**	    fixed procedure logic and statistics.  Procedures were mistakenly
**	    handled in the RDR_QTREE logic, but that bit is not set for
**	    procedures, so statistics and error handling was incorrect.  This
**	    has been fixed.
**	16-jul-92 (teresa)
**	    implement prototypes
**	18-sep-92 (teresa)
**	    add support for trace point to dump infoblk at unfix time.
**	24-feb-93 (teresa)
**	    Add suport to unfix Defaults cache objects if they're fixed.
**	19-jul-93 (teresa)
**	    Added a consistency check to assure that the RDR_PROCEDURE bit is
**	    not set when unfixing a non-procedure object.  (Reduces bug 53332
**	    from server shutdown AV to hitting a run-time consistency check)
**	23-aug-93 (teresa)
**	    changed comment in code for (above) fix to be a little clearer.
**	    Did not change code, just comment about consistency check.
**	28-sep-93 (robf)
**	    Only dereference sysinfo_blk if it points to something.
**	16-feb-94 (teresa)
**	    Modify checksum logic to always calculate checksum, whether or not
**	    tracepoint is set to skip invalidating cache if checksum is not
**	    set.  Also, clean up some of the comments so the code is easier to
**	    understand.
**      18-Jan-96 (ramra01)
**          Highly concurrent machines can lead to an AVs, due to checking
**          for the rdf_state of a recently released ULH_OBJECT or the
**	    object being referenced is non-existant.
**      23-jan-97 (cohmi01)
**          Remove checksum calls from here, now done before releasing mutex
**          to ensure that checksum is recomputed during same mutex interval
**          that the infoblock was altered in. (Bug 80245).
**      17-Feb-97 (rommi04)
**          Prevent SIGSEGVs by assuring that rdf_cb->rdf_info_blk and
**          rdf_cb->rdf_info_blk->rdr_object are not NULL before used. Removed
**          obsolete test on rdi_fcb.
**	09-may-1997 (shero03)
**	    If a private infoblk is being unfixed, ensure that it is not 
**	    referenced after the stream is deleted.  (B81571)
**	30-jul-97 (nanpr01)
**	    Set the streamid to NULL after thre stream is deallocated.
**	07-Aug-1997 (jenjo02)
**	    Set the streamid to NULL BEFORE the stream is deallocated.
**      09-may-00 (wanya01)
**          Bug 99982: Server crash with SEGV in rdf_unfix() and/or psl_make
**          _user_default().
**      11-jul-01 (stial01)
**          rdf_unfix: Remove two lines of code added on 09-may-00, which was
**          updating sys_rdf_ulhobj->rdf_shared_sessions after the memory was
**          freed. (B103486)
**      31-aug-2001 (rigka01) bug# 105248
**          X-int 451569 by stial01
**          rdf_unfix: Remove two lines of code added on 09-may-00, which
**          was updating sys_rdf_ulhobj->rdf_shared_sessions after the
**          memory was freed
**      31-aug-2001 (rigka01)
**          Bug 103486: more server crashes in rdf_unfix() and/or psl_make_
**          user_default(). info block indicates default information is
**          available but pointer to the info is null.
**          If attr is nulled for private info block, then unset
**          RDR_ATTRIBUTES; also ensure defaults information is not  
**          destroyed when info block is a private info block
**      31-aug-2001 (rigka01) bug# 105248
** 	    X-int 451569 by stial01 
**	    rdf_unfix: Remove two lines of code added on 09-may-00, which
**	    was updating sys_rdf_ulhobj->rdf_shared_sessions after the 
**	    memory was freed 
**      20-aug-2002 (huazh01)
**          Semaphore protection on rdf_ulhobject->rdf_shared_sessions.
**          This fixes bug 107625. INGSRV 1551.
**	8-Aug-2005 (schka24)
**	    Revise above fix to use thread-safe decrement.
**      20-Aug-2007 (kibro01) b118595
**          If the remote connection was lost, the object was not fully
**          created.  Check for NULL before attempting to destroy it.
**	7-Feb-2007 (kibro01) b119744
**	    Use rdu_drop_attachments so the same is done as in other places
**	19-oct-2009 (wanfr01) Bug 122755
**	    Need to mutex protect rdf_shared_sessions as the object may be 
**	    about to be destroyed.
**	    Need to pass infoblk to the deulh routines
**	    Added sanity check for rdf_shared_sessions
*/
DB_STATUS
rdf_unfix(  RDF_GLOBAL	*global,
	    RDF_CB	*rdf_cb)
{
    RDR_INFO	*usr_infoblk,
		*sys_infoblk;
    RDI_FCB	*rdi_fcb;
    DB_STATUS	status = E_DB_OK;
    DB_STATUS	ulh_status = E_DB_OK;
    i4	firstval, secondval;
    RDD_DEFAULT	*rdf_defobj;
    RDF_STATE   rdf_state_local;

    /* set up pointer to rdf_cb in global for subordinate
    ** routines.  The rdfcb points to the rdr_rb, which points to the rdr_fcb.
    */
    global->rdfcb = rdf_cb;
    rdi_fcb = (RDI_FCB *)(rdf_cb->rdf_rb.rdr_fcb);

    /* Check input parameter. */
    if ((rdf_cb->rdf_info_blk == NULL) || 
	(rdf_cb->rdf_info_blk->rdr_object == NULL) ||
	(rdi_fcb == NULL))
    {
        status = E_DB_ERROR;
        rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
        return(status);
    }

    CLRDBERR(&rdf_cb->rdf_error);
    usr_infoblk = rdf_cb->rdf_info_blk;
    global->rdf_ulhobject = (RDF_ULHOBJECT *)usr_infoblk->rdr_object;

    /* run a consistency check to assure that rdr_types_bits are valid --
    ** if RDR_PROCEDURE then usr_infoblk->rdr_rel had better be NULL.
    ** This is important because relation information is always
    ** allocated on the relcache and procedures are always put on the
    ** qtree cache.  If RDR_PROCEDURE is set, then RDF will unfix from
    ** qtree cache.
    */
    if (
	 (usr_infoblk->rdr_rel != NULL)
	 &&
	 (rdf_cb->rdf_rb.rdr_types_mask & RDR_PROCEDURE)
       )
    {
	status = E_DB_ERROR;
	rdu_ierror(global, status, E_RD0003_BAD_PARAMETER);
	return(status);
    }

    global->rdf_d_ulhobject = global->rdf_ulhobject->rdf_starptr;
    sys_infoblk = global->rdf_ulhobject->rdf_sysinfoblk;
    rdf_state_local = global->rdf_ulhobject->rdf_state;

    if (rdf_cb->rdf_rb.rdr_types_mask & RDR_QTREE )
    {
	/* we are releasing a QTREE cache object. */

	RDD_QRYMOD	*qrymod;
	RDD_QRYMOD	**qrymodpp;

	if (rdf_cb->rdf_rb.rdr_types_mask & RDR_PROTECT)
	    qrymodpp = &rdf_cb->rdf_rb.rdr_protect;
	else if (rdf_cb->rdf_rb.rdr_types_mask & RDR_INTEGRITIES)
	    qrymodpp = &rdf_cb->rdf_rb.rdr_integrity;
	else if (rdf_cb->rdf_rb.rdr_types_mask & RDR_RULE)
	    qrymodpp = &rdf_cb->rdf_rb.rdr_rule;
	qrymod = *qrymodpp;
	*qrymodpp = NULL;
	switch (qrymod->rdf_qtresource)
	{
	case RDF_SSHARED:
	{
	    global->rdf_ulhcb.ulh_hashid = Rdi_svcb->rdv_qtree_hashid;
	    global->rdf_ulhcb.ulh_object = (ULH_OBJECT *)qrymod->rdf_ulhptr;
	    global->rdf_init |= RDF_IULH;
	    status = rdu_rulh(global);
	    /* report statistics */
	    if (rdf_cb->rdf_rb.rdr_types_mask & RDR_PROTECT)
		Rdi_svcb->rdvstat.rds_u0_protect++;
	    else if (rdf_cb->rdf_rb.rdr_types_mask & RDR_INTEGRITIES)
		Rdi_svcb->rdvstat.rds_u1_integrity++;
	    else if (rdf_cb->rdf_rb.rdr_types_mask & RDR_RULE)
		Rdi_svcb->rdvstat.rds_u3_rule++;
	    break;
	}
	case RDF_SPRIVATE:
	{
	    PTR 	stream_id = qrymod->rdf_qtstreamid;
	    qrymod->rdf_qtstreamid = NULL;
	    Rdi_svcb->rdvstat.rds_u10_private++;
	    status = rdu_dstream(global, stream_id,
				 &qrymod->rdf_qtresource);
	    /* report statistics */
	    if (rdf_cb->rdf_rb.rdr_types_mask & RDR_PROTECT)
		Rdi_svcb->rdvstat.rds_u4p_protect++;
	    else if (rdf_cb->rdf_rb.rdr_types_mask & RDR_INTEGRITIES)
		Rdi_svcb->rdvstat.rds_u5p_integrity++;
	    else if (rdf_cb->rdf_rb.rdr_types_mask & RDR_RULE)
		Rdi_svcb->rdvstat.rds_u7p_rule++;
	    break;
	}
	case RDF_SUPDATE:
	case RDF_SNOACCESS:
	default:
	    {
		status = E_DB_SEVERE;
		Rdi_svcb->rdvstat.rds_u8_invalid++;
		    rdu_ierror(global, status, E_RD0110_QRYMOD);
		return(status);		    /* unexpected state */
	    }
	} /* endcase */
    }
    else

    {
	/*
	** we're unfixing a relation cache object
	*/

	/* handle any trace requests. */
	rdu_master_infodump(usr_infoblk, global, RDFUNFIX,0);

	status = rdu_gsemaphore(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);
	CSadjust_counter(&global->rdf_ulhobject->rdf_shared_sessions, -1);
/*
	if (global->rdf_ulhobject->rdf_shared_sessions < 0)
	{
                TRdisplay ("%@  Warning:  rdf_shared_session count = %d.  Resetting to zero.\n",global->rdf_ulhobject->rdf_shared_sessions);
		global->rdf_ulhobject->rdf_shared_sessions = 0;
	}
*/
	status = rdu_rsemaphore(global);
	if (DB_FAILURE_MACRO(status))
	    return(status);

	if ( rdf_state_local == RDF_SPRIVATE )
	{
	    /* if this is a private memory stream then close the stream,
	    ** but be sure to unfix the LDBcache object if appropriate.
	    */
	    if ( (usr_infoblk->rdr_types & RDR_DST_INFO) &&
		 global->rdf_ulhobject->rdf_shared_sessions
	       )
	    {
		/* When RDF built this private infoblk, it fixed an LDBdesc
		** cache object for the dd_t9_ldb_p, so unfix it
		*/
		if (global->rdf_d_ulhobject->rdf_state == RDF_SBAD)
		    /* destroy the distributed object */
		    ulh_status = rdu_i_dulh(global);
		else
		    /* release the distributed object */
		    ulh_status = rdu_r_dulh(global);
	    }

	    /* if default cache info is fixed, unfix it here */
	    if ( (usr_infoblk->rdr_2_types & RDR2_DEFAULT) &&
		 global->rdf_ulhobject->rdf_shared_sessions
	       )
	    {
		i4		i;
		DMT_ATT_ENTRY  *attr;	    /* ptr to DMT_ATT_ENTRY ptr array */

	      if ((global->rdf_ulhobject->rdf_sysinfoblk) &&
		  (usr_infoblk->rdr_attr == 
	          global->rdf_ulhobject->rdf_sysinfoblk->rdr_attr) )
	      {  
		/* in use by sys, don't destroy shared default cache object */
		usr_infoblk->rdr_types &= (~RDR_ATTRIBUTES);
		usr_infoblk->rdr_attr = NULL; 
		usr_infoblk->rdr_attr_names = NULL; 
		usr_infoblk->rdr_2_types &= (~RDR2_DEFAULT);
	      }
	      else 
		/* When RDF built this private infoblk, it fixed one or more
		** defaults cache entries to attribute descriptors.  Unfix
		** them now.
		*/
		for (i=DB_IMTID+1; i <= usr_infoblk->rdr_no_attr; i++)
		{
		    attr = usr_infoblk->rdr_attr[i];
		    if (attr->att_default)
		    {
			/* assure this bit is not set so the ulh_cb will be
			** initialzed, since each attribute could have a
			** different default.
			*/
			global->rdf_init &= ~RDF_DE_IULH;
			rdf_defobj = (RDD_DEFAULT*) (attr->att_default);
			global->rdf_de_ulhobject =
			  (RDF_DE_ULHOBJECT *)rdf_defobj->rdf_def_object;
			if (global->rdf_de_ulhobject->rdf_state == RDF_SBAD)
			    /* destroy the distributed object */
			    ulh_status = rdu_i_deulh(global, usr_infoblk, attr);
			else
			    /* release the distributed object */
			    ulh_status = rdu_r_deulh(global, usr_infoblk, attr);
		    }
		}
	    }
	}
	else
	{   /* This is NOT a private infoblk, so it is the shared one */

	    if (sys_infoblk)
	    {
		RDF_ULHOBJECT *old_rdf_ulhobject = global->rdf_ulhobject;
		global->rdf_ulhobject = (RDF_ULHOBJECT*)sys_infoblk->rdr_object;
		rdu_drop_attachments(global, sys_infoblk, 0);
		global->rdf_ulhobject = old_rdf_ulhobject;
	    }
	}

	if (sys_infoblk &&
	    sys_infoblk->rdr_invalid)
	{
	    /* If the info block was marked as invalid,
	    ** call ulh to invalidate the object */
	    ulh_status = rdu_iulh(global);
	    if (DB_FAILURE_MACRO(ulh_status))
	    {
		if (DB_SUCCESS_MACRO(status))
		    status = ulh_status;
		Rdi_svcb->rdvstat.rds_u9_fail++;
	    }
	    else
	    {
		/* now report statistics */
                if (rdf_state_local == RDF_SPRIVATE)
		{
		    if (rdf_cb->rdf_rb.rdr_types_mask & RDR_PROCEDURE)
			Rdi_svcb->rdvstat.rds_u6p_procedure++;
		    else
			Rdi_svcb->rdvstat.rds_u10_private++;
		}
		else
		{
		    if (rdf_cb->rdf_rb.rdr_types_mask & RDR_PROCEDURE)
			Rdi_svcb->rdvstat.rds_u2_procedure++;
		    else
			Rdi_svcb->rdvstat.rds_u11_shared++;
		}
	    }

	}
	else
	{   /* attempt to release ulh object even if there is a problem releasing
	    ** memory for a private stream, so that resource is not lost */

            RDF_ULHOBJECT   *ulhobj = 0;

            if (usr_infoblk && usr_infoblk->rdr_object)
                ulhobj = (RDF_ULHOBJECT *)usr_infoblk->rdr_object;
            else if (global->rdf_ulhcb.ulh_object)
            {
                ulhobj = (RDF_ULHOBJECT *)
                                     global->rdf_ulhcb.ulh_object->ulh_uptr;
                global->rdfcb->rdf_info_blk =  ulhobj->rdf_sysinfoblk;
            }

            if (! global->rdf_ulhobject)
            {
                global->rdf_ulhobject = ulhobj;
            }

            if (! ulhobj)
            {
                /* if we canot find the rdf_ulhobject for this resource,
                ** (which theoritically should never happen), then we cannot
                ** release it.  So, go ahead and set the status as though
                ** we had infact released it and unset the RDF_RULH and
                ** RDF_CLEANUP resources -- From rdfcall.c
                */

                Rdi_svcb->rdvstat.rds_u9_fail++;
            }
            else
            {
	        ulh_status = rdu_rulh(global);
	        if (DB_FAILURE_MACRO(ulh_status))
	        {
		    if (DB_SUCCESS_MACRO(status))
		       status = ulh_status;
		    Rdi_svcb->rdvstat.rds_u9_fail++;
	        }
	        else
	        {
		    /* now report statistics */
                    if (rdf_state_local == RDF_SPRIVATE)
		    {
		        if (rdf_cb->rdf_rb.rdr_types_mask & RDR_PROCEDURE)
			    Rdi_svcb->rdvstat.rds_u6p_procedure++;
		        else
			    Rdi_svcb->rdvstat.rds_u10_private++;
		    }
		    else
		    {
		        if (rdf_cb->rdf_rb.rdr_types_mask & RDR_PROCEDURE)
			    Rdi_svcb->rdvstat.rds_u2_procedure++;
		        else
			    Rdi_svcb->rdvstat.rds_u11_shared++;
		    }
	        }
	    }
	}

        if (rdf_state_local == RDF_SPRIVATE)
	{
	    PTR		stream_id = usr_infoblk->stream_id;
            RDF_ULHOBJECT   *sys_rdf_ulhobj;

	    usr_infoblk->stream_id = NULL;
            ulh_status = rdu_dstream(global, stream_id,
	    			 (RDF_STATE *)NULL);

            if (DB_FAILURE_MACRO(ulh_status))
            {
               /* preserve the most severe of the status */
               if (ulh_status > status)
                 status = ulh_status;
	    }
	}

	rdf_cb->rdf_info_blk = NULL;	    /* clear pointer to null */
    }
    return(status);
}
