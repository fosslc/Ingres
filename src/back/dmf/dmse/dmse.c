/*
** Copyright (c) 1985, 2009 Ingres Corporation
*/

/*
NO_OPTIM = ris_u64 i64_aix r64_us5 rs4_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <bt.h>
#include    <cm.h>
#include    <cs.h>
#include    <me.h>
#include    <mh.h>
#include    <pc.h>
#include    <sr.h>
#include    <tm.h>
#include    <tmtz.h>
#include    <tr.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <ulf.h>
#include    <adf.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dmse.h>
#include    <dmst.h>
#include    <dmftrace.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0m.h>
#include    <dm2f.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dmd.h>

/**
**
**  Name: DMSE.C - Sort Execution.
**
**  Description:
**      This file contains the routines that implement the DMF sort operations.
**
**          dmse_begin - Prepare to sort.
**          dmse_cost - Estimated cost to sort.
**          dmse_end - Terminate sort.
**          dmse_input_end - End the input phase.
**          dmse_get_record - Return sorted record.
**          dmse_put_record - Give record to be sorted.
**
**	    (Internal routines)
**	    new_root()
**	    tidy_dup()
**	    do_merge()
**	    input_run()
**	    output_run()
**	    write_record()
**	    read_record()
**
**	    (File handling routines--ML Sort technique #1 (default))
**	    srt0_open_files()
**	    srt0_translated_io()
**	    srt0_close_files()
**
**	    (File handling routines--ML Sort technique #2 (optional))
**	    srt1_open_files()
**	    srt1_translated_io()
**	    srt1_close_files()
**
**      The sort routines have the following optimizations.
**         
**	    o	Deallocate memory used for run phase and reallocate memory
**		for the merge phase.  Overall this should reduce the time
**		that large amounts of memory are in use.
**	    o	Change the dmse_get_record routine to pass a pointer to the
**		record back to the caller.  It is up to the caller to dispose
**		of the record before calling dmse_get_record for the next
**		record.
**	    o	Change the internal merge routines to handle pointers to records
**		from the read_record interface to the work file.
**	    o	Streamline the normal codepaths.
**	    o	Handle duplicate key checking when returning rows with
**		dmse_get_record.
**	    o	Handle duplicate removal as early as possible to reduce the
**		number of intermediate runs.
**	    o	If the heap never fills, the merge phase is skipped and records
**		are returned directly from the heap.
**	    o	Sometimes the heap size is adjusted to be larger than the
**		memory table indicates in the hope that the entire sort will
**		fit in the heap without filling it.
**
** DMF Sort Overview
**
** The DMF sort uses a heap sort technique, both to order the rows in the
** original runs written to external storage (assuming that the entire sort
** couldn't be performed in memory) and to order the rows read back in durin
** the merge phase (though far fewer row buffers are typically involved in 
** the merge operations). 
**
** A heap sort involves an array of buffer pointers, with the buffers 
** containing the actual records being sorted. The pointers and not the 
** buffers are manipulated during the sort process, significantly reducing 
** data movement overhead. The heap array is effectively a binary tree 
** structure in which the root node (slot 1) has exactly 2 children (slots
** 1 and 2), and in general, each node has exactly 2 children (slot i has 
** children 2*i and 2*i+1). 
**
** At any time during the sort, the heap array conforms to the heap property
** which requires that all records along a "branch" of the binary tree be 
** sorted. That is, heap[n] >= heap[n/2] >= heap[n/4] ... >= heap[1], and 
** heap[1] is always the smallest record in the heap. Adding a record to the
** heap involves 1 or 2 operations:
**  - if the heap is already full, the current root of the heap (heap[1]) is
**    written to secondary storage, and the remaining records are adjusted 
**    to replace the old root with the next smallest record (one pass down 
**    to the leaves of the heap is all that is necessary to do this).
**  - the new record is added at the end of the heap (or to the slot vacated by 
**    the "reheapifying" operation described above). A pass is made back up
**    the heap branch into which the record was inserted (slot i, slot i/2,
**    slot i/4, ...), swapping heap entries until the correct location is
**    found for the new record.
** The speed of the heap algorithm derives from the fact that each of these
** operations involves only log_base2(heap size) records. 
**
** For duplicates elimination sorts, duplicates are detected and discarded
** whenever permitted by the comparisons being made. This reduces the sizes
** of the intermediate runs and the complexity of the sort. The initial sort
** prepends each buffer with a run number, which then becomes part of the 
** sort key. This assures that the runs are written sequentially, and the
** records within each run are ordered. Run numbers are discarded for the
** merge phase.
**
**
**  History:
**      04-feb-1985 (derek)
**	    Written.
**	15-sept-1988 (nancy)
**	    Bug #3186 (and others) where small sorts with multiple runs return
**	    garbage values.  Fixed input_run() to do copy to get buffer for run
**	    nos. > 1 contents since they WON'T be on disk.
**	16-sept-1988 (nancy)
**	    Bug #3186 where sort returns "error trying to put a record" when
**	    do_merge gets bad return from SRclose.  Fixed do_merge() to do same
**	    check for SR_BADFILE that dmse_end() does.
**	24-Apr-89 (anton)
**	    Added local collation support
**	22-may-1989 (sandyh)
**	    Bug #5730 changed criteria to decide whether to read descriptor
**	    from memory or out of the SR file. This is an addendum fix to
**	    #3186.
**	03-Nov-1989 (fred)
**	    Added support for unsortable datatypes.  This work is detailed in
**	    the routines to which it applies.
**	29-jan-90 (jrb)
**	    Made temporary fix to dmse_cost for integer overflow.
**	28-mar-90 (nancy)
**	    Fixed dmse_put_record(). See comments below.
**	5-june-90 (jkb)
**	    Add another check for integer overflow 
**	24-sep-90 (rogerk)
**	    Merged 6.3 changes into 6.5.
**      02-feb-91 (jennifer)
**         Fixed bug 34235 by adding DMSE_EXTERNAL flag indicating
**  	   extimates are from external source, so a smaller number of 
**         pages are initially allocated.  Here's Rogerk's note when he
**	   integrated Jennifer's fix: "Integrated Jennifer's fix to avoid
**	   pre-allocating super-large sort files.  Add new sort flag -
**	   DMSE_EXTERNAL - which indicates that the rows are being fed to the
**	   sorter from an outside source.  In this case we do not trust the
**	   row_count estimate and do not try to pre-allocate the space for the
**	   sort files.  We start with a small amount (64 pages) and add space
**	   at runtime as required.  This prevents us from trying to allocate 50
**	   GB of disk space whenever we predict that a 10-way join will produce
**	   a trillion records."
**	17-jan-1992 (bryanp)
**	    Added enhancements for in-memory sorting.
**	6-jul-1992 (bryanp)
**	    Prototyping DMF.
**      25-sep-1992 (stevet)
**          Call TMtz_init() for the default timezone instead of TMzone.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	26-oct-92 (rickh)
**	    Replaced references to DMP_ATTS with DB_ATTS.  Also replaced
**	    DM_CMP_LIST with DB_CMP_LIST.
**	06-nov-92 (jrb)
**	    Added support for Multi-Location Sorts (overview: changed interface
**	    to dmse_begin to accept list of work locations, added new file-level
**	    routines, restructured DMS_SRT_CB to hold location lists)
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	14-dec-92 (jrb)
**	    Changed dmse_cost to fix various problems and make output more
**	    stable.
**	15-mar-1993 (rogerk)
**	    Fix compiler warnings.
**	30-mar-1993 (rmuth)
**	    In dmse_begin make a local var an i4 as we were overflowing.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Prototyped static routines in this file.
**		Cast some parameters to quiet compiler complaints following
**		    prototyping of LK, CK, JF, SR.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-oct-93 (jrb)
**          More changes for MLSorts: support for using a subset of all work
**          locations for smaller sorts.
**	31-jan-94 (jrb)
**	    Fixed bug 58017; open files in output_run if they were not yet
**	    open.
**	05-nov-93 (swm)
**	    Bug #58633
**	    Alignment offset calculations used a hard-wired value of 3
**	    to force alignment to 4 byte boundaries, for pointer alignment.
**	    Changed these calculations to use the ME_ALIGN_MACRO.
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**      06-mar-96 (nanpr01)
**          Increased tuple size project changes. 
**	    Make sure we allocate enough memory for merge runs if dmf
**	    in-memory sort cannot be used.
**	18-Jul-1997 (schte01)
**          Make sure sort control block and  arrays (SR_IO, DB_CMP_LIST)
**	    are aligned properly in dmse_begin. Unaligned CS_SEMAPHORE was 
**       causing unaligned access errors on alpha.
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dmse_begin() prototype, s_log_id to DMS_SRT_CB
**	    so that it can be passed down to dm2t_reclaim_tcb().
**	28-nov-97 (inkdo01)
**	    Various changes to speed sorting. There were several cases of 
**	    more comparisons being applied than necessary. Also introduced
**	    more code for early duplicates detection, including a "free chain"
**	    to track available buffers/heap slots.
**	    Also added a description of the heap sort process to the comment
**	    block in front of the module. 
**	15-jan-98 (inkdo01)
**	    Started changes for parallel DMF sort. Split old logic of
**	    major entry point functions into local "serial" functions,
**	    called from parent logic drivers.
**	27-Jul-1998 consi01 bug 81284 problem ingsrv 337
**          When no row estimate is available use all avalable work locations
**          for the sort.
**	20-apr-1999 (hanch04)
**	    Size should be size of a pointer.  Needed for 64 bit ptrs. 
**      18-jan-1999 (stial01)
**          dmse_begin_serial() Init cmp_type for all AD_NOSORT atts B100026
**      23-May-2000 (hweho01)
**          Turned off optimization for ris_u64 (AIX 64-bit platform).
**          Symptom: failure on createdb.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	15-Dec-2000 (ahaal01)
**	    Turned off optimization for rs4_us5 (AIX 32-bit platform).
**	    Symptom: sysmod brought dbms server (iidbms) down.
**      01-may-2001 (stial01)
**          Init adf_ucollation, added ucollation parameter to dmse_begin*
**	16-aug-2001 (toumi01)
**	    NO_OPTIM i64_aix because of SEGV doing "createdb -S iidbdb"
**	04-Apr-2003 (jenjo02)
**	    Refined parent/child sync, provide table_id to child
**	    DMS_SRT_CB so that dm2f_filename() yields a better
**	    name than one based on (0,0).
**	    Ensure that child threads reach certain barriers
**	    before allowing parent to procede (and perhaps free
**	    memory, srt_sem, before children are done with them).
**      23-Jule-2003 (hweho01)
**          Added r64_us5 (64-bit on AIX) into NO_OPTIM list.
**          Symptom: failure on creating iidbdb. 
**          Compiler : VisualAge 5.023.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	31-Mar-2004 (jenjo02)
**	    Restructured to use CS_CONDITION objects for 
**	    Parent/Child thread synchronization rather than
**	    just CS_SEMAPHORE.
**	17-Jun-2004 (hanch04)
**	    back out test code that should not have been submitted.
**      05-Nov-2004 (hweho01)
**          removed rs4_us5 and r64_us5 from NO_OPTIM list.
**      30-Dec-2004 (hweho01)
**          Restored NO_OPTIM request for AIX (rs4_us5 and r64_us5).
**          avoid runtime error during the process of creating iidbdb.
**          Compiler : C for AIX 6.006 (VisualAge).
**      12-Jan-2004 (hweho01)
**          Removed "*" char. from the NO_OPTIM line, so mkjam can 
**          process it correctly.
**      04-Jun-2005 (thaju02)
**          Large row width may exhaust memory. 
**	    dmse_begin: for srt_iwidth > 65536, revert to serial sort 
**	    and for srt_width >= 32768 and <=65536, use bsize of 100. 
**	    (B114636)
**	7-Jun-2005 (schka24)
**	    Be a bit more generous with sort memory.  (More work needs
**	    to be done in this area.)
**    25-Oct-2005 (hanje04)
**        Add prototype for new_root() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration
**	28-Jan-2006 (kschendel)
**	    Fix sync problems between parent and child, caused partly by
**	    mistaken idea that cbix/pbix were mutex protected.
**	    Rename STE_PWAIT to STE_PRSTRT since that's what it is,
**	    an indicator that the parent-restart point has been set
**	    and the parent MIGHT be waiting.
**	22-aug-07 (hayke02 & kibro01)
**	    Add memory barrier/fence calls (CS_membar()) before ste_pbix
**	    is incremented. This prevents the increment taking place before
**	    other instructions are executed i.e. the MEcopy() of record into
**	    xbp in dmse_put_record(), leading to incorrect values appearing in
**	    the exchange buffers and/or sort heap due to the parent overtaking
**	    the children. This change fixes bug 118591.
**	23-aug-07 (kibro01)
**	    Change CS_membar to CS_MEMBAR_MACRO
**	28-Nov-2007 (kschendel) b122041
**	    Use %lx for sid, %u for compare counts in trdisplays.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm2t_?, dmse_? functions converted to DB_ERROR *
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**      05-feb-2009 (joea)
**          Rename CM_ischarsetUTF8 to CMischarset_utf8.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**      22-Jun-2009 (hweho01) B122231
**          Need to re-check STE_CEOG for end of gets before executing 
**          parent_suspend() call, because the child may already race  
**          into completion.
**          Modify the formatting string in TRdisplay, avoid segv 
**          when trace point was turned on. The size of srt_sid would be  
**          8 or 4, depends on the build modes.
**	19-Aug-2009 (kschendel) 121804
**	    Need cm.h for proper CM declarations (gcc 4.3).
**	17-Nov-2009 (kschendel) SIR 122890
**	    Increase the DMF sort memory parameters a bit.  (We really need
**	    some sort of scaling config parameter here.)  It's not
**	    unreasonable to use 8 Mb (per sort thread) for sorting >100 Mb
**	    worth of stuff!
**      19-mar-2010 (joea)
**          In dmse_begin_serial, ensure the parallel sort buffers are properly
**          aligned.
**/

/*
**      Flag values to output_run().
*/

#define			START_OUTPUT_PASS   0x01L
#define                 STOP_OUTPUT_PASS    0x02L
#define                 BEGIN_OUTPUT_RUN    0x04L
#define			END_OUTPUT_RUN	    0x08L

/*
**      Flag values to input_run().
*/

#define                 START_INPUT_PASS    0x01L
#define			END_INPUT_PASS	    0x02L
#define			BEGIN_INPUT_RUN    0x04L

/*
**  Definition of static variables and forward static functions.
*/

static VOID 	 parent_suspend(
			DMS_SRT_CB        *sort_cb);
static	DB_STATUS   write_record(DMS_SRT_CB *sort_cb, char *record);
static	DB_STATUS   read_record(DMS_MERGE_DESC *merge_desc);
static	DB_STATUS   output_run(DMS_SRT_CB *sort_cb, i4 flag);
static	DB_STATUS   input_run(DMS_SRT_CB *sort_cb, i4 flag);
static	DB_STATUS   do_merge(DMS_SRT_CB *sort_cb, i4 passes);
static 	void 	    tidy_dup(
			DMS_SRT_CB	*sort_cb,
			i4		stop,
			i4		current);

static	DB_STATUS   get_in_memory_record(
			DMS_SRT_CB        *sort_cb,
			char               **record,
			DB_ERROR	*dberr);

static DB_STATUS srt0_open_files(
			DMS_SRT_CB	*s,
			i4		in_or_out);

static STATUS srt0_translated_io(
			DMS_SRT_CB          *s,
			i4		    read_write,
			i4		    in_or_out,
			i4		    blkno,
			char		    *buffer,
			CL_ERR_DESC	    *err);

static STATUS srt0_close_files(
			DMS_SRT_CB	    *s,
			i4		    in_or_out,
			CL_ERR_DESC	    *err);

static DB_STATUS srt1_open_files(
			DMS_SRT_CB	*s,
			i4		in_or_out);

static STATUS srt1_translated_io(
			DMS_SRT_CB          *s,
			i4		    read_write,
			i4		    in_or_out,
			i4		    blkno,
			char		    *buffer,
			CL_ERR_DESC	    *err);

static STATUS srt1_close_files(
			DMS_SRT_CB	    *s,
			i4		    in_or_out,
			CL_ERR_DESC	    *err);

static DB_STATUS new_root(
			DMS_SRT_CB	*sort_cb,
			i4		root,
			DB_ERROR	*dberr);

const static	struct MEMORY_INFO
{
    i4		bytes;		/* Bytes of data to sort. */
    i4		memory;		/* Amount of memory to allocate. */
    i4		disk_buffer;	/* Size of the initial disk buffer. */
    i4		extra;
}   memory_info[] =
	{
	    { 256 * 1024, 256 * 1024, 8 * 1024, 0 },
	    { 1024 * 1024, 512 * 1024, 16 * 1024, 0 },
	    { 4 * 1024 * 1024, 1024 * 1024, 32 * 1024, 0 },
	    { 10 * 1024 * 1024, 2048 * 1024, 64 * 1024, 0 },
	    { 30 * 1024 * 1024, 3072 * 1024, 64 * 1024, 0 },
	    { 100 * 1024 * 1024, 4096 * 1024, 128 * 1024, 0 },
	    { MAXI4,             8192 * 1024, 256 * 1024, 0 }
	};

/** Name: dmse_begin	- Prepare to sort.
**
** Description:
**      This routine is the external entry to DMF sort preparation. It 
**	determines whether this sort should be executed using parallel 
**	sort threads, or serially using the calling thread. If the former,
**	it locates and prepares the child threads (in the sort thread pool)
**	which will execute the sort, as well as building the controlling
**	thread's sort control block.
**
** Inputs:
**      flags                           Sort operation modifers.  Zero or
**					DMSE_ELIMINATE_DUPLICATES:
**					    Remove duplicate rows while sorting.
**					DMSE_CHECK_KEYS:
**					    Error if duplicate key values are
**					    encountered while sorting.
**					DMSE_EXTERNAL:
**					    Rows and rowcount estimates are
**					    generated from the client of DMF,
**					    rather than internally inside DMF.
**	table_id			Id of table being sorted.
**      att_list                        Pointer to array of attributes.
**	att_count			Count of the number of attributes.
**	key_count			Number of attributes in key.
**	cmp_count			How many attributes to consider
**					by SRT_ELIMINATE.
**	loc_array			Array of locations from the DCB.
**	wrkloc_mask			A bitmask relative to loc_array which
**					indicates which locations in the array
**					are to be used for sorting.
**	mask_size			Number of bits in wrkloc_mask.
**	spill_index			The first work loc to use.
**	width				Width of the record.
**	records				Estimated record count.  This is used
**					to calculate the amount of memory
**					the size of external buffers and the
**					merge pattern.
**      uiptr                           Pointer to a i4 in memory 
**                                      indicating if user has interrupted.   
**	lock_list			Lock list of current transaction.
**
** Outputs:
**      sort_cb                         The address of the SORT control block.
**	err_code			The reason for an error return.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR			E_DM9707_SORT_BEGIN
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-feb-98 (inkdo01)
**          Written for parallel sort project.
**	20-jul-98 (inkdo01)
**	    Changes to accomodate > 32 bit work location masks (assumption
**	    of 32 was in error).
**	23-nov-98 (inkdo01)
**	    Changes to support semaphore in parent thread DMS_SRT_CB for 
**	    parent/child synchronization.
**	24-dec-98 (inkdo01)
**	    Use semaphore to prevent children from starting until parent
**	    has finished initialization.
**	04-Apr-2003 (jenjo02)
**	    Added table_id to dmse_begin(), dmse_begin_serial()
**	    prototypes.
**	14-Apr-2005 (jenjo02)
**	    Added "cmp_count", srt_c_count to tweak SRT_ELIMINATE compares.
**	    Sort fields to the right of cmp_count will be excluded
**	    from equality consideration when eliminating duplicates.
**	28-Apr-2005 (jenjo02)
**	    Additional tweaks for above change. Add srt_rac_count to 
**	    compensate when "cmp_count" differs from att_count.
**	04-Jun-2005 (thaju02)
**	    Large row width may exhaust memory. For srt_iwidth > 65536, 
**	    revert to serial sort and for srt_width >= 32768 and <=65536, 
**	    use bsize of 100.  (B114636)
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Start kids with DMF facility so they can use
**	    ShortTerm memory.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
**	23-Mar-2010 (kschendel) SIR 123448
**	    Allow caller to suppress parallel sort (e.g. for partitioned
**	    bulk-load) by passing DMSE_NOPARALLEL flag.
*/
DB_STATUS
dmse_begin(
i4            flags,
DB_TAB_ID     *table_id,
DB_CMP_LIST	   *att_list,
i4            att_count,
i4		   key_count,
i4		   cmp_count,
DMP_LOC_ENTRY	   *loc_array,
i4		   *wrkloc_mask,
i4		   mask_size,
i4		   *spill_index,
i4            width,
i4            records,
i4            *uiptr,
i4		   lock_list,
i4		   log_id,
DMS_SRT_CB         **sort_cb,
PTR		   collation,
PTR		   ucollation,
DB_ERROR           *dberr)
{
    DM_SVCB	*svcb = dmf_svcb;
    i4		*wrklocvec[MAX_MERGE];
    DB_STATUS	status;
    i4		i, j;
    i4		bsize;
    i4		*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    for (;;)
    {
	/* First determine whether parallel sort is possible/adviseable.
	** The sort must be large enough, parallel sort threads must be
	** enabled in the config, this must not be a sort child, and
	** DM503 must not be set.
	*/
	if (svcb->svcb_stnthreads && svcb->svcb_strows <= records &&
		    !(flags & DMSE_CHILD) &&
		    !(flags & DMSE_NOPARALLEL) &&
		    !DMZ_SRT_MACRO(3))
	{
	    DM_STECB	*step;
	    DMS_SRT_CB	*s;
	    SCF_CB		scfcb;
	    SCF_FTC		ftc;
	    i4		mem;
	    i4		recs;
	    char		sem_name[CS_SEM_NAME_LEN+16];
	    char		thread_name[DB_MAXNAME*2];

	    if (DMZ_SRT_MACRO(4)) 
		svcb->svcb_stnthreads = 1;
	    else if (DMZ_SRT_MACRO(5)) 
		svcb->svcb_stnthreads = 2;
	    else if (DMZ_SRT_MACRO(6)) 
		svcb->svcb_stnthreads = 3;

	    /* Now prepare parent's sort cb, noting we're parallel'ing. */
	    status = dmse_begin_serial((flags | DMSE_PARALLEL), 
		table_id,
		att_list, 
		att_count, key_count, cmp_count, 
		loc_array, wrkloc_mask, mask_size,
		spill_index, width, records, uiptr, lock_list, log_id,
		sort_cb, collation, ucollation, dberr);
	    if (status != E_DB_OK) 
		return(status);

	    /* Start by zeroing thread blocks, just to be sure. */
	    MEfill(sizeof(SCF_CB), (u_char)0, (PTR)&scfcb);
	    MEfill(sizeof(SCF_FTC), (u_char)0, (PTR)&ftc);

	    scfcb.scf_type = SCF_CB_TYPE;	/* init SCF control blocks */
	    scfcb.scf_length = sizeof(SCF_CB);
	    scfcb.scf_session = DB_NOSESSION;
	    scfcb.scf_facility = DB_DMF_ID;
	    scfcb.scf_ptr_union.scf_ftc = &ftc;

	    /* So the kids can use ShortTerm DMF memory */
	    ftc.ftc_facilities = 1 << DB_DMF_ID;
	    ftc.ftc_data_length = 0;		/* don't copy the parm */
	    ftc.ftc_thread_name = thread_name;
	    ftc.ftc_priority = SCF_CURPRIORITY;
	    ftc.ftc_thread_entry = dmse_child_thread;
	    ftc.ftc_thread_exit = NULL;

	    s = *sort_cb;			/* parent thread sort cb */
	    width = s->srt_iwidth;		/* use internal width */

	    if (width >= 32768)
	    {
		if (width > 65536)
		    break;	/* revert to serial */
		else
		    bsize = 100;
	    }
	    else
		bsize = svcb->svcb_stbsize;

	    if (DMZ_SRT_MACRO(8)) 
		bsize = 500;
	    else if (DMZ_SRT_MACRO(9)) 
		bsize = 5000;
	    else if (DMZ_SRT_MACRO(10)) 
		bsize = 25000;

	    if (DMZ_SRT_MACRO(1)) 
		TRdisplay("%p BSIZE - %d\n", s->srt_sid, bsize);

	    mem = bsize * (sizeof(char *) + width);
						/* memory for exchange bufs */
	    recs = records / svcb->svcb_stnthreads;
	    s->srt_childix = 0;			/* 1st child to process */

	    /* If there are enough work locations, assign them separately
	    ** to the threads - not all to each thread. This improves
	    ** concurrency. */

	    for (j = 0; j < svcb->svcb_stnthreads; j++)
		wrklocvec[j] = wrkloc_mask;	/* first, init the vector */

	    if (BTcount((PTR)wrkloc_mask, mask_size) >= svcb->svcb_stnthreads)
	    {
		for (i = 0; i < svcb->svcb_stnthreads; i++) 
		{	/* first init them all */
		    PTR		wptr;
		    STATUS	memstat;
						
		    wptr = MEreqmem((u_i2)0, (u_i4)(mask_size/BITSPERBYTE),
			TRUE, &memstat);
		    wrklocvec[i] = (i4 *)wptr;  /* start of workloc vector */
		}
		for (j = 0, i = -1; (i = BTnext(i, (PTR)wrkloc_mask, 
			mask_size)) >= 0; j++)
		{
		    if (j == svcb->svcb_stnthreads) j = 0;
		    BTset(i, (PTR)wrklocvec[j]);
		}
	    }
		
	    /* Layout the synchronization semaphore. */
	    CScnd_init(&s->srt_cond);
	    CSw_semaphore(&s->srt_cond_sem, CS_SEM_SINGLE, 
			    STprintf(sem_name, "PsortP (%d,%d)",
					table_id->db_tab_base,
					table_id->db_tab_index
					    & ~DB_TAB_PARTITION_MASK));

	    s->srt_pstate = 0;

	    /* Loop over thread descriptors (DM_STECB's) prep'ing them 
	    ** for the dispatch of the corresponding threads. */

	    for (i = 0; i < svcb->svcb_stnthreads; i++)
	    {
		step = s->srt_stecbp[i];
		step->ste_pbix = 0;
		step->ste_cbix = 0;
		step->ste_npwaits = 0;
		step->ste_ncwaits = 0;
		step->ste_c_puts = 0;
		step->ste_c_gets = 0;
		step->ste_wrkloc = wrklocvec[i];
		step->ste_ccb = NULL;
		step->ste_pcb = s;		/* stuff parent sort cb */
		step->ste_records = recs;	/* rows per thread */
		step->ste_cflag = 0;
		step->ste_npres = 0;
		step->ste_ncres = 0;
		step->ste_bsize = bsize;
		step->ste_crstrt = step->ste_bsize * 3 / 4; 
		step->ste_prstrt = 0;

		STprintf(thread_name,
			    "<%PsortC (%d,%d).%d>",
			    table_id->db_tab_base,
			    table_id->db_tab_index
				& ~DB_TAB_PARTITION_MASK,
			    i);
		/*
		** NB: While the Child thread may terminate when
		**     it finishes its GETs, its STECB and 
		**     exchange buffer must endure until the
		**     Parent is done with it, which is to say
		** 	   the Child can't free this memory.
		*/
		status = dm0m_allocate(mem + sizeof(DMP_MISC), (i4)0,
			(i4)MISC_CB, (i4)MISC_ASCII_ID, 
			(char *)s, (DM_OBJECT **)&step->ste_misc, dberr);

		if ( status == E_DB_OK )
		{
		    ftc.ftc_data = (PTR)step;	/* This thread's cb */
		    status = scf_call(SCS_ATFACTOT, &scfcb);

		    if ( status )
			dm0m_deallocate((DM_OBJECT**)&step->ste_misc);
		}
		if ( status )
		    break;
		step->ste_misc->misc_data = ((char*)(step->ste_misc)) +
					sizeof(DMP_MISC);
	    }

	    /* Any kids spawned? */
	    if ( (s->srt_nthreads = i) > 0 )
	    {
		CSp_semaphore(1, &s->srt_cond_sem);
		s->srt_init_active += i;
		s->srt_puts_active += i;

		/* Wait til all kids are initialized */
		while ( s->srt_init_active )
		    parent_suspend(s);

		CSv_semaphore(&s->srt_cond_sem);

		/* All kids spawned and runable? */
		if ( status == E_DB_OK && s->srt_pstatus == E_DB_OK )
		    return(E_DB_OK);
	    }

	    /* Couldn't start up (all) the kids */
	    status = dmse_end(s, dberr); /* free to re-start as serial */
	    if (status != E_DB_OK) 
		return(status);

	    /* drop to serial logic */
	}	
	break;
    }

    /* If we get this far, fall through and do serial sort. */
	
    return(dmse_begin_serial(flags, table_id, att_list, att_count, 
				key_count, cmp_count, loc_array,
				wrkloc_mask, mask_size, spill_index, 
				width, records, uiptr, lock_list,
				log_id, sort_cb, collation, ucollation,
				dberr));
}

/** Name: dmse_begin_serial - Prepare to sort.
**
** Description:
**      This routine accepts a description of the fields to be sorted, the
**	width of the record, the estimated number of records, and some
**	flags.  Using this information it allocates an SORT control block,
**      initializes it, and returns the control block the caller.
**
** Inputs:
**      flags                           Sort operation modifers.  Zero or
**					DMSE_ELIMINATE_DUPLICATES:
**					    Remove duplicate rows while sorting.
**					DMSE_CHECK_KEYS:
**					    Error if duplicate key values are
**					    encountered while sorting.
**					DMSE_EXTERNAL:
**					    Rows and rowcount estimates are
**					    generated from the client of DMF,
**					    rather than internally inside DMF.
**	table_id			Id of table being sorted.
**      att_list                        Pointer to array of attributes.
**	att_count			Count of the number of attributes.
**	key_count			Number of attributes in key.
**	cmp_count			How many attributes to consider
**					by SRT_ELIMINATE.
**	loc_array			Array of locations from the DCB.
**	wrkloc_mask			A bitmask relative to loc_array which
**					indicates which locations in the array
**					are to be used for sorting.
**	mask_size			Number of bits in wrkloc_mask.
**	spill_index			The first work loc to use.
**	width				Width of the record.
**	records				Estimated record count.  This is used
**					to calculate the amount of memory
**					the size of external buffers and the
**					merge pattern.
**      uiptr                           Pointer to a i4 in memory 
**                                      indicating if user has interrupted.   
**	lock_list			Lock list of current transaction.
**
** Outputs:
**      sort_cb                         The address of the SORT control block.
**	err_code			The reason for an error return.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR			E_DM9707_SORT_BEGIN
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	04-feb-1986 (derek)
**          Created new for jupiter.
**	24-Apr-89 (anton)
**	    Added local collation support
**	26-jul-89 (jrb)
**	    Initialize cmp_precision field.
**	03-nov-89 (fred)
**	    Added support for unsortable datatypes.  Check datatypes on input
**	    for whether they are sortable.  If not, replace the type
**	    specification with DB_DIF_TYPE, telling adt_sortcmp() to just
**	    return that the tuples are different if so requested.
**      02-feb-91 (jennifer)
**         Fixed bug 34235 by adding DMSE_EXTERNAL flag indicating
**  	   extimates are from external source, so a smaller number of 
**         pages initially allocated.  See file-level history comment .
**	09-oct-91 (jrb) integrated the following change: 19-nov-90 (rogerk)
**	    Initialize the server's timezone setting in the adf control block.
**	    This was done as part of the DBMS Timezone changes.
**	17-jan-1992 (bryanp)
**	    In-memory sorting enhancements.
**      25-sep-1992 (stevet)
**          Call TMtz_init() for the default timezone instead of TMzone.
**	06-nov-92 (jrb)
**	    Changed for multi-location sorts; interface changed, more memory
**	    allocated now (for SR contexts), and new fields in SRT_CB are
**	    initialized.
**	30-mar-1993 (rmuth)
**	    Change bytes to an i4 as we were overflowing.
**      16-oct-93 (jrb)
**          Added support for using a subset of work locs.
**	03-feb-94 (swm)
**	    Bug #59610
**	    DMF sorter corrupts memory pool during createdb on axp_osf.
**	    Corrected calculation of sort record size offset in heap
**	    pointer initialisation. It was:
**		record += awidth + sizeof(i4 *);
**	    corrected it to:
**		record += awidth + sizeof(i4);
**	    This bug is only exhibited on platforms where sizeof(i4 *)
**	    is greater than sizeof(i4); eg. axp_osf. The symptom is
**	    that we write past the end of allocated memory and corrupt the
**	    DM0M memory pool.
**	9-may-94 (robf)
**          Update error handling on adt_sortcmp, this can fail so we need
**	    so check it, especially in the case of invalid security labels.
**      06-mar-96 (nanpr01)
**          Increased tuple size project changes. 
**	    Make sure we allocate enough memory for merge runs if dmf
**	    in-memory sort cannot be used.
**	18-Jul-1997 (schte01)
**          Make sure sort control block and  arrays (SR_IO, DB_CMP_LIST)
**	    are aligned properly. Unaligned CS_SEMAPHORE was causing unaligned
**	    access errors on alpha.
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dmse_begin() prototype, s_log_id to DMS_SRT_CB
**	    so that it can be passed down to dm2t_reclaim_tcb().
**      27-Jul-1998 consi01 bug 81284 problem ingsrv 337
**          When no row estimate is available use all avalable work locations
**          for the sort.
**      18-jan-1999 (stial01)
**          dmse_begin_serial() Init cmp_type for all AD_NOSORT atts B100026
**	1-mar-00 (inkdo01)
**	    Minor fix to alignment of insertion of default run number, to support
**	    both 32- and 64-bit servers (bug 100663).
**	04-Apr-2003 (jenjo02)
**	    Added table_id to dmse_begin_serial() prototype.
**	    Added session owner (srt_sid) to DMS_SRT_CB.
**	01-Apr-2004 (jenjo02)
**	    Memory allocation for parallel wrong, testing wrong
**	    flag SRT_PARALLEL instead of DMSE_PARALLEL.
**	16-dec-04 (inkdo01)
**	    Init cmp_collID.
**	21-feb-05 (inkdo01)
**	    Init. Unicode normalization flag.
**	15-may-2007 (gupsh01)
**	    Initialize adf_utf8_flag.
**	06-jun-2007 (toumi01)
**	    Protect against NULL return value from GET_DML_SCB.
**      13-Jan-2009 (hanal04) Bug 121496
**          Fully initialise s->srt_adfcb to avoid spurious exceptions
**          during sort execution.
*/
DB_STATUS
dmse_begin_serial(
i4            flags,
DB_TAB_ID     *table_id,
DB_CMP_LIST	   *att_list,
i4            att_count,
i4		   key_count,
i4		   cmp_count,
DMP_LOC_ENTRY	   *loc_array,
i4		   *wrkloc_mask,
i4		   mask_size,
i4		   *spill_index,
i4            width,
i4            records,
i4            *uiptr,
i4		   lock_list,
i4		   log_id,
DMS_SRT_CB         **sort_cb,
PTR		   collation,
PTR		   ucollation,
DB_ERROR           *dberr)
{
    DM_SVCB		*svcb = dmf_svcb;
    DMS_SRT_CB		*s;
    i4		i;
    i4             awidth;
    i4		block_size;
    i4		memory;
    i4			bytes;
    i4		num_locs;
    i4		avail_locs;
    i4		sr_entries;
    i4             max_merge_order;
    DB_STATUS		status;
    STATUS              stat;
    SYSTIME	systime;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    TMnow(&systime);

    /* How many work locations are available for use */
    avail_locs = BTcount((char *)wrkloc_mask, mask_size);

    if (avail_locs < 1)
    {
	SETDBERR(dberr, 0, E_DM013E_TOO_FEW_LOCS);
	return(E_DB_ERROR);
    }
    if (avail_locs > DM_WRKLOC_MAX)
    {
	SETDBERR(dberr, 0, E_DM013F_TOO_MANY_LOCS);
	return(E_DB_ERROR);
    }

    /* Compute adjusted row width and number of bytes in sort */
    awidth = DB_ALIGN_MACRO(width);
    if (records > MAXI4 / awidth)
	bytes = MAXI4;
    else
	bytes = awidth * records;

    if (bytes == 0 || (num_locs = bytes / DM_PG_SIZE /
                     DM_WRKSPILL_SIZE + 1) > avail_locs)
	num_locs = avail_locs;

    /* Figure number of SR_IO blocks we're going to need--depends on
    ** whether we're using technique #1 or #2 for striping the temp files.
    ** Technique #2 is turned on with a trace point.
    */
    if (DMZ_SRT_MACRO(2))
	sr_entries = num_locs + (num_locs & 1);
    else
	sr_entries = num_locs * 2;
	
    /*
    **	Allocate the SORT control block.  The sort control block contains the
    **	SORT_CB, the sort compare attributes and the last tuple returned if
    **	duplicates are being eliminated.
    */
    
    memory = DB_ALIGN_MACRO(sizeof(DMS_SRT_CB));
    memory += (att_count + 2) * DB_ALIGN_MACRO(sizeof(DB_CMP_LIST));
    memory += sr_entries * DB_ALIGN_MACRO(sizeof(SR_IO));

    if (flags & (DMSE_ELIMINATE_DUPS | DMSE_CHECK_KEYS))
	memory += awidth;

    status = dm0m_allocate(memory, (i4)0, (i4)SORT_CB,
	(i4)SORT_ASCII_ID, (char *)0, (DM_OBJECT **)sort_cb, dberr);
    if (status != E_DB_OK)
    {
	SETDBERR(dberr, 0, E_DM9704_SORT_ALLMEM_ERROR);
	return (E_DB_ERROR);
    }
    s = *sort_cb;

/* TRdisplay("Sort CB: %p, %p, %d\n", s, &s->srt_nthreads, memory); */
    /*
    **	Compute the amount of memory to use for the merge phase.
    **
    **	Select the best memory and buffer configuration based on the minimum
    **	memory required and the desired amount of memory to allocate and the
    **	disk block transfer size.
    **
    **	If the sort is sufficiently small, then we may be able to adjust the
    **	heap size upward to have a strong likelihood of holding the whole sort.
    **	Figure out how much memory is enough to hold all the estimated records
    **	in a single large heap. Fudge an extra 5% into the calculations in case
    **	the estimates are off.
    */
    for (i = 0; i < sizeof(memory_info) / sizeof(struct MEMORY_INFO) - 1; i++)
    {
	if (bytes <= memory_info[i].bytes)
	    break;
    }
    memory = memory_info[i].memory;
    block_size = memory_info[i].disk_buffer;

    if (bytes < svcb->svcb_int_sort_size)
    {
	if (records < 100)
	    records = 100;	/* fudge up to assure at least mimimal alloc. */
	memory = (records + (records * .05)) *
		    (awidth + sizeof(char *) + sizeof(i4));
	memory += block_size;
	if (memory < memory_info[i].memory)
	    memory = memory_info[i].memory;
    }
    else 
    {
      /****************************************************************/
      /* Adjust the memory size to make sure that enough merge memory */
      /* has been allocated                                           */
      /****************************************************************/
      max_merge_order = (memory - sizeof(DMP_MISC) - block_size) /
	(block_size + sizeof(DMS_MERGE_DESC) + awidth + 2 * sizeof(char *));
      if (max_merge_order <= 2)
      {
	 memory = (3 * (block_size + sizeof(DMS_MERGE_DESC) + awidth + 
		   2 * sizeof(char *))) + sizeof(DMP_MISC) + block_size; 
      }
    }

    /* dyn memory is different for parallel */
    if (flags & DMSE_PARALLEL) 
	memory = (svcb->svcb_stnthreads+1) *
		(sizeof(DMS_PMERGE_DESC) + sizeof(DMS_PMERGE_DESC *)) +
		svcb->svcb_stnthreads * sizeof(DM_STECB);

    /*	Initialize other miscellaneous variables. */

    CSget_sid(&s->srt_sid);
    CLRDBERR(&s->srt_dberr);
    s->srt_width = width;
    s->srt_iwidth = awidth;
    s->srt_o_offset = 0;
    s->srt_o_blkno = 0;
    s->srt_o_size = block_size;
    s->srt_i_size = block_size;
    s->srt_next_record = 0;
    s->srt_run_number = 0;
    s->srt_c_reads = 0;
    s->srt_c_writes = 0;
    s->srt_c_records = 0;
    s->srt_c_comps = 0;
    s->srt_misc = 0;
    s->srt_uiptr = uiptr;
    s->srt_lk_id = lock_list;
    s->srt_log_id = log_id;
    STRUCT_ASSIGN_MACRO(*table_id, s->srt_table_id);
    MEfill(sizeof(s->srt_adfcb), (u_char)0, (PTR)&s->srt_adfcb);
    s->srt_adfcb.adf_maxstring = DB_MAXSTRING;
    s->srt_adfcb.adf_collation = collation;
    s->srt_adfcb.adf_ucollation = ucollation;
    if(CMischarset_utf8())
      s->srt_adfcb.adf_utf8_flag = AD_UTF8_ENABLED;
    s->srt_swapped = FALSE;
    s->srt_num_locs  = num_locs;
    s->srt_array_loc = loc_array;
    s->srt_wloc_mask = wrkloc_mask;
    s->srt_mask_size = mask_size;
    s->srt_spill_index = *spill_index;
    s->srt_free = NULL;
    s->srt_c_free = 0;
    s->srt_nthreads = 0;
    s->srt_threads = 0;
    s->srt_init_active = 0;
    s->srt_puts_active = 0;
    s->srt_pstatus = E_DB_OK;
    CLRDBERR(&s->srt_pdberr);
    s->srt_secs = systime.TM_secs;
    s->srt_msecs = systime.TM_msecs;
    for (i = 0; i < MAX_MERGE; i++) 
    {
	s->srt_orun_desc.runs[i].offset  = 0;
	s->srt_orun_desc.runs[i].blkno   = 0;
	s->srt_orun_desc.runs[i].records = 0;
	s->srt_irun_desc.runs[i].offset  = 0;
	s->srt_irun_desc.runs[i].blkno   = 0;
	s->srt_irun_desc.runs[i].records = 0;
	s->srt_stecbp[i] = NULL;
    }

    /* Now update the spill index; note: this is actually a non-semphored
    ** DCB update, but we don't worry about it.
    */
    *spill_index = (*spill_index + num_locs) % avail_locs;

    /* The following is needed only if we're using the splitting method for
    ** multi-location sorts, but might as well set in both cases.
    */
    s->srt_split_loc = (num_locs & 1) ? num_locs/2 : -1;

    stat = TMtz_init(&s->srt_adfcb.adf_tzcb);
    if (stat != OK)
    {
	SETDBERR(dberr, 0, stat);
	dm0m_deallocate((DM_OBJECT **)sort_cb);
	return (E_DB_ERROR);
    }

    /*
    ** Calculate the number of blocks required to sort the estimated 
    ** rows.  If the rows are being generated from outside DMF, then
    ** don't trust the estimate and only preallocate a small amount
    ** of space.  If the sort is internal, then we assume the disk space
    ** calculation will be accurate and we attempt to preallocate all
    ** the space up front.
    */
    /* These are EXTERNAL estimates which can be grossly
    ** incorrect, so start with a relatively small file
    */
    if (flags & DMSE_EXTERNAL)
	s->srt_blocks = 64;
    else
	s->srt_blocks = (bytes / block_size) * 9 / 10;

    s->srt_ratts = (DB_CMP_LIST *)((char *)s +
		DB_ALIGN_MACRO(sizeof(DMS_SRT_CB)));
    s->srt_sr_files = (SR_IO *)((char *)s->srt_ratts + ((att_count + 2) *
		DB_ALIGN_MACRO(sizeof(DB_CMP_LIST))));
    s->srt_duplicate = (char *)s->srt_sr_files + (sr_entries *
		DB_ALIGN_MACRO(sizeof(SR_IO)));
    
    /*	Allocate the buffer for the run-creation phase. */

    status = dm0m_allocate(memory + sizeof(DMP_MISC) + sizeof(ALIGN_RESTRICT), 
                             (i4)0, (i4)MISC_CB,
			     (i4)MISC_ASCII_ID, (char *)s, 
                             (DM_OBJECT **)&s->srt_misc, dberr);
    if (status != E_DB_OK)
    {
	SETDBERR(dberr, 0, E_DM9704_SORT_ALLMEM_ERROR);
	dm0m_deallocate((DM_OBJECT **)sort_cb);
	return (E_DB_ERROR);
    }
    s->srt_misc->misc_data = ((char*)(s->srt_misc)) + sizeof(DMP_MISC);

    /* If parallel sort, layout dynamic memory here. The heap array only
    ** contains DMS_PMERGE_DESC's - one per sort thread - to contain the
    ** final merge of rows from the sorted fragments. 
    */
    if (flags & DMSE_PARALLEL)
    {
	i4	**heap;
	char	*record;

	s->srt_misc->misc_data = (char *)&s->srt_misc[1];
					/* 1st byte past DMP_MISC header */
	s->srt_heap = (i4 **)s->srt_misc->misc_data;
					/* it's all heap space */
	s->srt_o_buffer = NULL;
	s->srt_last_record = svcb->svcb_stnthreads;
	s->srt_record = (char *)ME_ALIGN_MACRO(s->srt_heap + sizeof(i4 *) *
			(s->srt_last_record + 1), sizeof(ALIGN_RESTRICT));
					/* point past the ptr array */

	heap = &s->srt_heap[1];
	record = s->srt_record;
	for (i = 1; i <= s->srt_last_record; i++)
	{
	    *heap = (i4 *)record;
	    record += sizeof(DMS_PMERGE_DESC);
	    heap++;
	}

	/* Finally, assign addrs of the DM_STECB's */
	for (i = 0; i < svcb->svcb_stnthreads; 
				i++, record += sizeof(DM_STECB))
	    s->srt_stecbp[i] = (DM_STECB *)record;
/* TRdisplay("Heap: %p, %p, %p, %d\n", s->srt_misc, record, heap,
     (memory+sizeof(DMP_MISC))); */
    }	/* end of DMSE_PARALLEL */

    /*
    ** The following code is tricky and wrong. But I don't want to change it
    ** because the timing is bad (we're near a release) and because the code is
    ** sensitive here. The following code puts a bunch of pointers into the
    ** memory we just allocated, but the math it does (particularly the
    ** calculation of srt_last_record) is probably wrong. If you follow it very
    ** carefully, you will see that it allows too much memory (better than not
    ** enough, since that would cause memory corruption) between pointers.
    **
    ** It seems to work right now.
    */
    else
    {
	s->srt_misc->misc_data = (char *)&s->srt_misc[1];
	s->srt_o_buffer = (char *)&s->srt_misc[1];
	s->srt_heap = (i4**)
	    ((char *)s->srt_o_buffer + block_size - sizeof(i4 *));
	s->srt_last_record = 
	    (memory - ((char *)s->srt_heap - 
                   (char *)s->srt_misc + sizeof(i4 *))) /
	    (awidth + sizeof(char *) + sizeof(i4 *));
	s->srt_record = (char *)s->srt_heap + 
	    sizeof(i4 *) * (s->srt_last_record + 1);

	/*	Initialize the heap pointers and the run-number for each record. */

	{
	    i4		**heap = &s->srt_heap[1];
	    char		*record = s->srt_record;

	    for (i = 1; i <= s->srt_last_record; i++)
	    {
		*heap = (i4 *)record;
		record += awidth + sizeof(i4 *);
		*((PTR *) heap) += sizeof(PTR);
		/* Place default run number (1) in i4 immediately 
		** preceding the row buffer. */
		((i4 *)(*heap))[-1] = 1;
		heap++;
	    }
	}
    }	/* end of !DMSE_PARALLEL */

    /*	Create the attribute comparison list. */

    MEcopy((PTR)att_list, (att_count * sizeof(DB_CMP_LIST)),
           (PTR)&s->srt_ratts[1]);

    {
	i4	    dt_bits;

	for (i = 1; i <= att_count; i++)
	{
	    status = adi_dtinfo(&s->srt_adfcb,
				    (DB_DT_ID) s->srt_ratts[i].cmp_type,
				    &dt_bits);
	    if (status)
	    {
		uleFormat(&s->srt_adfcb.adf_errcb.ad_dberror, 0, NULL,
				ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		SETDBERR(dberr, 0, E_DMF012_ADT_FAIL);
		dm0m_deallocate((DM_OBJECT **)&s->srt_misc);
		dm0m_deallocate((DM_OBJECT **)sort_cb);
		return (E_DB_ERROR);
       	    }
	    else if (dt_bits & AD_NOSORT)
	    {
		s->srt_ratts[i].cmp_type = DB_DIF_TYPE;
		/* but continue, to check for other AD_NOSORT cols */
	    }
	}
    }
    /*	Insert comparison on run number. */

    s->srt_ratts[0].cmp_offset = -((i4)sizeof(i4));
    s->srt_ratts[0].cmp_type = DB_INT_TYPE;
    s->srt_ratts[0].cmp_precision = 0;
    s->srt_ratts[0].cmp_length = 4;
    s->srt_ratts[0].cmp_direction = 0;
    s->srt_ratts[0].cmp_collID = -1;
    s->srt_ra_count = att_count + 1;
    s->srt_rac_count = cmp_count + 1;
    s->srt_atts = &s->srt_ratts[1];
    s->srt_a_count = att_count;
    s->srt_k_count = key_count;
    s->srt_c_count = cmp_count;

    /*	Mark the SORT_CB as ready to accept input. */

    s->srt_state = SRT_PUT;
    if (flags & DMSE_ELIMINATE_DUPS)
	s->srt_state |= SRT_ELIMINATE | SRT_CHECK_DUP;
    if (flags & DMSE_CHECK_KEYS)
	s->srt_state |= SRT_CHECK_KEY | SRT_CHECK_DUP;
    if (flags & DMSE_PARALLEL) 
    {
	s->srt_state |= SRT_PARALLEL;
    }
    else if ((svcb->svcb_status & (SVCB_SINGLEUSER | SVCB_RCP)) == 0)
    {
	DML_SCB *scb;

	/* Non-parallel sorts need occasional polls for user interrupts
	** during merging, but only if we're the real user thread.
	** Otherwise it's just a waste.
	*/
	scb = GET_DML_SCB(s->srt_sid);
	if (scb)
	    if ((scb->scb_s_type & SCB_S_FACTOTUM) == 0)
		s->srt_state |= SRT_NEED_CSSWITCH;
    }

    /*	Prepare the first work file for processing. */

    status = output_run(s, START_OUTPUT_PASS | BEGIN_OUTPUT_RUN);

    if (status == E_DB_OK)
    {
	if (DMZ_SRT_MACRO(1))
	    TRdisplay("%p DMSE_BEGIN: Records = %d Width = %d Atts = %d Heap = %d Memory = %d\n",
		s->srt_sid,
		records, width, att_count, s->srt_last_record, 
                s->srt_misc->misc_length);
	return (E_DB_OK);
    }

    *dberr = s->srt_dberr;
    /*	Handle error in output_run call. */

    dm0m_deallocate((DM_OBJECT **)&s->srt_misc);
    dm0m_deallocate((DM_OBJECT **)sort_cb);

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9707_SORT_BEGIN);
    }
    return (E_DB_ERROR);
}

/*{
** Name: dmse_cost	- Calculate the cost of a sort.
**
** Description:
**      Calculate the cost of a proposed sort.
**
** Inputs:
**      records                         Number of records to sort.
**      width				Width of each record.
**
** Outputs:
**      io_cost                         Cost in # of I/O operations.
**	cpu_cost			Cost in # of comparisions.
**	Returns:
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      04-sep-1986 (Derek)
**          Created for Jupiter.
**	29-jan-90 (jrb)
**	    Made temporary fix to avoid integer overflow while computing cpu
**	    and io cost estimates.  Proper fix entails doing some fundamental
**	    reworking of exception handling in the cl.
**	5-june-90 (jkb)
**	    Add another check for integer overflow
**	14-dec-92 (jrb)
**	    Changed return type to VOID since this routine never rtns an error.
**	    Also changed to be more tolerant of changes to the DMS_SRT_CB's
**	    size and fixed obvious bug in calculation of "buckets".
**      06-mar-96 (nanpr01)
**          Increased tuple size project changes. 
**	    Make sure we allocate enough memory for merge runs if dmf
**	    in-memory sort cannot be used. If we donot adjust the memory size,
**	    total_runs calculation may get into an infinite loop. 
**	10-may-05 (inkdo01)
**	    Change work fields to double to get rid of MAXI4 constraints.
**	10-feb-06 (wanfr01, inkdo01)
**	    Bug 115743
**	    Use '4' rather than sizeof(PTR) in order to standardize sort cost
**	    estimation between 32 bit and 64 bit platforms.  This avoids getting
**	    different (and possibly worse) qeps after upgrading to 64 bit.
**	27-july-06 (dougi)
**	    Changes to factor down sort cost estimates (which were previously
**	    ludicrously high - especially CPU) and use float parameters to
**	    allow larger sort sizes.
**	6-oct-2006 (dougi)
**	    Change above to multiplicative factors - they are a bit more 
**	    intuitive than division, even if they're < 1.0 and have the effect
**	    of reducing the estimates.
*/
VOID
dmse_cost(
f4             records,
i4		    width,
f4		    *io_cost,
f4		    *cpu_cost)
{
    DM_SVCB	*svcb = dmf_svcb;
    i4		i;
    double	e_cpu_cost;
    i4		memory;
    double	bytes;
    double	dbuckets;
    i4		buckets;
    i4		block_size;
    i4		merge_order;
    i4		merge_passes;
    double	awidth;
    i4		eliminate_duplicates = 0;
    double	druns;
    i4		init_runs;
    i4		init_records;
    i4		total_runs;
    double	add_val;
    i4		srtcb_size;

    awidth = width;

    /* round srtcb_size up to next multiple of 1024 so dmse_cost output will
    ** be less dependent on small changes to the DMS_SRT_CB
    */
    srtcb_size = (sizeof(DMS_SRT_CB) + 1023) & ~ 1023;

    /*
    **	Compute the minimum amount of memory required for the sort.
    */

    memory = 3 * MIN_BLOCK_SIZE + srtcb_size + awidth +
	     2 * (awidth + sizeof(PTR)) + sizeof(DB_CMP_LIST) * DB_MAX_COLS;

    /*	Compute the number of bytes to sort. */

    bytes = awidth * (double)records;

    /*
    **	Select the best memory and buffer configuration based on the minimum
    **	memory required and the desired amount of memory to allocate and the
    **	disk block transfer size.
    */

    for (i = 0; i < sizeof(memory_info) / sizeof(struct MEMORY_INFO) - 1; i++)
    {
	if (memory <= memory_info[i].memory &&
	    bytes <= memory_info[i].bytes)
	{
	    break;
	}
    }

    memory = memory_info[i].memory;
    block_size = memory_info[i].disk_buffer;

    /****************************************************************/
    /* Adjust the memory size to make sure that enough merge memory */
    /* has been allocated                                           */
    /****************************************************************/
    merge_order = (memory - sizeof(DMP_MISC) - block_size) /
	(block_size + sizeof(DMS_MERGE_DESC) + awidth + 2 * sizeof(char *));
    if (merge_order <= 2)
    {
	memory = (3 * (block_size + sizeof(DMS_MERGE_DESC) + awidth + 
		 2 * sizeof(char *))) + sizeof(DMP_MISC) + block_size; 
    }

    /*
    **	Compute the number of records that will fit in memory for the
    **	initial run creation.
    */

    buckets = (memory - (block_size + srtcb_size + awidth +
			    sizeof(DB_CMP_LIST) * DB_MAX_COLS))
		/ (awidth + 8);

    /*
    **	Compute:
    **	     cost = # comparsions needed to populate the heap.
    */

    {
	i4	    height = 1;
	i4	    count = 2;

	e_cpu_cost = 0.0;
	init_records = buckets;
	if (buckets > records)
	    init_records = records;
	do
	{
	    add_val = ((init_records > count + count ? count + count - 1
		: init_records) - (count - 1)) * height;

	    e_cpu_cost += add_val;
	    
	    height++;
	    count += count;
	}	while (count < init_records);
    }

    /*
    **	Compute the CPU cost to create the initial merge runs.
    **
    **	    cpu_p_cost = C(populate) + C(create_sorted_runs) + C(unpopulate)
    **		+ C(eliminate_duplicates)
    **
    **	    C(populate) is the number of comparison needed to load and build
    **		the heap.
    **
    **	    C(unpopulate) is the number of comparison needed to unload the
    **		set of records left in the heap after all records have been
    **		passed in.
    **
    **	    C(create_sorted_runs) is the number of comparisions needed to
    **		created the sorted runs assuming that the heap is fully
    **		populated.
    **
    **	    C(eliminate_duplicates) is the number of comparisons needed
    **		to eliminate duplicate records while creating the initial
    **		merge runs.
    */

    {
	double	    height,add_flt;

	height = MHln((double)(buckets + 1)) / MHln(2.0);
	if (height < 1.0)
	    height = 1.0;

	for (;;)
	{
		e_cpu_cost += e_cpu_cost;
		add_flt = 2.0 * ((double)(height - 1)) * 
				((double)(records - init_records));

		add_val = add_flt;

		e_cpu_cost += add_val;
		add_val = (records - 1) * eliminate_duplicates;
		e_cpu_cost += add_val;
		break;
	}
    }

    /*
    **	The number of runs created is estimated given that the average
    **	length of a run created by a tournament sort, is equal to twice
    **  the number of records that can fit in memory.  In the best case
    **	a sorted result is created by the initial run creation pass.
    */

    dbuckets = (double) (buckets + buckets);
    druns = (records + dbuckets) / dbuckets;

    /* init_runs can be prohibitively high, causing problems later in
    ** the estimation process. Test runs on 100G TPC H show 2M to be a 
    ** reasonable upper bound - such large sorts are unlikely to be 
    ** selected for a query plan anyway. */
    if (druns > 2000000.0)
	init_runs = 2000000;
    else init_runs = (i4)druns;

    /*
    **	Compute the best merge order.  The best merge order is the minimum
    **	breadth neccessary to merge in the minimum number of passes.
    */

    {
	i4		max_merge_order;
	i4		runs;
	i4		passes;
	i4		best_passes;
	i4		best_runs;
	i4		best_order;

	max_merge_order = (memory - (srtcb_size + awidth + block_size)) /
	    (block_size + sizeof(DMS_MERGE_DESC) + 
			      awidth + sizeof(char *));
	if (max_merge_order > MAX_MERGE)
	    max_merge_order = MAX_MERGE;

	for (best_passes = 32, best_runs = 1024, i = 2; 
                          i <= max_merge_order; i++)
	{
	    i4	runs;
	    i4	passes;

	    for (runs = i, passes = 1; runs < init_runs; runs *= i, passes++)
		;

	    if (passes < best_passes || (passes == best_passes && 
				       runs - init_runs < best_runs))
	    {
		best_passes = passes;
		best_runs = runs - init_runs;
		best_order = i;
	    }
	}
	if (init_runs == 1)
		best_order = 1;

	merge_order = best_order;
	merge_passes = best_passes;
    }

    /*
    **	Compute the total number of runs that will be generated and
    **	leave init_runs equal to the number of runs to use for
    **	the final merge.
    */

    for (total_runs = 0; init_runs > merge_order; )
    {
	total_runs += init_runs;
	init_runs = (init_runs + merge_order) / merge_order;
    }
    total_runs += init_runs;

    /*
    **	Compute the cpu cost to merge the runs.
    **
    **	    The cost to perfrom all but the last merge run is the
    **	    number of comparisons to k-way merge n records for each
    **	    pass.
    */

    
    {
	double	    height;

	height = MHln((double)(merge_order + 1)) / MHln(2.0);
	if (height < 1.0)
	    height = 1.0;

	add_val = records * 2 * (height - 1) * (merge_passes - 1);
	e_cpu_cost += add_val;
    
	/*
	**	    The final merge pass might use a smaller merge order so
	**	    we have to calculate the cost for the last pass seperately.
	*/

	height = MHln((double)(merge_order * init_runs)) / MHln(2.0);
	if (height < 1.0)
	    height = 1.0;

	add_val = ((double)records) * 2.0 * (height - 1);
	e_cpu_cost += add_val;
    }
    *cpu_cost = e_cpu_cost;

    /*
    **	The I/O cost to merge is equal to the number of blocks that
    **	are written and read from the work files for each pass plus
    **	the number of rereads from thr work file for overlapping
    **	runs plus the number of merge descriptor reads and writes.
    */

    add_val = ((bytes + block_size) / block_size);
    *io_cost = add_val * merge_passes * 2;
    add_val = total_runs + 4 * (total_runs + MAX_MERGE) / MAX_MERGE;
    *io_cost += add_val;

    /* Apply sort estimate adjustment factors. */
    *cpu_cost *= svcb->svcb_sort_cpufactor;
    *io_cost *= svcb->svcb_sort_iofactor;
    return;
}

/*{
** Name: dmse_put_record	- Add record to be sorted.
**
** Description:
**      This routine is the external entry for passing a row to be
**	sorted by DMF sort. It determines whether the sort is serial
**	(in which case the row is passed straight to the serial put 
**	function) or parallel (in which case, the put is added to 
**	the exchange buffer of the chosen child thread).
**
** Inputs:
**      sort_cb                         The sort control block.
**	record				A pointer to the record.
**
** Outputs:
**      err_code                        Reason for error return status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR			E_DM970B_SORT_PUT
**					E_DM9706_SORT_PROTOCOL_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-feb-98 (inkdo01) 
**          Written for parallel sort project.
**	22-jul-98 (inkdo01)
**	    Support new suspend function for better error synchronization.
**	25-nov-98 (inkdo01)
**	    Add support for semaphore to assure synchronization of psorts.
**	19-Apr-2004 (schka24)
**	    Recheck wait condition with semaphore held to avoid having
**	    both ends go to sleep.
**	1-Feb-2006 (kschendel)
**	    Above recheck was bogus because child counts up cbix
**	    without semaphore protection.  Fix comments.
**	12-Feb-2007 (kschendel)
**	    Just put round-robin to sort children, remove code that
**	    stuffed rows at a waiting child.  Children are faster, and
**	    in some environments thread scheduling was such that we
**	    tended to see "child waiting" all the time.  The result was
**	    that one child would get the preponderance of rows, negating
**	    much of the benefit of parallelism.
**	9-May-2007 (smeke01) b118280.
**	    Added check on child's WAIT flag before parent suspends. 
*/

DB_STATUS
dmse_put_record(
DMS_SRT_CB	    *sort_cb,
char		    *record,
DB_ERROR	    *dberr)
{
    DM_SVCB	*svcb = dmf_svcb;
    DM_STECB	*step;
    DM_STECB	**stepp;
    char	*xbp;
    i4	i, j;
    i4	pbix;
    DB_STATUS	status;
    bool	really_wait;

    if (!(sort_cb->srt_state & SRT_PARALLEL))
	return(dmse_put_record_serial(sort_cb, record, dberr));

    CLRDBERR(dberr);

    /* Parallel sort - identify child to receive this row. */

    sort_cb->srt_c_records++;

    /* Loop 'til we've found somewhere to put it */
    while ( (sort_cb->srt_pstate & SRT_PCERR) == 0 )
    {
	i = sort_cb->srt_childix;
	for (j = sort_cb->srt_nthreads; j > 0; --j)
	{
	    step = sort_cb->srt_stecbp[i];
	    if ((step->ste_pbix - step->ste_cbix) < (step->ste_bsize-1)) 
		break;			/* "-1" keeps them at safe distance */
	    ++ i;
	    if (i >= sort_cb->srt_nthreads) i = 0;
					/* next thread index */
	}

	if (j > 0) 
	    break;	/* got one - leave loop */
 
	/* Parent has apparently caught up to child on all threads. 
	** In this situation (unlikely as it may be, since the parent is 
	** typically much slower than the kids), parent is waited until 
	** one or more children gets 3/4 way round to the parent again.
	** Be careful not to wait if a child has raced ahead all of a
	** sudden, lest we go to sleep at the same time the children
	** think that the buffer is empty and go to sleep too...
	*/

	CSp_semaphore(1, &sort_cb->srt_cond_sem);

	really_wait = TRUE;
	for (j = 0; j < sort_cb->srt_nthreads; j++)
	{
	    step = sort_cb->srt_stecbp[j];
	    /* Recheck.  
	    ** (1) The child can continue to read and update the cbix even
	    ** whilst we have the semaphore, just as the parent can update
	    ** the pbix whilst the child has the semaphore. (2) However neither
	    ** child nor parent can update its respective WAIT flag without 
	    ** having the semaphore. Therefore we know that the child is either
	    ** active and cannot suspend itself until we've released the semaphore,
	    ** or it has already suspended itself and flagged this. Note that due
	    ** to (1) above, it is possible for the child to have grabbed the 
	    ** semaphore intending to suspend, but did not manage to update the
	    ** WAIT flag until after the parent had re-filled its buffer. Hence the 
	    ** check on the child's WAIT flag below (b118280). 
	    */
	    if ((step->ste_pbix - step->ste_cbix) < (step->ste_bsize-1))
	    {
		really_wait = FALSE;
		break;
	    }
	    else if (step->ste_cflag & STE_CWAIT)
	    {
		/* There's a full buffer for the child, but it's asleep!
		** So wake it up. (b118280).
		*/
	        step->ste_cflag &= ~STE_CWAIT;
	        CScnd_signal(&sort_cb->srt_cond, step->ste_ctid);
	    }
	    step->ste_prstrt = step->ste_pbix - step->ste_bsize / 4;
	    /* Tell child that prstrt is set, Parent will wait.  This
	    ** also prevents a child that races ahead from waiting, since
	    ** flag tests ARE semaphore protected.
	    */
	    step->ste_cflag |= STE_PRSTRT;
	}
	/* Wait on any kid */
	if (really_wait)
	    parent_suspend(sort_cb);

	CSv_semaphore(&sort_cb->srt_cond_sem);
    }	/* end of suspend loop */
    
    if ( (sort_cb->srt_pstate & SRT_PCERR) == 0 )
    {
	/* Address next exchange buffer entry and fill in the PUT. */
	pbix = step->ste_pbix % step->ste_bsize;
	xbp = step->ste_xbufp[pbix];
	MEcopy((PTR)record, sort_cb->srt_iwidth, (PTR)xbp);

	/*
	** If Parent has reach Child restart point and
	** the Child is waiting for that event,
	** wake it up.  (re-checking cwait after we get the
	** mutex, to make sure that the child is really sleeping
	** and not terminated or something.)
	*/
	CS_MEMBAR_MACRO();
	if ( ++step->ste_pbix >= step->ste_crstrt &&
	     step->ste_cflag & STE_CWAIT )
	{
	    CSp_semaphore(1, &sort_cb->srt_cond_sem);
	    if ( step->ste_cflag & STE_CWAIT
	      && step->ste_pbix >= step->ste_crstrt)
	    {
		/* Go ahead and clear cwait so that we don't bash at
		** the kid over and over.
		*/
		step->ste_cflag &= ~STE_CWAIT;
		CScnd_signal(&sort_cb->srt_cond, step->ste_ctid);
	    }
	    CSv_semaphore(&sort_cb->srt_cond_sem);
	}

	/* Put round-robin to kids for an even distribution of rows */
	if (++i >= sort_cb->srt_nthreads)
	    i = 0;
	sort_cb->srt_childix = i;
    }

    if ( sort_cb->srt_pstatus )
	*dberr = sort_cb->srt_pdberr;
    return(sort_cb->srt_pstatus);
}

/*{
** Name: dmse_put_record_serial	- Add record to be sorted.
**
** Description:
**      This routine adds a record to the set of records to be sorted.
**
** Inputs:
**      sort_cb                         The sort control block.
**	record				A pointer to the record.
**
** Outputs:
**      err_code                        Reason for error return status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR			E_DM970B_SORT_PUT
**					E_DM9706_SORT_PROTOCOL_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	04-feb-1986 (derek)
**          Created new for jupiter.
**	28-mar-1990 (nancy)
**	    (no bug number).  MEcopy into heap should use width of record
**	    (actual length) instead of iwidth.  This was causing server crash
**	    on machines where record buffer size was not rounded up.
**	17-jan-1992 (bryanp)
**	    Record whether or not the heap ever became full.
**	28-nov-97 (inkdo01)
**	    Several algorithmic improvements to reduce required number of
**	    comparisons. Also includes more logic to discard duplicates and
**	    implement free chain for tracking available buffers/heap slots
**	    freed by dynamically detected duplicates.
*/
DB_STATUS
dmse_put_record_serial(
DMS_SRT_CB	    *sort_cb,
char		    *record,
DB_ERROR	    *dberr)
{
    DMS_SRT_CB         *s = sort_cb;
    char		*x;
    char		*y;
    i4		i;
    i4		j;
    i4		branchend;
    i4		**heap = s->srt_heap;
    i4		runno;
    i4			cmpres;
    DB_STATUS		status;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (s->srt_state & SRT_PUT)
    {
	/*	Assign a heap slot for this record. */

	s->srt_c_records++;
	x = (char *) heap[1];
	runno = ((i4 *)x)[-1];		/* save root run number */
	if (s->srt_free == NULL &&
		(i = ++s->srt_next_record) > s->srt_last_record)
	{
	    /*  Heap is close to full. First see if the new record belongs 
	    ** to this run or the next or if it's a droppable duplicate. */

	    --s->srt_next_record;
	    s->srt_adfcb.adf_errcb.ad_errcode=0;
	    s->srt_c_comps++;
	    cmpres = adt_sortcmp(&s->srt_adfcb, s->srt_atts, 
                                  s->srt_a_count, record, (char *)x, 0);
	    if(s->srt_adfcb.adf_errcb.ad_errcode!=0)
	    {
	        uleFormat(&s->srt_adfcb.adf_errcb.ad_dberror, 0,
				NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		SETDBERR(dberr, 0, E_DM970B_SORT_PUT);
		return (E_DB_ERROR);
	    }
	    if ( s->srt_state & SRT_ELIMINATE &&
		(cmpres == 0 || abs(cmpres) > s->srt_c_count) )
	    {
		    return(E_DB_OK);	/* just drop the new guy */
	    }

	    /*
	    ** The heap IS full.  Write the smallest record to the output run.
	    */

	    s->srt_state |= SRT_HEAP_BECAME_FULL;
	    status = write_record(s, x);
	    if (status != E_DB_OK)
	    {
		*dberr = s->srt_dberr;

		if (dberr->err_code > E_DM_INTERNAL)
		{
		    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		    SETDBERR(dberr, 0, E_DM970B_SORT_PUT);
		}
		return (E_DB_ERROR);
	    }

	    if (cmpres < 0) runno++;		/* new row smaller ==> new no */

	    /*	Bring smallest row to root AND reheapify. */

	    status = new_root(s, 1, dberr);
	    if (status != E_DB_OK) 
		return(status);

	    /* Find slot to place new record. */
	    if (s->srt_free != NULL)	/* try free chain first */
	    {
		s->srt_c_free--;
		x = s->srt_free;
		s->srt_free = *((char **)x);
		i = ((i4 *)x)[-1];
		heap[i] = (i4 *)x;		/* reattach buffer to array */
	    }
	    else			/* else take last slot in heap */
	    {
		i = s->srt_next_record;
		x = (char *)heap[i];
	    }

	    ((i4 *)x)[-1] = runno;			/* save run number */
	    MEcopy(record, s->srt_iwidth, (char *)x);	/* copy row */
		
	}	/* end of heap full */
	else
	{
	    /* Heap isn't completely full. A new slot can be found without
	    ** writing the root to an output run.
	    **
	    ** Copy record to next available slot, then check for possible
	    ** need to increment run number. This only needs doing if this
	    ** is a duplicates elimination sort (because of early elimination
	    ** logic in this function), and even then only after we've 
	    ** started writing a run. */

	    /* Find slot to place new record. */
	    if (s->srt_free != NULL)
	    {
		s->srt_c_free--;
		x = s->srt_free;
		s->srt_free = *((char **)x);
		i = ((i4 *)x)[-1];
		heap[i] = (i4 *)x;		/* reattach buffer to array */
	    }
	    else
	    {
		i = s->srt_next_record;
		x = (char *)heap[i];
	    }

	    MEcopy(record, s->srt_iwidth, (char *)heap[i]);
	    ((i4 *)x)[-1] = runno;		/* reload run number */

	    /* If heap has been full, a record with higher key may already
	    ** have been written to an output run. So compare new record
	    ** to current root and increment its run number, if necessary. */
	    if (s->srt_state & SRT_ELIMINATE &&
		s->srt_state & SRT_HEAP_BECAME_FULL)
	    {
		s->srt_c_comps++;
		cmpres = adt_sortcmp(&s->srt_adfcb, s->srt_atts, 
                                  s->srt_a_count, record, (char *)heap[1], 0);
		if (cmpres < 0) (((i4 *)x)[-1])++;
	    }
	}

	/* New record is at the end of some heap branch. Now move it up to
	** its proper position, re-heapifying in the process. */

	branchend = i;				/* save for duplicates elim. */
        s->srt_adfcb.adf_errcb.ad_errcode=0;
	for (; j = i, i >>= 1; )
	{
	    s->srt_c_comps++;
	    if ((cmpres = adt_sortcmp(&s->srt_adfcb, s->srt_ratts, 
		s->srt_ra_count, (char *)heap[i], (char *)heap[j], 0)) > 0)
	    {
		/* If new record is less than current, swap 'em. */
		x = (char *)heap[i];
		heap[i] = heap[j];
		heap[j] = (i4*) x;
		continue;
	    }
	    if(s->srt_adfcb.adf_errcb.ad_errcode!=0)
	    {
	        uleFormat(&s->srt_adfcb.adf_errcb.ad_dberror, 0, 
				NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		SETDBERR(dberr, 0, E_DM970B_SORT_PUT);
		return (E_DB_ERROR);
	    }
	    if ( s->srt_state & SRT_ELIMINATE &&
		 (cmpres == 0 || abs(cmpres) > s->srt_rac_count) )
	    {
		/* Found a duplicate to drop - just restore current
		** heap branch. */
		if (j < branchend)
		{	
		    /* Only restore if something's been moved. */
		    x = (char *)heap[j];
		    tidy_dup(s, j, branchend);
						/* recurse if more'n one
						** level needs restoring */
		    heap[branchend] = (i4 *)x;
						/* restore top ptr */
		}
		/* Duplicate has been dropped and heap branch restored. Now
		** add the slot just freed to the free chain. Note, free chain 
		** entries are buffer/heap slot pairs and that they are
		** in ascending order of slot number. */
		/* if (branchend == s->srt_next_record) s->srt_next_record--;
		else */
		{
		    char	**z;
		    char	**oldz;

		    s->srt_c_free++;
		    x = (char *)heap[branchend];
		    heap[branchend] = NULL;
		    ((i4 *)x)[-1] = branchend;	
					/* place slot number into buffer */
		    if (s->srt_free == NULL)
		    {
			s->srt_free = x;
			*((char **)x) = NULL;
		    }
		    else for (oldz = &s->srt_free, z = (char **)s->srt_free; 
				z != NULL; oldz = z, z = (char **)*z)
	 	     if (((i4 *)z)[-1] > i)
	 	     {	/* add buffer to correct spot on free chain */
	    		*((char **)x) = *oldz;
	    		*oldz = x;
	    		break;
	 	     }
		}
	     }	/* end of duplicate elimination */

	    /* We only get here if the new record has been dropped, or if
	    ** it's porper location has been found. */
	    break;		/* found the spot - exit now */
	}
	return (E_DB_OK);
    }
    SETDBERR(dberr, 0, E_DM9706_SORT_PROTOCOL_ERROR);
    return (E_DB_ERROR);
}


/*{
** Name: new_root	- Move next smallest record to replace root
**
** Description:
**      Descend heap looking for next smallest record, moving records up
**	as we go. Leaves heap heapified, and with smallest at top. If
**	this is a duplicates removal sort, there is extra logic to detect
**	adjacent records which are the same, and to remove them by
**	recursing to replace one of them by the smallest record in its
**	sub-heap.
**
** Inputs:
**      sort_cb                         The sort control block.
**	root				Array entry at which we start.
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	2-dec-97 (inkdo01)
**	    Written.
*/
static DB_STATUS
new_root(
	DMS_SRT_CB	*sort_cb,
	i4		root,
	DB_ERROR	*dberr)
{
    DMS_SRT_CB	*s = sort_cb;
    i4	**heap = s->srt_heap;
    char	*x;
    i4	i, j;
    DB_STATUS	status;
    i4		cmpres;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);


    /*	Bring smallest row to root and reheapify in its wake. */

    x = (char *)heap[root];
    s->srt_adfcb.adf_errcb.ad_errcode=0;
    for (i = root, j = i+i; j <= s->srt_next_record; )
    {
	if (j < s->srt_next_record)
	 if (heap[j] != NULL && heap[j+1] != NULL)
	{
	    /* Compare siblings. If j+1 is smaller, promote it. If they're
	    ** equal and this is dups elimination, drop one and recurse to
	    ** replace it. Otherwise, promote j. */

    	    s->srt_c_comps++;
	    cmpres = adt_sortcmp(&s->srt_adfcb, s->srt_ratts, s->srt_ra_count, 
		(char *)heap[j], (char *)heap[j + 1], 0);
            if (s->srt_adfcb.adf_errcb.ad_errcode!=0)
            {
        	uleFormat(&s->srt_adfcb.adf_errcb.ad_dberror, 0, 
			NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		SETDBERR(dberr, 0, E_DM970B_SORT_PUT);
		return (E_DB_ERROR);
            }
	    if (cmpres > 0) 
		j++;
	    else if ( s->srt_state & SRT_ELIMINATE &&
		     (cmpres == 0 || abs(cmpres) > s->srt_rac_count) )
	    {					/* found a dup to eliminate */
		status = new_root(s, j+1, dberr);
		if (status != E_DB_OK) 
		    return(status);
		else 
		    continue;
	    }
	}
	else if (heap[j+1] != NULL) j++;

	/* If heap[j] is NULL, we're at the end of the branch, else heap[j]
	** is the next smaller to move up. */
	if (heap[j] == NULL) break;
	heap[i] = heap[j];		/* suck it up the heap */
	i = j;
	j += j;
    }

    /* Add buffer just freed (root) to the free chain. Note, the free chain
    ** consists of buffer/heap slot pairs (heap slot is prepended to buffer)
    ** and it is ordered on heap slot number. */
    /* if (i == s->srt_next_record) 
    {
	heap[i] = (i4 *)x;
    }
    else */
    {
	char	**y;
	char	**oldy;

	s->srt_c_free++;
	heap[i] = NULL;			/* show as unoccupied */
	((i4 *)x)[-1] = i;		/* identify corresponding array slot */
	if (s->srt_free == NULL)
	{
	    s->srt_free = x;
	    *((char **)x) = NULL;
	}
	else for (oldy = &s->srt_free, y = (char **)s->srt_free; y != NULL; 
		oldy = y, y = (char **)*y)
	 if (((i4 *)y)[-1] > i)
	 {  /* stuff entry in correct free chain location */
	    *((char **)x) = *oldy;
	    *oldy = x;
	    break;
	 }
    }

    return(E_DB_OK);
}


/*{
** Name: tidy_dup	- Restore current heap branch.
**
** Description:
**      When a dup was found on input, this function recurses down to the
**	point of insertion, then restores the buffer pointers on that
**	branch to what they were prior to the insertion.
**
** Inputs:
**      sort_cb                         The sort control block.
**	stop				Point of insertion (from which we
**					back our way up the branch).
**	current				Current buffer array entry.
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-nov-97 (inkdo01)
**	    Written.
*/
static void
tidy_dup(
DMS_SRT_CB	*sort_cb,
i4		stop,
i4		current)

{
    i4	newcurr;
    i4	**heap = sort_cb->srt_heap;


    /* All this does is recurse from end of current branch to point
    ** where duplicate was detected, then restore vector addresses 
    ** on the way back out. */

    newcurr = current>>1;		/* next node to examine */
    if (newcurr > stop) tidy_dup(sort_cb, stop, newcurr);

    heap[newcurr] = heap[current];	/* restore buffer pointers */

}

/*{
** Name: dmse_input_end	- Declare end of input stream.
**
** Description:
**      This is the external entry for indicating the end of the sort
**	input stream. If this is a serial sort, the call is passed 
**	to the serial input_end function, otherwise it is added to 
**	the exchange buffers of all child threads.
**
** Inputs:
**      sort_cb                         The sort control block.
**
** Outputs:
**      err_code                        Reason for error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR			E_DM9709_SORT_INPUT
**					E_DM9706_SORT_PROTOCOL_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-feb-98 (inkdo01)
**	    Written for parallel DMF sort project.
**	26-nov-98 (inkdo01)
**	    Add support for semaphore to assure synchronization of psorts.
*/
DB_STATUS
dmse_input_end(
DMS_SRT_CB        *sort_cb,
DB_ERROR          *dberr)
{
    DM_SVCB	*svcb = dmf_svcb;
    DM_STECB	*step;
    i4	i, j;
    DB_STATUS	status = E_DB_OK;

    if (DMZ_SRT_MACRO(1) && (DMZ_SRT_MACRO(3) ||
		sort_cb->srt_state & SRT_PARALLEL))
    {	/* report on put time */
	SYSTIME systime;
	TMnow(&systime);
	sort_cb->srt_secs = systime.TM_secs-sort_cb->srt_secs;
	sort_cb->srt_msecs = systime.TM_msecs - sort_cb->srt_msecs;
	if (sort_cb->srt_msecs < 0)
	{
	    sort_cb->srt_secs++;
	    sort_cb->srt_msecs+= 1000000;
	}
	TRdisplay("%p DMSE_TIMEP1: %f\n", sort_cb->srt_sid, 
				(float)sort_cb->srt_secs +
				(float)sort_cb->srt_msecs/1000000.);
	sort_cb->srt_secs = systime.TM_secs;
	sort_cb->srt_msecs = systime.TM_msecs;
    }

    if (!(sort_cb->srt_state & SRT_PARALLEL))
	return(dmse_input_end_serial(sort_cb, dberr));

    CLRDBERR(dberr);

    if (DMZ_SRT_MACRO(1))
	TRdisplay("%p DMSE_INPUT: Records = %d Runs = %d\n", 
	    sort_cb->srt_sid,
	    sort_cb->srt_c_records, sort_cb->srt_run_number);

    sort_cb->srt_c_records = 0;


    /* Signal the Kids that Parent is at EOF. */
    CSp_semaphore(1, &sort_cb->srt_cond_sem);

    if ( sort_cb->srt_pstate & SRT_PCERR )
    {
	SETDBERR(dberr, 0, E_DM9714_PSORT_CFAIL);
	status = E_DB_ERROR;
    }
    else
    {
	/* Next GET is first GET */
	sort_cb->srt_state |= SRT_GETFIRST;

	/* Signal EOF to children */
	sort_cb->srt_pstate |= SRT_PEOF;

	CScnd_broadcast(&sort_cb->srt_cond);
    }

    if (DMZ_SRT_MACRO(1)) TRdisplay("\n");

    CSv_semaphore(&sort_cb->srt_cond_sem);

    return(status);
}

/*{
** Name: dmse_input_end_serial	- Declare end of input stream.
**
** Description:
**      This routine is called after the last record has been passed
**	by the caller.
**
** Inputs:
**      sort_cb                         The sort control block.
**
** Outputs:
**      err_code                        Reason for error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR			E_DM9709_SORT_INPUT
**					E_DM9706_SORT_PROTOCOL_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	04-feb-1986 (derek)
**          Created new for jupiter.
**	17-jan-1992 (bryanp)
**	    In-memory sorting enhancements: at the end of the input phase,
**	    we may discover that the entire sort file never even filled up the
**	    heap. If so, then records are left in the heap. dmse_get_record
**	    then retrieves records from the heap rather than from the sort
**	    buffers.
**	28-nov-97 (inkdo01)
**	    Various improvements to sort algorithm, to reduce number of 
**	    comparisons.
*/
DB_STATUS
dmse_input_end_serial(
DMS_SRT_CB        *sort_cb,
DB_ERROR          *dberr)
{
    DMS_SRT_CB		*s = sort_cb;
    i4		i;
    i4		j;
    char		*x;
    i4		max_merge_order;
    i4		passes;
    i4		runs;
    i4		best_passes;
    i4		best_runs;
    i4		best_order;
    i4		orig_next_record;
    i4		**heap = s->srt_heap;
    DB_STATUS		status;
    char		*buffer;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if ((s->srt_state & SRT_PUT) == 0)
    {
	SETDBERR(dberr, 0, E_DM9706_SORT_PROTOCOL_ERROR);
	return (E_DB_ERROR);
    }

    /*	Write what is left in memory to output file. We write from the root,
    ** always reheapifying to replace root with next smallest. */
    orig_next_record = s->srt_next_record;

    while ((s->srt_state & SRT_HEAP_BECAME_FULL) != 0 && s->srt_next_record > 0)
    {
	status = write_record(sort_cb, (char *)heap[1]);
	if (status != E_DB_OK)
	{
	    *dberr = s->srt_dberr;
	    if (dberr->err_code > E_DM_INTERNAL)
	    {
		uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		SETDBERR(dberr, 0, E_DM9709_SORT_INPUT);
	    }
	    return (E_DB_ERROR);
	}

	s->srt_next_record--;
        s->srt_adfcb.adf_errcb.ad_errcode=0;

	/* Move next smallest to root - last in branch is nulled. */
	for (i = 1, j = 2; j <= orig_next_record; )
	{
	    if (j < orig_next_record && ((char *)heap[j] == NULL ||
	    	(char *)heap[j+1] != NULL && (s->srt_c_comps++,
		adt_sortcmp(&s->srt_adfcb, s->srt_ratts, s->srt_ra_count, 
                    (char *)heap[j], (char *)heap[j + 1], 0) > 0)))
	    {
	        j++;
	    }
	    if(s->srt_adfcb.adf_errcb.ad_errcode!=0)
	    {
	        uleFormat(&s->srt_adfcb.adf_errcb.ad_dberror, 0, 
				NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		SETDBERR(dberr, 0, E_DM9709_SORT_INPUT);
		return (E_DB_ERROR);
	    }
	    if ((char *)heap[j] == NULL) 
		break;
	    heap[i] = heap[j];
	    heap[j] = NULL;
	    i = j;
	    j += j;
	}
        s->srt_adfcb.adf_errcb.ad_errcode=0;
	if (heap[1] == NULL) 
	    break;
    }
    s->srt_state &= ~SRT_PUT;
    s->srt_state |= SRT_GETFIRST;

    if ((s->srt_state & SRT_HEAP_BECAME_FULL) == 0)
    {
	if (DMZ_SRT_MACRO(1))
	TRdisplay("%p DMSE_INPUT: Records = %d Runs = %d Merge_order = %d Passes = %d\n", 
	    s->srt_sid,
	    s->srt_c_records, s->srt_run_number, 0, 0);
	s->srt_c_records = 0;
	return (E_DB_OK);
    }

    /*	Terminate the current run. */

    status = output_run(s, END_OUTPUT_RUN | STOP_OUTPUT_PASS);
    if (status != E_DB_OK)
    {
	*dberr = s->srt_dberr;
	if (dberr->err_code > E_DM_INTERNAL)
	{
	    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM9709_SORT_INPUT);
	}
	return (E_DB_ERROR);
    }

    /*	Calculate the best merge order for this sort. */

    max_merge_order = (s->srt_misc->misc_length - sizeof(DMP_MISC) - 
                                         s->srt_o_size) /
	(s->srt_i_size + sizeof(DMS_MERGE_DESC) + 
                              s->srt_iwidth + 2 * sizeof(char *));
    if (max_merge_order > MAX_MERGE)
	max_merge_order = MAX_MERGE;
    for (best_passes = 32, best_runs = 1024, i = 2; i <= max_merge_order; i++)
    {
	for (runs = i, passes = 1;
	    runs < s->srt_run_number; 
            runs *= i, passes++)
	    ;
	if (passes < best_passes ||
	    (passes == best_passes && runs - s->srt_run_number < best_runs))
	{
	    best_passes = passes;
	    best_runs = runs - s->srt_run_number;
	    best_order = i;
	}
    }
    if (s->srt_run_number == 1)
	best_order = 1;

    if (DMZ_SRT_MACRO(1))
	TRdisplay("%p DMSE_INPUT: Records = %d Runs = %d Merge_order = %d Passes = %d\n", 
	    s->srt_sid,
	    s->srt_c_records, s->srt_run_number, best_order, best_passes);
    s->srt_c_records = 0;

    /*	Reconfigure the SORT CB for merging. */

    s->srt_heap = (i4**)
	((char *)s->srt_o_buffer + s->srt_o_size - sizeof(i4 *));
    s->srt_i_merge = (DMS_MERGE_DESC *)((char *)s->srt_heap +
	(best_order + 1) * sizeof(char *));
    s->srt_record = 
	(char *)s->srt_i_merge + sizeof(struct _DMS_MERGE_DESC) * best_order;
    buffer = (char *)s->srt_record + best_order * s->srt_iwidth;
    s->srt_last_record = best_order;
    s->srt_next_record = 1;

    /*	Initialize pointers into merge work area. */

    for (i = 1; i <= best_order; i++)
    {
	DMS_MERGE_DESC	    *m = &s->srt_i_merge[i - 1];

	s->srt_heap[i] = (i4 *)m;
	m->sort_cb = s;
	m->in_offset = 0;
	m->in_count = 0;
	m->in_blkno = 0;
	m->in_width = s->srt_iwidth;
	m->in_size = s->srt_o_size;
	m->rec_buffer = (char *)s->srt_record + (i - 1) * s->srt_iwidth;
	m->in_buffer = (char *)buffer + (i - 1) * s->srt_o_size;
    }

    /*	Perform all but the last merge pass.  */

    status = do_merge(s, best_passes);
    if (status == E_DB_OK || status == E_DB_INFO)
	status = input_run(s, START_INPUT_PASS | BEGIN_INPUT_RUN);
    if (status == E_DB_OK || status == E_DB_INFO)
	return (E_DB_OK);

    *dberr = s->srt_dberr;
    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9709_SORT_INPUT);
    }
    return (E_DB_ERROR);
}

/*{
** Name: dmse_get_record	- Get the next sorted record.
**
** Description:
**      This routine is the external entry for retrieving sorted 
**	rows. If this is a serial sort, it is passed straight to the
**	serial get function. Otherwise, logic is executed here to merge 
**	the rows returned from the individual child thread sorts.
**
** Inputs:
**      sort_cb                         The sort control block.
**      record                          Pointer to location to return record.
**
** Outputs:
**      err_code                        Reason for error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_INFO			Record has a duplicate prefix 
**                                      of the previous record.
**	    E_DB_ERROR			E_DM970A_SORT_GET
**					E_DM9706_SORT_PROTOCOL_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-feb-98 (inkdo01)
**	    Written for parallel DMF sort project.
**	22-jul-98 (inkdo01)
**	    Support for new suspend function for better error sync.
**	26-nov-98 (inkdo01)
**	    Add support for semaphore to assure synchronization of psorts.
**	31-Mar-2004 (jenjo02)
**	    Oops, check STE_CEOG for end of gets; get/put counts
**	    won't be equal if duplicates eliminated.
**	01-Apr-2004 (jenjo02)
**	    Correct over-counting srt_c_records.
**	22-Apr-2005 (toumi01)
**	    Before we cnd_signal to wake up a child session, recheck that
**	    that's what we want to do after we acquire the srt_cond_sem
**	    mutex (i.e. redo our dirty read under mutex protection).
**	07-Aug-2006 (smeke01) BUG 116455
**	    Removed the check of flag ste_cflag for STE_CWAIT as it was 
**	    not protected by semaphore at that point. This could lead to 
**	    the child, having acquired the semaphore, being about to 
**	    suspend itself but not having updated the flag. The parent thus
**	    does not signal the child and then suspends itself. Result 
**	    was bug 116455 where both child & parent are suspended at 
**	    the same time, unable to signal each other.
**	    I have left the check of the child restart point in as it 
**	    may save some machine cycles when they're most needed (see 
**	    comments below). 
*/
DB_STATUS
dmse_get_record(
DMS_SRT_CB        *sort_cb,
char               **record,
DB_ERROR          *dberr)
{
    DM_SVCB	*svcb = dmf_svcb;
    DM_STECB	*step;
    char	*xbp;
    DMS_SRT_CB	*s = sort_cb;
    DB_STATUS	status = E_DB_OK;
    i4		**heap = s->srt_heap;
    char	*x;
    i4		i, j, k;

    if (!(sort_cb->srt_state & SRT_PARALLEL))
	return(dmse_get_record_serial(sort_cb, record, dberr));

    CLRDBERR(dberr);

    /* This is a parallel sort. Rows are merged from the child sort 
    ** fragments using a heap with room for one row from each thread.
    */

    if ( s->srt_state & SRT_GETFIRST && (s->srt_pstate & SRT_PCERR) == 0 )
    {
	/* Wait for kids to end their PUTs */
	CSp_semaphore(1, &s->srt_cond_sem);

	while ( s->srt_puts_active && (s->srt_pstate & SRT_PCERR) == 0 )
	    parent_suspend(s);

	/* Prior to returning first sorted row, loop over child
	** thread exchange buffers getting first row from each. 
	** Turn them into the initial merge heap. */

	 for (i = 0; 
	      i < s->srt_nthreads && (s->srt_pstate & SRT_PCERR) == 0;
	      i++)
	 {	
	    step = s->srt_stecbp[i];

	    /*
	    ** If Child has rows but hasn't gotten one yet,
	    ** wait for it to do so.  (Child wakes Parent after first get,
	    ** no need for prstrt or PRSTRT flag here.)
	    */
	    while ( step->ste_c_puts > 0 && step->ste_c_gets == 0 &&
		   (s->srt_pstate & SRT_PCERR) == 0 )
	    {
		++ step->ste_npwaits;
		parent_suspend(s);
	    }
	}

	CSv_semaphore(&s->srt_cond_sem);

	/* Each child thread has now performed at least one get. Now
	** load first row from each thread into merge heap and heapify. */

	if ( (s->srt_pstate & SRT_PCERR) == 0 )
	{
	    s->srt_next_record = 0;	/* initialize # merge sources */

	    for (i = 0; i < s->srt_nthreads; i++)
	    {
		step = s->srt_stecbp[i];

		if ( step->ste_c_puts == 0 )
		    continue;

		j = ++s->srt_next_record;

		/* 1st record in exchange buffer */
		xbp = step->ste_xbufp[0];

		/* Build heap entry */
		((DMS_PMERGE_DESC *)heap[j])->record = xbp;
		((DMS_PMERGE_DESC *)heap[j])->step = step;

		/* Now move the row down to its proper place in the heap. */

		s->srt_adfcb.adf_errcb.ad_errcode = 0;
		for ( ; k = j, j >>= 1; )
		{
		    s->srt_c_comps++;
		    /* Compare row[k/2] to row[k] (k > 1). */
		    if (adt_sortcmp(&s->srt_adfcb, s->srt_atts, s->srt_a_count,
			((DMS_PMERGE_DESC *)heap[j])->record,
			((DMS_PMERGE_DESC *)heap[k])->record, 0) > 0)
		    {
			/* Swap k/2 for k. */
			x = (char *)heap[j];
			heap[j] = heap[k];
			heap[k] = (i4 *)x;
		    }
		}
	    }	/* end of initial merge heap build */

	    if (s->srt_next_record == 0)
	    {	/* in the unlikely event that no rows were sorted */
		SETDBERR(dberr, 0, E_DM0055_NONEXT);
		status = E_DB_ERROR;
	    }
	    else
	    {
		if (DMZ_SRT_MACRO(1))
		{	/* report on merge time */
		    SYSTIME systime;
		    TMnow(&systime);
		    s->srt_secs = systime.TM_secs-s->srt_secs;
		    s->srt_msecs = systime.TM_msecs - s->srt_msecs;
		    if (s->srt_msecs < 0)
		    {
			s->srt_secs++;
			s->srt_msecs+= 1000000;
		    }
		    TRdisplay("%p DMSE_TIMEP2: %f\n", 
					    s->srt_sid,
					    (float)s->srt_secs +
					    (float)s->srt_msecs/1000000.);
		    s->srt_secs = systime.TM_secs;
		    s->srt_msecs = systime.TM_msecs;
		}

		s->srt_state &= ~SRT_GETFIRST;	/* clear flag */
		s->srt_state |= SRT_GET;
		*record = ((DMS_PMERGE_DESC *)heap[1])->record;
		if (s->srt_state & SRT_CHECK_DUP)
		    MEcopy(*record, s->srt_iwidth, s->srt_duplicate);
	    }
	}
    }	/* end of GETFIRST */
    else for ( ; (s->srt_pstate & SRT_PCERR) == 0 ; )
    {
	i4	pbix;
	bool	dropped_thread = FALSE;
	
	/* Previous call returned heap[1]. Replace it with next sorted 
	** row from same sort thread, reheapify and return root again. */

	/* big loop allows duplicates elimination */

	step = ((DMS_PMERGE_DESC *)heap[1])->step;
				/* previous root's thread */
	if ( step->ste_cflag & STE_CEOG &&
	     step->ste_pbix >= step->ste_cbix )
	{
	    /* EOF on this guy. Promote last in heap */

	    /* But first, toss his exchange buffer */
	    if ( step->ste_misc )
		dm0m_deallocate((DM_OBJECT**)&step->ste_misc);

	    heap[1] = heap[s->srt_next_record];

	    if (--s->srt_next_record <= 0)
	    {	/* we've reached the sort end */
		SETDBERR(dberr, 0, E_DM0055_NONEXT);
		status = E_DB_ERROR;
	    }
	    else
	    {
		dropped_thread = TRUE;
		/* next step value */
		step = ((DMS_PMERGE_DESC *)heap[1])->step; 
	    }
	}
	else if (step->ste_pbix >= step->ste_cbix)
	{
	    /* Parent has caught up to this child and must wait */
	    CSp_semaphore(1, &s->srt_cond_sem);


	    /* Re-check under semaphore protection.  Child can still
	    ** race ahead (cbix is not protected), but it will see
	    ** our flag before it can decide to sleep.
	    */
	    if ( step->ste_pbix >= step->ste_cbix &&
		 ((s->srt_pstate & SRT_PCERR) == 0) && 
                 (( step->ste_cflag & STE_CEOG) == 0) ) 
	    {
		step->ste_prstrt = step->ste_pbix + 
				    step->ste_bsize * 3 / 4;
		step->ste_cflag |= STE_PRSTRT;
		++ step->ste_npwaits;
		parent_suspend(s);
	    }
	    CSv_semaphore(&s->srt_cond_sem);
	    continue;		/* when we resume, do it all again */
	}

	if (status == E_DB_OK && !dropped_thread)
	{
	    pbix = step->ste_pbix % step->ste_bsize;

	    /*
	    ** The check of the child restart point has been left in 
	    ** as it may save some machine cycles when they're most 
	    ** needed. The reasoning is as follows. The condition will 
	    ** always be true if the parent is keeping up with the child 
	    ** (with the result that the child doesn't need to suspend and 
	    ** so won't set the restart point further forward). However 
	    ** the check will save the cost of acquiring the semaphore in 
	    ** the case where the parent isn't keeping up. This is exactly 
	    ** when we want to save some time.
	    */
	    if ( step->ste_pbix >= step->ste_crstrt )
	    {
		/*
		** Parent has advanced sufficiently to
		** restart Child.
		*/
		CSp_semaphore(1, &s->srt_cond_sem);
		/*
		** Retest the condition after the dirty read above
		** as the child may have raced ahead, suspended
		** itself, and moved the restart point on further. 
		** If so, we need to iterate a few more times to 
		** get to the restart point before waking the child.
		*/
		if ( step->ste_cflag & STE_CWAIT &&
		     step->ste_pbix >= step->ste_crstrt )
		{
		    /* Go ahead and clear cwait so that we don't bash
		    ** at the kid over and over before it gets moving.
		    */
		    step->ste_cflag &= ~STE_CWAIT;
		    CScnd_signal(&s->srt_cond, step->ste_ctid);
		}
		CSv_semaphore(&s->srt_cond_sem);
	    }
	    xbp = step->ste_xbufp[pbix];
	    ((DMS_PMERGE_DESC *)heap[1])->record = xbp;  
					/* stick next row in heap */
	    CS_MEMBAR_MACRO();
	    step->ste_pbix++;		/* increment parent index */
	}

	if ( status == E_DB_OK )
	{
	    /* heap[1] has a row now. Time to reheap it all. */

	    s->srt_adfcb.adf_errcb.ad_errcode = 0;
	    for (x = (char *)heap[1], i = 1, j = 2;
		    j <= s->srt_next_record; )
	    {
		/* Pick smaller of 2i, 2i+1. */
		if (j < s->srt_next_record &&
		    (s->srt_c_comps++,
		    adt_sortcmp(&s->srt_adfcb, s->srt_atts,
			s->srt_a_count,
			((DMS_PMERGE_DESC *)heap[j])->record,
			((DMS_PMERGE_DESC *)heap[j + 1])->record, 0) > 0))
		    j++;
		/* Compare to newly read row. */
		if ((s->srt_c_comps++,
		    adt_sortcmp(&s->srt_adfcb, s->srt_atts,
			s->srt_a_count,
			((DMS_PMERGE_DESC *)heap[i])->record,
			((DMS_PMERGE_DESC *)heap[j])->record, 0) <= 0))
		    break;
		/* If new is smaller, we're done. If new is bigger,
		** swap 'em. */
		heap[i] = heap[j];
		heap[j] = (i4 *)x;
		i = j;
		j += j;
	    }	/* end of reheap loop */

	    *record = ((DMS_PMERGE_DESC *)heap[1])->record;
				    /* return smallest remaining row */

	    if (s->srt_state & SRT_CHECK_DUP)
	    {
		i4	compare;

		s->srt_adfcb.adf_errcb.ad_errcode = 0;
		s->srt_c_comps++;
		/* srt_c_count instead of srt_a_count */
		compare = adt_sortcmp(&s->srt_adfcb, s->srt_atts, 
		    s->srt_c_count, s->srt_duplicate, *record,
		    (i4)(s->srt_k_count + 1));

		if ( s->srt_state & SRT_ELIMINATE && compare == 0 )
		    continue;	/* to drop dup, just stay in loop */
		
		MEcopy(*record, s->srt_iwidth, s->srt_duplicate);

		if (s->srt_state & SRT_CHECK_KEY)
		{
		    if (compare < 0) 
			compare = -compare;
		    if (compare == 0 || compare > s->srt_k_count)
			status = E_DB_INFO;
		}
	    }	/* end of SRT_CHECK_DUP */
	}
	break;

    }	/* end of big "for" */

    /* If any thread errors thus far, end all Child threads */
    if ( s->srt_pstate & SRT_PCERR )
    {
	CSp_semaphore(1, &s->srt_cond_sem);

	if ( s->srt_threads )
	{
	    /* Broadcast the error */
	    CScnd_broadcast(&s->srt_cond);

	    /* Wait for all threads to end */
	    while ( s->srt_threads )
		parent_suspend(s);
	}
	CSv_semaphore(&s->srt_cond_sem);

	SETDBERR(dberr, 0, E_DM9714_PSORT_CFAIL);
	status = E_DB_ERROR;
    }

    if ( DB_SUCCESS_MACRO(status) )
	s->srt_c_records++;

    return(status);
}

/*{
** Name: dmse_get_record_serial	- Get the next sorted record.
**
** Description:
**      This routine returns the next sorted record to the caller.
**
** Inputs:
**      sort_cb                         The sort control block.
**      record                          Pointer to location to return record.
**
** Outputs:
**      err_code                        Reason for error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_INFO			Record has a duplicate prefix 
**                                      of the previous record.
**	    E_DB_ERROR			E_DM970A_SORT_GET
**					E_DM9706_SORT_PROTOCOL_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	06-feb-1986 (derek)
**          Created new for jupiter.
**	17-jan-1992 (bryanp)
**	    In-memory sorting enhancements: if the heap never became full,
**	    get the next record from the heap, not from the merged runs.
**	28-nov-97 (inkdo01)
**	    Various improvements to sort algorithm, to reduce number of 
**	    comparisons.
*/
DB_STATUS
dmse_get_record_serial(
DMS_SRT_CB        *sort_cb,
char               **record,
DB_ERROR          *dberr)
{
    DMS_SRT_CB         *s = sort_cb;
    i4		i;
    i4		j;
    i4		**heap = s->srt_heap;
    char		*x;
    DB_STATUS		status;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);


    if (DMZ_SRT_MACRO(1) && DMZ_SRT_MACRO(3) && s->srt_state & SRT_GETFIRST)
    {	/* report on merge time */
	SYSTIME systime;
	TMnow(&systime);
	s->srt_secs = systime.TM_secs-s->srt_secs;
	s->srt_msecs = systime.TM_msecs - s->srt_msecs;
	if (s->srt_msecs < 0)
	{
	    s->srt_secs++;
	    s->srt_msecs+= 1000000;
	}
	TRdisplay("%p DMSE_TIMEP2: %f\n", 
				s->srt_sid,
				(float)s->srt_secs +
				(float)s->srt_msecs/1000000.);
	s->srt_secs = systime.TM_secs;
	s->srt_msecs = systime.TM_msecs;
    }
    if ((s->srt_state & SRT_HEAP_BECAME_FULL) == 0)
	return (get_in_memory_record( sort_cb, record, dberr ));

    /* If 1st GET of a sorted set, SRT_GETFIRST logic (at end of function)
    ** is executed. This stuff is used for the 2nd and subsequent GET. */
    for (;(s->srt_state & SRT_GET);)
    {
	status = read_record((DMS_MERGE_DESC *)heap[1]);
				/* read replacement for previous record */
	if (status != E_DB_OK)
	{
	    if (status != E_DB_INFO)
		break;
	    /* Exhausted an input run - collapse the merge by one. */
	    heap[1] = heap[s->srt_next_record];
	    if (--s->srt_next_record <= 0)
	    {
		SETDBERR(dberr, 0, E_DM0055_NONEXT);
		return (E_DB_ERROR);
	    }
	}

	/*  Bring next smallest record to top of heap AND reheapify
	** in its wake. */

        s->srt_adfcb.adf_errcb.ad_errcode=0;
	for (x = (char *)heap[1], i = 1, j = 2;
	    j <= s->srt_next_record; )
	{
	    /* Pick smaller of 2i, 2i+1. */
	    if (j < s->srt_next_record &&
	    	(s->srt_c_comps++,
		adt_sortcmp(&s->srt_adfcb, s->srt_atts, s->srt_a_count, 
                    ((DMS_MERGE_DESC *)heap[j])->record,
		    ((DMS_MERGE_DESC *)heap[j + 1])->record, 0) > 0))
	    {
	        j++;
	    }
	    if(s->srt_adfcb.adf_errcb.ad_errcode!=0)
	    {
	        uleFormat(&s->srt_adfcb.adf_errcb.ad_dberror, 0, 
				NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		SETDBERR(dberr, 0, E_DM0055_NONEXT);
		return (E_DB_ERROR);
	    }
	    /* Compare to newly read record. */
	    if ((s->srt_c_comps++,
		adt_sortcmp(&s->srt_adfcb, s->srt_atts, s->srt_a_count, 
                    ((DMS_MERGE_DESC *)heap[i])->record,
		    ((DMS_MERGE_DESC *)heap[j])->record, 0) <= 0))
		break;
	    /* If new is bigger, swap 'em. */
	    heap[i] = heap[j];
	    heap[j] = (i4 *)x;
	    i = j;
	    j += j;
	}

	*record = ((DMS_MERGE_DESC *)heap[1])->record;
	if (s->srt_state & SRT_CHECK_DUP)
	{
	    i4	    compare;

	    /*
	    **	Assumption here is that an unsortable type cannot be part of the
	    **	key portion of the tuple.
	    */

	    s->srt_adfcb.adf_errcb.ad_errcode=0;
	    s->srt_c_comps++;
	    /* srt_c_count instead of srt_a_count */
	    compare = adt_sortcmp(&s->srt_adfcb, s->srt_atts, s->srt_c_count, 
		s->srt_duplicate, *record, (i4) (s->srt_k_count+1));
	    if(s->srt_adfcb.adf_errcb.ad_errcode!=0)
	    {
	        uleFormat(&s->srt_adfcb.adf_errcb.ad_dberror, 0, 
				NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		SETDBERR(dberr, 0, E_DM0055_NONEXT);
		return (E_DB_ERROR);
	    }
	    if ( s->srt_state & SRT_ELIMINATE && compare == 0 )
		continue;

	    MEcopy(*record, s->srt_iwidth, s->srt_duplicate);

	    /* Note, while duplicates removal can be done at any time during
	    ** a sort, duplicates checking/flagging can only be done at GET
	    ** time because that is the only time the sort caller is looking
	    ** for the appropriate return code. */
	    if (s->srt_state & SRT_CHECK_KEY)
	    {
		if (compare < 0)
		    compare = -compare;
		if ((compare == 0) || (compare > s->srt_k_count))
		{
		    s->srt_c_records++;
		    return (E_DB_INFO);
		}
	    }
	}
	s->srt_c_records++;
	return (E_DB_OK);
    }

    /* First GET gets got here. Once this is done, the heap is primed
    ** and ready for the rest. */
    if (s->srt_state & SRT_GETFIRST)
    {
	/* Check if no rows were given to sorter. */
	if (s->srt_next_record == 0)
	{
	    SETDBERR(dberr, 0, E_DM0055_NONEXT);
	    return (E_DB_ERROR);
	}

	s->srt_c_records++;
	s->srt_state &= ~SRT_GETFIRST;
	s->srt_state |= SRT_GET;
	*record = ((DMS_MERGE_DESC *)heap[1])->record;
	if (s->srt_state & SRT_CHECK_DUP)
	    MEcopy(*record, s->srt_iwidth, s->srt_duplicate);
	return (E_DB_OK);
    }
    if (s->srt_state & SRT_PUT)
        SETDBERR(&s->srt_dberr, 0, E_DM9706_SORT_PROTOCOL_ERROR);

    *dberr = s->srt_dberr;
    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM970A_SORT_GET);
    }
    return (E_DB_ERROR);
}

/*{
** Name: dmse_end	- Terminate a sort.
**
** Description:
**      External entry routine for sort termination. If this is a
**	serial sort, call is passed straight to serial end function. For
**	parallel sorts, calls are added to exchange buffers of all child 
**	threads, then serial end function is called to clean up parent
**	thread sort cb.
**
** Inputs:
**      sort_cb                         The sort control block.
**
** Outputs:
**      err_code                        The reason for an error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR			E_DM9708_SORT_END
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-feb-98 (inkdo01)
**	    Written for parallel DMF sort project.
**	9-jul-98 (inkdo01)
**	    Add code to properly synchronize parent/child dmse_end's.
**	26-nov-98 (inkdo01)
**	    Add support for semaphore to assure synchronization of psorts.
**	04-Apr-2003 (jenjo02)
**	    Wait for child threads to end before releasing common
**	    semaphore and freeing thread-shared memory.
*/
DB_STATUS
dmse_end(
DMS_SRT_CB        *sort_cb,
DB_ERROR          *dberr)
{
    DM_SVCB	*svcb = dmf_svcb;
    DM_STECB	*step;
    i4	i;
    STATUS	status;

    CLRDBERR(dberr);


    if (DMZ_SRT_MACRO(1) && (DMZ_SRT_MACRO(3) ||
		sort_cb->srt_state & SRT_PARALLEL))
    {	/* report on get time */
	SYSTIME systime;
	TMnow(&systime);
	sort_cb->srt_secs = systime.TM_secs-sort_cb->srt_secs;
	sort_cb->srt_msecs = systime.TM_msecs - sort_cb->srt_msecs;
	if (sort_cb->srt_msecs < 0)
	{
	    sort_cb->srt_secs++;
	    sort_cb->srt_msecs+= 1000000;
	}
	TRdisplay("%p DMSE_TIMEP3: %f\n", 
			sort_cb->srt_sid,
			(float)sort_cb->srt_secs +
			(float)sort_cb->srt_msecs/1000000.);
    }

    /* For parallel sorts, just loop over child threads posting the 
    ** end call in each. */

    if ( sort_cb->srt_state & SRT_PARALLEL )
    {
	CSp_semaphore(1, &sort_cb->srt_cond_sem);

	/* They should all be gone by now */
	if ( sort_cb->srt_threads )
	{
	    /* ...force out any remaining */
	    sort_cb->srt_pstate |= SRT_PERR;
	    CScnd_broadcast(&sort_cb->srt_cond);

	    while ( sort_cb->srt_threads )
		parent_suspend(sort_cb);
	}

	/* Free any remaining exchange buffer memory */
	for ( i = 0; i < sort_cb->srt_nthreads; i++ )
	{
	    step = sort_cb->srt_stecbp[i];
	    if ( step->ste_misc )
		dm0m_deallocate((DM_OBJECT**)&step->ste_misc);
	}

	/* Release, then destroy semaphore */
	CScnd_free(&sort_cb->srt_cond);
	CSv_semaphore(&sort_cb->srt_cond_sem);
	CSr_semaphore(&sort_cb->srt_cond_sem);
    }

    /* For parallel AND serial sorts, call serial function to clean up 
    ** sort environment. */
    return(dmse_end_serial(sort_cb, dberr));
}

/*{
** Name: dmse_end_serial	- Terminate a sort.
**
** Description:
**      Terminate the sort.  Delete any open files, deallocate the
**	SORT CB.
**
** Inputs:
**      sort_cb                         The sort control block.
**
** Outputs:
**      err_code                        The reason for an error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR			E_DM9708_SORT_END
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	05-feb-1986 (derek)
**          Created new for jupiter.
*/
DB_STATUS
dmse_end_serial(
DMS_SRT_CB        *sort_cb,
DB_ERROR          *dberr)
{
    DMS_SRT_CB		*s = sort_cb;
    STATUS		stat;
    CL_ERR_DESC		sys_err;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    for (;;)
    {
	if (DMZ_SRT_MACRO(1))
	    TRdisplay("%p DMSE_END  : Records = %d Reads = %d Writes = %d Comparisons = %u\n",
		s->srt_sid,
		s->srt_c_records, s->srt_c_reads, s->srt_c_writes,
		s->srt_c_comps);

	if (DMZ_SRT_MACRO(2))
	    stat = srt1_close_files(s, SRT_F_INPUT, &sys_err);
	else
	    stat = srt0_close_files(s, SRT_F_INPUT, &sys_err);

	if (stat != OK && stat != SR_BADFILE)
	{
	    uleFormat(NULL, E_DM9701_SR_CLOSE_ERROR, &sys_err,
		ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM9708_SORT_END);
	}

	if (DMZ_SRT_MACRO(2))
	    stat = srt1_close_files(s, SRT_F_OUTPUT, &sys_err);
	else
	    stat = srt0_close_files(s, SRT_F_OUTPUT, &sys_err);

	if (stat != OK && stat != SR_BADFILE)
	{
	    uleFormat(NULL, E_DM9701_SR_CLOSE_ERROR, &sys_err,
		ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM9708_SORT_END);
	}
	if (dberr->err_code)
	    break;

	dm0m_deallocate((DM_OBJECT **)&s->srt_misc);
	dm0m_deallocate((DM_OBJECT **)&sort_cb);
	return (E_DB_OK);
    }
    return (E_DB_ERROR);
}

/*{
** Name: parent_suspend		- Coordinate suspension of parent thread
**
** Description:
**	Parent waits to be signalled by some child.
**
** Inputs:
**      sort_cb                         The sort control block.
**		Caller must hold srt_cond_sem
**
** Outputs:
**	srt_pstatus			E_DB_ERROR if interrupted with
**      srt_perr_code                   E_DM0065_USER_INTR
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	22-jul-98 (inkdo01)
**	    Written for better error synchronization.
**	26-nov-98 (inkdo01)
**	    Add support for semaphore to assure synchronization of psorts.
**	04-Apr-2003 (jenjo02)
**	    Input "step" is optional; if present, parent is waiting
**	    on specific child; if absent, parent waits on any child.
**	    Make sure that the setting and testing of status
**	    bits are wrapped by mutex.
**	19-Apr-2004 (schka24)
**	    Don't issue interrupt messages, not helpful and they fill the
**	    dbms log when partitioned modify gets an error.
**	28-Jan-2006 (kschendel)
**	    Remove second param.  We now set PRSTRT inline to indicate that
**	    the prstrt point is meaningful.  SRT_PWAIT is the "real"
**	    parent-is-suspended flag.
*/
static VOID
parent_suspend(
DMS_SRT_CB        *s)
{

    i4		local_err;

    /* Parent is waiting for specific, or any child */
    s->srt_pstate |= SRT_PWAIT;

    if ( CScnd_wait(&s->srt_cond, &s->srt_cond_sem) )
    {
	/* Then we were interrupted */
	s->srt_pstate |= SRT_PERR;

	if ( s->srt_pstatus == E_DB_OK )
	{
	    s->srt_pstatus == E_DB_ERROR;
	    SETDBERR(&s->srt_pdberr, 0, E_DM0065_USER_INTR);

	    /* *** Eschew annoying and unhelpful messages...
	    ** The interrupt could have been an error propagating thru
	    ** a partitioned modify...
	    ** uleFormat(&s->srt_pdberr, 0, NULL, ULE_LOG, NULL, 
	    **		NULL, 0, NULL, &local_err, 0);
	    ** uleFormat(NULL, E_DM9715_PSORT_PFAIL, NULL, ULE_LOG, NULL, 
	    **		NULL, 0, NULL, &local_err, 0);
	    */
	}
    }

    s->srt_pstate &= ~SRT_PWAIT;

    return;

}

/*{
** Name: do_merge	- Perform all but the last merge.
**
** Description:
**      This routine handles the merging of the runs except for the last run.
**	The last run is merge as it is returned to the caller.
**
**
** Inputs:
**      sort_cb                         The sort control block.
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR			E_DM970C_SORT_DOMERGE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	06-feb-1986 (derek)
**          Created new for jupiter.
**	16-sept-1988 (nancy) -- bug #3186, sort returns internal error 
**	    (error trying to put a record) because of bad status from SRclose.
**	    Do same check as in dmse_end so that a close on a file that
**	    wasn't opened doesn't error.
**	28-nov-97 (inkdo01)
**	    Various improvements to sort algorithm, to reduce number of 
**	    comparisons.
**      15-jun-1999 (hweho01)
**          Corrected argument mismatch in ule_format() call.
**	13-Feb-2007 (kschendel)
**	    Add occasional cancel checks, but only if non-parallel.
**	    (For parallel sort, parent will check during suspend.)
*/
static DB_STATUS
do_merge(
DMS_SRT_CB          *sort_cb,
i4		    passes)
{
    DMS_SRT_CB		*s = sort_cb;
    i4		i;
    i4		j;
    i4		switch_count;
    char		*x;
    i4		**heap = s->srt_heap;
    SR_IO		*temp;
    DB_STATUS		status;
    STATUS		stat;
    CL_ERR_DESC		sys_err;
    i4		err_code;

    if (passes < 2)
    {
	s->srt_swapped = !s->srt_swapped;
	return (E_DB_OK);
    }

    /*
    **	Create/Open/Allocate the other work files for merging.
    */
    if (DMZ_SRT_MACRO(2))
	status = srt1_open_files(s, SRT_F_INPUT);
    else
	status = srt0_open_files(s, SRT_F_INPUT);

    if (status != E_DB_OK)
    {
	if (s->srt_dberr.err_code == 0)
	    SETDBERR(&s->srt_dberr, 0, E_DM970C_SORT_DOMERGE);
	return (E_DB_ERROR);
    }
    
    /* Outermost loop executes once for each pass other than the last
    ** (which is performed while the sorted records are actually being
    ** returned to the caller). A "pass" merges a bunch of runs into
    ** a smaller bunch of runs. */

    switch_count = 10000;
    while (--passes > 0)
    {
	/*  Swap the input and output files for the next run. */
	s->srt_swapped = !s->srt_swapped;

	status = input_run(s, START_INPUT_PASS);
	if (status == OK)
	{
	    status = output_run(s, START_OUTPUT_PASS);
	}
	if (status != E_DB_OK)
	    break;

	/* This loop executes once for each run output by 
	** the current pass. It merges "n" input runs into
	** a single output run ("n" is the merge order).*/

	for (;;)
	{
	    status = input_run(s, BEGIN_INPUT_RUN);
	    if (status != E_DB_OK)
	    {
		if (status == E_DB_INFO)
		{
		    status = E_DB_OK;
		    break;
		}
		break;
	    }
	    status = output_run(s, BEGIN_OUTPUT_RUN);
	    if (status != E_DB_OK)
		break;

	    /* This loop simply merges the rows from the current set
	    ** of input runs into the current output run. */

	    for (;;)
	    {
		if (s->srt_state & SRT_NEED_CSSWITCH && --switch_count <= 0)
		{
		    /* Occasional poll to read cancels from front-end,
		    ** only if non-parallel sort.
		    */
		    CScancelCheck(s->srt_sid);
		    switch_count = 10000;
		}
		status = write_record(s, 
                         ((DMS_MERGE_DESC *)heap[1])->record);
		if (status == E_DB_OK)
		    status = read_record((DMS_MERGE_DESC *)heap[1]);
		if (status != E_DB_OK)
		{
		    if (status != E_DB_INFO)
			break;

		    /* Exhausted another input run. Collapse the
		    ** merge order by 1. */
		    x = (char *) heap[1];
		    heap[1] = heap[s->srt_next_record];
		    heap[s->srt_next_record] = (i4 *) x;
		    if (--s->srt_next_record <= 0)
		    {
			status = E_DB_OK;
			break;
		    }
		}

		/* Locate the next smallest record, and position the last
		** read record while we're at it. */

	        s->srt_adfcb.adf_errcb.ad_errcode=0;
		for (x = (char *)heap[1], i = 1, j = 2;
                                   j <= s->srt_next_record; )
		{
		    /* Pick smaller of 2i, 2i+1. */
		    if (j < s->srt_next_record &&
	    		(s->srt_c_comps++,
			adt_sortcmp(&s->srt_adfcb, s->srt_atts, s->srt_a_count, 
			    ((DMS_MERGE_DESC *)heap[j])->record,
			    ((DMS_MERGE_DESC *)heap[j + 1])->record, 0) > 0))
		    {
			j++;
		    }
	            if(s->srt_adfcb.adf_errcb.ad_errcode!=0)
	            {
	                uleFormat(&s->srt_adfcb.adf_errcb.ad_dberror, 0, 
				NULL, ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
			status=E_DB_ERROR;
			break;
	            }
		    if ((s->srt_c_comps++, 
			adt_sortcmp(&s->srt_adfcb, s->srt_atts, s->srt_a_count, 
			    ((DMS_MERGE_DESC *)heap[i])->record,
			    ((DMS_MERGE_DESC *)heap[j])->record, 0) <= 0))
			break;		/* i <= j ==> we're heapified */
		    /* If new record is bigger, swap 'em. */
		    heap[i] = heap[j];
		    heap[j] = (i4 *)x;
		    i = j;
		    j += j;
		}
	    }

	    /*	Terminate the output run. */

	    if (status == E_DB_OK)
		status = output_run(s, END_OUTPUT_RUN);
	    if (status != E_DB_OK)
		break;
	}

	/*	End the current output pass. */

	if (status == E_DB_OK)
	    status = output_run(s, STOP_OUTPUT_PASS);
	if (status != E_DB_OK)
	    break;
    }

    /*	Handle any error logging. */

    if (status != E_DB_OK)
    {
	/*
	** If there is an error, don't return - instead, save the status and
	** drop below to close the sort file.
	*/
	if (s->srt_dberr.err_code > E_DM_INTERNAL)
	{
	    uleFormat(&s->srt_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
					    &err_code, 0);
	    SETDBERR(&s->srt_dberr, 0, E_DM970C_SORT_DOMERGE);
	}
    }
	
    /*	Close/Delete the input work file. */
    if (DMZ_SRT_MACRO(2))
	stat = srt1_close_files(s, SRT_F_INPUT, &sys_err);
    else
	stat = srt0_close_files(s, SRT_F_INPUT, &sys_err);

    if ((stat != OK)  &&  (stat != SR_BADFILE))
    {
	uleFormat(NULL, E_DM9701_SR_CLOSE_ERROR, &sys_err,
	    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0); 
	SETDBERR(&s->srt_dberr, 0, E_DM970C_SORT_DOMERGE);
	status = E_DB_ERROR;
    }

    if (status == E_DB_OK)
    {
	/* The next input file is the current output file. */
	s->srt_swapped = !s->srt_swapped;
	return (E_DB_OK);
    }

    if ((s->srt_dberr.err_code > E_DM_INTERNAL) &&
	(s->srt_dberr.err_code != E_DM970C_SORT_DOMERGE))
    {
	uleFormat(&s->srt_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
						&err_code, 0);
	SETDBERR(&s->srt_dberr, 0, E_DM970C_SORT_DOMERGE);
    }
    return (E_DB_ERROR);
}

/*{
** Name: read_record	- Read the next record from a merge input run.
**
** Description:
**      Read the next record from a merge input run.
**
** Inputs:
**      sort_cb                         The sort control block.
**      record                          Pointer to location to return record.
**
** Outputs:
**      err_code                        The reason for an error status.
**	Returns:
**	    E_DB_OK			Ok.
**	    E_DB_INFO			End of run.
**	    E_DB_ERROR			E_DM970E_SORT_READ_RECORD
**                                      E_DM0065_USER_INTR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	06-feb-1986 (derek)
**          Created new for jupiter.
*/
static DB_STATUS
read_record(
DMS_MERGE_DESC	   *merge_desc)
{
    DMS_MERGE_DESC	*m = merge_desc;
    i4			local_err;

    /*	Return pointer to next record. */

    if (--m->in_count >= 0)
    {
	if (m->in_offset + (i4) m->in_width <= (i4) m->in_size)
	{
	    m->record = &m->in_buffer[m->in_offset];
	    m->in_offset += m->in_width;
	    return (E_DB_OK);
	}
    }
    else
	return (E_DB_INFO);

    /*	Reconstruct fragmented record into record buffer. */

    {
	i4		left = m->in_width;
	char		*record = m->rec_buffer;
	STATUS		stat;
	CL_ERR_DESC	sys_err;

	m->record = record;
	for (;;)
	{
	    if (m->in_offset + left < (i4) m->in_size)
	    {
		MEcopy(&m->in_buffer[m->in_offset], left, record);
		m->in_offset += left;
		return (E_DB_OK);
	    }
	    MEcopy(&m->in_buffer[m->in_offset],
		(m->sort_cb->srt_i_size - m->in_offset), record);
	    left -= m->in_size - m->in_offset;
	    record += m->in_size - m->in_offset;
	    m->in_offset = 0;
	    m->in_blkno++;
	    m->sort_cb->srt_c_reads++;

	    if (DMZ_SRT_MACRO(2))
	    {
		stat = srt1_translated_io(m->sort_cb, SRT_F_READ, SRT_F_INPUT,
			m->in_blkno, m->in_buffer, &sys_err);
	    }
	    else
	    {
		stat = srt0_translated_io(m->sort_cb, SRT_F_READ, SRT_F_INPUT,
			m->in_blkno, m->in_buffer, &sys_err);
	    }
	    
	    if (stat == OK  &&  *(m->sort_cb->srt_uiptr) == 0)
		continue;
	    if (stat != OK)
		break;

	    SETDBERR(&m->sort_cb->srt_dberr, 0, E_DM0065_USER_INTR);
	    return (E_DB_ERROR);
	}

	/*	Handle error reporting. */

	uleFormat(NULL, E_DM9702_SR_READ_ERROR, &sys_err, ULE_LOG, NULL, 
	    NULL, 0, NULL, &local_err, 0);
	SETDBERR(&m->sort_cb->srt_dberr, 0, E_DM970E_SORT_READ_RECORD);
	return (E_DB_ERROR);
    }
}

/*{
** Name: write_record	- Write record to output buffer.
**
** Description:
**      This function writes the given record to the output buffer.
**	If the record has a different run number from the last record
**	written, then the current run is terminated and a new run initialated.
**	If the current record is a duplicate of the last record output and
**	duplicates should be elimiated, the record is dropped on the floor.
**
** Inputs:
**      sort_cb                         The sort control block.
**      record                          The record to write.
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR			E_DM970D_SORT_WRITE_RECORD
**                                      E_DM0065_USER_INTR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	06-feb-1986 (derek)
**          Created new for jupiter.
*/
static DB_STATUS
write_record(
DMS_SRT_CB        *sort_cb,
char               *record)
{
    DMS_SRT_CB         *s = sort_cb;
    i4		width;
    STATUS		stat;
    DB_STATUS		status;
    CL_ERR_DESC		sys_err;
    i4		err_code;
    i4		compare;

    /*	Check the run numbers. */

    if ((s->srt_state & SRT_PUT) &&
	((i4 *)record)[-1] != s->srt_run_number)
    {
	status = output_run(s, END_OUTPUT_RUN | BEGIN_OUTPUT_RUN);
	if (status != E_DB_OK)
	{
	    if (s->srt_dberr.err_code > E_DM_INTERNAL)
	    {
		uleFormat(&s->srt_dberr, 0, NULL, ULE_LOG, NULL, 
		    NULL, 0, NULL, &err_code, 0);
		SETDBERR(&s->srt_dberr, 0, E_DM970D_SORT_WRITE_RECORD);
	    }
	    return (E_DB_ERROR);
	}
    }

    /*	Check for duplicate records. */

    if (s->srt_state & SRT_ELIMINATE)
    {
	if (s->srt_state & SRT_DUPLICATE)
	{
	    s->srt_c_comps++;
	    s->srt_adfcb.adf_errcb.ad_errcode=0;
	    /* srt_c_count rather than srt_a_count */
	    compare = adt_sortcmp(&s->srt_adfcb, s->srt_atts, s->srt_c_count, 
				    s->srt_duplicate, record, 1);
	    if (s->srt_adfcb.adf_errcb.ad_errcode!=0)
	    {
		uleFormat(&s->srt_adfcb.adf_errcb.ad_dberror, 0, 
				NULL, ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		SETDBERR(&s->srt_dberr, 0, E_DM970D_SORT_WRITE_RECORD);
		return (E_DB_ERROR);
	    }

	    if ( compare == 0 )
		return(E_DB_OK);
	}
	MEcopy(record, s->srt_iwidth, s->srt_duplicate);
	s->srt_state |= SRT_DUPLICATE;
    }

    /*	Copy the record into the output buffer. */

    s->srt_o_records++;
    for (width = s->srt_iwidth;;)
    {
	if (s->srt_o_size - s->srt_o_offset > width)
	{
	    MEcopy(record, width, &s->srt_o_buffer[s->srt_o_offset]);
	    s->srt_o_offset += width;
	    return (E_DB_OK);
	}

	/*  Doesn't all fit, fragment the record between multiple buffers. */

	width -= s->srt_o_size - s->srt_o_offset;
	MEcopy(record, (s->srt_o_size - s->srt_o_offset), 
               &s->srt_o_buffer[s->srt_o_offset]);
	record += s->srt_o_size - s->srt_o_offset;

	/*
	**  Open files if they're not open.
	*/
	if ((s->srt_state & SRT_OUTPUT) == 0)
	{
	    STATUS	stat;
	    DB_STATUS	status;

	    if (DMZ_SRT_MACRO(2))
		status = srt1_open_files(s, SRT_F_OUTPUT);
	    else
		status = srt0_open_files(s, SRT_F_OUTPUT);

	    if (status != E_DB_OK)
	    {
		if (s->srt_dberr.err_code == 0)
		    SETDBERR(&s->srt_dberr, 0, E_DM970F_SORT_OUTPUT_RUN);
		return (E_DB_ERROR);
	    }
	    s->srt_state |= SRT_OUTPUT;
	}

	/*  Force full buffer to disk. */
	s->srt_c_writes++;

	if (DMZ_SRT_MACRO(2))
	{
	    stat = srt1_translated_io(s, SRT_F_WRITE, SRT_F_OUTPUT,
				s->srt_o_blkno, s->srt_o_buffer, &sys_err);
	}
	else
	{
	    stat = srt0_translated_io(s, SRT_F_WRITE, SRT_F_OUTPUT,
				s->srt_o_blkno, s->srt_o_buffer, &sys_err);
	}
	
	if (stat == OK && *(s->srt_uiptr) == 0)
	{
	    /*  Initialize for next buffer. */

	    s->srt_o_blkno++;
	    s->srt_o_offset = 0;
	    continue;
	}

	if (stat != OK)
	{
	    if (stat == SR_EXCEED_LIMIT)
	    {
		uleFormat(NULL, E_DM9703_SR_WRITE_ERROR, &sys_err, ULE_LOG, NULL,
		    NULL, 0, NULL, &err_code, 0);
		SETDBERR(&s->srt_dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
		return (E_DB_ERROR);
	    }
	    break;
	}
	SETDBERR(&s->srt_dberr, 0, E_DM0065_USER_INTR);
        return (E_DB_ERROR);
    }

    /*	Handle write error reporting. */

    uleFormat(NULL, E_DM9703_SR_WRITE_ERROR, &sys_err, 
                   ULE_LOG, NULL, 0, 0, 0, &err_code, 0);
    SETDBERR(&s->srt_dberr, 0, E_DM970D_SORT_WRITE_RECORD);
    return (E_DB_ERROR);
}

/*{
** Name: output_run	- Handle end of run processing.
**
** Description:
**      This routine handles the processing that is needed to end a run.
**	This either ends the last run of a pass or starts a new run.
**	Starting a new run requires that the run descriptor be updated
**	for the new run.  If the run descriptor is full, the descriptor
**	if rewritten to its location on the output file and is linked
**	to the next run descriptor.  The run descriptor is always allocated
**	at the beginning of a buffer.
**
** Inputs:
**      sort_cb                         The sort control block.
**      flag				Either START_OUTPUT_PASS, 
**                                      STOP_OUTPUT_PASS
**					BEGIN_OUTPUT_RUN, END_OUTPUT_RUN.
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR			E_DM970F_SORT_OUTPUT_RUN
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	06-feb-1986 (derek)
**          Created new for jupiter.
**	09-oct-91 (jrb) integrated the following change: 17-apr-1991 (bryanp)
**	    B36569: If we end an output pass, and we have used less than
**	    MAX_MERGE runs, and only the first buffer of the output file was
**	    used, then we do not need to write the buffer to disk. Instead,
**	    input_run recognizes this condition and re-uses the srt_o_buffer
**	    directly. The srt_state flag SRT_BUFFER_IN_MEMORY is used to
**	    track when we are in this condition.
**	31-jan-94 (jrb)
**	    Fixed bug 58017: open files if they are not yet open.  It was 
**	    possible for there to be so many duplicates in a sort that
**          we got 15 runs in a single buffer!  This means we'd call this
**          routine which would attempt to write the merge descriptor, but 
**	    since the first buffer hadn't been written, the files weren't open
**          and we'd get an error.
**      23-may-1994 (markm)
**          Split offset calculation into two fields offset and blkno,
**          this allows us to sort greater than MAXI4 tables (60023).
*/
static DB_STATUS
output_run(
DMS_SRT_CB        *sort_cb,
i4            flag)
{
    DMS_SRT_CB	    *s = sort_cb;
    STATUS	    stat;
    CL_ERR_DESC	    sys_err;
    i4	    err_code;

    if (flag & START_OUTPUT_PASS)
    {
	s->srt_o_offset = sizeof(DMS_RUN_DESC);
	s->srt_orun_desc.next_block = 0;
	s->srt_o_blkno = 0;
	s->srt_o_rnext = 0;
	s->srt_run_number = 0;
	s->srt_o_records = 0;
    }
    if (flag & END_OUTPUT_RUN)
    {
	s->srt_orun_desc.runs[s->srt_o_rnext -1].records = s->srt_o_records;
	if (s->srt_o_records == 0)
	    --s->srt_o_rnext;
    }
    if ((flag & BEGIN_OUTPUT_RUN) && s->srt_o_rnext < MAX_MERGE)
    {
        s->srt_orun_desc.runs[s->srt_o_rnext].offset = s->srt_o_offset;
        s->srt_orun_desc.runs[s->srt_o_rnext].blkno = s->srt_o_blkno;

	s->srt_o_rnext++;
	s->srt_run_number++;
	s->srt_o_records = 0;
	s->srt_state &= ~SRT_DUPLICATE;
	return (E_DB_OK);
    }
    if ((flag & (STOP_OUTPUT_PASS | BEGIN_OUTPUT_RUN)) == 0)
	return (E_DB_OK);

    for (;;)
    {
	/*  Handle output ending in merge block special. */

	if (s->srt_o_blkno != s->srt_orun_desc.next_block)
	{
	    /*  Write the last partial block. */

	    s->srt_c_writes++;

	    if (DMZ_SRT_MACRO(2))
	    {
		stat = srt1_translated_io(s, SRT_F_WRITE, SRT_F_OUTPUT,
			    s->srt_o_blkno, s->srt_o_buffer, &sys_err);
	    }
	    else
	    {
    		stat = srt0_translated_io(s, SRT_F_WRITE, SRT_F_OUTPUT,
			    s->srt_o_blkno, s->srt_o_buffer, &sys_err);
	    }
	    
	    if (stat != OK)
	    {
		if (stat == SR_EXCEED_LIMIT)
		{
		    uleFormat(NULL, E_DM9703_SR_WRITE_ERROR, &sys_err, ULE_LOG, NULL, 
			NULL, 0, NULL, &err_code, 0);
		    SETDBERR(&s->srt_dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
		    return (E_DB_ERROR);
		}
		break;
	    }

	    /*  Read the merge descriptor. */
	    s->srt_c_reads++;

	    if (DMZ_SRT_MACRO(2))
	    {
		stat = srt1_translated_io(s, SRT_F_READ, SRT_F_OUTPUT,
		    s->srt_orun_desc.next_block, s->srt_o_buffer, &sys_err);
	    }
	    else
	    {
		stat = srt0_translated_io(s, SRT_F_READ, SRT_F_OUTPUT,
		    s->srt_orun_desc.next_block, s->srt_o_buffer, &sys_err);
	    }
	    
	    if (stat != E_DB_OK)
		break;
	}

	/*  Update the merge descriptor. */

	s->srt_orun_desc.run_count = s->srt_o_rnext;
	*(DMS_RUN_DESC *)s->srt_o_buffer = s->srt_orun_desc;
	((DMS_RUN_DESC *)s->srt_o_buffer)->next_block = ++s->srt_o_blkno;
    	if (flag & STOP_OUTPUT_PASS)
	{
	    ((DMS_RUN_DESC *)s->srt_o_buffer)->next_block = -1;
	}

	/*
	** Write back the merge descriptor to the top of the output run.
	** If we are at the end of the pass and the current run is the
	** first run (indicated by the merge descriptor's next_block field
	** being zero), then no need to write the descriptor to disk.  In
	** this case we will skip reading it off the disk at the start of
	** the read phase as it is already in memory.
	*/
	if (((flag & STOP_OUTPUT_PASS) == 0) || (s->srt_orun_desc.next_block))
	{
	    /*  Write the merge descriptor back. */

	    s->srt_c_writes++;

	    /*
	    **  Open files if they're not open.
	    */
	    if ((s->srt_state & SRT_OUTPUT) == 0)
	    {
		STATUS	stat;
		DB_STATUS	status;
		DB_STATUS   err_code;

		if (DMZ_SRT_MACRO(2))
		    status = srt1_open_files(s, SRT_F_OUTPUT);
		else
		    status = srt0_open_files(s, SRT_F_OUTPUT);

		if (status != E_DB_OK)
		{
		    if (s->srt_dberr.err_code == 0)
			SETDBERR(&s->srt_dberr, 0, E_DM970F_SORT_OUTPUT_RUN);
		    return (E_DB_ERROR);
		}
		s->srt_state |= SRT_OUTPUT;
	    }


	    if (DMZ_SRT_MACRO(2))
	    {
		stat = srt1_translated_io(s, SRT_F_WRITE, SRT_F_OUTPUT,
			s->srt_orun_desc.next_block, s->srt_o_buffer, &sys_err);
	    }
	    else
	    {
		stat = srt0_translated_io(s, SRT_F_WRITE, SRT_F_OUTPUT,
			s->srt_orun_desc.next_block, s->srt_o_buffer, &sys_err);
	    }
	    
	    if (stat != OK)
	    {
		uleFormat(NULL, E_DM9703_SR_WRITE_ERROR, &sys_err, ULE_LOG, NULL, 
		    NULL, 0, NULL, &err_code, 0);
		SETDBERR(&s->srt_dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
		return (E_DB_ERROR);
	    }
	}

	if ((flag & STOP_OUTPUT_PASS) != 0)
	{
	    /*
	    **	B36569:
	    **	At the end of an output pass, if we had less than MAX_MERGE
	    **	runs, then we currently have block 0 in memory, and input_run
	    **	will take advantage of this fact, and will not try to read
	    **	the (possibly) unwritten runs from disk (instead, it will
	    **	copy the sort buffer over).
	    */
	    if (s->srt_orun_desc.next_block == 0)
		s->srt_state |= SRT_BUFFER_IN_MEMORY;
	    else
		s->srt_state &= ~SRT_BUFFER_IN_MEMORY;
	}
	else
	{
	    s->srt_o_rnext = 0;
	    s->srt_orun_desc.next_block = s->srt_o_blkno;
	    s->srt_o_offset = sizeof(DMS_RUN_DESC);
	}
	if (flag & BEGIN_OUTPUT_RUN)
	{
            s->srt_orun_desc.runs[s->srt_o_rnext].offset = s->srt_o_offset;
            s->srt_orun_desc.runs[s->srt_o_rnext].blkno = s->srt_o_blkno;

	    s->srt_o_rnext++;
	    s->srt_run_number++;
	    s->srt_o_records = 0;
	    s->srt_state &= ~SRT_DUPLICATE;
	}
	return (E_DB_OK);
    }

    /*	Handle error reporting. */

    uleFormat(NULL, E_DM9703_SR_WRITE_ERROR, &sys_err, 
                  ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
    SETDBERR(&s->srt_dberr, 0, E_DM970F_SORT_OUTPUT_RUN);
    return (E_DB_ERROR);
}

/*{
** Name: input_run	- Handle start of input run processing.
**
** Description:
**      This routine handles reading in the next set of run descriptors,
**	postioning each of the runs and reading in the first record from
**	each run and creating the merge heap.
**
** Inputs:
**      sort_cb                         The sort control block.
**	flag				Either START_INPUT_PASS, 
**                                      CREATE_INPUT_RUN.
**
** Outputs:
**	Returns:
**	    E_DB_OK			Ok.
**	    E_DB_INFO			No more runs to open.
**	    E_DB_ERROR			E_DM9710_SORT_INPUT_RUN.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	07-feb-1986 (derek)
**          Created new for jupiter.
**      23-may-1994 (markm)
**          Remove calculation for offset and blkno and replace with
**          direct assignments (60023).
*/
static DB_STATUS
input_run(
DMS_SRT_CB         *sort_cb,
i4            flag)
{
    DMS_SRT_CB		*s = sort_cb;
    DMS_MERGE_DESC	*m;
    i4		i;
    i4		j;
    i4		k;
    i4		**heap = s->srt_heap;
    char		*x;
    DB_STATUS		status;
    STATUS		stat;
    CL_ERR_DESC		sys_err;
    i4		err_code;

    if (flag & START_INPUT_PASS)
    {
	/*  Read the first run descriptor from the input file. */

	m = (DMS_MERGE_DESC *)heap[1];
	m->in_count = 0;
	s->srt_c_reads++;
	s->srt_i_rnext = 0;
	s->srt_next_record = 0;

	if (s->srt_orun_desc.next_block)
	{
	    if (DMZ_SRT_MACRO(2))
	    {
		stat = srt1_translated_io(s, SRT_F_READ, SRT_F_INPUT,
			(i4)0, m->in_buffer, &sys_err);
	    }
	    else
	    {
		stat = srt0_translated_io(s, SRT_F_READ, SRT_F_INPUT,
			(i4)0, m->in_buffer, &sys_err);
	    }
	    if (stat != OK)
	    {
		uleFormat(NULL, E_DM9702_SR_READ_ERROR, &sys_err, 
                        ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		SETDBERR(&s->srt_dberr, 0, E_DM9710_SORT_INPUT_RUN);
		return (E_DB_ERROR);
	    }
	}
	else
	{
	    /*  Copy merge header buffer from end output pass's buffer. */

	    MEcopy(s->srt_o_buffer, s->srt_o_size, m->in_buffer);
	}

	s->srt_irun_desc = *(DMS_RUN_DESC *)m->in_buffer;

    }

    if ((flag & BEGIN_INPUT_RUN) == 0)
	return (E_DB_OK);

    /*
    ** For each run being merged in this pass, build the merge descriptor
    ** describing this run. If necessary, read a new run descriptor from the
    ** sort file. If the run is not the first run described by the run
    ** descriptor, then we may need to read the appropriate sort block into
    ** memory, unless the sort block is already in memory, in which case we
    ** must copy it to the appropriate place. (The first run described by
    ** the run descriptor has its contents set up when the input run descriptor
    ** is set up).
    */

    for (i = 1; i <= s->srt_last_record; i++)
    {
	m = (DMS_MERGE_DESC *)heap[i];
	if (s->srt_i_rnext >= s->srt_irun_desc.run_count)
	{
	    if (s->srt_irun_desc.next_block == -1)
		break;
	    s->srt_c_reads++;
	    if (DMZ_SRT_MACRO(2))
	    {
		stat = srt1_translated_io(s, SRT_F_READ, SRT_F_INPUT,
			s->srt_irun_desc.next_block, m->in_buffer, &sys_err);
	    }
	    else
	    {
		stat = srt0_translated_io(s, SRT_F_READ, SRT_F_INPUT,
			s->srt_irun_desc.next_block, m->in_buffer, &sys_err);
	    }
	    if (stat != OK)
	    {
		uleFormat(NULL, E_DM9702_SR_READ_ERROR, &sys_err, 
			   ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		SETDBERR(&s->srt_dberr, 0, E_DM9710_SORT_INPUT_RUN);
		return (E_DB_ERROR);
	    }
	    s->srt_irun_desc = *(DMS_RUN_DESC *)m->in_buffer;
	    s->srt_i_rnext = 0;
	}
	    	    
        m->in_offset = s->srt_irun_desc.runs[s->srt_i_rnext].offset;
        m->in_blkno = s->srt_irun_desc.runs[s->srt_i_rnext].blkno;
        m->in_count = s->srt_irun_desc.runs[s->srt_i_rnext].records;

	/*  Don't re-read the block with the merge header. */

	if (s->srt_i_rnext)
	{
	    /*
	    ** BUG 3136 & 5730 & 36569 -- copy buffer contents since this block
	    ** has not been written to disk. (nancy - 3136)(sandyh - 5730)
	    */
	    if ((s->srt_state & SRT_BUFFER_IN_MEMORY) != 0 && m->in_blkno == 0)
	    {
		/*
		** At the end of the last output pass, block 0 of the sort
		** file was left in memory in srt_o_buffer. It may or may not
		** have been written to disk, depending on whether or not
		** there was more than one total block's worth of information
		** in the sort file, but it is definitely in memory, and so
		** we can take advantage of that fact to short-circuit the I/O
		** and just copy the block's contents over to the desired
		** buffer area.
		*/
		MEcopy(s->srt_o_buffer, s->srt_o_size, m->in_buffer);
	    }
	    else
	    {
		s->srt_c_reads++;
		if (DMZ_SRT_MACRO(2))
		{
		    srt1_translated_io(s, SRT_F_READ, SRT_F_INPUT, m->in_blkno,
			m->in_buffer, &sys_err);
		}
		else
		{
		    srt0_translated_io(s, SRT_F_READ, SRT_F_INPUT, m->in_blkno,
			m->in_buffer, &sys_err);
		}
	    }
	}
        s->srt_next_record++;
	s->srt_i_rnext++;
    }
    if (s->srt_next_record == 0)
	return (E_DB_INFO);

    for (k = 1; k <= s->srt_next_record; k++)
    {
	status = read_record((DMS_MERGE_DESC *)heap[k]);
	if (status != E_DB_OK)
	{
	    if (s->srt_dberr.err_code > E_DM_INTERNAL)
	    {
		uleFormat(&s->srt_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
		    &err_code, 0);
	    }
	    return (E_DB_ERROR);
	}

	/*  Move record up to correct place in heap. */

        s->srt_adfcb.adf_errcb.ad_errcode=0;
	for (i = k; j = i, i >>= 1; )
	{
	    i4		cmpres;
 	    s->srt_c_comps++;
	    if ((cmpres = adt_sortcmp(&s->srt_adfcb, s->srt_atts, 
		s->srt_a_count, ((DMS_MERGE_DESC *)heap[i])->record,
		((DMS_MERGE_DESC *)heap[j])->record, 0)) > 0)
	    {
		x = (char *) heap[i];
		heap[i] = heap[j];
		heap[j] = (i4*) x;
	    }
	    if(s->srt_adfcb.adf_errcb.ad_errcode!=0)
	    {
	        uleFormat(&s->srt_adfcb.adf_errcb.ad_dberror, 0, 
				NULL, ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		return (E_DB_ERROR);
	    }
	    if (cmpres <= 0) break;
	}
    }

    return (E_DB_OK);
}

/*{
** Name: get_in_memory_record	- Get the next sorted record, in-memory.
**
** Description:
**      This routine returns the next sorted record to the caller. It is used
**	in place of dmse_get_record when in-memory sorting is being performed.
**
**	The next record to be returned is the one at the root of the heap. It
**	is removed, duplicate checking is performed, and the heap is
**	re-heapified.
**
** Inputs:
**      sort_cb                         The sort control block.
**      record                          Pointer to location to return record.
**
** Outputs:
**      err_code                        Reason for error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_INFO			Record has a duplicate prefix 
**                                      of the previous record.
**	    E_DB_ERROR			E_DM970A_SORT_GET
**					E_DM9706_SORT_PROTOCOL_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	10-oct-1991 (bryanp)
**          Created new for in-memory sorting enhancements: When in-memory
**	    sorting is being performed, get the next record from the heap, not
**	    from the merged runs.
**	3-dec-97 (inkdo01)
**	    Improvements in sorting algorithms.
*/
static DB_STATUS
get_in_memory_record(
DMS_SRT_CB        *sort_cb,
char               **record,
DB_ERROR          *dberr)
{
    DMS_SRT_CB         *s = sort_cb;
    i4		i;
    i4		j;
    i4		**heap = s->srt_heap;
    char		*x;
    DB_STATUS		status;
    bool		first_get = FALSE;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (s->srt_state & SRT_PUT)
    {
	SETDBERR(dberr, 0, E_DM9706_SORT_PROTOCOL_ERROR);
	return (E_DB_ERROR);
    }

    /* no more records? */
    if (s->srt_next_record <= 0)
    {
	SETDBERR(dberr, 0, E_DM0055_NONEXT);
	return (E_DB_ERROR);
    }

    if (s->srt_state & SRT_GETFIRST)
    {
	s->srt_state &= ~SRT_GETFIRST;
	s->srt_state |= SRT_GET;
	s->srt_inmem_rem = s->srt_next_record;
					/* upper limit on row count */
	first_get = TRUE;
    }

    /*
    ** Point (*record) to the first record in the heap. Then remove that
    ** record from the heap and re-heapify it.
    */

    for (;(s->srt_state & SRT_GET);)
    {
	/* no more records? */
	if (s->srt_inmem_rem <= 0 || heap[1] == NULL)
	{
	    SETDBERR(dberr, 0, E_DM0055_NONEXT);
	    return (E_DB_ERROR);
	}

	*record = (char *)heap[1];
	s->srt_inmem_rem--;

	/* Bring next smallest record to top, and re-heapify en route. */
        s->srt_adfcb.adf_errcb.ad_errcode=0;
	for (i = 1, j = 2; j <= s->srt_next_record; )
	{
	    if (j < s->srt_next_record && (heap[j] == NULL ||
		heap[j+1] != NULL && (s->srt_c_comps++,
		adt_sortcmp(&s->srt_adfcb, s->srt_ratts, s->srt_ra_count, 
                    (char *)heap[j], (char *)heap[j + 1], 0) > 0)))
	    {
	        j++;
	    }
	    if(s->srt_adfcb.adf_errcb.ad_errcode!=0)
	    {
	        uleFormat(&s->srt_adfcb.adf_errcb.ad_dberror, 0, 
				NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		SETDBERR(dberr, 0, E_DM0055_NONEXT);
		return (E_DB_ERROR);
	    }
	    heap[i] = heap[j];
	    if (heap[j] == NULL) 
		break;		/* end of branch */
	    heap[j] = NULL;
	    i = j;
	    j += j;
	}

	/* Duplicate removal/check logic. */
	if ((s->srt_state & SRT_CHECK_DUP) != 0 && first_get == FALSE)
	{
	    i4	    compare;

 	    s->srt_c_comps++;
	    /* srt_c_count instead of srt_a_count */
	    compare = adt_sortcmp(&s->srt_adfcb, s->srt_atts, s->srt_c_count, 
		s->srt_duplicate, *record, (i4) (s->srt_k_count+1));
	    if(s->srt_adfcb.adf_errcb.ad_errcode!=0)
	    {
	        uleFormat(&s->srt_adfcb.adf_errcb.ad_dberror, 0, 
				NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		SETDBERR(dberr, 0, E_DM0055_NONEXT);
		return (E_DB_ERROR);
	    }
	    if ( s->srt_state & SRT_ELIMINATE && compare == 0 )
		continue;

	    MEcopy(*record, s->srt_iwidth, s->srt_duplicate);

	    /* Note, while duplicates removal can be done at any time during
	    ** a sort, duplicates checking/flagging can only be done at GET
	    ** time because that is the only time the sort caller is looking
	    ** for the appropriate return code. */
	    if (s->srt_state & SRT_CHECK_KEY)
	    {
		if (compare < 0)
		    compare = -compare;
		if ((compare == 0) || (compare > s->srt_k_count))
		{
		    s->srt_c_records++;
		    return (E_DB_INFO);
		}
	    }
	}
	else if ((s->srt_state & SRT_CHECK_DUP) != 0 && first_get == TRUE)
	{
	    MEcopy(*record, s->srt_iwidth, s->srt_duplicate);
	}

	s->srt_c_records++;
	return (E_DB_OK);
    }

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM970A_SORT_GET);
    }
    return (E_DB_ERROR);
}

/*{
** Name: srt0_open_files - Open all SR files for input or output runs
**
** Description:
**	Creates one or more files for input or output runs.  We create one file
**	per location; the filenames are acquired from dm2f_filename and created.
**
** Inputs:
**	    s			    The sort control block.
**	    in_or_out		    Either SRT_F_INPUT or SRT_F_OUTPUT
**				    depending on whether we are opening input
**				    or output merge files.
**
** Outputs:
**	none
**
**	Returns:
**	    E_DB_OK			Ok.
**	    E_DB_ERROR			Couldn't create files.
**	    
**	Exceptions:
**	    none
**
** Side Effects:
**	    SR_IO's in the sort cb are modified when SRopen get a hold of them.
**	    Any errors are returned in the sort cb's srt_err_code field.
**
** History:
**	27-dec-89 (jrb)
**	    Created.
**      16-oct-93 (jrb)
**	    Added support for using a subset of work locations for smaller
**	    sorts.
**	6-Feb-2004 (schka24)
**	    Update dm2f-filename call.
*/
static DB_STATUS
srt0_open_files(
DMS_SRT_CB	*s,
i4		in_or_out)
{
    i4	block_cnt;
    i4	retry_cnt;
    i4	next_bit = -1;
    i4	created;
    i4	closed;
    i4	ln = 0;
    i4	sr_entry;
    i4	nl = s->srt_num_locs;
    i4	prealloc;
    DM_FILE	filename;
    STATUS	stat;
    DB_STATUS	status;
    i4	err_code;
    CL_ERR_DESC	sys_err;
    i4	i;


    /* the block count depends on whether we're creating for the output
    ** run or input run; for the output run we use the estimated number of
    ** blocks passed in; for input run we have the actual block count
    */
    block_cnt = (in_or_out == SRT_F_INPUT) ? s->srt_o_blkno : s->srt_blocks;
    prealloc = block_cnt / nl;

    /* point to the first work location to use */
    for (i=0; i < s->srt_spill_index; i++)
        next_bit = BTnext(next_bit, (char *)s->srt_wloc_mask, s->srt_mask_size);


    for (created = 0; created < nl; created++)
    {
	/* Note: with BTnext we can easily wrap-around in the bit stream 
	**       because it returns a -1 when it hits the end
	*/
	while ((next_bit = BTnext(next_bit, (char *)s->srt_wloc_mask,
		s->srt_mask_size)) == -1
	    )
	{
	    continue;
	}
	sr_entry = ln + (in_or_out == SRT_F_INPUT  ?  nl  :  0);

	for (retry_cnt = 0;;)
	{
	    dm2f_filename((in_or_out == SRT_F_INPUT) ? DM2F_SRI : DM2F_SRO,
		&s->srt_table_id, ln, 0, 0, &filename);

	    stat = SRopen(&s->srt_sr_files[sr_entry],
		    s->srt_array_loc[next_bit].physical.name,
		    s->srt_array_loc[next_bit].phys_length,
		    filename.name, FILENAME_LENGTH,
		    (in_or_out == SRT_F_INPUT) ? s->srt_i_size : s->srt_o_size,
		    SR_IO_CREATE, prealloc, &sys_err);
	    if (stat == OK)
		break;
	    /*
	    ** Modify filename to get past problem of trying to create a file
	    ** which conflicts with an existing file that has been left around
	    ** due to unsuccessful completion of a previous sort.
	    */
	    if ((stat == SR_BADCREATE) || (stat == SR_BADOPEN))
	    {
		if (retry_cnt > MAX_SRT_NAME)
		    break;
		retry_cnt++;
	    }

	    /*
	    **  If can't open a file because of quotas, look for free tcb's to
	    **  free and retry.  If there are no free tcb's, return 
	    **  RESOURCE_QUOTA_EXCEED.
	    */
	    if (stat == SR_EXCEED_LIMIT)
	    {
		status = dm2t_reclaim_tcb(s->srt_lk_id, s->srt_log_id, &s->srt_dberr);
		if (status != E_DB_OK)
		{
		    if (s->srt_dberr.err_code != E_DM9328_DM2T_NOFREE_TCB)
		    {
			uleFormat(&s->srt_dberr, 0, NULL, ULE_LOG, NULL, 
				    NULL, 0, NULL, &err_code, 0);
			stat = FAIL;
		    }
		    break;
		}
	    }
	}

	if (stat != OK)
	{
	    uleFormat(NULL, E_DM9711_SR_OPEN_ERROR, &sys_err, ULE_LOG, NULL, 
			NULL, 0, NULL, &err_code, 2,
			s->srt_array_loc[next_bit].phys_length,
			s->srt_array_loc[next_bit].physical.name,
			FILENAME_LENGTH, &filename);

	    if (stat == SR_EXCEED_LIMIT)
		SETDBERR(&s->srt_dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
	    else
		CLRDBERR(&s->srt_dberr);/* let caller set the error code */ 

	    /* Now close all successfully opened files; ignore errors since
	    ** we're already in an error state.
	    */
	    for (closed = created; closed >= 0; closed--)
	    {
		sr_entry = ln + (in_or_out == SRT_F_INPUT  ?  nl  :  0);
		_VOID_ SRclose(&s->srt_sr_files[sr_entry], SR_IO_DELETE,
								    &sys_err);
		ln--;
	    }

	    return (E_DB_ERROR);
	}
	ln++;
    }
    return(E_DB_OK);
}

/*{
** Name: srt0_translated_io - Do translated reads/writes to merge files
**
** Description:
**	This function handles all I/O through SR for merge files.
**	The block number is translated into a location_number and
**	relative_block_number so that multiple sort locations can
**	be used.
**
**	The translation is straightforward: the block number is divided by
**	the number of locations to get the relative block number, and the
**	remainder gives the location number to use.  This has the effect of
**	placing one block on each location in a round-robin fashion.
**
**	
** Inputs:
**	s			The sort cb
**	read_write		Either SRT_F_READ or SRT_F_WRITE
**	in_or_out		Either SRT_F_INPUT or SRT_F_OUTPUT
**	blkno			The block number to read or write
**	buffer			The buffer to read_from/write_to
**	err			CL_ERR_DESC if SR call failed
**
** Outputs:
**	Returns:
**	    OK			Ok.
**	    SR return codes on error.
**	    
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	27-dec-89 (jrb)
**	    Created.
**      18-oct-93 (jrb)
**	    Added error msgs for failed reads and writes.
*/
static STATUS
srt0_translated_io(
DMS_SRT_CB          *s,
i4		    read_write,
i4		    in_or_out,
i4		    blkno,
char		    *buffer,
CL_ERR_DESC	    *err)
{
    i4	    rbn;	    /* relative block number */
    i4	    ln;		    /* location number */
    i4	    sr_entry;
    i4	    nl = s->srt_num_locs;
    i4	    is_input;
    STATUS	    stat;
    i4	    i;
    i4	    next_bit = -1;
    i4	    err_code;

    is_input = (in_or_out == SRT_F_INPUT  &&  s->srt_swapped == FALSE)
	    || (in_or_out == SRT_F_OUTPUT &&  s->srt_swapped == TRUE);

    ln = blkno % nl;
    rbn = blkno / nl;
    sr_entry = ln + (is_input  ?  nl  :  0);

    if (DMZ_SES_MACRO(10))
	dmd_siotrace(read_write, !is_input, blkno, sr_entry, ln, rbn);

    if (read_write == SRT_F_READ)
    {
	stat = SRread(&s->srt_sr_files[sr_entry], rbn, buffer, err);

	if (stat)
	{
	    /* We need to figure out what location had the error so we can
	    ** format a decent error msg; we know that ln tells us which 
	    ** bit in the srt_wloc_mask the culprit is.
	    */
	    for (i=0; i < ln+1; i++)
	    {
		while ((next_bit = BTnext(next_bit, (char *)s->srt_wloc_mask,
			s->srt_mask_size)) == -1
		    )
		{
		    continue;
		}
	    }

	    uleFormat(NULL, E_DM9712_SR_READ_ERROR, err, ULE_LOG, NULL, 
			NULL, 0, NULL, &err_code, 2,
			s->srt_array_loc[next_bit].phys_length,
			s->srt_array_loc[next_bit].physical.name, 0, rbn);
	}
    }
    else
    {
	stat = SRwrite(&s->srt_sr_files[sr_entry], rbn, buffer, err);

	if (stat)
	{
	    for (i=0; i < ln+1; i++)
	    {
		while ((next_bit = BTnext(next_bit, (char *)s->srt_wloc_mask,
			s->srt_mask_size)) == -1
		    )
		{
		    continue;
		}
	    }

	    uleFormat(NULL, E_DM9713_SR_WRITE_ERROR, err, ULE_LOG, NULL, 
			NULL, 0, NULL, &err_code, 2,
			s->srt_array_loc[next_bit].phys_length,
			s->srt_array_loc[next_bit].physical.name, 0, rbn);
	}
    }
    return (stat);
}

/*{
** Name: srt0_close_files - Close all input or output merge files.
**
** Description:
**	This function simply steps through all the files for the virtual
**	input or output file and closes them.
**
** Inputs:
**	s			The sort cb
**	in_or_out		Close the virtual input file or output file
**	err			CL_ERR_DESC from SRclose
**
** Outputs:
**	Returns:
**	    OK			Ok.
**	    SR error codes for errors
**	    
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	27-dec-89 (jrb)
**	    Created.
*/
static STATUS
srt0_close_files(
DMS_SRT_CB	    *s,
i4		    in_or_out,
CL_ERR_DESC	    *err)
{
    i4	    num_files;
    i4	    closed;
    i4	    ln = 0;
    i4	    nl = s->srt_num_locs;
    i4	    is_input;
    i4	    sr_entry;
    STATUS	    stat;
    CL_ERR_DESC	    sys_err;
    STATUS	    old_stat = OK;
    CL_ERR_DESC	    old_sys_err;

    is_input = (in_or_out == SRT_F_INPUT  &&  s->srt_swapped == FALSE)
	    || (in_or_out == SRT_F_OUTPUT &&  s->srt_swapped == TRUE);

    for (closed = 0; closed < nl; closed++)
    {
    	sr_entry = ln + (is_input  ?  nl  :  0);
	stat = SRclose(&s->srt_sr_files[sr_entry], SR_IO_DELETE, &sys_err);

	if (stat != OK  &&  stat != SR_BADFILE)
	{
	    /* save this error to return to caller, but in the meanwhile
	    ** continue trying to close files
	    */
	    old_stat = stat;
	    old_sys_err = sys_err;
	}

	ln++;
    }

    *err = old_sys_err;
    return (old_stat);
}

/*{
** Name: srt1_open_files - Open all SR files for input or output runs
**
** Description:
**	Creates one or more files for input or output runs.  The number of files
**	to be created is determined from a field in the sort cb; the filenames
**	are acquired from dm2f_filename and created.
**
** Inputs:
**	    s			    The sort control block.
**	    in_or_out		    Either SRT_F_INPUT or SRT_F_OUTPUT
**				    depending on whether we are opening input
**				    or output merge files.
**
** Outputs:
**	none
**
**	Returns:
**	    E_DB_OK			Ok.
**	    E_DB_ERROR			Couldn't create files.
**	    
**	Exceptions:
**	    none
**
** Side Effects:
**	    SR_IO's in the sort cb are modified when SRopen get ahold of them.
**	    Any errors are returned in the sort cb's srt_err_code field.
**
** History:
**	06-dec-89 (jrb)
**	    Created.
**	6-Feb-2004 (schka24)
**	    Update dm2f-filename call.
*/
static DB_STATUS
srt1_open_files(
DMS_SRT_CB	*s,
i4		in_or_out)
{
    i4	block_cnt;
    i4	num_files;
    i4	retry_cnt;
    i4	bits;
    i4	next_bit = -1;
    i4	created;
    i4	closed;
    i4	ln;
    i4	sr_entry;
    i4	nl = s->srt_num_locs;
    i4	prealloc;
    DM_FILE	filename;
    STATUS	stat;
    DB_STATUS	status;
    i4	err_code;
    CL_ERR_DESC	sys_err;


    /* find number of files to create; we use half of them, rounding up if
    ** there's an odd number of locations
    */
    num_files = (nl+1)/2;

    /* if creating files for the output runs, we use the low half of the
    ** srt_sr_files array, else we use the high half
    */
    if (in_or_out == SRT_F_INPUT)
    {
	for (bits=0; bits < nl/2; bits++)
	{
	    next_bit = 
		BTnext(next_bit, (char *)s->srt_wloc_mask, s->srt_mask_size);
	}
    }

    /* the block count depends on whether we're creating for the output
    ** run or input run; for the output run we use the estimated number of
    ** blocks passed in; for input run we have the actual block count
    */
    block_cnt = (in_or_out == SRT_F_INPUT) ? s->srt_o_blkno : s->srt_blocks;

    /* set location number; shift to upper half for input files */
    ln = (in_or_out == SRT_F_INPUT) ? nl/2 : 0;

    for (created = 0; created < num_files; created++)
    {
	for (retry_cnt = 0;;)
	{
	    dm2f_filename((in_or_out == SRT_F_INPUT) ? DM2F_SRI : DM2F_SRO,
		&s->srt_table_id, ln, 0, 0, &filename);

	    next_bit = 
		BTnext(next_bit, (char *)s->srt_wloc_mask, s->srt_mask_size);
	    prealloc = (ln != s->srt_split_loc) ? 2*block_cnt/nl : block_cnt/nl;
	    sr_entry = ln + (in_or_out == SRT_F_INPUT  &&  (nl & 1));

	    stat = SRopen(&s->srt_sr_files[sr_entry],
		    s->srt_array_loc[next_bit].physical.name,
		    s->srt_array_loc[next_bit].phys_length,
		    filename.name, FILENAME_LENGTH,
		    (in_or_out == SRT_F_INPUT) ? s->srt_i_size : s->srt_o_size,
		    SR_IO_CREATE, prealloc, &sys_err);
	    if (stat == OK)
		break;
	    /*
	    ** Modify filename to get past problem of trying to create a file
	    ** which conflicts with an existing file that has been left around
	    ** due to unsuccessful completion of a previous sort.
	    */
	    if ((stat == SR_BADCREATE) || (stat == SR_BADOPEN))
	    {
		if (retry_cnt > MAX_SRT_NAME)
		    break;
		retry_cnt++;
	    }

	    /*
	    **  If can't open a file because of quotas, look for free tcb's to
	    **  free and retry.  If there are no free tcb's, return 
	    **  RESOURCE_QUOTA_EXCEED.
	    */
	    if (stat == SR_EXCEED_LIMIT)
	    {
		status = dm2t_reclaim_tcb(s->srt_lk_id, s->srt_log_id, &s->srt_dberr);
		if (status != E_DB_OK)
		{
		    if (s->srt_dberr.err_code != E_DM9328_DM2T_NOFREE_TCB)
		    {
			uleFormat(&s->srt_dberr, 0, NULL, ULE_LOG, NULL, 
				    NULL, 0, NULL, &err_code, 0);
			stat = FAIL;
		    }
		    break;
		}
	    }
	}

	if (stat != OK)
	{
	    uleFormat(NULL, E_DM9700_SR_OPEN_ERROR, &sys_err, ULE_LOG, NULL, 
			NULL, 0, NULL, &err_code, 0);
	    if (stat == SR_EXCEED_LIMIT)
		SETDBERR(&s->srt_dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
	    else
		CLRDBERR(&s->srt_dberr);/* let caller set the error code */ 

	    /* Now close all successfully opened files; ignore errors since
	    ** we're already in an error state.
	    */
	    for (closed = created; closed >= 0; closed--)
	    {
		sr_entry = ln + (in_or_out == SRT_F_INPUT  &&  (nl & 1));
		_VOID_ SRclose(&s->srt_sr_files[sr_entry], SR_IO_DELETE,
								    &sys_err);
		ln--;
	    }

	    return (E_DB_ERROR);
	}
	ln++;
    }
    return(E_DB_OK);
}

/*{
** Name: srt1_translated_io - Do translated reads/writes to merge files
**
** Description:
**	This function handles all I/O through SR for merge files.
**	The block number is translated into a location_number and
**	relative_block_number so that multiple sort locations can
**	be used.
**
**	There are a few formulas for doing this translation and they might
**	not be easily understood at first.  Here's basically what we're trying
**	to accomplish: we have n sort locations and we want to use equal (or
**	nearly equal) space on all of them.  An additional nicety would be
**	to separate the input and output merge virtual files onto different
**	locations so that seek time is reduced (assuming the locations are
**	on different disks).
**
**	If n is even we simply spread the virtual merge files into n/2 pieces.
**	For example, if n=4 then the files would be allocated like this:
**
**	 l1	 l2	 l3	 l4
**	-----	-----	-----	-----
**	| O |	| O |	| I |	| I |	    O = Virtual output merge file
**	|   |	|   |	|   |	|   |	    I = Virtual input merge file
**	|   |	|   |	|   |	|   |
**	-----	-----	-----	-----
**
**	If n is odd we have a slight problem.  We can't just throw away one
**	location because n may equal 1.  So for odd locations we divide the
**	odd location in half; we use half of it for the virtual input file and
**	half for the virtual output file.  For example, if n=5 we would do this:
**
**	 l1	 l2	 l3	 l4	 l5
**	-----	-----	-----	-----	-----
**	| O |	| O |	| O |	| I |	| I |
**	|   |	|   |	|---|	|   |	|   |
**	|   |	|   |	| I |	|   |   |   |
**	-----	-----	-----	-----	-----
**
**	There are actually 6 files on these 5 locations: l3 is split (i.e. it
**	has two files on it).
**
**	In order to keep the space usage the same across all locations we use
**	two blocks at a time on locations that are not split, and one block
**	at a time on one that is.  For example, still using n=5, virtual block
**	numbers 0 and 1 for the output file would go to l1's 0 and 1 blocks.
**	Virtual blocks 2 and 3 would go to l2's 0 and 1 blocks, and virtual
**	block 4 would go to l3's block 0.  Then we wrap around to l1 again for
**	virtual blocks 5 and 6.  For the virtual input file it's the same
**	thing but working from l5 down to l3.  Again, l5 and l4 get two blocks
**	each for every one block which is written to l3.
**
**	
** Inputs:
**	s			The sort cb
**	read_write		Either SRT_F_READ or SRT_F_WRITE
**	in_or_out		Either SRT_F_INPUT or SRT_F_OUTPUT
**	blkno			The block number to read or write
**	buffer			The buffer to read_from/write_to
**	err			CL_ERR_DESC if SR call failed
**
** Outputs:
**	Returns:
**	    OK			Ok.
**	    SR return codes on error.
**	    
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	06-dec-89 (jrb)
**	    Created.
*/
static STATUS
srt1_translated_io(
DMS_SRT_CB          *s,
i4		    read_write,
i4		    in_or_out,
i4		    blkno,
char		    *buffer,
CL_ERR_DESC	    *err)
{
    i4	    rbn;	    /* relative block number */
    i4	    ln;		    /* location number */
    i4	    nl = s->srt_num_locs;
    i4	    sr_entry;	    /* entry for SR_IO in srt_sr_files array */
    i4	    is_input;
    STATUS	    stat;

    is_input = (in_or_out == SRT_F_INPUT  &&  s->srt_swapped == FALSE)
	    || (in_or_out == SRT_F_OUTPUT &&  s->srt_swapped == TRUE);

    ln = is_input  ?  nl-1 - (blkno % nl)/2  :  (blkno % nl)/2;
    rbn = (blkno / nl) * ((ln != s->srt_split_loc) + 1) + ((blkno % nl) % 2);
    sr_entry = ln + (is_input  &&  (nl & 1));

    if (DMZ_SES_MACRO(10))
	dmd_siotrace(read_write, !is_input, blkno, sr_entry, ln, rbn);

    if (read_write == SRT_F_READ)
	stat = SRread(&s->srt_sr_files[sr_entry], rbn, buffer, err);
    else
	stat = SRwrite(&s->srt_sr_files[sr_entry], rbn, buffer, err);

    return (stat);
}

/*{
** Name: srt1_close_files - Close all input or output merge files.
**
** Description:
**	This function simply steps through all the files for the virtual
**	input or output file and closes them.
**
** Inputs:
**	s			The sort cb
**	in_or_out		Close the virtual input file or output file
**	err			CL_ERR_DESC from SRclose
**
** Outputs:
**	Returns:
**	    OK			Ok.
**	    SR error codes for errors
**	    
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	06-dec-89 (jrb)
**	    Created.
*/
static STATUS
srt1_close_files(
DMS_SRT_CB	    *s,
i4		    in_or_out,
CL_ERR_DESC	    *err)
{
    i4	    num_files;
    i4	    closed;
    i4	    ln;
    i4	    nl = s->srt_num_locs;
    i4	    is_input;
    i4	    sr_entry;
    STATUS	    stat;
    CL_ERR_DESC	    sys_err;
    STATUS	    old_stat = OK;
    CL_ERR_DESC	    old_sys_err;

    /* find number of files to close */
    num_files = (nl+1)/2;

    is_input = (in_or_out == SRT_F_INPUT  &&  s->srt_swapped == FALSE)
	    || (in_or_out == SRT_F_OUTPUT &&  s->srt_swapped == TRUE);

    if (is_input)
	ln = s->srt_num_locs/2;
    else
	ln = 0;

    for (closed = 0; closed < num_files; closed++)
    {
	sr_entry = ln + (is_input  &&  (nl & 1));
	stat = SRclose(&s->srt_sr_files[sr_entry], SR_IO_DELETE, &sys_err);

	if (stat != OK  &&  stat != SR_BADFILE)
	{
	    /* save this error to return to caller, but in the meanwhile
	    ** continue trying to close files
	    */
	    old_stat = stat;
	    old_sys_err = sys_err;
	}

	ln++;
    }

    *err = old_sys_err;
    return (old_stat);
}
