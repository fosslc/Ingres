/*
** Copyright (c) 1989, 2008 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <pc.h>
#include    <lo.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <st.h>
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
#include    <dm0m.h>
#include    <dm2t.h>
#include    <dm2r.h>
#include    <dmm.h>

/**
** Name:  DMMDELETE.C - Delete a DI file that is NOT being used by the server.
**
** Description:
**      It is the responsibility of the user to determine that the file is
**	NOT in use by the server.  This routine assumes that it is not being
**      used and will do the DIdelete without taking any locks on the file.
**	However, it will assure the file is not a dbms catalog.
**
**  This file defines:
**    
**      dmm_delete()    -  Cause contents of a directory to be listed.
**	dmm_do_del()	-  verify file is not dbms catalog, DIdelete's it.
**
** History:
**      31-Mar-89 (teg)
**	    Initial Creation (for 6.2).
**	26-Oct-89 (neil)
**	    Add iievent to dmm_delete table (dmm_do_del) for Alerters.
**	23-feb-90 (carol)
**	    Add iidbms_comment and iisynonym to dmm_delete table.
**	21-may-90 (teg)
**	    aaaaaaaa.rcf -> aaaaaaaa.rfc
**	26-jun-90 (linda)
**	    Add the RMS Gateway extended system catalogs to the list of files
**	    which should never be deleted.  See comments in dmm_do_del().
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	29-may-92 (andre)
**	    added IIPRIV to dmm_delete table
**	10-jun-92 (andre)
**	    added IIXPRIV to dmm_delete table
**	18-jun-92 (andre)
**	    added IIXPROCEDURE and IIXEVENT to dmm_delete table
**	 7-jul-1992 (rogerk)
**	    Prototyping DMF.
**	28-jul-92 (rickh)
**	    Added new dbms_files:  iiintegrityidx, iiruleidx, iikey,
**	    iidefault, iidefaultidx.  For FIPS CONSTRAINTS.
**      02-nov-92 (anitap)
**          Added IISCHEMA and IISCHEMAIDX to dbms_files[].
**	07-jan-92 (rickh)
**	    Added IIPROCEDURE_PARAMETER.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system typedefs
**	15-jun-93 (rickh)
**	    Added IIXPROTECT index on IIPROTECT.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	23-may-1994 (andys)
**	    Add cast(s) to quiet qac/lint
**      03-jun-1996 (canor01)
**          For operating-system threads, remove external semaphore protection
**          for NMingpath().
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Added E_DM006A_TRAN_ACCESS_CONFLICT.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	29-Dec-2004 (schka24)
**	    Remove unused index iixprotect from catalog list.
**	05-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	10-Dec-2008 (jonj)
**	    SIR 120874: Remove last vestiges of CL_SYS_ERR,
**	    old form uleFormat.
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/


/*{
** Name: dmm_delete - Delete DI file specified by user.
**
**  INTERNAL DMF call format:    status = dmm_delete(&dmm_cb);
**
**  EXTERNAL call format:        status = dmf_call(DMM_DELETE, &dmm_cb);
**
** Description:
**
**  This routine deletes the specified file from the specified path.
**  It assumes that the file exists in that path.  The filename should
**  be one of the filenames returned to a user table from a prior dmm_list
**  call.
**
**  This routine is used for deleting DI files that are NOT in use (or needed)
**  by the server.  Since the server does not care about this file, there is
**  no reason to log the delete or to lock the file.  (Probabilities are that
**  this filename does not translate to a valid table id.)
**
**
**  Inputs:
**      dmm_cb 
**          .type                           Must be set to DMM_MAINT_CB.
**          .length                         Must be at least sizeof(DMM_CB).
**          .dmm_flags_mask                 should be zero
**          .dmm_tran_id                    Must be the current transaction 
**                                          identifier returned from the 
**                                          begin transaction operation.
**          .dmm_db_location                full pathname of dir containing file
**		.data_address			Pointer to Pathname
**		.data_in_size			number of characters in pathname
**          .dmm_file_name                  Name of attribute to put list into
**		.data_address			Pointer to filename
**		.data_in_size			number of characters in filename
**      
** Outputs:
**                                          of table created.
**      dmm_cb   
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK
**					    E_DM0010_BAD_DB_ID
**					    E_DM0064_USER_ABORT
**					    E_DM010C_TRAN_ABORTED
**					    E_DM001A_BAD_FLAG 
**					    E_DM003B_BAD_TRAN_ID
**					    E_DM0065_USER_INTR
**	    				    E_DM006A_TRAN_ACCESS_CONFLICT
**					    E_DM0131_DMMDELETE_ERROR
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
**	21-may-90 (teg)
**	    Fixed name of Rollforward config file: aaaaaaaa.rcf -> aaaaaaaa.rfc
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Added E_DM006A_TRAN_ACCESS_CONFLICT.
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
*/
DB_STATUS
dmm_delete(
DMM_CB        *dmm_cb)
{
    DMM_CB	    *dmm = dmm_cb;
    DML_XCB	    *xcb;
    DML_ODCB	    *odcb;
    i4         db_lockmode;
    i4	    local_error;
    i4	    status;
    char	    *path;
    char	    *fil;
    i4		    *err_code = &dmm->error.err_code;

    CLRDBERR(&dmm->error);


    status = E_DB_ERROR;
    for (;;)
    {
	if (dmm->dmm_flags_mask != 0)
	{
	    SETDBERR(&dmm->error, 0, E_DM001A_BAD_FLAG);
	    break;
	}

	/* verify path and file names are specified */
	path = dmm->dmm_db_location.data_address;
	fil  = dmm->dmm_filename.data_address;
	if ( (dmm->dmm_db_location.data_in_size ==0) ||
	     (dmm->dmm_filename.data_in_size ==0) ||
	     (*fil == '\0') || (*path == '\0'))
	{
	    SETDBERR(&dmm->error, 0, E_DM9181_SPECIFY_DEL_FILE);
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

	/* If it gets here, then input parameters were OK. */

	/* get a list of the directories/files subordinate to specified path. 
	** (Error code will be put into variable error).
	*/

	status = dmm_do_del(path, (u_i4) dmm->dmm_db_location.data_in_size,
		  fil, (u_i4) dmm->dmm_filename.data_in_size, &dmm->error);
	break;	    

    } /* end for */
   
    if (status != E_DB_OK)
    {
	if (dmm->error.err_code > E_DM_INTERNAL)
	{
	    uleFormat( &dmm->error, 0, NULL, ULE_LOG , NULL, 
		(char * )NULL, 0L, (i4 *)NULL, 
		&local_error, 0);
	    SETDBERR(&dmm->error, 0, E_DM0131_DMMDEL_ERROR);
	}
	switch (dmm->error.err_code)
	{
	    case E_DM004B_LOCK_QUOTA_EXCEEDED:
	    case E_DM0112_RESOURCE_QUOTA_EXCEED:
	    case E_DM0131_DMMDEL_ERROR:
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
** Name: dmm_do_del - Delete DI file specified by user.
**
** Description:
**
**  This routine attempts to find the delete filename in a lookup table
**  containing the names of dbms catalogs.  If a match is found, it refuses
**  to do the delete.  Otherwise, it attempts the DIdelete.
**
**
**  Inputs:
**	    path			    pathname to dir where file resides
**	    pathlen			    number of characters in path
**	    fil 			    name of file to delete
**	    pathlen			    number of characters in fil
**  Outputs:
**	    *err			    Reason for return status if error
**  RETURNS:
**          E_DB_OK                         Function completed normally. 
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dmm_cb.error.err_code.
**  History:
**	26-jun-90 (linda)
**	    Added RMS Gateway catalogs to the list not to be deleted.  For now,
**	    these are:  iigw02_relation, iigw02_attribute and iigw02_index.
**	28-jul-92 (rickh)
**	    Added new dbms_files:  iiintegrityidx, iiruleidx, iikey,
**	    iidefault, iidefaultidx.  For FIPS CONSTRAINTS.
**      02-nov-92 (anitap)
**          Added SCHEMA catalogs to the list not to be deleted. Also changed
**          the mapping of iievent from "aaaaaabo" to "aaaaaabp".
**	07-jan-92 (rickh)
**	    Added IIPROCEDURE_PARAMETER.
**	15-jun-93 (rickh)
**	    Added IIXPROTECT index on IIPROTECT.
*/
DB_STATUS
dmm_do_del(
char	    *path,
u_i4	    pathlen,
char	    *fil,
u_i4	    fillen,
DB_ERROR    *dberr)
{
    CL_ERR_DESC	    sys_err;
    DI_IO	    di_io;
    i4	    i;
    STATUS	    stat;

#define	 	IDENTICAL  	0
#define  	CORE_SIZE	12
#define  	CORE_CNT 	(sizeof(core_files) / sizeof (core_files[0]))
static const char *core_files[]=
{
	"aaaaaaaa.cnf",		/* config file */
	"aaaaaaaa.rfc",		/* recovery config file */
	"aaaaaaab.t00",		/* iirelation */
	"aaaaaaac.t00",		/* iirel_idx */
	"aaaaaaad.t00",		/* iiattribute */
	"aaaaaaae.t00",		/* iiindex */
	"aaaaaaai.t00"		/* iidevices */ 
};

#define	 	DBMS_SIZE 	10
#define  	DBMS_CNT 	(sizeof(dbms_files) / sizeof (dbms_files[0]))
static const char *dbms_files[]=
{
	/*iitree            */      "aaaaaabf.t",
	/*iiprotect         */      "aaaaaabg.t",
	/*iiintegrity       */      "aaaaaabh.t",
	/*iidbdepends       */      "aaaaaabi.t",
	/*iistatistics      */      "aaaaaabj.t",
	/*iihistogram       */      "aaaaaabk.t",
	/*iiprocedure       */      "aaaaaabl.t",
	/*iixdbdepends      */      "aaaaaabm.t",
	/*iirule	    */	    "aaaaaabn.t",
	/*iievent	    */	    "aaaaaabp.t",
	/*iidbms_comment    */	    "aaaaaaca.t",
	/*iisynonym	    */	    "aaaaaacb.t",
	/* iipriv	    */	    "aaaaaacc.t",
	/* iixpriv	    */	    "aaaaaacd.t",
	/* iixprocedure	    */	    "aaaaaace.t",
	/* iixevent	    */	    "aaaaaacf.t",
	/* notused	    */	    /* aaaaaach (39) was iixprotect */
	/*iilocations	    */	    "aaaaaaci.t",
	/*iidatabase        */      "aaaaaacj.t",
	/*iidbaccess        */      "aaaaaack.t",
	/*iiextend          */      "aaaaaacl.t",
	/*iiuser            */      "aaaaaacm.t",
	/*iiapplication_id  */      "aaaaaacn.t",
	/*iiusergroup       */      "aaaaaaco.t",
	/*iiintegrityidx    */	    "aaaaaadf.t",
	/*iiruleidx	    */	    "aaaaaadg.t",
	/*iikey		    */	    "aaaaaadh.t",
	/*iidefault	    */	    "aaaaaadi.t",
	/*iidefaultidx	    */	    "aaaaaadj.t",
	/*iischema          */      "aaaaaadk.t",
        /*iischemaidx       */      "aaaaaadl.t",
        /*iiprocedure_parameter */  "aaaaaadm.t",
	/*iigw02_relation   */      "aaaaaakk.t",
	/*iigw02_attribute  */      "aaaaaakl.t",
	/*iigw02_index	    */      "aaaaaakm.t"
};


    /* validate that filename not a config file or dbms catalog */
    
    CLRDBERR(dberr);

    if (fillen >= CORE_CNT);
    {
	for (i = 0; i < CORE_CNT; i++)
	{
	    if (MEcmp( (PTR) fil, (PTR) core_files[i], CORE_SIZE)
	    == IDENTICAL)
	    {
		SETDBERR(dberr, 0, E_DM918F_ILLEGAL_DEL_FIL);
		return (E_DB_ERROR);
	    }
	}	/* end loop to assure delete file is not a core catalog */
    }

    if (pathlen >= DBMS_CNT)
    {
	for (i = 0; i < DBMS_CNT; i++)
	{
	    if (MEcmp( (PTR) fil, (PTR) dbms_files[i], DBMS_SIZE)
	    == IDENTICAL)
	    {
		SETDBERR(dberr, 0, E_DM918F_ILLEGAL_DEL_FIL);
		return (E_DB_ERROR);
	    }
	}	/* end loop to assure delete file is not a dbms system catalog*/
    }

    stat = DIdelete( &di_io, path, pathlen, fil, fillen, &sys_err);
    if (stat != OK)
    {
	SETDBERR(dberr, 0, E_DM9190_BAD_FILE_DELETE);
    	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}

/*{
** Name: dmm_del_config - Delete config file from dump directory
**
** Description:
**      This routine deletes a config file.
**	It is called by UPGRADEDB (via an internal procedure) when converting 
**	a database to ensure no config files exist in the dump directory.
**
** Inputs:
**      dmm_cb 
**          .type                           Must be set to DMM_MAINT_CB.
**          .length                         Must be at least sizeof(DMM_CB).
**	    .dmm_flags_mask		    Must be zero.
**          .dmm_tran_id                    Must be the current transaction 
**                                          identifier returned from the 
**                                          begin transaction operation.
**          .dmu_location.data_address      Pointer to array of locations.
**                                          Each entry in array is of type
**                                          DB_LOC_NAME.  Must be zero if
**                                          no locations specified.
**	    .dmu_location.data_in_size      The size of the location array
**                                          in bytes.
**	    .dmm_db_name		    name of database
** Outputs:
**      dmm_cb
**          .error.err_code                 One of the following error numbers.
**						E_DM003B_BAD_TRAN_ID
**						E_DM0064_USER_ABORT
**						E_DM0065_USER_INTR
**						E_DM0072_NO_LOCATION
**						E_DM010C_TRAN_ABORTED
**						E_DM9190_BAD_FILE_DELETE
**	Returns:
**	    E_DB_OK			Completed successfully.
**	    E_DB_ERROR			failure, so check dmm_cb.error.err_code
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**	02-oct-92 (teresa)
**	    Initial creation
**	23-may-1994 (andys)
**	    Add cast(s) to quiet qac/lint
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
[@history_template@]...
*/
DB_STATUS
dmm_del_config(DMM_CB *dmm_cb)
{
    DI_IO		di_file;
    DB_STATUS		status;
    CL_ERR_DESC		sys_err;
    DMM_CB		*dmm = dmm_cb;
    DML_XCB		*xcb;
    DML_SCB		*scb;
    i4			*err_code = &dmm->error.err_code;
    char		location_buffer[MAX_LOC+1];
    char		dbname_str[DB_DB_MAXNAME+1];
    char		pathbuf[LO_PATH_MAX+1];
    char		*path = &pathbuf[0];
    i4		db_length;
    LOCATION		cl_loc;

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

    /* build location */
    MEcopy(dmm->dmm_db_location.data_address,
	dmm->dmm_db_location.data_in_size,
	location_buffer);
    location_buffer[dmm->dmm_db_location.data_in_size] = 0;
    MEcopy((char *)&dmm->dmm_db_name, sizeof(dmm->dmm_db_name), dbname_str);
    dbname_str[DB_DB_MAXNAME] = 0;
    STtrmwhite(dbname_str);
    db_length = STlength(dbname_str);
    LOingpath(location_buffer, dbname_str, LO_DMP, &cl_loc);
    LOtos(&cl_loc, &path);
    status = DIdelete(&di_file, path, STlength(path),
		      (PTR) "aaaaaaaa.cnf", STlength("aaaaaaaa.cnf"),
		      &sys_err);

    if ((status == DI_DIRNOTFOUND) || (status == DI_FILENOTFOUND))
    {
	/* the file did not exist.  This is not an error */
	status = E_DB_OK;
    }
    else if (status != OK)
    {
	SETDBERR(&dmm->error, 0, E_DM9190_BAD_FILE_DELETE);
    	status = E_DB_ERROR;
    }
    else
    {
	status = E_DB_OK;
    }

    return (status);
}
