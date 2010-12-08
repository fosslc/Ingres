/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <adf.h>
#include    <ade.h>
#include    <cs.h>
#include    <lk.h>
#include    <scf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <qefmain.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefscb.h>
#include    <qefdsh.h>
#include    <ex.h>
#include    <dmmcb.h>
#include    <dmucb.h>
#include    <tm.h>

#include    <dudbms.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <sxf.h>
#include    <qefprotos.h>

/**
**
**  Name: QENUTL.C - hold file utility functions.
**
**  Description:
**          qen_u1_append - append to memory or dmf hold file
**          qen_u2_position_tid - position rescan at saved tid
**          qen_u3_position_begin - position rescan at hold file beginning
**          qen_u4_read_positioned - read next row from hold file
**          qen_u5_save_position - save this position
**          qen_u6_clear - clear in memory hold for re-use
**          qen_u7_to_unget - copy row buffer to unget buffer
**	    qen_u8_current_tid - return current hold file tid  
**	    qen_u9_dump_hold - dump/clear hold for reset
**	    qen_u9a_void_hold - voids a hold file for the clear/dump routines
**	    qen_execute_cx - call ade_execute_cx and handle returns
**	    qen_u31_release_mem - close_stream on sort mem when done
**	    qen_u32_dosort - call inner child sort
**	    qen_u33_resetSortFile - reset a SHD
**	    qen_u40_readInnerJoinFlag - read an inner join flag
**	    qen_u41_storeInnerJoinFlag - store an inner join flag
**	    qen_u42_replaceInnerJoinFlag - change an inner join flag
**	    qen_u101_open_dmf - open dmt for hold file
**	    qen_u102_spill - spill mem tuples to dmf  
**	    qen_putIntoTempTable - Put a tuple into a temporary table.
**	    qen_initTempTable - initialize a temporary table
**	    qen_rewindTempTable - position a temporary table at beginning
**	    qen_getFromTempTable - get a tuple from a temporary table
**	    qen_destroyTempTable - destroy a temporary table
**	    qen_doneLoadingTempTable - finish loading a temporary table
**	    qen_openAndLink - open table and link DMT-CB into DSH list
**	    qen_closeAndUnlink - close table and unlink from DSH list
**	    qen_set_qualfunc - set DMRCB qualification function
**
**
**  History:
**      14-Nov-89 (davebf)
**          written
**
**	25-mar-92 (rickh)
**	    Upgraded tid hold files to use the same mechanism as whole tuple
**	    hold files.  Removed the following routines:
**		qen_u16_sinit_tid
**	        qen_u11_init_tid
**	        qen_u12_put_tid
**	        qen_u13_find_tid
**
**	    Added the following routines:
**		qen_u15_readTidTuple
**	    	qen_u16_repositionTidHoldFile
**
**	04-may-1992 (bryanp)
**	    Changed dmt_location from a DB_LOC_NAME to a DM_DATA.
**	05-May-92 (rickh)
**	    Numerous changes to fix right fsmjoins.  Most notably, added the
**	    following routines:
**		qen_u40_readInnerJoinFlag
**		qen_u41_storeInnerJoinFlag
**		qen_u42_replaceInnerJoinFlag
**	20-may-92 (rickh)
**	    Good hygiene.  Made all declarations of status initialize it to
**	    E_DB_OK.
**	10-aug-1992 (rickh)
**	    Added temp table management.
**	08-dec-92 (anitap)
**	    Included <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**	24-mar-93 (jhahn)
**	    Fixed bugs in qen_destroyTempTable.
**	11-may-93 (rickh)
**	    When clearing a hold file, release its memory stream and
**	    dump its dmf file if it exists.
**	17-may-93 (jhahn)
**	    Changed TempTable functions to use DMR_LOAD instead of DMR_PUT when
**	    not sorting.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-dec-93 (rickh)
**	    Extracted the guts of qen_u31_release_mem to
**	    qen_u33_resetSortFile so that there will be one way to
**	    release SHDs across QEF.
**	13-jan-94 (rickh)
**	    Fixed a funny cast in qen_u40_readInnerJoinFlag.
**	26-jan-94 (rickh)
**	    When spilling a hold file to disk, check the DMT_CB (not the
**	    DMR_CB) to figure out whether the table needs to be opened.
**	    This makes the spilling code re-usable for repeat queries and
**	    fixes bug 58925.
**	07-feb-94 (rickh)
**	    Enable configuration of the spill threshold for testing.
**	08-feb-94 (rickh)
**	    For sorting TID temp files, split TIDs into two i4s.
**	02-feb-95 (ramra01)
**	    For fsmjoin processing, ungetbuffer may get corrupted
**		bug 65386
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF and QEN_OJINFO structure instances by 
**	    pointers in numerous QEN_NODE structures.
**      19-dec-95 (ramra01)
**          Reverting to rev 13, scrapping the reuse FSMJoin node
**          buffer optimization
**	06-mar-96 (nanpr01)
**	    For increased tuple size project, get the pagesize for creating
**	    temporary table.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	28-april-98(angusm)
**	    bug 85267: allocate qen unget buffer with ME instead
**	    of ULM.
**	17-mar-1999 (somsa01)
**	    Added include of pc.h and lk.h to resolve references to locking
**	    ids.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-dec-03 (inkdo01)
**	    DSH is now parameter to many utility functions.
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety.
**	27-feb-04 (inkdo01)
**	    Changed dsh ptr to arrays of structs (SHD, STATUS) to
**	    ptrs to arrays of ptrs.
**	15-mar-04 (inkdo01)
**	    Changed dsh ptr to arrays of QEE_TEMP_TABLE structs to 
**	    ptr to array of ptrs.
**	9-Dec-2005 (kschendel)
**	    Fix a few stray i2 casts on mecopy's.
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
**	    Remove 4 routines that were not referenced anywhere.
**	4-Jun-2009 (kschendel) b122118
**	    Add open-and-link, close-and-unlink utilities.  Renamed
**	    qen-sort to qen-inner-sort, fix here.
**	2-Dec-2010 (kschendel) SIR 124685
**	    Warning / prototype fixes.
**/

/*	static functions	*/

static DB_STATUS
qen_u9a_void_hold(
QEN_HOLD   *hold,
QEE_DSH	   *dsh,
QEN_SHD    *shd );

static DB_STATUS
qen_u102_spill(
QEN_HOLD    *hold,
QEE_DSH    *dsh,
QEN_SHD	    *shd );



/*{
** Name: QEN_U1_APPEND	 - append to hold
**
** Description:
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	27-nov-90 (davebf)
**	    written  (adapted from qenjoin.c 6.3/02)
**	07-feb-94 (rickh)
**	    Enable configuration of the spill threshold for testing.
**
*/

DB_STATUS
qen_u1_append(
QEN_HOLD   *hold,
QEN_SHD    *shd,
QEE_DSH    *dsh )
{

    QEF_CB    *qcb = dsh->dsh_qefcb;
    ULM_RCB   ulm_rcb;
    DMR_CB    *dmrcb;
    DB_STATUS status = E_DB_OK;
    i4	      j, k, l;

retry:
    dsh->dsh_error.err_code = E_QE0000_OK;

    if(hold->hold_medium == HMED_IN_MEMORY)
    {

        if(shd->shd_streamid == (PTR) NULL)
        {

	    hold->unget_status = 0;   /* init */
	    
	    if(shd->shd_tup_cnt < 0)   /* if we're supposed to calculate */
	    {

		/* number of hold slots is current session capacity 
		** divided by  num sort-holds which might use memory */

		j = dsh->dsh_shd_cnt;	/* count of potential sort/holds */
		if(j < 1) j = 1;    	/* failsafe */
		k = qcb->qef_s_max / j;		/* sort memory capacity */
		l = shd->shd_width + sizeof(PTR); /* tuple width + array slot */
		shd->shd_tup_cnt = k / l;	    /* capacity for tuples */

		if(shd->shd_tup_cnt < 20)   /* if less than useful size */
		{
		    /* go to dmf hold without attempting to get memory */
		    status = qen_u101_open_dmf(hold, dsh);
		    if(status) return(status);
		    else goto retry;
		}
		/* fall thru with calculated shd_tup_cnt */
	    }

	    /*
	    ** For testing purposes, we may have configured this session's
	    ** spill threshold to some specific number of tuples.
	    */

	    if ( qcb->qef_spillThreshold > QEF_NO_SPILL_THRESHOLD )
	    {	shd->shd_tup_cnt = qcb->qef_spillThreshold; }

	    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_s_ulmcb, ulm_rcb);
	    /* Point ULM to where we want streamid returned */
	    ulm_rcb.ulm_streamid_p = &shd->shd_streamid;

	    /* Open memory stream for the mem_hold buffer */

	    status = ulm_openstream(&ulm_rcb);
	    if (status != E_DB_OK)
	    {
	      /* try to go to dmf hold */
	      status = qen_u101_open_dmf(hold, dsh);
	      if(status) return(status);
	      else goto retry;
	    }

	    /* allocate vector array */

	    ulm_rcb.ulm_psize = shd->shd_tup_cnt * sizeof(PTR);

	    if ((qcb->qef_s_used + ulm_rcb.ulm_psize) > qcb->qef_s_max)
	    {
	      /* try to go to dmf hold */
	      status = qen_u101_open_dmf(hold, dsh);
	      if(status) return(status);
	      else goto retry;
	    }

	    status = ulm_palloc(&ulm_rcb);
	    if (status != E_DB_OK)
	    {
	      /* try to go to dmf hold */
	      status = qen_u101_open_dmf(hold, dsh);
	      if(status) return(status);
	      else goto retry;
	    } 

	    shd->shd_vector = (PTR *)ulm_rcb.ulm_pptr;
	    shd->shd_size = ulm_rcb.ulm_psize;
	    qcb->qef_s_used += ulm_rcb.ulm_psize;

	    shd->shd_dups = 0;        /* ready to start */
	    hold->hold_tid = 0;

	}   /* end if streamid null */   

	if(shd->shd_dups == shd->shd_tup_cnt)   /* in mem full */
	{
	    /* open dmf hold file */
	    status = qen_u101_open_dmf(hold, dsh);
	    if(status)  return(status);

            /* Spill memory buffers to DMF */
	    status = qen_u102_spill(hold, dsh, shd );
	    if(status)  return(status);
	    else goto retry;
	}
	else    /* in memory hold not full */
	{

	    /* allocate tuple and insert addr in vector slot */

	    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_s_ulmcb, ulm_rcb);
	    ulm_rcb.ulm_streamid_p = &shd->shd_streamid;
	    ulm_rcb.ulm_psize = shd->shd_width;

	    if ((qcb->qef_s_used + ulm_rcb.ulm_psize) > qcb->qef_s_max)
	    {
		/* open dmf hold file */
		status = qen_u101_open_dmf(hold, dsh);
		if(status)  return(status);

		/* Spill memory buffers to DMF */
		status = qen_u102_spill(hold, dsh, shd );
		if(status)  return(status);
		else goto retry;
	    }

	    status = ulm_palloc(&ulm_rcb);
	    if (status != E_DB_OK)
	    {
		/* open dmf hold file */
		status = qen_u101_open_dmf(hold, dsh);
		if(status)  return(status);

		/* Spill memory buffers to DMF */
		status = qen_u102_spill(hold, dsh, shd );
		if(status)  return(status);
		else goto retry;
	    }

	    shd->shd_vector[shd->shd_dups] = ulm_rcb.ulm_pptr;
	    shd->shd_size += ulm_rcb.ulm_psize;
	    qcb->qef_s_used += ulm_rcb.ulm_psize;


	    /* copy row into to mem_hold buffer and set new eof index */
	    MEcopy((PTR)shd->shd_row, shd->shd_width,
		    (PTR)shd->shd_vector[shd->shd_dups++]);

	    shd->shd_next_tup = shd->shd_dups;    /* set next tup */  

	    hold->hold_status = HFILE2_AT_EOF;

	    return(E_DB_OK);
	}
    }
    else    /* already on DMF */
    {
       dmrcb = hold->hold_dmr_cb;
       dmrcb->dmr_flags_mask = 0;
       if (status = dmf_call(DMR_PUT, dmrcb))
       {
          dsh->dsh_error.err_code = dmrcb->error.err_code;
	  return(status);
       }

       /* The hold file is no longer positioned to read */
       hold->hold_status = HFILE2_AT_EOF;

       return(E_DB_OK);

    }
}

/*{
** Name: QEN_U2_POSITION_TID - position rescan at saved tid
**
** Description:
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	07-nov-89 (davebf)
**	    written.
**
*/

DB_STATUS
qen_u2_position_tid(
QEN_HOLD   *hold,
QEE_DSH	   *dsh,
QEN_SHD    *shd )
{
    DMR_CB    *dmrcb;
    DB_STATUS status = E_DB_OK;

    if(hold->hold_status == HFILE0_NOFILE)  /* not yet created */
	return(E_DB_OK);

    if( hold->hold_medium == HMED_ON_DMF )
    {
       dmrcb = hold->hold_dmr_cb;
       dmrcb->dmr_position_type = DMR_TID;
       dmrcb->dmr_tid = hold->hold_tid;
       dmrcb->dmr_flags_mask = 0;
       status = dmf_call(DMR_POSITION, dmrcb);
	if (status != E_DB_OK)
	{
	    if (dmrcb->error.err_code == E_DM0055_NONEXT)
	    {
		dsh->dsh_error.err_code = E_QE0002_INTERNAL_ERROR;
		status = E_DB_FATAL;
	    }
	    else
		dsh->dsh_error.err_code = dmrcb->error.err_code;
            return(status);
        }
     }
     else
        shd->shd_next_tup = (i4) hold->hold_tid;

     hold->hold_status = HFILE3_POSITIONED;

     return(E_DB_OK);
}


/*{
** Name: QEN_U3_POSITION_BEGIN - position rescan at beginning
**
** Description:
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	07-nov-89 (davebf)
**	    written.
**
*/

DB_STATUS
qen_u3_position_begin(
QEN_HOLD   *hold,
QEE_DSH	   *dsh,
QEN_SHD    *shd )
{
    DMR_CB    *dmrcb;
    DB_STATUS status = E_DB_OK;

    if(hold->hold_status == HFILE0_NOFILE)  /* not yet created */
	return(E_DB_OK);

    if(hold->hold_medium)          /*if hold_medium == HMED_ON_DMF  */
    {
       dmrcb = hold->hold_dmr_cb;
       dmrcb->dmr_position_type = DMR_ALL;
       dmrcb->dmr_flags_mask = 0;
       status = dmf_call(DMR_POSITION, dmrcb);
	if (status != E_DB_OK)
	{
	    if (dmrcb->error.err_code == E_DM0055_NONEXT)
	    {
		dsh->dsh_error.err_code = E_QE0002_INTERNAL_ERROR;
		status = E_DB_FATAL;
	    }
	    else
		dsh->dsh_error.err_code = dmrcb->error.err_code;
            return(status);
        }
     }
     else
        shd->shd_next_tup = 0;

     hold->hold_status = HFILE3_POSITIONED;

     return(E_DB_OK);
}



/*{
** Name: QEN_U4_READ_POSITIONED - read hold at position
**
** Description:
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	07-nov-89 (davebf)
**	    written.
**
*/

DB_STATUS
qen_u4_read_positioned(
QEN_HOLD   *hold,
QEE_DSH	   *dsh,
QEN_SHD    *shd )
{

    DMR_CB     *dmrcb;
    DB_STATUS status = E_DB_OK;

    if(hold->hold_medium == HMED_IN_MEMORY)   
    {

	/* Check for the end of buffer */
	if (shd->shd_next_tup == shd->shd_dups)
	{
	    dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
	    return(E_DB_ERROR);    
	}
	else
	{
	    /* Copy tuple to the row buffer */
	    MEcopy((PTR)(shd->shd_vector[shd->shd_next_tup++]), 
		    shd->shd_width, (PTR)shd->shd_row);
	    return(E_DB_OK);
	}

    }
    else                /* HMED_ON_DMF */
    {
	dmrcb = hold->hold_dmr_cb;
	dmrcb->dmr_flags_mask = DMR_NEXT;
	if ((status = dmf_call(DMR_GET, dmrcb)) != E_DB_OK)
	{
	    if (dmrcb->error.err_code == E_DM0055_NONEXT)
	    {
	    dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
	    return(E_DB_ERROR);    
	    }
	    else
	    {
		dsh->dsh_error.err_code = dmrcb->error.err_code;
		return(status);               
	    }
	} 
	return(E_DB_OK); 
    }
}

/*{
** Name: QEN_U5_SAVE_POSITION - save current position in hold for rescan
**
** Description:
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	07-nov-89 (davebf)
**	    written.
**
*/

DB_STATUS
qen_u5_save_position(
QEN_HOLD   *hold,
QEN_SHD    *shd )
{

    DMR_CB	*dmrcb;

     if(hold->hold_medium)
     {
		dmrcb = hold->hold_dmr_cb;
		hold->hold_tid = dmrcb->dmr_tid;
     }
     else
		hold->hold_tid = shd->shd_next_tup - 1;

     return(E_DB_OK);
}


/*{
** Name: QEN_U6_CLEAR_HOLD - set up to reuse in memory hold
**
** Description:
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	07-nov-89 (davebf)
**	    written.
**	4-feb-92 (rickh)
**	    Mark the hold file as empty.  This prevents FSMJOINs from trying
**	    to read from a hold file that has been cleared.
**	11-may-93 (rickh)
**	    When clearing a hold file, release its memory stream and
**	    dump its dmf file if it exists.
*/

DB_STATUS
qen_u6_clear_hold(
QEN_HOLD   *hold,
QEE_DSH	   *dsh,
QEN_SHD    *shd )
{
    DB_STATUS status = E_DB_OK;

    status = qen_u9a_void_hold( hold, dsh, shd);

    shd->shd_vector = (PTR *)NULL;
    shd->shd_next_tup = 0;
    shd->shd_dups = 0;          /* set up to reuse  */
    hold->hold_medium = HMED_IN_MEMORY;   /* go back to memory */
    hold->hold_status = HFILE1_EMPTY;	/* de-position the hold file */

    return( status );
}

/*{
** Name: QEN_U7_TO_UNGET - save current row buffer in unget
**
** Description:
**	This call will always follow first put to hold file and
**	only occurs when join node establishes its own hold.
**	The buffer is established on first use.  For the case
**	where the hold is on disk, a memory stream must be opened  
**	for this buffer.  
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	07-nov-89 (davebf)
**	    written.
**	29-sep-92 (rickh)
**	    Always return the status longword.  Stack noise isn't good enough.
**	02-feb-95 (ramra01)
**	    Allocate unget buffer every time we call it. bug 65386
**	22-jul-97 (inkdo01)
**	    Tried again. This time alloc unget once, but in more persistent
**	    stream.
**	27-apr-98(angusm)
**	    Use MEreqmem/MEfree for this: previous metod of allocating
**	    this buffer each time from ULM consumes too much memory
**	    from QEF sort pool for large FSM joins. Any size of FSM join 
**	    should be able to execute with default QEF memory.
**	    (bug 85267).
*/

DB_STATUS
qen_u7_to_unget(
QEN_HOLD   *hold,
QEN_SHD    *shd,
QEE_DSH    *dsh )
{

    DB_STATUS status = E_DB_OK;

/*  If we only allocate the unget_buffer if it is a new hold struct */
/*  we could potentially be pointing to a unallocated location      */
/*  at the time of MEcopy buffer value with a bad addr ptr due  to  */
/*  the memory stream beig freed by ClearHoldFile when the inner key*/
/*  changes. With the curent fix, it enures you have a new alloc    */
/*  each time and does get freed by ClearHoldFile, but you will get */
/*  your value from the unget_buffer as the chances of the location */
/*  being reused is remote, with a newer copy.			    */
/*								    */
/*  Above was a good idea, but shd stream is also freed by VoidHold */
/*  Ultimate solution is to allocate it once (as before), but in    */
/*  the DSH memory stream (which hangs around!).		    */
/*								    */
/*  Above was a good idea, except the memory was never getting      */
/*  released, causing a QEF memory leak.  Avoid using stream and    */
/*  just allocate a piece of memory, to be freed later.             */

    if(hold->unget_buffer == (PTR)NULL)
    {
        hold->unget_buffer = MEreqmem( (u_i4)0,
				      (u_i4)shd->shd_width, 0 , &status);
        if (status != E_DB_OK)
            return(status);    
    }

    MEcopy((PTR)shd->shd_row, shd->shd_width, (PTR)hold->unget_buffer);
    hold->unget_status = HFILE9_OCCUPIED;

    return( status );
}


/*{
** Name: QEN_U8_CURRENT_TID - return current hold file tid
**
** Description:
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	07-nov-89 (davebf)
**	    written.
*/

DB_STATUS
qen_u8_current_tid(
QEN_HOLD   *hold,
QEN_SHD    *shd,
u_i4	   *tidp )
{

    DMR_CB	*dmrcb;

    if(hold->hold_medium)
    {     
	dmrcb = hold->hold_dmr_cb;
	*tidp = dmrcb->dmr_tid;
    }     
    else
	if(hold->hold_status == HFILE2_AT_EOF)
	    *tidp = shd->shd_dups - 1;
	else
	    *tidp = shd->shd_next_tup - 1;

    return(E_DB_OK);
}


/*{
** Name: QEN_U9_DUMP_HOLD - dump hold for reset purposes
**
** Description:
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	07-nov-89 (davebf)
**	    written.
**
*/

DB_STATUS
qen_u9_dump_hold(
QEN_HOLD   *hold,
QEE_DSH	   *dsh,
QEN_SHD    *shd )
{
    DB_STATUS status = E_DB_OK;

    if(hold->hold_status != HFILE2_AT_EOF &&
       hold->hold_status != HFILE3_POSITIONED)
	return(E_DB_OK);

    status = qen_u9a_void_hold( hold, dsh, shd );

    shd->shd_mdata = (DM_MDATA *)NULL;
    shd->shd_tup_cnt = 0;

    hold->hold_status = HFILE2_AT_EOF;
    hold->unget_status = 0;  /* reset unget because inner stream will be reset*/

    return(E_DB_OK);

/* ?? Need to be sure a reposition isn't done after a dump_hold before
**  the first new append to hold */

}

/*{
** Name: QEN_U9A_VOID_HOLD - clear a hold file
**
** Description:
**	Clears out the contents of a hold file for qen_u6_clear_hold
**	and qen_u9_dump_hold.
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	12-may-93 (rickh)
**	    Extracted from qen_u9_dump_hold for use by both it and
**	    qen_u6_clear_hold.
**
*/
static DB_STATUS
qen_u9a_void_hold(
QEN_HOLD   *hold,
QEE_DSH	   *dsh,
QEN_SHD    *shd )
{
    QEF_CB    *qcb;
    DMR_CB    *dmrcb;
    DB_STATUS status = E_DB_OK;
    ULM_RCB   ulm_rcb;

    if( hold->hold_medium == HMED_ON_DMF )
    {
	/* Dump the hold file */
	dmrcb = hold->hold_dmr_cb;
	dmrcb->dmr_flags_mask = 0;
	status = dmf_call(DMR_DUMP_DATA, dmrcb);
	if (status != E_DB_OK)
	{
	    dsh->dsh_error.err_code = dmrcb->error.err_code;
	    return (status);
	}
    }
    else
    {
	/* release memory stream; otherwise sort tuple buffers 
	** would be orphaned */

	/* shd_node, shd_row, and shd_width are left as set by qee_hfile */
	shd->shd_vector = (PTR *)NULL;
	shd->shd_next_tup = 0;
	shd->shd_dups = 0;     
    }

    if (shd->shd_streamid != (PTR)NULL)
    {
        qcb = dsh->dsh_qefcb;
	STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_s_ulmcb, ulm_rcb);
	ulm_rcb.ulm_streamid_p = &shd->shd_streamid;
	qcb->qef_s_used -= shd->shd_size;
	shd->shd_size = 0;
	status = ulm_closestream(&ulm_rcb);
	if (status != E_DB_OK)
	{
	    dsh->dsh_error.err_code = ulm_rcb.ulm_error.err_code;
	    return(status);
	}
	/* ULM has nullified shd_streamid */
    }

    return( status );
}

/*{
** Name: QEN_EXECUTE_CX - execute compiled expression
**
** Description:
**
**	This convenience routine calls ade_execute_cx to execute a
**	compiled expression.  The caller supplies the proper ADE_EXCB.
**	The MAIN segment is executed.  If an error occurs, the QEF
**	adf-error handler is called.
**
**	Despite the "qen" moniker, this routine is OK to use from
**	pretty much anywhere in QEF, as long as it's using a dsh.
**
** Inputs:
**	dsh			Thread data segment header
**	excb			Pointer to CX execution control block.
**				Does nothing if null.
**
** Outputs:
**	Returns E_DB_OK or error (dsh_error filled in).
**	excb->excb_cmp, excb_value may be set by CX
**
** Side Effects:
**
**
** History:
**	07-nov-89 (davebf)
**	    written.
**	24-jan-92 (rickh)
**	    Unlike the other CXs, if the TIDNULL CX is empty, we should
**	    return false.
**	7-nov-95 (inkdo01)
**	    Changes to replace QEN_ADF and QEN_OJINFO structure instances by 
**	    pointers in numerous QEN_NODE structures.
**	2-mar-99 (inkdo01)
**	    Add cases for hash join node.
**	13-Dec-2005 (kschendel)
**	    Can count on qen_ade_cx now.
**	12-Jan-2006 (kschendel)
**	    Scrap and rework entirely to get rid of xcxcb nonsense and
**	    the giant switch.
*/

DB_STATUS
qen_execute_cx(QEE_DSH *dsh, ADE_EXCB *excb)
{
    DB_STATUS	status;

    if (excb == NULL)
	return (E_DB_OK);
    excb->excb_seg = ADE_SMAIN;
    status = ade_execute_cx(dsh->dsh_adf_cb, excb);
    if (status != E_DB_OK)
	status = qef_adf_error(&dsh->dsh_adf_cb->adf_errcb,
			status, dsh->dsh_qefcb, &dsh->dsh_error);
    return(status);
}


/*{
** Name: QEN_U31_RELEASE_MEM - release hold memory when finished
**
** Description:
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	07-nov-89 (davebf)
**	    written.
**	14-dec-93 (rickh)
**	    Extract guts to qen_u33_resetSortFile.  That routine can
**	    then be called whenever we need to reset SHDs.
**
**
*/

DB_STATUS
qen_u31_release_mem(
QEN_HOLD   *hold,
QEE_DSH	   *dsh,
QEN_SHD    *shd )
{

    if(hold->hold_medium != HMED_IN_MEMORY)
	return(E_DB_OK);

    return( qen_u33_resetSortFile( shd, dsh ) );
}

/*{
** Name: QEN_U32_DOSORT - call inner sort child to do sort 
**
** Description:
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	06-apr-90 (davebf)
**	    written.
**	20-nov-03 (inkdo01)
**	    Add support for "open" call to allow parallel query initialization.
**
*/

DB_STATUS
qen_u32_dosort(
QEN_NODE	*sortnode,
QEF_RCB		*rcb,
QEE_DSH		*dsh, 
QEN_STATUS	*node_status,    
QEN_HOLD	*hold,
QEN_SHD         *shd,
i4		function )
{
    DMR_CB	    *dmrcb;
    QEN_STATUS	    *sort_status;
    DB_STATUS	    status = E_DB_OK;


    /* Call the inner node to sort the inner relation and 
    * get the first tuple. */
    dmrcb = hold->hold_dmr_cb;
    dmrcb->dmr_flags_mask = 0;
    status = qen_inner_sort(sortnode, rcb, dsh, dmrcb, function );
    if (status) return(status);

    sort_status = dsh->dsh_xaddrs[sortnode->qen_num]->qex_status;
    node_status->node_u.node_sort.node_sort_status = 
	sort_status->node_u.node_sort.node_sort_status;
    if (sort_status->node_u.node_sort.
		node_sort_flags & QEN1_SORT_LAST_PARTITION)
	hold->hold_status2 |= HFILE_LAST_PARTITION;
		/* tell caller if we're on the last of a bunch
		** of partition groups */

    /* copy sort's shd to mine */
    MEcopy(
	    (PTR)dsh->dsh_shd[sortnode->node_qen.qen_sort.sort_shd],
	    sizeof(QEN_SHD),
	    (PTR)shd);
		
    /* make sure closestream doesn't find a duplicate */
    shd->shd_streamid = (PTR)NULL; 
                 
    if(node_status->node_u.node_sort.node_sort_status != QEN0_QEF_SORT)
	hold->hold_medium = HMED_ON_DMF;
    else
    {
	shd->shd_dups = shd->shd_tup_cnt;      /* set eof index */
	hold->hold_medium = HMED_IN_MEMORY;    /* ensure */     
    }
    hold->hold_status = HFILE2_AT_EOF;         /* setup for repos */
    /* reposition to read first tuple */
    qen_u3_position_begin(hold, dsh, shd );
    /* set no more inner stream */
    node_status->node_u.node_join.node_inner_status = QEN11_INNER_ENDS;

    return(status);
}

/*{
** Name: qen_u33_resetSortFile - reset a SHD
**
** Description:
**	This routine releases the memory and resets most of the fields
**	in a Sort-Hold descriptor.
**
**
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	14-dec-93 (rickh)
**	    Extracted from the guts of qen_u31_release_mem.  Corrected to
**	    not reset the row and width fields since these must persist
**	    for the next user of this data space if this is a repeat query .
**
**
*/

DB_STATUS
qen_u33_resetSortFile(
QEN_SHD    *shd,
QEE_DSH	   *dsh )
{

    DB_STATUS status = E_DB_OK;
    QEF_CB    *qcb = dsh->dsh_qefcb;
    ULM_RCB   ulm_rcb;

    shd->shd_vector = (PTR *)NULL;
    shd->shd_mdata = (DM_MDATA *)NULL;
    shd->shd_tup_cnt = 0;
    shd->shd_next_tup = 0;
    shd->shd_tid = 0;

    /*
    ** shd_row, shd_width, and shd_options aren't reset.  This is because
    ** they must persist for the next user of this data space in case this
    ** this is a repeat query.
    */

    if (shd->shd_streamid != (PTR)NULL)
    {
	STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_s_ulmcb, ulm_rcb);
	ulm_rcb.ulm_streamid_p = &shd->shd_streamid;
	qcb->qef_s_used -= shd->shd_size;
	shd->shd_size = 0;
	status = ulm_closestream(&ulm_rcb);
	if (status != E_DB_OK)
	{
	    dsh->dsh_error.err_code = ulm_rcb.ulm_error.err_code;
	    return(status);
	}
	/* ULM has nullified shd_streamid */
    }
    return(E_DB_OK);

}

/*{
** Name: qen_u40_readInnerJoinFlag - Read flag value for current inner tuple.
**
** Description:
**
**	For right fsmjoins, we maintain a hold file of flags.  Each flag
**	corresponds to an inner tuple that has the same key as the current
**	outer key.  The flag is TRUE if the corresponding inner tuple
**	inner joined with some outer tuple having the current key.
**	Otherwise, the flag is FALSE.  When the key changes, this hold
**	file is rescanned.  All inner tuples which did not inner join
**	with some outer are then emitted as right joins.
**
**
** Inputs:
**
**	hold		descriptor of hold file for inner join flags
**	shd		sort descriptor for that hold file
**	dsh		request control block for this node
**
**
** Outputs:
**
**	innerJoinFlag	flag value that was stored for current inner tuple
**
**	dsh->dsh_error.err_code	value returned from qen_u4_read_positioned
**
** Returns:
**
**	status from qen_u4_read_positioned
**
** Side Effects:
**
**
** History:
**	05-may-92 (rickh)
**	    Created.
**	13-jan-94 (rickh)
**	    Rewrote a funny cast to shut up the HP compiler.
**
*/
DB_STATUS
qen_u40_readInnerJoinFlag(
QEN_HOLD	*hold,
QEE_DSH		*dsh,
QEN_SHD         *shd,
bool		*innerJoinFlag )
{
    DB_STATUS	    status = E_DB_OK;

    status = qen_u4_read_positioned( hold, dsh, shd );
    *innerJoinFlag = *( ( bool * ) shd->shd_row );
    return( status );
}

/*{
** Name: qen_u41_storeInnerJoinFlag - Store flag value for current inner tuple.
**
** Description:
**
**	For a description of the hold file of inner join flags, see the
**	description of qen_u40_readInnerJoinFlag above.  This routine
**	stores a flag value for the current inner tuple.  If there is already
**	a value stored for this inner tuple, this routine will overwrite
**	it.  However, the value will be overwritten only if it has changed
**	from FALSE to TRUE.
**
** Inputs:
**
**	hold		descriptor of hold file for inner join flags
**	shd		sort descriptor for that hold file
**	rcb		request control block for this node
**	dsh		data segment header for this QEP
**	oldValue	previous join state for current inner tuple
**	newValue	new join state for current inner tuple
**
**
** Outputs:
**
**	dsh->dsh_error.err_code	value returned from qen_u1_append or
**				    qen_u42_replaceInnerJoinFlag
** Returns:
**
**	status from qen_u1_append or qen_u42_replaceInnerJoinFlag
**
** Side Effects:
**
**
** History:
**	05-may-92 (rickh)
**	    Created.
**
*/

DB_STATUS
qen_u41_storeInnerJoinFlag(
QEN_HOLD	*hold,
QEN_SHD         *shd,
QEE_DSH		*dsh,
bool		oldValue,
bool		newValue )
{
    DB_STATUS	    status = E_DB_OK;

    *( (bool *) shd->shd_row ) = newValue;

    /*
    ** If the hold file of inner join flags is empty, then unconditionally
    ** store the new value.
    */
    if ( !( hold->hold_status2 & HFILE_REPOSITIONED ) )
    {
	status = qen_u1_append( hold, shd, dsh );
    }
    else	/* only rewrite the value if it has just become true */
    {
	if ( oldValue == FALSE && newValue == TRUE )
	{
	    status = qen_u42_replaceInnerJoinFlag( hold, shd, dsh ); 
	}
    }

    return( status );
}

/*{
** Name: qen_u42_replaceInnerJoinFlag - Overwrite current flag value
**
** Description:
**
**	For a description of the hold file of inner join flags, see the
**	description of qen_u40_readInnerJoinFlag above.  This routine
**	overwrites the join value for the current inner tuple if
**	it has changed from FALSE to TRUE.
**
** Inputs:
**
**	hold		descriptor of hold file for inner join flags
**	shd		sort descriptor for that hold file
**	rcb		request control block for this node
**	dsh		data segment header for this QEP
**
**
** Outputs:
**
** Returns:
**
**	status from DMR_REPLACE (or E_DB_OK if an in memory replace)
**
** Side Effects:
**
**
** History:
**	05-may-92 (rickh)
**	    Created.
**
*/
DB_STATUS
qen_u42_replaceInnerJoinFlag(
QEN_HOLD	*hold,
QEN_SHD         *shd,
QEE_DSH		*dsh )
{
    DMR_CB	    *dmr_cb = hold->hold_dmr_cb;
    i4	    old_flags_mask;
    i4	    old_dmr_tid;    
    DB_STATUS	    status = E_DB_OK;

    dsh->dsh_error.err_code = E_QE0000_OK;

    if(hold->hold_medium == HMED_IN_MEMORY)
    {
	MEcopy((PTR)shd->shd_row, shd->shd_width,
	    (PTR)shd->shd_vector[ shd->shd_next_tup - 1 ]);
    }
    else
    {
	old_flags_mask = dmr_cb->dmr_flags_mask;
	old_dmr_tid = dmr_cb->dmr_tid;

	dmr_cb->dmr_flags_mask = DMR_CURRENT_POS;
	dmr_cb->dmr_tid = 0;
	dmr_cb->dmr_attset = (char *) 0;
	if (status = dmf_call(DMR_REPLACE, dmr_cb))
	{
	    dsh->dsh_error.err_code = dmr_cb->error.err_code;
	    return(status);
	}

	dmr_cb->dmr_flags_mask = old_flags_mask;
	dmr_cb->dmr_tid = old_dmr_tid;
    }

    return( status );
}

/*{
** Name: QEN_U101_OPEN_DMF - create and open temp file
**
** Description:
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	07-nov-89 (davebf)
**	    written.
**	04-may-1992 (bryanp)
**	    Changed dmt_location from a DB_LOC_NAME to a DM_DATA.
**	26-jan-94 (rickh)
**	    When spilling a hold file to disk, check the DMT_CB (not the
**	    DMR_CB) to figure out whether the table needs to be opened.
**	    This makes the spilling code re-usable for repeat queries and
**	    fixes bug 58925.
**	06-mar-96 (nanpr01)
**	    For increased tuple size project, get the pagesize for creating
**	    temporary table.
**	30-Jul-2004 (jenjo02)
**	    Use dsh_dmt_id rather than qef_dmt_id transaction context.
**
*/

DB_STATUS
qen_u101_open_dmf(
QEN_HOLD   *hold,
QEE_DSH    *dsh )
{
    DMR_CB    *dmrcb = hold->hold_dmr_cb;
    DMT_CB    *dmtcb = hold->hold_dmt_cb;
    DMT_CHAR_ENTRY char_array;
    DB_STATUS status = E_DB_OK;
    
    if(dmtcb->dmt_record_access_id != (PTR)NULL)    /* already open */
    {
	hold->hold_medium = HMED_ON_DMF;
	return(E_DB_OK);
    }

    dmtcb->dmt_db_id = dsh->dsh_qefcb->qef_rcb->qef_db_id;
    dmtcb->dmt_tran_id = dsh->dsh_dmt_id;
    dmtcb->dmt_flags_mask = DMT_DBMS_REQUEST;
    MEmove(8, (PTR) "$default", (char) ' ', 
	    sizeof(DB_LOC_NAME),
	    (PTR) dmtcb->dmt_location.data_address);
    dmtcb->dmt_location.data_in_size = sizeof(DB_LOC_NAME);
    dmtcb->dmt_char_array.data_address = (PTR) &char_array;
    dmtcb->dmt_char_array.data_in_size = sizeof(DMT_CHAR_ENTRY);
    char_array.char_id = DMT_C_PAGE_SIZE;
    char_array.char_value = hold->hold_pagesize;

    if (status = dmf_call(DMT_CREATE_TEMP, dmtcb))
    {
	dsh->dsh_error.err_code = dmtcb->error.err_code;
	if (dmtcb->error.err_code == 
		E_DM0078_TABLE_EXISTS)
	{
	    dsh->dsh_error.err_code = 
		    E_QE0050_TEMP_TABLE_EXISTS;
	    status = E_DB_SEVERE;
	}
	return(status);
    }
    dmtcb->dmt_char_array.data_address = 0;
    dmtcb->dmt_char_array.data_in_size = 0;
    dmtcb->dmt_flags_mask = 0;
    if ((status = qen_openAndLink(dmtcb, dsh)) != E_DB_OK)
    {
	return(status);
    }

    /* move the record access id into the DMR_CB */
    dmrcb->dmr_access_id = dmtcb->dmt_record_access_id;

    hold->hold_medium = HMED_ON_DMF;
    return(E_DB_OK);

}

/*{
** Name: QEN_U102_SPILL - spill memory buffers to dmf.
**
** Description:
**
**
** Inputs:
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	25-jan-91 (davebf)
**	    written
**
*/

static DB_STATUS
qen_u102_spill(
QEN_HOLD    *hold,
QEE_DSH	   *dsh,
QEN_SHD	    *shd )
{
    QEF_CB    *qcb = dsh->dsh_qefcb;
    ULM_RCB   ulm_rcb;
    DMR_CB    *dmrcb = hold->hold_dmr_cb;
    DB_STATUS status = E_DB_OK;

    /* Spill memory buffers to DMF */

    /* note dmr_data.data_address should equal shd_row. */

    for(shd->shd_next_tup = 0;
	shd->shd_next_tup < shd->shd_dups;   
	++shd->shd_next_tup)
    {
	dmrcb->dmr_data.data_address = shd->shd_vector[shd->shd_next_tup];
	dmrcb->dmr_flags_mask = 0;
	if (status = dmf_call(DMR_PUT, dmrcb))
	{
	    dsh->dsh_error.err_code = dmrcb->error.err_code;
	    return(status);
	}
	if(shd->shd_next_tup == hold->hold_tid)
	/* save dmr_tid that corresponds to current saved pos || 0 */
	hold->hold_tid = dmrcb->dmr_tid;
    }

    /* The hold file is no longer positioned to read */

    hold->hold_status = HFILE2_AT_EOF;


    /* release memory for in_memory hold */

    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_s_ulmcb, ulm_rcb);
    ulm_rcb.ulm_streamid_p = &shd->shd_streamid;
    qcb->qef_s_used -= shd->shd_size;
    if ((status = ulm_closestream(&ulm_rcb)) != E_DB_OK)
    {
	dsh->dsh_error.err_code = ulm_rcb.ulm_error.err_code;
	return (status);
    }
    /* ULM has nullified shd_streamid */

    dmrcb->dmr_data.data_address = shd->shd_row;   /* reset */
    return(E_DB_OK);
}

/*{
** Name: qen_initTempTable - initialize a temporary table
**
** Description:
**
**	This routine will initialize a temporary table descriptor.  If the 
**	table exists, it will be destroyed.
**
**	At this point, no DMF actions are taken.  The temporary table does
**	not exist.
**
**	TT_EMPTY is set in QEE_TEMP_TABLE.tt_statusFlags.
**
**
**
**
** Inputs:
**	dsh		Data segment header
**	tempTableNumber	index into array of temp table descriptors in DSH
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	10-aug-1992 (rickh)
**	    written 
**
*/
DB_STATUS qen_initTempTable(
	QEE_DSH		*dsh,
	i4		tempTableNumber
)
{
    QEE_TEMP_TABLE  *tempTable = dsh->dsh_tempTables[ tempTableNumber ];
    DB_STATUS status = E_DB_OK;

    for (;;)	/* something to break out of */
    {
        /* destroy the temp table if it exists */

        if ( tempTable->tt_statusFlags & TT_CREATED )
        {
	    status = qen_destroyTempTable( dsh, tempTableNumber );
	    if ( status != E_DB_OK )	break;
        }

        /* initialize the temp table state */

        tempTable->tt_statusFlags = TT_EMPTY;

	break;
    }

    return( status );
}



/*{
** Name: qen_putIntoTempTable - Put a tuple into a temporary table.
**
** Description:
**
**	This routine puts a tuple into the temporary table.  The tuple
**	must have been previously materialized into this temp table's
**	dsh row (as indexed by QEE_TEMP_TABLE.tt_tuple ).
**
**	If this is the first row to be put into the table, then
**	DMF is called to create the temp table.
**
**	TT_EMPTY is turned off in QEE_TEMP_TABLE.tt_statusFlags.
**
**	DMF is called to insert the tuple into the temp table.
**
**	DB_STATUS is returned containing either E_DB_OK or some error
**	further explained by the error code in the dsh.
**
** Inputs:
**	dsh		Thread data segment header
**	tempTableNumber	index into array of temp table descriptors in DSH
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	10-aug-1992 (rickh)
**	    written 
**	17-may-93 (jhahn)
**	    Changed TempTable functions to use DMR_LOAD instead of DMR_PUT when
**	    not sorting.
**	06-mar-96 (nanpr01)
**	    For increased tuple size project, get the pagesize for creating
**	    temporary table. Also change the parameter for tempTableNumber
**	    to qen_tempTable.
**	15-oct-98 (inkdo01)
**	    Turn on "DMR_NOSORT" when no keys are defined for temp table.
**	30-Jul-2004 (jenjo02)
**	    Use dsh_dmt_id rather than qef_dmt_id transaction context.
**
*/
DB_STATUS qen_putIntoTempTable(
	QEE_DSH		*dsh,
	QEN_TEMP_TABLE	*qentempTable
)
{
    i4		    tempTableNumber = qentempTable->ttb_tempTableIndex;
    QEE_TEMP_TABLE  *tempTable = dsh->dsh_tempTables[ tempTableNumber ];
    DMT_CB	    *dmt = tempTable->tt_dmtcb;
    DMR_CB	    *dmr = tempTable->tt_dmrcb;
    DM_MDATA	    dm_mdata;
    DMT_CHAR_ENTRY  char_array;
    DB_STATUS status = E_DB_OK;

    for (;;)	/* something to break out of */
    {
	/* create the table if it doesn't already exist */

	if ( !( tempTable->tt_statusFlags & TT_CREATED ) )
	{
	    dmt->dmt_db_id = dsh->dsh_qefcb->qef_rcb->qef_db_id;
	    dmt->dmt_tran_id = dsh->dsh_dmt_id;
	    MEmove(8, (PTR) "$default", (char) ' ', 
	            sizeof(DB_LOC_NAME),
	            (PTR) dmt->dmt_location.data_address);
	    dmt->dmt_location.data_in_size = sizeof(DB_LOC_NAME);

	    dmt->dmt_flags_mask = DMT_DBMS_REQUEST;
	    dmr->dmr_flags_mask = 0;
	    /*
	    ** We assume that the sort key descriptor was set up
	    ** earlier, probably at QEE time.
	    */

	    if ( tempTable->tt_statusFlags & TT_PLEASE_SORT )
		dmt->dmt_flags_mask |= DMT_LOAD;

	    if ( tempTable->tt_statusFlags & TT_NO_DUPLICATES )
		dmr->dmr_flags_mask |= DMR_NODUPLICATES;

	    if ( dmt->dmt_key_array.ptr_in_count == 0 )
		dmr->dmr_flags_mask |= DMR_NOSORT;

	    dmr->dmr_count = 1;
	    dmr->dmr_s_estimated_records = TT_TUPLE_COUNT_GUESS;
	    dmr->dmr_tid = 0;
	    dm_mdata.data_address = dsh->dsh_row[ tempTable->tt_tuple ];
	    dm_mdata.data_size =
		dsh->dsh_qp_ptr->qp_row_len[ tempTable->tt_tuple ];
	    dmr->dmr_mdata = &dm_mdata;
            /*  Pass the page size */
            dmt->dmt_char_array.data_address = (PTR) &char_array;
            dmt->dmt_char_array.data_in_size = sizeof(DMT_CHAR_ENTRY);
            char_array.char_id = DMT_C_PAGE_SIZE;
            char_array.char_value = qentempTable->ttb_pagesize;

	    status = dmf_call(DMT_CREATE_TEMP, dmt);
	    if (status != E_DB_OK)
	    {
	        if (dmt->error.err_code == E_DM0078_TABLE_EXISTS)
	        {
		    dsh->dsh_error.err_code = E_QE0050_TEMP_TABLE_EXISTS;
		    status = E_DB_ERROR;
	        }
	        else
	        {
		    dsh->dsh_error.err_code = dmt->error.err_code;
	        }
	        break;
	    }

	    /* Open the temp table */
            dmt->dmt_char_array.data_address = 0;
            dmt->dmt_char_array.data_in_size = 0; 
	    dmt->dmt_flags_mask = 0;
	    dmt->dmt_sequence = dsh->dsh_stmt_no;
	    dmt->dmt_access_mode = DMT_A_WRITE;
	    dmt->dmt_lock_mode = DMT_X;
	    dmt->dmt_update_mode = DMT_U_DIRECT;
	
	    status = qen_openAndLink(dmt, dsh);
	    if (status != E_DB_OK)
	    {
	        break;
	    }

            tempTable->tt_statusFlags |= TT_CREATED;
	    dmr->dmr_access_id = dmt->dmt_record_access_id;
	}	/* end of table creation */

	/* call DMF to insert the tuple */

	dm_mdata.data_address = dsh->dsh_row[ tempTable->tt_tuple ];
	dm_mdata.data_size =
	    dsh->dsh_qp_ptr->qp_row_len[ tempTable->tt_tuple ];
	dmr->dmr_mdata = &dm_mdata;

	status = dmf_call( DMR_LOAD, dmr );
	if (status != E_DB_OK)
	{
	    dsh->dsh_error.err_code = dmr->error.err_code; 
	    break;
	}

        tempTable->tt_statusFlags &= ~TT_EMPTY;

	break;
    }	/* end of code block */

    return( status );
}



/*{
** Name: qen_rewindTempTable - position a temporary table at beginning
**
** Description:
**
**	This routine positions a temporary table at beginning of file.
**	If sorting was requested on the table, sorting is finished
**	then the table is backed up to the beginning.
**
** Inputs:
**	dsh		Data segment header
**	tempTableNumber	index into array of temp table descriptors in DSH
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	10-aug-1992 (rickh)
**	    written 
**	17-may-93 (jhahn)
**	    Changed TempTable functions to use DMR_LOAD instead of DMR_PUT when
**	    not sorting.
**
*/
DB_STATUS qen_rewindTempTable(
	QEE_DSH		*dsh,
	i4		tempTableNumber
)
{
    QEE_TEMP_TABLE  *tempTable = dsh->dsh_tempTables[ tempTableNumber ];
    DMR_CB	    *dmr = tempTable->tt_dmrcb;
    DB_STATUS status = E_DB_OK;

    for (;;)	/* something to break out of */
    {
	if ( tempTable->tt_statusFlags & TT_CREATED )
	{
	    status = qen_doneLoadingTempTable( dsh, tempTableNumber );
	    if (status != E_DB_OK)	break;

	    dmr->dmr_position_type = DMR_ALL;
	    dmr->dmr_flags_mask = 0;
	    dmr->dmr_tid = 0;
	    status = dmf_call(DMR_POSITION, dmr);
	    if (status != E_DB_OK)
	    {
	        if (dmr->error.err_code == E_DM0055_NONEXT)
	        {
		    dsh->dsh_error.err_code = E_QE0002_INTERNAL_ERROR;
		    status = E_DB_FATAL;
	        }
	        else
		    dsh->dsh_error.err_code = dmr->error.err_code;

                break;
            }
	}	/* endif table created */

	break;
    }	/* end of code block */

    return( status );
}

/*{
** Name: qen_getFromTempTable - get a tuple from a temporary table
**
** Description:
**
**	This routine gets the next tuple from the temp table into
**	the table's dsh row (as indexed by QEE_TEMP_TABLE.tt_tuple ).
**
**	If dsh->error.err_code == E_QE0015_NO_MORE_ROWS, then
**	there are no more tuples in the table.
**
** Inputs:
**	dsh		data segment header
**	tempTableNumber	index into array of temp table descriptors in DSH
**
** Outputs:
**	E_DB_OK		if tuple successfully returned
**
**	E_DB_ERROR	if end of file encountered
**	    dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
**
**
**
** Side Effects:
**
**
** History:
**	10-aug-1992 (rickh)
**	    written 
**
*/
DB_STATUS qen_getFromTempTable(
	QEE_DSH		*dsh,
	i4		tempTableNumber
)
{
    QEE_TEMP_TABLE  *tempTable = dsh->dsh_tempTables[ tempTableNumber ];
    DMT_CB	    *dmt = tempTable->tt_dmtcb;
    DMR_CB	    *dmr = tempTable->tt_dmrcb;
    DB_STATUS status = E_DB_OK;

    for (;;)	/* something to break out of */
    {
	if ( tempTable->tt_statusFlags & TT_CREATED )
	{

	    dmr->dmr_access_id = dmt->dmt_record_access_id;
	    dmr->dmr_data.data_address = dsh->dsh_row[ tempTable->tt_tuple ];
	    dmr->dmr_data.data_in_size =
	        dsh->dsh_qp_ptr->qp_row_len[ tempTable->tt_tuple ];
	    dmr->dmr_flags_mask = DMR_NEXT;
	    if ((status = dmf_call(DMR_GET, dmr)) != E_DB_OK)
	    {
	        if (dmr->error.err_code == E_DM0055_NONEXT)
	        {
                    dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
		    return(E_DB_ERROR);    
	        }
	        else
	        {
		    dsh->dsh_error.err_code = dmr->error.err_code;
		    return(status);               
                }
            } 
	}
	else	/* table not created yet */
	{
            dsh->dsh_error.err_code = E_QE0015_NO_MORE_ROWS;
	    return(E_DB_ERROR);    
	}	/* endif table created or not */

	break;
    }	/* end of usual and customary code block */

    return( status );
}

/*{
** Name: qen_destroyTempTable - destroy a temporary table
**
** Description:
**
**	This routine gets rid of a temporary table.
**	This destroys the temp table and clears QEE_TEMP_TABLE.tt_statusFlags.
**
**
**
** Inputs:
**	dsh		data segment header
**	tempTableNumber	index into array of temp table descriptors in DSH
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	10-aug-1992 (rickh)
**	    written 
**	24-mar-1993 (jhahn)
**	    added check to see if table had already been closed.
**	    fixed bug when removing table from open table list.
**
*/
DB_STATUS qen_destroyTempTable(
	QEE_DSH		*dsh,
	i4		tempTableNumber
)
{
    QEE_TEMP_TABLE  *tempTable = dsh->dsh_tempTables[ tempTableNumber ];
    DMT_CB	    *dmt = tempTable->tt_dmtcb;
    i4	    err;
    DB_STATUS status = E_DB_OK;

    for (;;)	/* something to break out of */
    {
	if ((tempTable->tt_statusFlags & TT_CREATED) &&
	    (dmt->dmt_record_access_id != NULL))
	{
	    /* close the table */
	    status = qen_closeAndUnlink(dmt, dsh);
	    if (status != E_DB_OK)
	    {
	        qef_error(dmt->error.err_code, 0L, status, &err,
	            &dsh->dsh_error, 0);
	        break;
	    }

	    /* destroy the temporary table */
	    dmt->dmt_flags_mask = 0;
	    status = dmf_call(DMT_DELETE_TEMP, dmt);
	    if (status)
	    {
	        qef_error(dmt->error.err_code, 0L, status, &err,
	            &dsh->dsh_error, 0);
	        break;
	    }
	}	/* endif table was created */

	/* tidy up */

	tempTable->tt_statusFlags = TT_EMPTY;
	break;
    }	/* end of code block */

    return( status );
}



/*{
** Name: qen_doneLoadingTempTable - finish loading a temporary table
**
** Description:
**
**	This routine tells DMF that the loading is done.  If the table
**	is being sorted, this finishes the sort.
**
**	This routine can be called multiple times.  It will flag itself
**	that it's already been called and just return.
**
**
** Inputs:
**	dsh		data segment header
**	tempTableNumber	index into array of temp table descriptors in DSH
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	10-aug-1992 (rickh)
**	    written 
**	17-may-93 (jhahn)
**	    Changed TempTable functions to use DMR_LOAD instead of DMR_PUT when
**	    not sorting.
**
*/
DB_STATUS qen_doneLoadingTempTable(
	QEE_DSH		*dsh,
	i4		tempTableNumber
)
{
    QEE_TEMP_TABLE  *tempTable = dsh->dsh_tempTables[ tempTableNumber ];
    DMR_CB	    *dmr = tempTable->tt_dmrcb;
    DB_STATUS status = E_DB_OK;

    for (;;)	/* something to break out of */
    {
	if ( ( tempTable->tt_statusFlags &  TT_CREATED ) &&
	    !( tempTable->tt_statusFlags & TT_DONE_LOADING )
	   )
	{
	    dmr->dmr_flags_mask = DMR_ENDLOAD;
	    status = dmf_call(DMR_LOAD, dmr);
	    if (status != E_DB_OK)
	    {
		dsh->dsh_error.err_code = dmr->error.err_code;
		break;
	    }
	}

	tempTable->tt_statusFlags |= TT_DONE_LOADING;

	break;
    }	/* end of code block */

    return( status );
}

/*{
** Name: QEN_INITTIDTEMPTABLE - initialize temporary table
**
** Description:
**
**	This routine initializes the temporary table that buffers up the
**	inner tids which inner joined.
**
** Inputs:
**	tidTempTable	temp table descriptor from the QP
**	node		parent node
**	rcb		parent's QEF request control block
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	12-aug-92 (rickh)
**	    Written.
*/

DB_STATUS
qen_initTidTempTable(
QEN_TEMP_TABLE		*tidTempTable,
QEN_NODE		*node,
QEE_DSH			*dsh)
{
    QEN_STATUS	    *qen_status = dsh->dsh_xaddrs[node->qen_num]->qex_status;
    QEE_TEMP_TABLE  *dshTempTable =
	dsh->dsh_tempTables[ tidTempTable->ttb_tempTableIndex ];
    DB_STATUS status = E_DB_OK;

    status = qen_initTempTable( dsh, tidTempTable->ttb_tempTableIndex );
    if (status != E_DB_OK)	return (status);

    /*
    ** request sorting and duplicates removal.  the sortkey was filled
    ** in at QEE time.
    */

    dshTempTable->tt_statusFlags |= ( TT_PLEASE_SORT | TT_NO_DUPLICATES );

    qen_status->node_access &= ~QEN_TID_FILE_EOF ;
    return( status );

}

/*{
** Name: QEN_DUMPTIDTEMPTABLE - dump temporary table for reset purposes
**
** Description:
**
**	This routine destroys the temporary table that buffers up the
**	inner tids which inner joined.
**
** Inputs:
**	tidTempTable	temp table descriptor from the QP
**	node		parent node
**	rcb		parent's QEF request control block
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	12-aug-92 (rickh)
**	    Written.
*/

DB_STATUS
qen_dumpTidTempTable(
QEN_TEMP_TABLE		*tidTempTable,
QEN_NODE		*node,
QEE_DSH			*dsh)
{
    QEN_STATUS	    *qen_status = dsh->dsh_xaddrs[node->qen_num]->qex_status;
    DB_STATUS status = E_DB_OK;

    status = qen_destroyTempTable( dsh, tidTempTable->ttb_tempTableIndex );
    if (status != E_DB_OK)	return (status);

    qen_status->node_access &= ~QEN_TID_FILE_EOF ;
    return( status );

}

/*{
** Name: QEN_REWINDTIDTEMPTABLE - backup temporary table to beginning
**
** Description:
**
**	This routine backs up the temporary table that buffers up the
**	inner tids which inner joined.  As a byproduct, this routine
**	reads the first tid from the temporary table.
**
** Inputs:
**	tidTempTable	temp table descriptor from the QP
**	node		parent node
**	rcb		parent's QEF request control block
**
** Outputs:
**
** Side Effects:
**
**
** History:
**	12-aug-92 (rickh)
**	    Written.
*/

DB_STATUS
qen_rewindTidTempTable(
QEN_TEMP_TABLE		*tidTempTable,
QEN_NODE		*node,
QEE_DSH			*dsh)
{
    QEN_STATUS	    *qen_status = dsh->dsh_xaddrs[node->qen_num]->qex_status;
    DB_STATUS status = E_DB_OK;

    qen_status->node_access &= ~QEN_TID_FILE_EOF;

    status = qen_rewindTempTable( dsh, tidTempTable->ttb_tempTableIndex );
    if (status != E_DB_OK)	return (status);

    /* read first tuple from temporary table */

    status = qen_getFromTempTable( dsh, tidTempTable->ttb_tempTableIndex );
    if ( status == E_DB_ERROR )
    {
	if ( dsh->dsh_error.err_code != E_QE0015_NO_MORE_ROWS ) return( status );

	status = E_DB_OK;
	qen_status->node_access |= QEN_TID_FILE_EOF;
    }

    return( status );
}

/*{
** Name: QEN_READTIDTEMPTABLE - read a tid from the temporary table
**
** Description:
**
**	This routine compares the latest tid from the rescanned inner stream
**	with the current tid from the tid temp table.  If they are the same,
**	a "match" condition is returned.  If the inner stream tid is less
**	than the current tid, a "not found" condition is returned.  If
**	the inner stream tid is greater than the current temp table tid,
**	then the temp table is read again and the comparison is repeated.
**
**	What makes this routine work is the assumption that tids from both
**	the inner hold file and the temporary table are returned in ascending
**	order without duplicates.  This is so because the hold file is a
**	DMF heap and because we have sorted and removed duplicates from
**	the temporary table.
**
** Inputs:
**	tidTempTable	temp table descriptor from the QP
**	node		parent node
**	rcb		parent's QEF request control block
**	tid		tid from inner stream hold file
**
** Outputs:
**
** Returns:
**
**	E_DB_OK		if a match occurred
**	E_DB_WARN	otherwise
** 	E_DB_ERROR	end of tid file or serious error
**
** Side Effects:
**
**
** History:
**	12-aug-92 (rickh)
**	    Written.
**	08-feb-94 (rickh)
**	    For sorting TID temp files, split TIDs into two i4s.
*/


#define	WORD_MASK	0xFFFF
#define	BITS_IN_A_WORD	16

#define	PACK_TID( source, dest )	\
 dest = ( u_i4 ) ( ( source[ 0 ] << BITS_IN_A_WORD ) | source[ 1 ] )

#define	UNPACK_TID( source, dest )	\
    dest[ 0 ] = ( source >> BITS_IN_A_WORD ) & WORD_MASK;	\
    dest[ 1 ] = source & WORD_MASK;

DB_STATUS
qen_readTidTempTable(
QEN_TEMP_TABLE		*tidTempTable,
QEN_NODE		*node,
QEE_DSH			*dsh,
u_i4	   		tid )
{
    QEN_STATUS	*qen_status = dsh->dsh_xaddrs[node->qen_num]->qex_status;
    i4		*TIDhalves = ( i4 * ) dsh->dsh_row[ tidTempTable->ttb_tuple ];
    u_i4	innerJoinedTID;
    DB_STATUS	status = E_DB_OK;


    PACK_TID( TIDhalves, innerJoinedTID );

    if( tid > innerJoinedTID )   /* read another tid */
    {
        status = qen_getFromTempTable( dsh, tidTempTable->ttb_tempTableIndex );
        if ( status != E_DB_OK )
        {
	    if ( ( status == E_DB_ERROR ) &&
		 ( dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS ) )
	        qen_status->node_access |= QEN_TID_FILE_EOF;

	    return( E_DB_ERROR );
        }

	PACK_TID( TIDhalves, innerJoinedTID );

	/* how about a little sanity check, scarecrow?	*/

	if( tid > innerJoinedTID )
	{
	    dsh->dsh_error.err_code = E_QE0002_INTERNAL_ERROR;
	    return( E_DB_ERROR );
	}
    }

    if( tid == innerJoinedTID )   /* match occurred */
	return(E_DB_OK);
    else			/* no match */
	return(E_DB_WARN);	/* return no match */
}

/*{
** Name: QEN_APPENDTIDTEMPTABLE - add a tid to the temporary table
**
** Description:
**
**	This routine stores a tid in the temporary table that buffers up the
**	inner tids which inner joined.
**
** Inputs:
**	tidTempTable	temp table descriptor from the QP
**	node		parent node
**	rcb		parent's QEF request control block
**	tid		tid from inner stream hold file
**
** Outputs:
**
**
** Side Effects:
**
**
** History:
**	12-aug-92 (rickh)
**	    Written.
**	08-feb-94 (rickh)
**	    For sorting TID temp files, split TIDs into two i4s.
**	06-mar-96 (nanpr01)
**	    Changed the parameter of qen_putIntoTempTable. Now instead of
**	    passing just the index number entire QEN_TEMP_TABLE structure
**	    is passed. 
*/

DB_STATUS
qen_appendTidTempTable(
QEN_TEMP_TABLE		*tidTempTable,
QEN_NODE		*node,
QEE_DSH			*dsh,
u_i4	   		tid )
{
    i4		*TIDhalves = ( i4 * ) dsh->dsh_row[ tidTempTable->ttb_tuple ];

    /* put tid into the dsh row where dmf knows to look for it */

    UNPACK_TID( tid, TIDhalves );

    /* append to tid temporary table */

    return( qen_putIntoTempTable( dsh, tidTempTable ) );
}

/* Name: qen_openAndLink - Open a table and link to the DSH
**
** Description:
**
**	This routine opens a table, given a DMT CB, and links the
**	DMT CB into the DSH list of open tables.
**
**	There's very little to this, but it happens with some regularity,
**	and I figured it was time to make a routine out of it.
**
**	If the open fails, the DMF error info is stored into the DSH.
**
** Inputs:
**	dmtcb			Address of DMF table control block
**	dsh			Data segment header
**
** Outputs:
**	None
**	Returns E_DB_OK or error
**
** History:
**
**	20-Oct-2006 (kschendel) b122118
**	    Unify the half-dozen places that do this, so that it's right.
**	05-Aug-2010 (miket) SIR 122403
**	    Bail out if we find a locked encrypted table.
**	05-Aug-2010 (miket) SIR 122403
**	    Fix rookie error: dmt_enc_locked undefined unless E_DB_OK.
**
*/

DB_STATUS
qen_openAndLink(DMT_CB *dmtcb, QEE_DSH *dsh)
{

    DB_STATUS status;

    status = dmf_call(DMT_OPEN, dmtcb);
    if (status == E_DB_OK && dmtcb->dmt_enc_locked)
    {
	status = E_DB_ERROR;
	dmtcb->error.err_code = E_QE0190_ENCRYPT_LOCKED;
    }
    if (status != E_DB_OK)
    {
	dsh->dsh_error.err_code = dmtcb->error.err_code;
	return (status);
    }
	
    /* Insert at the front of the doubly linked list (not ring). */

    dmtcb->q_prev = (PTR)NULL;
    dmtcb->q_next = (PTR) dsh->dsh_open_dmtcbs;
    if (dsh->dsh_open_dmtcbs != (DMT_CB *)NULL)
	dsh->dsh_open_dmtcbs->q_prev = (PTR)dmtcb;
    dsh->dsh_open_dmtcbs = dmtcb;	    

    return (E_DB_OK);

} /* qen_openAndLink */

/* Name: qen_closeAndUnlink - Close a table and unlink from the DSH
**
** Description:
**
**	This routine closes a table, given its DMT CB, and unlinks the
**	DMT CB from the DSH list of open tables.
**
**	There's very little to this, but it happens with some regularity,
**	and I figured it was time to make a routine out of it.
**
**	Note that the unlink occurs even if the DMF close errors out.
**
** Inputs:
**	dmtcb			Address of DMF table control block
**	dsh			Data segment header
**
** Outputs:
**	None
**	Returns E_DB_OK or error, if error look in DMT CB for error code
**
** History:
**
**	20-Oct-2006 (kschendel) b122118
**	    Unify the half-dozen places that do this.
**
*/

DB_STATUS
qen_closeAndUnlink(DMT_CB *dmtcb, QEE_DSH *dsh)
{

    DB_STATUS status;

    status = dmf_call(DMT_CLOSE, dmtcb);
    dmtcb->dmt_record_access_id = NULL;		/* Show that it's closed */
    /* Fix up open ended doubly linked list (not ring). */
    if (dmtcb->q_prev != NULL)
	((DMT_CB *)dmtcb->q_prev)->q_next = dmtcb->q_next;
    else
	dsh->dsh_open_dmtcbs = (DMT_CB *) dmtcb->q_next;
    if (dmtcb->q_next != NULL)
	((DMT_CB *)dmtcb->q_next)->q_prev = dmtcb->q_prev;
    dmtcb->q_next = dmtcb->q_prev = NULL;	/* Paranoia cleaning */

    return (status);

} /* qen_closeAndUnlink */

/*
** Name: qen_set_qualfunc -- Set DMR-CB row qualification function
**
** Description:
**
**	ORIG nodes and K-Join nodes can apply row qualification (for
**	the inner in the case of K-join) directly in DMF in some
**	cases.  (The main restriction being that the qualification CX
**	must not reference the TID, since the TID is a construct and
**	not an actual column of the row.)
**
**	This routine is called from various places to determine whether
**	a DMF-level qualification is needed, based on the qep node, and
**	sets proper stuff in the passed DMR-CB as appropriate.
**
**	Important:  this has to be called before the first DMR_POSITION
**	call for the table or partition, because that's when DMF actually
**	sets the qualification poop into its internal data structures
**	(the RCB).  It won't work to alter any of the qualification arg
**	addresses from one DMR_GET to the next, they all have to be
**	constant from position time on.
**
** Inputs:
**	dmr		Pointer to DMR_CB to set qualification for.
**	node		QP node with potential qualification, should
**			be orig or kjoin.
**	dsh		Data segment header for thread
**
** Outputs:
**	dmr
**	    .dmr_q_XXX	Returns with qualification-function stuff set (or
**			nulled out if no DMF time qualification)
**
** History:
**	11-Apr-2008 (kschendel)
**	    Extract common code now that DMRCB setup is slightly more
**	    complicated.
*/

void
qen_set_qualfunc(DMR_CB *dmr,
    QEN_NODE *node, QEE_DSH *dsh)
{
    ADE_EXCB *excb;		/* ADF CX execution control block */
    QEN_ADF *qenadf;		/* CX descriptor in the QP */

    qenadf = NULL;
    if ((node->qen_type == QE_ORIG || node->qen_type == QE_ORIGAGG)
      && node->node_qen.qen_orig.orig_qual != NULL
      && (node->node_qen.qen_orig.orig_flag & ORIG_TKEY) == 0)
	qenadf = node->node_qen.qen_orig.orig_qual;
    else if (node->qen_type == QE_KJOIN
      && node->node_qen.qen_kjoin.kjoin_iqual != NULL
      && ! node->node_qen.qen_kjoin.kjoin_tqual)
	qenadf = node->node_qen.qen_kjoin.kjoin_iqual;
    if (qenadf == NULL)
    {
	dmr->dmr_q_rowaddr = NULL;
	dmr->dmr_q_fcn = NULL;
	dmr->dmr_q_arg = NULL;
	dmr->dmr_q_retval = NULL;
	dmr->dmr_qef_adf_cb = NULL;
    }
    else
    {
	excb = (ADE_EXCB *) dsh->dsh_cbs[qenadf->qen_pos];
	excb->excb_seg = ADE_SMAIN;
	if (dsh->dsh_qp_ptr->qp_status & QEQP_GLOBAL_BASEARRAY)
	    dmr->dmr_q_rowaddr = &dsh->dsh_row[qenadf->qen_input];
	else
	    dmr->dmr_q_rowaddr = &excb->excb_bases[qenadf->qen_input + ADE_ZBASE];
	dmr->dmr_q_fcn = (DB_STATUS (*)(void *,void *)) ade_execute_cx;
	dmr->dmr_q_arg = (PTR) excb;
	dmr->dmr_q_retval = &excb->excb_value;

	/* Which ADF_CB to pass???
	** Turns out we want the one that gets checked for ADF warnings after
	** the query executes.  That would seem to be the QEF-session
	** ADF CB, which is a pointer to "the" session ADF CB.  (blast
	** all these darn ADF CB's running around!)
	** DMF will collect arith warnings in its own ADF_CB and roll them
	** up into this one after the get is successfully completed.
	*/
	dmr->dmr_qef_adf_cb = (PTR) dsh->dsh_qefcb->qef_adf_cb;
    }
} /* qen_set_qualfunc */
