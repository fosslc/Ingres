/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <er.h>
#include    <ex.h>
#include    <pc.h>
#include    <tm.h>
#include    <cv.h>
#include    <cs.h>
#include    <tr.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <scf.h>
#include    <qsf.h>
#include    <adf.h>
#include    <gca.h>


/* added for scs.h prototypes, ugh! */
#include <dudbms.h>
#include <dmf.h>
#include <dmccb.h>
#include <dmrcb.h>
#include <copy.h>
#include <qefrcb.h>
#include <qefqeu.h>
#include <qefcopy.h>

#include    <sc.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <scu.h>
#include    <sc0m.h>
#include    <scfcontrol.h>
#include    <sc0e.h>

/**
**
**  Name: SCUMSVC.C - SCF Utility Memory Mgmt for sessions
**
**  Description:
**      This file contains the routines for the allocation,
**      deallocation, and general management of large chunks 
**      of memory allocated to users of SCF (i.e. other ingres 
**      dbms facilities.
**
**          scu_malloc - allocate some number of pages
**          scu_mfree - free some number of pages
**	    scu_mcount - count of pages for a session
**	    scu_mdump -	dump to mcb list (xDEBUG only)
**
**
**
**  History:
**      20-Jan-86 (fred)    
**          Created on jupiter
**	22-Mar-89 (fred)
**	    Add support for use of Sc_main_cb->sc_proxy_cb for performing work
**	    for other sessions.
**	21-may-89 (jrb)
**	    change to ule_format interface
**	13-jul-92 (rog)
**	    Included ddb.h for Sybil, and er.h because of a change to scs.h.
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	29-Jun-1993 (daveb)
**	    include <tr.h>, cast arg to CSget_scb correctly.
**	2-Jul-1993 (daveb)
**	    prototyped, removed bad func externs.
**	06-jun-1993 (shailaja)
**          Cast parameters to match prototypes.
**	7-Jul-1993 (daveb)
**	    fixed headers for prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	13-sep-93 (swm)
**	    Include cv.h to pickup CV_HEX_PTR_SIZE define which is now
**	    needed by scs.h.
**	11-oct-1993 (tad/johnst)
**	    Bug #56449
**	    Changed %x to %p for pointer values inscu_mfind_mcb(),
**	    scu_mdump().
**	    Changed TRdisplay format args from %x to %p where necessary to
**	    display pointer types. We need to distinguish this on, eg 64-bit
**	    platforms, where sizeof(PTR) > sizeof(int).
**      05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**          This results in owner within sc0m_allocate having to be PTR.
**    07-apr-95 (kch)
**          In the functions scu_malloc() and scu_mfree() the variable
**          status is now not set to E_SC0028_SEMAPHORE_DUPLICATE.
**          Previously, if status = E_CS0017_SMPR_DEADLOCK, then status
**          was set to E_SC0028_SEMAPHORE_DUPLICATE.
**          This change fixes bug 65445.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**	12-Mar-1998 (jenjo02)
**	    Session's MCB is now embedded in SCS_SSCB instead of being
**	    separately allocated.
**	14-jan-1999 (nanpr01)
**	    Changed the sc0m_deallocate to pass pointer to a pointer.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Jan-2001 (jenjo02)
**	    Added (SCD_SCB*)scb parameter to prototypes for
**	    scu_information(), scu_idefine(), scu_malloc(),
**	    scu_mfree(), scu_xencode().
**	30-Nov-2006 (kschendel) b122041
**	    Fix ult_check_macro usage.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value scf_error structure.
**	    Use new form of sc0ePut().
**/

/*
**  Forward and/or External typedef/struct references.
*/

GLOBALREF SC_MAIN_CB	*Sc_main_cb;	    /* server control block */

/*
**  Forward and/or External function references.
*/

/* (none) */


/*{
** Name: scu_malloc	- allocate some number of pages on behalf of a user
**
** Description:
**      This routine allocates some number of pages to a facility.  The memory 
**      allocated here is considered to be long-term memory (on the order
**      of session or server length of time).  All memory returned by this 
**      service is page (SCU_MPAGESIZE byte) aligned, and the unit of measuring
**      of this memory is pages (i.e. requests come in for number of pages, 
**      not number of bytes).  Note that this page size must be variable
**	from environment to environment.  This necessary to adjust for
**	environmental performance differences in paging activity.
** 
**      For reporting and monitoring purposes, this memory is tracked on a 
**      per facility per session basis;  therefore the caller will be asked 
**      to provide a facility id and an session id on the request.  If 
**      the memory is to be shared between sessions, the session id 
**      DB_NOSESSION should be provided.  No memory is shared between
**      facilities.
**
**      Furthermore, if memory is declared to have been used by a session
**      and that session subsequently terminates without releasing this
**      memory, the memory will be freed by SCF and an error logged reporting
**      the fact that this psuedo action was necessary.
**      Facilities are expected to be frugal with the server's memory.
**
**      As a side note, the allocation of memory on a per facility per session
**      basis will allow SCF to aid the facility designers in their debugging.
**      As facilities in sessions are activated, SCF can, with the appropriate
**      debugging flags, map/unmap the appropriate memory (on VMS at least)
**      so that a session's reference to another session's memory will be
**      trapped at the point of offense.  More details later.
**
** Inputs:
**      SCU_MALLOC                      the operation code to scf_call()
**      scf_cb                          the control block itself
**          .scf_session                session id this memory is for
**          .scf_facility               facility this memory is for
**					use constants in dbms.h
**          .scf_scm                    the memory request block with details
**              .scm_functions          one of the constants below or 0
**                  SCU_MZERO_MASK      zero the memory before returning
**		    SCU_MERASE_MASK	"erase" the memory (DoD) 
**					 ...on systems where applicable
**		    SCU_MNORESTRICT_MASK memory may be place in an unrestricted
**					 area on systems where applicable
**					 (specifically ibm/MVS "above the line")
**		    SCU_LOCKED_MASK 	memory may be locked when applicable.
**              .scm_in_pages           number of pages requested by the caller
**
** Outputs:
**      scf_cb                          control block
**          .scf_error                  error indicator
**              .err_code               E_SC_OK or ...
**                  E_SC0004_NO_MORE_MEMORY
**		    E_SC0107_BAD_SIZE_EXPAND
**                  E_SC0005_LESS_THAN_REQUESTED not as much as expected
**          .scf_scm                    as above
**              .scm_out_pages          the number of pages allocated
**              .scm_addr               the address of the first page allocated
**	Returns:
**	    one of E_DB_{OK, WARNING, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-Jan-86 (fred)
**          Created on jupiter
**	29-Jun-1993 (daveb)
**	    cast arg to CSget_scb correctly.
**	2-Jul-1993 (daveb)
**	    prototyped.
**      07-apr-95 (kch)
**          In this function the variable status is now not set to
**          E_SC0028_SEMAPHORE_DUPLICATE.
**          Previously, if status = E_CS0017_SMPR_DEADLOCK, then status
**          was set to E_SC0028_SEMAPHORE_DUPLICATE.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**	02-dec-1998 (nanpr01)
**	    Allow mlock calls for private cache. 
*/
DB_STATUS
scu_malloc(SCF_CB  *scf_cb, SCD_SCB *scb )
{
    i4			error;
    i4			delete = 0;
    SCS_MCB             *mcb;
    SCS_MCB		*mcb_list;
    DB_STATUS           status;

    status = E_DB_OK;
    CLRDBERR(&scf_cb->scf_error);

    for (;;)
    {
	/*
	** First, the parameter validation.  Must be a request
	** for > 0 pages, by a facility with a valid identification.
	** Finally, the function code must be valid.
	*/

	if (scf_cb->scf_scm.scm_in_pages < 0)
	{
	    SETDBERR(&scf_cb->scf_error, 0, E_SC0015_BAD_PAGE_COUNT);
	    status = E_DB_ERROR;
	    break;
	}

	if ((scf_cb->scf_scm.scm_functions & 
		~(SCU_MZERO_MASK | SCU_MERASE_MASK | SCU_MNORESTRICT_MASK |
		  SCU_LOCKED_MASK))
		    != 0)
	{
	    SETDBERR(&scf_cb->scf_error, 0, E_SC0019_BAD_FUNCTION_CODE);
	    status = E_DB_ERROR;
	    break;
	}

	/* Note that zeroing and erasing are mutually exclusive */

	if ((scf_cb->scf_scm.scm_functions & 
		(SCU_MZERO_MASK | SCU_MERASE_MASK)) ==
			    (SCU_MZERO_MASK | SCU_MERASE_MASK))
	{
	    SETDBERR(&scf_cb->scf_error, 0, E_SC0019_BAD_FUNCTION_CODE);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Once arriving here, parameters are OK,
	** proceed with the processing
	*/

	status = sc0m_allocate(0, (i4)sizeof(SCS_MCB), MCB_ID,
		    (PTR)SCU_MEM, MCB_ASCII_ID, (PTR *) &mcb);
	if (status != E_SC_OK)
	{
	    SETDBERR(&scf_cb->scf_error, 0, status);
	    status = E_DB_FATAL;
	    break;
	}

	mcb->mcb_session = scf_cb->scf_session;
	mcb->mcb_facility = scf_cb->scf_facility;
	status = sc0m_get_pages(scf_cb->scf_scm.scm_functions,
			scf_cb->scf_scm.scm_in_pages,
			&scf_cb->scf_scm.scm_out_pages,
			&scf_cb->scf_scm.scm_addr);
	if (status != E_SC_OK)
	{
	    SETDBERR(&scf_cb->scf_error, 0, status);
	    status = E_DB_FATAL;
	    break;
	}

	mcb->mcb_page_count = scf_cb->scf_scm.scm_out_pages;
	mcb->mcb_start_address = scf_cb->scf_scm.scm_addr;
	
	/*
	** Insert this mcb on all appropriate lists
	** and/or queues.  These include the global
	** memory list and the session memory list.
	*/

	if (status = CSp_semaphore(TRUE, &Sc_main_cb->sc_mcb_semaphore))
	{
	    SETDBERR(&scf_cb->scf_error, 0, status);
	    status = E_DB_FATAL;
	    break;
	}

	for(mcb_list = Sc_main_cb->sc_mcb_list->mcb_next;
		mcb_list != Sc_main_cb->sc_mcb_list;
		mcb_list = mcb_list->mcb_next)
	{
	    if (mcb_list->mcb_start_address >= mcb->mcb_start_address)
		break;
	}
	if ((mcb_list != Sc_main_cb->sc_mcb_list) &&
		(mcb_list->mcb_start_address == mcb->mcb_start_address))
	{
	    sc0ePut(NULL, E_SC0102_MEM_CORRUPT, NULL, 0);
	    SETDBERR(&scf_cb->scf_error, 0, E_SC0102_MEM_CORRUPT);
	    status = E_DB_FATAL;
	    break;
	}
	else if ((mcb_list != Sc_main_cb->sc_mcb_list) &&
		    (mcb_list->mcb_facility == mcb->mcb_facility) &&
		    (mcb_list->mcb_session == mcb->mcb_session) &&
		    (( ((char *) mcb->mcb_start_address) +
			(mcb->mcb_page_count * SCU_MPAGESIZE)) ==
		mcb_list->mcb_start_address))
	{
	    /*
	    ** then we can coallesce these mcb's -- they are next
	    ** to each other
	    */

	    mcb_list->mcb_page_count += mcb->mcb_page_count;
	    mcb_list->mcb_start_address = mcb->mcb_start_address;
	    delete = 1;
	}
	else if (   (mcb_list->mcb_prev != Sc_main_cb->sc_mcb_list) &&
		    (mcb_list->mcb_prev->mcb_facility == mcb->mcb_facility) &&
		    (mcb_list->mcb_prev->mcb_session == mcb->mcb_session) &&
		    (( ((char *) mcb_list->mcb_prev->mcb_start_address) +
			    (mcb_list->mcb_prev->mcb_page_count * SCU_MPAGESIZE))
			== mcb->mcb_start_address))
	{
	    mcb_list->mcb_prev->mcb_page_count += mcb->mcb_page_count;
	    delete = 1;
	}	    
	else
	{
	    /* not coallescable */
	    mcb->mcb_next = mcb_list;
	    mcb->mcb_prev = mcb_list->mcb_prev;
	    mcb->mcb_next->mcb_prev = mcb;
	    mcb->mcb_prev->mcb_next = mcb;
	}
	CSv_semaphore(&Sc_main_cb->sc_mcb_semaphore);

	if ((!delete) && mcb->mcb_session != DB_NOSESSION)
	{
	    mcb->mcb_sque.sque_next =
		scb->scb_sscb.sscb_mcb.mcb_sque.sque_prev->mcb_sque.sque_next;
	    mcb->mcb_sque.sque_prev =
		scb->scb_sscb.sscb_mcb.mcb_sque.sque_prev;
	    scb->scb_sscb.sscb_mcb.mcb_sque.sque_prev->mcb_sque.sque_next =
		mcb;
	    scb->scb_sscb.sscb_mcb.mcb_sque.sque_prev = mcb;
	}

	if (scf_cb->scf_scm.scm_in_pages != scf_cb->scf_scm.scm_out_pages)
	{
	    SETDBERR(&scf_cb->scf_error, 0, E_SC0005_LESS_THAN_REQUESTED);
	    status = E_DB_WARN;
	}
	if (delete)
	{
	    sc0m_deallocate(0, (PTR *)&mcb);
	}
	break;
    }
    if (status == E_DB_FATAL)
    {
	/* then something nasty has happened, and we should log the error */
	uleFormat(&scf_cb->scf_error, 0, NULL, ULE_LOG, NULL, 0, 0, 0, &error, 0);
    }
    return(status);
}

/*{
** Name: scu_mfree	- free some previously allocated memory
**
** Description:
**      This function frees some previously scu_malloc'd memory 
**      making it available for use to other facilities or to other 
**      sessions.  The same rules apply as did for scu_malloc(), 
**      namely that this is long term memory which is managed 
**      in units of SCU_MPAGESIZE byte pages.
**
** Inputs:
**      SCU_MFREE                       op code to scf
**      scf_cb                          control block telling us about it
**          .scf_session                session id
**          .scf_facility               facility id as above
**          .scf_scm                    memory request block 
**		.scm_in_pages		number of pages to free
**              .scm_addr               address of memory to be freed
**
** Outputs:
**      scf_cb
**          .scf_error
**              .err_code               E_SC_OK or ...
**                  E_SC0006_NOT_ALLOCATED This memory is not allocated
**					    to you or at all
**	Returns:
**	    One of E_DB_{OK, WARNING, ERROR, FATAL}
**	Exceptions:
**
**
** Side Effects:
**	    The memory is freed and may be unmapped so that the caller
**	    can no longer access it.
**
** History:
**	20-Jan-86 (fred)
**          Created on Jupiter
**	2-Jul-1993 (daveb)
**	    prototyped.
**      07-apr-95 (kch)
**          In this function the variable status is now not set to
**          E_SC0028_SEMAPHORE_DUPLICATE.
**          Previously, if status = E_CS0017_SMPR_DEADLOCK, then status
**          was set to E_SC0028_SEMAPHORE_DUPLICATE.
*/
DB_STATUS
scu_mfree(SCF_CB  *scf_cb, SCD_SCB *scb )
{
    DB_STATUS           status;
    i4			error;
    i4			deleted = 0;
    SCS_MCB		*mcb;

    status = E_DB_OK;
    CLRDBERR(&scf_cb->scf_error);

    /*
    ** Validate parameters.  Session must be valid,
    ** facility must be valid.  Both must be the owners
    ** of the memory in question.  Must actually
    ** be >= the requested number of pages at the address
    ** to be freed.
    */

    for (;;)	    /* just need something to break out of */
    {
	if (scf_cb->scf_scm.scm_in_pages <= 0)
	{
	    SETDBERR(&scf_cb->scf_error, 0, E_SC0015_BAD_PAGE_COUNT);
	    status = E_DB_ERROR;
	    break;
	}

	status = scu_mfind_mcb(scf_cb, &mcb, &deleted);
	if (status != E_SC_OK)
	    break;

	if (	(mcb->mcb_session != scf_cb->scf_session)
	     ||	(mcb->mcb_facility != scf_cb->scf_facility))
	{
	    SETDBERR(&scf_cb->scf_error, 0, E_SC0006_NOT_ALLOCATED);
	    status = E_DB_ERROR;
	    break;
	}

	/* upon arrival here, we know that the request is OK to perform */

	/* first unlink the mcb from the queues/lists in which it resides */


	if (!deleted)
	{
	    mcb->mcb_facility = NO_FACILITY;

	    /* first take it off the session queue, if appropriate */
	    if (mcb->mcb_session != DB_NOSESSION)
	    {
		mcb->mcb_sque.sque_next->mcb_sque.sque_prev
			= mcb->mcb_sque.sque_prev;
		mcb->mcb_sque.sque_prev->mcb_sque.sque_next
			= mcb->mcb_sque.sque_next;
	    }

	    
	    /* now unlink the mcb from the general mcb list and return it */

	    if (status = CSp_semaphore(TRUE, &Sc_main_cb->sc_mcb_semaphore))
	    {
		SETDBERR(&scf_cb->scf_error, 0, status);
		status = E_DB_ERROR;
		break;
	    }
	    mcb->mcb_next->mcb_prev = mcb->mcb_prev;
	    mcb->mcb_prev->mcb_next = mcb->mcb_next;
	    CSv_semaphore(&Sc_main_cb->sc_mcb_semaphore);

	    if ( status = sc0m_deallocate(0, (PTR *) &mcb) )
	    {
		SETDBERR(&scf_cb->scf_error, 0, status);
		status = E_DB_ERROR;
		break;
	    }
	}
	if ( status = sc0m_free_pages(scf_cb->scf_scm.scm_in_pages,
				 &scf_cb->scf_scm.scm_addr) )
	{
	    SETDBERR(&scf_cb->scf_error, 0, status);
	    status = E_DB_ERROR;
	    break;
	}

	break;
    }

    if ( status )
    {
	uleFormat(&scf_cb->scf_error, 0, NULL, ULE_LOG, NULL, 0, 0, 0, &error, 0);
#ifdef xDEBUG
	TRdisplay("SCU_MFREE error occured in call from facility = %w(%<%d.)\n",
	    "<unknown>,CLF,ADF,DMF,OPF,PSF,QEF,QSF,RDF,SCF,ULF,DUF",
	    scf_cb->scf_facility);
#endif
    }
    return(status);
}

/*
** Name: scu_mfind_mcb	- find the mcb which matches the memory to be freed
**
** Description:
**      This function searches the server's mcb list to find the mcb 
**      for the memory specified in the parameter.  Upon finding it, 
**      a successful status is returned.  If the memory cannot be found, 
**      and error is returned.
**
** Inputs:
**      cb                              the request block from the
**					user which specifies the mem in question
**
** Outputs:
**      mcb                             the address of the mcb which is found
**	*deleted			non-zero if mcb already deleted
**					0 otherwise
**	Returns:
**	    E_DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    [@description_or_none@]
**
** History:
**	28-mar-86 (fred)
**          Created on Jupiter.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	11-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
*/
DB_STATUS
scu_mfind_mcb(SCF_CB *scf_cb ,
	      SCS_MCB **mcb ,
	      i4  *deleted )
{
    SCS_MCB             *cur_mcb = Sc_main_cb->sc_mcb_list->mcb_next;
    SCS_MCB		*new_mcb;
    STATUS		status = E_DB_OK;

    *deleted = 0;
    if (status = CSp_semaphore(TRUE, &Sc_main_cb->sc_mcb_semaphore))
    {
	SETDBERR(&scf_cb->scf_error, 0, status);
	return(E_DB_ERROR);
    }

    while (cur_mcb != Sc_main_cb->sc_mcb_list)
    {
	/*
	** check out this mcb.  It must match in terms of 
	** number of pages, address, facility and session
	*/

	if (	(cur_mcb->mcb_session == scf_cb->scf_session)
	    &&	(cur_mcb->mcb_facility == scf_cb->scf_facility)
	    && (scf_cb->scf_scm.scm_addr >= cur_mcb->mcb_start_address)
	    && (scf_cb->scf_scm.scm_addr <
			((char *) cur_mcb->mcb_start_address +
				(cur_mcb->mcb_page_count * SCU_MPAGESIZE))) )
		break;
	cur_mcb = cur_mcb->mcb_next;
    }

    /* Now verify that all the constraints match up */
    if (    (cur_mcb == Sc_main_cb->sc_mcb_list) ||
	    (cur_mcb->mcb_page_count < scf_cb->scf_scm.scm_in_pages) ||
	    ( (((char *) scf_cb->scf_scm.scm_addr) +
		    (scf_cb->scf_scm.scm_in_pages * SCU_MPAGESIZE)) >
		(((char *) cur_mcb->mcb_start_address) +
		    (cur_mcb->mcb_page_count * SCU_MPAGESIZE)))	)
    {
	if (cur_mcb == Sc_main_cb->sc_mcb_list)
	    SETDBERR(&scf_cb->scf_error, 0, E_SC0006_NOT_ALLOCATED);
	else if (cur_mcb->mcb_page_count < scf_cb->scf_scm.scm_in_pages)
	    SETDBERR(&scf_cb->scf_error, 0, E_SC0020_WRONG_PAGE_COUNT);
	else
	    SETDBERR(&scf_cb->scf_error, 0, E_SC0021_BAD_ADDRESS);
	status = E_DB_ERROR;
    }
    else if (	(cur_mcb->mcb_start_address == scf_cb->scf_scm.scm_addr)
	    &&	(cur_mcb->mcb_page_count == scf_cb->scf_scm.scm_in_pages))
    {
	*mcb = cur_mcb;
    }
    else if (cur_mcb->mcb_start_address == scf_cb->scf_scm.scm_addr)
    {
	if (ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_MEMLOG, NULL, NULL))
	    TRdisplay("MFREE: Partial delete from front--was pages: %x, addr: %p\n",
		    cur_mcb->mcb_page_count, cur_mcb->mcb_start_address);
	cur_mcb->mcb_page_count -= scf_cb->scf_scm.scm_in_pages;
	cur_mcb->mcb_start_address = (PTR) ((char *) cur_mcb->mcb_start_address
					+ (scf_cb->scf_scm.scm_in_pages *
						SCU_MPAGESIZE));
	if (ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_MEMLOG, NULL, NULL))
	    TRdisplay("MFREE: losing %x pages yields -- pages: %x, addr: %p\n",
		    scf_cb->scf_scm.scm_in_pages,
		    cur_mcb->mcb_page_count,
		    cur_mcb->mcb_start_address);
	*mcb = cur_mcb;
	*deleted = 1;
    }
    else if (((char *) cur_mcb->mcb_start_address +
		    (cur_mcb->mcb_page_count * SCU_MPAGESIZE)) ==
	     ((char *) scf_cb->scf_scm.scm_addr +
		    (scf_cb->scf_scm.scm_in_pages * SCU_MPAGESIZE)))
    {
	if (ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_MEMLOG, NULL, NULL))
	    TRdisplay("MFREE: Partial delete from end--was pages: %x, addr: %p\n",
		    cur_mcb->mcb_page_count, cur_mcb->mcb_start_address);
	cur_mcb->mcb_page_count -= scf_cb->scf_scm.scm_in_pages;
	if (ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_MEMLOG, NULL, NULL))
	    TRdisplay("MFREE: --- Yields -- pages: %x, addr: %p\n",
		    cur_mcb->mcb_page_count, cur_mcb->mcb_start_address);
	*mcb = cur_mcb;
	*deleted = 1;
    }
    else
    {
	/*
	** This one is messy.  We have to split the mcb to allow for the
	** hole in the middle which we are/have deleted.  The new mcb is
	** inserted before cur_mcb.
	*/
	if (ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_MEMLOG, NULL, NULL))
	{
	    TRdisplay("MFREE: SPLIT--was pages: %x, addr: %p\n",
		    cur_mcb->mcb_page_count, cur_mcb->mcb_start_address);
	    TRdisplay("MFREE: SPLIT -- losing %x pages, addr: %p\n",
		    scf_cb->scf_scm.scm_in_pages,
		    scf_cb->scf_scm.scm_addr);
	}

	status = sc0m_allocate(0, (i4)sizeof(SCS_MCB), MCB_ID,
	    (PTR)SCU_MEM, MCB_ASCII_ID, (PTR *) &new_mcb);
	if ( status )
	{
	    SETDBERR(&scf_cb->scf_error, 0, status);
	    status = E_DB_ERROR;
	}
	else
	{
	    new_mcb->mcb_session = cur_mcb->mcb_session;
	    new_mcb->mcb_facility = cur_mcb->mcb_facility;
	    new_mcb->mcb_start_address = cur_mcb->mcb_start_address;
	    cur_mcb->mcb_start_address = (PTR) ((char *) scf_cb->scf_scm.scm_addr +
					(scf_cb->scf_scm.scm_in_pages * SCU_MPAGESIZE));
	    new_mcb->mcb_page_count = ((i4) ( ((char *) scf_cb->scf_scm.scm_addr) -
						(char *) new_mcb->mcb_start_address))
					    / SCU_MPAGESIZE;
	    cur_mcb->mcb_page_count = cur_mcb->mcb_page_count
					- new_mcb->mcb_page_count
					- scf_cb->scf_scm.scm_in_pages; 
	    
	    new_mcb->mcb_prev = cur_mcb->mcb_prev;
	    new_mcb->mcb_next = cur_mcb;
	    new_mcb->mcb_prev->mcb_next = new_mcb;
	    cur_mcb->mcb_prev = new_mcb;
	    if (ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_MEMLOG, NULL, NULL))
	    {
		TRdisplay("MFREE: SPLIT--before pages: %x, addr: %p\n",
			    new_mcb->mcb_page_count, new_mcb->mcb_start_address);
		TRdisplay("MFREE: SPLIT--after pages: %x, addr: %p\n",
			    cur_mcb->mcb_page_count, cur_mcb->mcb_start_address);
	    }
	    if (cur_mcb->mcb_session != DB_NOSESSION)
	    {
		/* then tag into the session list, too */
		new_mcb->mcb_sque.sque_next = cur_mcb;
		new_mcb->mcb_sque.sque_prev =
			    cur_mcb->mcb_sque.sque_prev;
		new_mcb->mcb_sque.sque_prev->mcb_sque.sque_next = new_mcb;
		cur_mcb->mcb_sque.sque_prev = new_mcb;
	    }
	    *mcb = new_mcb;
	    *deleted = 1;
	}
    }

    CSv_semaphore(&Sc_main_cb->sc_mcb_semaphore);
    return(status);
}

/*
** Name: scu_mcount	- Count the number of pages owned by a session
**
** Description:
**      This function runs thru the mcb list for an SCB and determines
**      the count of pages owned by this session.  Nothing too complicated.
**
** Inputs:
**      scb                             the scb in question
**
** Outputs:
**      count                           the count to be returned
**	Returns:
**	    E_DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	2-Apr-86 (fred)
**          Created on Jupiter
**	2-Jul-1993 (daveb)
**	    prototyped.
*/
i4
scu_mcount(SCD_SCB *scb ,
	   i4  *count )
{
    i4                  pages = 0;
    SCS_MCB             *cur_mcb;

    cur_mcb = scb->scb_sscb.sscb_mcb.mcb_sque.sque_next;
    while (cur_mcb != &scb->scb_sscb.sscb_mcb)
    {
	pages += cur_mcb->mcb_page_count;
	cur_mcb = cur_mcb->mcb_sque.sque_next;
    }
    *count = pages;
    return(E_SC_OK);
}

/*
** Name: scu_mdump	- dump the mcb list, given the header
**
** Description:
**      This routine dumps (using TRdisplay) the mcb list given by 
**      a particular header.  The header address is passed in. 
**      This routine is available ONLY when xDEBUG is defined.
**
** Inputs:
**      head                            The head of the mcb list
**	by_session			If (by_session) then print the session
**					    mcb list else the servers
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
**	1-Apr-86 (fred)
**          Created on Jupiter
**	2-Jul-1993 (daveb)
**	    prototyped.
**	11-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	7-Sep-2005 (schka24)
**	    Don't check pool-object length, sc0m likes to round it up and
**	    it's not worth figuring out what it rounds it up to. 
*/
DB_STATUS
scu_mdump(SCS_MCB *head ,
	  i4	  by_session )
{
    SCS_MCB             *current;
    i4			got_sem = 0;
    STATUS		status;

    if (!by_session)
    {
	if (status = CSp_semaphore(TRUE, &Sc_main_cb->sc_mcb_semaphore))
	    return(status);
	got_sem = 1;
    }
    if ((!head)
	||	(!(current = head->mcb_next))
	||	(by_session && (!(current = head->mcb_sque.sque_next)))
       )
    {
	if (got_sem)
    	    CSv_semaphore(&Sc_main_cb->sc_mcb_semaphore);

	return(E_DB_ERROR);
    }

    while (current != head)
    {
	if (    (!current)
	    ||  (current->mcb_type != MCB_ID)
	    ||  (current->mcb_ascii_id != MCB_ASCII_ID)
	   )
	{
	    TRdisplay ("%%% Invalid MCB found at %p ***\n", current);
	    break;
	}
	TRdisplay("*** MCB at %p ***\n", current);
	TRdisplay("\tmcb_next: %p\tmcb_sque.sque_next: %p\n",
	    current->mcb_next, current->mcb_sque.sque_next);
	TRdisplay("\tmcb_prev: %p\tmcb_sque.sque_prev: %p\n",
	    current->mcb_prev, current->mcb_sque.sque_prev);
	TRdisplay("\tmcb_length:%d.\t\tmcb_session: %p\n",
	    current->mcb_length, current->mcb_session);
	TRdisplay("\tmcb_type: %w %<(%d.)\tmcb_facility: %w %<(%x)\n",
	    "<unknown>,MCB_ID,<unknown>", current->mcb_type,
	    "no_facility,CLF,ADF,DMF,OPF,PSF,QEF,QSF,RDF,SCF,ULF,DUF,GCF,RQF,TPF,GWF,SXF,URS,ICE,CUF",
	    current->mcb_facility);
	TRdisplay("\tmcb_owner: %w %<(%d.\tmcb_page_count: %d. %<(%x)\n",
	    "SCF_MEM,SCU_MEM,SCC_MEM,SCS_MEM,SCD_MEM", current->mcb_owner,
	    current->mcb_page_count);
	TRdisplay("\tmcb_ascii_id: %4t\tmcb_start_address: %p\n",
	    4, &current->mcb_ascii_id, current->mcb_start_address);

	if (by_session)
	    current = current->mcb_sque.sque_next;
	else
	    current = current->mcb_next;
    }
    if (got_sem)
        CSv_semaphore(&Sc_main_cb->sc_mcb_semaphore);
   
    return(E_DB_OK);
}
