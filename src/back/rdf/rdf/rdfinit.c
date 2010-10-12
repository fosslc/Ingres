/*
**Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include	<iicommon.h>
#include	<dbdbms.h>
#include	<st.h>
#include	<ddb.h>
#include	<ulf.h>
#include	<ulm.h>
#include	<cs.h>
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
#include	<cui.h>

/**
**
**  Name: RDFINIT.C - Initialize RDF for a facility.
**
**  Description:
**	This file contains the routine for initializing RDF for a facility. 
**
**	rdf_initialize - Initialize RDF for a facility.
**
**
**  History:
**      28-mar-86 (ac)    
**          written
**	12-jan-87 (puree)
**	    modify for cache version.
**	16-mar-87 (puree)
**	    validate max. number of tables in cache against memory space.
**	17-Oct-89 (teg)
**	    change logic to initialize RDF from RDF owned cache, with poolid
**	    in server control block.  Also, remove hash init logic, as that is
**	    now performed in RDFBEGIN.C.  This is part of the fix to bug 5446.
**	20-dec-91 (teresa)
**	    added include of header files ddb.h, rqf.h and rdfddb.h for sybil
**	08-dec-92 (anitap
**	    Added include of header file qsf.h for CREATE SCHEMA.
**	08-apr-93 (ralph)
**	    DELIM_IDENT: 
**	    Initialize rdi_utiddesc.  Yuck.
**	06-jul-93 (teresa)
**	    change include of dbms.h to include of gl.h, sl.h, iicommon.h &
**	    dbdbms.h.
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
**	    RDI_SVCB *svcb is now a GLOBALREF, added macros 
**	    GET_RDF_SESSCB macro to extract *RDF_SESSCB directly
**	    from SCF's SCB, added (ADF_CB*)rds_adf_cb,
**	    deprecating rdu_gcb(), rdu_adfcb(), rdu_gsvcb()
**	    functions.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value DB_ERROR structure.
**	    Pass pointer to facilities DB_ERROR instead of just its err_code
**	    in rdu_ferror().
**      01-oct-2010 (stial01) (SIR 121123 Long Ids)
**          Store blank trimmed names in DMT_ATT_ENTRY
**/

/*{
** Name: rdf_initialize - Initialize RDF for a facility.
**
**	External call format:	status = rdf_call(RDF_INITIALIZE, &rdf_ccb);
**
** Description:
**
**	This function initializes RDF for a facility.  It is called once from
**	PSF and once from OPF.  It uses the same memory pool to service both
**	facilities, but each facility has its own facility control block, which
**	is allocated from the memory pool.  Each facility (PSF, OPF) should 
**	call this routine once per the life of the server, and the call should
**	be made at server start up time.
**
**	This function opens a memory stream on behalf of the calling facility,
**	and then allocates a facility control block for that facility.
**	RDF will obtain the memory pool poolid from the RDF server control 
**	block (SVCB).  That memory pool was allocated by a call to rdf_begin()
**	(by SCF) PRIOR TO the call to rdf_initialize by the facility (OPF/PSF).
**
**	A pointer to the facility control block will be returned to the calling 
**	facility. This pointer needs to be passed as an input parameter in the
**	subsequent rdf_calls()s.
**
**	rdf_init must be called in a single-thread manner.
**	
**  Inputs:
**          
**      rdf_ccb             RDF Control BLock from Calling Facility
**          .rdf_fac_id         Calling facility id.
**	    .rdf_fcb		Facility control block
**	svcb	    	Server Control Block
**	    .rdv_poolid      Memory Pool ID
**	    .rdv_memleft     Amount of memory remaining in the pool.
**
**  Outputs:
**	svcb
**          .rdv_memleft     Amount of memory remaining in the pool.
**      global              RDF Global Memory For This Session.
**          .rdf_ulmcb          Pointer to ULM File Control Block
**      rdf_ccb             RDF Control BLock from Calling Facility
**          .rdf_fcb            Pointer ot ULM File Control Block
**              .rdi_poolid         Memory Pool Id
**              .rdi_memleft        Unused Memory In Pool
**              .rdi_fac_id         Facility Id
**              .rdi_streamid       Memory Stream Id
**              .rdi_tiddesc        Tid
**                  .att_name           Attribute name.
**                  .att_number         Attribute number.
**                  .att_offset         Offset to attribute from start of tuple
**                  .att_type           Data Type for this attribute
**                  .att_width          number bytes to hold data for this attribute
**                  .att_prec           Precision for this data type (=0)
**                  .att_key_seq_number indicates where in key this attr is (=0)
**		.rdi_utiddesc	    TID (same as rdi_tiddesc, but with 
**				         upper-cased att_name field)
**              .rdi_qthashid       Qtree Hash Tbl Id
**              .rdi_hashid         Relation Hash Tbl Id
**              .rdi_alias          Flag.  True => alias is defined for facility.
**	    .rdf_error		RDF Error Control Block.
**		.err_code	One of the following error numbers.
**                              E_RD0000_OK - Operation succeed.
**                              E_RD0118_ULMOPEN - Error Opening Memory Stream.
**                              E_RD0127_ULM_PALLOC - Error Allocating Memory
**                                                    for facility control block
**
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_ERROR			Function failed due to error by caller;
**					error status in rdf_ccb.rdf_error.
**	    E_DB_FATAL			Function failed due to some internal
**					problem; error status in
**					rdf_ccb.rdf_error.
**	Exceptions:
**		none
** Side Effects:
**	A facility control block which contains the global memory allocation
** 	information and caching control information is allocated in the RDF
**	memory pool.
**	
** History:
**	28-mar-86 (ac)
**          written
**	12-jan-87 (puree)
**	    modify for cache version.
**	02-mar-87 (puree)
**	    replace global server control block with global pointer.
**      10-nov-87 (seputis)
**          added ULH object management for querymod trees
**	17-Oct-89 (teg)
**	    change logic to initialize RDF from RDF owned cache, with poolid
**	    in server control block.  Also, remove hash init logic, as that is
**	    now performed in RDFBEGIN.C.  This is part of the fix to bug 5446.
**	15-dec-89 (teg)
**	    added logic to stop using GLOBALRDF Rdi_svcb and get svcb ptr
**	    from SCF and save in FCB.
**	18-apr-90 (teresa)
**          rdu_ferror interface changed as part of fix for bug 4390.
**	23-jan-92 (teresa)
**	    changes for SYBIL:
**		Changed: rdv_c1_memleft->rdv_memleft; rdv_c1_poolid->rdv_poolid;
**		Removed from FCB: rdi_poolid; rdi_memleft; rdi_hashid; 
**				  rdi_qthashid; rdi_alias
**	08-apr-93 (ralph)
**	    DELIM_IDENT: 
**	    Initialize rdi_utiddesc.  Yuck.
**	24-mar-93 (smc)
**	    Fixed parameter of ulm_closestream() to match prototype.
**	12-feb-04 (inkdo01)
**	    Change TID size to 8 bytes (for partitioned tables).
*/
DB_STATUS
rdf_initialize( RDF_GLOBAL	*global,
		RDF_CCB		*rdf_ccb)
{
	RDI_FCB		*fcbptr;
        ULM_RCB         *ulm_rcb = &global->rdf_ulmcb;
	DB_STATUS	status = E_DB_OK;
	bool		stream_opened = FALSE;
	
    global->rdfccb = rdf_ccb;
    CLRDBERR(&rdf_ccb->rdf_error);
    
    do		/* control loop */
    {

	/* Set up ulm call input parameters. */

	ulm_rcb->ulm_poolid = Rdi_svcb->rdv_poolid;
        ulm_rcb->ulm_memleft = &Rdi_svcb->rdv_memleft;
        ulm_rcb->ulm_facility = DB_RDF_ID;
	ulm_rcb->ulm_streamid_p = (PTR*)NULL; /* streamid in ulm_streamid */

	/* Request a private ULM stream for the FCB */
	/* Open stream and allocate RDI_FCB with one effort */
	ulm_rcb->ulm_flags = ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;
	ulm_rcb->ulm_psize = sizeof(RDI_FCB);
	/* 
	** If the calling facility if PSF, need to allocate the space
	** for tid attribute entries.
	*/
	if (rdf_ccb->rdf_fac_id == DB_PSF_ID)
	{
		ulm_rcb->ulm_psize += 2*sizeof(DMT_ATT_ENTRY);
		ulm_rcb->ulm_psize += 2 * (sizeof(DB_ATT_NAME) + 1);
	}

	/* Allocate a block just large enough for our needs */
	ulm_rcb->ulm_blocksize = ulm_rcb->ulm_psize;

	status = ulm_openstream(ulm_rcb);
	if (DB_FAILURE_MACRO(status))
	{
	    rdu_ferror(global, status, 
		&ulm_rcb->ulm_error, E_RD0118_ULMOPEN, 0);
	    break;
	}

	stream_opened = TRUE;

	fcbptr = (RDI_FCB *)ulm_rcb->ulm_pptr;
	/* 
	** Store the memory allocation information in the facility control
	** block. 
	*/

	fcbptr->rdi_fac_id = rdf_ccb->rdf_fac_id;       /* facility id */
	fcbptr->rdi_streamid = ulm_rcb->ulm_streamid;	/* stream id */
	fcbptr->rdi_tiddesc = NULL;                     /* tid */
	fcbptr->rdi_utiddesc = NULL;                    /* TID */

	/* 
	** If the calling facility is PSF, initialize the attribute entry
	** of tids.
	*/
	if (rdf_ccb->rdf_fac_id == DB_PSF_ID)
	{
	    fcbptr->rdi_tiddesc = (DMT_ATT_ENTRY *)
                (((char *)ulm_rcb->ulm_pptr) + sizeof(RDI_FCB));
	    fcbptr->rdi_utiddesc = (DMT_ATT_ENTRY *)
                (((char *)ulm_rcb->ulm_pptr) + sizeof(RDI_FCB)
					     + sizeof(DMT_ATT_ENTRY));
	    fcbptr->rdi_tiddesc->att_nmstr = (char *)fcbptr->rdi_utiddesc +
				sizeof(DMT_ATT_ENTRY);
	    fcbptr->rdi_utiddesc->att_nmstr = fcbptr->rdi_tiddesc->att_nmstr +
				sizeof(DB_ATT_NAME) + 1;

	    cui_move(3, "tid", '\0', 4, fcbptr->rdi_tiddesc->att_nmstr);
	    fcbptr->rdi_tiddesc->att_nmlen = 3;
	    cui_move(3, "TID", '\0', 4, fcbptr->rdi_utiddesc->att_nmstr);
	    fcbptr->rdi_utiddesc->att_nmlen = 3;
	    fcbptr->rdi_tiddesc->att_number = 0L;
	    fcbptr->rdi_utiddesc->att_number = 0L;
	    fcbptr->rdi_tiddesc->att_offset = 0L;
	    fcbptr->rdi_utiddesc->att_offset = 0L;
	    fcbptr->rdi_tiddesc->att_type = (i4) DB_TID8_TYPE;
	    fcbptr->rdi_utiddesc->att_type = (i4) DB_TID8_TYPE;
	    fcbptr->rdi_tiddesc->att_width = DB_TID8_LENGTH;
	    fcbptr->rdi_utiddesc->att_width = DB_TID8_LENGTH;

	    /* Integers have no prec. */

	    fcbptr->rdi_tiddesc->att_prec = 0L;     
	    fcbptr->rdi_utiddesc->att_prec = 0L;     
	    fcbptr->rdi_tiddesc->att_key_seq_number = 0L;
	    fcbptr->rdi_utiddesc->att_key_seq_number = 0L;
	    
	    /* qtree hashid */

	}

	/* 
	** Return the address of the facility control block to the calling
	** facility.
	*/

	rdf_ccb->rdf_fcb = (PTR)fcbptr;

    } while (0); /* end control loop */

    if (DB_FAILURE_MACRO(status) && stream_opened)
	(VOID) ulm_closestream(ulm_rcb);

    return(status);
}
