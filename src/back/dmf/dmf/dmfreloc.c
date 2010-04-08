
/* 
**Copyright (c) 2004 Ingres Corporation
NO_OPTIM = dr6_us5
*/ 

#include    <compat.h> 
#include    <gl.h> 
#include    <tm.h>
#include    <me.h>
#include    <cs.h> 
#include    <di.h> 
#include    <jf.h> 
#include    <st.h>
#include    <pc.h>
#include    <ck.h> 
#include    <tr.h> 
#include    <er.h> 
#include    <si.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <bt.h>
#include    <lk.h> 
#include    <lg.h> 
#include    <ulf.h> 
#include    <adf.h>
#include    <dmf.h> 
#include    <dm.h>
#include    <dmp.h>
#include    <dm2d.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dm2r.h>
#include    <dm2f.h>
#include    <dm0c.h>
#include    <dmfjsp.h>
#include    <dm1b.h>
#include    <dmve.h>
#include    <dm0d.h>
#include    <dm0j.h>
#include    <dmpp.h>
#include    <dm0l.h>
#include    <sxf.h>
#include    <dma.h>
#include    <dmd.h>
#include    <dm0m.h>
#include    <dmftrace.h>
#include    <dudbms.h>
#include    <dmxe.h>
#include    <lo.h>
#include    <dmckp.h>
#include    <dmfrfp.h>
#include    <dm0llctx.h>

/*  macro for efficient comparison of strings (avoid proc calls if possible) */
# define STEQ_MACRO( a, b ) ( a[0] == b[0] && !STcompare(a, b) )
#define  RELOC_READ     0
#define  RELOC_WRITE    1

/* 
** The blocksize for jnl and dump files must be one of 4096,8192,16384,32768
**   (some multiple of 512)
** The blocksize for all other database files is 2048
** So all files will be copies using a block size of 512.
*/
#define COPY_BKSZ 512

typedef struct
{
	DMP_LOC_ENTRY *cp_oldloc;
	DMP_LOC_ENTRY *cp_newloc;
	i4        cp_usage;
	char          *cp_buf;
	i4        cp_mwrite_blks;
	i4        cp_bksz;
	STATUS         cp_status;
	DB_DB_NAME    *cp_dbname;
	DU_LOCATIONS  *cp_loctup;
	char          *cp_lo_what;
}   COPY_INFO;

/* externs */

/**
**
**  Name: DMFRELOC.C - Relocate Routines.
**
**  Description:
**      The file contains routines needed to:
**          - Relocate the default journal/dump/checkpoint/work location
**          - Relocate (copy) a database to a new database
**          - Relocate a table to new location(s)
**
**  History:
**      18-nov-1994 (alison)
**          Created.
**      04-jan-1995 (alison)
**          Cleanup error handling in all routines.
**          reloc_tbl_process() open/close db at beginning/end 
**          reloc_loc_process() if db relocation, first update iirelation.relloc
**             for 1 location tables. This way if the default location was
**             relocated, we will update iirelation.relloc for iidevices 
**             before we need to open iidevices to process multi-loc tables.
**          reloc_database() Moved some argument checking to dmfjsp.c
**          reloc_validate_table() Got rid of (unnecessary) rel_positioned parm
**          reloc_update_tbl_loc() Got rid of (unnecessary) rel_positioned parm
**          open_iidbdb() set dcb_bm_served = DCB_MULTIPLE, so cache value
**             locking is used.
**          renamed rfp_relocate() to dmfreloc()
**      25-jan-1995 (stial01)
**          New RELOC errors added to distinguish from RFP errors.
**          More error handling clean up.
**          reloc_loc_process() make sure each loc processed ONCE.
**          BUG 66547: reloc_update_tbl_loc() set TABLE_RECOVERY_DISALLOWED
**          so that table rollforward cant be done again without checkpointing
**          first.
**      22-feb-1995 (stial01)
**          dmfreloc() fixed size arg for dm0m_allocate()
**	6-mar-1995 (shero03)
**	    Bug #67275
**	    Remove the directory after relocating the ckp or jnl files.
**      09-mar-1995 (stial01)
**          BUG 67375: If you relocate a checkpointed database, there are 
**          problems if you then try to rollforward the new duplicate database
**          because relocatedb did not adjust the backup config file in the
**          dmp directory. When we relocate a database, the new database
**          will not inherit any checkpoint history from the original db.
**      10-mar-1995 (stial01)
**          BUG 67411: Added reloc_init_locmap()
**	22-mar-1995 (shero03)
**	    Bug #67569
**	    When searching through the config file first check the 
**	    usage flag for the proper entry.
**      05-apr-1995 (stial01)
**          BUG 67848 Disallow jnl,ckp,dmp,work relocation for iidbdb
**          Also, if relocating jnl,ckp,dmp,work locations, when updating
**          the config, make a copy (UPDTCOPY_CONFIG) in the dmp directory.
**          Also change the blocksize for reading/writing database files
**          from 2048 to 512, because the size of jnl and dump files are 
**          only guaranteed to be multiples of 512.
**      20-may-95 (hanch04)
**          Added include dmtcb.h
**	21-aug-95 (tchdi01)
**	    Made opend_iidbdb() exportable by removing the 'static' keyword.
**      17-may-1996 (hanch04)
**          move <ck.h> after <pc.h>
**	03-jun-1996 (canor01)
**	    Remove unnecessary semaphore calls.
**	 3-jul-96 (nick)
**	    Fix #75513 and whilst in here, do some tidying and other fixes.
**	23-sep-1996 (canor01)
**	    Move open_iidbdb() and build_loc() to dmfjnl.c.
**	26-Oct-1996 (jenjo02)
**	    Added *name parm to dm0s_minit() calls.
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**      08-Dec-1997 (hanch04)
**          Use new la_block field in LG_LA
**          for support of logs > 2 gig
**      26-feb-1998 (hweho01/bonro01)
**          Fixed the error in the parameter in ule_format()
**          for msg E_DM1517_RELOC_TBL_LOC_ERR.
**      20-sep-1998 (dacri01)
**          For bug# 91855.  Get X lock on iidatabase before locking the
**          configuration file.  Changes to dmfreloc() and reloc_default_loc().
**          Added a new parameter, mode, to get_iidatabase().
**      22-Feb-1999 (bonro01)
**          Replace references to svcb_log_id with
**          dmf_svcb->svcb_lctx_ptr->lctx_lgid
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	30-Nov-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID* to dmxe_begin() prototype.
**      30-may-2002 (stial01)
**          get_iiextend() blank pad varchar lname before compare (b107903)
**	17-jul-2003 (penga03)
**	    Added include adf.h.
**      08-Aug-2003 (inifa01)
**          Misc GENERIC build change
**          moved include of 'dm1b.h' before include of 'dmve.h'
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      07-apr-2005 (stial01)
**          Fix queue handling
**      08-Jul-2005 (fanra01)
**          Compiler warning for inconsistent condition.
**      04-Feb-2008 (hanal04) Bug 119781
**          Removed reloc_validate_table(). This was only being called
**          during JSX_DB_RELOC cases (copy of whole DB) and as such
**          was performing redundant checks that caused unnecessary
**          failures. Check for WORK|AWORK conflicts in reloc_check_newlocs().
**      05-Jan-2008 (hanal04) Bug 119781
**          Add back reloc_validate_table() table specific tests but
**          skip out of this routine for DB copy operations.
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here.
**	13-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	18-Nov-2008 (jonj)
**	    dm0c_?, dm2d_? functions converted to DB_ERROR *
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_?, dmf_rfp_? functions converted to DB_ERROR *
**	21-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	22-Nov-2008 (jonj)
**	    In reloc_tbl_process, reloc_validate_table, zero jsp->jsx_dberr.err_code
**	    after uleFormat to prevent dmfrfp from trying to re-uleFormat the
**	    error without needed message params.
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2t_?, dm2r_? functions converted to DB_ERROR *
**
**/

#define	    OPEN_CONFIG		1
#define	    UPDATE_CONFIG	2
#define	    CLOSE_CONFIG	3
#define	    CLOSEKEEP_CONFIG	4
#define	    UNKEEP_CONFIG	5
#define     UPDTCOPY_CONFIG     6 


/*
**  Forward and/or External function references.
*/

static DB_STATUS
reloc_default_loc(
DMF_JSX         *jsx,
DMP_DCB         *dcb,
DMP_DCB         *iidcb,
DM0C_CNF        *cnf,
COPY_INFO       *copy_info,
i4          usage,
DU_DATABASE     *database,
i4          lock_id,
i4          log_id,
DB_TRAN_ID      *tran_id);

static DB_STATUS cp_di_files(
    COPY_INFO       *copy_info,
    char            *filename,
    i4          l_filename,
    CL_ERR_DESC      *sys_err);

static DB_STATUS get_iidatabase(
    DMF_JSX             *jsx,
    DMP_DCB             *dcb,
    DB_DB_NAME          *dbname, 
    DU_DATABASE         *database,
    DB_LOC_NAME         *new_locname,
    i4              usage,
    i4              mode,
    i4              log_id,
    DB_TRAN_ID          *tran_id,
    i4              lock_id);

static DB_STATUS get_iilocation(
    DMF_JSX             *jsx,
    DMP_DCB             *dcb,
    DMP_RCB             *loc_rcb,
    DB_LOC_NAME         *newloc, 
    DU_LOCATIONS        *location,
    i4              log_id,
    DB_TRAN_ID          *tran_id,
    i4              lock_id);

static DB_STATUS reloc_database(
    DMF_JSX         *jsx,
    DMP_DCB         *dcb,
    DMP_DCB         *iidcb,
    DM0C_CNF        *cnf,
    COPY_INFO       *copy_info,
    DU_DATABASE     *database,
    i4          lock_id,
    i4          log_id,
    DB_TRAN_ID      *tran_id);

DB_STATUS build_loc(
    DB_DB_NAME      *dbname,
    i4               l_path,
    char             *path,
    char             *loc_type,
    DMP_LOC_ENTRY   *location);

static DB_STATUS get_iiextend(
    DMF_JSX             *jsx,
    DMP_DCB             *dcb,
    DB_DB_NAME          *dbname, 
    DB_LOC_NAME         *lname,
    i4              ext_flags,
    DMP_RCB             *rcb,
    DU_EXTEND           *extend,
    i4              log_id,
    DB_TRAN_ID          *tran_id,
    i4              lock_id);

static DB_STATUS config_handler(
    DMF_JSX	    *jsx,
    i4	    operation,
    DMP_DCB	    *dcb,
    DM0C_CNF	    **config);

static DB_STATUS delete_files(
    DMP_LOC_ENTRY   *location,
    char	    *filename,
    i4	    l_filename,
    CL_ERR_DESC	    *sys_err);

static DB_STATUS reloc_update_tbl_loc(
    DMF_JSX         *jsx,
    DMP_DCB         *dcb,
    DB_TAB_ID       *table_id,
    DMP_RELATION    *relrecord,
    DMP_RCB         *rel_rcb,
    DMP_RCB         *dev_rcb);

static DB_STATUS reloc_validate_table(
    DMF_JSX         *jsx,
    DMP_DCB         *dcb,
    DB_TAB_ID       *table_id,
    DMP_RELATION    *relrecord,
    DMP_RCB         *rel_rcb,
    DMP_RCB         *dev_rcb);

static DB_STATUS reloc_check_loc_lists(
    DMF_JSX         *jsx,
    DMP_DCB         *dcb,
    DMP_DCB         *iidcb,
    i4         log_id,
    DB_TRAN_ID      *tran_id,
    i4         lock_id);

DB_STATUS open_iidbdb(
    DMF_JSX         *jsx,
    DMP_DCB                **iidcbp,
    i4                access_mode,
    i4                lock_mode,
    i4                iidb_flag);

static DB_STATUS alloc_newdcb(
    DMF_JSX         *jsx,
    DMP_DCB         *orig_dcb,
    DMP_DCB         **newdcb,
    DMP_LOC_ENTRY   *root_loc,
    DU_DATABASE     *database);

static DB_STATUS reloc_db_dircreate(
    DMF_JSX         *jsx,
    COPY_INFO       *copy_info);

static DB_STATUS reloc_check_newlocs(
    DMF_JSX             *jsx,
    DMP_DCB             *dcb);

static DB_STATUS reloc_init_locmap(
    DMF_RFP         *rfp,
    DMF_JSX         *jsx,
    DMP_DCB         *dcb);


/*{
** Name: dmfreloc            - Routine to do PURE relocation functions
**
** Description:
**
**      This routine is called for the PURE relocation functions,
**      i.e. relocation without recovery functions.
**
**      Allocate a buffer for copying files, open the config file for
**      the database specified and open the iidbdb database.
**      Then call the appropriate routine to do the actual relocation:
**      There are two categories for pure relocation functions:
**
**          1) Relocate a database to a new database (copy it) 
**             The routine reloc_database() is called.
**
**          2) Relocate the default checkpoint, dump, journal or work
**             locations. The routine reloc_default_loc() is called.
**
** Inputs:
**      jsx                     Pointer to Journal Support info.
**      dcb                     Pointer to DCB for this database.
**
** Outputs:
**	Returns:
**	    E_DB_OK, E_DB_ERROR
**      err_code		reason for status != E_DB_OK
**
** Side Effects:
**	    none
**
** History:
**        18-nov-1994 (alison)
**           Created for Database relocation
**        22-feb-1995 (stial01)
**           dmfreloc() fixed size arg for dm0m_allocate()
**        05-apr-1995 (stial01)
**           BUG 67848 Disallow jnl,ckp,dmp,work relocation for iidbdb
**           Also change the blocksize for reading/writing database files
**           from 2048 to 512, because the size of jnl and dump files are 
**           only guaranteed to be multiples of 512.
**	 3-jul-96 (nick)
**	    Tidy.
**	28-Jul-2005 (schka24)
**	    Back out x-integration for bug 111502 (not shown), doesn't
**	    apply to r3.
[@history_template@]...
*/
DB_STATUS
dmfreloc(
DMF_JSX         *jsx,
DMP_DCB         *dcb)
{
    DMP_DCB             *iidcb;
    DM0C_CNF            *cnf;
    bool                open_db;
    bool                open_iidb;
    bool                open_cnf;
    DB_STATUS           status;
    DB_STATUS           close_status;
    i4             db_flag;
    i4             iidb_flag;
    i4             lock_mode;
    i4             lock_id;
    i4             log_id = 0;
    DB_TRAN_ID          tran_id;
    i4             dmxe_flag;
    DU_DATABASE         database;
    DB_TAB_TIMESTAMP    ctime;
    i4             usage;
    i4             status1;
    i4             cur_status1;
    char                *usg_str;
    COPY_INFO           copy_info;
    DM_OBJECT           *misc_buffer;
    char                error_buffer[ER_MAX_LEN];
    i4             error_length;
    i4			local_err;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Disallow relocation of ckp,jnl,dmp,work locations for iidbdb 
    ** There are places in the code that depend on the fact that the    
    ** iidbdb database NEVER has alternate locations; it always has II_DATABASE
    ** as its database location and II_DUMP as its dump location (see B39106)
    */
    if (jsx->jsx_status1 & (JSX_CKPT_RELOC | JSX_JNL_RELOC | JSX_DMP_RELOC
				| JSX_WORK_RELOC))
    {
	if ((MEcmp((PTR)&dcb->dcb_name, (PTR)DB_DBDB_NAME,
		sizeof(DB_DB_NAME)) == 0) )
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1518_RELOC_IIDBDB_ERR);
	    return (E_DB_ERROR);
	}
    }

    /*
    ** Allocate a buffer for reading/writing files
    */
    copy_info.cp_mwrite_blks = 128;  /* the minimum */
    copy_info.cp_bksz = COPY_BKSZ;
    status = dm0m_allocate((i4)( sizeof(DMP_MISC) + 
			(copy_info.cp_mwrite_blks * copy_info.cp_bksz)), 
			(i4)0, (i4)MISC_CB, (i4)MISC_ASCII_ID,
			(char *)0, (DM_OBJECT **)&misc_buffer, &jsx->jsx_dberr);
    if (status != OK)
    {
	dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	dmfWriteMsg(NULL, E_DM150A_RELOC_MEM_ALLOC, 0);
	return (E_DB_ERROR);
    }
    else
    {
	((DMP_MISC *)misc_buffer)->misc_data = (char *)misc_buffer +
		sizeof(DMP_MISC);
	copy_info.cp_buf = (char *)misc_buffer + sizeof(DMP_MISC);
    }

    for (status1 = jsx->jsx_status1; ; )
    {
	if (status1 & JSX_CKPT_RELOC)
	{
	    /* Relocate default checkpoint location: new_ckp_location= */
	    usage = DCB_CHECKPOINT;
	    cur_status1 = JSX_CKPT_RELOC;
	    usg_str = "CHECKPOINT";
	}
	else if (status1 & JSX_JNL_RELOC)
	{
	    /* Relocate default journal location: new_jnl_location= */
	    usage = DCB_JOURNAL;
	    cur_status1 = JSX_JNL_RELOC;
	    usg_str = "JOURNAL";
	}
	else if (status1 & JSX_DMP_RELOC)
	{
	    /* Relocate default dump location: new_dump_location= */
	    usage = DCB_DUMP;
	    cur_status1 = JSX_DMP_RELOC;
	    usg_str = "DUMP";
	}
	else if (status1 & JSX_WORK_RELOC)
	{
	    /* Relocate default work location: new_work_location= */
	    usage = DCB_WORK;
	    cur_status1 = JSX_WORK_RELOC;
	    usg_str = "WORK";
	}
	else if (status1 & JSX_DB_RELOC)
	{
	    /* Relocate entire database: new_database= */
	    cur_status1 = JSX_DB_RELOC;
	    usg_str = "DATABASE";
	}
	else
	    break;     /* all relocate requests processed */

	/* reset status1 so we process this request only once */
	status1 &= ~cur_status1;

	open_db = open_iidb = open_cnf = FALSE;
	iidcb = 0;

	do
	{
            /* Open the iidbdb database */
            iidb_flag = 0;  /* FIX ME */
            status = open_iidbdb(jsx, &iidcb, DM2D_A_WRITE, DM2D_IX, iidb_flag);

            if (status != E_DB_OK)
            {
                /* error formatted in open_iidbdb */
                break;
            }

            open_iidb = TRUE;

            /*  Begin a transaction. */
            dmxe_flag = (i4)DMXE_JOURNAL;
            status = dmxe_begin(DMXE_WRITE, dmxe_flag, iidcb,
                iidcb->dcb_log_id, &iidcb->dcb_db_owner,
                dmf_svcb->svcb_lock_list, &log_id, &tran_id, &lock_id,
	        (DB_DIS_TRAN_ID *)NULL,
                &jsx->jsx_dberr);
            if (status != E_DB_OK)
            {
		dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
                break;
            }

            if (cur_status1 == JSX_DB_RELOC)
            {
                /*
                ** Get the iidatabase tuple for read
                **   Use key on dcb_name (DM_1_DATABASE_KEY)
                */
                status = get_iidatabase(jsx, iidcb, &dcb->dcb_name, &database,
                                0, 0, RELOC_READ, log_id, &tran_id, lock_id);
                if (status != E_DB_OK)
                {
                    /* FIX ME */
                    break;
                }
            }
            else
            {
                /*
                ** Get the iidatabase tuple for update
                **   Use key on dcb_name (DM_1_DATABASE_KEY)
                */
                status = get_iidatabase(jsx, iidcb, &dcb->dcb_name, &database,
                                0, 0, RELOC_WRITE, log_id, &tran_id, lock_id);
                if (status != E_DB_OK)
                {
                    /* FIX ME */
                    break;
                }
            }

	    /*
	    ** Open the database
	    */
	    if (jsx->jsx_status & JSX_WAITDB)
		db_flag = 0;
	    else
		db_flag = DM2D_NOWAIT;
	    if (cur_status1 == JSX_WORK_RELOC || cur_status1 == JSX_DB_RELOC)
		lock_mode = DM2D_X;
	    else
		lock_mode = DM2D_IS;

	    status = dm2d_open_db(dcb, DM2D_A_READ, lock_mode,
		dmf_svcb->svcb_lock_list, db_flag, &jsx->jsx_dberr);

	    if (status != E_DB_OK)
	    {
		dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		break;
	    }

	    open_db = TRUE;

	    /* Open and read the configuration file */
	    status = config_handler(jsx, OPEN_CONFIG, dcb, &cnf);
	    if (status != E_DB_OK)
	    {
		/* error formatted in config_handler */
		break;
	    }

	    open_cnf = TRUE;

	    /*
	    ** There are two types of PURE relocation without recovery:
	    **   - relocate database (make a copy)
	    **   - relocate default ckp/dmp/jnl/work locations
	    */
	    if (cur_status1 == JSX_DB_RELOC)
	    {
		status = reloc_database(jsx, dcb, iidcb, cnf, &copy_info, 
                                &database, lock_id, log_id,
                                &tran_id);
	    }
	    else
	    {
		status = reloc_default_loc(jsx, dcb, iidcb, cnf, &copy_info,
                                usage, &database, lock_id, log_id,
                                &tran_id);
            }
	} while (FALSE);

	if (status != E_DB_OK)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1507_RELOC_ERR);
	    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		error_buffer, ER_MAX_LEN, &error_length, &local_err, 2,
		STlength(usg_str), usg_str, 0, 0);
	    dmf_put_line(0, error_length, error_buffer);
	}

        if (status == E_DB_OK && log_id)
        {
            /*
            ** Commit the transaction
            */
            status = dmxe_commit(&tran_id, log_id, lock_id, dmxe_flag,
                &ctime, &jsx->jsx_dberr);

            log_id = 0;
            if (status != E_DB_OK)
		dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
        }
        else if (log_id)
        {
            /* Abort transaction in progress */
            close_status = dmxe_abort((DML_ODCB *)0, iidcb, &tran_id,
                        log_id, lock_id, dmxe_flag, 0,0, &jsx->jsx_dberr);
            if (close_status != E_DB_OK)
            {
		dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
                if (close_status > status)
                    status = close_status;
            }
        }

	/* Close the database */
	if (open_db)
	{
	    close_status = dm2d_close_db(dcb, dmf_svcb->svcb_lock_list,
			db_flag, &jsx->jsx_dberr);
	
	    if (close_status != E_DB_OK)
	    {
		dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		if (close_status > status)
		    status = close_status;
	    }
	}

	/* Close the iidbdb database */
	if (open_iidb)
	{
	    close_status = dm2d_close_db(iidcb, dmf_svcb->svcb_lock_list,
			iidb_flag, &jsx->jsx_dberr); /* FIX ME */
	    if (close_status != E_DB_OK)
	    {
		dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		if (close_status > status)
		    status = close_status;
	    }
	}

	/* Free the DCB memory used for the iidbdb open */
	if (iidcb)
	    dm0m_deallocate((DM_OBJECT **)&iidcb);

	/* Close the config file */
	if (open_cnf)
	{
	    DB_ERROR	save_dberr = jsx->jsx_dberr;

	    close_status = config_handler(jsx, CLOSEKEEP_CONFIG, dcb, &cnf);
	    if (close_status != E_DB_OK)
	    {
		/* error formatted in config_handler */
		if (close_status > status)
		    status = close_status;
	    }
	    jsx->jsx_dberr = save_dberr;
	}
    }

    /* Free the read/write buffer */
    dm0m_deallocate((DM_OBJECT **)&misc_buffer);

    return (status);
}


/*{
** Name: reloc_default_loc       - Relocate default ckp/dmp/jnl/work locations
**
** Description:
**
**      This routine is called to relocate the default checkpoint, dump,
**      journal or work locations.
**
**      First verification of the new location name is done. The new
**      location name must exist in iidbdb:iilocation and have compatible
**      location usage. For work locations we also verify that this 
**      database has been extended to the new location by checking the
**      config file.
**
**      If we can do the relocation we copy the files to the new location,
**      then we update iidbdb:iidatabase.ckpdev, iidatabase.dmpdev,
**      iidatabase.jnldev or iidatabase.sortdev. Then we update the config
**      file and delete the files from the old location.
**      Note: no files are copied/deleted for work locations.
**
** Inputs:
**      jsx                     Pointer to Journal Support info.
**      dcb                     Pointer to DCB for this database.
**      iidcb                   Pointer to the DCB for the iidbdb database.
**      cnf                     Pointer to the config info for this database.
**      copy_info               Pointer to info for copying files (containing
**                                 a preallocated buffer)
**      usage                   location usage: DCB_CHECKPOINT, DCB_DUMP,
**                                 DCB_JOURNAL, DCB_WORK.
**
** Outputs:
**	Returns:
**	    E_DB_OK, E_DB_ERROR
**      err_code		reason for status != E_DB_OK
**
** Side Effects:
**	    none
**
** History:
**        18-nov-1994 (alison)
**           Created for Database relocation
**	6-mar-1995 (shero03)
**	    Bug #67275
**	    Remove the directory after relocating the ckp or jnl files.
**	22-mar-1995 (shero03)
**	    Bug #67569
**	    When searching through the config file first check the 
**	    usage flag for the proper entry.
**      06-apr-1995 (stial01)
**          When updating config, make a backup copy in dmp directory.
[@history_template@]...
*/
static DB_STATUS
reloc_default_loc(
DMF_JSX         *jsx,
DMP_DCB         *dcb,
DMP_DCB         *iidcb,
DM0C_CNF        *cnf,
COPY_INFO       *copy_info,
i4          usage,
DU_DATABASE     *database,
i4          lock_id,
i4          log_id,
DB_TRAN_ID      *tran_id)
{
    JSX_LOC             *jsx_newloc;
    i4             lock_mode;
    DMP_RCB             *loc_rcb = 0;
    DMP_LOC_ENTRY       *oldloc, *newloc; 
    DMP_LOC_ENTRY       tmploc;
    DMP_LOC_ENTRY       newlocbuf;
    DMP_LOC_ENTRY       saveoldloc;
    DB_LOC_NAME         *oldname;
    DM_TID              tid;
    DB_STATUS           status,close_status,di_status;
    DU_LOCATIONS        location;
    DI_IO               di_file;
    CL_ERR_DESC         sys_err;
    DB_OWN_NAME         *user_name;
    i4             compare;
    char                error_buffer[ER_MAX_LEN];
    i4             error_length;
    i4             loc_status;
    i4             i;
    i4			local_err;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    do
    {
	/*
	** - Find the ext entry for the old location 
	**      For CKPT, JNL and DMP we can find this in cnf struct.
	** - Depending on USAGE, init oldname DB_LOC_NAME ptr to correct
	**      default location (CKPT,JNL,WORK,DMP) in iidatabase tuple
	**      NOTE: database tuple is has not been fetched yet.
	*/
	if (usage == DCB_CHECKPOINT)
	{
	    jsx_newloc = &jsx->jsx_def_list[JSX_CKPTLOC];
	    oldloc = dcb->dcb_ckp;
	    oldname = &database->du_ckploc;
	    copy_info->cp_lo_what = LO_CKP;
	    loc_status = DU_CKPS_LOC;
	}
	else if (usage == DCB_JOURNAL)
	{
	    jsx_newloc = &jsx->jsx_def_list[JSX_JNLLOC];
	    oldloc = dcb->dcb_jnl;
	    oldname = &database->du_jnlloc;
	    copy_info->cp_lo_what = LO_JNL;
	    loc_status = DU_JNLS_LOC;
	}
	else if (usage == DCB_DUMP)
	{
	    jsx_newloc = &jsx->jsx_def_list[JSX_DMPLOC];
	    oldloc = dcb->dcb_dmp;
	    oldname = &database->du_dmploc;
	    copy_info->cp_lo_what = LO_DMP;
	    loc_status = DU_DMPS_LOC;
	}
	else if (usage == DCB_WORK)
	{
	    jsx_newloc = &jsx->jsx_def_list[JSX_WORKLOC];
	    oldloc = 0;
	    oldname = &database->du_workloc;
	    copy_info->cp_lo_what = LO_WORK;
	    loc_status = DU_WORK_LOC | DU_AWORK_LOC;
	}

	/* If relocating the journal file,
	** the database must not be journaled
	** the database must be journal disabled
        */
	if (usage == DCB_JOURNAL &&
	    ((cnf->cnf_dsc->dsc_status & DSC_JOURNAL) != 0) && 
	    ((cnf->cnf_dsc->dsc_status & DSC_DISABLE_JOURNAL) != 0))
	{
	    dmfWriteMsg(NULL, E_DM1505_RELOC_DISABLE_JNL, 0);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** If relocating the work location:
	** The database must be extended to the new work location
	** Find the ext entry for the new location
	*/
	if (usage == DCB_WORK)
	{
	    status = find_dcb_ext(jsx, dcb, &jsx_newloc->loc_name, usage, 
					&newloc);
	    if (status != E_DB_OK)
	    {
		dmfWriteMsg(NULL, E_DM1503_RELOC_NEWLOC_NOTFOUND, 0);
		break;
	    }

	    /*
	    ** Check that the usage for the new location is correct 
	    */
	    if (!(newloc->flags & usage))
	    {
		status = E_DB_ERROR;
		dmfWriteMsg(NULL, E_DM1506_RELOC_INVL_USAGE, 0);
		break;
	    }
	}

	/*
	** We didn't get the location entry for WORK yet, we wanted to
	** see what was the default work location name in the iidatabase 
	** record.
	*/
	if (!oldloc)
	{
	    status = find_dcb_ext(jsx, dcb, oldname, usage, &oldloc);
	    if (status != E_DB_OK)
	    {
		SETDBERR(&jsx->jsx_dberr, 0, E_DM150C_RELOC_LOC_CONFIG_ERR);
		uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			error_buffer, ER_MAX_LEN, &error_length, &local_err,
			2, sizeof (DB_LOC_NAME), oldname, 0, 0);
		dmf_put_line(0, error_length, error_buffer);
		break;
	    }
	}

	/*
	** Check location names really exist in iilocation table 
	** For WORK, we checked that the db was extended to the new work
	** location and that the usage was correct.
	** For JNL/CKP/DMP we need to make sure that the location name is 
	** valid, and that the usage is compatible.
	*/
	status = get_iilocation(jsx, iidcb, loc_rcb, &jsx_newloc->loc_name,
				&location, log_id, tran_id, lock_id);
	if (status != E_DB_OK)
	{
	    dmfWriteMsg(NULL, E_DM1503_RELOC_NEWLOC_NOTFOUND, 0);
	    break;
	}

	if (usage != DCB_WORK)
	{
	    /* Validate location usage is compatible */
	    if ( (location.du_status & loc_status) == 0)
	    {
		status = E_DB_ERROR;
		dmfWriteMsg(NULL, E_DM1506_RELOC_INVL_USAGE, 0);
		break;
	    }

	    /* For JNL/CKP/DMP, build a DMP_LOC_ENTRY for the new location */ 
	    newloc = &newlocbuf; 
	    if (build_loc(&dcb->dcb_name, location.du_l_area,
                        location.du_area, copy_info->cp_lo_what,
                        newloc) != E_DB_OK)
            {
                 status = E_DB_ERROR;
                break;
            }
	    newloc->flags = usage;
	    MEcopy((PTR)&jsx_newloc->loc_name, sizeof(newloc->logical),
			(PTR)&newloc->logical);
	}

	/* Check if oldloc = newloc */
	compare = MEcmp((char *)&oldloc->logical,
		(char *)&jsx_newloc->loc_name,
		sizeof(DB_LOC_NAME));
	if (compare == 0)
	{
	    status = E_DB_ERROR;
	    dmfWriteMsg(NULL, E_DM1500_RELOC_OLDLOC_EQ_NEW, 0);
	    break;
	}

	/*
	** Save the old location, we need to know it to delete files
	** from there when we are done.
	** (We will lose this info when we update our current cnf and dcb) 
	*/
	MEcopy(oldloc, sizeof *oldloc, &saveoldloc);
	/*
	** Create files in the new location and then block by block 
	** copy the old file(s) to the new file(s)
	** NOTE: if a checkpoint has not been done for this db, the
	**       CKP/DMP directories may not exist yet.
	**       if journalling is disabled for this db, the JNL 
	**       directory may not exist yet.
	** NOTE: if usage = WORK, no need to copy any files at all
	*/ 
	if (usage != DCB_WORK)
	{
	    copy_info->cp_oldloc = oldloc;
	    copy_info->cp_newloc = newloc;
	    copy_info->cp_usage = usage; 
	    copy_info->cp_status = OK;
	    copy_info->cp_loctup = &location;
	    copy_info->cp_dbname = &dcb->dcb_name;
	    di_status = DIlistfile(&di_file, oldloc->physical.name,
		oldloc->phys_length, cp_di_files, (PTR)copy_info,
		&sys_err);
	    if ( (copy_info->cp_status != OK) || 
	         (di_status != DI_ENDFILE && di_status != DI_DIRNOTFOUND))
	    {
		status = E_DB_ERROR;
		dmfWriteMsg(NULL, E_DM1501_RELOC_CP_FI_ERR, 0);
		break;
	    }
	}

	/* 
	** Update iidatabase table:
	**    CKPT: update iidatabase set ckpdev='newckpt' where dname ='...'
	**    DUMP: update iidatabase set dmpdev='newdmp' where dname='...'
	**    JNL:  update iidatabase set jnldev='newjnl' where dname='...'
	**    WORK: update iidatabase set sortdev=newsort' where dname='...'
	*/
        status = get_iidatabase(jsx, iidcb, &dcb->dcb_name, database,
                        &newloc->logical, usage, RELOC_WRITE, log_id, tran_id,
			lock_id);
	if (status != E_DB_OK)
	{
	    /* update error formatted in get_iidatabase */
	    break;
	}

	/*
	** Update config file
	** FIX ME: we have a problem if we write out the new config file
	**         but our updates to iidatabase dont' get committed
	**    Default location for ckpt,jnl and dump are in specific place
	**    in the cnf_ext array.
	**    See dm2d_open_db() in which dcbs for jnl/ckp/dmp are init
	**    by looping thru cnf_ext. If it were to see multiple JNL 
	**    exts, dcb_jnl would be set to the last one it saw.
	**    There is no check to see if the logical name matches the
	**    one in iidatabase.
	** For now, for ckp,dmp,jnl, overwrite the old cnf_ext with
	** the new ckp/dmp/jnl location.
	** Maybe instead, we should add a new cnf_ext for the new location
	** and when we open a db, make sure that we use the ckp/dmp/jnl
	** location that matches iidatabase.ckploc,dmploc,jnlloc.
	*/
	if (usage != DCB_WORK)
	{
	    for (i = 0; i < cnf->cnf_dsc->dsc_ext_count; i++)
	    {
		/*
		**  Bug #67569 - first check the usage
		*/
		if ((cnf->cnf_ext[i].ext_location.flags & usage) == 0)
		    continue;

		compare = MEcmp((char *)&oldloc->logical,
		    (char *)&cnf->cnf_ext[i].ext_location.logical,
		    sizeof(DB_LOC_NAME));
		if (compare == 0)
		{
		    MEcopy(newloc, sizeof *newloc,
			&cnf->cnf_ext[i].ext_location);
		    break;
		}
	    }

	    if (i >= cnf->cnf_dsc->dsc_ext_count)
	    {
		status = E_DB_ERROR;
		SETDBERR(&jsx->jsx_dberr, 0, E_DM150C_RELOC_LOC_CONFIG_ERR);
		uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			error_buffer, ER_MAX_LEN, &error_length, &local_err,
			2, sizeof (DB_LOC_NAME), &oldloc->logical, 0,0);
		dmf_put_line(0, error_length, error_buffer);
		break;
	    }
	
	    /*
	    ** If changing dump location, fix dump location in dcb so
	    ** we will correctly back up the config file to the new dump
	    ** location when we update the config file
	    */
	    if (usage == DCB_DUMP && cnf->cnf_dcb->dcb_dmp)
	    {
		MEcopy(&cnf->cnf_ext[i].ext_location, 
			sizeof *cnf->cnf_dcb->dcb_dmp,
			cnf->cnf_dcb->dcb_dmp);
	    }

	    /* Update config and make a backup */
	    status = config_handler(jsx, UPDTCOPY_CONFIG, dcb, &cnf);
	    if (status != E_DB_OK)
	    {
		/* error formatted in config_handler */
		break;
	    }

	}

    } while (FALSE);

    if (status == E_DB_OK)
    {
	/* 
	** Delete the files from the old location
	** (except if usage = WORK, don't move files)  
	*/
	if (usage != DCB_WORK)
	{
	    di_status = DIlistfile(&di_file, saveoldloc.physical.name,
			saveoldloc.phys_length, delete_files, (PTR)&saveoldloc,
			&sys_err);
	    if (di_status != DI_ENDFILE && di_status != DI_DIRNOTFOUND)
	    {
		status = E_DB_ERROR;
		dmfWriteMsg(NULL, E_DM1502_RELOC_DEL_FI_ERR, 0);
	    }

	    /* B67275 - delete the old directory */
	    di_status = DIdirdelete(&di_file,
				saveoldloc.physical.name,
				saveoldloc.phys_length,
				0, 0, &sys_err);
	}
    }

    return (status);
     
}


/*{
** Name: find_dcb_ext            - Find dcb_ext entry for specified location
**
** Description:
**
**      This routine goes through the array of extent information in the
**      dcb (dcb_ext) looking for the specified location. If found, 
**      this routine will return the address of the appropriate extent block.
**
** Inputs:
**      jsx                     Pointer to Journal Support info.
**      dcb                     Pointer to DCB for this database.
**      loc                     Pointer to the location name to search for.
**      usage                   location usage: DCB_CHECKPOINT, DCB_DUMP,
**                                 DCB_JOURNAL, DCB_WORK.
**
** Outputs:
**	Returns:
**	    E_DB_OK, E_DB_ERROR
**      ret_ext                 Place to put address of extent block if 
**                              location name is found.
**      err_code		reason for status != E_DB_OK
**
** Side Effects:
**	    none
**
** History:
**        18-nov-1994 (alison)
**           Created for Database relocation
[@history_template@]...
*/
DB_STATUS
find_dcb_ext(
DMF_JSX         *jsx,
DMP_DCB         *dcb,
DB_LOC_NAME     *loc,
i4          usage,
DMP_LOC_ENTRY   **ret_ext)
{
    DMP_EXT             *ext;
    i4                  j;
    i4             compare; 

    CLRDBERR(&jsx->jsx_dberr);

    ext = dcb->dcb_ext;
    *ret_ext = NULL;
    for (j = 0; j < ext->ext_count; j++)
    {
	/*
	**  Bug B67569 - check the usage first
	*/
	if (( ext->ext_entry[j].flags & usage) == 0)
	    continue;

	compare = 
	    MEcmp((char *)loc->db_loc_name,
	    (char *)&ext->ext_entry[j].logical,
	    sizeof(DB_LOC_NAME));

	if (compare == 0)
	{
	    *ret_ext = &ext->ext_entry[j];
	    break;
	}
    }
    if (!(*ret_ext))
	return (E_DB_ERROR);
    else
	return (E_DB_OK);
}

/*{
** Name: get_iidatabase          - Get/Update iidatabase tuple for this db
**
** Description:
**
**      This routine will get the iidatabase tuple for the specified 
**      database. It will optionally update the default checkpoint,
**      dump, journal or work location in the iidatabase tuple.
**
** Inputs:
**      jsx                     Pointer to Journal Support info.
**      dcb                     Pointer to DCB for the iidbdb database.
**      dbname                  Pointer to the db name we are searching for.
**      loc                     Pointer to the new default location name, 
**                              If nil, we will only get the iidatabase tuple
**                              for this db.
**                              If not nil, we will get the iidatabase tuple
**                              for this db, update the appropriate default
**                              location and replace the iidatabase tuple.
**      usage                   location usage: DCB_CHECKPOINT, DCB_DUMP,
**                                 DCB_JOURNAL, DCB_WORK.
**                              Only specified when loc != nil, to indicate
**                              which default location to update (ckp/dmp/
**                              jnl/work).
**      log_id                  Log id.
**      tran_id                 Transaction id.
**      lock_id                 Lock id.
**
** Outputs:
**	Returns:
**	    E_DB_OK, E_DB_ERROR
**      err_code		reason for status != E_DB_OK
**
** Side Effects:
**	    none
**
** History:
**        18-nov-1994 (alison)
**           Created for Database relocation
**	01-oct-1998 (nanpr01)
**	    Added the new parameter in dm2r_replace for update performance 
**	    improvement.
[@history_template@]...
*/
static DB_STATUS 
get_iidatabase(
DMF_JSX             *jsx,
DMP_DCB             *dcb,
DB_DB_NAME          *dbname,
DU_DATABASE         *database,
DB_LOC_NAME         *new_locname,
i4              usage,
i4              mode,
i4              log_id,
DB_TRAN_ID          *tran_id,
i4              lock_id)
{
    DB_TAB_ID           iidatabase;
    DMP_RCB             *rcb = 0;
    DB_STATUS           status,close_status;
    DB_TAB_TIMESTAMP    stamp;
    DM_TID              tid;
    DM2R_KEY_DESC       key_list[1];
    char                error_buffer[ER_MAX_LEN];
    i4             error_length;
    i4			local_err;
    DB_ERROR		local_dberr;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    iidatabase.db_tab_base = DM_B_DATABASE_TAB_ID;
    iidatabase.db_tab_index = 0;

    key_list[0].attr_number = DM_1_DATABASE_KEY;
    key_list[0].attr_operator = DM2R_EQ;
    key_list[0].attr_value = (char *)dbname;

    if (mode == RELOC_WRITE)
    {
	status = dm2t_open(dcb, &iidatabase, DM2T_IX, DM2T_UDIRECT,
		DM2T_A_WRITE, 0, 20, 0, log_id, lock_id,
		0, 0, DM2T_IX, tran_id, &stamp, &rcb, (DML_SCB *)0, &jsx->jsx_dberr);
    }
    else
    {
	status = dm2t_open(dcb, &iidatabase, DM2T_IS, DM2T_UDIRECT,
		DM2T_A_READ, 0, 20, 0, log_id, lock_id, 
		0, 0, 0, tran_id, &stamp, &rcb, (DML_SCB *)0, &jsx->jsx_dberr);
    }

    if (status != E_DB_OK)
    {
	dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	dmfWriteMsg(NULL, E_DM100F_JSP_OPEN_IIDB, 0);
	return (E_DB_ERROR);
    }

    status = dm2r_position(rcb, DM2R_QUAL, key_list, 1, &tid, &jsx->jsx_dberr);

    if (status == E_DB_OK)
    {
	status = dm2r_get(rcb, &tid, DM2R_GETNEXT, (char *)database,
			&jsx->jsx_dberr);

	if (status == E_DB_OK && new_locname)
	{
	    if (usage == DCB_CHECKPOINT)
		MEcopy(new_locname, sizeof *new_locname,&database->du_ckploc);
	    else if (usage == DCB_JOURNAL)
		MEcopy(new_locname, sizeof *new_locname,&database->du_jnlloc);
	    else if (usage == DCB_WORK)
		MEcopy(new_locname, sizeof *new_locname,&database->du_workloc);
	    else if (usage == DCB_DUMP)
		MEcopy(new_locname, sizeof *new_locname,&database->du_dmploc);

	    status = dm2r_replace(rcb, 0, DM2R_BYPOSITION, (char *)database, 
			(char *)NULL, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
		dmfWriteMsg(NULL, E_DM1504_RELOC_UPDT_IIDB_ERR, 0);
	}	
    }

    if (rcb)
    {
	close_status = dm2t_close(rcb, 0, &local_dberr);
	if (close_status != E_DB_OK)
	{
	    dmfWriteMsg(&local_dberr, 0, 0);
	    if (status == E_DB_OK)
	    {
		jsx->jsx_dberr = local_dberr;
		status = close_status;
	    }
	}
    }

    if (status != E_DB_OK)
    {
	return (E_DB_ERROR);
    }

    return (E_DB_OK); 
}


/*{
** Name: get_iilocation          - Get iilocation tuple for specified location
**
** Description:
**
**      This routine will get the iilocation tuple for the specified 
**      location.
**
** Inputs:
**      jsx                     Pointer to Journal Support info.
**      dcb                     Pointer to DCB for the iidbdb database.
**      loc_rcb                 Pointer to RCB for iilocation table.
**                              If nil, this routine will open/close 
**                                the iilocation table.
**      loc                     Pointer to location name we are searching for.
**      log_id                  Log id.
**      tran_id                 Transaction id.
**      lock_id                  Lock id.
**
** Outputs:
**	Returns:
**	    E_DB_OK, E_DB_ERROR
**   location                Location tuple returned.
**      err_code		reason for status != E_DB_OK
**
** Side Effects:
**	    none
**
** History:
**        18-nov-1994 (alison)
**           Created for Database relocation
[@history_template@]...
*/
static DB_STATUS
get_iilocation(
DMF_JSX             *jsx,
DMP_DCB             *dcb,
DMP_RCB             *loc_rcb,
DB_LOC_NAME         *loc, 
DU_LOCATIONS        *location,
i4              log_id,
DB_TRAN_ID          *tran_id,
i4              lock_id)
{
    DB_TAB_ID           iilocation;
    DMP_RCB             *rcb = 0;
    DB_TAB_TIMESTAMP    stamp;
    DB_STATUS           status,local_status;
    DB_STATUS           close_status;
    DM2R_KEY_DESC       key_list[1];
    DM_TID              tid;
    struct
    {
	i2               count; 
	char        logical[sizeof(*loc)];
    } logical_text;

    DB_ERROR		local_dberr;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    iilocation.db_tab_base = DM_B_LOCATIONS_TAB_ID;
    iilocation.db_tab_index = 0;

    /* Open the iilocations table (ONLY if it isn't already open) */
    if (loc_rcb)
	rcb = loc_rcb;
    else
    {
	status = dm2t_open(dcb, &iilocation, DM2T_IS, DM2T_UDIRECT,
		DM2T_A_READ, 0, 20, 0, log_id, lock_id, 0, 0, 0, tran_id,
		&stamp, &rcb, (DML_SCB *)0, &jsx->jsx_dberr);

	if (status != E_DB_OK)
	{
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    dmfWriteMsg(NULL, E_DM1018_JSP_II_LOCATION, 0);
	    return (E_DB_ERROR); 
	}
    }

    for (logical_text.count = 0;
		loc->db_loc_name[logical_text.count] != ' ';
		logical_text.count++)
    {
	;
    }
	
    MEcopy((PTR)loc, sizeof(logical_text.logical),
		(PTR)logical_text.logical);
    key_list[0].attr_number = DM_1_LOCATIONS_KEY;
    key_list[0].attr_value = (char *)&logical_text;
    key_list[0].attr_operator = DM2R_EQ;

    status = dm2r_position(rcb, DM2R_QUAL, key_list,
			1, &tid, &jsx->jsx_dberr);

    if (status == E_DB_OK)
    {
	status = dm2r_get(rcb, &tid, DM2R_GETNEXT,
			(char *)location, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1023_JSP_NO_LOC_INFO);

    }

    /* Close the location table (ONLY if we did the open) */
    if (rcb && !loc_rcb)
    {
	close_status = dm2t_close(rcb, 0, &local_dberr);
	if (close_status != E_DB_OK)
	{
	    dmfWriteMsg(&local_dberr, 0, 0);
	    if (status == E_DB_OK)
	    {
		jsx->jsx_dberr = local_dberr;
		status = close_status;
	    }
	}
    }

    if (status != E_DB_OK)
    {
	return (E_DB_ERROR);
    }
    else
	return (E_DB_OK);

}


/*{
** Name: cp_di_files             - Copy db file from one location to another
**
** Description:
**
**      This routine will copy a database file from one location to another.
**      The new/old location information is specified in the copy_info
**      paramater.
**
** Inputs:
**      copy_info               Information needed to perform the copy:
**                                  old location name
**                                  new location name
**                                  database name
**                                  location usage information
**                                  buffer for reading/writing file
**      file_name               Name of file to copy 
**      file_length             Length of file name.
**
** Outputs:
**	Returns:
**       OK, FAIL
**   sys_error               reason for status != OK
**
** Side Effects:
**	    none
**
** History:
**        18-nov-1994 (alison)
**           Created for Database relocation
**        11-apr-1995 (stial01)
**           Change the blocksize for reading/writing database files
**           from 2048 to 512, because the size of jnl and dump files are 
**           only guaranteed to be multiples of 512.
**        08-dec-1995 (canor01)
**           Checkpoint "files" can actually be directories of files.
**           If so, call the file copy routines recursively.
**	2-Mar-2007 (kschendel) SIR 122757
**	    New files can be opened PRIVATE for copying.
**	09-Sep-2008 (jonj)
**	    Correct type of ule_format output err_code.
[@history_template@]...
*/
static STATUS
cp_di_files(
COPY_INFO     *copy_info,
char          *file_name,
i4        file_length,
CL_ERR_DESC    *sys_error)
{
    DI_IO               di_old, di_new, di_dir;
    DB_STATUS           err_code = E_DB_OK;
    DB_STATUS           status, close_status;
    i4             pagesize;
    CL_ERR_DESC         sys_err;
    i4             old_endpg;
    i4             mwrite_blocks;
    i4             page_count, save_page_count;
    i4             page;
    i4             filebytes;
    char                db_name[64];
    i4             i;
    bool                newopen, oldopen = FALSE;
    char                temp_loc[DB_AREA_MAX + 4];
    STATUS              di_retcode;
    LOCATION            oldloc, newloc;
    char                error_buffer[ER_MAX_LEN];
    i4             error_length;
    bool                recreate = FALSE;
    i4             db_len;
    char                *cp;
    i4			flagword;
    LOCATION            loc;
    LOINFORMATION	loinf;
    COPY_INFO		new_copy_info;
    LO_DIR_CONTEXT	lodircon;
    STATUS		retval;
    i4			uleError;
    DB_ERROR		local_dberr, close_dberr;

    CLRDBERR(&local_dberr);

    pagesize = copy_info->cp_bksz;  /* smallest ingres page size */

    /*
    ** First, see if the file is really a directory.  If so,
    ** recurse.
    */
    flagword = LO_I_TYPE;
    copy_info->cp_oldloc->physical.name[copy_info->cp_oldloc->phys_length] = EOS;
    LOfroms( PATH, copy_info->cp_oldloc->physical.name, &oldloc );
    file_name[ file_length ] = EOS;
    LOfstfile( file_name, &oldloc );
    if ((LOinfo( &oldloc, &flagword, &loinf ) == OK) &&
        (loinf.li_type == LO_IS_DIR))
    {
        DI_IO	di_file;
	i4	old_save_len, new_save_len;

	/* We have a directory */

	MEcopy( copy_info, sizeof *copy_info, &new_copy_info );
	old_save_len = copy_info->cp_oldloc->phys_length;
	new_save_len = copy_info->cp_newloc->phys_length;

	/* Get old directory name as a string */
	LOtos( &oldloc, &cp );
	STcopy( cp, new_copy_info.cp_oldloc->physical.name );
	new_copy_info.cp_oldloc->phys_length = STlength( cp );

	copy_info->cp_newloc->
		physical.name[copy_info->cp_newloc->phys_length] = EOS;

	/* first make sure the parent exists */
	LOfroms( PATH, copy_info->cp_newloc->physical.name, &newloc );
	LOcreate( &newloc );

	/* create the new directory */
	status = DIdircreate(&di_dir, 
			copy_info->cp_newloc->physical.name,
			copy_info->cp_newloc->phys_length,
			file_name, file_length,
			&sys_err);

	/* now add the directory name to the location */
	LOfaddpath( &newloc, file_name, &newloc );
	LOtos( &newloc, &cp );
	STcopy( cp, new_copy_info.cp_newloc->physical.name );
	new_copy_info.cp_newloc->phys_length = STlength( cp );

	/* recurse with the directory name */
	status = DIlistfile(&di_file, 
			new_copy_info.cp_oldloc->physical.name,
			new_copy_info.cp_oldloc->phys_length, 
			cp_di_files, 
			(PTR)&new_copy_info,
			&sys_err);

	/* it's ok if we have reached the end of this directory */
	if ( status == DI_ENDFILE )
	    status = OK;

	/* restore the copy_info structure */
	copy_info->cp_oldloc->phys_length = old_save_len;
	copy_info->cp_newloc->phys_length = new_save_len;
	
	return status;
    }

    /* 
    ** First see exactly how many bytes are in the file. If it is a 
    ** multiple of pagesize, we'll use the DI routines to do the copy.
    ** If it is NOT a multiple of pagesize, we better use SI routines
    ** (Checkpoint files may be compressed by user, therefore may not
    ** be multiple of pagesize bytes)
    */
#ifdef xxx  /* FIX ME */       
#endif

    /* create the new file */
    status = DIcreate(&di_new, copy_info->cp_newloc->physical.name,
			copy_info->cp_newloc->phys_length,
			file_name, file_length,
			pagesize, &sys_err);

    if (status == DI_DIRNOTFOUND)
    {
	char        temp_loc[DB_AREA_MAX + 4];

	for (db_len = 0; db_len < sizeof(*copy_info->cp_dbname); db_len++)
	{
	    if (copy_info->cp_dbname->db_db_name[db_len] == ' ')
		break;
	}

	MEmove(copy_info->cp_loctup->du_l_area, copy_info->cp_loctup->du_area,
		0, sizeof(temp_loc), temp_loc);
	status = LOingpath(temp_loc, 0, copy_info->cp_lo_what, &loc);
	if (!status)
	    LOtos(&loc, &cp);

	if (!status && *cp != EOS)
	{
	    status = DIdircreate(&di_dir, cp, STlength(cp),
			(char *)copy_info->cp_dbname, (u_i4)db_len,
			&sys_err);
	    if (status == OK)
		recreate = TRUE;
	}
    }
    else if (status == DI_EXISTS)
    {
	status = DIdelete(&di_new, copy_info->cp_newloc->physical.name,
			copy_info->cp_newloc->phys_length,
			file_name, file_length,
			&sys_err);
	if (status == OK)
	    recreate = TRUE;
    }

    if (recreate)
    {
	status = DIcreate(&di_new, copy_info->cp_newloc->physical.name,
			copy_info->cp_newloc->phys_length,
			file_name, file_length,
			pagesize, &sys_err);
    } 

    if (status != OK)
    {
	copy_info->cp_status = status;
	uleFormat(NULL, E_DM9002_BAD_FILE_CREATE, &sys_err, ULE_LOG,
		NULL, error_buffer, ER_MAX_LEN, &error_length, &uleError, 5, 
		sizeof (copy_info->cp_dbname), &copy_info->cp_dbname,
		11, "Not a table",
		copy_info->cp_newloc->phys_length,
		copy_info->cp_newloc->physical.name,
		file_length, file_name, 0, 0);
	dmf_put_line(0, error_length, error_buffer);
	return (FAIL);
    }

    do
    {
	/* open the file in the NEW location */
	for (; ;)
	{
	    status = DIopen(&di_new, copy_info->cp_newloc->physical.name, 
		copy_info->cp_newloc->phys_length, file_name, file_length,
		pagesize, DI_IO_WRITE, DI_PRIVATE_MASK, &sys_err);

	    if (status == DI_EXCEED_LIMIT)
	    {
#ifdef xxx /* FIX ME */
		status = dm2t_reclaim_tcb(cnf->cnf_lock_list, &local_dberr);
		if (status == E_DB_OK)
		    continue;
		if (local_dberr.err_code == E_DM9328_DM2T_NOFREE_TCB)
#endif
		    SETDBERR(&local_dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
	    }
	    break;
	} 

	if (status != OK)
	{
	    SETDBERR(&local_dberr, 0, E_DM9004_BAD_FILE_OPEN);
	    break;
	}
	else
	    newopen = TRUE;

	/* open the file in the OLD location */
	for (; ;)
	{
	    status = DIopen(&di_old, copy_info->cp_oldloc->physical.name, 
		copy_info->cp_oldloc->phys_length, file_name, file_length,
		pagesize, DI_IO_READ, 0, &sys_err);

	    if (status == DI_EXCEED_LIMIT)
	    {
#ifdef xxx /* FIX ME */
		status = dm2t_reclaim_tcb(cnf->cnf_lock_list, &local_dberr);
		if (status == E_DB_OK)
		    continue;
		if (local_dberr.err_code == E_DM9328_DM2T_NOFREE_TCB)
#endif
		    SETDBERR(&local_dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
	    }
	    break;
	} 

	if (status != OK)
	{
	    SETDBERR(&local_dberr, 0, E_DM9004_BAD_FILE_OPEN);
	    break;
	}
	else
	    oldopen = TRUE;

	/* Find out how many pages are allocated in the old file */
	status = DIsense(&di_old, &old_endpg, &sys_err);
	if (status != OK)
	{
	    SETDBERR(&local_dberr, 0, E_DM9007_BAD_FILE_SENSE);
	    break;
	}

	/* Try to alloc new file in new location */
	status = DIalloc(&di_new, old_endpg + 1, &page, &sys_err);
	if (status != OK)
	{
	    SETDBERR(&local_dberr, 0, E_DM9000_BAD_FILE_ALLOCATE);
	    break;
	}

	/* Flush the new file */
	status = DIflush(&di_new, &sys_err);
	if (status != OK)
	{
	    SETDBERR(&local_dberr, 0, E_DM9008_BAD_FILE_FLUSH);
	    break; 
	}

	/*
	** Loop reading pages from the old file, writing to the new file
	** Read/write mwrite_blocks pages at a time
	*/
	mwrite_blocks = copy_info->cp_mwrite_blks;

	for (i = 0; i <= old_endpg; i+= page_count)
	{
	    /* 
	    ** Determine how many pages to read/write.
	    ** Use mwrite_blocks unless there are not this many pages
	    ** left in the file
	    */
	    page_count = mwrite_blocks;  /* init */
	    if (i + page_count > old_endpg + 1)
		page_count = old_endpg + 1 - i;
	    save_page_count = page_count;

	    status = DIread(&di_old, &page_count, (i4)i, 
			copy_info->cp_buf, &sys_err);
	    if (status != OK || page_count != save_page_count)
	    {
		SETDBERR(&local_dberr, 0, E_DM9005_BAD_FILE_READ);
		break;
	    }
	    status = DIwrite(&di_new, &page_count, (i4)i,  
			copy_info->cp_buf, &sys_err);
	    if (status != OK || page_count != save_page_count)
	    {
		SETDBERR(&local_dberr, 0, E_DM9006_BAD_FILE_WRITE);
		break;
	    }
	}

    } while (FALSE);

    if (local_dberr.err_code)
    {
	uleFormat(&local_dberr, 0, &sys_err, ULE_LOG,
		NULL, NULL, 0, NULL, &err_code, 5, 
		sizeof (copy_info->cp_dbname), &copy_info->cp_dbname,
		11, "Not a table",
		copy_info->cp_newloc->phys_length,
		copy_info->cp_newloc->physical.name,
		file_length, file_name, 0, 0);
    }

    CLRDBERR(&close_dberr);

    /* Close files used in relocate */
    if (oldopen)
    {
	close_status = DIclose(&di_old, &sys_err);
	if (close_status != OK)
	{
	    SETDBERR(&close_dberr, 0, E_DM9001_BAD_FILE_CLOSE);
	    if (status == OK)
		status = close_status;
	}
    }

    if (newopen)
    {
	close_status = DIforce(&di_new, &sys_err);
	if (close_status != OK)
	{
	    SETDBERR(&close_dberr, 0, E_DM9001_BAD_FILE_CLOSE);
	    if (status == OK)
		status = close_status;
	}
	close_status = DIclose(&di_new, &sys_err);
	if (close_status != OK)
	{
	    SETDBERR(&close_dberr, 0, E_DM9001_BAD_FILE_CLOSE);
	    if (status == OK)
		status = close_status;
	}
    }
    
    if (close_dberr.err_code)
    {
	uleFormat(&close_dberr, 0, &sys_err, ULE_LOG,
		NULL, NULL, 0, NULL, &err_code, 5, 
		sizeof (copy_info->cp_dbname), &copy_info->cp_dbname,
		11, "Not a table",
		copy_info->cp_newloc->phys_length,
		copy_info->cp_newloc->physical.name,
		file_length, file_name, 0, 0);
    }

    if (status == OK && local_dberr.err_code == 0 && close_dberr.err_code == 0)
	copy_info->cp_status = OK;
    else
	copy_info->cp_status = FAIL;

    return (copy_info->cp_status);
}


/*{
** Name: reloc_database          - Relocate a database (make a duplicate copy)
**
** Description:
**
**      This routine is called to make a duplicate copy of a database.
**
**      For each location for this database, all files are copied to
**      the directory for the new database in the same location.
**      As we process each location, we are building a new config file 
**      for the new database. After all files are copied, we insert      
**      an iidatabase tuple for the new database and we insert iiextend
**      tuples for each location we have extended the new database to.
**
** Inputs:
**      jsx                     Pointer to Journal Support info.
**      dcb                     Pointer to DCB for this database.
**      iidcb                   Pointer to the DCB for the iidbdb database.
**      cnf                     Pointer to the config info for this database.
**      copy_info               Pointer to info for copying files (containing
**                                 a preallocated buffer)
**
** Outputs:
**	Returns:
**	    E_DB_OK, E_DB_ERROR
**      err_code		reason for status != E_DB_OK
**
** Side Effects:
**	    none
**
** History:
**        18-nov-1994 (alison)
**           Created for Database relocation
**        09-mar-1995 (stial01)
**           BUG 67375: when relocating a checkpointed database, don't
**           create ckp,dmp,jnl directories or copy ckp,dmp,jnl files.
**           Also fix the config file for the duplicate database to have
**           no checkpoint history.
[@history_template@]...
*/
static DB_STATUS
reloc_database(
DMF_JSX         *jsx,
DMP_DCB         *dcb,
DMP_DCB         *iidcb,
DM0C_CNF        *cnf,
COPY_INFO       *copy_info,
DU_DATABASE     *database,
i4          lock_id,
i4          log_id,
DB_TRAN_ID      *tran_id)
{
    DMP_DCB             *newdcb = 0;
    bool                open_newdb = FALSE;
    i4             newdb_flag;
    i4             lock_mode;
    DB_TAB_TIMESTAMP    stamp;
    DM_TID              tid;
    DB_STATUS           status,close_status,di_status;
    DB_STATUS            local_status;
    DU_DATABASE         newdatabase;
    DI_IO               di_file;
    CL_ERR_DESC         sys_err;
    DB_OWN_NAME         *user_name;
    i4             dmxe_flag;
    i4             compare;
    char                error_buffer[ER_MAX_LEN];
    i4             error_length;
    DMP_EXT             *ext;
    i4             k;
    i4             j;
    DB_TAB_ID           iilocation;
    DB_TAB_ID           iidatabase;
    DU_LOCATIONS        location;
    DB_TAB_ID           iiextend;
    DU_EXTEND           extend;
    DMP_RCB             *loc_rcb = 0;
    DMP_RCB             *ext_rcb = 0;
    DMP_RCB             *db_rcb = 0;
    DM_OBJECT           *newcnf_buffer = 0;
    DM0C_DSC            *newdesc;
    DM0C_JNL            *newjnl;
    DM0C_DMP            *newdmp;
    DM0C_EXT            *newext;
    i4              ext_flags;
    i4                   newdblen, len;
    char                *filename;
    DI_IO               di_cnf;
    DMP_LOC_ENTRY       *root_loc;
    DB_LOC_NAME         *new_locname;
    i4                  n;
    i4             loc_done = 0;
    i4			local_err;		
    DB_ERROR		local_dberr;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Make sure new dbname is different from old dbname
    */
    compare = MEcmp((char *)&jsx->jsx_newdbname, &dcb->dcb_name,
		sizeof(DB_DB_NAME));
    if (compare == 0)
    {
	dmfWriteMsg(NULL, E_DM1508_RELOC_OLDDB_EQ_NEW, 0);
	return (E_DB_ERROR);
    }

    /* Get length of new database name */
    for (newdblen = 0; newdblen < sizeof(jsx->jsx_newdbname.db_db_name);
		newdblen++)
    {
        if (jsx->jsx_newdbname.db_db_name[newdblen] == ' ')
            break;
    }


    iilocation.db_tab_base = DM_B_LOCATIONS_TAB_ID;
    iilocation.db_tab_index = 0;
    iiextend.db_tab_base = DM_B_EXTEND_TAB_ID;
    iiextend.db_tab_index = DM_I_EXTEND_TAB_ID;
    iidatabase.db_tab_base = DM_B_DATABASE_TAB_ID;
    iidatabase.db_tab_index = 0;

    do
    {
	/* 
	** See if there is already an iidatabase tuple with name = newdbname
	** If so, the new db already exists
	*/
	status = get_iidatabase(jsx, iidcb, &jsx->jsx_newdbname, &newdatabase,
			0, 0, RELOC_READ, log_id, tran_id, lock_id);
	if (status == E_DB_OK)
	{
	    status = E_DB_ERROR;
	    dmfWriteMsg(NULL, E_DM0124_DB_EXISTS, 0);
	    break;
	}
	else
	{
	    /* FIX ME, make sure errcode because positioning failed */
	    /* 
	    ** Build an iidatabase tuple for newdb
	    */
	    MEcopy(database, sizeof(DU_DATABASE), &newdatabase);
	    MEcopy(jsx->jsx_newdbname.db_db_name, sizeof(newdatabase.du_dbname),
			newdatabase.du_dbname);
	}

	/*
	** Allocate a buffer to copy the in-memory copy of the config file
	** We need to fix the location entries   
	*/
	status = dm0m_allocate(cnf->cnf_bytes + sizeof(DMP_MISC),
			(i4)0, (i4)MISC_CB, (i4)MISC_ASCII_ID,
			(char *)NULL, &newcnf_buffer, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	{
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    dmfWriteMsg(NULL, E_DM150A_RELOC_MEM_ALLOC, 0);
	    break;
	}
	newdesc = (DM0C_DSC *)((char *)newcnf_buffer + sizeof(DMP_MISC));
	((DMP_MISC *)newcnf_buffer)->misc_data = (char *)newdesc; 
	MEcopy(cnf->cnf_data, cnf->cnf_bytes,
		(char *)((DMP_MISC *)newcnf_buffer)->misc_data);
	newjnl = (DM0C_JNL *)(newdesc + 1);
	newdmp = (DM0C_DMP *)(newjnl + 1);
	newext = (DM0C_EXT *)(newdmp + 1);
	
	/* 
	** If old/new location lists specified:
	**    - Check location names in jsx_loc_list and jsx_nloc_list
	**    - Don't allow multiple locations map to the same location
	**      This is a temporary restriction, we don't want the
	**      number of entries in the config to change
	**    - Make sure relocation is valid for every table    
	*/
	if (jsx->jsx_status1 & JSX_LOC_LIST)
	{
	    status = reloc_check_loc_lists(jsx, dcb, iidcb,
			log_id, tran_id, lock_id);
	    if (status != E_DB_OK)
	    {
		/* specific error formatted in reloc_check_loc_lists() */
		break;
	    }

	    status = reloc_check_newlocs(jsx, dcb);
	    if (status != E_DB_OK)
	    {
		/* specific error formatted in reloc_check_newlocs */
		break;
	    }

	    status = reloc_loc_process(jsx, dcb, RELOC_PREPARE_LOCATIONS);

	    if (status != E_DB_OK)
	    {
		/* specific error formatted in reloc_loc_process */
		break;
	    }
	}

	/* Open the iilocations table */
	status = dm2t_open(iidcb, &iilocation, DM2T_IS, DM2T_UDIRECT, 
		DM2T_A_READ, 0, 20, 0, log_id, lock_id,
		0, 0, 0, tran_id, &stamp, &loc_rcb, (DML_SCB *)0, &jsx->jsx_dberr);

	if (status != E_DB_OK)
	{
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    break;
	}

	/*
	** Process each extent for the database:
	**    Check if this location is being relocated, if yes get new 
	**    logical location name.
	**    Using the logical location name, get the area name from
	**    the iilocations table.
	**    Using the area name from the iilocations table, build the
	**    location name for the new database. (build_loc() which calls
	**    LOingpath().
	**    Copy all files to the new location, cp_di_files() will create
	**    the directory if necessary.
	**    Adjust extent information in config file for new database
	*/
	ext = dcb->dcb_ext;
	root_loc = 0;
	for (j = 0; j < ext->ext_count; j++)
	{
	    /* 
	    ** If loc/newloc list specified, see if this location
	    ** is on the (old)loc list
	    **
	    ** If it is, get the newloc and copy the files there instead.
	    */
	    if (jsx->jsx_status1 & (JSX_LOC_LIST))
	    {
		if ( reloc_this_loc(jsx, &ext->ext_entry[j].logical,
					&new_locname))
		    /* Now move new loc name into ext array */
		    MEcopy(new_locname, sizeof(DB_LOC_NAME),
				&newext[j].ext_location.logical);
	    }

	    /*
	    ** Remember the ROOT location so we can write our
	    ** new config file there
	    */
	    if (newext[j].ext_location.flags & DCB_ROOT)
                root_loc = &newext[j].ext_location;

	    /* Get the iilocation tuple for the target location */
	    status = get_iilocation(jsx, iidcb, loc_rcb, 
				&newext[j].ext_location.logical, 
				&location, log_id, tran_id, lock_id);
	    if (status != E_DB_OK)
	    {
		dmfWriteMsg(NULL, E_DM1023_JSP_NO_LOC_INFO, 0);
		break;
	    }

	    /* Get the usage from dcb (or iiextend) NOT iilocation */
	    if (ext->ext_entry[j].flags & DCB_JOURNAL)
		copy_info->cp_lo_what = LO_JNL;
	    else if (ext->ext_entry[j].flags & (DCB_WORK | DCB_AWORK))
		copy_info->cp_lo_what = LO_WORK;
	    else if (ext->ext_entry[j].flags & DCB_DATA)
		copy_info->cp_lo_what = LO_DB;
	    else if (ext->ext_entry[j].flags & DCB_CHECKPOINT)
		copy_info->cp_lo_what = LO_CKP;
	    else if (ext->ext_entry[j].flags & DCB_DUMP)
		copy_info->cp_lo_what = LO_DMP;

	    if (build_loc(&jsx->jsx_newdbname, location.du_l_area,
			location.du_area, copy_info->cp_lo_what,
			&newext[j].ext_location) != E_DB_OK)
	    {
		status = E_DB_ERROR;
		break;
	    }

	    copy_info->cp_oldloc = &ext->ext_entry[j];;
	    copy_info->cp_newloc = &newext[j].ext_location;
	    copy_info->cp_usage = ext->ext_entry[j].flags; 
	    copy_info->cp_status = OK;
	    copy_info->cp_loctup = &location;
	    copy_info->cp_dbname = &jsx->jsx_newdbname;

	    /*
	    ** Don't create directories or copy files for
	    ** jnl,ckp,dmp locations
	    */
	    if ( STEQ_MACRO( copy_info->cp_lo_what, LO_JNL ) )
		continue;   /* dont copy files for jnl locations */

	    if ( STEQ_MACRO( copy_info->cp_lo_what, LO_CKP ) )
		continue;   /* dont copy files for ckp locations */

	    if ( STEQ_MACRO( copy_info->cp_lo_what, LO_DMP ) )
		continue;   /* dont copy files for dmp locations */

	    /*
	    ** Always create the directory for this location 
	    ** If the directory exists, make sure it is empty
	    */
	    status = reloc_db_dircreate(jsx, copy_info);
	    if (status != E_DB_OK)
	    {
		dmfWriteMsg(NULL, E_DM011A_ERROR_DB_DIR, 0);
		break;
	    }

	    /*
	    ** Increment count of locations processed.
	    ** If an error occurs we need to delete any files created.
	    */
	    loc_done++;

	    if ( STEQ_MACRO( copy_info->cp_lo_what, LO_WORK ) )
		continue;   /* dont copy files for work locations */

	    di_status = DIlistfile(&di_file, ext->ext_entry[j].physical.name,
		ext->ext_entry[j].phys_length, cp_di_files, (PTR)copy_info,
		&sys_err);
	    if (di_status == DI_DIRNOTFOUND)
		continue;  /* this is really only ok for JNL/DMP/CKP */
	    if ((di_status != DI_ENDFILE) || (copy_info->cp_status != OK))
	    {
		status = E_DB_ERROR;
		SETDBERR(&jsx->jsx_dberr, 0, E_DM150B_RELOC_CP_FI_ERR);
		uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    error_buffer, ER_MAX_LEN, &error_length, &local_err, 3,
		    sizeof (DB_LOC_NAME), &copy_info->cp_oldloc->logical, 
		    sizeof (DB_LOC_NAME), &copy_info->cp_newloc->logical,
		    0, 0);
		dmf_put_line(0, error_length, error_buffer);
		break;
	    }

	} /* end for looping through locations, copying files */

	if (status != E_DB_OK)
	    break;

	/*
	** If any default locations were relocated, we need to update
	** iidatabase.ckpdev, iidatabase.dbdev, iidatabase.dmpdev,
	** iidatabase.jnldev and or iidatabase.sortdev for the new database.
	*/
	if (jsx->jsx_status1 & JSX_LOC_LIST)
	{
	    if (reloc_this_loc(jsx, &newdatabase.du_ckploc, &new_locname))
		MEcopy((PTR)new_locname, sizeof(DB_LOC_NAME),
			&newdatabase.du_ckploc);
	    if (reloc_this_loc(jsx, &newdatabase.du_dbloc, &new_locname))
		MEcopy((PTR)new_locname, sizeof(DB_LOC_NAME),
			&newdatabase.du_dbloc);
	    if (reloc_this_loc(jsx, &newdatabase.du_dmploc, &new_locname))
		MEcopy((PTR)new_locname, sizeof(DB_LOC_NAME),
			&newdatabase.du_dmploc);
	    if (reloc_this_loc(jsx, &newdatabase.du_jnlloc, &new_locname))
		MEcopy((PTR)new_locname, sizeof(DB_LOC_NAME),
			&newdatabase.du_jnlloc);
	    if (reloc_this_loc(jsx, &newdatabase.du_workloc, &new_locname))
		MEcopy((PTR)new_locname, sizeof(DB_LOC_NAME),
			&newdatabase.du_workloc);
	}

	/*
	** Try to create a new database the way it is done by 
	** the createdb utility (dbutil/duf/duc)
	** FIX ME: will this put ever fail???
	*/
	newdatabase.du_dbid = TMsecs();
	status = dm2t_open(iidcb, &iidatabase, DM2T_IX, DM2T_UDIRECT,
                DM2T_A_WRITE, 0, 20, 0, log_id, lock_id,
                0, 0, DM2T_IX, tran_id, &stamp, &db_rcb, (DML_SCB *)0, 
		&jsx->jsx_dberr);
	if (status != E_DB_OK)
	{
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    break;
	}

	status = dm2r_put(db_rcb, 0, (char *)&newdatabase, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	{
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    break;
	}

	/* 
	** Open iiextend catalog
	**  (this time for update)
	*/
	status = dm2t_open(iidcb, &iiextend, DM2T_IX, DM2T_UDIRECT,
                DM2T_A_WRITE, 0, 20, 0, log_id, lock_id,
                0, 0, DM2T_IX, tran_id, &stamp, &ext_rcb, (DML_SCB *)0, 
		&jsx->jsx_dberr);

	if (status != E_DB_OK)
	{
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    break;
	}

	/*
	** Insert records in iiextend for each location newdb has been
	** extended to.
	*/
	for (j = 0; j < ext->ext_count; j++)
	{

	    /*
	    ** Fetch the iiextend tuple for the old location
	    **      (Except if usage is jnl/dmp/ckp, in which case 
	    **          there is no iiextend record)
	    **     The fetch is not really necessary... just checking
	    **     that iiextend catalog matches the config file
	    ** Update the dname (and maybe the lname) and insert
	    ** the new iiextend record
	    */
	    if (ext->ext_entry[j].flags & (DCB_JOURNAL | DCB_CHECKPOINT 
						| DCB_DUMP))
		continue;

	    ext_flags = 0;
	    if (ext->ext_entry[j].flags & DCB_WORK)
		ext_flags |= DU_EXT_WORK;
	    if (ext->ext_entry[j].flags & DCB_AWORK)
		ext_flags |= DU_EXT_AWORK;
	    if (ext->ext_entry[j].flags & DCB_DATA)
		ext_flags |= DU_EXT_DATA;
	    status = get_iiextend(jsx, iidcb, &dcb->dcb_name, 
			&ext->ext_entry[j].logical, ext_flags, ext_rcb, 
                        &extend, log_id, tran_id, lock_id);
	    if (status != E_DB_OK)
	    {
		uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    error_buffer, ER_MAX_LEN, &error_length, &local_err, 2,
		    sizeof (DB_LOC_NAME), &ext->ext_entry[j].logical, 0, 0);
		dmf_put_line(0, error_length, error_buffer);
		break;
	    }

	    MEmove(newdblen, (PTR)&jsx->jsx_newdbname, ' ',
		sizeof extend.du_dname, (PTR)extend.du_dname);
	    extend.du_d_length = newdblen;
	    for (len = 0; len < sizeof newext[j].ext_location.logical; len++)
	    {
		if (newext[j].ext_location.logical.db_loc_name[len] == ' ')
		    break;
	    }
	    MEmove(len, (PTR)&newext[j].ext_location.logical, ' ',
		sizeof extend.du_lname, extend.du_lname);
	    extend.du_l_length = len;
	    /* FIX ME flag parm for put */
	    status = dm2r_put(ext_rcb, 0, (char *)&extend, &jsx->jsx_dberr); 
	    if (status != E_DB_OK)
	    {
		dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		break;
	    }
	}

	if (status != E_DB_OK)
	{
	    break;
	}

	/* 
	** Fix up some more fields in the config file 
	*/
	newdesc->dsc_dbid = newdatabase.du_dbid;
	newdesc->dsc_open_count = 0;
	MEcopy(jsx->jsx_newdbname.db_db_name, sizeof newdesc->dsc_name,
		&newdesc->dsc_name);

	/*
	** BUG 67375
	** We didn't copy CKP,JNL,DMP  directories
	** Erase checkpoint history from config file
	*/
	newdesc->dsc_status &= ~(DSC_CKP_INPROGRESS | DSC_DUMP |
				 DSC_ROLL_FORWARD | DSC_PARTIAL_ROLL |
				 DSC_DUMP_DIR_EXISTS | DSC_JOURNAL |
				 DSC_DISABLE_JOURNAL);
	newjnl->jnl_count = 0;
	newjnl->jnl_ckp_seq = 0;
	newjnl->jnl_fil_seq = 0;
	newjnl->jnl_blk_seq = 0;
	newjnl->jnl_first_jnl_seq = 0;
	newjnl->jnl_la.la_sequence = 0;
	newjnl->jnl_la.la_block    = 0;
	newjnl->jnl_la.la_offset   = 0;
	newdmp->dmp_count = 0;
	newdmp->dmp_ckp_seq = 0;
	newdmp->dmp_fil_seq = 0;
	newdmp->dmp_blk_seq = 0;
	newdmp->dmp_la.la_sequence = 0;
	newdmp->dmp_la.la_block    = 0;
	newdmp->dmp_la.la_offset   = 0;
	for (j = 0; j < DM_CNODE_MAX; j++)
	{
	    struct _JNL_CNODE_INFO *p = 0;
	    p = &newjnl->jnl_node_info[j];
	    p->cnode_fil_seq = 0;
	    p->cnode_la.la_sequence = 0;
	    p->cnode_blk_seq      = 0;
	    p->cnode_la.la_block  = 0;
	    p->cnode_la.la_offset = 0;
	}

	/*
	** Write out our new config file (in newcnf buffer)
	*/
	if (!root_loc)
	{
	    status = E_DB_ERROR;
	    dmfWriteMsg(NULL, E_DM923A_CONFIG_OPEN_ERROR, 0);
	    break;
	}
	filename = "aaaaaaaa.cnf";
	status = DIopen(&di_cnf, (char *)&root_loc->physical, 
			root_loc->phys_length,
			filename, (i4)12, (i4)DM_PG_SIZE,
			DI_IO_WRITE, DI_SYNC_MASK, &sys_err);
	if (status != E_DB_OK)
	{
	    /* FIX ME (what about EXCEED TCB LIMIT */
	    dmfWriteMsg(NULL, E_DM923A_CONFIG_OPEN_ERROR, 0);
	    break;
	}
	n = cnf->cnf_bytes / DM_PG_SIZE;
	status = DIwrite(&di_cnf, &n, 0, 
			(char *)((DMP_MISC *)newcnf_buffer)->misc_data,
			&sys_err);
	if (status != OK)
	{
	    /* FIX ME */
	    dmfWriteMsg(NULL, E_DM923B_CONFIG_CLOSE_ERROR, 0);
	    break;
	}
	status = DIclose(&di_cnf, &sys_err);
	if (status != OK)
	{
	    dmfWriteMsg(NULL, E_DM923B_CONFIG_CLOSE_ERROR, 0);
	    break;
	}

	/*
	** Alloc and init a dcb for the new database 
	*/
	status = alloc_newdcb(jsx, dcb, &newdcb, root_loc, &newdatabase);

	if (status != E_DB_OK)
	    break;

	/* 
	** See if we can open the new db we created 
	*/
	newdb_flag = 0;
	status = dm2d_open_db(newdcb, DM2D_A_WRITE, DM2D_X,
                dmf_svcb->svcb_lock_list, newdb_flag, &jsx->jsx_dberr);

	if (status != E_DB_OK)
	{
	    dmfWriteMsg(NULL, E_DM1512_RELOC_CANT_OPEN_NEWDB, 0);
	    break;
	}

	open_newdb = TRUE;

	/*
	** If we relocated and non-default data locations, we need
	** to update iirelation.relloc and iidevices.devloc in the
	** new database
	*/
	if (jsx->jsx_status1 & JSX_NLOC_LIST)
	{
	    status = reloc_loc_process(jsx, newdcb, RELOC_FINISH_LOCATIONS);
	}

    } while (FALSE);

    /* Close new database */
    if (open_newdb)
    {
	close_status = dm2d_close_db(newdcb, dmf_svcb->svcb_lock_list,
			newdb_flag, &local_dberr); 
	if (close_status != E_DB_OK)
	{ 
	    dmfWriteMsg(&local_dberr, 0, 0);
	    if (close_status > status)
	    {
		status = close_status;
		jsx->jsx_dberr = local_dberr;
	    }
	}
    }

    /* Free the DCB memory used for the new database open */ 
    if (newdcb)
	dm0m_deallocate((DM_OBJECT **)&newdcb);

    /* Close iilocations table */
    if (loc_rcb)
    {
	close_status = dm2t_close(loc_rcb, 0, &local_dberr);
	if (close_status != E_DB_OK)
	{
	    dmfWriteMsg(&local_dberr, 0, 0);
	    if (close_status > status)
	    {
		status = close_status;
		jsx->jsx_dberr = local_dberr;
	    }
	}
    }

    /* Close iiextend table */
    if (ext_rcb)
    {
	close_status = dm2t_close(ext_rcb, 0, &local_dberr);
	if (close_status != E_DB_OK)
	{
	    dmfWriteMsg(&local_dberr, 0, 0);
	    if (close_status > status)
	    {
		status = close_status;
		jsx->jsx_dberr = local_dberr;
	    }
	}
    }

    /* Close iidatabase table */
    if (db_rcb)
    {
	close_status = dm2t_close(db_rcb, 0, &local_dberr);
	if (close_status != E_DB_OK)
	{
	    dmfWriteMsg(&local_dberr, 0, 0);
	    if (close_status > status)
	    {
		status = close_status;
		jsx->jsx_dberr = local_dberr;
	    }
	}
    }

    if (status != E_DB_OK)
    {
	/* Delete any files/directories we created for the new database */
	for (j = 0; j < loc_done; j++)
	{
	    if ( !(newext[j].ext_location.flags & DCB_WORK))
	    {
		di_status = DIlistfile(&di_file, 
				newext[j].ext_location.physical.name,
				newext[j].ext_location.phys_length,
				delete_files, (PTR)&newext[j].ext_location,
				&sys_err);
		if (di_status != DI_ENDFILE && di_status != DI_DIRNOTFOUND)
		{
		    dmfWriteMsg(NULL, E_DM1509_RELOC_DEL_NEWFI_ERR, 0);
		}
	    }

	    /* Now delete the directory we created */
	    di_status = DIdirdelete(&di_file,
				newext[j].ext_location.physical.name,
				newext[j].ext_location.phys_length,
				0, 0, &sys_err);

	}
    }

    if (newcnf_buffer)
	dm0m_deallocate((DM_OBJECT **)&newcnf_buffer);

    return (status);
     
}


/*{
** Name: get_iiextend            - Get iiextend tuple for specified db,loc
**
** Description:
**
**      This routine will get the iiextend tuple for the specified 
**      database and location.
**
** Inputs:
**      jsx                     Pointer to Journal Support info.
**      dcb                     Pointer to DCB for the iidbdb database.
**      dbname                  Pointer to the name of the database we are
**                                 searching for.
**      lname                   Pointer to the location name we are searching
**                                 for.
**      ext_flags               location usage flags (DU_EXT_DATA,
**                                 DU_EXT_WORK, DU_EXT_AWORK) 
**      rcb                     Pointer to the RCB for the iiextend table.
**      log_id                  Log id.
**      tran_id                 Transaction id.
**      lock_id                 Lock id.
**
** Outputs:
**	Returns:
**	    E_DB_OK, E_DB_ERROR
**      extend                  iiextend tuple returned.
**      err_code		reason for status != E_DB_OK
**
** Side Effects:
**	    none
**
** History:
**        18-nov-1994 (alison)
**           Created for Database relocation
[@history_template@]...
*/
static DB_STATUS 
get_iiextend(
DMF_JSX             *jsx,
DMP_DCB             *dcb,
DB_DB_NAME          *dbname, 
DB_LOC_NAME         *lname,
i4              ext_flags,
DMP_RCB             *rcb,
DU_EXTEND           *extend,
i4              log_id,
DB_TRAN_ID          *tran_id,
i4              lock_id)
{
    DB_STATUS           status;
    DM_TID              tid;
    DM2R_KEY_DESC       key_list[1];
    struct
    {
	i2              count;              /* length of ... */
	char            name[DB_MAXNAME];   /* A dbname. */
    }                   d_name;
    char		pad_lname[DB_MAXNAME];
    char buf[1000];
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    key_list[0].attr_number = DM_1_EXTEND_KEY;
    key_list[0].attr_operator = DM2R_EQ;
    key_list[0].attr_value = (char *)&d_name;
    MEcopy( (PTR)dbname, DB_MAXNAME, (PTR)d_name.name);
    d_name.count = DB_MAXNAME;

    if (jsx->jsx_status & JSX_VERBOSE)
    {
	STprintf(buf, "get iiextend %32.32s %32.32s (%x)\n", 
		    lname->db_loc_name, dbname, ext_flags);
	dmf_put_line(0, STlength(buf), buf);
    }

    status = dm2r_position(rcb, DM2R_QUAL, key_list, 1, &tid, &jsx->jsx_dberr);

    for ( ;status == E_DB_OK;)
    {
	if (status == E_DB_OK)
	{
	    status = dm2r_get(rcb, &tid, DM2R_GETNEXT, (char *)extend,
			&jsx->jsx_dberr);
	    if (status != E_DB_OK)
		break;

	    if (jsx->jsx_status & JSX_VERBOSE)
	    {
		STprintf(buf, "compare (%d) '%s' du_status %x\n", 
		    extend->du_l_length, extend->du_lname, extend->du_status);
		dmf_put_line(0, STlength(buf), buf);
	    }

	    /* varchar extend->du_lname must be padded before comparing */
	    MEfill(DB_MAXNAME, ' ', pad_lname);
	    MEcopy(extend->du_lname, extend->du_l_length, pad_lname);
	    if ( (MEcmp((char *)(lname->db_loc_name), pad_lname,
			DB_MAXNAME) == 0)
		 && (extend->du_status & ext_flags))
	      break;
	}
    }
    if ((jsx->jsx_status & JSX_VERBOSE) && status != E_DB_OK)
    {
	STprintf(buf, "NOT FOUND\n");
	dmf_put_line(0, STlength(buf), buf);
    }

    return (status);

}


/*{
** Name: delete_files	- Delete all alias file in a directory.
**
** Description:
**      This routine is called to delete all alias files and all temporary
**	table files in a directory. 
**
**	The following files will be deleted:
**
**		aaaaaaaa.cnf
**		aaaaaaaa.tab - ppp00000.tab	    Database  tables.
**		ppppaaaa.tab - pppppppp.tab	    Temporary tables.
**		*.m*				    Modify temps.
**		*.d*				    Deleted files.
**		*.ali				    Alias files.
**		*.sm*				    Sysmod temps.
**		Any file with wrong name	    User created junk.
**
** Inputs:
**      arg_list                        Agreed upon argument.
**	filename			Pointer to filename.
**	filelength			Length of file name.
**
** Outputs:
**      sys_err				Operating system error.
**	Returns:
**	    OK
**	    DI_ENDFILE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
[@history_template@]...
*/
static DB_STATUS
delete_files(
DMP_LOC_ENTRY	    *location,
char		    *filename,
i4		    l_filename,
CL_ERR_DESC	    *sys_err)
{
    DB_STATUS	    status;
    DB_STATUS	    err_code;
    DI_IO	    di_file;
    LOCATION	    oldloc,loc;
    char            *cp;
    char            full_name[MAX_LOC];
    i4              flagword;
    LOINFORMATION   loinf;
    LO_DIR_CONTEXT  lc;

    /* see if it's a directory */
    MEcopy( location->physical.name, location->phys_length, full_name );
    full_name[location->phys_length] = EOS;
    LOfroms( PATH, full_name, &oldloc );
    filename[l_filename] = EOS;
    LOfaddpath( &oldloc, filename, &oldloc );
    flagword = LO_I_TYPE;
    if ((LOinfo(&oldloc, &flagword, &loinf) == OK) &&
        (loinf.li_type == LO_IS_DIR))
    {
	/* Yes It's a directory. Kill everything in it. */

	LOtos(&oldloc, &cp);
	LOfroms( PATH, cp, &loc );
	if (LOwcard(&loc, NULL, NULL, NULL, &lc) == OK)
	    do
	    {
		status = LOdelete( &loc );
	    } while (status == OK && LOwnext(&lc, &loc) == OK);
	LOwend (&lc);
	status = LOdelete( &oldloc );
    }
    else
    {
	/* Not a directory */
        
	status = DIdelete(&di_file, (char *)&location->physical,
			location->phys_length, filename, l_filename,
			sys_err);
    }
    if (status == OK)
	return (status);

    uleFormat(NULL, E_DM9003_BAD_FILE_DELETE, sys_err, ULE_LOG, NULL, 
	    NULL, 0, NULL, 
            &err_code, 4,
	    4, "None",
	    4, "None",
	    location->phys_length, &location->physical,
	    l_filename, filename);
    return (DI_ENDFILE);
}

/*{
** Name: config_handler	- Handle configuration file operations.
**
** Description:
**      This routine manages the opening/closing and updating of
**	the configuration file.
**
** Inputs:
**	operation			Config operation.
**          UPDTCOPY_CONFIG             Update the config file and make
backup

**      dcb                             Pointer to DCB for database.
**	cnf				Pointer to CNF pointer.
**
** Outputs:
*	err_code			Reason for error return.
**	Returns:
**	    E_DB_OK
**	    E_DB_INFO			If .rfc open and it doesn't exist.
**	    E_DB_ERROR
**	Exceptions:
**
**
** Side Effects:
**
**
** History:
**      09-mar-1987 (Derek)
**          Created for Jupiter.
**	02-feb_1989 (EdHsu)
**	    Online backup support.
**	17-may-90 (bryanp)
**	    Added UPDTCOPY_CONFIG argument to update config and make backup.
**	    Added 'default' to switch to catch programming errors.
**	20-sep-1993 (andys)
**	     Fix arguments to ule_format call.
[@history_template@]...
*/
static DB_STATUS
config_handler(
DMF_JSX		    *jsx,
i4             operation,
DMP_DCB		    *dcb,
DM0C_CNF	    **config)
{
    DM0C_CNF		*cnf = *config;
    i4		i;
    i4		open_flags = 0;
    DB_STATUS		status;
    i4			local_err;
    DB_ERROR		local_dberr;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);
    
    switch (operation)
    {
    case OPEN_CONFIG:

	/*  Open the configuration file. */

	*config = 0;
	status = dm0c_open(dcb, open_flags, dmf_svcb->svcb_lock_list, 
                           config, &jsx->jsx_dberr);
	if (status == E_DB_OK)
	{
	    if (dcb->dcb_ext)
		return (E_DB_OK);

	    cnf = *config;
	    
	    /*  Allocate and initialize the EXT. */

	    status = dm0m_allocate((i4)(sizeof(DMP_EXT) +
		cnf->cnf_dsc->dsc_ext_count * sizeof(DMP_LOC_ENTRY)),
		0, (i4)EXT_CB,	(i4)EXT_ASCII_ID, (char *)dcb,
		(DM_OBJECT **)&dcb->dcb_ext, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	    {
		uleFormat(&jsx->jsx_dberr, 0, 0, ULE_LOG, NULL, 
			NULL, 0, NULL, &local_err, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM150A_RELOC_MEM_ALLOC);
		break;
	    }

	    dcb->dcb_ext->ext_count = cnf->cnf_dsc->dsc_ext_count;
	    for (i = 0; i < cnf->cnf_dsc->dsc_ext_count; i++)
	    {
		STRUCT_ASSIGN_MACRO(cnf->cnf_ext[i].ext_location, 
                                    dcb->dcb_ext->ext_entry[i]);
		if (cnf->cnf_ext[i].ext_location.flags & DCB_JOURNAL)
		    dcb->dcb_jnl = &dcb->dcb_ext->ext_entry[i];
		else if (cnf->cnf_ext[i].ext_location.flags & DCB_CHECKPOINT)
		    dcb->dcb_ckp = &dcb->dcb_ext->ext_entry[i];
		else if (cnf->cnf_ext[i].ext_location.flags & DCB_DUMP)
		    dcb->dcb_dmp = &dcb->dcb_ext->ext_entry[i];
		else if (cnf->cnf_ext[i].ext_location.flags & DCB_ROOT)
		{
		    dcb->dcb_root = &dcb->dcb_ext->ext_entry[i];
		    dcb->dcb_location = dcb->dcb_ext->ext_entry[i];
		}
	    }
	
	    return (E_DB_OK);
	}
	if (status == E_DB_INFO)
	    return (status);

	if (*config)
	    dm0c_close(*config, 0, &local_dberr);
	*config = 0;
	dmfWriteMsg(NULL, E_DM150F_RELOC_CNF_OPEN, 0);
	return (E_DB_ERROR);

    case CLOSEKEEP_CONFIG:
	status = dm0c_close(cnf, 0, &jsx->jsx_dberr);

	if (status == E_DB_OK)
	{
	    *config = 0;
	    return (E_DB_OK);
	}
	dmfWriteMsg(NULL, E_DM1510_RELOC_CNF_CLOSE, 0);
	return (E_DB_ERROR);

    case UNKEEP_CONFIG:

	dm0m_deallocate((DM_OBJECT **)&dcb->dcb_ext);
	dcb->dcb_root = 0;
	dcb->dcb_jnl = 0;
	dcb->dcb_dmp = 0;
	dcb->dcb_ckp = 0;
	return (E_DB_OK);
	    
    case CLOSE_CONFIG:

	if (*config == 0)
	    return (E_DB_OK);
	status = dm0c_close(cnf, 0, &jsx->jsx_dberr);

	if (status == E_DB_OK)
	{
	    if (dcb->dcb_ext)
	    {
		/*	Deallocate the EXT. */

		dm0m_deallocate((DM_OBJECT **)&dcb->dcb_ext);
		dcb->dcb_root = 0;
		dcb->dcb_jnl = 0;
		dcb->dcb_dmp = 0;
		dcb->dcb_ckp = 0;
	    }
	    *config = 0;
	    return (status);
	}

	dmfWriteMsg(NULL, E_DM1510_RELOC_CNF_CLOSE, 0);
	return (E_DB_ERROR);

    case UPDATE_CONFIG:

	/*  Write configuration file to disk. */

	status = dm0c_close(cnf, DM0C_UPDATE | DM0C_LEAVE_OPEN, &jsx->jsx_dberr);

	if (status == E_DB_OK)
	    return (status);

	dmfWriteMsg(NULL, E_DM1510_RELOC_CNF_CLOSE, 0);
	return (E_DB_ERROR);

    case UPDTCOPY_CONFIG:

	/*  Write configuration file to disk and make backup copy */

	status = dm0c_close(cnf, DM0C_UPDATE | DM0C_LEAVE_OPEN | DM0C_COPY,
			    &jsx->jsx_dberr);

	if (status == E_DB_OK)
	    return (status);

	uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, 
			NULL, 0, NULL, &local_err, 0);
        SETDBERR(&jsx->jsx_dberr, 0, E_DM1510_RELOC_CNF_CLOSE);
	return (E_DB_ERROR);

    default:
	/* programming error */

	dmfWriteMsg(NULL, E_DM1510_RELOC_CNF_CLOSE, 0);
	return (E_DB_ERROR);
    }
    return (status);
}


/*{
** Name: reloc_tbl_process       - Do table relocate prepare/finish operations
**
** Description:
**
**      This routine will see if the requested locations are valid for all
**      the tables specified. Note the table_id's for the table names in
**      jsx_tbl_list are available in the rfp table control blocks.
**
** Inputs:
**      rfp                     Rollforward context. 
**      jsx                     Pointer to Journal Support info.
**      u_i4               phase
**      dcb                     Pointer to DCB for this database.
**
** Outputs:
**	Returns:
**	    E_DB_OK, E_DB_ERROR
**      err_code		reason for status != E_DB_OK
**
** Side Effects:
**	    none
**
** History:
**        18-nov-1994 (alison)
**           Created for Database relocation
**	10-mar-95 (stial01)
**	    Fix 67411.
**	 3-jul-96 (nick)
**	    Above fix for 67411 screwed the error handling and caused
**	    #77513.
[@history_template@]...
*/
DB_STATUS
reloc_tbl_process(
DMF_RFP             *rfp,
DMF_JSX             *jsx,
DMP_DCB             *dcb,
u_i4           phase)
{
    DB_TAB_ID           table_id;
    RFP_QUEUE		*totq;
    RFP_TBLCB		*tblcb = 0;
    RFP_TBLHCB		*hcb = rfp->rfp_tblhcb_ptr;
    DM2R_KEY_DESC       qual_list[1];
    DMP_RCB             *rel_rcb = 0;
    DMP_RCB             *dev_rcb = 0;
    DB_STATUS		status, close_status;
    DB_TRAN_ID          tran_id;
    DB_TAB_TIMESTAMP    stamp;
    DMP_RELATION        relrecord;
    DM_TID		tid;
    i4             lock_mode;
    i4             req_access_mode;
    i4             i;
    char                error_buffer[ER_MAX_LEN];
    i4             error_length;
    DMP_LOC_ENTRY       *oldloc, *newloc;
    i4             lock_id;
    i4             log_id = 0;
    i4             dmxe_flag = DMXE_JOURNAL;
    DB_OWN_NAME         username;
    i4             dbdb_flag = 0;
    char                *cp = (char *)&username;
    DB_LOC_NAME         *loc_name;
    i4			local_err;
    DB_ERROR		local_dberr;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    do
    {
	if (phase == RELOC_PREPARE_TABLES)
	{
	    dbdb_flag = DM2D_NOLOGGING;

	    lock_mode = DM2T_IS;
	    req_access_mode = DM2T_A_READ;

	    tran_id.db_high_tran = 0; /* FIXME what should this be */
	    tran_id.db_low_tran = 0;
	    lock_id = dmf_svcb->svcb_lock_list;
	    log_id = 0;
	}
	else
	{
	    dbdb_flag = DM2D_NLK;

	    lock_mode = DM2T_IX;
	    req_access_mode = DM2T_A_WRITE;

	    if (jsx->jsx_status & JSX_JOFF)
		dmxe_flag = 0;

	    IDname(&cp);
	    MEmove(STlength(cp), cp, ' ', sizeof(username), 
			username.db_own_name);
	}

	if ((jsx->jsx_status & JSX_WAITDB) == 0)
	    dbdb_flag |= DM2D_NOWAIT;

	status = rfp_open_database(rfp, jsx, dcb, dbdb_flag);
	if (status != E_DB_OK)
	{
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    break;
	}

	/*
	** Validate that all location names in location/new_location
	** lists are valid data locations
	*/
	for (i = 0; i < jsx->jsx_loc_cnt && status == E_DB_OK; i++)
	{
	    loc_name = &jsx->jsx_loc_list[i].loc_name;
	    status = find_dcb_ext(jsx, dcb, loc_name, DCB_DATA, &oldloc);
	    if (status == E_DB_OK)
	    {
		loc_name = &jsx->jsx_nloc_list[i].loc_name;
		status = find_dcb_ext(jsx, dcb, loc_name, DCB_DATA, &newloc);
	    }
	}

	if (status != E_DB_OK)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM150C_RELOC_LOC_CONFIG_ERR);
	    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		error_buffer, ER_MAX_LEN, &error_length, err_code, 2,
		sizeof (DB_LOC_NAME), loc_name, 0, 0);
	    dmf_put_line(0, error_length, error_buffer);
	    /* already logged */
	    jsx->jsx_dberr.err_code = 0;
	    break;
	}

	if (phase == RELOC_FINISH_TABLES)
	{
	    status = dmf_rfp_begin_transaction(DMXE_WRITE,
			dmxe_flag, dcb, dcb->dcb_log_id, &username,
			dmf_svcb->svcb_lock_list, 
			&log_id, &tran_id, &lock_id, &jsx->jsx_dberr);

	    if (status != E_DB_OK)
		break;
	}

	/* Open iirelation catalog */
	table_id.db_tab_base = DM_B_RELATION_TAB_ID;
	table_id.db_tab_index = DM_I_RELATION_TAB_ID;
	status = dm2t_open(dcb, &table_id, lock_mode, DM2T_UDIRECT, 
		req_access_mode, (i4)0, (i4)20, (i4)0,
                log_id, lock_id, 
		(i4)0, (i4)0, (i4)0, &tran_id,
		&stamp, &rel_rcb, (DML_SCB *)0, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	{
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    break;
	}

	/* Open the iidevices catalog */
	table_id.db_tab_base = DM_B_DEVICE_TAB_ID;
	table_id.db_tab_index = DM_I_DEVICE_TAB_ID;
  
	status = dm2t_open(dcb, &table_id, lock_mode, DM2T_UDIRECT, 
		req_access_mode, (i4)0, (i4)20, (i4)0,
		log_id, lock_id,
		(i4)0, (i4)0, (i4)0, &tran_id,
		&stamp, &dev_rcb, (DML_SCB *)0, &jsx->jsx_dberr);

	if (status != E_DB_OK)
	{
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    break;
	}

	/* Turn off logging, replace fails otherwise */
	rel_rcb->rcb_logging = 0;
	dev_rcb->rcb_logging = 0;

	qual_list[0].attr_number = DM_1_RELATION_KEY;
	qual_list[0].attr_operator = DM2R_EQ;

	/*
	** Loop through all the tables and check/update location information.
	*/
	for ( totq = hcb->tblhcb_htotq.q_next;
	      totq != &hcb->tblhcb_htotq;
	      totq = tblcb->tblcb_totq.q_next
	      )
	{
	    tblcb = (RFP_TBLCB *)((char *)totq - 
				CL_OFFSETOF(RFP_TBLCB, tblcb_totq));

	    /*
	    ** If recovery is not going to be performed on the table
	    ** then skip it
	    */
	    if ((tblcb->tblcb_table_status & RFP_RECOVER_TABLE ) == 0)
		continue;

	    qual_list[0].attr_value = (char *)&tblcb->tblcb_table_id;

	    /*
	    ** Position on iirelation
	    */
	    status = dm2r_position( rel_rcb, DM2R_QUAL, qual_list,
			     (i4)1,
			     (DM_TID *)0, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	    {
		/* FIX ME */
		break;
	    }

	    /*
	    ** Look for record in iirelation
	    */
	    for (;;)
	    {
		status = dm2r_get( rel_rcb, &tid, DM2R_GETNEXT,
				   (char *)&relrecord, &jsx->jsx_dberr);
		if (status != E_DB_OK)
		{
		    if (jsx->jsx_dberr.err_code == E_DM0055_NONEXT)
		    {
			status = E_DB_OK;
			CLRDBERR(&jsx->jsx_dberr);
		    }
		    break;
		}

		/*
		** Check table, need to match on both base and index
		*/
		if ((relrecord.reltid.db_tab_base != 
				    tblcb->tblcb_table_id.db_tab_base)
				    ||
		    (relrecord.reltid.db_tab_index != 
				    tblcb->tblcb_table_id.db_tab_index))
		{
		    continue;
		}

		/*
		** If core catalog, view or gateway table,
		** we can't relocate the table
		*/
		if (relrecord.relstat & (TCB_CONCUR | TCB_VIEW | TCB_GATEWAY))
		{
		    status = E_DB_ERROR;
		    dmfWriteMsg(NULL, E_DM1511_RELOC_NO_CORE_GATE_VIEW, 0);
		    break;
		}

		status = reloc_validate_table(jsx, dcb, 
				&tblcb->tblcb_table_id, &relrecord, 
				rel_rcb, dev_rcb);

		if (status != E_DB_OK)
		{
		    /* Specific err formatted in reloc_validate_table() */	
		    break;
		}

		if (phase == RELOC_FINISH_TABLES)
		{
		    status = reloc_update_tbl_loc(jsx, dcb,
				&tblcb->tblcb_table_id, &relrecord, 
				rel_rcb, dev_rcb);
		    if (status != E_DB_OK)
			break;
		}
	    }

	    if ( status != E_DB_OK )
		break;
	}

	/* error in loop checking tables */
	if (status != E_DB_OK)
	    break;

	if (phase == RELOC_PREPARE_TABLES)
	{
	    /*
	    ** BUG 67411: to process jnl/dmp records we need mapping
	    ** of old location numbers to new location numbers
	    */
	    status = reloc_init_locmap(rfp, jsx, dcb);
	    if (status != E_DB_OK)
		break;
	}
    } while (FALSE);

    /* Close the relation catalog */
    if (rel_rcb)
    {
	close_status = dm2t_close(rel_rcb, 0, &local_dberr);
	if (close_status != E_DB_OK)
	{
	    dmfWriteMsg(&local_dberr, 0, 0);
	    if (close_status > status)
	    {
		status = close_status;
		jsx->jsx_dberr = local_dberr;
	    }
	}
    }

    /* Close the iidevices catalog */
    if (dev_rcb)
    {
	close_status = dm2t_close(dev_rcb, 0, &local_dberr);
	if (close_status != E_DB_OK)
	{
	    dmfWriteMsg(&local_dberr, 0, 0);
	    if (close_status > status)
	    {
		status = close_status;
		jsx->jsx_dberr = local_dberr;
	    }
	}

    }

    if (status == E_DB_OK && log_id)
    {
	close_status = dmf_rfp_commit_transaction(&tran_id, log_id, lock_id,
				dmxe_flag, &stamp, &local_dberr);
	if (close_status != E_DB_OK)
	{
	    dmfWriteMsg(&local_dberr, 0, 0);
	    if (close_status > status)
	    {
		status = close_status;
		jsx->jsx_dberr = local_dberr;
	    }
	}
    }
    else if (log_id)
    {
	close_status = dmf_rfp_abort_transaction(dcb, dcb, &tran_id, log_id,
				lock_id, dmxe_flag, (i4)0, 
				(LG_LSN *)0, &local_dberr);
	if (close_status != E_DB_OK)
	{
	    dmfWriteMsg(&local_dberr, 0, 0);
	    if (close_status > status)
	    {
		status = close_status;
		jsx->jsx_dberr = local_dberr;
	    }
	}
    }

    /* Close the database */
    local_dberr = jsx->jsx_dberr;
    close_status = rfp_close_database(rfp, dcb, dbdb_flag);
    if (close_status != E_DB_OK)
    {
	dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	if (close_status > status)
	    status = close_status;
    }
    else
        jsx->jsx_dberr = local_dberr;

    return (status);
}

/*{
** Name: reloc_update_tbl_loc    - Update relloc, devloc for specified table.
**
** Description:
**
**      This routine will update where necessary iirelation.relloc and
**      iidevices.devloc for the specified table to reflect any location
**      changes that have been made.
**
** Inputs:
**      jsx                     Pointer to Journal Support info.
**      dcb                     Pointer to DCB for this database.
**      table_id                Table id for table we are interested in
**      relrecord               iirelation tuple for table 
**      rel_rcb                 Pointer to RCB for iirelation
**      dev_rcb                 Pointer to RCB for iidevices
**
** Outputs:
**	Returns:
**	    E_DB_OK, E_DB_ERROR
**      err_code		reason for status != E_DB_OK
**
** Side Effects:
**	    none
**
** History:
**        18-nov-1994 (alison)
**           Created for Database relocation
**        28-jan-1995 (stial01)
**           BUG 66547: Set TCB2_TBL_RECOVERY_DISALLOWED
[@history_template@]...
*/
static DB_STATUS
reloc_update_tbl_loc(
DMF_JSX             *jsx,
DMP_DCB             *dcb,
DB_TAB_ID           *table_id,
DMP_RELATION        *relrecord,
DMP_RCB             *rel_rcb,
DMP_RCB             *dev_rcb)
{
    DM_TID          reltid;
    i4         error, local_error;
    i4         status = E_DB_OK;
    i4         local_status;
    DM2R_KEY_DESC   devkey_desc[2];
    DM2R_KEY_DESC   relkey_desc[1];
    i4         i,k;
    i4         journal;
    i4         dm0l_jnlflag;
    DMP_DEVICE      devrecord;
    DM_TID          devtid;
    i4         compare;
    DB_LOC_NAME     *new_relloc = 0;
    DB_LOC_NAME     *new_devloc = 0;
    bool             upd_devloc = FALSE;
    bool             upd_relloc = FALSE;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    /* First see if the default data loc is in jsx->jsx_loc_list */
    upd_relloc = reloc_this_loc(jsx, &relrecord->relloc, &new_relloc);

    do
    {
	if (!(relrecord->relstat & TCB_MULTIPLE_LOC))
	    break;

	devkey_desc[0].attr_operator = DM2R_EQ;
	devkey_desc[0].attr_number = DM_1_DEVICE_KEY;
	devkey_desc[0].attr_value = (char *) &table_id->db_tab_base;
	devkey_desc[1].attr_operator = DM2R_EQ;
	devkey_desc[1].attr_number = DM_2_DEVICE_KEY;
	devkey_desc[1].attr_value = (char *) &table_id->db_tab_index;

	status = dm2r_position(dev_rcb, DM2R_QUAL, devkey_desc, (i4)2,
		(DM_TID *)0, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	    break;

	for ( ; ; )
	{
	    status = dm2r_get(dev_rcb, &devtid, DM2R_GETNEXT,
		(char *)&devrecord, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
		break;

	    /* First see if this data loc is in jsx->jsx_loc_list */
	    if ( reloc_this_loc(jsx, &devrecord.devloc, &new_devloc))
	    {
		MEcopy((PTR)new_devloc,
			sizeof (DB_LOC_NAME),
			(PTR)&devrecord.devloc);
		status = dm2r_replace(dev_rcb, 0, DM2R_BYPOSITION,
			(char *)&devrecord, (char *)NULL, &jsx->jsx_dberr);
		if (status != E_DB_OK)
		{
		    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		    dmfWriteMsg(NULL, E_DM1513_RELOC_CANT_UPDT_IIDEVICES, 0);
		    break;
		}
		upd_devloc = TRUE;
	    }
	}
	if (status == E_DB_ERROR && jsx->jsx_dberr.err_code == E_DM0055_NONEXT)
	{
	    status = E_DB_OK;
	    CLRDBERR(&jsx->jsx_dberr);
	}

    } while (FALSE);

    if ((status == E_DB_OK) && (upd_relloc || upd_devloc))
    {
	/*
	** Bump the iirelation relstamp since we have made
	** a structural change to the table
	*/
	TMget_stamp((TM_STAMP *)&relrecord->relstamp12);
	if (new_relloc)
	{
	    MEcopy((PTR)new_relloc, sizeof (DB_LOC_NAME),
			(PTR)&relrecord->relloc);
	}

	/*
	** Turn on TCB2_TBL_RECOVERY_DISALLOWED, so table rollforward 
	** can't be done again until another checkpoint is done for this table
	*/
	relrecord->relstat2 |= TCB2_TBL_RECOVERY_DISALLOWED;

	status = dm2r_replace(rel_rcb, &reltid, DM2R_BYPOSITION,
			(char *)relrecord, (char *)NULL, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	{
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    dmfWriteMsg(NULL, E_DM1514_RELOC_CANT_UPDT_IIRELATION, 0);
	}
    }

    return (status);
}


/*{
** Name: reloc_validate_table    - See if relocation valid for specified table
**
** Description:
**
**      This routine will see if the requested location changes for the 
**      the specified table are valid.  
**
** Inputs:
**      jsx                     Pointer to Journal Support info.
**      dcb                     Pointer to DCB for this database.
**      table_id                Table id for table we are interested in
**      relrecord               iirelation tuple for table 
**      rel_rcb                 Pointer to RCB for iirelation
**      dev_rcb                 Pointer to RCB for iidevices
**
** Outputs:
**	Returns:
**	    E_DB_OK, E_DB_ERROR
**      err_code		reason for status != E_DB_OK
**
** Side Effects:
**	    none
**
** History:
**        18-nov-1994 (alison)
**           Created for Database relocation
**        31-Jan-2008 (hanal04) Bug 119781
**           No need to check for location conflicts if we are copying
**           a whole database to a newdatabase.
[@history_template@]...
*/
static DB_STATUS
reloc_validate_table(
DMF_JSX             *jsx,
DMP_DCB             *dcb,
DB_TAB_ID           *table_id,
DMP_RELATION        *relrecord,
DMP_RCB             *rel_rcb,
DMP_RCB             *dev_rcb)
{
    DMP_EXT             *ext;
    i4             size;
    DM_OBJECT           *misc_buffer;
    DB_LOC_NAME         **newlocs;
    i4             new_cnt = 0; 
    i4             i;
    i4         status, local_status;
    i4         compare;
    DMP_DEVICE      devrecord;
    DM_TID          devtid;
    DM_TID         reltid;
    DM2R_KEY_DESC   key_desc[2];
    DM2R_KEY_DESC   relkey_desc[1];
    bool            no_next = FALSE;
    DB_LOC_NAME     *new_relloc = 0;
    DB_LOC_NAME     *new_devloc = 0;
    DB_LOC_NAME     *loc;
    char            error_buffer[ER_MAX_LEN];
    i4         error_length;
    i4			local_err;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** We only need to check for location conflicts if the
    ** table resides on multiple locations
    ** 
    ** No need to check for conflicts if we are doing a DB copy.
    */
    if ((relrecord->relstat & TCB_MULTIPLE_LOC) == 0 ||
         (jsx->jsx_status1 & JSX_DB_RELOC) != 0)
	return (E_DB_OK);

    ext = dcb->dcb_ext;
    size = sizeof(DB_LOC_NAME *) * ext->ext_count;
    status = dm0m_allocate((i4)sizeof(DMP_MISC) + size, (i4)0,
		(i4)MISC_CB, (i4)MISC_ASCII_ID, (char *)NULL,
		&misc_buffer, &jsx->jsx_dberr);
    if (status != OK)
    {
	dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	dmfWriteMsg(NULL, E_DM150A_RELOC_MEM_ALLOC, 0);
	return (E_DB_ERROR);
    }
    else
    {
	((DMP_MISC *)misc_buffer)->misc_data = (char *)misc_buffer +
		sizeof(DMP_MISC);
	newlocs = (DB_LOC_NAME **)((char *)misc_buffer + sizeof(DMP_MISC));
    }

    /*
    ** Now look at all locations for this table
    ** For each location determine what will be the location after
    ** the relocation, (does it change or not)
    ** See if the new location name is in the newlocs array yet.
    **   If it is ->     ERROR: you cannot relocate a portion of a 
    **                   table to a location on which another portion of
    **                   the table resides.
    ** Otherwise, add the location to the newlocs array.
    */

    /*
    ** First see if the default data loc is in jsx->jsx_loc_list 
    ** If yes, add new_relloc to newlocs array
    */
    if ( reloc_this_loc(jsx, &relrecord->relloc, &new_relloc))
	loc = new_relloc;
    else
	/* Else add relrecord->relloc to newlocs array */
	loc = &relrecord->relloc;

    newlocs[new_cnt++] = loc;

    /* IIDEVICES: Position table using base table id */
    key_desc[0].attr_operator = DM2R_EQ;
    key_desc[0].attr_number = DM_1_DEVICE_KEY;
    key_desc[0].attr_value = (char *) &table_id->db_tab_base;
    key_desc[1].attr_operator = DM2R_EQ;
    key_desc[1].attr_number = DM_2_DEVICE_KEY;
    key_desc[1].attr_value = (char *) &table_id->db_tab_index;
    
    do
    {
	status = dm2r_position(dev_rcb, DM2R_QUAL, key_desc, (i4)2,
		(DM_TID *)0, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	    break;
	for ( ; ; )
	{
	    status = dm2r_get(dev_rcb, &devtid, DM2R_GETNEXT,
		(char *)&devrecord, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
		break;

	    /*
	    ** First see if this data loc is in jsx->jsx_loc_list
	    ** If yes, try to add new_devloc to newlocs array
	    */
	    if ( reloc_this_loc(jsx, &devrecord.devloc, &new_devloc))
		loc = new_devloc;
	    else
		/* Else try to add devrecord.devloc to newlocs array */
		loc = &devrecord.devloc;
   
	    /*
	    ** First make sure the loc is not in newlocs array yet...
	    ** If it is ->     ERROR: you cannot relocate a portion of a
	    **               table to a location on which another portion 
	    **             of the table resides
	    */
	    for (i = 0; i < new_cnt; i++)
	    {
		compare = MEcmp((char *)loc, newlocs[i],
				sizeof(DB_LOC_NAME));
		if (compare == 0)
		{
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1517_RELOC_TBL_LOC_ERR);
		    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			error_buffer, ER_MAX_LEN, &error_length, err_code, 2,
			sizeof (DB_TAB_NAME), &relrecord->relid, 0, 0);
		    dmf_put_line(0, error_length, error_buffer);
		    /* already logged */
		    jsx->jsx_dberr.err_code = 0;
		    status = E_DB_ERROR;
		    break;
		}
	    }

	    if (status == E_DB_ERROR)
		break;

	    newlocs[new_cnt++] = loc;
	}
    } while (FALSE);

    dm0m_deallocate((DM_OBJECT **)&misc_buffer);
    if (status == E_DB_ERROR && jsx->jsx_dberr.err_code == E_DM0055_NONEXT)
	return (E_DB_OK);
    return (status);
}



/*{
** Name: reloc_this_loc          - Should this location be relocated, 
**                                  and if so, to what new location
**
** Description:
**
**      Given a location name this routine will look at jsx->jsx_loc_list
**      to see if the location is going to be relocated. If it is,
**      new_locname will be set to the new location name.
**
** Inputs:
**      jsx                     Pointer to Journal Support info.
**   locname                 Pointer to the location we are searching for.
**
** Outputs:
**	Returns:
**          TRUE, FALSE
**      new_locname             Place to put the new location name if 
**                                 this location is being relocated.
**
** Side Effects:
**	    none
**
** History:
**        18-nov-1994 (alison)
**           Created for Database relocation
[@history_template@]...
*/
bool
reloc_this_loc(
DMF_JSX             *jsx,
DB_LOC_NAME         *locname,
DB_LOC_NAME         **new_locname)
{
    i4 i;
    i4 compare;

    *new_locname = 0;

    for (i = 0; i < jsx->jsx_loc_cnt; i++)
    {
	/* if loc == nloc, this loc recovered without relocate */
	compare = MEcmp((char *)&jsx->jsx_loc_list[i].loc_name,
			(char *)&jsx->jsx_nloc_list[i].loc_name,
			sizeof (DB_LOC_NAME));
	if (compare == 0)
	    continue;

	compare = MEcmp((char *)locname,
			(char *)&jsx->jsx_loc_list[i].loc_name,
			sizeof (DB_LOC_NAME));

	if (compare == 0)
	{
	    *new_locname = &jsx->jsx_nloc_list[i].loc_name;
	    break;
	}
	
    }

    if (*new_locname)
	return (TRUE);
    else
	return (FALSE);
}


/*{
** Name: reloc_check_loc_lists   - Verify location lists specified by user
**
** Description:
**
**      This routine will verify the location names in jsx_loc_list
**      and jsx_nloc_list. All location names must be defined to this
**      installation, so we make sure an iilocation tuple exists for
**      each location name.
**      Then there are two cases:
**        1) Relocating a location
**           For this case, all locations must be data locations for 
**           which this database has been extended.
**        2) Relocating a database
**           For database relocation, all locations in the old location list
**           must be a valid data/ckp/dmp/jnl/work location for the original 
**           database, and the corresponding location in the new location
**           list must have compatible usage.
** Inputs:
**      jsx                     Pointer to Journal Support info.
**      dcb                     Pointer to DCB for database 
**      iidbdb_dcb              Pointer to DCB for iidbdb database
**      log_id                  Log id
**      tran_id                 Pointer to transaction id
**      lock_id                 Lock_id
**
** Outputs:
**	Returns:
**	    E_DB_OK, E_DB_ERROR
**      err_code		reason for status != E_DB_OK
**
** Side Effects:
**	    none
**
** History:
**        18-nov-1994 (alison)
**           Created for Database relocation
[@history_template@]...
*/
static DB_STATUS
reloc_check_loc_lists(
DMF_JSX         *jsx,
DMP_DCB         *dcb,
DMP_DCB         *iidcb,
i4         log_id,
DB_TRAN_ID      *tran_id,
i4         lock_id)
{
    DMP_EXT             *ext;
    DMP_RCB             *ext_rcb;
    DMP_RCB             *loc_rcb;
    DB_TAB_ID           table_id;
    DB_STATUS           status;
    DB_STATUS           close_status;
    DU_LOCATIONS        location;
    DU_LOCATIONS        new_location;
    DU_EXTEND           extend;
    DB_TAB_TIMESTAMP    stamp;
    i4             j, k;
    i4             tmpflags;
    i4             ext_flags;
    DMP_LOC_ENTRY       *oldloc, *newloc;
    char                error_buffer[ER_MAX_LEN];
    i4             error_length;
    DB_LOC_NAME         *loc_name;
    i4			local_err;
    DB_ERROR		local_dberr;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    do
    {

	/* Open iiextend table */
	table_id.db_tab_base = DM_B_EXTEND_TAB_ID;
	table_id.db_tab_index = DM_I_EXTEND_TAB_ID;
	status = dm2t_open(iidcb, &table_id, DM2T_IS, DM2T_UDIRECT,
		DM2T_A_READ, 0, 20, 0, log_id, lock_id,
		0, 0, 0, tran_id, &stamp, &ext_rcb, (DML_SCB *)0, &jsx->jsx_dberr);

	if (status != E_DB_OK)
	{
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    break;
	}
	
	/* Open iilocations table */
	table_id.db_tab_base = DM_B_LOCATIONS_TAB_ID;
	table_id.db_tab_index = 0;
	status = dm2t_open(iidcb, &table_id, DM2T_IS, DM2T_UDIRECT,
		DM2T_A_READ, 0, 20, 0, log_id, lock_id,
		0, 0, 0, tran_id, &stamp, &loc_rcb, (DML_SCB *)0, &jsx->jsx_dberr);

	if (status != E_DB_OK)
	{
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    break;
	}

	/*
	** For each location in jsx_loc_list and jsx_nloc_list:
	**   
	** - Make sure location is valid for this installation, 
	**    get the iilocations tuple.
	** - If not relocating database, each location must be a 
	**   valid data location, so get the iiextend tuple and the  
	**   dcb_ext entry
	*/
	ext = dcb->dcb_ext;
	for (j = 0; j < jsx->jsx_loc_cnt; j++)
	{
	    /* Get location tuple for old location */
	    loc_name = &jsx->jsx_loc_list[j].loc_name;
	    status = get_iilocation(jsx, iidcb, loc_rcb, loc_name, 
				&location, log_id, tran_id, lock_id);

	    /* Get location tuple for new location */
	    if (status == E_DB_OK)
	    {
		loc_name = &jsx->jsx_nloc_list[j].loc_name;
		status = get_iilocation(jsx, iidcb, loc_rcb, loc_name,
				&new_location, log_id, tran_id, lock_id);
	    }

	    if (status != E_DB_OK)
	    {
		SETDBERR(&jsx->jsx_dberr, 0, E_DM150E_RELOC_IILOCATIONS_ERR);
		uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			error_buffer, ER_MAX_LEN, &error_length, &local_err, 2,
			sizeof (DB_LOC_NAME), loc_name, 0, 0);
		dmf_put_line(0, error_length, error_buffer);
		break;
	    }

	    if (!(jsx->jsx_status1 & JSX_DB_RELOC))
	    {

		/* Get iiextend tuple for old data location */
		loc_name = &jsx->jsx_loc_list[j].loc_name;
		status = get_iiextend(jsx, iidcb, &dcb->dcb_name, 
				loc_name, DU_EXT_DATA, ext_rcb,
				&extend, log_id, tran_id, lock_id);

		/* Get iiextend tuple for new data location */
		if (status == E_DB_OK)
		{
		    loc_name = &jsx->jsx_nloc_list[j].loc_name;
		    status = get_iiextend(jsx, iidcb, &dcb->dcb_name,
				loc_name, DU_EXT_DATA, ext_rcb,
				&extend, log_id, tran_id, lock_id);
		}

		if (status != E_DB_OK)
		{
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM150D_RELOC_IIEXTEND_ERR);
		    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			error_buffer, ER_MAX_LEN, &error_length, &local_err, 2,
			sizeof (DB_LOC_NAME), loc_name, 0, 0);
		    dmf_put_line(0, error_length, error_buffer);
		    break;
		}

		/* Get dcb ext for old data location */
		loc_name = &jsx->jsx_loc_list[j].loc_name;
		status = find_dcb_ext(jsx, dcb, loc_name, 
                                        DCB_DATA, &oldloc);

		/* Get dcb ext for new data location */
		if (status == E_DB_OK)
		{
		    loc_name = &jsx->jsx_nloc_list[j].loc_name;
		    status = find_dcb_ext(jsx, dcb, loc_name, 
                                        DCB_DATA, &newloc);
		}

		if (status != E_DB_OK)
		{
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM150C_RELOC_LOC_CONFIG_ERR);
		    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			error_buffer, ER_MAX_LEN, &error_length, &local_err, 2,
			sizeof (DB_LOC_NAME), loc_name, 0, 0);
		    dmf_put_line(0, error_length, error_buffer);
		    break;
		}
	    }
	    else
	    {

		/*
		** Validation for db relocation:
		** For current location name in location list, find all
		** uses it has been extended for for this db
		** Make sure there is an iiextend tuple for all data and work
		** extensions.
		*/
		tmpflags = 0;
		for (k = 0; k < ext->ext_count; k++)
		{
		    if (MEcmp((char *)&ext->ext_entry[k].logical,
				(char *)&jsx->jsx_loc_list[j].loc_name,
				sizeof(DB_LOC_NAME)))
			continue;

		    /* keep track of all usages for this loc name */
		    tmpflags |= ext->ext_entry[k].flags;

		    if (ext->ext_entry[k].flags & DCB_WORK)
			ext_flags = DU_EXT_WORK;
		    else if (ext->ext_entry[k].flags & DCB_AWORK)
			ext_flags = DU_EXT_AWORK;
		    else if (ext->ext_entry[k].flags & DCB_DATA)
			ext_flags = DU_EXT_DATA;
		    else
			ext_flags = 0;

		    if (ext_flags)
		    {
			status = get_iiextend(jsx, iidcb, &dcb->dcb_name,
				&jsx->jsx_loc_list[j].loc_name, ext_flags,
				 ext_rcb, &extend, log_id, tran_id, lock_id);
		
			if (status != E_DB_OK)
			{
			    SETDBERR(&jsx->jsx_dberr, 0, E_DM150D_RELOC_IIEXTEND_ERR);
			    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
					NULL, error_buffer, ER_MAX_LEN,
					&error_length, &local_err, 2,
					sizeof (DB_LOC_NAME), loc_name, 0, 0);
			    dmf_put_line(0, error_length, error_buffer);
			    break;
			}
		   }
		}

		if (status != E_DB_OK)
		{
		    status = E_DB_ERROR;
		    break;
		}

		/* Make sure new loc usage is compatible with old loc usage */
		if (   ((tmpflags & DCB_DATA) &&
				!(new_location.du_status & DU_DBS_LOC))
		    || ((tmpflags & DCB_CHECKPOINT) &&
				!(new_location.du_status & DU_CKPS_LOC))
		    || ((tmpflags & DCB_JOURNAL) && 
				!(new_location.du_status & DU_JNLS_LOC))
		    || ((tmpflags & DCB_DUMP) &&
				!(new_location.du_status & DU_DMPS_LOC))
		    || ((tmpflags & DCB_WORK) &&
				!(new_location.du_status & DU_WORK_LOC))
		    || ((tmpflags & DCB_AWORK) &&
				!(new_location.du_status & DU_AWORK_LOC)) )
		{
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1515_RELOC_USAGE_ERR);
		    status = E_DB_ERROR;
		    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			error_buffer, ER_MAX_LEN, &error_length, &local_err, 3,
			sizeof (DB_LOC_NAME), &jsx->jsx_loc_list[j].loc_name, 
			sizeof (DB_LOC_NAME), &jsx->jsx_nloc_list[j].loc_name,
			0, 0);
		    dmf_put_line(0, error_length, error_buffer);
		    break;
		}
	    }
	}

    } while (FALSE);

    /* Close iiextend only if we opened it */
    if (ext_rcb)
    {
	close_status = dm2t_close(ext_rcb, 0, &local_dberr);
	if (close_status != E_DB_OK)
	{
	    dmfWriteMsg(&local_dberr, 0, 0);
	    if (status == E_DB_OK)
	    {
		status = close_status;
		jsx->jsx_dberr = local_dberr;
	    }

	}
    }

    /* Close iilocations only if we opened it */
    if (loc_rcb)
    {
	close_status = dm2t_close(loc_rcb, 0, &local_dberr);
	if (close_status != E_DB_OK)
	{
	    dmfWriteMsg(&local_dberr, 0, 0);
	    if (status == E_DB_OK)
	    {
		status = close_status;
		jsx->jsx_dberr = local_dberr;
	    }
	}
    }

    return (status);
}


/*{
** Name: reloc_loc_process       - Location relocation prepare/finish operations
**
** Description:
**
**      This routine will see if the requested locations are valid for all
**      the tables that will be affected by the relocate. 
**      We need to scan iirelation and for each table see if any of its 
**      locations are affected by the relocate.
**
** Inputs:
**      jsx                     Pointer to Journal Support info.
**      dcb                     Pointer to DCB for this database.
**      u_i4               phase
**
** Outputs:
**	Returns:
**	    E_DB_OK, E_DB_ERROR
**      err_code		reason for status != E_DB_OK
**
** Side Effects:
**	    none
**
** History:
**        18-nov-1994 (alison)
**           Created for Database relocation
[@history_template@]...
*/
DB_STATUS
reloc_loc_process(
DMF_JSX             *jsx,
DMP_DCB             *dcb,
u_i4           phase)
{
    DB_TAB_ID         table_id;
    DMP_RELATION      relrecord;
    DMP_DEVICE        devrecord;
    DMP_RCB           *rel_rcb = 0;
    DMP_RCB           *dev_rcb = 0;
    DMP_RCB           *ridx_rcb = 0;
    i4           status, close_status;
    DM_TID            tid;
    DB_TAB_TIMESTAMP  stamp;
    DB_TRAN_ID        tran_id;
    DMP_LOC_ENTRY     *oldloc, *newloc;
    i4           i;
    char              error_buffer[ER_MAX_LEN];
    i4           error_length;
    i4           lock_mode;
    i4           req_access_mode;
    i4           table_cnt = 0;
    i4           scan_cnt;
    i4           maxscans;
    i4			local_err;
    DB_ERROR		local_dberr;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    do 
    {

	if (phase == RELOC_FINISH_LOCATIONS)
	{
	    lock_mode = DM2T_IX;
	    req_access_mode = DM2T_A_WRITE;
	    maxscans = 2;
	}
	else
	{
	    lock_mode = DM2T_IS;
	    req_access_mode = DM2T_A_READ;
	    maxscans = 1;
	}

	/* Open iirelation catalog */
	tran_id.db_high_tran = 0;
	tran_id.db_low_tran = 0;

	table_id.db_tab_base = DM_B_RELATION_TAB_ID;
	table_id.db_tab_index = DM_I_RELATION_TAB_ID;
	status = dm2t_open(dcb, &table_id, lock_mode, DM2T_UDIRECT, 
		req_access_mode, (i4)0, (i4)20, (i4)0,
                (i4)0, dmf_svcb->svcb_lock_list, 
		(i4)0, (i4)0, (i4)0, &tran_id,
		&stamp, &rel_rcb, (DML_SCB *)0, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	{
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	    break;
	}

	/* Turn off logging, replace fails otherwise */
	rel_rcb->rcb_logging = 0;

	/*
	** Scan IIRELATION:
	**    If not updating, scan IIRELATIONS once
	**    If updating, scan IIRELATIONS twice
	** Defer the following until the last scan:
	**    Open of IIDEVICES
	**    Processing of tables that reside on multiple locations
	** This is because if we are updating, there is a chance 
	** that the user relocated the default data location,
	** and we need to first update IIRELATION.relloc for IIDEVICES 
	** before we try to open IIDEVICES.
	*/
	for (scan_cnt = 1; scan_cnt <= maxscans; scan_cnt++)
	{

	    status = dm2r_position(rel_rcb, DM2R_ALL, (DM2R_KEY_DESC *)0,
		(i4)0, (DM_TID *)0, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	    {
		dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		break;
	    }

	    if (scan_cnt == maxscans)
	    {
		/* Open the iidevices catalog */
		table_id.db_tab_base = DM_B_DEVICE_TAB_ID;
		table_id.db_tab_index = DM_I_DEVICE_TAB_ID;
  
		status = dm2t_open(dcb, &table_id, lock_mode, DM2T_UDIRECT, 
		    req_access_mode, (i4)0, (i4)20, (i4)0,
		    (i4)0, dmf_svcb->svcb_lock_list, 
		    (i4)0, (i4)0, (i4)0, &tran_id,
		    &stamp, &dev_rcb, (DML_SCB *)0, &jsx->jsx_dberr);

		if (status != E_DB_OK)
		{
		    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
		    break;
		}

		/* Turn off logging, replace fails otherwise */
		dev_rcb->rcb_logging = 0;
	    }

	    /* Scan relation catalog, check all tables */
	    for ( ; ; )
	    {
		status = dm2r_get(rel_rcb, &tid, DM2R_GETNEXT,
                        (char *)&relrecord, &jsx->jsx_dberr);

		if (status == E_DB_ERROR)
		{
		    if (jsx->jsx_dberr.err_code == E_DM0055_NONEXT)
		    {
			CLRDBERR(&jsx->jsx_dberr);
			status = E_DB_OK;
		    }
		    else
		    {
			/* FIX ME error getting relrecord for next table */
		    }
		    break;
		}

		table_cnt++;

		/* 
		** Process each table ONCE
		**   process tables with one location on FIRST scan 
		*/
		if (((relrecord.relstat & TCB_MULTIPLE_LOC) == 0)
			&& scan_cnt != 1)
		{
		    continue;
		}

		/*
		** Process each table ONCE
		**   process tables with multiple locations on LAST scan 
		*/
		if ((relrecord.relstat & TCB_MULTIPLE_LOC)
			&& scan_cnt != maxscans)
		{
		    continue;
		}

		/*
		** Make sure there is no conflict with old/new locations
		** for this table
		*/
		status = reloc_validate_table(jsx, dcb, &relrecord.reltid,
			&relrecord, rel_rcb, dev_rcb);
		if (status != E_DB_OK)
		{
		    /* specific error formatted in reloc_validate_table */
		    break;
		}

		if (phase == RELOC_FINISH_LOCATIONS)
		{
		    status = reloc_update_tbl_loc(jsx, dcb, &relrecord.reltid,
				&relrecord, rel_rcb, dev_rcb);
		    if (status != E_DB_OK)
		    {
			/* specific err formatted in reloc_update_tbl_loc()*/
			break;
		    }
		}
	    }
	}
    } while (FALSE);

    /* Close the iirel_idx catalog */
    if (ridx_rcb)
    {
	close_status = dm2t_close(ridx_rcb, 0, &local_dberr);
	if (close_status != E_DB_OK)
	{
	    dmfWriteMsg(&local_dberr, 0, 0);
	    if (status == E_DB_OK)
	    {
		status = close_status;
		jsx->jsx_dberr = local_dberr;
	    }
	}
    }

    /* Close the relation catalog */
    if (rel_rcb)
    {
	close_status = dm2t_close(rel_rcb, 0, &local_dberr);
	if (close_status != E_DB_OK)
	{
	    dmfWriteMsg(&local_dberr, 0, 0);
	    if (status == E_DB_OK)
	    {
		status = close_status;
		jsx->jsx_dberr = local_dberr;
	    }
	}
    }

    /* Close the iidevices catalog */
    if (dev_rcb)
    {
	close_status = dm2t_close(dev_rcb, 0, &local_dberr);
	if (close_status != E_DB_OK)
	{
	    dmfWriteMsg(&local_dberr, 0, 0);
	    if (status == E_DB_OK)
	    {
		status = close_status;
		jsx->jsx_dberr = local_dberr;
	    }
	}

    }

    return (status);
}


/*{
** Name: alloc_newdcb            - Allocate dcb for new database
**
** Description:
**
**      This routine will allocate and initialize a dcb for the new
**      database we created during database relocation.
**
** Inputs:
**      jsx                     Pointer to Journal Support info.
**      orig_dcb                Pointer to DCB for original db we were copying.
**      root_loc                Location entry for root location of new db
**      database                iidatabase tuple for new database
**
** Outputs:
**	Returns:
**	    E_DB_OK, E_DB_ERROR
**     newdcb                  Pointer to DCB for new DCB allocated.
**      err_code		reason for status != E_DB_OK
**
** Side Effects:
**	    none
**
** History:
**        18-nov-1994 (alison)
**           Created for Database relocation
[@history_template@]...
*/
static DB_STATUS
alloc_newdcb(
DMF_JSX         *jsx,
DMP_DCB         *orig_dcb,
DMP_DCB         **newdcb,
DMP_LOC_ENTRY   *root_loc,
DU_DATABASE     *database)
{
    DMP_DCB             *ndcb;
    DB_STATUS           status;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    status = dm0m_allocate((i4)sizeof(DMP_DCB),
	DM0M_ZERO, (i4)DCB_CB, (i4)DCB_ASCII_ID, (char *)NULL,
	(DM_OBJECT **)&ndcb, &jsx->jsx_dberr);

    if (status != E_DB_OK)
    {
	*newdcb = 0;
	dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	dmfWriteMsg(NULL, E_DM150A_RELOC_MEM_ALLOC, 0);
	return (status);
    }

    /* Initialize the DCB for this database */
    MEmove(sizeof(database->du_dbname), database->du_dbname, ' ',
	sizeof(ndcb->dcb_name), (PTR)&ndcb->dcb_name);
    MEmove(sizeof(database->du_own), (PTR)&database->du_own, ' ',
	sizeof(ndcb->dcb_db_owner), (PTR)&ndcb->dcb_db_owner);
    MEmove(sizeof(database->du_dbloc),
	(PTR)&database->du_dbloc, ' ',
	sizeof(ndcb->dcb_location.logical),
	(PTR)&ndcb->dcb_location.logical);
    STRUCT_ASSIGN_MACRO(*root_loc, ndcb->dcb_location);
    ndcb->dcb_type = DCB_CB;
    ndcb->dcb_access_mode = DCB_A_WRITE;
    ndcb->dcb_served = DCB_MULTIPLE;
    ndcb->dcb_db_type = DCB_PUBLIC;
    ndcb->dcb_log_id = dmf_svcb->svcb_lctx_ptr->lctx_lgid;
    dm0s_minit(&ndcb->dcb_mutex, "DCB mutex");

    *newdcb = ndcb;
    return (E_DB_OK);
}



/*{
** Name: reloc_db_dircreate      - Create a directory for new database location 
**
** Description:
**
**      This routine will create a directory for a location in the new
**      database.
**      If the directory already exists, any files in it will be deleted.
**
** Inputs:
**      jsx                     Pointer to Journal Support info.
**      copy_info               Pointer to info for copying files to new 
**                                   directory.
**
** Outputs:
**	Returns:
**	    E_DB_OK, E_DB_ERROR
**     newdcb                  Pointer to DCB for new DCB allocated.
**      err_code		reason for status != E_DB_OK
**
** Side Effects:
**	    none
**
** History:
**        18-nov-1994 (alison)
**           Created for Database relocation
**        27-01-1997 (nicph02)
**           Fixed bug 80086 : Missing E_DB_OK status when a diectory 
**           already exists.
[@history_template@]...
*/
static DB_STATUS
reloc_db_dircreate(
DMF_JSX         *jsx,
COPY_INFO       *copy_info)
{
    DI_IO               di_dir;
    DB_STATUS           status, di_status;
    CL_ERR_DESC         sys_err;
    char                temp_loc[DB_AREA_MAX + 4];
    i4             db_len;
    char                *cp;
    LOCATION            loc;
    char                error_buffer[ER_MAX_LEN];
    i4             error_length;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    for (db_len = 0; db_len < sizeof(*copy_info->cp_dbname); db_len++)
    {
	if (copy_info->cp_dbname->db_db_name[db_len] == ' ')
	    break;
    }

    MEmove(copy_info->cp_loctup->du_l_area, copy_info->cp_loctup->du_area,
		0, sizeof(temp_loc), temp_loc);

    status = LOingpath(temp_loc, 0, copy_info->cp_lo_what, &loc);
    if (!status)
	LOtos(&loc, &cp);

    if (!status && *cp != EOS)
    {
	status = DIdircreate(&di_dir, cp, STlength(cp),
			(char *)copy_info->cp_dbname, (u_i4)db_len,
			&sys_err);
	if (status == DI_EXISTS)
	{

	    if ( STEQ_MACRO( copy_info->cp_lo_what, LO_CKP ) )
		SETDBERR(&jsx->jsx_dberr, 0, E_DM919F_DMM_LOCV_CKPEXISTS);
	    else if ( STEQ_MACRO( copy_info->cp_lo_what, LO_JNL ) )
		SETDBERR(&jsx->jsx_dberr, 0, E_DM91A2_DMM_LOCV_JNLEXISTS);
	    else if ( STEQ_MACRO( copy_info->cp_lo_what, LO_DB ) )
		SETDBERR(&jsx->jsx_dberr, 0, E_DM91A1_DMM_LOCV_DATAEXISTS);
            else if ( STEQ_MACRO( copy_info->cp_lo_what, LO_DMP ) )
		SETDBERR(&jsx->jsx_dberr, 0, E_DM91A0_DMM_LOCV_DMPEXISTS);
	    else if ( STEQ_MACRO( copy_info->cp_lo_what, LO_WORK ) ) 
		SETDBERR(&jsx->jsx_dberr, 0, E_DM91A9_DMM_LOCV_WORKEXISTS);
	
	    dmfWriteMsg(&jsx->jsx_dberr, 0, 0);

            status = E_DB_OK;
#ifdef xxx
	    di_status = DIlistfile(&di_dir, 
				copy_info->cp_newloc->physical.name,
				copy_info->cp_newloc->phys_length,
				delete_files, (PTR)copy_info->cp_newloc,
				&sys_err);
	    if (di_status != DI_ENDFILE)
	    {
		dmfWriteMsg(NULL, E_DM1502_RELOC_DEL_FI_ERR, 0);
		status = E_DB_ERROR;
	    }

#endif
	}
    }
    return (status);
}


/*{
** Name: reloc_check_newlocs     - For db reloc, check for valid new loc list
**
** Description:
**
**      This routine will check the location/new_location lists specified 
**      during database relocation.  
**      It will check to see if two existing locations are being mapped
**      into one location in the new database. We currently do not allow
**      this because it would require us to build a config file for the
**      new database with less EXT entries.
**
** Inputs:
**      jsx                     Pointer to Journal Support info.
**      dcb                     Pointer to DCB for original database
**
** Outputs:
**	Returns:
**	    E_DB_OK, E_DB_ERROR
**     newdcb                  Pointer to DCB for new DCB allocated.
**      err_code		reason for status != E_DB_OK
**
** Side Effects:
**	    none
**
** History:
**        18-nov-1994 (alison)
**           Created for Database relocation
[@history_template@]...
*/
static DB_STATUS
reloc_check_newlocs(
DMF_JSX             *jsx,
DMP_DCB             *dcb)
{
    DMP_EXT             *ext;
    i4             size;
    DM_OBJECT           *misc_buffer;
    i4             i, j; 
    i4         status, local_status;
    DB_LOC_NAME     *tmp_loc;
    DB_LOC_NAME     *loc;
    i4         compare;
    i4         new_cnt = 0;
    struct LOC_LIST
    {
	DB_LOC_NAME    *loc_name;
	i4         flags;
    } *newlocs;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);


    ext = dcb->dcb_ext;
    size = sizeof(*newlocs) * ext->ext_count;
    status = dm0m_allocate((i4)sizeof(DMP_MISC) + size, (i4)0,
		(i4)MISC_CB, (i4)MISC_ASCII_ID, (char *)NULL,
		&misc_buffer, &jsx->jsx_dberr);
    if (status != OK)
    {
	dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	dmfWriteMsg(NULL, E_DM150A_RELOC_MEM_ALLOC, 0);
	return (E_DB_ERROR);
    }
    else
    {
	((DMP_MISC *)misc_buffer)->misc_data = (char *)misc_buffer +
		sizeof(DMP_MISC);
	newlocs = (struct LOC_LIST *)((char *)misc_buffer + sizeof(DMP_MISC));
    }

    /*
    ** Now look at all locations 
    ** For each location determine what will be the location after
    ** the relocation, (does it change or not)
    ** See if the new location name is in the newlocs array yet.
    **   If it is ->     ERROR: you cannot relocate more than one  
    **                   location to the same location.
    ** Otherwise, add the location to the newlocs array.
    */

    for (j = 0; j < ext->ext_count; j++)
    {
	/*
	** First see if this loc is in jsx->jsx_loc_list
	** If yes, try to add corresponding jsx->jsx_nloc list to newlocs array 
	*/
	if ( reloc_this_loc(jsx, &ext->ext_entry[j].logical, &tmp_loc))
	    loc = tmp_loc;
	else
	    /* Else try to add this location to newlocs array */
	    loc = &ext->ext_entry[j].logical;

	/*
	** First make sure the loc is not in newlocs array yet
	** If it is ->   ERROR: for database recovery you cannot relocate 
	**               more than one location to the same location.
	*/
	for (i = 0; i < new_cnt; i++)
	{
	    compare = MEcmp((char *)loc, newlocs[i].loc_name,
				sizeof(DB_LOC_NAME));
	    if (compare == 0 && (ext->ext_entry[j].flags & newlocs[i].flags))
	    {
		dmfWriteMsg(NULL, E_DM1516_RELOC_DB_LOC_ERR, 0);
		status = E_DB_ERROR;
		break;
	    }
            if (compare == 0 && (((ext->ext_entry[j].flags & DCB_WORK) &&
                                  (newlocs[i].flags & DCB_AWORK)) ||
                                 ((ext->ext_entry[j].flags & DCB_AWORK) &&
                                  (newlocs[i].flags & DCB_WORK))))
            {
		dmfWriteMsg(NULL, E_DM1506_RELOC_INVL_USAGE, 0);
                status = E_DB_ERROR;
                break;
            }
	}

	if (status == E_DB_ERROR)
	    break;

	newlocs[new_cnt].loc_name = loc;
	newlocs[new_cnt].flags = ext->ext_entry[j].flags;
	new_cnt++;
    }

    dm0m_deallocate((DM_OBJECT **)&misc_buffer);
    return (status);
}


/*{
** Name: reloc_init_locmap    - Init array of extent_count size with newloc ids
**
** Description:
**
**      Allocate memory to hold an array of new location numbers, one for each
**      extent of the database.
**      For each extent of the database, see if it was in the -location list.
**      If it was, get the location number of the corresponding location in 
**      the -new_location list.
**      For example if ii_database is location 0 and newdataloc is location 5
**      and the following was specified:
**        -location=ii_database -new_location=newdataloc
**      Then:
**        locmap[0]=5, locmap[1]=1, locmap[2]=2, locmap[3]=3, locmap[4]=4,
**        locmap[5]=5
**
** Inputs:
**      jsx                     Pointer to Journal Support info.
**      dcb                     Pointer to DCB for original database
**
** Outputs:
**	Returns:
**	    E_DB_OK, E_DB_ERROR
**
** Side Effects:
**	    none
**
** History:
**        10-mar-1995 (stial01)
**           Created for bug 67411.
**
[@history_template@]...
*/
static DB_STATUS
reloc_init_locmap(
DMF_RFP             *rfp,
DMF_JSX             *jsx,
DMP_DCB             *dcb)
{
    DMP_EXT             *ext = dcb->dcb_ext;
    i4             size;
    DB_STATUS           status;
    i4             *newloc_ids;
    i4             i;
    DB_LOC_NAME         *new_locname;
    DMP_LOC_ENTRY       *newloc;
    DMP_LOC_ENTRY       *firstloc;
    i4             err_code;
    DM_OBJECT           *misc_buffer;

    size = sizeof(i4) * ext->ext_count;
    status = dm0m_allocate((i4)sizeof(DMP_MISC) + size, (i4)0,
		(i4)MISC_CB, (i4)MISC_ASCII_ID, (char *)NULL,
		&misc_buffer, &jsx->jsx_dberr);
    if (status != OK)
    {
	dmfWriteMsg(&jsx->jsx_dberr, 0, 0);
	dmfWriteMsg(NULL, E_DM1304_RFP_MEM_ALLOC, 0);
	return (E_DB_ERROR);
    }
    else
    {
	((DMP_MISC *)misc_buffer)->misc_data = (char *)misc_buffer +
		sizeof(DMP_MISC);
	newloc_ids = (i4 *)((char *)misc_buffer + sizeof(DMP_MISC));
    }

    firstloc = &ext->ext_entry[0];

    for (i = 0; i < ext->ext_count; i++)
    {
	newloc_ids[i] = i;  
	if (reloc_this_loc(jsx, &ext->ext_entry[i].logical, &new_locname))
	{
	    status = find_dcb_ext(jsx, dcb, new_locname,
			DCB_DATA, &newloc);
	    if (status == E_DB_OK)
	    {
		newloc_ids[i] = (newloc - firstloc);
	    }
	    else
	    {
		dm0m_deallocate((DM_OBJECT **)&misc_buffer);
		return (E_DB_ERROR);
	    }
	}
    }
    rfp->rfp_locmap = (DMP_MISC *)misc_buffer;
    return (E_DB_OK);
}
