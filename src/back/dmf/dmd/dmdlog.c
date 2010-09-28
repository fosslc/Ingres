/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <pc.h>
#include    <si.h>
#include    <sl.h>
#include    <st.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <lg.h>
#include    <lk.h>
#include    <me.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dmd.h>
#include    <dmfrcp.h>
#include    <lgdstat.h>
#include    <lgdef.h>

/**
**
**  Name: DMDLOG.C - Debug print routine for log records.
**
**  Description:
**      This module contains the routinte to format log records.
**
**          dmd_log - Format log records.
**
**
**  History:
**      13-nov-1986 (Derek)
**          Created for Jupiter.
**	15-aug-1989 (rogerk)
**	    Added support for TCB_GATEWAY tables.
**	18-sep-1989 (rogerk)
**	    Added DM0L_DMU log record.
**	 2-oct-1989 (rogerk)
**	    Added DM0L_FSWAP log record.
**	 9-apr-1990 (sandyh)
**	    Fixed logdump A/V on BCP records.
**	25-feb-1991 (rogerk)
**	    Added DM0L_TEST log record.
**	16-jul-1991 (bryanp)
**	    B38527: Added new DM0L_MODIFY & DM0L_INDEX log records, renamed
**	    old records to DM0L_OLD_MODIFY & DM0L_OLD_INDEX. Added new
**	    DM0L_SM0_CLOSEPURGE log record.
**      30-aug-1991 (rogerk)
**          Part of Rollforward of relocate/reorganize fixes.  Added
**          DM0L_REORG flag to modify log record.
**      19-aug-1991 (jnash) merged 24-sep-1990 (rogerk)
**          Merged 6.3 changes into 6.5.
**	18-Oct-1991 (rmuth)
**	    Add extend and allocation to DM0L_CREATE, DM0L_MODIFY,
**	    DM0L_INDEX.
**	28-oct-1991 (mikem)
**	    Fixed logging dump of the DM0LASSOC, DM0LALLOC, DM0LDEAlLOC, 
**	    DM0LEXTEND records by dmd_log() to pass table id's as 2 i4's 
**	    rather than counting on passing a structure on the stack to result 
**	    in pushing 2 i4's on the stack.  This fixed the problem with 
**	    logdump printing negative (uninitialized) values for table id's 
**	    on sparc's for these log records.
**	 3-nov-1991 (rogerk)
**	    Added handling of DM0L_OVFL log record.
**	    Added as part of fixes for REDO handling of file extends.
**	07-july-1992 (jnash)
**	    Add DMF Function prototyping.
**      25-sep-1992 (nandak)
**          Printing format of distributed transaction id changed.
**	26-oct-1992 (rogerk & jnash)
**	    Reduced Logging Project: changed to support variable length
**	    table and owner names in new log records.
**	    Added new log records: NOFULL, FMAP, AI, BTPUT, BTDEL, BTSPLIT,
**	    BTOVFL, BTFREE, BTUPDTOVFL, DISASSOC, CRVERIFY.
**	    Removed old log records: FSWAP.
**	13-jan-1993 (jnash)
**	     More reduced logging.  Eliminate SBI, SREP, SDEL, SPUT support.
**	10-feb-1993 (jnash)
**	     More reduced logging.  Create dmd_format_log_hdr(), 
**	     dmd_format_log() and dmd_put_line().  Call these routines
**	     rather than using inline code.  Routines also used by AUDITDB.  
**	     Also, permit hex dump of log record header and contents.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed special tran_id, db_id fields from the end transaction
**		record.  These values are correctly written the in record
**		header now.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Responded to some lint suggestions.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	24-may-1993 (rogerk)
**	    Fixed formatting of the BI and AI record operation fields.
**	21-jun-1993 (mikem)
**	    su4_us5 port.  Added include of me.h to pick up new macro 
**	    definitions for MEcopy, MEfill, ...
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    The DM0L_CLOSEDB log record no longer exists.
**	    Made some attempts to resolve extra-newline problems when printing
**		partial lines using multiple TRformat calls.
**	26-jul-1993 (rogerk)
**	    Added split direction to SPLIT log records.
**	    Added split position value to the trace output for SPLIT records.
**	    Added ecp_begin_la to the ECP record.
**	    Include respective CL headers for all cl calls.
**	23-aug-1993 (rogerk)
**	    Added a statement count field to the modify and index log records.
**	20-sep-1993 (rogerk)
**	    Add new DM0L_NONJNL_TAB log record flag to log trace output.
**	18-oct-1993 (jnash)
**	    Change -verbose output to note compressed REPLACE logs.
**	    Move -verbose header output before log contents output. 
**	18-oct-93 (jrb)
**          Added tracing for DM0LEXTALTER.
**	18-oct-1993 (rogerk)
**	    Added dum_buckets, dui_buckets fields to modify and index records.
**	    Changed to print dum_reltups, dui_reltups always.
**	    Added username to bcp transaction information.
**	22-nov-1993 (bryanp) B56479
**	    Removed a "%<" from a TRdisplay.
**	11-dec-1993 (rogerk)
**	    Added support in replace record formatting for new fields added in
**	    changes to replace record compression.
**	    Added dmfrcp.h include to silence compiler warnings.
**	31-jan-1994 (rogerk)
**	    Changed dumping of create and destroy log records to handle view
**	    log records with location counts of zero.
**      10-apr-1995 (stial01) B66586
**          Integrate fix done by nick, Verbose logging of modify and index
**          didn't log key compression in the 'STATUS:' field.
**	06-mar-1996 (stial01 for bryanp)
**	    Added a page_size field to a number of log records.
**      01-may-1996 (stial01)
**          DM0L_AI removed ai_page: after image pg immediately follows DM0l_AI
**          DM0L_BI removed bi_page: before image pg immediately follows DM0L_BI
**	12-dec-1996 (shero03)
**	    Added RTree log records
**	    DM0L_SPLIT removed split_page: split page immediately follows 
**	    DM0L_SPLIT
**	10-jan-1997 (nanpr01)
**	    Added a spaceholder for the page for alignment of DM0L_BI,
**	    DM0L_AI and DM0l_BTSPLIT.
**      08-Dec-1997 (hanch04)
**          Use new la_block field when block number is needed instead
**          of calculating it from the offset
**          Keep track of block number and offset into the current block.
**          for support of logs > 2 gig
**	16-feb-1999 (nanpr01)
**	    Support for update mode lock.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	04-may-1999 (hanch04)
**	    Change TRformat's print function to pass a PTR not an i4.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      18-apr-2003 (stial01)
**          Fixed printing of DM0LBTPUT, DM0LBTDEL log records
**	6-Feb-2004 (schka24)
**	    Updates to index, modify records.
**	1-Apr-2004 (schka24)
**	    Add rawdata record.
**      12-may-2004 (stial01)
**          Fixed printing of log records
**      11-may-2005 (horda03) Bug 114471/INGSRV 3294 
**          New DM0L_HEADER flag values displayed.
**      12-apr-2007 (stial01)
**          Added support for rollforwarddb -incremental
**      11-mar-2008 (stial01)
**          Added new log records to trace output
**      01-may-2008 (horda03) Bug 120349
**          Use macros from %w and %v TRdisplay directives
**          defined where type fields are defined. Thus giving
**          consistent text everywhere.
**	30-May-2008 (jonj)
**	    LSNs are composed of 2 u_i4s; display them in hex
**	    rather than signed decimal, "%x,%x" everywhere.
**	23-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**      01-apr-2010 (stial01)
**          Changes for Long IDs, db_buffer holds (dbname, owner ...)
**	21-Jul-2010 (stial01) (SIR 121123 Long Ids)
**          Remove table name,owner from log records.
*/

/*
**  Forward function references.
*/

static VOID dmd_hexdump_contents(
    			i4	(*format_routine)(
			PTR	     arg,
			i4	     length,
			char         *buffer),
    char            *prefix,
    i4	    prefix_len,
    char            *record,
    i4	    record_len,
    char	    *line_buf);

static VOID dmd_hexdump_header(
			i4	(*format_routine)(
    			PTR	     arg,
			i4	     length,
			char         *buffer),
    char	    *h,
    char	    *line_buf, 
    i4	    l_line_buf);

static i4
dmd_put_partial_line(
    PTR		    arg,
    i4		    length,
    char            *buffer);

static VOID FormatBufs(
    LG_BUFFER	    *bufs, 
    i4 		    length);

static VOID FormatDisTranId(
    DB_DIS_TRAN_ID *dis_tran_id);

/*{
** Name: dmd_log	- Format log record.
**
** Description:
**      This routine will display a formatted log record.
**
** Inputs:
**      record                          The log record.
**	size				Log page size.
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
**      13-nov-1986 (Derek)
**          Created for Jupiter.
**      25-feb-1989 (ac)
**          Added 2PC support.
**	15-aug-1989 (rogerk)
**	    Added support for TCB_GATEWAY tables.
**	    Added MULTI_LOC, GATEWAY, RULE , and SYSTEM_MAINTAINED to format
**	    string for printing out table attributes.
**	18-sep-1989 (rogerk)
**	    Added DM0L_DMU log record.
**	 2-oct-1989 (rogerk)
**	    Added DM0L_FSWAP log record.
**	 9-apr-1990 (sandyh)
**	    Fixed BCP record tracing A/V.
**	25-feb-1991 (rogerk)
**	    Added DM0L_TEST log record.
**	16-jul-1991 (bryanp)
**	    B38527: Added new DM0L_MODIFY & DM0L_INDEX log records, renamed
**	    old records to DM0L_OLD_MODIFY & DM0L_OLD_INDEX. Added new
**	    DM0L_SM0_CLOSEPURGE log record.
**      30-aug-1991 (rogerk)
**          Part of Rollforward of relocate/reorganize fixes.  Added
**          DM0L_REORG flag to modify log record.
**	28-oct-1991 (mikem)
**	    Fixed logging of the DM0LASSOC, DM0LALLOC, DM0LDEAlLOC, DM0LEXTEND
**	    records to pass table id's as 2 i4's rather than counting on
**	    passing a structure on the stack to result in pushing 2 i4's on
**	    the stack.  This fixed the problem with logdump printing negative
**	    (uninitialized) values for table id's on sparc's for these log
**	    records.
**	 3-nov-1991 (rogerk)
**	    Added handling of DM0L_OVFL log record.
**	    Added as part of fixes for REDO handling of file extends.
**	07-july-1992 (jnash)
**	    Add function prototypes, make function VOID.
**	26-oct-1992 (rogerk & jnash)
**	    Reduced Logging Project: 
**	    Changed to reflect new values of log record types.  With new log
**	    record formats for almost every log record the type values were
**	    changed so that we can continue to recognize old log records.
**	    Changed to support variable length table and owner names in new 
**	    log records.  Added new log record types.
**	13-jan-1993 (jnash)
**	     More reduced logging.  Eliminate SBI, SREP, SDEL, SPUT support.
**	10-feb-1993 (jnash)
**	     More reduced logging.  Modify routine to call dmd_format_dm_hdr()
**	     and dmd_format_log() rather than inline code.  
*/
VOID
dmd_log(
bool		verbose_flag,
PTR             record,
i4		size)
{
#define 	MAX_OUTPUT_WIDTH	256
    char	   line_buf[MAX_OUTPUT_WIDTH];

    dmd_format_dm_hdr(dmd_put_line, (DM0L_HEADER *)record, line_buf, 
	sizeof(line_buf));

    dmd_format_log(verbose_flag, dmd_put_line, record, size, line_buf, 
	sizeof(line_buf));

    return;
}

/*{
** Name: dmd_format_dm_hdr	- Format log record (DM0L) header
**
** Description:
**      This routine will display a log record header.
**
** Inputs:
**      record                          The log record.
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
**	10-feb-1993 (jnash)
**	     Extracted from dmd_log() and made into new function.  Made
**	     log record type appear as first item in output.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	12-dec-1996 (shero03)
**	    Added RTree log records
**	29-Apr-2004 (gorvi01)
**		Added UNEXTEND log records.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Display DM0L_CRHEADER when present.
*/
VOID
dmd_format_dm_hdr(
		    i4	    (*format_routine)(
		    PTR		arg,
		    i4		length,
		    char        *buffer),
DM0L_HEADER	*h,
char		*line_buf,
i4		l_line_buf)
{
    LG_LSN	comp_lsn; 
    DM0L_CRHEADER *CRhdr;
    DM0L_CRHEADER *CRhdr2;

    if (h->flags & DM0L_CLR)
    {
	comp_lsn = h->compensated_lsn;
    }
    else
    {
	comp_lsn.lsn_high = 0; 
	comp_lsn.lsn_low = 0; 
    }

    TRformat(format_routine, 0, line_buf, l_line_buf,
    	"%8* LSN=(%x,%x), COMP_LSN=(%x,%x), DBID=0x%x, XID=0x%x%x\n", 
	h->lsn.lsn_high, h->lsn.lsn_low, 
	comp_lsn.lsn_high, comp_lsn.lsn_low, 
	h->database_id, 
	h->tran_id.db_high_tran, h->tran_id.db_low_tran);

    TRformat(format_routine, 0, line_buf, l_line_buf,
    	"%8* %12w Length:%6d  Flags: %v\n",
	DM0L_RECORD_TYPE, h->type,
	h->length, 
	DM0L_HEADER_FLAGS,
	h->flags);

    /* Don't print CR information if  SINGLEUSER server like JSP */
    if ( (h->flags & DM0L_CR_HEADER ) &&
	(dmf_svcb->svcb_status & SVCB_SINGLEUSER) == 0)
    {
        CRhdr = (DM0L_CRHEADER*)((char*)h + sizeof(DM0L_HEADER));
	if (h->type == DM0LASSOC)
	    CRhdr2 = CRhdr + 1;
	else
	    CRhdr2 = NULL;

	TRformat(format_routine, 0, line_buf, l_line_buf,
	    "%8* PREV_LSN=(%x,%x), PREV_LGA=<%x,%x,%x>\n"
	    "%8* PREV_JFA=(%d,%d), PREV_BUFID=%d LG_ID=%d\n",
	    CRhdr->prev_lsn.lsn_high,
	    CRhdr->prev_lsn.lsn_low,
	    CRhdr->prev_lga.la_sequence,
	    CRhdr->prev_lga.la_block,
	    CRhdr->prev_lga.la_offset,
	    CRhdr->prev_jfa.jfa_filseq,
	    CRhdr->prev_jfa.jfa_block,
	    CRhdr->prev_bufid,
	    h->lg_id);

	if (CRhdr2)
	    TRformat(format_routine, 0, line_buf, l_line_buf,
		"%8* HDR2_LSN=(%x,%x), HDR2_LGA=<%x,%x,%x>\n"
		"%8* HDR2_JFA=(%d,%d), HDR2_BUFID=%d\n",
		CRhdr2->prev_lsn.lsn_high,
		CRhdr2->prev_lsn.lsn_low,
		CRhdr2->prev_lga.la_sequence,
		CRhdr2->prev_lga.la_block,
		CRhdr2->prev_lga.la_offset,
		CRhdr2->prev_jfa.jfa_filseq,
		CRhdr2->prev_jfa.jfa_block,
		CRhdr2->prev_bufid);
    }

    return;
}

/*{
** Name: dmd_format_log	- Format log record.
**
** Description:
**      This routine will display a formatted log record.
**
** Inputs:
**      record                          The log record.
**	log_page_size			Log page size.
**	line_buf			Output buffer
**	l_line_buf			Length of output buffer.
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
**	10-feb-1993 (jnash)
**	    Extracted from dmd_log() and made into new function.
**	    Call TRformat instead of TRdisplay, pass in format function 
**	    name.  Add verbose option to dump header and additional info.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed special tran_id, db_id fields from the end transaction
**		record.  These values are correctly written the in record
**		header now.
**	    Changed Abortsave and EndMini log records to be written as CLR's.
**	        The compensated_lsn field is now used to skip backwards in
**		in the log file rather than using the log addresses in the
**		log records themselves.
**	    Changed ECP to point back to the BCP via an LSN instead of a LA.
**	24-may-1993 (rogerk)
**	    Fixed formatting of the BI and AI record operation fields.
**	    Right-shifted the list of types since the first value in the 
**	    vector is assumed to be zero.
**	26-jul-1993 (bryanp)
**	    The DM0L_CLOSEDB log record no longer exists.
**	26-jul-1993 (rogerk)
**	    Added split direction to SPLIT log records.
**	    Added split position value to the trace output for SPLIT records.
**	    Added ecp_begin_la to the ECP record.
**	18-oct-1993 (jnash)
**	    Change -verbose output to note compressed REPLACE logs.
**	    Fix BCP display to output log addresses in lg1,lg2,lg3 format.
**	    Move -verbose header display prior to log body. 
**	18-oct-93 (jrb)
**          Added tracing for DM0LEXTALTER.
**	18-oct-1993 (rogerk)
**	    Added dum_buckets, dui_buckets fields to modify and index records.
**	    Changed to print dum_reltups, dui_reltups always.
**	    Took bcp_bof out of BCP log record since it is not set.
**	    Added username to bcp transaction information.
**	11-dec-1993 (rogerk)
**	    Added support in replace record formatting for new fields added in
**	    changes to replace record compression.
**	31-jan-1994 (rogerk)
**	    Changed dumping of create and destroy log records to handle view
**	    log records with location counts of zero.
**      10-apr-1995 (stial01) B66586
**          Integrate fix done by nick, Verbose logging of modify and index
**          didn't log key compression in the 'STATUS:' field.
**      06-mar-1996 (stial01 for bryanp)
**          Added a page_size field to a number of log records.
**      01-may-1996 (stial01)
**          DM0L_AI removed ai_page: after image pg immediately follows DM0l_AI
**          DM0L_BI removed bi_page: before image pg immediately follows DM0L_BI
**	12-dec-1996 (shero03)
**	    Added RTree log records
**	    DM0L_SPLIT removed split_page: split page immediately follows 
**	    DM0L_SPLIT
**	09-jan-1997 (nanpr01)
**	    Fixed the previous BUS error. We store the page first followed
**	    by the key.
**	10-jan-1997 (nanpr01)
**	    Added a spaceholder for the page for alignment of DM0L_BI,
**	    DM0L_AI and DM0l_BTSPLIT.
**	23-Dec-2003 (jenjo02)
**	    Added dui_relstat2 to DM0L_INDEX, btp_partno, btd_partno,
**	    bto_tidsize, btf_tidsize, all for Global indexes.
**	1-Apr-2004 (schka24)
**	    Add rawdata, factor out header hexdump.
**	13-apr-2004 (thaju02)
**	    Online Modify Project. Added case for readnolock lsn log 
**	    record (DM0LRNLLSN).
**       5-May-2006 (hanal04) Bug 116059
**          BCP flag CP_FIRST added.
**	22-May-2006 (jenjo02)
**	    Add ixklen, spl_range_klen for Clustered tables.
**	11-Sep-2006 (jonj)
**	    Add tx_lkid to DM0LBCP.
**	17-Dec-2008 (jonj)
**	    B121349, don't display MODIFY locations if dum_loc_count is zero.
**      23-Feb-2009 (hanal04) Bug 121652
**          Added DM0LUFMAP case to deal with FMAP updates needed for an
**          extend operation where a new FMAP will not reside in the last
**          FMAP or new FMAP itself.
**	29-Oct-2009 (kschendel) SIR 122739
**	    Remove ixklen, add btflags
**	5-Nov-2009 (kschendel) SIR 122739
**	    Remove old-index record.
**	15-Jan-2010 (jonj)
**	    Use common lock formatter LKkey_to_string().
**          
*/
VOID
dmd_format_log(
bool		    verbose_flag,
			i4	(*format_routine)(
			PTR	     arg,
			i4	     length,
			char         *buffer),
char                *record,
i4		    log_page_size,
char		    *line_buf,
i4		    l_line_buf)
{
    DM0L_HEADER     *h = (DM0L_HEADER *)record;
    PTR		    rec_ptr;

    if (verbose_flag)
	dmd_hexdump_header(format_routine, (char *)h, line_buf, l_line_buf);

    switch (h->type)
    {
    case DM0LBT:
	{
	    DM0L_BT	    *r = (DM0L_BT*)h;

            TRformat(format_routine, 0, line_buf, l_line_buf,
		"%12* USERNAME: %~t TIME: %?\n",
		sizeof(r->bt_name), &r->bt_name, &r->bt_time);
	}
	break;

    case DM0LET:
	{
	    DM0L_ET	    *r = (DM0L_ET*)h;

            TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TIME: %? STATE: %v%s\n",
		&r->et_time, DM0L_ET_FLAGS, r->et_flag,
		((r->et_master) ? " (BY RECOVERY PROCESS)" : ""));
	}
	break;

    case DM0LBI:
	{
	    DM0L_BI	    *r = (DM0L_BI*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) PAGE: %d Size %d Op: %w\n",
		r->bi_tbl_id.db_tab_base, r->bi_tbl_id.db_tab_index,
		r->bi_pageno, r->bi_page_size,
		DM0L_BI_OPERATION, r->bi_operation);

	    if (verbose_flag)
		dmd_hexdump_contents(format_routine, "    BEFORE IMAGE", 16, 
		      (PTR)( (char *)r + sizeof(DM0L_BI) - sizeof(DMPP_PAGE)),
		      r->bi_page_size, line_buf);
	}
	break;

    case DM0LAI:
	{
	    DM0L_AI	    *r = (DM0L_AI*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) PAGE: %d Size %d Op: %w\n",
		r->ai_tbl_id.db_tab_base, r->ai_tbl_id.db_tab_index,
		r->ai_pageno, r->ai_page_size,
		DM0L_BI_OPERATION, r->ai_operation);

	    if (verbose_flag)
		dmd_hexdump_contents(format_routine, "    AFTER IMAGE", 15, 
		      (PTR)( (char *)r + sizeof(DM0L_AI) - sizeof(DMPP_PAGE)),
		      r->ai_page_size, line_buf);
	}
	break;

    case DM0LPUT:
	{
	    DM0L_PUT	    *r = (DM0L_PUT*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) TID: [%d,%d] SIZE: %d\n",
		r->put_tbl_id.db_tab_base, r->put_tbl_id.db_tab_index,
		r->put_tid.tid_tid.tid_page, r->put_tid.tid_tid.tid_line,
		r->put_rec_size);

	    if (verbose_flag)
	    {
		rec_ptr = (PTR) (((char *)r) + sizeof(DM0L_PUT));
		dmd_hexdump_contents(format_routine, "    TUPLE", 9, rec_ptr, 
		    r->put_rec_size, line_buf);
	    }

	}
	break;

    case DM0LDEL:
	{
	    DM0L_DEL	    *r = (DM0L_DEL*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) TID: [%d,%d] SIZE: %d\n",
		r->del_tbl_id.db_tab_base, r->del_tbl_id.db_tab_index,
		r->del_tid.tid_tid.tid_page, r->del_tid.tid_tid.tid_line,
		r->del_rec_size);

	    if (verbose_flag)
	    {
		rec_ptr = (PTR) (((char *)r) + sizeof(DM0L_DEL));
		dmd_hexdump_contents(format_routine, "    TUPLE", 9, rec_ptr, 
		    r->del_rec_size, line_buf);
	    }
	}
	break;

    case DM0LREP:
	{
	    DM0L_REP	    *r = (DM0L_REP*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) OTID: [%d,%d] NTID: [%d,%d] SIZE: old %d, new %d\n", 
		r->rep_tbl_id.db_tab_base, r->rep_tbl_id.db_tab_index,
		r->rep_otid.tid_tid.tid_page, r->rep_otid.tid_tid.tid_line,
		r->rep_ntid.tid_tid.tid_page, r->rep_ntid.tid_tid.tid_line,
		r->rep_orec_size, r->rep_nrec_size);
	    TRformat(format_routine, 0, line_buf, l_line_buf,
		"%12* Compressed Sizes: old %d, new %d, diff_offset %d\n",
		r->rep_odata_len, r->rep_ndata_len, r->rep_diff_offset);

	    if (verbose_flag)
	    {
		rec_ptr = (PTR) (((char *)r) + sizeof(DM0L_REP));

		if (r->rep_header.flags & DM0L_COMP_REPL_OROW)
		{
		    dmd_hexdump_contents(format_routine, 
			"    COMPRESSED OLD ROW", 22, rec_ptr, 
			r->rep_odata_len, line_buf);
		}
		else
		{
		    dmd_hexdump_contents(format_routine, 
			"    OLD ROW", 11, rec_ptr, 
			r->rep_odata_len, line_buf);
		}

		rec_ptr = (PTR)rec_ptr + r->rep_odata_len;
		dmd_hexdump_contents(format_routine, 
		    "    COMPRESSED NEW ROW", 22, rec_ptr, 
		    r->rep_ndata_len, line_buf);
	    }
	}
	break;

    case DM0LBMINI:
	break;

    case DM0LEMINI:
	{
	    DM0L_EM	    *r = (DM0L_EM*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* BMINI LSN: (%x,%x)\n",
		r->em_header.compensated_lsn.lsn_high,
		r->em_header.compensated_lsn.lsn_low);
	}
	break;

    case DM0LBALLOC:
	{
	    DM0L_BALLOC	    *r = (DM0L_BALLOC*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) FREE: %d  NEXT: %d\n",
		r->ba_tbl_id.db_tab_base, r->ba_tbl_id.db_tab_index,
		r->ba_free, r->ba_next_free);
	}
	break;
 
    case DM0LBDEALLOC:
	{
	    DM0L_BDE	    *r = (DM0L_BDE*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) FREE: %d  NEXT: %d\n",
		r->bd_tbl_id.db_tab_base, r->bd_tbl_id.db_tab_index,
		r->bd_free, r->bd_next_free);
	}
	break;
 
    case DM0LCALLOC:
	{
	    DM0L_CALLOC	    *r = (DM0L_CALLOC*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) FREE: %d  ROOT: %d\n",
		r->ca_tbl_id.db_tab_base, r->ca_tbl_id.db_tab_index,
		r->ca_free, r->ca_root);
	}
	break;

    case DM0LSAVEPOINT:
	{
	    DM0L_SAVEPOINT  *r = (DM0L_SAVEPOINT*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* ID: %d\n", r->s_id);
	}
	break;

    case DM0LABORTSAVE:
	{
	    DM0L_ABORT_SAVE *r = (DM0L_ABORT_SAVE*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	        "%12* ID: %d SAVEPOINT LSN: (%x,%x)\n",
		r->as_id, 
		r->as_header.compensated_lsn.lsn_high,
		r->as_header.compensated_lsn.lsn_low);
	}
	break;

    case DM0LFRENAME:
	{
	    DM0L_FRENAME    *r = (DM0L_FRENAME*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* PATH: %t OLD: %t NEW: %t\n",
		r->fr_l_path, &r->fr_path, sizeof(r->fr_oldname),
		&r->fr_oldname, sizeof(r->fr_newname), &r->fr_newname);
	}
	break;

    case DM0LFCREATE:
	{
	    DM0L_FCREATE    *r = (DM0L_FCREATE*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
		"%12* PATH: %t FILE: %t\n",
		r->fc_l_path, &r->fc_path, sizeof(r->fc_file), &r->fc_file);
	}
	break;

    case DM0LSBACKUP:
	{
	    DM0L_SBACKUP    *r = (DM0L_SBACKUP *)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* DBID: %x\n", r->sbackup_dbid);
	}
	break;

    case DM0LEBACKUP:
	{
	    DM0L_EBACKUP    *r = (DM0L_EBACKUP *)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* DBID: %x\n", r->ebackup_dbid);
	}
	break;

    case DM0LOPENDB:
	{
	    DM0L_OPENDB	    *r = (DM0L_OPENDB*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* NAME(%~t,%~t) ID:%d ROOT: %t\n",
		sizeof(r->o_dbname), &r->o_dbname, sizeof(r->o_dbowner),
		&r->o_dbowner, r->o_dbid, 
                r->o_l_root, &r->o_root);
	}
	break;

    case DM0LCREATE:
	{
	    DM0L_CREATE	    *r = (DM0L_CREATE*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE:(%d, %d) NAME:(%~t,%~t)\n%12* Alloc: %d\n",
		r->duc_tbl_id.db_tab_base, r->duc_tbl_id.db_tab_index,
		sizeof(r->duc_name), &r->duc_name, sizeof(r->duc_owner),
		&r->duc_owner, r->duc_allocation);
	    if (r->duc_loc_count)
	    {
		TRformat(format_routine, 0, line_buf, l_line_buf,
		    "%#.#{%16* LOCATION: %.#s\n%}\n",
		    r->duc_loc_count, sizeof(DB_LOC_NAME), 
		    r->duc_location, DB_LOC_MAXNAME, 0);
	    }
	}
	break;

    case DM0LCRVERIFY:
	{
	    DM0L_CRVERIFY    *r = (DM0L_CRVERIFY*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE:(%d, %d) NAME:(%~t,%~t), RELPAGES: %d PSZ:%d\n",
		r->ducv_tbl_id.db_tab_base, r->ducv_tbl_id.db_tab_index,
		sizeof(r->ducv_name), &r->ducv_name, sizeof(r->ducv_owner),
		&r->ducv_owner, r->ducv_relpages, r->ducv_page_size);

	}
	break;

    case DM0LDESTROY:
	{
	    DM0L_DESTROY    *r = (DM0L_DESTROY*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d, %d) NAME(%~t,%~t)\n",
		r->dud_tbl_id.db_tab_base, r->dud_tbl_id.db_tab_index,
		sizeof(r->dud_name), &r->dud_name, 
		sizeof(r->dud_owner), &r->dud_owner);
	    if (r->dud_loc_count)
	    {
		TRformat(format_routine, 0, line_buf, l_line_buf,
		    "%#.#{%16* LOCATION: %.#s\n%}",
		    r->dud_loc_count, sizeof(DB_LOC_NAME),
		    r->dud_location, DB_LOC_MAXNAME, 0);
	    }
	}
	break;

    case DM0LRELOCATE:
	{
	    DM0L_RELOCATE   *r = (DM0L_RELOCATE*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d, %d) NAME(%~t,%~t)\n%12* OLD_LOCATION: %t NEW_LOCATION: %t\n", 
		r->dur_tbl_id.db_tab_base, r->dur_tbl_id.db_tab_index,
		sizeof(r->dur_name), &r->dur_name, sizeof(r->dur_owner),
		&r->dur_owner, sizeof(r->dur_olocation), &r->dur_olocation,
		sizeof(r->dur_nlocation), &r->dur_nlocation);
	}
	break;

    case DM0LOLDMODIFY:
	{
	    DM0L_OLD_MODIFY	    *r = (DM0L_OLD_MODIFY*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) NAME(%~t,%~t)\n",
		r->dum_tbl_id.db_tab_base, r->dum_tbl_id.db_tab_index,
		sizeof(r->dum_name), &r->dum_name, sizeof(r->dum_owner),
		&r->dum_owner);
	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* STRUCTURE: %w STATUS: %v\n",
		DM0L_DUM_STRUCTS, r->dum_structure,
		RELSTAT, r->dum_status);
	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* MINPAGES: %d MAXPAGES: %d IFILL: %d DFILL: %d LFILL: %d\n",
		r->dum_min, r->dum_max, r->dum_ifill, 
                r->dum_dfill, r->dum_lfill);
	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%#.#{%16* LOCATION: %.#s\n%}",
		r->dum_loc_count, sizeof(DB_LOC_NAME),
		r->dum_location, DB_LOC_MAXNAME, 0);
	}
 	break;

    case DM0LMODIFY:
	{
	    DM0L_MODIFY	    *r = (DM0L_MODIFY*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) NAME(%~t,%~t)\n",
		r->dum_tbl_id.db_tab_base, r->dum_tbl_id.db_tab_index,
		sizeof(r->dum_name), &r->dum_name, sizeof(r->dum_owner),
		&r->dum_owner);
	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* STRUCTURE: %w STATUS: %v\n",
                DM0L_DUM_STRUCTS, r->dum_structure,
		RELSTAT, r->dum_status);
	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* MINPAGES: %d MAXPAGES: %d IFILL: %d DFILL: %d LFILL: %d\n",
		r->dum_min, r->dum_max, r->dum_ifill, 
                r->dum_dfill, r->dum_lfill);
	    TRformat(format_routine, 0, line_buf, l_line_buf,
		"%12* BUCKETS: %d  RELTUPS: %d\n", 
		r->dum_buckets, r->dum_reltups);
	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* Alloc: %d  Extend: %d  Page size: %d Type: %d  Name: (%d, %d)\n",
		r->dum_allocation, r->dum_extend, r->dum_page_size, r->dum_pg_type,
		r->dum_name_id, r->dum_name_gen);
	    if ( r->dum_loc_count )
		TRformat(format_routine, 0, line_buf, l_line_buf,
		    "%#.#{%16* LOCATION: %.#s\n%}",
		    r->dum_loc_count, sizeof(DB_LOC_NAME),
		    r->dum_location, DB_LOC_MAXNAME, 0);
	}
	break;

    case DM0LLOAD:
	{
	    DM0L_LOAD	    *r = (DM0L_LOAD*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) NAME(%~t,%~t) LOCATION: %t\n",
		r->dul_tbl_id.db_tab_base, r->dul_tbl_id.db_tab_index,
		sizeof(r->dul_name), &r->dul_name, sizeof(r->dul_owner),
		&r->dul_owner, sizeof(r->dul_location), &r->dul_location);
	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* STRUCTURE: %w RECREATE: %d\n",
		DM0L_DUM_STRUCTS, r->dul_structure, r->dul_recreate);
	}
	break;

    case DM0LINDEX:
	{
	    DM0L_INDEX	    *r = (DM0L_INDEX*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) NAME(%~t,%~t)\n",
		r->dui_tbl_id.db_tab_base, r->dui_tbl_id.db_tab_index,
		sizeof(r->dui_name), &r->dui_name, sizeof(r->dui_owner),
		&r->dui_owner);
	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* STRUCTURE: %w STATUS: %v STAT2: %v\n",
		TCB_STORAGE, r->dui_structure,
                RELSTAT, r->dui_status,
                RELSTAT2, r->dui_relstat2);
	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* MINPAGES: %d MAXPAGES: %d IFILL: %d DFILL: %d LFILL: %d\n",
		r->dui_min, r->dui_max, r->dui_ifill, 
                r->dui_dfill, r->dui_lfill);
	    TRformat(format_routine, 0, line_buf, l_line_buf,
		"%12* BUCKETS: %d  RELTUPS: %d\n", 
		r->dui_buckets, r->dui_reltups);
	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* Alloc: %d  Extend: %d  Page size: %d Type: %d  Name: (%d, %d)\n",
		r->dui_allocation, r->dui_extend, r->dui_page_size, r->dui_pg_type,
		r->dui_name_id, r->dui_name_gen);
	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%#.#{%16* LOCATION: %.#s\n%}", r->dui_loc_count,
		sizeof(DB_LOC_NAME), r->dui_location, DB_LOC_MAXNAME, 0);
	}
	break;

    case DM0LBPEXTEND:
	{
	    DM0L_BPEXTEND	*r = (DM0L_BPEXTEND*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d)\n",
		r->bp_tbl_id.db_tab_base, r->bp_tbl_id.db_tab_index);
	}
	break;

    case DM0LBCP:
	{
	    DM0L_BCP		*r = (DM0L_BCP *)h;
	    struct db		*dbp = 0;
	    struct tx		*txp = 0;
	    i4		i = 0;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TYPE: %w FLAG: %w\n",
		DM0L_BCP_TYPE, r->bcp_type, ",LAST,FIRST", r->bcp_flag);
	    if (r->bcp_type == CP_DB && r->bcp_count)
	    {
		dbp = r->bcp_subtype.type_db;
		for (i = 0; i < r->bcp_count; i++)
		{
		    TRformat(format_routine, 0, line_buf, l_line_buf,
		    	"%16* ID: %x (%.#s,%.#s) EXT_ID: %d, STATUS: %x\n%20* LOCATION: %.#s\n",
			dbp->db_id, sizeof(DB_DB_NAME), &dbp->db_name,
			sizeof(DB_DB_OWNER), &dbp->db_owner, dbp->db_ext_dbid, 
			dbp->db_status, dbp->db_l_root, &dbp->db_root);
		    dbp++;
		}
	    }
	    else if (r->bcp_type == CP_TX && r->bcp_count)
	    {
		txp = r->bcp_subtype.type_tx;
		for (i = 0; i < r->bcp_count; i++)
		{
		    TRformat(format_routine, 0, line_buf, l_line_buf,
		        "%16* TRAN_ID: %x%x DB: %x LK_ID: %x\n",
			txp->tx_tran.db_high_tran, txp->tx_tran.db_low_tran, 
			txp->tx_dbid, txp->tx_lkid);
		    TRformat(format_routine, 0, line_buf, l_line_buf,
			"%16* FIRST: <%d,%d,%d> LAST: <%d,%d,%d>\n",
			txp->tx_first.la_sequence, 
			txp->tx_first.la_block,
			txp->tx_first.la_offset,
			txp->tx_last.la_sequence,
			txp->tx_last.la_block,
			txp->tx_last.la_offset);
		    TRformat(format_routine, 0, line_buf, l_line_buf,
			"%16* STATUS: %x USERNAME: %~t\n", txp->tx_status,
			sizeof(txp->tx_user_name), &txp->tx_user_name);
		    txp++;
		}
	    }
	}
	break;

    case DM0LECP:
	{
	    DM0L_ECP		*r = (DM0L_ECP *)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* BCP LSN: (%x,%x)  BCP_LA: (%d,%d,%d)  BOF: (%d,%d,%d)\n",
		r->ecp_begin_lsn.lsn_high, 
		r->ecp_begin_lsn.lsn_low,
		r->ecp_begin_la.la_sequence,
		r->ecp_begin_la.la_block,
		r->ecp_begin_la.la_offset,
		r->ecp_bof.la_sequence,
		r->ecp_bof.la_block,
		r->ecp_bof.la_offset);
	}
	break;

    case DM0LSM1RENAME:
	{
	    DM0L_SM1_RENAME	*r = (DM0L_SM1_RENAME*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) PATH: %t\n",
		r->sm1_tbl_id.db_tab_base, r->sm1_tbl_id.db_tab_index,
		r->sm1_l_path, &r->sm1_path);
	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* OLD: %t TEMP: %t NEW: %t RENAME: %t\n",
		sizeof(r->sm1_oldname), &r->sm1_oldname,
		sizeof(r->sm1_tempname), &r->sm1_tempname,
		sizeof(r->sm1_newname), &r->sm1_newname,
		sizeof(r->sm1_rename), &r->sm1_rename);
	}
	break;

    case DM0LSM2CONFIG:
	{
	    DM0L_SM2_CONFIG	*r = (DM0L_SM2_CONFIG*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d)\n",
		r->sm2_tbl_id.db_tab_base, r->sm2_tbl_id.db_tab_index);
	}

	break;

    case DM0LSM0CLOSEPURGE:
	{
	    DM0L_SM0_CLOSEPURGE	*r = (DM0L_SM0_CLOSEPURGE*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d)\n",
		r->sm0_tbl_id.db_tab_base, r->sm0_tbl_id.db_tab_index);
	}
	break;

    case DM0LLOCATION:
	{
	    DM0L_LOCATION	*r = (DM0L_LOCATION *)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* NAME: %~t LOCATION: %t\n",
		sizeof(r->loc_name), &r->loc_name, r->loc_l_extent,
		&r->loc_extent);

	    if (verbose_flag)
		dmd_hexdump_header(format_routine, (char *)h, line_buf,
		    l_line_buf);
	}
	break;

    case DM0LEXTALTER:
	{
	    DM0L_EXT_ALTER	*r = (DM0L_EXT_ALTER *)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* LOCATION: %~t DROP_LOC: %d ADD_LOC: %d\n",
		sizeof(r->ext_lname), &r->ext_lname, r->ext_otype,
		r->ext_ntype);

	    if (verbose_flag)
		dmd_hexdump_header(format_routine, (char *)h, line_buf,
		    l_line_buf);
	}
	break;

    case DM0LSETBOF:
	{
	    DM0L_SETBOF		*r = (DM0L_SETBOF *)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* OLD BOF: <%d,%d,%d> NEW BOF: <%d,%d,%d>\n",
		r->sb_oldbof.la_sequence,
		r->sb_oldbof.la_block,
		r->sb_oldbof.la_offset,
		r->sb_newbof.la_sequence,
		r->sb_newbof.la_block,
		r->sb_newbof.la_offset);
	}
	break;

    case DM0LALTER:
	{
	    DM0L_ALTER	    *r = (DM0L_ALTER*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d, %d) NAME(%~t,%~t) : #actions %d, action %d\n",
		r->dua_tbl_id.db_tab_base, r->dua_tbl_id.db_tab_index,
		sizeof(r->dua_name), &r->dua_name, sizeof(r->dua_owner),
		&r->dua_owner, r->dua_count, r->dua_action);
	}
	break;

    case DM0LP1:
	{
	    DM0L_P1		*r = (DM0L_P1 *)h;
	    i4		i;

	    if (r->p1_count == 0 && r->p1_flag == P1_LAST)
	    {
		TRformat(dmd_put_partial_line, 0, line_buf, l_line_buf,
		    "%12* FLAG:%v TRAN_ID: %x%x ", "LAST", r->p1_flag,
		    r->p1_tran_id.db_high_tran, r->p1_tran_id.db_low_tran);

                FormatDisTranId(&r->p1_dis_tran_id);

		TRformat(format_routine, 0, line_buf, l_line_buf,
		    "%12* OWNER (%~t) FIRST_LA (%d,%d,%d) CP_LA (%d,%d,%d)\n",
		    sizeof(r->p1_name), &r->p1_name,
		    r->p1_first_lga.la_sequence,
		    r->p1_first_lga.la_block,
		    r->p1_first_lga.la_offset,
		    r->p1_cp_lga.la_sequence,
		    r->p1_cp_lga.la_block,
		    r->p1_cp_lga.la_offset);
	    }
	    else
	    {
		if (r->p1_flag == P1_RELATED)
		    TRformat(format_routine, 0, line_buf, l_line_buf,
		      	"Locks on the related lock list.\n");
		else
		    TRformat(format_routine, 0, line_buf, l_line_buf,
		    	"Locks on the transaction lock list.\n");
		for (i = 0; i < r->p1_count; i++)
		{
		    char	keybuf[256];

		    TRformat(dmd_put_partial_line, 0, line_buf, l_line_buf,
		    "%12* Gr: %3w Attribute: %4w ", 
		    LK_LOCK_MODE_MEANING,  r->p1_type_lk[i].lkb_grant_mode,
		    ",PHYS", r->p1_type_lk[i].lkb_attribute);

		    TRformat(format_routine, 0, line_buf, l_line_buf, " %s\n",
		    	     LKkey_to_string((LK_LOCK_KEY*)&r->p1_type_lk[i],
			     			keybuf));
		}
	    }
	}
	break;

    case DM0LDMU:
	{
	    DM0L_DMU	    *r = (DM0L_DMU*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d, %d) NAME(%~t,%~t) Operation: %d, flag 0x%x\n",
		r->dmu_tabid.db_tab_base, r->dmu_tabid.db_tab_index,
		sizeof(r->dmu_tblname), &r->dmu_tblname,
		sizeof(r->dmu_tblowner), &r->dmu_tblowner,
		r->dmu_operation, r->dmu_flag);
	}
	break;

    case DM0LASSOC:
        {
            DM0L_ASSOC      *r = (DM0L_ASSOC*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
            	"%12* TABLE: (%d, %d) Leaf:%d Old:%d New:%d\n",
		r->ass_tbl_id.db_tab_base, r->ass_tbl_id.db_tab_index, 
		r->ass_leaf_page, r->ass_old_data, r->ass_new_data);
        }
        break;

    case DM0LALLOC:
        {
            DM0L_ALLOC      *r = (DM0L_ALLOC*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) Page: %d\n",
		r->all_tblid.db_tab_base, r->all_tblid.db_tab_index, 
                r->all_free_pageno);
        }
        break;

    case DM0LDEALLOC:
        {
            DM0L_DEALLOC    *r = (DM0L_DEALLOC*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) Page: %d\n",
		r->dall_tblid.db_tab_base, r->dall_tblid.db_tab_index, 
                r->dall_free_pageno);
        }
        break;

    case DM0LEXTEND:
        {
            DM0L_EXTEND     *r = (DM0L_EXTEND *)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) Old_pages:%d New_pages:%d\n",
		r->ext_tblid.db_tab_base, r->ext_tblid.db_tab_index, 
                r->ext_old_pages, r->ext_new_pages);
        }
        break;

    case DM0LOVFL:
        {
            DM0L_OVFL      *r = (DM0L_OVFL*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) New:%d Parent:%d Ovfl:%d Main:%d\n",
		r->ovf_tbl_id.db_tab_base, r->ovf_tbl_id.db_tab_index, 
		r->ovf_newpage, r->ovf_page, r->ovf_ovfl_ptr, r->ovf_main_ptr); 
	}
        break;

    case DM0LNOFULL:
        {
            DM0L_NOFULL      *r = (DM0L_NOFULL*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	     	"%12* TABLE: (%d,%d) Page:%d\n",
		r->nofull_tbl_id.db_tab_base, r->nofull_tbl_id.db_tab_index, 
		r->nofull_pageno);
	}
        break;

    case DM0LFMAP:
        {
            DM0L_FMAP      *r = (DM0L_FMAP*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) Page:%d Index:%d\n",
		r->fmap_tblid.db_tab_base, r->fmap_tblid.db_tab_index, 
		r->fmap_fmap_pageno, r->fmap_map_index);
	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%16* FHDR page: %d First used:%d First free:%d Last used: %d\n",
		r->fmap_fhdr_pageno, r->fmap_first_used, r->fmap_first_free,
		r->fmap_last_free);
	}
        break;

    case DM0LUFMAP:
        {
            DM0L_UFMAP      *r = (DM0L_UFMAP*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) Page:%d Index:%d\n",
		r->fmap_tblid.db_tab_base, r->fmap_tblid.db_tab_index, 
		r->fmap_fmap_pageno, r->fmap_map_index);
	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%16* FHDR page: %d First used:%d Last used: %d\n",
		r->fmap_fhdr_pageno, r->fmap_first_used, r->fmap_last_used);
	}
        break;

    case DM0LBTPUT:
        {
            DM0L_BTPUT	    *r = (DM0L_BTPUT*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) Bid:(%d,%d) Tid:(%d,%d) Partno: %d\n",
		r->btp_tbl_id.db_tab_base, r->btp_tbl_id.db_tab_index, 
		r->btp_bid.tid_tid.tid_page, r->btp_bid.tid_tid.tid_line,
		r->btp_tid.tid_tid.tid_page, r->btp_tid.tid_tid.tid_line,
		r->btp_partno);

	    if (verbose_flag)
	    {
		rec_ptr = (PTR) (((char *)r) + sizeof(DM0L_BTPUT));
		dmd_hexdump_contents(format_routine, "    KEY", 7, rec_ptr, 
		    r->btp_key_size, line_buf);
	    }
	}
        break;

    case DM0LBTDEL:
        {
            DM0L_BTDEL	    *r = (DM0L_BTDEL*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) Bid:(%d,%d) Tid:(%d,%d), Partno: %d\n",
		r->btd_tbl_id.db_tab_base, r->btd_tbl_id.db_tab_index, 
		r->btd_bid.tid_tid.tid_page, r->btd_bid.tid_tid.tid_line,
		r->btd_tid.tid_tid.tid_page, r->btd_tid.tid_tid.tid_line,
		r->btd_partno);

	    if (verbose_flag)
	    {
		rec_ptr = (PTR) (((char *)r) + sizeof(DM0L_BTDEL));
		dmd_hexdump_contents(format_routine, "    KEY", 7, rec_ptr, 
		    r->btd_key_size, line_buf);
	    }
	}
        break;

    case DM0LBTSPLIT:
        {
            DM0L_BTSPLIT    *r = (DM0L_BTSPLIT*)h;
    	    char	    *desc_key;
	    
	    desc_key = (char *)&r->spl_vbuf + r->spl_page_size;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	      "%12* TABLE: (%d,%d) Old:%d New:%d Pos:%d Dir:%d Rlen:%d\n",
		r->spl_tbl_id.db_tab_base, r->spl_tbl_id.db_tab_index, 
		r->spl_cur_pageno, r->spl_sib_pageno, r->spl_split_pos,
		r->spl_split_dir, r->spl_range_klen); 

	    if (verbose_flag)
	    {
		dmd_hexdump_contents(format_routine, "    SPLIT PAGE", 12, 
		    (PTR)&r->spl_vbuf, r->spl_page_size, line_buf);
		dmd_hexdump_contents(format_routine, "    DESC KEY", 12, 
		    (PTR)desc_key, r->spl_desc_klen, 
		    line_buf);
	    }
	}
        break;

    case DM0LBTOVFL:
        {
            DM0L_BTOVFL    *r = (DM0L_BTOVFL*)h;
	
	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) Leaf:%d Ovfl:%d TidSize: %d\n",
		r->bto_tbl_id.db_tab_base, r->bto_tbl_id.db_tab_index, 
		r->bto_leaf_pageno, r->bto_ovfl_pageno,
		r->bto_tidsize); 

	    if (verbose_flag)
	    {
		if (r->bto_lrange_len)
		{
		    rec_ptr = (PTR)&r->bto_vbuf[0];
		    dmd_hexdump_contents(format_routine, "    LRANGE KEY", 14,  rec_ptr, 
			r->bto_lrange_len, line_buf);
		}
		if (r->bto_lrange_len)
		{
		    rec_ptr = (PTR)&r->bto_vbuf[r->bto_lrange_len];
		    dmd_hexdump_contents(format_routine, "    RRANGE KEY", 14, rec_ptr, 
			r->bto_rrange_len, line_buf);
		}
	    }
	}
        break;

    case DM0LBTFREE:
        {
            DM0L_BTFREE    *r = (DM0L_BTFREE*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) Leaf:%d Ovfl:%d Prev: %d Tidsize: %d\n",
		r->btf_tbl_id.db_tab_base, r->btf_tbl_id.db_tab_index, 
		r->btf_mainpage, r->btf_ovfl_pageno, r->btf_prev_pageno,
		r->btf_tidsize); 

	    if (verbose_flag)
	    {
		if (r->btf_dupkey_len)
		{
		    rec_ptr = (PTR)&r->btf_vbuf[0];
		    dmd_hexdump_contents(format_routine, "    DUP KEY", 11, rec_ptr, 
			r->btf_dupkey_len, line_buf);
		}
		if (r->btf_lrange_len)
		{
		    rec_ptr = (PTR)&r->btf_vbuf[r->btf_dupkey_len];
		    dmd_hexdump_contents(format_routine, "    LRANGE KEY", 14, rec_ptr, 
			r->btf_lrange_len, line_buf);
		}
		if (r->btf_rrange_len)
		{
		    rec_ptr = (PTR)&r->btf_vbuf[r->btf_dupkey_len +
				r->btf_lrange_len];
		    dmd_hexdump_contents(format_routine, "    RRANGE KEY", 14, rec_ptr, 
			r->btf_rrange_len, line_buf);
		}
	    }
	}
        break;

    case DM0LBTUPDOVFL:
        {
            DM0L_BTUPDOVFL	*r = (DM0L_BTUPDOVFL*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) Leaf:%d Ovfl:%d\n",
		r->btu_tbl_id.db_tab_base, r->btu_tbl_id.db_tab_index, 
		r->btu_mainpage, r->btu_pageno); 

	    if (verbose_flag)
	    {
		if (r->btu_lrange_len)
		{
		    rec_ptr = (PTR)&r->btu_vbuf[0];
		    dmd_hexdump_contents(format_routine, "    LRANGE KEY", 14, rec_ptr, 
			r->btu_lrange_len, line_buf);
		}
		if (r->btu_rrange_len)
		{
		    rec_ptr = (PTR)&r->btu_vbuf[r->btu_lrange_len];
		    dmd_hexdump_contents(format_routine, "    RRANGE KEY", 14, rec_ptr, 
			r->btu_rrange_len, line_buf);
		}
		if (r->btu_olrange_len)
		{
		    rec_ptr = (PTR)&r->btu_vbuf[r->btu_lrange_len + 
					r->btu_rrange_len];
		    dmd_hexdump_contents(format_routine, "    OLD RRANGE KEY", 18, 
			rec_ptr, r->btu_olrange_len, line_buf);
		}
		if (r->btu_orrange_len)
		{
		    rec_ptr = (PTR)&r->btu_vbuf[r->btu_lrange_len + 
			r->btu_rrange_len + r->btu_olrange_len];
		    dmd_hexdump_contents(format_routine, "    OLD LRANGE KEY", 18, 
			rec_ptr, r->btu_orrange_len, line_buf);
		}
	    }
	}
        break;

    case DM0LDISASSOC:
        {
            DM0L_DISASSOC	*r = (DM0L_DISASSOC*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) Page: %d\n",
		r->dis_tbl_id.db_tab_base, r->dis_tbl_id.db_tab_index, 
		r->dis_pageno); 
	}
        break;

    case DM0LRTDEL:
        {
            DM0L_RTDEL	    *r = (DM0L_RTDEL*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) Bid:(%d,%d) Tid:(%d,%d)\n",
		r->rtd_tbl_id.db_tab_base, r->rtd_tbl_id.db_tab_index, 
		r->rtd_bid.tid_tid.tid_page, r->rtd_bid.tid_tid.tid_line,
		r->rtd_tid.tid_tid.tid_page, r->rtd_tid.tid_tid.tid_line);

	    if (verbose_flag)
	    {
		rec_ptr = (PTR) (((char *)r) + sizeof(DM0L_RTDEL));
		dmd_hexdump_contents(format_routine, "    KEY", 7, rec_ptr, 
		    r->rtd_key_size, line_buf);
	    }
	}
        break;

    case DM0LRTPUT:
        {
            DM0L_RTPUT	    *r = (DM0L_RTPUT*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) Bid:(%d,%d) Tid:(%d,%d)\n",
		r->rtp_tbl_id.db_tab_base, r->rtp_tbl_id.db_tab_index, 
		r->rtp_bid.tid_tid.tid_page, r->rtp_bid.tid_tid.tid_line,
		r->rtp_tid.tid_tid.tid_page, r->rtp_tid.tid_tid.tid_line);

	    if (verbose_flag)
	    {
		rec_ptr = (PTR) (((char *)r) + sizeof(DM0L_RTPUT));
		dmd_hexdump_contents(format_routine, "    KEY", 7, rec_ptr, 
		    r->rtp_key_size, line_buf);
	    }
	}
        break;

    case DM0LRTREP:
        {
            DM0L_RTREP	    *r = (DM0L_RTREP*)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) Bid:(%d,%d) Tid:(%d,%d)\n",
		r->rtr_tbl_id.db_tab_base, r->rtr_tbl_id.db_tab_index, 
		r->rtr_bid.tid_tid.tid_page, r->rtr_bid.tid_tid.tid_line,
		r->rtr_tid.tid_tid.tid_page, r->rtr_tid.tid_tid.tid_line);

	    if (verbose_flag)
	    {
		rec_ptr = (PTR) (((char *)r) + sizeof(DM0L_RTREP));
		dmd_hexdump_contents(format_routine, "    OKEY", 8, rec_ptr, 
		    r->rtr_okey_size, line_buf);
		rec_ptr = rec_ptr + r->rtr_okey_size;
		dmd_hexdump_contents(format_routine, "    NKEY", 8, rec_ptr, 
		    r->rtr_nkey_size, line_buf);
	    }
	}
        break;

    case DM0LRAWDATA:
	{
	    DM0L_RAWDATA *r = (DM0L_RAWDATA *)h;

	    TRformat(format_routine, 0, line_buf, l_line_buf,
		"%12* Type: %w (%d)  Instance: %d  Total: %d  Offs: %d for %d\n",
		DM_RAWD_TYPE_MEANING, r->rawd_type, r->rawd_type,
		r->rawd_instance, r->rawd_total_size,
		r->rawd_offset, r->rawd_size);

	    if (verbose_flag)
		dmd_hexdump_contents(format_routine, "    RAW", 7,
			((PTR) r) + sizeof(DM0L_RAWDATA),
			r->rawd_size, line_buf);
	}
	break;

    case DM0LTEST:
	{
	    DM0L_TEST	    *r = (DM0L_TEST*)h;

	    if (verbose_flag)
		dmd_hexdump_header(format_routine, (char *)h, line_buf,
		    l_line_buf);

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TEST NUMBER: %d\n", r->tst_number);
	}
	break;

    case DM0LBSF:
	{
	    DM0L_BSF    *r = (DM0L_BSF *)h;

	    if (verbose_flag)
		dmd_hexdump_header(format_routine, (char *)h, line_buf,
		    l_line_buf);

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) NAME(%~t,%~t)\n",
		r->bsf_tbl_id.db_tab_base, r->bsf_tbl_id.db_tab_index,
		sizeof(r->bsf_name), &r->bsf_name, sizeof(r->bsf_owner),
		&r->bsf_owner);
	}
	break;

    case DM0LESF:
	{
	    DM0L_ESF    *r = (DM0L_ESF *)h;

	    if (verbose_flag)
		dmd_hexdump_header(format_routine, (char *)h, line_buf,
		    l_line_buf);

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* TABLE: (%d,%d) NAME(%~t,%~t)\n",
		r->esf_tbl_id.db_tab_base, r->esf_tbl_id.db_tab_index,
		sizeof(r->esf_name), &r->esf_name, sizeof(r->esf_owner),
		&r->esf_owner);
	}
	break;

    case DM0LRNLLSN:
        {
            DM0L_RNL_LSN    *r = (DM0L_RNL_LSN *)h;

            if (verbose_flag)
                dmd_hexdump_header(format_routine, (char *)h, line_buf,
                    l_line_buf);

            TRformat(format_routine, 0, line_buf, l_line_buf,
                "%12* TABLE: (%d,%d) NAME(%~t,%~t)\n",
                r->rl_tbl_id.db_tab_base, r->rl_tbl_id.db_tab_index,
                sizeof(r->rl_name), &r->rl_name, sizeof(r->rl_owner),
                &r->rl_owner);
        }
        break;

    case DM0LDELLOCATION:
	{
	    DM0L_DEL_LOCATION	*r = (DM0L_DEL_LOCATION *)h;

	    if (verbose_flag)
		dmd_hexdump_header(format_routine, (char *)h, line_buf,
		    l_line_buf);

	    TRformat(format_routine, 0, line_buf, l_line_buf,
	    	"%12* NAME: %~t LOCATION: %t\n",
		sizeof(r->loc_name), &r->loc_name, r->loc_l_extent,
		&r->loc_extent);

	    if (verbose_flag)
		dmd_hexdump_header(format_routine, (char *)h, line_buf,
		    l_line_buf);
	}
	break;

    case DM0LBTINFO:
	{
	    DM0L_BTINFO	    *r = (DM0L_BTINFO *)h;

            TRformat(format_routine, 0, line_buf, l_line_buf,
		"%12* USERNAME: %~t TIME: %?\n",
		sizeof(r->bti_bt.bt_name), &r->bti_bt.bt_name, 
		&r->bti_bt.bt_time);
	}
	break;

    case DM0LJNLSWITCH:
	{
	    DM0L_JNL_SWITCH	*r = (DM0L_JNL_SWITCH *)h;

	    break;
		
	}

    default:
	TRformat(format_routine, 0, line_buf, l_line_buf,
	    "ILLEGAL RECORD TYPE: %d", h->type);
	break;
    }	

    return;
}

/*{
** Name: dmd_format_header	- Hex dump of log record header
**
** Description:
**      This routine displays a log record header in hex.  
**
** Inputs:
**      record                          The log record header
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
**	10-feb-1993 (jnash)
**	    Created for the reduced logging project.  Should be 
**	    changed to include header offset if header increases
**	    in size beyond one line.
*/
static VOID
dmd_hexdump_header(
			i4 (*format_routine)(
			PTR	     arg,
			i4	     length,
			char         *buffer),
char                *h,
char		    *line_buf,
i4		    l_line_buf)
{
    DM0L_CRHEADER *CRhdr;

    i4 	len = sizeof(DM0L_HEADER);
    i4 	off = 0;

    while (len > 0)
    {
	TRformat(format_routine, 0, line_buf, l_line_buf,
	    "\tHEADER:   %< %9.4{%x %}%2<\n", h + off, 0);
	len -= 36;
	off += 36;
    }

    if ( ((DM0L_HEADER*)h)->flags & DM0L_CR_HEADER )
    {
        h = (char*)h + sizeof(DM0L_HEADER);

	len = sizeof(DM0L_CRHEADER);
	off = 0;

	while (len > 0)
	{
	    TRformat(format_routine, 0, line_buf, l_line_buf,
		"\tCRHEADER: %< %9.4{%x %}%2<\n", h + off, 0);
	    len -= 36;
	    off += 36;
	}
    }

    return;
}

/*{
** Name: dmd_hexdump_contents	- Hex dump of log record contents. 
**
** Description:
**      This routine outputs one component of the log record in hex.  
**	The output component may be a key, tuple, before-image, etc.
**
** Inputs:
**      record                          The log record.
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
**	10-feb-1993 (jnash)
**	     Created.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Responded to some lint suggestions.
**	22-nov-1993 (bryanp) B56479
**	    Removed a "%<" from a TRdisplay.
*/
static VOID
dmd_hexdump_contents(
			i4	(*format_routine)(
			PTR	     arg,
			i4	     length,
			char         *buffer),
char                *prefix,
i4		    prefix_len,
char                *record,
i4		    record_len,
char		    *line_buf)
{
    char		display_buf[136];
    char		tmp_buf[32 + sizeof(i4)];   /* XXXX */
    char		*aligned_buffer;
    i4		i;
    char		*l;

    /*
    ** Return immediately if zero length entity (probably a CLR).
    */
    if (record_len == 0)
	return;

    /*
    ** Convert record to hex.
    */
    l = line_buf;

    /*
    ** Add the prefix formatting.
    */
    MEcopy("    ", 4, l);
    MEcopy(prefix, prefix_len, l + 4);
    l += prefix_len + 4;

    /*
    ** Output the record header prefix.
    */

    /*
    ** Insert string end of line indication.
    */
    *l = 0;
    TRformat(format_routine, 0, display_buf, sizeof(display_buf), 
	"%s (length %d bytes):\n", line_buf, record_len);

    /*
    ** Output the record contents 32 bytes per output row.  
    ** TRformat requires aligned storage.
    */
    for (i = 0; i < record_len; i += 32)
    {
	aligned_buffer = (char *)ME_ALIGN_MACRO(tmp_buf, sizeof(i4));
	MEcopy(record + i, 32, aligned_buffer);
	TRformat(format_routine, 0, display_buf, sizeof(display_buf), 
  	    "\t[%4.1d]: %8.4{%x %}%2< >%32.1{%c%}<\n", i, aligned_buffer, 0);
    }

    return;
}

/*{
** Name: dmd_format_lg_hdr	- Output log record header info.
**
** Description:
**      This routine outputs log record header info.
**
** Inputs:
**      length                          Length of record
**	buffer				buffer length
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
**	10-feb-1993 (jnash)
**	    Created for the reduced logging project.  
*/
VOID
dmd_format_lg_hdr(
			i4	(*format_routine)(
			PTR	     arg,
			i4	     length,
			char         *buffer),
LG_LA		*lga,
LG_RECORD	*lgr,
i4		log_block_size)
{
    char	    display_buf[132];

    TRformat(format_routine, 0, display_buf, sizeof(display_buf),
	"\nLA=(%d,%d,%d),PREV_LA=(%d,%d,%d),LEN=%d,SEQ=%d\n", 
	lga->la_sequence, 
	lga->la_block,
	lga->la_offset,
	lgr->lgr_prev.la_sequence,
	lgr->lgr_prev.la_block,
	lgr->lgr_prev.la_offset,
	lgr->lgr_length,
	lgr->lgr_sequence);

    return;
}

/*{
** Name: dmd_put_line	- Output log record.
**
** Description:
**      This routine is passed as the function call argument to 
**	dmd_format_log().
**
** Inputs:
**      length                          Length of record
**	buffer				buffer length
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
**	10-feb-1993 (jnash)
**	     Created.
**      26-apr-1993 (bryanp)
**	    Removed unused 'count' variable.
*/
i4
dmd_put_line(
PTR		arg,
i4		length,
char            *buffer)
{
    TRdisplay("%t\n", length, buffer);
}

/*{
** Name: dmd_put_partial_line	- Output part of a log record.
**
** Description:
**      This routine is used as the function call argument to 
**	TRformat() when the caller is only formatting part of a line. The
**	difference between this routine and dmd_put_line is that this routine
**	doesn't automatically append a newline. Thus if you call TRformat
**	with this routine multiple times, you keep appending to the current
**	line.
**
** Inputs:
**      length                          Length of record
**	buffer				buffer length
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
**      26-jul-1993 (bryanp)
**	    Created.
*/
static i4
dmd_put_partial_line(
PTR		arg,
i4		length,
char            *buffer)
{
    TRdisplay("%t", length, buffer);
}

/*{
** Name: dmd_log_info	- TRdisplay log information.
**
** Description:
**	
**	Replaces the myriad of "logstat"-type reports on the
**	elements of the logging system.
**
** Inputs:
**      options                         That which to display
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
**	15-Mar-2006 (jenjo02)
**	    Written to consolidate the mess, extracted
**	    from logstat, dmfrecover.
**	    Added breakdown of log waits, additional LSN
**	    information in header, transactions, and buffers, 
**	    display all buffers when possible, not just first 20.
**	21-Mar-2006 (jenjo02)
**	    Add lgs_optimwrite, lgs_optimpages.
**	05-Sep-2006 (jonj)
**	    Add "1PC", "ILOG" tr_status bit interpretations.
**	11-Sep-2006 (jonj)
**	    Add logfull_commit waits, handle_wts, handle_ets.
**      23-Jun-2008 (hanal04) Bug 120529
**          Reformat the 'Logging System Summary' output so that we can
**          display values up to 10 digits.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add ldb_last_commit, ldb_jfib, ldb_first_la,
**	    ldb_last_lsn, to database display.
**	
*/
VOID
dmd_log_info(
i4		options)
{
    CL_ERR_DESC	    sys_err;
    LG_STAT	    stat;
    i4	    	    status;
    i4	    	    length;
    i4	    	    bufcnt;
    LG_HEADER	    header;
    LG_PROCESS	    lpb;
    LG_DATABASE	    ldb;
    LG_TRANSACTION  lxb;
    LG_LA	    acp_start;
    LG_LA	    acp_end;
    LG_LA	    acp_prevcp;
    i4	    	    lgd_status;
    i4	    	    lgd_csp_pid;
    i4	    	    active_log;
    i4	    	    pcnt_used, used_blocks;
    u_i8	    res_bytes;
#define DEFAULT_BUFCNT	20	/* 20 picked at random */
    LG_BUFFER	    bufs[DEFAULT_BUFCNT];
    LG_BUFFER	    *bufp;
    PTR		    bufmem;
    i4		    i;
    i4	    	    err_code;
    i4	    	    count_per_tick;
    LG_LA	    forced_lga;
    LG_LSN	    forced_lsn;

    /*  Get the log header, always. */

    status = LGshow(LG_A_HEADER, (PTR)&header, sizeof(header), 
		    &length, &sys_err);
    if (status)
    {
	uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		    NULL, 0, NULL, &err_code, 0);
	TRdisplay("Can't show logging header.\n");
	return;
    }

    if ( options & (DMLG_STATISTICS | DMLG_BUFFER_UTIL) )
    {
	status = LGshow(LG_S_STAT, (PTR)&stat, sizeof(stat), &length, &sys_err);

	if (status)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			NULL, 0, NULL, &err_code, 0);
	    TRdisplay("Can't show logging statistics.\n");
	    return;
	}
    }

    if ( options & DMLG_STATISTICS )
    {
	options &= ~DMLG_STATISTICS;

	TRdisplay("%22*=%@ Logging System Summary%11*=\n\n");
	TRdisplay("    Database add         %10d", stat.lgs_add);
	TRdisplay("%8* Database removes     %10d\n", stat.lgs_remove);
	TRdisplay("    Transaction begins   %10d", stat.lgs_begin);
	TRdisplay("%8* Transaction ends     %10d\n", stat.lgs_end);
	TRdisplay("    Log read i/o's       %10d", stat.lgs_readio);
	TRdisplay("%8* Log write i/o's      %10d\n", stat.lgs_writeio);
	TRdisplay("    Log writes           %10d", stat.lgs_write);
	TRdisplay("%8* Log forces           %10d\n", stat.lgs_force);
	/* Optimized writes only possible with single-partition log */
	if ( header.lgh_partitions == 1 )
	{
	    TRdisplay("    Log optimized writes %10d", stat.lgs_optimwrites);
	    TRdisplay("%8* Log optimized pages  %10d\n", stat.lgs_optimpages);
	}
	TRdisplay("    Log waits            %10d", stat.lgs_wait[LG_WAIT_ALL]);
	TRdisplay("%8* Log splits           %10d\n", stat.lgs_split);
	TRdisplay("    Log group commit     %10d", stat.lgs_group);
	TRdisplay("%8* Log group count      %10d\n", stat.lgs_group_count);
	TRdisplay("    Check commit timer   %10d", stat.lgs_check_timer);
	TRdisplay("%8* Timer write          %10d\n", stat.lgs_timer_write);
	TRdisplay("    Timer write, time    %10d", stat.lgs_timer_write_time);
	TRdisplay("%8* Timer write, idle    %10d\n", stat.lgs_timer_write_idle);
	TRdisplay("    Inconsistent db      %10d", stat.lgs_inconsist_db);
	TRdisplay("%8* Kbytes written       %10d\n", stat.lgs_kbytes / 2);
	TRdisplay("    ii_log_file read     %10d", stat.lgs_log_readio);
	TRdisplay("%8* ii_dual_log read     %10d\n", stat.lgs_dual_readio);
	TRdisplay("    write complete       %10d", stat.lgs_log_writeio);
	TRdisplay("%8* dual write complete  %10d\n", stat.lgs_dual_writeio);
	TRdisplay("    All logwriters busy  %10d", stat.lgs_no_logwriter);
	TRdisplay("%8* Max write queue len  %10d\n", stat.lgs_max_wq);
	TRdisplay("                         %10* ");
	TRdisplay("%8* Max write queue cnt  %10d\n", stat.lgs_max_wq_count);
	TRdisplay("    Log Waits By Type:\n");
	TRdisplay("      Force              %10d", stat.lgs_wait[LG_WAIT_FORCE]);
	TRdisplay("%8* Free Buffer          %10d\n", stat.lgs_wait[LG_WAIT_FREEBUF]);
	TRdisplay("      Split Buffer       %10d", stat.lgs_wait[LG_WAIT_SPLIT]);
	TRdisplay("%8* Log Header I/O       %10d\n", stat.lgs_wait[LG_WAIT_HDRIO]);
	TRdisplay("      Ckpdb Stall        %10d", stat.lgs_wait[LG_WAIT_CKPDB]);
	TRdisplay("%8* Opendb               %10d\n", stat.lgs_wait[LG_WAIT_OPENDB]);
	TRdisplay("      BCP Stall          %10d", stat.lgs_wait[LG_WAIT_BCP]);
	TRdisplay("%8* Logfull Stall        %10d\n", stat.lgs_wait[LG_WAIT_LOGFULL]);
	TRdisplay("      Lastbuf            %10d", stat.lgs_wait[LG_WAIT_LASTBUF]);
	TRdisplay("%8* Forced I/O           %10d\n", stat.lgs_wait[LG_WAIT_FORCED_IO]);
	TRdisplay("      Event              %10d", stat.lgs_wait[LG_WAIT_EVENT]);
	TRdisplay("%8* Mini Transaction     %10d\n", stat.lgs_wait[LG_WAIT_MINI]);
	TRdisplay("      Logfull Commit     %10d\n", stat.lgs_wait[LG_WAIT_COMMIT]);
    } /* DMLG_STATISTICS */

    if ( options & DMLG_BUFFER_UTIL )
    {
	options &= ~DMLG_BUFFER_UTIL;

	if ( (count_per_tick = stat.lgs_buf_util_sum / 69) > 0 )
	{
	    /*
	    ** Display buffer utilization profile.
	    **
	    ** Note that header write i/o's are not included in these
	    ** counts, so lgs_writeio won't be the same as lgs_buf_util_sum.
	    **
	    ** Each tick on the graph is 1/69 of the total  
	    ** (determined by the amount of white space remaining on each
	    **  line after the line header (    nn-nn% ))
	    */
	    TRdisplay("\n----Buffer utilization profile%50*-\n");
	    TRdisplay("      <10%% %#**\n",
		  stat.lgs_buf_util[0]
		  ? stat.lgs_buf_util[0] / count_per_tick
		    ? stat.lgs_buf_util[0] / count_per_tick
		    : 1
		  : 0 );
	    TRdisplay("    10-19%% %#**\n",
		  stat.lgs_buf_util[1]
		  ? stat.lgs_buf_util[1] / count_per_tick
		    ? stat.lgs_buf_util[1] / count_per_tick
		    : 1
		  : 0 );
	    TRdisplay("    20-29%% %#**\n",
		  stat.lgs_buf_util[2]
		  ? stat.lgs_buf_util[2] / count_per_tick
		    ? stat.lgs_buf_util[2] / count_per_tick
		    : 1
		  : 0 );
	    TRdisplay("    30-39%% %#**\n",
		  stat.lgs_buf_util[3]
		  ? stat.lgs_buf_util[3] / count_per_tick
		    ? stat.lgs_buf_util[3] / count_per_tick
		    : 1
		  : 0 );
	    TRdisplay("    40-49%% %#**\n",
		  stat.lgs_buf_util[4]
		  ? stat.lgs_buf_util[4] / count_per_tick
		    ? stat.lgs_buf_util[4] / count_per_tick
		    : 1
		  : 0 );
	    TRdisplay("    50-59%% %#**\n",
		  stat.lgs_buf_util[5]
		  ? stat.lgs_buf_util[5] / count_per_tick
		    ? stat.lgs_buf_util[5] / count_per_tick
		    : 1
		  : 0 );
	    TRdisplay("    60-69%% %#**\n",
		  stat.lgs_buf_util[6]
		  ? stat.lgs_buf_util[6] / count_per_tick
		    ? stat.lgs_buf_util[6] / count_per_tick
		    : 1
		  : 0 );
	    TRdisplay("    70-79%% %#**\n",
		  stat.lgs_buf_util[7]
		  ? stat.lgs_buf_util[7] / count_per_tick
		    ? stat.lgs_buf_util[7] / count_per_tick
		    : 1
		  : 0 );
	    TRdisplay("    80-89%% %#**\n",
		  stat.lgs_buf_util[8]
		  ? stat.lgs_buf_util[8] / count_per_tick
		    ? stat.lgs_buf_util[8] / count_per_tick
		    : 1
		  : 0 );
	    TRdisplay("      >90%% %#**\n",
		  stat.lgs_buf_util[9]
		  ? stat.lgs_buf_util[9] / count_per_tick
		    ? stat.lgs_buf_util[9] / count_per_tick
		    : 1
		  : 0 );
	}
    } /* DMLG_BUFFER_UTIL */

    /* If any other options, gather what we'll need */
    if ( options )
    {
	status = LGshow(LG_S_ACP_START, (PTR)&acp_start, sizeof(acp_start), 
			&length, &sys_err);
	if (status)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			NULL, 0, NULL, &err_code, 0);
	    TRdisplay("Can't show archive window start.\n");
	    return;
	}

	status = LGshow(LG_S_ACP_END, (PTR)&acp_end, sizeof(acp_end), 
			&length, &sys_err);
	if (status)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			NULL, 0, NULL, &err_code, 0);
	    TRdisplay("Can't show archive window end.\n");
	    return;
	}

	status = LGshow(LG_S_ACP_CP, (PTR)&acp_prevcp, sizeof(acp_prevcp),
			&length, &sys_err);
	if (status)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			NULL, 0, NULL, &err_code, 0);
	    TRdisplay("Can't show archive window prev CP.\n");
	    return;
	}

	status = LGshow(LG_S_FORCED_LGA, (PTR)&forced_lga, sizeof(forced_lga),
			&length, &sys_err);
	if (status)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			NULL, 0, NULL, &err_code, 0);
	    TRdisplay("Can't show forced LGA.\n");
	    return;
	}

	status = LGshow(LG_S_FORCED_LSN, (PTR)&forced_lsn, sizeof(forced_lsn),
			&length, &sys_err);
	if (status)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			NULL, 0, NULL, &err_code, 0);
	    TRdisplay("Can't show forced LSN.\n");
	    return;
	}

	status = LGshow(LG_S_LGSTS, (PTR)&lgd_status, sizeof(lgd_status), 
                        &length, &sys_err);
	if (status)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			NULL, 0, NULL, &err_code, 0);
	    TRdisplay("Can't show logging system status.\n");
	    return;
	}
	if ((lgd_status & LGD_ONLINE) == 0)
	{
	    TRdisplay("Logging system not ONLINE.\n");
	    return;
	}
	status = LGshow(LG_S_DUAL_LOGGING,
			(PTR)&active_log, sizeof(active_log), 
                        &length, &sys_err);
        if (status)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			NULL, 0, NULL, &err_code, 0);
	    TRdisplay("Can't show active log status.\n");
	    return;
	}

	/* Figure percentage of log file used. */

	status = LGshow(LG_A_RES_SPACE, (PTR)&res_bytes, 
			sizeof(res_bytes), &length, &sys_err);
	if (status)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			NULL, 0, NULL, &err_code, 0);
	}

	if (header.lgh_end.la_block < header.lgh_begin.la_block)
	{
	    used_blocks = 
		header.lgh_count + 1 - 
		(header.lgh_begin.la_block - header.lgh_end.la_block);
	}
	else
	{
	    used_blocks = header.lgh_end.la_block - 
			  header.lgh_begin.la_block + 1;
	}

	used_blocks+= (res_bytes / header.lgh_size) + 1;
	
	pcnt_used = used_blocks * 100 / header.lgh_count;

	status = LGshow(LG_A_BCNT, (PTR)&bufcnt, sizeof(bufcnt),
			&length, &sys_err);
	if (status)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			NULL, 0, NULL, &err_code, 0);
	}
	status = LGshow(LG_S_CSP_PID, (PTR)&lgd_csp_pid, sizeof(lgd_csp_pid), 
                        &length, &sys_err);
	if (status)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			NULL, 0, NULL, &err_code, 0);
	    TRdisplay("Can't show logging system CSP Process ID.\n");
	    return;
	}

	if ( options & DMLG_HEADER )
	{
	    TRdisplay("\n----Current log file header%53*-\n");
	    TRdisplay("    Block size: %d    Block count: %d    Partitions: %d    Buffer count: %d\n",
		header.lgh_size, header.lgh_count, header.lgh_partitions, bufcnt);
	    TRdisplay("    CP interval: %d   Logfull interval: %d   Abort interval: %d\n",  
		header.lgh_l_cp, header.lgh_l_logfull, header.lgh_l_abort);
	    TRdisplay("    Last Transaction Id: %x%x   Last LSN: <%x,%x>\n",
		header.lgh_tran_id.db_high_tran, header.lgh_tran_id.db_low_tran,
		header.lgh_last_lsn.lsn_high, header.lgh_last_lsn.lsn_low);
	    TRdisplay("    Begin: <%d:%d:%d>    CP: <%d:%d:%d>    End: <%d:%d:%d>\n",
		header.lgh_begin.la_sequence, 
		header.lgh_begin.la_block,
		header.lgh_begin.la_offset,
		header.lgh_cp.la_sequence, header.lgh_cp.la_block,
		header.lgh_cp.la_offset,
		header.lgh_end.la_sequence, header.lgh_end.la_block,
		header.lgh_end.la_offset);
	    TRdisplay("    Forced LGA,LSN: <%d,%d,%d>,<%x,%x>\n",
		forced_lga.la_sequence,
		forced_lga.la_block,
		forced_lga.la_offset,
		forced_lsn.lsn_high,
		forced_lsn.lsn_low);
	    TRdisplay("    Percentage of log file in use or reserved: %d\n", 
		pcnt_used);
	    TRdisplay("    Log file blocks reserved by recovery system: %u\n", 
		(u_i4) (res_bytes / header.lgh_size));
	    TRdisplay("    Archive Window: <%d,%d,%d>..<%d,%d,%d>\n",
		acp_start.la_sequence, acp_start.la_block,
		acp_start.la_offset,
		acp_end.la_sequence,
		acp_end.la_block,
		acp_end.la_offset);
	    TRdisplay("    Previous CP:    <%d,%d,%d>\n",
		acp_prevcp.la_sequence,
		acp_prevcp.la_block,
		acp_prevcp.la_offset);
	    TRdisplay("    Status:         %v\n",
                LGD_STATES, lgd_status);
	    TRdisplay("    Active Log(s):         %v\n",
			"II_LOG_FILE,II_DUAL_LOG", active_log);
	    if (lgd_csp_pid)
		TRdisplay("    CSP Process ID: %x\n", lgd_csp_pid);

	} /* DMLG_HEADER */

	if ( options & DMLG_PROCESSES )
	{
	    /*  Display active processes. */

	    length = 0;
	    TRdisplay("\n----List of active processes%52*-\n\
    ID           PID   TYPE      OPEN_DB   WRITE   FORCE    WAIT   BEGIN     END\n\
%80*-\n");

	    do
	    {
		status = LGshow(LG_N_PROCESS, (PTR)&lpb, sizeof(lpb), &length,
				&sys_err);
		if (status)
		{
		    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
				NULL, 0, NULL, &err_code, 0);
		    TRdisplay("Can't show process information.\n");
		    return;
		}
		if (length)
		{
#ifdef VMS		/* only VMS really likes to print PID's in hex */
		    TRdisplay("    %8x  %8x %v %6.4{%8d%}\n",
#else
		    TRdisplay("    %8x  %8d %8v %6.4{%8d%}\n",
#endif
		    lpb.pr_id, lpb.pr_pid, 
                       PR_TYPE, lpb.pr_type, &lpb.pr_stat, 0);
		}
	    } while ( length );

	} /* DMTR_PROCESSES */

	if ( options & DMLG_DATABASES )
	{
	    /*  Display open databases. */
	    
	    length = 0;
	    TRdisplay("\n----List of active databases%52*-");
	    do
	    {
		status = LGshow(LG_N_DATABASE, (PTR)&ldb, sizeof(ldb), &length,
				&sys_err);
		if (status)
		{
		    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
				NULL, 0, NULL, &err_code, 0);
		    TRdisplay("Can't show database information.\n");
		    return;
		}
		if (length)
		{
		    TRdisplay("\n    Id: %8x    Database: (%~t,%~t)    Status: %v\n\
%8* Tx_cnt: %d  Begin: %d End: %d Read: %d Write: %d Force: %d Wait: %d\n\
%8* Location:       %64.#s\n\
%8* Journal Window: <%d,%d,%d>..<%d,%d,%d>\n\
%8* Start Backup Location: <%d,%d,%d> (%x,%x)\n",
		    ldb.db_id, DB_DB_MAXNAME, &ldb.db_buffer[0], 
		    DB_OWN_MAXNAME, &ldb.db_buffer[DB_DB_MAXNAME],
                    DB_STATUS_MEANING, ldb.db_status,
		    ldb.db_stat.trans, ldb.db_stat.begin, ldb.db_stat.end,
		    ldb.db_stat.read, ldb.db_stat.write,
		    ldb.db_stat.force, ldb.db_stat.wait,
		    *(i4 *)&ldb.db_buffer[DB_DB_MAXNAME+DB_OWN_MAXNAME+4], 
		    &ldb.db_buffer[DB_DB_MAXNAME+DB_OWN_MAXNAME+8],
		    ldb.db_f_jnl_la.la_sequence, 
		    ldb.db_f_jnl_la.la_block,
		    ldb.db_f_jnl_la.la_offset,
		    ldb.db_l_jnl_la.la_sequence, 
		    ldb.db_l_jnl_la.la_block,
		    ldb.db_l_jnl_la.la_offset,
		    ldb.db_sbackup.la_sequence, 
		    ldb.db_sbackup.la_block,
		    ldb.db_sbackup.la_offset,
		    ldb.db_sback_lsn.lsn_high, ldb.db_sback_lsn.lsn_low);

		    /* Show current MVCC jnl info, if pertinent */
		    if ( ldb.db_status & DB_MVCC )
		    {
		        TRdisplay("\
%8* First LGA: <%d,%d,%d> Last Commit LSN: (%x,%x) Last LSN: (%x,%x) IdLow: %d IdHigh: %d\n\
%8*   Jnl JFA: (%d,%d) Ckpseq: %d Bksz: %d Maxcnt: %d Next Byte: %d Bytes Left: %d\n",
			ldb.db_first_la.la_sequence,
			ldb.db_first_la.la_block,
			ldb.db_first_la.la_offset,
			ldb.db_last_commit.lsn_high, ldb.db_last_commit.lsn_low,
			ldb.db_last_lsn.lsn_high, ldb.db_last_lsn.lsn_low,
			ldb.db_lgid_low, ldb.db_lgid_high,
			ldb.db_jfib.jfib_jfa.jfa_filseq,
			ldb.db_jfib.jfib_jfa.jfa_block,
			ldb.db_jfib.jfib_ckpseq,
			ldb.db_jfib.jfib_bksz,
			ldb.db_jfib.jfib_maxcnt,
			ldb.db_jfib.jfib_next_byte,
			ldb.db_jfib.jfib_bytes_left);
		    }
		}
	    } while ( length );
	} /* DMLG_DATABASES */

	if ( options &
	    (DMLG_TRANSACTIONS | DMLG_USER_TRANS | DMLG_SPECIAL_TRANS) )
	{

	    /*  Display transactions. */

	    length = 0;
	    TRdisplay("\n----List of transactions%56*-");

	    do
	    {
		status = LGshow(LG_N_TRANSACTION, (PTR)&lxb, sizeof(lxb), &length,
				&sys_err);
		if (status)
		{
		    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
				NULL, 0, NULL, &err_code, 0);
		    TRdisplay("Can't show transaction information.\n");
		    return;
		}
		if ( length )
		{
		    if ( options & DMLG_USER_TRANS &&
			(lxb.tr_status & TR_PROTECT) == 0)
			continue;
		    if ( options & DMLG_SPECIAL_TRANS &&
			(lxb.tr_status & TR_PROTECT) != 0)
			continue;

		    TRdisplay("\n    Tx_id: %x    Tran_id: %x%x    Database: %x\n\
%8* Process: %x    ",
			lxb.tr_id, lxb.tr_eid.db_high_tran, lxb.tr_eid.db_low_tran,
			lxb.tr_db_id, lxb.tr_pr_id);

		    FormatDisTranId(&lxb.tr_dis_id);

		    TRdisplay("Session: %p\n\
%8* First: <%d,%d,%d>    Last: <%d,%d,%d>    Cp: <%d,%d,%d>\n\
%8* FirstLSN: <%x,%x>   LastLSN: <%x,%x>   WaitLSN: <%x,%x>\n\
%8* Write: %d  Split: %d  Force: %d  Wait: %d  Reserved: %u\n\
%8* WaitBuf: %d  Status: %v",
			lxb.tr_sess_id,
			lxb.tr_first.la_sequence,
			lxb.tr_first.la_block,
			lxb.tr_first.la_offset,
			lxb.tr_last.la_sequence, 
			lxb.tr_last.la_block,
			lxb.tr_last.la_offset,
			lxb.tr_cp.la_sequence,
			lxb.tr_cp.la_block,
			lxb.tr_cp.la_offset,
			lxb.tr_first_lsn.lsn_high,
			lxb.tr_first_lsn.lsn_low,
			lxb.tr_last_lsn.lsn_high,
			lxb.tr_last_lsn.lsn_low,
			lxb.tr_wait_lsn.lsn_high,
			lxb.tr_wait_lsn.lsn_low,
			lxb.tr_stat.write, lxb.tr_stat.split, 
			lxb.tr_stat.force, lxb.tr_stat.wait, 
			(u_i4) (lxb.tr_reserved_space / header.lgh_size),
			lxb.tr_wait_buf,
			TR_STATUS_MEANING, lxb.tr_status);

		    if ( lxb.tr_lock_id )
			TRdisplay("  LKid: %x",
			    lxb.tr_lock_id);
		    if ( lxb.tr_status & TR_SHARED )
			TRdisplay("\n%8* Handles: %d  Prepares: %d  Writers: %d  ETs: %d",
			    lxb.tr_handle_count,
			    lxb.tr_handle_preps,
			    lxb.tr_handle_wts,
			    lxb.tr_handle_ets);
		    else if ( lxb.tr_status & TR_SHARED_HANDLE )
			TRdisplay("\n%8* Shared_Id: %x",
			    lxb.tr_shared_id);
			
			TRdisplay("\n%8* Wait Reason: %w\n%8* User: <%~t>\n",
			    LG_WAIT_REASON, lxb.tr_wait_reason,
			    lxb.tr_l_user_name, &lxb.tr_user_name[0]); 
		}
	    } while ( length );
	} /* DMLG_TRANSACTIONS */

	if ( options & DMLG_BUFFERS )
	{
	    /*  Display log buffer status */

	    /* Get temp memory for all buffers if possible */
	    bufp = &bufs[0];
	    bufmem = NULL; 

	    if ( bufcnt > DEFAULT_BUFCNT )
	    {
		bufmem = MEreqmem(0, sizeof(LG_BUFFER) * bufcnt, 
				    FALSE, &status);
		if ( status )
		{
		    bufmem = NULL;
		    bufcnt = DEFAULT_BUFCNT;
		}
		else
		    bufp = (LG_BUFFER*)bufmem;
	    }

	    length = 0;
	    TRdisplay("\n---- List of free log pages %52*-\n");

	    status = LGshow(LG_S_FBUFFER, (PTR)bufp, 
				bufcnt * sizeof(LG_BUFFER), &length,
				&sys_err);
	    if (status)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			    NULL, 0, NULL, &err_code, 0);
		TRdisplay("Can't show buffer information.\n");
		if ( bufmem )
		    MEfree(bufmem);
		return;
	    }
	    TRdisplay("       (There are %d free log pages)\n",
			    (length / sizeof(LG_BUFFER)));
	    FormatBufs(bufp, length);

	    length = 0;
	    TRdisplay("\n---- List of write queue pages %49*-\n");

	    status = LGshow(LG_S_WBUFFER, (PTR)bufp,
				bufcnt * sizeof(LG_BUFFER), &length,
				&sys_err);
	    if (status)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			    NULL, 0, NULL, &err_code, 0);
		TRdisplay("Can't show buffer information.\n");
		if ( bufmem )
		    MEfree(bufmem);
		return;
	    }
	    TRdisplay("       (There are %d pages on the write queue)\n",
			    (length / sizeof(LG_BUFFER)));

	    FormatBufs(bufp, length);

	    status = LGshow(LG_S_CBUFFER, (PTR)bufp,
				    bufcnt * sizeof(LG_BUFFER), &length,
				    &sys_err);
	    if (status)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			    NULL, 0, NULL, &err_code, 0);
		TRdisplay("Can't show buffer information.\n");
		if ( bufmem )
		    MEfree(bufmem);
		return;
	    }
	    TRdisplay("\n---- Current Buffer Information %48*-\n");

	    FormatBufs(bufp, length);

	    TRdisplay("     timer_lbb=%d,last_used=%d,total_ticks=%d,maxticks=%d\n",
			bufp->buf_timer_lbb, bufp->buf_last_used,
			bufp->buf_total_ticks, bufp->buf_tick_max);

	    TRdisplay("%80*=\n");

	    if ( bufmem )
		MEfree(bufmem);
	} /* DMLG_BUFFERS */
    } /* if ( options ) */

    return;
}

/*
** History:
**      15-Mar-2006 (jenjo02)
**          Added num_writers, num_commit, FirstLSN, LastLSN,
**	    ForceLSN to display.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add buf_id.
*/
static VOID
FormatBufs(LG_BUFFER	*bufs, i4 length)
{
    i4		i;

    for (i = 0; i < (length / sizeof(LG_BUFFER)); i++)
    {
	TRdisplay("Id: %8d State: (%x) %v\n",
		    bufs[i].buf_id,
		    bufs[i].buf_state,
                   LBB_STATE, bufs[i].buf_state);
	TRdisplay("    buffer is at %d; next is %d, prev is %d\n",
		    bufs[i].buf_offset, bufs[i].buf_next_offset,
		    bufs[i].buf_prev_offset);
	TRdisplay(
	    "    nextbyte:%d bytesused:%d num_waiters:%d num_writers:%d num_commit:%d Resume:%d\n",
		    bufs[i].buf_next_byte, bufs[i].buf_bytes_used,
		    bufs[i].buf_n_waiters, 
		    bufs[i].buf_n_writers, bufs[i].buf_n_commit,
		    bufs[i].buf_resume_cnt);
	TRdisplay("    BufLGA:<%d,%d,%d> ForcedLGA:<%d,%d,%d>\n",
		    bufs[i].buf_lga.la_sequence,
		    bufs[i].buf_lga.la_block,
		    bufs[i].buf_lga.la_offset,
		    bufs[i].buf_forced_lga.la_sequence,
		    bufs[i].buf_forced_lga.la_block,
		    bufs[i].buf_forced_lga.la_offset);
	TRdisplay("    FirstLSN:<%x,%x> LastLSN:<%x,%x> ForcedLSN:<%x,%x>\n",
		    bufs[i].buf_first_lsn.lsn_high,
		    bufs[i].buf_first_lsn.lsn_low,
		    bufs[i].buf_last_lsn.lsn_high,
		    bufs[i].buf_last_lsn.lsn_low,
		    bufs[i].buf_forced_lsn.lsn_high,
		    bufs[i].buf_forced_lsn.lsn_low);
	TRdisplay(
	"    Owners: asgnd=%x prim=%x (task=%d), dual=%x (task=%d)\n",
		    bufs[i].buf_assigned_owner,
		    bufs[i].buf_prim_owner, bufs[i].buf_prim_task,
		    bufs[i].buf_dual_owner, bufs[i].buf_dual_task);
    }

    return;
}
/*
** Name: FormatDisTranId     - display distributed transaction id.
**
** Description:
**	This routine formats DB_DIS_TRAN_ID information using TRdisplay.
**
** Inputs:
**	    dis_tran_id			Distributed transaction id.
**
** Outputs:
**	    None
**	Returns:
**	    VOID
**
** History:
**	1-oct-1992 (nandak)
**	    Created as part of sharing the transaction type.
**	26-jul-1993 (bryanp)
**	    Fix formatting problems in format_dis_tran_id for XA tran types.
**      23-sep-1993 (iyer)
**          Change qualifiers for all original XA tran ID fields since they
**          are now part of another structure. In addition format and print
**          the branch_seqnum and branch_flag of the extended XA distributed
**          transaction ID.
*/
static VOID
FormatDisTranId(DB_DIS_TRAN_ID *dis_tran_id)
{
    i4         gtrid_length;
    i4         bqual_length;
    i4	       longword_count;
    i4         i=0;


    if (dis_tran_id->db_dis_tran_id_type == DB_INGRES_DIS_TRAN_ID_TYPE)
    {
	TRdisplay("Dis_tran_id: <Ingres,%d,%d> ",
  dis_tran_id->db_dis_tran_id.db_ingres_dis_tran_id.db_tran_id.db_high_tran,
  dis_tran_id->db_dis_tran_id.db_ingres_dis_tran_id.db_tran_id.db_low_tran);
    }
    else if (dis_tran_id->db_dis_tran_id_type == DB_XA_DIS_TRAN_ID_TYPE)
    {
	gtrid_length =
	  dis_tran_id->db_dis_tran_id.db_xa_extd_dis_tran_id.
	    db_xa_dis_tran_id.gtrid_length;
	bqual_length =
	  dis_tran_id->db_dis_tran_id.db_xa_extd_dis_tran_id.
	    db_xa_dis_tran_id.bqual_length;

	TRdisplay("Dis_tran_id: <XA,%d,%d,%d,",
		   dis_tran_id->db_dis_tran_id.db_xa_extd_dis_tran_id.
		   db_xa_dis_tran_id.formatID,
		   gtrid_length, bqual_length);

	/*
	** Format the remainder of the xa tran id in hexadecimal. If the tran
	** id is not exactly a multiple of sizeof(i4) in length, then
	** we print some garbage at the end, so exercise caution when reading
	** the output...
	*/
	longword_count = (gtrid_length + bqual_length + sizeof(i4) - 1) /
			    sizeof(i4);
	for (i = 0; i < longword_count; i++)
	    TRdisplay("%x",
		*(i4 *)
                &dis_tran_id->db_dis_tran_id.db_xa_extd_dis_tran_id.
		      db_xa_dis_tran_id.data [i*sizeof(i4)]);
	TRdisplay("> ");

	TRdisplay(" <EXTD,%d,%d> ",
	     dis_tran_id->db_dis_tran_id.db_xa_extd_dis_tran_id.branch_seqnum,
	     dis_tran_id->db_dis_tran_id.db_xa_extd_dis_tran_id.branch_flag);
        
    }
    else if (dis_tran_id->db_dis_tran_id_type == 0)
	TRdisplay("Dis_tran_id: <0,0> ");
    else
	TRdisplay("Dis_tran_id: <UNKNOWN> ");

    return;
}
