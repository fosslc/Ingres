/*
**Copyright (c) 2004 Ingres Corporation
**
*/

/*
NO_OPTIM = r64_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <bt.h>
#include    <cs.h>
#include    <di.h>
#include    <er.h>
#include    <me.h>
#include    <pc.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <tr.h>
#include    <dbms.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <lg.h>
#include    <lgdef.h>
#include    <lgdstat.h>
#include    <lgkdef.h>

/**
**
**  Name: LGREAD.C - Implements the LGread function of the logging system
**
**  Description:
**	This module contains the code which implements LGread.
**	
**	External Routines:
**
**	    LGread -- Read a record from the logfile
**	    LGchk_sum -- Compute log page checksum.
**
**	Internal Routines:
**
**	    LG_read
**	    map_page
**
**  History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	12-oct-1992 (bryanp)
**	    Fix a typo in a TRdisplay.
**	29-oct-1992 (bryanp)
**	    Error message handling improvements.
**	18-jan-1993 (rogerk)
**	    Took out error message logged when a caller asks to read
**	    past the Log File EOF.
**	    Added CL_CLEAR_ERR call.
**	17-mar-1993 (rogerk)
**	    Moved lgd status values to lgdstat.h so that they are visible
**	    by callers of LGshow(LG_S_LGSTS).
**	26-apr-1993 (andys)
**	    Cluster 6.5 Project I
**	    Renamed stucture members of LG_LA to new names. This means
**	    replacing lga_high with la_sequence, and lga_low with la_offset.
**	21-jun-1993 (rogerk)
**	    Use casts to quiet ansi compiler.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <tr.h>
**	20-sep-1993 (bryanp)
**	    Changed the checksum routine to be non-destructive. Since, in a dual
**		logging environment, both the primary and the dual I/Os may be
**		in progress, we must ensure that the checksum routine doesn't
**		actually touch the page while it's checksumming it (if it did,
**		then we could have the situation where one thread is computing
**		the checksum while another thread is writing the page, thus
**		causing the page that gets written to be written in a funny
**		state).
**	07-oct-93 (johnst)
**	    Bug #58881
**	    Removed (i4) cast for ule_format() PTR parameter. This caused
**	    64-bit pointer truncation problem for DEC alpha.
**	31-jan-1994 (jnash)
**	  - Fail checksum of a log header containing all zeroes.
**	  - Fix lint detected sys_err initialization in LG_read().
**	05-Jan-1996 (jenjo02)
**	    Mutex granularity project. Semaphores must now be explicitly
**	    named when calling LKMUTEX functions. Lone lgd_mutex now
**	    accompanied by several others.
**	18-Jan-1996 (jenjo02)
**          lbk_table replaced with lfbb_table (LFBs) and ldbb_table
**          (LDBs) to ensure that semaphores now embedded in those
**          structures are not corrupted. Similarly, sbk_table 
**          replaced with lpbb_table (LPBs), lxbb_table (LXBs), and
**          lpdb_table (LPDs).
**	06-mar-1996 (stial01 for bryanp)
**	    Use 'copy_long' rather than 'MEcopy' to handle long log records.
**	28-Oct-1996 (jenjo02)
**	    Dirty read BOF, last forced LGA values from log header instead
**	    of taking a SHARE semaphore on lfb_cp_mutex and lfb_wq_mutex.
**      08-Dec-1997 (hanch04)
**          Use new la_block field when block number is needed instead
**          of calculating it from the offset
**          for support of logs > 2 gig
**	26-Jan-1998 (jenjo02)
**	    Partitioned Log File Project:
**	    Replace DIread/write with LG_READ/WRITE macros, DI_IO structures
**	    with LG_IO structures.
**	11-Dec-1998 (jenjo02)
**	    When checking for buffers yet to be written vs EOF, check while
**	    holding lfb_wq_mutex.
**	25-Feb-1999 (jenjo02)
**	    Removed test for buffers still on the write queue. If LGforce()
**	    is working properly, all needed buffers should be on disk!
**	13-Apr-1999 (jenjo02)
**	    EOF test in LG_read() must only include the LGA's sequence and
**	    block, not the offset, hence the LGA_ macros cannot be used.
**	08-Sep-2000 (jenjo02)
**	    Modified LGchk_sum to compute checksum value from first
**	    and last 16 "used" bytes of block.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      12-Feb-2003 (hweho01)
**          Turned off optimizer for AIX hybrid build by using 
**          the 64-bit configuration string (r64_us5), because the    
**          error happens only with the 64-bit compiler option.  
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**          Removed unnecessary copy_long routine
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**	6-Apr-2007 (kschendel) SIR 122890
**	    Use an intermediate for aliasing external i4 id's and LG_ID.
**	    Originally this was to fix gcc -O2's strict aliasing, but
**	    Ingres has so many other strict-aliasing violations that
**	    -fno-strict-aliasing is needed.  The id/LG_ID thing was so
**	    egregious, though, that I've kept this change.
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
**	14-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add ability to "read" from log buffers, bulk read
**	    I/O for any reader that supplies a large enough context block pool
**	    (see also LGposition, dm0l_allocate).
**	08-Apr-2010 (jonj)
**	    SIR 121619 MVCC: Change lgc_bufid from i4 to *i4. "lgc_bufid != NULL"
**	    means it was requested; *lgc_bufid = 0 means couldn't do it.
*/
static STATUS	LG_read(LG_ID *lx_id, LGC *lgc, i4  *channel,
			i4 *retry, i4  *disable,
			LG_IO **di_io, CL_ERR_DESC *sys_err);
static STATUS	map_page(LG_LXID lx_id, LGC *lgc, CL_ERR_DESC *sys_err);

/*{
** Name: LGread	- Read record from  log file.
**
** Description:
**	Reads the next record or page from the log file.  The context that
**	was initialized in a previous call to LGposition() contains the
**	information used to read the next record.  The log address of the
**	record is returned, along with a pointer to the location of
**	the stored record.  If reading pages, then the record pointer 
**	actually points to a page.  The LRH is part of the record 
**      returned as well as the record length.  
**
**      Note, the context is allocated by the client at position time
**      and it is up to the client to free it.  You cannot depend on it
**      being available all the time.
**
** Inputs:
**      lx_id                           Transaction identifier.
**	context				Pointer to context.
**	lga				When reading from log buffers,
**					the specific LGA to read.
**	bufid				For MVCC undo, the log buffer (LBB)
**					lbb_id into which LGwrite placed
**					the record associated with lga.
**
** Outputs:
**	lga				The LGA of the log record.
**	record				Address of buffer containing record.
**	bufid				If present, count of physical I/O
**					done for this request.
**	sys_err				Reason for error return status.
**	Returns:
**	    OK
**	    LG_ENDOFILE		    Read past end of file.
**	    LG_BADPARAM		    Parameter(s) in error.
**	    LG_READERROR	    Error reading log file.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	2-mar-1993 (bryanp)
**	    Cluster support: no more LPS field.
**	07-oct-93 (johnst)
**	    Bug #58881
**	    Removed (i4) cast for ule_format() PTR parameter. This caused
**	    64-bit pointer truncation problem for DEC alpha.
**	10-oct-93 (swm)
**	    Bug #56439
**	    Now that lgc_check has changed from i4 to PTR, check lgc_check
**	    against context with a direct pointer comparison rather with a
**	    (i4) checksum.
**	10-Nov-2009 (kschendel) SIR 122757
**	    Remove unused l_context arg.
**	14-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add ability to "read" from log buffers and bulk read
**	    I/O for any reader that supplies a large enough context block pool
**	    (see also LGposition, dm0l_allocate).
*/
/* ARGSUSED */
STATUS
LGread(
LG_LXID		    lx_id,
PTR		    context,
LG_RECORD	    **record,
LG_LA		    *lga,
i4		    *bufid,
CL_ERR_DESC	    *sys_err)
{
    register LGC    *lgc = (LGC *)context;
    char	    *rec;
    i4	    length;
    i4	    move;
    i4	    r_length;
    i4	    status;
    i4	    err_code;

    CL_CLEAR_ERR(sys_err);

    if ((lgc->lgc_status & LGC_VALID) == 0 || (lgc->lgc_check != (PTR)lgc))
    {
	uleFormat(NULL, E_DMA462_LGREAD_BAD_CXT, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
		    0, lgc->lgc_status, 0, lgc->lgc_check, 0, lgc);
	return(LG_BADPARAM);
    }

    /* Check if read the first log page. */

    if (lgc->lgc_position == LG_P_PAGE &&
	lgc->lgc_lga.la_block== 0 &&
	lgc->lgc_current_lga.la_block == 0)
    {
	lgc->lgc_current_lga.la_block  = -1;
	lgc->lgc_current_lga.la_offset = -1;
    }

    /*
    ** Read from log buffers means something only when postioned by LGA and
    ** reading backwards (MVCC undo), and only if the bufid looks ok.
    **
    ** It means look for the LGA in LBB[bufid] first, then
    ** the page pool, then resort to a read I/O.
    */
    if ( (lgc->lgc_bufid = bufid) )
    {
	if ( *lgc->lgc_bufid > 0 && *lgc->lgc_bufid <= lgc->lgc_bufcnt &&
             lgc->lgc_position == LG_P_LGA && lgc->lgc_direction == LG_D_PREVIOUS )
	{
	    /* "lga" is the record we want to read */
	    lgc->lgc_lga = *lga;
	    lgc->lgc_offset = lgc->lgc_lga.la_offset;
	}
	else
	    *lgc->lgc_bufid = 0;
    }

    /* No physical I/O, yet */
    lgc->lgc_readio = 0;
    
    /*	Insure that the current page is mapped. */

    status = map_page(lx_id, lgc, sys_err);
    if (status)
    {
	if (status != LG_ENDOFILE)
	{
	    uleFormat(NULL, status,sys_err,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	    status = E_DMA463_LGREAD_MAP_PAGE;
	}
	return (status);
    }

    /*	Check for page mode access. */

    if (lgc->lgc_position == LG_P_PAGE)
    {
	lgc->lgc_lga.la_block++;
	lgc->lgc_lga.la_offset = 0;
	*record = (LG_RECORD *) lgc->lgc_buffer;
	return(OK);
    }

    /*	Determine if we will stay on the same page. */

    if (lgc->lgc_direction == LG_D_FORWARD)
    {
	/*  Increment to the begining of the next record. */

	lgc->lgc_offset += sizeof(i4);

	/*  Check maximum offset of the current page. */

	while (lgc->lgc_offset >= lgc->lgc_high)
    	{
	    /*	Increment to next page, and just past the header. */

	    lgc->lgc_offset = lgc->lgc_low;
	    lgc->lgc_lga.la_block++;
	    lgc->lgc_lga.la_offset = 0;

	    /*	Wrap around to the begining of the physical file. */

	    if (lgc->lgc_lga.la_block > lgc->lgc_max)
	    {
		lgc->lgc_lga.la_sequence++;
		lgc->lgc_lga.la_block  = 1;
		lgc->lgc_lga.la_offset = 0;
	    }

	    /*	Map the new page. */

	    status = map_page(lx_id, lgc, sys_err);
	    if (status)
	    {
		if (status != LG_ENDOFILE)
		{
		    uleFormat(NULL, status,sys_err,
				ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		    status = E_DMA463_LGREAD_MAP_PAGE;
		}
		return (status);
	    }

	    /*	Loop in case the page is empty. */
	}
	    
	/*  Get the length of the record. */

	length = *(i4 *)&((char *)lgc->lgc_buffer)[lgc->lgc_offset];

	/*  Check that the length of the record is legal. */

	if (length < sizeof(LG_RECORD) + 2 * sizeof(i4) ||
	    length > LG_MAX_RSZ ||
	    (length & (sizeof(i4) - 1)))
	{
	    uleFormat(NULL, E_DMA464_LGREAD_BADFORMAT, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 7,
		    0, lgc->lgc_offset,
		    0, lgc->lgc_lga.la_sequence,
		    0, lgc->lgc_lga.la_block,
		    0, lgc->lgc_lga.la_offset,
		    0, length,
		    0, (sizeof(LG_RECORD) + 2 * sizeof(i4)),
		    0, LG_MAX_RSZ);
	    return(LG_BADFORMAT);
	}

	/*  Check to see if the record is contained in the page. */

	if (lgc->lgc_offset + length <= lgc->lgc_size)
	{
	    /*	Return pointer to record and update for next call. */

	    *record = 
		(LG_RECORD *) &((char *)lgc->lgc_buffer)[lgc->lgc_offset];
	    lgc->lgc_offset += length - sizeof(i4);

	    lga->la_sequence = lgc->lgc_lga.la_sequence;
	    lga->la_block = lgc->lgc_lga.la_block + 
				(lgc->lgc_offset / lgc->lgc_size);
	    lga->la_offset = lgc->lgc_lga.la_offset + 
				(lgc->lgc_offset % lgc->lgc_size);

	    if (*(i4 *)&((char *)lgc->lgc_buffer)[lgc->lgc_offset]
		!= length)
	    {
		uleFormat(NULL, E_DMA465_LGREAD_BADFORMAT, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 6,
			0, lgc->lgc_offset,
			0, lgc->lgc_lga.la_sequence,
			0, lgc->lgc_lga.la_block,
			0, lgc->lgc_lga.la_offset,
			0, length,
			0, (*(i4 *)&((char *)lgc->lgc_buffer)
						    [lgc->lgc_offset]));
		return(LG_BADFORMAT);
	    }

	    return(OK);
	}
    }
    else
    {
	/* direction == LG_D_PREVIOUS */

	/*  Check minimum offset of current page. */

	while (lgc->lgc_offset < lgc->lgc_low)
	{
	    /*	Decrement to previous page. */

	    lgc->lgc_lga.la_block--;
	    lgc->lgc_lga.la_offset = 0;
	    if (lgc->lgc_lga.la_block < lgc->lgc_min)
	    {
		lgc->lgc_lga.la_sequence--;
		lgc->lgc_lga.la_block = lgc->lgc_max;
		lgc->lgc_lga.la_offset = 0;

		/*
		** If reading from log buffers, decrement
		** to previous log buffer. If that's 0,
		** wrap to the last buffer.
		*/
		if ( lgc->lgc_bufid && --(*lgc->lgc_bufid) == 0 )
		    *lgc->lgc_bufid = lgc->lgc_bufcnt;
	    }
	    
	    status = map_page(lx_id, lgc, sys_err);
	    if (status)
	    {
		if (status != LG_ENDOFILE)
		{
		    uleFormat(NULL, status,sys_err,
				ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		    status = E_DMA463_LGREAD_MAP_PAGE;
		}
		return (status);
	    }

	    /*	Get offset of last entry on the page. */

	    lgc->lgc_offset = lgc->lgc_high - sizeof(i4);

	    /*	Loop to skip over empty pages. */
	}

	/*  Get the length of the record. */

	length = *(i4 *)&((char *)lgc->lgc_buffer)[lgc->lgc_offset];

	/*  Check that the length of the record is legal. */

	if (length < sizeof(LG_RECORD) + 2 * sizeof(i4) ||
	    length > LG_MAX_RSZ ||
	    (length & (sizeof(i4) - 1)))
	{
	    uleFormat(NULL, E_DMA464_LGREAD_BADFORMAT, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 7,
		    0, lgc->lgc_offset,
		    0, lgc->lgc_lga.la_sequence,
		    0, lgc->lgc_lga.la_block,
		    0, lgc->lgc_lga.la_offset,
		    0, length,
		    0, (sizeof(LG_RECORD) + 2 * sizeof(i4)),
		    0, LG_MAX_RSZ);
	    return(LG_BADFORMAT);
	}

	/*  Check to see if the record is contained in the page. */

	if ((i4)(lgc->lgc_offset - length + sizeof(i4)) >= 
						    (i4)(lgc->lgc_low))
	{
	    /*	Return pointer to record and update for next call. */

	    lga->la_sequence = lgc->lgc_lga.la_sequence;
	    lga->la_block = lgc->lgc_lga.la_block +
				(lgc->lgc_offset / lgc->lgc_size);
	    lga->la_offset = lgc->lgc_lga.la_offset +
				(lgc->lgc_offset % lgc->lgc_size);
	    lgc->lgc_offset -= length;
	    *record = 
		(LG_RECORD *) &((char *)lgc->lgc_buffer)[lgc->lgc_offset +
		sizeof(i4)];

	    /*	Check length field at beginning of record. */

	    if (*(i4 *)&((char *)lgc->lgc_buffer)[lgc->lgc_offset
		+ sizeof(i4)] != length)
	    {
		uleFormat(NULL, E_DMA465_LGREAD_BADFORMAT, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 6,
			0, lgc->lgc_offset,
			0, lgc->lgc_lga.la_sequence,
			0, lgc->lgc_lga.la_block,
			0, lgc->lgc_lga.la_offset,
			0, length,
			0, (*(i4 *)&((char *)lgc->lgc_buffer)
					[lgc->lgc_offset + sizeof(i4)]));
		return(LG_BADFORMAT);
	    }

	    /*	If reading linked records, store previous lga. */

	    if (lgc->lgc_direction == LG_D_PREVIOUS)
	    {
		lgc->lgc_lga = ((LG_RECORD *)*record)->lgr_prev;
		lgc->lgc_offset = lgc->lgc_lga.la_offset;
		lgc->lgc_lga.la_offset = 0;
	    }

	    /*
	    ** If bufid is there, overload it a bit with
	    ** a count of physical I/O done.
	    */
	    if ( bufid )
	        *bufid = lgc->lgc_readio;

	    return(OK);
	}
    }

    /*
    **	At this point the record spans one or more pages.  The record
    **  will be constructed into the LGC record buffer and a pointer
    **  to the record in this buffer will be returned.
    */

    /*	Record will be stored in LGC record area. */

    *record = (LG_RECORD *) lgc->lgc_record;
    rec = (char *) lgc->lgc_record;
    r_length = length;

    if (lgc->lgc_direction == LG_D_FORWARD)
    {
	/*  Keep looping until no more bytes needed. */

	while (length)
	{
	    /*	Compute amount to move. */

	    move = lgc->lgc_high - lgc->lgc_offset;
	    if (move > length)
		move = length;

	    /*	Move from buffer to record. */
	    MEcopy(&((char *)lgc->lgc_buffer)[lgc->lgc_offset], move, rec);

	    /*	Update offset. */

	    lgc->lgc_offset += move;
	    rec += move;

	    /*	Check for end of record. */

	    if ((length -= move) == 0)
	    {
		/*  Set the LGA and return. */

		lgc->lgc_offset -= sizeof(i4);
		lga->la_block = lgc->lgc_lga.la_block +
				(lgc->lgc_offset / lgc->lgc_size);
		lga->la_offset = lgc->lgc_lga.la_offset +
				(lgc->lgc_offset % lgc->lgc_size );
		lga->la_sequence = lgc->lgc_lga.la_sequence;
		if (*(i4 *)&((char *)lgc->lgc_buffer)[lgc->lgc_offset]
		    != r_length)
		{
		    uleFormat(NULL, E_DMA465_LGREAD_BADFORMAT, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 6,
			    0, lgc->lgc_offset,
			    0, lgc->lgc_lga.la_sequence,
			    0, lgc->lgc_lga.la_block,
			    0, lgc->lgc_lga.la_offset,
			    0, r_length,
			    0, (*(i4 *)&((char *)lgc->lgc_buffer)
							[lgc->lgc_offset]));
		    return(LG_BADFORMAT);
		}
		return(OK);
	    }

	    /*	Increment to next page. */

	    lgc->lgc_lga.la_block++;
	    lgc->lgc_lga.la_offset = 0;
	    if (lgc->lgc_lga.la_block > lgc->lgc_max)
	    {
		lgc->lgc_lga.la_sequence++;
		lgc->lgc_lga.la_block = 1;
		lgc->lgc_lga.la_offset = 0;
	    }

	    /*	Map the page. */

	    status = map_page(lx_id, lgc, sys_err);
	    if (status)
	    {
		if (status != LG_ENDOFILE)
		{
		    uleFormat(NULL, status,sys_err,
				ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		    status = E_DMA463_LGREAD_MAP_PAGE;
		}
		return (status);
	    }

	    /*	Set starting offset for new page. */

	    lgc->lgc_offset = lgc->lgc_low;
	}
    }
    else
    {
	/*  Set the LGA. */

	lga->la_sequence = lgc->lgc_lga.la_sequence;
	lga->la_block = lgc->lgc_lga.la_block +
			(lgc->lgc_offset / lgc->lgc_size);
	lga->la_offset = lgc->lgc_lga.la_offset + 
			(lgc->lgc_offset % lgc->lgc_size);

	/*  Increment to include the length in return record. */

	lgc->lgc_offset += sizeof(i4);

	/*  Keep looping while more to move. */

	while (length)
	{
	    /*	Compute amount to move. */

	    move = lgc->lgc_offset - lgc->lgc_low;
	    if (move > length)
		move = length;

	    /*	Move from buffer to record, */

	    MEcopy(&((char *)lgc->lgc_buffer)[lgc->lgc_offset - move],
					move, &rec[length - move]);

	    /*	Update the offset. */

	    lgc->lgc_offset -= move;

	    /*	Check for end of record. */

	    if ((length -= move) == 0)
	    {
		if (*(i4 *)&((char *)lgc->lgc_buffer)[lgc->lgc_offset]
		    != r_length)
		{
		    uleFormat(NULL, E_DMA465_LGREAD_BADFORMAT, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 6,
			    0, lgc->lgc_offset,
			    0, lgc->lgc_lga.la_sequence,
			    0, lgc->lgc_lga.la_block,
			    0, lgc->lgc_lga.la_offset,
			    0, r_length,
			    0, (*(i4 *)&((char *)lgc->lgc_buffer)
							[lgc->lgc_offset]));
		    return(LG_BADFORMAT);
		}

		/*  Move to next record. */

		lgc->lgc_offset -= sizeof(i4);

		/*  Save previous if reading by previous pointer. */

		if (lgc->lgc_direction == LG_D_PREVIOUS)
		{
		    lgc->lgc_lga = ((LG_RECORD *)lgc->lgc_record)->lgr_prev;
		    lgc->lgc_offset = lgc->lgc_lga.la_offset;
		    lgc->lgc_lga.la_offset = 0;
		}

		/*
		** If bufid is there, overload it a bit with
		** a count of physical I/O done.
		*/
		if ( bufid )
		    *bufid = lgc->lgc_readio;

		return(OK);
	    }

	    /*	Decrement to previous page. */

	    lgc->lgc_lga.la_block--;
	    lgc->lgc_lga.la_offset = 0;
	    if (lgc->lgc_lga.la_block < lgc->lgc_min)
	    {
		lgc->lgc_lga.la_sequence--;
		lgc->lgc_lga.la_block = lgc->lgc_max;
		lgc->lgc_lga.la_offset = 0;

		/*
		** If reading from log buffers, decrement
		** to previous log buffer. If that's 0,
		** wrap to the last buffer.
		*/
		if ( lgc->lgc_bufid && --(*lgc->lgc_bufid) == 0 )
		    *lgc->lgc_bufid = lgc->lgc_bufcnt;
	    }

	    /*	Map the new page. */

	    status = map_page(lx_id, lgc, sys_err);
	    if (status)
	    {
		if (status != LG_ENDOFILE)
		{
		    uleFormat(NULL, status,sys_err,
				ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		    status = E_DMA463_LGREAD_MAP_PAGE;
		}
		return (status);
	    }

	    /*	Set starting offset for new page. */

	    lgc->lgc_offset = lgc->lgc_high;
	}
    }

    /* NOTREACHED */
}

/*{
** Name: LG_read	- Read a log record.
**
** Description:
**(     This routine reads the next log record.  Next can be:
**	    Next log record in file forward.
**	    Next log record in file backwards.
**	    Next log record in file backwards by lx_id.
**	    Next can be a record by record address.
**)
**	If this process is using slave processes to perform log read I/O,
**	then the returned channel is actually a slave file operation code,
**	which the slave interprets as a request to read from the appropriate
**	log file.
**
** Inputs:
**      lx_id                           Log indentifier.
**	lgc				The log context, with
**	    lgc_buffer			    the last known block
**	    lgc_lga			    the log block to read
**	    *lgc_bufid			    if not zero, look first in that
**					    log buffer
**	    lgc_curblock		The current block in the block pool
**					0-n
**	    lgc_readblocks		The number of viable blocks in the pool
**	retry				0 -- this is the first read attempt
**					1 -- retry the read from the other log
**	disable				checked only if retry is non-zero:
**					0 -- don't disable the log
**					1 -- disable the log
**
** Outputs:
**	lgc
**	    lgc_buffer			The block if found either in the
**					log buffer or block pool,
**					otherwise NULL; a physical I/O
**					is needed.
**	    lgc_readblocks		The number of blocks to DIread
**					if I/O is needed.
**      channel				Address of location to return channel.
**					If slaves are in use, one of:
**					LG_SL_READ_PRIM - read from primary
**					LG_SL_READ_DUAL - read from dual
**	di_io				LG_IO to use for the read.
**	retry				Status of dual logging:
**					0 -- no (more) retry is possible
**					1 -- this read can be retried.
**
**	Returns:
**	    OK
**	    LG_BADPARAM
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	12-oct-1992 (bryanp)
**	    Fix a typo in a TRdisplay.
**	18-jan-1993 (rogerk)
**	    Took out error message logged when a caller asks to read
**	    past the Log File EOF.  This is an expected occurrence and
**	    was fooling people into thinking there was an error. (It is
**	    possible that if there are cases where we mistakenly attempt
**	    to read past the EOF, that this error message would have been
**	    very useful).
**	31-jan-1994 (jnash)
**	    Fix lint detected sys_err initialization.
**	25-Jan-1996 (jenjo02)
**	    New mutexi, not just lgd_mutex.
**	28-Oct-1996 (jenjo02)
**	    Dirty read BOF, last forced LGA values from log header instead
**	    of taking a SHARE semaphore on lfb_cp_mutex and lfb_wq_mutex.
**	11-Dec-1998 (jenjo02)
**	    When checking for buffers yet to be written vs EOF, check while
**	    holding lfb_wq_mutex.
**	25-Feb-1999 (jenjo02)
**	    Removed test for buffers still on the write queue. If LGforce()
**	    is working properly, all needed buffers should be on disk!
**	13-Apr-1999 (jenjo02)
**	    EOF test in LG_read() must only include the LGA's sequence and
**	    block, not the offset, hence the LGA_ macros cannot be used.
**	14-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add ability to read from log buffer, expose
**	    entire LGC to LG_read(). Add ability to bulk-read log blocks.
**	31-Mar-2010 (jonj)
**	    SIR 121619 MVCC:
**	    If dm0p's reading below BOF, quietly return LG_ENDOFILE.
**	    Make sure lgc_buffer isn't null when checking the block pool.
*/
static STATUS
LG_read(
LG_ID               *lx_id,
LGC		    *lgc,
i4		    *channel,
i4		    *retry,
i4		    *disable,
LG_IO		    **di_io,
CL_ERR_DESC	    *sys_err)
{
    register LGD    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LXB    *lxb;
    register LPD    *lpd;
    register LPB    *lpb;
    register LFB    *lfb;
    LG_LA	    *lga = &lgc->lgc_lga;
    LBB		    *lbb;
    LBH		    *lbh;
    SIZE_TYPE	    *lxbb_table;
    i4	    err_code;
    STATUS	    status;
    i4		    eofblock;

    LG_WHERE("LG_read")

    CL_CLEAR_ERR(sys_err);

    /* Remember current buffer, if any */
    lbh = lgc->lgc_buffer;

    /* No new buffer found, yet */
    lgc->lgc_buffer = (LBH*)NULL;
    lgc->lgc_current_lbb = FALSE;

    /*	Check the lg_id. */

    if (lx_id->id_id == 0 || (i4)(lx_id->id_id) > lgd->lgd_lxbb_count)
    {
	uleFormat(NULL, E_DMA45B_LG_READ_BAD_ID, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
		    0, lx_id->id_id, 0, lgd->lgd_lxbb_count);
	return (LG_BADPARAM);
    }

    lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);
    lxb = (LXB *)LGK_PTR_FROM_OFFSET(lxbb_table[lx_id->id_id]);

    if (lxb->lxb_type != LXB_TYPE ||
	lxb->lxb_id.id_instance != lx_id->id_instance)
    {
	uleFormat(NULL, E_DMA45C_LG_READ_BAD_XACT, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
		    0, lxb->lxb_type, 0, lxb->lxb_id.id_instance,
		    0, lx_id->id_instance);
	return (LG_BADPARAM);
    }

    lfb = (LFB *)LGK_PTR_FROM_OFFSET(lxb->lxb_lfb_offset);

    /* Check if this is called for the retry of a read. */
    
    if (*retry)
    {
	/* 
	** Decide which channel to return for the retry and which channel
	** to be disabled for dual logging.
	*/
    
	if (*channel == LG_PRIMARY_DI_CB)
	{
	    *channel = LG_DUAL_DI_CB;
	    lgd->lgd_stat.dual_readio++;
	    if (*disable)
	    {
		uleFormat(NULL, E_DMA45D_LGREAD_DISABLE_PRIM, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		LG_disable_dual_logging(LGD_II_DUAL_LOG, lfb);
	    }
	}
	else
	{
	    *channel = LG_PRIMARY_DI_CB;
	    lgd->lgd_stat.log_readio++;
	    if (*disable)
	    {
		uleFormat(NULL, E_DMA45E_LGREAD_DISABLE_DUAL, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		LG_disable_dual_logging(LGD_II_LOG_FILE, lfb);
	    }
	}
	*retry = 0; /* no more retry is possible */
	if (lfb->lfb_status & LFB_USE_DIIO)
	    *di_io = &lfb->lfb_di_cbs[*channel];
	else
	    *di_io = &Lg_di_cbs[*channel];
	return (OK);
    }

    /*
    ** Check for reading a log record that is outside of the current range
    ** of the log file.
    */
    if (lga->la_sequence == 0)
    {
	/*
	** If log sequence number (the high portion of the log address) is
	** not given, then just make sure that the block requested is within the
	** range of the log file.
	*/
	if (lga->la_block > lfb->lfb_header.lgh_count)
	{
	    uleFormat(NULL, E_DMA45F_LGREAD_LA_RANGE, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
		    0, lga->la_sequence,
		    0, lga->la_block,
		    0, lga->la_offset,
		    0, lfb->lfb_header.lgh_count);
	    return (LG_ENDOFILE);
	}
    }
    else
    {
	/*
	** Check for out of range - log address requested must be :
	** greater than the log header BOF marker, less than the last
	** forced log record address, and must not specify a position
	** in the log header.
	**
	** If reading from the log buffers, we're permitted
	** to read records that haven't been forced yet, so skip
	** that check.
	**
	** Note that we're only interested in the sequence and block
	** i.e. log page, not the offset within the block.
	*/
	if ( (lga->la_sequence < lfb->lfb_header.lgh_begin.la_sequence ||
	     (lga->la_sequence == lfb->lfb_header.lgh_begin.la_sequence &&
		lga->la_block < lfb->lfb_header.lgh_begin.la_block))
	    ||
	      (!lgc->lgc_bufid &&
	      ((lga->la_sequence > lfb->lfb_forced_lga.la_sequence ||
	       (lga->la_sequence == lfb->lfb_forced_lga.la_sequence &&
		lga->la_block > lfb->lfb_forced_lga.la_block))))
	    ||
	       lga->la_block == 0 )
	{
	    /*
	    ** Lock the write queue mutex (which is used to update lfb_forced_lga)
	    ** and check again.
	    */
	    LG_mutex(SEM_EXCL, &lfb->lfb_wq_mutex);

	    /* 
	    ** Any failure now is either an "expected" EOF condition
	    ** or a failure by LGforce() to properly flush the log
	    ** buffers to disk.
	    */
	    if ( (lga->la_sequence < lfb->lfb_header.lgh_begin.la_sequence ||
		 (lga->la_sequence == lfb->lfb_header.lgh_begin.la_sequence &&
		    lga->la_block < lfb->lfb_header.lgh_begin.la_block))
		||
		  (lga->la_sequence > lfb->lfb_forced_lga.la_sequence ||
		  (lga->la_sequence == lfb->lfb_forced_lga.la_sequence &&
		    lga->la_block > lfb->lfb_forced_lga.la_block))
		||
		   lga->la_block == 0 )
	    {
		/*
		** Log an informational error message giving the read request
		** parameters if the EOF error does not look 'normal'.  All
		** read past EOF situations are not errors, we sometimes scan
		** log files until the EOF is reached.  If the read request does
		** not indicate one block after the log file EOF then write
		** the error message.
		*/

		/*
		** If looking in log buffers, just return LG_ENDOFILE.
		** dm0p has cached some now-stale value of the BOF and
		** expects to quietly see LG_ENDOFILE and deal with it
		** by switching to journal reads.
		*/
		if ( !lgc->lgc_bufid )
		{
		    /*
		    ** Check if request is one past EOF.  Handle case where request
		    ** wraps around the end of the log file.
		    */
		    if ( (lga->la_sequence != lfb->lfb_forced_lga.la_sequence ||
			    lga->la_block != lfb->lfb_forced_lga.la_block + 1)
		      && (lga->la_sequence != lfb->lfb_forced_lga.la_sequence + 1 ||
			    lga->la_block != 1) )
		    {
			uleFormat(NULL, E_DMA460_LGREAD_EOF_REACHED, (CL_ERR_DESC *)NULL,
					ULE_LOG, NULL, NULL, 0, NULL, &err_code, 9,
				0, lga->la_sequence,
				0, lga->la_block,
				0, lga->la_offset,
				0, lfb->lfb_header.lgh_begin.la_sequence,
				0, lfb->lfb_header.lgh_begin.la_block,
				0, lfb->lfb_header.lgh_begin.la_offset,
				0, lfb->lfb_forced_lga.la_sequence,
				0, lfb->lfb_forced_lga.la_block,
				0, lfb->lfb_forced_lga.la_offset);
			if ((lfb->lfb_forced_lga.la_block != 
						lfb->lfb_header.lgh_end.la_block) &&
			    (lfb->lfb_forced_lga.la_offset != 
						lfb->lfb_header.lgh_end.la_offset))
			{
			    uleFormat(NULL, E_DMA461_LGREAD_INMEM_EOF, (CL_ERR_DESC *)NULL,
					ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
				0, lfb->lfb_header.lgh_end.la_sequence,
				0, lfb->lfb_header.lgh_end.la_block,
				0, lfb->lfb_header.lgh_end.la_offset);
			}
		    }
		}
		LG_unmutex(&lfb->lfb_wq_mutex);
		return (LG_ENDOFILE);
	    }
	    LG_unmutex(&lfb->lfb_wq_mutex);
	}
    }

    /* Look in the log buffers, if so inclined */
    if ( lgc->lgc_bufid && *lgc->lgc_bufid )
    {
	/*
	** If the buffer matches sequence+block, then
	** copy the buffer contents to the LGC.
	*/
	lbb = (LBB*)(((char*)LGK_PTR_FROM_OFFSET(lfb->lfb_first_lbb)) +
		((*lgc->lgc_bufid-1) * sizeof(LBB)));

	if ( lbb->lbb_lga.la_sequence == lga->la_sequence &&
	     lbb->lbb_lga.la_block == lga->la_block )
	{
	    if ( status = LG_mutex(SEM_EXCL, &lbb->lbb_mutex) )
		return(status);
	    if ( lbb->lbb_lga.la_sequence == lga->la_sequence &&
		 lbb->lbb_lga.la_block == lga->la_block )
	    {
		lbh = (LBH*)LGK_PTR_FROM_OFFSET(lbb->lbb_buffer);
		lgc->lgc_buffer = (LBH*)lgc->lgc_blocks;

		/* Only need to copy what's used on the page */
		MEcopy((PTR)lbh, lbb->lbb_bytes_used, (PTR)lgc->lgc_buffer);
		/*
		** If CURRENT LBB, then it's in a state of flux
		** and while it's stable for this LGA read, it
		** won't be for a later read on a higher
		** offset.
		*/
		if ( lbb->lbb_state & LBB_CURRENT )
		{
		    lgc->lgc_current_lbb = TRUE;
		    /*
		    ** Update our copy of LBH with lbb_bytes_used; this 
		    ** only gets set by LGwrite when the buffer
		    ** is put on the write queue. If it's LBB_CURRENT,
		    ** lbh_used won't reflect what's actually on the page
		    ** yet, but lbb_bytes_used will.
		    */
		    lbh = (LBH*)lgc->lgc_buffer;
		    lbh->lbh_used = (u_i2)lbb->lbb_bytes_used;
		}
		(VOID)LG_unmutex(&lbb->lbb_mutex);
		return(OK);
	    }
	    (VOID)LG_unmutex(&lbb->lbb_mutex);
	}

	/* Buffer does not match, check the block pool */
    }

    /* Check the block pool forward... */
    if ( lbh )
    {
	if ( lgc->lgc_direction == LG_D_FORWARD )
	{
	    while ( ++lgc->lgc_curblock < lgc->lgc_readblocks )
	    {
		/* Look at following blocks in the block pool */
		lbh = (LBH*)((char*)lbh + lgc->lgc_size);

		if ( lga->la_sequence == lbh->lbh_address.la_sequence &&
		     lga->la_block == lbh->lbh_address.la_block )
		{
		    lgc->lgc_buffer = lbh;
		    return(OK);
		}
	    }
	}
	/* ... or backward */
	else while ( --lgc->lgc_curblock >= 0 )
	{
	    /* Look at preceding blocks in the block pool */
	    lbh = (LBH*)((char*)lbh - lgc->lgc_size);

	    if ( lga->la_sequence == lbh->lbh_address.la_sequence &&
		 lga->la_block == lbh->lbh_address.la_block )
	    {
		lgc->lgc_buffer = lbh;
		return(OK);
	    }
	}
    }

    if ( lgc->lgc_bufid )
    {
	/* Not found in any buffer or block pool, I/O needed */
	*lgc->lgc_bufid = 0;

	/* If we must, force the log before trying the read */
	if ( LGA_GT(lga, &lfb->lfb_forced_lga) )
	{
	    if ( status = LGforce(LG_LAST, *(LG_LXID*)lx_id,
				    (LG_LSN*)NULL, (LG_LSN*)NULL,
				    sys_err) )
	        return(status);
	}
    }

    lpd = (LPD *)LGK_PTR_FROM_OFFSET(lxb->lxb_lpd);
    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpd->lpd_lpb);

    /* Decide first block to read, number of blocks to read */
    lgc->lgc_firstblock = lga->la_block;

    if ( (lgc->lgc_readblocks = lgc->lgc_numblocks) > 1 )
    {
	if ( lgc->lgc_direction == LG_D_FORWARD )
	{
	    if ( lga->la_sequence )
		eofblock = lfb->lfb_forced_lga.la_block;
	    else
		eofblock = lgc->lgc_max;

	    /*
	    ** Determine lgc_readblocks as the number of blocks to DIread,
	    ** the lesser of blocks from LGA to physical EOF, and
	    ** the number of blocks that will fit in the block pool.
	    */
	    if ( eofblock >= lga->la_block )
		lgc->lgc_readblocks = eofblock - lga->la_block + 1;
	    else
		lgc->lgc_readblocks = lgc->lgc_max - lga->la_block + 1;
	    if ( lgc->lgc_readblocks > lgc->lgc_numblocks )
		lgc->lgc_readblocks = lgc->lgc_numblocks;
	}
	else
	{
	    /* Reading backwards */
	    lgc->lgc_readblocks = lga->la_block;

	    if ( lgc->lgc_readblocks > lgc->lgc_numblocks )
	        lgc->lgc_readblocks = lgc->lgc_numblocks;
	    lgc->lgc_firstblock = lga->la_block - lgc->lgc_readblocks + 1;
	}
    }

    /*	Count another physical I/O */

    lgd->lgd_stat.readio++;
    lpb->lpb_stat.readio++;
    lgc->lgc_readio++;

    /* If dual logging is on, decide which log file to read from. */

    if (lfb->lfb_status & LFB_DUAL_LOGGING)
    {
	u_i4	channel_blk;
	u_i4	dual_channel_blk;

	*retry = 1;	/* Indicate that this read can be retried */

	if (lgc->lgc_firstblock > lfb->lfb_channel_blk)
	    channel_blk = lgc->lgc_firstblock - lfb->lfb_channel_blk;
	else
	    channel_blk = lfb->lfb_channel_blk - lgc->lgc_firstblock;

	if (lgc->lgc_firstblock > lfb->lfb_dual_channel_blk)
	    dual_channel_blk = lgc->lgc_firstblock - lfb->lfb_dual_channel_blk;
	else
	    dual_channel_blk = lfb->lfb_dual_channel_blk - lgc->lgc_firstblock;

#ifdef xDEBUG
	/*
	** Check for automated testing bits to force reading from a certain
	** channel, and, if set, make that channel appear 'best' by setting
	** its distance value to 0 and the 'other' distance to 1.
	*/
	if (BTtest((i4)LG_T_READ_FROM_PRIMARY,(char *)lgd->lgd_test_bitvec))
	    channel_blk = 0, dual_channel_blk = 1;
	else if (BTtest((i4)LG_T_READ_FROM_DUAL,(char *)lgd->lgd_test_bitvec))
	    dual_channel_blk = 0, channel_blk = 1;
#endif

	if (channel_blk < dual_channel_blk)
	{
	    lfb->lfb_channel_blk = lgc->lgc_firstblock;
	    *channel = LG_PRIMARY_DI_CB;
	    lgd->lgd_stat.log_readio++;
	}
	else
	{
	    lfb->lfb_dual_channel_blk = lgc->lgc_firstblock;
	    *channel = LG_DUAL_DI_CB;
	    lgd->lgd_stat.dual_readio++;
	}
    }
    else
    {
	/*	Return channel. */

	*retry = 0;

	if (lfb->lfb_active_log == LGD_II_LOG_FILE)
	{
	    *channel = LG_PRIMARY_DI_CB;
	    lgd->lgd_stat.log_readio++;
	}
	else
	{
	    *channel = LG_DUAL_DI_CB;
	    lgd->lgd_stat.dual_readio++;
	}
    }
    if (lfb->lfb_status & LFB_USE_DIIO)
	*di_io = &lfb->lfb_di_cbs[*channel];
    else
	*di_io = &Lg_di_cbs[*channel];
    return ( (*channel != -1) ? OK : LG_BADPARAM );
}

/*
** Name: map_page	- Check page in LGC.
**
** Description:
**      This routine checks to see if the current page in the LGC
**	is the page requested.  If it is not the page requested,
**	the page is read from disk, and the LGC is updated.
**
**	If an error is encountered reading one of the log files, but the other
**	log file is still available, the log file which could not be read is
**	disabled.
**
**	If this process has logging system slaves (Lg_read_slave != 0), then
**	map_page calls the slave to perform the I/O.
**
** Inputs:
**	lx_id				Transaction identifier.
**      lgc                             The LGC to use.
**
** Outputs:
**	sys_err				Reason for error status.
**	Returns:
**	    OK
**	    LG_READERROR
**	    LG_ENDOFILE
**	    LG_BADFORMAT
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	29-oct-1992 (bryanp)
**	    Error message handling improvements.
**	21-jun-1993 (rogerk)
**	    Use casts to quiet ansi compiler on i4 vs. short comparisons.
**	25-Jan-1996 (jenjo02)
**	    Removed acquisition and freeing of lgd_mutex.
**	14-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add ability to read from log buffers,
**	    and bulk read up to lgc_numblocks.
*/
static STATUS
map_page(
LG_LXID		    lx_id,
register LGC        *lgc,
CL_ERR_DESC	    *sys_err)
{
    STATUS		status;
    i4		channel;
    LG_IO		*di_io;
    LG_I4ID_TO_ID	xid;
    i4			retry = 0;
    i4			disable = 0;
    i4		length;
    i4		test_val;
    i4		test_point;
    i4		err_code;

    /* Already have the requested log page? */
    if (lgc->lgc_lga.la_sequence == lgc->lgc_current_lga.la_sequence &&
	lgc->lgc_lga.la_block == lgc->lgc_current_lga.la_block)
    {
	/* 
	** If this log page was the "current" LBB (in a state of flux),
	** go ahead and use it if this read is for a lower offset
	** record, otherwise fall through get a fresh copy of the page.
	*/
	if ( !lgc->lgc_current_lbb ||
	     lgc->lgc_lga.la_offset < lgc->lgc_current_lga.la_offset )
	{
	    return (OK);
	}
    }
    if (lgc->lgc_position != LG_P_PAGE && lgc->lgc_lga.la_block == 0)
    {
	uleFormat(NULL, E_DMA455_LGREAD_HDR_READ, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	return (LG_ENDOFILE);
    }

    xid.id_i4id = lx_id;
    while (1)
    {
	/* Returns lgc_buffer if page found in log buffers or block pool */
	if (status = LG_read(&xid.id_lgid, lgc, &channel, &retry,
			    &disable, &di_io, sys_err))
	    return (status);

	/* If no lgc_buffer returned, then physical I/O is needed */
	if ( !lgc->lgc_buffer )
	{
	    /*
	    ** If the testpoint LG_T_NEXT_READ_FAILS is set on, then simulate
	    ** a failure reading from this channel and turn the test point off so
	    ** that the next read (the failover read from the other channel) can
	    ** work properly.
	    */
	    length = LG_T_NEXT_READ_FAILS;
	    status = LGshow(LG_S_TP_VAL, (PTR)&test_val, sizeof(test_val),
				    &length, sys_err);
	    if (status == OK)
	    {
		if (test_val == 0)
		{
		    /*
		    ** Read the blocks into the block pool.
		    **
		    ** LG_read set lgc_readblocks to the number of
		    ** blocks to read and lgc_firstblock to the first of those blocks.
		    */
		    status = LG_READ(di_io, &lgc->lgc_readblocks, lgc->lgc_firstblock,
					    lgc->lgc_blocks, sys_err);
		}
		else
		{
		    test_point = LG_T_NEXT_READ_FAILS;
		    status = LGalter(LG_A_TP_OFF, (PTR)&test_point,
				    sizeof(test_point), sys_err);
		    if (status)
		    {
			uleFormat(NULL, status, sys_err,
				ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
			return (status);
		    }
		    status = LG_READERROR;
		}
	    }
	    else
	    {
		return (status);
	    }

	    if (status)
	    {
		/* 
		** Read failed. Issue another read if dual logging is on and disable
		** dual logging. 
		*/

		uleFormat(NULL, status, sys_err,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		uleFormat(NULL, E_DMA457_LGREAD_IO_ERROR, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
			0, lgc->lgc_size,
			0, lgc->lgc_lga.la_offset,
			0, (channel == LG_PRIMARY_DI_CB ? "PRIMARY" : "DUAL"));

		if (retry)
		{
		    /* dual logging is on and we haven't yet retried the read */
		    disable = 1;	/* disable the log file we read from */
		    continue;
		}
		else
		{
		    lgc->lgc_status = 0;
		    return (LG_READERROR);
		}
	    }

	    if ( lgc->lgc_direction == LG_D_FORWARD )
	    {
		/* Set lgc_buffer, current block to first block read */
		lgc->lgc_buffer = (LBH*)lgc->lgc_blocks;
		lgc->lgc_curblock = 0;
	    }
	    else
	    {
		/* Set lgc_buffer, current block to last block read */
	        lgc->lgc_buffer = (LBH*)((char*)lgc->lgc_blocks + 
				(lgc->lgc_size * (lgc->lgc_readblocks-1)));
		lgc->lgc_curblock = lgc->lgc_readblocks-1;
	    }
	}

	/*
	** Check the completeness of the page if not read by page,
	** and if not "read" from the log buffers.
	*/

	if ( lgc->lgc_position != LG_P_PAGE && (!lgc->lgc_bufid || !*lgc->lgc_bufid) )
	{
	    if (LGchk_sum((PTR)lgc->lgc_buffer, lgc->lgc_size) != 
					lgc->lgc_buffer->lbh_checksum)
	    {
		uleFormat(NULL, E_DMA458_LGREAD_CKSUM_ERROR, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 5,
			    0, lgc->lgc_lga.la_sequence,
			    0, lgc->lgc_lga.la_block,
			    0, lgc->lgc_lga.la_offset,
			    0, LGchk_sum((PTR)lgc->lgc_buffer, lgc->lgc_size),
			    0, lgc->lgc_buffer->lbh_checksum);
		if (retry)
		{
		    disable = 1;	/* disable the log file we read from */
		    continue;
		}
		else
		{
		    return (LG_CHK_SUM_ERROR);
		}
	    }
	}

	/*
	** Make sure the correct log buffer was read in (make sure we did not
	** wrap around the log file accidentally). Also verify the high and
	** low byte counts in log buffer. Treat these two cases differently:
	** wrong buffer address means we read off the end (or beginning) of the
	** file, while unreasonable lbh_used value indicates that the log page
	** is the wrong format.
	*/
	if ((lgc->lgc_buffer->lbh_address.la_sequence != 
		lgc->lgc_lga.la_sequence) ||
	    ((u_i4) lgc->lgc_buffer->lbh_used < (u_i4) sizeof(LBH)) ||
	    ((u_i4) lgc->lgc_buffer->lbh_used > (u_i4) lgc->lgc_size))
	{
	    if ((lgc->lgc_buffer->lbh_address.la_sequence !=
						    lgc->lgc_lga.la_sequence))
		uleFormat(NULL, E_DMA459_LGREAD_WRONG_PAGE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 6,
			0, lgc->lgc_buffer->lbh_address.la_sequence,
			0, lgc->lgc_buffer->lbh_address.la_block,
			0, lgc->lgc_buffer->lbh_address.la_offset,
			0, lgc->lgc_lga.la_sequence,
			0, lgc->lgc_lga.la_block,
			0, lgc->lgc_lga.la_offset);
	    else
		uleFormat(NULL, E_DMA45A_LGREAD_BADFORMAT, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 6,
			0, lgc->lgc_buffer->lbh_address.la_sequence,
			0, lgc->lgc_buffer->lbh_address.la_block,
			0, lgc->lgc_buffer->lbh_address.la_offset,
			0, lgc->lgc_buffer->lbh_used,
			0, sizeof(LBH),
			0, lgc->lgc_size);

	    if (retry)
	    {
		disable = 1;	/* disable the log file we read from */
		continue;
	    }
	    else
	    {
		if ((lgc->lgc_buffer->lbh_address.la_sequence !=
						    lgc->lgc_lga.la_sequence))
		    return (LG_ENDOFILE);
		else
		    return (LG_BADFORMAT);
	    }
	}

	/*
	** A page has been successfully read.
	*/

	break;
    }

    lgc->lgc_current_lga = lgc->lgc_lga;
    lgc->lgc_high = lgc->lgc_buffer->lbh_used;
    lgc->lgc_low = sizeof(LBH);

    return (OK);
}

/*{
** Name: LGchk_sum - Compute the checksum of a buffer.
**
** Description:
**	This routine computes the checksum of a buffer.  The algorithm must
**	insure that the entire buffer was written to disk.  If the host
**	operating system writes a large buffer in random sector order, this
**	checksum must insure that it can determine if the entire buffer was
**	written or if any part of it failed.
**
**	We checksum the logfile header differently than we checksum a logfile
**	page. We can tell the difference by checking to see whether "size" is
**	< 512 bytes or not...
**
**	The returned checksum doesn't actually include the checksum. That is,
**	we compute a checksum, then we xor out the old checksum from the value
**	which we return. This replaces an older technique which saved the
**	old checksum, set the checksum field to 0, computed the checksum, then
**	restored the old checksum. The problem with this technique is that, at
**	least briefly, the page is in a state where the checksum field is 0,
**	which is dangerous if another thread is trying to write the page while
**	we are checksumming it (this can happen in a dual logging world).
**
** Inputs:
**	buffer				The buffer to be checksumed.
**	size				The size of the buffer.
** Outputs:
**	Returns:
**	    Value of the checksum.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	23-aug-1993 (bryanp)
**	    Use local "buf_ptr" variable -- can't do arithmetic on a PTR.
**	20-sep-1993 (bryanp)
**	    Changed the checksum routine to be non-destructive. Since, in a dual
**		logging environment, both the primary and the dual I/Os may be
**		in progress, we must ensure that the checksum routine doesn't
**		actually touch the page while it's checksumming it (if it did,
**		then we could have the situation where one thread is computing
**		the checksum while another thread is writing the page, thus
**		causing the page that gets written to be written in a funny
**		state).
**	31-jan-1994 (jnash)
**	    Fail checksum of a log header containing all zeroes.
**	    by generating a checksum == -1.
**	08-Sep-2000 (jenjo02)
**	    Rather than checksumming 16 bytes every 512 bytes, just
**	    checksum the first and last 16 "used" bytes of the log buffer.
**	    Do likewise for the log header instead of checksumming
**	    the entire header.
**	    Determine if log header if "size" is its size instead of
**	    size < 512.
*/
u_i4
LGchk_sum(PTR buffer, i4 size)
{
    register	u_i4 sum = 0;
    i4		i;
    LG_HEADER	*header = (LG_HEADER*)NULL;
    LBH		*lbh = (LBH*)NULL;
    char	*buf_ptr = (char *)buffer;
    u_i4	*v;

    /*
    ** Compute the checksum of the log block, log header
    ** from the first and last 16 bytes of "buffer".
    **
    ** If a non-header page, include the last 16 "used"
    ** 16 bytes instead of the end of the buffer; we
    ** don't care about anything beyond that anyway.
    */
    if ( size == sizeof(LG_HEADER) )
	header = (LG_HEADER *)buffer;
    else
    {
	lbh = (LBH *)buffer;
	if ( lbh->lbh_used >= (sizeof(u_i4) * 8) )
	    size = lbh->lbh_used;
    }
	    

    for ( i = 2; i--; )
    {
	v = (u_i4 *)buf_ptr;
	sum ^= v[0] ^ v[1] ^ v[2]^ v[3] ;
	buf_ptr += size - (sizeof(u_i4) * 4);
    }
    
    /* Remove the lbh_checksum/lgh_checksum value from "sum" */
    if ( header )
    {
	if (CL_OFFSETOF(LG_HEADER,lgh_checksum) < (4 * sizeof(u_i4)))
	    sum ^= header->lgh_checksum;

	/*
	** If the checksum is zero, check if entire header is zero.
	** If it is, return a nonzero checksum, ensuring a checksum failure.
	*/
	if ( sum == 0 )
	{
	    LG_HEADER	zero_header;

	    MEfill(sizeof(LG_HEADER), '\0', (PTR)&zero_header); 

	    if (MEcmp((char *)header, (char *)&zero_header, 
		      sizeof(LG_HEADER)) == 0)
	    {
		sum = -1;
	    }
	}
    }
    else
    {
	if (CL_OFFSETOF(LBH,lbh_checksum) < (4 * sizeof(u_i4)))
	    sum ^= lbh->lbh_checksum;
    }

    return(sum);
}
