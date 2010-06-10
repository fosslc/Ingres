/*
** Copyright (c) 1986, 2005 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <tm.h>
#include    <cm.h>
#include    <cs.h>
#include    <di.h>
#include    <id.h>
#include    <jf.h>
#include    <si.h>
#include    <cp.h>
#include    <lo.h>
#include    <pc.h>
#include    <er.h>
#include    <tm.h>
#include    <me.h>
#include    <st.h>
#include    <tr.h>
#include    <ex.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dudbms.h>
#include    <lk.h>
#include    <cx.h>
#include    <lg.h>
#include    <adf.h>
#include    <adp.h>
#include    <ulf.h>
#include    <sxf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dma.h>
#include    <dm1c.h>
#include    <dm1r.h>
#include    <dm2d.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dm2r.h>
#include    <dm0c.h>
#include    <dmfjsp.h>
#include    <dm0m.h>
#include    <dm0j.h>
#include    <dmd.h>
#include    <dmfinit.h>
#include    <dmucb.h>
#include    <dmpepcb.h>
#include    <dmpecpn.h>

/*
**
**  Name: DMFATP.C - DMF Audit Trail Processor.
**
**  Description:
**	Function prototypes defined in DMFJSP.H.
**
**      This file contains the routines that manage reading journal files
**	and writing audit trail information for the user.
**
**	During normal AUDITDB operation, the journal file is read twice.  
**	The "prescan phase" gathers information about aborted and abort-to-
**	savepoint transactions.  The "output phase" outputs formatted audit 
**	information after applying user-specified filtering, 
**	ensuring that journal records are not associated with an
**	aborted or abort-to-savepoint transaction. 
**
**	The prescan phase is bypassed if the -all option is specified with
**	no begin/end time (-b/-e).  Time checks impose a prescan 
**	phase as a result of the use of ET -- not BT -- timestamps;
**	that is, the ET times are gathered during the prescan phase
**	and used during the output phase.
**
**	Table name and owner criteria are not supported for AUDITDB -all 
**	output; name and owner information is only resident in 
**	certain log records.
**
**          dmfatp - Audit trail producer
**
**	(Internal)
**
**	    complete - Complete audit processing.
**	    display_record - Display record to be audited.
**	    atp_prepare - Prepare to audit.
**	    atp_process_record - Process a journal record.
**	    write_line - Write line to report.
**	    write_tuple - Write tuple to report.
**	    preprocess_record - catalog record during prescan phase.
**	    check_tab_own - Check table ownership and name.
**	    check_abort - Qualify record during output phase by checking
**	      abort list.
**	    move_tx_to_atx - Move incomplete transactions to abort list.
**	    add_abortsave - Catalog abortsave log record.
**	    add_abort - Catalog abort operation.
**	    lookup_atx - Search ATX list.
**	    add_atx - Add ATX element to list.
**	    atp_put_line - Display output line.
**	    atp_lobj_cpn_redeem - Redeem a large object coupon.
**	    atp_lobj_copyout - Copy out a large object.
**
**  History:
**      17-nov-1986 (Derek)
**           Created for Jupiter.
**	17-apr-1989 (jrb)
**	    Initialized precision field(s) correctly.
**      21-apr-89 (fred)
**          Altered so that adg_startup() is not called (very poorly, I might
**	    add).  Adg_startup() had already been called by dmf_init(), so
**	    this routine will  now call dmf_getadf() (new routine) to obtain
**	    the standing adf session control block.
**      14-jul-89 (fred)
**	    Altered write_tuple() so that it will correctly pass the potential
**	    length of the output string to adc_cvinto().  Instead of the amount
**	    of space left, it was passing the amount of space used so far...
**	 2-oct-89 (rogerk)
**	    Added handling of DM0L_FSWAP log records.
**	 27-Nov-89 (ac)
**	    Fixed several journal files merging problems.
**	26-dec-89 (greg)
**	    write_tuple() -- use I2ASSIGN_MACRO
**	17-may-90 (rogerk)
**	    Added fixes for byte_align problems in write_tuple.  Added
**	    checks for overflowing the display buffer and added routine
**	    expand_displaybuf.  Allocated tuple buffers from dm0m rather
**	    than on the stack.
**      19-nov-90 (rogerk)
**          Initialized timezone field in the adf control block.
**          This was done as part of the DBMS Timezone changes.
**       4-feb-91 (rogerk)
**          Bypass server cache locking during auditdb since we don't read
**          any data that is actually protected by the cache lock.  Bypassing
**          cache lock allows us to run while database is locked by Fast
**          Commit server.  Pass DM2D_IGNORE_CACHE_PROTOCOL to dm2d_open_db
**          and dm2d_close_db calls.
**      25-feb-1991 (rogerk)
**          Add check for auditdb on journal_disabled database.  Give
**          a warning message.
**      16-jul-1991 (bryanp)
**          B38527: Add support for DM0L_OLD_MODIFY & DM0L_OLD_INDEX.
**      30-aug-1991 (rogerk)
**          Fix A in audit of RELOCATE log records.  The TRdisplay arguments
**          id not match the format string.  Broke up the old location/new
**          location information onto two lines to make sure we can't overflow
**          the display buffer.
**      16-aug-1991 (jnash) merged 24-sep-1990 (rogerk)
**          Merge B1 changes.  Include byte_align fixes
**      16-aug-1991 (jnash) merged 24-sep-1990 (rogerk)
**          Merged 6.3 changes into 6.5.
**      23-oct-1991 (jnash)
**          Correct LINT detected dm0a_write parameter list problems.
**          Fix LINT detected problem with incorect initialization of "h".
**	20-dec-1991 (bryanp) integrated: 6-sep-1991 (markg)
**	    Fixed bug in prepare() where begin and end date were being 
**	    processed incorrectly. Bug 39150. 
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedef and fixed a cast to quiesce the ANSI 
**	    C compiler warnings.
**	18-feb-1992 (bryanp)
**	    Fix some portability problems noticed by Scott Garfinkle:
**	    1) include <lg.h>
**	    2) correct TM_STAMP usages.
**	    3) don't dereference or do arithmetic on PTR types.
**	7-july-1992 (ananth)
**	    Prototyping DMF.
**	25-aug_1992 (ananth)
**	    Get rid of HP compiler warnings.
**      24-sep-1992 (stevet)
**          Replaced reference to _timezone with _tzcb to support new
**          timezone data structure.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Added support for variable length
**	    table and owner names.
**	17-nov-1992 (robf)
** 	    Updated security auditing to fully SXF.
**	20-nov-1992 (jnash) 
**	    Reduced Logging Project: Remove FSWAP log records.  
**	14-dec-1992 (rogerk) 
**	    Include dm1b.h before dm0l.h for prototype definitions.  
**	21-dec-1992 (robf)
**	    Error E_DM1200_ATP_DB_NOT_JNL, indicating database isn't
**	    journaled so couldn't be audited was getting lost. Make sure
**	    this is logged rather than silently exiting.
**	13-jan-1993 (jnash)
**	    Reduced logging project.  Eliminate DM0L_SPUT, DM0L_SDEL
**	    and DM0L_SREP support.
**	08-feb-93 (jnash)
**	    Major changes imposed by the reduced logging project.
**	      - Introduce journal file prescan phase.  The prescan is
**		necessary to eliminate tx abort and abort-to-savepoint 
**	   	records from AUDITDB output, and also to check End
**		Transaction timestamps.  The prescan is always used on 
**		normal (non -all) AUDITDB output, and also on AUDITDB -ALL
**		if -b/-e are also specified.  Introduces new abort 
**		transaction (ATX) and abort savepoint lists (ABS).  
**		AUDITDB now manages three lists:
**		  1) a TX list of active transactions cataloged at BT time, 
**		     This list remains as it previously defined, and is 
**		     fixed in size to 256 elements.
**	          2) an ATX list of aborted transactions,
**	 	  3) an ABS list of abort-to-savepoint entries, queued off ATX.
**	      - Introduce support for "AUDITDB -all" (unsupported) 
**		to output all journal data.  Shows up here as JSX_AUDIT_ALL,
**		bypasses prescan phase if -b or -e not specified.
**	      - Introduce support for "AUDITDB -all -verbose" 
**		which outputs additional tuple/key information  -verbose
**		useful only in combination with AUDITDB -all.
**	      - Don't bother to read journal records beyond -e time. 
**	      - For -b/-e handling, use timestamps cataloged in End 
**		Transaction log records rather than begin transaction (BT) 
**	  	logs.  Prior use of BT timestamps was incorrect, as 
**		updates cannot be assured to exist until the transaction 
**		commits.  This imposes a prescan if either -b or -e specified.
**	15-mar-1993 (rogerk)
**	    Added direction parameter to dm0j_open routine.
**	16-mar-1993 (rmuth)
**	    Include di.h
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		dmfatp.c no longer  has its own journal merging routine.
**		    Instead, it calls dm0j_merge to merge the records, and
**		    dm0j_merge calls back to auditdb to process each record.
**		Initialize the DMF_ATP with zeros at startup.
**		Include dmfinit.h to pick up function prototypes.
**	26-apr-1993 (jnash)
**	    Add support for auditing multiple tables.
**	       Up to 64 tables/files can be specified as:
**	           auditdb -table=t1,t2,..., -file=f1,f2,...
**	       If "-file" is specified without list of filenames, .trl filenames
**	       of the format "table_name.trl" are created.
**	    Change #a to -all, #v to -verbose (actually in dmfjsp.c).
**	    Inhibit output of INDEX records in non -all case.  This makes 
**		default output format more like before (although there are
**		still differences from 6.4).
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	24-may-1993 (andys)
**	    Pass appropriate logfile blocksize to dmd_log and dmd_format_log
**	    rather than using hardwired 4096.
**	21-jun-93 (jnash)
**	    If dm2d_open_db() fails, try again with force flag.  This allows
**	    AUDITDB to open most inconsistent databases.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (jnash)
**	    - Add 'auditdb -inconsistent' to open inconsistent dbs. 
**	      This replaces default action implemented by 21-jun change.
**	    - Implement 'auditdb -wait' to wait for the archiver
**	      to archive up to the current eof before starting the audit.
**	      Auditdb now attaches to the logging system and may signal 
**	      archive events.
**	    - Ignore ET records written by the RCP to mark a database 
**	      inconsistent.  These are not real ET markers.
**          - Change references of db_f_jnl_cp to db_f_jnl_la.
**	20-sep-1993 (bryanp)
**	    Fixed some bugs in cluster journal file handling.
**	20-sep-1993 (andys)
**	    Fix arguments to ule_format call.
**	20-sep-1993 (jnash)
**	    - Add delimited id support.  This includes applying proper 
**	      case to user and table names.
**      18-oct-1993 (jnash)
**	    Add support for replace log record row compression.  Unpack
**	    new rows.  Hexdump old and new rows if ascii output 
**	    requested and if old row compressed.  Fail if binary output 
**	    requested (-file specified) and old row compressed.
**	18-oct-93 (jrb)
**          Added support for DM0LEXTALTER.
**	11-dec-1993 (rogerk)
**	    Changed replace log record compression algorithms.  Changes made
**	    to the row_unpack routine calls in display_record.
**	31-jan-1994 (jnash)
**	    Fix bug where E_DB_INFO returned from dmfatp() if -e passed.
**	    This is not an error condition.  Also, delete tx entry from 
**	    list prior to other ET maintenance, otherwise it can be left 
**	    unlinked.
**	21-feb-1994 (jnash)
**	  - Add support for -start_lsn and -end_lsn, which check
**	    for a specific range of lsn's.
**	  - Eliminate prescan phase for auditdb -all when -b/-e used.  
**	    In retro, this was not necessary.  Also, we now use 
**	    use both BT and ET timestamps for auditdb -all -b/-e output.
**	    This is not consistent with rollforwarddb, but does permit
**	    a greater degree of resolution.
**	  - Add auditdb #c support, allowing specification of a 
**	    specific checkpoint from which to start output.  Unlike
**	    rollforwarddb, auditdb #c requires specification of a
**	    checkpoint sequence number.
**	  - Change process_record() to atp_process_record() to make it 
**	    unique.
**	23-feb-1994 (jnash)
**	    Fix compile problem detected by VMS compiler.
**	9-mar-94 (stephenb)
**	    Update dma_write_audit() calls to new prototype.
**      14-mar-94 (stephenb)
**          Add new parameter to dma_write_audit() to match current prototype.
**	23-may-1994 (andys)
**	    Make first argument to preprocess_record a PTR to quiesce 
**	    lint/qac warnings.
**	22-feb-1995 (rudtr01)
**	    Fix bug 66913. Print out the BT and ET records for
**	    a normal audit, so the user can plan a rollforwarddb.
**	    This information used to be available in 6.4, but was
**	    somehow omitted during the reduced-logging project.
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**	 6-dec-1995 (nick)
**	    Output to a file would generate incomplete 'repnew' lines causing
**	    the subsequent load into a relation to fail at this point. #72440
**	 8-jan-1996 (nick)
**	    Audit tuple row length was being calculated incorrectly leading
**	    to the audit trail file being opened with a too small record 
**	    size on VMS. #73736
**	25-feb-1996 (nick)
**	    Compressed tuples got written to audit trail files without
**	    being uncompressed first. #74796
**	 4-mar-1996 (nick)
**	    '-i' qualifier wasn't working for BT and ET records. #75151	
**	    '-b/-e' qualifiers weren't working for BT and ET records. #75032
**	29-mar-96 (nick)
**	    Correct confusion between DB_MAXNAME and DB_TAB_NAME in 
**	    check_tab_name().
**	06-mar-1996 (stial01 for bryanp)
**	    Call dm0m_tballoc to allocate tuple buffers.
**      06-mar-1996 (stial01)
**          atp_prepare() alloc hash queue ptrs correctly
**	 4-jul-96 (nick)
**	    Ensure we initialise the rcb lock modes on core catalogs queried
**	    in here. #77557
**      12-dec-1996 (cohmi01)
**          Add detailed err messages for corrupt system catalogs (Bug 79763).
**	21-Jul-1997 (carsu07)
**	    When using the -e flat with auditdb included a comparison between 
**          the specified -e time and the transaction begin time. (Bug 83448)
**      12-Aug-1997 (carsu07)
**          Added a check for the -e flag before comparing timestamps. (#84419)
**      15-jul-1996 (ramra01 for bryanp)
**          Build new DB_ATTS values for new iiattribute fields.
**	13-sep-1996 (canor01)
**	    Add NULL tuple buffer to dmpp_uncompress call.
**      10-mar-1997 (stial01)
**          Pass size of tuple buffer to dm0m_tballoc
**      28-aug-1997 (hanch04)
**          added tm.h
**      02-Sep-1997 (carsu07)
**	    Updated the ATP_ABS structure so that abs_l_reserved and abs_owner
**	    are now type PTR and not type i4.  This is to ensure that
**	    the structures ATP_ABS and DM_OBJECT correspond when
**	    dm0m_deallocate() is called from add_abort(). (Bug 83641)
**	    Also corrected the arguments in the TMcmp_stamp function call.
**	20-feb-1998(angusm)
**	    Extend size of image field in _ATP_TUP structure to accomodate 
**	    tuples on larger page sizes (bug 89050)
**	21-April-1998(angusm)
**	    Old bug 37246: auditdb with -t flag to extract journal
**	    information for specific table also needs -u flag,
**	    else table ownership is not properly resolved and
**	    'table not found/not exists" errors are reported.
**      08-Dec-1997 (hanch04)
**          Use new la_block field when block number is needed instead
**          of calculating it from the offset
**          Keep track of block number and offset into the current block.
**          for support of logs > 2 gig
**	21-Aug-1998 (kitch01)
**		Bug 92590. Amend write_tuple to correctly convert decimal
**		fields to character format.
**      21-apr-1999 (hanch04)
**          Replace STindex with STchr
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**      04-may-1999 (hanch04)
**          Change TRformat's print function to pass a PTR not an i4.
**	29-jul-1999 (i4jo01)
**	    Fixes entered for bug 95311 (variable page size tracing segv).
**	27-aug-1999 (i4jo01)
**	    Increase number of simultaneous tx tracked. (b98560).
**	15-Dec-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LGbegin() prototype.
**	18-jan-2000 (gupsh01)
**	    Added code for flags -abort_transactions and -internal_data for
**	    auditdb, and also for indicating whether a transaction modifies
**	    a nonjournaled table.
**      26-jan-2000 (gupsh01)
**          Added code for flags -no_header and -limit_output for
**          auditdb.
**      28-feb-2000 (gupsh01)
**          Added support for specifying table names in owner_name.table_name
**          format for auditdb.
**      09-mar-2000 (gupsh01)
**          Added support for displaying the transaction status with 
**          Option -internal_data, for the ascii output of auditdb.
**      02-may-2000 (gupsh01)
**	    For visual analyser project 
**          1) Added support for displaying the abortsave transactions with the
**             Option -aborted_transaction.
**          2) Extended the support for -start_lsn and -end_lsn to non -all cases
**          3) Added support for directing the output to the -file option to the 
**             standard output when -file=- is supplied 
**          4) Change the order of the output for the lsn_low and lsn_high in the
**             Ascii output of auditdb when using -internal_data Option
**      15-sep-2000 (gupsh01)
**          Added a new function format_emptyTuple(), to format an empty tuple for
**          ET and BT records which are printed out to a file, when using the
**          -aborted_transactions flag.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      09-nov-2000 (gupsh01)
**          Added additional check for -aborted_transactions when the -b and -e
**          options are also provided.
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to dma_write_audit() if not C2SECURE.
**	23-mar-2001 (somsa01)
**	    Enabled the processing of large object columns within a table.
**	    Also, removed unused variables.
**      01-may-2001 (stial01)
**          Init adf_ucollation
**	23-jul-2001 (somsa01)
**	    In atp_lobj_cpn_redeem() and atp_lobj_copyout(), to prevent
**	    SIGBUS errors, use MEcopy() to grab the DMPE_RECORD info into
**	    the local variable. Also, added the passing of the offset
**	    separately to atp_lobj_copyout().
**      04-apr-2002 (horda03) Bug 107181.
**          Allow User interrupts. Prior to this change, users could not
**          interrupt an AUDITDB.
**	26-May-2002 (hanje04)
**	    Replaced longnats brought in by X-Integ with i4's.
**	08-Oct-2002 (hanch04)
**	    Replace the i4 for length with a SIZE_TYPE (size_t) to allow
**	    64-bit memory allocation.
**      26-nov-2002 (stial01)
**          Used logged compression type instead of current compression type
**      27-sep-2002 (ashco01) Bug 108815.
**          Do not print LOCATION title or value for CREATE transactions
**          when object does not have a location count.
**	24-jun-2003 (gupsh01) Bug 110431
**	    Fixed various problems related to -b/-e flags.  
**	    i) auditdb -b/-e skipped transactions for non -all cases which 
**	    begins before the specified -b time but is committed after
**	    the begin time.
**	    ii) The abortsave end transaction records are always printed 
**	    even if the -aborted_transactions flag is not specified.
**	    iii) In the -all cases, the -b and -e flags do not scan all the 
**	    records within a given interval, but only the begin and the end 
**	    transaction records. (Bug 110431)
**	30-may-2001 (devjo01) s103715.
**	    Add cx.h, id.h, and fixed up a cast for cleanup.
**	03-jun-2002 (devjo01)
**	    Replace LGcluster() with CXcluster_enabled().  Fix problem
**	    with auditing a regular table with clustering, but exposed
**	    a stack corruption bug if a table containing a blob is 
**	    audited.  Temporarilly suppressing that feature.
**	03-Sep-2003 (hanal04) Bug 110838 INGSRV 2500
**	    Prevent SIGSEGV in write_tuple() when we do not have a
**	    tuple descriptor.
**      10-sep-2003 (horda03) Bug 110889/INGSRV 2521
**          Ensure that any exceptions have been handled prior to checking
**          for dmfatp_interrupt. On Windows, need to invoke EXinterrupt()
**          in order for the exception handling function to be executed
**          following a user interrupt.
**      12-jan-2004 (huazh01)
**          Change ATP_ATX_HASH from 255 to 20011 for performance
**          improvement. 
**          INGSRV2598, bug 111304.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	16-dec-04 (inkdo01)
**	    Added various collID's.
**      04-Apr-2005 (chopr01) Bug113295  INGSRV3013
**          Added check for row_version changes to DM0L_PUT, DM0L_REP 
**          and DM0L_DEL records in atp_uncompress(). This requires the 
**          journal record pointer to be added as a prameter.
**      22-jul-2005 (horda03) Bug 114902/INGSRV3359
**          Fix for bug 114155 didn't add pool_type to the ATP_ABS and ATP_ATX
**          objects. To avoid a similar event in future, changed these
**          structure to use DM_OBJECT (as all the structures should).
**	06-May-2005 (jenjo02)
**	    Rewrote blob copy/redeem functions to also work with clustering
**	    such that the copy/redeem stuff will also work when driven
**	    as dm0j_merge() callbacks.
**	16-aug-2005 (abbjo03)
**	    If a table has blob columns, open the audit trail file in SI_VAR
**	    mode with 8192 length records.
**      07-mar-2006 (stial01)
**          check_abort() Since we no longer write SAVEPOINT records to log,
**          need to fix test for beginning of savepoint window. (b115681)
**	06-Nov-2006 (kiria01) b117042
**	    Conform CM macro calls to relaxed bool form
**      27-Nov-2006 (hweho01)
**          Initialized the pointer timestamp to NULL in atp_process_record(),
**          avoid SIGBUS in auditdb.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
**      30-Jan-2003 (hanal04) Bug 109530 INGSRV 2077
**          Added check for row_version changes to DM0L_PUT and
**          DM0L_DEL records in write_tuple(). This requires the 
**          journal record pointer to be added as a prameter.
**       1-Jul-2003 (hanal04) Bug 110505 INGSRV 2399
**          In write_tuple change the output format back to non-nullable
**          long text to avoid printing characters in place of nulls.
**	14-Feb-2008 (thaju02)
**	    For auditdb -file -table, write out data for columns that
**	    follow a lobj column. (B119967)
**	27-Feb-2008 (kschendel) SIR 122739
**	    Add rowaccess struct to table-data, remove old style rowaccess.
**	    Needs a certain amount of hopping around because the journal
**	    record compression might be different from the current table
**	    compression!
**      28-Feb-2008 (thaju02)
**          Following a load, auditdb reports E_DM1205 on a mini-xaction.
**          Display emini record.(B119971)
**    11-Apr-2008 (kschendel)
**        dm2r position call updated, fix here.
**        Add missing parameter to dm1r-cvt-row calls. (!)
**	30-May-2008 (jonj)
**	    LSNs are 2 u_i4's; display them in hex rather than
**	    signed decimal.
**      14-may-2008 (stial01)
**          Ignore DM0LJNLSWITCH log records
**	12-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	18-Nov-2008 (jonj)
**	    SIR 120874: dm0c_?, dm0j_? dm2d_? functions converted to DB_ERROR *dberr.
**	21-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2t_?, dm2r_? functions converted to DB_ERROR *
**	08-Dec-2008 (jonj)
**	    SIR 120874: dma_? auditing functions converted to DB_ERROR *
**      09-jul-2009 (stial01)
**          Format hex LSN with 0x (b122286)
**	19-Nov-2009 (kschendel) SIR 122890
**	    uleFormat doesn't necessarily return a null terminated string;
**	    the returned string has to be null terminated by hand.
**	16-Mar-2010 (troal01)
**	    Added dmf_get_srs FEXI.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

/*
**  References to global variables.
*/

static      ADF_CB      adf_cb;                 /*  ADF control block. */
static      i4		dmfatp_interrupt = 0;

/*
**  Forward type references.
*/

typedef struct _DMF_ATP	DMF_ATP;	/* Audit context. */
typedef struct _ATP_TX	ATP_TX;		/* Transaction hash array. */
typedef struct _ATP_ATX	ATP_ATX;	/* Abort Transaction hash array. */
typedef struct _ATP_TD	ATP_TD;		/* Table descriptions. */
typedef struct _ATP_ABS	ATP_ABS;	/* Abort to savepoint description. */
typedef struct _ATP_QUEUE ATP_QUEUE;	/* Queue f/b links */
typedef struct _ATP_HDR ATP_HDR;	/* Audit tuple header */
typedef struct _ATP_TUP ATP_TUP;	/* Audit trail record */
typedef struct _ATP_LOBJ_SEG ATP_LOBJ_SEG;

typedef struct _ATP_B_TX ATP_B_TX;	/* Transaction to be audited with -b flag */

typedef struct _ATP_JNL_NO_TIME ATP_JNL_NO_TIME; /* List of Journal numbers where 
                                                 ** no time record found
                                                 */


/*
** Constant definitions.
*/

/*
** ATP_ROW_TRUNCATED_MESSAGE:  When a row is being formatted for output in
** an audit trail, and the row exceeds the display buffer's memory, then
** we truncate the row and append this truncated message.
*/
#define ATP_ROW_TRUNCATED_MESSAGE   "> ... ROW TRUNCATED"

/*
** Maximum output buffer size - this is the longest size row we will display
** in an audit trail.  This value was picked somewhat arbitrarily in order
** to stop runaway memory usage and can be incremented if any users reach
** the limit.
*/
#define	ATP_MAX_DISPLAYBUF	    8192
#define ATP_MAX_LINESIZE	    80

static i4	current_journal_num;	/* The current journal file number. */
static i4	latest_jnl;		/* The last journal sequence. */

/*}
** Name: ATP_QUEUE - ATP queue definition.
**
** Description:
** 	This structure defines an ATP state queue.
**
** History:
**	08-Feb-1993 (jnash)
**	    Created for the reduced logging project.
*/
struct _ATP_QUEUE
{
    ATP_QUEUE		*stq_next;		/*  Next queue entry. */
    ATP_QUEUE		*stq_prev;		/*  Previous queue entry. */
};


/*}
** Name: ATP_HDR - ATP audit trail header
**
** Description:
** 	This structure defines an ATP audit trail tuple header
**
** History:
**	08-jan-1996 (nick)
**	    Created.
*/
struct _ATP_HDR
{
    char		date[12];	/*  Date of operation. */
    DB_OWN_NAME		username;	/*  Username performing change. */
    char		operation[8];	/*  Operation. */
    DB_TRAN_ID		tran_id;	/*  Transaction identifier. */
    i4		table_base;	/*  Table identifier. */
    i4		table_index;
};
# define	ATP_HDR_SIZE	(sizeof(ATP_HDR))

/*}
** Name: ATP_TUP - ATP audit trail tuple
**
** Description:
** 	This structure defines an ATP audit trail tuple
**
** History:
**	08-jan-1996 (nick)
**	    Created.
**	20-feb-1998(angusm)
**	    Extend size of image field in _ATP_TUP structure to accomodate 
**	    tuples on larger page sizes (bug 89050)
*/
struct _ATP_TUP
{
    ATP_HDR	audit_hdr;
    char	image[DM_TUPLEN_MAX_V5];
};

struct _ATP_LOBJ_SEG
{
    char	lobj_seg[DMPE_CPS_COMPONENT_PART_SIZE];
};

/*
** Name: DMF_ATP - Audit context.
**
** Description:
**      This structure contains information needed by the audit routines.
**
** History:
**      17-nov-1986 (Derek)
**          Created for Jupiter.
**	 7-may-1990 (rogerk)
**	    Added atp_rowbuf, atp_alignbuf, and atp_displaybuf.
**	08-feb-93 (jnash)
**	    Reduced logging project.  Add atp_atx_state, atp_atxh.
**	26-jul-93 (rogerk)
**	    Added support for compressed log rows in log records.  Added
**		atp_uncompbuf, used to uncompress rows before using adc to
**		convert them to displayable types.
**	    Added compression and row_width fields to table descriptor struct.
**	    Changed attribute descriptors dramatically.  Instead of just
**		storing type and length info for each attribute, we now carry
**		around a full DB_ATTS array - which is needed by the uncompress
**		routines.  Changed the attribute array to be allocated
**		dynamically and be variable length to prevent the TD cache
**		from expanding from 64K to 1M.
**	    Increased the maximum number of Table Descriptors from 64 to 128.
**	18-jan-2000 (gupsh01)
**	    Added the array for base table size to support -abort_transactions.	
**      15-sep-2000 (gupsh01)
**          Added the array for index table to support -aborted_transactions.
**      12-jan-2004 (huazh01)
**         change the hash table size 'ATP_ATX_HASH' from 255 to 20011.
**         INGSRV2598, b111304.
**      24-mar-2004 (horda03) Bug 101206/INGSRV2769
**          Added atp_begin_jnl, which specifies the journal file at which
**          to begin scanning, This may be different to the first journal
**          file associated to a checkpoint.
[@history_template@]...
*/
struct _DMF_ATP
{
    DMP_DCB         *atp_dcb;		/* Pointer to DCB for database. */
    DMF_JSX	    *atp_jsx;		/* Pointer to journal context. */
    i4	    	    atp_f_jnl;		/* First journal file. */
    i4	    	    atp_l_jnl;		/* Last journal file. */
    i4	    	    atp_block_size;	/* Journal block size. */
    i4	    	    atp_lineno;		/* Current listing line number. */
    i4	    	    atp_pageno;		/* Current listing page number. */
    char	    atp_date[32];	/* Date of report. */
    CPFILE	    atp_auditfile[JSX_MAX_TBLS];  /* CP stream array. */
    i4		    atp_base_id[JSX_MAX_TBLS];    /* Table id array */
    i4              atp_index_id[JSX_MAX_TBLS];    /* Table index array */
    i4		    atp_base_size[JSX_MAX_TBLS];    /* Table size array */
    char	    *atp_rowbuf;	/* Buffer used for converting columns */
    char	    *atp_alignbuf;	/* Buffer used on byteswap machines */
    char	    *atp_uncompbuf;	/* Buffer used for uncompressing rows */
    char	    *atp_displaybuf;	/* Buffer used for display lines */
    i4	    	    atp_sdisplaybuf;	/* Size of the display buffer */
    ATP_TX	    *atp_tx_free;	/* Free TX list. */
#define			ATP_TXH_CNT	1024
#define			ATP_TX_CNT	2048
#define			ATP_TD_CNT	1024
#define                 ATP_B_TX_CNT	1024
    struct _ATP_TX			/* Transaction description. */
    {
	ATP_TX	    *tx_next;		/* Next transaction. */
	ATP_TX	    *tx_prev;           /* Previous transaction. */
	DB_OWN_NAME tx_username;	/* Username. */
	DB_TRAN_ID  tx_tran_id;		/* Transaction id. */
	DB_TAB_TIMESTAMP
		    tx_timestamp;	/* Transaction begin time. */
	i4	    tx_flag;
#define			TX_MINI		0x01
    }		    *atp_txh[ATP_TXH_CNT];
    ATP_TX	    atp_tx[ATP_TX_CNT];
    struct _ATP_B_TX
    {
        ATP_B_TX    *b_tx_next;
        ATP_B_TX    *b_tx_prev;
        ATP_B_TX    *b_tx_missing_bt;
        DB_TRAN_ID  b_tx_tran_id;
    }               *atp_btime_txs [ATP_B_TX_CNT];
    ATP_B_TX        *atp_btime_open_et; /* List of ET txs where BT not read */
    struct _ATP_TD			/* Table description. */
    {
	DB_TAB_ID   table_id;
	/* The current state of the table may have a different compression
	** type than what the log record says.  Hence, get-table-data
	** allocates a maximum size (across all compression types) control
	** array and stores it separately.  The row-accessor is "set up"
	** for whatever compression type is in rac.compression_type.
	*/
	DMP_ROWACCESS data_rac;		/* Data-row accessor */
	PTR	    cmp_control;	/* Compression-control array base */
	i4	    control_size;	/* Size in bytes of array */
	u_i4	    relstat2;
	i4	    row_width;
	i4	    lobj_att_count;
	i4	    row_version;	
	DB_ATTS	    *att_array;
    }		    atp_td[ATP_TD_CNT];
    ATP_ATX	    **atp_atxh;		/* Array of hash buckets for
					** aborted transactions. */
#define		    ATP_ATX_HASH    20011	
    ATP_QUEUE 	    atp_atx_state; 	/* Abort TX state queue. */
    i4              atp_begin_jnl;      /* Journal file to begin scan */

    /* Stuff needed by Large Object copy/redeem functions */
    DB_TAB_ID	    *lobj_base_tabid;
    DB_TRAN_ID	    *lobj_tran_id;
    DMPE_COUPON	    *lobj_coupon;
    DB_DATA_VALUE   *lobj_dvunder;
    DB_DATA_VALUE   *lobj_dvinto;
    ATP_LOBJ_SEG    *lobj_segarray;
    i4		    lobj_segindex; 
    u_i4	    lobj_segment1;
    i4		    lobj_found_start;
    i4		    lobj_reptype;
    FILE	    **lobj_fdesc;

};

/*}
** Name: ATP_ATX	- ATP abort list.
**
** Description:
**      This structure defines the list of transactions that are NOT
**	of interest during the output phase.  Built during the prescan phase, 
**	an element is added when a aborted transaction is encountered, or when 
**	abort-to savepoint is found, or when a transaction outside the 
**	specified -b/-e interval is found.  The list remains empty if no 
**	prescan takes place.
**
** History:
**     08-feb-93 (jnash)
**          Created for the reduced logging project.
**	15-Aug-2005 (jenjo02)
**	    Use DM0L types defined in dm.h rather than picking a
**	    number out of the air.
**     26-jun-2009 (huazh01 for thaju02)
**          add ATX_OPENXACT flag. (b122177)
*/
struct _ATP_ATX	
{
    DM_OBJECT       atx_dm_object;
#define                 ATX_ASCII_ID	  CV_C_CONST_MACRO('A', 'T', 'X', 'C')
    ATP_QUEUE       atx_state;		    /* Next ATX on queue */
    i4	    atx_status;
#define 		ATX_ABORTSAVE	  0x1  /* Abortsave in transaction */
#define 		ATX_ABORT	  0x2  /* Transaction aborted */
#define 		ATX_OPENXACT	  0x4  /* Transaction aborted */
    DB_TRAN_ID      atx_tran_id;	    /* Transaction id. */
    ATP_QUEUE       atx_abs_state;	    /* Abortsave queue */
    i4	    atx_abs_count;	    /* Count of tx on abortsave q */
};

/*}
** Name: ATP_ABS	- ATP abort-to-savepoint window information.
**
** Description:
**      This structure defines the list of abort-to-savepoint windows
**	found during the prescan phase.  The list remains empty if no prescan
**	takes place.
**
** History:
**      08-feb-93 (jnash)
**          Created for the reduced logging project.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**      02-Sep-1997 (carsu07)
**          Updated the ATP_ABS structure so that abs_l_reserved and abs_owner
**          are now type PTR and not type i4.  This is to ensure that
**          the structures ATP_ABS and DM_OBJECT correspond when
**          dm0m_deallocate() is called from add_abort(). (Bug 83641)
**	15-Aug-2005 (jenjo02)
**	    Use DM0L types defined in dm.h rather than picking a
**	    number out of the air.
*/
struct _ATP_ABS
{
    DM_OBJECT       abs_dm_object;
#define                 ABS_ASCII_ID	  CV_C_CONST_MACRO('A', 'B', 'S', 'C')
    ATP_QUEUE       abs_state;		    /* Next ABS on queue */
    i4	            abs_sp_id;	  	    /* Savepoint id */
    LG_LSN	    abs_start_lsn;	    /* Savepoint start lsn */
    LG_LSN	    abs_end_lsn;	    /* Savepoint end lsn */
};

struct _ATP_JNL_NO_TIME
{
   ATP_JNL_NO_TIME *next;
   i4              jnl;
};
/*
**  Forward function references.
*/

static STATUS ex_handler(
    EX_ARGS     *ex_args);

static DB_STATUS add_atx(
    DMF_ATP  	*atp,
    DB_TRAN_ID	*tran_id,
    i4	atx_status,
    ATP_ATX	**ret_atx);

static i4 atp_put_line(
    PTR		arg,
    i4		length,
    char	*buffer);

static DB_STATUS atp_archive_wait(
    DMF_JSX	 *jsx,
    DMP_DCB	 *dcb);

static DB_STATUS add_abortsave(
    DMF_ATP	*atp,
    PTR		record);

static DB_STATUS add_abort(
    DMF_ATP	*atp,
    DB_TRAN_ID	*tran_id);

static DB_STATUS add_btime(
    DMF_ATP	*atp,
    DB_TRAN_ID	*tran_id,
    i4		missing_bt);

static STATUS	atp_prepare(
    DMF_ATP     *atp,
    DMF_JSX	*jsx,
    DMP_DCB	*dcb);

static DB_STATUS build_trl_name(
    char	    *tbl_name,
    char 	    *trl_filename);

static bool check_abort(
    DMF_ATP	*atp,
    PTR		record);

static bool table_in_list(
    DMF_ATP	*atp,
    i4	tbl_id);

static bool check_tab_own(
    DMF_ATP	*atp,
    PTR	 	record);

static bool check_tab_name(
    DMF_ATP  	*atp,
    char 	*tbl_name,
    i4		tbl_len,
    char        *own_name,
    i4		own_len,
    bool	is_partition);

static bool check_username(
    DMF_ATP	*atp,
    ATP_TX	*tx);

static STATUS	complete(
    DMF_ATP	*atp);

static DB_STATUS move_tx_to_atx(
    DMF_ATP	*atp);

static STATUS	display_record(
    DMF_ATP	*atp,
    PTR		record,
    ATP_TX	*tx);

static DB_STATUS expand_displaybuf(
    DMF_ATP	    *atp,
    i4	    bytes_needed,
    i4	    bytes_used);

static i4 get_tab_index(
    DMF_ATP	*atp,
    i4	tbl_id);

static VOID lookup_atx(
    DMF_ATP	    *atp,
    DB_TRAN_ID	    *tran_id,
    ATP_ATX	    **return_atx); 

static DB_STATUS preprocess_record(
    PTR		atp,
    PTR		record,
    i4		*err_code);

static DB_STATUS atp_process_record(
    PTR		atp,
    char	*record,
    i4		*err_code);

static VOID	write_line(
    DMF_ATP	*atp,
    char	*line,
    i4	l_line);

static VOID	write_tuple(
    i4		force_hexdump,
    DMF_ATP     *atp,
    char	*prefix,
    DB_TRAN_ID  *tran_id,
    DB_TAB_ID   *table_id,
    PTR		*record,
    i2		rep_type,
    i4		size,
    i2		comptype,
    PTR		jrecord);

static ATP_TD * get_table_descriptor(
    DMF_ATP     *atp,
    DB_TAB_ID   *table_id);

static DB_STATUS atp_uncompress(
    DMF_ATP	*atp,
    DB_TAB_ID	*table_id,
    PTR		**record,
    i4	*size,
    i2  comptype,
    PTR jrecord);

DB_STATUS format_emptyTuple(
        DMF_ATP *atp,
        DB_TAB_ID *table_id,
        PTR *image,
        i4 *size);

static DB_STATUS atp_lobj_cpn_redeem(
    DMF_ATP		*atp,
    DB_TRAN_ID		*tran_id,
    DB_TAB_ID		*base_tblid,
    DB_DATA_VALUE	*dvfrom,
    DB_DATA_VALUE	*dvinto,
    i2			rep_type);

static DB_STATUS lobj_redeem(
    PTR		atp_ptr,
    PTR		record,
    i4		*err_code);

static DB_STATUS atp_lobj_copyout(
    DMF_ATP	*atp,
    DB_TRAN_ID	*tran_id,
    DB_TAB_ID	*base_tblid,
    i2		lobj_dttype,
    i2		rep_type,
    PTR		recptr,
    i4		offset,
    FILE	**fdesc);

static DB_STATUS lobj_copy(
    PTR		atp_ptr,
    PTR		record,
    i4		*err_code);

/*{
** Name: dmfatp	- Audit Trail Processor.
**
** Description:
**      This routine implements the audit trail processor.
**
**	Cluster journal files are gross and require some special handling. In
**	a cluster installation, each node performs its own archiving into its
**	own set of cluster journal files for its node. The journal files from
**	the various nodes must be merged together in order to be interpreted
**	properly. Currently, we merge journal files whenever we take a
**	checkpoint, so the only journal files for a database which may not be
**	merged are the ones for the latest journal file history entry. The code
**	currently handles this by checking for the "journal-not-found" error
**	from dm0j_open -- if this occurs on the very last journal file for a
**	cluster, then we dynamically merge the node journal files and audit them
**	(we don't save the results of the merging -- we just merge them in order
**	to audit them).
**
** Inputs:
**      jsx                             Pointer to DMF_JSX journaling context
**	dcb				Pointer to DCB for the database.
**
** Outputs:
**      err_code                        Reason for error return status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      16-nov-1986 (Derek)
**           Created for Jupiter.
**      21-apr-89 (fred)
**          Altered so that adg_startup() is not called (very poorly, I might
**	    add).  odg_startuk() had already been called by dmf_init(), so
**	    this routine will  now call dmf_getadf() (new routine) to obtain
**	    the standing adf session control block.
**	 27-Nov-89 (ac)
**	    Fixed the problem that .JNL file doesn't merge with .N## journal
**	    files.
**       4-feb-91 (rogerk)
**          Bypass server cache locking during auditdb since we don't read
**          any data that is actually protected by the cache lock.  Bypassing
**          cache lock allows us to run while database is locked by Fast
**          Commit server.  Pass DM2D_IGNORE_CACHE_PROTOCOL to dm2d_close call.
**	08-feb-93 (jnash)
**	    Reduced logging project.  Modified to use a two-pass algorithm
**	    by default.  JSX_AUDIT_ALL option dumps all journals records 
**	    and, if no -b/-e option specified by the caller, bypasses the 
**	    prescan phase.
**	15-mar-1993 (rogerk)
**	    Added direction parameter to dm0j_open routine.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Initialize the DMF_ATP with zeros at startup.
**	20-sep-1993 (bryanp)
**	    Fixed some cluster journal file handling bugs.
**	20-sep-1993 (jnash)
**	    Close config file if -wait error detected.
**	31-jan-1994 (jnash)
**	    E_DB_INFO returned from process_record() is expected
**	    for -e, and should not return non-E_DB_OK.
**	21-feb-1994 (jnash)
**	  - Skip prescan phase for -all.
**	  - Add -start_lsn support.  Issue error if start_lsn is 
**	    less than the lsn of first record in first journal file,
**	    as an aid in associating lsn's to journal files. 
**	  - Add verbose output identifying journal file processed.
**	15-jan-1999 (nanpr01)
**	    Pass point to a pointer in dm0j_close routines.
**	03-apr-2001 (somsa01)
**	    Changed latest_jnl to be a static variable in this source
**	    file, as well as saving the current journal file number in
**	    current_journal_num.
**      04-apr-2002 (horda03) Bug 107181.
**        Add an exception handler to catch any user interrupts. Following
**        a user interrupt, abort the Audit process at a convenient point.
**	24-jun-2003 (gupsh01)
**	    Fixed the -all case for auditdb, when the -b and -e flags are 
**	    also specified.
**      10-sep-2003 (horda03) Bug 110889/INGSRV 2521
**          On Windows, in order for the fix for Bug 107181 to work, need
**          to call EXinterrupt() to ensure that any user interrupts have
**          executed the exception handler. 
**      18-oct-2004 (horda03) Buf 101206/INGSRV2769
**          Speed up the AUDITDB -b<time> command. See below for chnges.
*/
DB_STATUS
dmfatp(
DMF_JSX	    *jsx,
DMP_DCB	    *dcb)
{
    PTR			record;
    i4			l_record;
    i4			i,j;
    DB_STATUS		status;
    DM0J_CTX		*jnl = 0;
    DM0C_CNF		*cnf;
    DMF_ATP		atp;
    ATP_B_TX            *b_tx;
    ATP_B_TX            *p_b_tx;
    bool		audit_all;
    bool		verbose;
    bool		lsn_check = FALSE;
    DB_STATUS		local_status;
    EX_CONTEXT          context;
    char                line_buffer [256];
    DB_ERROR		local_dberr;
    i4			local_err;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    if (EXdeclare(ex_handler, &context) != OK)
    {
        EXdelete();
        dmfWriteMsg(NULL, E_DM904A_FATAL_EXCEPTION, 0);
        PCexit(FAIL);
    }

    MEfill( sizeof(atp), 0, (char *)&atp );

    /*	Prepare to audit this database. */

    status = atp_prepare(&atp, jsx, dcb);
    if (status != E_DB_OK)
    {
        EXdelete();
	return (status);
    }

    /* On NT, need to ensure that any User interrupts
    ** are delivered by calling EXinterrupt.
    */
    EXinterrupt( EX_DELIVER );

    if (dmfatp_interrupt)
    {
       EXdelete();
       SETDBERR(&jsx->jsx_dberr, 0, E_DM0065_USER_INTR);
       return E_DB_ERROR;
    }

    status = dm0c_open(dcb, DM0C_PARTIAL, dmf_svcb->svcb_lock_list,
	&cnf, &jsx->jsx_dberr);
    if (status != E_DB_OK)
    {
        EXdelete();
        SETDBERR(&jsx->jsx_dberr, 0, E_DM1209_ATP_CNF_OPEN);
	return (status);
    }

    j = cnf->cnf_jnl->jnl_count - 1;
    latest_jnl = cnf->cnf_jnl->jnl_history[j].ckp_l_jnl;

    status = dm0c_close(cnf, 0, &jsx->jsx_dberr);
    if (status != E_DB_OK)
    {
        EXdelete();
        SETDBERR(&jsx->jsx_dberr, 0, E_DM120A_ATP_CNF_CLOSE);
	return (status);
    }

    /* On NT, need to ensure that any User interrupts
    ** are delivered by calling EXinterrupt.
    */
    EXinterrupt( EX_DELIVER );

    if (dmfatp_interrupt)
    {
       EXdelete();
       SETDBERR(&jsx->jsx_dberr, 0, E_DM0065_USER_INTR);
       return E_DB_ERROR;
    }

    /*
    ** If -wait, signal Archiver and wait for purge to current eof.
    */
    if (atp.atp_jsx->jsx_status1 & JSX_ATP_WAIT)
    {
	status = atp_archive_wait(jsx, dcb);
	if (status != E_DB_OK)
	{
            EXdelete();
	    if (cnf)
		(VOID)dm0c_close(cnf, 0, &local_dberr);
	    return (status);
	}

       /* On NT, need to ensure that any User interrupts
       ** are delivered by calling EXinterrupt.
       */
       EXinterrupt( EX_DELIVER );

        if (dmfatp_interrupt)
        {
           EXdelete();
	   SETDBERR(&jsx->jsx_dberr, 0, E_DM0065_USER_INTR);
           return E_DB_ERROR;
        }
    }

    audit_all = (jsx->jsx_status & JSX_AUDIT_ALL) ? TRUE : FALSE;
    verbose   = (jsx->jsx_status & JSX_VERBOSE) ? TRUE : FALSE;

    for (;;)
    {
	/*
	** If not audit_all, perform the prescan phase.
	*/
	if  (! audit_all ||
		(jsx->jsx_status2 & JSX2_ABORT_TRANS))
	{
            i4 check_flag;
            DM0L_HEADER *h;
            ATP_JNL_NO_TIME *jnl_no_time = 0;
            ATP_JNL_NO_TIME *last_jnl_no_time = 0;

            /* Doing the prescan phase, so if the -B or LSN specified,
            ** find the journal where the required records will be found.
            **
            ** The LSN number is contained in all records, so can view the
            ** first record in the journal file.
            **
            ** Time information is only contained in the BT or ET
            ** records, so if -b specified need to scan the journal file
            ** for this record. Note, it is possible for a journal file
            ** not to have a BT or ET record.
            **
            ** Bug 101206:
            **
            ** Previously, two full scans was made of the journals, the first
            ** to determine the TX's to be displayed, the second to display
            ** the TX's. The second scan always started at hte first journal
            ** file, so if the time interval was near the end of the journals,
            ** a significant amount of time was wasted reading through
            ** journal files that would hold no records to display.
            **
            ** The new approach is to first scan the journal files, looking for
            ** the first timestamp, if the timestamp is before the time specified
            ** the next journal is examined. This continues until either all
            ** journals have been examined or a time after the specified time
            ** is found.
            **
            ** A scan starting at the last journal file found with a timestamp
            ** before the journal file with the time stamp now commences. This
            ** scan records all the BT/ET record found. Any ET records found
            ** not after the -B time are discarded. Because the scan may not
            ** begin the the FIRST journal record, it is possible for ET records
            ** to be found without the corresponding BT record. A linked list of
            ** such transactions is created.
            **
            ** If there are transactions to be displayed, where the BT record is
            ** missing, start scaning the remaining journal files until all
            ** records are found. This will constitute the start journal for the
            ** output process.
            **
            ** Example. Consider the journal sequence 53,54,55,56, 57 and 58.
            **          53 is the first journal following a CKPDB, so the first
            **            record will be a BT for 10:00
            **          54 has many records, but the first one with time is 10:04.
            **          55 has many records, but first one with time is 10:05
            **          56 & 57 don't have any BT/ET records.
            **          58 first time'd recorx is 10:08 (an ET which BT record was
            **            written at 10:02 (so is in 53)
            **
            **          If the -b time was 10:07, the sequence is as follows;
            **          
            **          Pass 1: Find journal file which straddles specified time.
            **            Open 54, read records, and determine that the first time is 10:04
            **            Open 55, read records, and determine that the first time is 10:05
            **            Open 56, all records read, no time interval.
            **            Open 57, all records read, no time interval.
            **
            **          Pass 2: We now know that the time specified can only be in the last
            **                  journal. So we can start scanning for TX's from the last
            **                  journal with time.
            **            Open 55, read records recording BT and ET.
            **            Skip Journal 56 and 57 as no BT/ET records in these files
            **            Open 58, read records recording BT/ET. The 10:08 ET record
            **                    doesn't have a BT record, so it is added to the list
            **                    of TX's who's BT record has not been found.
            **
            **          Pass 3: We now know all the TX's to be output, we do not know where
            **            Open 54, read all records for missing BT record(s).
            **            Because we didn't find the missing BT we would check Jnl 53, but as
            **            this is the first journal file, we assume the missing BT records
            **            are in here.
            **
            **          Pass 4: Scan through the Journal files from 53.
            **
            **          The above example is the worse possible, where the interval is in
            **          the first journal file, but consider the situation if Journals 1
            **          to 52 also existed. The above example, will differ as follows
            **
            **          Pass 1: Journals 1 through 52 are examined for the first timestamp
            **                  record, then journal 53 etc (as above).
            **
            **          Pass 2: as above
            **
            **          Pass 3: Journals 54 and 53 are read to find the missing BT records.
            **                  Once the last BT record is found in 53, the scan stop.
            **
            **          Pass 4: As above.
            **
            **        So you can see, that we don't re-read the records in journals 1 to 52.
            **        A significant saving in time.
            **
            ** For fututre consideration:
            **   For LSN searches it is very easy, all records have LSNs, so only need to
            **   examine the first record of every journal. If a new record could be written
            **   to the start of the journal file, with the LSN and time, then this would
            **   mean Pass 1 would be much quicker, the situation where there are no BT/ET
            **   records would be avoided.
            **
            **   It would then also be possible to perform a binary search of the available
            **   journal files to further reduce the number of files we need to open to
            **   determine the starting point of the scan.
            **
            **   If the ET record could include the LSN of it's corresponding BT record, then
            **   the backward scan (Pass 3) through the journals to find the starting journal
            **   for pass 4 would be simpler. We could record the first LSN of each journal
            **   during Pass 1, then simply need to start pass 4 from the journal file with
            **   the highest LSN less than or equal to the lowest LSN of the ET transactions.
            */

            if ( (check_flag = atp.atp_jsx->jsx_status & (JSX_BTIME | JSX_START_LSN) )  )
            {
               i4 first_record;
               i4 jnl_with_time = atp.atp_begin_jnl; /* First Journal after checkpoint */

               if (atp.atp_jsx->jsx_status & JSX_VERBOSE)
               {
                  TRformat(atp_put_line, 0, line_buffer, sizeof(line_buffer),
                           "%@ Begin Prescan Journals %d..%d. JSX Status = %08x\n",
                           atp.atp_begin_jnl + 1, atp.atp_l_jnl, atp.atp_jsx->jsx_status);
               }

               for (i = atp.atp_begin_jnl + 1; check_flag && (i < atp.atp_l_jnl); i++)
               {
                  first_record = 1;

                  for (status = dm0j_open(DM0J_MERGE, (char *)&dcb->dcb_jnl->physical, 
                                          dcb->dcb_jnl->phys_length, 
                                          &dcb->dcb_name, atp.atp_block_size,
                                          i, 0, 0, DM0J_M_READ,
                                         -1, DM0J_FORWARD, (DB_TAB_ID *)0, &jnl, 
					 &jsx->jsx_dberr);
                       (status == E_DB_OK) &&
                       ((status = dm0j_read(jnl, &record, &l_record, &jsx->jsx_dberr)) == E_DB_OK) ; )
                  {
                     h = (DM0L_HEADER *)record;

                     if ( first_record && (check_flag & JSX_START_LSN))
                     {
                        /* Is the LSN for the current record (first in Journal file)
                        ** before or equal to the LSN specified. If before, then we
                        ** need to start the search in the preceding journal file.
                        */
                        if (LSN_LTE(&atp.atp_jsx->jsx_start_lsn, &h->lsn))
                        {
                           /* LSN of Journal record is after the LSN
                           ** specified.
                           */
                           if (LSN_EQ(&atp.atp_jsx->jsx_start_lsn, &h->lsn))
                           {
                              /* This is the specified LSN, so start here. */

                              atp.atp_begin_jnl = i;
                           }
                           else
                           {
                              /* Required LSN may be in the previous journal file. */

                              atp.atp_begin_jnl = i - 1;
                           }

                           check_flag &= ~JSX_START_LSN;
                        }
                     }

                     if (check_flag & JSX_BTIME)
                     {
                        if ( (h->type == DM0LBT) ||
                             (h->type == DM0LET)   )
                        {
                           u_char           *tmp_dot;
                           char             tmp_time[TM_SIZE_STAMP];
                           DB_TAB_TIMESTAMP *timestamp;
                           DB_TAB_TIMESTAMP tmp_tm;

                           timestamp = (h->type == DM0LBT) ?  &((DM0L_BT *)h)->bt_time :
                                                              &((DM0L_ET *)h)->et_time;

                           /*
                           ** Equality comparisons between times in log records
                           ** and ascii strings do not work without first zeroing the 
                           ** ms part (which is actually a random number).  
                           */
                           TMstamp_str((TM_STAMP *)timestamp, tmp_time);
                           tmp_dot = (u_char *)STindex(tmp_time, ".", 0);
                           *tmp_dot = EOS;
                           TMstr_stamp(tmp_time, (TM_STAMP *)&tmp_tm);

                           if (TMcmp_stamp((TM_STAMP *)&tmp_tm, (TM_STAMP *)&atp.atp_jsx->jsx_bgn_time) >= 0)
                           {
                              /* Current Timestamp is at or after the required time,
                              ** so start search from a previous journal where we
                              ** found the start time.
                              */
                              atp.atp_begin_jnl = jnl_with_time;

                              check_flag &= ~JSX_BTIME;
                           }

                           jnl_with_time = i;

                           break;
                        }
                     }
                     else
                     {
                        /* Not interested in the time, so terminqte loop
                        ** Already checked LSN of the first record above.
                        */
                        break;
                     }

                     first_record = 0;
                  }

                  if (status != E_DB_OK)
                  {
                     ATP_JNL_NO_TIME *no_time;

                     if (jsx->jsx_dberr.err_code != E_DM0055_NONEXT)
                     {
                        /* Unexpected error, so revert to long scan
                        */
                        atp.atp_begin_jnl = atp.atp_f_jnl;
                        jnl_no_time = 0;

                        break;
                     }

                     if ( (no_time = (ATP_JNL_NO_TIME *) MEreqmem( 0, sizeof(ATP_JNL_NO_TIME), TRUE, NULL)) )
                     {
                        no_time->next = 0;

                        no_time->jnl = i;

                        if (last_jnl_no_time)
                        {
                           last_jnl_no_time->next = no_time;
                        }
                        else
                        {
                           jnl_no_time = no_time;
                        }

                        last_jnl_no_time = no_time;
                     }
                  }

                  (VOID) dm0j_close(&jnl, &local_dberr);
                  jnl = NULL;
               }

               if (atp.atp_jsx->jsx_status & JSX_VERBOSE)
               {
                  TRformat(atp_put_line, 0, line_buffer, sizeof(line_buffer),
                           "%@ End Prescan at Jnl %d. Status = %08x\n",
                           i, status);
               }

               if (jnl)
               {
                  (VOID) dm0j_close(&jnl, &local_dberr);
                  jnl = NULL;
               }

               if ( (status == E_DB_OK) && check_flag )
               {
                  /* Didn't find the required Start Time/LSN
                  ** so we need to check from the last journal
                  ** scanned with time.
                  */
                  atp.atp_begin_jnl = jnl_with_time;
               }
            }
            
            /* Found the journal where the tx's we interested in will
            ** end, so read through and identify all TX's to be displayed.
            */
            if (atp.atp_jsx->jsx_status & JSX_VERBOSE)
            {
               TRformat(atp_put_line, 0, line_buffer, sizeof(line_buffer),
                        "%@ Scanning Journals %d..%d for TXs to display.\n",
                        atp.atp_begin_jnl, atp.atp_l_jnl);
            }

	    for (i = atp.atp_begin_jnl; i && i <= atp.atp_l_jnl; i++)
	    {
                ATP_JNL_NO_TIME *check_no_time;

                for( check_no_time = jnl_no_time; check_no_time; check_no_time = check_no_time->next)
                {
                   if (check_no_time->jnl >= i)
                   {
                      break;
                   }
                }

                if (check_no_time && (check_no_time->jnl == i))
                {
                   /* No point scanning this JNL file as it contains no records with timestamps */

                   continue;
                }

		/* 
		** Open the next journal file.
		*/
		status = dm0j_open(DM0J_MERGE, (char *)&dcb->dcb_jnl->physical, 
		    dcb->dcb_jnl->phys_length, 
		    &dcb->dcb_name, atp.atp_block_size, i, 0, 0, DM0J_M_READ,
		    -1, DM0J_FORWARD, (DB_TAB_ID *)0, &jnl, &jsx->jsx_dberr);
		if (status != E_DB_OK)
		{
		   if (jsx->jsx_dberr.err_code == E_DM9034_BAD_JNL_NOT_FOUND &&
		       CXcluster_enabled() != 0 &&
		       (i == latest_jnl || i == latest_jnl + 1))
		   {
		      /* 
		      ** Merge the VAXcluster journals only if last in current
		      ** sequence.
		      */
		      status = dm0j_merge((DM0C_CNF *)0, dcb, (PTR)&atp,
					preprocess_record, &jsx->jsx_dberr);
		      if (status != E_DB_OK)
		      {
		          dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		      }
		   }
		   else
		   {
		      dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		      SETDBERR(&jsx->jsx_dberr, 0, E_DM1202_ATP_JNL_OPEN);
		   }
		   break;
		}

		for (;;)
		{
                    /* On NT, need to ensure that any User interrupts
                    ** are delivered by calling EXinterrupt.
                    */
                    EXinterrupt( EX_DELIVER );

                    if (dmfatp_interrupt)
                    {
                        status = E_DB_ERROR;
                        break;
                    }

		    /*	Read the next record from this journal file. */

		    status = dm0j_read(jnl, &record, &l_record, &jsx->jsx_dberr);
		    if (status != E_DB_OK)
		    {
			if (jsx->jsx_dberr.err_code == E_DM0055_NONEXT)
			{
			    status = E_DB_OK;
			    CLRDBERR(&jsx->jsx_dberr);
			}

			SETDBERR(&jsx->jsx_dberr, 0, E_DM1203_ATP_JNL_READ);
			break;
		    }

		    /*	
		    ** Preprocess the record.  E_DB_INFO is returned
		    ** when the either the end of the journal is found,
		    ** or when an END TRANSACTION time is found that is after 
		    ** the "-e" time specified on the command line.
		    */
		    status = preprocess_record((PTR)&atp, record, err_code);
		    if (status == E_DB_OK)
			continue;

		    break;
		}

		/*  Close the current journal file. */

		if (jnl)
		{
		    (VOID) dm0j_close(&jnl, &local_dberr);
		}

		/*
		** Break for both errors and warnings (eof)
		*/
		if (status != E_DB_OK)
		    break;
	    }

            /* On NT, need to ensure that any User interrupts
            ** are delivered by calling EXinterrupt.
            */
            EXinterrupt( EX_DELIVER );

            if (dmfatp_interrupt)
            {
	       SETDBERR(&jsx->jsx_dberr, 0, E_DM0065_USER_INTR);
               status = E_DB_ERROR;
               break;
            }

            if (atp.atp_jsx->jsx_status & JSX_VERBOSE)
            {
               i4 cnt = 0;

               for(b_tx = atp.atp_btime_open_et; b_tx; b_tx = b_tx->b_tx_missing_bt)
               {
                  cnt++;
               }

               TRformat(atp_put_line, 0, line_buffer, sizeof(line_buffer),
                        "%@ Identified TX's to be displayed. Need to find %d BT records",
                        cnt);
            }

	    /*
	    ** Move open transactions that have neither committed nor
	    ** aborted from the TX queue to the ATX queue.  
	    ** This will inhibit their output during the output phase.
	    */
	    if (status != E_DB_ERROR)
		status = move_tx_to_atx(&atp);

            /* If the -b flag was specified, may need to scan the preceeding
            ** journals for the BT records.
            */
            if (atp.atp_btime_open_et)
            {
               DM0L_HEADER *h;

               for (i = atp.atp_begin_jnl -1; i && i > atp.atp_f_jnl; i--)
               {
                  if (atp.atp_jsx->jsx_status & JSX_VERBOSE)
                  {
                     TRformat(atp_put_line, 0, line_buffer, sizeof(line_buffer),
                              "%@ Checking Jnl %d for missing BT records\n",
                              i);
                  }

                  status = dm0j_open(DM0J_MERGE, (char *)&dcb->dcb_jnl->physical, 
                    dcb->dcb_jnl->phys_length, 
                    &dcb->dcb_name, atp.atp_block_size, i, 0, 0, DM0J_M_READ,
                    -1, DM0J_FORWARD, (DB_TAB_ID *)0, &jnl, &jsx->jsx_dberr);
                  if (status != E_DB_OK)
                  {
                     dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		     SETDBERR(&jsx->jsx_dberr, 0, E_DM1202_ATP_JNL_OPEN);
                     break;
                  }

                  for(;;)
                  {
                     /*  Read the next record from this journal file. */

                     status = dm0j_read(jnl, &record, &l_record, &jsx->jsx_dberr);
                     if (status != E_DB_OK)
                     {
                        if (jsx->jsx_dberr.err_code == E_DM0055_NONEXT)
			{
                              status = E_DB_OK;
			      CLRDBERR(&jsx->jsx_dberr);
			}

			SETDBERR(&jsx->jsx_dberr, 0, E_DM1203_ATP_JNL_READ);
                        break;
                     }

                     h = (DM0L_HEADER *) record;

                     if (h->type != DM0LBT)
                        continue;

                     for( b_tx = atp.atp_btime_open_et, p_b_tx = 0; b_tx; b_tx = b_tx->b_tx_missing_bt)
                     {
                        if (b_tx->b_tx_tran_id.db_high_tran == h->tran_id.db_high_tran &&
                            b_tx->b_tx_tran_id.db_low_tran == h->tran_id.db_low_tran)
                        {
                           break;
                        }

                        p_b_tx = b_tx;
                     }

                     if (!b_tx)
                        continue;

                     if (!p_b_tx)
                     {
                        if (!(atp.atp_btime_open_et = b_tx->b_tx_missing_bt))
                        {
                           /* Found the last BT record to display */

                           break;
                        }
                     }
                     else
                     {
                        /* Remove the Transaction from the list of TX's where the BT record
                        ** has not been found.
                        */
                        p_b_tx->b_tx_missing_bt = b_tx->b_tx_missing_bt;
                     }
                  }

                  if (jsx->jsx_dberr.err_code != E_DM1203_ATP_JNL_READ)
                  {
                     break;
                  }

                  (VOID) dm0j_close(&jnl, &local_dberr);
                  jnl = 0;

                  if (!atp.atp_btime_open_et)
                  {
                     /* Found all the BT records, so terminate loop */

                     break;
                  }
               }

               /* If all the BT records were found, i is the journal file of the
               ** last BT found. If there are still BT records to be found, i will
               ** be atp.atp_f_jnl, and the BT records had better be in there!
               */
               if ( i >= atp.atp_f_jnl)
                  atp.atp_begin_jnl = i;

               if (jnl)
               {
                  (VOID) dm0j_close(&jnl, &local_dberr);
                  jnl = 0;
               }

               if (atp.atp_jsx->jsx_status & JSX_VERBOSE)
               {
                  TRformat(atp_put_line, 0, line_buffer, sizeof(line_buffer),
                           "%@ All BT records found.\n",
                           atp.atp_begin_jnl + 1, atp.atp_l_jnl, atp.atp_jsx->jsx_status);
               }
            }
	}

	/*
	** Stop for errors, fall thru if end of journal found.
	*/ 
	if (status == E_DB_ERROR) 
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM9055_AUDITDB_PRESCAN_FAIL);
	    break;
	}

	/*
	** Now perform the output phase, copying records from the
	** journals to the selected output device.
	*/
        if (atp.atp_jsx->jsx_status & JSX_VERBOSE)
        {
           TRformat(atp_put_line, 0, line_buffer, sizeof(line_buffer),
                    "%@ Begin Output phase Journals %d..%d\n",
                    atp.atp_begin_jnl, atp.atp_l_jnl);
        }

	for (i = atp.atp_begin_jnl; i && i <= atp.atp_l_jnl; i++)
	{
	    /* 
	    ** Open the next journal file.
	    */
	    status = dm0j_open(DM0J_MERGE, (char *)&dcb->dcb_jnl->physical, 
		dcb->dcb_jnl->phys_length, 
		&dcb->dcb_name, atp.atp_block_size, i, 0, 0, DM0J_M_READ,
		-1, DM0J_FORWARD, (DB_TAB_ID *)0, &jnl, &jsx->jsx_dberr);

	    if (status != E_DB_OK)
	    {
		if (jsx->jsx_dberr.err_code == E_DM9034_BAD_JNL_NOT_FOUND &&
		    CXcluster_enabled() != 0 &&
		    (i == latest_jnl || i == latest_jnl + 1))
		{
		    /* 
		    ** Merge the VAXcluster journals only if last in current
		    ** sequence.
		    */
		    current_journal_num = i;
		    status = dm0j_merge((DM0C_CNF *)0, dcb, (PTR)&atp,
					atp_process_record, &jsx->jsx_dberr);
		    if (status != E_DB_OK)
		    {
			dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		    }
		}
		else
		{
		    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1202_ATP_JNL_OPEN);
		}
		break;
	    }

	    current_journal_num = i;

	    /*
  	    ** If verbose, output the id of the journal we are working on. 
	    ** Don't do this for first journal, which is clear from INFODB.
	    */
	    if ( (verbose) && (i != atp.atp_begin_jnl) )
	    {
		SIprintf(
		    "\nATP: Start processing journal file sequence %d.\n\n", i);
	    }

	    for (;;)
	    {
                /* On NT, need to ensure that any User interrupts
                ** are delivered by calling EXinterrupt.
                */
                EXinterrupt( EX_DELIVER );

                if (dmfatp_interrupt)
                {
                    status = E_DB_ERROR;
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM0065_USER_INTR);
                    break;
                }

		/*	
		** Read the next record from this journal file.
		*/
		status = dm0j_read(jnl, &record, &l_record, &jsx->jsx_dberr);
		if (status != E_DB_OK)
		{
		    if (jsx->jsx_dberr.err_code == E_DM0055_NONEXT)
		    {
			status = E_DB_OK;
			CLRDBERR(&jsx->jsx_dberr);
		    }
		    else
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1203_ATP_JNL_READ);
		    break;
		}

		/*
		** Perform one-time check that start lsn not less than
		** the lsn of the first journal record.
		*/
		if ( (!lsn_check) && (jsx->jsx_status1 & JSX_START_LSN) )
		{
		    DM0L_HEADER		*h = (DM0L_HEADER *)record;

		    if (LSN_LT(&jsx->jsx_start_lsn, &h->lsn))
		    {
			char	    error_buffer[ER_MAX_LEN+1];
			i4	    error_length;

			uleFormat(NULL, E_DM1053_JSP_INV_START_LSN, 
			    (CL_ERR_DESC *)NULL, ULE_LOOKUP, (DB_SQLSTATE *)NULL,
			    error_buffer, ER_MAX_LEN, 
			    &error_length, err_code, 5,
			    0, jsx->jsx_start_lsn.lsn_high,
			    0, jsx->jsx_start_lsn.lsn_low,
			    0, h->lsn.lsn_high, 
			    0, h->lsn.lsn_low,
			    0, atp.atp_f_jnl);
			error_buffer[error_length] = '\0';
			SIprintf("%s\n", error_buffer);

			SETDBERR(&jsx->jsx_dberr, 0, E_DM1203_ATP_JNL_READ);
			status = E_DB_ERROR;
			break;
		    }

		    lsn_check = TRUE;
		}

		/*	
	 	** Select transaction by time and user, and according to
		** whether it resides on the abort list.
	 	*/
		status = atp_process_record((PTR)&atp, record, err_code);
		if (status == E_DB_OK)
		    continue;

		/* 
		** Break for both errors and warnings.
		*/
		break;
	    }

	    /*  Close the current journal file. */

	    if (jnl)
	    {
		(VOID) dm0j_close(&jnl, &local_dberr);
	    }

	    if (status != E_DB_OK)
		break;
	}

	break;
    }

    EXdelete();

    /* 
    ** Perform cleanup processing, retain "status".
    */
    local_status = complete(&atp);
    (VOID) dm2d_close_db(dcb, dmf_svcb->svcb_lock_list,
		DM2D_IGNORE_CACHE_PROTOCOL, &local_dberr);

    if (status == E_DB_INFO)
  	status = E_DB_OK;

    return (status);
}

/*{
** Name: preprocess_record	- Preprocess journal files record
**
** Description:
**      This routine catalogs information required for the output phase.
**	It is always called for normal (non -all) operation, and for -all
**	if either -b or -e are specified.
**
**	The routine is interested in BT, ABORTSAVE and ET records.  BTs
**	are used to catalog transaction commencement.  If the transaction
**	doesn't commit prior to the end of the interval of interest,
**	it will be placed on the ATX by the calling routine.  Abortsaves 
**	define an abortsave window, and are entered on the ABS queue.
**
**	ETs determine if the transaction committed normally
**	or aborted.  If aborted, the transaction is entered on the ATX
**	queue and any abortsave information is dissolved.
**
**	Higher-layer code is responsible for moving remaining TX 
**	entries to the ATX when the end of the journal (or interval of 
**	interest) is found.
**
**
** Inputs:
**      atp                             Pointer to audit context.
**      record				Pointer to the record to catalog.
**
** Outputs:
**      err_code                        Reason for error return status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	    E_DB_INFO 			Early stop: -e time found.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-jan-1993 (jnash).
**          Created for the reduced logging project.
**	24-may-1993 (andys)
**	    Pass appropriate logfile blocksize to dmd_log and dmd_format_log
**	    rather than using hardwired 4096.
**	26-jul-1993 (jnash)
**	    Ignore ET records written by the RCP to mark a database 
**	    inconsistent.
**	31-jan-1994 (jnash)
**	    Delete tx entry from list prior to other ET maintenance,
**  	    otherwise it may be left dangling.
**	21-feb-1994 (jnash)
**	  - Eliminate auditdb -all code from this routine.  A prescan 
**	    phase is not necessary in this case.
**	  - Make equality time matches work by zero ms part of time
**	    in log record.
**	23-may-1994 (andys)
**	    Make first argument to preprocess_record a PTR to quiesce 
**	    lint/qac warnings.
**	21-Jul-1997 (carsu07)
**          Compare the end time specified by the -e flag with the transaction
**          begin time to ensure auditdb does not include transactions started
**          after the specified -e end time. (Bug 83448)
**      12-Aug-1997 (carsu07)
**          Added a check for the -e flag before comparing timestamps. (#84419)
**      02-Sep-1997 (carsu07)
**	    Corrected the arguments in the TMcmp_stamp function call. (#83641)
**	27-Apr-2000 (thaju02)
**	    Moved the portion of code which removes the txn from the tx 
**	    queue, from before to after the check of ET timestamp falling 
**	    after the -e time. Auditdb removed tx from tx queue but 
**	    neglected to place tx on the atx queue, thus record was displayed. 
**	    If the ET timestamp is greater than the -e time, move_tx_to_atx() 
**	    will remove txn from tx queue and add it to atx queue. (B101268)
**	30-may-2001 (devjo01)
**	    Remove unneeded and incorrect cast of return from STchr.
**	23-June-2003 (gupsh01)
**	    Refix bug 103173. The previous fix did not scan the aborted 
**	    transactions for Begin time and End time correctly.
**	28-Feb-2008 (thaju02) 
**	    Following a load, auditdb reports E_DM1205 on a mini-xaction.
**	    Put mini-tx on txq.(B119971)
**	17-Sep-2008 (thaju02)
**	    If mini-tx on txq, return E_DB_OK. 
**	    
[@history_template@]...
*/
static DB_STATUS
preprocess_record(
PTR         atp_ptr,
PTR	    record,
i4	    *err_code)
{
    DMF_ATP	*atp = (DMF_ATP *) atp_ptr;
    DMF_JSX	*jsx = atp->atp_jsx;
    DB_STATUS	status = E_DB_OK;
    ATP_TX	*tx;
    ATP_TX	**txq;
    DM0L_HEADER	*h = (DM0L_HEADER *)record;
    i4		et_flag;
    i4		audit_tx;

    /* NB: "err_code" will point to jsx->jsx_dberr.err_code */

    CLRDBERR(&jsx->jsx_dberr);

    /* Only interested in 3 types of record DM0LBT, DM0LET and
    ** DM0LABORTSAVE.
    */
    switch( h->type)
    {
       case DM0LBT:
       case DM0LET:
       case DM0LEMINI:
          break;

       case DM0LABORTSAVE:
          /*
          ** Record abortsave window information.
          ** Skip this check if the JSX2_ABORT_TRANS flag is set.
          */
          if (!(atp->atp_jsx->jsx_status2 & JSX2_ABORT_TRANS)) 
              return add_abortsave(atp, record);
          else
              return E_DB_OK;

       default :
          return E_DB_OK;
    }

    /*	
    ** Find BT information for this record. 
    ** We keep a catalog of BTs during the prescan so that we can
    ** move transactions that have not completed prior to the end of
    ** the journal (or the -e time) to the abort queue.  It
    ** would be best to not maintain a fixed size list as we
    ** do here, but to allocate dynamic memory for each tx.
    */

    txq = &atp->atp_txh[h->tran_id.db_low_tran & (ATP_TXH_CNT - 1)];
    for (tx = (ATP_TX*)txq; tx = tx->tx_next;)
    {
	if (tx->tx_tran_id.db_high_tran == h->tran_id.db_high_tran &&
	    tx->tx_tran_id.db_low_tran == h->tran_id.db_low_tran)
	{
	    break;
	}
    }

    if (tx == 0)
    {
	if ((h->type == DM0LBT) || (h->type == DM0LEMINI))
	{
	   tx = atp->atp_tx_free;
	   if (tx == 0)
	   {
	       dmfWriteMsg(NULL, E_DM1206_ATP_TOO_MANY_TX, 0);
	       return (E_DB_OK);
	   }
	   atp->atp_tx_free = tx->tx_next;

	   /*
	   **  Initialize the new tx.  
	   */
	   tx->tx_tran_id = h->tran_id;
	   if (h->type == DM0LEMINI)
	   {
		MEcopy("$ingres", 7, &tx->tx_username);
		tx->tx_timestamp.db_tab_high_time =
		tx->tx_timestamp.db_tab_low_time = 0;
		tx->tx_flag = TX_MINI;
	   }
	   else
	   {
		tx->tx_username = ((DM0L_BT *)h)->bt_name;
		tx->tx_timestamp = ((DM0L_BT *)h)->bt_time;
		tx->tx_flag = 0;
	   }

	   /*  Insert onto the TX hash queue. */

	   tx->tx_next = *txq;
	   tx->tx_prev = (ATP_TX*)txq;
	   if (*txq)
	       (*txq)->tx_prev = tx;
	   *txq = tx;

           return E_DB_OK;
        }
    }
    else if ((h->type == DM0LBT) || (h->type == DM0LEMINI))
    {
      /* Reused TX id ? */

      return OK;
    }

    /* Now we have an ET record */

    /*
    ** Look for ETs where not a DM0L_DBINCONST abort. 
    */
    if ((((DM0L_ET *)record)->et_flag & DM0L_DBINCONST) == 0)
    {
	DB_TAB_TIMESTAMP    *timestamp;
	DB_TAB_TIMESTAMP    conv_timestamp;
        DB_TRAN_ID  	    tmp_tran_id;
	char	    	    *tmp_dot;
	char		    tmp_time[TM_SIZE_STAMP];

	/*
	** Equality comparisons between times in log records
	** and ascii strings do not work without first zeroing the 
	** ms part (which is actually a random number).  
	*/
	timestamp = &((DM0L_ET *)record)->et_time;
	TMstamp_str((TM_STAMP *)timestamp, tmp_time);
	tmp_dot = STchr(tmp_time, '.');
	*tmp_dot = '\0';
	TMstr_stamp(tmp_time, (TM_STAMP *)&conv_timestamp);

	et_flag = ((DM0L_ET *)record)->et_flag;
        tmp_tran_id = h->tran_id;

	/*
	** If ET timestamp is later than -e time, end the prescan.
	** This and all other open transactions on the TX queue will be 
	** copied to the ATX by the calling routine. 
	**
	** Note that this check may be wrong because cluster clock differences 
	** or internal server delays could cause a log record with an 
	** earlier timestampt to reside later in the log file.  
	*/
	if ((atp->atp_jsx->jsx_status & JSX_ETIME) &&
            TMcmp_stamp((TM_STAMP *)&conv_timestamp, (TM_STAMP *)&atp->atp_jsx->jsx_end_time) >= 0)
	{
	    return (E_DB_INFO);
	}

	/*
	** We are now done with this TX and can delete it.
	*/
        if (tx)
        {
	    if (tx->tx_next)
	        tx->tx_next->tx_prev = tx->tx_prev;
	    tx->tx_prev->tx_next = tx->tx_next;
	    tx->tx_next = atp->atp_tx_free;
	    atp->atp_tx_free = tx;
	}

	/*
	** If an abort ET is found, 
	** or if the ET time is earlier than the -b time, add 
  	** the element to the abort list.
	** Also if -aborted_transactions flag is set, then we
	** do not scan the ABORT transactions.
	*/
        if ( (atp->atp_jsx->jsx_status & JSX_BTIME) &&
             (audit_tx = TMcmp_stamp((TM_STAMP *)&conv_timestamp,
 	      (TM_STAMP *)&atp->atp_jsx->jsx_bgn_time)) < 0 ) 
        {
            return add_abort(atp, &tmp_tran_id);
        }

        if ((et_flag & DM0L_ABORT) &&
                !(atp->atp_jsx->jsx_status2 & JSX2_ABORT_TRANS))
	{  
	    return add_abort(atp, &tmp_tran_id);
	} 

	if ( (atp->atp_jsx->jsx_status & JSX_BTIME) &&
	     (audit_tx >= 0) )
	{
	    /* OK We're interested in this TX. So add to list of BTIME displays */

	    return add_btime(atp, &tmp_tran_id, tx ? FALSE:TRUE);
	}
    }

    return (E_DB_OK);
}

/*{
** Name: complete	- Complete audit processing.
**
** Description:
**      This routine completes audit processing by close the audit files,
**	deallocating the transaction table, and closing the database.
**
** Inputs:
**      atp                             Pointer to audit context.
**
** Outputs:
**      err_code                        Reason for error return status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      17-nov-1986 (Derek)
**           Created for Jupiter.
**	 7-may-1990 (rogerk)
**	    Deallocate new atp_rowbuf, atp_alignbuf and atp_displaybuf fields.
**	08-feb-1993 (jnash)
**	    Deallocate ABS and ATX.
**	26-jul-93 (rogerk)
**	    Deallocate new atp_uncompbuf segment.
**	    Deallocate variable length attribute descriptors from the
**		Table Descriptor arrays.
**	06-mar-1996 (stial01 for bryanp)
**	    Call dm0m_tbdealloc to deallocate tuple buffers.
**	15-jan-1999 (nanpr01)
**	    pass ptr to ptr to dm0m_tbdealloc to deallocate tuple buffers.
**	08-feb-1999 (nanpr01)
**	    Fix the segv. Since the pointer gets initialized to null after
**	    deallocation, it's reference causes segv. Use a while loop instead
**	    of the for loop to avoid extra local variable.
**	4-Nov-2008 (kibro01) b121164
**	    If we failed to find a specific table but continued with the
**	    others, one or more of these table files will be NULL, so don't
**	    try to close it.
[@history_template@]...
*/
static DB_STATUS
complete(
DMF_ATP	    *atp)
{
    ATP_ATX	    *atx;
    ATP_ABS	    *ab;
    ATP_QUEUE	    *st;
    ATP_QUEUE	    *st1;
    i4	    i;

    /*	Close all audit trail files. */

    if (atp->atp_jsx->jsx_status & (JSX_AUDITFILE | JSX_FNAME_LIST))
    {
	for (i = 0; i < atp->atp_jsx->jsx_tbl_cnt; i++)
	{
	    /* If we didn't open this, the table didn't exist
	    ** so don't try to close it (kibro01) b121164
	    */
	    if (atp->atp_auditfile[i] != NULL)
	    CPclose(&atp->atp_auditfile[i]);
    }
    }

    /*
    ** Deallocate the buffers used for converting and displaying tuple
    ** attribute information.  The memory allocated had a MISC header
    ** at the top of it, so we must include this when deallocating.
    */
    dm0m_tbdealloc(&atp->atp_rowbuf);
    dm0m_tbdealloc(&atp->atp_alignbuf);
    dm0m_tbdealloc(&atp->atp_uncompbuf);

    if (atp->atp_displaybuf)
    {
	atp->atp_displaybuf = 
		(char *)(atp->atp_displaybuf - sizeof(DMP_MISC));
	(VOID) dm0m_deallocate((DM_OBJECT **)&atp->atp_displaybuf);
	atp->atp_sdisplaybuf = 0;
    }

    /*
    ** Deallocate any table descriptor attribute information control blocks.
    */
    for (i = 0; i < ATP_TD_CNT; i++)
    {
	if (atp->atp_td[i].att_array)
	{
	    atp->atp_td[i].att_array = (DB_ATTS *)
		((char *)atp->atp_td[i].att_array - sizeof(DMP_MISC));
	    (VOID) dm0m_deallocate((DM_OBJECT **)&atp->atp_td[i].att_array);
	}
    }

    /*
    ** Clean up any left over ATX queue elements.
    */
    while ((st = atp->atp_atx_state.stq_next) !=
	(ATP_QUEUE *) &atp->atp_atx_state.stq_next)
    {
        atx = (ATP_ATX *)((char *)st - CL_OFFSETOF(ATP_ATX, atx_state));

	/*
	** If abortsave context exists, dissolve it
	*/
	while ((st1 = atx->atx_abs_state.stq_next) != 
		(ATP_QUEUE *)&atx->atx_abs_state.stq_next)
	{
	    ab = (ATP_ABS *)((char *)st1 - CL_OFFSETOF(ATP_ABS, abs_state));

	    ab->abs_state.stq_next->stq_prev = ab->abs_state.stq_prev;
	    ab->abs_state.stq_prev->stq_next = ab->abs_state.stq_next;
	    dm0m_deallocate((DM_OBJECT **)&ab);
	}

	atx->atx_state.stq_next->stq_prev = atx->atx_state.stq_prev;
	atx->atx_state.stq_prev->stq_next = atx->atx_state.stq_next;
	dm0m_deallocate((DM_OBJECT **)&atx);
    }


    return (E_DB_OK);
}

/*{
** Name: display_record	- Display journal record on audit trail.
**
** Description:
**      Copy the journal record to a binary audit history of a table,
**	or format the record and print the audit trail.
**
** Inputs:
**      atp                             Pointer to ATP context.
**      record                          The record to display.
**	tx				Transaction information.
**
** Outputs:
**      err_code                        Reason for error return status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      17-nov-1986 (Derek)
**           Created for Jupiter.
**	 2-oct-1989 (rogerk)
**	    Added handling of DM0L_FSWAP log record.  Made it look like
**	    two FRENAME records.
**      16-jul-1991 (bryanp)
**          B38527: Add support for DM0L_OLD_MODIFY & DM0L_OLD_INDEX.
**      30-aug-1991 (rogerk)
**          Fix AV in audit of RELOCATE log records.  The TRdisplay arguments
**          did not match the format string.  Broke up the old location/new
**          location information onto two lines to make sure we can't overflow
**          the display buffer.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Added support for variable length
**	    table and owner names.
**	20-nov-1992 (jnash)
**	    Reduced Logging Project: Remove FSWAP log records.
**	08-feb-1993 (jnash)
**	    Reduced logging project.  If JSX_AUDIT_ALL, display of journal
**	    records. 
**      26-apr-1993 (jnash)
**          Add support for multiple tables, disable display of index data,
**      18-oct-1993 (jnash)
**	    Add support for replace log record row compression.  Hexdump old 
**	    and new rows if ascii output is requested and if old row
**          compressed.  Fail if binary output is requested and old row is
**          compressed.
**	18-oct-93 (jrb)
**          Added support for DM0LEXTALTER.
**	11-dec-1993 (rogerk)
**	    Changed replace log record compression algorithms.  Changes made
**	    to the row_unpack routine.
**	 6-dec-1995 (nick)
**	    The 'repnew' part of an update record was being written using
**	    the wrong size. #72440
**	 8-jan-1996 (nick)
**	    Changes to fix #73736 ; make the audit tuple structure file
**	    visible, alter references to this appropriately and rationalise
**	    the record length calculation.
**	25-feb-1996 (nick)
**	    Uncompress tuples in audit trail file output via 
**	    atp_uncompress(). #74796
**	06-may-1996 (thaju02)
**	    New Page Format Project.
**		Pass page size to write_tuple() and atp_uncompress().
**	29-jul-1999 (natjo01)
**	    For variable page sizes, make sure we allocate enough
**	    room for the expanded tuple after the replace operation.
**	    (b95311)
**	17-aug-1999 (hayke02)
**	    We now do not allocate memory for recbuf_obj and assign
**	    recbuf_ptr until nrec_size has been set to rep_nrec_size.
**	    This change fixes bug 98426.
**	18-jan-2000 (gupsh01)
**	    Added support for output switched -aborted_transactions and 
**	    -internal_data required for the visual journal analser
**	    project. 
**      09-mar-2000 (gupsh01)
**          Added support for displaying the transaction status with 
**          Option -internal_data, for the ascii output of auditdb.
**	02-may-2000 (gupsh01)
**	    Added support for printing out the ABORTSAVE records.
**	    Also added support for printing to stdout if -file=- is
**	    provided.
**	    Switched the order in which the lsn_high and lsn_low values 
**	    are printed out for the ascii output of auditdb. The lsn_high 
**	    is now printed out first. 
**      15-sep-2000 ( gupsh01)
**          Added formatting of empty tuple for BT and ET records in case
**          of -aborted_transaction option for file output of auditdb.
**	27-mar-2001 (somsa01)
**	    If JSX2_EXPAND_LOBJS is set, do not print out information on
**	    updates to extension tables to stdout. Also, if we're writing
**	    record information to a file, properly write out large
**	    objects so that they can be properly copied back into a
**	    database.
**	23-jul-2001 (somsa01)
**	    Added the passing of the offset separately to
**	    atp_lobj_copyout().
**      27-sep-2002 (ashco01) Bug 108815.
**          Do not print LOCATION title or value for CREATE transactions
**          when object does not have a location count.
**	29-Apr-2004 (gorvi01)
**		Added case for DM0L_DEL_LOCATION used for unextenddb.
**      30-Jan-2003 (hanal04) Bug 109530 INGSRV 2077
**          Added journal record pointer parameter to write_tuple().
**	14-Feb-2008 (thaju02)
**	    For auditdb -file -table, write out data for columns that 
**	    follow a lobj column. (B119967)
**	28-Feb-2008 (thaju02)
**	    Following a load, auditdb reports E_DM1205 on a mini-xaction.
**	    Display emini record.(B119971)
**	27-Oct-2009 (kschendel)
**	    Frename display is missing the directory separator between
**	    path and filename.
**	5-Nov-2009 (kschendel) SIR 122739
**	    Delete old-index record.
*/
static DB_STATUS
display_record(
DMF_ATP	    *atp,
PTR	    record,
ATP_TX	    *tx)
{
    DMF_JSX		*jsx = atp->atp_jsx;
    ATP_TUP		audit_record;
    DB_STATUS		status;
    i4			type;		/*  Type of log record. */
    i4			size;		/*  Size of audit record. */
    i4			count;		/*  Count of bytes written. */
    char		date[32];	/*  Formatted timestamp. */
    DB_DATA_VALUE	from;		/*  From type description. */
    DB_DATA_VALUE	to;		/*  To type description. */
    i4			table_size;
    i4			owner_size;
    i4			file_index;
    char		*table_name;
    char		*owner_name;
    char		*rec_ptr;
    char		line_buffer[256];
    char 		*orec_ptr;	/* for update records ... */
    char 		*nrec_ptr;
    char		*pathsep;
    i4			nrec_size;
    i4			orec_size;
    i4			ndata_len;
    i4			odata_len;
    u_i4		local_tid;
    DB_TAB_ID		tab_id;
    FILE		*fdesc;
    ATP_TD		*td;
    DB_TAB_ID		table_id;
    i4			i;
    i4			*err_code = &jsx->jsx_dberr.err_code;

# define	    ATP_OP_SIZE    sizeof(audit_record.audit_hdr.operation)

    /*	First check for writes to the audit file. */

    CLRDBERR(&jsx->jsx_dberr);

    /* Is there a better way to do this ...? */
#if defined(VMS)
    pathsep = "]";
#elif defined(NT_GENERIC)
    pathsep = "\\";
#else
    pathsep = "/";
#endif

    status = OK;
    type = ((DM0L_HEADER *)record)->type;
    if (atp->atp_jsx->jsx_status & (JSX_AUDITFILE | JSX_FNAME_LIST))
    {
	/*  
	** Select records of interest. 
	*/
	switch (type)
	{
	    case DM0LBT:
		if(! (atp->atp_jsx->jsx_status2 & JSX2_ABORT_TRANS))
		    return (E_DB_OK);
		break;

	    case DM0LPUT:
		if (! table_in_list(atp, 
		    ((DM0L_PUT *)record)->put_tbl_id.db_tab_base))
		    return (E_DB_OK);
		break;

	    case DM0LDEL:
		if (! table_in_list(atp, 
		    ((DM0L_DEL *)record)->del_tbl_id.db_tab_base))
		    return (E_DB_OK);
		break;

	    case DM0LREP:
		if (! table_in_list(atp, 
		    ((DM0L_REP *)record)->rep_tbl_id.db_tab_base))
		    return (E_DB_OK);
		break;

	    case DM0LET:
		if(! (atp->atp_jsx->jsx_status2 & JSX2_ABORT_TRANS))
		    return (E_DB_OK);
		break;

	    case DM0LABORTSAVE:
                if(! (atp->atp_jsx->jsx_status2 & JSX2_ABORT_TRANS))
		    return (E_DB_OK);
                break;

	    default:
		return (E_DB_OK);
	}


	/*  Fill in header of the record. */

	audit_record.audit_hdr.username = tx->tx_username;
	audit_record.audit_hdr.tran_id = tx->tx_tran_id;
	audit_record.audit_hdr.table_index = 0;

	/*  Convert the timestamp to a INGRES data datatype. */

	TMstamp_str((TM_STAMP *)&tx->tx_timestamp, date);
	from.db_datatype = DB_CHR_TYPE;

	/* remove the miliseconds. */
	from.db_length = STlength(date) - 3;
	from.db_prec = 0;
	from.db_data = date;
	from.db_collID = -1;

	to.db_datatype = DB_DTE_TYPE;
	to.db_length = 12;
	to.db_prec = 0;
	to.db_data = audit_record.audit_hdr.date;
	to.db_collID = -1;

	(VOID) adc_cvinto(&adf_cb, &from, &to);

	/*  
	** Operation dependent settings. 
	*/
	if ( type == DM0LBT ) 
	{
	    i4	i;

	    /* 
	    ** Prints out the DM0L_BT records for -aborted_transactions
	    ** for -file case. 
	    */
	    MEcopy( "begin           ", ATP_OP_SIZE,
		    audit_record.audit_hdr.operation );
	    MEfill(sizeof(audit_record.image),0,audit_record.image);

	    /*  Write the record to each file for respective tables. */
	    for (i = 0; i < atp->atp_jsx->jsx_tbl_cnt; i++)
	    {	    
		audit_record.audit_hdr.table_base = atp->atp_base_id[i];
		tab_id.db_tab_base = atp->atp_base_id[i];
		tab_id.db_tab_index = atp->atp_index_id[i];
		size = atp->atp_base_size[i];
		status = format_emptyTuple( atp, &tab_id,
					    (PTR *)&audit_record.image, &size);
		file_index = get_tab_index( atp,
					    audit_record.audit_hdr.table_base );
	
		/* if -file=- is provided print to stdout. */
		if (STcompare("-",atp->atp_jsx->jsx_fname_list[file_index])
		    == 0)
		{
		    fdesc = stdout;
		}
		else
		    fdesc = atp->atp_auditfile[file_index];

		if(atp->atp_jsx->jsx_status2 & JSX2_INTERNAL_DATA)
		{
		    local_tid = 0;			
		    status = CPwrite(sizeof(LG_LSN), 
				     (char *) &((DM0L_HEADER *)record)->lsn,
				     &count, &fdesc);
		    status = CPwrite(sizeof(u_i4),(char *) &local_tid, &count,
				     &fdesc); 
		}

		/*
		** Properly adjust the size for large object columns.
		*/
		table_id.db_tab_base = audit_record.audit_hdr.table_base;
		table_id.db_tab_index = audit_record.audit_hdr.table_index;
		td = get_table_descriptor(atp, &table_id);
		if (size && td->lobj_att_count)
		{
		    for (i = 1; i <= td->data_rac.att_count; i++)
		    {
			if (td->att_array[i].flag & ATT_PERIPHERAL)
			{
			   size = size - td->att_array[i].length +
				  DB_CNTSIZE + 1;
			}
		    }
		}

		status = CPwrite(ATP_HDR_SIZE + size, (char *)&audit_record, 
				 &count, &fdesc);
	    }
	}
	else if ( type == DM0LABORTSAVE ) 
	{
	    i4	i;

	    /* 
	    ** Prints out the DM0L_ABORT_SAVE records for
	    ** -aborted_transactions for -file case. 
	    ** For every Abortsave record a savepoint entry is also
	    ** written in order to provide information about the savepoint
	    ** lsn. the savepoint id is printed out in the local_tid field.
	    */
	    MEfill(sizeof(audit_record.image),0,audit_record.image);

	    /*  Write the record to each file for respective tables. */
	    for (i = 0; i < atp->atp_jsx->jsx_tbl_cnt; i++)
	    {
		tab_id.db_tab_base = atp->atp_base_id[i];
		tab_id.db_tab_index = atp->atp_index_id[i];
		size = atp->atp_base_size[i];
		status = format_emptyTuple( atp, &tab_id,
					    (PTR *)&audit_record.image, &size);
		MEcopy( "absave       ", ATP_OP_SIZE,
			audit_record.audit_hdr.operation );
		audit_record.audit_hdr.table_base = atp->atp_base_id[i];
		size = atp->atp_base_size[i];
		file_index = get_tab_index( atp,
					    audit_record.audit_hdr.table_base );

		/* if -file=- is provided print to stdout. */
        	if (STcompare("-",atp->atp_jsx->jsx_fname_list[file_index])
		    == 0)
		{
		    fdesc = stdout;
		}
		else
		    fdesc = atp->atp_auditfile[file_index];

		if(atp->atp_jsx->jsx_status2 & JSX2_INTERNAL_DATA)
		{
		    local_tid = 0;			
		    status = CPwrite(sizeof(LG_LSN), 
				     (char *) &((DM0L_HEADER *)record)->lsn,
				     &count, &fdesc);
		    status = CPwrite(sizeof(u_i4),(char *) &local_tid, &count, 
				     &fdesc);
		}

		/*
		** Properly adjust the size for large object columns.
		*/
		table_id.db_tab_base = audit_record.audit_hdr.table_base;
		table_id.db_tab_index = audit_record.audit_hdr.table_index;
		td = get_table_descriptor(atp, &table_id);
		if (size && td->lobj_att_count)
		{
		    for (i = 1; i <= td->data_rac.att_count; i++)
		    {
			if (td->att_array[i].flag & ATT_PERIPHERAL)
			{
			   size = size - td->att_array[i].length +
				  DB_CNTSIZE + 1;
			}
		    }
		}

		status = CPwrite(ATP_HDR_SIZE + size, (char *)&audit_record, 
				 &count, &fdesc);

		MEcopy( "savept       ", ATP_OP_SIZE,
			audit_record.audit_hdr.operation );

        	if(atp->atp_jsx->jsx_status2 & JSX2_INTERNAL_DATA)
        	{
		    local_tid = ((DM0L_ABORT_SAVE *)record)->as_id;
		    status = CPwrite(sizeof(LG_LSN),
			(char *) &((DM0L_HEADER *)record)->compensated_lsn,
			&count, &fdesc);
		    status = CPwrite(sizeof(u_i4),(char *) &local_tid, &count,
				     &fdesc);
        	}

		status = CPwrite(ATP_HDR_SIZE + size, (char *)&audit_record,
				 &count, &fdesc);
	    }
	}
	else if (type == DM0LPUT) 
	{
	    MEcopy("append  ", ATP_OP_SIZE, audit_record.audit_hdr.operation);
	    audit_record.audit_hdr.table_base 
                 = ((DM0L_PUT *)record)->put_tbl_id.db_tab_base;
	    size = ((DM0L_PUT *)record)->put_rec_size;
	    table_size = ((DM0L_PUT *)record)->put_tab_size;
	    owner_size = ((DM0L_PUT *)record)->put_own_size;
	    rec_ptr = &((DM0L_PUT *)record)->put_vbuf[table_size + owner_size];
	    status = atp_uncompress(atp, &((DM0L_PUT *)record)->put_tbl_id,
					(PTR **)&rec_ptr, &size,
					((DM0L_PUT *)record)->put_comptype, record);
	    MEcopy(rec_ptr, size, audit_record.image);
	    local_tid = ((DM0L_PUT *)record)->put_tid.tid_i4;
	}
	else if (type == DM0LDEL)
	{
	    MEcopy("delete  ", ATP_OP_SIZE, audit_record.audit_hdr.operation);
	    audit_record.audit_hdr.table_base
                  = ((DM0L_DEL *)record)->del_tbl_id.db_tab_base;
	    size = ((DM0L_DEL *)record)->del_rec_size;
	    table_size = ((DM0L_DEL *)record)->del_tab_size;
	    owner_size = ((DM0L_DEL *)record)->del_own_size;
	    rec_ptr = &((DM0L_DEL *)record)->del_vbuf[table_size + owner_size];
	    status = atp_uncompress(atp, &((DM0L_DEL *)record)->del_tbl_id,
					(PTR **)&rec_ptr, &size,
					((DM0L_DEL *)record)->del_comptype, record);
	    MEcopy(rec_ptr, size, audit_record.image);
	    local_tid = ((DM0L_DEL *)record)->del_tid.tid_i4;
	}
	else if (type == DM0LREP)
	{
	    char	*recbuf_ptr;
	    DMP_MISC	*recbuf_obj;
	     
	    /*
	    ** We cannot write a binary history if old rows are compressed 
	    ** because we cannot reconstruct either the old or the new row.  
	    */
	    if (((DM0L_HEADER *)record)->flags & DM0L_COMP_REPL_OROW)
	    {
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1218_ATP_OROW_COMPRESS);
		return (E_DB_ERROR);
	    }

	    audit_record.audit_hdr.table_base 
                  = ((DM0L_REP *)record)->rep_tbl_id.db_tab_base;
	    table_size = ((DM0L_REP *)record)->rep_tab_size;
	    owner_size = ((DM0L_REP *)record)->rep_own_size;

	    MEcopy("repold  ", ATP_OP_SIZE, audit_record.audit_hdr.operation);
	    orec_size = ((DM0L_REP *)record)->rep_orec_size;
	    orec_ptr = &((DM0L_REP *)record)->rep_vbuf[table_size + owner_size];

	    status = atp_uncompress(atp, &((DM0L_REP *)record)->rep_tbl_id,
					(PTR **)&orec_ptr, &orec_size,
					((DM0L_REP *)record)->rep_comptype, record);
	    MEcopy(orec_ptr, orec_size, audit_record.image);
	    file_index = get_tab_index(atp, audit_record.audit_hdr.table_base);

	    /* if -file=- is provided print to stdout. */
	    if (STcompare("-",atp->atp_jsx->jsx_fname_list[file_index]) == 0)
		fdesc = stdout;
	    else
		fdesc = atp->atp_auditfile[file_index];

            local_tid = ((DM0L_REP *)record)->rep_otid.tid_i4;
	    if (atp->atp_jsx->jsx_status2 & JSX2_INTERNAL_DATA)
	    {
		status = CPwrite(sizeof(LG_LSN),
				 (char *) &((DM0L_HEADER *)record)->lsn,
				 &count, &fdesc);
		status = CPwrite(sizeof(u_i4),(char *) &local_tid, &count,
				 &fdesc);
	    }

	    /*
	    ** Properly adjust the size for large object columns.
	    */
	    table_id.db_tab_base = audit_record.audit_hdr.table_base;
	    table_id.db_tab_index = audit_record.audit_hdr.table_index;
	    td = get_table_descriptor(atp, &table_id);
	    if (!orec_size || !td->lobj_att_count)
	    {
		status = CPwrite(ATP_HDR_SIZE + orec_size,
				 (char *) &audit_record, &count, &fdesc);
	    }
	    else
	    {
		i4	rec_offset = 0;
		i4	out_size = 0;
		i4	last_lobj = 0;

		/*
		** We've got at least one large object to contend with. So,
		** let's first write out the audit record header. Then, we
		** will write out each column. Whenever we get to a large
		** object, we'll call atp_lobj_copyout() to write out the
		** large object properly.
		*/
		status = CPwrite(ATP_HDR_SIZE, (char *)&audit_record,
				 &count, &fdesc);
		rec_offset += ATP_HDR_SIZE;

		for (i = 1; i <= td->data_rac.att_count; i++)
		{
		    if (td->att_array[i].flag & ATT_PERIPHERAL)
		    {
			/*
			** First, write out everything before the large
			** object.
			*/
			status = CPwrite(out_size,
					 (char *)&audit_record + rec_offset,
					 &count, &fdesc);
			rec_offset += out_size;
			out_size = 0;
			last_lobj = i;

			/*
			** Now, write out the large object.
			*/
			status = atp_lobj_copyout(
					atp,
					&((DM0L_HEADER *)record)->tran_id,
					&table_id,
					td->att_array[i].type,
					1,
					(char *)&audit_record,
					rec_offset,
					&fdesc);
			rec_offset += td->att_array[i].length;
		    }
		    else
			out_size += td->att_array[i].length;
		}

                if (last_lobj < td->data_rac.att_count)
                {
                    status = CPwrite(out_size,
                                (char *)&audit_record + rec_offset,
                                &count, &fdesc);
                }
	    }

	    MEcopy("repnew  ", ATP_OP_SIZE, audit_record.audit_hdr.operation);

	    /*
	    ** reset orec values as they'll have changed if the tuple
	    ** is compressed
	    */
	    orec_size = ((DM0L_REP *)record)->rep_orec_size;
	    orec_ptr = &((DM0L_REP *)record)->rep_vbuf[table_size + owner_size];
	    nrec_ptr = &((DM0L_REP *)record)->rep_vbuf[table_size + 
						    owner_size + orec_size];
	    nrec_size = ((DM0L_REP *)record)->rep_nrec_size;
	    ndata_len = ((DM0L_REP *)record)->rep_ndata_len;

	    status = dm0m_allocate(nrec_size + sizeof(DMP_MISC), (i4)0,
			(i4)MISC_CB, (i4)MISC_ASCII_ID, (char *)NULL,
			(DM_OBJECT**)&recbuf_obj, &jsx->jsx_dberr);
	    if (status != OK)
	    {
		uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		uleFormat(NULL, E_DM9425_MEMORY_ALLOCATE, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
		    0, nrec_size);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1211_ATP_MEM_ALLOCATE);
		return (E_DB_ERROR);
	    }

	    recbuf_ptr = (char *)recbuf_obj + sizeof (DMP_MISC);
	    recbuf_obj->misc_data = recbuf_ptr;

	    /*
	    ** N.B. if compression is in use, the delta is based on 
	    ** the difference in the compressed tuples - this usually means
	    ** the delta is large and also requires us to call the row unpack
	    ** BEFORE uncompressing.  Stash result into temporary buffer as
	    ** uncompression can't be done in situ.
	    */
	    dm0l_row_unpack(orec_ptr, orec_size, nrec_ptr, ndata_len,
		recbuf_ptr, nrec_size, ((DM0L_REP *)record)->rep_diff_offset);

	    status = atp_uncompress(atp, &((DM0L_REP *)record)->rep_tbl_id,
					(PTR **)&recbuf_ptr, &nrec_size,
					((DM0L_REP *)record)->rep_comptype, record);

	    MEcopy(recbuf_ptr, nrec_size, audit_record.image);
		local_tid = ((DM0L_REP *)record)->rep_ntid.tid_i4;
	    /* generic write uses this variable, not nrec_size */
	    size = nrec_size;

	    (VOID) dm0m_deallocate((DM_OBJECT**)&recbuf_obj);
	}
	else 
	{
	    i4	i;

	    /*
	    ** Prints out the DM0L_ET records for -aborted_transactions
	    ** for -file case.
	    */
	    if (((DM0L_ET *)record)->et_flag & DM0L_COMMIT)
	    {
		if (((DM0L_HEADER *)record)->flags & DM0L_NONJNLTAB)
		{
		    MEcopy( "commitnj  ", ATP_OP_SIZE,
			    audit_record.audit_hdr.operation );
		}
		else
		{
		    MEcopy( "commit  ", ATP_OP_SIZE,
			    audit_record.audit_hdr.operation );
		}
	    }
	    else if (((DM0L_ET *)record)->et_flag & DM0L_ABORT)
	    {
		if (((DM0L_HEADER *)record)->flags & DM0L_NONJNLTAB)
		{
		    MEcopy( "abortnj ", ATP_OP_SIZE,
			    audit_record.audit_hdr.operation );
		}
		else
		{
		    MEcopy( "abort   ", ATP_OP_SIZE,
			    audit_record.audit_hdr.operation );
		}
	    }
	    else if (((DM0L_ET *)record)->et_flag & DM0L_DBINCONST)
	    {
		if (((DM0L_HEADER *)record)->flags & DM0L_NONJNLTAB)
		{
		    MEcopy( "inconsistnj", ATP_OP_SIZE,
			    audit_record.audit_hdr.operation );
		}
		else
		{
		    MEcopy( "inconsist", ATP_OP_SIZE,
			    audit_record.audit_hdr.operation );
		}
	    }

	    MEfill(sizeof(audit_record.image),0,audit_record.image);

	    /*  Write the record to each file for respective tables. */
	    for (i = 0; i < atp->atp_jsx->jsx_tbl_cnt; i++)
	    {
		audit_record.audit_hdr.table_base = atp->atp_base_id[i];
		tab_id.db_tab_base = atp->atp_base_id[i];
		tab_id.db_tab_index = atp->atp_index_id[i];
		size = atp->atp_base_size[i];
		status = format_emptyTuple( atp, &tab_id,
					    (PTR *)&audit_record.image,
					    &size);
		file_index = get_tab_index( atp,
					    audit_record.audit_hdr.table_base );

		/* if -file=- is provided print to stdout. */
		if (STcompare("-",atp->atp_jsx->jsx_fname_list[file_index])
		    == 0)
		{
		    fdesc = stdout;
		}
        	else
		    fdesc = atp->atp_auditfile[file_index];

		if(atp->atp_jsx->jsx_status2 & JSX2_INTERNAL_DATA)
		{	
		    local_tid = 0;				
		    status = CPwrite(sizeof(LG_LSN), 
				     (char *) &((DM0L_HEADER *)record)->lsn,
				     &count, &fdesc);
		    status = CPwrite(sizeof(u_i4),(char *) &local_tid, &count, 
				     &fdesc);
		}

		/*
		** Properly adjust the size for large object columns.
		*/
		table_id.db_tab_base = audit_record.audit_hdr.table_base;
		table_id.db_tab_index = audit_record.audit_hdr.table_index;
		td = get_table_descriptor(atp, &table_id);
		if (size && td->lobj_att_count)
		{
		    for (i = 1; i <= td->data_rac.att_count; i++)
		    {
			if (td->att_array[i].flag & ATT_PERIPHERAL)
			{
			   size = size - td->att_array[i].length +
				  DB_CNTSIZE + 1;
			}
		    }
		}

		status = CPwrite(ATP_HDR_SIZE + size, (char *)&audit_record,
				 &count, &fdesc);
	    }
	}

	/*  Write the record. */
	if (status == OK && !((type == DM0LBT) || (type == DM0LET) ||
	    (type == DM0LABORTSAVE)))
	{
	    file_index = get_tab_index(atp, audit_record.audit_hdr.table_base);

	    /* if -file=- is provided print to stdout. */
	    if (STcompare("-",atp->atp_jsx->jsx_fname_list[file_index]) == 0)
		fdesc = stdout;
	    else
		fdesc = atp->atp_auditfile[file_index];

	    if(atp->atp_jsx->jsx_status2 & JSX2_INTERNAL_DATA)
	    {
		status = CPwrite(sizeof(LG_LSN), 
			(char *) &((DM0L_HEADER *)record)->lsn,	&count, 
			&fdesc);
		status = CPwrite(sizeof(u_i4),(char *) &local_tid, &count, 
			&fdesc);
	    }

	    /*
	    ** We're now ready to write the audit record to the file.
	    ** However, large objects need special treatment. Therefore,
	    ** let's find out if we have any large objects to work with.
	    */
	    table_id.db_tab_base = audit_record.audit_hdr.table_base;
	    table_id.db_tab_index = audit_record.audit_hdr.table_index;
	    td = get_table_descriptor(atp, &table_id);

	    if (!size || !td->lobj_att_count)
	    {
		status = CPwrite(ATP_HDR_SIZE + size, (char *)&audit_record,
				 &count, &fdesc);
	    }
	    else
	    {
		i4 rec_offset = 0;
		i4 out_size = 0;
		i4 last_lobj = 0;
		i2 rep_type = ((DM0L_HEADER *)record)->type == DM0LREP ? 2 : 0;

		/*
		** We've got at least one large object to contend with. So,
		** let's first write out the audit record header. Then, we
		** will write out each column. Whenever we get to a large
		** object, we'll call atp_lobj_copyout() to write out the
		** large object properly.
		*/
		status = CPwrite(ATP_HDR_SIZE, (char *)&audit_record,
				 &count, &fdesc);
		rec_offset += ATP_HDR_SIZE;

		for (i = 1; i <= td->data_rac.att_count; i++)
		{
		    if (td->att_array[i].flag & ATT_PERIPHERAL)
		    {
			/*
			** First, write out everything before the large
			** object.
			*/
			status = CPwrite(out_size,
					 (char *)&audit_record + rec_offset,
					 &count, &fdesc);
			rec_offset += out_size;
			out_size = 0;
			last_lobj = i;

			/*
			** Now, write out the large object.
			*/
			status = atp_lobj_copyout(
					atp,
					&((DM0L_HEADER *)record)->tran_id,
					&table_id,
					td->att_array[i].type,
					rep_type,
					(char *)&audit_record,
					rec_offset,
					&fdesc);
			rec_offset += td->att_array[i].length;
		    }
		    else
			out_size += td->att_array[i].length;
		}

		if (last_lobj < td->data_rac.att_count)   
		{
		    status = CPwrite(out_size,
				(char *)&audit_record + rec_offset,
				&count, &fdesc);
		}
	    }
	}
	
	if (status != OK)
	{
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1201_ATP_WRITING_AUDIT);
		return (E_DB_ERROR);
	}
	
	return (E_DB_OK);
    }

    /*
    ** ASCII Output Requested 
    */

    switch (type)
    {
    case DM0LBT:
		TMstamp_str((TM_STAMP *)&((DM0L_BT *)record)->bt_time, date);
		STprintf(line_buffer, "Begin   : Transaction Id %08x%08x %s\
			Username %*s",
			((DM0L_HEADER *)record)->tran_id.db_high_tran,
			((DM0L_HEADER *)record)->tran_id.db_low_tran,
			date,
			sizeof(DB_OWN_NAME), &((DM0L_BT *)record)->bt_name);
		write_line(atp, line_buffer, STlength(line_buffer));
		break;
    
	case DM0LET:
		TMstamp_str((TM_STAMP *)&((DM0L_ET *)record)->et_time, date);
		STprintf(line_buffer, "End     : Transaction Id %08x%08x %s",
			((DM0L_HEADER *)record)->tran_id.db_high_tran,
			((DM0L_HEADER *)record)->tran_id.db_low_tran,
			date);
		write_line(atp, line_buffer, STlength(line_buffer));
		if (atp->atp_jsx->jsx_status2 &  JSX2_ABORT_TRANS)
		{
		/* 
		** If the flag is -aborted_transactions then print out the 
		** Operation for the transaction. 
		*/		
		if (((DM0L_ET *)record)->et_flag & DM0L_COMMIT)
		{
		    if(((DM0L_HEADER *)record)->flags & DM0L_NONJNLTAB)
			MEcopy("commitnj  ", ATP_OP_SIZE, 
					audit_record.audit_hdr.operation);
		    else
			MEcopy("commit  ", ATP_OP_SIZE, 
					audit_record.audit_hdr.operation);
		}
		else if (((DM0L_ET *)record)->et_flag & DM0L_ABORT)
		{
			if(((DM0L_HEADER *)record)->flags & DM0L_NONJNLTAB)
				MEcopy("abortnj ", ATP_OP_SIZE, 
					audit_record.audit_hdr.operation);
				else	
				MEcopy("abort   ", ATP_OP_SIZE, 
					audit_record.audit_hdr.operation);
		}
		else if (((DM0L_ET *)record)->et_flag & DM0L_DBINCONST)
		{
			if(((DM0L_HEADER *)record)->flags & DM0L_NONJNLTAB)
				MEcopy("inconsistnj", ATP_OP_SIZE, 
					audit_record.audit_hdr.operation);
			else	
				MEcopy("inconsist", ATP_OP_SIZE, 
					audit_record.audit_hdr.operation);
		}
		STprintf(line_buffer,"        Operation %s", 
					audit_record.audit_hdr.operation);
		write_line(atp, line_buffer, STlength(line_buffer));
		}
		break;
   
	case DM0LABORTSAVE:
		STprintf(line_buffer, "  AbortSave  : Transaction Id %08x%08x ",
                        ((DM0L_HEADER *)record)->tran_id.db_high_tran,
                        ((DM0L_HEADER *)record)->tran_id.db_low_tran);
                write_line(atp, line_buffer, STlength(line_buffer));
				if(atp->atp_jsx->jsx_status2 & JSX2_INTERNAL_DATA)
		{
			STprintf(line_buffer, "  SAVEPOINT LSN=(0x%x,0x%x) SAVEPOINT ID=%d ",
				((DM0L_HEADER *)record)->compensated_lsn.lsn_high,
				((DM0L_HEADER *)record)->compensated_lsn.lsn_low,
				((DM0L_ABORT_SAVE *)record)->as_id);
			write_line(atp, line_buffer, STlength(line_buffer));
		}
	        break;
 
	case DM0LPUT:
		table_size = ((DM0L_PUT *)record)->put_tab_size;
		owner_size = ((DM0L_PUT *)record)->put_own_size;
		table_name = &((DM0L_PUT *)record)->put_vbuf[0];
		owner_name = &((DM0L_PUT *)record)->put_vbuf[table_size];

		if (atp->atp_jsx->jsx_status2 & JSX2_EXPAND_LOBJS &&
		    STbcompare(table_name, 7, "iietab_", 7, FALSE) == 0)
		{
		    break;
		}

		size = ((DM0L_PUT *)record)->put_rec_size;
		rec_ptr = &((DM0L_PUT *)record)->put_vbuf[table_size + owner_size];
		STprintf(line_buffer, "  Insert/Append  : Transaction Id %08x%08x Id (%d,%d)\
			Table [%*s,%*s]",
			((DM0L_HEADER *)record)->tran_id.db_high_tran,
			((DM0L_HEADER *)record)->tran_id.db_low_tran,
			((DM0L_PUT *)record)->put_tbl_id.db_tab_base,
			((DM0L_PUT *)record)->put_tbl_id.db_tab_index,
			table_size, table_name, owner_size, owner_name);
		write_line(atp, line_buffer, STlength(line_buffer));
		if(atp->atp_jsx->jsx_status2 & JSX2_INTERNAL_DATA)
		{
			STprintf(line_buffer, "	LSN=(0x%x,0x%x) TID %d",
				((DM0L_HEADER *)record)->lsn.lsn_high,
				((DM0L_HEADER *)record)->lsn.lsn_low,
				((DM0L_PUT *)record)->put_tid.tid_i4);
			write_line(atp, line_buffer, STlength(line_buffer));
		}
		write_tuple(0, atp, "Record: ",
			    &((DM0L_HEADER *)record)->tran_id,
			    &((DM0L_PUT *)record)->put_tbl_id,
			    (PTR *) rec_ptr, 0, size,
			    ((DM0L_PUT *)record)->put_comptype, record);
		break;
    
	case DM0LREP:
		table_size = ((DM0L_REP *)record)->rep_tab_size;
		owner_size = ((DM0L_REP *)record)->rep_own_size;
		table_name = &((DM0L_REP *)record)->rep_vbuf[0];
		owner_name = &((DM0L_REP *)record)->rep_vbuf[table_size];

		if (atp->atp_jsx->jsx_status2 & JSX2_EXPAND_LOBJS &&
		    STbcompare(table_name, 7, "iietab_", 7, FALSE) == 0)
		{
		    break;
		}

		orec_size = ((DM0L_REP *)record)->rep_orec_size;
		nrec_size = ((DM0L_REP *)record)->rep_nrec_size;
		odata_len = ((DM0L_REP *)record)->rep_odata_len;
		ndata_len = ((DM0L_REP *)record)->rep_ndata_len;
		orec_ptr = &((DM0L_REP *)record)->rep_vbuf[table_size + owner_size];
		nrec_ptr = &((DM0L_REP *)record)->rep_vbuf[table_size + 
			owner_size + odata_len];
			STprintf(line_buffer, "  Update/Replace : Transaction Id %08x%08x Id (%d,%d)\
				Table [%*s,%*s]",
				((DM0L_HEADER *)record)->tran_id.db_high_tran,
				((DM0L_HEADER *)record)->tran_id.db_low_tran,
				((DM0L_REP *)record)->rep_tbl_id.db_tab_base,
				((DM0L_REP *)record)->rep_tbl_id.db_tab_index,
				table_size, table_name, owner_size, owner_name);

			write_line(atp, line_buffer, STlength(line_buffer));
			if(atp->atp_jsx->jsx_status2 & JSX2_INTERNAL_DATA)
			{
				STprintf(line_buffer, "  LSN=(0x%x,0x%x) OLD TID %d, NEW TID %d",
					((DM0L_HEADER *)record)->lsn.lsn_high,
					((DM0L_HEADER *)record)->lsn.lsn_low,
					((DM0L_REP *)record)->rep_otid.tid_i4,
					((DM0L_REP *)record)->rep_ntid.tid_i4);
				write_line(atp, line_buffer, STlength(line_buffer));
			}
			
			/*
			** Display of tuple information depends on compression.
			** If old record compression used, just dump the changed bits.
			** If old record compression not used, display the old record
			** from the log, then uncompress the new record.
			*/
			if (((DM0L_HEADER *)record)->flags & DM0L_COMP_REPL_OROW)
			{
			    write_tuple(1, atp, 
					"Old:    ",
					&((DM0L_HEADER *)record)->tran_id,
					&((DM0L_REP *)record)->rep_tbl_id,
					(PTR *)orec_ptr, 1, odata_len, 
					((DM0L_REP *)record)->rep_comptype,
                                        record);
			    write_tuple(1, atp, 
					"New:    ",
					&((DM0L_HEADER *)record)->tran_id,
					&((DM0L_REP *)record)->rep_tbl_id,
					(PTR *)nrec_ptr, 2, ndata_len, 
					((DM0L_REP *)record)->rep_comptype,
                                        record);
			}
			else
			{
			    write_tuple(0, atp, "Old:    ",
					&((DM0L_HEADER *)record)->tran_id,
					&((DM0L_REP *)record)->rep_tbl_id,
					(PTR *)orec_ptr, 1, orec_size, 
					((DM0L_REP *)record)->rep_comptype,
                                        record);
			    dm0l_row_unpack(orec_ptr, orec_size, nrec_ptr,
					ndata_len, audit_record.image,
					nrec_size, 
					((DM0L_REP *)record)->rep_diff_offset);
			    write_tuple(0, atp, "New:    ",
					&((DM0L_HEADER *)record)->tran_id,
					&((DM0L_REP *)record)->rep_tbl_id,
					(PTR *)audit_record.image, 2, nrec_size,
					((DM0L_REP *)record)->rep_comptype,
                                        record);
			}
			break;
	
	case DM0LDEL:
		table_size = ((DM0L_DEL *)record)->del_tab_size;
		owner_size = ((DM0L_DEL *)record)->del_own_size;
		table_name = &((DM0L_DEL *)record)->del_vbuf[0];
		owner_name = &((DM0L_DEL *)record)->del_vbuf[table_size];

		if (atp->atp_jsx->jsx_status2 & JSX2_EXPAND_LOBJS &&
		    STbcompare(table_name, 7, "iietab_", 7, FALSE) == 0)
		{
		    break;
		}

		size = ((DM0L_DEL *)record)->del_rec_size;
		rec_ptr = &((DM0L_DEL *)record)->del_vbuf[table_size + owner_size];
		STprintf(line_buffer, "  Delete : Transaction Id %08x%08x Id (%d,%d)\
			Table [%*s,%*s]",
			((DM0L_HEADER *)record)->tran_id.db_high_tran,
			((DM0L_HEADER *)record)->tran_id.db_low_tran,
			((DM0L_DEL *)record)->del_tbl_id.db_tab_base,
			((DM0L_DEL *)record)->del_tbl_id.db_tab_index,
			table_size, table_name, owner_size, owner_name);
		write_line(atp, line_buffer, STlength(line_buffer));
		if(atp->atp_jsx->jsx_status2 & JSX2_INTERNAL_DATA)
		{
			STprintf(line_buffer, "  LSN=(0x%x,0x%x) TID %d",
				((DM0L_HEADER *)record)->lsn.lsn_high,
				((DM0L_HEADER *)record)->lsn.lsn_low,
				((DM0L_DEL *)record)->del_tid.tid_i4);
			write_line(atp, line_buffer, STlength(line_buffer));
		}
		write_tuple(0, atp, "Record: ",
			    &((DM0L_HEADER *)record)->tran_id,
			    &((DM0L_DEL *)record)->del_tbl_id,
			    (PTR *) rec_ptr, 0, size,
			    ((DM0L_DEL *)record)->del_comptype, record);
		break;
	
	case DM0LCREATE:
	STprintf(line_buffer, "  Create  : Transaction Id %08x%08x Id (%d,%d)\
		Table [%*s,%*s]",
		((DM0L_HEADER *)record)->tran_id.db_high_tran,
		((DM0L_HEADER *)record)->tran_id.db_low_tran,
		((DM0L_CREATE *)record)->duc_tbl_id.db_tab_base,
		((DM0L_CREATE *)record)->duc_tbl_id.db_tab_index,	    
		sizeof(DB_OWN_NAME),&((DM0L_CREATE *)record)->duc_name,
		sizeof(DB_OWN_NAME), &((DM0L_CREATE *)record)->duc_owner);
	write_line(atp, line_buffer, STlength(line_buffer));


	/* ashco01 - bug 108815 */
	if (((DM0L_CREATE *)record)->duc_loc_count)
	{
	    STprintf(line_buffer, "\tLocation %*s",
		sizeof(DB_LOC_NAME), ((DM0L_CREATE *)record)->duc_location);
	    write_line(atp, line_buffer, STlength(line_buffer));
	}
	break;
    
	case DM0LDESTROY:
	STprintf(line_buffer, "  Drop/Destroy : Transaction Id %08x%08x Id (%d,%d)\
		Table [%*s,%*s]",
		((DM0L_HEADER *)record)->tran_id.db_high_tran,
		((DM0L_HEADER *)record)->tran_id.db_low_tran,
		((DM0L_DESTROY *)record)->dud_tbl_id.db_tab_base,
		((DM0L_DESTROY *)record)->dud_tbl_id.db_tab_index,
		sizeof(DB_OWN_NAME),&((DM0L_DESTROY *)record)->dud_name,
		sizeof(DB_OWN_NAME), &((DM0L_DESTROY *)record)->dud_owner); 
	write_line(atp, line_buffer, STlength(line_buffer));
	STprintf(line_buffer, "\tLocation %*s",
	    sizeof(DB_LOC_NAME), ((DM0L_DESTROY *)record)->dud_location);
  	write_line(atp, line_buffer, STlength(line_buffer));
	break;

    case DM0LALTER:
	STprintf(line_buffer, "  Alter   : Transaction Id %08x%08x Id (%d,%d)\
 Table [%*s,%*s]",
	    ((DM0L_HEADER *)record)->tran_id.db_high_tran,
	    ((DM0L_HEADER *)record)->tran_id.db_low_tran,
	    ((DM0L_ALTER *)record)->dua_tbl_id.db_tab_base,
	    ((DM0L_ALTER *)record)->dua_tbl_id.db_tab_index,
	    sizeof(DB_OWN_NAME),&((DM0L_ALTER *)record)->dua_name,
	    sizeof(DB_OWN_NAME), &((DM0L_ALTER *)record)->dua_owner); 
  	write_line(atp, line_buffer, STlength(line_buffer));
	break;

    case DM0LRELOCATE:
        STprintf(line_buffer, "  Relocate: Transaction Id %08x%08x \
 Table [%*s,%*s]  Id (%d,%d)",
	    ((DM0L_HEADER *)record)->tran_id.db_high_tran,
	    ((DM0L_HEADER *)record)->tran_id.db_low_tran,
            sizeof(DB_OWN_NAME), &((DM0L_RELOCATE *)record)->dur_name,
            sizeof(DB_OWN_NAME), &((DM0L_RELOCATE *)record)->dur_owner,
            ((DM0L_RELOCATE *)record)->dur_tbl_id.db_tab_base,
            ((DM0L_RELOCATE *)record)->dur_tbl_id.db_tab_index);
        write_line(atp, line_buffer, STlength(line_buffer));
        STprintf(line_buffer, "\tOld Location %*s",
            sizeof(DB_LOC_NAME), &((DM0L_RELOCATE *)record)->dur_olocation);
        write_line(atp, line_buffer, STlength(line_buffer));
        STprintf(line_buffer, "\tNew Location %*s",
            sizeof(DB_LOC_NAME), &((DM0L_RELOCATE *)record)->dur_nlocation);
  	write_line(atp, line_buffer, STlength(line_buffer));
	break;

    case DM0LOLDMODIFY:
        STprintf(line_buffer, "  Modify  : Transaction Id %08x%08x Id (%d,%d)\
 Table [%*s,%*s]",
            ((DM0L_HEADER *)record)->tran_id.db_high_tran,
            ((DM0L_HEADER *)record)->tran_id.db_low_tran,
            ((DM0L_OLD_MODIFY *)record)->dum_tbl_id.db_tab_base,
            ((DM0L_OLD_MODIFY *)record)->dum_tbl_id.db_tab_index,
            sizeof(DB_OWN_NAME),&((DM0L_OLD_MODIFY *)record)->dum_name,
            sizeof(DB_OWN_NAME), &((DM0L_OLD_MODIFY *)record)->dum_owner);
        write_line(atp, line_buffer, STlength(line_buffer));
        break;

    case DM0LMODIFY:
	STprintf(line_buffer, "  Modify  : Transaction Id %08x%08x Id (%d,%d)\
 Table [%*s,%*s]",
	    ((DM0L_HEADER *)record)->tran_id.db_high_tran,
	    ((DM0L_HEADER *)record)->tran_id.db_low_tran,
	    ((DM0L_MODIFY *)record)->dum_tbl_id.db_tab_base,
	    ((DM0L_MODIFY *)record)->dum_tbl_id.db_tab_index,
	    sizeof(DB_OWN_NAME),&((DM0L_MODIFY *)record)->dum_name,
	    sizeof(DB_OWN_NAME), &((DM0L_MODIFY *)record)->dum_owner);
  	write_line(atp, line_buffer, STlength(line_buffer));
	break;

    case DM0LINDEX:
	STprintf(line_buffer, "  Index   : Transaction Id %08x%08x \
 Id (%d,%d) Table [%*s,%*s]",
	    ((DM0L_HEADER *)record)->tran_id.db_high_tran,
	    ((DM0L_HEADER *)record)->tran_id.db_low_tran,
	    ((DM0L_INDEX *)record)->dui_idx_tbl_id.db_tab_base,
	    ((DM0L_INDEX *)record)->dui_idx_tbl_id.db_tab_index,
	    sizeof(DB_OWN_NAME),&((DM0L_INDEX *)record)->dui_name,
	    sizeof(DB_OWN_NAME), &((DM0L_INDEX *)record)->dui_owner); 
  	write_line(atp, line_buffer, STlength(line_buffer));
	break;

    case DM0LFCREATE:
	STprintf(line_buffer, "  Fcreate : Transaction Id %08x%08x \
 File %*s%s%*s",
        ((DM0L_HEADER *)record)->tran_id.db_high_tran,
	((DM0L_HEADER *)record)->tran_id.db_low_tran,
	((DM0L_FCREATE *)record)->fc_l_path,
	&((DM0L_FCREATE *)record)->fc_path,
	pathsep,
	sizeof(((DM0L_FCREATE *)record)->fc_file),
	&((DM0L_FCREATE *)record)->fc_file);
    write_line(atp, line_buffer, STlength(line_buffer));
    break;

    case DM0LLOCATION:
	STprintf(line_buffer, "  Location: Transaction Id %08x%08x \
 Type %d", 
        ((DM0L_HEADER *)record)->tran_id.db_high_tran,
	((DM0L_HEADER *)record)->tran_id.db_low_tran,
	((DM0L_LOCATION*)record)->loc_type);
	write_line(atp, line_buffer, STlength(line_buffer));
	STprintf(line_buffer, "\tLogical location %*s Physical location %*s",
	sizeof(DB_LOC_NAME), 
	&((DM0L_LOCATION*)record)->loc_name,
	((DM0L_LOCATION*)record)->loc_l_extent,
	&((DM0L_LOCATION*)record)->loc_extent);
	write_line(atp, line_buffer, STlength(line_buffer));
    break;

    case DM0LEXTALTER:
	STprintf(line_buffer, "  ExtAlter: Transaction Id %08x%08x \
 DType %d  AType %d", 
        ((DM0L_HEADER *)record)->tran_id.db_high_tran,
	((DM0L_HEADER *)record)->tran_id.db_low_tran,
	((DM0L_EXT_ALTER*)record)->ext_otype,
	((DM0L_EXT_ALTER*)record)->ext_ntype);

	write_line(atp, line_buffer, STlength(line_buffer));

	STprintf(line_buffer, "\tLocation %*s", sizeof(DB_LOC_NAME), 
	    &((DM0L_EXT_ALTER*)record)->ext_lname);

	write_line(atp, line_buffer, STlength(line_buffer));
    break;

    case DM0LFRENAME:
	STprintf(line_buffer, "  Frename : Transaction Id %08x%08x \
 File %*s%s%*s to %*s",
	((DM0L_HEADER *)record)->tran_id.db_high_tran,
	((DM0L_HEADER *)record)->tran_id.db_low_tran,
	((DM0L_FRENAME *)record)->fr_l_path,
	&((DM0L_FRENAME *)record)->fr_path,
	pathsep,
	sizeof(((DM0L_FRENAME *)record)->fr_oldname),
	&((DM0L_FRENAME *)record)->fr_oldname,
	sizeof(((DM0L_FRENAME *)record)->fr_newname),
	&((DM0L_FRENAME *)record)->fr_newname);
    write_line(atp, line_buffer, STlength(line_buffer));
    break;

    case DM0LDELLOCATION:
	STprintf(line_buffer, "  Location: Transaction Id %08x%08x \
 Type %d", 
        ((DM0L_HEADER *)record)->tran_id.db_high_tran,
	((DM0L_HEADER *)record)->tran_id.db_low_tran,
	((DM0L_DEL_LOCATION*)record)->loc_type);
	write_line(atp, line_buffer, STlength(line_buffer));
	STprintf(line_buffer, "\tLogical location %*s Physical location %*s",
	sizeof(DB_LOC_NAME), 
	&((DM0L_DEL_LOCATION*)record)->loc_name,
	((DM0L_DEL_LOCATION*)record)->loc_l_extent,
	&((DM0L_DEL_LOCATION*)record)->loc_extent);
	write_line(atp, line_buffer, STlength(line_buffer));
    break;

    case DM0LEMINI:
	STprintf(line_buffer, "End Mini: Transaction Id %08x%08x",
		((DM0L_HEADER *)record)->tran_id.db_high_tran,
		((DM0L_HEADER *)record)->tran_id.db_low_tran);
                write_line(atp, line_buffer, STlength(line_buffer));
	if(atp->atp_jsx->jsx_status2 & JSX2_INTERNAL_DATA)
	{
	    STprintf(line_buffer, "  LSN=(0x%x,0x%x)   BMINI LSN=(0x%x,0x%x)",
		((DM0L_HEADER *)record)->lsn.lsn_high,
		((DM0L_HEADER *)record)->lsn.lsn_low,
		((DM0L_HEADER *)record)->compensated_lsn.lsn_high,
		((DM0L_HEADER *)record)->compensated_lsn.lsn_low);
	    write_line(atp, line_buffer, STlength(line_buffer));
	}
    break;

    }



	if((atp->atp_jsx->jsx_status2 & JSX2_INTERNAL_DATA) &&
		( type != DM0LPUT) && (type != DM0LREP) && 
		(type != DM0LDEL) && (type != DM0LEMINI))
	{
		STprintf(line_buffer, "  LSN=(0x%x,0x%x)", 
			((DM0L_HEADER *)record)->lsn.lsn_high,
			((DM0L_HEADER *)record)->lsn.lsn_low);
		write_line(atp, line_buffer, STlength(line_buffer));
	}
    return (E_DB_OK);
}

/*{
** Name: atp_process_record	- Qualify the journal record.
**
** Description:
**	Called during the output phase, this routine qualifies the record 
**	from the journal, outputting it if appropriate.
**
**	auditdb -start_lsn and -end_lsn are handled specially.
**	In this mode, auditdb simply outputs all journal records
**	in the specified range, with -e/-b qualifications but 
**	without anything else.
**
** Inputs:
**      atp                             Pointer to audit context.
**      record                          Pointer to the journal file record.
**
** Outputs:
**      err_code                        Reason for error code.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	    E_DB_INFO			End time found
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      17-nov-1986 (derek)
**	    Created for Jupiter.
**	16-dec-1988 (rogerk)
**	    Integrated Sun changes.
**	03-Jan-1990 (ac)
**	    Ignored the DM0LSBACKUP and DM0LEBACKUP log records.
**      16-jul-1991 (bryanp)
**          B38527: Add support for DM0L_OLD_MODIFY & DM0L_OLD_INDEX.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Added support for variable length
**	    table and owner names.
**	08-feb-93 (jnash)
**	    Reduced logging project.  Rewritten for use with new
**	    AUDITDB algorithms.  Dump records in log record format
**	    if auditdb -all or auditdb -all -verbose, in old format otherwise.
**	    Time checks now operate on end transaction timestamps, not
**	    BT times.
**	26-apr-1993 (bryanp)
**	    Adjusted argument types slightly so that this function can be
**	    passed to dm0j_merge() as a record processing routine.
**	24-may-1993 (andys)
**	    Pass appropriate logfile blocksize to dmd_log and dmd_format_log
**	    rather than using hardwired 4096.
**	26-jul-1993 (jnash)
**	    Ignore ET records written by the RCP to mark a database 
**	    inconsistent.
**	18-oct-1993 (jnash)
**	    Add support for replace log record row compression.  
**	    Do not ignore errors in display_record() calls.
**	31-jan-1994 (jnash)
**	    Delete tx entry prior to doing other ET maintenance.
**	21-jan-1994 (jnash)
**	  - Add support for -start_lsn and -end_lsn.  These
**	    auditdb -all options bypass transaction list checking.
**	  - Change auditdb -all to disregard transaction context.  Also, 
**	    use both BT and ET timestamps for -all output.
**	  - Fix exact time matches on -e times by zeroing ms part.
**	  - Change routine name to atp_process_record to avoid confusion
**	    w rolldb procedure of same name.
**	22-feb-1995 (rudtr01)
**	    Fix bug 66913. Print out the BT and ET records for
**	    a normal audit, so the user can plan a rollforwarddb.
**	    This information used to be available in 6.4, but was
**	    somehow omitted during the reduced-logging project.
**	 4-mar-1996 (nick)
**	    Alter above change by rudtr01 ; BT and ET records were printed
**	    out unconditionally as a result of this which is incorrect. 
**	    Bugs #75032 & #75151
**	18-jan-2000 (gupsh01)
**	    Added support for -aborted_transactions for Visual jouranal
**	    analyser project. 
**	02-may-2000 (gupsh01)
**	   Added support for printing out the Abort Save transactions.
**	   Added support for -start_lsn and -end_lsn for non -all cases.
**	   This is for the journal analyser project.
**      09-nov-2000 (gupsh01)
**         Added additional check for -aborted_transactions when the -b and -e
**         options are also provided. 
**	18-Oct-2002 (jenjo02)
**	   STAR 12242427 BUG 108970. Fixed corrupted "begin" "end"
**	   timestamp constraint checking which was causing bogus
**	   E_DM1206_ATP_TOO_MANY_TX, E_DM1205_ATP_NO_BT_FOUND
**	   errors.
**	19-jun-2003 (gupsh01)
**	   Fixed a previous change in behavior with -b and -e options, 
**	   where auditdb will print transactions which begin after the
**	   -b time, rather than printing transactions which ended after
**	   specified begin time. Also modified the abortsave transaction
**	   which should only be printed with -aborted_transaction flag.
**	   Refix Bug 103173. Removed check for begin and end transactions 
**	   from this routine.
*/
static DB_STATUS
atp_process_record(
PTR	    atp_ptr,
char	    *record,
i4	    *err_code)
{
    DMF_ATP	    	*atp = (DMF_ATP *)atp_ptr;
    DMF_JSX		*jsx = atp->atp_jsx;
    ATP_TX	    	**txq;
    ATP_TX	    	*tx;
    ATP_B_TX            *b_tx;
    DB_STATUS	    	status;
    DM0L_HEADER	    	*h = (DM0L_HEADER *)record;
    char	    	line_buffer[256];
    bool	    	verbose;
    bool	    	audit_all;
    DB_TAB_TIMESTAMP    *timestamp = NULL;
    DB_TAB_TIMESTAMP    conv_timestamp;
    char	    	*tmp_dot;
    char		tmp_time[TM_SIZE_STAMP];
    DM_OBJECT           *mem_ptr;

    /* NB: "err_code" will point to jsx->jsx_dberr.err_code */
    *err_code = 0;

    CLRDBERR(&jsx->jsx_dberr);


    audit_all = (atp->atp_jsx->jsx_status & JSX_AUDIT_ALL) ? TRUE : FALSE;
    verbose   = (atp->atp_jsx->jsx_status & JSX_VERBOSE) ? TRUE : FALSE;

    /*
    ** Normal auditdb -e operates only on ET times. 
    ** auditdb -all -b/-e operates on both BT and ET times.
    */
    if ( (atp->atp_jsx->jsx_status & JSX_ETIME) && (h->type == DM0LET) )
	timestamp = &((DM0L_ET *)h)->et_time;

    if ((audit_all) && (h->type == DM0LBT)) 
	timestamp = &((DM0L_BT *)h)->bt_time;

    /*
    ** Equality comparisons between times in log records
    ** and ascii strings do not work without first zeroing the 
    ** ms part (which is actually a random number).  
    */
    if (timestamp)
    {
	u_char	    	*tmp_dot;
	char		tmp_time[TM_SIZE_STAMP];

	TMstamp_str((TM_STAMP *)timestamp, tmp_time);
	tmp_dot = (u_char *)STindex(tmp_time, ".", 0);
	*tmp_dot = EOS;
	TMstr_stamp(tmp_time, (TM_STAMP *)&conv_timestamp);

        if (((atp->atp_jsx->jsx_status & JSX_ETIME) &&
            TMcmp_stamp((TM_STAMP *)&conv_timestamp, (TM_STAMP *)&atp->atp_jsx->jsx_end_time) > 0))
        {
           /* Stop when timestamp after -e time */

           return (E_DB_INFO);
        }
    }

    /*
    ** Perform -start_lsn and -end_lsn checking, which supercedes normal 
    ** auditdb processing.  This mode does not check in-transaction lists; 
    ** we just output all journal records in the specified range.  
    ** -b and -e are honored, and are checked prior to the lsn's.
    */
    if (audit_all)
    {
      if (check_abort(atp, record))
      {
	/*
	** Ignore records prior to -start_lsn, stop when record found 
	** after -end_lsn.
	*/
	if ((atp->atp_jsx->jsx_status1 & JSX_START_LSN) &&
	     (LSN_LT(&h->lsn, &atp->atp_jsx->jsx_start_lsn)))
	{
	  return (E_DB_OK);
	}

	if ((atp->atp_jsx->jsx_status1 & JSX_END_LSN) &&
	    (LSN_GT(&h->lsn, &atp->atp_jsx->jsx_end_lsn)))
	{
	  return (E_DB_INFO);
	}

	/* 
	** Output journal record header and body
	*/
	TRformat(atp_put_line, 0, line_buffer, sizeof(line_buffer), "\n");
	dmd_format_dm_hdr(atp_put_line, h, line_buffer, sizeof(line_buffer));
	dmd_format_log(verbose, atp_put_line, record, atp->atp_block_size, 
		line_buffer, sizeof(line_buffer));
      }
      return (E_DB_OK);
    }

    /*
    ** Normal (non -all) case. 
    */

    /*
    ** Determine BT previously cataloged.
    */
    txq = &atp->atp_txh[h->tran_id.db_low_tran & (ATP_TXH_CNT - 1)];
    for (tx = (ATP_TX *)txq; tx = tx->tx_next;)
    {
	if (tx->tx_tran_id.db_high_tran == h->tran_id.db_high_tran &&
	    tx->tx_tran_id.db_low_tran == h->tran_id.db_low_tran)
	{
	    break;
	}
    }

    if (tx == 0)
    {
	if (h->type != DM0LBT)
	{
	    /* 
            ** If txn list is full, that error has already been reported. 
            ** If we haven't reached the end of the txn list AND
            ** this is not a BT...  
	    */
	    if ( atp->atp_tx_free && !(atp->atp_jsx->jsx_status & JSX_BTIME) )
	    {
		/*
		** Don't give the warning if it is one of the exected internal
		** transactions that don't use BT/ET records:  SBACKUP, EBACKUP.
		*/
		if ((h->type != DM0LSBACKUP) && (h->type != DM0LEBACKUP)
			&& h->type != DM0LJNLSWITCH)
		    dmfWriteMsg(NULL, E_DM1205_ATP_NO_BT_FOUND, 0);

	      /*
	      ** Dump the unrecognized log and return.  We trust this occurrence 
	      ** is uncommon; it is not expected.
	      */
	      dmd_format_dm_hdr(atp_put_line, h, line_buffer,
		sizeof(line_buffer));

		dmd_format_log(TRUE, atp_put_line, record, atp->atp_block_size, 
		    line_buffer, sizeof(line_buffer));
	    }

	    return(E_DB_OK);
	}
	else
	{
            if (atp->atp_jsx->jsx_status & JSX_BTIME)
            {
               /* Check that this TX is one to be displayed */

               for( b_tx = atp->atp_btime_txs [h->tran_id.db_low_tran % ATP_B_TX_CNT];
                    b_tx;
                    b_tx = b_tx->b_tx_next)
               {
                  if (b_tx->b_tx_tran_id.db_high_tran == h->tran_id.db_high_tran &&
                      b_tx->b_tx_tran_id.db_low_tran == h->tran_id.db_low_tran)
                  {
                     break;
                  }
               }

               if (!b_tx)
               {
                  /* This TX isn't to be displayed */
                  return E_DB_OK;
               }

               /* We can now delete this entry */

               if (b_tx->b_tx_next)
               {
                  b_tx->b_tx_next->b_tx_prev = b_tx->b_tx_prev;
               }

               b_tx->b_tx_prev->b_tx_next = b_tx->b_tx_next;

               mem_ptr = (DM_OBJECT *)((char *)b_tx - sizeof(DMP_MISC));

               (VOID) dm0m_deallocate( (DM_OBJECT **)mem_ptr );
            }

	    /*  Create TX entry for new transaction. */

	    tx = atp->atp_tx_free;
	    if (tx == 0)
	    {
		dmfWriteMsg(NULL, E_DM1206_ATP_TOO_MANY_TX, 0);
		return (E_DB_OK);
	    }
	    atp->atp_tx_free = tx->tx_next;

	    /*  
	    ** Initialize the new tx.   
	    ** The tran_id is not strictly necessary to save here, 
	    ** but is convenient.  
	    ** tx_timestamp is only used for display purposes. 
	    ** tx_username is used to check the command line user name.
	    */
	    tx->tx_tran_id = h->tran_id;
	    tx->tx_username = ((DM0L_BT *)h)->bt_name;
	    tx->tx_timestamp = ((DM0L_BT *)h)->bt_time;
	    tx->tx_flag = 0;

	    /*  Insert onto the TX hash queue. */

	    tx->tx_next = *txq;
	    tx->tx_prev = (ATP_TX*)txq;
	    if (*txq)
		(*txq)->tx_prev = tx;
	    *txq = tx;
	}
    }

    /*
    ** Equality comparisons between times in log records
    ** and ascii strings do not work without first zeroing the 
    ** ms part (which is actually a random number).  
    */
    if ((atp->atp_jsx->jsx_status & (JSX_BTIME | JSX_ETIME)) && 
	(h->type == DM0LET))
    {
        TMstamp_str((TM_STAMP *)&((DM0L_ET *)h)->et_time, tmp_time);
	tmp_dot = STchr(tmp_time, '.');
	*tmp_dot = '\0';
	TMstr_stamp(tmp_time, (TM_STAMP *)&conv_timestamp);
    }

    /*	
    ** Qualify and output the record, the prescan has been done.  
    ** i)  If the transaction is not in the abort queue. 
    ** ii) Here we check for the username. 
    ** iii) Check for table owner name unless the record is of one of the types 
    **	   DM0LBT, DM0LET or DM0LABORTSAVE.
    */
    if (check_abort(atp, record) &&		
		check_username(atp, tx) && 
		(check_tab_own(atp, record) || 
       	  (h->type == DM0LBT || h->type == DM0LET ||	
		    h->type == DM0LABORTSAVE)))
    {
	/*
	** If the record is not on the abort list and satisfies
	** user-specified table ownership criteria, output it.
	** User time criteria has already been handled during the 
	** prescan phase.
	** Check for start_lsn and end_lsn flags and restrict the 
	** output if present.
	*/

	/*    
	** If the flag -aborted_transaction is specified then
	** do not pass in a record with flags DM0L_CLR 
	** unless it is of type DM0LSAVE.
	*/
	if ( (atp->atp_jsx->jsx_status2 & JSX2_ABORT_TRANS) && 
		( (((DM0L_HEADER *)record)->flags & DM0L_CLR) && 
			(h->type != DM0LABORTSAVE) ))
	  return (E_DB_OK);

	/*
	** Ignore records prior to -start_lsn, stop when record found 
	** after -end_lsn.
	*/
        if ((atp->atp_jsx->jsx_status1 & JSX_START_LSN) &&
	    (LSN_LT(&h->lsn, &atp->atp_jsx->jsx_start_lsn)))
	{
	    return (E_DB_OK);
	}

        if ((atp->atp_jsx->jsx_status1 & JSX_END_LSN) &&
	    (LSN_GT(&h->lsn, &atp->atp_jsx->jsx_end_lsn)))
	{
	    return (E_DB_INFO);
	}

	status = display_record(atp, (PTR)h, tx);
	if (status != E_DB_OK)
	    return(E_DB_ERROR);
    }

    /*	
    ** Delete transaction information when real ET is seen.
    */
    if ((h->type == DM0LET) && 
	((((DM0L_ET *)record)->et_flag & DM0L_DBINCONST) == 0))
    {
	/*  Delete transaction table entry. */

	if (tx->tx_next)
	    tx->tx_next->tx_prev = tx->tx_prev;
	tx->tx_prev->tx_next = tx->tx_next;
	tx->tx_next = atp->atp_tx_free;
	atp->atp_tx_free = tx;

	/*	
	** If we are past user-specified end time, terminate the output phase.
	*/
	if ((atp->atp_jsx->jsx_status & JSX_ETIME) &&
	    (TMcmp_stamp((TM_STAMP *)&conv_timestamp,
			 (TM_STAMP *)&atp->atp_jsx->jsx_end_time) >= 0))
	{
	    return(E_DB_INFO);
	}
    }

    return (E_DB_OK);
}

/*{
** Name: check_username	- Check name qualification.
**
** Description:
**      This routine checks if a journal record is qualified on the
**	basis of command line name criteria.
**
** Inputs:
**      atp                             Pointer to audit context.
**      tx				Transaction context
**
** Outputs:
**      TRUE				If name criteria satisifed
**      FALSE				If name criteria not satisifed
**
** Side Effects:
**	    none
**
** History:
**	08-feb-93 (jnash)
**	    Reduced logging project.  Extracted from process_record().
*/
static bool
check_username(
DMF_ATP	    *atp,
ATP_TX	    *tx)
{

    if ( ((atp->atp_jsx->jsx_status & JSX_ANAME) == 0 ||
	    MEcmp((char *)&tx->tx_username, (char *)&atp->atp_jsx->jsx_a_name,
	    sizeof(DB_OWN_NAME)) == 0) )
	return(TRUE);

    return(FALSE);
}

/*{
** Name: atp_is_index	- Check if db_tab_index represents index
**
** Description:
**      This routine checks if a db_tab_index is set to non-zero
**	but also checks if it is not a partition, allowing updates to
**	partitions to be output by auditdb etc.
**
** Inputs:
**      tab_index	tbl_id.db_tab_index value
**
** Outputs:
**      TRUE				If this is an index
**      FALSE				If this is not an index
**
** Side Effects:
**	    none
**
** History:
**	23-May-2007 (kibro01) b117123
**	    Created to check if db_tab_index is actually an index, just
**	    because it is non-zero.
*/
static bool
atp_is_index(i4 tab_index, bool *is_partition)
{
	*is_partition = FALSE;

	if (tab_index == 0)
	{
		return (FALSE);
	}

	if (tab_index & DB_TAB_PARTITION_MASK)
	{
		*is_partition = TRUE;
		return (FALSE);
	}
	return (TRUE);
}

/*{
** Name: check_tab_own	- Check table name owner
**
** Description:
**      This routine checks if a journal record is qualified on the
**	basis of command line table name and owner criteria.
**
** Inputs:
**      atp                             Pointer to audit context.
**      record				The journal record.
**
** Outputs:
**      TRUE				If name & owner criteria satisifed
**      FALSE				If name & owner criteria not satisifed
**
** Side Effects:
**	    none
**
** History:
**	08-feb-93 (jnash)
**	    Reduced logging project.  Extracted from process_record().
**	26-apr-93 (jnash)
**	    Don't audit PUT, DELETE or REPLACE index operations.
**	    This is for compatibility with earlier verions of the program.
**	    Note "audit -all" does not call this function, which is what we 
**	    want to happen.  Also fix problem where tables owned by
** 	    $ingres not output when -t (-table) specified.
**	20-sep-93 (jnash)
**	    Delim id support. Do case insensitive compare of $ingres name.
**	23-May-07 (kibro01) b117123
**	    Check if the record is on a partition, so we can work out the
**	    table name differently further on (and don't reject it as an
**	    index)
[@history_template@]...
*/
static bool
check_tab_own(
DMF_ATP	    *atp,
PTR	    record)
{
    DM0L_HEADER	    *h = (DM0L_HEADER *)record;
    char	    *table_name = (char *)NULL;
    char	    *owner_name = (char *)NULL;
    i4	    table_size = DB_TAB_MAXNAME;
    i4	    owner_size = DB_OWN_MAXNAME;
    char	    *sys_owner = "$ingres";
    bool	    index = FALSE;
    bool	    is_partition = FALSE;

    switch (h->type)
    {
    case DM0LPUT:
	index = atp_is_index(((DM0L_PUT *)h)->put_tbl_id.db_tab_index, 
			&is_partition);
	if (index)
	    break;

	table_size = ((DM0L_PUT *)h)->put_tab_size;
	owner_size = ((DM0L_PUT *)h)->put_own_size;
	table_name = &((DM0L_PUT *)h)->put_vbuf[0];
	owner_name = &((DM0L_PUT *)h)->put_vbuf[table_size];
	break;

    case DM0LDEL:
	index = atp_is_index(((DM0L_DEL *)h)->del_tbl_id.db_tab_index, 
			&is_partition);
	if (index)
	    break;

	table_size = ((DM0L_DEL *)h)->del_tab_size;
	owner_size = ((DM0L_DEL *)h)->del_own_size;
	table_name = &((DM0L_DEL *)h)->del_vbuf[0];
	owner_name = &((DM0L_DEL *)h)->del_vbuf[table_size];
	break;

    case DM0LREP:
	index = atp_is_index(((DM0L_REP *)h)->rep_tbl_id.db_tab_index, 
			&is_partition);
	if (index)
	    break;

	table_size = ((DM0L_REP *)h)->rep_tab_size;
	owner_size = ((DM0L_REP *)h)->rep_own_size;
	table_name = (char *)&((DM0L_REP *)h)->rep_vbuf[0];
	owner_name = (char *)&((DM0L_REP *)h)->rep_vbuf[table_size];
	break;

    case DM0LCREATE:
	table_name = (char *)&((DM0L_CREATE *)h)->duc_name.db_tab_name;
	owner_name = (char *)&((DM0L_CREATE *)h)->duc_owner.db_own_name;
	break;

    case DM0LDESTROY:
	table_name = (char *)&((DM0L_DESTROY *)h)->dud_name.db_tab_name;
	owner_name = (char *)&((DM0L_DESTROY *)h)->dud_owner.db_own_name;
	break;

    case DM0LRELOCATE:
	table_name = (char *)&((DM0L_RELOCATE *)h)->dur_name.db_tab_name;
	owner_name = (char *)&((DM0L_RELOCATE *)h)->dur_owner.db_own_name;
	break;

    case DM0LOLDMODIFY:
	table_name = (char *)&((DM0L_OLD_MODIFY *)h)->dum_name.db_tab_name;
	owner_name = (char *)&((DM0L_OLD_MODIFY *)h)->dum_owner.db_own_name;
	break;

    case DM0LMODIFY:
	table_name = (char *)&((DM0L_MODIFY *)h)->dum_name.db_tab_name;
	owner_name = (char *)&((DM0L_MODIFY *)h)->dum_owner.db_own_name;
	break;

    case DM0LINDEX:
	table_name = (char *)&((DM0L_INDEX *)h)->dui_name.db_tab_name;
	owner_name = (char *)&((DM0L_INDEX *)h)->dui_owner.db_own_name;
	break;

    case DM0LALTER:
	table_name = (char *)&((DM0L_ALTER *)h)->dua_name.db_tab_name;
	owner_name = (char *)&((DM0L_ALTER *)h)->dua_owner.db_own_name;
	break;
    }

    /*
    ** Finally, check for a specific table name & system catalog.
    ** Don't display index updates for compatibility with earlier releases.
    ** 
    ** NOTE: This code is not strictly correct in that owner names are not 
    ** checked (owner.tablename support).
    **
    ** Added check that if the table is specified with owner name in owner.table
    ** format check if owner names is same as record's table owner name.
    */
    if ((atp->atp_jsx->jsx_status & (JSX_TNAME | JSX_TBL_LIST)) == 0 ||
	(table_name && check_tab_name(atp, table_name, table_size,
					owner_name, owner_size, is_partition)))
    {
	/*
	** (If not an index AND -a or -t or -table specified) OR
 	** (found a table entry above AND its not a system catalog)
	**     output it.
	*/
	if ((!index) &&
	    (atp->atp_jsx->jsx_status & 
		(JSX_SYSCATS | JSX_TNAME | JSX_TBL_LIST)) || 
	    (owner_name && STbcompare(sys_owner, 7, owner_name, 7, 1)))
	{
	    return (TRUE);
	}
    }

    return (FALSE);
}

/*{
** Name: check_tab_name		- Check if table is audited
**
** Description:
**      Given the name of a table, check the list of command line table names
**	to see if its being audited.
**
** Inputs:
**      atp                             Pointer to audit context.
**      tbl_name			The name of the table 
**      tbl_len				Length of table name.
**      own_name                        Owner of the table.
**      own_len                         Length of the owner's name.
**	is_partition			Is this a partition?
**
** Outputs:
**	Returns:
**	    TRUE if table specified on command line
**	    FALSE if table not specified
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-apr-1993 (jnash)
**	    Created in support of command line -table option.
**	26-feb-1996 (nick)
**	    Compare for the entire length of the table name else we match
**	    on prefix substrings. #74956
**	29-mar-96 (nick)
**	    Correct confusion between DB_MAXNAME and DB_TAB_NAME.
**      28-feb-2000 (gupsh01)
**          Added support for specifying table names in owner_name.table_name
**          format for auditdb. Additional parameters own_name and own_len, are
**          required now to be passed as input.
**      28-feb-2000 (gupsh01)
**          Added support for specifying table names in owner_name.table_name
**          format for auditdb. Additional parameters own_name and own_len, are
**          required now to be passed as input.
**	23-May-2007 (kibro01) b117123
**	    Need to evaluate table name from partition name to check if this
**	    table is desired.
*/
static bool
check_tab_name(
DMF_ATP     *atp,
char	    *tbl_name,
i4	    tbl_len,
char        *own_name,
i4          own_len,
bool	    is_partition)
{

    i4	i;
    char        tablebuf[DB_TAB_MAXNAME];
    char        *table;
    char        ownerbuf[DB_OWN_MAXNAME];
    char        *owner;

    if (is_partition)
    {
	char *tmp;
	/* find the '-' at the end of "iixx pnnn-" and skip past it */
	tmp=STchr(tbl_name, '-');
	if (tmp)
	{
		tbl_len -= ( (tmp + 1) - tbl_name );
		tbl_name = (tmp + 1);
	}
    }

    if (tbl_len < DB_TAB_MAXNAME)
    {
        MEfill(DB_TAB_MAXNAME, ' ', tablebuf);
        MEcopy(tbl_name, tbl_len, tablebuf);
        table = tablebuf;
    }
    else
    {
        table = tbl_name;
    }

    if (own_len < DB_OWN_MAXNAME)
    {
        MEfill(DB_OWN_MAXNAME, ' ', ownerbuf);
        MEcopy(own_name, own_len, ownerbuf);
        owner = ownerbuf;
    }
    else
    {
        owner = own_name;
    }

    for (i = 0; i < atp->atp_jsx->jsx_tbl_cnt; i++)
    {
	if (MEcmp((char *)&atp->atp_jsx->jsx_tbl_list[i].tbl_name,
		table, DB_TAB_MAXNAME) == 0)
	{
	if (STbcompare((char *)&atp->atp_jsx->jsx_tbl_list[i].tbl_owner.db_own_name,
                                             0,"", 0, 0) != 0)
              {
                   if(MEcmp((char *)&atp->atp_jsx->jsx_tbl_list[i].tbl_owner,
                                              owner, DB_OWN_MAXNAME) == 0)
                   return (TRUE);
               }
               else
               return(TRUE);
	}
    }

    return (FALSE);
}

/*{
** Name: table_in_list		- CHeck if table id is being audited
**
** Description:
**      Given a table id, check to see if the table is audited.
**
** Inputs:
**      atp                             Pointer to audit context.
**      tbl_id				The table id 
**
** Outputs:
**	Returns:
**	    TRUE if table audited
**	    FALSE if table not audited
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-apr-1993 (jnash)
**	    Created in support of auditing multiple tables.
*/
static bool
table_in_list(
DMF_ATP     *atp,
i4	    tbl_id)
{
    if (get_tab_index(atp, tbl_id) == -1)
	return(FALSE);

    return (TRUE);
}

/*{
** Name: get_tab_index		- Find offset in list of audited tables.
**
** Description:
**      Given a table id, find the related offset in the atp_base_id list.
**
** Inputs:
**      atp                             Pointer to audit context.
**      tbl_id				The table id 
**
** Outputs:
**	Returns:
**	    index into table 
**	    -1 if the table is not in the list
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-apr-1993 (jnash)
**           Created in support of auditing multiple tables.
*/
static i4
get_tab_index(
DMF_ATP	    *atp,
i4	    tbl_id)
{

    i4	i;
    i4	j = -1;

    for (i = 0; i < atp->atp_jsx->jsx_tbl_cnt; i++)
    {
	if (atp->atp_base_id[i] == tbl_id)
	{
	    j = i;
	    break;
	}
    }

    return (j);
}

/*{
** Name: check_abort 	- Qualify record via searching abort list
**
** Description:
**	Called during the output phase to a determine if a record is
**	part of an aborted transaction or abort-to-savepoint interval.
**
** Inputs:
**	atp				Process Context
**	record				Journal record
**
** Outputs:
**	Returns:
**	    TRUE			if record may be output
**	    FALSE			if record is part of aborted or 
**					  abort to savepoint transaction
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      08-feb-1993 (jnash)
**          Created for the reduced logging project.
**      03-june-2004 (zhahu2)
**      Updated for a complete reading of journal file for records with 
**      a journaled database (INGSRV2847/b112417). 
**      07-mar-2006 (stial01)
**          check_abort() Since we no longer write SAVEPOINT records to log,
**          need to fix test for beginning of savepoint window. (b115681)
**      26-jun-2009 (huazh01 for thaju02)
**         do not qualify open transaction with flag ATX_OPENXACT
**         enabled. (b122177)
*/
static bool
check_abort(
DMF_ATP		*atp,
PTR		record)
{
    ATP_ATX	    *atx;
    ATP_ABS	    *ab;
    ATP_QUEUE	    *st;
    DM0L_HEADER     *h = (DM0L_HEADER *)record;
    bool	    abortsave_found = FALSE;

    /*
    ** Find the prior transaction history, if any.
    ** If no prior history, record is not part of an abort or abortsave tx.
    */
    lookup_atx(atp, &h->tran_id, &atx);
    if (atx == 0)
	return (TRUE);

    /*
    ** If the transaction aborted the record certainly does not qualify.
    */
    if (atx->atx_status & (ATX_ABORT | ATX_OPENXACT ))
	return (FALSE);

    /*
    ** If record is within an abort-to-savepoint window, it doesn't qualify.
    */
    for (st = atx->atx_abs_state.stq_next;
        st != (ATP_QUEUE *)&atx->atx_abs_state.stq_next;
        st = ab->abs_state.stq_next)
    {
	ab = (ATP_ABS *)((char *)st - CL_OFFSETOF(ATP_ABS, abs_state));

	if ((LSN_GT(&h->lsn, &ab->abs_start_lsn)) && 
	    (LSN_LTE(&h->lsn, &ab->abs_end_lsn))) 
	{
	    return (FALSE);
	}
    }

    return (TRUE);
}

/*{
** Name: move_tx_to_atx 	- Move active transaction to abort queue.
**
** Description:
**	Called after completion of the prescan phase, this routine
**	moves any remainging TX entries to the ATX queue.
**	This makes these transactions not appear in output phase.
**
** Inputs:
**	atp				Process Context
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
**      08-feb-1993 (jnash)
**          Created for the reduced logging project.
**	28-Feb-2008 (thaju02)
**	    Following a load, auditdb reports E_DM1205 on a mini-xaction.
**	    Keep mini-tx on txq.(B119971)
**      26-jun-2009 (huazh01 for thaju02)
**          Flag open transaction by setting ATX_OPENXACT. (b122177)
*/
static DB_STATUS
move_tx_to_atx(
DMF_ATP		*atp)
{
    DMF_JSX	    *jsx = atp->atp_jsx;
    DB_STATUS	    status = E_DB_OK;
    ATP_TX	    **txq;
    ATP_TX	    *tx;
    ATP_TX 	    *prev;
    ATP_ATX	    *tmp_atx;
    i4	    i;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Find all previously cataloged BTs
    */
    for (i = 0; i < ATP_TXH_CNT; i++)
    {
	txq = &atp->atp_txh[i];
	for (tx = (ATP_TX *)txq; tx = tx->tx_next;)
        {
	    /* leave mini xaction on the txq */
	    if (tx->tx_flag & TX_MINI)
		continue;

	    /*
	    ** Create an ATX entry if one does not already exist, flagging
	    ** the entry as representing an aborted transaction and 
	    ** transferring the critical bit of information -- the tran_id.
	    */ 
	    lookup_atx(atp, &tx->tx_tran_id, &tmp_atx);
	    if (tmp_atx == 0)
	    {
		status = add_atx(atp, &tx->tx_tran_id, ATX_ABORT, &tmp_atx);
		if (status != E_DB_OK)
		    break;
	    }
	    else
		tmp_atx->atx_status |= ATX_OPENXACT;

	    /*
	    ** Remove the BT TX entry.
	    */
	    prev = tx->tx_next;

	    if (tx->tx_next)
		    tx->tx_next->tx_prev = tx->tx_prev;
	    tx->tx_prev->tx_next = tx->tx_next;
	    tx->tx_next = atp->atp_tx_free;
	    atp->atp_tx_free = tx;

	    tx = prev;
	    if (tx == 0)
		break;
	}

	/*
	** Exit if error.
	*/
	if (status)
	    break;
    }

    return (status);
}

/*{
** Name: atp_prepare	- Prepare to audit.
**
** Description:
**      This routine opens the database, reads the configuration file to
**	compute the journals to use, creates the output files, and initializes
**	the transaction hash table.
**
** Inputs:
**	jsx				Pointer to journal support context.
**	dcb				Pointer to DCB for database.
**
** Outputs:
**      atp                             Pointer to the audit context.
**      err_code                        Reason for error code.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      17-nov-1986 (Derek)
**           Created for Jupiter.
**	07-nov-88 (thurston)
**	    Now inits the new ADF_CB.adf_maxstring field to be DB_MAXSTRING.
**	 7-may-1990 (rogerk)
**	    Initialize and allocate new atp_rowbuf, atp_alignbuf and
**	    atp_displaybuf fields.
**      19-nov-90 (rogerk)
**          Initialized timezone field in the adf control block.
**          Also set the collation sequence while I was at it.
**          This was done as part of the DBMS Timezone changes.
**       4-feb-91 (rogerk)
**          Bypass server cache locking during auditdb since we don't read
**          any data that is actually protected by the cache lock.  Bypassing
**          cache lock allows us to run while database is locked by Fast
**          Commit server.  Pass DM2D_IGNORE_CACHE_PROTOCOL to dm2d_open call.
**      25-feb-1991 (rogerk)
**          Add check for auditdb on journal_disabled database.  Give
**          a warning message.  Moved check for if database is journaled until
**          after the config file is opened.  Put check at error handling
**          at bottom of routine to see if config file needs closing.
**      23-oct-1991 (jnash)
**          Correct LINT detected dm0a_write arg list mismatch.
**	6-sep-1991 (markg)
**	    Fixed bug where begin and end date were being processed 
**	    incorrectly. Bug 39150.
**      24-sep-1992 (stevet)
**          Replace reference to _timezone with _tzcb to support new
**          timezone data structure.
**	08-feb-93 (jnash)
**	    Reduced logging project.  Allocate memory for transaction abort 
**	    list (ATP_ATX).
**	21-apr-93 (jnash)
**	    Change name of function from prepare() to atp_prepare
**	    to eliminate function name ambiguity when debugging.
**	26-apr-1993 (jnash)
**	    Add support for auditing multiple tables.   Change routine
**	    name from prepare() to atp_prepare() for uniqueness.
**	21-jun-93 (jnash)
**	    If dm2d_open_db() fails, try again with force flag.  This allows
**	    AUDITDB to open most inconsistent databases.
**	26-jul-93 (rogerk)
**	    Added support for compressed log rows in log records.
**	    Added atp_uncompbuf, used to uncompress rows before using 
**		adc to convert them to displayable types.
**	    Added variable length attribute arrays (att_array, att_ptr_list)
**		to the Table Descriptor structs.  Initialized them here.
**	26-jul-93 (jnash)
**	    Open database with NOCK flag if '-inconsistent' command line 
**	    flag passed.  This replaces default action implemented by 
**	    21-jun change.
**	20-sep-1993 (andys)
**	    Fix arguments to ule_format call.
**	20-sep-93 (jnash)
**	    Add delimited identifier support.
**	21-feb-94 (jnash)
**	    Add #c support.  This involves determining if checkpoint is 
**	    valid and setting up first and last journals of interest.
**	06-mar-1996 (stial01 for bryanp)
**	    Call dm0m_tballoc to allocate tuple buffers.
**      06-mar-1996 (stial01)
**          atp_prepare() alloc hash queue ptrs correctly
**	 4-jul-96 (nick)
**	    Initialise rcb to ensure shared locks on catalogs.
**	03-jan-1997 (nanpr01)
**	    Use the #define values rather than hard-coded constants.
**      10-mar-1997 (stial01)
**          Pass size of tuple buffer to dm0m_tballoc
**	30-may-1997 (nanpr01)
**	    There is no need to logically open a core catalog. Instead 
**	    just allocate the rcb.
**	21-April-1998(angusm)
**	    Old bug 37246: auditdb with -t flag to extract journal
**	    information for specific table also needs -u flag,
**	    else table ownership is not properly resolved and
**	    'table not found/not exists" errors are reported.
**	 19-may-1999 (wanfr01)
**	    INGSRV 870, bug 97131
**	    Fixed segmentation fault if you have more than ATP_TX_CNT 
**	    due to boundary check value (atp->atp_tx_free) not being 
**	    initialized.
**	18-jan-2000 (gupsh01)
**          Added support for -aborted_transactions for Visual jouranal
**          analyser project.                                              
**	27-mar-2001 (somsa01)
**	    Set up the FEXI functions.
**      18-jun-2001 (gupsh01)
**           Added support for owner_name.table_name for tables.
**           Set the case of the tbl_owner as per the applicable
**           case. Also if the owner name is provided then search in the 
**	     catalogs for that owner and table name, instead of the 
**	     session user.
**	19-Apr-2004 (hanje04)
**	    SIR 111507
**	    Need to define adf_outarg.ad_i8width = -1 in atp_prepare() so the 
**	    default value is used in ADF. Otherwise we get and error calling 
**	    adg_init().
**	21-feb-05 (inkdo01)
**	    Add code to set Unicode normalization flag.
**	22-Sep-2005 (jenjo02)
**	    Use standard dm2t_open(), etc, rather than direct dm2r_allocate_rcb() 
**	    stuff to adhere to table open/close locking and other protocols.
**	23-Apr-2007 (kiria01) 118141
**	    Explicitly initialise Unicode items in the ADF_CB so that
**	    the Unicode session characteristics get reset per session.
**	10-may-2007 (gupsh01)
**	    Initialize adf_utf8_flag for UTF8 character set.
**	15-May-2007 (toumi01)
**	    For supportability add process info to shared memory.
**	3-Nov-2008 (kibro01) b121164
**	    If we fail to find a specific table, continue looping to handle
**	    all the tables which are known.
**	9-sep-2009 (stephenb)
**	    Add dmf_last_id FEXI.
**	19-Nov-2009 (kschendel) SIR 122890
**	    Avoid using error-buffer as both source and target simultaneously,
**	    might be the cause of occasional dm1215 message garblings.
*/
static DB_STATUS
atp_prepare(
DMF_ATP     *atp,
DMF_JSX	    *jsx,
DMP_DCB	    *dcb)
{
    i4		start;
    i4		end;
    i4		i;
    DM0C_CNF            *cnf = 0;
    DB_STATUS           status;
    CL_ERR_DESC		sys_err;
    STATUS              local_status;
    LOCATION            audit_loc;
    char		buf[MAX_LOC];
    DMP_RINDEX		rindexrecord;
    DMP_RELATION	relation;
    DM2R_KEY_DESC	key_desc[2];
    DB_TRAN_ID		tran_id;
    DMP_RCB		*rel = NULL;
    DMP_RCB		*r_idx = NULL;
    DM_TID		audit_tid;
    DM_TID		tid;
    DMP_MISC		*mem_ptr;
    i4		mem_needed;
    i4	        error_length;
    bool		dbachecked;
    char	        error_buffer[ER_MAX_LEN+1];
    char	        tmp_name[DB_MAXNAME+1];
    DB_TAB_ID		catalog_id;
    DB_TAB_TIMESTAMP	timestamp;
    bool	one_table_bad = FALSE;
    bool	one_table_good = FALSE;
    i4			local_err;
    DB_ERROR		local_dberr;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Initialize parts of the atp that might be needed during
    ** cleanup if an error occurs.
    */
    atp->atp_rowbuf = NULL;
    atp->atp_alignbuf = NULL;
    atp->atp_uncompbuf = NULL;
    atp->atp_displaybuf = NULL;
    atp->atp_sdisplaybuf = 0;

    /*	Lock the database. */

    status = dm2d_open_db(dcb, DM2D_A_READ, DM2D_IS, 
	dmf_svcb->svcb_lock_list,
        DM2D_IGNORE_CACHE_PROTOCOL, &jsx->jsx_dberr);
    if (status != E_DB_OK)
    {
	if (jsx->jsx_dberr.err_code == E_DM004C_LOCK_RESOURCE_BUSY)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM120E_ATP_DB_BUSY);
	    return (E_DB_INFO);
	}
	if (jsx->jsx_dberr.err_code == E_DM0053_NONEXISTENT_DB)
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1207_ATP_NON_DB);
 	else if (jsx->jsx_status1 & JSX_ATP_INCONS)
	{
	    /*
 	    ** '-inconsistent' command line flag specified, try again 
 	    **	with force flag to attempt open of inconsistent databases.  
	    */
	    status = dm2d_open_db(dcb, DM2D_A_READ, DM2D_IS, 
		dmf_svcb->svcb_lock_list, 
	 	DM2D_IGNORE_CACHE_PROTOCOL | DM2D_NOCK, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1208_ATP_OPEN_DB);
	}
	if (status != E_DB_OK)
	    return (E_DB_ERROR);
    }

    /*
    ** Get case semantics for this database, save in jsx.
    */
    status = jsp_get_case(dcb, jsx);
    if (status != E_DB_OK)
	return (E_DB_ERROR);

    /*  Find journal information. */

    for (;;)
    {

	/*  Open the configuration file. */

	status = dm0c_open(dcb, DM0C_PARTIAL, dmf_svcb->svcb_lock_list,
	    &cnf, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1209_ATP_CNF_OPEN);
	    break;
	}

	/* 
	** Make sure that the database is journaled. 
        ** Allow auditing on a journal_disabled database.
        */
        if (((dcb->dcb_status & DCB_S_JOURNAL) == 0) &&
            ((cnf->cnf_dsc->dsc_status & DSC_DISABLE_JOURNAL) == 0))
        {
            status = E_DB_ERROR;
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1200_ATP_DB_NOT_JNL);
            break;
        }

        /*
        ** Check for journal disabled status of the database.
        ** Give a warning message if the user is rolling forward a
        ** journal_disabled database.
        */
        if (cnf->cnf_dsc->dsc_status & DSC_DISABLE_JOURNAL)
        {
            /*
            ** Give warning - write to terminal, not the error log.
            */
            uleFormat(NULL, E_DM93A1_AUDITDB_JNLDISABLE, (CL_ERR_DESC *)NULL,
                ULE_LOOKUP, (DB_SQLSTATE *)NULL,
                error_buffer, ER_MAX_LEN, &error_length, &local_err, 1,
                sizeof(dcb->dcb_name.db_db_name), dcb->dcb_name.db_db_name);
	    error_buffer[error_length] = '\0';
            SIprintf("%s\n", error_buffer);
        }

	/*
	** The set of journal files we need to process is calculated from the 
	** checkpoint history and the start and end times given by the caller. 
	** If no start time is given, we begin processing at the last 
	** checkpoint. If no end time is given, we stop processing at the 
	** current time. Therefore, if neither time is specifed the default is
	** to process everything since the last checkpoint. If both start and 
	** end times are given then we can assume that start <= end (the 
	** start > end condition is tested for when the command line is
	** validated). It is possible, if the caller specifies no start 
	** time and an end time that precedes the last checkpoint, that we 
	** will have a similar condition where start > end here. If this is 
	** the case we return an error to the caller.
	**
	** Once we know the range of checkpoints that we are interested in, we
	** calculate the corresponding range of journal files that need to be
	** processed. If there are no journal files associated with the 
	** starting checkpoint, loop through the checkpoint history looking
	** for the next checkpoint that does have journal files associated with
	** it. Similarly, if there are no journal files associated with the 
	** ending checkpoint, loop backward through the checkpoint history 
	** looking for a preceding checkpoint with associated journal files.
	**
	** This logic has not changed with the introduction of -start_lsn
	** and -end_lsn support.  We still require the user to specify
	** times of interest, and we qualify output records (in 
	** preprocess_record() and process_record()) via the LSN's).
	** It would be nice if at some point if journals with the 
	** specified set of lsn's were found automatically.
	*/

	if (jsx->jsx_status & JSX_CKP_SELECTED)
	{
	    /*
	    ** User has specified a starting checkpoint sequence number.
	    ** Validate that its okay, and if so save start/end journals.
	    ** Note special case when starting checkpoint represents
	    ** time when database was not journaled, when 
	    ** starting journal number is zero.  We look for the 
	    ** next checkpoint in this case.  We don't use 
	    ** jnl_first_jnl_seq here because rolldb users may
	    ** override jnl_first_jnl_seq check with #f. 
	    */
	    for (i = 0; i < cnf->cnf_jnl->jnl_count; i++)
	    {
		if ( (cnf->cnf_jnl->jnl_history[i].ckp_sequence == 
			jsx->jsx_ckp_number) &&
		   (cnf->cnf_jnl->jnl_history[i].ckp_f_jnl != 0) )
		{
		    break;
		}
	    }

	    if (i >= cnf->cnf_jnl->jnl_count)
	    {
		uleFormat(NULL, E_DM1219_ATP_INV_CKP, (CL_ERR_DESC *) NULL, 
		    ULE_LOOKUP, (DB_SQLSTATE *) NULL, error_buffer, 
		    ER_MAX_LEN, &error_length, &local_err, 0);
		error_buffer[error_length] = '\0';
		SIprintf("%s\n", error_buffer);

		SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		break;
	    }

	    start = i;
	    end = cnf->cnf_jnl->jnl_count - 1;
	}
	else
	{
	    start = cnf->cnf_jnl->jnl_count - 1;
	    end = cnf->cnf_jnl->jnl_count - 1;

	    for (i = 0; i < cnf->cnf_jnl->jnl_count; i++)
	    {
		if ((jsx->jsx_status & JSX_BTIME) &&
		    TMcmp_stamp((TM_STAMP *)&jsx->jsx_bgn_time, 
			(TM_STAMP *)&cnf->cnf_jnl->jnl_history[i].ckp_date) < 0)
		{
		    if (i)
		    {
			start = i - 1;
			break;
		    }
		}
	    }

	    for (i = 0; i < cnf->cnf_jnl->jnl_count; i++)
	    {
		if ((jsx->jsx_status & JSX_ETIME) &&
		    TMcmp_stamp((TM_STAMP *)&jsx->jsx_end_time, 
			(TM_STAMP *)&cnf->cnf_jnl->jnl_history[i].ckp_date) < 0)
		{
		    if (i)
		    {
			end = i - 1;
			break;
		    }
		}
	    }

	    if (start > end)
	    {
		char	    bgn_time[TM_SIZE_STAMP];
		char	    end_time[TM_SIZE_STAMP];

		TMstamp_str((TM_STAMP *)&jsx->jsx_end_time, end_time);
		TMstamp_str((TM_STAMP *)&cnf->cnf_jnl->jnl_history[start].ckp_date, bgn_time);

		uleFormat(NULL, E_DM1025_JSP_BAD_DATE_RANGE, (CL_ERR_DESC *) NULL,
		    ULE_LOOKUP, (DB_SQLSTATE *) NULL, error_buffer, ER_MAX_LEN, 
		    &error_length, &local_err, 2, TM_SIZE_STAMP, bgn_time, 
		    TM_SIZE_STAMP, end_time);

		dmf_put_line(0, error_length, error_buffer);

		SETDBERR(&jsx->jsx_dberr, 0, E_DM1004_JSP_BAD_ARGUMENT);
		break;
	    }

	    for (; start < end; start++)                                 
	    {
		if (cnf->cnf_jnl->jnl_history[start].ckp_f_jnl)
		    break;
	    }

	    for (; start < end; end--)
	    {
		if (cnf->cnf_jnl->jnl_history[end].ckp_l_jnl)
		    break;
	    }
	}

	/*  Remember starting and ending journal file numbers. */

	atp->atp_begin_jnl = atp->atp_f_jnl = cnf->cnf_jnl->jnl_history[start].ckp_f_jnl;
	atp->atp_l_jnl = cnf->cnf_jnl->jnl_history[end].ckp_l_jnl;
	atp->atp_block_size = cnf->cnf_jnl->jnl_bksz;

	/*  Close the configuration file. */

	status = dm0c_close(cnf, 0, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM120A_ATP_CNF_CLOSE);
	    break;
	}

        /* Reset cnf pointer so error handling code knows file is closed */
        cnf = NULL;

	/*  Initialize the rest of the ATP. */

	atp->atp_dcb = dcb;
	atp->atp_jsx = jsx;

	/*
	** Initialize BT transaction free list.
	*/
	for (i = 0; i < ATP_TXH_CNT; i++)
	    atp->atp_txh[i] = 0;

        atp->atp_tx_free = NULL;
	for (i = 0; i < ATP_TX_CNT; i++)
	{
	    atp->atp_tx[i].tx_next = atp->atp_tx_free;
	    atp->atp_tx_free = &atp->atp_tx[i];
	}

	for (i = 0; i < ATP_TD_CNT; i++)
	{
	    MEfill(sizeof(ATP_TD), 0, &atp->atp_td[i]);
	}

	for (i = 0; i < JSX_MAX_TBLS; i++)
	{
        atp->atp_base_id[i] = 0;
        atp->atp_base_size[i] = 0;
    }

	/*
	** Initialize Aborted Transaction hash queue and pointers.
	*/
	mem_needed = (ATP_ATX_HASH * sizeof(ATP_ATX *)) + sizeof(DMP_MISC);
	status = dm0m_allocate(mem_needed, (i4)0, (i4)MISC_CB, 
	    (i4)MISC_ASCII_ID, (char *)NULL, (DM_OBJECT**)&mem_ptr, &jsx->jsx_dberr);
	if (status != OK)
	{
	    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM9425_MEMORY_ALLOCATE, (CL_ERR_DESC *)NULL, ULE_LOG, 
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
		0, mem_needed);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1211_ATP_MEM_ALLOCATE);
	    break;
	}

	atp->atp_atxh = (ATP_ATX **)((char *)mem_ptr + sizeof(DMP_MISC));
	mem_ptr->misc_data = (char*)atp->atp_atxh;

	for (i = 0; i < ATP_ATX_HASH; i++)
	    atp->atp_atxh[i] = 0;

	atp->atp_atx_state.stq_next = (ATP_QUEUE *)&atp->atp_atx_state.stq_next;
	atp->atp_atx_state.stq_prev = (ATP_QUEUE *)&atp->atp_atx_state.stq_next;

	/*
	** Initialize Missing BT Transaction hash queue and pointers.
	*/
	for (i = 0; i < ATP_B_TX_CNT; i++)
	    atp->atp_btime_txs[i] = 0;

        atp->atp_btime_open_et = 0;

	/*
	** Apply proper case to objects needing it.
	** Adjustment performed in place, assuming that AUDITDB 
	** operates on only one database.
 	*/
	if (jsx->jsx_status & (JSX_TNAME | JSX_TBL_LIST)) 
	{
	    for (i = 0; i < jsx->jsx_tbl_cnt; i++)
	    {
		jsp_set_case(jsx, jsx->jsx_tbl_list[i].tbl_delim ? 
		    jsx->jsx_delim_case : jsx->jsx_reg_case,
		    DB_TAB_MAXNAME, (char *)&jsx->jsx_tbl_list[i].tbl_name, 
		    tmp_name);

		MEcopy(tmp_name, DB_TAB_MAXNAME, 
		    (char *)&jsx->jsx_tbl_list[i].tbl_name);

		/* owner name should also be adjusted, if it exists */
		if (STcmp((char *)
		    &jsx->jsx_tbl_list[i].tbl_owner.db_own_name,"" ) != 0)
		{ 
		    jsp_set_case(jsx, 
	 	       jsx->jsx_tbl_list[i].tbl_delim ? 
			jsx->jsx_delim_case : jsx->jsx_reg_case,
		       DB_OWN_MAXNAME, (char *)&jsx->jsx_tbl_list[i].tbl_owner, 
		       tmp_name);
		
		    MEcopy(tmp_name, DB_OWN_MAXNAME, 
			(char *)&jsx->jsx_tbl_list[i].tbl_owner);
		}
	    }
	}

	if (jsx->jsx_status & JSX_UNAME) 
	{
	    jsp_set_case(jsx, jsx->jsx_username_delim ? 
		jsx->jsx_delim_case : jsx->jsx_reg_case,
		DB_OWN_MAXNAME, (char *)&jsx->jsx_username.db_own_name, tmp_name);

	    MEcopy(tmp_name, DB_OWN_MAXNAME, (char *)&jsx->jsx_username);
	}

	if (jsx->jsx_status & JSX_ANAME) 
	{
	    jsp_set_case(jsx, 
		jsx->jsx_aname_delim ? jsx->jsx_delim_case : jsx->jsx_reg_case,
		DB_OWN_MAXNAME, (char *)&jsx->jsx_a_name, tmp_name);

	    MEcopy(tmp_name, DB_OWN_MAXNAME, (char *)&jsx->jsx_a_name);
	}


	/* Auditing is a security event, audit record must be written. 
	   we  can't do this until the atp_jsx is  initialized */
	if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
	{
	    SXF_ACCESS          access;
	    /*
	    **	First record fact we are auditing this database
	    */
	    access = SXF_A_SELECT;
	    if (status)
		access |= SXF_A_FAIL;
	    else
		access |= SXF_A_SUCCESS;

	    local_status = dma_write_audit( SXF_E_DATABASE,
		access,
		dcb->dcb_name.db_db_name, /* Object name (database) */
		sizeof(dcb->dcb_name.db_db_name), /* Object name (database) */
		&dcb->dcb_db_owner, /* Object owner */
            	I_SX2709_AUDITDB,   /*  Message */
		TRUE,		    /* Force */
		&local_dberr, NULL);

	    /*
	    **	Next if auditing a specific table record that as well
	    */
	    for (i = 0; i < jsx->jsx_tbl_cnt; i++)
	    {
		/*
		**	Note we don't fill in object owner since
		**	auditdb doesn't let you specify a table owner,
		**	just a name.
		*/
		local_status = dma_write_audit(SXF_E_TABLE, access,
		    jsx->jsx_tbl_list[i].tbl_name.db_tab_name, DB_TAB_MAXNAME,
		    NULL, 	  	       	  /* Object owner */
		    I_SX270A_AUDITDB_TABLE,	  /*  Message */
		    TRUE,			  /* Force */
		    &local_dberr, NULL);
	    }
	}

	/*
	** Allocate buffer for formatting attributes in order to display
	** them.  This buffer must be large enough to hold the longest
	** text attribute possible.
	*/
	atp->atp_rowbuf = dm0m_tballoc(DM_MAXTUP);
	if (atp->atp_rowbuf == NULL)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1211_ATP_MEM_ALLOCATE);
	    break;
	}

#ifdef BYTE_ALIGN
	/*
	** Allocate buffer for moving attributes to aligned storage so that
	** they can be converted to display format.  This allocation is
	** only needed on byte-align machines.
	*/
	atp->atp_alignbuf = dm0m_tballoc(DM_MAXTUP);
	if (atp->atp_alignbuf == NULL)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1211_ATP_MEM_ALLOCATE);
	    break;
	}
#endif /* BYTE_ALIGN */

	/*
	** Allocate buffer for uncompressing rows so that they can then
	** be formatted.  The ADF conversion routines do not handle
	** compressed data.
	*/
	atp->atp_uncompbuf = dm0m_tballoc(DM_MAXTUP);
	if (atp->atp_uncompbuf == NULL)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1211_ATP_MEM_ALLOCATE);
	    break;
	}

	/*
	** Allocate buffer for building display output when formatting
	** tuples.  There is no way to accurately predict the size
	** needed for this buffer - so we just allocate a reasonable
	** size (1000 bytes) initially - and then the routines which use
	** this buffer must make sure it is big enough.  If not, then
	** those routines should throw away this buffer and allocate
	** a larger one.
	*/
	mem_needed = 1000 + sizeof(DMP_MISC);
	status = dm0m_allocate(mem_needed, (i4)0, (i4)MISC_CB, 
	    (i4)MISC_ASCII_ID, (char *)NULL, (DM_OBJECT**)&mem_ptr, &jsx->jsx_dberr);
	if (status != OK)
	{
	    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM9425_MEMORY_ALLOCATE, (CL_ERR_DESC *)NULL, ULE_LOG, 
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
		0, mem_needed);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1211_ATP_MEM_ALLOCATE);
	    break;
	}
	atp->atp_displaybuf = (char *)mem_ptr + sizeof(DMP_MISC);
	mem_ptr->misc_data = atp->atp_displaybuf;
	atp->atp_sdisplaybuf = 1000;

	/*  
	** Validate table names and create the .trl files.
	** We do this only if -f or -file specified, because it is only in this
	** case that we absolutely need access to system catalog information.  
	** If we did this in all cases then the program would fail to audit 
	** a table that has been dropped (now we hex dump the table in 
	** this case).
	*/
	if (jsx->jsx_status & (JSX_AUDITFILE | JSX_FNAME_LIST))
	{
	    /*
	    ** open iirel_idx
	    */
	    catalog_id.db_tab_base = DM_B_RIDX_TAB_ID;
	    catalog_id.db_tab_index = DM_I_RIDX_TAB_ID;
	    tran_id.db_high_tran = 0;
	    tran_id.db_low_tran = 0;

	    status = dm2t_open(dcb, &catalog_id, DM2T_IS, DM2T_UDIRECT,
		DM2T_A_READ, (i4)0, (i4)20, 0, 0, 
		dmf_svcb->svcb_lock_list, (i4)0,(i4)0, 0, &tran_id,
		&timestamp, &r_idx, (DML_SCB *)0, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	    {
		SETDBERR(&jsx->jsx_dberr, 0, E_DM120B_ATP_CATALOG_ERROR);
		break;
	    }

	    /*
	    ** open iirelation
	    */
	    catalog_id.db_tab_base = DM_B_RELATION_TAB_ID;
	    catalog_id.db_tab_index = DM_I_RELATION_TAB_ID;
	    tran_id.db_high_tran = 0;
	    tran_id.db_low_tran = 0;

	    status = dm2t_open(dcb, &catalog_id, DM2T_IS, DM2T_UDIRECT, 
		DM2T_A_READ, (i4)0, (i4)20, 0, 0, 
		dmf_svcb->svcb_lock_list, (i4)0, (i4)0, 0, &tran_id, 
		&timestamp, &rel, (DML_SCB *)0, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	    {
		SETDBERR(&jsx->jsx_dberr, 0, E_DM120B_ATP_CATALOG_ERROR);
		break;
	    }

	    /*
	    ** Verify table existence and open output file for all
	    ** audited tables.
	    */
	    /*
	    ** bug 37246: if -u flag not set,
	    ** default to current user
	    */
	    if (jsx->jsx_username.db_own_name[0] == '\0')
	    {

        	char        *cp = (char *)&(jsx->jsx_username);

        	IDname(&cp);
                MEmove(STlength(cp), cp, ' ', 
		sizeof(jsx->jsx_username), jsx->jsx_username.db_own_name);
	    }
	    for (i = 0; i < jsx->jsx_tbl_cnt; i++)
	    {
		i2 file_type;
		i4 rec_len;

		dbachecked = FALSE;
		
		/*  Position table using table name and user id. */
		key_desc[0].attr_operator = DM2R_EQ;
		key_desc[0].attr_number = DM_1_RELIDX_KEY;
		key_desc[0].attr_value = (char *)&jsx->jsx_tbl_list[i].tbl_name;
		key_desc[1].attr_operator = DM2R_EQ;
		key_desc[1].attr_number = DM_2_RELIDX_KEY;

		/* jsx_username should be used if the user did not supply any 
		** table owner name. If the table owner is provided then the 
		** username should be the owner name provided
		*/		
	 
		if (STcmp((char *)
		    &jsx->jsx_tbl_list[i].tbl_owner.db_own_name,"" ) != 0)
		    key_desc[1].attr_value = 
			(char *)&jsx->jsx_tbl_list[i].tbl_owner;			
		else 
		    key_desc[1].attr_value = (char *)&jsx->jsx_username;
			

		for (;;)
		{
		    status = dm2r_position(r_idx, DM2R_QUAL, key_desc, 
			      (i4)2,
			      (DM_TID *)0, &jsx->jsx_dberr);
		    if (status != E_DB_OK)
			break;	    

		    /* 
		    ** Retrieve records with this key, should find one. 
		    */
		    status = dm2r_get(r_idx, &tid, DM2R_GETNEXT, 
			    (char *)&rindexrecord, &local_dberr);

		    if (status != E_DB_OK && local_dberr.err_code != E_DM0055_NONEXT)
		    {
			SETDBERR(&jsx->jsx_dberr, 0, E_DM120B_ATP_CATALOG_ERROR);
			break;
		    }
		    if (status == E_DB_OK)
		    {
			audit_tid.tid_i4 = rindexrecord.tidp;
			break;
		    }
		    else
		    {
			/* Try finding the table with dba name. */

			if (dbachecked == TRUE)
			    break;
			dbachecked = TRUE;
			key_desc[1].attr_value = (char *)&dcb->dcb_db_owner;
		    }

		} /* End for loop. */


		if (status != E_DB_OK)
		{
		    if (local_dberr.err_code == E_DM0055_NONEXT)
		    {
			STncpy( tmp_name,
			    jsx->jsx_tbl_list[i].tbl_name.db_tab_name, 
			    DB_TAB_MAXNAME);
			tmp_name[ DB_TAB_MAXNAME ] = '\0';
			STtrmwhite(tmp_name);
			uleFormat(NULL, E_DM1215_ATP_TBL_NONEXIST,
			    (CL_ERR_DESC *)NULL, ULE_LOOKUP, (DB_SQLSTATE *)0, 
			    error_buffer, ER_MAX_LEN, &error_length, &local_err, 
			    1, 0, tmp_name);
			error_buffer[error_length] = '\0';
			SIprintf("%s\n", error_buffer);

			/* Record that we can't find this table 
			** (kibro01) b121164 */
		        atp->atp_base_id[i] = -1;
		        atp->atp_auditfile[i] = NULL;
			one_table_bad = TRUE;
			status = E_DB_OK;
		        continue;
		    }
		    break;
		}

		/*
		** Now have table_id if it is a known table.  
		** Get the relation record for this table to find
		** out the table width.
		*/

		status = dm2r_get(rel, &audit_tid, DM2R_BYTID, (char *)&relation,
		    &local_dberr);
		if (status != E_DB_OK)
		{
		    STncpy( tmp_name,
			jsx->jsx_tbl_list[i].tbl_name.db_tab_name, 
			DB_TAB_MAXNAME);
		    tmp_name[ DB_TAB_MAXNAME ] = '\0';
		    STtrmwhite(tmp_name);
		    uleFormat(NULL, E_DM1215_ATP_TBL_NONEXIST,
			(CL_ERR_DESC *)NULL, ULE_LOOKUP, (DB_SQLSTATE *)0, 
			error_buffer, ER_MAX_LEN, &error_length, &local_err, 
			1, 0, tmp_name);
		    error_buffer[error_length] = '\0';
		    SIprintf("%s\n", error_buffer);

		    /* Record that we can't find this table (kibro01) b121164 */
		    atp->atp_base_id[i] = -1;
		    atp->atp_auditfile[i] = NULL;
		    one_table_bad = TRUE;
		    status = E_DB_OK;
		    continue;
		}
		
		atp->atp_base_id[i] = relation.reltid.db_tab_base;
		atp->atp_base_size[i] = relation.relwid;
		one_table_good = TRUE;

		/*
		** Create output files as necessary.
		*/
		if (jsx->jsx_status & JSX_AUDITFILE)
		{
		    /*
		    ** -t specifies a single table & hardcoded filename.
		    */
		    MEcopy("audit.trl", 10, buf);
		}
		else if ((jsx->jsx_status & JSX_FNAME_LIST) &&
			 (jsx->jsx_fname_cnt != 0)) 
		{
		    /*
			** A list of file names has been specified.
		    */
		    MEcopy((char *)&jsx->jsx_fname_list[i][0], 
			DI_PATH_MAX, buf);
		}
		else 
		{
		    /*	
		    ** -file specified without a filename list.
		    ** Build a .trl filename using the table name.
		    */ 

		    /*
		    ** Disallow automatic file name generation when table name
		    ** is delimited.   
		    */
		    if (jsx->jsx_tbl_list[i].tbl_delim)
		    {
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1217_ATP_DLM_FNAME);
			status = E_DB_ERROR;
			break;
		    }

		    status = build_trl_name(
			(char *)&jsx->jsx_tbl_list[i].tbl_name, buf);
		    if (status != E_DB_OK)
		    {
			SETDBERR(&jsx->jsx_dberr, 0, E_DM1210_ATP_BAD_TRL_CREATE);
			break;
		    }
		}

		LOfroms(PATH & FILENAME, buf, &audit_loc);
	        if (relation.relstat2 & TCB2_HAS_EXTENSIONS)
		{
		    file_type = SI_VAR;
		    rec_len = 8192;
		}
		else
		{
		    file_type = SI_BIN;
		    rec_len = ATP_HDR_SIZE + relation.relwid;
		}
		status = CPopen(&audit_loc, "w", file_type, rec_len,
		    &atp->atp_auditfile[i]);
		if (status != E_DB_OK)
		{
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1210_ATP_BAD_TRL_CREATE);
		    break;
		}
	    }

	    /* If there was an unknown table, then give an error message unless
	    ** there was at least one we can audit (kibro01) b121164
	    */
	    if (one_table_bad && !one_table_good)
	    {
		SETDBERR(&jsx->jsx_dberr, 0, E_DM120F_ATP_TBL_NOTFOUND);
		break;
	    }
	    if (status != E_DB_OK)
		break;

	    /* 
	    ** Close iirelation, iirel_idx
	    */
	    status = dm2t_close( rel, DM2T_NOPURGE, &jsx->jsx_dberr);
	    rel = NULL;
	    if (status != E_DB_OK)
		break; 
	    status = dm2t_close( r_idx, DM2T_NOPURGE, &jsx->jsx_dberr);
	    r_idx = NULL;
	    if (status != E_DB_OK)
		break; 
	}

	/*  Initialize ADF. */

	{
	    adf_cb.adf_dfmt = DB_US_DFMT;
	    adf_cb.adf_mfmt.db_mny_sym[0] = '$';
	    adf_cb.adf_mfmt.db_mny_sym[1] = '\0';
	    adf_cb.adf_mfmt.db_mny_lort = DB_LEAD_MONY;
	    adf_cb.adf_mfmt.db_mny_prec = 2;
	    adf_cb.adf_decimal.db_decspec = 1;
	    adf_cb.adf_decimal.db_decimal = '.';
	    adf_cb.adf_qlang = DB_QUEL;
	    adf_cb.adf_exmathopt = ADX_IGN_MATHEX;
	    adf_cb.adf_outarg.ad_c0width = -1;
	    adf_cb.adf_outarg.ad_i1width = -1;
	    adf_cb.adf_outarg.ad_i2width = -1;
	    adf_cb.adf_outarg.ad_i4width = -1;
	    adf_cb.adf_outarg.ad_i8width = -1;
	    adf_cb.adf_outarg.ad_f4width = 15;
	    adf_cb.adf_outarg.ad_f8width = 23;
	    adf_cb.adf_outarg.ad_f4prec  = 7;
	    adf_cb.adf_outarg.ad_f8prec  = 7;
	    adf_cb.adf_outarg.ad_f4style = 'g';
	    adf_cb.adf_outarg.ad_f8style = 'g';
	    adf_cb.adf_outarg.ad_t0width  = -1;
	    adf_cb.adf_maxstring = DB_MAXSTRING;
	    adf_cb.adf_collation = dcb->dcb_collation;
	    adf_cb.adf_ucollation = dcb->dcb_ucollation;
	    adf_cb.adf_utf8_flag = 0;

	    if (adf_cb.adf_ucollation)
	    {
		if (dcb->dcb_dbservice & DU_UNICODE_NFC)
		    adf_cb.adf_uninorm_flag = AD_UNINORM_NFC;
		else adf_cb.adf_uninorm_flag = AD_UNINORM_NFD;
	    }
	    else adf_cb.adf_uninorm_flag = 0;
	    adf_cb.adf_tzcb = dmf_svcb->svcb_tzcb;
	    adf_cb.adf_srv_cb = (PTR) dmf_getadf();
	    adf_cb.adf_unisub_status = AD_UNISUB_OFF;
	    adf_cb.adf_unisub_char[0] = '\0';
	}
	    
	status = adg_init(&adf_cb);
	if (status != E_DB_OK)
	{
	    /* Print text. */
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM120C_ATP_ADF_ERROR);
	    status = adg_shutdown(&adf_cb);
	    break;
	}

	/*
	** Setup the FEXI functions for large object processing.
	*/
	{
	    FUNC_EXTERN DB_STATUS	dmf_tbl_info();
	    FUNC_EXTERN DB_STATUS	dmf_last_id();
	    FUNC_EXTERN DB_STATUS	dmf_get_srs();
 
	    status = adg_add_fexi(&adf_cb, ADI_01PERIPH_FEXI, dmpe_call);
	    if (status != E_DB_OK)
	    {
		uleFormat( &adf_cb.adf_errcb.ad_dberror, 0, NULL, ULE_LOG,
			    NULL, NULL, 0, NULL, &local_err, 0 );
		SETDBERR(&jsx->jsx_dberr, 0, E_DM120C_ATP_ADF_ERROR);
		break;
	    }

	    status = adg_add_fexi(&adf_cb, ADI_03ALLOCATED_FEXI, dmf_tbl_info);
	    if (status != E_DB_OK)
	    {
		uleFormat( &adf_cb.adf_errcb.ad_dberror, 0, NULL, ULE_LOG,
			    NULL, NULL, 0, NULL, &local_err, 0 );
		SETDBERR(&jsx->jsx_dberr, 0, E_DM120C_ATP_ADF_ERROR);
		break;
	    }
 
	    status = adg_add_fexi(&adf_cb, ADI_04OVERFLOW_FEXI, dmf_tbl_info);
	    if (status != E_DB_OK)
	    {
		uleFormat( &adf_cb.adf_errcb.ad_dberror, 0, NULL, ULE_LOG,
			    NULL, NULL, 0, NULL, &local_err, 0 );
		SETDBERR(&jsx->jsx_dberr, 0, E_DM120C_ATP_ADF_ERROR);
		break;
	    }
	    status = adg_add_fexi(&adf_cb, ADI_07LASTID_FEXI, 
				  dmf_last_id );
	    if (status != E_DB_OK)
	    {
		uleFormat( &adf_cb.adf_errcb.ad_dberror, 0, NULL, ULE_LOG,
			    NULL, NULL, 0, NULL, &local_err, 0 );
		SETDBERR(&jsx->jsx_dberr, 0, E_DM120C_ATP_ADF_ERROR);
		break;
	    }
	    status = adg_add_fexi(&adf_cb, ADI_08GETSRS_FEXI,
				  dmf_get_srs );
	    if (status != E_DB_OK)
	    {
		uleFormat(&adf_cb.adf_errcb.ad_dberror, 0,
		    0, ULE_LOG, NULL, NULL, 0, NULL, &local_err, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM120C_ATP_ADF_ERROR);
		break;
	    }
	}
	
	/*
	** Init the logging system, required only for -wait option 
	*/
	if (jsx->jsx_status1 & JSX_ATP_WAIT)
	{
	    status = LGinitialize(&sys_err, jsx->jsx_lgk_info);
	    if (status)
	    {
		(VOID) uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &local_err, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM9011_BAD_LOG_INIT);
		break;
	    }
	}
	return (E_DB_OK);
    }    

    /*	
    ** Error has been detected.
    **
    ** If we left the config file open, then close it.
    */
    if (cnf)
    {
        status = dm0c_close(cnf, 0, &local_dberr);
        if (status != E_DB_OK)
        {
            uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
		(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		&local_err, 0);
        }
    }
    if ( rel )
	dm2t_close(rel, DM2T_NOPURGE, &local_dberr);
    if ( r_idx )
	dm2t_close(r_idx, DM2T_NOPURGE, &local_dberr);

    if (jsx->jsx_dberr.err_code == E_DM1200_ATP_DB_NOT_JNL)
	return(E_DB_INFO);

    return (E_DB_ERROR);
}

/*{
** Name: build_trl_name		- construct auditdb .trl file name
**
** Description:
**      Given the name of the target table, construct the the of the
**	audit file used to hold auditdb output.  If the table name
**	contains unprintable characters or if the table name is larger 
**	than allowed, a warning is returned.
**
** Inputs:
**      tbl_name			The name of the table to audit
**
** Outputs:
**      trl_filename			The name of file to create.
**	Returns:
**	    E_DB_WARN			if error creating file name
**	    E_DB_OK 			if no error 
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-apr-1993 (jnash)
**           Created in support of auditing multiple tables.
*/
static DB_STATUS
build_trl_name(
char		*tbl_name,
char		*trl_filename)
{
    char	*tmp_ptr;
    char	tmp_char;
    i4	i = 0;
    i4	tbl_name_len  = DB_TAB_MAXNAME;

    /*
    ** Find length of the table name.
    */
    tmp_ptr = tbl_name + DB_TAB_MAXNAME - 1;
    while (tmp_ptr >= tbl_name && *tmp_ptr-- == ' ')
	--tbl_name_len;

    /*
    ** If the table name length is too large for this operating system,
    ** (including the .trl suffix) or if it contains unprintable characters, 
    ** return error.
    */
    if (tbl_name_len + 4 > DI_PATH_MAX)
    {
	return (E_DB_WARN);
    }

    tmp_ptr = tbl_name;
    while (i++ < tbl_name_len)
    {
	/*
	** Test for invalid characters in file name (CMprint is a macro).
	** XXX Change with delim identifiers?? XXX
	*/
	tmp_char = *tmp_ptr++;
	if (!CMprint(&tmp_char))
	{
	    return (E_DB_WARN);
	}
    }

    /*
    ** Move table_name.trl into output buffer.
    */
    MEcopy(tbl_name, tbl_name_len, trl_filename);
    MEcopy(".trl\0", 5, &trl_filename[tbl_name_len]);

    return (E_DB_OK);
}

/*{
** Name: write_line	- Write line to report.
**
** Description:
**      Write line to report, if line is longer then 132 characters
**	then wraparound.  Also print a header on every page.
**
**	This routine is not used to write to a binary ".trl" file.
**
** Inputs:
**      atp                             Audit trail context.
**	line				Pointer to line to print.
**	l_line				Length of line to print.
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
**      19-nov-1986 (Derek)
**           Created for Jupiter.
**	08-dec-1989 (sandyh)
**	     Added local static to keep track of database id so that we
**	     start a new report page when a new database is audited. This
**	     occurs when either no database is specified on the command
**	     line or several are, bug 9089.
**	18-feb-1992 (bryanp)
**	    time_stamp is a TM_STAMP, not a i4.
**      26-jan-2000 (gupsh01)
**          Added code for flags -no_header for auditdb.
[@history_template@]...
*/
static VOID
write_line(
DMF_ATP	    *atp,
char	    *line,
i4	    l_line)
{
    i4	    count;
    i4	    state;
#define			NORMAL	    1L
#define			LONG_LINE   2L
    TM_STAMP	    time_stamp;
    static i4  local_db_id;

    if ((atp->atp_lineno == 0) && (atp->atp_pageno == 0))
	local_db_id = atp->atp_dcb->dcb_id;

    for (state = NORMAL; l_line;)
    {
	if (local_db_id != atp->atp_dcb->dcb_id) /* reset if new database */
	{
	    local_db_id = atp->atp_dcb->dcb_id;
	    atp->atp_lineno = 0;
	}

	/*
	** If -no_header provided then do not write header.
	*/

	if (((atp->atp_lineno % 60) == 0) &&
			!(atp->atp_jsx->jsx_status2 & JSX2_NO_HEADER))
	{
	    if (atp->atp_pageno == 0)
	    {
		TMget_stamp((TM_STAMP *)&time_stamp);
		TMstamp_str((TM_STAMP *)&time_stamp, atp->atp_date);
	    }

	    atp->atp_pageno++;
	    SIprintf("\fAudit for database %*s\t\t\t\t%s\t\t\tPage %4d\n\n",
		    sizeof(atp->atp_dcb->dcb_name),
		    &atp->atp_dcb->dcb_name, atp->atp_date, atp->atp_pageno);
	    atp->atp_lineno = 2;
	}

	switch (state)
	{
	case NORMAL:

	    if (l_line <= 132)
	    {
		SIwrite(l_line, line, &count, stdout);
		SIwrite(1, "\n", &count, stdout);
		l_line = 0;
	    }
	    else
	    {
		SIwrite(132, line, &count, stdout);
		SIwrite(1, "\n", &count, stdout);
		line += 132;
		l_line -= 132;
		state = LONG_LINE;
	    }    
	    atp->atp_lineno++;
	    break;

	case LONG_LINE:
	    if (l_line <= 116)
	    {
		SIwrite(16, "                ", &count, stdout);
		SIwrite(l_line, line, &count, stdout);
		SIwrite(1, "\n", &count, stdout);
		l_line = 0;
	    }
	    else
	    {
		SIwrite(16, "                ", &count, stdout);
		SIwrite(116, line, &count, stdout);
		SIwrite(1, "\n", &count, stdout);
		line += 116;
		l_line -= 116;
	    }    
	    atp->atp_lineno++;
	    break;
	}
    }
}

/*{
** Name: write_tuple	- Write a tuple to the report.
**
** Description:
**      Write a tuple to the record.  Either format using a description from
**	the database, or display in hex.
**
** Inputs:
**      force_hexdump			If non-zero, force hexdump of tuple
**      atp                             Pointer to ATP context.
**      prefix                          Prefix for the record.
**      table_id                        Table id.
**	record				Pointer to record.
**	rep_type			In the case of DM0L_REP, this
**					reflects whether the given record
**					is the old data (1) or the new
**					data (2). Otherwise, it is zero.
**	size				Size of the record.
**	comptype			The (logged) compression type.
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
**      19-nov-1986 (Derek)
**           Created for Jupiter.
**	22-may-1989 (Sandyh)
**	     Fixed bug 5736 - Handling of NULLs in record string.
**      14-jul-89 (fred)
**	    Altered write_tuple() so that it will correctly pass the potential
**	    length of the output string to adc_cvinto().  Instead of the amount
**	    of space left, it was passing the amount of space used so far...
**	26-dec-89 (greg)
**	    Use I2ASSIGN_MACRO
**	17-may-90 (rogerk)
**	    Added fixes for byte_align problems.  Convert all attributes
**	    into aligned storage allocated in prepare.  If the source column
**	    itself needs alignment then copy to aligned storage before
**	    the conversion.  Also added checks for overflowing the display
**	    buffer.  Added expand_displaybuf routine to allocate bigger
**	    display buffer - if we can't allocate a bigger buffer then
**	    truncate the row being formatted.
**	16-aug-1991 (jnash) merged 24-sep-90 (rogerk)
**	    Merge above fixes with B1 changes.  Include byte_align fixes
**	    in special case logical key columns.
**	18-feb-1992 (bryanp)
**	    Can't dereference or do arithmetic on PTR variables.
**	08-feb-93 (jnash)
**	    Don't dump CLRs, which appear here as zero length tuples.
**	26-jul-93 (rogerk)
**	    Moved code that looks-up/builds table descriptors to a new routine:
**		get_table_descriptor.
**	    Added support for compressed log rows in log records.
**	    Added atp_uncompbuf, used to uncompress rows before using adc to
**		convert them to displayable types.
**	    Added call to find tuple-level accessors and to uncompress rows
**		belonging to compressed tables.
**	18-oct-93 (jnash)
**	    Add support for replace log record row compression.  Add 
**	    force_hexdump flag, used when old rows are compressed.
**	06-may-1996 (thaju02)
**	    Add page size parameter to be passed to dm1c_getaccessors()..
**	13-sep-1996 (canor01)
**	    Add NULL tuple buffer to dmpp_uncompress call.
**	29-sep-1998 (nicph02)
**	    Logical_key columns consistently displayed in HEX format
**	    (bug 45752)
**	21-Aug-1998 (kitch01)
**	    Bug 92590. Decimal fields were not being converted to the char
**	    output field correctly because the precision of the from field
**	    was being set to zero. Amended so that this precision is set
**	    to the attributes precision.
**      26-jan-2000 (gupsh01)
**          Added code for -limit_output for auditdb. This will restrict
**	    the output to a single line and will also check for
**	    non-printable characters.
**	26-mar-2001 (somsa01)
**	    In the case of large objects, call atp_lobj_cpn_redeem() to
**	    properly grab he large object.
**	19-jul-2001 (stephenb)
**	    Fix SEGV when auditing blobs, caused by bad memory copy
**	03-Sep-2003 (hanal04) Bug 110838 INGSRV 2500
**	    Restore the test for a null td before trying to
**	    uncompress a record.
**	19-apr-2004 (gupsh01)
**	    Add adf_cb to call to dmpp_compress compression functions.
**	11-nov-2004 (gupsh01)
**	    Fixed auditdb -limit_output option.
**      30-Jan-2003 (hanal04) Bug 109530 INGSRV 2077
**          Added journal record parameter so that we can determine whether 
**          the current table descriptor (td) has a different row version
**          to the journal record. If this is the case we must convert
**          the row to the current format to safely display NULL values
**          added by ALTER TABLE or skip columns dropped by ALTER TABLE. 
**          This support has been added for DM0L_PUT and DM0L_DEL records.
**          As we may now be trying to display NULL values the to.db_datatype
**          has been modified to -DB_LTXT_TYPE in order to prevent 
**          E_AD1012 in adc_cvinto().
**          dm1r_cvt_row() will strip dropped columns and so we should
**          skip acress dropped columns when reading the generated
**          record buffer.
**          Also note that additional support for nullable dates was added
**          to dm1r_cvt_row().
**
**       1-Jul-2003 (hanal04) Bug 110505 INGSRV 2399
**          Change to.db_datatype back to DB_LTXT_TYPE as E_AD1012 is
**          handled by this function and using -DB_LTXT_TYPE causes
**          values to be displayed in place of NULLs.
**	13-Feb-2008 (kschendel) SIR 122739
**	    Revise uncompress call, remove record-type and tid.
*/
static VOID
write_tuple(
i4	    force_hexdump,
DMF_ATP     *atp,
char	    *prefix,
DB_TRAN_ID  *tran_id,
DB_TAB_ID   *table_id,
PTR	    *record,
i2	    rep_type,
i4	    size,
i2	    comptype,
PTR         jrecord)
{
    DMF_JSX		*jsx = atp->atp_jsx;
    ATP_TD		*td = NULL;
    char		*r;
    char		*l;
    char		*line_buffer;
    char		*data_ptr;
    DB_DATA_VALUE	from;
    DB_DATA_VALUE	to;
    DB_STATUS   s = E_DB_OK;
    i4          type;               /*  Type of log record. */
    u_i2        row_version;
    bool	convert_row = FALSE;
    i4			i;
    i4			bytes_used;
    i4			space_left;
    i4			space_needed;
    i4			uncompressed_length;
    u_i2		length;			/* Must be an I2 */
    DB_STATUS		status = E_DB_OK;
    i4			local_err;

    CLRDBERR(&jsx->jsx_dberr);

    type = ((DM0L_HEADER *)jrecord)->type;

    /*
    ** Don't dump log records which do not contain a copy of the updated row.
    ** This is true of some CLR records.
    ** CLRs are flagged as such in the header, so we don't dump them.
    */ 
    if (size == 0)
	return;

    /*
    ** Get table descriptor to use for formatting the tuple.  If no table 
    ** descriptor is available, then we resort to hex dumping to row.
    ** Don't bother attempting to find table descriptor if we are
    ** forcing a hexdump.
    */
    if (! force_hexdump)
	td = get_table_descriptor(atp, table_id);

    /*
    ** Initialize the record buffer.
    ** Make sure there is enough room for the prefix information (4 bytes 
    ** spacing + 8 byte prefix + open bracket) - there should always be
    ** enough space but just to be safe ....
    **
    ** If there is no tuple discriptor, then make sure there is enough
    ** space for the hex dump.   We do not check for space for rows which
    ** will be formatted as we don't know the space requirements until we
    ** start processing the columns.
    */
    space_left = atp->atp_sdisplaybuf;
    space_needed = 13;
    if (td == NULL)
	space_needed += size * 2;
    if (space_left < space_needed)
    {
	status = expand_displaybuf(atp, space_needed, 0);
	if (DB_FAILURE_MACRO(status))
	{
	    dmfWriteMsg(NULL, E_DM1212_ATP_DISPLAY, 0);
	    return;
	}
    }
    line_buffer = atp->atp_displaybuf;
    l = line_buffer;
    r = (char *)record;

    /*
    ** If the table is compressed, then uncompress the record and change
    ** our record pointer to point into the uncompress buffer.
    */
    if (td && (comptype != TCB_C_NONE))
    {
	/* Reset rowaccessor if journal record compression type doesn't
	** match what it is currently set for.  The compression-control
	** array OUGHT to be large enough for any kind of compression.
	*/
	if (comptype != td->data_rac.compression_type)
	{
	    td->data_rac.compression_type = comptype;
	    td->data_rac.cmp_control = td->cmp_control;
	    td->data_rac.control_count = td->control_size;
	    status = dm1c_rowaccess_setup(&td->data_rac);
	    if (status != E_DB_OK)
	    {
		TRdisplay("Row uncompression setup error on table (%d, %d)\n",
			table_id->db_tab_base, table_id->db_tab_index);
		TRdisplay("\tLogged compression type: %d\n", comptype);
		dmfWriteMsg(NULL, E_DM1212_ATP_DISPLAY, 0);
		return;
	    }
	}
	status = (*td->data_rac.dmpp_uncompress)(&td->data_rac,
	    r, atp->atp_uncompbuf, td->row_width, &uncompressed_length,
	    NULL, td->row_version, &adf_cb);
	if (DB_FAILURE_MACRO(status))
	{
	    TRdisplay("Row uncompression error on table (%d, %d)\n",
		table_id->db_tab_base, table_id->db_tab_index);
	    TRdisplay("\tTable width: %d, uncompressed length: %d\n",
		td->row_width, uncompressed_length);
	    dmfWriteMsg(NULL, E_DM1212_ATP_DISPLAY, 0);
	    return;
	}

	r = atp->atp_uncompbuf;
	size = uncompressed_length;
    }
    else
    {
        /* Other journal records which contain row_version details
	** can be added to this switch.
	*/
        switch (type)
        {
        case DM0LPUT:
	    row_version = (u_i2)((DM0L_PUT *)jrecord)->put_row_version;
	    if(td && (td->row_version != row_version))
	    {
		convert_row = TRUE;
	    }
	    break;

        case DM0LDEL:
	    row_version = (u_i2)((DM0L_DEL *)jrecord)->del_row_version;
	    if(td && (td->row_version != row_version))
	    {
		convert_row = TRUE;
	    }
	    break;

        default:
	    break;
        }

	if(convert_row == TRUE)
	{
            s = dm1r_cvt_row(td->data_rac.att_ptrs, td->data_rac.att_count, r,
	        atp->atp_uncompbuf, td->row_width, &uncompressed_length,
	        row_version, &adf_cb);
            if (s != E_DB_OK)
	    {
                return;
	    }
	    r = atp->atp_uncompbuf;
	    size = uncompressed_length;
	}
    }

    /*
    ** Add the prefix formatting.
    */
    MEcopy("    ", 4, l);
    MEcopy(prefix, 8, l + 4);
    l += 12;

    /*
    ** If there is no tuple descriptor, then just format a hex dump
    ** of the row.
    */
    if (td == 0)
    {
	/*
	** Hex dump the record.  We have already verified that there
	** is sufficient space in the display buffer.
	*/
	for (i = 0; i < size; i++)
	{
	    *l++ = ((char *)"0123456789abcdef")[(*r >> 4) & 0xf];
	    *l++ = ((char *)"0123456789abcdef")[*r++ & 0x0f];
	}

	write_line(atp, line_buffer, 12 + size * 2);
	return;
    }

    /*
    ** Cycle through the attributes, formatting each one into the
    ** display buffer.
    */
    for (i = 1; i <= td->data_rac.att_count; i++)
    {
	/* Mark the beginning of the record.
	** We have already guaranteed that there is space in the display buffer.
	*/
	if (i == 1)
	    *l++ = '<';

	/* If we called dm1r_cvt_row() drop columns will have been
	** removed from the output buffer.
	*/
        if (convert_row && td->att_array[i].ver_dropped &&
		 (td->att_array[i].ver_dropped <= td->row_version))
	{
	    continue;
	}

	/*
	** Convert the attribute into a displayable type.  We use longtext.
	** If byte alignment is required, then copy the column to an aligned
	** buffer before calling adc.  Both the input and output buffers
	** for the conversion must be properly aligned.
	*/
	to.db_datatype = DB_LTXT_TYPE;
	to.db_prec = 0;
	to.db_length = DB_MAXTUP;
	to.db_data = atp->atp_rowbuf;
	to.db_collID = -1;

	from.db_datatype = td->att_array[i].type;
	from.db_prec = td->att_array[i].precision;
	from.db_length = td->att_array[i].length;
	from.db_data = r;
	from.db_collID = td->att_array[i].collID;

#ifdef BYTE_ALIGN
	if (ME_NOT_ALIGNED_MACRO(r))
	{
	    MEcopy((PTR) r, td->att_array[i].length, 
		(PTR) atp->atp_alignbuf);
	    from.db_data = atp->atp_alignbuf;
	}
#endif /* BYTE_ALIGN */

	/*
	** Convert datatype to its LONGTEXT representation so it can
	** be put into the display buffer.
	**
	** If this is a logical key type, then instead of converting it
	** to longtext, convert it to a HEX string (the ascii representation
	** of one).
	*/
	if (td->att_array[i].type == DB_TABKEY_TYPE
		|| td->att_array[i].type == -DB_TABKEY_TYPE
		|| td->att_array[i].type == DB_LOGKEY_TYPE
		|| td->att_array[i].type == -DB_LOGKEY_TYPE)
	{
	    /*
	    ** Convert datatype to HEX string for display.  Logical Key
	    ** datatypes don't convert very well to longtext.
	    */
	    status = adc_kcvt(&adf_cb, ADC_0001_FMT_SQL, &from,
			    to.db_data, &to.db_length);

	    /*
	    ** Set data pointer and length fields.
	    */
	    length = (u_i2)to.db_length;
	    data_ptr = to.db_data;
	}
	else if (td->att_array[i].flag & ATT_PERIPHERAL)
	{
	    if (atp->atp_jsx->jsx_status2 & JSX2_EXPAND_LOBJS)
	    {
		/*
		** This is a large object type. Thus, we have to basically
		** "redeem" the base table's coupon.
		*/
		status = atp_lobj_cpn_redeem(atp, tran_id, table_id,
					     &from, &to, rep_type);

		/*
		** Set data pointer and length fields.
		** Get the length of the text from the longtext count field.
		** The actual data starts after the length value.
		*/
		I2ASSIGN_MACRO(*(i2 *)to.db_data, length);
        	data_ptr = (char *)to.db_data + DB_CNTSIZE;
	    }
	    else
	    {
		char	cpn_label[32] = "{LOBJ COUPON}";

		/*
		** We have been told to NOT expand the large object. Thus,
		** put out a label to say that this is a coupon.
		*/
		length = STlength(cpn_label);
		MEcopy(&length, DB_CNTSIZE, (PTR)to.db_data);
		MEcopy( &cpn_label, STlength(cpn_label),
			(PTR)(to.db_data + DB_CNTSIZE) );

		/*
		** Set data pointer field.
		*/
		data_ptr = (PTR)to.db_data + DB_CNTSIZE;
	    }
	}
	else
	{
	    status = adc_cvinto(&adf_cb, &from, &to);

	    /*
	    ** Set data pointer and length fields.
	    ** Get the length of the text from the longtext count field.
	    ** The actual data starts after the length value.
	    */
	    I2ASSIGN_MACRO(*(i2 *)to.db_data, length);
            data_ptr = (char *)to.db_data + DB_CNTSIZE;
	}

	/*
	** Check for conversion error.
	** OK to get warning because value was NULL (bug 5736).
	*/
	if (DB_FAILURE_MACRO(status))
	{
	    if (adf_cb.adf_errcb.ad_errcode != E_AD1012_NULL_TO_NONNULL)
	    {
		dmfWriteMsg(&adf_cb.adf_errcb.ad_dberror, 0, 0);
		uleFormat(NULL, E_DM1213_ATP_COLUMN_DISPLAY, (CL_ERR_DESC *)NULL, 
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
		    &local_err, 1, 0, i);
	    }

	    /*
	    ** If no column to output, set length to zero so no data
	    ** will be copied.  We will still write a column delimiter
	    ** however.
	    */
	    length = 0;
	}

	/*
	** Check for available space in the display buffer.  Include the
	** column delimeter at the end.
	**
	** If we don't have space and can't get space to display this row,
	** then display as much as we can and append a TRUNCATED message.
	*/

	if (!(atp->atp_jsx->jsx_status2 & JSX2_LIMIT_OUTPUT))	
		space_left = atp->atp_sdisplaybuf - (l - line_buffer);
	else
	{
        /* Replace any non printable characters by # */ 

	char *p = data_ptr;
	i4 bcnt = 0;
	
	while ((p - data_ptr) < length)
	{
	  if (!CMprint(p) || CMcntrl(p))
	  {
	    for (bcnt = CMbytecnt(p); bcnt > 0; bcnt--)
	    {
	      *p = '#';
	      p++;
	    }
	  }
	  else
	    CMnext(p);
	}
	/*
	** we will truncate the output to 80 characters. save 4 bytes
	** for "...>" at the end of the record line to indicate 
	** truncated record.
	*/
	
	space_left = ATP_MAX_LINESIZE - (l - line_buffer) - 4;
	}

	if (space_left < (i4)(length + 1))
	{
	    /*
	    ** Allocate a new buffer.  The allocation routine will
	    ** move the used space over to the new buffer.
	    */
	    bytes_used = atp->atp_sdisplaybuf - space_left;
	    
	    if (!(atp->atp_jsx->jsx_status2 & JSX2_LIMIT_OUTPUT)) 
	    status = expand_displaybuf(atp, length + 1, bytes_used);
	   
	    if (DB_FAILURE_MACRO(status) || 
			(atp->atp_jsx->jsx_status2 & JSX2_LIMIT_OUTPUT))
	    {
		/*
		** If we can't get more space, then write as much of the row
		** as we can and output a truncated message.
		*/
		length = (u_i2)space_left;
		MEcopy((PTR)data_ptr, length, (PTR)l);
		l += length;

		/*
		** Now go back and write truncated message over the end of
		** the display output.
		*/
		if (!(atp->atp_jsx->jsx_status2 & JSX2_LIMIT_OUTPUT))
 		{		
		MEcopy((PTR) ATP_ROW_TRUNCATED_MESSAGE,
		       sizeof(ATP_ROW_TRUNCATED_MESSAGE),
		       (PTR) (l - sizeof(ATP_ROW_TRUNCATED_MESSAGE)));
		}
		else
		{
		   /* Insert the ...> to indicate a truncated record */
 
		    MEcopy("...>", 4, l);
		    l += 4;
		}
		/*
		** Output the row with truncated message and return.
		*/
		write_line(atp, line_buffer, l - line_buffer);
		return;
	    }
	    line_buffer = atp->atp_displaybuf;
	    l = line_buffer + bytes_used;
	}

	/*
	** Copy text string to display buffer.
	*/
	if (length)
	{
	    MEcopy((PTR)data_ptr, length, (PTR)l);
	    l += length;
	}

	/*
	** Write column delimiter.
	*/
	if (i == td->data_rac.att_count)
	    *l++ = '>';
	else
	    *l++ = '|';

	/*
	** Increment record pointer to point to the beginning of the
	** next column.
	*/
	r += td->att_array[i].length;
    }

    /*	Write the record. */

    write_line(atp, line_buffer, l - line_buffer);
}

/*{
** Name: get_table_descriptor	- return a row attribute descriptor
**
** Description:
**	Returns a table descriptor which gives a list of the attributes
**	in the table with the specified table id.
**
**	The descriptor is looked up in the current list of descriptors.
**	If not found an attempt is made to build a new descriptor from
**	system catalog information.
**
**	If the table is not found in the system catalogs then no TD is
**	returned (the call returns a NULL value).
**
** Inputs:
**      atp                             Pointer to ATP context.
**      table_id                        Table id.
**
** Outputs:
**	Returns:
**	    NULL			Table not described in catalogs.
**	    Table Descriptor		Pointer to valid Table Descriptor
**
** Exceptions:
**	none
**
** Side Effects:
**	none
**
** History:
**	26-jul-1993 (rogerk)
**	    Created new routine from code in write_tuple.
**	    Added lookups into iirelation for tuple compression information.
**	    Changed attribute descriptors dramatically.  Instead of just
**		storing type and length info for each attribute, we now carry
**		around a full DB_ATTS array - which is needed by the uncompress
**		routines.  Changed the attribute array to be allocated
**		dynamically and be variable length to prevent the TD cache
**		from expanding from 64K to 1M.
**	    Added att_ptr_list array as well with pointers to the att entries.
**		This is required for the interface to the compression routines.
**	 4-jul-96 (nick)
**	    Ensure we take shared locks on the catalogs.
**      12-dec-1996 (cohmi01)
**          Add detailed err messages for corrupt system catalogs (Bug 79763).
**      15-jul-1996 (ramra01 for bryanp)
**          Build new DB_ATTS values for new iiattribute fields.
**	16-mar-2000 (thaju02)
**	    attintl_id is initialized to zero for catalogs, thus
**	    causing auditdb -a to report E_AD2004, E_DM1213.
**	    If attintl_id is 0, use attid value. (b92143)
**	03-mar-2004 (gupsh01)
**	    initialized attver_altcol to support alter table alter column.
**	13-Apr-2009 (thaju02) 
**	    If partition, build table descriptor from master. (B121912)
[@history_template@]...
*/
static ATP_TD *
get_table_descriptor(
DMF_ATP     *atp,
DB_TAB_ID   *table_id)
{
    DMF_JSX		*jsx = atp->atp_jsx;
    ATP_TD		*td = NULL;
    DMP_RCB		*rel_rcb = NULL;
    DMP_RCB		*attr_rcb = NULL;
    DMP_MISC		*mem_ptr;
    DB_ATTS		*att;
    DB_TRAN_ID		tran_id;
    DMP_ATTRIBUTE	attrrecord;
    DMP_RELATION	rel_record;
    DM2R_KEY_DESC	key_desc[2];
    DM_TID		tid;
    i4		data_cmpcontrol_size;
    i4		mem_needed;
    i4		attr_count;
    i4		i;
    i4		local_err;
    DB_STATUS		status = E_DB_OK;
    i2			attid;
    i4			idx;
    DB_ERROR		local_dberr;
    i4			attnmsz;
    char		*nextattname;
    DB_ATTS		*curatt;
    char		*p;
    i4			alen;


    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Look for row descriptor in the current descriptor cache.
    ** Hash the table id to find the slot in which the descriptor
    ** would be held if there were one.
    */
    i = table_id->db_tab_base & (ATP_TD_CNT - 1);
    if (atp->atp_td[i].table_id.db_tab_base == table_id->db_tab_base)
    {
	td = &atp->atp_td[i];
	return (td);
    }

#ifdef xDEBUG
    /*
    ** Pre 6.5 AUDITDB scanned the atp_td array to find table descriptors
    ** rather than just hashing to the proper slot.  To make sure I am
    ** not missing some subtle dependency for this behaviour, make sure
    ** that scanning the array would not have succeeded when a direct
    ** hash probe failed.
    */
    for (i = 0; i < ATP_TD_CNT; i++)
    {
	if (atp->atp_td[i].table_id.db_tab_base == table_id->db_tab_base)
	{
	    TRdisplay("AUDITDB: Warning: found Table Descriptor by Scan\n");
	    td = &atp->atp_td[i];
	    return (td);
	}
    }
#endif

    /*
    ** Table Descriptor not found - need to allocate a new one (if possible).
    ** Hash the table id to find a slot in the table descriptor array.
    **
    ** Zero out the current table description (if any) stored there.  If
    ** we error or break out of the code below before finishing the building
    ** of the descriptor we expect that the descriptor table_id fields will
    ** still be zeroed.
    */
    i = table_id->db_tab_base & (ATP_TD_CNT - 1);
    td = &atp->atp_td[i];

    td->table_id.db_tab_base = 0;
    td->table_id.db_tab_index = 0;

    /*
    ** Delete any previous attribute array in this table descriptor.
    */
    if (td->att_array)
    {
	td->att_array = (DB_ATTS *)((char *)td->att_array - sizeof(DMP_MISC));
	(VOID) dm0m_deallocate((DM_OBJECT **) &td->att_array);
    }
    MEfill(sizeof(DMP_ROWACCESS), 0, &td->data_rac);

    for(;;)
    {
	tran_id.db_high_tran = 0;
	tran_id.db_low_tran = 0;

	/**************************************************************
	**
	** Lookup the table in IIRELATION
	** Fill in the table compression information.
	**
	**************************************************************/

	status = dm2r_rcb_allocate(atp->atp_dcb->dcb_rel_tcb_ptr, (DMP_RCB *)0,
	    &tran_id, dmf_svcb->svcb_lock_list, 0, &rel_rcb, &local_dberr);
	if (status != E_DB_OK)
	    break;

	/*  Initialize the RCB. */

	rel_rcb->rcb_lk_mode = RCB_K_IS;
	rel_rcb->rcb_k_mode = RCB_K_IS;
	rel_rcb->rcb_access_mode = RCB_A_READ;
	rel_rcb->rcb_lk_id = dmf_svcb->svcb_lock_list;
    
	key_desc[0].attr_operator = DM2R_EQ;
	key_desc[0].attr_number = DM_1_RELATION_KEY;
	key_desc[0].attr_value = (char *)&table_id->db_tab_base;

	status = dm2r_position(rel_rcb, DM2R_QUAL, key_desc, (i4)1,
	      (DM_TID *)0, &local_dberr);
	if (status != E_DB_OK)
	    break;

	/* if table is a partition, build tuple desc from master */
	idx = ((table_id->db_tab_index & DB_TAB_PARTITION_MASK) ?
		0 : table_id->db_tab_index);

	for (;;)
	{
	    /* 
	    ** Get a qualifying iirelation record.  
	    */
	    status = dm2r_get(rel_rcb, &tid, DM2R_GETNEXT, 
		(char *)&rel_record, &local_dberr);
	    if (status != E_DB_OK)
		break;

	    if ((rel_record.reltid.db_tab_base != table_id->db_tab_base) ||
	    	(rel_record.reltid.db_tab_index != idx))
		continue;

	    td->relstat2 = rel_record.relstat2;
	    td->data_rac.compression_type = rel_record.relcomptype;
	    td->row_width = rel_record.relwid;
	    td->data_rac.att_count = rel_record.relatts;
	    td->lobj_att_count = 0;
	    td->row_version = rel_record.relversion;	
	    break;
	}

	/*
	** If the table was not found, then set status to OK since it
	** is not an error condition and break to below to return
	** without a table descriptor.
	*/
	if (status != E_DB_OK)
	{
	    if (local_dberr.err_code == E_DM0055_NONEXT)
		status = E_DB_OK;
	    break;
	}

	status = dm2r_release_rcb(&rel_rcb, &local_dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** Now that we know the number of attributes in the table,
	** allocate the attribute array.
	**
	** We allocate a DB_ATTS array to hold attribute information and
	** a pointer list in which each pointer entry points to the
	** corresponding DB_ATTS array entry.
	**
	** We allocate one extra DB_ATTS entry in the att_array because
	** it is indexed from 1 to att_count.  The row-accessor att_ptrs
	** array is indexed 0 to att_count-1.
	**
	** Because the log records might have a different compression type
	** from how the table is defined now, compute the worst-case
	** control array needed for this number of attributes, and allocate
	** that.  (assuming row-versioning needed, too!)
	*/
	data_cmpcontrol_size = dm1c_cmpcontrol_size(
			TCB_C_NONE, td->data_rac.att_count, 1);
	i = dm1c_cmpcontrol_size(
			TCB_C_STANDARD, td->data_rac.att_count, 1);
	if (i > data_cmpcontrol_size)
	    data_cmpcontrol_size = i;
	i = dm1c_cmpcontrol_size(
			TCB_C_HICOMPRESS, td->data_rac.att_count, 1);
	if (i > data_cmpcontrol_size)
	    data_cmpcontrol_size = i;

	/* *Note* New compression type?  add a trial for it too. */

	data_cmpcontrol_size = DB_ALIGN_MACRO(data_cmpcontrol_size);

	attnmsz = (td->data_rac.att_count + 1) * sizeof(DB_ATT_STR);
	attnmsz = DB_ALIGN_MACRO(attnmsz);

	mem_needed = sizeof(DMP_MISC) + 
		((td->data_rac.att_count + 1) * sizeof(DB_ATTS)) + 
		(td->data_rac.att_count * sizeof(DB_ATTS *)) +
		data_cmpcontrol_size + 
		attnmsz;

	status = dm0m_allocate(mem_needed, (i4)0, (i4)MISC_CB, 
	    (i4)MISC_ASCII_ID, (char *)NULL, (DM_OBJECT**)&mem_ptr, &jsx->jsx_dberr);
	if (DB_FAILURE_MACRO(status))
	{
	    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    uleFormat(NULL, E_DM9425_MEMORY_ALLOCATE, (CL_ERR_DESC *)NULL, ULE_LOG, 
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 1,
		0, mem_needed);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1211_ATP_MEM_ALLOCATE);
	    break;
	}
	td->att_array = (DB_ATTS *)((char *)mem_ptr + sizeof(DMP_MISC));
	mem_ptr->misc_data = (char*)td->att_array;
	td->data_rac.att_ptrs = (DB_ATTS **) ((char *)td->att_array +
			((td->data_rac.att_count + 1) * sizeof(DB_ATTS)));
	p = (char *) td->data_rac.att_ptrs + 
				td->data_rac.att_count * sizeof(DB_ATTS *);
	if (data_cmpcontrol_size > 0)
	{
	    td->cmp_control = (PTR) p;
	    td->control_size = data_cmpcontrol_size;
	    p += data_cmpcontrol_size;
	}

	nextattname = p;

	/**************************************************************
	**
	** Lookup the table in IIATTRIBUTE.
	** Fill in the DB_ATTS arrays.
	**
	**************************************************************/

	status = dm2r_rcb_allocate(atp->atp_dcb->dcb_att_tcb_ptr, (DMP_RCB *)0,
	    &tran_id, dmf_svcb->svcb_lock_list, 0, &attr_rcb, &local_dberr);
	if (status != E_DB_OK)
	    break;

	/*  Initialize the RCB. */

	attr_rcb->rcb_lk_mode = RCB_K_IS;
	attr_rcb->rcb_k_mode = RCB_K_IS;
	attr_rcb->rcb_access_mode = RCB_A_READ;
	attr_rcb->rcb_lk_id = dmf_svcb->svcb_lock_list;
    
	key_desc[0].attr_operator = DM2R_EQ;
	key_desc[0].attr_number = DM_1_ATTRIBUTE_KEY;
	key_desc[0].attr_value = (char *)&table_id->db_tab_base;
	key_desc[1].attr_operator = DM2R_EQ;
	key_desc[1].attr_number = DM_2_ATTRIBUTE_KEY;
	key_desc[1].attr_value = (char *)&idx;

	status = dm2r_position(attr_rcb, DM2R_QUAL, key_desc, (i4)2,
	      (DM_TID *)0, &local_dberr);
	if (status != E_DB_OK)
	    break;

	attr_count = 0;
	for (;;)
	{
	    /* 
	    ** Get a qualifying attribute record.  
	    */
	    status = dm2r_get(attr_rcb, &tid, DM2R_GETNEXT, 
		  (char *)&attrrecord, &local_dberr);
	    if (status != E_DB_OK )
		break;

	    if ((attrrecord.attrelid.db_tab_base != table_id->db_tab_base) ||
		(attrrecord.attrelid.db_tab_index != idx))
		continue;	    

	    /*
	    ** Check for wacko attribute information.
	    */
	    attr_count++;
	    if ((attrrecord.attid > td->data_rac.att_count) || (attrrecord.attid <= 0))
	    {
		uleFormat(NULL, E_DM00A8_BAD_ATT_ENTRY,
		    (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL, (i4)0, 
		    (i4 *)NULL, &local_err, (i4)5, 0, attrrecord.attid,
		    sizeof(DB_OWN_NAME), rel_record.relowner.db_own_name,
		    sizeof(DB_TAB_NAME), rel_record.relid.db_tab_name,
		    sizeof(DB_DB_NAME), atp->atp_dcb->dcb_name.db_db_name,
		    0, td->data_rac.att_count);
		status = E_DB_ERROR;
		break;
	    }
	    else if (attr_count > td->data_rac.att_count)
	    {
		uleFormat(NULL, E_DM00A9_BAD_ATT_COUNT,
		    (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL, (i4)0, 
		    (i4 *)NULL, &local_err, (i4)5, 0, attr_count,
		    sizeof(DB_OWN_NAME), rel_record.relowner.db_own_name,
		    sizeof(DB_TAB_NAME), rel_record.relid.db_tab_name,
		    sizeof(DB_DB_NAME), atp->atp_dcb->dcb_name.db_db_name,
		    0, td->data_rac.att_count);
		status = E_DB_ERROR;
		break;
	    }

	    /*
	    ** Got a qualifying record, get attr description.
	    */
	    attid = (attrrecord.attintl_id ? 
		attrrecord.attintl_id : attrrecord.attid);

	    att = &td->att_array[attid];

	    for (alen = DB_ATT_MAXNAME;  
		attrrecord.attname.db_att_name[alen-1] == ' ' 
			&& alen >= 1; alen--);
	    att->attnmlen = alen;
	    att->attnmstr = nextattname;
	    MEcopy(attrrecord.attname.db_att_name, alen, att->attnmstr);
	    att->attnmstr[alen] = '\0';
	    nextattname += (alen + 1);

	    att->offset = attrrecord.attoff;
	    att->type = attrrecord.attfmt;
	    att->length = attrrecord.attfml;
	    att->precision = attrrecord.attfmp;
	    att->key = attrrecord.attkey;
	    att->flag = attrrecord.attflag;
	    if (att->flag & ATT_PERIPHERAL)
		td->lobj_att_count++;
	    att->key_offset = 0;
	    COPY_DEFAULT_ID(attrrecord.attDefaultID, att->defaultID);
	    att->intl_id = attid;
	    att->ver_added = attrrecord.attver_added;
	    att->ver_dropped = attrrecord.attver_dropped;
	    att->val_from = attrrecord.attval_from;
	    att->ver_altcol = attrrecord.attver_altcol;
	    att->collID = attrrecord.attcollID;

	    /*
	    ** Set up list of pointers to attribute info for use in
	    ** data compression. Note that it is indexed from 0 to attintl_id-1,
	    ** not from 1 to attintl_id (as is the att_array above).
	    */
	    td->data_rac.att_ptrs[attid - 1] = att;
	}

	/* Expected way to end the above loop is with NONEXT */
	if (local_dberr.err_code == E_DM0055_NONEXT)
	    status = E_DB_OK;

	if (status != E_DB_OK)
	    break;

	status = dm2r_release_rcb(&attr_rcb, &local_dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** If no attributes were found, then don't treat as an error since
	** it is sometimes expected, but break to below to return without
	** a table descriptor.
	**
	** Treat inconsistent table data the same as not finding any table.
	*/
	if ((attr_count == 0) || (attr_count != td->data_rac.att_count))
	    break;

	/*
	** Finish initializion of the Table Descriptor.
	** Go ahead and setup the row accessor for the current compression,
	** since that's very likely what will be in the journals too.
	*/
	td->table_id.db_tab_base = table_id->db_tab_base;
	td->table_id.db_tab_index = idx;
	td->data_rac.cmp_control = td->cmp_control;
	td->data_rac.control_count = td->control_size;
	status = dm1c_rowaccess_setup(&td->data_rac);
	if (status != E_DB_OK)
	    break;

	/*
	** Special handling of IITREE.  Don't audit the binary treetree field.
	*/
	if (table_id->db_tab_base == DM_B_TREE_TAB_ID)
	    attr_count--;

	break;
    }

    /*
    ** If we have successfully built a Table Descriptor then return it.
    */
    if ((status == E_DB_OK) && (td->table_id.db_tab_base))
	return (td);

    /*
    ** If an error occured, then log an error message and return no table
    ** descriptor.  Auditdb will continue, but references to this table will
    ** require Hex Dumps.
    */
    if (DB_FAILURE_MACRO(status))
    {
	if (local_dberr.err_code)
	    dmfWriteMsg(&local_dberr, 0, 0);
	dmfWriteMsg(NULL, E_DM120D_ATP_DESCRIBE, 0);
    }

    if (rel_rcb != 0)
    {
	status = dm2r_release_rcb(&rel_rcb, &local_dberr);
	if (status != E_DB_OK)
	    dmfWriteMsg(&local_dberr, 0, 0);
    }

    if (attr_rcb != 0)
    {
	status = dm2r_release_rcb(&attr_rcb, &local_dberr);
	if (status != E_DB_OK)
	    dmfWriteMsg(&local_dberr, 0, 0);
    }

    /*
    ** Free attribute descriptor array if one was allocated.
    */
    if (td && (td->att_array))
    {
	td->att_array = (DB_ATTS *)((char *)td->att_array - sizeof(DMP_MISC));
	(VOID) dm0m_deallocate((DM_OBJECT **)&td->att_array);
    }

    return ((ATP_TD *) NULL);
}

/*{
** Name: expand_displaybuf - Increase size of output display buffer.
**
** Description:
**	This routine is called when the audit trail display buffer is not
**	large enough to hold a formatted row.
**
**	This routine will replace the display buffer with a larger one.
**	The contents of the old buffer are copied to the new one.
**
**	The caller should reset any pointers into the old display buffer
**	as they are no longer valid.
**
**	The parameter 'bytes_needed' indicates how many bytes the caller
**	must have.  This routine will probably allocate a buffer much
**	larger than this (rounded up to the next 1k).
**
**	A pointer to the new buffer will be put in atp_displaybuf and
**	the new size will be recorded in atp_sdisplaybuf.
**
** Inputs:
**	atp				Audit Context.
**	bytes_needed			New memory space needed.
**
** Outputs:
**	Returns:
**	    OK
**	    E_DB_ERROR			Memory could not be allocated.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**       7-may-1990 (rogerk)
**          Created for 6.3/02.
[@history_template@]...
*/
static DB_STATUS
expand_displaybuf(
DMF_ATP	    *atp,
i4	    bytes_needed,
i4	    bytes_used)
{
    DMF_JSX		*jsx = atp->atp_jsx;
    DB_STATUS		status;
    DMP_MISC		*mem_ptr;
    char		*new_buffer;
    i4		new_size;
    i4		mem_needed;
    i4		local_err;
    DB_ERROR		local_dberr;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Calculate the new buffer size.
    ** It must be at least bytes_used + bytes_needed.  Adjust up to the
    ** next 1k size (by adding 1k-1 and zeroing out bits less significant
    ** than 1k (hex 0x400)).
    */
    new_size = bytes_used + bytes_needed;
    new_size = (new_size + 0x400 - 1) & 0x7ffffc00;

    /*
    ** Check for too much memory - currently, we don't support more than
    ** 16,000 bytes of output.  This is an arbitrary limit to catch runaway
    ** memory problems and could be increased if desired.
    */
    if (new_size > ATP_MAX_DISPLAYBUF)
    {
	dmfWriteMsg(NULL, E_DM1214_ATP_DISPLAY_OVFL, 0);
	return (E_DB_ERROR);
    }

    /*
    ** Allocate new buffer.
    */
    mem_needed = new_size + sizeof(DMP_MISC);
    status = dm0m_allocate(mem_needed, (i4)0, (i4)MISC_CB, 
	(i4)MISC_ASCII_ID, (char *)NULL, (DM_OBJECT**)&mem_ptr, &jsx->jsx_dberr);
    if (DB_FAILURE_MACRO(status))
    {
	uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	uleFormat(NULL, E_DM9425_MEMORY_ALLOCATE, (CL_ERR_DESC *)NULL, ULE_LOG, 
	    NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 1,
	    0, mem_needed);
        SETDBERR(&jsx->jsx_dberr, 0, E_DM1211_ATP_MEM_ALLOCATE);
	return (E_DB_ERROR);
    }
    new_buffer = (char *)mem_ptr + sizeof(DMP_MISC);
    mem_ptr->misc_data = new_buffer;

    /*
    ** Just for safety, make sure there already is a display buffer allocated.
    */
    if (atp->atp_displaybuf)
    {
	/*
	** Copy used bytes from old buffer to new one.
	*/
	if (bytes_used > 0)
	    MEcopy((PTR)atp->atp_displaybuf, bytes_used, (PTR)new_buffer);

	/*
	** Deallocate old buffer.  Adjust for MISC header at top of allocated
	** memory buffer.
	*/
	atp->atp_displaybuf = atp->atp_displaybuf - sizeof(DMP_MISC);
	(VOID) dm0m_deallocate((DM_OBJECT **)&atp->atp_displaybuf);
    }

    /*
    ** Set pointers to new buffer.
    */
    atp->atp_displaybuf = new_buffer;
    atp->atp_sdisplaybuf = new_size;

    return (E_DB_OK);
}

/*{
** Name: add_abortsave 	- Catalog abort to savepoint
**
** Description:
**	Called when an abort to savepoint journal record is found, this 
**	function updates the savepoint context on the abort transaction list.
**
**	Ecountering an abortsave log record makes this list and this
**	list element element persist into the output phase of processing.
**
** Inputs:
**	atp			Process Context
**	record			Savepoint record
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      08-feb-1993 (jnash)
**          Created for the reduced logging project.
*/
static DB_STATUS
add_abortsave(
DMF_ATP		*atp,
PTR		record)
{
    DMF_JSX	    *jsx = atp->atp_jsx;
    ATP_ATX	    *atx;
    ATP_ABS	    *ab;
    DM0L_ABORT_SAVE *log = (DM0L_ABORT_SAVE *)record;
    DM0L_HEADER     *h = (DM0L_HEADER *)record;
    DB_STATUS	    status;
    i4	    mem_needed;
    i4		    local_err;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Find prior transaction history.
    */
    lookup_atx(atp, &h->tran_id, &atx);

    /*  
    ** If the transaction entry does not exist, there must have been no 
    ** prior savepoint in this transaction, so create one now.
    */
    if (atx == 0)
    {
	status = add_atx(atp, &h->tran_id, ATX_ABORTSAVE, &atx);
	if (status != E_DB_OK)
	    return (E_DB_ERROR);   		
    }

    /*
    ** Create new ABS list entry queued off ATX. 
    */
    mem_needed = sizeof(ATP_ABS);
    status = dm0m_allocate(mem_needed, 0, (i4)DM_ABS_CB,
	(i4)ABS_ASCII_ID, (char *)NULL, (DM_OBJECT **)&ab, &jsx->jsx_dberr);
    if (status != E_DB_OK)
    {
	uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	uleFormat(NULL, E_DM9425_MEMORY_ALLOCATE, (CL_ERR_DESC *)NULL, ULE_LOG, 
	    NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 1,
	    0, mem_needed);
        SETDBERR(&jsx->jsx_dberr, 0, E_DM1211_ATP_MEM_ALLOCATE);
	return (E_DB_ERROR);   		
    }

    ab->abs_sp_id = log->as_id;
    ab->abs_start_lsn = log->as_header.compensated_lsn;
    ab->abs_end_lsn   = log->as_header.lsn;

    /*	
    ** Queue ABS to the end of the list.
    */
    ab->abs_state.stq_next = atx->atx_abs_state.stq_next;
    ab->abs_state.stq_prev = (ATP_QUEUE *)&atx->atx_abs_state.stq_next;
    atx->atx_abs_state.stq_next->stq_prev = &ab->abs_state;
    atx->atx_abs_state.stq_next = &ab->abs_state;
    atx->atx_abs_count++;

    return (E_DB_OK);
}

/*{
** Name: add_abort	 	- Catalog aborted transaction
**
** Description:
**	Called when an ET of an aborted transaction is found, 
**	or when ET of transaction outside user time interval is found,
**	this function updates the ATX accordingly, removing all 
**	abortsave context.  Records associated with an aborted 
**	transaction are thereby not output during the output phase.
**
** Inputs:
**	atp			Pointer to Process Context
**	tran_id			pointer to transaction id.
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      08-feb-1993 (jnash)
**	    Created for the reduced logging project.
**	08-feb-1999 (nanpr01)
**	    Fix the segv. Since the pointer gets initialized to null after
**	    deallocation, it's reference causes segv. Use a while loop instead
**	    of the for loop to avoid extra local variable.
*/
static DB_STATUS
add_abort(
DMF_ATP		*atp,
DB_TRAN_ID	*tran_id)
{
    DMF_JSX	    *jsx = atp->atp_jsx;
    ATP_ATX	    *atx;
    ATP_ABS	    *ab;
    ATP_QUEUE	    *st;
    DB_STATUS	    status;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Find the prior transaction history, if any.
    */
    lookup_atx(atp, tran_id, &atx);

    /*  
    ** If no entry currently exists, create an ATX entry for this transaction.
    */
    if (atx == 0)
    {
	status = add_atx(atp, tran_id, ATX_ABORT, &atx);
	if (status != E_DB_OK)
	    return (E_DB_ERROR);   		
    }
    else
    {
	/*
	** Prior context exists, dissolve all saveabort and abortsave entries.
	*/
	while ((st = atx->atx_abs_state.stq_next) != 
		(ATP_QUEUE *)&atx->atx_abs_state.stq_next)
	{
	    ab = (ATP_ABS *)((char *)st - CL_OFFSETOF(ATP_ABS, abs_state));

	    ab->abs_state.stq_next->stq_prev = ab->abs_state.stq_prev;
	    ab->abs_state.stq_prev->stq_next = ab->abs_state.stq_next;
	    dm0m_deallocate((DM_OBJECT **)&ab);
	    atx->atx_abs_count--;
	}

#ifdef xDEV_TEST
	if (atx->atx_abs_count != 0)
	    TRdisplay("add_abort: atx_abs_count != 0.\n");
#endif

	/*
	** Mark transaction as aborted.
	*/
	atx->atx_status = ATX_ABORT;
    }

    return (E_DB_OK);
}

/*{
** Name: lookup_atx   - 	Find atx for a given transaction.
**
** Description:
**      This routine searches the aborted transaction hash queue for a
**      given transaction.
**
** Inputs:
**      atp				ATP context
**      DB_TRAN_ID			Transaction id
**
** Outputs:
**	return_tx			NULL if tx not found
**					Non-null if tx found
**      Returns:
**          VOID
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      08-feb-1993 (jnash)
**          Created for the reduced logging project.
*/
static VOID
lookup_atx(
DMF_ATP		*atp,
DB_TRAN_ID	*tran_id,
ATP_ATX		**return_atx)
{
    ATP_ATX	*atx;

    *return_atx = NULL;

    atx = (ATP_ATX *) &atp->atp_atxh[tran_id->db_low_tran % ATP_ATX_HASH];
    while (atx = (ATP_ATX *) atx->atx_dm_object.obj_next)
    {
        if (atx->atx_tran_id.db_high_tran == tran_id->db_high_tran &&
            atx->atx_tran_id.db_low_tran == tran_id->db_low_tran)
        {
	    *return_atx = atx;
            return;
        }
    }

    return;
}

/*{
** Name: add_atx   - 	Add an ATX control block.
**
** Description:
**      This routine adds an aborted transaction element for a
**      given transaction to the ATX list.
**
** Inputs:
**      atp				ATP control block
**      tran_id			 	Transaction id
**      status			 	ATX status
**
** Outputs:
**      Returns:
**      atx				Pointer to allocated atx.
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      08-feb-1993 (jnash)
**          Created for the reduced logging project.
**	26-apr-1993 (bryanp)
**	    Correct memory size parameter in error message (lint).
*/
static DB_STATUS
add_atx(
DMF_ATP	    	*atp,
DB_TRAN_ID	*tran_id,
i4		atx_status,
ATP_ATX		**ret_atx)
{
    DMF_JSX	*jsx = atp->atp_jsx;
    DB_STATUS	status;
    ATP_ATX	*atx;
    ATP_ATX	**atxq;
    i4		local_err;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Allocate an ATX structure.
    */
    status = dm0m_allocate(sizeof(ATP_ATX), 0, DM_ATX_CB, ATX_ASCII_ID,
		(char *)atp, (DM_OBJECT **)&atx, &jsx->jsx_dberr);
    if (status != E_DB_OK)
    {
	uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	uleFormat(NULL, E_DM9425_MEMORY_ALLOCATE, (CL_ERR_DESC *)NULL, ULE_LOG, 
	    NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 1,
	    0, sizeof(ATP_ATX));
        SETDBERR(&jsx->jsx_dberr, 0, E_DM1211_ATP_MEM_ALLOCATE);
	return (E_DB_ERROR);   		
    }

    /*
    ** Fill in the Aborted Transaction information.
    */
    atx->atx_tran_id = *tran_id;
    atx->atx_status = atx_status;

    /*
    ** Put abort history on ATX queue.
    */
    atxq = &atp->atp_atxh[tran_id->db_low_tran % ATP_ATX_HASH];
    atx->atx_dm_object.obj_next = (DM_OBJECT *) *atxq;
    atx->atx_dm_object.obj_prev = (DM_OBJECT *) atxq;
    if (*atxq)
        (*atxq)->atx_dm_object.obj_prev = (DM_OBJECT *) atx;
    *atxq = atx;

    /*
    ** Insert ATX on active state queue.
    */
    atx->atx_state.stq_next = atp->atp_atx_state.stq_next;
    atx->atx_state.stq_prev = (ATP_QUEUE *)&atp->atp_atx_state.stq_next;
    atp->atp_atx_state.stq_next->stq_prev = &atx->atx_state;
    atp->atp_atx_state.stq_next = &atx->atx_state;
    atx->atx_abs_state.stq_next = (ATP_QUEUE *)&atx->atx_abs_state.stq_next;
    atx->atx_abs_state.stq_prev = (ATP_QUEUE *)&atx->atx_abs_state.stq_next;
    atx->atx_abs_count = 0;

    *ret_atx = atx;

    return (E_DB_OK);
}

/*{
** Name: atp_put_line   - Output journal record.
**
** Description:
**      This routine is passed by AUDITDB as the function call argument to
**      dmd_format_log() to output the log record.
**
** Inputs:
**      length                          The length of the buffer
**      buffer                   	The buffer to output.
**
** Outputs:
**      Returns:
**          VOID
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      08-feb-1993 (jnash)
**          Created for the reduced logging project.
**	30-may-2001 (devjo01)
**	    Pass up return value to quiet compiler warning.
**	    
*/
static i4
atp_put_line(
PTR		arg,
i4		length,
char            *buffer)
{
    return(dmf_put_line(0, length, buffer));
}

/*{
** Name: atp_wait_for_archiver
**
** Description:
**	Called during auditdb -wait, this routine waits for the  
**	archiver to purge the database up to the current log file eof.
**	It signals archive events as required.
**
** Inputs:
**      jsx				Pointer to checkpoint context.
**	dcb				Pointer to DCB for the database.
**
** Outputs:
**      err_code                        Reason for error return status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-jul-1993 (jnash)
**          Created for auditdb -wait from similar dmfcpp code.
**          Change db_f_jnl_cp to db_f_jnl_la references.
**	20-sep-1993 (andys)
**	    Fix arguments to ule_format call.
*/
static DB_STATUS
atp_archive_wait(
DMF_JSX	        *jsx,
DMP_DCB	        *dcb)
{
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;
    LG_LA		jnl_la;
    LG_LA		saved_eof;
    LG_DATABASE		database;
    LG_HEADER 		header;
    i4		event;
    i4		event_mask;
    i4		length;
    i4             local_err;
    char	        error_buffer[ER_MAX_LEN];
    bool		wait_complete = FALSE;
    bool 		have_transaction = FALSE;
#ifdef xATP_TEST
    bool		zero_window = FALSE;
#endif
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    for (;;)
    {
	/*
	** Find the current end of the log file.
	*/
	cl_status = LGshow(LG_A_HEADER, (PTR)&header, sizeof(header), 
                        &length, &sys_err);
	if (cl_status)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
            SIprintf("Can't show logging header.\n", error_buffer);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1216_ATP_WAIT);
	    break;
	}

	/*
	** Save the log file eof.  We will wait till the archiver gets here
	** if necessary.
	*/
	saved_eof = header.lgh_end;

	/* Start a transaction to use for LGevent waits. */
	cl_status = LGbegin(LG_NOPROTECT, dcb->dcb_log_id, 
	    (DB_TRAN_ID *)&jsx->jsx_tran_id, &jsx->jsx_tx_id, 
	    sizeof(dcb->dcb_db_owner),
	    dcb->dcb_db_owner.db_own_name, 
	    (DB_DIS_TRAN_ID*)NULL, &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM900C_BAD_LOG_BEGIN, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1216_ATP_WAIT);
	    break;
	}

	have_transaction = TRUE;

	/*
	** Start Wait cycle:
	**
	**	Show the database and check Journal window.
	**	Until the archiver has processed up to the
	**	    eof address (recorded above), signal an archive
	**	    cycle and wait for archive processing to complete.
	*/
	for (;;)
	{
	    /*
	    ** Show the database to get its current Journal windows.
	    */
	    database.db_id = (LG_DBID) dcb->dcb_log_id;
	    cl_status = LGshow(LG_S_DATABASE, (PTR)&database, sizeof(database),
		&length, &sys_err);
	    if (cl_status != E_DB_OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
		    0, LG_S_DATABASE);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1216_ATP_WAIT);
		break;
	    }

	    /*
	    ** If the database journal window indicates there is no 
	    ** journal work to process, then the wait is complete.
	    */
	    if ((database.db_f_jnl_la.la_sequence == 0) &&
		(database.db_f_jnl_la.la_block == 0) &&
		(database.db_l_jnl_la.la_sequence == 0) &&
		(database.db_l_jnl_la.la_block == 0))
	    {
		wait_complete = TRUE;
#ifdef xATP_TEST
		zero_window = TRUE;
#endif
		break;
	    }

	    /*
	    ** Save the log address of the start of the journal window.
	    ** This is as far as the archiver has proceeded.
	    */
            jnl_la.la_sequence = database.db_f_jnl_la.la_sequence;
            jnl_la.la_block    = database.db_f_jnl_la.la_block;
            jnl_la.la_offset   = database.db_f_jnl_la.la_offset;

	    /*
	    ** If the Archiver has caught up to the saved log address, 
	    ** then the wait is complete.
	    */
	    if (LGA_GTE(&jnl_la, &saved_eof))
	    {
		wait_complete = TRUE;
		break;
	    }

#ifdef xATP_TEST
	    SIprintf(
		"%@ Wait continues... jnl_la:<%d,%d%d>, saved_eof:<%d,%d,%d>\n",
		jnl_la.la_sequence,
		jnl_la.la_block,
		jnl_la.la_offset,
		saved_eof.la_sequence,
		saved_eof.la_block,
		saved_eof.la_offset);
#endif

	    /*
	    ** Check the state of the archiver.  If it is dead then we have
	    ** to fail.  If its alive but not running then give it a nudge.
	    */
	    event_mask = (LG_E_ARCHIVE | LG_E_START_ARCHIVER);

	    cl_status = LGevent(LG_NOWAIT,
		jsx->jsx_tx_id, event_mask, &event, &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		uleFormat(NULL, E_DM900F_BAD_LOG_EVENT, &sys_err, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code,
		    1, 0, event_mask);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1216_ATP_WAIT);
		break;
	    }
	    else if (event & LG_E_START_ARCHIVER)
	    {
		uleFormat(NULL, E_DM1144_CPP_DEAD_ARCHIVER, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1216_ATP_WAIT);
		break;
	    }

	    /*
	    ** If there is still archiver work to do, but the archiver is
	    ** is not processing our database, then signal an archive event.
	    */
	    if ( ! (event & LG_E_ARCHIVE))
	    {
		cl_status = LGalter(LG_A_ARCHIVE, (PTR)0, 0, &sys_err);
		if (cl_status != OK)
		{
		    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
			(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		    uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL, 
			(char *)NULL, (i4)0, (i4 *)NULL, err_code,
			1, 0, LG_A_ARCHIVE);
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1216_ATP_WAIT);
		    break;
		}
	    }

	    /*
	    ** Wait a few seconds before rechecking the database/system states.
	    */
	    PCsleep(5000);
	}

	/*
	** If we broke out of the above loop because of an error, then fall
	** through to error handling code below.
	*/
	if ( ! wait_complete)
	    break;

#ifdef xATP_TEST
	if (zero_window)
	    SIprintf("%@ Wait complete via zero journal window.\n");
	else
	    SIprintf(
		"%@ Wait complete. jnl_la: <%d,%d%d>, saved_eof: <%d,%d,%d>\n",
		jnl_la.la_sequence,
		jnl_la.la_block,
		jnl_la.la_offset,
		saved_eof.la_sequence,
		saved_eof.la_block,
		saved_eof.la_offset);
#endif

	cl_status = LGend(jsx->jsx_tx_id, 0, &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1216_ATP_WAIT);
	    break;
	}

	return(E_DB_OK);
    }

    if (have_transaction)
    {
	cl_status = LGend(jsx->jsx_tx_id, 0, &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	    uleFormat(NULL, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);
	}
    }

    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	(char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0);

    return(E_DB_ERROR);
}


/*{
** Name: atp_uncompress	- Uncompress audit tuple ( if necessary )
**
** Description:
**      Uncompresses the audit record tuple if table information
**	requires it.  If no uncompression is needed, this routine
**	just returns else it places the uncompressed tuple in the
**	buffer the ATP provides for it and alters the record pointer and
**	size appropriately.
**
** Inputs:
**      atp                             Pointer to ATP context.
**	table_id			The table the record belongs to.
**      record                          The record to display.
**	size				Size of the record.
**
** Outputs:
**	record				Points to the uncompressed tuple.
**	size				The uncompressed size.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    The uncompressed tuple is placed in the atp->atp_uncombuf.
**
** History:
**      25-feb-1996 (nick)
**           Created.
**	06-may-1996 (thaju02)
**	     Add page_size parameter.
**	13-sep-1996 (canor01)
**	    Add NULL tuple buffer to dmpp_uncompress call.
**	29-jul-1999 (i4jo01)
**	    We should not be checking the tuple size against the size
**	    of the row found in the system catalogs. It could have
**	    changed due to an alter table. (b95311)
**      04-Apr-2005 (chopr01) Bug113295  INGSRV3013
**          Added journal record parameter so that we can determine whether 
**          the current table descriptor (td) has a different row version
**          to the journal record. If this is the case we must convert
**          the row to the current format to safely display NULL values
**          added by ALTER TABLE or skip columns dropped by ALTER TABLE. 
**          This support has been added for DM0L_PUT, DM0L_DEL and DM0L_REP 
**          records. 
**	13-Feb-2008 (kschendel) SIR 122739
**	    Revise uncompress call, remove record-type and tid.
*/
static DB_STATUS
atp_uncompress(
DMF_ATP		*atp,
DB_TAB_ID	*table_id,
PTR		**record,
i4		*size,
i2		comptype,
PTR             jrecord)
{

    ATP_TD	*td = NULL;
    DB_STATUS	status = E_DB_OK;
    char	*r = (char *)*record;
    i4		uncompressed_length;
    DB_STATUS   s = E_DB_OK;
    u_i2        row_version;
    bool	convert_row = FALSE;
    DM0L_HEADER  *h = (DM0L_HEADER *)record;
    i4		operation = ((DM0L_HEADER *)jrecord)->type;

    td = get_table_descriptor(atp, table_id);
    if (td == NULL)
	return (E_DB_OK);		/* Nothing we can do??? */

    /* Other journal records which contain row_version details
    ** can be added to this switch.
    */

    if (comptype != TCB_C_NONE)
    {
	/* Reset rowaccessor if journal record compression type doesn't
	** match what it is currently set for.  The compression-control
	** array OUGHT to be large enough for any kind of compression.
	*/
	if (comptype != td->data_rac.compression_type)
	{
	    td->data_rac.compression_type = comptype;
	    td->data_rac.cmp_control = td->cmp_control;
	    td->data_rac.control_count = td->control_size;
	    status = dm1c_rowaccess_setup(&td->data_rac);
	    if (status != E_DB_OK)
	    {
		TRdisplay("Row uncompression setup error on table (%d, %d)\n",
			table_id->db_tab_base, table_id->db_tab_index);
		TRdisplay("\tLogged compression type: %d\n", comptype);
		dmfWriteMsg(NULL, E_DM1212_ATP_DISPLAY, 0);
		return (status);
	    }
	}

	status = (*td->data_rac.dmpp_uncompress)(&td->data_rac,
		r, atp->atp_uncompbuf, td->row_width, &uncompressed_length,
		NULL, td->row_version, &adf_cb);
	if (DB_FAILURE_MACRO(status))
	{
	    TRdisplay("Row uncompression error on table (%d, %d)\n",
		    table_id->db_tab_base, table_id->db_tab_index);
	    TRdisplay("\tTable width: %d, uncompressed length: %d\n",
		    td->row_width, uncompressed_length);
	    dmfWriteMsg(NULL, E_DM1212_ATP_DISPLAY, 0);
	}
	else
	{
	    *record = (PTR *)atp->atp_uncompbuf;
	    *size = uncompressed_length;
	}
    }
    else
    {
	switch (operation)
	{
	case DM0LPUT:
	    row_version = (u_i2)((DM0L_PUT *)jrecord)->put_row_version;
	    if(td->row_version != row_version)
	    {
		convert_row = TRUE;
	    }
	    break;

	case DM0LDEL:
	    row_version = (u_i2)((DM0L_DEL *)jrecord)->del_row_version;
	    if(td->row_version != row_version)
	    {
		convert_row = TRUE;
	    }
	    break;

	case DM0LREP:
	    row_version = (u_i2)((DM0L_REP *)jrecord)->rep_orow_version;
	    if(td->row_version != row_version)
	    {
		convert_row = TRUE;
	    }
	    break;

	default:
	    break;
	}

	if (convert_row)
	{
	    s = dm1r_cvt_row(td->data_rac.att_ptrs, td->data_rac.att_count, r,
		    atp->atp_uncompbuf, td->row_width, &uncompressed_length,
		    row_version, &adf_cb);
	    if (s != E_DB_OK)
	    {
		return E_DB_ERROR;
	    }
	    *record = (PTR *)atp->atp_uncompbuf;
	    *size = uncompressed_length;
	}
	else if (*size != td->row_width)
	{
	    /*
	    ** this means ( probably ) that the tuple is compressed
	    ** yet the relation is no longer - error this for now
	    ** ( we don't know which compression is in use - we could
	    ** run the tuple through all of them to see if we get a full
	    ** row back I guess but what are they ? )
	    */
	    TRdisplay("Row uncompression error on table (%d, %d)\n",
		table_id->db_tab_base, table_id->db_tab_index);
	    TRdisplay("\tTable width: %d, tuple size : %d\n",
		td->row_width, *size);
	    dmfWriteMsg(NULL, E_DM1212_ATP_DISPLAY, 0);
	    status = E_DB_ERROR;
	}
    }

    return(status);
}

/*{
** Name: ex_handler	- DMFATP exception handler.
**
** Description:
**	Exception handler for ATP program.  This handler is declared at Audit
**	startup.  All exceptions which come through here will cause the
**	program to fail.
**
**	An error message including the exception type and PC will be written
**	to the log file before exiting.
**
** Inputs:
**      ex_args                         Exception arguments.
**
** Outputs:
**	Returns:
**	    EXDECLARE
**	Exceptions:
**	    none
**
** Side Effects:
**	Static variable dmfatp_interrupt set if handler invoked for a user
**      interrupt.
**
** History:
**     04-apr-2002 (horda03) Bug 107181.
**        Created.
**	29-Aug-2008 (kschendel) Bug 120690
**	    Attempt to at least minimally handle arithmetic overflows thrown
**	    by alter-column coercions.
*/
static STATUS
ex_handler(
EX_ARGS	    *ex_args)
{
    i4	    err_code;

    if (ex_args->exarg_num == EX_DMF_FATAL)
    {
	err_code = E_DM904A_FATAL_EXCEPTION;
    }
    else if (ex_args->exarg_num == EXINTR)
    {
	dmf_put_line(0, sizeof("Exiting due to interrupt...")-1,
			"Exiting due to interrupt...");

        dmfatp_interrupt = 1;
	return (EXCONTINUES);
    }
    else if (ex_args->exarg_num == EXINTOVF
      || ex_args->exarg_num == EXFLTOVF
      || ex_args->exarg_num == EXDECOVF)
    {
	dmfWriteMsg(NULL, E_DM019D_ROW_CONVERSION_FAILED, 0);
	return (EXCONTINUES);
    }
    else
    {
	err_code = E_DM9049_UNKNOWN_EXCEPTION;
    }

    /*
    ** Report the exception.
    ** Note: the fourth argument to ulx_exception must be FALSE so
    ** that it does not try to call scs_avformat() since there is no
    ** session in the ATP.
    */
    (VOID) ulx_exception( ex_args, DB_DMF_ID, err_code, FALSE );

    /*
    ** Returning with EXDECLARE will cause execution to return to the
    ** top of the DMFATP routine.
    */
    return (EXDECLARE);
}

/*{
** Name: add_btime    - Add TX details to list of TXs to be displayed.
**
** Description:
**      The specifed transaction is added to the list of transactions to be displayed.
**      Also, the transaction is added to the list of transactions where the BT has not
**      yet been read.
**
** Inputs:
**      tran_id		- Transaction ID
**	missing_bt	- Add to tx id list of missing BT.
**
** Outputs:
**      err_code        - Reason for error return status.
**	Returns:
**	    E_DB_OK
**          E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	None.
**
** History:
**     01-Sep-2004 (horda03) Bug 107181.
**        Created.
**     25-Apr-2007 (kibro01) b115996
**        Ensure the misc_data pointer in a MISC_CB is set correctly and
**        that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**        since that's what we're actually using (the type is MISC_CB)

*/
static DB_STATUS
add_btime(
DMF_ATP    *atp,
DB_TRAN_ID *tran_id,
i4         missing_bt)
{
   DMF_JSX	*jsx = atp->atp_jsx;
   i4 mem_needed;
   ATP_B_TX *b_tx;
   ATP_B_TX **b_txq;
   DB_STATUS status;
   DM_OBJECT *mem_ptr;
   i4			*err_code = &jsx->jsx_dberr.err_code;

   CLRDBERR(&jsx->jsx_dberr);

   mem_needed = sizeof(ATP_B_TX) + sizeof(DMP_MISC);

   status = dm0m_allocate(mem_needed, (i4)0, (i4)MISC_CB, 
            (i4)MISC_ASCII_ID, (char *)NULL, &mem_ptr, &jsx->jsx_dberr);

   if (status != OK)
   {
       uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
                  (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
       uleFormat(NULL, E_DM9425_MEMORY_ALLOCATE, (CL_ERR_DESC *)NULL, ULE_LOG, 
                  NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
                  0, mem_needed);
       SETDBERR(&jsx->jsx_dberr, 0, E_DM1211_ATP_MEM_ALLOCATE);
       return status;
   }

   b_tx = (ATP_B_TX *) ((char *)mem_ptr + sizeof(DMP_MISC));
   ((DMP_MISC*)mem_ptr)->misc_data = (char*)b_tx;

   b_tx->b_tx_tran_id = *tran_id;
   

   /* Put the Output TX on the BTIME list */

   b_txq = &atp->atp_btime_txs [tran_id->db_low_tran % ATP_B_TX_CNT];

   b_tx->b_tx_next = *b_txq;
   b_tx->b_tx_prev = (ATP_B_TX *)b_txq;

   if (*b_txq)
      (*b_txq)->b_tx_prev = b_tx;

   *b_txq = b_tx;

   if (missing_bt)
   {
      b_tx->b_tx_missing_bt = atp->atp_btime_open_et;

      atp->atp_btime_open_et = b_tx;
   }
   else
      b_tx->b_tx_missing_bt = 0;

   return E_DB_OK;
}
/*{
** Name: atp_lobj_cpn_redeem	- Redeem a Large Object coupon.
**
** Description:
**	This procedure takes a Large Object coupon and "redeems" it.
**	The rows are obtained from the appropriate extension tables
**	which already exist in the journal file(s).
**
** Inputs:
**	atp				Pointer to audit context.
**	tran_id				Pointer to the transaction id.
**	base_tblid			Pointer to the base table id.
**	dvfrom				Pointer to the data value to be
**					converted.
**		.db_datatype		Datatype id of data value to be
**					converted.
**		.db_prec		Precision/Scale of data value
**					to be created.
**		.db_length		Length of data value to be
**					converted.
**		.db_data		Pointer to the actual data for
**					data value to be converted.
**	dvinto				Pointer to the data value to be
**					created.
**		.db_datatype		Datatype id of data value being
**					created.
**		.db_prec		Precision/Scale of data value
**					being created.
**		.db_length		Length of data value being
**					created.
**		.db_data		Pointer to location to put the
**					actual data for the data value
**					being created.
**	rep_type			In the case of DM0L_REP, this
**					reflects whether the coupon
**					specified is that of the old record
**					(1) or the new record (2). Otherwise,
**					it is zero.
**
** Outputs:
**	adc_dvinto			The resulting data value.
**		.db_data		The actual data for the data
**					value created will be placed
**					here.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	none
**
** History:
**	26-mar-2001 (somsa01)
**	    Created.
**	23-jul-2001 (somsa01)
**	    To prevent SIGBUS errors, use MEcopy() to grab the DMPE_RECORD
**	    info into the local variable.
**	06-May-2005 (jenjo02)
**	    Reorganized code to work on clusters.
**	22-Sep-2005 (jenjo02)
**	    Sam was right. Copy ADP_PERIPHERAL to aligned local structure.
*/

static DB_STATUS
atp_lobj_cpn_redeem(
DMF_ATP		*atp,
DB_TRAN_ID	*tran_id,
DB_TAB_ID	*base_tblid,
DB_DATA_VALUE	*dvfrom,
DB_DATA_VALUE	*dvinto,
i2		rep_type)
{
    DMF_JSX		*jsx = atp->atp_jsx;
    DB_STATUS		status;
    PTR			record;
    i4			l_record;
    DM0J_CTX		*jnl_etab;
    DB_DATA_VALUE	dvunder;
    i4			local_err;
    ADP_PERIPHERAL	periph;
    DB_ERROR		local_dberr;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Determine what underlying format ADF thinks is good.
    */
    if (adi_per_under(&adf_cb, dvfrom->db_datatype, &dvunder))
	return(E_DB_ERROR);

    atp->lobj_dvunder = &dvunder;
    atp->lobj_dvinto = dvinto;

    /* alignment paranoia */
    MEcopy(dvfrom->db_data, sizeof(ADP_PERIPHERAL), (PTR)&periph);
    atp->lobj_coupon = (DMPE_COUPON*)&periph.per_value.val_coupon;

    atp->lobj_base_tabid = base_tblid;
    atp->lobj_tran_id = tran_id;
    atp->lobj_reptype = rep_type;
    atp->lobj_segment1 = 1;
    atp->lobj_found_start = FALSE;

    /*
    ** Now, open up the current journal file for the extension table
    ** search.
    */
    status = dm0j_open( DM0J_MERGE, (char *)&atp->atp_dcb->dcb_jnl->physical, 
			atp->atp_dcb->dcb_jnl->phys_length, 
			&atp->atp_dcb->dcb_name, atp->atp_block_size,
			current_journal_num, 0, 0, DM0J_M_READ, -1,
			DM0J_FORWARD, (DB_TAB_ID *)0, &jnl_etab, &jsx->jsx_dberr);
    if (status != E_DB_OK)
    {
	if (jsx->jsx_dberr.err_code == E_DM9034_BAD_JNL_NOT_FOUND &&
	    CXcluster_enabled() != 0 && (current_journal_num == latest_jnl ||
	    current_journal_num == latest_jnl + 1))
	{
	    /* 
	    ** Merge the VAXcluster journals only if last in current
	    ** sequence.
	    */
	    status = dm0j_merge((DM0C_CNF *)0, atp->atp_dcb, (PTR)atp,
				lobj_redeem, &jsx->jsx_dberr);
	}
	if ( status != E_DB_OK )
	{
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    return(status);
	}
    }
    else while ( status == E_DB_OK )
    {
	/*
	** Read the next extension table record.
	*/
	status = dm0j_read(jnl_etab, &record, &l_record, &jsx->jsx_dberr);

	if ( status == E_DB_OK )
	    status = lobj_redeem((PTR)atp, record, err_code);

	if ( status )
	    (VOID) dm0j_close(&jnl_etab, &local_dberr);
    }

    if ( status && jsx->jsx_dberr.err_code == E_DM0055_NONEXT )
    {
	status = E_DB_OK;
	CLRDBERR(&jsx->jsx_dberr);
    }

    return(status);
}

/*{
** Name: lobj_redeem	- Redeem one segment of a blob.
**
** Description:
**	This procedure takes a Large Object coupon and "redeems" it.
**	The rows are obtained from the appropriate extension tables
**	which already exist in the journal file(s).
**	
**	The function may be called directly from atp_lobj_cpn_redeem()
**	(non-clustered) or driven as a callback from dm0j_merge()
**	in a cluster environment.
**
**	It deals with a single journal record.
**
** Inputs:
**	atp				Pointer to audit context.
**	  lobj_tran_id			Pointer to the transaction id.
**	  lobj_base_tabid		Pointer to the base table id.
**	  lobj_coupon			Pointer to lobj coupon.
**	  lobj_dvunder			Pointer to the data value to be
**					converted.
**		.db_datatype		Datatype id of data value to be
**					converted.
**		.db_prec		Precision/Scale of data value
**					to be created.
**		.db_length		Length of data value to be
**					converted.
**		.db_data		Pointer to the actual data for
**					data value to be converted.
**	  lobj_dvinto			Pointer to the data value to be
**					created.
**		.db_datatype		Datatype id of data value being
**					created.
**		.db_prec		Precision/Scale of data value
**					being created.
**		.db_length		Length of data value being
**					created.
**		.db_data		Pointer to location to put the
**					actual data for the data value
**					being created.
**	  lobj_reptype			In the case of DM0L_REP, this
**					reflects whether the coupon
**					specified is that of the old record
**					(1) or the new record (2). Otherwise,
**					it is zero.
**
**	record				Pointer to a journal record.
**
** Outputs:
**	atp
**	  lobj_dvinto			The resulting data value.
**		.db_data		The actual data for the data
**					value created will be placed
**					here.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	none
**
** History:
**	06-May-2005 (jenjo02)
**	    Extracted from atp_lobj_cpn_redeem.
**	01-Mar-2010 (jonj) BUG 123357
**	    get_table_descriptor() won't return an ATP_TD
**	    if the etab_tbl_id from the journal record
**	    has been dropped from iirelation.
*/
static DB_STATUS
lobj_redeem(
PTR	atp_ptr,
char	*record,
i4	*err_code)
{
    DMF_ATP		*atp = (DMF_ATP*)atp_ptr;
    DMF_JSX		*jsx = atp->atp_jsx;
    DB_STATUS		status = E_DB_OK;
    DM0L_HEADER		*h;
    DB_TAB_ID		etab_tbl_id;
    ATP_TD		*td;
    DB_DATA_VALUE	dvto;
    DMPE_RECORD		etab_record;
    i4			write_size;
    i4			count;
    i4			table_size;
    i4			owner_size;
    u_i2		newlen, seglen, oldlen;

    /* NB: "err_code" will point to jsx->jsx_dberr.err_code */

    CLRDBERR(&jsx->jsx_dberr);


    etab_tbl_id.db_tab_index = 0;

    switch (((DM0L_HEADER *)record)->type)
    {
	case DM0LDEL:
	    etab_tbl_id.db_tab_base =
		    ((DM0L_DEL *)record)->del_tbl_id.db_tab_base;
	    break;

	case DM0LPUT:
	    etab_tbl_id.db_tab_base =
		    ((DM0L_PUT *)record)->put_tbl_id.db_tab_base;
	    break;

	case DM0LREP:
	    etab_tbl_id.db_tab_base =
		    ((DM0L_REP *)record)->rep_tbl_id.db_tab_base;
	    break;

	default:
	    return(E_DB_OK);
    }

    td = get_table_descriptor(atp, &etab_tbl_id);

    if (!(atp->lobj_found_start) && (!td || !(td->relstat2 & TCB2_EXTENSION)))
	return(E_DB_OK);

    if (atp->lobj_found_start && (td->relstat2 & TCB2_HAS_EXTENSIONS) &&
	etab_tbl_id.db_tab_base == atp->lobj_base_tabid->db_tab_base &&
	etab_tbl_id.db_tab_index == atp->lobj_base_tabid->db_tab_index)
    {
	/*
	** We're at the base table journal entry. We're done.
	*/
        SETDBERR(&jsx->jsx_dberr, 0, E_DM0055_NONEXT);
	/* NB: "err_code" will point to jsx->jsx_dberr.err_code, but JIC */
	*err_code = E_DM0055_NONEXT;
	return(E_DB_ERROR);
    }

    h = (DM0L_HEADER *)record;
    if (h->tran_id.db_high_tran == atp->lobj_tran_id->db_high_tran &&
	h->tran_id.db_low_tran == atp->lobj_tran_id->db_low_tran)
    {
	switch (h->type)
	{
	    case DM0LDEL:
		table_size = ((DM0L_DEL *)h)->del_tab_size;
		owner_size = ((DM0L_DEL *)h)->del_own_size;
		MEcopy( &((DM0L_DEL *)h)->del_vbuf[table_size + owner_size],
			sizeof(DMPE_RECORD), (PTR)&etab_record );
		STRUCT_ASSIGN_MACRO(((DM0L_DEL *)h)->del_tbl_id,
				    etab_tbl_id);
		break;

	    case DM0LPUT:
		table_size = ((DM0L_PUT *)h)->put_tab_size;
		owner_size = ((DM0L_PUT *)h)->put_own_size;
		MEcopy( &((DM0L_PUT *)h)->put_vbuf[table_size + owner_size],
			sizeof(DMPE_RECORD), (PTR)&etab_record );
		STRUCT_ASSIGN_MACRO(((DM0L_PUT *)h)->put_tbl_id,
				    etab_tbl_id);
		break;

	    case DM0LREP:
		table_size = ((DM0L_REP *)h)->rep_tab_size;
		owner_size = ((DM0L_REP *)h)->rep_own_size;
		MEcopy( &((DM0L_REP *)h)->rep_vbuf[table_size + owner_size],
			sizeof(DMPE_RECORD), (PTR)&etab_record );
		STRUCT_ASSIGN_MACRO(((DM0L_REP *)h)->rep_tbl_id,
				    etab_tbl_id);
		break;
	}

	if ( atp->lobj_reptype )
	{
	    if ((atp->lobj_reptype == 1 && h->type == DM0LDEL) ||
		(atp->lobj_reptype == 2 && h->type == DM0LPUT))
	    {
		/*
		** In the case of a base table DM0LREP operation
		** we need to find the right extension table operations.
		*/
	    }
	    else
		return(E_DB_OK);
	}

	if (etab_tbl_id.db_tab_base == (u_i4)atp->lobj_coupon->cpn_etab_id &&
	    etab_record.prd_segment0 == 0 &&
	    etab_record.prd_segment1 == atp->lobj_segment1 &&
	    etab_record.prd_log_key.tlk_high_id ==
		atp->lobj_coupon->cpn_log_key.tlk_high_id &&
	    etab_record.prd_log_key.tlk_low_id ==
		atp->lobj_coupon->cpn_log_key.tlk_low_id)
	{
	    /*
	    ** This is an extension table record that we're interested in.
	    */
	    atp->lobj_found_start = TRUE;
	    STRUCT_ASSIGN_MACRO(*atp->lobj_dvinto, dvto);
	    atp->lobj_dvunder->db_data = (char*)&etab_record.prd_user_space;
	    if ((status = adc_cvinto(&adf_cb, atp->lobj_dvunder, &dvto)) == E_DB_OK)
	    {
		if (atp->lobj_segment1 == 1)
		    STRUCT_ASSIGN_MACRO(dvto, *atp->lobj_dvinto);
		else
		{
		    /*
		    ** We have to concatenate the two LONGTEXT
		    ** datatypes.
		    */
		    I2ASSIGN_MACRO(*(i2 *)atp->lobj_dvunder->db_data, seglen);
		    I2ASSIGN_MACRO(*(i2 *)atp->lobj_dvinto->db_data, oldlen);
		    if (oldlen >= (i2)atp->lobj_dvinto->db_length)
		    {
			/* MaxlengthReached = TRUE */
			SETDBERR(&jsx->jsx_dberr, 0, E_DM0055_NONEXT);
			/* NB: "err_code" will point to jsx->jsx_dberr.err_code, but JIC */
			*err_code = E_DM0055_NONEXT;
			status = E_DB_ERROR;
		    }
		    newlen = oldlen + seglen;
		    MEcopy(&newlen, DB_CNTSIZE, (PTR)atp->lobj_dvinto->db_data);
		    MEcopy( (PTR)(atp->lobj_dvunder->db_data + DB_CNTSIZE),
			    seglen,
			    (PTR)(atp->lobj_dvinto->db_data + DB_CNTSIZE + oldlen) );
		}

		atp->lobj_segment1++;
	    }
	}
    }

    return(status);
}

/*{
** Name: atp_lobj_copyout	- Copy out a Large Object.
**
** Description:
**	This procedure takes a Large Object coupon and writes it in the
**	right format to the output file specified by the user. This is so
**	that it can be copied back into a table properly.
**
** Inputs:
**	atp		Pointer to audit context.
**	tran_id		Pointer to the transaction id.
**	base_tblid	Pointer to the base table id.
**	lobj_dttype	The type of the large object column.
**	rep_type	In the case of DM0L_REP, this reflects whether
**			the coupon specified is that of the old record
**			(1) or the new record (2). Otherwise, it is zero.
**	recptr		Pointer to the audit record.
**	offset		Offset into recptr, pointing us to the large
**			object coupon.
**	fdesc		Pointer to the output file.
**
** Outputs:
**	none.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none.
**
** History:
**	28-mar-2001 (somsa01)
**	    Created.
**	23-jul-2001 (somsa01)
**	    To prevent SIGBUS errors, use MEcopy() to grab the DMPE_RECORD
**	    info into the local variable as well as ADP_PERIPHERAL. Also,
**	    added the passing of the offset separately.
**	06-May-2005 (jenjo02)
**	    Reorganized code to work on clusters.
**	22-Sep-2005 (jenjo02)
**	    Sam was right. Copy ADP_PERIPHERAL to aligned local structure.
**	16-jan-2006 (wanfr01)
**	    Bug 117495
**	    Initialize null_data buffer before using.
**	16-jan-2006 (wanfr01)
**	    Bug 117495
**	    Modification of fix to account for changed structure of null_data
**	20-Feb-2008 (thaju02)
**	    For repold/new, if no corresponding lobj del/ins record found
**	    (lobj data was not updated), then write out a '{LOBJ COUPON}' 
**	    placeholder. (B119967)
*/

static DB_STATUS
atp_lobj_copyout(
DMF_ATP		*atp,
DB_TRAN_ID	*tran_id,
DB_TAB_ID	*base_tblid,
i2		lobj_dttype,
i2		rep_type,
PTR		recptr,
i4		offset,
FILE		**fdesc)
{
    DMF_JSX		*jsx = atp->atp_jsx;
    DB_STATUS		status = E_DB_OK;
    ATP_LOBJ_SEG	segarray[32];
    ATP_LOBJ_SEG	null_data;
    i4			write_size;
    i4			count;
    DB_DATA_VALUE	empty_dv;
    PTR			record;
    i4			l_record;
    i4			local_err;
    DM0J_CTX		*jnl_etab;
    ADP_PERIPHERAL	periph;
    DB_ERROR		local_dberr;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    MEfill(sizeof(null_data), 0, (char *) &null_data);
    /*
    ** Determine what underlying format ADF thinks is good.
    */
    if (adi_per_under(&adf_cb, lobj_dttype, &empty_dv))
	return(E_DB_ERROR);

    /* alignment paranoia */
    MEcopy(recptr + offset, sizeof(ADP_PERIPHERAL), (PTR)&periph);
    atp->lobj_coupon = (DMPE_COUPON*)&periph.per_value.val_coupon;

    atp->lobj_base_tabid = base_tblid;
    atp->lobj_tran_id = tran_id;
    atp->lobj_reptype = rep_type;
    atp->lobj_fdesc = fdesc;

    atp->lobj_segarray = &segarray[0];
    atp->lobj_segindex = 0;
    atp->lobj_segment1 = 1;
    atp->lobj_found_start = FALSE;

    /*
    ** Now, open up the current journal file for the extension table
    ** search.
    */
    status = dm0j_open( DM0J_MERGE, (char *)&atp->atp_dcb->dcb_jnl->physical, 
			atp->atp_dcb->dcb_jnl->phys_length, 
			&atp->atp_dcb->dcb_name, atp->atp_block_size,
			current_journal_num, 0, 0, DM0J_M_READ, -1,
			DM0J_FORWARD, (DB_TAB_ID *)0, &jnl_etab, &jsx->jsx_dberr);
    if (status != E_DB_OK)
    {
	if (jsx->jsx_dberr.err_code == E_DM9034_BAD_JNL_NOT_FOUND &&
	    CXcluster_enabled() != 0 && (current_journal_num == latest_jnl ||
	    current_journal_num == latest_jnl + 1))
	{
	    /* 
	    ** Merge the VAXcluster journals only if last in current
	    ** sequence.
	    */
	    status = dm0j_merge((DM0C_CNF *)0, atp->atp_dcb, (PTR)atp,
				lobj_copy, &jsx->jsx_dberr);
	}
	if ( status != E_DB_OK )
	{
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    return(status);
	}
    }
    else while ( status == E_DB_OK )
    {
	/*
	** Read the next extension table record.
	*/
	status = dm0j_read(jnl_etab, &record, &l_record, &jsx->jsx_dberr);

	if ( status == E_DB_OK )
	    status = lobj_copy((PTR)atp, record, err_code);

	if ( status )
	    (VOID) dm0j_close(&jnl_etab, &local_dberr);
    }


    if ( status && jsx->jsx_dberr.err_code == E_DM0055_NONEXT )
    {
	status = E_DB_OK;
	CLRDBERR(&jsx->jsx_dberr);
    }

    if ( status == E_DB_OK )
    {
	/*
	** Write out the large object.
	*/
	if (atp->lobj_segindex == 1 && *(i2 *)&segarray[
		atp->lobj_segindex - 1] == 0)
	{
	    empty_dv.db_data = (char*)&null_data;
	    status = adc_getempty(&adf_cb, &empty_dv);
	    if ( status == E_DB_OK )
		status = CPwrite(DB_CNTSIZE + 1, empty_dv.db_data, &count, fdesc);
	}
	else
	{
	    if ((atp->lobj_reptype) && (atp->lobj_found_start == 0) &&
		(atp->lobj_segindex == 0))
	    {
		/* 
		** for repold/new, corresponding iietab delete/insert
		** record not found, put a placeholder in for lobj col.  
		*/
		char    lobj_label[32] = "{LOBJ COUPON}";
		u_i2    lobj_len = STlength(lobj_label);
		char    *segptr = (PTR)&segarray[0];

		MEcopy(&lobj_len, DB_CNTSIZE, (PTR)segptr);
		MEcopy( &lobj_label, STlength(lobj_label), 
				(PTR)(segptr + DB_CNTSIZE));
		atp->lobj_segindex++;
	    }

	    write_size = (atp->lobj_segindex - 1) *
			 DMPE_CPS_COMPONENT_PART_SIZE +
			 DB_CNTSIZE + *(i2 *)&segarray[
				atp->lobj_segindex - 1];
	    status = CPwrite(write_size, (PTR)&segarray, &count, fdesc);

	    if ( status == E_DB_OK )
	    {
		/*
		** Now, write out a NULL data value to signify the end of the
		** large object.
		*/
		empty_dv.db_data = (char*)&null_data;
		status = adc_getempty(&adf_cb, &empty_dv);
		if ( status == E_DB_OK )
		    status = CPwrite(DB_CNTSIZE + 1, empty_dv.db_data, &count, fdesc);
	    }
	}
    }

    return(status);
}

/*{
** Name: lobj_copy 	- Copy a piece of a Large Object.
**
** Description:
**	This procedure takes a Large Object coupon and writes it in the
**	right format to the output file specified by the user. This is so
**	that it can be copied back into a table properly.
**	
**	The function may be called directly from atp_lobj_copyout()
**	(non-clustered) or driven as a callback from dm0j_merge()
**	in a cluster environment.
**
**	It deals with a single journal record.
**
** Inputs:
**	atp			Pointer to audit context.
**	  lobj_tran_id		Pointer to the transaction id.
**	  lobj_base_tabid	Pointer to the base table id.
**	  lobj_coupon		Pointer to the lobj coupon.
**	  lobj_segarray		Pointer to segment array
**	  lobj_segindex		Current segment in the array
**	  lobj_reptype		In the case of DM0L_REP, this reflects whether
**				the coupon specified is that of the old record
**				(1) or the new record (2). Otherwise, it is zero.
**	  lobj_fdesc		Pointer to pointer to FILE for output.
**	record			Journal record of interest.
**
** Outputs:
**	atp
**	  lobj_segarray		updated from journal record.
**	  lobj_segindex		next segment in the array
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none.
**
** History:
**	06-May-2005 (jenjo02)
**	    Extracted from atp_lobj_copyout.
**	01-Mar-2010 (jonj) BUG 123357
**	    get_table_descriptor() won't return an ATP_TD
**	    if the etab_tbl_id from the journal record
**	    has been dropped from iirelation.
*/
static DB_STATUS
lobj_copy(
PTR		atp_ptr,
char		*record,
i4		*err_code)
{
    DMF_ATP		*atp = (DMF_ATP*)atp_ptr;
    DMF_JSX		*jsx = atp->atp_jsx;
    DB_STATUS		status = E_DB_OK;
    DM0L_HEADER		*h;
    DB_TAB_ID		etab_tbl_id;
    ATP_TD		*td;
    DMPE_RECORD		etab_record;
    i4			write_size;
    i4			count;
    i4			table_size;
    i4			owner_size;

    /* NB: "err_code" will point to jsx->jsx_dberr.err_code */

    CLRDBERR(&jsx->jsx_dberr);

    etab_tbl_id.db_tab_index = 0;

    switch (((DM0L_HEADER *)record)->type)
    {
	case DM0LDEL:
	    etab_tbl_id.db_tab_base =
		    ((DM0L_DEL *)record)->del_tbl_id.db_tab_base;
	    break;

	case DM0LPUT:
	    etab_tbl_id.db_tab_base =
		    ((DM0L_PUT *)record)->put_tbl_id.db_tab_base;
	    break;

	case DM0LREP:
	    etab_tbl_id.db_tab_base =
		    ((DM0L_REP *)record)->rep_tbl_id.db_tab_base;
	    break;

	default:
	    return(E_DB_OK);
    }

    td = get_table_descriptor(atp, &etab_tbl_id);

    if (!(atp->lobj_found_start) && (!td || !(td->relstat2 & TCB2_EXTENSION)))
	return(E_DB_OK);

    if (atp->lobj_found_start && td->relstat2 & TCB2_HAS_EXTENSIONS &&
	etab_tbl_id.db_tab_base == atp->lobj_base_tabid->db_tab_base &&
	etab_tbl_id.db_tab_index == atp->lobj_base_tabid->db_tab_index)
    {
	/*
	** We're at the base table journal entry. We're done.
	*/
        SETDBERR(&jsx->jsx_dberr, 0, E_DM0055_NONEXT);
	/* NB: "err_code" will point to jsx->jsx_dberr.err_code, but JIC */
	*err_code = E_DM0055_NONEXT;
	return(E_DB_ERROR);
    }

    h = (DM0L_HEADER *)record;
    if (h->tran_id.db_high_tran == atp->lobj_tran_id->db_high_tran &&
	h->tran_id.db_low_tran == atp->lobj_tran_id->db_low_tran)
    {
	switch (h->type)
	{
	    case DM0LDEL:
		table_size = ((DM0L_DEL *)h)->del_tab_size;
		owner_size = ((DM0L_DEL *)h)->del_own_size;
		MEcopy( &((DM0L_DEL *)h)->del_vbuf[table_size + owner_size],
			sizeof(DMPE_RECORD), (PTR)&etab_record );
		STRUCT_ASSIGN_MACRO(((DM0L_DEL *)h)->del_tbl_id,
				    etab_tbl_id);
		break;

	    case DM0LPUT:
		table_size = ((DM0L_PUT *)h)->put_tab_size;
		owner_size = ((DM0L_PUT *)h)->put_own_size;
		MEcopy( &((DM0L_PUT *)h)->put_vbuf[table_size + owner_size],
			sizeof(DMPE_RECORD), (PTR)&etab_record );
		STRUCT_ASSIGN_MACRO(((DM0L_PUT *)h)->put_tbl_id,
				    etab_tbl_id);
		break;

	    case DM0LREP:
		table_size = ((DM0L_REP *)h)->rep_tab_size;
		owner_size = ((DM0L_REP *)h)->rep_own_size;
		MEcopy( &((DM0L_REP *)h)->rep_vbuf[table_size + owner_size],
			sizeof(DMPE_RECORD), (PTR)&etab_record );
		STRUCT_ASSIGN_MACRO(((DM0L_REP *)h)->rep_tbl_id,
				    etab_tbl_id);
		break;
	}

	if ( atp->lobj_reptype )
	{
	    if ((atp->lobj_reptype == 1 && h->type == DM0LDEL) ||
		(atp->lobj_reptype == 2 && h->type == DM0LPUT))
	    {
		/*
		** In the case of a base table DM0LREP operation
		** we need to find the right extension table operations.
		*/
	    }
	    else
		return(E_DB_OK);
	}

	if (etab_tbl_id.db_tab_base == (u_i4)atp->lobj_coupon->cpn_etab_id &&
	    etab_record.prd_segment0 == 0 &&
	    etab_record.prd_segment1 == atp->lobj_segment1 &&
	    etab_record.prd_log_key.tlk_high_id ==
		atp->lobj_coupon->cpn_log_key.tlk_high_id &&
	    etab_record.prd_log_key.tlk_low_id ==
		atp->lobj_coupon->cpn_log_key.tlk_low_id)
	{
	    /*
	    ** This is an extension table record that we're interested in.
	    */
	    atp->lobj_found_start = TRUE;
	    if (atp->lobj_segindex == 32)
	    {
		write_size = (atp->lobj_segindex - 1) *
			     (DMPE_CPS_COMPONENT_PART_SIZE + 1) +
			     DB_CNTSIZE +
			     *(i2 *)&atp->lobj_segarray[
				atp->lobj_segindex] + 1;
		status = CPwrite(write_size, (char*)atp->lobj_segarray,
				 &count, atp->lobj_fdesc);
		atp->lobj_segindex = 0;
	    }

	    MEcopy( (char*)&etab_record.prd_user_space,
		    DMPE_CPS_COMPONENT_PART_SIZE,
		    (char*)&atp->lobj_segarray[atp->lobj_segindex] );

	    atp->lobj_segindex++;
	    atp->lobj_segment1++;
	}
    }

    return(status);
}

/*
** Name: format_emptyTuple() formats an empty tuple for a table
** Description:
**
** created this new routing which will format
** an empty tuple for the table record, this will
** be required to be called each time a BT and ET record
** is to be written out to a file.
**
** Inputs:
**    atp               Pointer to ATP context.
**    table_id          The table the record belongs to.
**    image             The record to display.
**    size              Size of the record.
**
** Outputs:
**    image            Points to the an empty formatted record.
**    size             The empty formatted size of the record.
**
** Returns:
**    E_DB_OK
**    E_DB_ERROR
**
** Exceptions:
**    none
**
** Side Effects:
**
**
** History:
**      11-sep-2000 (gupsh01)
**          Created.
*/
DB_STATUS format_emptyTuple(
        DMF_ATP *atp,
        DB_TAB_ID *table_id,
        PTR *image,
        i4 *size)
{
	DMF_JSX *jsx = atp->atp_jsx;
        ATP_TD  *td = NULL;
        DB_DATA_VALUE   dv;
        char    *tuple_buffer;
        char    *data_ptr;
        char    *l;
        i4      i = 0;
        i4      total = 0;
        i4      length = 0;
        DB_STATUS       status = E_DB_OK;
        i4      local_err;
        DMP_MISC       *datavalue_obj;
        char    *datavalue_ptr;
        i4      mem_needed;
	i4			*err_code = &jsx->jsx_dberr.err_code;

	CLRDBERR(&jsx->jsx_dberr);

        tuple_buffer = (char *) image;
        l = tuple_buffer;

        td = get_table_descriptor(atp, table_id);
        if (td)
        {
            for (i = 1; i <= td->data_rac.att_count; i++)
            {
                mem_needed= td->att_array[i].length + sizeof(DMP_MISC);
                status = dm0m_allocate(td->att_array[i].length + sizeof(DMP_MISC),
                    (i4)0, (i4)MISC_CB,
                    (i4)MISC_ASCII_ID, (char *)NULL, (DM_OBJECT**)&datavalue_obj, 
		    &jsx->jsx_dberr);

		if (status != OK)
		{
		    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
				(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		    uleFormat(NULL, E_DM9425_MEMORY_ALLOCATE, (CL_ERR_DESC *)NULL, ULE_LOG ,
				NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
				0, mem_needed);
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1211_ATP_MEM_ALLOCATE);
		    return (E_DB_ERROR);
		}

		datavalue_ptr = (char *)datavalue_obj + sizeof (DMP_MISC);
		datavalue_obj->misc_data = datavalue_ptr;

		if (td->att_array[i].type < 0)
		{
		    dv.db_datatype = 0 - td->att_array[i].type;
		    dv.db_length = td->att_array[i].length - 1;
		}
		else
		{
		    dv.db_datatype = abs (td->att_array[i].type);
		    dv.db_length = td->att_array[i].length;
		}

		dv.db_prec = td->att_array[i].precision;
		dv.db_data = datavalue_ptr;
		dv.db_collID = td->att_array[i].collID;

		/*
		** Convert the data into an empty value.
		*/

		status = adc_1getempty_rti( &adf_cb, &dv );

		length = dv.db_length;
		data_ptr = (char *)dv.db_data;

		if (DB_FAILURE_MACRO(status))
		{
		    status = E_DB_ERROR;
		    dmfWriteMsg(&adf_cb.adf_errcb.ad_dberror, 0, 0);
		    uleFormat(NULL, E_DM121A_ATP_LSN_NO_ALL, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			&local_err, 1, 0, 0);
		    length = 0;
		}

		/* add this value to the buffer */

		MEcopy((PTR)data_ptr, length, (PTR)l);

		/*
		** write a null byte to l in the end of
		** the record is nullable.
		*/

		if (td->att_array[i].type < 0)
		MEfill(1, (u_char)0, l);

		l += td->att_array[i].length;
		total += td->att_array[i].length;

		(VOID) dm0m_deallocate((DM_OBJECT**)&datavalue_obj);
	    }

	    /*
	    ** if total size is greater than size then replace
	    ** size by new total
	    */
	    if(total > *size)
	      *size = total;
        }
        else
        {
            status = E_DB_ERROR;
            uleFormat(NULL, E_DM121B_ATP_NOEMPTY_TUPLE, (CL_ERR_DESC *)NULL,
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 1, 0, 0);
        }
return status;
}

