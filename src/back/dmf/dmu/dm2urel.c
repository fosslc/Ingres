/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <di.h>
#include    <me.h>
#include    <pc.h>
#include    <sr.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <cs.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <ulf.h>
#include    <sxf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmucb.h>
#include    <dmxcb.h>
#include    <dm.h>
#include    <dml.h>
#include    <dm0m.h>
#include    <dmp.h>
#include    <dm2f.h>
#include    <dm2r.h>
#include    <dm2t.h>
#include    <dm0c.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0p.h>
#include    <dm1p.h>
#include    <cm.h>
#include    <dmse.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dma.h>

/**
**
**  Name: DM2URELOCATE.C - Relocate table utility operation.
**
**  Description:
**      This file contains routines that perform the relocate table
**	functionality.
**	    dm2u_relocate - Relocate a permanent table.
**
**  History:
**	20-may-1989 (Derek)
**	    Split out from DM2U.C
**      22-jun-89 (jennifer)
**          Added checks for security (DAC) to insure that only a user
**          with security privilege is allowed to create or delete 
**          a secure table.  A secure table is indicated by having 
**          relstat include TCB_SECURE.  Also changed the default 
**          definitions for iiuser, iilocations, iiextend, iidbaccess,
**          iiusergroup, iirole and iidbpriv to include TCB_SECURE.
**          The only secure tables are in the iidbdb and include those
**          listed above plus the iirelation of the iidbdb (so that 
**          no-one can turn off the TCB_SECURE state of the other ones).
**	11-sep-89 (rogerk)
**	    Log DMU operation at the end of Relocate operation.
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	19-nov-1990 (bryanp)
**	    Pass correct message parameters to message E_DM9029.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	     4-dec-91 (rogerk)
**		Added support for Set Nologging.
**	15-apr-1992 (bryanp)
**	    Remove "FCB" argument from dm2f_rename()
**	14-apr-1992 (rogerk)
**	    Changed relocate loop to process multiple pages per IO, rather
**	    than acting only one page at a time.  Uses svcb_mwrite_blocks
**	    as the grouping factor.  Also, pages are now allocated from
**	    dm0m rather than off of the stack.
**	6-jul-1992 (bryanp)
**	    Prototyping DMF.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	05-oct-1992 (rogerk)
**	    Reduced Logging Project.  In preparation for support of Partial
**	    TCB's for the new recovery schemes, changed references to
**	    some tcb fields to use the new tcb_table_io structure fields.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Fill in new loc_status field when
**	    initializing data in location array.
**	31-oct-1992 (jnash)
**	    Reduced Logging Project (phase 2): 
**	    - Fill in loc_config_id in newloc_info and oldloc_info, new params
**	      to dm0l_fcreate, dm0l_frename, dm0l_relocate, dm0l_dmu.
**	    - Added handling of journal status for catalog updates.  When a
**	      core catalog is updated (by a put, delete or replace), the journal
**	      status of the log record depends on the journal state of the user
**	      table which is being altered, not the journal status of the actual
**	      system catalog.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	    Removed DMPP_ACC_PLV argument from dm2f_write_file.
**	18-nov-1992 (robf)
**	    Removed dm0a.h
**	16-feb-1993 (rogerk)
**	    Changed table-in-use check to look at tcb_open_count, not the
**	    tcb_ref_count.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV: Removed unused xcb_lg_addr.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	21-jun-1993 (jnash)
**	    Check for error returned from dm2t_close() call.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	23-aug-1993 (rogerk)
**	  - Reordered steps in the modify process for new recovery protocols.
**	  - Updates made during DDL operations are now journaled if the db
**	    is journaled regardless of the table jnl state.
**	  - Fix CL prototype mismatches.
**	08-sep-93 (swm)
**	    Moved cs.h include above lg.h because lg.h now needs the CS_SID
**	    session id. type definition.
**	20-sep-1993 (rogerk)
**	    Final support added for non-journaled tables.  Drops of non-jnl
**	    tables are always journaled, other DDL operations are not.
**	    Full explanation of non journaled table handling given in DMRFP.
**	18-apr-1994 (jnash)
**	    fsync project.  Unless overridden by config option, open 
**	    temp files in sync'd databases DI_USER_SYNC_MASK, force them 
**	    prior to close.
**      10-nov-1994 (cohmi01)
**          Bugs 48713, 45905
**          Obtain and pass timeout value to dm2t_control and dm2t_open.
**      27-dec-94 (cohmi01)
**          For RAW I/O, changes to pass more info down to dm2f_xxx functions.
**          Pass location ptr to dm2f_rename().
**          When updating pending queue, add additional info to XCCB.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass page_size to dm2u_file_create.
**	    When allocating local page buffers, allow for multiple page sizes.
**      06-mar-1996 (stial01)
**          Pass page_size to dm2u_write_file()
**          Pass page_size to dm0l_relocate()
**	17-sep-96 (nick)
**	    Remove compiler warning.
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**	25-Aug-1997 (jenjo02)
**	    Added log_id (LG_LXID) to dm2f_build_fcb(), dm2f_open_file(),
**	    dm2f_create_file(), dm2f_delete_file(), dm2f_rename(),
**	    dm2f_add_fcb() dm2f_release_fcb() prototypes so that it 
**	    can be passed on to dm2t_reclaim_tcb().
**	03-Dec-1998 (jenjo02)
**	    For RAW files, save page size in FCB to XCCB.
**	19-mar-1999 (nanpr01)
**	    Productize raw location support.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	25-May-1999 (jenjo02)
**	    Before purging the "old" TCB, sense the old locations and save
**	    the last-allocated number of pages from each. This information
**	    would otherwise be lost if any of the locations are RAW (for RAW,
**	    "fcb_physical_allocation" can only be determined by reading the
**	    FHDR).
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Nov-2000 (jenjo02)
**	    Deleted xcb_tq_next|prev, xccb_tq_next|prev.
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to dma_table_access() if neither
**	    C2 nor B1SECURE.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here.
**	04-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2f_?, dm2t_?, dm2r_? functions converted to DB_ERROR *
**/

/*{
** Name: dm2u_relocate - Relocate a table.
**
** Description:
**      This routine is called from dmu_relocate () to relocate a table.
**
** Inputs:
**	dcb				The database id. 
**	xcb				The transaction id.
**	tbl_id				The internal table id of the base table 
**					to be destroyed.
**	location			Locations where the table will relocate.
**	oldlocation			Old locations of the table.
**      l_count                         Count of locations to move.
**	db_lockmode			Lockmode of the database for the caller.
** Outputs:
**      err_data                        Which location in the array failed.
**      err_code                        The reason for an error return status.
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
**      28-may-86 (ac)
**          Created from dmu_relocate().
**	18-oct-88 (mmm)
**	    added "sync_flag" parameter to dm2f_open_file(), dm2f_build_fcb(),
**	    and dm2f_create_file() calls.
**	16-jan-89 (mmm)
**	    Reversed logic of sync flag in config file so that by default
**	    a database will be "sync'd".
**	28-feb-89 (EdHsu)
**	    Add call to dm2u_ckp_lock to be mutually exclusive with ckp.
**	11-sep-89 (rogerk)
**	    Log DMU operation at the end of the Relocate.
**	24-sep-1990 (rogerk)
**	    Fix boolean truncation problem caused by assigning a i4 
**	    bitmask to a boolean value (multiple_locations).
**	19-nov-1990 (bryanp)
**	    Pass correct message parameters to message E_DM9029.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	     4-feb-1991 (rogerk)
**		Add support for Set Nologging.  If transaction is Nologging,
**		then mark table-opens done here as nologging.  Also, don't
**		write any log records.
**      23-oct-1991(jnash) 
**          Temporary insertion of zeroes into dm0l_fcreate param list
**          til we finger out what is really going on (merge mania).
**	15-apr-1992 (bryanp)
**	    Remove "FCB" argument from dm2f_rename()
**	14-apr-1992 (rogerk)
**	    Changed relocate loop to process multiple pages per IO, rather
**	    than acting only one page at a time.  Uses svcb_mwrite_blocks
**	    as the grouping factor.  Also, pages are now allocated from
**	    dm0m rather than off of the stack.
**	09-jun-1992 (kwatts)
**	    6.5 MPF Changes. Added page_fmt accessor parameter to
**	    dm2f_write_file call.
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Fill in new loc_status field when
**	    initializing data in location array.
**	31-oct-1992 (jnash)
**	    Reduced Logging Project (phase 2): 
**	    - Fill in loc_config_id in newloc_info and oldloc_info, new params
**	      to dm0l_fcreate, dm0l_frename, dm0l_relocate, dm0l_dmu.
**	    - Added handling of journal status for catalog updates.  When a
**	      core catalog is updated (by a put, delete or replace), the journal
**	      status of the log record depends on the journal state of the user
**	      table which is being altered, not the journal status of the actual
**	      system catalog.
**	    - Removed DMPP_ACC_PLV argument from dm2f_write_file.
**	16-feb-1993 (rogerk)
**	    Changed table-in-use check to look at tcb_open_count, not the
**	    tcb_ref_count.  The tcb may legally be referenced by DBMS
**	    background threads.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	28-may-1993 (robf)
**	    Secure 2.0: Reworked ORANGE code
**	21-june-1993 (jnash)
**	    Check for error returned from dm2t_close() call.
**	23-aug-1993 (rogerk)
**	  - Reordered steps in the relocate process for new recovery protocols.
**	      - The relocate work is now done BEFORE logging the dm0l_relcoate
**		log record.  This is done so that any non-executable relocate
**		attempt (one with an impossible allocation for example) will
**		not be attempted during rollforward recovery.
**	      - We update the system catalog entries after the relocate.
**	      - We now log separate dm0l_dmu records for each location affected.
**	  - We now call dm2u_file_create to log/create the new files.
**	  - Changes to handling of non-journaled tables.  ALL system catalog 
**	    updates in a journaled database are now journaled regardless of
**	    whether or not the user table being affected is.
**	  - Updates made during DDL operations are now journaled if the db
**	    is journaled regardless of the table jnl state.  Changed DMU to
**	    to be a journaled log record.
**	  - Changed error handling logic.
**	20-sep-1993 (rogerk)
**	    Final support added for non-journaled tables.  Drops of non-jnl
**	    tables are always journaled, other DDL operations are not.
**	    Full explanation of non journaled table handling given in DMRFP.
**	18-apr-1994 (jnash)
**	    fsync project.  Unless overridden by config option, open 
**	    temp files in sync'd databases DI_USER_SYNC_MASK, force them 
**	    prior to close.
**     02-oct-1994 (Bin Li)
**          Fix bug 60170, modify the dm2u_relocate function, reverse the 
**     the checking order for locations. If the locations are invalid, set
**     the xcb->xcb_flags accordingly.
**      10-nov-1994 (cohmi01)
**          Bugs 48713, 45905
**          Obtain and pass timeout value to dm2t_control and dm2t_open.
**      27-dec-94 (cohmi01)
**          For RAW I/O, changes to pass more info down to dm2f_xxx functions.
**          Pass location ptr to dm2f_rename().
**          When updating pending queue, add additional info to XCCB.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass page_size to dm2u_file_create.
**	    When allocating local page buffers, allow for multiple page sizes.
**      06-mar-1996 (stial01)
**          Pass page size to dm2f_build_fcb, dm2f_read_file, 
**	17-sep-96 (nick)
**	    Remove compiler warning.
**	06-dec-1996 (nanpr01)
**	    bug 79368 : fix segv in server. 
**	06-dec-1996 (nanpr01)
**	    bug 79484 : to let caller know which location failed in the
**	    location array we need to pass an extra argument.
**	03-jan-1997 (nanpr01)
**	    Use the #define values rather than hard-coded constants.
**	03-jun-1997 (nanpr01)
**	    Initialize new fields in xccb.
**	01-oct-1998 (nanpr01)
**	    Added the new parameter in dm2r_replace for update performance 
**	    enhancement.
**	03-Dec-1998 (jenjo02)
**	    For RAW files, save page size in FCB to XCCB.
**	28-may-1999 (nanpr01)
**	    Added table name and owner name in parameter list to display
**	    sensible error messages in errlog.log.
**	23-Jun-1999 (jenjo02)
**	    Before purging the "old" TCB, sense the old locations and save
**	    the last-allocated number of pages from each. This information
**	    would otherwise be lost if any of the locations are RAW (for RAW,
**	    "fcb_physical_allocation" can only be determined by reading the
**	    FHDR).
**	6-Feb-2004 (schka24)
**	    Update calls to build-fcb.
**	6-Aug-2004 (schka24)
**	    Table access check now a no-op, remove.
**	01-Dec-2006 (kiria01) b117225
**	    Initialise the lockid parameter to dm2u_raw_device_lock avoid random
**	    implicit lock conversions.
**	6-Nov-2009 (kschendel) SIR 122757
**	    Straighten out new vs old sync flags, allow direct-io option.
*/
DB_STATUS
dm2u_relocate(
DMP_DCB		*dcb,
DML_XCB		*xcb,
DB_TAB_ID	*tbl_id,
DB_LOC_NAME	*location,
DB_LOC_NAME	*oldlocation,
i4		l_count,
i4		db_lockmode,
DB_TAB_NAME	*tab_name,
DB_OWN_NAME	*tab_owner,
DB_ERROR	*dberr)
{
    DML_XCCB	*xccb;    
    i4		ref_count;    
    DMP_TCB	*tcb;
    DMP_RCB	*reloc_rcb;
    DMP_RCB	*rel_rcb, *dev_rcb;
    i4		error, local_error;
    i4		status, local_status;
    i4		i, k;
    DM_TID	tid;    
    i4		page;
    DMP_RELATION  relrecord;
    DMP_DEVICE	devrecord;
    DB_OWN_NAME	owner_name;
    DB_TAB_NAME	table_name;
    DM_FILE	del_filename;
    LK_LKID	lockid;
    i4		filelength;
    DB_TAB_ID	table_id;
    DM2R_KEY_DESC  key_desc[2];
    i4		compare;
    DB_TAB_TIMESTAMP timestamp;
    i4		journal;
    i4		dm0l_jnlflag;
    i4		loc_count;
    DMP_LOCATION    *newloc_info;
    DMP_LOCATION    *oldloc_info;
    DMP_LOCATION    *l;
    i4		lk_mode;
    bool	sconcur;
    DMP_MISC	*loc_cb = (DMP_MISC *)0;
    DMP_MISC	*page_cb = (DMP_MISC *)0;
    bool	multiple_locations;
    bool	logging;
    i4		loc_index;
    i4		dup_loc_id;
    u_i4	new_sync_flag, old_sync_flag;
    u_i4	directio_align;
    DMPP_PAGE	*page_array;
    i4		page_size;
    i4		allocate_size;
    i4		page_count;
    i4		mwrite_blocks;
    LG_LSN	lsn;
    bool	newfile_open = FALSE;
    bool	oldfile_open = FALSE;
    i4		timeout = 0;
    i4		has_raw_location = 0;
    i4		*oldloc_endpage; 
    i4		endpage;
    DB_ERROR	local_dberr;

    CLRDBERR(dberr);

    status = dm2u_ckp_lock(dcb, tab_name, tab_owner, xcb, dberr);
    if (status != E_DB_OK)
	return (E_DB_ERROR);

    multiple_locations = FALSE;
    dmf_svcb->svcb_stat.utl_relocate++;
    directio_align = dmf_svcb->svcb_directio_align;
    if (dmf_svcb->svcb_rawdb_count > 0 && directio_align < DI_RAWIO_ALIGN)
	directio_align = DI_RAWIO_ALIGN;

    MEfill(sizeof(LK_LKID), 0, &lockid);

    /*
    /* The reading side uses the typical sync flags for tables.
    */
    if (dcb->dcb_sync_flag & DCB_NOSYNC_MASK)
	old_sync_flag = 0;
    else 
	old_sync_flag = DI_SYNC_MASK;
    if (dmf_svcb->svcb_directio_tables)
	old_sync_flag |= DI_DIRECTIO_MASK;

    logging = ((xcb->xcb_flags & XCB_NOLOGGING) == 0);

    /*
    ** Allocate space for relocate operation.  This includes
    **
    **     - Old Location Array
    **     - New Location Array
    **     - Page Buffers for IO
    **	   - Old Location sense information
    **
    ** Eventually, it would be nice to have the ability to reserve group
    ** buffers from the buffer manager to use for IO operations.  This would
    ** allow us to not have to allocate page buffers here; to take advantage
    ** of shared memory for IO's; and to make use of adjustable group buffer
    ** sizes.
    */
    mwrite_blocks = dmf_svcb->svcb_mwrite_blocks;
    allocate_size = sizeof(DMP_MISC) +
		    (sizeof(DMP_LOCATION) * l_count * 2) +
		    (sizeof(i4) * l_count);

    status = dm0m_allocate(allocate_size,
	    DM0M_ZERO, 
	    (i4)MISC_CB,
	    (i4)MISC_ASCII_ID, (char *)0, (DM_OBJECT **)&loc_cb, 
	    dberr);
    if (status != E_DB_OK)
	return (E_DB_ERROR);

    newloc_info = (DMP_LOCATION *)((char *)loc_cb + sizeof(DMP_MISC));
    oldloc_info = (DMP_LOCATION *)((char *)newloc_info +
	(l_count * sizeof(DMP_LOCATION)));
    oldloc_endpage = (i4 *)((char *)oldloc_info + 
	(l_count * sizeof(DMP_LOCATION)));
    loc_cb->misc_data = (char *)newloc_info;

    loc_index = 0;
    loc_count = l_count;

    for(k = 0; k < l_count; k++)
    {
	if (MEcmp((PTR)DB_DEFAULT_NAME, (PTR)&oldlocation[k], 
	    sizeof(DB_LOC_NAME)) == 0)
	{
	    STRUCT_ASSIGN_MACRO(dcb->dcb_root->logical,
		oldloc_info[loc_index].loc_name);
	    oldloc_info[loc_index].loc_ext = dcb->dcb_root;
	    oldloc_info[loc_index].loc_id = 0;
	    oldloc_info[loc_index].loc_fcb = 0;	    
	    oldloc_info[loc_index].loc_status = LOC_VALID;
	    oldloc_info[loc_index].loc_config_id = 0;
	}
	else
	{	
	    for (i = 0; i < dcb->dcb_ext->ext_count; i++)
	    {	
		compare = MEcmp((char *)&oldlocation[k], 
		  (char *)&dcb->dcb_ext->ext_entry[i].logical, 
		  sizeof(DB_LOC_NAME));
		if (compare == 0 && 
		    (dcb->dcb_ext->ext_entry[i].flags & DCB_DATA))
		{
		    STRUCT_ASSIGN_MACRO(oldlocation[k], 
                                   oldloc_info[loc_index].loc_name);
		    oldloc_info[loc_index].loc_ext =
					&dcb->dcb_ext->ext_entry[i];
		    oldloc_info[loc_index].loc_id = 0;
		    oldloc_info[loc_index].loc_fcb = 0;	    
		    oldloc_info[loc_index].loc_status = LOC_VALID;
		    oldloc_info[loc_index].loc_config_id = i;
		    if (dcb->dcb_ext->ext_entry[i].flags & DCB_RAW)
		    {
			status = dm2u_raw_device_lock(dcb, xcb, &oldlocation[k],
					&lockid, dberr);
			if (status != LK_NEW_LOCK)
			{
			    compare = 1;
			    break;
			}
			oldloc_info[loc_index].loc_status |= LOC_RAW;
		    }
		    break;
		}
		else
		    compare = 1;
	    }
	    if (compare != 0)
	    {
		SETDBERR(dberr, k, E_DM0187_BAD_OLD_LOCATION_NAME);
		if (loc_cb != 0)
		    dm0m_deallocate((DM_OBJECT **)&loc_cb);
		return (E_DB_ERROR);
	    }
        }
	/*  Check location, must be default or valid location. */

	if (MEcmp((PTR)DB_DEFAULT_NAME, (PTR)&location[k], 
	    sizeof(DB_LOC_NAME)) == 0)
	{
	    STRUCT_ASSIGN_MACRO(dcb->dcb_root->logical,
		newloc_info[loc_index].loc_name);
	    newloc_info[loc_index].loc_ext = dcb->dcb_root;
	    newloc_info[loc_index].loc_id = 0;
	    newloc_info[loc_index].loc_fcb = 0;	    
	    newloc_info[loc_index].loc_status = LOC_VALID;
	    newloc_info[loc_index].loc_config_id = 0;;
		    
	}
	else
	{	
	    for (i = 0; i < dcb->dcb_ext->ext_count; i++)
	    {	
		compare = MEcmp((char *)&location[k], 
		  (char *)&dcb->dcb_ext->ext_entry[i].logical, 
		  sizeof(DB_LOC_NAME));
		if (compare == 0 && 
		    (dcb->dcb_ext->ext_entry[i].flags & DCB_DATA))
		{
		    STRUCT_ASSIGN_MACRO(location[k],
			newloc_info[loc_index].loc_name);
		    newloc_info[loc_index].loc_ext =
			&dcb->dcb_ext->ext_entry[i];
		    newloc_info[loc_index].loc_id = 0;
		    newloc_info[loc_index].loc_fcb = 0;	    
		    newloc_info[loc_index].loc_status = LOC_VALID;
		    newloc_info[loc_index].loc_config_id = i;
		    if (dcb->dcb_ext->ext_entry[i].flags & DCB_RAW)
		    {
			status = dm2u_raw_device_lock(dcb, xcb, &location[k],
					&lockid, dberr);
			if (status != LK_NEW_LOCK)
			{
			    compare = 1;
			    break;
			}
			status = dm2u_raw_location_free(dcb, xcb, &location[k],
					dberr);
			if (status != E_DB_OK)
			{
			    compare = 1;
			    break;
			}
			newloc_info[loc_index].loc_status |= LOC_RAW;
			has_raw_location = 1;
		    }
		    break;
		}
		else
		    compare = 1;
	    }
	    if (compare != 0) 
	    {
		SETDBERR(dberr, k, E_DM001D_BAD_LOCATION_NAME);
		if (loc_cb != 0)
		    dm0m_deallocate((DM_OBJECT **)&loc_cb);
		return (E_DB_ERROR);
	    }
	}

        /*
	** Check if the old location and the new location are the same.
	** If they are then skip this location and decrement the count
	** of locations to move.
	*/
	if (MEcmp((char *)&oldlocation[k], (char *)&location[k],
		sizeof(DB_LOC_NAME)) == 0)
	{
	    loc_count--;
	    continue;
	}

	loc_index++;
    }

    /*
    ** If there are no locations being moved, then return.
    */
    if (loc_count == 0)
    {
	if (loc_cb != 0)
	    dm0m_deallocate((DM_OBJECT **)&loc_cb);
	return (E_DB_OK);
    }

    do
    {
	lk_mode = DM2T_X;
	/* Obtain timeout if any from set lockmode */
        timeout = dm2t_get_timeout(xcb->xcb_scb_ptr, tbl_id);
	if (tbl_id->db_tab_base <= DM_SCONCUR_MAX)
	    lk_mode = DM2T_IX;

	/* Get the location id for the locations we are moving. 
	** To do this we must open the table. */

	status = dm2t_open(dcb, tbl_id, lk_mode,
	    DM2T_UDIRECT, DM2T_A_WRITE, timeout,
	    (i4)20, xcb->xcb_sp_id,
	    xcb->xcb_log_id, xcb->xcb_lk_id,(i4)0, (i4)0, 
	    db_lockmode, &xcb->xcb_tran_id, &timestamp,
	    &reloc_rcb, (DML_SCB *)0, dberr);
	if (status)
	    break;
	reloc_rcb->rcb_xcb_ptr = xcb;

	/*
	** Check for NOLOGGING - no updates should be written to the log
	** file if this session is running without logging.
	*/
	if (xcb->xcb_flags & XCB_NOLOGGING)
	    reloc_rcb->rcb_logging = 0;

	/* Check the tcb to insure that the table to relocate is not
	** currently open by this same transaction.
	** Save the relation name and owner for relocate checks.
	*/

	tcb = reloc_rcb->rcb_tcb_ptr;

	page_size = tcb->tcb_rel.relpgsize;
	ref_count = tcb->tcb_open_count;
	sconcur = tcb->tcb_rel.relstat & TCB_CONCUR;
        multiple_locations = (tcb->tcb_rel.relstat & TCB_MULTIPLE_LOC) != 0;
	MEcopy((char *)&tcb->tcb_rel.relid, sizeof(DB_TAB_NAME),
	   (char *)&table_name);
	MEcopy((char *)&tcb->tcb_rel.relowner, sizeof(DB_OWN_NAME),
	   (char *)&owner_name);
	*tab_name = table_name;
	*tab_owner = owner_name;

	/* New files are opened in load-file sync mode. */

	if (dcb->dcb_sync_flag & DCB_NOSYNC_MASK)
	    new_sync_flag = 0;
	else
	    new_sync_flag = DI_USER_SYNC_MASK;
	if (dmf_svcb->svcb_directio_load)
	    new_sync_flag |= DI_DIRECTIO_MASK;
	new_sync_flag |= DI_PRIVATE_MASK;

	/*
	** Sense all locations to ensure that fcb_physical_allocation
	** is properly updated for each location. Before purging the
	** TCB (and losing RAW sense information), we'll save each
	** old location's fcb_physical_allocation for later reading
	** and writing.
	*/
	status = dm2f_sense_file(tcb->tcb_table_io.tbio_location_array,
				 tcb->tcb_table_io.tbio_loc_count,
				 &table_name, &dcb->dcb_name,
				 &endpage, dberr);
	if (status != E_DB_OK)
	{
	    if ( dm2t_close(reloc_rcb, DM2T_NOPURGE, &local_dberr) )
	        *dberr = local_dberr;
	    reloc_rcb = 0;
	    break;
	}

	for (k = 0; k < loc_count; k++)
	{
	    for (i=0; i < tcb->tcb_table_io.tbio_loc_count; i++)	
	    {
		l = &tcb->tcb_table_io.tbio_location_array[i];
		
		compare = MEcmp((char *)&oldloc_info[k].loc_name, 
		    (char *)&l->loc_name,
		    sizeof(DB_LOC_NAME));
		if (compare == 0 )
		{
		    oldloc_info[k].loc_id = l->loc_id;
		    newloc_info[k].loc_id = l->loc_id;
		    /* Save the last-allocated page from oldloc */
		    oldloc_endpage[k] = l->loc_fcb->fcb_physical_allocation;
		    break;
		}
	    }
	    if (compare != 0)
	    {
		status = dm2t_close(reloc_rcb, DM2T_NOPURGE, dberr);
		reloc_rcb = 0;
		SETDBERR(dberr, k, E_DM0187_BAD_OLD_LOCATION_NAME);
		break;
	    }
	}
	if (dberr->err_code)
	    break;

	/*
	** Check for duplicate location names.  You cannot relocate
	** a portion of a table to a location on which another portion
	** of the table resides.
	**
	** If one of the new locations is equivalent to a location in the
	** tcb location list, make sure that location is being relocated
	** as well.
	*/
	for (k = 0; k < loc_count; k++)
	{
	    /* Check if table already resides on this location */
	    for (i=0; i < tcb->tcb_table_io.tbio_loc_count; i++)	
	    {
		compare = MEcmp((char *)&newloc_info[k].loc_name, 
		    (char *)&tcb->tcb_table_io.tbio_location_array[i].loc_name, 
		    sizeof(DB_LOC_NAME));
		if (compare == 0)
		    break;
	    }
	    if (compare != 0)
		continue;

	    /*
	    ** If table does reside on this location, make sure the part
	    ** that does is being relocated to a different spot.
	    */
	    dup_loc_id = tcb->tcb_table_io.tbio_location_array[i].loc_id;
	    for (i = 0; i < loc_count; i++)
	    {
		compare = (oldloc_info[i].loc_id == dup_loc_id);
		if (compare)
		    break;
	    }
	    if (compare == 0)
	    {
		status = dm2t_close(reloc_rcb, DM2T_NOPURGE, dberr);
		reloc_rcb = 0;
		SETDBERR(dberr, k, E_DM001E_DUP_LOCATION_NAME);
		break;
	    }
	}
	if (dberr->err_code)
	    break;

	if ((ref_count > 1) && (sconcur == 0))
	{
	    status = dm2t_close(reloc_rcb, DM2T_NOPURGE, dberr);
	    reloc_rcb = 0;
	    SETDBERR(dberr, 0, E_DM005D_TABLE_ACCESS_CONFLICT);
	    break;
	}		

	/* 
	** Request a table control lock for exclusively control of the
	** table.
	*/

	status = dm2t_control(dcb, tbl_id->db_tab_base, xcb->xcb_lk_id, 
	    LK_X, (i4)0, timeout, dberr);
	if (status != E_DB_OK)
	    break;

	/* Now close the table, use the purge flag to insure
	** that the physical file is closed and the tcb deallocated. 
	*/
	status = dm2t_close(reloc_rcb, DM2T_PURGE, dberr);
	if (status != E_DB_OK)
	    break;

	allocate_size = sizeof(DMP_MISC) + (page_size * mwrite_blocks);
	if (directio_align != 0)
	    allocate_size += directio_align;

	status = dm0m_allocate(allocate_size,
		DM0M_ZERO, 
		(i4)MISC_CB,
		(i4)MISC_ASCII_ID, (char *)0, (DM_OBJECT **)&page_cb, 
		&local_dberr);
	if (status != E_DB_OK)
	{
	    /*
	    ** If we couldn't allocate that much memory, try using a smaller
	    ** page array, but check err_code, not "status".
	    */
	    if (local_dberr.err_code == E_DM923D_DM0M_NOMORE)
	    {
		mwrite_blocks = 1;
		allocate_size = sizeof(DMP_MISC) +
				(mwrite_blocks * page_size);
		if (directio_align != 0)
		    allocate_size += directio_align;
		status = dm0m_allocate(allocate_size,
		    DM0M_ZERO,
		    (i4)MISC_CB,
		    (i4)MISC_ASCII_ID, (char *)0, (DM_OBJECT **)&page_cb, 
		    &local_dberr);
	    }
	    if (status != E_DB_OK)
	    {
		*dberr = local_dberr;
		break;
	    }
	}
	page_array = (DMPP_PAGE *)((char *)page_cb + sizeof(DMP_MISC));
	if (directio_align != 0)
	    page_array = (DMPP_PAGE *) ME_ALIGN_MACRO(page_array, directio_align);
	page_cb->misc_data = (char *)page_array;

	/* 
        ** See if file exists at new location. If so an error
        ** occurred.
	*/

	/* Allocate fcb for new and old locations. */

	status = dm2f_build_fcb(xcb->xcb_lk_id, xcb->xcb_log_id,
	    newloc_info, loc_count, page_size, DM2F_ALLOC, DM2F_TAB,
	    tbl_id, 0, 0, (DMP_TABLE_IO *)0, 0, dberr);
	if (status == E_DB_OK)
	{
	    status = dm2f_open_file(xcb->xcb_lk_id, xcb->xcb_log_id,
		newloc_info, loc_count, (i4)page_size,
		DM2F_E_WRITE, new_sync_flag, (DM_OBJECT *)dcb, dberr);
	}
	if (status == E_DB_OK || dberr->err_code != E_DM9291_DM2F_FILENOTFOUND) 
	{
	    if (status == E_DB_OK)
	    {
		if (!has_raw_location)
		{
		    uleFormat(NULL, E_DM9029_UNEXPECTED_FILE, 0, ULE_LOG, NULL, 
			NULL,0,NULL,
			&error, 2, newloc_info[0].loc_ext->phys_length, 
			&newloc_info[0].loc_ext->physical, sizeof(DM_FILE), 
			&newloc_info[0].loc_fcb->fcb_filename);
		    SETDBERR(dberr, 0, E_DM0104_ERROR_RELOCATING_TABLE);
		}
	    }
	    if (!has_raw_location)
	    {
	        local_status = dm2f_release_fcb(xcb->xcb_lk_id, xcb->xcb_log_id,
		    newloc_info, loc_count, DM2F_ALLOC, &local_dberr);
	    }
	    break;
	}
	else
	    CLRDBERR(dberr);
    } while (FALSE);

    if (dberr->err_code != 0)
    {
	if (loc_cb != 0)
	    dm0m_deallocate((DM_OBJECT **)&loc_cb);
	if (page_cb)
	    dm0m_deallocate((DM_OBJECT **)&page_cb);
	return (E_DB_ERROR);
    }

    /* Open the relation table. */

    table_id.db_tab_base = DM_B_RELATION_TAB_ID;
    table_id.db_tab_index = DM_I_RELATION_TAB_ID;
   
    status = dm2t_open(dcb, &table_id, DM2T_IX,
	    DM2T_UDIRECT, DM2T_A_WRITE, (i4)0, (i4)20, xcb->xcb_sp_id,
	    xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0, (i4)0, 
            db_lockmode, &xcb->xcb_tran_id, &timestamp,
	    &rel_rcb, (DML_SCB *)0, dberr);
    if (status)
    {
	if (loc_cb != 0)
	    dm0m_deallocate((DM_OBJECT **)&loc_cb);
	if (page_cb)
	    dm0m_deallocate((DM_OBJECT **)&page_cb);
	return (E_DB_ERROR);
    }
    rel_rcb->rcb_xcb_ptr = xcb;

    /*
    ** Check for NOLOGGING - no updates should be written to the log
    ** file if this session is running without logging.
    */
    if (xcb->xcb_flags & XCB_NOLOGGING)
	rel_rcb->rcb_logging = 0;

    /* Open the device table if needed. */

    dev_rcb = 0;
    if (multiple_locations)
    {    
	table_id.db_tab_base = DM_B_DEVICE_TAB_ID;
	table_id.db_tab_index = DM_I_DEVICE_TAB_ID;

	status = dm2t_open(dcb, &table_id, DM2T_IX,
	    DM2T_UDIRECT, DM2T_A_WRITE, (i4)0, (i4)20, xcb->xcb_sp_id,
	    xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0, (i4)0, 
	    db_lockmode, 
	    &xcb->xcb_tran_id, &timestamp,
	    &dev_rcb, (DML_SCB *)0, dberr);
	if (status)
	{
	    if (loc_cb != 0)
		dm0m_deallocate((DM_OBJECT **)&loc_cb);
	    if (page_cb)
		dm0m_deallocate((DM_OBJECT **)&page_cb);
	    return (E_DB_ERROR);
	}
	dev_rcb->rcb_xcb_ptr = xcb;

	/*
	** Check for NOLOGGING - no updates should be written to the log
	** file if this session is running without logging.
	*/
	if (xcb->xcb_flags & XCB_NOLOGGING)
	    dev_rcb->rcb_logging = 0;
    }    
    
    /* 
    **  All parameters have been checked, can really reloacte the table. 
    **  First log that a relocate is taking place. 
    **  Take a savepoint for error recovery.
    **  Then change the relation record to contain the new location. 
    **  Create a new file, and copy the pages from the new to the old. 
    **  When all of this has succeeded, rename the old file 
    **  and pend for deletion.
    **  If any error occur along the way, abort to the savepoint.
    */

    for (k = 0; k < loc_count ; k++)
    {
	/* 
	**  Read the relation record for this table to update the location.
	*/
    
	/*  Position table using base table id. */

	table_id.db_tab_base = tbl_id->db_tab_base;
	table_id.db_tab_index = tbl_id->db_tab_index;
	key_desc[0].attr_operator = DM2R_EQ;
	key_desc[0].attr_number = DM_1_RELATION_KEY;
	key_desc[0].attr_value = (char *) &table_id.db_tab_base;

	status = dm2r_position(rel_rcb, DM2R_QUAL, key_desc, (i4)1,
                          (DM_TID *)0, dberr);
	if (status != E_DB_OK)
	    break;	    

	for (;;)
	{
	    status = dm2r_get(rel_rcb, &tid, DM2R_GETNEXT, 
                         (char *)&relrecord, dberr);
	    if (status != E_DB_OK)
		break;	    

	    if ((relrecord.reltid.db_tab_base == table_id.db_tab_base)
		&&
		(relrecord.reltid.db_tab_index == table_id.db_tab_index))
	    {
		break;
	    }
	}
	if (status != E_DB_OK)
	    break;

	/*
	** Found the relation record in relation table.
	** If we are relocating the first location of the table then replace
	** the relloc field of the iirelation row to show its new value.
	*/
	if (newloc_info[k].loc_id == 0)
	    MEcopy((char *)&newloc_info[k].loc_name, 
                      sizeof(DB_LOC_NAME),
                      (char *)&relrecord.relloc);
	/*
	** Bump the iirelation relstamp since we have made a structural 
	** change to the table.
	*/
	TMget_stamp((TM_STAMP *)&relrecord.relstamp12);

	journal = ((relrecord.relstat & TCB_JOURNAL) != 0);
	dm0l_jnlflag = (journal ? DM0L_JOURNAL : 0);

	rel_rcb->rcb_usertab_jnl = (journal) ? 1 : 0;

	if ((dev_rcb) && (newloc_info[k].loc_id != 0))
	{
	    /* 
	    **  Read the DEVICE record for the old location update the location.
	    */
	
	    /*  Position table using base table id. */

	    table_id.db_tab_base = tbl_id->db_tab_base;
	    table_id.db_tab_index = tbl_id->db_tab_index;
	    key_desc[0].attr_operator = DM2R_EQ;
	    key_desc[0].attr_number = DM_1_DEVICE_KEY;
	    key_desc[0].attr_value = (char *) &table_id.db_tab_base;
	    key_desc[1].attr_operator = DM2R_EQ;
	    key_desc[1].attr_number = DM_2_DEVICE_KEY;
	    key_desc[1].attr_value = (char *) &table_id.db_tab_index;

	    status = dm2r_position(dev_rcb, DM2R_QUAL, key_desc, (i4)2,
			      (DM_TID *)0, dberr);
	    if (status != E_DB_OK)
		break;	    

	    for (;;)
	    {
		status = dm2r_get(dev_rcb, &tid, DM2R_GETNEXT, 
			     (char *)&devrecord, dberr);
		if (status != E_DB_OK)
		    break;	    

		if (devrecord.devlocid == oldloc_info[k].loc_id)
		    break;
	    }
	    if (status != E_DB_OK)
		break;

	    /* Found the device record in device table. */

	    MEcopy((char *)&newloc_info[k].loc_name, 
                          sizeof(DB_LOC_NAME),
			  (char *)&devrecord.devloc);

	    dev_rcb->rcb_usertab_jnl = (journal) ? 1 : 0;
	}

	/********************************************************
	**
	** Perform RELOCATE of this location.
	**
	*********************************************************/

	/*
	** Log and create the new file.
	** The build_fcb call was done above.
	*/
	status = dm2u_file_create(dcb, xcb->xcb_log_id, xcb->xcb_lk_id,
	    tbl_id, new_sync_flag, &newloc_info[k], 1, page_size,
	    logging, journal, dberr);
	if (status != E_DB_OK)
	    break;
	newfile_open = TRUE;

	/*
	** Allocate file descriptor and open old file.
	*/
	status = dm2f_build_fcb(xcb->xcb_lk_id, xcb->xcb_log_id,
	    &oldloc_info[k], 1, page_size, DM2F_ALLOC, DM2F_TAB,
	    tbl_id, 0, 0, (DMP_TABLE_IO *)0, 0, dberr);
	if (status != E_DB_OK)
	    break;

	status = dm2f_open_file(xcb->xcb_lk_id, xcb->xcb_log_id,
	    &oldloc_info[k], 1, (i4)page_size, DM2F_A_WRITE,
	    old_sync_flag, (DM_OBJECT *)dcb, dberr);
	if (status != E_DB_OK)
	    break;
	oldfile_open = TRUE;

	/*
	** oldloc_endpage[k] holds page allocation from old file,
	** which we'll use for reading and writing, but we must
	** still dm2f_sense_file() to ensure that fcb_physical_allocation
	** is initialized else dm2f_read_file will fail.
	*/
	status = dm2f_sense_file(&oldloc_info[k], 1, 
	    &table_name, &dcb->dcb_name, &endpage, dberr); 
	if (status != E_DB_OK)
	    break;
	
	
	page = 0;
	status = dm2f_alloc_file(&newloc_info[k], 1, 
	    &table_name, &dcb->dcb_name, oldloc_endpage[k], &page, dberr); 
	if (status != E_DB_OK)
	    break;

	status = dm2f_flush_file(&newloc_info[k], 1,  
	    &table_name, &dcb->dcb_name, dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** Loop reading pages from the old file and writing them to the
	** new file.  Read/Write 'mwrite_blocks' pages at a time.
	*/

	for (i = 0; i < oldloc_endpage[k]; i += page_count)
	{
	    /* check to see if user interrupt occurred. */
	    if (xcb->xcb_scb_ptr->scb_ui_state & SCB_USER_INTR)
	    {
		SETDBERR(dberr, 0, E_DM0065_USER_INTR);
		status = E_DB_ERROR;
		break;
	    }

	    /* check to see if force abort occurred. */
	    if (xcb->xcb_scb_ptr->scb_ui_state & SCB_FORCE_ABORT)
	    {
		SETDBERR(dberr, 0, E_DM010C_TRAN_ABORTED);
		status = E_DB_ERROR;
		break;
	    }

	    /*
	    ** Determine how many pages to read/write.
	    ** Use mwrite_blocks unless there are not this many pages
	    ** left in the file.
	    */
	    page_count = mwrite_blocks;
	    if (i + page_count > oldloc_endpage[k])
		page_count = oldloc_endpage[k] - i;

	    /*
	    ** Read old file.
	    */
	    status = dm2f_read_file(&oldloc_info[k], 1, page_size,
		&table_name, &dcb->dcb_name, &page_count,
		(i4)i, (char *)page_array, dberr);
	    if (status != E_DB_OK)
		break;

            /*
            ** Write new file.
            */
	    status = dm2f_write_file(&newloc_info[k], 1, page_size, 
		&table_name, &dcb->dcb_name, &page_count,
		(i4)i, (char *)page_array, dberr); 
	    if (status != E_DB_OK)
		break;
	}

	if (status != E_DB_OK)
	    break;
	
	/*
	** Close files use in relocate.
	*/
	oldfile_open = FALSE;
	status = dm2f_close_file(&oldloc_info[k], 1, (DM_OBJECT *)dcb, dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** Opening file DI_USER_SYNC_MASK requires force prior to close.
	*/
	if (new_sync_flag & DI_USER_SYNC_MASK)
	{
	    status = dm2f_force_file(&newloc_info[k], 1, &table_name, 
		&dcb->dcb_name, dberr);
	    if (status != E_DB_OK)
		break;
	}

	newfile_open = FALSE;
	status = dm2f_close_file(&newloc_info[k], 1, (DM_OBJECT *)dcb, dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** Log the file relocate operation - unless logging is disabled.
	**
	** The RELOCATE log record is written after the actual file copy.
	** Note that until now, the user table (and its metadata) has
	** not been physically altered.  We now log the relocate before
	** updating the catalogs and swapping the underlying files.
	*/
	if (logging)
	{
	    status = dm0l_relocate(xcb->xcb_log_id, dm0l_jnlflag, 
		tbl_id, &table_name, &owner_name, 
		&oldloc_info[k].loc_ext->logical, 
		&newloc_info[k].loc_ext->logical, 
		page_size,
		newloc_info[k].loc_id,
		oldloc_info[k].loc_config_id, 
		newloc_info[k].loc_config_id, 
		&newloc_info[k].loc_fcb->fcb_filename,
		(LG_LSN *)0, &lsn, dberr);
	    if (status != E_DB_OK)
		break;
	}

	/*
	** Update iirelation and iidevices to show the new device.
	*/
	status = dm2r_replace(rel_rcb, (DM_TID *)0, DM2R_BYPOSITION, 
                     (char *)&relrecord, (char *)0, dberr);
	if (status != E_DB_OK)
	    break;
	if ((dev_rcb) && (newloc_info[k].loc_id != 0))
	{
	    status = dm2r_replace(dev_rcb, (DM_TID *)0, DM2R_BYPOSITION, 
                     (char *)&devrecord, (char *)0, dberr);
	    if (status != E_DB_OK)
		break;
	}

	if ((oldloc_info[k].loc_status & LOC_RAW) == 0)
	{
	    dm2f_filename(DM2F_DES,tbl_id, 
			   oldloc_info[k].loc_id, 0, 0, &del_filename);
	    filelength = sizeof(DM_FILE);

	    /*
	    ** Log the file rename operation - unless logging is disabled.
	    */
	    if (logging)
	    {
		status = dm0l_frename(xcb->xcb_log_id, 
		    dm0l_jnlflag, &oldloc_info[k].loc_ext->physical, 
		    oldloc_info[k].loc_ext->phys_length, 
		    &oldloc_info[k].loc_fcb->fcb_filename, 
		    &del_filename, tbl_id,
		    l_count, newloc_info[k].loc_config_id,
		    (LG_LSN *)0, &lsn, dberr);
		if (status != E_DB_OK)
		    break;
	    }

	    /* Rename the old file. */

	    status = dm2f_rename(xcb->xcb_lk_id, xcb->xcb_log_id,
		&oldloc_info[k].loc_ext->physical, 
		oldloc_info[k].loc_ext->phys_length, 
		&oldloc_info[k].loc_fcb->fcb_filename, filelength, 
		&del_filename, filelength, 
		&dcb->dcb_name, dberr);
	    if (status != E_DB_OK)
		break;

	    /* Now pend this file for delete. */
	    
	    status = dm0m_allocate((i4)sizeof(DML_XCCB), 
		DM0M_ZERO, (i4)XCCB_CB,
		(i4)XCCB_ASCII_ID, (char *)xcb, 
		(DM_OBJECT **)&xccb, dberr);
	    if (status != E_DB_OK)
		break;
	    
	    xccb->xccb_operation = XCCB_F_DELETE;
	    xccb->xccb_f_len = filelength;
	    MEcopy((char *)&del_filename, 
		   filelength, (char *)&xccb->xccb_f_name); 
	    xccb->xccb_p_len = oldloc_info[k].loc_ext->phys_length;
	    MEcopy((char *)&oldloc_info[k].loc_ext->physical, 
		   oldloc_info[k].loc_ext->phys_length, 
		   (char *)&xccb->xccb_p_name);

	    xccb->xccb_q_next = xcb->xcb_cq_next;
	    xccb->xccb_q_prev = (DML_XCCB*)&xcb->xcb_cq_next;
	    xcb->xcb_cq_next->xccb_q_prev = xccb;
	    xcb->xcb_cq_next = xccb;
	    xccb->xccb_xcb_ptr = xcb;
	    xccb->xccb_sp_id = xcb->xcb_sp_id;
	}

	/*
	** Release old file FCB allocated by dm2f_build_fcb.
	** The FCBs for all the new locations were allocated in one
	** dm2f_build_fcb before this location loop and will be deallocated
	** in a single call at the end of the loop.
	*/
	status = dm2f_release_fcb(xcb->xcb_lk_id, xcb->xcb_log_id,
				 &oldloc_info[k], 1, DM2F_ALLOC, dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** Log the DMU operation.  This marks a spot in the log file to
	** which we can only execute rollback recovery once.  If we now
	** issue update statements against the newly-indexed table, we
	** cannot do abort processing for those statements once we have
	** begun backing out the index.
	*/
	if (logging)
	{
	    status = dm0l_dmu(xcb->xcb_log_id, dm0l_jnlflag, tbl_id, 
		&table_name, &owner_name, (i4)DM0LRELOCATE, 
		(LG_LSN *)0, dberr);
	    if (status != E_DB_OK)
		break;
	}

    } /* end for */

    /*
    ** Clean up resources allocated before the per-location loop above.
    */
    while (status == E_DB_OK)
    {
	/*
	** Release the newfile FCBs
	*/
	status = dm2f_release_fcb(xcb->xcb_lk_id, xcb->xcb_log_id,
			     newloc_info, k, DM2F_ALLOC, dberr);
	if (status != E_DB_OK)
	    break;

	/*
	** Close the iirelation and iidevices tables.
	*/
	status = dm2t_close(rel_rcb, DM2T_PURGE, dberr);
	rel_rcb = 0;
	if (status != E_DB_OK)
	    break;

	if (dev_rcb)
	{
	    status = dm2t_close(dev_rcb, DM2T_PURGE, dberr);
	    dev_rcb = 0;
	    if (status != E_DB_OK)
		break;
	}

	if (status == E_DB_OK)
	{
	    if (loc_cb != 0)
		dm0m_deallocate((DM_OBJECT **)&loc_cb);
	    if (page_cb)
		dm0m_deallocate((DM_OBJECT **)&page_cb);
	    return (E_DB_OK);
	}
    }

    /*
    ** Error cleanup
    */

    /*
    ** Clean up resources that may have been allocated in the above
    ** per-location loop.  The variable 'k' shows the last location that
    ** was being processed.
    */

    if (newfile_open)
    {
	newfile_open = FALSE;
	status = dm2f_close_file(&newloc_info[k], 1, (DM_OBJECT *)dcb,
				&local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    if (oldfile_open)
    {
	oldfile_open = FALSE;
	status = dm2f_close_file(&oldloc_info[k], 1, (DM_OBJECT *)dcb,
				&local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    if (oldloc_info[k].loc_fcb != 0)
    {
	status = dm2f_release_fcb(xcb->xcb_lk_id, xcb->xcb_log_id,
				 &oldloc_info[k], k, DM2F_ALLOC, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    if (newloc_info[0].loc_fcb != 0)
    {
	status = dm2f_release_fcb(xcb->xcb_lk_id, xcb->xcb_log_id,
				 newloc_info, k, DM2F_ALLOC, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    if (rel_rcb)
    {
	status = dm2t_close(rel_rcb, DM2T_PURGE, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    if (dev_rcb)
    {
	status = dm2t_close(dev_rcb, DM2T_PURGE, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    if (loc_cb != 0)
	dm0m_deallocate((DM_OBJECT **)&loc_cb);
    if (page_cb)
	dm0m_deallocate((DM_OBJECT **)&page_cb);

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(dberr, 0, E_DM0104_ERROR_RELOCATING_TABLE);
    }

    return (E_DB_ERROR);
}
