/*
** Copyright (c) 1985, 2008 Ingres Corporation
NO_OPTIM=dr6_us5 pym_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <ex.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <scf.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dml.h>
#include    <dmpp.h>
#include    <dm0m.h>
#include    <dm0s.h>
#include    <dm1p.h>
#include    <dm1c.h>
#include    <dm2r.h>
#include    <dm2f.h>
#include    <dm2t.h>
#include    <dm0pbmcb.h>

/**
**
**  Name: DM2F.C - FCB handling primitives.
**
**  Description:
**      This files contains the routines that manipulation FCB's.
**
**          dm2f_build_fcb - Allocate and open FCB for TCB
**	    dm2f_release_fcb - Release an FCB from a TCB.
**	    dm2f_filename - Build filename from relid.
**	    dm2f_open_file - Open a file.
**	    dm2f_close_file - Close a file.
**	    dm2f_create_file - Create and open a file.
**	    dm2f_delete_file - Delete a file.
**          dm2f_alloc_file - Allocate space to a file.
**          dm2f_force_file - Force pages to a file. 
**          dm2f_flush_file - Flush the file header.
**          dm2f_read_file  - Read pages from a file.
**          dm2f_rename - Rename a file.
**          dm2f_sense_file - Sense logical end of file. 
**          dm2f_write_file - Write pages to a file.
**          dm2f_add_fcb - Add new location information to FCB array.
**	    dm2f_check_access - Test access to page through location array.
**	    dm2f_write_list - Gather Write a database file.
**	    dm2f_complete - Gather Write completion handler.
**	    dm2f_check_list - Check if Gather Write has any queued I/Os.
**	    dm2f_force_list - Wait for I/O to complete.
**	    dm2f_sense_raw - "sense" physical end of raw area.
**
**  A temporary table FCB may be in "deferred I/O" state. In this state, no
**  physical file has yet been created, and (obviously) no disk space has yet
**  been actually allocated. The first time that we go to write a page to this
**  file, we must create the file(s) and allocate the space. This is called
**  "materializing" the file. In a multi-location FCB, the first location is
**  the only one marked with deferred I/O status.
**
**  History:
**      07-dec-1985 (derek)    
**          Created new for jupiter.
**      20-nov-87 (jennifer)
**          Added multiple location support.
**      17-oct-88 (mmm)
**	    Added sync_flag parameter to dm2f_open_file(), dm2f_build_fcb(),
**	    and dm2f_create_file().
**	 3-may-89 (rogerk)
**	    Added page number parameter to DM9005 and DM9006 error messages.
**	14-jun-1991 (Derek)
**	    Added support for file extend project.
**	    Added new arguments to dm2f_init_file for preallocation of space.
**	    Added support for initializing allocation data structures in
**	    a newly initialized file.
**	    Added dm2f_file_size routine.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    13-dec-90 (rachael)
**		Added ule_format call in dm2f_close_file() preceeding
**		log_di_error This will print the return from the DIclose and
**		show whether the qio call was made or if the fcb had something
**		other than DI_IO_ASCII_ID in io_type
**	    31-may-1991 (bryanp)
**		Enhanced Rachael's fix to print the DI status value when ANY DI
**		call fails.
**      23-oct-1991 (jnash)
**          Add error param to dm1px calls and additional error handling.  
**          Also fix numerous addressing problems with local error codes. 
**	28-oct-1991 (bryanp)
**	    Fix fhdr/fmap initialization in dm2f_init_file.
**	7-Feb-1991 (rmuth)
**	    Changed the algorithm which works out how much space to 
**	    allocate to each file of a multi-location table when the
**	    table is initially created.
**	19-feb-1992 (bryanp)
**	    Temporary table enhancements. In particular, implemented the
**	    deferred I/O concept.
**	15-apr-1992 (bryanp)
**	    Remove the FCB argument from dm2f_rename().
**	16-apr-1992 (bryanp)
**	    Protoypes for DM2F functions.
**	18-jun-1992 (bryanp)
**	    Pass DI_ZERO_ALLOC when materializing a temp table so that
**	    the deferred allocation is made real on Unix systems.
**	29-August-1992 (rmuth)
**	    Various file extend changes
**	    - Changed to use dm1p_init_fhdr and dm1p_add_fmap. 
**	    - Changed to use dm1p_used_range, dm1p_single_fmap_free.
**	    - Moved di.h for the dm1p prototypes.
**	    - Remove DM2F_LOAD file option. The load files will now always
**	      call dm1s_deallocate to init the file.
**	22-sep-1992 (bryanp)
**	    Improve error message formatting for allocation/extend errors.
**	    Revert 18-jun-1992 change; we should no longer need DI_ZERO_ALLOC
**	    for temporary tables.
**	5-oct-1992 (bryanp)
**	    Fixed use of 'err_code' in dm2f_set_alloc()
**	26-oct-92 (rogerk)
**	    Reduced Logging Project: Added support for partially open tables.
**	    Also changed dm2f_sense_file and dm2f_alloc_file to handle cases
**	    where some files have been extended beyond the range of other
**	    files in the same table group.  DM2F will now hide those extra
**	    extended pages from the view of mainline code and treat them
**	    as though they did not exist.
**	30-October-1992 (rmuth)
**	    - Add dm2f_galloc_file, dm2f_guarantee_space and allocate_to_file.
**	    - Remove references to DI_ZERO_ALLOC.
**	    - Prototyping some DI functions showed errors.
**	05-nov-92 (jrb)
**	    Changed dm2f_filename for sort file naming.
**	14-dec-92 (jnash)
**	    Reduced Logging Project: Eliminate dm2f_init_file routine to reduce
**	    the knowledge that this module has about table structures.  The
**	    dm2f_init_file functionality is now embodied in dm1s.
**	    Removed DMPP_ACC_PLV argument from dm2f_write_file.
**	    Corrected description of arguments to dm2f_alloc,dm2f_galloc.
**	2-feb-93 (jnash)
**	    Remove & from fcb_tcb_ptr->tbio_dbname and tbio_relid 
**	    references.  These are pointers.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system typedefs
**	21-June-1993 (rmuth)
**	    Add dm2f_round_up(), used to make sure that the allocation and
**	    extend values for multi-location tables are factors of 
**	    DI_EXTENT_SIZE.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**	20-sep-1993 (andys)
**	    Fix arguments to ule_format call.
**	10-dec-1993 (rmuth)
**	    dm2f_set_alloc() : Sense returns last used pageno but alloc 
**	    requires the number of pages. Wierd but true, this showed up as 
**	    an end-of-file bug as we allocated one less page than needed and 
**	    DIwrite returned an error.
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.
**      20-Dec-1994 (cohmi01)
**          Add support for RAW (character special) files. Ch dm2f_read_file(),
**          dm2f_write_file() to locate real block and compute virtual EOF.
**          Ch dm2f_sense_file() and others to virtualize some operations.
**	    Add dm2f_rawnamdir() for operations on the raw name directory,
**	    which keeps tracks of logical 'file' names and is stored in the
**	    first portion of the raw file Added dm2f_get_rawname() for
**	    adjusting naming conventions between unix and raw files.
**	13-Feb-1995 (cohmi01)
**	    For RAW I/O, calculate & save dio->io_alloc_eof after DIopen()
**	    of raw files to prevent lengthy EOF calc by DIsense().
**	22-Feb-1995 (cohmi01)
**	    Add prototype for dm2f_rawnamdir(), fix calls to it.
**	14-sep-1995 (nick)
**	    Correct and add comments ( some of which had previously been lost
**	    over the ages ) - basically the allocate routines take the
**	    page number to allocate from ( aka the current number of pages
**	    in the file ) rather than the current last page number.  Also, they
**	    return the last page number allocated, not the first !  No actual
**	    change to any code.
**	27-dec-1995 (dougb)
**	    Porting change:  Make all references to RAW I/O specific to the
**	    supported Unix platforms.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass tbio_page_size to dm2f_open_file in dm2f_build_fcb.
**      06-mar-1996 (stial01)
**          Pass page size to dm2f_build_fcb(), which may get null tbio ptr
**          Also to dm2f_read_file(), dm2f_write_file(), materialize_fcb()
**          Pass page size to dm2f_add_fcb()
**	18-jun-1996 (jenjo02)
**	    Lock fcb_mutex when extending a file to prevent concurrency 
**	    problems (allocate_to_file()). Without the mutex protection, 
**	    FHDR/FMAPs can be corrupted and multiple servers may
**	    encounter write-past-end-of-file (DM93AF) errors in the 
**	    WriteBehind threads.
**	16-Oct-1996 (jenjo02)
**	    Create fcb_mutex semaphore name using file name to aid in iimonitor,
**	    sampler identification.
**	04-nov-1996 (canor01)
**	    Previous change passed fcb_filename to STprintf().  This is a
**	    structure; it should pass fcb_filename.name.
**	31-oct-1996 (nanpr01 for ICL phil.p)
**	    Incorporate ITBs changes for support of Gather Write. 
**	    New routines :-
**		dm2f_write_list,
**		dm2f_complete,
**		dm2f_check_list,
**		dm2f_force_list.
**	31-oct-1996 (nanpr01 for ICL itb)
**	    Copied a change from dm2f_write_file to dm2f_write_list for
**	    raw files.  These functions should be largely identical.
** 	19-jun-1997 (wonst02)
** 	    Added DM2F_READONLY flag for dm2f_build_fcb (readonly database).
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**	26-Aug-1997 (jenjo02)
**	    Added log_id (LG_LXID) to dm2f_build_fcb(), dm2f_open_file(),
**	    dm2f_create_file(), dm2f_delete_file(), dm2f_rename(),
**	    dm2f_add_fcb() dm2f_release_fcb() prototypes so that it 
**	    can be passed on to dm2t_reclaim_tcb().
**	    Added fcb_log_id to FCB in which log_id is saved for later
**	    use when FCB is materialized.
**      22-Mar-1998 (linke01)
**          rollforwarddb failuer in back end lar test.Added NO_OPTIM for 
**          pym_us5.
**          Added NO_OPTIM for pym_us5.
**	27-jul-1998 (rigka01)
**	    Cross integrate fix for bug 90691 from 1.2 to 2.0
**	    In highly concurrent environment, temporary file names may not
**	    be unique so improve algorithm for determining names 
**	01-oct-1998 (somsa01)
**	    In dm2f_write_file(), if we get DI_NODISKSPACE from DIwrite(),
**	    set the error code to E_DM9448_DM2F_NO_DISK_SPACE.
**	    (Bug #93633)
**	03-Dec-1998 (jenjo02)
**	    Added pgsize parm to dm2f_rawnamdir() prototype, which now
**	    computes fcb_rawstart, fcb_physical_allocation in units of
**	    pgsize pages to support multiple page sizes in raw files.
**      21-mar-1999 (nanpr01)
**          Supporting raw locations.
**	16-Apr-1999 (jenjo02)
**	    Additional changes for RAW locations.
**	11-May-1999 (jenjo02)
**	    Fixed RAW multi-location filesize calculation rounding error.
**	14-Jun-1999 (jenjo02)
**	    More corrections to RAW multi-locations, ifdef'd out GatherWrite
**	    code until it's ready.
**      15-jun-1999 (hweho01)
**          Corrected argument mismatch in ule_format() call.
**	29-Jun-1999 (jenjo02)
**	    Un-ifdef'd GatherWrite, which is now implemented.
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Jan-2001 (jenjo02)
**	    Get *DML_SCB using macro instead of SCU_INFORMATION.
**	12-Mar-2001 (jenjo02)
**	    Productized RAW file support.
**	23-Apr-2001 (jenjo02)
**	    Cleaned up end of RAW location detection, added
**	    E_DM0194_BAD_RAW_PAGE_NUMBER message detailing
**	    the transgression.
**	03-dec-2001 (somsa01)
**	    In dm2f_sense_raw(), make sure we EOS terminate raw_buf before
**	    using it in CVal().
**      20-sept-2004 (huazh01)
**          Remove the fix for b90691 and b92261. The 
**          fix for b90691 has been rewritten. 
**          INGSRV2643, b111502. 
**	6-Feb-2004 (schka24)
**	    Change around temp filename generator scheme.
**      16-nov-2008 (stial01)
**          Redefined name constants without trailing blanks.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**	01-Dec-2008 (jonj)
**	    SIR 120874: dm2f_? functions converted to DB_ERROR *,
**	    use new form uleFormat, macroize log_di_error.
**	10-Dec-2008 (jonj)
**	    SIR 120874: Remove last vestiges of CL_SYS_ERR,
**	    old form uleFormat.
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**      21-Jan-2009 (horda03) Bug 121519
**          Extend filename generator to handle Table ID up to
**          0x4643C824.
**	24-May-2009 (kiria01) b122051
**	    Limit filename length in sem formatting to avoid overrun.
**/

/*
**  Forward function definitions.
*/

static	    VOID    logDiErrorFcn(STATUS s, i4 error, CL_ERR_DESC *sys_err,
				DM_OBJECT *cb, i4 l_path, char *path,
				i4 l_file, char *file, i4 page,
				PTR FileName, i4 LineNumber);
#define	log_di_error(s,error,sys_err,cb,l_path,path,l_file,file,page) \
	logDiErrorFcn(s,error,sys_err,cb,l_path,path,l_file,file,page,\
			__FILE__, __LINE__)

static DB_STATUS    materialize_fcb(DMP_LOCATION *loc, i4 l_count,
				i4 page_size,
				DB_TAB_NAME *tab_name, DB_DB_NAME *db_name,
				DB_ERROR *dberr);
static VOID 	calculate_allocation(
			DMP_LOCATION	    *loc,
			i4		    l_count,
			i4		    *total_pages);

static DB_STATUS allocate_to_file(
			i4		    flag,
			DMP_LOCATION        *loc,
			i4             l_count,
			DB_TAB_NAME         *tab_name,
			DB_DB_NAME          *db_name,
			i4		    count,
			i4             *page,
			DB_ERROR	    *dberr);

VOID dm2f_complete(
			DMP_CLOSURE     *clo,
			STATUS          di_err,
			CL_ERR_DESC      *sys_err);



/*
** Constants used when allocating space to a file
*/
#define DM2F_NOT_GUARANTEE_SPACE	0x01
#define	DM2F_GUARANTEE_SPACE		0x02

/*{
** Name: dm2f_build_fcb	- Build an FCB.
**
** Description:
**      Given a TCB, allocate a FCB, construct
**	the file name, initialize the FCB and open the file
**      if requested to do so.
**      The dm2f_build_fcb does not allocate any pages
**	for a load file used for sort data.  This is done
**	by the routines which build the table from sorted
**	data.
**      This routine tries to reclaim memory and open file
**	quota when resources are short.
**
** Inputs:
**	lock_list			Lock list of current transaction.
**	log_id				Log txn id of current transaction.
**      loc                             The LOC array of files to open.
**      l_count                         Count of the number of locations.
**      page_size                       Page size
**      flag				Flag indicating action required.
**					DM2F_OPEN	Open the table
**					DM2F_TEMP	Temporary file
**					DM2F_LOAD	(obsolete)
**					DM2F_ALLOC	Only alloc fcb; not open
**					DM2F_DEFERRED_IO - Used with DM2F_TEMP
**					    to indicate that file creation and
**					    disk space allocation should be
**					    deferred.
**					DM2F_READONLY 	Used w/DM2F_OPEN for
**					    readonly database.
**      fn_flag                         File name flag used to determine
**                                      what type of file to open.
**      table_id                        Table id used to create file names.  
**	name_id				Temp name generator ID, see name_gen
**	name_gen			Temp name generator number, needed
**					for DM2F_MOD or _DES only, pass zero
**					if you don't care, some value if
**					a predictable name is needed.
**      tbio                            TABLE_IO cb that owns this FCB or zero
**                                      for temporary files used during
**                                      modify, index, etc.
**	sync_flag			set in DCB, and passed into by caller -
**					value is system specific, and is passed
**					without interpretation to DIopen().
**
** Outputs:
**      err_code                        Reason for error status.
**					Which includes one of the following
**                                      error codes:
**					E_DM0112_RESOURCE_QUOTA_EXCEED
**                                      E_DM9336_DM2F_BUILD_ERROR
**				        E_DM0114_FILE_NOT_FOUND
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
**	07-dec-1985 (derek)
**          Created new for jupiter.
**      20-nov-87 (jennifer)
**          Added multiple location support.
**      18-may-88  (teg)
**	    added external error msg when file not found during open
**	17-oct-88 (mmm)
**	    Added new parameter "sync_flag" to call.  Caller will grab this 
**	    flag straight from the DCB and pass it in.
**	16-jan-1992 (bryanp)
**	    Added support for DM2F_DEFERRED_IO flag for use with temp tables.
**	29-August-1992 (rmuth)
**	    Remove DM2F_LOAD file option. The load files will now always
**	    call dm1s_deallocate to init the file.
**	26-oct-92 (rogerk)
**	    Reduced Logging Project: Added support for partially open tables.
**	    Only open FCB's for those locations which are marked VALID (although
**	    the FCB's are still allocated for non-valid locations).
**	    Added call to dm2f_close_file in error handling since the 
**	    dm2f_open_file routine may now return with some files open on
**	    an error condition.  Changed routine to accept table io pointer
**	    rather than a tcb pointer.
**	14-dec-92 (rogerk)
**	    Took out check for callers which passed a TCB instead of a TBIO
**	    pointer since the server code is now all up-to-date.
**	17-dec-92 (rogerk & jnash)
**	    Initialize fcb's physical allocation field.
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.
**	27-dec-1995 (dougb)
**	    Porting change:  Make all references to RAW I/O specific to the
**	    supported Unix platforms.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass tbio_page_size to dm2f_open_file in dm2f_build_fcb.
** 	19-jun-1997 (wonst02)
** 	    Added DM2F_READONLY flag for dm2f_build_fcb (readonly database).
**	23-Apr-2001 (jenjo02)
**	    Set each RAW location's FCB rawpages to that of the 
**	    smallest raw location; this sets the upper bound to
**	    which the raw file can grow.
**	6-Feb-2004 (schka24)
**	    Pass thru name-generator id.
**	6-Nov-2009 (kschendel) SIR 122757
**	    Pass direct IO request to DI if appropriate.
*/
DB_STATUS
dm2f_build_fcb(
i4		lock_list,
i4		log_id,
DMP_LOCATION	*loc,
i4		l_count,
i4		page_size,
i4		flag,
i4		fn_flag,
DB_TAB_ID	*table_id,
i2		name_id,
i4		name_gen,
DMP_TABLE_IO	*tbio,
u_i4		sync_flag,
DB_ERROR	*dberr)
{
    i4		i;
    i4		n;
    DMP_FCB	*f;
    DM_PAGENO	page;
    DB_STATUS	local_status, status;
    bool	reclaimed;
    CL_ERR_DESC		sys_err;
    DMP_LOCATION	*l;
    i4		k;
    i4		local_error;
    char	sem_name[CS_SEM_NAME_LEN];
    i4		smallest_raw_location = MAXI4;
    i4		raw_locations = 0;
    DB_ERROR	local_dberr;
    i4		*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    for (k=0; k < l_count; k++)
    {
	l = &loc[k];

	reclaimed = FALSE;
	for (;;)
	{
	    /*
	    ** Allocate an FCB.  If no memory, then try to free up an unused
	    ** tcb to get more memory.  If still fail, then log that we are
	    ** out of memory and return RESOURCE error.
	    */
	    status = dm0m_allocate((i4)(sizeof(DMP_FCB) + sizeof(DI_IO)), 
		(i4)0, (i4)FCB_CB, (i4)FCB_ASCII_ID,
		(char *)tbio, (DM_OBJECT **)&f, dberr);
	    if (status == E_DB_OK)
	    {
		break;
	    }
	    else
	    {
		if ((dberr->err_code == E_DM923D_DM0M_NOMORE) && (reclaimed == FALSE))
		{
		    reclaimed = TRUE;
		    status = dm2t_reclaim_tcb(lock_list, log_id, dberr);
		}
		if (status != E_DB_OK)
		{
		    if ((dberr->err_code == E_DM9328_DM2T_NOFREE_TCB) ||
			(dberr->err_code == E_DM923D_DM0M_NOMORE))
		    {
			uleFormat(NULL, E_DM923D_DM0M_NOMORE, NULL, ULE_LOG, NULL,
			    NULL, 0, NULL, err_code, 0);
			SETDBERR(dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
		    }
		    break;
		}
	    }
	}
	if (status != E_DB_OK)
	    break;

	/*
	** Initialize the FCB.
	**
	** If this entry in the location array specifies a valid file, then
	** record the file information and mark the file CLOSED.
	**
	** If this location entry is not initialized, then we mark the
	** FCB NOTINIT to signify that it contains no open file and no
	** valid file information.  Before this location/file can be used,
	** it will need to be initialized later with a dm2f_add_file call.
	** The subsequent dm2f_open_file call will skip over non-initialized
	** fcb's.
	*/
	l->loc_fcb = f;
	f->fcb_di_ptr = (struct _DI_IO*) ((char *)f + sizeof(*f));
	f->fcb_tcb_ptr = tbio;
	f->fcb_locklist = lock_list;
	f->fcb_log_id = log_id;

	f->fcb_state = FCB_NOTINIT;
	if (l->loc_status & LOC_VALID)
	{
	    f->fcb_state = FCB_CLOSED;
	    f->fcb_namelength = 12;
	    f->fcb_last_page = -1;
	    f->fcb_deferred_allocation = 0;
	    f->fcb_physical_allocation = 0;
	    f->fcb_location = l->loc_ext;
	    dm2f_filename(fn_flag, table_id, l->loc_id, name_id, name_gen, 
		&f->fcb_filename);
	    dm0s_minit(&f->fcb_mutex,
			STprintf(sem_name, "FCB %.*s", f->fcb_namelength, 
			         f->fcb_filename.name));
	    /* Extract RAW location file start and number of pages */
	    if (l->loc_status & LOC_RAW)
	    {
		raw_locations++;

		/* Convert rawstart,rawpages from RAW units to page-size units */
		f->fcb_rawstart = (l->loc_ext->raw_start * DM2F_RAW_KBLKSIZE)
				  / (page_size >> 10);
		f->fcb_rawpages = (l->loc_ext->raw_blocks * DM2F_RAW_KBLKSIZE)
				  / (page_size >> 10);

		/* Keep track of the smallest raw location */
		if ( f->fcb_rawpages < smallest_raw_location )
		    smallest_raw_location = f->fcb_rawpages;
	    }
	}
	else
	    dm0s_minit(&f->fcb_mutex, "FCB mutex");
    }

    /*
    ** If an error occurred allocating memory for the fcb array, then
    ** release all of the allocated FCB's.
    */
    if (status != E_DB_OK)
    {
	for (i=0; i<k; i++)
	{
	    l = &loc[i];
	    dm0m_deallocate((DM_OBJECT **)&l->loc_fcb);
	}

	if (dberr->err_code > E_DM_INTERNAL)
	{
	    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM9336_DM2F_BUILD_ERROR);
	}
	return (E_DB_ERROR);
    }

    /* Bind all raw locations to the smallest raw location */
    if ( raw_locations )
    {
	for ( k = 0; k < l_count; k++ )
	{
	    l = &loc[k];
	    if ( (l->loc_status & (LOC_VALID | LOC_RAW)) == (LOC_VALID | LOC_RAW) )
		l->loc_fcb->fcb_rawpages = smallest_raw_location;
	}
    }


    /*
    ** If the request was to only allocate FCB's but not open them, then
    ** we can return now.
    */
    if ( flag & DM2F_ALLOC )
	return (E_DB_OK);


    do
    {
	/* Now open the file for normal tables, not temporary. */
	if (flag & DM2F_OPEN)
	{
	    /* Ask for direct I/O if config says so.
	    ** If the caller is asking for any sort of sync, assume it's
	    ** a (regular) table file and go by directio-tables.
	    ** If the caller is not asking for sync, assume that it's
	    ** a pseudo-temporary table, or something weird like a load
	    ** temporary, and go by the load_optim setting.
	    */
	    if ((sync_flag & (DI_USER_SYNC_MASK | DI_SYNC_MASK)
		 && dmf_svcb->svcb_directio_tables)
	      || ((sync_flag & (DI_USER_SYNC_MASK | DI_SYNC_MASK)) == 0
		  && dmf_svcb->svcb_directio_load) )
		sync_flag |= DI_DIRECTIO_MASK;
	    status = dm2f_open_file(lock_list, log_id, loc, l_count, 
		    (i4)page_size, 
		    (flag & DM2F_READONLY) ? DM2F_A_READ : DM2F_A_WRITE, 
		    sync_flag, (DM_OBJECT *)f, dberr);
	    if (status == E_DB_OK)
		return (E_DB_OK);

	    break;
	}

	/* If get here and not TEMP file, then error */
	if ((flag & DM2F_TEMP) == 0)
	{
	    SETDBERR(dberr, 0, E_DM9336_DM2F_BUILD_ERROR);
	    break;
	}
	/* Flag all locn's as TEMP so that we can do nice things such as
	** avoiding truncate-before delete, maybe other stuff too.
	*/
	for ( k = 0; k < l_count; k++ )
	{
	    loc[k].loc_fcb->fcb_state |= FCB_TEMP;
	}

	/* Handle processing of temporary tables. */
    
	if ((flag & DM2F_DEFERRED_IO) != 0)
	{
	    loc[0].loc_fcb->fcb_state |= FCB_DEFERRED_IO;
	}
	else
	{
	    i4 di_flags = 0;

	    /* No sync for temp-table files, but if config wants directio
	    ** for load files, it's probably good for temp tables too.
	    ** Unfortunately can't use PRIVATE, gtt is private to session,
	    ** but not necessarily private to thread.
	    */
	    if (dmf_svcb->svcb_directio_load)
		di_flags |= DI_DIRECTIO_MASK;
	    status = dm2f_create_file(lock_list, log_id, loc, l_count, 
					page_size, di_flags,
					(DM_OBJECT *)f, dberr);
	    if (status != E_DB_OK)
		break;
	}


	return (E_DB_OK);
    } while (0);

    /*
    ** Handle error recovery.
    ** Close any files that were successfully opened and then release the
    ** allocated fcb's.
    */

    local_status = dm2f_close_file(loc, l_count, (DM_OBJECT *)f, &local_dberr);
    if (local_status != E_DB_OK)
    {
        uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
    }

    for (k=0; k<l_count; k++)
    {
	l = &loc[k];
	dm0s_mrelease(&l->loc_fcb->fcb_mutex);
	dm0m_deallocate((DM_OBJECT **)&l->loc_fcb);
    }

    /* If file is not found, then make sure this is propagated up to PSF
    ** so warning/error can be output to user -- teg (for bug #2344)
    */
    if (dberr->err_code == E_DM9291_DM2F_FILENOTFOUND) 
        dberr->err_code = E_DM0114_FILE_NOT_FOUND;

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9336_DM2F_BUILD_ERROR);
    }

    return (E_DB_ERROR);
}

/*{
** Name: dm2f_release_fcb	- Release an FCB from a TCB.
**
** Description:
**      This function will release  FCBs from a location array.  
**      In the process
**	of releasing a FCB, the file will be closed and the FCB
**	deallocated.
**
** Inputs:
**	lock_list			Lock list of current transaction.
**      loc                             The location list to be released.
**      l_count                         The count of locations.
**      flags                           Flags indicating close, delete.
**
** Outputs:
**      err_code                        The reason for an error status.
**					Which includes one of the following
**                                      error codes:
**					E_DM0112_RESOURCE_QUOTA_EXCEED
**					E_DM9340_DM2F_RELEASE_ERROR
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
**	07-dec-1985 (derek)
**          Created new for jupiter.
**      20-nov-87 (jennifer)
**          Added multiple location support.
*/
DB_STATUS
dm2f_release_fcb(
i4		    lock_list,
i4		    log_id,
DMP_LOCATION	    *loc,
i4             l_count,
i4             flag,
DB_ERROR	    *dberr)
{
    DB_STATUS		    status;
    i4                 k;
    DMP_LOCATION            *l;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);


    status = E_DB_OK;
    if ((flag & DM2F_ALLOC) == 0)
    {
	status = dm2f_close_file(loc, l_count, 0, dberr);
    }
    if (status == E_DB_OK && (flag & DM2F_TEMP))
    {
	status = dm2f_delete_file(lock_list, log_id, loc, l_count, (i4)0,
		(i4)0, dberr);
    }

    for (k=0; k < l_count ; k++)
    {
	l = &loc[k];
	dm0s_mrelease(&l->loc_fcb->fcb_mutex);
	dm0m_deallocate((DM_OBJECT **)&l->loc_fcb);
    }

    if (status == OK)
	return (E_DB_OK);

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9340_DM2F_RELEASE_ERROR);
    }
    return (E_DB_ERROR);
}

/*{
** Name: dm2f_filename	- Construct a filename from a table id.
**
** Description:
**      This routine constructs the file name for a table given the tables
**	table identifer.
**
**	Naming conventions are currently:
**
**	Tables:	    xxxxxxxx.Tll	x's encode table id (4 bits per char)
**					ll is location number
**					"table id" is index id if nonzero,
**					else table id;  no partition bit.
**		    xxxxxxxx.Xll	for TempTable indexes, as above, but
**					the X removes ambiguity with regular
**					tables.
**					
**	All others depend on what's executing: a regular server,
**	rollforwarddb, or something else (other jsp functions, acp).
**
**	For regular servers and rollforwarddb:
**		rsssssss.cll		rsssssss is name-generator, see below
**					ll is location number
**					c via DM2F_xxx file:
**					   D - (DES) destroy table temp file
**					   M - (MOD) modify/index temp file
**					   I - (SRI) sort input merge file
**					   O - (SRO) sort output merge file
**
**	Name-generator is name_gen * 512 + svcb_tfname_id, expressed
**	in radix-37 encoded as a-z 0-9 _.  The first letter only gets a
**	range of 0-18 and is encoded as r-z 0-9.  (This prevents collisions
**	with other forms.)  If the passed-in name_gen parameter is zero
**	we'll generate a number via svcb_tfname_gen.  The caller is
**	responsible for generating its own name_gen in any situation where
**	the resulting filename must be predictable (this means index
**	and modify).
**
**	For fastload, any other non-rollforward JSP / ACP:
**		qssttttt.cll		q is fixed*
**					ssttttt is <name generator>*2^30
**					plus table id, all radix 37
**					cll is as above
**
**           * - to support new range of table id's, this first character
**               will be b..p. Thus the file names will not clash with
**               any other generated file. 
**
**	For the fastload et al case the ss name-generator can range from
**	1 to 350, again with zero meaning make one from svcb_tfname_gen.
**	Although it's highly unlikely that any such program will want
**	more than one round of temporary file.  These situations use
**	a different scheme because a) they are typically specific to a
**	table, which gets locked;  and/or b) they are run by users and
**	we don't want to cause problems in the tfname-id space.
**
**	This particular setup assumes the current 3FFFFFF limit on table
**	ID numbers.  It would not be hard to grab a few more bits of
**	table ID by sharing with the ss part, there's no real need for
**	1368 temp files (the old limit was 256 anyway).
**
**	Special Cases:
**	DM2F_SM1:	a0tttttt.sm1	tttttt is table id 5 bits per char
**                                      For extended table ids, 0 can be 1, 2 or 3.
**	DM2F_OVF:	a0tttttt.ovf	not used any more
**
** Inputs:
**      flag                            Type of file name to return.
**                                      DM2F_TAB for normal file associated
**                                      with the table.  DM2F_DES for 
**                                      a filename for renaming the file
**                                      for a delete operation. DM2F_MOD
**                                      for temporary files during a 
**                                      modify or index command.  DM2F_SM1
**                                      and DM2F_SM2 for temporary files
**                                      during a SYSMOD of core catalogs.
**      table_id                        Pointer to the table id.
**      loc_id                          Id associated with this location 
**                                      for tables with multiple locations.
**	name_id				Name generator ID for DM2F_MOD and
**					DM2F_DES only, ignored for others.
**					If name_gen is zero this is ignored,
**					else it's used along with name_gen
**					to generate a caller required name.
**      name_gen			Name generator sequence for
**					DM2F_MOD and DM2F_DES only.  Ignored
**					for others.  0 = generate another
**					name sequence from svcb.
**
** Outputs:
**      filename                        Pointer to the filename.
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      28-jan-1987 (Derek)
**	    Created for Jupiter.
**      20-nov-87 (jennifer)
**          Added multiple location support.
**	08-sep-88 (sandyh)
**	    Added support for sort filenames.
**	06-nov-92 (jrb)
**	    Added support for new sort-file naming rules.  Sort files end with
**	    either oxx or ixx (where xx is a hexidecimal number).
**	27-jul-1998 (rigka01)
**	    Cross integrate fix for bug 90691 from 1.2 to 2.0
**	    In highly concurrent environment, temporary file names may not
**	    be unique so improve algorithm for determining names 
**	 3-aug-98 (hayke02)
**	    Modify the previous change so that modify temp files are named
**	    using the stmt_counter (as before), and not the new temp table
**	    counter. This change fixes bug 92261.
**      20-sept-2004 (huazh01)
**          Remove the fix for b90691 and b92261. The fix for b90691
**          has been rewritten. 
**          INGSRV2643, b111502. 
**	6-Feb-2004 (schka24)
**	    Turn off partition indicator bit before generating a name
**	    for a table.  Revise temp table generation scheme, again,
**	    to get rid of the old DMU statement count limit which was
**	    a problem for partitioned tables.
**	25-Jul-2005 (schka24)
**	    Fix to only turn off partition sign bit from index-id, keep
**	    signed (negative) temp table id.  Fix bad x-integration.
**	14-Aug-2006 (jonj)
**	    Really, turn off just the partition sign bit. 
**	    Temp Table Indexes have a file type starting with ".x"
**	    to distinguish them from ambiguous ".t" file names.
*/
VOID
dm2f_filename(
	i4		flag,
	DB_TAB_ID	*table_id,
	u_i4		loc_id,
	i2		name_id,
	i4		name_gen,
	DM_FILE		*filename)
{
    char	    fname[8];
    DM_SVCB         *svcb = dmf_svcb;
    i4		    i;
    i4		    toss_err;
    i8		    gen;
    u_i4	    unumber;

    static const char    hex_alpha[16] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g',
					      'h', 'i', 'j', 'k', 'l', 'm', 'n',
					      'o', 'p' };

    static const char rad37_alpha[37] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g',
					     'h', 'i', 'j', 'k', 'l', 'm', 'n',
					     'o', 'p', 'q', 'r', 's', 't', 'u',
					     'v', 'w', 'x', 'y', 'z', '0', '1',
					     '2', '3', '4', '5', '6', '7', '8',
					     '9', '_' };

    static const char rad37_fc[19] = { 'r', 's', 't', 'u', 'v',
					  'w', 'x', 'y', 'z', '0',
					  '1', '2', '3', '4', '5',
					  '6', '7', '8', '9' };

    static const char    hex_num[16] = { '0', '1', '2', '3',
					    '4', '5', '6', '7',
					    '8', '9', 'a', 'b',
					    'c', 'd', 'e', 'f' };

    static const char    spec_alpha[32] = {'a', 'b', 'c', 'd', 'e', 'f', 'g',
					      'h', 'i', 'j', 'k', 'l', 'm', 'n',
					      'o', 'p', 'q', 'r', 's', 't', 'u',
                                              'v', 'w', 'x', 'y', 'z', '0', '1',
                                              '2', '3', '4', '5'};

    static const char tempfile_fc[17] = { 'b', 'c', 'd', 'e', 'f',
                                             'g', 'h', 'i', 'j', 'k',
                                             'l', 'm', 'n', 'o', 'p', '_' };

    if ((unumber = table_id->db_tab_index) != 0)
	unumber &= ~DB_TAB_PARTITION_MASK;	/* Clear partition indicator bit */
    else
	unumber = table_id->db_tab_base;

    if (flag == DM2F_TAB)
    {
	filename->name[0] = hex_alpha[(unumber >> 28) & 0x0f];
	filename->name[1] = hex_alpha[(unumber >> 24) & 0x0f];
	filename->name[2] = hex_alpha[(unumber >> 20) & 0x0f];
	filename->name[3] = hex_alpha[(unumber >> 16) & 0x0f];
	filename->name[4] = hex_alpha[(unumber >> 12) & 0x0f];
	filename->name[5] = hex_alpha[(unumber >> 8) & 0x0f];
	filename->name[6] = hex_alpha[(unumber >> 4) & 0x0f];
	filename->name[7] = hex_alpha[unumber & 0x0f];
	filename->name[8] = '.';
	/* Temp Table indexes are suffixed with ".xnn" */
	if ( table_id->db_tab_base < 0 && table_id->db_tab_index > 0 )
	    filename->name[9] = 'x';
	else
	    filename->name[9] = 't';
	filename->name[10] = hex_num[(loc_id >> 4) & 0x0f];
	filename->name[11] = hex_num[loc_id & 0x0f];
    }
    else if (flag == DM2F_SM1)
    {
	filename->name[0] = 'a';
	filename->name[1] = hex_num[(unumber >> 30) & 0x03]; /* 0, 1, 2 or 3 */
	filename->name[2] = spec_alpha[(unumber >> 25) & 0x01f];
	filename->name[3] = spec_alpha[(unumber >> 20) & 0x01f];
	filename->name[4] = spec_alpha[(unumber >> 15) & 0x01f];
	filename->name[5] = spec_alpha[(unumber >> 10) & 0x01f];
	filename->name[6] = spec_alpha[(unumber >> 5)  & 0x01f];
	filename->name[7] = spec_alpha[(unumber >> 0)  & 0x01f];
	MEcopy(".sm1", 4, &filename->name[8]);
    }
    else
    {
	if (name_gen == 0)
	{
	    name_id = svcb->svcb_tfname_id;
	    name_gen = CSadjust_counter((i4 *) &svcb->svcb_tfname_gen, 1);
	}
	if (svcb->svcb_tfname_id != -1)
	{
	    /* Normal serverish case, use rad37 generator */
	    /* The "id" part keeps different servers and rollforwarddb
	    ** separate so that they can't generate name collisions.
	    ** The id for rollforwarddb is something predictable, ie 0,
	    ** since the id part isn't logged for MODIFY or INDEX.
	    ** A name-gen of up to MAXI4 should last for many years.
	    ** The actual maximum is 3522862626 (0xd1fa9e22).
	    */
	    unumber = (u_i4) name_gen;
	    if (unumber > (u_i4) 0xd1fa9e21)
	    {
		/* No error status from here, no safe way to go on.
		** FIXME if this ever actually happens (unlikely?), one
		** possibility would be to hold the current ID lock and
		** get another ID.  There are probably lots to spare.
		*/
		uleFormat(NULL, E_DM92A3_NAME_GEN_OVF, NULL, ULE_LOG,
			NULL, NULL, 0, NULL, &toss_err, 0);
		EXsignal(EX_DMF_FATAL, 0);
	    }
	    gen = (i8) unumber;		/* No sign-extend */
	    gen = gen * 512 + name_id;
	    for (i = 7; i > 0; --i)
	    {
		filename->name[i] = rad37_alpha[ gen % 37 ];
		gen /= 37;
	    }
	    filename->name[0] = rad37_fc[gen];
	}
	else
	{
	    /* fastload, probably */

            i4 use_new_algorithm;

            /* New filename algorithm added to support Table ID's in the
            ** range 0x4221AD5 to 0x4643C824. Lower Table IDs will generate the
            ** same filename as they did before this change.
            **
            ** For this range the table_id will spill over into the lead
            ** character position. This will be achieved maintaining a unique
            ** filename. (See tempfile_fc).
            ** 
            ** This will distinguish it from all other table names;
            **
            **    DM2F_TAB only uses hex_alpha characters, which doesn't include integers.
            ** 
            **    DM2F_SM1 starts with an 'a' and then has a numeric (0, 1, 2 or 3);
            ** 
            **    Generated name starts with rad37_fc ('r'..'z', '0'..'9')
            **
            **    For table id in previous range. the file will start with 'q', the
            **    remainder of the filename is unchanged for before.
            **    New range ids will begin with the tempfile_fc character ('b'..'p' or '_')
            **    the next character will be a rad37_fc (this has the side effect of reducing
            **    the ss part to 703) the rest follow the original algorithm.
            **
            ** Note; As far as I am aware, there is another valid character for filenames
            **       common to NT, Unix and VMS, and that is the "-". If that was thrown
            **       into the mix, then the "q" filename handle 0x04B90860 table IDs, and this 
            **       new algorithm would extend the max table ID to 0x50498E60.
            **       However, as this changes the filenames generated for existing table ids
            **       I don't want to do that now. Leave it for some brave soul in the future.
            */
#define DM2F_USE_NEW_ALGORITHM (u_i4) 0x4221AD4

            if (unumber > DM2F_USE_NEW_ALGORITHM)
            {
                use_new_algorithm = 1;

                unumber -= DM2F_USE_NEW_ALGORITHM + 1;
            }
            else
               use_new_algorithm = 0;

	    for (i = 7; i >= 3; --i)
	    {
		filename->name[i] = rad37_alpha[ unumber % 37 ];
		unumber /= 37;
	    }

            if (use_new_algorithm)
            {
               filename->name[0] = tempfile_fc [ unumber ];
               filename->name[1] = rad37_fc [(name_gen / 37) % 19];
            }
            else
            {
	       filename->name[0] = 'q';
	       filename->name[1] = rad37_alpha[(name_gen / 37) % 37];
            }

	    filename->name[2] = rad37_alpha[name_gen % 37];
	}
	filename->name[8] = '.';
	if (flag == DM2F_DES)
	    filename->name[9] = 'd';
	if (flag == DM2F_MOD)
	    filename->name[9] = 'm';
	if (flag == DM2F_SRI)
	    filename->name[9] = 'i';
	if (flag == DM2F_SRO)
	    filename->name[9] = 'o';
	filename->name[10] = hex_num[(loc_id >> 4) & 0x0f];
	filename->name[11] = hex_num[loc_id & 0x0f];
    }
}

/*{
** Name: dm2f_open_file	- Open a database file.
**
** Description:
**      This routine opens all the database files associated with
**      the locations specified by the location array.  
**      Error handling includes automatic
**	file reclaimation.
**
** Inputs:
**	lock_list			Lock list of current transaction.
**	log_id				Log txn id of current transaction.
**      loc                             Pointer to a location array.
**      l_count                         Count of number of locations.
**	blk_size			Block size.
**	flag				DM2F_A_READ  - open for read/write
**					DM2F_A_WRITE
**					DM2F_E_READ  - handle file not found
**					DM2F_E_WRITE   as a caller error. 
**	sync_flag			set in DCB, and passed into by caller -
**					value is system specific, and is passed
**					without interpretation to DIopen().
**	cb				Control block to use for error info.
**
** Outputs:
**      err_code                        Reason for error return status.
**					Which includes one of the following:
**					E_DM0112_RESOURCE_QUOTA_EXCEED
**                                      E_DM923F_DM2F_OPEN_ERROR
**					E_DM9291_DM2F_FILENOTFOUND
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
**      11-may-1987 (Derek)
**          Created for Jupiter.
**      20-nov-87 (jennifer)
**          Added multiple location support.
**	24-jul-88 (mikem)
**	    Added argument to DIopen, to accept database specific control over
**	    methods of disk I/O syncing. 
**	17-oct-88 (mmm)
**	    Added new sync_flag to call.  Caller will grab this flag straight
**	    from the DCB and pass it in.
**	16-jan-1992 (bryanp)
**	    Issue error message if fcb is in deferred I/O state.
**	27-sep-1992 (bryanp)
**	    fcb_state |= FCB_OPEN to preserve FCB_DEFERRED_IO flags if they're
**	    on.
**	26-oct-92 (rogerk)
**	    Reduced Logging Project: Added support for partially open tables.
**	    Changed the routine to skip over fcb's which are marked FCB_NOTINIT.
**	    These represent locations which should not be opened in this
**	    instance of the table control block.  Also changed the error
**	    handling of this routine to not close all open files if any open
**	    file request fails.  It is now the responsibility of the caller
**	    to call dm2f_close_file before deallocating the fcb's.
**	    Also changed to set LOC_OPEN state in the location array for 
**	    successfully opened locations.
**	13-Feb-1995 (cohmi01)
**	    For RAW I/O, calculate & save dio->io_alloc_eof after DIopen()
**	    of raw files to prevent lengthy EOF calc by DIsense().
**	27-dec-1995 (dougb)
**	    Porting change:  Make all references to RAW I/O specific to the
**	    supported Unix platforms.
**	14-Oct-2005 (jenjo02)
**	    Add DI_DIRECTIO_MASK hint to open regular tables with
**	    DIRECTIO when available.
**	6-Nov-2009 (kschendel) SIR 122739
**	    Let caller decide about direct IO.
*/
DB_STATUS
dm2f_open_file(
i4		lock_list,
i4		log_id,
DMP_LOCATION	*loc,
i4		l_count,
i4		blk_size,
i4		flag,
u_i4		sync_flag,
DM_OBJECT	*cb,
DB_ERROR	*dberr)
{
    i4             di_flags;
    DMP_LOCATION        *l;
    i4             k;
    CL_ERR_DESC		sys_err;
    DB_STATUS		status;
    STATUS		s;
    i4		local_error;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    di_flags = DI_IO_READ;
    if (flag == DM2F_A_WRITE || flag == DM2F_E_WRITE)
	di_flags = DI_IO_WRITE;
    for (k = 0; k< l_count; k++)
    {
	for (;;)
	{
	    s = OK;
	    l = &loc[k];	
	    
	    /*
	    ** Skip over this file if the location has not been initialized.
	    ** If any files are skipped then this will be a PARTIALLY open
	    ** table.
	    */
	    if (l->loc_fcb->fcb_state & FCB_NOTINIT)
		break;

	    if ((l->loc_fcb->fcb_state & (FCB_DEFERRED_IO | FCB_MATERIALIZING))
					== FCB_DEFERRED_IO)
	    {
		s = FAIL;
		SETDBERR(dberr, 0, E_DM923F_DM2F_OPEN_ERROR);
		break;
	    }

	    /* if RAW location, special handling for naming and FCB info */
	    if (l->loc_status & LOC_RAW)
	    {
                /* open the RAW file, use normal 'table' options */
	        s = DIopen(l->loc_fcb->fcb_di_ptr,
		       l->loc_ext->physical.name,
		       l->loc_ext->phys_length, 
                       DM2F_RAW_LINKNAME, sizeof(DM2F_RAW_LINKNAME),
 		       blk_size, di_flags, sync_flag,
 		       &sys_err);

		/*
		** fcb_rawstart and rawpages were established when the FCB
		** was built. Set io_alloc_eof to the absolute end of
		** the raw file.
		*/
		l->loc_fcb->fcb_di_ptr->io_alloc_eof =
			l->loc_fcb->fcb_rawstart + l->loc_fcb->fcb_rawpages;
	    }
	    else
	    {
		/* If page size is less than direct-IO alignment requirement,
		** force NO direct-io even if caller asked for it.  (because
		** some pages in an array of page buffers would be misaligned.)
		*/
		if (di_flags & DI_DIRECTIO_MASK
		  && blk_size < dmf_svcb->svcb_directio_align)
		    di_flags &= ~DI_DIRECTIO_MASK;
	        s = DIopen(l->loc_fcb->fcb_di_ptr, l->loc_ext->physical.name, 
		       l->loc_ext->phys_length, 
                       l->loc_fcb->fcb_filename.name, 
		       l->loc_fcb->fcb_namelength,
 		       blk_size, di_flags,
		       sync_flag,
 		       &sys_err);
	    }

	    if (s == OK)
	    {
		l->loc_fcb->fcb_state |= FCB_OPEN;
		l->loc_status |= LOC_OPEN;
		break;
	    }
	    if (s == DI_EXCEED_LIMIT)
	    {
		status = dm2t_reclaim_tcb(lock_list, log_id, dberr);

		if (status == E_DB_OK)
		    continue;

		if (dberr->err_code != E_DM9328_DM2T_NOFREE_TCB)
		{
		    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
			(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
		}
		break;
	    }
	    else
		break;
	}
	if (s != OK)
	    break;

    }
    if (s== OK)
	return (E_DB_OK);
	
    /*	Handle errors. */

    /* Log error unless file-not-found and user asked not to log it */
    if ((s != DI_FILENOTFOUND) || (flag <= DM2F_A_WRITE))
    {
	log_di_error(s, E_DM9004_BAD_FILE_OPEN, &sys_err, cb,
	    l->loc_ext->phys_length, &l->loc_ext->physical.name[0], 
	    l->loc_fcb->fcb_namelength, &l->loc_fcb->fcb_filename.name[0], 
	    (i4)0);
    }

    if (s == DI_FILENOTFOUND)
	SETDBERR(dberr, 0, E_DM9291_DM2F_FILENOTFOUND);
    else if ((s == DI_EXCEED_LIMIT) && (dberr->err_code == E_DM9328_DM2T_NOFREE_TCB))
	SETDBERR(dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
    else
	SETDBERR(dberr, 0, E_DM923F_DM2F_OPEN_ERROR);
    return (E_DB_ERROR);
}

/*{
** Name: dm2f_create_file	- Create and open a database file.
**
** Description:
**      This routine creates and opens database files at the specified 
**      locations.
**      Error handling includes automatic
**	file reclaimation.
**
** Inputs:
**	lock_list			Lock list of current transaction.
**	log_id				Log txn id of current transaction.
**      loc                             Pointer to a location array.
**      l_count                         Count of number of locations.
**	blk_size			Block size.
**	sync_flag			set in DCB, and passed into by caller -
**					value is system specific, and is passed
**					without interpretation to DIopen().
**	cb				Control block to use for error info.
**
** Outputs:
**      err_code                        Reason for error return status.
**					Which includes one of the following:
**                                      E_DM9337_DM2F_CREATE_ERROR
**					E_DM0112_RESOURCE_QUOTA_EXCEED
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
**      11-may-1987 (Derek)
**          Created for Jupiter.
**      20-nov-87 (jennifer)
**          Added multiple location support.
**	24-jul-88 (mikem)
**	    Added argument to DIopen, to accept database specific control over
**	    methods of disk I/O syncing. 
**	16-jan-1992 (bryanp)
**	    Reject call if file is in deferred I/O state.
**	27-sep-1992 (bryanp)
**	    fcb_state |= FCB_OPEN to preserve FCB_DEFERRED_IO flags if they're
**	    on.
**	26-oct-92 (rogerk)
**	    Reduced Logging Project: Added support for partially open tables.
**	    Reject the create operation if this is a partially open table
**	    descriptor.  Server algorithms will not work if part of the
**	    described file set cannot be created.
**	    Also set the LOC_OPEN state when opening newly created files
**	    and initialize the fcb's allocation values.
**	18-Jan-1995 (cohmi01)
**	    For RAW I/O, dont actually create file, just update RND.
**	13-Feb-1995 (cohmi01)
**	    For RAW I/O, calculate & save dio->io_alloc_eof after DIopen()
**	    of raw files to prevent lengthy EOF calc by DIsense().
**	27-dec-1995 (dougb)
**	    Porting change:  Make all references to RAW I/O specific to the
**	    supported Unix platforms.
**	7-Mar-2007 (kschendel) SIR 122757
**	    We don't really expect file-exists, except maybe for temp tables,
**	    so if it happens do the delete without truncation.
*/
DB_STATUS
dm2f_create_file(
i4		lock_list,
i4		log_id,
DMP_LOCATION	*loc,
i4		l_count,
i4		blk_size,
u_i4		sync_flag,
DM_OBJECT	*cb,
DB_ERROR	*dberr)
{
    DB_STATUS		status;
    STATUS		s;
    CL_ERR_DESC		sys_err;
    DMP_LOCATION        *l;
    i4             k;
    i4		local_error;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);
 
    for (k=0; k< l_count; k++)
    {
	for (;;)
	{
	    s = OK;
	    l = &loc[k];	
	    
	    /*
	    ** If an unitialized FCB is encountered, or a deferred-io fcb
	    ** that should not be being created, then signal an error.
	    */
	    if ((l->loc_fcb->fcb_state & FCB_NOTINIT) ||
		((l->loc_fcb->fcb_state & (FCB_DEFERRED_IO | FCB_MATERIALIZING))
					== FCB_DEFERRED_IO))
	    {
		s = FAIL;
		SETDBERR(dberr, 0, E_DM9337_DM2F_CREATE_ERROR);
		break;
	    }

	    /* A raw file is never "created", so just open it */
	    if (l->loc_status & LOC_RAW)
	    {
                /* open the RAW file, use normal 'table' options */
	        s = DIopen(l->loc_fcb->fcb_di_ptr,
		       l->loc_ext->physical.name,
		       l->loc_ext->phys_length, 
                       DM2F_RAW_LINKNAME, sizeof(DM2F_RAW_LINKNAME),
 		       blk_size, DI_IO_WRITE, sync_flag,
 		       &sys_err);

		/*
		** fcb_rawstart and rawpages were established when the FCB
		** was built. Set io_alloc_eof to the absolute end of
		** the raw file.
		*/

		l->loc_fcb->fcb_di_ptr->io_alloc_eof =
			l->loc_fcb->fcb_rawstart + l->loc_fcb->fcb_rawpages;
	    }
	    else
	    {
	        s = DIcreate(l->loc_fcb->fcb_di_ptr, l->loc_ext->physical.name, 
		       l->loc_ext->phys_length, 
		       l->loc_fcb->fcb_filename.name,
		       l->loc_fcb->fcb_namelength,
		       blk_size, &sys_err);
	        if ((s == DI_BADDIR) || (s == DI_DIRNOTFOUND))
	        {
		    break;
	        }
	        if (s == DI_EXISTS)
	        {
		    s = DIdelete(NULL,
			l->loc_ext->physical.name,
		       l->loc_ext->phys_length, 
		       l->loc_fcb->fcb_filename.name,
		       l->loc_fcb->fcb_namelength,
                      &sys_err);
		    if (s == E_DB_OK)
  		        continue;
	        }

	        if (s == OK)
		    s = DIopen(l->loc_fcb->fcb_di_ptr, 
		       l->loc_ext->physical.name, 
		       l->loc_ext->phys_length, 
		       l->loc_fcb->fcb_filename.name, 
		       l->loc_fcb->fcb_namelength,
		       blk_size, DI_IO_WRITE, sync_flag,
		       &sys_err);
	    }   /* end - processing for regular UNIX file */

	    if (s == OK)
	    {
		l->loc_fcb->fcb_state |= FCB_OPEN;
		l->loc_status |= LOC_OPEN;
		l->loc_fcb->fcb_physical_allocation = 0;
		l->loc_fcb->fcb_logical_allocation = 0;
		break;
	    }
	    if (s == DI_EXCEED_LIMIT)
	    {
		status = dm2t_reclaim_tcb(lock_list, (i4)0, dberr);

		if (status == E_DB_OK)
		    continue;

		/*
		** If we can't proceed due to file open quota, log the 
		** reason for failure and return RESOURCE_EXCEEDED.
		*/
		if (dberr->err_code != E_DM9328_DM2T_NOFREE_TCB)
		    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, 
				&local_error, 0);
		break;
	    }
	    else
		SETDBERR(dberr, 0, E_DM9004_BAD_FILE_OPEN);

	    break;
	}
	if (s != OK)
	    break;
    }

    if (s == OK)
	return(E_DB_OK);

    /*	Handle errors. */

    log_di_error(s, E_DM9337_DM2F_CREATE_ERROR, &sys_err, cb,
	l->loc_ext->phys_length, &l->loc_ext->physical.name[0], 
	l->loc_fcb->fcb_namelength, &l->loc_fcb->fcb_filename.name[0], 
	(i4)0);

    /* Close any files that may have been openned. */
    status = dm2f_close_file(loc, l_count, cb, &local_dberr);
    
    if ((s == DI_EXCEED_LIMIT) && (dberr->err_code == E_DM9328_DM2T_NOFREE_TCB))
	SETDBERR(dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
    else if (s == DI_BADDIR)
	SETDBERR(dberr, 0, E_DM9293_DM2F_BADDIR_SPEC);
    else if (s == DI_DIRNOTFOUND)
	SETDBERR(dberr, 0, E_DM9292_DM2F_DIRNOTFOUND);
    else
	SETDBERR(dberr, 0, E_DM9004_BAD_FILE_OPEN);
    return (E_DB_ERROR);
}

/*{
** Name: dm2f_close_file	- Close a database file.
**
** Description:
**      This routine closes database files at specifed locations.
**      Error handling includes automatic
**	file reclaimation.
**
** Inputs:
**      loc                             Pointer to a location array.
**      l_count                         Count of number of locations.
**	cb				Control block to use for error info.
**
** Outputs:
**      err_code                        Reason for error return status.
**					E_DM9240_DM2F_CLOSE_ERROR
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
**      11-may-1987 (Derek)
**          Created for Jupiter.
**      20-nov-87 (jennifer)
**          Added multiple location support.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    13-dec-90 (rachael)
**		Added ule_format call in dm2f_close_file() preceeding
**		log_di_error This will print the return from the DIclose and
**		show whether the qio call was made or if the fcb had something
**		other than DI_IO_ASCII_ID in io_type.
**	    31-may-1991 (bryanp)
**		Enhanced Rachael's fix to print the DI status value when ANY DI
**		call fails.
**	26-oct-92 (rogerk)
**	    Reduced Logging Project: Added support for partially open tables.
**	    Changed to unset LOC_OPEN state in the location array for 
**	    closed locations.
*/
DB_STATUS
dm2f_close_file(
DMP_LOCATION        *loc,
i4             l_count,
DM_OBJECT	    *cb,
DB_ERROR	    *dberr)
{
    DB_STATUS		status;
    STATUS              s;
    CL_ERR_DESC		sys_err;
    DMP_LOCATION        *l;
    i4             k;
    i4		error_closing = FALSE;    

    CLRDBERR(dberr);

    for (k=0; k< l_count ; k++)
    {
	l = &loc[k];
	if (cb == 0)
	    cb = (DM_OBJECT *)l->loc_fcb;
	if (l->loc_fcb->fcb_state & FCB_OPEN)
	{
	    s = DIclose(l->loc_fcb->fcb_di_ptr, &sys_err);
	    if (s == OK)
	    {
		l->loc_fcb->fcb_state = FCB_CLOSED;
		l->loc_status &= ~LOC_OPEN;
		continue;

	    }
	    /*	Handle errors. */

	    log_di_error(s, E_DM9001_BAD_FILE_CLOSE, &sys_err, cb,
		l->loc_ext->phys_length, &l->loc_ext->physical.name[0],
		l->loc_fcb->fcb_namelength, &l->loc_fcb->fcb_filename.name[0],
		(i4)0);
	    error_closing = TRUE;
	}
    }

    if (error_closing == FALSE)
	return (E_DB_OK);
    SETDBERR(dberr, 0, E_DM9240_DM2F_CLOSE_ERROR);
    return (E_DB_ERROR);
}

/*{
** Name: dm2f_delete_file	- Delete a database file.
**
** Description:
**      This routine delete a database file.  Error handling includes automatic
**	file reclaimation.
**
** Inputs:
**	lock_list			Lock list of current transaction.
**	log_id				Log txn id of current transaction.
**      loc                             Pointer to a location array.
**      l_count                         Count of number of locations.
**	cb				Control block to use for error info.
**	flag				If set, don't error if file not found.
**
** Outputs:
**      err_code                        Reason for error return status.
**					Which includes one of the following:
**					E_DM0112_RESOURCE_QUOTA_EXCEED
**					E_DM9290_DM2F_DELETE_ERROR
**					E_DM9291_DM2F_FILENOTFOUND
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
**      11-may-1987 (Derek)
**          Created for Jupiter.
**      20-nov-87 (jennifer)
**          Added multiple location support.
**	16-jan-1992 (bryanp)
**	    If the file is in deferred I/O state, no actual file exists, and so
**	    no deletion needs to be done.
**	26-oct-92 (rogerk)
**	    Reduced Logging Project: Added support for partially open tables.
**	    Reject the delete operation if this is a partially open table
**	    descriptor.  Server algorithms will not work if part of the
**	    described file set cannot be deleted.
**	7-Mar-2007 (kschendel) SIR 122757
**	    Don't truncate-before-delete for temp table files.
*/
DB_STATUS
dm2f_delete_file(
i4		lock_list,
i4		log_id,
DMP_LOCATION	*loc,
i4		l_count,
DM_OBJECT	*cb,
i4		flag,
DB_ERROR	*dberr)
{
    DB_STATUS	    status;
    STATUS          s = OK;
    DB_STATUS	    save_s = OK;
    CL_ERR_DESC	    sys_err;
    DMP_LOCATION    *l;
    i4         k;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** If the file is still in deferred I/O state, then no actual file(s)
    ** exists, and so no deletion needs to be done:
    */
    if (loc[0].loc_fcb->fcb_state & FCB_DEFERRED_IO)
    {
	return (E_DB_OK);
    }

    /*
    ** Delete each file in the location list.  If we get an error
    ** trying to delete one of the files, log the error, save the error
    ** code and continue deleting the files in the list.
    **
    ** If more than one error is encountered, the error code for the
    ** most recent error will be returned.
    **
    ** If we can't delete a file because of open-file quota, don't bother
    ** trying to continue as subsequent deletes will fail also.
    */

    for (k=0; k< l_count; k++)
    {
	for (;;)
	{
	    l = &loc[k];	

	    /*
	    ** Make sure the location has been initialized.  Dm2f_delete cannot
	    ** be called on a partial table.
	    */
	    if (l->loc_fcb->fcb_state & FCB_NOTINIT)
	    {
		s = FAIL;
		SETDBERR(dberr, 0, E_DM9290_DM2F_DELETE_ERROR);
		break;
	    }

	    if (cb == 0)
		cb = (DM_OBJECT *)l->loc_fcb;

    	    /* RAW files are never deleted */
    	    if ((l->loc_status & LOC_RAW) == 0)
	    {
		/* Don't bother with truncate-before-delete if temp table.
		** We only need it for regular tables; and then only if
		** some other server might have the file held open on an fd,
		** so that it doesn't hold the disk space.  (It would be
		** nice to test for multi-server, or slaves, and avoid
		** the pre-truncate if neither;  but there isn't a good
		** way to tell that I know of.)
		*/
		struct _DI_IO *f = l->loc_fcb->fcb_di_ptr;
		if (l->loc_fcb->fcb_state & FCB_TEMP)
		    f = NULL;

		s = DIdelete(f,
			l->loc_ext->physical.name,
			l->loc_ext->phys_length,
			l->loc_fcb->fcb_filename.name,
			l->loc_fcb->fcb_namelength,
			&sys_err);
	    }

	    if (s == E_DB_OK)
		break;
	    if ((s == DI_BADDIR) || (s == DI_DIRNOTFOUND))
		break; 
	    if (s == DI_EXCEED_LIMIT)
	    {
		status = dm2t_reclaim_tcb(lock_list, (i4)0, dberr);

		if (status == E_DB_OK)
		    continue;

		/*
		** If we can't proceed due to file open quota, log the 
                ** reason for failure and return RESOURCE_EXCEEDED.
		*/
		log_di_error(s, E_DM9003_BAD_FILE_DELETE, &sys_err, cb,
		    l->loc_ext->phys_length, &l->loc_ext->physical.name[0],
		    l->loc_fcb->fcb_namelength, 
		    &l->loc_fcb->fcb_filename.name[0],
		    (i4)0);

		if (dberr->err_code == E_DM9328_DM2T_NOFREE_TCB)
		{
		    SETDBERR(dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
		}
		else
		{
		    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, 
				NULL, 0, NULL, err_code, 0);
		    SETDBERR(dberr, 0, E_DM9290_DM2F_DELETE_ERROR);
		}
		return (E_DB_ERROR);
	    }

	    /*
	    ** Don't log FILENOTFOUND if flag is set - caller expects this
	    ** condition.
	    */
	    if ((s != DI_FILENOTFOUND) || (flag == 0))
	    {
		log_di_error(s, E_DM9003_BAD_FILE_DELETE, &sys_err, cb,
		    l->loc_ext->phys_length, &l->loc_ext->physical.name[0], 
		    l->loc_fcb->fcb_namelength, 
		    &l->loc_fcb->fcb_filename.name[0],
		    (i4)0);
	    }

	    break;
	}

	/*
	** Save this error unless this is less severe error than one already
	** saved (ie don't return FILENOTFOUND if we have gotten IO error).
	*/
	if ((s != OK) && ((s != DI_FILENOTFOUND) || (save_s == OK)))
	    save_s = s;
    }

    if (save_s == OK)
	return (E_DB_OK);

    /*	Handle errors. Messages have already been logged above. */

    if (save_s == DI_BADDIR)
	SETDBERR(dberr, 0, E_DM9293_DM2F_BADDIR_SPEC);
    else if (save_s == DI_DIRNOTFOUND)
	SETDBERR(dberr, 0, E_DM9292_DM2F_DIRNOTFOUND);
    else if (save_s == DI_FILENOTFOUND)
	SETDBERR(dberr, 0, E_DM9291_DM2F_FILENOTFOUND);
    else
	SETDBERR(dberr, 0, E_DM9290_DM2F_DELETE_ERROR);

    return (E_DB_ERROR);
}

/*{
** Name: logDiErrorFcn	- {@comment_text@}
**
** Description:
**      Log DI error for DM2F.
**
** Inputs:
**	s				DI STATUS value.
**      error                           The error to log.
**	sys_err				Pointer to DI sys_err.
**	cb				Control block for error info.
**	path				Pointer to path name.
**	l_path				Length of path name.
**	file				Pointer to file name.
**	l_file				Length of file name.
**	page				Page number.
**	FileName			File where macro issued
**	LineNumber			Line number within that file.
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
**      12-feb-1987 (Derek)
**          Created for Jupiter.
**      20-nov-87 (jennifer)
**          Added multiple location support.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    31-may-1991 (bryanp)
**		Added STATUS parameter and changed callers to pass the actual
**		DI status value. Added ule_format call to print the DI status
**		value.
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**	26-oct-92 (rogerk)
**	    Reduced Logging Project: Added support for partially open tables.
**	    Add DMP_TABLE_IO structure to list of debug output structures.
**	2-feb-93 (jnash)
**	    Remove "&" from fcb_tcb_ptr->tbio_dbname and tbio_relid 
**	    references.  These are pointers.
**	20-sep-1993 (andys)
**	    Fix arguments to ule_format call.
**	01-Dec-2008 (jonj)
**	    SIR 120874: renamed to logDiErrorFcn, invoked by log_di_error macro.
[@history_template@]...
*/
static VOID
logDiErrorFcn(
STATUS		    s,
i4		    error,
CL_ERR_DESC	    *sys_err,
DM_OBJECT	    *cb,
i4		    l_path,
char		    *path,
i4		    l_file,
char		    *file,
i4		    page,
PTR		    FileName,
i4		    LineNumber)
{
    i4		local_err_code;
    DB_DB_NAME          db;
    DB_TAB_NAME         tab;
    char		*db_name = db.db_db_name;
    char		*tbl_name = tab.db_tab_name;
    DB_ERROR	local_dberr;

    local_dberr.err_code = 0;
    local_dberr.err_data = 0;
    local_dberr.err_file = FileName;
    local_dberr.err_line = LineNumber;

    STmove(DB_UNKNOWN_DB, ' ', sizeof(db), db.db_db_name);
    STmove(DB_NOT_TABLE, ' ', sizeof(tab), tab.db_tab_name);
    if (cb)
    {
	switch (cb->obj_type)
	{
	case DCB_CB:
	    db_name = (char *) &((DMP_DCB *)cb)->dcb_name;
	    break;

	case RCB_CB:
	    db_name = 
               (char *) &((DMP_RCB *)cb)->rcb_tcb_ptr->tcb_dcb_ptr->dcb_name;
	    tbl_name = 
               (char *) &((DMP_RCB *)cb)->rcb_tcb_ptr->tcb_rel.relid;
	    break;

	case TCB_CB:
	    db_name = (char *) &((DMP_TCB *)cb)->tcb_dcb_ptr->dcb_name;
	    tbl_name = (char *) &((DMP_TCB *)cb)->tcb_rel.relid;
	    break;

	case TBIO_CB:
	    db_name = (char *) ((DMP_TABLE_IO *)cb)->tbio_dbname;
	    tbl_name = (char *) ((DMP_TABLE_IO *)cb)->tbio_relid;
	    break;

	case FCB_CB:
	    if (((DMP_FCB *)cb)->fcb_tcb_ptr != 0)
	    {
		db_name = (char *) ((DMP_FCB *)cb)->fcb_tcb_ptr->tbio_dbname;
		tbl_name = (char *) ((DMP_FCB *)cb)->fcb_tcb_ptr->tbio_relid;
	    }
	    path = (char *) &((DMP_FCB *)cb)->fcb_location->physical; 
	    l_path = ((DMP_FCB *)cb)->fcb_location->phys_length;
	    file = (char *) &((DMP_FCB *)cb)->fcb_filename;
	    l_file = ((DMP_FCB *)cb)->fcb_namelength;
	    break;
	}
    }
    else
    {
	path = "Unknown Path";
	l_path = 12;
	file = "Unknown file";
	l_file = 12;
    }

    if (s)
    {
	local_dberr.err_code = s;
        uleFormat(&local_dberr, 0, sys_err, ULE_LOG, NULL,
		    NULL, 0, NULL, &local_err_code, 0);
    }

    local_dberr.err_code = error;

    uleFormat(&local_dberr, 0, sys_err, ULE_LOG, NULL, NULL, 0, NULL, &local_err_code, 5,
	sizeof(DB_DB_NAME), db_name, sizeof(DB_TAB_NAME), tbl_name,
	l_path, path, l_file, file, 0, page);
}

/*{
** Name: dm2f_alloc_file	- Allocate space for a database file.
**
** Description:
**
**	This routine is just a wrapper for the allocate_to_file
**	routine. It calls this routine indicating that the space
**	does NOT need to be guaranteed on disc.
**
** Inputs:
**      loc                             Pointer to a location array.
**      l_count                         Count of number of locations.
**      tab_name                        Table name for error logging.
**      db_name                         Database name for error logging.
**      count                           Number of pages to allocate.
**      page                            Pointer to a variable which must
**					hold the page to allocate from.
**
** Outputs:
**      page                            Pointer to a variable to return the
**                                      last page number allocated.
**      err_code                        Reason for error return status.
**					Which includes one of the following:
**					E_DM0112_RESOURCE_QUOTA_EXCEED
**					E_DM9339_DM2F_ALLOC_ERROR
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
**      20-nov-87 (jennifer)
**          Created for Added multiple location support.
**	16-jan-1992 (bryanp)
**	    Added deferred I/O support: remember allocation in fcb.
**	22-sep-1992 (bryanp)
**	    Check for FCB_MATERIALIZING so this can be called from
**	    materialize_fcb()
**	    Overload count field so that negative count means that this is the
**	    materialization call (in which case we don't need the semaphore,
**	    because materialize_fcb() already owns the semaphore).
**	26-oct-92 (rogerk)
**	    Changed allocation to fix problem with files which are not
**	    proportioned correctly.  Allocate no longer assumes that all
**	    files are currently evenly allocated.
**	26-oct-92 (rogerk)
**	    Reduced Logging Project: Added support for partially open tables.
**	    Reject the alloc operation if the file on which the allocate
**	    must be done is not currently open.
**	30-October-1992 (rmuth)
**	    Changed to call allocate_to_file.
**	14-sep-95 (nick)
**	    Correct and restore comments ; parameter 'page' is the next
**	    page to allocate and NOT the last page allocated.
*/
DB_STATUS
dm2f_alloc_file(
DMP_LOCATION        *loc,
i4             l_count,
DB_TAB_NAME         *tab_name,
DB_DB_NAME          *db_name,
i4		    count,
i4             *page,
DB_ERROR	    *dberr)
{
    DB_STATUS	status;

    status = allocate_to_file( DM2F_NOT_GUARANTEE_SPACE,
			       loc, l_count,
			       tab_name, db_name,
			       count, page,
			       dberr );
    return( status );
}

/*{
** Name: dm2f_file_size	- Return file size given allocation, number of 
**			  locations and current location.
**
** Description:
**      This routine returns the number of pages to allocate to a file
**	given the allocation amount, the number of locations and which
**	location we are currently creating a file in. 
**
** Inputs:
**	allocation			Total allocation.
**	no_locations			Number of locations table is spread
**					across.
**	current_location		Current location we are creating the
**					file on.
**
** Outputs:
**	Returns:
**	    file_size			File size in pages of the file at
**					this location.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	04-apr-90 (Derek)
**	7-Feb-1992 (rmuth)
**	    Changed the algorithm which works out how many pages to allocate
**	    to each file in a multi-location table. The old algorithm tried
**	    to allocate a factor of DI_EXTENT_SIZE to each location. Ie if
**	    the allocation was 4 pages and the table had two locations it 
**	    would try and allocate 16 pages to each location, but due to
**	    code problem it actually allocated 17 pages to each file. The
**	    new algorithm works the same way as the extend code, ie it
**	    will not allocate more space than was asked for but it will
**	    spread the allocation across the locations in DI_EXTENT_SIZE's.
**	    So a 4 page table with two locations will be allocated 4 pages
**	    to the first location and none to the second and a 34 page table 
**	    with two locations will have 18 pages allocated to location 1 and
**	    16 pages allocated to location 2. 
*/
i4
dm2f_file_size(
i4		    allocation,
i4		    no_locations,
i4		    current_location)
{
    i4	stripe_size;
    i4	full_stripes_needed;
    i4	stripe_remainder;
    i4	extra_extents_needed;
    i4	extent_remainder;

    if (no_locations == 1)
    {
	return (allocation);
    }

    stripe_size		= DI_EXTENT_SIZE * no_locations;
    full_stripes_needed	= allocation / stripe_size;
    stripe_remainder	= allocation - ( full_stripes_needed * stripe_size );
    extra_extents_needed= stripe_remainder / DI_EXTENT_SIZE;
    extent_remainder	= stripe_remainder - 
			    ( extra_extents_needed * DI_EXTENT_SIZE );

    allocation = full_stripes_needed * DI_EXTENT_SIZE;

    if ( current_location < extra_extents_needed )
    {
	allocation = allocation + DI_EXTENT_SIZE;
    }
    else if ( current_location ==  extra_extents_needed )
    {
	allocation = allocation +  extent_remainder;
    }

    return( allocation );
}

/*{
** Name: dm2f_force_file	- Forces Page to be written to a database file.
**
** Description:
**      This routine forces the pages specified to the database 
**      files associated with
**      the locations specified by the location array.  
**
** Inputs:
**      loc                             Pointer to a location array.
**      l_count                         Count of number of locations.
**      tab_name                        Name of table for error logging.
**      db_name                         Name of database for error logging.
**
** Outputs:
**      err_code                        Reason for error return status.
**					E_DM933A_DM2F_FORCE_ERROR
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
**      20-nov-87 (jennifer)
**          Created for Added multiple location support.
**	16-jan-1992 (bryanp)
**	    Reject this call if the file is in deferred I/O state.
*/
DB_STATUS
dm2f_force_file(
DMP_LOCATION        *loc,
i4             l_count,
DB_TAB_NAME         *tab_name,
DB_DB_NAME          *db_name,
DB_ERROR	    *dberr)
{
    CL_ERR_DESC		sys_err;
    DB_STATUS		status;
    STATUS		s;
    i4             k;
    DMP_LOCATION        *l;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    s = OK;
    for (k = 0; k< l_count; k++)
    {
	l = &loc[k];	
	if ((l->loc_fcb->fcb_state & FCB_DEFERRED_IO) != 0)
	{
	    s = FAIL;
	    break;
	}

	/*
	** Skip locations which have not had any IO performed on them since
	** the last force or flush operation.
	**
	** Note that this will also bypass non-open locations in a partially
	** open table.
	*/
	if ((l->loc_fcb->fcb_state & FCB_IO) == 0)
	    continue;
	s = DIforce(l->loc_fcb->fcb_di_ptr, &sys_err);
	if (s == OK)
	{
	    l->loc_fcb->fcb_state &= ~FCB_IO;
	    continue;
	}

	/* Error occurred forcing the file */
	uleFormat(NULL, E_DM9006_BAD_FILE_WRITE, &sys_err, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 5,
	    sizeof(DB_DB_NAME), db_name, sizeof(DB_TAB_NAME), tab_name,
	    l->loc_ext->phys_length, &l->loc_ext->physical,
	    l->loc_fcb->fcb_namelength, &l->loc_fcb->fcb_filename, 0, -1);
	break;
    }
    if (s== OK)
	return (E_DB_OK);
	
    /* Handle errors. */
    SETDBERR(dberr, 0, E_DM933A_DM2F_FORCE_ERROR);
    return (E_DB_ERROR);
}

/*{
** Name: dm2f_flush_file	- Flush header information for a database file.
**
** Description:
**      This routine flushes the header information for the database 
**      files associated with
**      the locations specified by the location array.  
**
** Inputs:
**      loc                             Pointer to a location array.
**      l_count                         Count of number of locations.
**      tab_name                        Table name for logging.
**      db_name                         Database name for logging.
**
** Outputs:
**      err_code                        Reason for error return status.
**					E_DM933B_DM2F_FLUSH_ERROR
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
**      20-nov-87 (jennifer)
**          Created for Added multiple location support.
**	18-feb-89 (EdHsu)
**	    Turning off FCB_FLUSH bit if DIflush is ok.
**	11-Feb-2008 (kschendel) b122122
**	    Flush != force!  Don't clear the fcb-io flag here.
*/
DB_STATUS
dm2f_flush_file(
DMP_LOCATION        *loc,
i4		    l_count,
DB_TAB_NAME         *tab_name,
DB_DB_NAME          *db_name,
DB_ERROR	    *dberr)
{
    CL_ERR_DESC		sys_err;
    DB_STATUS		status;
    STATUS		s;
    i4             k;
    DMP_LOCATION        *l;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    s = OK;
    for (k = 0; k< l_count; k++)
    {
	l = &loc[k];	

	/*
	** Skip locations which have not had any alloc operations performed 
	** on them since the last flush.
	**
	** Note that this will also bypass non-open locations in a partially
	** open table.
	*/
	if ((l->loc_fcb->fcb_state & FCB_FLUSH) == 0)
	{
	    continue;
	}
	s = DIflush(l->loc_fcb->fcb_di_ptr, &sys_err);
	if (s == OK)
	{
	    l->loc_fcb->fcb_state &= ~(FCB_FLUSH);
	    continue;
	}

	/* Error occurred flushing the file */
	uleFormat(NULL, E_DM9008_BAD_FILE_FLUSH, &sys_err, ULE_LOG, NULL, 
	    0, 0, 0, err_code, 4,
	    sizeof(DB_DB_NAME), db_name, sizeof(DB_TAB_NAME), tab_name,
	    l->loc_ext->phys_length, &l->loc_ext->physical,
	    l->loc_fcb->fcb_namelength, &l->loc_fcb->fcb_filename);
	break;
    }
    if (s== OK)
	return (E_DB_OK);

    /* Handle errors. */
    SETDBERR(dberr, 0, E_DM933B_DM2F_FLUSH_ERROR);
    return (E_DB_ERROR);
}

/*{
** Name: dm2f_read_file	- Read a database file.
**
** Description:
**      This routine reads the pages specified from the database 
**      files associated with
**      the locations specified by the location array.  
**
** Inputs:
**      loc                             Pointer to a location array.
**      l_count                         Count of number of locations.
**      page_size                       Page size
**      tab_name                        Table name for logging.
**      db_name                         Database name for logging.
**      page                            Page to start reading from.
**      count                           Number of pages to read.
**
** Outputs:
**      data                            Pointer to an area to return page.
**      err_code                        Reason for error return status.
**					E_DM933C_DM2F_READ_ERROR
**					E_DM9335_DM2F_ENDFILE
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
**      20-nov-87 (jennifer)
**          Created for Added multiple location support.
**	 3-may-89 (rogerk)
**	    Added page number parameter to DM9005 error message.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    26-dec-1990 (rachael
**		Added ule_format of CL error
**	16-jan-1992 (bryanp)
**	    This call is illegal if the file is in deferred I/O state.
**	22-sep-1992 (bryanp)
**	    Improve error logging in DM9335 case.
**	26-oct-92 (rogerk)
**	    Reduced Logging Project: Added support for partially open tables.
**	    Reject the read operation if one of the files on which the read
**	    is requested is not currently open.
**	18-Jan-95 (cohmi01)
**	    For RAW I/O, add level of indirection when computing blk number.
**	    Add the beginning of the extent for this table in the raw file.
**	27-dec-1995 (dougb)
**	    Porting change:  Make all references to RAW I/O specific to the
**	    supported Unix platforms.
*/
DB_STATUS
dm2f_read_file(
DMP_LOCATION        *loc,
i4             l_count,
i4             page_size,
DB_TAB_NAME         *tab_name,
DB_DB_NAME          *db_name,
i4             *count,
i4             page,
char                *data,
DB_ERROR	    *dberr)
{
    CL_ERR_DESC		sys_err;
    DB_STATUS		status;
    STATUS		s = OK;
    i4             k;
    DMP_LOCATION        *l;
    i4             pg_cnt;
    i4             extent;
    i4             pageno;    
    char                *local_data;
    i4             cnt_read;
    i4             local_count;
    i4             local_page;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (loc[0].loc_fcb->fcb_state & FCB_DEFERRED_IO)
    {
	SETDBERR(dberr, 0, E_DM933C_DM2F_READ_ERROR);
	return (E_DB_ERROR);
    }

    local_data = data;
    local_count = 0;
    local_page = page;    
    for (;;)
    {
	/* For multiple locations must figure out how to 
        ** determine which files the read should be 
        ** done on. */
	extent = local_page >> DI_LOG2_EXTENT_SIZE;
	l = &loc[extent % l_count];
	
	if (l_count > 1)
	{
	    pg_cnt = DI_EXTENT_SIZE - (local_page & (DI_EXTENT_SIZE - 1)) ;
	    pageno = (extent/l_count)*DI_EXTENT_SIZE +
			(local_page & (DI_EXTENT_SIZE -1));
	    if (*count < pg_cnt)
		pg_cnt = *count;
	}
	else
	{
	    pg_cnt = *count;
	    pageno = local_page;
	}

	/*
	** Verify that this location is open.
	*/
	if ((l->loc_fcb->fcb_state & FCB_OPEN) == 0)
	{
	    s = FAIL;
	    SETDBERR(dberr, 0, E_DM933C_DM2F_READ_ERROR);
	    break;
	}

	/* Considerations for RAW locations */
	if (l->loc_status & LOC_RAW)
	{
	    /* 
	    ** Reading beyond raw file bounds is unforgivable,
	    ** so read nothing.
	    */
	    if ( pageno + pg_cnt > l->loc_fcb->fcb_physical_allocation )
	    {
		s = DI_ENDFILE;
		SETCLERR(&sys_err, 0, 0);
		cnt_read = 0;

		uleFormat(NULL, E_DM0194_BAD_RAW_PAGE_NUMBER, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 6,
		    sizeof("Read")-1, "Read",
		    0, pageno,
		    sizeof(l->loc_name), l->loc_name,
		    0, l->loc_fcb->fcb_rawstart,
		    0, l->loc_fcb->fcb_physical_allocation,
		    0, l->loc_fcb->fcb_rawpages);
	    }
	    else
	    {
		/* Offset pageno to first page of file */
		pageno += l->loc_fcb->fcb_rawstart;
	    }
        }

	cnt_read = pg_cnt;    /* try to read all */

	if ( s == OK )
	    s = DIread(l->loc_fcb->fcb_di_ptr, &cnt_read, pageno, 
			       local_data, &sys_err); 
	if ((s != OK) && (s != DI_ENDFILE))
	{
	    uleFormat(NULL, s, &sys_err, ULE_LOG,NULL,NULL,0,NULL,err_code,0);
	    uleFormat(NULL, E_DM9005_BAD_FILE_READ, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 5,
		sizeof(DB_DB_NAME), db_name,
		sizeof(DB_TAB_NAME), tab_name,
		l->loc_ext->phys_length, &l->loc_ext->physical,
		l->loc_fcb->fcb_namelength, &l->loc_fcb->fcb_filename,
		0, pageno);
	    SETDBERR(dberr, 0, E_DM933C_DM2F_READ_ERROR);
	    break;
	}
	if (cnt_read != pg_cnt)
	{
	    local_count = local_count + cnt_read;
	    uleFormat(NULL, E_DM93AF_BAD_PAGE_CNT, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 3,
		0, pageno, 0, pg_cnt, 0, cnt_read);
	    uleFormat(NULL, E_DM9005_BAD_FILE_READ, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 5,
		sizeof(DB_DB_NAME), db_name,
		sizeof(DB_TAB_NAME), tab_name,
		l->loc_ext->phys_length, &l->loc_ext->physical,
		l->loc_fcb->fcb_namelength, &l->loc_fcb->fcb_filename,
		0, pageno);
	    SETDBERR(dberr, 0, E_DM9335_DM2F_ENDFILE);
	    break;
	}
	else
	    local_count = local_count + pg_cnt;

	local_data = local_data + pg_cnt * page_size;
	local_page = local_page + pg_cnt;
	*count = *count - pg_cnt;
	if (*count == 0)
	    break;    
    }
    *count = local_count;
    if (s == OK)
	return (E_DB_OK);

    return (E_DB_ERROR);
}

/*{
** Name: dm2f_sense_file	- Read a database file.
**
** Description:
**      This routine senses the logical end of file for all the database 
**      files associated with
**      the locations specified by the location array.  
**
** Inputs:
**      loc                             Pointer to a location array.
**      l_count                         Count of number of locations.
**      tab_name                        Table name for logging;
**      db_name                         Database name for logging;
**
** Outputs:
**      page                            Pointer to a vriable where last
**                                      logical page number can be returned.
**      err_code                        Reason for error return status.
**					E_DM933D_DM2F_SENSE_ERROR
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
**      20-nov-87 (jennifer)
**          Created for Added multiple location support.
**	14-jul-88 (mmm)
**	    Fixed concurrency problem shown up on unix.  The passed in
**	    variable "page" was in some instances a pointer into a table
**	    control block.  This structure was not protected by a critical
**	    section during the call to dm2f_sense_file(), so other threads
**	    could access the structure while dm2f_sense_file() was changing
**	    the value of "page" (last page in the relation).  All the code
**	    is written, such that it is OK that this critical section does
**	    not exist, as long as the value of page is alway either the same
**	    or increasing.  The previous code in the routine would zero the
**	    "page" variable upon entry, which would in some concurrent instances
**	    cause havic for other threads which accessed the table control
**	    block and saw this zero.  The fix was to maintain a local variable
**	    and only update the passed in variable with the new end of file
**	    value.
**	16-jan-1992 (bryanp)
**	    If the file is in deferred I/O state, return the deferred allocation
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Added support for partially open tables
**	    and to fix problems with multi-location files which do not match
**	    up in file sizes.  We now return a table size that reflects the
**	    maximum size that is possible given the individual file sizes and
**	    which still keeps the file proportions within legal limits.
**	    For partial table support, we assume that unopened files have the
**	    same size as other files in the table.
**	    Also added requirement that the fcb mutex be locked during the
**	    sense operation since information is stored in shared fcb fields.
**	18-Jan-95 (cohmi01)
**	    For RAW I/O, avoid actual sense, physical file size is meaningless,
**	    we are concerned with extent size, already in the FCB (its static).
**	27-dec-1995 (dougb)
**	    Porting change:  Make all references to RAW I/O specific to the
**	    supported Unix platforms.
*/
DB_STATUS
dm2f_sense_file(
DMP_LOCATION        *loc,
i4             l_count,
DB_TAB_NAME         *tab_name,
DB_DB_NAME          *db_name,
i4		    *page,
DB_ERROR	    *dberr)
{
    STATUS		s = OK;
    CL_ERR_DESC		sys_err;
    DB_STATUS		status;
    i4             	k;
    DMP_LOCATION        *l;
    DMP_FCB		*f;
    i4             	page_count = 0;
    i4			sense_page;
    i4			full_extents;
    i4			min_pages;
    i4			remainder_exts;
    i4			remainder_pgs;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** Acquire the fcb mutex before starting the sense operation.
    ** Need the mutex even for the deferred IO case since the fcb
    ** could potentially be in the middle of transforming from
    ** deferred state to not.
    */
    dm0s_mlock(&loc[0].loc_fcb->fcb_mutex);
    if (loc[0].loc_fcb->fcb_state & FCB_DEFERRED_IO)
    {
	*page = loc[0].loc_fcb->fcb_deferred_allocation - 1;
	dm0s_munlock(&loc[0].loc_fcb->fcb_mutex);
	return (E_DB_OK);
    }

    /*
    ** Sense each open file in the location array.
    ** Skip those locations which are not open.
    **
    ** The page count returned from each sense is stored in the fcb.
    */
    for (k = 0; k < l_count; k++)
    {
	l = &loc[k];
	f = l->loc_fcb;

	/*
	** Check if this location is open.  Set its physical allocation
	** to zero temporarily.
	*/
	if ((f->fcb_state & FCB_OPEN) == 0)
	{
	    f->fcb_physical_allocation = 0;
	    continue;
	}

	/* Considerations for RAW locations */
	if (l->loc_status & LOC_RAW)
	{
	    /* Determine number of pages in file from FHDR, if available */
	    if ( page_count == 0 &&
		 f->fcb_tcb_ptr &&
		 f->fcb_tcb_ptr->tbio_lalloc > 0 )
	    {
		page_count = f->fcb_tcb_ptr->tbio_lalloc + 1;
		full_extents = page_count / DI_EXTENT_SIZE;
		min_pages = (full_extents / l_count) * DI_EXTENT_SIZE;
		remainder_exts = full_extents % l_count;
		remainder_pgs = page_count % DI_EXTENT_SIZE;
	    }

	    if ( page_count )
	    {
		/* Compute number of pages in this location */
		if ( k < remainder_exts )
		    f->fcb_physical_allocation = min_pages + DI_EXTENT_SIZE;
		else if ( k == remainder_exts )
		    f->fcb_physical_allocation = min_pages + remainder_pgs;
		else
		    f->fcb_physical_allocation = min_pages;
	    }
	    /*
	    ** Can't determine from FHDR. If physical allocation is unknown
	    ** set it to the raw location's total page count.
	    ** This solves a bootstrapping problem whereby dm1p is attempting
	    ** to pagefix a FHDR/FMAP in order to set tbio_lalloc
	    ** and dm0p has called here to verify that the FHDR/FMAP page 
	    ** number is valid.
	    **
	    ** Otherwise, leave fcb_physical_allocation untouched.
	    */
	    else if ( f->fcb_physical_allocation == 0 )
		f->fcb_physical_allocation = f->fcb_rawpages;
	}
	else
	{
	    s = DIsense(f->fcb_di_ptr, &sense_page, &sys_err); 
	    if (s != OK)
	    {
		uleFormat(NULL, E_DM9007_BAD_FILE_SENSE, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 4,
		    sizeof(DB_DB_NAME), db_name,
		    sizeof(DB_TAB_NAME), tab_name,
		    l->loc_ext->phys_length, 
		    &l->loc_ext->physical,
		    f->fcb_namelength, 
		    &f->fcb_filename);
		break;
	    }

	    /*
	    ** Adjust sense_page to show the number of pages in the file rather
	    ** than the last page number.
	    */
	    f->fcb_physical_allocation = sense_page + 1;
	}
    }

    if (s != OK)
    {
	dm0s_munlock(&loc[0].loc_fcb->fcb_mutex);
	SETDBERR(dberr, 0, E_DM933D_DM2F_SENSE_ERROR);
	return (E_DB_ERROR);
    }

    /*
    ** Calculate the logical file size given the current distribution
    ** of physical file sizes among the partitions.
    */
    calculate_allocation(loc, l_count, &page_count);

    /* Release the mutex when complete. */
    dm0s_munlock(&loc[0].loc_fcb->fcb_mutex);

    /*
    ** Return the last logical page number in the table rather than
    ** the actual number of pages (have to subtract one).
    */
    *page = page_count - 1;
    return (E_DB_OK);
}

/*{
** Name: calculate_allocation	- Determine logical allocation of a multi-
**				  location table.
**
** Description:
**
** Inputs:
**      loc                             Pointer to a location array.
**      l_count                         Count of number of locations.
**
** Outputs:
**      total_pages                     Total logical pages in the file set.
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	The fcb_logical_allocation values in the location arrays are filled in.
**
** History:
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Created to resolve outstanding problems
**	    with multi-location file groups where file sizes are left out
**	    of proportion with each other.
*/
static VOID
calculate_allocation(
DMP_LOCATION	    *loc,
i4		    l_count,
i4		    *total_pages)
{
    i4             k;
    DMP_LOCATION        *l;
    i4             page_count;
    i4		minimum_pages;
    i4		max_file_size;
    i4		logical_pages;

    /*
    ** Find the minimum physical file size of all sensed files
    ** (skipping those non-open files).
    */
    minimum_pages = 0;
    for (k = 0; k < l_count; k++)
    {
	l = &loc[k];

	if ((l->loc_fcb->fcb_state & FCB_OPEN) &&
	    ((l->loc_fcb->fcb_physical_allocation < minimum_pages) ||
	     (minimum_pages == 0)))
	{
	    minimum_pages = l->loc_fcb->fcb_physical_allocation;
	}
    }

    /*
    ** Determine maximum legal file size for the first file of the location
    ** array.  Its greatest legal size is the next DI_EXTENT boundary past
    ** the minimum encountered file size.
    */
    max_file_size = minimum_pages + DI_EXTENT_SIZE;
    max_file_size -= (max_file_size % DI_EXTENT_SIZE);

    /*
    ** Cycle through the locations we have sensed and determine the
    ** maximum legal sizes for each file that will still allow a proper
    ** distribution of pages.
    **
    ** Sum up the total count of pages as we go through.
    */
    page_count = 0;
    for (k = 0; k < l_count; k++)
    {
	l = &loc[k];

	/*
	** Assume unopened files have maximum possible allocation.
	*/
	if ((l->loc_fcb->fcb_state & FCB_OPEN) == 0)
	    l->loc_fcb->fcb_physical_allocation = max_file_size;

	/*
	** Once a file has been found that has less than the current
	** maximum number of pages, then readjust the maximum file size.
	** Files subsequent to this one can have no more pages than
	** the largest full DI_EXTENT_SIZE boundary that this file reaches.
	*/
	logical_pages = max_file_size;
	if (l->loc_fcb->fcb_physical_allocation < max_file_size)
	{
	    logical_pages = l->loc_fcb->fcb_physical_allocation;
	    max_file_size = logical_pages - (logical_pages % DI_EXTENT_SIZE);
	}

	l->loc_fcb->fcb_logical_allocation = logical_pages;
	page_count += logical_pages;
    }

    *total_pages = page_count;
}

/*{
** Name: dm2f_rename	- Rename one database file at one location.
**
** Description:
**      This routine rename one file at one location to a new name.
**      This is primarily used for backing out of dmu operations.
**
**	The file MUST be closed when dm2f_rename is called. Some systems let
**	you rename an open file; others don't. Regardless, we don't ever
**	rename an open file -- we close it first, rename it, then open it under
**	the new name.
**
** Inputs:
**	lock_list			Lock list of current transaction.
**	log_id				Log txn id of current transaction.
**      phys                            Pointer to location/path name.
**      l_phys                          Length of path name.
**      ofname                          Pointer to old filename.
**      l_ofname                        Length of oldfilename.
**      nfname                          Pointer to new filename.
**      l_nfname                        Length of new filename.
**      db_name                         Database name for logging.
**
** Outputs:
**      err_code                        Reason for error return status.
**					Which includes one of the following:
**					E_DM0112_RESOURCE_QUOTA_EXCEED
**					E_DM933E_DM2F_RENAME_ERROR
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
**      20-nov-87 (jennifer)
**          Created for Added multiple location support.
**	15-apr-1992 (bryanp)
**	    Removed the FCB argument. This function no longer supports renaming
**	    an open file. To avoid complexities of changing the CL interface,
**	    we'll continue to pass the parameter, but it'll always be zero.
**	24-jan-1995 (cohmi01)
**	    For RAW location: Pass in DMP_LOC parm, needed for determining
**	    if file is in a raw location, and for calls to dm2f_rawnamdir().
**	    dm2f_rawnamdir() does the rename, by adjusting etry in the
**	    Raw Name Directory (RND). No file exists to be renamed.
**	27-dec-1995 (dougb)
**	    Porting change:  Make all references to RAW I/O specific to the
**	    supported Unix platforms.
**	21-mar-1999 (nanpr01)
**	    We should not be calling dm2f_rename operation for raw location.
*/
DB_STATUS
dm2f_rename(
i4		    lock_list,
i4		    log_id,
DM_PATH             *phys,
i4              l_phys,
DM_FILE             *ofname,
i4              l_ofname,
DM_FILE             *nfname,
i4              l_nfname,
DB_DB_NAME          *db_name,
DB_ERROR	    *dberr)
{
    CL_ERR_DESC		sys_err;
    DB_STATUS		status;
    STATUS		s;
    i4		local_error;

    CLRDBERR(dberr);

    for (;;)
    {
	s = DIrename((DI_IO *)NULL, phys->name, l_phys, ofname->name, l_ofname,
                    nfname->name, l_nfname, &sys_err);
	if (s != OK)
	{
	    /*
	    ** If DIrename fails because of open file quota, try to free
	    ** up some file descriptors and try again.
	    */
	    if (s == DI_EXCEED_LIMIT)
	    {
		status = dm2t_reclaim_tcb(lock_list, (i4)0, dberr);

		if (status == E_DB_OK)
		    continue;

		if (dberr->err_code != E_DM9328_DM2T_NOFREE_TCB)
		    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, 
				&local_error, 0);
	    }
	    break;
	}
	break;
    }
    if (s == OK)
	return (E_DB_OK);
	
    /*	Handle errors. */
    uleFormat(NULL, E_DM9009_BAD_FILE_RENAME, &sys_err, ULE_LOG, NULL,
	(char *)NULL, 0L, (i4 *)NULL, &local_error, 4,
	sizeof(*db_name), db_name, 
	l_phys,	phys, l_ofname, ofname, l_nfname, nfname);

    /*
    ** If we failed because of lack of open file descriptors, return
    ** RESOURCE error.
    */
    if ((s == DI_EXCEED_LIMIT) && (dberr->err_code == E_DM9328_DM2T_NOFREE_TCB))
	SETDBERR(dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
    else
	SETDBERR(dberr, 0, E_DM933E_DM2F_RENAME_ERROR);

    return (E_DB_ERROR);
}

/*{
** Name: dm2f_write_file	- Write a database file.
**
** Description:
**      This routine writes the pages specified to the database 
**      files associated with
**      the locations specified by the location array.  
**
**	The first dm2f_write_file call to a file in deferred I/O state causes
**	the file to be automatically materialized. The underlying physical
**	file(s) are created, and any deferred disk space allocation is
**	performed.
**
** Inputs:
**      loc                             Pointer to a location array.
**      l_count                         Count of number of locations.
**      page_size                       Page size
**      tab_name                        Table name for logging.
**      db_name                         Database name for logging.
**      page                            Page to start writing to.
**      count                           Number of pages to write.
**
** Outputs:
**      data                            Pointer to an area to return page.
**      err_code                        Reason for error return status.
**					E_DM933F_DM2F_WRITE_ERROR
**					E_DM9335_DM2F_ENDFILE
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
**      20-nov-87 (jennifer)
**          Created for Added multiple location support.
**	 3-may-89 (rogerk)
**	    Added page number parameter to DM9006 error message.
**	16-jan-1992 (bryanp)
**	    Materialize the file if it's in deferred I/O state.
**	09-jun-1992 (kwatts)
**	    Extended interface with page_fmt parameter to get at accessors.
**	22-sep-1992 (bryanp)
**	    Improve error logging in DM9335 case.
**	26-oct-92 (rogerk)
**	    Reduced Logging Project: Added support for partially open tables.
**	    Reject the write operation if one of the files on which the write
**	    is requested is not currently open.
**	14-dec-1992 (jnash & rogerk)
**	    Removed DMPP_ACC_PLV argument from dm2f_write_file.
**	18-Jan-95 (cohmi01)
**	    For RAW I/O, add level of indirection when computing blk number.
**	    Add the beginning of the extent for this table in the raw file.
**	27-dec-1995 (dougb)
**	    Porting change:  Make all references to RAW I/O specific to the
**	    supported Unix platforms.
**      06-mar-96 (stial01)
**          Pass page size in case we need to materialize_fcb()
**	01-oct-1998 (somsa01)
**	    If we get DI_NODISKSPACE from DIwrite(), set the error code to
**	    E_DM9448_DM2F_NO_DISK_SPACE.  (Bug #93633)
*/
DB_STATUS
dm2f_write_file(
DMP_LOCATION        *loc,
i4             l_count,
i4             page_size,
DB_TAB_NAME         *tab_name,
DB_DB_NAME          *db_name,
i4             *count,
i4             page,
char                *data,
DB_ERROR	    *dberr)
{
    CL_ERR_DESC		sys_err;
    DB_STATUS		status;
    STATUS		s = OK;
    DMP_LOCATION        *l;
    char                *local_data;
    i4             pg_cnt;
    i4             extent;
    i4             pageno;    
    i4             local_count;
    i4             local_page;
    i4			cnt_written;
    DMP_TABLE_IO        *tbio;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (loc[0].loc_fcb->fcb_state & FCB_DEFERRED_IO)
    {
	status = materialize_fcb(loc, l_count, page_size, 
				tab_name, db_name, dberr);
	if (status)
	{
	    if (dberr->err_code > E_DM_INTERNAL)
	    {
		uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		SETDBERR(dberr, 0, E_DM933F_DM2F_WRITE_ERROR);
	    }
	    return (status);
	}
    }

    local_data = data;
    local_count = 0;
    local_page = page;    

    for (;;)
    {
	/* For multiple locations must figure out how to 
        ** determine which files the read should be 
        ** done on. */
	extent = local_page >> DI_LOG2_EXTENT_SIZE;
	l = &loc[extent % l_count];
	
	if (l_count > 1)
	{
	    pg_cnt = DI_EXTENT_SIZE - (local_page & (DI_EXTENT_SIZE - 1)) ;
	    pageno = (extent/l_count)*DI_EXTENT_SIZE +
		    (local_page & (DI_EXTENT_SIZE -1));
	    if (*count < pg_cnt)
		pg_cnt = *count;
	}
	else
	{
	    pg_cnt = *count;
	    pageno = local_page;
	}

	/*
	** Verify that this location is open.
	*/
	if ((l->loc_fcb->fcb_state & FCB_OPEN) == 0)
	{
	    s = FAIL;
	    SETDBERR(dberr, 0, E_DM933F_DM2F_WRITE_ERROR);
	    break;
	}

	/* Considerations for RAW locations */
	if (l->loc_status & LOC_RAW)
	{
	    /* 
	    ** Writing beyond raw file bounds is unforgivable,
	    ** so write nothing.
	    */
	    if (pageno + pg_cnt > l->loc_fcb->fcb_physical_allocation)
	    {
		s = DI_NODISKSPACE;
		SETCLERR(&sys_err, 0, 0);

		uleFormat(NULL, E_DM0194_BAD_RAW_PAGE_NUMBER, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 6,
		    sizeof("Write")-1, "Write",
		    0, pageno,
		    sizeof(l->loc_name), l->loc_name,
		    0, l->loc_fcb->fcb_rawstart,
		    0, l->loc_fcb->fcb_physical_allocation,
		    0, l->loc_fcb->fcb_rawpages);
	    }
	    else
	    {
		/* Offset pageno to first page of file */
		pageno += l->loc_fcb->fcb_rawstart;
	    }
        }

	cnt_written = pg_cnt;    /* try to write all */

	if ( s == OK )
	    s = DIwrite(l->loc_fcb->fcb_di_ptr, &cnt_written, pageno, 
			   local_data, &sys_err); 
	if ((s != OK) && (s != DI_ENDFILE))
	{
	    uleFormat(NULL, s, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    if (s == DI_NODISKSPACE)
	    {
		uleFormat(NULL, E_DM9448_DM2F_NO_DISK_SPACE, &sys_err, ULE_LOG,
		    NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 5,
		    sizeof(DB_DB_NAME), db_name,
		    sizeof(DB_TAB_NAME), tab_name,
		    l->loc_ext->phys_length, &l->loc_ext->physical,
		    l->loc_fcb->fcb_namelength, &l->loc_fcb->fcb_filename,
		    0, pageno);
		SETDBERR(dberr, 0, E_DM9448_DM2F_NO_DISK_SPACE);
	    }
	    else
	    {
		uleFormat(NULL, E_DM9006_BAD_FILE_WRITE, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 5,
		    sizeof(DB_DB_NAME), db_name,
		    sizeof(DB_TAB_NAME), tab_name,
		    l->loc_ext->phys_length, &l->loc_ext->physical,
		    l->loc_fcb->fcb_namelength, &l->loc_fcb->fcb_filename,
		    0, pageno);
		SETDBERR(dberr, 0, E_DM933F_DM2F_WRITE_ERROR);
	    }
	    break;
	}
	l->loc_fcb->fcb_state |= FCB_IO;
	if (cnt_written != pg_cnt)
	{
	    local_count = local_count + cnt_written;
	    uleFormat(NULL, E_DM93AF_BAD_PAGE_CNT, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 3,
		0, pageno, 0, pg_cnt, 0, cnt_written);
	    uleFormat(NULL, E_DM9006_BAD_FILE_WRITE, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 5,
		sizeof(DB_DB_NAME), db_name,
		sizeof(DB_TAB_NAME), tab_name,
		l->loc_ext->phys_length, &l->loc_ext->physical,
		l->loc_fcb->fcb_namelength, &l->loc_fcb->fcb_filename,
		0, pageno);
	    SETDBERR(dberr, 0, E_DM9335_DM2F_ENDFILE);
	    break;
	}
	else
	    local_count = local_count + pg_cnt;

	local_data = local_data + pg_cnt * page_size;
	local_page = local_page + pg_cnt;
	*count = *count - pg_cnt;
	if (*count == 0)
	    break;    

    }
    *count = local_count;
    if (s == OK)
	return (E_DB_OK);

    return (E_DB_ERROR);
}

/*{
** Name: dm2f_set_alloc 	- Set current allocation size of deferred file
**
** Description:
**	The MODIFY code sometimes does tricky things like destroy a file, and
**	rename a new file to be the name of the destroyed file, and then builds
**	a new FCB on top of the file. When the new file is a deferred I/O file,
**	we have the problem that we don't know how to set the deferred
**	allocation amount for the file. So we require the caller to set the
**	value by calling this routine.
**
**	Sometimes, the file starts out as a deferred I/O file, but by the time
**	the modify operation completes, the deferred I/O has spilled over and
**	created a real file. In this case, the modify code will have closed the
**	file and then re-opened it. When a file is closed and re-opened, it
**	may lose any unwritten allocation. Therefore, this allocation must be
**	restored.
**
** Inputs:
**      loc                             Pointer to a location array.
**      l_count                         Count of number of locations.
**      count                           Number of pages in the file..
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
**	4-apr-1992 (bryanp)
**	    Created for deferred I/O project.
**	3-Oct-1992 (bryanp)
**	    Fixed use of err_code.
**	2-feb-93 (jnash)
**	    Remove "&" from tbio_dbname and tbio_relid references.  These are
**	    pointers.
**	10-dec-1993 (rmuth)
**	    Sense returns last used pageno but alloc requires the number
**	    of pages. Wierd but true, this showed up as an end-of-file
**	    bug as we allocated one less page than needed and DIwrite 
**	    returned an error.
*/
VOID
dm2f_set_alloc(
DMP_LOCATION        *loc,
i4             l_count,
i4             count)
{
    i4         err_code;
    DB_DB_NAME      *db_name;
    DB_TAB_NAME     *tbl_name;
    i4         last_page;
    i4         pages_needed;
    DB_ERROR   local_dberr;

    if (loc[0].loc_fcb->fcb_state & FCB_DEFERRED_IO)
    {
	loc[0].loc_fcb->fcb_deferred_allocation = count;
    }
    else
    {
	/*
	** sense the file. if it doesn't have "count" pages, then allocate
	** enough pages so that it does.
	*/
	if (loc[0].loc_fcb->fcb_tcb_ptr != 0)
	{
	    db_name = (DB_DB_NAME *)loc[0].loc_fcb->fcb_tcb_ptr->tbio_dbname;
	    tbl_name = (DB_TAB_NAME *)loc[0].loc_fcb->fcb_tcb_ptr->tbio_relid;
	}
	else
	{
	    db_name = (DB_DB_NAME *)"UNKNOWN DB";
	    tbl_name = (DB_TAB_NAME *)"UNKNOWN TBL";
	}
	if (dm2f_sense_file(loc, l_count, tbl_name, db_name, &last_page,
			    &local_dberr) == E_DB_OK)
	{
	    if (last_page < (count - 1))
	    {
		pages_needed = (count - 1) - last_page;
		last_page++;
		if (dm2f_alloc_file(loc, l_count, tbl_name, db_name,
				    pages_needed, &last_page,
				    &local_dberr) != E_DB_OK)
		{
		    uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
				&err_code, 0);
		}
	    }
	}
	else
	{
	    uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL,
				&err_code, 0);
	}
    }

    return;
}

/*
** Name: materialize_fcb	- transform file from deferred I/O state
**
** Description:
**	The first call to dm2f_write_file on a file which is in deferred I/O
**	state causes that file to be automatically materialized. The steps
**	involved in materializing the fcb are:
**
**	    1) turn off the FCB_DEFERRED_IO flag
**	    2) create the underlying physical file(s).
**	    3) open the underlying physical file(s).
**	    4) perform the disk space allocation which was deferred
**	    5) flush the deferred allocation to disk.
**
**	Currently, deferred I/O is only used for temporary tables. This
**	assumption is used in this routine in that we pass neither
**	DI_SYNC_FLAG nor DI_ZERO_ALLOC to the open file code.
**
** Inputs:
**      loc                             Pointer to a location array.
**      l_count                         Count of number of locations.
**      page_size                       Page size
**      tab_name                        Table name for logging.
**      db_name                         Database name for logging.
**	page_fmt			Accessors for page access.
**
** Outputs:
**      err_code                        Reason for error return status.
**
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
**	16-jan-1992 (bryanp)
**	    Created for the deferred I/O project.
**	18-jun-1992 (bryanp)
**	    Pass DI_ZERO_ALLOC when materializing a temp table so that
**	    the deferred allocation is made real on Unix systems.
**	09-jun-1992 (kwatts)
**	    Extended interface with page_fmt parameter to get at accessors.
**	22-sep-1992 (bryanp)
**	    Revert 18-jun-1992 change; we should no longer need DI_ZERO_ALLOC
**	    for temporary tables.
**	    Call alloc_file rather than init_file when materializing table.
**	    This means page_fmt is no longer needed; remove it.
**	    Don't need dm2f_open_file since dm2f_create_file opens the file.
**	26-oct-1992 (rogerk)
**	    Initialize 'page' parameter to dm2f_alloc_file.
**      06-mar-96 (stial01)
**          Added page size parameter to support variable page size temps
**	7-Mar-2007 (kschendel)
**	    Set direct IO for temp-table files according to config.
*/
static DB_STATUS
materialize_fcb(
DMP_LOCATION	*loc,
i4		l_count,
i4		page_size,
DB_TAB_NAME	*tab_name,
DB_DB_NAME	*db_name,
DB_ERROR	*dberr)
{
    DB_STATUS	status;
    i4		page = 0;
    i4		di_flags = 0;
    i4		*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    dm0s_mlock(&loc[0].loc_fcb->fcb_mutex);

    if ((loc[0].loc_fcb->fcb_state & FCB_DEFERRED_IO) == 0)
    {
	/*
	** We started materializing while someone else was materializing, and
	** they're now done, so we don't need to do anything, so just return.
	*/
	dm0s_munlock(&loc[0].loc_fcb->fcb_mutex);
	return (E_DB_OK);
    }

    do
    {
	loc[0].loc_fcb->fcb_state |= FCB_MATERIALIZING;

	/* It would be nice to set PRIVATE for temp-table files, but
	** that falls down with parallel query.
	** Do however ask for direct I/O, but only if load files also use
	** direct IO.
	*/
	if (dmf_svcb->svcb_directio_load)
	    di_flags |= DI_DIRECTIO_MASK;
	status = dm2f_create_file(loc[0].loc_fcb->fcb_locklist, 
				loc[0].loc_fcb->fcb_log_id, loc, l_count,
				page_size, (i4)0,
				(DM_OBJECT *)NULL,
				dberr);
	if (status)
	    break;

	/*
	** By passing the page count as a negative value, we indicate to
	** dm2f_alloc_file that this we are materializing this file, so it
	** won't try to acquire the mutex on the FCB (which would fail, since
	** we own the mutex in this subroutine).
	*/
	page = 0;
	status = dm2f_alloc_file(loc, l_count,
				tab_name, db_name,
				- loc[0].loc_fcb->fcb_deferred_allocation,
				&page, dberr);
	if (status)
	    break;

	status = dm2f_flush_file(loc, l_count,
				tab_name, db_name,
				dberr);
	if (status)
	    break;

	loc[0].loc_fcb->fcb_state &= ~(FCB_DEFERRED_IO | FCB_MATERIALIZING);

	dm0s_munlock(&loc[0].loc_fcb->fcb_mutex);

	return (E_DB_OK);
    } while (0);   

    dm0s_munlock(&loc[0].loc_fcb->fcb_mutex);

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM933F_DM2F_WRITE_ERROR);
    }
    return (status);
}

/*{
** Name: dm2f_add_fcb - fill in location desription in unused slot of location
**			array.
**
** Description:
**
** Inputs:
**	lock_list			Transaction lock list (used to reclaim
**					tcb's if a file quota is reached).
**      loc                             The LOC array of files.
**      page_size                       Page size
**      l_count                         The number of locations.
**      flag				Flag indicating action required:
**						DM2F_OPEN : open file after
**							    slot is filled in.
**	table_id			Table Id of table which owns the
**					location array.  Used to build file
**					names for each location.
**	sync_flag			set in DCB, and passed into by caller -
**					value is system specific, and is passed
**					without interpretation to DIopen().
**
** Outputs:
**      err_code                        Reason for error status.
**					Which includes one of the following
**                                      error codes:
**					E_DM0112_RESOURCE_QUOTA_EXCEED
**                                      E_DM9336_DM2F_BUILD_ERROR
**				        E_DM0114_FILE_NOT_FOUND
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
**	26-oct-92 (rogerk)
**	    Reduced Logging Project: Added support for partially open tables.
*/
DB_STATUS
dm2f_add_fcb(
i4		   lock_list,
i4		   log_id,
DMP_LOCATION       *loc,
i4            page_size,
i4            l_count,
i4		   flag,
DB_TAB_ID          *table_id,
u_i4	   sync_flag,
DB_ERROR           *dberr)
{
    DB_STATUS		    status = E_DB_OK;
    DMP_FCB		    *f;
    DMP_LOCATION            *l;
    i4		    k;
    char		    sem_name[CS_SEM_NAME_LEN];
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** Fill in any unitialized FCB's which are now described in the
    ** location array.
    */
    for (k=0; k < l_count; k++)
    {
	l = &loc[k];
    	f = l->loc_fcb;

	if ((f->fcb_state == FCB_NOTINIT) && (l->loc_status & LOC_VALID))
	{
	    f->fcb_state = FCB_CLOSED;
	    f->fcb_namelength = 12;
	    f->fcb_last_page = -1;
	    f->fcb_deferred_allocation = 0;
	    f->fcb_location = l->loc_ext;
	    dm2f_filename(DM2F_TAB, table_id, l->loc_id, 0, 0,
		&f->fcb_filename);
	    dm0s_name(&f->fcb_mutex,
			STprintf(sem_name, "FCB %.*s", f->fcb_namelength, 
				 f->fcb_filename.name));
	}
    }

    /*
    ** Open any new initialized locations which have not been previously opened.
    */
    if (flag & DM2F_OPEN)
    {
	/* Make sure we set direct-IO request, same as build-fcb would */
	if ((sync_flag & (DI_USER_SYNC_MASK | DI_SYNC_MASK)
	     && dmf_svcb->svcb_directio_tables)
	  || ((sync_flag & (DI_USER_SYNC_MASK | DI_SYNC_MASK)) == 0
	      && dmf_svcb->svcb_directio_load) )
	    sync_flag |= DI_DIRECTIO_MASK;
	status = dm2f_open_file(lock_list, log_id, loc, l_count, 
		page_size, DM2F_A_WRITE, 
		sync_flag, (DM_OBJECT *)f, dberr);
    }

    if (status == E_DB_OK)
	return (E_DB_OK);

    if (dberr->err_code == E_DM9291_DM2F_FILENOTFOUND) 
	SETDBERR(dberr, 0, E_DM0114_FILE_NOT_FOUND);

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM928F_DM2F_ADD_FCB_ERROR);
    }

    return (E_DB_ERROR);
}

/*{
** Name: dm2f_check_access - check whether the passed in location array allows
**				access to the location requested.
**
** Description:
**
** Inputs:
**      loc                             The LOC array of files.
**      l_count                         The number of locations.
**	page_number			The page number we wish to access.
**
** Outputs:
**	Returns:
**	    TRUE			The specified page can be accessed.
**	    FALSE			The specified page can't be accessed.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-oct-92 (rogerk)
**	    Reduced Logging Project: Added support for partially open tables.
*/
bool
dm2f_check_access(
DMP_LOCATION       *loc,
i4            l_count,
DM_PAGENO	   page_number)
{
    DMP_LOCATION            *l;
    i4		    extent;

    /* 
    ** For multiple locations must figure out on which file the page resides.
    */
    l = &loc[0];
    if (l_count > 1)
    {
	extent = page_number >> DI_LOG2_EXTENT_SIZE;
	l = &loc[extent % l_count];
    }

    /*
    ** Return location open status.
    */
    return ((l->loc_fcb->fcb_state & FCB_OPEN) != 0);
}

/*{
** Name: dm2f_galloc_file	- Allocate space for a database file 
**			          guaranteing that the space will exist.
**
** Description:
**	This routine is juts a wrapper for the allocate_to_file
**	routine. It calls this routine requesting that the space
**	be guaranteed on disc.
**
** Inputs:
**      loc                             Pointer to a location array.
**      l_count                         Count of number of locations.
**      tab_name                        Table name for error logging.
**      db_name                         Database name for error logging.
**      count                           Number of pages to allocate.
**      page                            Pointer to a variable which must
**					hold the page to allocate from.
**
** Outputs:
**      page                            Pointer to a variable to return the
**                                      last page number allocated.
**      err_code                        Reason for error return status.
**					Which includes one of the following:
**					E_DM0112_RESOURCE_QUOTA_EXCEED
**					E_DM92CF_DM2F_GALLOC_ERROR
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
**	30-October-1992 (rmuth)
**	    Created.
*/
DB_STATUS
dm2f_galloc_file(
DMP_LOCATION        *loc,
i4             l_count,
DB_TAB_NAME         *tab_name,
DB_DB_NAME          *db_name,
i4		    count,
i4             *page,
DB_ERROR	    *dberr)
{
    DB_STATUS	status;

    status = allocate_to_file( DM2F_GUARANTEE_SPACE,
			       loc, l_count,
			       tab_name, db_name,
			       count, page,
			       dberr );
    return( status );
}


/*{
** Name: dm2f_guarantee_space	- Guarantee that disc space for the page
**			 	  range passed will exist when needed.
**				  
** Description:
**	This routine guarantees that the disc space for the page range
**	specified will exist when needed.  It is mainly used during
**	a table build process where we first dm2f_alloc space for the
**	file, then dm2f_write a portion of that file, and then if
**	there is any space which has been dm2f_alloced but not 
**	dm2f_written we dm2f_guarantee those pages.
**
**	If the file is in deferred I/O state we just return without
**	guaranteing the space.
**
**	Note: I could have used the code for dm2f_write_file to do
**	      the same thing, but it looks like this code relies
**	      on DI_EXTENT_SIZE being a power of 2, we may well
**	      change this in the future hence this code. Besides
**	      i was too brain dead today to work out what it was 
**	      doing.
**
** Inputs:
**      loc                             Pointer to a location array.
**      l_count                         Count of number of locations.
**      tab_name                        Table name for error logging.
**      db_name                         Database name for error logging.
**	start_page			Page number to start the guarantee
**					process.
**	number_of_pages			Number of pages to guarantee, this
**					should include the start_pageno.
**
** Outputs:
**      err_code                        Reason for error return status.
**					Which includes one of the following:
**					E_DM0112_RESOURCE_QUOTA_EXCEED
**					E_DM92CF_DM2F_GUARANTEE_ERROR
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
**	30-October-1992 (rmuth)
**	    Created.
**	06-Apr-1999 (jenjo02)
**	    Don't ignore raw locations - though preallocated,
**	    the pages still need to be initialized.
*/
DB_STATUS
dm2f_guarantee_space(
DMP_LOCATION        *loc,
i4             l_count,
DB_TAB_NAME         *tab_name,
DB_DB_NAME          *db_name,
i4		    start_page,
i4             number_of_pages,
DB_ERROR	    *dberr)
{
    DMP_LOCATION        *l;
    DMP_DCB             *dcb;
    CL_ERR_DESC		sys_err;
    STATUS		s = OK;
    i4             guarantee_cnt= 0;
    i4             local_page;
    i4		stripe_size;
    i4		full_stripes_to_start;
    i4		full_stripes_to_end;
    i4		stripe_remainder_to_start;
    i4		stripe_remainder_to_end;
    i4		location_start_page;
    i4             e_add;
    i4             s_add;
    i4             k;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** If we do not need to guarantee any space on disc then just return
    */
    if ((loc[0].loc_fcb->fcb_state & FCB_DEFERRED_IO) || ( number_of_pages==0) )
    {
	return (E_DB_OK);
    }


    /*
    ** If a multi-location table then we need to spread the
    ** allocation across all locations so set up some
    ** information. Optimise the code so we only have to
    ** call DIguarantee_space once per location.
    */
    if (l_count > 1 )
    {
	stripe_size = l_count * DI_EXTENT_SIZE;

	full_stripes_to_start = (start_page ) / stripe_size;

	full_stripes_to_end   = (start_page + number_of_pages) / stripe_size;

	stripe_remainder_to_start = 
		start_page - (full_stripes_to_start * stripe_size );

	stripe_remainder_to_end = (start_page + number_of_pages) - 
				   	( full_stripes_to_end  * stripe_size );
    }

    /*
    ** Cycle through all locations guaranteeing the space
    */
    for (k = 0; k < l_count; k++)
    {
	if (l_count > 1)
	{
	    /* 
	    ** For multiple locations must figure out which files 
	    ** DIguarantee_space should be called on.
	    */
           
	    if ( stripe_remainder_to_end < DI_EXTENT_SIZE)
		e_add = stripe_remainder_to_end;
	    else 
		e_add = DI_EXTENT_SIZE;

	    if ( stripe_remainder_to_start < DI_EXTENT_SIZE)
		s_add = stripe_remainder_to_start;
	    else 
		s_add = DI_EXTENT_SIZE;

	    guarantee_cnt = ((full_stripes_to_end * DI_EXTENT_SIZE) + e_add) - 
                            ((full_stripes_to_start * DI_EXTENT_SIZE) + s_add);      

	    location_start_page = (full_stripes_to_start * DI_EXTENT_SIZE) +
									s_add;

	    l = &loc[k];

	    stripe_remainder_to_end   = stripe_remainder_to_end - e_add;
	    stripe_remainder_to_start = stripe_remainder_to_start - s_add;
	}
	else
	{
	    /*
	    ** Single location table
	    */
	    l 		        = &loc[0];
	    location_start_page = start_page;
	    guarantee_cnt 	= number_of_pages;	    
	}

	/*
	** If need to guarantee space to this location then do it
	*/
	if (guarantee_cnt != 0)
	{
	    /*
	    ** RAW locations contain preallocated space which does
	    ** not need to be initialized.
	    */
	    if (l->loc_status & LOC_RAW)
	    {
		/*
		** Check for extending beyond end of raw file.
		** This should not be possible due to similar check in
		** allocate_to_file, which should have preceded this.
		*/
		if (location_start_page + guarantee_cnt > l->loc_fcb->fcb_rawpages )
		{
		    s = DI_EXCEED_LIMIT;
		    SETCLERR(&sys_err, 0, 0);

		    uleFormat(NULL, E_DM0194_BAD_RAW_PAGE_NUMBER, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, err_code, 6,
			sizeof("Guarantee")-1, "Guarantee",
			0, location_start_page + guarantee_cnt,
			sizeof(l->loc_name), l->loc_name,
			0, l->loc_fcb->fcb_rawstart,
			0, l->loc_fcb->fcb_physical_allocation,
			0, l->loc_fcb->fcb_rawpages);
		}
	    }
	    else
	    {
		s = DIguarantee_space( l->loc_fcb->fcb_di_ptr, 
				       location_start_page,
				       guarantee_cnt, 
				       &sys_err); 
	    }

	    if (s != OK)
		break;

	    number_of_pages = number_of_pages - guarantee_cnt;

	    /* Preceding allocate_to_file updated fcb_physical_allocation */

	    if (number_of_pages == 0)
		break;
	}
    }

    if (s == OK)
    {	
	return (E_DB_OK);
    }
	
    /*
    ** Handle errors. 
    */
    uleFormat(NULL, s, &sys_err, ULE_LOG, NULL,NULL,0,NULL,err_code,0);

    uleFormat(NULL,
	    E_DM9000_BAD_FILE_ALLOCATE, &sys_err, ULE_LOG, NULL,
	    0, 0, 0, err_code, 4,
	    sizeof(DB_DB_NAME), db_name,
	    sizeof(DB_TAB_NAME), tab_name,
	    l->loc_ext->phys_length, 
	    &l->loc_ext->physical,
	    l->loc_fcb->fcb_namelength, 
	    &l->loc_fcb->fcb_filename);

    if (s == DI_EXCEED_LIMIT)
	SETDBERR(dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
    else
	SETDBERR(dberr, 0, E_DM92CE_DM2F_GUARANTEE_ERR);

    return (E_DB_ERROR);
}

/*{
** Name: allocate_to_file	- Allocate space for a database file.
**
** Description:
**      This routine allocates the pages specified on the database 
**      files associated with the locations specified by the location array.  
**
**	If the file is in deferred I/O state, then the allocation is not
**	performed at this time; it is deferred until the file is actually
**	materialized. The allocation amount is remembered in the FCB.
**
**	When materializing the file, this routine is called to actually perform
**	the deferred allocation. It can tell that it's called from the
**	materialize_fcb() routine because the page count is negative (ugh! I
**	should really add a new flag argument to convey this information...).
**
** Inputs:
**	flag				Indicates if space needs to be
**					guranteed or not.
**      loc                             Pointer to a location array.
**      l_count                         Count of number of locations.
**      tab_name                        Table name for error logging.
**      db_name                         Database name for error logging.
**      count                           Number of pages to allocate.
**      page                            Pointer to value that gives the next
**					page number at which spot the table
**					should be extended.
**
** Outputs:
**      page                            Pointer to a variable to 
**                                      return last page allocated.
**      err_code                        Reason for error return status.
**					Which includes one of the following:
**					E_DM0112_RESOURCE_QUOTA_EXCEED
**					E_DM9339_DM2F_ALLOC_ERROR
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
**	30-October-1992 (rmuth)
**	    Created from rogerk's dm2f_alloc_file.
**	17-dec-92 (rogerk & jnash)
**	    Put in consistency check to ensure that dm2f_sense was properly
**	    called before attempting to allocate the table.
**	02-feb-95 (cohmi01)
**	    For RAW locations, never actually call DI allocation, as the
**	    raw file is as big as its going to get. Handle LOC_RAWINIT as a 
**	    NOOP, any other request is treated as an out-of-space condition.
**	12-sep-95 (nick)
**	    Corrected comments - we take the next page number to allocate 
**	    from ( i.e. the actual number of pages physically already there )
**	    and return the last page number allocated ( not the number of
**	    pages in the table )
**	27-dec-1995 (dougb)
**	    Porting change:  Make all references to RAW I/O specific to the
**	    supported Unix platforms.
**	18-jun-1996 (jenjo02)
**	    Lock fcb_mutex when extending a file to prevent concurrency 
**	    problems (allocate_to_file()). Without the mutex protection, 
**	    FHDR/FMAPs can be corrupted and multiple servers may
**	    encounter write-past-end-of-file (DM93AF) errors in the 
**	    WriteBehind threads.  If count<0, fcb_mutex is already held, 
**	    so note this fact and leave the mutex locked. This is an ugly hack.
**	23-Apr-2001 (jenjo02)
**	    There is no compelling reason to initialize RAW space, but
**	    check that this extension does not break raw bounds.
*/
static
DB_STATUS
allocate_to_file(
i4		    flag,
DMP_LOCATION        *loc,
i4             l_count,
DB_TAB_NAME         *tab_name,
DB_DB_NAME          *db_name,
i4		    count,
i4             *page,
DB_ERROR	    *dberr)
{
    STATUS		s = OK;
    DMP_LOCATION        *l;
    i4             alloc_cnt;
    i4             local_page;
    i4             k;
    i4             newsize;
    i4             file_size;
    i4             num_extents;
    i4             minimum_pgs;
    i4             remainder_exts;
    i4             remainder_pgs;
    i4			mutex_held = (count < 0);
    CL_ERR_DESC		sys_err;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (mutex_held == 0)
	dm0s_mlock(&loc[0].loc_fcb->fcb_mutex);

    if ((loc[0].loc_fcb->fcb_state & FCB_DEFERRED_IO) != 0 && count > 0)
    {
	if ((loc[0].loc_fcb->fcb_state & (FCB_DEFERRED_IO | FCB_MATERIALIZING))
					== FCB_DEFERRED_IO)
	{
	    loc[0].loc_fcb->fcb_deferred_allocation += count;
	    *page += (count - 1);
	    if (mutex_held == 0)
		dm0s_munlock(&loc[0].loc_fcb->fcb_mutex);
	    return (E_DB_OK);
	}
    }

    if (count < 0)
	count = (0 - count);

    /*
    ** Consistency Check:  Ensure that the location array's physical allocation
    ** values have been properly initialized by dm2f_sense_file.  If the
    ** caller has specified a non-empty table, then ensure that at least
    ** the first location is listed as having a physical allocation.  If
    ** the physical allocation of the first location is zero, then the caller
    ** has likely not sensed the end of file properly.
    */
    if ((*page > 0) && (loc[0].loc_fcb->fcb_physical_allocation == 0))
    {
        uleFormat(NULL, E_DM9053_INCONS_ALLOC, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 3,
            loc[0].loc_fcb->fcb_namelength, &loc[0].loc_fcb->fcb_filename,
	    0, *page, 
	    0, loc[0].loc_fcb->fcb_physical_allocation);
	if ( flag & DM2F_GUARANTEE_SPACE )
	    SETDBERR(dberr, 0, E_DM92CF_DM2F_GALLOC_ERROR);
	else
	    SETDBERR(dberr, 0, E_DM9339_DM2F_ALLOC_ERROR);

	if (mutex_held == 0)
	    dm0s_munlock(&loc[0].loc_fcb->fcb_mutex);
	return (E_DB_ERROR);
    }

    /*
    ** Need to figure out the number of page to allocate to each file partition.
    ** To do this, we initially ignore the current distribution of pages
    ** accross the partitions and instead just focus on the resulting size
    ** after the allocation.
    **
    ** For each location we determine the number of pages which would need
    ** to be in that location to make up a table of exactly the requested
    ** size: current size + allocation request (or (*page + count)).  We
    ** then allocate the required pages to bring each location up to its
    ** desired size.
    */

    /*
    ** The following are used in the allocation loop to determine the
    ** size requirements for each partition:
    **
    **     newsize		- The size the file will be after the allocation
    **     num_extents		- Pages are allocated to partitions in extents
    **                            of DI_EXTENT_SIZE.  This value gives the
    **                            number of complete extent blocks in the new
    **                            file.  This value is used only temporarily
    **                            to help calculate the subsequent values.
    **     minimum_pgs		- As tables are allocated, space is added to
    **                            each partition evenly.  All files of a table
    **                            partition should contain roughly the same
    **                            number of pages - within DI_EXTENT_SIZE of
    **                            each other.  This value gives the minimum
    **                            number of pages that each file MUST have in
    **                            order to make up a file of 'newsize'.
    **                            Each file of the new table will have at least
    **                            'minimum_pgs' and may have more.
    **     remainder_exts	- As stated just above, each partition of the
    **                            new table will have at least 'minimum_pgs' 
    **                            pages.  This value gives the number of files
    **                            in the location group which will have one
    **                            more full extent beyond 'minimum_pgs'.
    **     remainder_pgs	- This value gives the number of pages (less
    **                            than a full DI_EXTENT_SIZE) to allocate to
    **                            the last partition in order to make up
    **                            'newsize' pages.
    **
    ** As each location is processed, its resultant file size is determined
    ** by assigning to it 'minimum_pgs' pages plus another full DI_EXTENT if
    ** its location number falls into the range of 'remainder_exts' or just
    ** the 'remainder_pgs' amount if the location number is the next location
    ** past 'remainder'exts'.
    **
    ** Before actually allocating pages, the current physical allocation of
    ** the location is checked.  The allocation request will be the difference
    ** between the desired file size and its current allocation.
    */
    newsize = *page + count;	/* final # of pages */
    num_extents = newsize / DI_EXTENT_SIZE;
    minimum_pgs = (num_extents / l_count) * DI_EXTENT_SIZE;
    remainder_exts = num_extents % l_count;
    remainder_pgs = newsize % DI_EXTENT_SIZE;

    for (k = 0;k < l_count;k++)
    {
	l = &loc[k];

	file_size = minimum_pgs;
	if (k < remainder_exts)
	    file_size += DI_EXTENT_SIZE;
	else if (k == remainder_exts)
	    file_size += remainder_pgs;

	/* 
	** NB. fcb_physical_allocation is the # of pages not 
	** the last page #
	*/
	alloc_cnt = file_size - l->loc_fcb->fcb_physical_allocation;

	if (alloc_cnt > 0)
	{
	    /*
	    ** Verify that this location is currently open.  We cannot
	    ** allocate space in a non-open file of a partially open table.
	    */
	    if ((l->loc_fcb->fcb_state & FCB_OPEN) == 0)
	    {
		s = FAIL;
		SETDBERR(dberr, 0, E_DM9339_DM2F_ALLOC_ERROR);
		break;
	    }

	    /*
	    ** RAW space is preallocated and does not need to be
	    ** initialized, but we'll go ahead and check for 
	    ** extending beyond raw location's bounds.
	    */
	    if (l->loc_status & LOC_RAW)
	    {
		if ( l->loc_fcb->fcb_physical_allocation + alloc_cnt >
		     l->loc_fcb->fcb_rawpages )
		{
		    s = DI_EXCEED_LIMIT;
		    SETCLERR(&sys_err, 0, 0);

		    uleFormat(NULL, E_DM0194_BAD_RAW_PAGE_NUMBER, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, err_code, 6,
			sizeof("Allocate")-1, "Allocate",
			0, l->loc_fcb->fcb_physical_allocation + alloc_cnt,
			sizeof(l->loc_name), l->loc_name,
			0, l->loc_fcb->fcb_rawstart,
			0, l->loc_fcb->fcb_physical_allocation,
			0, l->loc_fcb->fcb_rawpages);
		}
	    }
	    else
	    {
		/*
		** Select the type of cooked allocation needed
		*/
		if ( flag & DM2F_GUARANTEE_SPACE )
		{
		    s = DIgalloc( l->loc_fcb->fcb_di_ptr, alloc_cnt, 
				  &local_page, &sys_err); 
		}
		else if ( flag & DM2F_NOT_GUARANTEE_SPACE )
		{
		    s = DIalloc( l->loc_fcb->fcb_di_ptr, alloc_cnt, 
			     &local_page, &sys_err); 
		}
		else
		{
		    s = FAIL;
		    SETDBERR(dberr, 0, E_DM9339_DM2F_ALLOC_ERROR);
		    break;
		}
	    }

	    if (s != OK)
	    {
                uleFormat(NULL, s, &sys_err, ULE_LOG,NULL,NULL,0,NULL,err_code,0);
		uleFormat(NULL, E_DM9000_BAD_FILE_ALLOCATE, &sys_err, ULE_LOG, NULL,
		    0, 0, 0, err_code, 4,
		    sizeof(DB_DB_NAME), db_name,
		    sizeof(DB_TAB_NAME), tab_name,
		    l->loc_ext->phys_length, 
		    &l->loc_ext->physical,
		    l->loc_fcb->fcb_namelength, 
		    &l->loc_fcb->fcb_filename);
		/*
		** Set the errcode accordingly
		*/
		if (s == DI_EXCEED_LIMIT)
		    SETDBERR(dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
		else
		    if ( flag & DM2F_GUARANTEE_SPACE )
			SETDBERR(dberr, 0, E_DM92CF_DM2F_GALLOC_ERROR);
		    else
			SETDBERR(dberr, 0, E_DM9339_DM2F_ALLOC_ERROR);
		break;
	    }
	    l->loc_fcb->fcb_physical_allocation += alloc_cnt;

	    /*
	    ** Guaranteeing space does an implicit flush so do not
	    ** need to set the flush flag.
	    */
	    if ( flag & DM2F_NOT_GUARANTEE_SPACE )
	    {
	        l->loc_fcb->fcb_state |= FCB_FLUSH;
	    }
	}
    }

    if (s == OK)
    {	
	/*
	** Calculate the new logical file size given the current distribution
	** of physical file sizes among the partitions.
	*/
	calculate_allocation(loc, l_count, &newsize);

	/* 
	** this is the # of pages in the logical file hence 
	** adjust to return the last page #
	*/
	*page = newsize - 1;
	if (mutex_held == 0)
	    dm0s_munlock(&loc[0].loc_fcb->fcb_mutex);
	return (E_DB_OK);
    }
	
    /*	Handle errors. */

    if (mutex_held == 0)
	dm0s_munlock(&loc[0].loc_fcb->fcb_mutex);
    return (E_DB_ERROR);
}

/*{
** Name: dm2f_round_up - Rounds up to the next factor DI_EXTENT_SIZE.
**
** Description:
**	This routine rounds the value passed in to the next factor
**	of DI_EXTENT_SIZE. Used on multi-location table to make sure
**	that their allocation and extend sizes are factors of
**	DI_EXTENT_SIZE. 
**
** Inputs:
**	value			The value to be rounded.
**
** Outputs:
**	value			The rounded value;
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	21-June-1993 (rmuth)
**	    Created.
*/
VOID
dm2f_round_up(
i4            *value)
{
    /*
    ** If not a multiple oF DI_EXTENT_SIZE then round up
    */
    if (((*value / DI_EXTENT_SIZE ) * DI_EXTENT_SIZE ) != *value)
    {
	*value = (( *value / DI_EXTENT_SIZE ) + 1 ) *  DI_EXTENT_SIZE;
    }

    return; 
}

/*{
** Name: dm2f_write_list	- Gather Write a database file.
**
** Description:
**      This routine writes the pages specified to the database 
**      files associated with the locations specified by the location array.  
**
**	The first dm2f_write_list call to a file in deferred I/O state causes
**	the file to be automatically materialized. The underlying physical
**	file(s) are created, and any deferred disk space allocation is
**	performed.
**
** Inputs:
**      loc                             Pointer to a location array.
**      l_count                         Count of number of locations.
**	page_size			Page size 
**      tab_name                        Table name for logging.
**      db_name                         Database name for logging.
**      count                           Number of pages to write.
**      page                            Page to start writing to.
**      data                            Pointer to an area to write out.
**      evcomp				Callers completion handler.
**      closure				Ptr to completion data.
**
** Outputs:
**      err_code                        Reason for error return status.
**					E_DM933F_DM2F_WRITE_ERROR
**					E_DM9335_DM2F_ENDFILE
** Returns:
**	    E_DB_OK
**	    E_DB_ERROR
** Exceptions:
**     none
**
** Side Effects:
**     none
**
** History:
**      31-oct-1996 (nanpr01 for ICL itb)
**	    Added for gather write.
**	31-oct-1996 (nanpr01 for ICL itb)
**	    Copied a change from dm2f_write_file to dm2f_write_list for
**	    raw files.  These functions should be largely identical.
**	09-Apr-2001 (jenjo02)
**	    Added gw_io, gw_pages parms to dm2f_write_list(), DIgather_write()
**	    prototypes.
**	15-Mar-2010 (smeke01) b119529
**	    DM0P_BSTAT fields bs_gw_pages and bs_gw_io are now u_i8s.
*/
DB_STATUS
dm2f_write_list(
DMP_CLOSURE	*clo,
char		*di,
DMP_LOCATION    *loc,
i4             	l_count,
i4		page_size,
DB_TAB_NAME     *tab_name,
DB_DB_NAME      *db_name,
i4              *count,
i4              page,
char		*data,
VOID            (*evcomp)(),
PTR    		closure,
i4		*queued_count,
u_i8		*gw_pages,
u_i8		*gw_io,
DB_ERROR	*dberr)
{
    CL_ERR_DESC		sys_err;
    DB_STATUS		status;
    STATUS		s = OK;
    DMP_LOCATION        *l;
    char                *local_data;
    i4             	pg_cnt;
    i4             	extent;
    i4             	pageno;    
    i4             	local_count;
    i4             	local_page;
    i4			cnt_written;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (loc[0].loc_fcb->fcb_state & FCB_DEFERRED_IO)
    {
	status = materialize_fcb(loc, l_count, page_size, tab_name, db_name, 
				 dberr);
	if (status != E_DB_OK)
	{
	    if (dberr->err_code > E_DM_INTERNAL)
	    {
		uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		SETDBERR(dberr, 0, E_DM933F_DM2F_WRITE_ERROR);
	    }
	    return (status);
	}
    }

    local_data = data;
    local_count = 0;
    local_page = page;    

    for (;;)
    {
	/* For multiple locations must figure out how to 
        ** determine which files the write should be 
        ** done on. */
	extent = local_page >> DI_LOG2_EXTENT_SIZE;
	l = &loc[extent % l_count];
	
	if (l_count > 1)
	{
	    pg_cnt = DI_EXTENT_SIZE - (local_page & (DI_EXTENT_SIZE - 1)) ;
	    pageno = (extent/l_count)*DI_EXTENT_SIZE +
		    (local_page & (DI_EXTENT_SIZE -1));
	    if (*count < pg_cnt)
		pg_cnt = *count;
	}
	else
	{
	    pg_cnt = *count;
	    pageno = local_page;
	}

	/*
	** Verify that this location is open.
	*/
	if ((l->loc_fcb->fcb_state & FCB_OPEN) == 0)
	{
	    s = FAIL;
	    SETDBERR(dberr, 0, E_DM933F_DM2F_WRITE_ERROR);
	    break;
	}

	/* Considerations for RAW locations */
	if (l->loc_status & LOC_RAW)
	{
	    /* 
	    ** Writing beyond raw file bounds is unforgivable,
	    ** so write nothing.
	    */
	    if (pageno + pg_cnt > l->loc_fcb->fcb_physical_allocation)
	    {
		s = DI_NODISKSPACE;
		SETCLERR(&sys_err, 0, 0);

		uleFormat(NULL, E_DM0194_BAD_RAW_PAGE_NUMBER, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 6,
		    sizeof("WriteList")-1, "WriteList",
		    0, pageno,
		    sizeof(l->loc_name), l->loc_name,
		    0, l->loc_fcb->fcb_rawstart,
		    0, l->loc_fcb->fcb_physical_allocation,
		    0, l->loc_fcb->fcb_rawpages);
	    }
	    else
	    {
		/* Offset pageno to first page of file */
		pageno += l->loc_fcb->fcb_rawstart;
	    }
        }

	cnt_written = pg_cnt;    /* try to write all */


	if ( s == OK )
	{
	    /* Initialize 1st-level closure structure for dm2f_complete() */
	    clo->clo_l = loc; 
	    clo->clo_db_name = db_name; 
	    clo->clo_tab_name = tab_name; 
	    clo->clo_pageno = pageno; 
	    clo->clo_pg_cnt = pg_cnt; 
	    clo->clo_evcomp = (VOID (*)())evcomp; 
	    clo->clo_data = closure;
	    clo->clo_di = di;

	    s = DIgather_write(DI_QUEUE_LISTIO, clo->clo_di,
		       l->loc_fcb->fcb_di_ptr, &cnt_written, pageno, 
		       local_data, dm2f_complete, (char *)clo, 
		       queued_count, gw_pages, gw_io, &sys_err); 
	}

	if ((s != OK) && (s != DI_ENDFILE))
	{
	    uleFormat(NULL, s, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    if (s == DI_NODISKSPACE)
	    {
		uleFormat(NULL, E_DM9448_DM2F_NO_DISK_SPACE, &sys_err, ULE_LOG,
		    NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 5,
		    sizeof(DB_DB_NAME), db_name,
		    sizeof(DB_TAB_NAME), tab_name,
		    l->loc_ext->phys_length, &l->loc_ext->physical,
		    l->loc_fcb->fcb_namelength, &l->loc_fcb->fcb_filename,
		    0, pageno);
		SETDBERR(dberr, 0, E_DM9448_DM2F_NO_DISK_SPACE);
	    }
	    else
	    {
	    uleFormat(NULL, E_DM9006_BAD_FILE_WRITE, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 5,
		sizeof(DB_DB_NAME), db_name,
		sizeof(DB_TAB_NAME), tab_name,
		l->loc_ext->phys_length, &l->loc_ext->physical,
		l->loc_fcb->fcb_namelength, &l->loc_fcb->fcb_filename,
		0, pageno);
	    }
	    SETDBERR(dberr, 0, E_DM933F_DM2F_WRITE_ERROR);
	    break;
	}

	l->loc_fcb->fcb_state |= FCB_IO;
        if (cnt_written != pg_cnt)
	{
	    local_count = local_count + cnt_written;
	    uleFormat(NULL, E_DM93AF_BAD_PAGE_CNT, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 3,
		0, pageno, 0, pg_cnt, 0, cnt_written);
	    uleFormat(NULL, E_DM9006_BAD_FILE_WRITE, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 5,
		sizeof(DB_DB_NAME), db_name,
		sizeof(DB_TAB_NAME), tab_name,
		l->loc_ext->phys_length, &l->loc_ext->physical,
		l->loc_fcb->fcb_namelength, &l->loc_fcb->fcb_filename,
		0, pageno);
	    SETDBERR(dberr, 0, E_DM9335_DM2F_ENDFILE);
	    break;
	}
	else
	    local_count = local_count + pg_cnt;

	local_data = local_data + pg_cnt * page_size;
	local_page = local_page + pg_cnt;
	*count = *count - pg_cnt;
	if (*count == 0)
	    break;    

    }
    *count = local_count;

    return( (s) ? E_DB_ERROR : E_DB_OK );
}

/*{
** Name: dm2f_complete	- Gather Write completion handler.
**
** Description:
**      This routine writes the pages specified to the database 
**
** Inputs:
**      closure				Ptr to completion data.
**      di_err				DI error status
**	sys_err				Ptr to system error info
**
** Outputs:
**      None					
** Returns:
**	    VOID
** Exceptions:
**     none
**
** Side Effects:
**     Calls callers completion handler if there is one.
**
** History:
**      31-oct-1996 (nanpr01 for ICL itb)
**	    Added for gather write.
**	05-Dec-2008 (jonj)
**	    SIR 120874: pass DB_ERROR * to callback function
*/
VOID
dm2f_complete(
DMP_CLOSURE         *clo,
STATUS              di_err,
CL_ERR_DESC	    *sys_err)
{
    i4             	err_code;
    DB_STATUS		status = ( di_err == OK ) ? E_DB_OK : E_DB_ERROR;
    DB_ERROR		local_dberr;

    CLRDBERR(&local_dberr);

    if ((di_err != OK) && (di_err != DI_ENDFILE))
    {
	uleFormat(NULL, di_err, sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &err_code, 0);
	if (di_err == DI_NODISKSPACE)
	{
	    uleFormat(NULL, E_DM9448_DM2F_NO_DISK_SPACE, sys_err, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, &err_code, 5,
		sizeof(DB_DB_NAME), clo->clo_db_name,
		sizeof(DB_TAB_NAME), clo->clo_tab_name,
		clo->clo_l->loc_ext->phys_length,
		&clo->clo_l->loc_ext->physical,
		clo->clo_l->loc_fcb->fcb_namelength,
		&clo->clo_l->loc_fcb->fcb_filename,
		0, clo->clo_pageno);
	    SETDBERR(&local_dberr, 0, E_DM9448_DM2F_NO_DISK_SPACE);
	}
	else
	{
	    uleFormat(NULL, E_DM9006_BAD_FILE_WRITE, sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &err_code, 5,
		    sizeof(DB_DB_NAME), clo->clo_db_name,
		    sizeof(DB_TAB_NAME), clo->clo_tab_name,
		    clo->clo_l->loc_ext->phys_length,
		    &clo->clo_l->loc_ext->physical,
		    clo->clo_l->loc_fcb->fcb_namelength,
		    &clo->clo_l->loc_fcb->fcb_filename,
		    0, clo->clo_pageno);
	    SETDBERR(&local_dberr, 0, E_DM933F_DM2F_WRITE_ERROR);
	}
    }
    else if (di_err == DI_ENDFILE)
    {
	uleFormat(NULL, E_DM93AF_BAD_PAGE_CNT, sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &err_code, 3,
		0, clo->clo_pageno, 0, clo->clo_pg_cnt, 0, 0);
	uleFormat(NULL, E_DM9006_BAD_FILE_WRITE, sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &err_code, 5,
		sizeof(DB_DB_NAME), clo->clo_db_name,
		sizeof(DB_TAB_NAME), clo->clo_tab_name,
		clo->clo_l->loc_ext->phys_length,
		&clo->clo_l->loc_ext->physical,
		clo->clo_l->loc_fcb->fcb_namelength, 
                &clo->clo_l->loc_fcb->fcb_filename,
		0, clo->clo_pageno);
	SETDBERR(&local_dberr, 0, E_DM9335_DM2F_ENDFILE);
    }

    /* Call 2nd level (caller's) completion handler */

    (clo->clo_evcomp)( status, (PTR)clo->clo_data, &local_dberr);

    return;
}

/*{
** Name: dm2f_check_list   - see if gather write has queued any I/O's
**
** Description:
**	This routine returns the number of outstanding I/O's for
**	this thread.
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**	    i4			number of outstanding I/O's.'s
**
** Exceptions:
**     none
**
** Side Effects:
**     None.
**
** History:
**      31-oct-1996 (nanpr01 for ICL itb)
**	    Added for gather write.
**	15-Mar-2010 (smeke01) b119529
**	    DM0P_BSTAT fields bs_gw_pages and bs_gw_io are now u_i8s.
*/
i4
dm2f_check_list(VOID)
{
    return( DIgather_write(DI_CHECK_LISTIO, 
			 (PTR)0, 
			 (DI_IO *)NULL, 
			 (i4 *)NULL, 
			 (i4)0,
			 (PTR)0, 
			 0, 
			 (PTR)0,
			 (i4*)NULL,
			 (u_i8*)NULL,
			 (u_i8*)NULL,
			 (CL_ERR_DESC *)0) );
}

/*{
** Name: dm2f_force_list	- Wait for I/O to complete
**
** Description:
**      This routine is called when a thread cannot issue any more
**      dm2f_write_list calls without getting the results of the I/O's
**      that the thread has already submitted.
**
** Inputs:
**      None.
**
** Outputs:
**      None					
**
** Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
** Exceptions:
**     none
**
** Side Effects:
**     Suspends the calling thread.
**
** History:
**      31-oct-1996 (nanpr01 for ICL itb)
**	    Added for gather write.
**	15-Mar-2010 (smeke01) b119529
**	    DM0P_BSTAT fields bs_gw_pages and bs_gw_io are now u_i8s.
*/
STATUS
dm2f_force_list(VOID)
{
    CL_ERR_DESC		sys_err;
    i4			err_code;
    STATUS		di_err;

    di_err = DIgather_write(DI_FORCE_LISTIO,
			 (PTR)0, 
			 (DI_IO *)NULL, 
			 (i4 *)NULL, 
			 (i4)0,
			 (PTR)0, 
			 0, 
			 (PTR)0,
			 (i4*)NULL,
			 (u_i8*)NULL,
			 (u_i8*)NULL,
			 &sys_err);

    if (di_err)
	uleFormat(NULL, di_err, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &err_code, 0);

    return(di_err ? E_DB_ERROR : E_DB_OK);
}

/*{
** Name: dm2f_sense_raw	- Return absolute EOF for RAW file
**
** Description:
**	Reads special file in RAW directory to extract
**	total number of DM2F_RAW_BLKSIZE blocks in the
**	raw area. This file and size were placed there
**	when the raw directory was created by mkraware.sh
**
** Inputs:
**	path		Full path name of raw file
**	l_path		length of that path
**	lock_list	Session's lock list.
**
** Outputs:
**	raw_blocks	Where file size, in units of DM2F_RAW_BLKSIZE,
**	     	   	will be returned
**
** Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
** Exceptions:
**     none
**
** Side Effects:
**     none
**
** History:
**	12-Mar-2001 (jenjo02)
**	    Written.
**	03-dec-2001 (somsa01)
**	    Make sure we EOS terminate raw_buf before using it in
**	    CVal().
*/
DB_STATUS
dm2f_sense_raw(
char	*path,
i4	l_path,
i4	*raw_blocks,
i4	lock_list,
DB_ERROR	*dberr)
{
    DB_STATUS   status = E_DB_OK;
    STATUS	cl_status;
    DI_IO	di_context;
    CL_ERR_DESC  sys_err;
    i4		one = 1;
    char	raw_buf[DM2F_RAW_BLKSSIZE+1], *rawblocks = &raw_buf[0];

    CLRDBERR(dberr);

    for (;;)
    {
	cl_status = DIopen(&di_context,
		       path, l_path,
                       DM2F_RAW_BLKSNAME, sizeof(DM2F_RAW_BLKSNAME),
		       (i4)DM2F_RAW_BLKSSIZE+1, DI_IO_READ, 
		       DI_SYNC_MASK, &sys_err);
	if ( cl_status == OK )
	{
	    cl_status = DIread(&di_context, &one, 0, rawblocks, &sys_err);
	    raw_buf[DM2F_RAW_BLKSSIZE] = '\0';
	    if ( cl_status || CVal(rawblocks, raw_blocks) )
	    {
		SETDBERR(dberr, 0, E_DM9007_BAD_FILE_SENSE);
		status = E_DB_ERROR;
	    }
	    else
	    {
		/* Ensure that total blocks is a usable multiple */
		*(raw_blocks) &= ~DM2F_RAW_MINBLKS-1;
	    }

	    DIclose(&di_context, &sys_err);
	}
	else if ( cl_status == DI_EXCEED_LIMIT )
	{
	    status = dm2t_reclaim_tcb(lock_list, (i4)0, dberr);

	    if (status == E_DB_OK)
		continue;
	    if (dberr->err_code == E_DM9328_DM2T_NOFREE_TCB)
		SETDBERR(dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
	}
	else
	{
	    SETDBERR(dberr, 0, E_DM923F_DM2F_OPEN_ERROR);
	    status = E_DB_ERROR;
	}
	break;
    }

    return(status);
}
