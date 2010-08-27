/*
** Copyright (c) 1985, 2008 Ingres Corporation
NO_OPTIM=dr6_us5 i64_aix
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <nm.h>
#include    <pc.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <lg.h>
#include    <lk.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <scf.h>
#include    <dudbms.h>
#include    <dmccb.h>
#include    <dmmcb.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmm.h>
#include    <dm2d.h>

/**
** Name: DMCADD.C - Functions used to add a database to a server.
**
** Description:
**      This file contains the functions necessary to add a database
**      to a server.  This file defines:
**    
**      dmc_add_db() -  Routine to perform the add 
**                      database operation.
**
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.      
**	 6-feb-1989 (rogerk)
**	    Added solecache option.  Took out restriction that fast commit
**	    be used with sole-server.
**	20-aug-1989 (ralph)
**	    Set DM0C_CVCFG for config file conversion if DMC_CVCFG is on
**	20-nov-1990 (bryanp)
**	    Check return code from dmm_add_create().  Root location for the
**	    iidbdb should be 'ii_database', not 'iidatabase'.
**	8-jul-1992 (walt)
**	    Prototyping DMF.
**	04-nov-92 (jrb)
**	    Changed "II_SORT" to "II_WORK" for multi-location sorts project.
**	14-dec-92 (jrb)
**	    Remove "misc_flag" from call to dmm_add_create.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	27-jul-93 (ralph)
**	    DELIM_IDENT:
**	    Pass dbservice value into dmm_add_create() when creating iidbdb.
**	14-sep-93 (jrb)
**	    Fixed problem where DU_OPERATIVE bit was never set in iidbdb's
**	    config file because createdb never ran the iiqef_alter_db internal
**	    procedure on the iidbdb (there is some problem preventing it from
**          being run).  So we now set the DU_OPERATIVE bit in the iidbdb's
**	    config file from the start.
**	01-oct-93 (jrb)
**	    Added DMC_ADMIN_DB as a valid flag to dmc_add_db.
**	31-jan-1994 (bryanp) B57822
**	    Log scf_error.err_code if scf_call fails.
** 	08-Apr-1997 (wonst02)
** 	    Use access mode passed back from dm2d_add_db().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-dec-2000 (somsa01)
**	    Added NO_OPTIM for ibm_lnx. Unfortunately, I do not remember
**	    why this was done in the first place. So, I will submit this
**	    change for now and try to take it out later.
**	12-dec-2000 (somsa01)
**	    Removed NO_OPTIM for ibm_lnx.
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	18-Nov-2008 (jonj)
**	    dm2d_? functions converted to DB_ERROR *
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/

/*{
** Name: dmc_add_db - Adds a database to a server.
**
**  INTERNAL DMF call format:    status = dmc_add_db(&dmc_cb);
**
**  EXTERNAL call format:        status = dmf_call(DMC_ADD_DB,&dmc_cb);
**
** Description:
**    The dmc_add_db function handles the addition of a database to a DMF
**    server.  The dmc_add function can also create a core database by
**    passing a access mode of DMC_A_CREATE.
**
**    A database cannot be opened by a user session until it has
**    been added to the server.  This command causes DMF to determine if the DB
**    exists and will establish a default working environment for this 
**    database which includes the access mode and journalling status.
**    The defaults supplied at database creation are used if this information
**    is not passed with this command.  An internal DMF identifier is returned
**    to the caller and must be used on all subsequent calls altering the
**    state of this database, such as open, close, delete, etc.
**
** Inputs:
**      dmc_cb 
**          .type                           Must be set to DMC_CONTROL_CB.
**          .length                         Must be at least sizeof(DMC_CB).
**          .dmc_op_type                    Must be DMC_DATABASE_OP.
**          .dmc_session_id		    Must be the SCF session id.
**          .dmc_flags_mask                 Must be zero, DMC_JOURNAL, or
**                                          DMC_NOJOURNAL.
**          .dmc_id                         Server identifier.
**          .dmc_db_name                    Must be a valid database name.
**          .dmc_db_location_array.data_address
**                                          Pointer to an area used to pass
**                                          the list of locations for this
**                                          database. Of type DMC_LOC_ENTRY.
**          .dmc_db_location_array.data_insize
**                                          Size of area containing location
**                                          names.
**          .dmc_db_access_mode             Must be one of the following:
**                                          DMC_A_READ, DMC_A_WRITE.
**          .dmc_owner                      Must be a valid database owner.
**
**          <dmc_db_location_array> entries  of type DMC_LOC_ENTRY
**              .loc_flags		    LOC_ROOT, LOC_LOG, LOC_JOURNAL,
**					    LOC_DATA.
**              .loc_location               Location name.
**		.loc_l_extent		    Length of loc_extent.
**              .loc_extent                 Fully qualified host file system
**                                          path.
**          .dmc_s_type                     Must be DMC_S_SINGLE for
**                                          a database to be served by 
**                                          a single server or DMC_S_MULTIPLE
**                                          for a database to be served by
**                                          multiple servers.
**
** Output:
**      dmc_cb 
**          .dmc_db_id                      Internal database identifier.
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM000F_BAD_DB_ACCESS_MODE
**                                          E_DM0011_BAD_DB_NAME
**                                          E_DM001A_BAD_FLAG
**                                          E_DM001D_BAD_LOCATION_NAME
**                                          E_DM002D_BAD_SERVER_ID
**                                          E_DM0041_DB_QUOTA_EXCEEDED
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**                                          E_DM0053_NONEXISTENT_DB
**                                          E_DM0065_USER_INTR
**                                          E_DM0064_USER_ABORT
**					    E_DM0084_ERROR_ADDING_DB
**                                          E_DM0112_RESOURCE_QUOTA_EXCEEDED
**                     
**      Returns:
**          E_DB_OK                         Function completed normally. 
**          E_DB_WARN                       Function completed normally with 
**                                          a termination status which is in 
**                                          dmc_cb.error.err_code.
**          E_DB_ERROR                      Function completed abnormally with 
**                                          a termination status which is in
**                                          dmc_cb.error.err_code.
**          E_DB_FATAL                      Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmc_cb.error.err_code.
** History:
**      01-sep-1985 (jennifer)
**          Created for Jupiter.
**	18-nov-1985 (derek)
**	    Completed the code for dmc_add_db().
**	 6-feb-1989 (rogerk)
**	    Added solecache option.  Took out restriction that fast commit
**	    be used with sole-server.
**	04-apr-1989 (Derek)
**	    Added create core database support.
**	20-aug-1989 (ralph)
**	    Set DM0C_CVCFG for config file conversion if DMC_CVCFG is set
**	20-nov-1990 (bryanp)
**	    Check return code from dmm_add_create().
**	02-mar-91 (teresa)
**	    add new parameter to dmm_add_create.
**      8-nov-1992 (ed)
**          DB_MAXNAME increase to 32
**	04-nov-92 (jrb)
**	    Changed "II_SORT" to "II_WORK" for multi-location sorts project.
**	14-dec-92 (jrb)
**	    Remove "misc_flag" from call to dmm_add_create.
**	02-jun-93 (ralph)
**	    DELIM_IDENT:
**	    Pass dbservice value into dmm_add_create() when creating iidbdb.
**	14-sep-93 (jrb)
**	    Fixed problem where DU_OPERATIVE bit was never set in iidbdb's
**	    config file because createdb never ran the iiqef_alter_db internal
**	    procedure on the iidbdb (there is some problem preventing it from
**          being run).  So we now set the DU_OPERATIVE bit in the iidbdb's
**	    config file from the start.
**	01-oct-93 (jrb)
**	    Added DMC_ADMIN_DB as a valid flag.
**	25-oct-93 (vijay)
**	    Remove incorrect type cast.
**	06-oct-93 (swm)
**	    Bug #56437
**	    Removed incorrect i4 cast of dmc->dmc_id.
**	31-jan-1994 (bryanp) B57822
**	    Log scf_error.err_code if scf_call fails.
**	06-may-1996 (nanpr01)
**	    Get rid of the compiler warning message by propely casting the
**	    pointers.
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          Introduced support for the Distributed Multi-Cache Management
**          (DMCM) protocol.
** 	08-Apr-1997 (wonst02)
** 	    Use access mode passed back from dm2d_add_db().
**	21-aug-1997 (nanpr01)
**	    Pass back the version to the caller for FIPS upgrade.
**      27-May-1999 (hanal04)
**          If the caller has already locked the CNF file pass this
**          flag into dm2d_add_db() along with the lock list id. Ensure 
**          DMC_CNF_LOCKED is recognised as a valid flag. b97083.
**	16-oct-1999 (nanpr01)
**	    Fixed compiler warning.
**	08-Jan-2001 (jenjo02)
**	    Removed unneeded CSget_sid(); dmc_session_id has SID.
**	26-mar-2001 (stephenb)
**	    Add support for Unicode collation
**	12-Nov-2009 (kschendel) SIR 122882
**	    Change cmptlvl to an integer.
**	09-aug-2010 (maspa05) b123189, b123960
**	    Pass flag for readonlydb through to dm2d_add_db
*/

static const DMM_LOC_LIST    loc_list[4] =
{
    {  DMM_L_JNL,
	{ 'i', 'i', '_', 'j', 'o', 'u', 'r', 'n',
	  'a', 'l', ' ', ' ', ' ', ' ', ' ', ' ',
	  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' },
	10, "II_JOURNAL"
    },
    {  DMM_L_CKP,
	{ 'i', 'i', '_', 'c', 'h', 'e', 'c', 'k',
	  'p', 'o', 'i', 'n', 't', ' ', ' ', ' ',
	  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' },
	13, "II_CHECKPOINT"		    },
    {  DMM_L_DMP,
	{ 'i', 'i', '_', 'd', 'u', 'm', 'p', ' ',
	  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' },
	7, "II_DUMP"
    },
    {  DMM_L_WRK,
	{ 'i', 'i', '_', 'w', 'o', 'r', 'k', ' ',
	  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' },
	7, "II_WORK"
    }
};
static const DB_LOC_NAME	    dbdb_location = 
{
    'i', 'i', '_', 'd', 'a', 't', 'a', 'b', 'a', 's', 'e', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '
};


DB_STATUS
dmc_add_db(
DMC_CB    *dmc_cb)
{
    DMC_CB		*dmc = dmc_cb;
    DM_SVCB		*svcb = dmf_svcb;
    DMC_LOC_ENTRY	*location;
    i4		loc_count;
    i4		flags;
    i4		mode;
    i4		dm2mode;
    i4		error,local_error;
    DMM_LOC_LIST	*loc_ptr[4];
    DB_STATUS		status;
    DMP_DCB		*dcb;

    CLRDBERR(&dmc->error);

    for (status = E_DB_ERROR;;)
    {
	/*	Verify control block parameters. */

	if (dmc->dmc_op_type != DMC_DATABASE_OP)
	{
	    SETDBERR(&dmc->error, 0, E_DM000C_BAD_CB_TYPE);
	    break;
	}

	if (dmc->dmc_id != svcb->svcb_id)
	{
	    SETDBERR(&dmc->error, 0, E_DM002D_BAD_SERVER_ID);
	    break;
	}

	flags = 0;
	if (dmc->dmc_flags_mask &
	    ~(DMC_NOJOURNAL | DMC_JOURNAL | DMC_FSTCOMMIT | DMC_SOLECACHE |
              DMC_CNF_LOCKED | DMC_CVCFG | DMC_ADMIN_DB | DMC_DMCM))
	{
	    SETDBERR(&dmc->error, 0, E_DM001A_BAD_FLAG);
	    break;
	}
	if (dmc->dmc_flags_mask & DMC_NOJOURNAL)
	    flags |= DM2D_NOJOURNAL;
	if (dmc->dmc_flags_mask & DMC_JOURNAL)
	    flags |= DM2D_JOURNAL;
	if (dmc->dmc_flags_mask & DMC_FSTCOMMIT)
	    flags |= DM2D_FASTCOMMIT;
	if (dmc->dmc_flags_mask & DMC_SOLECACHE)
	    flags |= DM2D_BMSINGLE;
	if (dmc->dmc_flags_mask & DMC_CVCFG)
	    flags |= DM2D_CVCFG;
        
        /* b97083 - Is the CNF file already locked by caller? */
        if (dmc->dmc_flags_mask & DMC_CNF_LOCKED)
            flags |= DM2D_CNF_LOCKED;

	if (dmc->dmc_s_type & DMC_S_SINGLE)
	    flags |= DM2D_SINGLE;
	if (dmc->dmc_s_type & DMC_S_MULTIPLE)
	    flags |= DM2D_MULTIPLE;

        /*
        ** (ICL phil.p)
        */
        if (dmc->dmc_flags_mask & DMC_DMCM)
            flags |= DM2D_DMCM;

        if (dmc->dmc_flags_mask2 & DMC2_READONLYDB)
            flags |= DM2D_READONLYDB;
	/*
	** It is an error to specify Fast Commit without specifying to
	** use a single buffer manager.
        ** (ICL phil.p) UNLESS running DMCM, which effectively means
        ** running FastCommit in a Multi-Cache environment.
	*/

        if (!(flags & DM2D_DMCM))
        {
            if ((flags & (DM2D_FASTCOMMIT | DM2D_BMSINGLE)) == DM2D_FASTCOMMIT)
	    {
		SETDBERR(&dmc->error, 0, E_DM0115_FCMULTIPLE);
                break;
	    }
        }

	mode = dmc->dmc_db_access_mode;
	if (mode != DMC_A_READ &&
	    mode != DMC_A_WRITE &&
	    mode != DMC_A_CREATE &&
	    mode != DMC_A_DESTROY)
	{
	    SETDBERR(&dmc->error, 0, E_DM000F_BAD_DB_ACCESS_MODE);
	    break;
	}
	dm2mode = (mode == DMC_A_READ) ? DM2D_A_READ : DM2D_A_WRITE;

	/*  Check that at least one location was passed in. */

	location = (DMC_LOC_ENTRY *)dmc->dmc_db_location.data_address;
	loc_count = dmc->dmc_db_location.data_in_size / sizeof(DMC_LOC_ENTRY);
	if (loc_count == 0)
	{
	    SETDBERR(&dmc->error, 0, E_DM002A_BAD_PARAMETER);
	    break;
	}
	    
	/*  Check if database should be created. */

	if (mode == DMC_A_CREATE)
	{
	    SCF_CB              scf_cb;
	    SCF_SCI             sci_list[2]; 
	    DB_NAME		collation;
	    DB_NAME		ucollation;
	    char		*p;
	    char		ucolname[] = "udefault";
	    i4		dbservice;

	    scf_cb.scf_length = sizeof(SCF_CB);
	    scf_cb.scf_type = SCF_CB_TYPE;
	    scf_cb.scf_facility = DB_DMF_ID;
	    scf_cb.scf_session = (SCF_SESSION)dmc->dmc_session_id;
	    scf_cb.scf_ptr_union.scf_sci = (SCI_LIST *)sci_list;
	    sci_list[0].sci_length = sizeof(dbservice);
	    sci_list[0].sci_code = SCI_DBSERVICE;
	    sci_list[0].sci_aresult = (char *)&dbservice;
	    sci_list[0].sci_rlength = 0;
	    scf_cb.scf_len_union.scf_ilength = 1;

	    status = scf_call(SCU_INFORMATION, &scf_cb);
	    if (status != E_DB_OK)
	    {
		uleFormat(&scf_cb.scf_error, 0, (CL_ERR_DESC *)NULL, 
		    ULE_LOG, NULL, (char *)0, (i4)0, (i4 *)0, 
		    &error, 0);
		SETDBERR(&dmc->error, 0, E_DM002F_BAD_SESSION_ID);
		break;
	    }

	    /*	Collation for iidbdb can only be the default. */

	    MEfill(sizeof(collation.db_name), ' ', collation.db_name);
	    NMgtAt("II_COLLATION", &p);
	    if (p && *p)
		MEmove(STlength(p), p, ' ', sizeof(collation.db_name), 
				collation.db_name);
	    MEmove(STlength(ucolname), ucolname, ' ', 
		   sizeof(ucollation.db_name), ucollation.db_name);

	    loc_ptr[0] = (DMM_LOC_LIST *) &loc_list[0];
	    loc_ptr[1] = (DMM_LOC_LIST *) &loc_list[1];
	    loc_ptr[2] = (DMM_LOC_LIST *) &loc_list[2];
	    loc_ptr[3] = (DMM_LOC_LIST *) &loc_list[3];

	    /* Even though the iidbdb is not "operative" at this stage, we
	    ** will mark it as operative in the config file now (it will not
	    ** be marked operative in the iidatabase catalog until after it
	    ** is fully created).  Although we would like to mark the iidbdb
	    ** "inoperative" in the config file now and update it to operative
	    ** status when creation is successfully completed (as is done for
	    ** all other DBs) the internal procedure "iiqef_alter_db" which
	    ** updates this bit will not work on the iidbdb; see comments in
	    ** createdb regarding this problem.
	    */ 
	    status = dmm_add_create(0, &dmc->dmc_db_name, &dmc->dmc_db_owner,
		 1, dbservice, DU_OPERATIVE, 
		(DB_LOC_NAME *) &dbdb_location, 11, "II_DATABASE", 4, 
		loc_ptr, collation.db_name, ucollation.db_name, &dmc->error);

	    if (status != E_DB_OK)
	    {
		if (dmc->error.err_code > E_DM_INTERNAL)
		{
		    uleFormat( &dmc->error, 0, NULL, ULE_LOG , NULL,
			    (char * )0, 0L, (i4 *)0, &local_error, 0);
		    SETDBERR(&dmc->error, 0, E_DM0084_ERROR_ADDING_DB);
		}
		break;
	    }
	}
	else if (mode == DMC_A_DESTROY)
	{
	    return (E_DB_OK);
	}

	/*  Call the physical layer to construct a DCB for this database. */

	status = dm2d_add_db(flags, &dm2mode, &dmc->dmc_db_name, 
		&dmc->dmc_db_owner, loc_count, (DM2D_LOC_ENTRY *)location, &dcb, 
		(i4 *)dmc->dmc_lock_list, &dmc->error);
	if (status != E_DB_OK)
	{
	    if (dmc->error.err_code > E_DM_INTERNAL)
	    {
		uleFormat( &dmc->error, 0, NULL, ULE_LOG , NULL,
		    (char * )0, 0L, (i4 *)0, &local_error, 0);
		SETDBERR(&dmc->error, 0, E_DM0084_ERROR_ADDING_DB);
	    }
	    break;
	}

	/*  Use the access mode passed back */
	dmc->dmc_db_access_mode = 
		(dm2mode == DM2D_A_READ) ? DMC_A_READ : DMC_A_WRITE;
	dmc->dmc_db_id = (char *)dcb;
	dmc->dmc_dbservice = dcb->dcb_dbservice;
	dmc->dmc_dbcmptlvl = dcb->dcb_dbcmptlvl;
	dmc->dmc_1dbcmptminor = dcb->dcb_1dbcmptminor;
	return (E_DB_OK);
    }

    return (status);
}
