/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <pc.h>
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
#include    <dm2t.h>
#include    <dm2r.h>
#include    <dm0m.h>
#include    <dmm.h>

/**
** Name:  DMMLIST.C - Function used to interface to DIlistxxx() functions.
**
** Description:
**      This file contains the functions necessary to interface to list files
**	or directories subordinate to specified path.  (Of course, specified
**	path is always a directory.)
**
**  This file defines:
**    
**      dmm_list()    -  Cause contents of a directory to be listed.
**	dmm_makelist()-  Do DI calls
**	dmm_putlist() -  Called by DIlistdir or DIlistfile to put file/dir
**			 name into user's list table
**
** History:
**      22-Mar-89 (teg)
**	    Initial Creation (for 6.2).
**	21-jun-89 (greg)
**	    Hack around reference to CL_ERR_DESC member.
**	    This must be fixed correctly for 6/26 code freeze.
**	19-nov-90 (rogerk)
**	    Initialized timezone and collating sequence in the adf cb.
**	    This was done as part of the DBMS Timezone changes.
**	 4-dec-91 (rogerk)
**	    Added support for Set Nologging.
**	15-oct-91 (mikem)
**	    Eliminated unix compiler warning in dmm_makelist().
**	 7-jul-1992 (roger)
**	    Prototyping DMF.
**      25-sep-92 (stevet)
**          Changed svcb_timezone to svcb_tzcb to support new timezone
**          adjustment methods.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system typedefs
**	21-jun-1993 (rogerk)
**	    Fix prototype warning.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**      05-jul-1995 (chech02)
**          If option -opurge is specified in verifydb, the interal
**          database procedure, iiQEF_list_file, is called. Then we
**          will get SEGV, because the rcb->rcb_xcb_ptr is null.
**          The fix is made by Darrin and Ranjit to assign xcb to 
**          rcb->rcb_xcb_ptr in dmm_makelist().
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Added E_DM006A_TRAN_ACCESS_CONFLICT.
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	05-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_?, dm2r_? functions converted to DB_ERROR *
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/


#define             DMM_LIST_ARGCNT     7


/*{
** Name: dmm_list - List files and/or subdirectories subordinate to directory
**		    specified by path:
**
**  INTERNAL DMF call format:    status = dmm_list(&dmm_cb);
**
**  EXTERNAL call format:        status = dmf_call(DMM_DIRLIST, &dmm_cb);
**
** Description:
**	The dmm_list function will supply information about what files and/or
**	subdirectories are subordinate to a specified directory.  This directory
**	should be an INGRES default or extended data location.
**
**	The list function works as follows:  Use will create a table with
**	a column that will hold the list results.  DMF will open that table,
**	put a tuple in for each file/dir it finds, and close that table when
**	the DIlist function is complete.  The user is responsible to empty the
**	DIlist table before calling the QEFLISTFILE or QEFLISTDIR commands.
**	It is also the user's responsibility to know which location the
**	list table is for.  THIS IS A VERY SPECIAL PURPOSE INTERFACE, AND WILL
**      PROBABLY NEVER BE VISIBLE TO THE USER.
**
**      dmm_cb 
**          .type                           Must be set to DMM_MAINT_CB.
**          .length                         Must be at least sizeof(DMM_CB).
**          .dmm_flags_mask                 DMM_FILELIST, DMM_DIRLIST bitmap
**          .dmm_tran_id                    Must be the current transaction 
**                                          identifier returned from the 
**                                          begin transaction operation.
**          .dmm_db_location                full pathname of area to search
**          .dmm_table_name                 name of user table to put list into
**          .dmm_owner                      owner of above user table
**          .dmm_att_name                   Name of attribute to put list into
**      
** Outputs:
**                                          of table created.
**      dmm_cb   
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK
**					    E_DM005D_TABLE_ACCESS_CONFLICT
**					    E_DM0010_BAD_DB_ID
**					    E_DM0064_USER_ABORT
**					    E_DM010C_TRAN_ABORTED
**					    E_DM001A_BAD_FLAG 
**					    E_DM003B_BAD_TRAN_ID
**					    E_DM0065_USER_INTR
**	    				    E_DM006A_TRAN_ACCESS_CONFLICT
**					    E_DM0130_DMMLIST_ERROR
**					    
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dmm_cb.error.err_code.
**          E_DB_FATAL                      Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmm_cb.error.err_code.
**
** History:
**      22-mar-89 (teg)
**          Initial Creation.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Added E_DM006A_TRAN_ACCESS_CONFLICT.
**	6-Feb-2004 (schka24)
**	    Get rid of DMU count.
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
*/
DB_STATUS
dmm_list(
DMM_CB        *dmm_cb)
{
    DMM_CB	    *dmm = dmm_cb;
    DML_XCB	    *xcb;
    DML_ODCB	    *odcb;
    i4         db_lockmode;
    i4	    local_error;
    i4	    status;
    i4	    valid_flagmask;
    i4		    *err_code = &dmm->error.err_code;

    CLRDBERR(&dmm->error);

    status = E_DB_ERROR;
    for (;;)
    {
	valid_flagmask = ~(DMM_FILELIST | DMM_DIRLIST);

	if ( (dmm->dmm_flags_mask & valid_flagmask) != 0)
	{
	    SETDBERR(&dmm->error, 0, E_DM001A_BAD_FLAG);
	    break;
	}

	/* must specify one type of list as minimun */
	if ( dmm->dmm_flags_mask == 0)
	{
	    SETDBERR(&dmm->error, 0, E_DM001A_BAD_FLAG);
	    break;
	}

	/* verify table,owner,column names are specified */
	if ( (dmm->dmm_table_name.db_tab_name[0] == EOS) ||
	     (dmm->dmm_owner.db_own_name[0] == EOS) )
	{
	    SETDBERR(&dmm->error, 0, E_DM918E_SPECIFY_LIST_TABLE);
	    break;
	}
	if (dmm->dmm_att_name.db_att_name[0] == EOS)
	{
	    SETDBERR(&dmm->error, 0, E_DM918C_SPECIFY_LIST_COLNAME);
	    break;
	}

	/* verify environment is suitable for continuing */
	xcb = (DML_XCB*) dmm->dmm_tran_id;
	if (dm0m_check((DM_OBJECT *)xcb, (i4)XCB_CB) != E_DB_OK)
	{
	    SETDBERR(&dmm->error, 0, E_DM003B_BAD_TRAN_ID);
	    break;
	}

	/* Check for external interrupts */
	if ( xcb->xcb_scb_ptr->scb_ui_state )
	    dmxCheckForInterrupt(xcb, err_code);

	if (xcb->xcb_state & XCB_USER_INTR)
	{
	    SETDBERR(&dmm->error, 0, E_DM0065_USER_INTR);
	    break;
	}
	if (xcb->xcb_state & XCB_FORCE_ABORT)
	{
	    SETDBERR(&dmm->error, 0, E_DM010C_TRAN_ABORTED);
	    break;
	}
	if (xcb->xcb_state & XCB_ABORT)
	{
	    SETDBERR(&dmm->error, 0, E_DM0064_USER_ABORT);
	    break;
	}	    

	odcb = (DML_ODCB*) xcb->xcb_odcb_ptr;
	if (dm0m_check((DM_OBJECT *)odcb, (i4)ODCB_CB) != E_DB_OK)
	{
	    SETDBERR(&dmm->error, 0, E_DM0010_BAD_DB_ID);
	    break;
	}
	if (xcb->xcb_scb_ptr != odcb->odcb_scb_ptr)
	{
	    SETDBERR(&dmm->error, 0, E_DM0010_BAD_DB_ID);
	    break;
	}

	/*  Check that this is a update transaction on the database 
        **  that can be updated. */

	if (odcb != xcb->xcb_odcb_ptr)
	{
	    SETDBERR(&dmm->error, 0, E_DM005D_TABLE_ACCESS_CONFLICT);
	    break;
	}

	if (xcb->xcb_x_type & XCB_RONLY)
	{
	    SETDBERR(&dmm->error, 0, E_DM006A_TRAN_ACCESS_CONFLICT);
	    break;
	}

	db_lockmode = DM2T_S;
	if (odcb->odcb_lk_mode == ODCB_X)
	    db_lockmode = DM2T_X;


	/* If it gets here, then input parameters were OK. */

	/* get a list of the directories/files subordinate to specified path. 
	** (Error code will be put into variable error).
	*/

	status = dmm_makelist(odcb->odcb_dcb_ptr, xcb, dmm->dmm_flags_mask,
			       &dmm->dmm_table_name, &dmm->dmm_owner, 
			       &dmm->dmm_att_name,
			       dmm->dmm_db_location.data_address,
			       dmm->dmm_db_location.data_in_size, 
			       &dmm->dmm_count, &dmm->error); 
	break;	    

    } /* end for */

   
    if (status != E_DB_OK)
    {
	if (dmm->error.err_code > E_DM_INTERNAL)
	{
	    uleFormat( &dmm->error, 0, NULL, ULE_LOG , NULL, 
		(char * )NULL, 0L, (i4 *)NULL, 
		&local_error, 0);
	    SETDBERR(&dmm->error, 0, E_DM0130_DMMLIST_ERROR);
	}
	switch (dmm->error.err_code)
	{
	    case E_DM004B_LOCK_QUOTA_EXCEEDED:
	    case E_DM0112_RESOURCE_QUOTA_EXCEED:
	    case E_DM0130_DMMLIST_ERROR:
	    case E_DM006A_TRAN_ACCESS_CONFLICT:
	    	xcb->xcb_state |= XCB_STMTABORT;
		break;

	    case E_DM0042_DEADLOCK:
	    case E_DM004A_INTERNAL_ERROR:
	    case E_DM0100_DB_INCONSISTENT:
		xcb->xcb_state |= XCB_TRANABORT;
		break;
	    case E_DM0065_USER_INTR:
		xcb->xcb_state |= XCB_USER_INTR;
		break;
	    case E_DM010C_TRAN_ABORTED:
		xcb->xcb_state |= XCB_FORCE_ABORT;
		break;
	}
    }

    return (status);

}

/*{
** Name: dmm_makelist	- list all files and/or subdirectories in a directory
**
** Description:
**      This routine list all files and/or subdirectories found under a
**	directory, as specified by the value of flag (bitmap with 
**	DMM_FILELIST and/or DMM_DIRLIST).  It puts these files into a
**	user specified relation.  There are very strict requirements about
**	what this relation looks like.  For now, the relation must be a
**	single tuple that is large enough to hold a character string
**	DI_FILENAME_MAX long.  HOPEFULLY, THIS RESTRICTION WILL BE RELAXED IN
**	THE FUTURE.  (This strict restriction is there because I do not know
**	how to interface to ADF to build a blank tuple and insert the
**	formatted file/dir name attribute into the correct location of that
**	tuple.  This should be added as soon as there is time.)
**
**	Basically, dmm_makelist searches iirelation to get the table id
**	for the user specified table.  It validates the table meets the
**	format requirements for a DIlistxxx() function. If so, it opens the
**	table (dm2t_open), then calls DIlistdir() and/or DIlistfile() to
**	get information about the path.  The DIlistxxx() routine will call
**	dmm_putlist() to put the file into the open relation, using
**	dm2r_put().  After the DIlistxxx() function returns to dmm_makelist,
**	dmm_makelist will close the open table (dm2t_close).
**
**	Of course, there are lots of support functions that must be performed,
**	such as allocating memory, setting up ADF control/data blocks so that
**	ADF can format the tuple, etc.
**
** Inputs:
**
**	dcb				database control block
**	xcb				transaction control block
**	flag				bitmap of list type: DMM_FILELIST, DMM_DIRLIST
**	table				name of table to list files into
**	owner				owner of table to list files into
**	att_name			column in table to hold filenames
**	path				full pathname of directory to list
**	pathlen				number of characters in path
**
** Outputs:
**	tuple_cnt			number of files found during list and
**					 put into user specified table
**	error				error status
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    DMM_PUTLIST() is called by DIlistdir() or DIlistfile()
**
** Side Effects:
**	    tuples are placed in user's table for each directory/file found
**
** History:
**      22-mar-89 (teg)
**          Initial Creation.
**	07-jun-90 (teresa)
**	    modify to stop considering empty directories as an error condition.
**	19-nov-90 (rogerk)
**	    Initialized timezone and collating sequence in the adf cb.
**	    These fields may not have been absolutely necessary, but it
**	    seemed safer to not leave them garbage.
**	    This was done as part of the DBMS Timezone changes.
**	29-jan-90 (teresa)
**	    dm0m_deallocate does not zero ptr it is passed, so manually clear 
**	    it to avoid double deallocation of the object.  (Portability bug)
**	29-jan-91 (teresa)
**	    UNIX portability bug -- dm0m_deallcate does not return status, so
**	    do not check status.  Also, dm0m_deallocate does not zero the
**	    ptr it is passed, so clear it afterwards.
**	 4-feb-1991 (rogerk)
**	    Add support for Set Nologging.  If transaction is Nologging, then
**	    mark table-opens done here as nologging.
**	09-oct-91 (jrb)
**	    Merged 6.4 changes into 6.5 (actually, merged 6.5 to 6.4 then
**	    replaced in 6.5).
**	15-oct-91 (mikem)
**	    Eliminated unix compiler warning ("illegal structure pointer 
**	    combination") by casting value assigned to "tuple" to be the 
**	    correct type (DMP_MISC).
**      25-sep-92 (stevet)
**          Changed svcb_timezone to svcb_tzcb to support new timezone
**          adjustment methods.
**	21-jun-1993 (rogerk)
**	    Fix prototype warnings.
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here.
**      17-Apr-2008 (kibro01) b120276
**          Initialise ADF_CB structure
[@history_template@]...
*/
DB_STATUS
dmm_makelist(
DMP_DCB			*dcb,
DML_XCB			*xcb,
i4			flag, /* DMM_FILELIST, DMM_DIRLIST */
DB_TAB_NAME		*table,
DB_OWN_NAME		*owner,
DB_ATT_NAME		*att_name,
char			*path,
i4			pathlen,
i4			*tuple_cnt,
DB_ERROR		*dberr)
{
	i4	*arglist[DMM_LIST_ARGCNT];	/* arguments for dmm_putlist --
						**  arglist[0] RCB ptr
						**  arglist[1] ADF_CB ptr
						**  arglist[2] in DB_DATA_VALUE 
						**  arglist[3] out DB_DATA_VALUE
						**  arglist[4] tuple cnt
						**  arglist[5] error code ptr
						**  arglist[6] ptr to tuple buf
						*/
	i4		    *err_code = &dberr->err_code; /* holds error code */
	i4		    tempstat,locstat;		/* local status */
	DB_TAB_TIMESTAMP    timestamp;
	DMP_RCB		    *r_idx = NULL;		/* rcb for iirel_idx*/
	DMP_RCB		    *rel = NULL;		/* rcb for iirelation*/
	DMP_RCB		    *rcb = NULL;		/* rcb for table */
	DM2R_KEY_DESC	    key_desc[2];		/* search key */
	DM_TID		    tid, rel_tid;
	DMP_RINDEX	    rindexrecord;
	DMP_RELATION	    relation;
	ADF_CB              adf_cb;
	DB_DATA_VALUE	    from_dv;
	DB_DATA_VALUE	    to_dv;
	DB_DATA_VALUE	    empty_dv;
	DI_IO		    io_cb;
	STATUS		    status;
	CL_ERR_DESC	    sys_err_code;
	DB_TAB_ID	    table_id;
	DB_TRAN_ID	    tran_id;
	DMP_TCB		    *tcb;
	i4		    row_cnt = 0;
	i4		    i;			/* ye 'ole faithful counter */
	DMP_MISC	    *tuple = NULL;
	i2		    col_id;
	i4		    cmp_res;
	DB_ERROR	    local_dberr;

    CLRDBERR(dberr);
	
    for (;;)	/* begin control loop */
    {

	/*
	**  get table id from iirelation, iirel_idx catalogs.
	*/

	/* Find iirelation tuple using 2ndary index */

	table_id.db_tab_base = DM_B_RIDX_TAB_ID;
	table_id.db_tab_index = DM_I_RIDX_TAB_ID;
	tran_id.db_high_tran = 0;
	tran_id.db_low_tran = 0;

	locstat = dm2t_open(dcb, &table_id, DM2T_IX,
	    DM2T_UDIRECT, DM2T_A_READ, (i4)0, (i4)20, 
	    0, 0,dmf_svcb->svcb_lock_list, (i4)0,(i4)0, 
	    0,&tran_id, &timestamp, &r_idx, (DML_SCB *)0, dberr);
	if (locstat != E_DB_OK)
	{
	    SETDBERR(dberr, 0, E_DM918B_DMM_SYSCAT_ERROR);
	    break;
	}

	/*  Position table using table name and user id. */

	key_desc[0].attr_operator = DM2R_EQ;
	key_desc[0].attr_number = DM_1_RELIDX_KEY;
	key_desc[0].attr_value = table->db_tab_name;
	key_desc[1].attr_operator = DM2R_EQ;
	key_desc[1].attr_number = DM_2_RELIDX_KEY;
	key_desc[1].attr_value = owner->db_own_name;

	for (;;)
	{
	    locstat = dm2r_position(r_idx, DM2R_QUAL, key_desc, 
		      (i4)2,
		      (DM_TID *)0, dberr);
	    if (locstat != E_DB_OK)
		break;	    

	    /* 
	    ** Retrieve records with this key, should find one. 
	    */
	    locstat = dm2r_get(r_idx, &tid, DM2R_GETNEXT, 
		    (char *)&rindexrecord, dberr);

	    if (locstat != E_DB_OK && dberr->err_code != E_DM0055_NONEXT)
		break;
	    if (locstat == E_DB_OK)
	    {
		rel_tid.tid_i4 = rindexrecord.tidp;
		break;
	    }
	    break;
	} /* End retrieve from secondary index loop. */

	if (locstat != E_DB_OK)
	{
	    if (dberr->err_code == E_DM0055_NONEXT)
		SETDBERR(dberr, 0, E_DM918A_LISTTAB_NONEXISTENT);
	    SETDBERR(dberr, 0, E_DM918A_LISTTAB_NONEXISTENT);
	    break;
	}

	/* Close the secondary index table, no longer needed. */
	locstat = dm2t_close(r_idx, DM2T_NOPURGE, dberr);
	r_idx = NULL;
	if (locstat != E_DB_OK)
	    break; 

	/* now open iirelation table so we can get table id
	*/

	table_id.db_tab_base = DM_B_RELATION_TAB_ID;
	table_id.db_tab_index = DM_I_RELATION_TAB_ID;
	tran_id.db_high_tran = 0;
	tran_id.db_low_tran = 0;

	locstat = dm2t_open(dcb, &table_id, DM2T_IX,
			DM2T_UDIRECT, DM2T_A_READ, (i4)0, (i4)20, 
			0, 0,dmf_svcb->svcb_lock_list, (i4)0,(i4)0, 
			0,&tran_id, &timestamp, &rel, (DML_SCB *)0, dberr);
	if (locstat != E_DB_OK)
	    break; 

	/* get iirelation tuple */
	locstat = dm2r_get(rel, &rel_tid, DM2R_BYTID, (char *)&relation,
				dberr);
	if (locstat != E_DB_OK)
	    break;

	/* Close the iirelation table, no longer needed. */
	locstat = dm2t_close(rel, DM2T_NOPURGE, dberr);
	rel = NULL;
	if (locstat != E_DB_OK)
	    break;

	/* now open the user table to hold filenames/dirnames found when
	** a DIlist is done for the specified directory.
	*/
	table_id.db_tab_base = relation.reltid.db_tab_base;
	table_id.db_tab_index = relation.reltid.db_tab_index;

	locstat = dm2t_open(dcb, &table_id, DM2T_X,  DM2T_UDIRECT, DM2T_A_WRITE,
			    (i4)0, (i4)20, (i4)0, 
			    xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0, 
			    (i4)0, (i4)0, &xcb->xcb_tran_id, 
			    &timestamp, &rcb, (DML_SCB *)0, dberr);
	if (locstat != E_DB_OK)
	{
	    SETDBERR(dberr, 0, E_DM9189_DMM_LISTTAB_OPEN_ERR);
	    break;
	}
	rcb->rcb_xcb_ptr = xcb;	/* Darrin is responsible...... */
	/*
	** Check for NOLOGGING - no updates should be written to the log
	** file if this session is running without logging.
	*/
	if (xcb->xcb_flags & XCB_NOLOGGING)
	    rcb->rcb_logging = 0;

	/* for now we only permit listing into single column tables
	** that will hold name -- this restriction should be lifted
	** soon, and this check should be eleminated.
	*/
	tcb = rcb -> rcb_tcb_ptr;
	locstat = dm0m_allocate( sizeof(DMP_MISC) + 
				(i4)tcb->tcb_rel.relwid, DM0M_ZERO, 
				(i4) MISC_CB, (i4) MISC_ASCII_ID,
				(char *) 0, (DM_OBJECT **) &tuple, dberr);
	if (locstat != E_DB_OK)
	{
	    SETDBERR(dberr, 0, E_DM9188_DMM_ALLOC_ERROR);
	    break;
	}

	/* 
	**  build adf control block, so we can use adf to build empty tuple
	*/
	MEfill(sizeof(ADF_CB),0,(PTR)&adf_cb);
	adf_cb.adf_maxstring = DB_MAXSTRING;
	adf_cb.adf_collation = dcb->dcb_collation;
	adf_cb.adf_tzcb = dmf_svcb->svcb_tzcb;

	/* 
	**  initialize empty tuple, determine which column to list to 
	*/
	tuple->misc_data = (char *)&tuple[1];
	for (i=1, col_id = 0; i<= tcb->tcb_rel.relatts; i++)
	{
	    DB_ATT_NAME	tmpattnm;

	    MEmove(tcb->tcb_atts_ptr[i].attnmlen, tcb->tcb_atts_ptr[i].attnmstr,
		' ', DB_ATT_MAXNAME, tmpattnm.db_att_name);

	    /* see if this col is specified col */
    	    cmp_res = MEcmp( tmpattnm.db_att_name, att_name->db_att_name,
		sizeof(DB_ATT_NAME) );
	    if (cmp_res == 0)
		col_id = i;
	
	    /* now build empty entry for this attribute */
	    empty_dv.db_length = tcb->tcb_atts_ptr[i].length;
	    empty_dv.db_datatype = (DB_DT_ID) tcb->tcb_atts_ptr[i].type;
	    empty_dv.db_data = 
		(PTR) &tuple->misc_data[tcb->tcb_atts_ptr[i].offset];
	    locstat = adc_getempty( &adf_cb, &empty_dv);
	    if (locstat != E_DB_OK)
	    {
		/* generate error message that name could not be cohersed into 
		** table, then return with nonzero status to DIlistxxx().
		*/
		if ( adf_cb.adf_errcb.ad_emsglen)
		{
		    uleFormat(NULL, adf_cb.adf_errcb.ad_errcode, NULL, ULE_MESSAGE, NULL, 
			       adf_cb.adf_errcb.ad_errmsgp, 
			       adf_cb.adf_errcb.ad_emsglen, 0, err_code, 0);
		    SETDBERR(dberr, 0, E_DM9182_CANT_MAKE_LIST_TUPLE);
		}
		else if	(adf_cb.adf_errcb.ad_errcode)
		{
		    *dberr = adf_cb.adf_errcb.ad_dberror;
		}
		return (E_DB_ERROR);
	    } /* end if error encountered obtaining empty attribute from ADF */
	} /* end loop for each attribute in table */

	/* assure that column user specified is valid -- ie, in table */
	if (col_id == 0)
	{
	    locstat = E_DB_ERROR;
	    SETDBERR(dberr, 0, E_DM9187_INVALID_LIST_TBL_FMT);
	    break;
	}

	/* assure user table is valid -- list col is big enough. */
	if (tcb->tcb_atts_ptr[col_id].length < DI_FILENAME_MAX)
	{
	    locstat = E_DB_ERROR;
	    SETDBERR(dberr, 0, E_DM9187_INVALID_LIST_TBL_FMT);
	    break;
	}

	/* 
	**	build ADF data blocks.  These will be parameters
	**	to routine DMM_PUTLIST, which is called by DIlistdir().
	*/
	from_dv.db_data = NULL;
	from_dv.db_length = 0;
	from_dv.db_datatype = DB_CHA_TYPE;
	to_dv.db_length = tcb->tcb_atts_ptr[col_id].length;
	to_dv.db_datatype = (DB_DT_ID) tcb->tcb_atts_ptr[col_id].type;
	to_dv.db_data = 
		(PTR) &tuple->misc_data[tcb->tcb_atts_ptr[col_id].offset];

	/* build parameters for DMM_PUTLIST, which will be called from 
	**  DIlistxxx():
	**	arglist[0]	Pointer to RCB for table.
	**	arglist[1]	Pointer to initialized ADF_CB
	**	arglist[2]	Pointer to ADF's IN DB_DATA_VALUE
	**	arglist[3]	Pointer to ADF's OUT DB_DATA_VALUE
	**	arglist[4]	Ptr to counter (of # rows inserted into table)
	**	arglist[5]	Ptr to err_cb for any errors that occur.
	**	arglist[6]	Ptr to tuple buffer
	*/
	arglist[0] = (i4 *) rcb;
	arglist[1] = (i4 *) &adf_cb;
	arglist[2] = (i4 *) &from_dv;
	arglist[3] = (i4 *) &to_dv;
	arglist[4] = (i4 *) &row_cnt;
	arglist[5] = (i4 *) err_code;
	arglist[6] = (i4 *) tuple;


	if (flag & DMM_FILELIST) 
	{
	    *err_code = E_DM0140_EMPTY_DIRECTORY;
	    status = DIlistfile(&io_cb, path, pathlen, dmm_putlist,
				(PTR) arglist, &sys_err_code);
	    if (status == DI_ENDFILE || status == OK)
	    {
		/* If an error occurred placing an entry in the user supplied
		** table, then arglist[5] (error) will contain an error code.
		** this error code should be logged and translated to a
		*/
		if (*err_code == E_DM0140_EMPTY_DIRECTORY)
		{
		    /* the directory to search was empty.  This is NOT
		    ** considered an error */
		    *err_code = 0;
		    status = E_DB_OK;
		}
		else if (*err_code != 0)
		{
		    locstat = E_DB_ERROR;
		    SETDBERR(dberr, 0, E_DM9185_CANT_LIST_DIR);
		    break;
		}
		status = E_DB_OK;
	    }
	    else if (status == DI_DIRNOTFOUND)
	    {
		/* 
		** There user specified parent directory did not exist.
		*/
		    locstat = E_DB_ERROR;
		    SETDBERR(dberr, 0, E_DM9186_LISTDIR_NONEXISTENT);
		    break;

	    }
	    else
	    {
		/* An operating system error or CL error of some sort has been
		** encountered.
		*/
		locstat = E_DB_ERROR;
		SETDBERR(dberr, 0, E_DM9185_CANT_LIST_DIR);
		break;
	    }
	} /* end of DIlistfile processing */

	if (flag & DMM_DIRLIST)
	{
	    *err_code = E_DM9185_CANT_LIST_DIR;
	    status = DIlistdir(&io_cb, path, pathlen, dmm_putlist,
				(PTR)arglist, &sys_err_code);
	    if (status == DI_ENDFILE || status == OK)
	    {
		/* If an error occurred placing an entry in the user supplied
		** table, then arglist[5] (error) will contain an error code.
		** this error code should be logged and translated to a
		*/
		if (*err_code == E_DM0140_EMPTY_DIRECTORY)
		{
		    /* the directory to search was empty.  This is NOT
		    ** considered an error */
		    *err_code = 0;
		    status = E_DB_OK;
		}
		else if (*err_code != 0)
		{
		    locstat = E_DB_ERROR;
		    SETDBERR(dberr, 0, E_DM9185_CANT_LIST_DIR);
		    break;
		}
		status = E_DB_OK;
	    }
	    else if (status == DI_DIRNOTFOUND)
	    {
		/* 
		** There user specified parent directory did not exist.
		*/
		    locstat = E_DB_ERROR;
		    SETDBERR(dberr, 0, E_DM9186_LISTDIR_NONEXISTENT);
		    break;

	    }
	    else
	    {
		/* An operating system error or CL error of some sort has been
		** encountered.
		*/
		locstat = E_DB_ERROR;
		SETDBERR(dberr, 0, E_DM9185_CANT_LIST_DIR);
		break;
	    }
	}  /* end of DIlistdir processing */

	/* now we are done, so deallocate memory and close the table we listed 
	**  to.
	*/
	
	dm0m_deallocate( (DM_OBJECT **) &tuple);
	
	locstat = dm2t_close(rcb, DM2T_NOPURGE, dberr);
	    
	break;

   } /* end control loop */

   if (locstat != E_DB_OK)
   {
	row_cnt = 0;
	/* if we have an error code that is internal, log it.
	**  else, push this external error code out to caller.
	*/
	if ( dberr->err_code > E_DM_INTERNAL)
	{
	    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM9184_DMMLIST_FAILED);

	}

	/* close any open tables, deallocate any allocated memory. */
	if (r_idx)
	{
	    tempstat = dm2t_close(r_idx, DM2T_NOPURGE, &local_dberr);
	}
	if (rel)
	{
	    tempstat = dm2t_close(rel, DM2T_NOPURGE, &local_dberr);
	}
	if (rcb)
	{
	    tempstat = dm2t_close(rcb, DM2T_NOPURGE, &local_dberr);
	}

	if (tuple)
	{
	    dm0m_deallocate( (DM_OBJECT **) &tuple);
	}

   }
   *tuple_cnt = row_cnt;
   return (locstat);
}

/*{
** Name: DMM_PUTLIST	- Routine called by DIlistdir or DIlistfile to
**			  put file/dir name into a relation.
**
** Description:
**
**      DIlistdir and DIlistfile are routines that list all directories/files
**	subordinate to the search directory.  These routines call a user
**	supplied routine (DMM_PUTLIST in this case) once for each dir/file they
**	find.  This routine does something with that dir/file (in this case
**	we tuck it away in a relation) then returns to the DIlistdir or
**	DIlistfile routine.  The Call Parameters of this routine are not
**	flexible, because they are defined by the DIlistxxx interface.
**	Therefore, all (real) arguments to this routine must be passed in
**	as i4  *arg_list and typed to what they really are.
**
**	There is an important assumption.  The 1st column of the user supplied
**	table MUST be where the file/dir name goes.  If this column will not
**	hold the name, then an error will be generated, and processing of
**	the DIlistdir() or DIlistfile() will be terminated.
**	 
**
** Inputs:
**	arglist[0]			Pointer to RCB for table.
**	arglist[1]			Pointer to initialized ADF_CB
**	arglist[2]			Pointer to ADF's IN DB_DATA_VALUE
**	arglist[3]			Pointer to ADF's OUT DB_DATA_VALUE
**	arglist[4]			Ptr to counter (for # files/dir found)
**	arglist[5]			Ptr to err_cb for any errors that occur.
**	arglist[6]			Ptr to tuple buffer
**
** Outputs:
**      arglist[4]   found_count	number of files/dirs found (living ctr)
**	arglist[5]   err_cb		error code if error encountered
**	Returns:
**	    OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    User table is populated with tuples.
**
** History:
**      21-Mar-1989  (teg)
**	    Initial Creation.
**	02-Aug-1989 (teg)
**	    removed check for DIlistxxx error.
**	07-feb-94 (swm)
**	    Bug #59609
**	    Unfix page error occurs during createdb on Alpha AXP OSF.
**	    Changed type of arglist from i4  * to PTR * since it is used
**	    to pass an array of pointers. This avoids pointer truncation
**	    on platforms where sizeof(PTR) > sizeofof(i4), eg. ap_osf.
[@history_template@]...
*/

DB_STATUS
dmm_putlist(
PTR	    *arglist,	    /* array of real arguments */
char	    *name,	    /* name of directory or file */
i4	    name_len,	    /* number of bytes in name */
CL_ERR_DESC  *sys_err)	    /* CL error status */
{
    DMP_RCB		*rcb;
    i4		*err;
    ADF_CB              *adf_cb;
    DB_DATA_VALUE	*from_dv;
    DB_DATA_VALUE	*to_dv;
    DB_STATUS		locstat;
    i4		*counter;
    DMP_MISC		*tuple;
    DB_ERROR		local_dberr;

    for (;;)	    /* control loop */
    {

	/*
	**  translate arguments from a PTR array to what they really are.
	*/
	rcb = (DMP_RCB *) arglist[0];
	adf_cb = (ADF_CB *) arglist[1];
	from_dv = (DB_DATA_VALUE *) arglist[2];
	to_dv = (DB_DATA_VALUE *) arglist[3];
	counter = (i4 *) arglist[4];
    	err = (i4 *) arglist[5];
	tuple = (DMP_MISC *) arglist[6];
	
	/* 
	**	build ADF Control  and data blocks.  Then
	**	call ADF to convert name character string to format
	**	required by tuple 
	*/
	from_dv->db_data = (PTR) name;
	from_dv->db_length = (i4) name_len;
	from_dv->db_datatype = DB_CHA_TYPE;

	locstat = adc_cvinto( adf_cb, from_dv, to_dv);
	if (locstat != E_DB_OK)
	{
	    /* generate error message that name could not be cohersed into 
	    ** table, then return with nonzero status to DIlistxxx().
	    */
	    if ( adf_cb->adf_errcb.ad_emsglen)
	    {
		uleFormat(NULL, adf_cb->adf_errcb.ad_errcode, NULL, ULE_MESSAGE, NULL, 
			   adf_cb->adf_errcb.ad_errmsgp, 
			   adf_cb->adf_errcb.ad_emsglen, NULL, err, 0);
		*err = E_DM9182_CANT_MAKE_LIST_TUPLE;
	    }
	    else
	    if	(adf_cb->adf_errcb.ad_errcode)
	    {
		*err = adf_cb->adf_errcb.ad_errcode;
	    }
	    return (E_DB_ERROR);
	    
	}

	/* not that we have the tuple, put it into the
	** table.
	*/

	locstat = dm2r_put(rcb, DM2R_NODUPLICATES, tuple->misc_data, &local_dberr);
	if (locstat != E_DB_OK)
	{
	    *err = local_dberr.err_code;

	    /* generate error message that name could not be cohersed into table,
	    ** then return with nonzero status to DIlistxxx().
	    */
	    if ( local_dberr.err_code > E_DM_INTERNAL)
	    {
		uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err, 0);
		*err = E_DM9180_CANT_ADD_LIST_TUPLE;
	    }

	    return (E_DB_ERROR);
	    
	}

	break;
    }	/* end error control loop */
    
    *err = E_DB_OK;
    (*counter)++;
    return (E_DB_OK);
}
