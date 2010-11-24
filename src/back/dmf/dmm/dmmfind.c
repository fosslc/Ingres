/*

Problems --
	-- may want to add a verbose flag and add messages to make verbose
*/

/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <er.h>
#include    <cs.h>
#include    <cv.h>
#include    <di.h>
#include    <lo.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <ulf.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmmcb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dudbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dm2t.h>
#include    <dm2r.h>
#include    <dmxe.h>
#include    <dm0c.h>
#include    <dm0m.h>
#include    <dmm.h>
#include    <sqlstate.h>

/**
**
** Name: DMMFIND.C - Functions needed for finddbs
**
** Description:
**      This file contains the functions necessary to search a data location
**	for databases and update iidbdb catalogs with information that it
**	findbs.
**    
**	db_insert()	-   Format an iiphys_database tuple and insert it into 
**			    the catalog
**      dmm_finddbs()	-   Routine to perfrom the finddbs operation on a
**			    single location
**	dmm_message()	-   Send a mesasge to the FE.
**	ext_insert()	-   Format an iiphys_extend tuple and insert it into 
**			    the catalog
**	getloc()	-   get location name from iilocations that matches a 
**			    given v5 area
**	getuser()	-   get name from iitemp_codemap that matches a given 
**			    v5 user code
**	log_db()	-   Log a database when you find one [called from
**			    DIlistdir]
**	open_dbtbl()	-   Open table iiphys_database and format an empty tuple
**	open_exttbl()	-   Open table iiphys_extend and format an empty tuple
**	read_admin()	-   Obtain relevent info about unconverted v5 database
**			    from admin30 file
**	read_config()	-   Obtain relevent information for a database from 
**			    config file
**
** History:
**      28-dec-90 (teresa)
**	    initial creation.
**	06-feb-92 (ananth)
**	    Use the ADF_CB in the RCB instead of allocating it on the stack.
**	 7-jul-1992 (rogerk)
**	    Prototyping DMF.
**	06-nov-92 (jrb)
**	    Changed "sort" to "work" all over the place; for Multi-Location
**	    Sorts.
**	13-jan-93 (mikem)
**	    Lint directed changes.  Fixed "=" vs. "==" bug in error handling
**	    in read_config().  Previous to this fix DIsense failures would be
**	    ignored.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system typedefs
**	28-may-1993 (robf)
**	    Secure 2.0: Remove old ORANGE definitions
**	21-jun-1993 (rogerk)
**	    Fix prototype warning.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**	    Replace "JNL" define with "DMMFIND_JNL", "CKP" with "DMMFIND_CKP".
**	20-sep-1993 (andys)
**	    Fix arguments to ule_format call.
**      03-jun-1996 (canor01)
**          For operating-system threads, remove external semaphore protection
**          for NMingpath().
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
**      06-nov-00 (wansh01)
**          changed (*error = E_UL0002_BAD_ERROR_LOOKUP) in line 1048 
**          from '=' to '==' to compare the error code not set.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	16-dec-04 (inkdo01)
**	    Add bunches of collID's.
**      29-nov-2005 (horda03) Bug 48659/INGSRV 2716
**          A TX can now be started with a non-journaled BT record. This
**          leaves the XCB_DELAYBT flag set but sets the XCB_NONJNL_BT
**          flag. Need to write a journaled BT record if the update
**          will be journaled.
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here.
**	05-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_? functions converted to DB_ERROR *
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2t_?, dm2r_? functions converted to DB_ERROR *
**	12-Nov-2009 (kschendel) SIR 122882
**	    cmptlvl is an integer now.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Made dmm_message() static, rewrote with a variable number
**	    of parameters.
**/

/*
**  Forward and/or External typedef/struct references.
**  (-- ie, structures defined in this module  -- )
*/
typedef struct _DUF_ADMIN_HEADER DUF_ADMIN_HEADER;  /* format of admin30 hdr */
typedef struct _DUF_CODEMAP	DUF_CODEMAP;	    /* format of iitemp_codemap
						    ** tuple */
typedef struct _LOGDB_CB	LOGDB_CB;	    /* info passed to log_db()
						    ** and its subordinate
						    ** routines */

/* 
**  routines in this module 
*/

static DB_STATUS    db_insert (LOGDB_CB *logdb_cb, i4 *error);
static DB_STATUS    ext_insert (LOGDB_CB *logdb_cb, i4 *error);
static DB_STATUS    getloc(
				LOGDB_CB    *logdb_cb,
				i4	    devtype,
				char        *area,
				char        *locname,
				i4	    *bad_50db,
				i4	    *error);

static VOID	    getuser (
				LOGDB_CB *logdb_cb,
				char *code,
				DB_OWN_NAME *name);
static STATUS	    log_db(
				LOGDB_CB	*logdb_cb,
				char		*dirname,
				i4		dirlen,
				CL_ERR_DESC	*error);

static DB_STATUS    open_dbtbl(
				DMP_DCB		*dcb, 
				LOGDB_CB	*logdb_cb, 
				DMM_CB 		*dmm);

static DB_STATUS    open_exttbl(
				DMP_DCB			*dcb,
				LOGDB_CB		*logdb_cb,
				DMM_CB			*dmm);

static DB_STATUS    read_admin(
				DI_IO	    *di_io,
				LOGDB_CB    *logdb_cb,
				i4	    *ret_err);

static DB_STATUS    read_config(
				DI_IO	    *di_io,
				LOGDB_CB    *logdb_cb,
				char	    *filnam,
				i4	    fillen,
				i4	    *ret_err);

static DB_STATUS    dmm_message(
				i4	*error,
				i4	msg_id,
				i4	count,
				...);


/* 
**  constants used by this module 
**  -- mostly to indicate filenames and location names
*/
#define	    CONFIG_FILE		    "aaaaaaaa.cnf"
#define	    L_CONFIG_FILE	    12
#define	    RECOVERY_CONFIG_FILE    "aaaaaaaa.rfc"
#define	    L_RECOVERY_CONFIG_FILE  12
#define	    ADMIN30_FILE	    "admin30"
#define	    L_ADMIN30_FILE	    7
#define	    OLDADMIN30_FILE	    "oldadmin30"
#define	    L_OLDADMIN30_FILE	    10
#define	    MAXPATH50		    255
#define	    DMM_MSGBUF_SIZE	    500
#define	    CKP_DEFAULT		    "ii_checkpoint"
#define	    CKP_AREA		    "II_CHECKPOINT"
#define	    JNL_DEFAULT		    "ii_journal"
#define	    JNL_AREA		    "II_JOURNAL"
#define	    WORK_DEFAULT	    "ii_work"
#define	    LWORK_DEFAULT	    7
#define	    DMP_DEFAULT		    "ii_dump"
#define	    LDMP_DEFAULT	    7
#define	    DMMFIND_JNL		    1
#define	    DMMFIND_CKP		    2
#define	    CONFIG_TEXT		    "config file"

/* constants to describe attribute offsets in iiphys_extend dv array */
#define	    LNAME		    0
#define	    DNAME		    1
#define	    EXT_STATUS		    2

/* constants to describe attribute offsets in iiphys_database dv array */
#define	    NAME		    0
#define	    OWN			    1
#define	    DBDEV		    2
#define	    CKPDEV		    3
#define	    JNLDEV		    4
#define	    SORTDEV		    5
#define	    ACCESS		    6
#define	    DBSERVICE		    7
#define	    DBCMPTLVL		    8
#define	    DBCMPTMINOR		    9
#define	    DB_ID		    10
#define	    DMPDEV		    11

/*}
** Name: DUF_ADMIN_HEADER - used to access admin header from 5.0 admin file.
**
** Description:
**  ADMIN file struct
**
**      The ADMIN struct describes the initial part of the ADMIN file
**      which exists in each database.  This file is used to initially
**      create the database, to maintain some information about the
**      database, and to access the RELATION and ATTRIBUTE relations
**      on OPENR calls.
**      The ADMIN_LOG struct is after the ADMIN struct in the file
**      and contains information about what logs are being and have
**      been used.
**
** History:
**	02-jan-91  (Teresa)
**	    lifted from DUFCV60.QSC to put into server so that DI is not
**	    performed from FEs any longer.
*/
struct _DUF_ADMIN_HEADER
{
        char    ah_owner[2];		    /* user code of data base owner */
        i2      ah_flags;		    /* database flags */
        char    ah_version[8];              /* ingres version that created DB */
        char    ah_dbarea[MAXPATH50+1];     /* database AREA */
        char    ah_ckparea[MAXPATH50+1];    /* checkpoint AREA */
        char    ah_jnlarea[MAXPATH50+1];    /* journal AREA */
        i4      ah_time_sync_turned_off;    /* changed for syncdb */
        char    ah_pad[16];
};

/*}
** Name: DUF_CODEMAP - used to read a tuple from iitemp_codemap.
**
** Description:
**	Format of iitemp_codemap tuple.  The iitemp_codemap is created
**	from iicodemap via the finddbs FE.  The iicodemap table is created
**	by convto60 during conversion of the iidbdb.  If this installation
**	has never been converted, there will NOT be an iicodemap table.
**
**	This table must be created as:
**	    create table iitemp_codemap (uname c(12), ucode c(2))
**	    modify iitemp_codemap to btree on ucode
**	
** History:
**	14-jan-91  (Teresa)
**	    Initial creation.
*/
#define	    UCODE_LEN	    2
#define	    UNAME_LEN	    12

struct _DUF_CODEMAP
{
	char	uname[UNAME_LEN];   /* name that ucode translates to */
        char    ucode[UCODE_LEN];   /* user code of data base owner */
};

/*}
** Name: LOGDB_CB - structure for dmm_finddbs() to communicate with
**		    subordinate routines
**
** Description:
**
**	Control Block to permit dmm_finddbs to communiate with log_db() and
**	subordinate routines.  This is necesary because routine LOG_DB is 
**	called via DIlistdir.
**
**	note:  loc_name, loc_path and pathname are ALWAYS filled in by
**		dmm_finddbs before calling DIlistdir to call log_db().
**		If log_db or any of its subordinate routines wishes to report
**		an error, then it will set list_stat to the error code.  All
**		error messages generated by log_db or subordinate routines will
**		always take the dir_name, loc_name anmd path_name. They may
**		optionally take a 'filename' parameter.  Routine dmm_finddbs 
**		will use the filename parameter in the error message if 
**		filename[0] != '\0'.
**
** History:
**	02-jan-91 (teresa)
**	    initial creation.
**	06-feb-92 (ananth)
**	    Get rid of the field "adfcb". Use the ADF_CB in the RCB instead
**	    of allocating it on the stack. 
*/
struct _LOGDB_CB
{
	/**********************************/
	/* status, flags and housekeeping */
	/**********************************/
	DB_STATUS   list_stat;			/* return status info */
	i4	    db_count;			/* number of databases found */
	i4	    flags;			/* bitflags */
#define		STOP	    0X0001L	/* if set, abort futher processing */
#define		NOCODEMAP   0X0002L	/* if set, then dont try to lookup ucode
					** in iitemp_codemap */
#define		PRIV_50DBS  0x0004L	/* if set, mark unconverted v5 DBS as
					** private databases */
#define		VERBOSE	    0x0008L	/* verbose mode specified */
	DML_XCB	    *xcb;			/* ptr to Transaction CB */
	DMP_RCB	    *db_rcb;			/* iiphys_database RCB ptr */
	DMP_RCB	    *ext_rcb;			/* iiphys_extend RCB ptr */

	/****************************/
	/* ptrs to allocated memory */
	/****************************/
	DMP_MISC    *dbdvs_mem;			/* memory allocated by 1st
						** pass of routine db_insert */
	DMP_MISC    *exdvs_mem;			/* memory allocated by 1st
						** pass of routine ext_insert */
	DMP_MISC    *db_mem;			/* alloc. memory for db_tup */
	DMP_MISC    *db_e_mem;			/* alloc memory for db_etup */
	DMP_MISC    *db_d_mem;			/* alloc mem for db_datavalues*/
	DMP_MISC    *ext_mem;			/* alloc memory for ext_tup */
	DMP_MISC    *ext_e_mem;			/* alloc mem for ext_etup */
	DMP_MISC    *ext_d_mem;			/* alloc mem for ext_datavalues*/

	/*************/
	/* WORKSPACE */
	/*************/
	char	    *db_tup;			/* ptr to buffer of a work
						** iiphys_database tuple */
	char	    *db_etup;			/* ptr to buffer of empty 
						** iiphys_database tuple */
	char	    *ext_tup;			/* ptr to buffer of a work 
						** iiphys_extend tuple */
	char	    *ext_etup;			/* ptr to buffer of empty 
						** iiphys_extend tuple */
	DB_DATA_VALUE
		    *db_datavals;		/* array of datavalues for
						** iiphys_database */
	DB_DATA_VALUE
		    *ext_datavals;		/* array of datavalues for
						** iiphys_extend */
	DB_DATA_VALUE
		    *insdb_dvs;			/* array of datavalues used by
						** routine db_insert to describe
						** structure iidatabase */
	DB_DATA_VALUE
		    *insext_dvs;		/* array of datavalues used by
						** ext_insert() to describe
						** structure iiextend */
	DU_DATABASE iidatabase;			/* iiphys_database work tuple */
	DU_EXTEND   iiextend;			/* iiphys_extend work tuple */

	/*****************/
	/* ascii strings */
	/*****************/
	char	    dir_name[DB_MAXNAME+1];	/* null terminated dir name */
	char	    filename[DB_MAXNAME+1];	/* optional name of file */
	char	    loc_name[DB_LOC_MAXNAME+1];	/* null terminated logical name 
						** of search location */
	char	    loc_path[DB_AREA_MAX+1];	/* null terminated pathname that
						** loc_name translates to */
	char	    path_name[DB_AREA_MAX+1];	/* null terminated pathname to
						** ingres data parent directory 
						** for this location.  This is
						** loc_path with :[INGRES.DATA]
						** appended in whatever format
						** is appropriate to target
						** machine.*/
	i4	    path_len;			/* # of chars in path_name */
};

/*{
** Name: db_insert	-   Insert a tuple into iiphys_database.
**
** Description:
**
**      This routine formats an input tuple and outputs the formatted tuple
**	to DBMS catalog iiphys_database.  
**
**	NOTE:	this routine assumes that the iiphys_database table is
**		open before it is called.
**
** Inputs:
**	logdb_cb		Log_db Control Block.  Contains:
**	    .db_rcb		    ptr to Record control block for open 
**				     iiphys_database table
**	        .rcb_tcb_ptr		Table control block for open
**					 iiphys_database table
**		    .relatts		    number of attributes in table
**		    .relwid		    number of bytes to contain 1 tuple
**
**	    .iidatabase		    tuple to format and insert
**
** Outputs:
**	error			Error code:  E_DM919D_BAD_IIPHYS_DATABASE
**					     E_DM0000_OK
**      Returns:
**	    E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
** History:
**	22-jan-91 (teresa)
**	    initial creation
**	06-feb-92 (ananth)
**	    Use the ADF_CB in the RCB. There isn't a pointer to it in
**	    LOGDB_CB anymore.
*/
static DB_STATUS 
db_insert (
LOGDB_CB	*logdb_cb,
i4		*error)
{

    i4		i;	    /* a counter */
    DMP_RCB		*rcb = logdb_cb->db_rcb;
    DMP_TCB		*tcb = rcb->rcb_tcb_ptr;
    ADF_CB		*adf_cb = rcb->rcb_adf_cb;
    DB_DATA_VALUE	*from_dv;   /* dv to describe area as string */
    DB_DATA_VALUE	*to_dv;    /* dv to convert area as string to area
				    ** as formatted in iilocation tuple */
    i4		err_code;
    DB_STATUS		ret_stat;
    DB_ERROR		local_dberr;



    for (;;) /* error control loop */
    {
	/* start by copying the empty tuple into the work tuple area */
	MEcopy( (PTR) logdb_cb->db_etup, tcb->tcb_rel.relwid,
		(PTR) logdb_cb->db_tup);

	/* loop for each attribute in tuple */
	from_dv = (DB_DATA_VALUE *) logdb_cb->insdb_dvs;
	to_dv   = (DB_DATA_VALUE *) logdb_cb->db_datavals;
	for (i=0; i<tcb->tcb_rel.relatts; i++)
	{
	    *error = E_DM0000_OK;

	    /* format data */
	    ret_stat = adc_cvinto( adf_cb, &from_dv[i], &to_dv[i]);
	    if (ret_stat != E_DB_OK)
	    {
		/* generate error message that name could not be cohersed into 
		** table, then return with nonzero status to DIlistxxx().
		*/
		if ( adf_cb->adf_errcb.ad_emsglen)
		{
		    uleFormat(&adf_cb->adf_errcb.ad_dberror, 0, NULL, ULE_MESSAGE, 
			       (DB_SQLSTATE *) NULL,
			       adf_cb->adf_errcb.ad_errmsgp, 
			       adf_cb->adf_errcb.ad_emsglen, NULL, error, 0);
		    *error = E_DM919D_BAD_IIPHYS_DATABASE;
		}
		else
		if  (adf_cb->adf_errcb.ad_errcode)
		{
		    *error = adf_cb->adf_errcb.ad_errcode;
		}
		ret_stat = (E_DB_ERROR);
		break;
	    }
	} /* end loop for each attribute in tuple */

	if (ret_stat != E_DB_OK)
	    break;

	/* put data into tuple */
	ret_stat = dm2r_put(rcb, DM2R_NODUPLICATES, logdb_cb->db_tup, 
			    &local_dberr);
	if (ret_stat != E_DB_OK)
	{
	    *error = local_dberr.err_code;

	    /* generate error message */
	    if ( local_dberr.err_code > E_DM_INTERNAL)
	    {
		uleFormat(&local_dberr, 0, NULL, ULE_LOG, (DB_SQLSTATE *)NULL, 
		    NULL, 0, NULL, &err_code, 0);
		*error =  E_DM919D_BAD_IIPHYS_DATABASE;
	    }
	    ret_stat = E_DB_ERROR;
	    break;
	}
	break;

    }	/* end error control loop */

    return (ret_stat);
}

/*{
** Name: dmm_finddbs - search a location for databases
**
**  INTERNAL DMF call format:      status = dmm_finddbs(&dmm_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMM_FIND_DBS, &dmm_cb);
**
** Description:
**      This routine searches a location for database directories.  For
**	each database directory it finds, it updates iiphys_database and
**	iiphys_extend with information about that database.
**
** Inputs:
**      dmm_cb 
**          .type                           Must be set to DMM_MAINT_CB.
**          .length                         Must be at least sizeof(DMM_CB).
**	    .dmm_flags_mask		    Must be zero.
**          .dmm_tran_id                    Must be the current transaction 
**                                          identifier returned from the 
**                                          begin transaction operation.
**          .dmm_db_location		    Pointer to device name to search.
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
**	    				    E_DM006A_TRAN_ACCESS_CONFLICT
**                                          E_DM010C_TRAN_ABORTED
**
**         .error.err_data                  Set to attribute in error by 
**                                          returning index into attribute list.
**
** History:
**	28-dec-90 (teresa)
**	    initial creation
**	06-feb-92 (ananth)
**	    No need to allocate the ADF_CB on the stack. Instead use the one
**	    in the RCB.
**	21-jun-1993 (rogerk)
**	    Fix prototype warning.
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
dmm_finddbs(
DMM_CB    *dmm_cb)
{
    DMM_CB	    *dmm = dmm_cb;  /* DMM control block of input parameters */
    DML_XCB	    *xcb;	    /* transaction control block */
    DML_ODCB	    *odcb;	    /* open database control block */
    DI_IO	    di_io;	    /* DI I/O control block */
    CL_ERR_DESC	    sys_err;	    /* System error information */
    STATUS	    status;	    /* return status from DI calls */
    DB_STATUS	    ret_stat;	    /* return status for routine dmm_finddbs */
    LOGDB_CB	    logdb_cb;	    /* Control block to pass parameters to
				    ** routine Log_db() */
    DB_ERROR	    local_dberr;
    i4	    	    *err_code = &dmm->error.err_code;
    i4	    	    error;
    i4         db_lockmode;

    /* stuff that LO CL calls need to translate an area to a directory name */
    char	    *stringptr;	    /* ptr that LO CL uses to translate
				    ** locations to strings */
    LOCATION	    searchloc;	    /* location built by LOingpath that contains
				    ** the location to dir: AREA:[INGRES.DATA]*/
    char            loc_buffer[DB_AREA_MAX+1];	/* CL work buffer */

    CLRDBERR(&dmm->error);


    /* first zero out logdb_cb, since it has uninitialized flags that could
    ** drive processing in certain error conditions.  Eleminate this possibility
    */
    MEfill(sizeof(logdb_cb), '\0', (PTR) &logdb_cb);

    /*
    **	Check arguments. 
    */
    for (;;)
    {
	ret_stat = E_DB_ERROR;
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
	    SETDBERR(&dmm->error, 0, E_DM005D_TABLE_ACCESS_CONFLICT);
	    break;
	}

	db_lockmode = DM2T_S;
	if (odcb->odcb_lk_mode == ODCB_X)
	    db_lockmode = DM2T_X;

	/*  
	**  Check Location. 
	*/
	if (dmm->dmm_db_location.data_in_size == 0 ||
	    dmm->dmm_db_location.data_address == 0)
	{
	    SETDBERR(&dmm->error, 0, E_DM0072_NO_LOCATION);
	    break;
	}
	if (dmm->dmm_location_name.db_loc_name[0] == '\0')
	{
	    SETDBERR(&dmm->error, 0, E_DM0072_NO_LOCATION);
	    break;
	}

	/* If it gets here, then input parameters were OK. */
	ret_stat = E_DB_OK;

	/*
	** init parameters to log_db(), which will be called by DIlistdir() for
	** each directory it finds 
	*/
	logdb_cb.xcb = xcb;			/* ptr to transaction CB */
	if (dmm->dmm_flags_mask & DMM_UNKNOWN_DBA)
	    logdb_cb.flags |= NOCODEMAP;
	if (dmm->dmm_flags_mask & DMM_PRIV_50DBS)
	    logdb_cb.flags |= PRIV_50DBS;
	MEfill (DB_AREA_MAX, ' ', (PTR)logdb_cb.loc_path);
	if (dmm->dmm_db_location.data_in_size > DB_AREA_MAX)
	    dmm->dmm_db_location.data_in_size = DB_AREA_MAX;
	MEcopy ( dmm->dmm_db_location.data_address, 
		 dmm->dmm_db_location.data_in_size,
		 (PTR)logdb_cb.loc_path
	       );
	logdb_cb.loc_path[dmm->dmm_db_location.data_in_size] = '\0';
	(VOID)STzapblank(logdb_cb.loc_path,logdb_cb.loc_path);	/* eleminate 
								** whitespace*/
	MEcopy( (PTR)dmm->dmm_location_name.db_loc_name, DB_LOC_MAXNAME,
		(PTR)logdb_cb.loc_name);
	logdb_cb.loc_name[DB_LOC_MAXNAME]='\0';	/* null terminate */
	(VOID)STzapblank(logdb_cb.loc_name,logdb_cb.loc_name);	/* eleminate 
								** whitespace*/

	/* 
	**  build full pathname to this area's DB directory, and put into 
	**  logdb_cb
	*/
	STcopy(logdb_cb.loc_path, loc_buffer);
	LOingpath(loc_buffer, 0, LO_DB, &searchloc);
	LOtos(&searchloc, &stringptr);
	STcopy(stringptr, logdb_cb.path_name);
	logdb_cb.path_len = STlength(stringptr);

	/*
	**	now attempt to open the two work tables that finddbs needs:
	**		iiphys_database and iiphys_extend
	*/
	ret_stat = open_dbtbl(odcb->odcb_dcb_ptr, &logdb_cb, dmm);

	if (ret_stat != E_DB_OK)
	{
	    uleFormat(&dmm->error, 0, NULL, ULE_LOG, (DB_SQLSTATE *) NULL, 
			NULL, 0, NULL, err_code, 0);
	    SETDBERR(&dmm->error, 0, E_DM0144_NONFATAL_FINDBS_ERROR);
	    break;
	}	
	ret_stat = open_exttbl(odcb->odcb_dcb_ptr, &logdb_cb, dmm);
	if (ret_stat != E_DB_OK)
	{
	    uleFormat(&dmm->error, 0, NULL, ULE_LOG, (DB_SQLSTATE *) NULL, 
			NULL, 0, NULL, err_code, 0);
	    SETDBERR(&dmm->error, 0, E_DM0144_NONFATAL_FINDBS_ERROR);
	    break;
	}	

	/*
	** If this is the first write operation for this transaction,
	** then we need to write the begin transaction record.
	*/
	if (xcb->xcb_flags & XCB_DELAYBT)
	{
	    status = dmxe_writebt(xcb, FALSE, &dmm->error);
	    if (status != E_DB_OK)
	    {
		xcb->xcb_state |= XCB_TRANABORT;
		break;
	    }
	}

	/* 
	**	call DIlistdir to find all directories in a location 
	*/
	status = DIlistdir(&di_io, logdb_cb.path_name, logdb_cb.path_len, 
			    log_db, (PTR) &logdb_cb, &sys_err);
	if (status == OK)
	{
	    dmm->dmm_count = logdb_cb.db_count;
	}
	else /* determine what the problem was */
	{
	    if ( (status == DI_BADDIR) || (status == DI_DIRNOTFOUND) )
	    {
		/* search directory does not exist or else there is syntax error
		** in pathname */
		ret_stat = dmm_message (&error, W_DM5401_LOC_DOESNOT_EXIST, 2,
		    logdb_cb.path_len, logdb_cb.path_name,
		    sizeof(logdb_cb.loc_name), logdb_cb.loc_name);
		SETDBERR(&dmm->error, 0, E_DM0144_NONFATAL_FINDBS_ERROR);
	    }
	    else if (status == DI_ENDFILE)
	    {
		/* routine log_db or a subordinate routine encountered an error,
		** so processing was aborted */
		if (logdb_cb.list_stat != E_DM0000_OK)
		{
		    /* print the error that was returned.  All errors returned
		    ** use the following parameters:
		    **	directory name, location name, location path
		    ** Error msgs may optionally have a 4th parameter of filename.
		    */
		    if (logdb_cb.filename[0] == '\0')
			ret_stat = dmm_message (&error, logdb_cb.list_stat, 3,
				0, logdb_cb.dir_name, 0, logdb_cb.loc_name,
				0, logdb_cb.path_name);
		    else
			ret_stat = dmm_message (&error, logdb_cb.list_stat, 4,
				0, logdb_cb.dir_name, 0, logdb_cb.loc_name,
				0, logdb_cb.path_name, 
				DB_MAXNAME, logdb_cb.filename);
                  
		    /* see if this is a fatal error */
		    if (logdb_cb.list_stat == E_DM5410_CLOSE_ERROR)
		    {
			uleFormat (NULL, logdb_cb.list_stat, NULL, ULE_LOG,
				    (DB_SQLSTATE *) NULL,
				    NULL, 0, NULL, &error, 4,
				    0, logdb_cb.dir_name, 0, logdb_cb.loc_name,
				    0, logdb_cb.path_name, 
				    DB_MAXNAME, logdb_cb.filename);
			SETDBERR(&dmm->error, 0, E_DM0145_FATAL_FINDBS_ERROR);
			ret_stat = E_DB_FATAL;
		    }
		    else if (logdb_cb.flags & STOP)
		    {
			SETDBERR(&dmm->error, 0, E_DM0144_NONFATAL_FINDBS_ERROR);
			ret_stat = E_DB_ERROR;
		    }
		    else /* ignore error and continue processing */
			CLRDBERR(&dmm->error);
		}
		else
		{
		    /* Some DIlist CL routine return DI_ENDFILE instead of
		    ** OK.  This is probably a CL bug, but deal with it
		    ** here. */
		    CLRDBERR(&dmm->error);
                    ret_stat = E_DB_OK;
		}
	    }
	    else
	    {
		/* unanticipated DIlistdir error.  Report as warning */
		ret_stat = E_DB_WARN;
		SETDBERR(&dmm->error, 0, logdb_cb.list_stat);
	    }
	}
	break;	    /* break out of control loop, as work is done */
    }

    /* we can get here by success completion or by taking an error break
    ** from above control loop.
    */

    {
        /* deallocate any memory we have successfully allocated.  Also close
        ** any open tables */

	DB_STATUS   locstat;
	i4	    locerror;

	if (logdb_cb.db_mem)
	{
	    (VOID) dm0m_deallocate( (DM_OBJECT **) &logdb_cb.db_mem);
	}
	if (logdb_cb.db_e_mem)
	{
	    (VOID) dm0m_deallocate( (DM_OBJECT **) &logdb_cb.db_e_mem);
	}
	if (logdb_cb.db_d_mem)
	{
	    (VOID) dm0m_deallocate( (DM_OBJECT **) &logdb_cb.db_d_mem);
	}
	if (logdb_cb.ext_mem)
	{
	    (VOID) dm0m_deallocate( (DM_OBJECT **) &logdb_cb.ext_mem);
	}
	if (logdb_cb.ext_e_mem)
	{
	    (VOID) dm0m_deallocate( (DM_OBJECT **) &logdb_cb.ext_e_mem);
	}
	if (logdb_cb.ext_d_mem)
	{
	    (VOID) dm0m_deallocate( (DM_OBJECT **) &logdb_cb.ext_d_mem);
	}
	if (logdb_cb.dbdvs_mem)
	{
	    (VOID) dm0m_deallocate( (DM_OBJECT **) &logdb_cb.dbdvs_mem);
	}
	if (logdb_cb.exdvs_mem)
	{
	    (VOID) dm0m_deallocate( (DM_OBJECT **) &logdb_cb.exdvs_mem);
	}

	if (logdb_cb.db_rcb)
	{
	    locstat = dm2t_close(logdb_cb.db_rcb, DM2T_NOPURGE, &local_dberr);
	    logdb_cb.db_rcb = (DMP_RCB *) NULL;
	    if (locstat != E_DB_OK)
		if (locstat > ret_stat)
		{
		    ret_stat = locstat;
		    dmm->error = local_dberr;
		}
	}
	if (logdb_cb.ext_rcb)
	{
	    locstat = dm2t_close(logdb_cb.ext_rcb, DM2T_NOPURGE, &local_dberr);
	    logdb_cb.ext_rcb = (DMP_RCB *) NULL;
	    if (locstat != E_DB_OK)
		if (locstat > ret_stat)
		{
		    ret_stat = locstat;
		    dmm->error = local_dberr;
		}
	}	

    }

    /* determine what error status/error code to return */
    if (ret_stat != E_DB_OK)
    {
	i4	    loc_error;

	if (dmm->error.err_code > E_DM_INTERNAL)
	{
	    uleFormat( &dmm->error, 0, NULL, ULE_LOG , (DB_SQLSTATE *) NULL, 
		(char * )NULL, 0L, (i4 *)NULL, 
		&loc_error, 0);
	    SETDBERR(&dmm->error, 0, E_DM0144_NONFATAL_FINDBS_ERROR);
	}
	if (dmm->error.err_code == E_DM0144_NONFATAL_FINDBS_ERROR)
	    (VOID) dmm_message (&loc_error, E_DM5411_ERROR_SEARCHING_LOC, 2,
				0, logdb_cb.loc_name,
				logdb_cb.path_len, logdb_cb.path_name);

	switch (dmm->error.err_code)
	{
	    case E_DM004B_LOCK_QUOTA_EXCEEDED:
	    case E_DM0112_RESOURCE_QUOTA_EXCEED:
	    case E_DM0144_NONFATAL_FINDBS_ERROR:
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

    return ret_stat;
}

/*{
** Name: dmm_message	-   Send a mesasge to the FE.
**
** Description:
**      Some DMM functionality sends inforamtive or warning messages to the FE 
**	using the same mechanism that DB Procedures use to send messages.  This 
**	routine handles formatting the message and sending it to the FE via  
**	SCF's SCC_MESSAGE mechanism.
**
**	This was written for the DMM finddbs logic.  The basic rule here is
**	that INTERNAL errors are reported via ULE_FORMAT and USER INFORMATION
**	is reported via dmm_message.  In general, if the error message starts
**	with "E_DM9" it is an internal error and should be reported via 
**	ule_format.  If a message starts with "I_DM5", "W_DM5" or "E_DM5", then
**	it should be sent via dmm_message.  The exception is message
**	E_DM5410_CLOSE_ERROR, which is sent both by ule_format and by
**	dmm_message.
**
** Inputs:
**	error				address of i4 to put error code
**					    into in the event that dmm_message
**					    encounters an error
**      msg_id                          message number (a hex constant)
**      count                           number of parameters assoc with message
**					    (max # parameters allowed is 12)
**	pNval				address of parameter [N = 1 .. 12 ]
**	pNsize				number of bytes used to contain
**					    parameter [N = 1 .. 12]
**
** Outputs:
**	error				will contain an error code if return
**					    status is NOT E_DB_OK:
**						E_DM0000_OK
**						E_DM9195_BAD_MESSAGESEND
**      Returns:
**	    E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      04-jan-91 (teresa)
**          initial creation (modeled from routine dm1u_talk() in dm1u.c).
**	24-oct-92 (andre)
**	    in SCF_CB, generic_eror hjas been replaced with SQLSTATE
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Rewrote with variable number of parameters for prototyping.
[@history_template@]...
*/
static DB_STATUS
dmm_message(
i4	*error,
i4	msg_id,
i4	count,
...)
{
#define NUM_ER_ARGS	12
    char        cbuf[DMM_MSGBUF_SIZE];
    char        *buf;
    SCF_CB      scf_cb;
    DB_STATUS   stat;
    i4     mlen;
    char	hex_string[sizeof(i4)+1];
    char	*sqlstate_str = SS5000G_DBPROC_MESSAGE;
    i4		i;
    va_list	ap;
    ER_ARGUMENT er_args[NUM_ER_ARGS];

    va_start( ap, count );
    for ( i = 0; i < count && i < NUM_ER_ARGS; i++ )
    {
        er_args[i].er_size = (i4) va_arg( ap, i4 );
	er_args[i].er_value = (PTR) va_arg( ap, PTR );
    }
    va_end( ap );
    
    *error = E_DM0000_OK;
    buf = &cbuf[0];

    stat = uleFormat(NULL, msg_id, NULL, ULE_LOOKUP, (DB_SQLSTATE *) NULL, buf,
			DMM_MSGBUF_SIZE, &mlen, error, i, 
			er_args[0].er_size, er_args[0].er_value,
			er_args[1].er_size, er_args[1].er_value,
			er_args[2].er_size, er_args[2].er_value,
			er_args[3].er_size, er_args[3].er_value,
			er_args[4].er_size, er_args[4].er_value,
			er_args[5].er_size, er_args[5].er_value,
			er_args[6].er_size, er_args[6].er_value,
			er_args[7].er_size, er_args[7].er_value,
			er_args[8].er_size, er_args[8].er_value,
			er_args[9].er_size, er_args[9].er_value,
			er_args[10].er_size, er_args[10].er_value,
			er_args[11].er_size, er_args[11].er_value );

    if ( (stat == E_DB_ERROR) ||
	 ( (stat == E_DB_WARN) &&  (*error == E_UL0002_BAD_ERROR_LOOKUP) )
       )
    {
        /* ule_format cannot find the message number in the lookup file,
	** or it found it but was unable to format it.  In either case,
        ** output a TRACE.  Change the contents of the message to the FE
	** to indicate that there is a message but we cannot format it
	*/
        TRdisplay("ULE error formatting DMF informative/warning message:\n%s\n",
                    cbuf);
	CVlx(msg_id,hex_string);
	hex_string[sizeof(i4)] = '\0'; /* null terminate */
	STpolycat(3, "COMMUNICATIONS ERROR:  unable to format message ",
		  hex_string, "\n", buf);
    }

    /* Format SCF request block */
    scf_cb.scf_facility = DB_DMF_ID;
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_nbr_union.scf_local_error = msg_id;
    for (i = 0; i < DB_SQLSTATE_STRING_LEN; i++)
	scf_cb.scf_aux_union.scf_sqlstate.db_sqlstate[i] = sqlstate_str[i];
	
    scf_cb.scf_ptr_union.scf_buffer = buf;
    scf_cb.scf_len_union.scf_blength = mlen;
    stat = scf_call(SCC_MESSAGE, &scf_cb);
    if (DB_FAILURE_MACRO(stat))
    {
        TRdisplay("ERROR SENDING MSG TO FE:\n\n%s\n", cbuf);
	*error =  E_DM9195_BAD_MESSAGESEND;
    }
    return (stat);
}	

/*{
** Name: ext_insert	-   Insert a tuple into iiphys_extend.
**
** Description:
**
**      This routine formats an input tuple and outputs the formatted tuple
**	to DBMS catalog iiphys_extend.  
**
**	NOTE:	this routine assumes that the iiphys_extend table is
**		open before it is called.
**
** Inputs:
**	logdb_cb		Log_db Control Block.  Contains:
**	    .ext_rcb		    ptr to Record control block for open 
**				     iiphys_extend table
**	        .rcb_tcb_ptr		Table control block for open
**					 iiphys_extend table
**		    .relatts		    number of attributes in table
**		    .relwid		    number of bytes to contain 1 tuple
**
**	    .iiextend		    tuple to format and insert
**
** Outputs:
**	error			Error code:  E_DM919C_BAD_IIPHYS_EXTEND
**					     E_DM9188_DMM_ALLOC_ERROR
**      Returns:
**	    E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
** History:
**	14-jan-91 (teresa)
**	    initial creation
**	06-feb-92 (ananth)
**	    Use the ADF_CB in the RCB. There isn't a pointer to it in
**	    LOGDB_CB anymore.
*/
static DB_STATUS 
ext_insert (
LOGDB_CB	*logdb_cb,
i4		*error)
{

    i4		i;
    DMP_RCB		*rcb = logdb_cb->ext_rcb;
    DMP_TCB		*tcb = rcb->rcb_tcb_ptr;
    ADF_CB		*adf_cb = rcb->rcb_adf_cb;
    DB_DATA_VALUE	*from_dv;   /* dv to describe area as string */
    DB_DATA_VALUE	*to_dv;    /* dv to convert area as string to area
				    ** as formatted in iilocation tuple */
    i4			err_code;
    DB_STATUS		ret_stat;
    DB_ERROR		local_dberr;



    for (;;) /* error control loop */
    {
	/* start by copying the empty tuple into the work tuple area */
	MEcopy( (PTR) logdb_cb->ext_etup, tcb->tcb_rel.relwid,
		(PTR) logdb_cb->ext_tup);

	/* loop for each attribute in tuple */
	from_dv = (DB_DATA_VALUE *) logdb_cb->insext_dvs;
	to_dv   = (DB_DATA_VALUE *) logdb_cb->ext_datavals;
	for (i=0; i<tcb->tcb_rel.relatts; i++)
	{
	    /* format data */
	    ret_stat = adc_cvinto( adf_cb, &from_dv[i], &to_dv[i]);
	    if (ret_stat != E_DB_OK)
	    {
		/* generate error message that name could not be cohersed into 
		** table, then return with nonzero status to DIlistxxx().
		*/
		if ( adf_cb->adf_errcb.ad_emsglen)
		{
		    uleFormat(&adf_cb->adf_errcb.ad_dberror, 0, NULL, ULE_MESSAGE, 
			       (DB_SQLSTATE *) NULL,
			       adf_cb->adf_errcb.ad_errmsgp, 
			       adf_cb->adf_errcb.ad_emsglen, NULL, &err_code, 0);
		    *error = E_DM919C_BAD_IIPHYS_EXTEND;
		}
		else
		if  (adf_cb->adf_errcb.ad_errcode)
		{
		    *error = adf_cb->adf_errcb.ad_errcode;
		}
		ret_stat = (E_DB_ERROR);
		break;
	    }
	} /* end loop for each attribute in tuple */

	if (ret_stat != E_DB_OK)
	    break;

	/* put data into tuple */
	ret_stat = dm2r_put(rcb, DM2R_NODUPLICATES, logdb_cb->ext_tup, 
			    &local_dberr);
	if (ret_stat != E_DB_OK)
	{
	    *error = local_dberr.err_code;

	    /* generate error message */
	    if ( local_dberr.err_code > E_DM_INTERNAL)
	    {
		uleFormat(&local_dberr, 0, NULL, ULE_LOG, (DB_SQLSTATE *) NULL, 
		    NULL, 0, NULL, &err_code, 0);
		*error = E_DM919C_BAD_IIPHYS_EXTEND;
	    }
	    ret_stat = E_DB_ERROR;
	    break;
	}
	break;

    }	/* end error control loop */

    return (ret_stat);
}

/*{
** Name: getloc - get location name from iilocations that matches a given area
**
** Description:
**
**      This routine reads iitemp_locations and searches for an area that matches
**	the input area.  If one is found, it returns that name.  If none are
**	found or more than 1 is found, then it is impossible to map area to
**	a location name. This is NOT a dmf bug, but an installation setup bug,
**	so getloc returns DMM_USER_ERRROR.
**
**	Routine get_loc translates certain v5 area keywords to their v6 
**	equivalent before searching iitemp_locations.
**
**	ASSUMPTIONS:
**	    iitemp_locations is keyed on area. This is different that the
**	    iilocations catalog, which is keyed on lname.  This routine 
**	    also assumes that iitemp_locations is closed when it is called, and
**	    will assure that iitemp_locations is closed when it exits.
**
** Inputs:
**
**	logdb_cb		log_db control block.  Contains
**	devtype		    Indicates device type to search for:
**				DMMFIND_JNL - journal
**	area		    A pathname of a location obtained from admin30
**
** Outputs:
**	locname		    name of location that area maps to
**	bad_50db	    flag.  True -> there was a problem mapping this
**				area that is due to installation definition
**				error and NOT due to DMF internal error.  If
**				bad_50db=TRUE, then return status = E_DB_OK
**	error		    Error code if DMF internal error occurs
**				    E_DM9196_BADOPEN_IITEMPLOC
**				    E_DM9197_BADREAD_IITEMPLOC
**				    or errors returned from DMF subroutines
**
**      Returns:
**	    E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          file iitemp_locations is opened, read and closed.  THis may result
**	    in page locks on this file that are held until the end of the
**	    transaction.
**
** History:
**	09-jan-91 (teresa)
**	    initial creation
**	06-feb-92 (ananth)
**	    Use the ADF_CB in the RCB. There isn't a pointer to it in
**	    LOGDB_CB anymore.
*/
static DB_STATUS 
getloc(
LOGDB_CB    *logdb_cb,
i4	    devtype,
char        *area,
char        *locname,
i4	    *bad_50db,
i4	    *error)
{
    char		area_v6[DB_AREA_MAX+4];
    char		area_v5[MAXPATH50+1];
    i4		row_cnt=0;
    DML_XCB		*xcb= logdb_cb->xcb;
    DML_ODCB		*odcb = (DML_ODCB *) xcb->xcb_odcb_ptr;
    DMP_DCB		*dcb = (DMP_DCB *)odcb->odcb_dcb_ptr;
    DB_TAB_ID		table_id;
    DB_TAB_TIMESTAMP    timestamp;
    DMP_RCB		*rcb=0;
    DMP_TCB		*tcb;
    DB_STATUS		ret_stat;
    i4		tupcnt=0;
    ADF_CB		*adf_cb;
    DU_LOCATIONS	tuple;	    /* an iilocation tuple */
    DM2R_KEY_DESC       key_desc;   /* key for iitemp_locations table */
    DB_DATA_VALUE	from_dv;    /* dv to describe area as string */
    DB_DATA_VALUE	to_dv;	    /* dv to convert area as string to area
				    ** as formatted in iilocation tuple */
    i4		err_code;
    DB_ERROR		local_dberr;

    /* first translate default keywords to their v6 counter-part */
    *bad_50db = FALSE;
    STcopy(area, area_v5);

    if ( (devtype == DMMFIND_JNL) && !STcompare(area_v5, "JNL_INGRES") )
	STcopy(JNL_AREA, area_v5);
    else if ( (devtype==DMMFIND_JNL) && !STcompare(area_v5, "jnl_ingres") )
	STcopy(JNL_AREA, area_v5);
    else if ( (devtype==DMMFIND_JNL) && !STcompare(area_v5, "default") )
	STcopy(JNL_AREA, area_v5);
    else if ( (devtype==DMMFIND_CKP) && !STcompare(area_v5, "CKP_INGRES") )
	STcopy(CKP_AREA, area_v5);
    else if ( (devtype==DMMFIND_CKP) && !STcompare(area_v5, "ckp_ingres") )
	STcopy(CKP_AREA, area_v5);
    else if ( (devtype==DMMFIND_CKP) && !STcompare(area_v5, "default") )
	STcopy(CKP_AREA, area_v5);

    /* control loop */
    for (;;)
    {
        /* now open iitemp_locations and try to map the checkpoint or
        ** journal area to a location name */
    
	*error = E_DM0000_OK;
	table_id.db_tab_base = DM_B_TEMP_LOC_TAB_ID;
	table_id.db_tab_index = DM_I_TEMP_LOC_TAB_ID;

	ret_stat = dm2t_open(dcb, &table_id, DM2T_X,  DM2T_UDIRECT, DM2T_A_READ,
			    (i4)0, (i4)20, (i4)0, 
			    xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0, 
			    (i4)0, 0, &xcb->xcb_tran_id, 
			    &timestamp, &rcb, (DML_SCB *)0, &local_dberr);
	if (ret_stat != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, NULL, ULE_LOG, (DB_SQLSTATE *) NULL, 
		NULL, 0, NULL, &err_code, 0);
	    *error = E_DM9196_BADOPEN_IITEMPLOC;
	    break;
	}

	/* build a key from pathname read from admin30 file */
	tcb = rcb->rcb_tcb_ptr;

	adf_cb = rcb->rcb_adf_cb;

	from_dv.db_length = STlength(area_v5);
	from_dv.db_datatype = (DB_DT_ID) DB_CHA_TYPE;
	from_dv.db_data = (PTR) area_v5;
	from_dv.db_prec = 0;
	from_dv.db_collID = -1;

	to_dv.db_length = tcb->tcb_atts_ptr[DM_1_TEMP_LOC_KEY].length;
	to_dv.db_datatype = 
			(DB_DT_ID) tcb->tcb_atts_ptr[DM_1_TEMP_LOC_KEY].type;
	to_dv.db_data = (PTR) area_v6;
	to_dv.db_prec = 0;
	to_dv.db_collID = tcb->tcb_atts_ptr[DM_1_TEMP_LOC_KEY].collID;

	/* convert string read from admin30 file into format used in
	** iitemp_locations file */
	ret_stat = adc_cvinto( adf_cb, &from_dv, &to_dv);
	if (ret_stat != E_DB_OK)
	{
	    /* generate error message that name could not be cohersed into 
	    ** table, then return with nonzero status to DIlistxxx().
	    */
	    if ( adf_cb->adf_errcb.ad_emsglen)
	    {
		uleFormat(&adf_cb->adf_errcb.ad_dberror, 0, NULL, ULE_MESSAGE, 
			   (DB_SQLSTATE *) NULL,  adf_cb->adf_errcb.ad_errmsgp, 
			   adf_cb->adf_errcb.ad_emsglen, NULL, &err_code, 0);
		*error = E_DM9197_BADREAD_IITEMPLOC;
	    }
	    else
	    if  (adf_cb->adf_errcb.ad_errcode)
	    {
		*error = adf_cb->adf_errcb.ad_errcode;
	    }
	    ret_stat = E_DB_ERROR;
	    break;
	}
	
	key_desc.attr_operator = DM2R_EQ;
	key_desc.attr_number = DM_1_TEMP_LOC_KEY;
	key_desc.attr_value = area_v6;	    	    

	ret_stat = dm2r_position(rcb, DM2R_QUAL, &key_desc, (i4)1,
			       (DM_TID *)NULL, &local_dberr);
	if (ret_stat != E_DB_OK)
	{
	    *error = local_dberr.err_code;
	    break;      
	}
	for (;;)
	{
	    DM_TID	tid;
	    /* loop to get tuples.  We want exactly 1 tuple or
	    ** we should report a user error */
	    ret_stat = dm2r_get(rcb, &tid, DM2R_GETNEXT, 
		(char *)&tuple, &local_dberr);
	    *error = local_dberr.err_code;

	    if (ret_stat != E_DB_OK && *error != E_DM0055_NONEXT)
		break;
	    else if (*error == E_DM0055_NONEXT)
	    {
		ret_stat = E_DB_OK;
		*error = E_DM0000_OK;
		break;
	    }
	    if (++tupcnt == 1)
		MEcopy ((PTR)tuple.du_lname, tuple.du_l_lname, (PTR)locname);
	}
	if (ret_stat != E_DB_OK)
	    break;
	if (tupcnt == 0)
	{
	    /* cannot find entry in iitemp_locations to map to this area */
	    ret_stat = dmm_message (error, W_DM5407_NO_V6_LOCATION, 1,
				    MAXPATH50, area_v5);
	    if (ret_stat != E_DB_OK)
		break;
	    *bad_50db = TRUE;
	}
	else if (tupcnt > 1)
	{
	    /* too many entries in iitemp_locations map to this area. We
	    ** dont know which one to use, so cannot determine loc name */
	    ret_stat = dmm_message (error, W_DM5408_MULTIPLE_V6_LOCATIONS, 1,
				    MAXPATH50, area_v5);
	    if (ret_stat != E_DB_OK)
		break;
	    *bad_50db = TRUE;
	}
	break;	/* exit control loop */

    }  /* end of control loop */
    
    /* close iitemp_locations table if it is open */
    if (rcb)
    {
	DB_STATUS	locstat;
	i4	locerror;

	MEcopy( (PTR)tcb->tcb_rel.relid.db_tab_name, DB_TAB_MAXNAME, 
		(PTR)logdb_cb->filename);
	logdb_cb->filename[DB_TAB_MAXNAME] = '\0'; /* null terminate */

	locstat = dm2t_close(rcb, DM2T_NOPURGE, &local_dberr);
	rcb = (DMP_RCB *) NULL;
	if (locstat != E_DB_OK)
	{
	    logdb_cb->list_stat = E_DM5410_CLOSE_ERROR;
	    logdb_cb->flags |= STOP;
	    *error = E_DM5410_CLOSE_ERROR;
	    ret_stat = E_DB_FATAL;
	}
	else
	    logdb_cb->filename[0] = '\0';   /* clear this error parameter since
					    ** there was not an error */
    }	
    return (ret_stat);
}

/*{
** Name: getuser - get name from iitemp_codemap that matches a given user code
**
** Description:
**
**      This routine reads iitemp_codempat and searches for a user code that 
**	matches the input code.  If one is found, it returns the name assocaited
**	with that code.  If none are found or more than 1 is found, then it 
**	returns the string "UNKNOWN".
**
**	ASSUMPTIONS:
**	    iitemp_codemap is keyed on ucode.  This routine also assumes that 
**	    iitemp_codemap is closed when it is called, and will assure that 
**	    iitemp_codemap is closed when it exits.
**
** Inputs:
**
**	logdb_cb	    log_db control block.
**	code		    A two letter user code read from admin30 file
**
** Outputs:
**	name		    a 12 char user name that the user code maps to.
**
**      Returns:
**	    NONE.
**      Exceptions:
**          none
**
** Side Effects:
**          file iitemp_codemap is opened, read and closed.  THis may result
**	    in page locks on this file that are held until the end of the
**	    transaction.
**
** History:
**	14-jan-91 (teresa)
**	    initial creation
**	06-feb-92 (ananth)
**	    Use the ADF_CB in the RCB. There isn't a pointer to it in
**	    LOGDB_CB anymore.
*/
static VOID 
getuser (
LOGDB_CB    *logdb_cb,
char	    *code,
DB_OWN_NAME    *name)
{
    DML_XCB		*xcb= logdb_cb->xcb;
    DML_ODCB		*odcb = (DML_ODCB *) xcb->xcb_odcb_ptr;
    DMP_DCB		*dcb = (DMP_DCB *)odcb->odcb_dcb_ptr;
    ADF_CB		*adf_cb;
    DB_DATA_VALUE	from_dv;
    DB_DATA_VALUE	to_dv;
    DM2R_KEY_DESC       key_desc;   /* key for iitemp_codemap table */
    DB_TAB_ID		table_id;
    DB_TAB_TIMESTAMP    timestamp;
    DMP_RCB		*rcb=0;
    DMP_TCB		*tcb;
    i4		tupcnt=0;
    DUF_CODEMAP		tuple;
    char		code_key[4];
    char		own_buf[DB_OWN_MAXNAME+1];
    DB_STATUS		ret_stat;
    bool		found = FALSE;
    i4		error,err_code,locerr;
    DB_ERROR		local_dberr;

    /* first blank pad the name */
    MEfill(DB_OWN_MAXNAME, ' ', (PTR) name->db_own_name);
    
    if (logdb_cb->flags & NOCODEMAP)
    {
	/* just set owner name to "UNKNOWN" */
	STcopy ("UNKNOWN", own_buf);
	MEcopy ( (PTR) own_buf, STlength(own_buf), (PTR) name->db_own_name);
	return;
    }

    /* if we get here, we expect to have table iitemp_codemap to look up ucode
    ** and translate to uname */

    /* control loop */
    for (;;)
    {
        /* now open iitemp_codemap and try to map the usercode to a username */
    
	error = E_DM0000_OK;
	table_id.db_tab_base = DM_B_CODEMAP_TAB_ID;
	table_id.db_tab_index = DM_I_CODEMAP_TAB_ID;

	ret_stat = dm2t_open(dcb, &table_id, DM2T_X,  DM2T_UDIRECT, DM2T_A_READ,
			    (i4)0, (i4)20, (i4)0, 
			    xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0, 
			    (i4)0, 0, &xcb->xcb_tran_id, 
			    &timestamp, &rcb, (DML_SCB *)0, &local_dberr);
	if (ret_stat != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, NULL, ULE_LOG, (DB_SQLSTATE *) NULL, 
		NULL, 0, NULL, &err_code, 0);
	    uleFormat (NULL, E_DM9199_BADOPEN_TEMPCODEMAP, 0, ULE_LOG,
		(DB_SQLSTATE *) NULL, NULL, 0, NULL, &locerr, 0);
	    break;
	}

	/* build a key from pathname read from admin30 file */
	tcb = rcb->rcb_tcb_ptr;

	adf_cb = rcb->rcb_adf_cb;

	from_dv.db_length = UCODE_LEN;
	from_dv.db_datatype = (DB_DT_ID) DB_CHA_TYPE;
	from_dv.db_data = (PTR) code;
	from_dv.db_prec = 0;
	from_dv.db_collID = -1;

	to_dv.db_length = tcb->tcb_atts_ptr[DM_1_CODEMAP_KEY].length;
	to_dv.db_datatype = 
			(DB_DT_ID) tcb->tcb_atts_ptr[DM_1_CODEMAP_KEY].type;
	to_dv.db_data = (PTR) code_key;
	to_dv.db_prec = 0;
	to_dv.db_collID = tcb->tcb_atts_ptr[DM_1_CODEMAP_KEY].collID;

	/* convert string read from admin30 file into format used in
	** iitemp_codemap file */
	ret_stat = adc_cvinto( adf_cb, &from_dv, &to_dv);
	if (ret_stat != E_DB_OK)
	{
	    /* generate error message that name could not be cohersed into 
	    ** table, then return with nonzero status to DIlistxxx().
	    */
	    if ( adf_cb->adf_errcb.ad_emsglen)
	    {
		uleFormat(&adf_cb->adf_errcb.ad_dberror, 0, NULL, ULE_MESSAGE, 
			   (DB_SQLSTATE *) NULL,  adf_cb->adf_errcb.ad_errmsgp, 
			   adf_cb->adf_errcb.ad_emsglen, NULL, &err_code, 0);
		uleFormat(NULL, E_DM919A_BADREAD_TEMPCODEMAP, NULL, ULE_LOG,
		    (DB_SQLSTATE *) NULL, NULL, 0, NULL, &err_code, 0);
	    }
	    else
	    if  (adf_cb->adf_errcb.ad_errcode)
	    {
		error = adf_cb->adf_errcb.ad_errcode;
		uleFormat(&adf_cb->adf_errcb.ad_dberror, 0, NULL, ULE_LOG, 
		    (DB_SQLSTATE *) NULL, NULL, 0, NULL, &err_code, 0);
	    }
	    ret_stat = E_DB_ERROR;
	    break;
	}
	
	key_desc.attr_operator = DM2R_EQ;
	key_desc.attr_number = DM_1_CODEMAP_KEY;
	key_desc.attr_value = code_key;	    	    

	ret_stat = dm2r_position(rcb, DM2R_QUAL, &key_desc, (i4)1,
			       (DM_TID *)0, &local_dberr);
	error = local_dberr.err_code;

	if (ret_stat != E_DB_OK)
	    break;      
	for (;;)
	{
	    DM_TID	tid;
	    /* loop to get tuples.  We want exactly 1 tuple or
	    ** we should report a user error */
	    ret_stat = dm2r_get(rcb, &tid, DM2R_GETNEXT, 
		(char *)&tuple, &local_dberr);
	    error = local_dberr.err_code;

	    if (ret_stat != E_DB_OK && error != E_DM0055_NONEXT)
		break;
	    else if (error == E_DM0055_NONEXT)
	    {
		ret_stat = E_DB_OK;
		error = E_DM0000_OK;
		break;
	    }
	    if (++tupcnt == 1)
	    {
	    	found = TRUE;
		MEcopy ((PTR)tuple.uname, UNAME_LEN, (PTR)name->db_own_name);
	    }
	}
	if (ret_stat != E_DB_OK)
	    break;
	if (tupcnt == 0)
	{
	    /* cannot find entry in iitemp_locations to map to this area */
	    ret_stat = dmm_message (&error, W_DM5406_CANT_TRANSLATE_UCODE, 1,
				    sizeof(code), code);
	    if (ret_stat != E_DB_OK)
		break;
	}
	else if (tupcnt > 1)
	{
	    /* too many entries in iitemp_locations map to this area. We
	    ** dont know which one to use, so cannot determine loc name */
	    ret_stat = dmm_message (&error, W_DM5406_CANT_TRANSLATE_UCODE, 1,
				    sizeof(code), code);
	    if (ret_stat != E_DB_OK)
		break;
	}
	break;	/* exit control loop */

    }  /* end of control loop */

    if (ret_stat != E_DB_OK)
    {
	ret_stat = dmm_message (&error, W_DM5406_CANT_TRANSLATE_UCODE, 1,
				    sizeof(code), code);
    }
    
    /* close iitemp_codemap table if it is open */
    if (rcb)
    {

	MEcopy( (PTR)tcb->tcb_rel.relid.db_tab_name, DB_TAB_MAXNAME, 
		(PTR)logdb_cb->filename);
	logdb_cb->filename[DB_TAB_MAXNAME] = '\0'; /* null terminate */

	ret_stat = dm2t_close(rcb, DM2T_NOPURGE, &local_dberr);
	rcb = (DMP_RCB *) NULL;
	if (ret_stat != E_DB_OK)
	{
	    logdb_cb->list_stat = E_DM5410_CLOSE_ERROR;
	    logdb_cb->flags |= STOP;
	}
	else
	    logdb_cb->filename[0] = '\0';   /* clear this error parameter since
					    ** there was not an error */
    }	
}

/*{
** Name: log_db - Log a database when you find one
**
** Description:
**      This routine determines whether or not a config file exists.  If one
**	does not exist, then it looks for an oldstype (v5) admin30 or 
**	oldadmbine30 file.  If either of those exist, it reports a v5 database.
**	If the config file exists, it reads pertainent information from it
**	and puts that information into the iiphys_database and iiphys_extend
**	catalogs.
**
** Inputs:
**	logdb_cb		Ptr to Log_db Control Block
**	    loc_path		    pathname that is being searched.
**	    loc_name		    name of area that is being searched
**	dirname			name of directory found by DIlistdir
**	dirlen			number of characters in dirname
**	error			pointer to operating system error codes 
**
** Outputs:
**	logdb_cb		Log_db Control Block
**	    list_stat		    status that log_db returns
**					E_DM0000_OK
**					E_DM5410_CLOSE_ERROR
**				        NOTE: if list_stat != E_DM0000_OK, then
**					    it will be an error message constant
**					    and that constant will require the
**					    following 3 parameters found in
**					    the logdb_cb:   dir_name, loc_name
**					    and loc_path.  It may optionally
**					    require parameter filename.
**	    dir_name		    name of directory that log_db is currently
**					processing (null terminated)
**	    filename		    extra parameter for error messages that
**					may be returned by some subordiate
**					routines.
** Returns:
**	OK
**	FAIL	- the only thing that can cause a fail to be returned is if
**		  there is an error encountered attempting to close an open
**		  file.  That is a fatal error that will shut down the server.
**		  All other errors are logged and possibly reported to the
**		  FE and then treated as though no error occurred.
**
** History:
**	28-dec-90 (teresa)
**	    initial creation
*/
static STATUS
log_db(
LOGDB_CB	*logdb_cb,
char		*dirname,
i4		dirlen,
CL_ERR_DESC	*error)
{
#define		UNKNOWN_DB_TYPE	    0	    /* type know known */
#define		REGULAR_DB_TYPE	    1	    /* config file found */
#define		RECOVERY_DB_TYPE    2	    /* recovery config file found */
#define		UNCONVERTED_DB_TYPE 3	    /* admin30 file found */
#define		PARTIALY_CONVERTED  4	    /* oldadmin30 file found */
#define		ERROR_ENCOUNTERED   5	    /* unantisipated DIopen error */

    i4		    db_type;		    /* Type of directory we have found,
					    ** which is one of:
					    **	    UNKNOWN_DB_TYPE,
					    **	    REGULAR_DB_TYPE,
					    **	    RECOVERY_DB_TYPE,
					    **	    UNCONVERTED_DB_TYPE,
					    **	    PARTIALY_CONVERTED,
					    **	    ERROR_ENCOUNTERED	*/
    /* assorted error status words. */
    STATUS	    ret_stat;		    /* return status for this routine */
    STATUS	    status=OK;		    /* status returned from DIopen() */
    CL_ERR_DESC	    open_err;		    /* error info returned from DIopen*/
    DB_STATUS	    stat=E_DB_OK;	    /* return status from routine
					    ** read_config() or read_admin() */
    i4	    err_code,locerr;	    /* place for uleformat to return
					    ** an error if necessary. */
    /* required for LO calls to build pathname */
    char            loc_buffer[DB_AREA_MAX+1]; /* buffer to hold a pathname */
    char            *stringptr;		    /* ptr that LO CL uses to translate
					    ** locations to strings */
    char            db_buffer[DB_MAXNAME+1]; /* work directory for LO call.
					    ** contains name of db directory */
    char            db_dir[DB_AREA_MAX+1]; /* buffer to hold full pathname to 
					    ** desired directory */
    u_i4	    db_len;		    /* number of characters in db_dir */
    LOCATION	    location;		    /* location used to build pathname*/
    /* required for DIlistdir() */
    char	    config_name[DB_MAXNAME+1];  /* name of config or admin30
					    ** that we are trying to open */
    u_i4	    config_len;		    /* number of characters of data
					    ** to use from buffer config_name */
    DI_IO	    di_io;		    /* DI CL IO control block - used to
					    ** open files */
    
    /*
    **  copy the directory name to logdb_cb incase something goes wrong --
    **	atleast we wil be able to determine what directory it went wrong on.
    */
    MEfill(DB_MAXNAME, ' ', logdb_cb->dir_name);
    MEcopy((PTR)dirname, dirlen, (PTR)logdb_cb->dir_name);
    logdb_cb->dir_name[dirlen]='\0';	/* null terminate */
    CVlower(logdb_cb->dir_name);	/* force to lower case ?? */

    STcopy (logdb_cb->dir_name, db_buffer);
    STcopy (logdb_cb->loc_path, loc_buffer);
    LOingpath(loc_buffer, db_buffer, LO_DB, &location);
    LOtos(&location, &stringptr);
    STcopy(stringptr, db_dir);
    db_len = STlength(stringptr);

    /* now determine which type of directory this is */
    db_type = UNKNOWN_DB_TYPE;
    for (;;)
    {
	/* first attempt to open the config file */
	STcopy( CONFIG_FILE, config_name);
	config_len = L_CONFIG_FILE;
	status = DIopen(&di_io, db_dir, db_len, config_name, config_len, 
			(i4)DM_PG_SIZE, DI_IO_READ, 0, &open_err);
	if (status == OK)
	{
	    /* config file was found -- the usual case */
	    db_type = REGULAR_DB_TYPE;
	    break;
	}
	else if (status != DI_FILENOTFOUND)
	{
	    db_type = ERROR_ENCOUNTERED;
	    break;
	}

	/* config file does not exist, search for recovery config file */
	STcopy ( RECOVERY_CONFIG_FILE, config_name);
	config_len = L_RECOVERY_CONFIG_FILE;
	status = DIopen(&di_io, db_dir, db_len, config_name, config_len,
		        (i4)DM_PG_SIZE, DI_IO_READ, 0, &open_err);
	if (status == OK)
	{
	    /* recovery config file was found, this is still v6 database */
	    db_type = RECOVERY_DB_TYPE;
	    break;
	}
	else if (status != DI_FILENOTFOUND)
	{
	    db_type = ERROR_ENCOUNTERED;
	    break;
	}

	/* recover config file does not exist either, so see if its a v5 db */
	STcopy ( ADMIN30_FILE, config_name);
	config_len = L_ADMIN30_FILE;
	status = DIopen(&di_io, db_dir, db_len, config_name, config_len,
			(i4)DM_PG_SIZE, DI_IO_READ, 0, &open_err);
	if (status == OK)
	{
	    /* This is an unconverted v5.0 database */
	    db_type = UNCONVERTED_DB_TYPE;
	    break;
	}
	else if (status != DI_FILENOTFOUND)
	{
	    db_type = ERROR_ENCOUNTERED;
	    break;
	}
	/* last try -- search for partially converted db.  If the oldadmin30
	** file does not exist, then this is not a database at all, but some
	** other type of directory. 
	*/
	STcopy (OLDADMIN30_FILE, config_name);
	config_len = L_OLDADMIN30_FILE;
	status = DIopen(&di_io, db_dir, db_len, config_name, config_len,
		        (i4)DM_PG_SIZE, DI_IO_READ, 0, &open_err);
	if (status == OK)
	{
	    /* this is partially converted v5 database */
	    db_type = PARTIALY_CONVERTED;
	    break;
	}
	else if (status != DI_FILENOTFOUND)
	{
	    db_type = ERROR_ENCOUNTERED;
	    break;
	}
	break;
    }

    /* if type is UNKNOWNDB_TYPE then this is not a DB and no file is open.
    **  just return a success status and pretend we have never seen this 
    **	directory.
    ** if type is REGULAR_DB_TYPE or RECOVERY_DB_TYPE, then a config file is
    **	open, so read anything interesting from it.
    ** if type is UNCONVERTED_DB_TYPE or PARTIALY_CONVERTED, then an admin30
    **	format file is open, and database has not yet been converted to v6.
    ** if type is ERROR_ENCOUNTERED, then we have an error which we should
    **  report and then continue to the next database. No DI file is open 
    **	because the open failed.
    */
    switch (db_type)
    {
    case UNKNOWN_DB_TYPE:
	ret_stat = OK;
	break;
    case ERROR_ENCOUNTERED:
	/* print message to FE that we could not handle this db, but do not
	** generate error, or else processing will not continue on to other
	** locations */
	ret_stat = dmm_message(&err_code, W_DM5400_DIOPEN_ERR, 2,	
	    config_len, config_name, db_len, db_dir);
	ret_stat = OK;
	break;
    case UNCONVERTED_DB_TYPE:
    case PARTIALY_CONVERTED:
	/* this is a version 5 database, so call routine read_admin to read
	** the version 5 admin30 file and put the data into the appropriate
	** iidbdb catalogs */
	/* installation knows about unconverted v5 dbs, so process it */
	stat = read_admin(&di_io, logdb_cb, &err_code);
	if (DB_SUCCESS_MACRO(stat))
	{
	    ret_stat = OK;
	}
	else
	{
	    /* output the error message to errlog.log that read_admin found.
	    ** return a success status, as we wish to continue to the next
	    ** database */
	    uleFormat(NULL, err_code, NULL, ULE_LOG, (DB_SQLSTATE *) NULL, 
		NULL, 0, NULL,
		&locerr, 2, db_len, db_dir, 0, logdb_cb->loc_name);
	    if (logdb_cb->flags & STOP)
	    {
		ret_stat = FAIL;
	    }
	    else
	    {
		ret_stat = OK;
		err_code = E_DM0000_OK;
	    }
	}
	status = DIclose (&di_io, &open_err);
	if (status != OK)
	{
	    logdb_cb->list_stat = E_DM5410_CLOSE_ERROR;
	    MEcopy( (PTR)config_name, config_len, (PTR)logdb_cb->filename);
	    logdb_cb->filename[config_len] = '\0'; /* null terminate */
	    ret_stat = FAIL;
	}
	break;
    case REGULAR_DB_TYPE:
    case RECOVERY_DB_TYPE:
	/* this is a version 6 database, so call routine read_config
	** to read the config file and put the data into the appropriate
	** iidbdb catalogs */
	stat = read_config(&di_io, logdb_cb, config_name, config_len,&err_code);
	if (DB_SUCCESS_MACRO(stat))
	{
	    ret_stat = OK;
	}
	else
	{
	    /* output the error message to errlog.log that read_config found.
	    ** return a success status, as we wish to continue to the next
	    ** database */
	    uleFormat(NULL, err_code, 0, ULE_LOG, (DB_SQLSTATE *) NULL,
		NULL, 0, NULL,
		&locerr, 2, db_len, db_dir, 0, logdb_cb->loc_name);
	    if (logdb_cb->flags & STOP)
		ret_stat = FAIL;
	    else
		ret_stat = OK;
	    err_code = E_DM0000_OK;
	}
	status = DIclose (&di_io, &open_err);
	if (status != OK)
	{
	    logdb_cb->list_stat = E_DM5410_CLOSE_ERROR;
	    MEcopy( (PTR)config_name, config_len, (PTR)logdb_cb->filename);
	    logdb_cb->filename[config_len] = '\0'; /* null terminate */
	    ret_stat = FAIL;
	}
	break;
    default:
	/* no other case should be possible.  If we get one, act as though
	** we have found a directory that is NOT a database */
	ret_stat = OK;
    };
    return (ret_stat);
}        

/*{
** Name: open_dbtbl - Open table iiphys_database and format an empty tuple
**
** Description:
**	
**	This routine open the finddbs work table iiphys_database, which is a 
**	work table for tuples found during finddbs execution.  This table has 
**	the identical format as iidatabase, and tuples from this table will 
**	eventually be copied to iidatabase.  This table will be populated from
**	entries in config files (and for unconverted v5 databases, entries from
**	admin30 files).  This routine allocates memory and formats an "empty"
**	iiphys_database tuple and also allocates workspace (ie, memory) to build
**	a tuple.  It also allocates memory to hold an array of DB_DATA_VALUES to
**	describe each attribute in the table.
**
**	The "empty" tuple will be initialized via ADF to represent an empty
**	attribute for each attribute in the table.  The work tuple is workspace
**	where an actual tuple will be built to be inserted into iiphys_database.
**
**	The empty tuple will be copied to the work tuple and then individual
**	attributes will be obtained from the config file and formatted by ADF
**	and put into the work tuple.  Once the work tuple is populated, it will
**	be inserted into table iiphys_database.  However, the work to populate
**	the work tuple and insert it into the catalog are NOT done in this
**	routine.  The logic is discussed here so that you can see why this
**	routine allocates both an empty and a work tuple.
**
**	Routine open_dbtbl also allocates memory for an array of DataValues to
**	describe the internal representation of an iiphys_database tuple.  This
**	DV array is initialized to represent fields in the logdb_cb.iidatabase
**	tuple.  This DV array will be used by db_insert to convert the internal
**	representation of a tuple into the format that the table stores it in.
**
** Inputs:
**	dcb		    Ptr to Open Database Control Block
**	logdb_cb	    log_db() control block.  Contains:
**	    xcb			Ptr to Transaction Control Block
**
** Outputs:
**	logdb_cb	    Log_db() control block.
**	    db_mem		ptr to allocated memory to hold db_tup
**	    db_tup		workspace for iiphys_database tuple
**	    db_e_mem		ptr to allocated memory to hold db_etup
**	    db_etup		initialized iiphys_database "empty" tuple
**	    db_d_mem		Ptr to allocated memory to hold db_datavals
**	    db_datavals		ptr to array of initialized Datavalues for
**				 each attribute in iiphys_database
**	    rcb			Ptr to Open Table Record Control Block
**	error		    Error Code:		E_DM0000_OK
**						E_DM9191_BADOPEN_IIPHYSDB
**						E_DM9193_BAD_PHYSDB_TUPLE
**						E_DM9188_DMM_ALLOC_ERROR
** Returns:
**	E_DB_OK
**	E_DB_FAIL
**
** History:
**	03-jan-91 (teresa)
**	    initial creation
**	06-feb-92 (ananth)
**	    Use the ADF_CB in the RCB. There isn't a pointer to it in
**	    LOGDB_CB anymore.
*/
static DB_STATUS
open_dbtbl(
DMP_DCB			*dcb,
LOGDB_CB		*logdb_cb,
DMM_CB			*dmm)
{
	DML_XCB		    *xcb=logdb_cb->xcb;
	ADF_CB		    *adf_cb;
	DU_DATABASE	    *iidatabase= &logdb_cb->iidatabase;
	DMP_TCB		    *tcb;		/* ptr to Table control block */
	DMP_RCB		    *r;
	DB_TAB_ID	    table_id;		/* table id of table to open */
	DB_STATUS	    locstat;		/* local status */
	DB_TAB_TIMESTAMP    timestamp;
	char		    *tuple;		/* local ptr to work tuple */
	char		    *etuple;		/* local ptr to empty tuple */
	DMP_MISC	    *alloc_ptr;		/* used to allocate memory */
	DB_TRAN_ID	    tran_id;
	i4		    *err_code = &dmm->error.err_code;
	i4		    i;			/* a counter */
	DB_DATA_VALUE	    *dv;		/* ptr to datavalue to describe
						** format of this attribute.
						*/
	i4		    size;		/* number of bytes to allocate
						** for array of data values */

    for (;;) /* error control loop */
    {
	/* first open the table */
	table_id.db_tab_base = DM_B_PHYS_DB_TAB_ID;
	table_id.db_tab_index = DM_I_PHYS_DB_TAB_ID;
	tran_id.db_high_tran = 0;
        tran_id.db_low_tran = 0;

	locstat = dm2t_open(dcb, &table_id, DM2T_X,  DM2T_UDIRECT, DM2T_A_WRITE,
			    (i4)0, (i4)20, (i4)0, 
			    xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0, 
			    (i4)0, 0, &xcb->xcb_tran_id, 
			    &timestamp, &r, (DML_SCB *)0, &dmm->error);

	logdb_cb->db_rcb = r;
	adf_cb = r->rcb_adf_cb;

	if (locstat != E_DB_OK)
	{
	    uleFormat(&dmm->error, 0, NULL, ULE_LOG, (DB_SQLSTATE *) NULL,
		NULL, 0, NULL, err_code, 0);
	    SETDBERR(&dmm->error, 0, E_DM9191_BADOPEN_IIPHYSDB);
	    break;
	}

	/* now construct an empty tuple and DB_DATA_VALUES for each attribute */
	tcb = r -> rcb_tcb_ptr;

	    /* allocate memory for the work tuple */
	locstat = dm0m_allocate( sizeof(DMP_MISC) + 
				(i4)tcb->tcb_rel.relwid, DM0M_ZERO, 
				(i4) MISC_CB, (i4) MISC_ASCII_ID,
				(char *) 0, (DM_OBJECT **) &alloc_ptr, &dmm->error);
	if (locstat != E_DB_OK)
	{
	    uleFormat(&dmm->error, 0, NULL, ULE_LOG, (DB_SQLSTATE *) NULL,
		NULL, 0, NULL, err_code, 0);
	    SETDBERR(&dmm->error, 0, E_DM9188_DMM_ALLOC_ERROR);
	    break;
	}
	alloc_ptr->misc_data = (char *)&alloc_ptr[1];
	tuple = (char *) alloc_ptr->misc_data;
	logdb_cb->db_tup = (char *) alloc_ptr->misc_data;
	logdb_cb->db_mem = alloc_ptr;
	alloc_ptr = 0;

	    /* allocate memory for the empty tuple */
	locstat = dm0m_allocate( sizeof(DMP_MISC) + 
				(i4)tcb->tcb_rel.relwid, DM0M_ZERO, 
				(i4) MISC_CB, (i4) MISC_ASCII_ID,
				(char *) 0, (DM_OBJECT **) &alloc_ptr, &dmm->error);
	if (locstat != E_DB_OK)
	{
	    uleFormat(&dmm->error, 0, NULL, ULE_LOG, (DB_SQLSTATE *) NULL,
		NULL, 0, NULL, err_code, 0);
	    SETDBERR(&dmm->error, 0, E_DM9188_DMM_ALLOC_ERROR);
	    break;
	}
	alloc_ptr->misc_data = (char *)&alloc_ptr[1];
	etuple = (char *) alloc_ptr->misc_data;
	logdb_cb->db_etup = (char *) alloc_ptr->misc_data;
	logdb_cb->db_e_mem = alloc_ptr;
	alloc_ptr = 0;

	/* 
	** Allocate memory for the array of DB_DATA_VALUES to describe
	** each attribute 
	*/

	size= sizeof(DMP_MISC) + (tcb->tcb_rel.relatts * sizeof(DB_DATA_VALUE));
	locstat = dm0m_allocate( size, DM0M_ZERO, 
				(i4) MISC_CB, (i4) MISC_ASCII_ID,
				(char *) 0, (DM_OBJECT **) &alloc_ptr, &dmm->error);
	if (locstat != E_DB_OK)
	{
	    uleFormat(&dmm->error, 0, NULL, ULE_LOG, (DB_SQLSTATE *) NULL,
		NULL, 0, NULL, err_code, 0);
	    SETDBERR(&dmm->error, 0, E_DM9188_DMM_ALLOC_ERROR);
	    break;
	}
	alloc_ptr->misc_data = (char *) &alloc_ptr[1];
	dv = (DB_DATA_VALUE *) alloc_ptr->misc_data;
	logdb_cb->db_datavals = (DB_DATA_VALUE *) alloc_ptr->misc_data;
	logdb_cb->db_d_mem = alloc_ptr;
	alloc_ptr = 0;

	/* 
	**  initialize empty tuple and build the list of DB_DATA_VALUES to
	**  describe attributes in the work tuple.
	*/

	for (i=1; i<= tcb->tcb_rel.relatts; i++)
	{

	    /* build empty entry for this attribute */
	    dv[i-1].db_length = tcb->tcb_atts_ptr[i].length;
	    dv[i-1].db_datatype = (DB_DT_ID) tcb->tcb_atts_ptr[i].type;
	    dv[i-1].db_data = (PTR) &etuple[tcb->tcb_atts_ptr[i].offset];
	    dv[i-1].db_prec = 0;
	    dv[i-1].db_collID = tcb->tcb_atts_ptr[i].collID;
	    locstat = adc_getempty( adf_cb, dv);
	    if (locstat != E_DB_OK)
	    {
		/* generate error message that name could not be cohersed into 
		** table
		*/
		if ( adf_cb->adf_errcb.ad_emsglen)
		{
		    uleFormat(&adf_cb->adf_errcb.ad_dberror, 0, NULL, ULE_MESSAGE,
			       (DB_SQLSTATE *) NULL,
			       adf_cb->adf_errcb.ad_errmsgp, 
			       adf_cb->adf_errcb.ad_emsglen, NULL, err_code, 0);
		    SETDBERR(&dmm->error, 0, E_DM9193_BAD_PHYSDB_TUPLE);
		}
		else
		if	(adf_cb->adf_errcb.ad_errcode)
		{
		    dmm->error = adf_cb->adf_errcb.ad_dberror;
		}
		return (E_DB_ERROR);
	    } /* end if error encountered obtaining empty attribute from ADF */

	    /*
	    ** If we get here, we have successfully built the empty attribute
	    ** via ADF.  Now reassign the DB_DATA_VALUE.db_data to point
	    ** to the work tuple offset instead of to the empty tuple offset 
	    */
	    dv[i-1].db_data = (PTR) &tuple[tcb->tcb_atts_ptr[i].offset];

	} /* end loop for each attribute in table */

	/* Now build the dvs for the internal representation of the
	** iiphys_database tuple
	*/

	size= sizeof(DMP_MISC) + 
	      (tcb->tcb_rel.relatts * sizeof(DB_DATA_VALUE));
	locstat = dm0m_allocate( size, DM0M_ZERO, (i4) MISC_CB, 
				(i4) MISC_ASCII_ID, (char *) 0, 
				(DM_OBJECT **) &alloc_ptr, &dmm->error);
	if (locstat != E_DB_OK)
	{
	    uleFormat(&dmm->error, 0, NULL, ULE_LOG, (DB_SQLSTATE *) NULL,
		NULL, 0, NULL, err_code, 0);
	    SETDBERR(&dmm->error, 0, E_DM9188_DMM_ALLOC_ERROR);
	    break;
	}
	alloc_ptr->misc_data = (char *) &alloc_ptr[1];
	logdb_cb->insdb_dvs = (DB_DATA_VALUE *) alloc_ptr->misc_data;
	logdb_cb->dbdvs_mem = alloc_ptr;
	alloc_ptr=0;

	/* initialize the DVs for the iiphys_database catalog
	*/
	dv = (DB_DATA_VALUE *) logdb_cb->insdb_dvs;
	/* NAME */
	dv[NAME].db_length = DB_DB_MAXNAME;
	dv[NAME].db_datatype = (DB_DT_ID) DB_CHA_TYPE;
	dv[NAME].db_data = &iidatabase->du_dbname[0];
	dv[NAME].db_prec = 0;
	dv[NAME].db_collID = -1;
	/* OWN */
	dv[OWN].db_length = DB_OWN_MAXNAME;
	dv[OWN].db_datatype = (DB_DT_ID) DB_CHA_TYPE;
	dv[OWN].db_data = (PTR) &iidatabase->du_own;
	dv[OWN].db_prec = 0;
	dv[OWN].db_collID = -1;
	/* DBDEV */
	dv[DBDEV].db_length = DB_LOC_MAXNAME;
	dv[DBDEV].db_datatype = (DB_DT_ID) DB_CHA_TYPE;
	dv[DBDEV].db_data = (PTR) &iidatabase->du_dbloc;
	dv[DBDEV].db_prec = 0;
	dv[DBDEV].db_collID = -1;
	/* CKPDEV */
	dv[CKPDEV].db_length = DB_LOC_MAXNAME;
	dv[CKPDEV].db_datatype = (DB_DT_ID) DB_CHA_TYPE;
	dv[CKPDEV].db_data = (PTR) &iidatabase->du_ckploc;
	dv[CKPDEV].db_prec = 0;
	dv[CKPDEV].db_collID = -1;
	/* JNLDEV */
	dv[JNLDEV].db_length = DB_LOC_MAXNAME;
	dv[JNLDEV].db_datatype = (DB_DT_ID) DB_CHA_TYPE;
	dv[JNLDEV].db_data = (PTR) &iidatabase->du_jnlloc;
	dv[JNLDEV].db_prec = 0;
	dv[JNLDEV].db_collID = -1;
	/* SORTDEV */
	dv[SORTDEV].db_length = DB_LOC_MAXNAME;
	dv[SORTDEV].db_datatype = (DB_DT_ID) DB_CHA_TYPE;
	dv[SORTDEV].db_data = (PTR) &iidatabase->du_workloc;
	dv[SORTDEV].db_prec = 0;
	dv[SORTDEV].db_collID = -1;
	/* ACCESS */
	dv[ACCESS].db_length = sizeof(iidatabase->du_access);
	dv[ACCESS].db_datatype = (DB_DT_ID) DB_INT_TYPE;
	dv[ACCESS].db_data = (PTR) &iidatabase->du_access;
	dv[ACCESS].db_prec = 0;
	dv[ACCESS].db_collID = -1;
	/* DBSERVICE */
	dv[DBSERVICE].db_length = sizeof(iidatabase->du_dbservice);
	dv[DBSERVICE].db_datatype = (DB_DT_ID) DB_INT_TYPE;
	dv[DBSERVICE].db_data = (PTR) &iidatabase->du_dbservice;
	dv[DBSERVICE].db_prec = 0;
	dv[DBSERVICE].db_collID = -1;
	/* DBCMPTLVL */
	dv[DBCMPTLVL].db_length = sizeof(iidatabase->du_dbcmptlvl);
	dv[DBCMPTLVL].db_datatype = (DB_DT_ID) DB_INT_TYPE;
	dv[DBCMPTLVL].db_data = (PTR) &iidatabase->du_dbcmptlvl;
	dv[DBCMPTLVL].db_prec = 0;
	dv[DBCMPTLVL].db_collID = -1;
	/* DBCMPTMINOR */
	dv[DBCMPTMINOR].db_length = sizeof(iidatabase->du_1dbcmptminor);
	dv[DBCMPTMINOR].db_datatype = (DB_DT_ID) DB_INT_TYPE;
	dv[DBCMPTMINOR].db_data = (PTR) &iidatabase->du_1dbcmptminor;
	dv[DBCMPTMINOR].db_prec = 0;
	dv[DBCMPTMINOR].db_collID = -1;
	/* DB_ID */
	dv[DB_ID].db_length = sizeof(iidatabase->du_dbid);
	dv[DB_ID].db_datatype = (DB_DT_ID) DB_INT_TYPE;
	dv[DB_ID].db_data = (PTR) &iidatabase->du_dbid;
	dv[DB_ID].db_prec = 0;
	dv[DB_ID].db_collID = -1;
	/* DMPDEV */
	dv[DMPDEV].db_length = DB_LOC_MAXNAME;
	dv[DMPDEV].db_datatype = (DB_DT_ID) DB_CHA_TYPE;
	dv[DMPDEV].db_data = (PTR) &iidatabase->du_dmploc;
	dv[DMPDEV].db_prec = 0;
	dv[DMPDEV].db_collID = -1;

	break;
    }
    return locstat;    
}

/*{
** Name: open_exttbl - Open table iiphys_extend and format an empty tuple
**
** Description:
**	
**	This routine open the finddbs work table iiphys_extend, which is a work
**	table for tuples found during finddbs execution.  This table has the
**	identical format as iiextend, and tuples from this table will 
**	eventually be copied to iiextend.  This table will be populated from
**	entries in config files.  This routine allocates memory and formats an 
**	"empty" iiphys_extend tuple and also allocates workspace (ie, memory) 
**	to build a tuple.  It also allocates memory to hold an array of 
**	DB_DATA_VALUES to describe each attribute in the table.
**
**	The "empty" tuple will be initialized via ADF to represent an empty
**	attribute for each attribute in the table.  The work tuple is workspace
**	where an actual tuple will be built to be inserted into iiphys_extend
**
**	The empty tuple will be copied to the work tuple and then individual
**	attributes will be obtained from the config file and formatted by ADF
**	and put into the work tuple.  Once the work tuple is populated, it will
**	be inserted into table iiphys_extend.  However, the work to populate
**	the work tuple and insert it into the catalog are NOT done in this
**	routine.  The logic is discussed here so that you can see why this
**	routine allocates both an empty and a work tuple.
**
**	Routine open_exttbl also allocates memory for an array of DataValues to
**	describe the internal representation of an iiphys_extend tuple.  This
**	DV array is initialized to represent fields in the logdb_cb.iiextend
**	tuple.  This DV array will be used by db_insert to convert the internal
**	representation of a tuple into the format that the table stores it in.
**
** Inputs:
**	dcb		    Ptr to Open Database Control Block
**	logdb_cb	    log_db() control block.  Contains:
**	    xcb			Ptr to Transaction Control Block
**
** Outputs:
**	logdb_cb	    Log_db() control block.
**	    ext_mem		ptr to allocated memory to hold ext_tup
**	    ext_tup		workspace for iiphys_extend tuple
**	    ext_e_mem		ptr to allocated memory to hold ext_etup
**	    ext_etup		initialized iiphys_extend "empty" tuple
**	    ext_d_mem		Ptr to allocated memory to hold ext_datavals
**	    ext_datavals	ptr to array of initialized Datavalues for
**				 each attribute in iiphys_extend
**	    ext_rcb		Ptr to Open Table Record Control Block
**	error		    Error Code:		E_DM0000_OK
**						E_DM9192_BADOPEN_IIPHYSEXT
**						E_DM9194_BAD_PHYSEXT_TUPLE
**						E_DM9188_DMM_ALLOC_ERROR
**
** Inputs:
**	dcb		    Ptr to Open Database Control Block
**	xcb		    Ptr to Transaction Control Block
**
** Outputs:
**	ext_dvs		    ptr to array of initialized Datavalues for
**			    each attribute in iiphys_extend
**	rcb		    Ptr to Open Table Record Control Block
**	ext_etuple	    Ptr to allocated memory for empty tuple
**	ext_tuple	    Ptr to allocated memory for work tuple
**	error		    Error Code:		E_DM0000_OK
** Returns:
**	E_DB_OK
**	E_DB_FAIL
**
** History:
**	03-jan-91 (teresa)
**	    initial creation
**	06-feb-92 (ananth)
**	    Use the ADF_CB in the RCB. There isn't a pointer to it in
**	    LOGDB_CB anymore.
*/
static DB_STATUS
open_exttbl(
DMP_DCB			*dcb,
LOGDB_CB		*logdb_cb,
DMM_CB			*dmm)
{
	DML_XCB		    *xcb=logdb_cb->xcb;
	ADF_CB		    *adf_cb;
	DU_EXTEND	    *iiextend= &logdb_cb->iiextend;
	DMP_TCB		    *tcb;		/* ptr to Table control block */
	DMP_RCB		    *r;			/* record control block */
	DB_TAB_ID	    table_id;		/* table id of table to open */
	DB_STATUS	    locstat;		/* local status */
	DB_TAB_TIMESTAMP    timestamp;
	char		    *tuple;		/* local ptr to work tuple */
	char		    *etuple;		/* local ptr to empty tuple */
	DMP_MISC	    *alloc_ptr;		/* used to allocate memory */
	DB_TRAN_ID	    tran_id;
	i4		    *err_code = &dmm->error.err_code;
	i4		    i;			/* a counter */
	DB_DATA_VALUE	    *dv;		/* ptr to datavalue to describe
						** format of this attribute.
						*/
	i4		    size;		/* number of bytes to allocate
						** for array of data values */

    for (;;) /* error control loop */
    {
	/* first open the table */
	table_id.db_tab_base = DM_B_PHYS_EXTEND_TAB_ID;
	table_id.db_tab_index = DM_I_PHYS_EXTEND_TAB_ID;
	tran_id.db_high_tran = 0;
        tran_id.db_low_tran = 0;

	locstat = dm2t_open(dcb, &table_id, DM2T_X,  DM2T_UDIRECT, DM2T_A_WRITE,
			    (i4)0, (i4)20, (i4)0, 
			    xcb->xcb_log_id, xcb->xcb_lk_id, (i4)0, 
			    (i4)0, 0, &xcb->xcb_tran_id, 
			    &timestamp, &r, (DML_SCB *)0, &dmm->error);

	logdb_cb->ext_rcb = r;
	adf_cb = r->rcb_adf_cb;

	if (locstat != E_DB_OK)
	{
	    uleFormat(&dmm->error, 0, NULL, ULE_LOG, (DB_SQLSTATE *) NULL,
		NULL, 0, NULL, err_code, 0);
	    SETDBERR(&dmm->error, 0, E_DM9192_BADOPEN_IIPHYSEXT);
	    break;
	}	    

	/* now construct an empty tuple and DB_DATA_VALUES for each attribute */
	tcb = r -> rcb_tcb_ptr;

	    /* allocate memory for the work tuple */
	locstat = dm0m_allocate( sizeof(DMP_MISC) + 
				(i4)tcb->tcb_rel.relwid, DM0M_ZERO, 
				(i4) MISC_CB, (i4) MISC_ASCII_ID,
				(char *) 0, (DM_OBJECT **) &alloc_ptr, &dmm->error);
	if (locstat != E_DB_OK)
	{
	    uleFormat(&dmm->error, 0, NULL, ULE_LOG, (DB_SQLSTATE *) NULL,
		NULL, 0, NULL, err_code, 0);
	    SETDBERR(&dmm->error, 0, E_DM9188_DMM_ALLOC_ERROR);
	    break;
	}
	alloc_ptr->misc_data = (char *)&alloc_ptr[1];
	tuple = (char *) alloc_ptr->misc_data;
	logdb_cb->ext_tup = (char *) alloc_ptr->misc_data;
	logdb_cb->ext_mem = alloc_ptr;
	alloc_ptr = 0;

	    /* allocate memory for the empty tuple */
	locstat = dm0m_allocate( sizeof(DMP_MISC) + 
				(i4)tcb->tcb_rel.relwid, DM0M_ZERO, 
				(i4) MISC_CB, (i4) MISC_ASCII_ID,
				(char *) 0, (DM_OBJECT **) &alloc_ptr, &dmm->error);
	if (locstat != E_DB_OK)
	{
	    uleFormat(&dmm->error, 0, NULL, ULE_LOG, (DB_SQLSTATE *) NULL,
		NULL, 0, NULL, err_code, 0);
	    SETDBERR(&dmm->error, 0, E_DM9188_DMM_ALLOC_ERROR);
	    break;
	}
	alloc_ptr->misc_data = (char *)&alloc_ptr[1];
	etuple = (char *) alloc_ptr->misc_data;
	logdb_cb->ext_etup = (char *) alloc_ptr->misc_data;
	logdb_cb->ext_e_mem = alloc_ptr;
	alloc_ptr = 0;

	    /* allocate memory for the array of DB_DATA_VALUES to describe
	    ** each attribute */
	size= sizeof(DMP_MISC) + (tcb->tcb_rel.relatts * sizeof(DB_DATA_VALUE));
	locstat = dm0m_allocate( size, DM0M_ZERO, 
				(i4) MISC_CB, (i4) MISC_ASCII_ID,
				(char *) 0, (DM_OBJECT **) &alloc_ptr, &dmm->error);
	if (locstat != E_DB_OK)
	{
	    uleFormat(&dmm->error, 0, NULL, ULE_LOG, (DB_SQLSTATE *) NULL,
		NULL, 0, NULL, err_code, 0);
	    SETDBERR(&dmm->error, 0, E_DM9188_DMM_ALLOC_ERROR);
	    break;
	}
	alloc_ptr->misc_data = (char *) &alloc_ptr[1];
	dv = (DB_DATA_VALUE *) alloc_ptr->misc_data;
	logdb_cb->ext_datavals = (DB_DATA_VALUE *) alloc_ptr->misc_data;
	logdb_cb->ext_d_mem = alloc_ptr;
	alloc_ptr = 0;

	/* 
	**  initialize empty tuple and build the list of DB_DATA_VALUES to
	**  describe attributes in the work tuple.
	*/

	for (i=1; i<= tcb->tcb_rel.relatts; i++)
	{

	    /* build empty entry for this attribute */
	    dv[i-1].db_length = tcb->tcb_atts_ptr[i].length;
	    dv[i-1].db_datatype = (DB_DT_ID) tcb->tcb_atts_ptr[i].type;
	    dv[i-1].db_data = (PTR) &etuple[tcb->tcb_atts_ptr[i].offset];
	    dv[i-1].db_prec = 0;
	    dv[i-1].db_collID = tcb->tcb_atts_ptr[i].collID;
	    locstat = adc_getempty( adf_cb, dv);
	    if (locstat != E_DB_OK)
	    {
		/* generate error message that name could not be cohersed into 
		** table
		*/
		if ( adf_cb->adf_errcb.ad_emsglen)
		{
		    uleFormat(&adf_cb->adf_errcb.ad_dberror, 0, NULL, ULE_MESSAGE,
			       (DB_SQLSTATE *) NULL, 
			       adf_cb->adf_errcb.ad_errmsgp, 
			       adf_cb->adf_errcb.ad_emsglen, NULL, err_code, 0);
		    SETDBERR(&dmm->error, 0, E_DM9194_BAD_PHYSEXT_TUPLE);
		}
		else
		if	(adf_cb->adf_errcb.ad_errcode)
		{
		    dmm->error = adf_cb->adf_errcb.ad_dberror;
		}
		return (E_DB_ERROR);
	    } /* end if error encountered obtaining empty attribute from ADF */

	    /*
	    ** If we get here, we have successfully built the empty attribute
	    ** via ADF.  Now reassign the DB_DATA_VALUE.db_data to point
	    ** to the work tuple offset instead of to the empty tuple offset 
	    */
	    dv[i-1].db_data = (PTR) &tuple[tcb->tcb_atts_ptr[i].offset];
	} /* end loop for each attribute in table */

	/* Now build the dvs for the internal representation of the
	** iiphys_extend tuple
	*/
	/* memory has not been allocated yet, so allocate memory and 
	** set flag to request initialization */

	size= sizeof(DMP_MISC) + 
	      (tcb->tcb_rel.relatts * sizeof(DB_DATA_VALUE));
	locstat = dm0m_allocate( size, DM0M_ZERO, (i4) MISC_CB, 
				(i4) MISC_ASCII_ID, (char *) 0, 
				(DM_OBJECT **) &alloc_ptr, &dmm->error);
	if (locstat != E_DB_OK)
	{
	    uleFormat(&dmm->error, 0, NULL, ULE_LOG, (DB_SQLSTATE *) NULL,
		NULL, 0, NULL, err_code, 0);
	    SETDBERR(&dmm->error, 0, E_DM9188_DMM_ALLOC_ERROR);
	    break;
	}
	alloc_ptr->misc_data = (char *) &alloc_ptr[1];
	logdb_cb->insext_dvs = (DB_DATA_VALUE *) alloc_ptr->misc_data;
	logdb_cb->exdvs_mem = alloc_ptr;
	alloc_ptr = 0;

	dv = (DB_DATA_VALUE *) logdb_cb->insext_dvs;
	/* lname */
	dv[LNAME].db_length = DB_LOC_MAXNAME;
	dv[LNAME].db_datatype = (DB_DT_ID) DB_CHA_TYPE;
	dv[LNAME].db_data = &iiextend->du_lname[0];
	dv[LNAME].db_prec = 0;
	dv[LNAME].db_collID = -1;
	/* dname */
	dv[DNAME].db_length = DB_DB_MAXNAME;
	dv[DNAME].db_datatype = (DB_DT_ID) DB_CHA_TYPE;
	dv[DNAME].db_data = &iiextend->du_dname[0];
	dv[DNAME].db_prec = 0;
	dv[DNAME].db_collID = -1;
	/* status */
	dv[EXT_STATUS].db_length = sizeof(iiextend->du_status);
	dv[EXT_STATUS].db_datatype = (DB_DT_ID) DB_INT_TYPE;
	dv[EXT_STATUS].db_data = (PTR) &iiextend->du_status;
	dv[EXT_STATUS].db_prec = 0;
	dv[EXT_STATUS].db_collID = -1;

	break;
    }
    return locstat;    
}

/*{
** Name: read_admin	-   get relevent info about unconverted v5 database
**
** Description:
**
**      This routine reads the admin30 (or oldadmin30) file for an unconverted
**	database and takes relevent information from it.  Since some information
**	that is required for iidatabase and iiextend is NOT in the admin30 file,
**	this routine supplies default information.  Note that a v5 database may
**	exist only in a single location, so only 1 iiextend record is required.
**	
**	MAPPINGS ARE AS FOLLOWS:
**
**	iidatabase  db_tbl		source
**	----------  -----------		------
**	name	    du_dbname		logdb_cb.dir_name
**	own	    du_own		FUNCTION(admin30.ah_owner)
**	dbdev	    du_dbloc		logcb_cb.loc_name
**	ckdev	    du_ckloc		FUNCTION(admin30.ckparea)
**	jnldev	    du_jnlloc		FUNCTION(admin30.jnlarea)
**	sortdev	    du_workloc		DEFAULT = ii_work
**	access	    du_access		if logdb_cb.priv_50dbs 
**					    DU_CONVERTING
**					else 
**					    DU_GLOBAL | DU_CONVERTING
**	dbservice   du_dbservice	0
**	dbcmptlvl   du_dbcmptlvl	DU_CUR_DBCMPTLVL
**	dbcmptminor du_1dbcmptminor	DU_1CUR_DBCMPTMINOR
**	db_id	    
**	dmp_dev	    du_dmpdev		DEFAULT = ii_dump
**
**	iiextend    du_extend		source
**	--------    ---------		-------
**	lname	    du_l_length		(adf will format)
**		    du_lname		logcb_cb.loc_name
**	dname	    du_d_length		(adf will format)
**		    du_dname		logdb_cb.dir_name
**	status	    du_status		DU_EXT_OPERATIVE
**
**  Assumptions:
**	this routine assumes that the caller opens the file before calling
**	read_admin and will close it after read_admin returns.
**
** Inputs:
**	di_io			    DI I/O control block (contains information
**					about the open file)
**	logdb_cb		    log_db Control Block - Contains:
**	    db_count			running counter of DBs found
**
** Outputs:
**	logdb_cb		    log_db Control Block - Contains:
**	    db_count			running counter of DBs found
**
**      Returns:
**	    E_DB_OK
**      Exceptions:
**          none
**
** Side Effects:
**          tuples are placed in iiphys_database and iiphys_extend and the
**	    following catalogs are open, read and closed: iitemplocation and
**	    iitempcodemap
**
** History:
**	09-jan-91 (teresa)
**	    initial creation
**	20-sep-1993 (andys)
**	    Fix arguments to ule_format call.
*/
static DB_STATUS
read_admin(
DI_IO	    *di_io,
LOGDB_CB    *logdb_cb,
i4	    *ret_err)
{

    char		buf[DM_PG_SIZE+1];	/* buffer to read page into */
    DU_DATABASE		*iidatabase= &logdb_cb->iidatabase;	/* contains 
						** iiphys_database info */
    DU_EXTEND		*iiextend= &logdb_cb->iiextend;		/* contains 
						** info about 1 loc*/
    DUF_ADMIN_HEADER	*admin_hdr= (DUF_ADMIN_HEADER*) &buf[0];
    i4		readsize;
    i4		error = E_DM0000_OK;
    i4		error2 = E_DM0000_OK;
    CL_ERR_DESC		err_code;
    STATUS		readstat = OK;
    DB_STATUS		ret_stat = E_DB_OK;
    i4		bad_loc=FALSE;		/* boolean */
    DB_STATUS		locstat;
    i4		locerror;

    ret_stat = dmm_message(ret_err, I_DM5421_50DB_FOUND, 2,
			   0, logdb_cb->dir_name, 0, logdb_cb->loc_name);
    if (ret_stat != E_DB_OK)
	return(ret_stat);

    /* read the admin30 header into memory */
    *ret_err = E_DM0000_OK;
    readsize = 1;	/* admin30 header is smaller than 1 page, but we have
			** to read a whole 2048 byte page */
    readstat = DIread(di_io, &readsize, (i4) 0, buf, &err_code);
    if (readstat != OK)
    {
	/* print a warning that we could not read this admin30 file. */
	ret_stat = dmm_message(ret_err, W_DM5403_ADMIN_READ_ERROR, 2,
			      0, logdb_cb->dir_name, 0, logdb_cb->loc_name);
	return (E_DB_OK);
    }

    /* copy relevent information from header and build iidatabase tuple */
	/* name */
    MEfill(DB_DB_MAXNAME, ' ', iidatabase->du_dbname);
    MEcopy(logdb_cb->dir_name, STlength(logdb_cb->dir_name),
	  iidatabase->du_dbname);
	/* own */
    MEfill(DB_OWN_MAXNAME, ' ', iidatabase->du_own.db_own_name);
    getuser(logdb_cb, admin_hdr->ah_owner, &iidatabase->du_own);
	/* dbdev */
    MEfill(DB_LOC_MAXNAME, ' ', iidatabase->du_dbloc.db_loc_name);
    MEcopy(logdb_cb->loc_name, STlength(logdb_cb->loc_name),
	   iidatabase->du_dbloc.db_loc_name);
	/* jnldev */
    MEfill(DB_LOC_MAXNAME, ' ', iidatabase->du_jnlloc.db_loc_name);
    ret_stat = getloc(logdb_cb, DMMFIND_JNL, admin_hdr->ah_jnlarea, 
		     iidatabase->du_jnlloc.db_loc_name, &bad_loc, &error);
    if (ret_stat != E_DB_OK)
    {
	if (error == E_DM5410_CLOSE_ERROR)
	    return (ret_stat);
	else if (! bad_loc)
	    (VOID) uleFormat(NULL, error, NULL, ULE_LOG, (DB_SQLSTATE *) NULL, 
		NULL, 0, NULL, &error2, 0);
	ret_stat = dmm_message (&error, W_DM5404_BAD_JNL_LOC, 2, 
				0, logdb_cb->dir_name, 0, logdb_cb->loc_name);
	return (E_DB_OK);	
    };

	/* ckpdev */
    MEfill(DB_LOC_MAXNAME, ' ', iidatabase->du_ckploc.db_loc_name);
    ret_stat = getloc(logdb_cb, DMMFIND_CKP, admin_hdr->ah_ckparea,
		      iidatabase->du_ckploc.db_loc_name, &bad_loc, &error);
    if (ret_stat != E_DB_OK)
    {
	if (error == E_DM5410_CLOSE_ERROR)
	    return (ret_stat);
	else if (! bad_loc)
	    (VOID) uleFormat(NULL, error, NULL, ULE_LOG, (DB_SQLSTATE *) NULL, 
		NULL, 0, NULL, &error2, 0);
	ret_stat = dmm_message (&error, W_DM5405_BAD_CKP_LOC, 2, 
				0, logdb_cb->dir_name, 0, logdb_cb->loc_name);
	return (E_DB_OK);	
    };
	/* sortdev */
    MEfill(DB_LOC_MAXNAME, ' ', iidatabase->du_workloc.db_loc_name);
    MEcopy (WORK_DEFAULT, LWORK_DEFAULT, iidatabase->du_workloc.db_loc_name);
	/* access */
    iidatabase->du_access = (logdb_cb->flags & PRIV_50DBS) ? DU_CONVERTING : 
						    DU_GLOBAL | DU_CONVERTING;
	/* dbservice */
    iidatabase->du_dbservice = 0;
	/* dbcmptlvl */
    iidatabase->du_dbcmptlvl = DU_CUR_DBCMPTLVL;
	/* dbcmptminor */
    iidatabase->du_1dbcmptminor = DU_1CUR_DBCMPTMINOR;
	/* dbid */
    iidatabase->du_dbid = TMsecs();  /* give database a unique identifier */
	/* dmpdev */
    MEfill(DB_LOC_MAXNAME, ' ', iidatabase->du_dmploc.db_loc_name);
    MEcopy (DMP_DEFAULT, LDMP_DEFAULT, iidatabase->du_dmploc.db_loc_name);

    /* build iiextend tuple for this database */
    MEfill(DB_LOC_MAXNAME, ' ', iiextend->du_lname);
    MEcopy(logdb_cb->loc_name,STlength(logdb_cb->loc_name),iiextend->du_lname);
    MEfill(DB_DB_MAXNAME, ' ', iiextend->du_dname);
    MEcopy(logdb_cb->dir_name,STlength(logdb_cb->dir_name),iiextend->du_dname);
    iiextend->du_status = DU_EXT_OPERATIVE;    

    /* call subroutines  to put iidatabase and iiextend tuples into tables */
    for (;;) /* control loop */
    {
	ret_stat = db_insert (logdb_cb, &error);
	if (ret_stat != E_DB_OK)
	{
	    /* print message to errlog.log explaining what went wrong and
	    ** send message to FE explaining that there was an internal err */
	    uleFormat(NULL, error, NULL, ULE_LOG, (DB_SQLSTATE *) NULL,
			NULL, 0, NULL, &locerror, 0);
	    locstat = dmm_message(&locerror,E_DM5412_IIPHYSDB_ERROR,0);
	    if ( (locstat != E_DB_OK) && (locstat > ret_stat) )
	    {
		ret_stat = locstat;
		error = locerror;
	    }
	    /* indicate to log_cb what the failure was and to stop */
	    logdb_cb->flags |= STOP;
	    logdb_cb->list_stat = E_DM5414_ADMIN30_ERROR;
	    break;
	}
	ret_stat = ext_insert (logdb_cb, &error);
	if (ret_stat != E_DB_OK)
	{
	    /* print message to errlog.log explaining what went wrong and
	    ** send message to FE explaining that there was an internal err */
	    uleFormat(NULL, error, NULL, ULE_LOG, (DB_SQLSTATE *) NULL,
			NULL, 0, NULL, &locerror, 0);
	    locstat = dmm_message(&locerror, E_DM5413_IIPHYSEXT_ERROR, 0);
	    if ( (locstat != E_DB_OK) && (locstat > ret_stat) )
	    {
		ret_stat = locstat;
		error = locerror;
	    }
	    /* indicate to log_cb what the failure was and to stop */
	    logdb_cb->flags |= STOP;
	    logdb_cb->list_stat = E_DM5414_ADMIN30_ERROR;
	    break;
	}

	ret_stat = E_DB_OK;
	/* after we have assured that this is a valid database, increment 
	** datbase found count */
	++logdb_cb->db_count;
	break;
    }
    
    return (ret_stat);
}

/*{
** Name: read_config	-   Obtain relevent information from config file
**
** Description:
**
**      This routine reads the config file and takes relevent information from
**	the header record (DM0C_DSC) and from extent records (DM0C_EXT).
**	The format of the config file has changed from v6.2 to v6.3.  That
**	change is more of a structural change (which order the records are in)
**	than the format of the records.  This routine is able to read either
**	format.  This routine is lifed from DUF code from routine DUFIDB.C.
**
**	The mapping of data in the config file is as follows:
**
**	iidatabase  db_tbl		source
**	----------  -----------		------
**	name	    du_dbname		dsc_name
**	own	    du_own		dsc_owner
**	dbdev	    du_dbloc		logdb_cb.loc_name
**	ckdev	    du_ckloc		APPROPRIATE EXTEND RECORD:
**					 ext_locaton.logical
**	jnldev	    du_jnlloc		APPROPRIATE EXTEND RECORD:
**					 ext_locaton.logical
**	sortdev	    du_workloc		APPROPRIATE EXTEND RECORD:
**					 ext_locaton.logical
**	access	    du_access		FUNCTION:(dsc_dbaccess, dsc_status)
**	dbservice   du_dbservice	dsc_dbservice
**	dbcmptlvl   du_dbcmptlvl	dsc_dbcmptlvl
**	dbcmptminor du_1dbcmptminor	dsc_1dbcmptminor
**	db_id	    du_dbid		dsc_dbid
**	dmp_dev	    du_dmpdev		APPROPRIATE EXTEND RECORD:
**					 ext_locaton.logical
**
**	NOTE: there will be an iiphys_extend record for the default data
**	      location, and 1 for each extended data location in the config
**	      file.
**
**	iiextend    du_extend		source
**	--------    ---------		-------
**	lname	    du_l_length		(adf will format)
**		    du_lname		APPROPRIATE EXTEND RECORD:
**					 ext_locaton.logical
**	dname	    du_d_length		(adf will format)
**		    du_dname		logdb_cb.dir_nam
**	status	    du_status		DU_EXT_OPERATIVE
**
**
**  Assumptions:
**	this routine assumes that the caller opens the file before calling
**	read_admin and will close it after read_admin returns.
**
**  NOTE    PORTIONS OF THIS SHOULD REALLY BE REPLACED WITH A CALL TO DM0C.C
**	    .C ROUTINES TO READ CONFIG FILE.  HOWEVER, THERE ARE PROBLEMS WITH 
**	    THE DM0C.C FILE WHEN THIS WAS WRITTEN.  THE PROBLEMS WERE NOT 
**	    RESOLVED IN A REASONABLE TIMEFRAME, SO THOSE SECTIONS ARE LIFTED 
**	    FROM DUF.
**
** Inputs:
**	di_io			    DI I/O control block (contains information
**					about the open file)
**	logdb_cb		    log_db Control Block - Contains:
**	    db_count			running counter of DBs found
**	filnam			    ASCII name of config/recovery config file.
**	fillen			    number of chars in filnam
**
** Outputs:
**	logdb_cb		    log_db Control Block - Contains:
**	    db_count			running counter of DBs found
**
**      Returns:
**	    E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          tuples are placed in iiphys_database and iiphys_extend.
**
** History:
**	22-jan-91 (teresa)
**	    initial creation
**	13-jan-93 (mikem)
**	    Lint directed changes.  Fixed "=" vs. "==" bug in error handling
**	    in read_config().  Previous to this fix DIsense failures would be
**	    ignored.
*/
static DB_STATUS
read_config(
DI_IO	    *di_io,
LOGDB_CB    *logdb_cb,
char	    *filnam,
i4	    fillen,
i4	    *ret_err)
{
    DB_STATUS	    ret_stat = E_DB_OK;
    DB_STATUS	    loc_stat;
    i4	    locerror;
    CL_ERR_DESC	    sys_err;		/* error code returned from CL */
    STATUS	    di_stat=OK;		/* error status returned from CL */
    i4	    last_pg;		/* number of pages in config file --
					** note: DIsense returns page number of
					** last page and 1st page is #0, so this
					** must be incremented by 1 to get 
					** number of pages */
    i4	    size;		/* min number of bytes of memory to
					** allocate */
    DMP_MISC	    *alloc_ptr=NULL;	/* ptr to memory we have allocated */
    i4	    error;		/* local error code */
    DM0C_CNF	    cnf_buf;		/* what we read config file into */
    DM0C_EXT	    *ext_entry,*next;	/* Pointer to extension entry in
					** the configuration file.
					*/
    DM0C_DSC	    *desc;		/* Pointer to a description entry in
					** the configuration file.
					*/
    DU_DATABASE	    *iidatabase= &logdb_cb->iidatabase;	/* internal 
					** representation of 
					** iiphys_database tuple */
    DU_EXTEND	    *iiextend= &logdb_cb->iiextend;	/* internal 
					** representation of
					** iiphys_extend tuple */
    i4         count = 0;	/* a counter */
    DB_ERROR	    local_dberr;


    *ret_err = E_DM0000_OK;

    for(;;) /* control loop */
    {
	/* Print an informational message that a DB has been found in
	** this area.
	*/
	ret_stat = dmm_message(ret_err, I_DM5420_DB_FOUND, 2,
			    0, logdb_cb->dir_name, 0, logdb_cb->loc_name);
	if (ret_stat != E_DB_OK)
	    break;

	di_stat = DIsense(di_io, &last_pg, &sys_err);
	if (di_stat != OK)
	{
	    uleFormat(NULL, E_DM9007_BAD_FILE_SENSE, NULL, ULE_LOG,
			(DB_SQLSTATE *) NULL, NULL, 0, NULL, 
			&error, 4, 0, logdb_cb->dir_name, 0, CONFIG_FILE,
			logdb_cb->path_len, logdb_cb->path_name,
			fillen, filnam );
	    *ret_err = E_DM5412_IIPHYSDB_ERROR;
	    ret_stat = E_DB_ERROR;
	    break;
	}

	/* 
	**  If we get here, this is a good config file.  Alloccate memory
	**  and read its contents.
	**  NOTE:   each config file may be a different size.  So, we
	**	    allocate as much memory as we need to read this config 
	**	    file and deallocate it afterwards.  It is probably
	**	    more efficient to save size from pass to pass and only
	**	    deallocate and reallocate when needed, but for now I
	**	    am following the KISS principle.  After this code is
	**	    tested and stable, this enhancement should be made, as
	**	    it will speed up finddbs and reduce memory segmentation.
	*/
	last_pg++;
	size = DM_PG_SIZE * (last_pg);
	ret_stat = dm0m_allocate( size, DM0M_ZERO, 
				(i4) MISC_CB, (i4) MISC_ASCII_ID,
				(char *) 0, (DM_OBJECT **) &alloc_ptr, &local_dberr);
	if (ret_stat != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, NULL, ULE_LOG, (DB_SQLSTATE *) NULL, 
		NULL, 0, NULL, &error, 0);
	    *ret_err = E_DM9188_DMM_ALLOC_ERROR;
	    logdb_cb->flags |= STOP;
	    break;
	}
	alloc_ptr->misc_data = (char *) &alloc_ptr[1];
	cnf_buf.cnf_data = (char *) alloc_ptr->misc_data;

	di_stat = DIread(di_io, &last_pg, (i4) 0, cnf_buf.cnf_data,
			 &sys_err);
	if (di_stat != OK)
	{
	    uleFormat(NULL, *ret_err, NULL, ULE_LOG, (DB_SQLSTATE *) NULL, 
		NULL, 0, NULL, &error, 0);
	    ret_stat = E_DB_ERROR;
	    *ret_err = W_DM5409_CONFIG_READ_ERROR;
	    break;
	}	    

	/* Get info about the db from the config file header */
	desc = (DM0C_DSC *) cnf_buf.cnf_data;
	MEcopy(desc->dsc_name.db_db_name, sizeof(desc->dsc_name.db_db_name),
	       iidatabase->du_dbname);
	MEcopy(desc->dsc_owner.db_own_name, sizeof(desc->dsc_owner.db_own_name),
		iidatabase->du_own.db_own_name);
	iidatabase->du_dbid = desc->dsc_dbid;
	iidatabase->du_1dbcmptminor = desc->dsc_1dbcmptminor;
	iidatabase->du_access = desc->dsc_dbaccess;
	/* if database is marked as private, clear GLOBAL bit from access */
	if (desc->dsc_type & DSC_PRIVATE)
	    iidatabase->du_access &= ~DU_GLOBAL;
	iidatabase->du_dbservice = desc->dsc_dbservice;
	iidatabase->du_dbcmptlvl = desc->dsc_dbcmptlvl;

	/* build the static part of the iiphys_extend tuple entry.  The
	** lname portion will be filled in dynamically */
	MEfill(DB_DB_MAXNAME, ' ', iiextend->du_dname);
	MEcopy(logdb_cb->dir_name,STlength(logdb_cb->dir_name),
	       iiextend->du_dname);
	iiextend->du_status = DU_EXT_OPERATIVE;    	
	
	/* 
	** Get location names from config file --
	*/
	next = (DM0C_EXT*) ( (char *)desc + desc->length);
	while (count <  desc->dsc_ext_count)
	{    
	    for (;;)
	    {
		/* this loop assumes there will always be as many extent records
		** in the config file as the config file header says there are.
		** you will always hit an extent record in this loop.  The 
		** loop will break after we have found all extent records in the
		** config file. 
		*/
		if ( next -> type == DM0C_T_EXT)
		{
		    ext_entry = next;
		    next = (DM0C_EXT*) ( (char *)next + next->length);
		    count++;
		    break;
		}
		next = (DM0C_EXT*) ( (char *)next + next->length);
	    };

	    if (ext_entry->ext_location.flags & DCB_ROOT)
	    {   
		/* this is the default data location, so copy to the
		** iidatabase tuple */
		MEcopy((PTR) ext_entry->ext_location.logical.db_loc_name,
		       sizeof(DB_LOC_NAME),
		       (PTR) iidatabase->du_dbloc.db_loc_name);
	    }
	    if (ext_entry->ext_location.flags & DCB_DATA)
	    {   
		/* this is an extended location for this database, so put the
		** information into iiphys_extend */

		MEcopy((PTR) ext_entry->ext_location.logical.db_loc_name,
		       sizeof(DB_LOC_NAME), (PTR) iiextend->du_lname);
		ret_stat = ext_insert (logdb_cb, ret_err);
		if (ret_stat != E_DB_OK)
		{
		    /* print message to errlog.log explaining what went wrong 
		    ** and send message to FE explaining that there was an 
		    ** internal err */
		    uleFormat(NULL, *ret_err, NULL, ULE_LOG, (DB_SQLSTATE *) NULL, 
			NULL, 0, NULL, &error, 0);
		    loc_stat = dmm_message(&error, E_DM5413_IIPHYSEXT_ERROR, 0);
		    /* indicate to log_cb what the failure was and to stop */
		    logdb_cb->flags |= STOP;
		    logdb_cb->list_stat = E_DM5415_CONFIG_ERROR;
		    break;
		}
	    }
	    else if (ext_entry->ext_location.flags & DCB_JOURNAL)
	    {
		/* This is a journal extension entry, so copy to the 
		** iidatabase tuple */
		MEcopy((PTR) ext_entry->ext_location.logical.db_loc_name,
		       sizeof(DB_LOC_NAME),
		       (PTR) iidatabase->du_jnlloc.db_loc_name);
	    }
	    else if (ext_entry->ext_location.flags & DCB_CHECKPOINT)
	    {
		/* This is a checkpoint extension entry, so copy to the
		** iidatabase tuple */
		MEcopy((PTR) ext_entry->ext_location.logical.db_loc_name,
		       sizeof(DB_LOC_NAME),
		       (PTR) iidatabase->du_ckploc.db_loc_name);
	    }
	    else if (ext_entry->ext_location.flags & DCB_WORK)
	    {
		/* This is a default work extension entry, so copy to the
		** iidatabase tuple */
		MEcopy((PTR) ext_entry->ext_location.logical.db_loc_name,
		       sizeof(DB_LOC_NAME),
		       (PTR) iidatabase->du_workloc.db_loc_name);
	    }
	    else if (ext_entry->ext_location.flags & DCB_DUMP)
	    {
		/* This is a dump extension entry, so copy to the iidatabase
		** tuple */
		MEcopy((PTR) ext_entry->ext_location.logical.db_loc_name,
		       sizeof(DB_LOC_NAME),
		       (PTR) iidatabase->du_dmploc.db_loc_name);
	    }

	}	/* end loop to check each extent record in config file */

	/* now put iidatbase tuple into iiphys_database */

	ret_stat = db_insert (logdb_cb, &error);
	if (ret_stat != E_DB_OK)
	{
	    /* print message to errlog.log explaining what went wrong and
	    ** send message to FE explaining that there was an internal err */
	    uleFormat(NULL, error, NULL, ULE_LOG, (DB_SQLSTATE *) NULL, 
			NULL, 0, NULL, &locerror, 0);
	    loc_stat = dmm_message(&error,E_DM5412_IIPHYSDB_ERROR,0);
	    if ( (loc_stat != E_DB_OK) && (loc_stat > ret_stat) )
	    {
		ret_stat = loc_stat;
		*ret_err = error;
	    }
	    /* indicate to log_cb what the failure was and to stop */
	    logdb_cb->flags |= STOP;
	    logdb_cb->list_stat = E_DM5414_ADMIN30_ERROR;
	    break;
	}


	/* after we have assured that this is a valid database, increment 
	** db found count 
	*/
	++logdb_cb->db_count;

	break;
    }

    /* if memory is allocated, deallocate it */
    if (alloc_ptr)
    {
	dm0m_deallocate((DM_OBJECT **)&alloc_ptr);
    }
    return (ret_stat);
}
