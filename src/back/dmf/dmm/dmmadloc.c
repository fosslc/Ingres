/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <pc.h>
#include    <ck.h>
#include    <cs.h>
#include    <cv.h>
#include    <di.h>
#include    <jf.h>
#include    <lo.h>
#include    <me.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <lk.h>
#include    <lg.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dmccb.h>
#include    <dmmcb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm2d.h>
#include    <dm0c.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dmm.h>

/**
**
** Name: DMMADLOC.C - Functions needed to add location to database config file.
**
** Description:
**      This file contains the functions necessary to add locations to
**	the database configuration file. This function is used by the INGCNTRL
**	utility to extend a database to allow files to be stored on new
**	location.
**    
**      dmm_add_location()       -  Routine to perfrom the add operation.
**
** History:
**      20-mar-89 (derek) 
**          Created for Orange.
**	23-Oct-1991 (rmuth)
**	    Added some extra error handling, changed existing to work in
**	    routine dmm_add_location.
**	 7-jul-1992 (rogerk)
**	    Prototyping DMF.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	05-nov-92 (jrb)
**	    Changes for multi-location sorts.
**	22-oct-1992 (bryanp)
**	    Improve error logging.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	14-dec-92 (jrb)
**	    Removed a bunch of CONVTO60 support and made change to allow WORK
**	    directory to already exist at extend (or CREATEDB) time.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	20-apr-94 (mikem)
**          bug #62347
**          In order to provide better error logging diagnostics added new 
**	    error E_DM91A8_DMM_LOCV_DILISTDIR, which will also log the name
**          of the directory that the failure occurred on.  Also fixed some
**	    lint recognized unused variables.
**	23-may-1994 (andys)
**	    Add casts to quiet qac/lint
**	03-jun-1996 (canor01)
**	    For operating-system threads, remove external semaphore protection 
**	    for NMingpath().
**	15-oct-96 (mcgem01)
**	    E_DM91A8 now takes SystemAdminUser as a parameter.
**	15-may-97 (mcgem01)
**	    Fix compiler error by changing the order of includes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	12-mar-1999 (nanpr01)
**	    Integrated Support for raw locations.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	25-Mar-2001 (jenjo02)
**	    Productized RAW locations.
**	02-Apr-2001 (jenjo02)
**	    Added raw_start to dm2d_extend_db() prototype.
**	11-May-2001 (jenjo02)
**	    Added raw_pct to dm2d_extend_db() prototype.
**	23-May-2001 (jenjo02)
**	    If DMM_CHECK_LOC_AREA flag, this is a call from
**	    iiQEF_check_loc_area database procedure; all
**	    we need to do is check for the existence of
**	    the location's path and skip the extension
**	    process.
**	15-Oct-2001 (jenjo02)
**	    Add operations to CHECK/MAKE/EXTEND Area
**	    locations.
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	05-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	18-Nov-2008 (jonj)
**	    dm2d_? functions converted to DB_ERROR *
**      17-dec-2009 (joea)
**          Expand various buffers in dmm_add_location and dmm_loc_validate
**          to MAX_LOC+1, as suggested by the CL LO Spec.  In dmm_loc_validate,
**          return error if path, including dbname, is > DB_AREA_MAX.
**/

/*
** Forward Declarations
*/
static STATUS	list_func(VOID);


/*{
** Name: dmm_add_location - Verifies/makes Area path and/or adds a 
**			    location to a database.
**
**  INTERNAL DMF call format:      status = dmm_add_loc(&dmm_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMM_ADD_LOCATION, &dmm_cb);
**
** Description:
**	Depending on one or more flags in dmm_flags_mask:
**
**	    DMM_CHECK_LOC_AREA	- Simply verify that the location's
**				  Area path, minus dbname, exists
**	    DMM_MAKE_LOC_AREA   - Make any missing directories in
**				  the Area's path, minus dbname. Note
**				  that Raw Area paths must be made
**				  using MKRAWAREA and will not be 
**				  made here.
**	    DMM_EXTEND_LOC_AREA	- Extend a database to a location. By
**				  now, the Area path must exist. Add 
**				  the location to the database config
**				  file and create the database-specific
**				  directory.
**
** Inputs:
**      dmm_cb 
**          .type                           Must be set to DMM_MAINT_CB.
**          .length                         Must be at least sizeof(DMM_CB).
**          .dmm_tran_id                    Must be the current transaction 
**                                          identifier returned from the 
**                                          begin transaction operation.
**          .dmm_location.data_address      Pointer to array of locations.
**                                          Each entry in array is of type
**                                          DB_LOC_NAME.  Must be zero if
**                                          no locations specified.
**	    .dmm_location.data_in_size      The size of the location array
**                                          in bytes.
**	    dmm_loc_type		    Type of location:
**						DMM_L_JNL
**						DMM_L_CKP
**						DMM_L_DMP
**						DMM_L_WRK
**						DMM_L_AWRK
**						DMM_L_DATA
**	    dmm_flags_mask		    What to do:
**						DMM_CHECK_LOC_AREA
**						DMM_MAKE_LOC_AREA
**						DMM_EXTEND_LOC_AREA
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
**      25-sep-1991 (mikem) integrated following change: 10-Oct-90 (pholman)
**          Replace DB (obsolete) by LO_DB.
**	05-nov-92 (jrb)
**	    Changed for multi-location sorts.
**	23-may-1994 (andys)
**	    Add casts to quiet qac/lint
**	12-mar-1999 (nanpr01)
**	    Integrated Support for raw locations.
**	23-May-2001 (jenjo02)
**	    If DMM_CHECK_LOC_AREA flag, this is a call from
**	    iiQEF_check_loc_area database procedure; all
**	    we need to do is check for the existence of
**	    the location's path and skip the extension
**	    process.
**	15-Oct-2001 (jenjo02)
**	    Added DMM_MAKE_LOC_AREA flag to make missing Area
**	    directories. Pass dmm_flags_mask to dmm_loc_validate().
**	    DMM_EXTEND_LOC_AREA flag must now be passed to
**	    induce the extend-to-database operation.
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
**	09-Mar-2009 (drivi01)
**	    Initialize l_db_dir, l_area, loc_flags and physical_length
**	    variables to avoid bogus values.
*/

static STATUS
list_func(VOID)
{
    return (FAIL);
}


DB_STATUS
dmm_add_location(
DMM_CB    *dmm_cb)
{
    DMM_CB	    *dmm = dmm_cb;
    DML_XCB	    *xcb;
    DML_SCB	    *scb;
    STATUS	    status;
    i4		    *err_code = &dmm->error.err_code;
    i4	    	    l_db_dir = 0;
    i4	    	    l_area = 0;
    i4	    	    loc_flags = 0;
    char	    *physical_name;
    i4	    	    physical_length = 0;
    char	    db_buffer[sizeof(DB_DB_NAME) + sizeof(int)];
    char	    db_dir[MAX_LOC+1];
    char	    area[MAX_LOC+1];
    char	    location_buffer[MAX_LOC+1];
    LOCATION	    cl_loc;
    DMM_LOC_LIST    **loc_list;

    CLRDBERR(&dmm->error);

    /*	Check arguments. */

    for (status = E_DB_ERROR;;)
    {
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

	if ( dmm->error.err_code )
	    break;

	/*  Check Database location. */

	if (dmm->dmm_db_location.data_in_size == 0 ||
	    dmm->dmm_db_location.data_address == 0 ||
	    (dmm->dmm_loc_list.ptr_in_count &&
		(dmm->dmm_loc_list.ptr_size != sizeof(DMM_LOC_LIST) ||
		 dmm->dmm_loc_list.ptr_address == 0 ||
		 dmm->dmm_loc_list.ptr_in_count != 1)))
	{
	    SETDBERR(&dmm->error, 0, E_DM0072_NO_LOCATION);
	    status = E_DB_ERROR;
	    break;
	}

	/*  Check location to be added. */

	loc_list = (DMM_LOC_LIST **)dmm->dmm_loc_list.ptr_address;
	loc_flags = dmm->dmm_flags_mask;

	/* Raw areas can only be checked, not made, use MKRAWAREA */
	if ( loc_list[0]->loc_raw_pct )
	{
	    loc_flags &= ~DMM_MAKE_LOC_AREA;
	    loc_flags |=  DMM_CHECK_LOC_AREA;

	    /* Only valid usage for Raw is DATABASE */
	    if ( loc_list[0]->loc_type != DMM_L_DATA )
	    {
		SETDBERR(&dmm->error, 0, E_DM0197_INVALID_RAW_USAGE);
		status = E_DB_ERROR;
		break;
	    }
	}

	if ( status = dmm_loc_validate(loc_list[0]->loc_type, 
		    &dmm->dmm_db_name, 
		    loc_list[0]->loc_l_area, loc_list[0]->loc_area,
		    area, &l_area, db_dir, &l_db_dir, &loc_flags, &dmm->error) )
	{
	    /*
	    ** If not extending to a raw area (just checking existence),
	    ** return a warning that MKRAWAREA must be run to create
	    ** the area, we can't do it here.
	    */
	    if ( dmm->error.err_code == E_DM0080_BAD_LOCATION_AREA &&
		 loc_list[0]->loc_raw_pct &&
		 (dmm->dmm_flags_mask & DMM_EXTEND_LOC_AREA) == 0 )
	    {
		SETDBERR(&dmm->error, 0, E_DM0198_RUN_MKRAWAREA);
		status = E_DB_WARN;
	    }
	    break;
	}

	/* The location's area exists or was created */

	/*
	** Cross-check RAW usage.
	**
	** Note that dmm_loc_validate() set DCB_RAW in our loc_flags
	** if this location mapped to a raw area.
	** Notify caller if this occurs.
	*/

	/* Raw Area? */
	if ( loc_flags & DCB_RAW )
	{
	    loc_list[0]->loc_type |= DMM_L_RAW;

	    /* Error if raw_pct not specified */
	    if ( loc_list[0]->loc_raw_pct == 0 )
	    {
		SETDBERR(&dmm->error, 0, E_DM0195_AREA_IS_RAW);
		status = E_DB_ERROR;
		break;
	    }
	}
	/* Cooked area */
	else if ( loc_list[0]->loc_raw_pct )
	{
	    /* Error if raw_pct is specified */
	    SETDBERR(&dmm->error, 0, E_DM0196_AREA_NOT_RAW);
	    status = E_DB_ERROR;
	    break;
	}

	/* If not extending location to a database, we're done */
	if ( dmm->dmm_flags_mask & DMM_EXTEND_LOC_AREA )
	{
	    /*
	    ** Make path out of root DB location information,
	    ** i.e., where to look for CONFIG file.
	    */

	    MEcopy(dmm->dmm_db_location.data_address,
		dmm->dmm_db_location.data_in_size,
		location_buffer);
	    location_buffer[dmm->dmm_db_location.data_in_size] = 0;
	    MEcopy((char *)&dmm->dmm_db_name, sizeof(dmm->dmm_db_name), db_buffer);
	    db_buffer[sizeof(dmm->dmm_db_name)] = 0;
	    STtrmwhite(db_buffer);

	    if ( LOingpath(location_buffer, db_buffer, LO_DB, &cl_loc) )
	    {
		SETDBERR(&dmm->error, 0, E_DM0080_BAD_LOCATION_AREA);
		status = E_DB_ERROR;
	    }
	    else
	    {
		LOtos(&cl_loc, &physical_name);

		physical_length = STlength(physical_name);

		status = dm2d_extend_db(scb->scb_lock_list, &dmm->dmm_db_name,
		    physical_name, physical_length, &loc_list[0]->loc_name,
		    area, l_area, db_dir, l_db_dir, loc_flags, 
		    loc_list[0]->loc_raw_pct,
		    &loc_list[0]->loc_raw_start, 
		    &loc_list[0]->loc_raw_blocks, 
		    &dmm->error);
	    }
	}
	break;
    }
    return (status);    
}

/*
** Name: dmm_loc_validate	- does something with locations.
**
** Description:
**	This routine does stuff with locations -- I guess it checks them out.
**
** History:
**	22-oct-1992 (bryanp)
**	    Improve error logging.
**	20-apr-94 (mikem)
**          bug #62347
**          In order to provide better error logging diagnostics added new 
**	    error E_DM91A8_DMM_LOCV_DILISTDIR, which will also log the name
**          of the directory that the failure occurred on.
**	18-jan-95 (cohmi01)
**	    For raw locations, if loc_type is negative, its raw. Turn
**	    on DCB_RAW. Dont tack on database name to raw file name.
**	12-mar-1999 (nanpr01)
**	    Integrated Support for raw locations.
**	12-Mar-2001 (jenjo02)
**	    Productized for RAW locations.
**	23-May-2001 (jenjo02)
**	    If dbname is omitted, use loc_dir as db_dir, just check
**	    for loc_dir existence. Accept "type" of zero and handle it.
**	15-Oct-2001 (jenjo02)
**	    Instead of relying on missing dbname, check input flags 
**	    for DMM_CHECK_LOC_AREA. If area does not exist and
**	    DMM_MAKE_LOC_AREA flag is set, call LOmkingpath()
**	    to make the missing directories.
**	17-Oct-2001 (jenjo02)
**	    Changed RAW notification from LOingpath() from non-portable
**	    check of filename to portable check of return code.
**	07-Mar-2002 (jenjo02)
**	    Removed test for writable directory. Readonly databases
**	    won't pass this test and there's no way during create
**	    location to know if the corresponding area is going
**	    to be used read-only or not. Also ignore outcome
**	    of LOmkingpath(); if read-only, the path can't be
**	    made and we'll have to let run-time catch any
**	    permissions problems.
**  	26-Mar-2009 (troal01)
**	    Added a string length check to ensure that stack overflows
**	    can't occur in this function. 118840
*/
DB_STATUS
dmm_loc_validate(
i4		type,
DB_DB_NAME	*dbname,
i4		l_area,
char		*area,
char		*area_dir,
i4		*l_area_dir,
char		*path,
i4		*l_path,
i4		*flags,
DB_ERROR	*dberr)
{
    char	    db_buffer[sizeof(DB_DB_NAME) + sizeof(int)];
    char	    loc_buffer[MAX_LOC+1];
    char	    loc_dir[MAX_LOC+1];
    char	    db_dir[MAX_LOC+1];
    int		    loc_flags;
    int		    i;
    int		    l_db_dir;
    int		    l_loc_dir;
    char	    *loc_type;
    char	    *ptr;
    STATUS	    status;
    LOCATION	    cl_loc;
    LOINFORMATION   lo_info;
    i4		    lo_flags;
    CL_ERR_DESC	    sys_err;
    DI_IO	    di_context;
    JFIO	    jf_io;
    char	    *raw_check = NULL;
    i4		    *err_code = &dberr->err_code;

    *err_code = E_DB_OK;

    for (;;)
    {
	/*  Check location type . */

	switch ( type )
	{
	case DMM_L_JNL:
	    loc_type = LO_JNL, loc_flags = DCB_JOURNAL;
	    break;
	case DMM_L_CKP:
	    loc_type = LO_CKP, loc_flags = DCB_CHECKPOINT;
	    break;
	case DMM_L_DMP:
	    loc_type = LO_DMP, loc_flags = DCB_DUMP;
	    break;
	case DMM_L_WRK:
	    loc_type = LO_WORK, loc_flags = DCB_WORK;
	    break;
	case DMM_L_AWRK:
	    loc_type = LO_WORK, loc_flags = DCB_AWORK;
	    break;
	case DMM_L_DATA:
	    loc_type = LO_DB, loc_flags = DCB_DATA;
	    /* RAW only allowed for USAGE=DATABASE */
	    raw_check = LO_RAW;
	    break;
	default:
	    uleFormat(NULL, E_DM919E_DMM_LOCV_BADPARAM, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, err_code, 1,
		    0, type);
	    SETDBERR(dberr, 0, E_DM002A_BAD_PARAMETER);
	    return(E_DB_ERROR);
	}

	/*	Construct path name for location and pathname for database. */

	MEmove(l_area, area, 0, sizeof(loc_buffer), loc_buffer);

	/* Get path for location, perform RAW check iff LO_DB */
	if ( (status = LOingpath(loc_buffer, raw_check, loc_type, &cl_loc)) )
	{
	    /* Does location map to a raw area? */
	    if ( status == LO_IS_RAW )
		loc_flags |= DCB_RAW;
	    else
	    {
		/* Quit now; something is wrong with path */
		SETDBERR(dberr, 0, E_DM0080_BAD_LOCATION_AREA);
		break;
	    }
	}

	/* check to see if location path is too long */
	if(STlength(cl_loc.string) > DB_AREA_MAX)
	{
                SETDBERR(dberr, 0, E_DM0080_BAD_LOCATION_AREA);
		break;
	}

	LOtos(&cl_loc, &ptr);
	STcopy(ptr, loc_dir);
	l_loc_dir = STlength(ptr);

	/* If checking location existance, or RAW... */
	if ( *flags & (DMM_CHECK_LOC_AREA | DMM_MAKE_LOC_AREA)
	     || loc_flags & DCB_RAW )
	{
	    /* Exclude dbname from path */
	    STcopy(loc_dir, db_dir);
	    l_db_dir = l_loc_dir;

	    /* If RAW, LOingpath has already verified existence */
	    if ( (loc_flags & DCB_RAW) == 0 )
	    {
		/* Ensure location is extant directory */
		lo_flags = LO_I_TYPE;

		if ( (status = LOinfo(&cl_loc, &lo_flags, &lo_info))
		     || (lo_flags & LO_I_TYPE) == 0
		     || lo_info.li_type != LO_IS_DIR )
		{
		    /* Make the path if instructed */
		    if ( status ) 
		    {
			if ( *flags & DMM_MAKE_LOC_AREA )
			    LOmkingpath(loc_buffer, loc_type, &cl_loc);
			else
			    SETDBERR(dberr, 0, E_DM0080_BAD_LOCATION_AREA);
		    }
		    else
		    {
			/* Not directory */
			SETDBERR(dberr, 0, E_DM011A_ERROR_DB_DIR);
		    }
		}
	    }
	}

	/* Extending DB to Cooked Location? */
	if ( *flags & DMM_EXTEND_LOC_AREA && dberr->err_code == 0 &&
	     (loc_flags & DCB_RAW) == 0 )
	{
	    /*	Get path for database */
	    for ( i = 0;
		  i < sizeof(DB_DB_NAME) &&
		    (db_buffer[i] = dbname->db_db_name[i]) != ' ';
		  i++ );
	    db_buffer[i] = EOS;

	    LOingpath(loc_buffer, db_buffer, loc_type, &cl_loc);
	    LOtos(&cl_loc, &ptr);
	    STcopy(ptr, db_dir);
	    l_db_dir = STlength(ptr);
            if (l_db_dir > DB_AREA_MAX)
            {
                SETDBERR(dberr, 0, E_DM0080_BAD_LOCATION_AREA);
                break;
            }

	    switch ( type )
	    {
	    case DMM_L_CKP:
	    case DMM_L_DMP:
		/*  Check that location exists and directory doesn't exist. */

		status = CKlistfile(db_dir, l_db_dir, list_func, 0, &sys_err);
		if (status == CK_ENDFILE)
		{
		    if (type == DMM_L_CKP)
			uleFormat(NULL, E_DM919F_DMM_LOCV_CKPEXISTS, (CL_ERR_DESC *)NULL,
				    ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		    else
			uleFormat(NULL, E_DM91A0_DMM_LOCV_DMPEXISTS, (CL_ERR_DESC *)NULL,
				    ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		    SETDBERR(dberr, 0, E_DM0124_DB_EXISTS);
		}
		else if (status != CK_DIRNOTFOUND)
		{
		    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err,
				    ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		    uleFormat(NULL, E_DM91A3_DMM_LOCV_CKLISTFILE, (CL_ERR_DESC *)NULL,
				    ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		    SETDBERR(dberr, 0, E_DM011A_ERROR_DB_DIR);
		}
		break;

	    case DMM_L_DATA:
	    case DMM_L_WRK:
	    case DMM_L_AWRK:
		/*  Check that location exists; one might think we should also
		**	check to ensure that the directory does NOT already exist, but
		**	instead we allow it to exist.  This is because there are
		**	legitimate circumstances where the directory already exists
		**	such as when an extend operation creates the directory but is
		**	then aborted (and the directory creation is not rolled back, of
		**	course); also, we allow two separate locations to point to the
		**	same area and extending a db to both of these locations means
		**	that the db directory would already exist when attempting the
		**	second extend.
		*/
		status = DIlistdir(&di_context, loc_dir, l_loc_dir,
		    list_func, 0, &sys_err);

		if (status != DI_ENDFILE)
		{
		    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err,
				    ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		    uleFormat(NULL, E_DM91A8_DMM_LOCV_DILISTDIR, (CL_ERR_DESC *)NULL,
				    ULE_LOG, NULL, NULL, 0, NULL, err_code, 2,
				    l_loc_dir, loc_dir, 
				    STlength (SystemAdminUser), SystemAdminUser);
		    SETDBERR(dberr, 0, E_DM0080_BAD_LOCATION_AREA);
		}
		break;

	    case DMM_L_JNL:
		/*  Check that location exists and directory doesn't exist. */

		status = JFlistfile(&jf_io, db_dir, l_db_dir,
						    list_func, 0, &sys_err);
		if (status == JF_END_FILE)
		{
		    uleFormat(NULL, E_DM91A2_DMM_LOCV_JNLEXISTS, (CL_ERR_DESC *)NULL,
				    ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		    SETDBERR(dberr, 0, E_DM0124_DB_EXISTS);
		}
		else if (status != JF_DIR_NOT_FOUND)
		{
		    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err,
				    ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		    uleFormat(NULL, E_DM91A6_DMM_LOCV_JFLISTFILE, (CL_ERR_DESC *)NULL,
				    ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		    SETDBERR(dberr, 0, E_DM011A_ERROR_DB_DIR);
		}
		break;
	    }
	}
	break;
    }
    if ( dberr->err_code )
	return (E_DB_ERROR);

    if (l_area_dir)
    {
	*l_area_dir = l_loc_dir;
	MEcopy(loc_dir, l_loc_dir, area_dir);
    }

    *flags = loc_flags;
    *l_path = l_db_dir;
    MEcopy(db_dir, l_db_dir, path);

    return (E_DB_OK);
}
