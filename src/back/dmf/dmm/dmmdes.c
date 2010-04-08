/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
**  NO_OPTIM = dr6_us5
*/

#include    <compat.h>
#include    <er.h>
#include    <gl.h>
#include    <pc.h>
#include    <ck.h>
#include    <cs.h>
#include    <di.h>
#include    <jf.h>
#include    <lo.h>
#include    <me.h>
#include    <nm.h>
#include    <st.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dudbms.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmmcb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dm0c.h>
#include    <dmm.h>

/**
**
** Name: DMMDESTROY.C - Functions needed to destroy a database.
**
** Description:
**      This file contains the functions necessary to destroy a 
**	database.  This	function is used by DESTROYDB utility to destroy
**	the files and directories assocaiated with a database.  The
**	iidbdb updates occur in the DESTROYDB utility..
**    
**      dmm_destroy()       -  Routine to perfrom the destroy operation.
**
** History:
**      20-mar-89 (derek) 
**          Created for Orange.
**      08-aug-90 (ralph)
**          Fixed destroydb problem when data directory not found.
**	    Corrected calls to ule_format.
**	20-nov-1990 (bryanp)
**	    Handle errors deleting locations.
**	25-sep-1991 (mikem) integrated following change: 5-aug-1991 (bryanp)
**	    Log more information before returning E_DM011C.
**	 7-jul-1992 (rogerk)
**	    Prototyping DMF.
**	05-nov-92 (jrb)
**	    Make sure WORK directories get destroyed when the DB is destroyed.
**	    (Multi-location sorts project)
**	15-mar-1993 (rogerk)
**	    Fix compile warnings.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system typedefs
**		Cast some parameters to quiet compiler complaints following
**		    prototyping of LK, CK, JF, SR.
**	21-jun-1993 (rogerk)
**	    Fix prototype warning.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	 3-jan-1993 (rogerk)
**	    Added signaling of Conistency Point before destroying database to
**	    make sure that no log records remain in the log that describe
**	    the demolished db.
**	    Added include of tr.h to quite compiler warnings.
**      31-jan-1995 (stial01)
**          BUG 66501: added destroy_loc_list(), del_all_files()
**	18-aug-1995 (harpa06)
**	    If the user decided that they want to checkpoint
**	    their database into a directory (not TAR up the tables)
**	    then each checkpoint directory in a CKP location must 
**	    have all of it's files deleted and then the directory 
**	    removed.
**      13-sep-1995 (chech02) b71218
**          in dmm_destroy(), if DIdirdelete() fails, err_code of dmm  
**          struct should be set to E_DM011C and E_DB_ERROR should be returned.
**	25-mar-1996 (harpa06)
**	    Bug #75633: Access violation occurs during destroydb in lar SEP
**	    tests on NT due to an attempt in freeing memory already freed.
**	    Also did a little cleanup.
**      06-jun-1996 (strpa01)
**          Now check for ALias locations when destroying database.
**          Previously when attempting to delete an Alias location 
**          mapped to same physical directories as default, an attempt to 
**          delete the extended location directory would fail due to presence
**          of config file (deleted from default dir last) (Bug 71738). Now
**          Aliasing is tested for with Alias file processing. Added routine
**          del_alias _files()
**      03-jun-1996 (canor01)
**          For operating-system threads, remove external semaphore protection
**          for NMingpath().
**	21-jun-96 (nick)
**	    Fix ACCVIO caused by above change by strpa01 ; #77231.
**	15-may-97 (mcgem01)
**	    Change order of includes.
** 	24-may-1997 (wonst02)
** 	    Use the database access mode for readonly databases.
**	23-jan-1997 (walro03)
**	    No-optimize this file for ICL DRS 6000 (dr6_us5).  Compiler error:
**	    no table entry for op =.
**	25-Feb-1999 (shero03)
**	    Do case insensitive compare for aaaaaaaa.cnf for DOS & PKZIP
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	05-Mar-2002 (jenjo02)
**	    Call dms_destroy_db() for Sequence Generators.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	05-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	18-Nov-2008 (jonj)
**	    dm0c_? functions converted to DB_ERROR *
[@history_template@]...
**/

/*
**  Defines of other constants.
*/
typedef struct
{
	STATUS	    error;
	char	    *path_name;	
	int	    path_length;
	int	    pad;
}   DELETE_INFO;

/*
**  Definition of static variables and forward static functions.
*/

static STATUS	list_func(VOID);
static STATUS	del_di_files(
			DELETE_INFO	*delete_info,
			char		*file_name,
			i4		file_length,
			CL_ERR_DESC	*sys_err);
static STATUS	del_all_files(
			DELETE_INFO	*delete_info,
			char		*file_name,
			i4		file_length,
			CL_ERR_DESC	*sys_err);
static STATUS	del_ck_files(
			DELETE_INFO	*delete_info,
			char		*file_name,
			i4		file_length,
			CL_ERR_DESC	*sys_err);
static STATUS	del_jf_files(
			DELETE_INFO	*delete_info,
			char		*file_name,
			i4		file_length,
			CL_ERR_DESC	*sys_err);
static STATUS   del_alias_files(
                        DELETE_INFO     *delete_info,
                        char            *file_name,
                        i4              file_length,
                        CL_ERR_DESC      *sys_err);
static DB_STATUS destroy_loc_list(
			DMM_CB    *dmm_cb);


/*{
** Name: dmm_destroy - Destroy a database and all assocaiated files and directory.
**
**  INTERNAL DMF call format:      status = dmm_destroy(&dmm_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMM_DESTROY_DB, &dmm_cb);
**
** Description:
**      The dmm_destory function handles detroying all files and directories
**      associated the database located at the specified location.
**      The configuration file found at the specified location is used to find 
**      all other locations associated with this database.
**
** Inputs:
**      dmm_cb 
**          .type                           Must be set to DMM_MAINT_CB.
**          .length                         Must be at least sizeof(DMM_CB).
**	    .dmm_flags_mask		    Must be zero.
**          .dmm_tran_id                    Must be the current transaction 
**                                          identifier returned from the 
**                                          begin transaction operation.
**          .dmm_db_location		    Pointer to device name.
**          .dmm_loc_list                   OPTIONAL: if initialized, it may
**                                          be used to delete locations if
**                                          the database directory does not
**                                          exist and the location info in
**                                          the config file cannnot be 
**                                          processed.
**
** Output:
**      dmm_cb
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM0010_BAD_DB_ID
**                                          E_DM001A_BAD_FLAG
**                                          E_DM001D_BAD_LOCATION_NAME
**                                          E_DM002A_BAD_PARAMETER
**                                          E_DM003B_BAD_TRAN_ID
**                                          E_DM0042_DEADLOCK
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM0064_USER_INTR
**                                          E_DM0065_USER_ABORT
**                                          E_DM010C_TRAN_ABORTED
**
**         .error.err_data                  Set to attribute in error by 
**                                          returning index into attribute list.
**
** History:
**	20-mar-1989 (Derek)
**	    Created for ORANGE.
**      08-aug-1990 (ralph)
**          Fixed destroydb problem when data directory not found
**	20-nov-1990 (bryanp)
**	    Handle errors deleting locations.
**	25-sep-1991 (mikem) integrated following change: 5-aug-1991 (bryanp)
**	    Log more information before returning E_DM011C.
**      17-oct-1991 (mikem)
**          Eliminated unix compile warning ("warning: statement not reached")
**          changing the way "while (xxx == E_DB_OK)" loop was coded.
**      8-nov-1992 (ed)
**          remove DB_MAXNAME dependency
**	21-jun-1993 (rogerk)
**	    Fix prototype warning.
**	 3-jan-1993 (rogerk)
**	    Added signaling of Conistency Point before destroying database to
**	    make sure that no log records remain in the log that describe
**	    the demolished db.
**      31-jan-1995 (stial01)
**          BUG 66510: call destroy_loc_list() if ROOT location does 
**          not exist anymore. The dmm_cb now contains location information
**          for all locations in case location information can't be processed
**          from the config file.
**      13-sep-1995 (chech02) b71218
**          in dmm_destroy(), if DIdirdelete() fails, err_code of dmm  
**          struct should be set to E_DM011C and E_DB_ERROR should be returned.
**      06-jun-1996 (strpa01)
**          Now check for ALias locations when destroying database.
**          Previously when attempting to delete an Alias location 
**          mapped to same physical directories as default, an attempt to 
**          delete the extended location directory would fail due to presence
**          of config file (deleted from default dir last) (Bug 71738). Now
**          Aliasing is tested for with Alias file processing. Added routine
**          del_alias _files()
**	21-jun-96 (nick)
**	    Modify above change ; the server would ACCVIO if the cnf file
**	    wasn't opened.
** 	24-may-1997 (wonst02)
** 	    Use the database access mode for readonly databases.
**      15-jun-1999 (hweho01)
**          Corrected argument mismatch in ule_format() call.
**	15-Oct-2001 (jenjo02)
**	    Skip alias processing for RAW locations.
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
**      31-May-2007 (wonca01) BUG#118407
**          Check status of DIdirdelete(), if it is equal to DI_BADDELETE,
**          setup error message info structure to contain the path of the 
**          directory that failed to be deleted.
*/

static STATUS
list_func(VOID)
{
    return (FAIL);
}

DB_STATUS
dmm_destroy(
DMM_CB    *dmm_cb)
{
    DMM_CB	    *dmm = dmm_cb;
    DM0C_CNF	    *config = 0;
    DM0C_CNF	    *cnf;
    DM0C_EXT	    *ext;
    DMP_DCB	    dcb;
    DML_XCB	    *xcb;
    DML_SCB	    *scb;
    LK_LOCK_KEY	    db_key;
    i4	    db_length;
    i4	    lock_flag;
    i4	    physical_length;
    i4	    i;
    i4	    j;
    i4	    item;
    i4	    event;
    STATUS	    status;
    DB_STATUS	    open_status;
    DB_ERROR	    open_dberr;
    i4		    *err_code = &dmm->error.err_code;
    char	    *physical_name;
    i4	    l_loc_dir;
    i4	    l_db_dir;
    char	    *ptr;
    char	    db_buffer[sizeof(DB_DB_NAME) + sizeof(int)];
    char	    loc_buffer[sizeof(DM_PATH) + sizeof(int)];
    char	    loc_dir[sizeof(DM_PATH) + sizeof(int)];
    char	    db_dir[sizeof(DM_PATH) + sizeof(int)];
    LOCATION	    cl_loc;
    CL_ERR_DESC	    sys_err;
    DI_IO	    di_context;
    JFIO	    jf_io;
    DELETE_INFO	    delete_info;
    DM_FILE	    alias_file;
    DMP_LOC_ENTRY   *loc;
    DMP_LOC_ENTRY   rdloc;
    i4         cnf_flag = 0;
    bool	    used_rfc = FALSE;

    CLRDBERR(&dmm->error);

    /*	Check arguments. */

    /*  Check Transaction Id. */

    xcb = (DML_XCB *)dmm->dmm_tran_id;
    if (dm0m_check((DM_OBJECT *)xcb, (i4)XCB_CB) == E_DB_OK)
    {
	scb = xcb->xcb_scb_ptr;

	/* Check for external interrupts */
	if ( scb->scb_ui_state )
	    dmxCheckForInterrupt(xcb, err_code);
	if ( xcb->xcb_state )
	{
	    if (xcb->xcb_state & XCB_USER_INTR)
		SETDBERR(&dmm->error, 0, E_DM0065_USER_INTR);
	    else if (xcb->xcb_state & XCB_FORCE_ABORT)
		SETDBERR(&dmm->error, 0, E_DM010C_TRAN_ABORTED);
	    else if (xcb->xcb_state & XCB_ABORT)
		SETDBERR(&dmm->error, 0, E_DM0064_USER_ABORT);
	}
    }
    else
	SETDBERR(&dmm->error, 0, E_DM003B_BAD_TRAN_ID);

    /*  Check Location. */

    if ( dmm->error.err_code == 0 &&
	(dmm->dmm_db_location.data_in_size == 0 ||
	 dmm->dmm_db_location.data_address == 0) )
    {
	SETDBERR(&dmm->error, 0, E_DM0072_NO_LOCATION);
    }

    if ( dmm->error.err_code )
	return (E_DB_ERROR);

    /*	Construct path name for location and pathname for database. */

    for (i = 0; i < sizeof(DB_DB_NAME); i++)
	if ((db_buffer[i] = dmm->dmm_db_name.db_db_name[i]) == ' ')
	    break;
    db_buffer[i] = 0;
    db_length = i;
    MEmove(dmm->dmm_db_location.data_in_size,
	   dmm->dmm_db_location.data_address, 0,
	   sizeof(loc_buffer), loc_buffer);

    /*	Get path for location. */

    if (dmm->dmm_db_access & DU_RDONLY)
    {
        /*
        ** A readonly database's .cnf file is in the II_DATABASE area.
        */
        LOCATION	db_location;

        LOingpath(ING_DBDIR, 0, LO_DB, &db_location);
        LOtos(&db_location, &ptr);
        STcopy(ptr, loc_dir);
        l_loc_dir = STlength(ptr);

        LOingpath(ING_DBDIR, db_buffer, LO_DB, &db_location);
        LOtos(&db_location, &ptr);
        rdloc.phys_length = STlength(ptr);
        MEmove(rdloc.phys_length, ptr, ' ',
    	    	sizeof(rdloc.physical), (PTR)&rdloc.physical);
    }
    else
    {
        LOingpath(loc_buffer, 0, LO_DB, &cl_loc);
        LOtos(&cl_loc, &ptr);
        STcopy(ptr, loc_dir);
        l_loc_dir = STlength(ptr);
    }

    /*	Get path for database. */

    LOingpath(loc_buffer, db_buffer, LO_DB, &cl_loc);
    LOtos(&cl_loc, &ptr);
    STcopy(ptr, db_dir);
    l_db_dir = STlength(ptr);

    /*	Build a DCB so that me can use the DM0C routines. */

    STRUCT_ASSIGN_MACRO(dmm->dmm_db_name, dcb.dcb_name);
    STRUCT_ASSIGN_MACRO(dmm->dmm_owner, dcb.dcb_db_owner);
    MEmove(l_db_dir, db_dir, ' ',
	sizeof(dcb.dcb_location.physical), (PTR)&dcb.dcb_location.physical);
    dcb.dcb_location.phys_length = l_db_dir;
    dcb.dcb_location.flags = 0;
    MEmove(8, "$default", ' ',
	sizeof(dcb.dcb_location.logical), (PTR)&dcb.dcb_location.logical);
    dcb.dcb_access_mode = DCB_A_WRITE;
    dcb.dcb_status = DCB_SINGLE;
    dcb.dcb_db_type = DCB_PUBLIC;
    dcb.dcb_served = DCB_SINGLE;
    dcb.dcb_opn_count = 0;
    dcb.dcb_ref_count = 0;

    /*  Check that database directory exists. */

    status = DIlistfile(&di_context, db_dir, l_db_dir,
	list_func, 0, &sys_err);
    if (status != DI_ENDFILE)
    {
        /* OK if dir not found when destroying the database */
        if (status == DI_DIRNOTFOUND)
	{
	    /*
	    ** The database directory does not exist,
	    ** If dmm_loc_list was initialized in the dmm control block,
	    ** we can try to delete the other locations
	    */
	    destroy_loc_list(dmm);
            return (E_DB_OK);
	}
	uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			NULL, 0, NULL, err_code, 0);
	SETDBERR(&dmm->error, 0, E_DM011A_ERROR_DB_DIR);
	return (E_DB_ERROR);
    }

    /*	Lock database. */

    lock_flag = 0;
    if (dmm->dmm_flags_mask & DMM_NOWAIT)
	lock_flag |= LK_NOWAIT;
    db_key.lk_type = LK_DATABASE;
    MEcopy((PTR)&dcb.dcb_name, LK_KEY_LENGTH, (PTR)&db_key.lk_key1);
    status = LKrequest(lock_flag, xcb->xcb_lk_id, (LK_LOCK_KEY *)&db_key,
	LK_X, (LK_VALUE *)0, (LK_LKID *)0, (i4)0, &sys_err);
    if (status != OK)
    {
	if (status == LK_BUSY)
	{
	    SETDBERR(&dmm->error, 0, E_DM004C_LOCK_RESOURCE_BUSY);
	    return (E_DB_ERROR);
	}

	uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			NULL, 0, NULL, err_code, 0);
	uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG, NULL,
	    NULL, 0, NULL, err_code, 2, 0, LK_X, 0, xcb->xcb_lk_id);
	SETDBERR(&dmm->error, 0, E_DM011C_ERROR_DB_DESTROY);
	return (E_DB_ERROR);
    }

    /*	Wait for journals to be flushed by ACP. */

    db_key.lk_type = LK_JOURNAL;
    status = LKrequest(0, xcb->xcb_lk_id, (LK_LOCK_KEY *)&db_key,
	LK_X, (LK_VALUE *)0, (LK_LKID *)0, (i4)0, &sys_err);
    if (status != OK)
    {
	uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			NULL, 0, NULL, err_code, 0);
	uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG, NULL,
	    NULL, 0, NULL, err_code, 2, 0, LK_X, 0, xcb->xcb_lk_id);
	SETDBERR(&dmm->error, 0, E_DM011C_ERROR_DB_DESTROY);
	return (E_DB_ERROR);
    }

    /*
    ** Signal a Consistency Point to ensure that no log records describing
    ** to this database can be listed in the last CP or be thought to be
    ** needed in any REDO recovery cycle.
    */
    item = 1;
    status = LGalter(LG_A_CPNEEDED, (PTR)&item, sizeof(item), &sys_err);
    if (status != OK)
    {
	uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			NULL, 0, NULL, err_code, 0);
	uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code,
	    1, 0, LG_A_CPNEEDED);
	SETDBERR(&dmm->error, 0, E_DM011C_ERROR_DB_DESTROY);
	return (E_DB_ERROR);
    }

    /*
    ** Wait for the Consistency Point to finish.
    */
    status = LGevent(LG_INTRPTABLE, xcb->xcb_log_id, LG_E_ECPDONE, 
	&event, &sys_err);
    if (status != OK)
    {
	if (status == LG_INTERRUPT)
	{
	    TRdisplay("  Destroydb interrupted during wait for Consistency Point.\n");
	}
	else
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			NULL, 0, NULL, err_code, 0);
	    uleFormat(NULL, E_DM900F_BAD_LOG_EVENT, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code,
		1, 0, LG_E_ECPDONE);
	}
	SETDBERR(&dmm->error, 0, E_DM011C_ERROR_DB_DESTROY);
	return (E_DB_ERROR);
    }

    /*	Change config access/service. */
    if (dmm->dmm_db_access & DU_RDONLY)
	cnf_flag |= DM0C_READONLY;
    	
    open_status = dm0c_open(&dcb, cnf_flag, xcb->xcb_lk_id, &cnf, &open_dberr);
    if (open_status)
    {
	/* Open failed, so try rfc instead */
	open_status = dm0c_open(&dcb, DM0C_TRYRFC, xcb->xcb_lk_id,
			&cnf, &open_dberr);
	if (open_status == E_DB_OK)
	    used_rfc = TRUE;
    }


    /* Now loop through to find all directories associated with the  */
    /* database.                                                     */

    for (;;)
    {
        if (open_status != E_DB_OK)
	    break;

	/*
	**  Loop through the list of locations deleting any alias files that 
	**  might be present.
	*/

    	if (! (dmm->dmm_db_access & DU_RDONLY))
	{
	  for (i = 0; i < cnf->cnf_c_ext; i++)
	  {
	    /*  Skip journal, dump, work, checkpoint, and RAW locations. */

	    if (cnf->cnf_ext[i].ext_location.flags & (DCB_JOURNAL | DCB_DUMP | 
                                DCB_CHECKPOINT | DCB_WORK | DCB_AWORK | DCB_RAW))
		continue;

	    /*  Turn off any leftover alias flags. */
	    cnf->cnf_ext[i].ext_location.flags &= ~DCB_ALIAS;

	    /*
	    ** Delete any leftover alias files in the directory.
	    */
	    delete_info.error = OK;
	    delete_info.path_name = 
			(char *)&cnf->cnf_ext[i].ext_location.physical;
	    delete_info.path_length = cnf->cnf_ext[i].ext_location.phys_length;

	    status = DIlistfile(&di_context, delete_info.path_name,
		delete_info.path_length, del_alias_files, (PTR) &delete_info,
		&sys_err);

	    if ((status == DI_ENDFILE) || (status == DI_DIRNOTFOUND))
	    {
		status = OK;
		continue;
	    }
	    else
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
			NULL, 0, NULL, err_code, 0);
		uleFormat(NULL, E_DM9003_BAD_FILE_DELETE, &sys_err, ULE_LOG, NULL,
			NULL, 0, NULL, err_code, 4,
			sizeof(dcb.dcb_name), &dcb.dcb_name,
			4, "None",
			delete_info.path_length,
			delete_info.path_name,
			12, "zzzz0000.ali");
		break;
	    }
	  }

	  if (status)
	    break;

	  /*  Build alias file name. */

	  alias_file = *(DM_FILE *)"zzzz0000.ali";

	  /*  Now create new alias files to detect location aliasing. */

	  for (i = 0; i < cnf->cnf_c_ext; i++)
	  {
	    /*  Skip journal, dump, work, checkpoint, and RAW locations. */

	    if (cnf->cnf_ext[i].ext_location.flags & 
	    (DCB_JOURNAL | DCB_CHECKPOINT | DCB_DUMP | DCB_WORK | DCB_AWORK | DCB_RAW))
		continue;

	    /*  Create alias files in locations. */

	    status = DIcreate(&di_context,
		(char *)&cnf->cnf_ext[i].ext_location.physical,
		cnf->cnf_ext[i].ext_location.phys_length,
		(char *)&alias_file, 
		sizeof(alias_file), DM_PG_SIZE, &sys_err);

	    if (status == OK)
		continue;

	    if (status == DI_EXISTS)
	    {
		cnf->cnf_ext[i].ext_location.flags |= DCB_ALIAS;
		status = OK;
		continue;
	    }
	    else if (status == DI_DIRNOTFOUND)
	    {
		status = OK;
		continue;
	    }
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG, 
				NULL, NULL, 0, NULL, err_code, 0);
		uleFormat(NULL, E_DM9002_BAD_FILE_CREATE, &sys_err, ULE_LOG,
				NULL, NULL, 0, NULL, err_code, 4, 
				sizeof(dcb.dcb_name), &dcb.dcb_name,
				4, "None", 
				cnf->cnf_ext[i].ext_location.phys_length,
				(char *)&cnf->cnf_ext[i].ext_location.physical,
				sizeof(alias_file), (char *)&alias_file);
		break;
	    }
	  }
	  if (status)
	    break;
	}

	/*	Delete the DB,JNL,CKP,WORK, DUMP files and directories. */
        /*      Skip Alias and Raw Locations.                                   */

	for (i = 0; i < cnf->cnf_c_ext; i++)
	{
	    delete_info.error = OK;
	    delete_info.path_name = 
				(char *)&cnf->cnf_ext[i].ext_location.physical;
	    delete_info.path_length = cnf->cnf_ext[i].ext_location.phys_length;

	    /*
	    ** Note that DCB_RAW modifier will cause us to fall
	    ** into the default case, just what we want (do nothing).
	    */
	    switch (cnf->cnf_ext[i].ext_location.flags)
	    {
	    case DCB_ROOT:
	    case DCB_ROOT|DCB_DATA:
	    case DCB_DATA:
    		if (dmm->dmm_db_access & DU_RDONLY)
		    continue;
		/* otherwise, fall through */
	    case DCB_WORK:
	    case DCB_AWORK:
		status = DIlistfile(&di_context, delete_info.path_name,
		    delete_info.path_length, del_di_files, (PTR) &delete_info,
		    &sys_err);
		if (status == DI_ENDFILE)
		{
		    status = delete_info.error;
		    if (status == OK &&
			(cnf->cnf_ext[i].ext_location.flags & DCB_ROOT) == 0)
		    {
			status = DIdirdelete(&di_context, 
			    delete_info.path_name, delete_info.path_length,
			    0, 0, &sys_err);
                        if (status == DI_BADDELETE)
                        {
                            sys_err.moreinfo[0].size = delete_info.path_length;
                            STlcopy(delete_info.path_name, sys_err.moreinfo[0].data.string, delete_info.path_length);
                        }
		    }
		}
		else if (status == DI_DIRNOTFOUND)
		    status = OK;
		break;

	    case DCB_JOURNAL:
	    case DCB_DUMP:
		status = JFlistfile(&jf_io,
		    delete_info.path_name, delete_info.path_length,
		    del_jf_files, (PTR)&delete_info,
		    &sys_err);
		if (status == JF_END_FILE)
		{
		    status = delete_info.error;
		    if (status == OK)
			status = JFdirdelete(&jf_io,
			    delete_info.path_name, delete_info.path_length,
			    0, 0, &sys_err);
		}
		else if (status == JF_DIR_NOT_FOUND)
		    status = OK;
		break;

	    case DCB_CHECKPOINT:
		status = CKlistfile(
		    delete_info.path_name, delete_info.path_length,
		    del_ck_files, (PTR)&delete_info,
		    &sys_err);
		if (status == CK_ENDFILE)
		{
		    status = delete_info.error;
		    if (status == OK)
			status = CKdirdelete(
			    delete_info.path_name, delete_info.path_length,
			    0, 0, &sys_err);
		}
		else if (status == CK_DIRNOTFOUND)
		    status = OK;
		break;
     
            case DCB_ALIAS:
            default:   
                status = OK;

	    }
	    if (status)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			NULL, 0, NULL, err_code, 0);
		break;
	    }
	}
	if (status)
	    break;


	/* Clean up any cached DML_SEQ's for this database */
	dms_destroy_db(cnf->cnf_dsc->dsc_dbid);
	
	status = dm0c_close(cnf, 0, &dmm->error);
	if (status != OK)
	    break;
	config = 0;

	/*	Delete configuration file. */

	if (used_rfc)
	    /* it will have been deleted in the loop above */
	    break;

	if (dmm->dmm_db_access & DU_RDONLY)
	    loc = &rdloc;
	else
	    loc = &dcb.dcb_location;

	status = DIdelete(&di_context, (char *)&loc->physical,
	    		  loc->phys_length, "aaaaaaaa.cnf", 12, &sys_err);
	if (status != E_DB_OK)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
			NULL, 0, NULL, err_code, 0);
	    uleFormat(NULL, E_DM9003_BAD_FILE_DELETE, &sys_err, ULE_LOG, NULL,
		NULL, 0, NULL, err_code, 4,
		sizeof(dcb.dcb_name), &dcb.dcb_name,
		0, "Configuration_File",
		dmm->dmm_db_location.data_in_size,
		    dmm->dmm_db_location.data_address,
		12, "aaaaaaaa.cnf");
	    
	}
	break;
    }
    if (open_status != E_DB_OK && open_dberr.err_code != E_DM012A_CONFIG_NOT_FOUND)
    {
	uleFormat(&open_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			NULL, 0, NULL, err_code, 0);
    }
    if ((open_status != E_DB_OK && open_dberr.err_code == E_DM012A_CONFIG_NOT_FOUND)
	 || status == E_DB_OK)
    {
	/*	Delete database root directory. */

	status = DIdirdelete(&di_context, loc_dir, l_loc_dir,
	    db_buffer, db_length, &sys_err);
	if (status != E_DB_OK)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
			NULL, 0, NULL, err_code, 0);
	    uleFormat(NULL, E_DM9003_BAD_FILE_DELETE, &sys_err, ULE_LOG, NULL,
		NULL, 0, NULL, err_code, 4,
		sizeof(dcb.dcb_name), &dcb.dcb_name,
		0, "",
		dmm->dmm_db_location.data_in_size,
		    dmm->dmm_db_location.data_address,
		db_length, db_buffer);
	    SETDBERR(&dmm->error, 0, E_DM011C_ERROR_DB_DESTROY);
            return (E_DB_ERROR);
	}

	return (E_DB_OK);
    }

    if (dmm->error.err_code)
    {
	uleFormat(&dmm->error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			NULL, 0, NULL, err_code, 0);
    }

    if (config)
    {
	status = dm0c_close(cnf, 0, &dmm->error);
	if (status)
	    uleFormat(&dmm->error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			NULL, 0, NULL, err_code, 0);
    }

    SETDBERR(&dmm->error, 0, E_DM011C_ERROR_DB_DESTROY);
    return (E_DB_ERROR);
}

/*
** Name: del_di_files		- wrapper function around DIdelete.
**
** History:
**	26-apr-1993 (bryanp)
**	    Log CL error code if CL call fails.
**	25-Feb-1999 (shero03)
**	    Do case insensitive compare for aaaaaaaa.cnf for DOS & PKZIP
*/
static STATUS
del_di_files(
DELETE_INFO	*delete_info,
char		*file_name,
i4		file_length,
CL_ERR_DESC	*sys_err)
{
    DI_IO	di_context;
    DB_STATUS	err_code;

    if (file_length == 12 &&
        STncasecmp(file_name, "aaaaaaaa.cnf", 12) == 0)
	return (delete_info->error = OK);

    delete_info->error = DIdelete(&di_context,
	delete_info->path_name, delete_info->path_length,
	file_name, file_length, sys_err);
    if (delete_info->error == OK)
	return (OK);

    uleFormat(NULL, delete_info->error, (CL_ERR_DESC *)sys_err, ULE_LOG, NULL,
			NULL, 0, NULL, &err_code, 0);
    uleFormat(NULL, E_DM9003_BAD_FILE_DELETE, sys_err, ULE_LOG, NULL, 
	    NULL, 0, NULL,
	    &err_code, 4,
	    0, "",
	    0, "",
	    delete_info->path_length, delete_info->path_name,
	    file_length, file_name);
    return (FAIL);
}

/*
** Name: del_ck_files		- wrapper function around CKdelete.
**
** History:
**	26-apr-1993 (bryanp)
**	    Log CL error code if CL call fails.
**	18-aug-1995 (harpa06)
**	    If the user decided that they want to checkpoint
**	    their database into a directory (not TAR up the tables)
**	    then each checkpoint directory in a CKP location must 
**	    have all of it's files deleted and then the directory 
**	    removed.
**	25-mar-1996 (harpa06)
**	    Bug #75633: Access violation occurs during destroydb in lar SEP
**	    tests on NT due to an attempt in freeing memory already freed.
**	    Also did a little cleanup.
**      13-feb-2003 (chash01) x-integrate change#461908
**          Compiler complains that local variable f is not initialized.
**          Examine other DI calls indicate that f should be a pointer 
**          to  DI_IO structure.  Make f a DI_IO variable and pass its
**          pointer into DIdirdelete().
*/
static STATUS
del_ck_files(
DELETE_INFO	*delete_info,
char		*file_name,
i4		file_length,
CL_ERR_DESC	*sys_err)
{
    DB_STATUS	  err_code;
    LOCATION  	  loc;
    LOCATION      temploc;
    LOINFORMATION loinf;
    i4            flagword;
    PTR           dummy;
    char	  *temp;
    DELETE_INFO   new_delete_info;
    CL_ERR_DESC    new_sys_err;
    CL_ERR_DESC   *error_code;
    DI_IO	  f;


    delete_info->error = CKdelete(
	delete_info->path_name, delete_info->path_length,
	file_name, file_length, sys_err);
    if (delete_info->error == OK)
	return (OK);

    /* The removal of the file has failed, could it be that
    ** the file is a directory?
    */
    flagword=LO_I_TYPE;
    temp = (char *) MEreqmem (0, MAX_LOC + 1, TRUE, NULL);
    STncpy ( temp, delete_info->path_name, delete_info->path_length);
    temp[ delete_info->path_length ] = '\0';
    LOfroms (PATH, temp, &loc);
    dummy = MEreqmem (0, MAX_LOC + 1, TRUE, NULL);
    temp = (char *)  dummy;	
    STncpy(temp, file_name, STlength (file_name));
    temp[ STlength (file_name) ] = '\0';
    LOfroms (FILENAME, temp, &temploc);
    LOstfile (&temploc, &loc);
    if ((LOinfo (&loc, &flagword, &loinf) == OK) && (loinf.li_type == LO_IS_DIR))
    {
        /* It's a directory! Remove all files and subdirectories from within
	** this directory.
	*/
	new_delete_info.path_name = (char *) dummy;
        LOtos (&loc, &temp);
        STncpy ( new_delete_info.path_name, temp, STlength(temp));
	new_delete_info.path_name[ STlength (temp) ] = '\0';
	new_delete_info.path_length = STlength (temp);
	new_delete_info.pad = delete_info->pad;
	new_delete_info.error = 0;

	CKlistfile(new_delete_info.path_name, new_delete_info.path_length,
		   del_ck_files, (PTR)&new_delete_info, &new_sys_err);

	if (new_delete_info.error == OK)
	{
		/* Remove the directory we thought was a file. */

		error_code = (CL_ERR_DESC *) MEreqmem (0,sizeof (CL_ERR_DESC),
			     TRUE, NULL);
		delete_info->error = DIdirdelete (&f, new_delete_info.path_name, 
			     new_delete_info.path_length, NULL, 0, error_code);
		MEfree (temp);
		MEfree ((PTR)error_code);
		MEfree (new_delete_info.path_name);
		return (OK);
	}

        else 
	{
		/* We could not delete all files and directories in this
		** subdirectory. */

		MEfree (temp);
		MEfree (new_delete_info.path_name);
		return (FAIL);
	}
    }
	
    else 
    {
        /* we could not delete the file and it is not a subdirectory. */

    	uleFormat(NULL, delete_info->error, (CL_ERR_DESC *)sys_err, ULE_LOG, NULL,
			NULL, 0, NULL, &err_code, 0);
    	uleFormat(NULL, E_DM9003_BAD_FILE_DELETE, sys_err, ULE_LOG, NULL, 
	    NULL, 0, NULL,
	    &err_code, 4,
	    0, "",
	    0, "",
	    delete_info->path_length, delete_info->path_name,
	    file_length, file_name);

	MEfree (temp);
    	return (FAIL);
    }
}

/*
** Name: del_jf_files		- wrapper function around JFdelete.
**
** History:
**	26-apr-1993 (bryanp)
**	    Log CL error code if CL call fails.
*/
static STATUS
del_jf_files(
DELETE_INFO	*delete_info,
char		*file_name,
i4		file_length,
CL_ERR_DESC	*sys_err)
{
    JFIO	jf_io;
    DB_STATUS	err_code;

    delete_info->error = JFdelete(&jf_io,
	delete_info->path_name, delete_info->path_length,
	file_name, file_length, sys_err);
    if (delete_info->error == OK)
	return (OK);

    uleFormat(NULL, delete_info->error, (CL_ERR_DESC *)sys_err, ULE_LOG, NULL,
		    NULL, 0, NULL, &err_code, 0);
    uleFormat(NULL, E_DM9003_BAD_FILE_DELETE, sys_err, ULE_LOG, NULL, 
	    NULL, 0, NULL,
	    &err_code, 4,
	    0, "",
	    0, "",
	    delete_info->path_length, delete_info->path_name,
	    file_length, file_name);
    return (FAIL);
}


/*
** Name: destroy_loc_list       Destroy database areas using dmm_loc_list 
**
** Inputs:
**      dmm_cb                  dmm control block
**
** Outputs:
**      Returns:
**          E_DB_OK, E_DB_ERROR
**      err_code                reason for status != E_DB_OK
**
** History:
**      31-jan-1995 (stial01)
**          BUG 66510: written.
*/
static DB_STATUS
destroy_loc_list(
DMM_CB    *dmm_cb)
{
    DMM_CB              *dmm = dmm_cb;
    char                temp_loc[DB_AREA_MAX + 4];
    LOCATION            loc;
    char                *cp;
    i4             i;
    DMM_LOC_LIST        **loc_list;
    CL_ERR_DESC          sys_err;
    DI_IO               di_context;
    JFIO                jf_io;
    DELETE_INFO         delete_info;
    STATUS              status = E_DB_OK;
    char                *lo;
    char                db_buffer[sizeof(DB_DB_NAME) + sizeof(int)];
    i4			error;

    loc_list = (DMM_LOC_LIST **)dmm->dmm_loc_list.ptr_address;

    /* If dmm_loc_list was not init, let destroydb proceed anyway */
    if (dmm->dmm_loc_list.ptr_in_count == 0 || !loc_list)
	return (E_DB_OK);

    /* Construct database name */
    for (i = 0; i < sizeof(DB_DB_NAME); i++)
        if ((db_buffer[i] = dmm->dmm_db_name.db_db_name[i]) == ' ')
            break;
    db_buffer[i] = 0;

    do
    {
	/* Delete all directories for this database */
	for (i = 0; i < dmm->dmm_loc_list.ptr_in_count; i++)
	{
	    /* If no loc_list entry, let destroydb proceed */
	    if (!loc_list[i])
		break;    

	    switch (loc_list[i]->loc_type)
	    {
		case DMM_L_JNL:  lo = LO_JNL;
				 break;
		case DMM_L_CKP:  lo = LO_CKP;
				 break;
		case DMM_L_DMP:  lo = LO_DMP;
				 break;
		case DMM_L_WRK:  lo = LO_WORK;
				 break;
		case DMM_L_AWRK: lo = LO_WORK;
				 break;
		case DMM_L_DATA: lo = LO_DB;
				 break;
		default:         continue;
	    }

	    /* Get path for location */
	    MEmove(loc_list[i]->loc_l_area, loc_list[i]->loc_area, 0,
			sizeof(temp_loc), temp_loc);
	    status = LOingpath(temp_loc, db_buffer, lo, &loc);
	    if (!status)
		LOtos(&loc, &cp);

	    if (status || *cp == EOS)
		continue;

	    delete_info.error = OK;
	    delete_info.path_name = cp; 
	    delete_info.path_length = STlength(cp);

	    /*
	    ** Delete the files in the path, then delete the directory
	    */
	    switch (loc_list[i]->loc_type)
	    {
	    case DMM_L_DATA:
	    case DMM_L_WRK:
	    case DMM_L_AWRK:
		status = DIlistfile(&di_context, delete_info.path_name,
		    delete_info.path_length, del_all_files, (PTR) &delete_info,
		    &sys_err);
		if (status == DI_ENDFILE)
		{
		    status = delete_info.error;
		    if (status == OK)
		    {
			status = DIdirdelete(&di_context, 
			    delete_info.path_name, delete_info.path_length,
			    0, 0, &sys_err);
		    }
		}
		else if (status == DI_DIRNOTFOUND)
		    status = OK;
		break;

	    case DMM_L_JNL:
	    case DMM_L_DMP:
		status = JFlistfile(&jf_io,
		    delete_info.path_name, delete_info.path_length,
		    del_jf_files, (PTR)&delete_info,
		    &sys_err);
		if (status == JF_END_FILE)
		{
		    status = delete_info.error;
		    if (status == OK)
			status = JFdirdelete(&jf_io,
			    delete_info.path_name, delete_info.path_length,
			    0, 0, &sys_err);
		}
		else if (status == JF_DIR_NOT_FOUND)
		    status = OK;
		break;

	    case DMM_L_CKP:
		status = CKlistfile(
		    delete_info.path_name, delete_info.path_length,
		    del_ck_files, (PTR)&delete_info,
		    &sys_err);
		if (status == CK_ENDFILE)
		{
		    status = delete_info.error;
		    if (status == OK)
			status = CKdirdelete(
			    delete_info.path_name, delete_info.path_length,
			    0, 0, &sys_err);
		}
		else if (status == CK_DIRNOTFOUND)
		    status = OK;
		break;
	    }
	    if (status)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			NULL, 0, NULL, &error, 0);
		break;
	    }
	}
    } while (FALSE);

    return (status);
}
/*
** Name: del_all_files		- wrapper function around DIdelete.
**
** History:
**      31-jan-1995 (stial01)
**          BUG 66510: written.
*/
static STATUS
del_all_files(
DELETE_INFO	*delete_info,
char		*file_name,
i4		file_length,
CL_ERR_DESC	*sys_err)
{
    DI_IO	di_context;
    DB_STATUS	err_code;

    delete_info->error = DIdelete(&di_context,
	delete_info->path_name, delete_info->path_length,
	file_name, file_length, sys_err);
    if (delete_info->error == OK)
	return (OK);

    uleFormat(NULL, delete_info->error, (CL_ERR_DESC *)sys_err, ULE_LOG, NULL,
			NULL, 0, NULL, &err_code, 0);
    uleFormat(NULL, E_DM9003_BAD_FILE_DELETE, sys_err, ULE_LOG, NULL, 
	    NULL, 0, NULL,
	    &err_code, 4,
	    0, "",
	    0, "",
	    delete_info->path_length, delete_info->path_name,
	    file_length, file_name);
    return (FAIL);
}
/*{
** Name: del_alias_files - Delete all alias files in a directory.
**
** Description:
**      This routine is called to delete all alias filesin a directory.
**
**      The following files will be deleted:
**
**              *.ali                               Alias files.
**
**
** Inputs:
**      Delete_info                     File info block.
**      file_name                        Pointer to filename.
**      file_length                      Length of file name.
**
** Outputs:
**      delete_info->error               Operating system error.
**      Returns:
**          OK
**          FAIL
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      06-jun-1996 (strpa01)
**      Created del_alias_files() as part of alias file processing.
[@history_template@]...
*/
static DB_STATUS
del_alias_files(
DELETE_INFO     *delete_info,
char            *file_name,
i4              file_length,
CL_ERR_DESC      *sys_err)
{
    DI_IO       di_context;
    DB_STATUS   err_code;

    /*
    ** If file does not end in ".ali", then don't delete it.
    */
    if ((file_length != sizeof(DM_FILE)) || (file_name[8] != '.') ||
        (MEcmp(&file_name[9], "ali", 3) != 0))
    {
        return (OK);
    }

    delete_info->error = DIdelete(&di_context,
        delete_info->path_name, delete_info->path_length,
        file_name, file_length, sys_err);
    if (delete_info->error == OK)
        return (OK);

    uleFormat(NULL, delete_info->error, (CL_ERR_DESC *)sys_err, ULE_LOG, NULL,
			NULL, 0, NULL, &err_code, 0);
    uleFormat(NULL, E_DM9003_BAD_FILE_DELETE, sys_err, ULE_LOG, NULL, 
	    NULL, 0, NULL,
            &err_code, 4,
            0, "",
            0, "",
            delete_info->path_length, delete_info->path_name,
            file_length, file_name);
    return (FAIL);

}

