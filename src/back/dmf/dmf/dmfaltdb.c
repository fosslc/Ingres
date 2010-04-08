/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <pc.h>
#include    <me.h>
#include    <tr.h>
#include    <tm.h>
#include    <st.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dudbms.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dm2d.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dm2r.h>
#include    <dm0c.h>
#include    <dm0m.h>
#include    <dmfjsp.h>
#include    <sxf.h>
#include    <dma.h>
#include    <dmxe.h>
/*Included for Unicode*/
#include    <adulcol.h>
#include    <aduucol.h>
#include    <ercl.h> 
/**
**
**  Name: DMFALTDB.C - DMF Database Alter.
**
**  Description:
**	This module contains routines to implement the ALTERDB command.
**
**	Alterdb is designed to be an entry point to the DMFJSP process.
**	A user request for ALTERDB is translated into a call to DMFJSP
**	with the "alterdb" option.  The JSP parses the command line and
**	then calls the dmfalter routine in this module to do alterdb
**	processing.
**
**	External entry points:
**	    dmfalter	    - Alter database state or characteristics.
**
**	Internal routines:
**	    disable_jnl	    	- immediately turn off journaling on a database.
**	    alter_jnl_bksz	- change journal file block size.
**	    alter_jnl_maxcnt	- change number of journal blocks in jnl file.
**	    alter_jnl_blkcnt	- change initial journal file allocation.
**	    delete_oldest_ckp	- delete the oldest checkpoint.
**	    delete_invalid_ckp  - delete all the invalid checkpoints
**          convert_unicode     - converts non-unicode database into unicode
**	    alter_mvcc		- switch MVCC on or off, effective the next
**				  time the DB is opened.
**
**
**  History: 
**	25-feb-1991 (rogerk)
**	    Created as part of the Archiver Stability project.
**	26-mar-1991 (rogerk)
**	    Changed name of the file from dmfalter.c to dmfaltdb.c so it
**	    won't clash with the same named file in the DMF module tests.
**	07-jul-1992 (ananth)
**	    Prototyping DMF.
**	18-nov-1992 (robf)
**	    Altering a database is a security event, make sure it gets
**	    recorded.
**	30-nov-1993 (rmuth)
**	    Include di.h for dm0c.h
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system type definitions.
**	26-apr-1993 (jnash)
**	    Add support for -init_jnl_blocks, -max_jnl_blocks and 
**	    -jnl_block_size options.
**	24-may-1993 (jnash)
**	    Fix format of one TRformat() message.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**	20-sep-1993 (jnash)
**	    Add -delete_oldest_ckp.
**	    Modify VERBOSE activities in line with support for AUDITDB
**	    -verbose flag.
**	22-nov-1993 (jnash)
**	    B56095.  When journaling disabled, also disable rollfowarddb #c
**	    from a prior checkpoint.
**	31-jan-1994 (jnash)
**	    Update config file jnl_first_jnl_seq when deleting oldest 
**	    checkpoint.
**	9-mar-94 (stephenb)
**	    Update dma_write_audit() calls to new prototype.
**	14-mar-94 (stephenb)
**	    Add new parameter to dma_write_audit() to match current prototype.
**	28-mar-1994 (jnash)
**	    Fix AV in alterdb -delete_oldest_ckp by initinalizing cnf.
**      24-jan-1995 (stial01)
**          BUG 66473, delete_oldest_ckp() ckp_mode is now bit pattern.
**	21-feb-1995 (shero03)
**	    Bug #66987
**	    Don't delete the last database checkpoint if there are
**	    table check points
**      23-Mar-1995 (liibi01)
**         Alterdb -delete_oldest_ckp doesn't work for offline checkpoints,
**         the error message says there is no checkpoint to delete.
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**      01-sep-95 (tchdi01)
**          Added support for the new option
**                 -production=(off|on)
**          which sets the mode of the database. This is handled by set_pmode().
**
**          Added support for the +w option which can be used to make the program
**          wait until the database is avaialble
**
**          Added #include <dudbms.h> and <dmxe.h>
**	15-feb-96 (emmag)
**	    Include me.h to fix a linker error on NT.
**	24-may-96 (nick)
**	    Zero out last log address journaled when disabling journaling.
** 	14-apr-1997 (wonst02)
** 	    Added set_accmode() for readonly database support.
**	12-may-1997 (wonst02)
**	    Disable journaling on a readonly database.
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**      08-Dec-1997 (hanch04)
**          Initialize new la_block field in LG_LA
**          for support of logs > 2 gig
**	19-May-1998 (devjo01)
**	    Corrected 'cnf' usage.  Was pointing to deallocated memory
**	    which sometimes contained configuration information from
**	    previous call to dm0c_open, and sometimes trash. (b90918)
**	19-May-1998 (devjo01)
**	    Corrected 'cnf' usage.  Was pointing to deallocated memory
**	    which sometimes contained configuration information from
**	    previous call to dm0c_open, and sometimes trash. (b90918)
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	30-Nov-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID* to dmxe_begin() prototype.
**	15-Dec-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LGbegin() prototype.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to dma_write_audit() if not C2SECURE.
**	11-apr-2001 (rigka01)
**	    Bug 104242 : auditdb <testdb> -delete_oldest_ckp may delete
**	    the only valid checkpoint.  This occurs when the oldest
**	    checkpoint is the only valid checkpoint and the most recent
**	    checkpoint is not valid.  The change is in delete_oldest_ckp().
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      13-Apr-2004 (nansa02)
**          Added two new flags delete_invalid_ckp 
**          and convert_unicode.
**      22-Mar-2006 (stial01)
**          switch_journal() make sure switch done, otherwise retry (b115385)
**      31-Mar-2006 (stial01)
**          Format switch complete message after journal switch verified.
**      19-oct-06 (kibro01) bug 116436
**          Change jsp_file_maint parameter to be sequence number rather than
**          index into the jnl file array, and add a parameter to specify
**          whether to fully delete (partial deletion leaves journal and
**          checkpoint's logical existence)
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here.
**	10-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	18-Nov-2008 (jonj)
**	    SIR 120874: dm0c_?, dm2d_? functions converted to DB_ERROR *
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dmse_?, dm2r_?, dm2t_? functions converted to DB_ERROR *
**	08-Dec-2008 (jonj)
**	    SIR 120874: dma_? auditing functions converted to DB_ERROR *
**	25-Aug-2009 (kschendel) 121804
**	    Need dm0m.h to satisfy gcc 4.3.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Added alter_mvcc to enable/disable MVCC.
**	02-Mar-2010 (frima01) SIR 121619
**	    Use ALTERDB_MSG_LEN = 132 + DB_MAXNAME + 1 for message buffers.
[@history_template@]...
**/


/* Static function prototypes. */

static DB_STATUS disable_jnl(
    DMF_JSX	*jsx,
    DMP_DCB	*dcb,
    DM0C_CNF	*cnf);

static DB_STATUS alter_jnl_blkcnt(
    DMF_JSX	*jsx,
    DMP_DCB	*dcb,
    DM0C_CNF    *cnf);

static DB_STATUS alter_jnl_bksz(
    DMF_JSX	*jsx,
    DMP_DCB	*dcb,
    DM0C_CNF    *cnf);

static DB_STATUS alter_jnl_maxcnt(
    DMF_JSX	*jsx,
    DMP_DCB	*dcb,
    DM0C_CNF    *cnf);

static DB_STATUS delete_oldest_ckp(
    DMF_JSX     *jsx,
    DMP_DCB     *dcb,
    DM0C_CNF    **cnf);

static DB_STATUS delete_invalid_ckp(
    DMF_JSX     *jsx,
    DMP_DCB     *dcb,
    DM0C_CNF    **cnf,
    bool        only_at_start);

static DB_STATUS convert_unicode(
    DMF_JSX     *jsx,
    DMP_DCB     *dcb,
    DM0C_CNF    *cnf);

static DB_STATUS set_pmode(
    DMF_JSX     *jsx,
    DMP_DCB     *dcb,
    DM0C_CNF    *cnf);

static DB_STATUS set_accmode(
    DMF_JSX     *jsx,
    DMP_DCB	*dcb,
    DM0C_CNF    *cnf);

static DB_STATUS switch_journal(
    DMF_JSX     *jsx,
    DMP_DCB	*dcb,
    DM0C_CNF    **cnf);

static DB_STATUS alter_mvcc(
    DMF_JSX     *jsx,
    DMP_DCB	*dcb,
    DM0C_CNF    *cnf);

# define	ALTERDB_MSG_LEN	132 + DB_MAXNAME + 1

/*{
** Name: dmfalter	- Alter database state or characteristics.
**
** Description:
**	This routine is the entry point for the ALTERDB command.
**
**	Alterdb is designed to be an entry point to the DMFJSP process.
**	A user request for ALTERDB is translated into a call to DMFJSP
**	with the "alterdb" option.  The JSP parses the command line and
**	then calls the dmfalter routine in this module to do alterdb
**	processing.
**
**	Alterdb is used to change a database's state or characteristics.
**	The currently supported options to alterdb are:
**
**	    -disable_journaling
**	    -max_jnl_blocks
**	    -jnl_block_size
**	    -delete_oldest_ckp
*           -delete_invalid_ckp
**          -convert_unicode
**	    -production=(on|off)
**	    -disable_mvcc
**	    -enable_mvcc
**	    (-|+)w
**
**	The meaning of each alterdb option is described below:
**
**	DISABLE_JOURNALING  - immediately turn off journaling on a database.
**
**	    The disable journaling option is used primarily as a workaround
**	    for journaling system problems which prevent the archiver from
**	    moving log records for a database from the Log File to the
**	    database journal file.
**
**	    Disabling Journaling is a way to dynamically turn off journaling
**	    on the database without having to first flush out all journaled
**	    records awaiting processing in the Log File.
**
**	    All journaling work will be skipped on the database until a
**	    new checkpoint is taken to enable journaling : ckpdb +j.
**
**	    The database is marked as not be journaled by clearning the 
**	    JOURNAL flag in the database config file, and a new flag -
**	    JOURNAL_DISABLE - is asserted.
**
**	    Any database which have opened the database before the execution
**	    of this command will continue to treat it as journaled and
**	    will write journal records to the Log File.  Those records
**	    will be skipped by the archiver.
**
**	    If the specified database is not journaled or has already had
**	    journaling disabled, then the request will return with a warning.
**
**	    The function prototype for this routine is defined in DMFJSP.H.
**
**	TARGET_JNL_BLOCKS  - Alter the size of target journal files.
**
**	    Journal files are composed of a number of blocks of a certain 
**	    size. Users desiring to change the number of blocks in a journal 
**	    file -- and hence the size of the file -- do so via this option.
**
**	    The command may be entered at any time, and results in an 
**	    update of the database configuration file both on the database
**	    and journal locations.  The archiver will use this new value 
**	    the next time the config file is read.
**
**	JNL_BLOCK_SIZE  - Alter the journal file block size.
**
**	    The journal file block size is a primary factor in archiver 
**	    performance.  Large block sizes are recommended for high
**	    volume journaled databases, with the largest size supported by 
**	    the operating system usually resulting in the best performance.
**	    The block size may be altered by this option.
**	    
**	    Altering the journal file block size affects the size of the
**	    target journal file.  Accordingly, one usually adjusts 
**	    the TARGET_JNL_BLOCKS and JNL_BLOCK_SIZE parameters together.
**
**	    This command cannnot be executed when a journaled database is 
**	    open.  Either journaling must be disabled or the database closed
**	    prior to executing the command.
**
**	DELETE_OLDEST_CKP  - Delete the oldest set of journal/dump/ckp files.
**
**	    This option is used to free up disk space associated with 
**	    the oldest checkpoint.  It can be used when a checkpoint
**	    fails because of no available disk space, or during normal 
**	    maintenance.  It will not delete the only remaining valid
**	    checkpoint.
**
**      PRODUCTION=(ON|OFF) - Set the mode of the database
**
**          This option is used to set the production mode either on or off.
**          When the database is in production mode, DDL statements, and any
**          other actions that change the contents of system catalogs in
**          the database are disallowed
**
**      (+|-)W - wait until the database is available
**
**          If the database is used by another process or thread, +w makes
**          us wait until the database is available. -w forces us to fail
**          with an error.
**
**	DELETE_INVALID_CKP  - Deletes all invalid set of journal/dump/ckp files.
**
**	    This option is used to free up disk space associated with
**	    the invalid checkpoints.  It can be used when a checkpoint
**	    fails because of no available disk space, or during normal
**	    maintenance.
**
**     CONVERT_UNICODE - Converts a non-Unicode database into Unicode.  
**         
**         This option is used to convert a non-Unicode database into
**         Unicode database.It can be used when making a transition 
**         into Unicode enabled environment.  
**
**     ENABLE/DISABLE_MVCC - Enables or disables MVCC in a database.
**         
**         This option may be used to disable/enable MVCC locking mode
**	   in a database, which is by default enabled.
**
** Inputs:
**      journal_context			Pointer to DMF_JSX
**	dcb				Pointer to DCB.
**
** Outputs:
**      err_code                        Reason for error return status.
**					    E_DM1400_ALT_ERROR_ALTERING_DB
**
**	Returns:
**	    E_DB_OK			Operation successfull.
**	    E_DB_WARN			Operation not done.
**	    E_DB_ERROR			Operation failed.
**	Exceptions:
**	    none
**
** Side Effects:
**	none
**
** History:
**	25-feb-1991 (rogerk)
**	    Created for Archiver Stability project.
**	26-apr-1993 (jnash)
**	    Add support for -jnl_blk_size, -max_jnl_blocks and 
**	    -init_jnl_blocks options.
**	20-sep-1993 (jnash)
**	    Add support for -delete_oldest_ckp.
**	18-oct-1993 (jnash)
**	    Fix bug in param checking.
**      01-sep-1995 (tchdi01)
**          Added check for the JSX2_PMODE flag
** 	14-apr-1997 (wonst02)
** 	    Added JSX2_READ_ONLY and JSX2_READ_WRITE for readonly support.
**      13-Apr-2004 (nansa02)
**          Added support for -delete_invalid_ckp and    
**          -convert_unicode.
**	21-feb-05 (inkdo01)
**	    Added support for -i flag (Unicode conversion with NFC).
**      28-Oct-2008 (hanal04) Bug 121140
**          If switch_journal() fails break out of the outer loop to
**          report failure.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Added enable/disable MVCC.
[@history_template@]...
*/
DB_STATUS
dmfalter(
DMF_JSX	    *jsx,
DMP_DCB	    *dcb)
{
    DM0C_CNF		*cnf;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		local_status;
    i4		local_error;
    bool		config_open = FALSE;
    STATUS		error;
    char		local_buffer[ALTERDB_MSG_LEN];
    i4			jnl_fil_seq;
    DB_ERROR		local_dberr;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Check for possible options.
    */
    if (((jsx->jsx_status & (JSX_JDISABLE | JSX_ALTDB_CKP_DELETE )) == 0) &&  
         ((jsx->jsx_status1 & 
          (JSX_JNL_BLK_SIZE | JSX_JNL_NUM_BLOCKS | JSX_JNL_INIT_BLOCKS)) == 0) &&
         ((jsx->jsx_status2 & 
	  (JSX2_PMODE | JSX2_OCKP | JSX2_READ_ONLY | JSX2_READ_WRITE |
	   JSX2_SWITCH_JOURNAL | JSX2_ALTDB_CKP_INVALID | JSX2_ALTDB_UNICODED |
	   JSX2_ALTDB_UNICODEC | JSX2_ALTDB_DMVCC | JSX2_ALTDB_EMVCC))==0))
    {
	return (E_DB_OK);
    }

    for(;;)
    {
	SXF_ACCESS          access;
	/*
	** Open the database config file.
	*/
	status = dm0c_open(dcb, 0, dmf_svcb->svcb_lock_list, &cnf, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	    break;
	config_open = TRUE;

	/*
	**	Make sure access is security audited.
	*/
	if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
	{
	    access = SXF_A_ALTER;
	    if (status)
		access |= SXF_A_FAIL;
	    else
		access |= SXF_A_SUCCESS;

	    local_status = dma_write_audit( SXF_E_DATABASE,
			    access,
			    dcb->dcb_name.db_db_name, /* Object name (database) */
			    sizeof(dcb->dcb_name.db_db_name), /* Object name (database) */
			    &dcb->dcb_db_owner, /* Object owner */
			    I_SX270D_ALTERDB,   /*  Message */
			    TRUE,		    /* Force */
			    &local_dberr, NULL);
	}
	/*
	** Check for alter action.
	*/
	if (jsx->jsx_status & JSX_JDISABLE)
	{
	    /*
	    ** Make call to disable journaling on the database.
	    ** This call may return a warning if journaling is already
	    ** disabled on the database.
	    */
	    status = disable_jnl(jsx, dcb, cnf);
	    if (status != E_DB_OK)
		break;
	}
	else if (jsx->jsx_status1 & JSX_JNL_BLK_SIZE)
	{
	    /*
	    ** Change the journal file block size.
	    ** This call will return a warning if the database is journaled.
	    */
	    status = alter_jnl_bksz(jsx, dcb, cnf);
	    if (status != E_DB_OK)
		break;
	}
	else if (jsx->jsx_status1 & JSX_JNL_NUM_BLOCKS)
	{
	    /*
	    ** Change the number of blocks in a journal file.
	    */
	    status = alter_jnl_maxcnt(jsx, dcb, cnf);
	    if (status != E_DB_OK)
		break;
	}
	else if (jsx->jsx_status1 & JSX_JNL_INIT_BLOCKS)
	{
	    /*
	    ** Change the initial journal file allocation amount.
	    */
	    status = alter_jnl_blkcnt(jsx, dcb, cnf);
	    if (status != E_DB_OK)
		break;
	}
	else if (jsx->jsx_status & JSX_ALTDB_CKP_DELETE)
	{
	    /*
	    ** Config file needs to be closed for delete_oldest_ckp(), is
	    ** reopened on return.
	    */
	    status = dm0c_close(cnf, 0, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
		break;

	    /*
	    ** Delete oldest journal/dump version.  
	    */
	    status = delete_oldest_ckp(jsx, dcb, &cnf);
	    if (status != E_DB_OK)
		break;
	}
        
         else if (jsx->jsx_status2 & JSX2_ALTDB_CKP_INVALID)
        {
            /*
            ** Config file needs to be closed for delete_invalid_ckp(), is
            ** reopened on return.
            */
	    status = dm0c_close(cnf, 0, &jsx->jsx_dberr);
            if (status != E_DB_OK)
		break;

            /*
            ** Delete specific journal/dump version.
            */
            status = delete_invalid_ckp(jsx, dcb, &cnf, FALSE);
            if (status != E_DB_OK)
               break;
        }
         
        else if (jsx->jsx_status2 & (JSX2_ALTDB_UNICODEC | JSX2_ALTDB_UNICODED))
        {
            /* Change non-Unicode database into Unicode */
            status = convert_unicode(jsx, dcb, cnf);
            if (DB_FAILURE_MACRO(status))
                break;
        }

        else if (   (jsx->jsx_status2 & JSX2_PMODE)
		 || (jsx->jsx_status2 & JSX2_OCKP))
        {
            /* Set production mode or enable/disable online checkpoint */
            status = set_pmode(jsx, dcb, cnf);
            if (DB_FAILURE_MACRO(status))
                break;
        }
	else if (jsx->jsx_status2 & (JSX2_READ_ONLY | JSX2_READ_WRITE))
	{
	    /* Set the readonly or read-write access mode */
	    status = set_accmode(jsx, dcb, cnf);	   
            if (DB_FAILURE_MACRO(status))
                break;
	}
	else if (jsx->jsx_status2 & (JSX2_SWITCH_JOURNAL))
	{
	    /* Save current jnl_fil_seq so we can verify switch completed */
	    jnl_fil_seq = cnf->cnf_jnl->jnl_fil_seq;

	    for ( ; ; )
	    {
		/* Close the current journal and start a new one */
		status = switch_journal(jsx, dcb, &cnf);	   
		if (DB_FAILURE_MACRO(status))
		    break;

		/* verify switch journal completed */
		if (jnl_fil_seq == cnf->cnf_jnl->jnl_fil_seq)
		{
		    continue;
		}

		/* switch complete */
		TRformat(dmf_put_line, 0, local_buffer, sizeof(local_buffer),
		    "%@ ALTERDB: Next Journal is now active for Database '%~t'.\n",
		    sizeof(dcb->dcb_name), &dcb->dcb_name);

		break;
	    }
            if (DB_FAILURE_MACRO(status))
                break;
	}
	else if ( jsx->jsx_status2 & (JSX2_ALTDB_DMVCC | JSX2_ALTDB_EMVCC) )
	{
	    /* Enable/disable MVCC in database */
	    status = alter_mvcc(jsx, dcb, cnf);
            if (DB_FAILURE_MACRO(status))
                break;
	}

	/*
	** Close and update the config file.
	*/
	status = dm0c_close(cnf, (DM0C_UPDATE | DM0C_COPY), &jsx->jsx_dberr);
	if (status != E_DB_OK)
	    break;
	config_open = FALSE;

	break;
    }

    if ((config_open) && (cnf))
    {
	local_status = dm0c_close(cnf, 0, &local_dberr);
	if (local_status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
		(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
		&local_error, 0);
	}
    }

    /*
    ** Check for error in Alterdb processing.
    **
    ** Note that some options may return warnings.  These should be
    ** returned back to the caller with the err_code left to the warning
    ** value (rather than reset here to give a traceback).
    */
    if (DB_FAILURE_MACRO(status))
    {
	if (jsx->jsx_dberr.err_code > E_DM_INTERNAL)
	{
	    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
		(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
		&local_error, 0);

	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1400_ALT_ERROR_ALTERING_DB);
	}
    }

    return (status);
}

/*{
** Name: disable_jnl	- disable journaling on database
**
** Description:
**	This routine is called to disable journaling on a database.
**
**	This routine will turn off the JOURNAL status and turn on
**	the JOURNAL_DISABLE status on the database.
**
**	The caller must have opened the database config file for
**	writing and pass in a pointer to the config file data.
**
**	Upon return, the caller should write out the new contents of
**	the config file.
**
**	If journaling is not enabled or has already been disabled, this
**	routine will return with a warning.
**
** Inputs:
**      jsx				Journal context.
**	dcb				Database context.
**	cnf				Config file control block.
**
** Outputs:
**      err_code                        Reason for error return status.
**					    E_DM1402_ALT_DB_NOT_JOURNALED
**					    E_DM1403_ALT_JNL_NOT_ENABLED
**					    E_DM1404_ALT_JNL_DISABLE_ERR
**
**	Returns:
**	    E_DB_OK			Operation successfull.
**	    E_DB_WARN			Operation not done.
**	    E_DB_ERROR			Operation failed.
**	Exceptions:
**	    none
**
** Side Effects:
**	An warning message is written to the log file to indicate that
**	journaling has been disabled.
**
**	The database is marked with JOURNAL_DISABLE status which will cause
**	the archiver - if it is signaled to process records for this
**	database - to skip work on the db and to write a warning message
**	to the error log.
**
** History:
**	25-feb-1991 (rogerk)
**	    Created for Archiver Stability project.
**	20-sep-1993 (jnash)
**	    Remove JSX_VERBOSE TRformat(), now done in any case.
**	22-nov-1993 (jnash)
**	    B56095.  When journaling disabled, also disable rollfowarddb #c
**	    from a prior checkpoint.
**	24-may-96 (nick)
**	    Zero out last log address journaled ; if we don't do this we
**	    can't recover from the situation where we had a corrupt value
**	    here which precipitated the disabling of journaling in the
**	    first place.
[@history_template@]...
*/
static DB_STATUS
disable_jnl(
DMF_JSX	    *jsx,
DMP_DCB	    *dcb,
DM0C_CNF    *cnf)
{
    i4		local_error;
    char		local_buffer[ALTERDB_MSG_LEN];

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Verify that the database is journaling and not already disabled.
    */
    if (cnf->cnf_dsc->dsc_status & DSC_DISABLE_JOURNAL)
    {
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1403_ALT_JNL_NOT_ENABLED);
	return (E_DB_WARN);
    }

    if ((cnf->cnf_dsc->dsc_status & DSC_JOURNAL) == 0)
    {
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1402_ALT_DB_NOT_JOURNALED);
	return (E_DB_WARN);
    }

    TRformat(dmf_put_line, 0, local_buffer, sizeof(local_buffer),
	"%@ ALTERDB: Disabling Journaling for Database '%~t'.\n",
	sizeof(dcb->dcb_name), &dcb->dcb_name);

    /*
    ** Log informational message to the error log indicating that 
    ** journaling has been disabled.  Write database name to local buffer
    ** so we can strip blanks off of it.
    */
    STncpy( local_buffer, dcb->dcb_name.db_db_name, DB_MAXNAME);
    local_buffer[ DB_MAXNAME ] = '\0';
    STtrmwhite(local_buffer);
    uleFormat(NULL, E_DM1401_ALT_JOURNAL_DISABLED, (CL_ERR_DESC *)NULL, ULE_LOG,
	(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
	&local_error, 1, 0, local_buffer);


    /*
    ** Turn off journaling status on database.
    ** Mark the database as JOURNAL_DISABLED.
    */
    cnf->cnf_dsc->dsc_status &= ~DSC_JOURNAL;
    cnf->cnf_dsc->dsc_status |= DSC_DISABLE_JOURNAL;

    /*
    ** Disable rolldb's from a point prior to this.
    */
    cnf->cnf_jnl->jnl_first_jnl_seq  = 0;
    cnf->cnf_jnl->jnl_la.la_sequence = 0;
    cnf->cnf_jnl->jnl_la.la_block    = 0;
    cnf->cnf_jnl->jnl_la.la_offset   = 0;

    return (E_DB_OK);
}

/*{
** Name: alter_mvcc	- enable/disable MVCC access to a database
**
** Description:
**	Enables or disables MVCC access to a database.
**
**	By default, MVCC is enabled, but may be switched off
**	and back on again if one wishes.
**
**	The runtime change in MVCC status will not occur until the next
**	time the database is opened.
**
** Inputs:
**      jsx				Journal context.
**	dcb				Database context.
**	cnf				Config file control block.
**
** Outputs:
**      err_code                        Reason for error return status.
**					    E_DM1402_ALT_DB_NOT_JOURNALED
**					    E_DM1403_ALT_JNL_NOT_ENABLED
**					    E_DM1404_ALT_JNL_DISABLE_ERR
**
**	Returns:
**	    E_DB_OK			Operation successfull.
**	    E_DB_WARN			Operation not done.
**	    E_DB_ERROR			Operation failed.
**	Exceptions:
**	    none
**
** Side Effects:
**	An warning message is written to the log file to indicate that
**	MVCC has been enabled or disabled.
**
**	The config file is marked with DSC_NOMVCC when disabled, reset
**	when enabled.
**
** History:
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Created.
[@history_template@]...
*/
static DB_STATUS
alter_mvcc(
DMF_JSX	    *jsx,
DMP_DCB	    *dcb,
DM0C_CNF    *cnf)
{
    i4			local_error, err_code;
    char		local_buffer[ALTERDB_MSG_LEN];

    CLRDBERR(&jsx->jsx_dberr);

    if ( jsx->jsx_status2 & JSX2_ALTDB_DMVCC )
    {
	/* Disable MVCC unless already disabled */
        if ( cnf->cnf_dsc->dsc_status & DSC_NOMVCC )
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1411_ALT_MVCC_NOT_ENABLED);
	    return (E_DB_WARN);
	}
	cnf->cnf_dsc->dsc_status |= DSC_NOMVCC;
	err_code = E_DM1413_ALT_MVCC_DISABLED;
    }

    if ( jsx->jsx_status2 & JSX2_ALTDB_EMVCC )
    {
	/* Enable MVCC unless already enabled */
        if ( !(cnf->cnf_dsc->dsc_status & DSC_NOMVCC) )
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1412_ALT_MVCC_NOT_DISABLED);
	    return (E_DB_WARN);
	}
	cnf->cnf_dsc->dsc_status &= ~DSC_NOMVCC;
	err_code = E_DM1414_ALT_MVCC_ENABLED;
    }

    TRformat(dmf_put_line, 0, local_buffer, sizeof(local_buffer),
	"%@ ALTERDB: MVCC %s for Database '%~t'.\n",
	    (cnf->cnf_dsc->dsc_status & DSC_NOMVCC)
		? "disabled" : "enabled",
	    sizeof(dcb->dcb_name), &dcb->dcb_name);

    /*
    ** Log informational message to the error log indicating that 
    ** enabled or disabled.  Write database name to local buffer
    ** so we can strip blanks off of it.
    */
    STncpy( local_buffer, dcb->dcb_name.db_db_name, DB_MAXNAME);
    local_buffer[ DB_MAXNAME ] = '\0';
    STtrmwhite(local_buffer);
    uleFormat(NULL, err_code, (CL_ERR_DESC *)NULL, ULE_LOG,
	(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
	&local_error, 1, 0, local_buffer);

    return (E_DB_OK);
}

/*{
** Name: alter_jnl_bksz	- Alter the journal file block size.
**
** Description:
**	This routine is called to Alter the journal file block size.
**
**	The caller must have opened the database config file for
**	writing and pass in a pointer to the config file data.
**
**	Upon return, the caller should write out the new contents of
**	the config file.
**
**	If the database is open and is journaled, the routine will 
**	return with a warning.  The block size cannot be modified unless
**	these conditions are met.
**
** Inputs:
**      jsx				Journal context.
**	dcb				Database context.
**	cnf				Config file control block.
**
** Outputs:
**      err_code                        Reason for error return status.
**					    E_DM1402_ALT_DB_NOT_JOURNALED
**					    E_DM1403_ALT_JNL_NOT_ENABLED
**					    E_DM1404_ALT_JNL_DISABLE_ERR
**
**	Returns:
**	    E_DB_OK			Operation successfull.
**	    E_DB_WARN			Operation not done.
**	Exceptions:
**	    none
**
** Side Effects:
**	A warning message is written to the errlog to indicate that
**	this change has been made.
**
** History:
**	26-apr-1993 (jnash)
**	    Created.
**	20-sep-1993 (jnash)
**	    Remove JSX_VERBOSE TRformat(), now done in any case.
[@history_template@]...
*/
static DB_STATUS
alter_jnl_bksz(
DMF_JSX	    *jsx,
DMP_DCB	    *dcb,
DM0C_CNF    *cnf)
{
    i4	bksz = jsx->jsx_jnl_block_size;
    i4	local_error;
    char	local_buffer[ALTERDB_MSG_LEN];
    DB_STATUS	status;

    CLRDBERR(&jsx->jsx_dberr);

    /* 
    ** We allow this change only when the database is not currently
    ** journaled.  Otherwise we would have a journal file with more than 
    ** one block size. 
    */
    if ((cnf->cnf_dsc->dsc_status & DSC_JOURNAL) &&
	  (cnf->cnf_dsc->dsc_status & DSC_DISABLE_JOURNAL) == 0 )
    {
	/*
	** Database is journaled, block size cannot be changed.
	*/
	TRformat(dmf_put_line, 0, local_buffer, sizeof(local_buffer),
	    "%@ ALTERDB: Database '%~t' is journaled, block size cannot be changed.\n",
	    sizeof(dcb->dcb_name), &dcb->dcb_name);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1041_JSP_JNL_BKSZ_ERR);
	return (E_DB_WARN);
    }

    /*
    ** Check for valid block sizes.  Note that any changes to this list must 
    ** correspond to similar changes in jf.c
    */
    if ((bksz != 4096) && (bksz != 8192) && 
	(bksz != 16384) && (bksz != 32768) && (bksz != 65536))
    {
	TRformat(dmf_put_line, 0, local_buffer, sizeof(local_buffer),
	    "%@ ALTERDB: '%d' is not a valid journal block size.\n", bksz);

	SETDBERR(&jsx->jsx_dberr, 0, E_DM1041_JSP_JNL_BKSZ_ERR);
	return (E_DB_WARN);
    }

    TRformat(dmf_put_line, 0, local_buffer, sizeof(local_buffer),
	"%@ ALTERDB: Changed Journal Block Size for Database '%~t' to %d bytes.\n",
	sizeof(dcb->dcb_name), &dcb->dcb_name, bksz);

    /*
    ** Log informational message to the error log indicating that 
    ** the block size has been changed.
    */
    STncpy( local_buffer, dcb->dcb_name.db_db_name, DB_MAXNAME);
    local_buffer[ DB_MAXNAME ] = '\0';
    STtrmwhite(local_buffer);

    uleFormat(NULL, E_DM103E_JSP_JNL_BKSZ_CHGD, (CL_ERR_DESC *)NULL, ULE_LOG,
	(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
	&local_error, 3, 0, cnf->cnf_jnl->jnl_bksz, 0, bksz, 0, 
	local_buffer);

    /*
    ** Change the journal file block size
    */
    cnf->cnf_jnl->jnl_bksz = bksz;

    return (E_DB_OK);
}

/*{
** Name: alter_jnl_maxcnt	- Alter the number of block in a journal file.
**
** Description:
**	This routine is called to alter the target size of a journal file
**	by changing the number of blocks in the file.
**
**	The caller must have opened the database config file for
**	writing and pass in a pointer to the config file data.
**
**	Upon return, the caller should write out the new contents of
**	the config file.
**
**	This call may be made at any time.  If made while the database 
**	is open and journaled, the archiver will notice the new size the 
**	next time the config file is opened.
**
** Inputs:
**      jsx				Journal context.
**	dcb				Database context.
**	cnf				Config file control block.
**
** Outputs:
**      err_code                        Reason for error return status.
**
**	Returns:
**	    E_DB_OK			Operation successfull.
**	    E_DB_WARN			Operation not done.
**	Exceptions:
**	    none
**
** Side Effects:
**	A warning message is written to the errlog to indicate that
**	this change has been made.
**
** History:
**	26-apr-1993 (jnash)
**	    Created.
**	20-sep-1993 (jnash)
**	    Remove JSX_VERBOSE TRformat(), now done in any case.
[@history_template@]...
*/
static DB_STATUS
alter_jnl_maxcnt(
DMF_JSX	    *jsx,
DMP_DCB	    *dcb,
DM0C_CNF    *cnf)
{
    i4	local_error;
    i4	max_blocks = jsx->jsx_max_jnl_blocks;
    char	local_buffer[ALTERDB_MSG_LEN];

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Check for valid number of blocks.  
    */
    if ((max_blocks < 32) ||
        (max_blocks > 65536)) 			/* arbitrary */
    {
	TRformat(dmf_put_line, 0, local_buffer, sizeof(local_buffer),
	    "%@ ALTERDB: '%d' is not a valid number of journal blocks.\n",
	    max_blocks);

	SETDBERR(&jsx->jsx_dberr, 0, E_DM1040_JSP_JNL_SIZE_ERR);
	return (E_DB_WARN);
    }

    TRformat(dmf_put_line, 0, local_buffer, sizeof(local_buffer),
	"%@ ALTERDB: Changed target journal file size for database '%~t' to %d blocks.\n",
	sizeof(dcb->dcb_name), &dcb->dcb_name, max_blocks);

    /*
    ** Log informational message to the error log indicating that 
    ** the block size has been changed.
    */
    STncpy( local_buffer, dcb->dcb_name.db_db_name, DB_MAXNAME);
    local_buffer[ DB_MAXNAME ] = '\0';
    STtrmwhite(local_buffer);

    uleFormat(NULL, E_DM103D_JSP_JNL_SIZE_CHGD, (CL_ERR_DESC *)NULL, ULE_LOG,
	(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
	&local_error, 3, 0, cnf->cnf_jnl->jnl_maxcnt, 0, max_blocks, 0, 
	local_buffer);

    /*
    ** Change the target allocation.
    */
    cnf->cnf_jnl->jnl_maxcnt = max_blocks;

    /*
    ** If the target allocation is greater than the configured initial 
    ** allocation, also alter the initial allocation amount. 
    */
    if (cnf->cnf_jnl->jnl_blkcnt > max_blocks)
	cnf->cnf_jnl->jnl_blkcnt = max_blocks;

    return (E_DB_OK);
}

/*{
** Name: alter_jnl_blkcnt	- Alter the initial journal file allocation.
**
** Description:
**	This routine is called to alter the amount of space allocated to
**	a journal file when that file is created.
**
**	The caller must have opened the database config file for
**	writing and pass in a pointer to the config file data.
**
**	Upon return, the caller should write out the new contents of
**	the config file.
**
**	This call may be made at any time.  If made while the database 
**	is open and journaled, the archiver will notice the new allocation the 
**	next time a journal file is created.
**
** Inputs:
**      jsx				Journal context.
**	dcb				Database context.
**	cnf				Config file control block.
**
** Outputs:
**      err_code                        Reason for error return status.
**
**	Returns:
**	    E_DB_OK			Operation successfull.
**	    E_DB_WARN			Operation not done.
**	Exceptions:
**	    none
**
** Side Effects:
**	A warning message is written to the errlog to indicate that
**	this change has been made.
**
** History:
**	26-apr-1993 (jnash)
**	    Created.
**	20-sep-1993 (jnash)
**	    Remove JSX_VERBOSE TRformat(), now done in any case.
[@history_template@]...
*/
static DB_STATUS
alter_jnl_blkcnt(
DMF_JSX	    *jsx,
DMP_DCB	    *dcb,
DM0C_CNF    *cnf)
{
    i4	local_error;
    i4	init_blocks = jsx->jsx_init_jnl_blocks;
    char	local_buffer[ALTERDB_MSG_LEN];

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** Check for valid number of blocks.  
    */
    if ((init_blocks < 0) || (init_blocks > cnf->cnf_jnl->jnl_maxcnt))
    {
	TRformat(dmf_put_line, 0, local_buffer, sizeof(local_buffer),
	    "%@ ALTERDB: '%d' blocks is greater than target journal file size (%d blocks).\n",
	    init_blocks, cnf->cnf_jnl->jnl_maxcnt);

	SETDBERR(&jsx->jsx_dberr, 0, E_DM1042_JSP_JNL_BKCNT_ERR);
	return (E_DB_WARN);
    }

    TRformat(dmf_put_line, 0, local_buffer, sizeof(local_buffer),
	"%@ ALTERDB: Changed initial journal file allocation size for database '%~t' to %d blocks.\n",
	sizeof(dcb->dcb_name), &dcb->dcb_name, init_blocks);

    /*
    ** Log informational message to the error log indicating that 
    ** the block size has been changed.
    */
    STncpy(local_buffer, dcb->dcb_name.db_db_name, DB_MAXNAME);
    local_buffer[ DB_MAXNAME ] = '\0';
    STtrmwhite(local_buffer);

    uleFormat(NULL, E_DM103F_JSP_JNL_BKCNT_CHGD, (CL_ERR_DESC *)NULL, ULE_LOG,
	(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
	&local_error, 3, 0, cnf->cnf_jnl->jnl_blkcnt, 0, init_blocks, 0, 
	local_buffer);

    /*
    ** Change the initial allocation.
    */
    cnf->cnf_jnl->jnl_blkcnt = init_blocks;

    return (E_DB_OK);
}

/*{
** Name: delete_oldest_ckp	- Delete the oldest ckp/journal/dump set.
**
** Description:
**	This routine is called to delete the oldest checkpoint and to 
**	update journal & dump information.
**
** Inputs:
**      jsx				Journal context.
**	dcb				Database context.
**
** Outputs:
**	config				Ptr to ptr to config file, zero
**					 if not opened.
**      err_code                        Reason for error return status.
**
**	Returns:
**	    E_DB_OK			Operation successfull.
**	    E_DB_INFO 			If there are no available
**					 checkpoints to delete. 
**	    E_DB_ERROR 			Error, including database in use.
**	Exceptions:
**	    none
**
** Side Effects:
**	The config file is opened and remains open on normal exit, is
**	closed if error.
**	A warning message is written to the errlog to indicate that
**	this change has been made.
**
** History:
**	20-sep-1993 (jnash)
**	    Created.
**	31-jan-1994 (jnash)
**	    Update config file jnl_first_jnl_seq when deleting oldest 
**	    checkpoint.
**	28-mar-1994 (jnash)
**	    Fix AV by initinalizing cnf.
**      24-jan-1995 (stial01)
**          BUG 66473: ckp_mode is now bit pattern.
**	21-feb-1995 (shero03)
**	    Bug #66987
**	    Don't delete the last database checkpoint if there are
**	    table check points
**      01-sep-1995 (tchdi01)
**          Added support for the +w option which can be used to make
**          the program wait until the exclusive lock on the database
**          held by another program is released
**	19-May-1998 (devjo01)
**	    Corrected 'cnf' usage.  Was pointing to deallocated memory
**	    which sometimes contained configuration information from
**	    previous call to dm0c_open, and sometimes trash. (b90918)
**	11-apr-2001 (rigka01)
**	    Bug 104242 : auditdb <testdb> -delete_oldest_ckp may delete
**	    the only valid checkpoint.  This occurs when the oldest
**	    checkpoint is the only valid checkpoint and the most recent
**	    checkpoint is not valid.  
**	20-oct-2006 (kibro01)
**	    Rework including change for jsp_file_maint changes and
**	    some simplification of redundant variables
**	13-nov-2006 (kibro01)
**	    Scan forwards, not backwards, through journals, otherwise
**	    we find journal 0 which is always valid, so think there is 
**	    one to delete in the case where there is a valid table-only
**	    checkpoint beyond the first.
**	9-May-2008 (kibro01) b120373
**	    Support -keep=n checkpoints
[@history_template@]...
*/
static DB_STATUS
delete_oldest_ckp(
DMF_JSX	    *jsx,
DMP_DCB	    *dcb,
DM0C_CNF    **config)
{
    DM0C_CNF		*cnf;
    i4			most_recent_valid_ckp;
    i4			local_error;
    i4			i;
    i4			j;
    DB_STATUS		local_status;
    DB_STATUS		status;
    char		local_buffer[ALTERDB_MSG_LEN];
    bool     		dump_history_found;
    bool     		db_open = FALSE;
    bool		other_valid_ckps = FALSE;
    bool     		config_open = FALSE;
    i4    		open_flags;
    i4			this_seq;
    bool		all_ok = TRUE;
    i4			keep_n = jsx->jsx_keep_ckps;
    i4			keep_n_this;
    bool		deleted_one = FALSE;
    DB_ERROR		local_dberr;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    *config = 0;

    /*
    ** Open the database to: 
    ** 1) Ensure that no one else has it opened exclusively 
    ** 2) Allow jsp_file_maint() to work.
    */
    open_flags = DM2D_IGNORE_CACHE_PROTOCOL;
    if ((jsx->jsx_status & JSX_WAITDB) == 0)
        open_flags |=  DM2D_NOWAIT;
    status = dm2d_open_db(dcb, DM2D_A_READ, DM2D_IS,
	    dmf_svcb->svcb_lock_list, open_flags, &jsx->jsx_dberr);

    /*
    ** Perform any security audits.
    ** We do this before checking the result of the OPEN since
    ** want to audit failed open attempts as well as successfull ones.
    */
    if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
    {
	SXF_ACCESS          access;
	/*
	**	Record we are checkpointing this database,
	**	checkpoint can be regarded as selecting from database
	*/
	access = SXF_A_SELECT;
	if (status)
	    access |= SXF_A_FAIL;
	else
	    access |= SXF_A_SUCCESS;

	local_status = dma_write_audit( SXF_E_DATABASE,
	    access,
	    dcb->dcb_name.db_db_name, 	/* Object name (database) */
	    sizeof(dcb->dcb_name.db_db_name), 
	    &dcb->dcb_db_owner, 	/* Object owner */
	    I_SX270B_CKPDB,     	/*  Message */
	    TRUE,		    	/* Force */
	    &local_dberr, NULL);
    }

    /*
    ** Check for failure. Check explicitly for Database-in-use error.
    */
    if (status != E_DB_OK)
    {
	if (jsx->jsx_dberr.err_code == E_DM004C_LOCK_RESOURCE_BUSY)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM110D_CPP_DB_BUSY);
	} 
	else
	{
	    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1108_CPP_OPEN_DB);
	}
	all_ok = FALSE;
    }

    if (all_ok)
    {
	db_open = TRUE;

	status = dm0c_open(dcb, 0, dmf_svcb->svcb_lock_list, config, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	    all_ok = FALSE;
    }

    if (all_ok)
    {
	cnf = *config;
	config_open = TRUE;

	/*
	** Determine if there are any checkpoints and if so, the first
	** is the oldest.  
	*/
	cnf = *config;

	/* Cannot have a checkpoint to delete unless there are at least 2 */
	if (cnf->cnf_jnl->jnl_count < 2)
	{
	    TRformat(dmf_put_line, 0, local_buffer, sizeof(local_buffer),
		"%@ ALTERDB: No checkpoints available to delete.\n");
	    status = E_DB_INFO;
	    all_ok = FALSE;
	}
    }

    do
    {

      keep_n_this = keep_n;
      most_recent_valid_ckp = -1;

      if (all_ok)
      {
	/*
	** check that there is a second-oldest checkpoint
	*/
	dump_history_found = FALSE;

	/*
	** Scan forwards to find a valid checkpoint other than in journal 0
	** position.  If we find one we can delete the one in position 0
	** Bug 66987 ignore table checkpoints during this scan
 	*/

	for (i = 1; i < cnf->cnf_jnl->jnl_count ; i++)
	{
	    if (cnf->cnf_jnl->jnl_history[i].ckp_mode & CKP_TABLE)
		continue;

	    this_seq = cnf->cnf_jnl->jnl_history[i].ckp_sequence;
	    if (cnf->cnf_jnl->jnl_history[i].ckp_mode & CKP_ONLINE)
	    {
		dump_history_found = FALSE;
		for ( j = 0; j < cnf->cnf_dmp->dmp_count; j++)
		{
		    if (cnf->cnf_dmp->dmp_history[j].ckp_sequence == this_seq)
		    {
			dump_history_found = TRUE;
			break;
		    }
		}
	    }
	    else
	    {
		/* offline checkpoints have no associated dump history */
		dump_history_found = TRUE;
	    }

	    if (dump_history_found &&
		DMFJSP_VALID_CHECKPOINT(cnf, i, j))
	    {
	      if (most_recent_valid_ckp == -1)
	      {
		if (jsx->jsx_status & JSX_VERBOSE)
		{
		    TRformat(dmf_put_line, 0, local_buffer,
			sizeof(local_buffer),
			"%@ ALTERDB: Found VALID checkpoint seq no: %d\n",
			this_seq);
		}
		if (--keep_n_this > 0)
		    continue;
	        most_recent_valid_ckp = this_seq;
		break;
	      }
	    }
	    else if (jsx->jsx_status & JSX_VERBOSE)
	    {
		TRformat(dmf_put_line, 0, local_buffer, sizeof(local_buffer),
		    "%@ ALTERDB: Found INVALID checkpoint seq no: %d\n",
		    this_seq);
	    }
	}

	/*
	** Found a valid checkpoint other than checkpoint in jnl position 0
	** so we can delete the first one (in position 0)
	*/
	if (most_recent_valid_ckp != -1)
	{
	  status = jsp_file_maint(jsx, dcb, &cnf, 
		cnf->cnf_jnl->jnl_history[0].ckp_sequence, TRUE);
	  if (status != E_DB_OK)
	  {
	      all_ok = FALSE;
	  } 
	  else
	  {

	      /*
	      ** If the database is journaled, and if journals are valid, 
	      ** update the last valid checkpoint from which a rolldb +j 
	      ** may occur.  
	      */
	      if ( ((cnf->cnf_dsc->dsc_status & DSC_JOURNAL) &&
	         (cnf->cnf_jnl->jnl_first_jnl_seq != 0)) &&
	         (cnf->cnf_jnl->jnl_first_jnl_seq < 
	          cnf->cnf_jnl->jnl_history[0].ckp_sequence) )
	      {
	        cnf->cnf_jnl->jnl_first_jnl_seq = 
		    cnf->cnf_jnl->jnl_history[0].ckp_sequence;
	      }
	      /*
	      ** If we successfully deleted one, tidy up invalid-at-start
	      */
              status = delete_invalid_ckp(jsx, dcb, &cnf, TRUE);
	      deleted_one = TRUE;
	  }
	} /* oldest is not first valid */
	else
	{
	    if (!deleted_one)
	        TRformat(dmf_put_line, 0, local_buffer, sizeof(local_buffer),
		    "%@ ALTERDB: No checkpoints available to delete.\n");
	    status = E_DB_INFO;
	}

      }
    } while (status == E_DB_OK &&
	((jsx->jsx_status2 & JSX2_ALTDB_KEEP_N) != 0));


    if (all_ok)
    {
	status = dm2d_close_db(dcb, dmf_svcb->svcb_lock_list, 
	    DM2D_IGNORE_CACHE_PROTOCOL, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM110A_CPP_CLOSE_DB);
	}

    }

    /*
    ** Preserve possible E_DB_INFO status. 
    */
    if (DB_FAILURE_MACRO(status))
    {
	if (db_open)
	{
	    (VOID)dm2d_close_db(dcb, dmf_svcb->svcb_lock_list, 
		DM2D_IGNORE_CACHE_PROTOCOL, &local_dberr);
	}

	if (config_open)
	{
	    (VOID)dm0c_close(*config, 0, &local_dberr);
	    *config = 0;
	}

	return(E_DB_ERROR);
    }

    return(status);
}

/*{
Name: delete_invalid_ckp	- Delete the invalid ckp/journal/dump set.
**
** Description:
**	This routine is called to delete the invalid checkpoints and to
**	update journal & dump information.
**
** Inputs:
**      jsx				Journal context.
**	dcb				Database context.
**
** Outputs:
**	config				Ptr to ptr to config file, zero
**					 if not opened.
**      err_code                        Reason for error return status.
**
**	Returns:
**	    E_DB_OK			Operation successfull.
**	    E_DB_INFO 			If there are no available
**					 checkpoints to delete.
**	    E_DB_ERROR 			Error, including database in use.
**	Exceptions:
**	    none
**
** Side Effects:
**	The config file is opened and remains open on normal exit, is
**	closed if error.
**	A warning message is written to the errlog to indicate that
**	this change has been made.
**
** History:13-Apr-2004 (nansa02)
**	    Created.
**	19-oct-06 (kibro01) bug 116436
**	    Rework deletion of checkpoints, especially invalid.
**	    Allow deletion of invalid checkpoints with no dump files
**	    Allow partial deletion of invalid checkpoints
**	31-Mar-08 (kibro01) b119961
**	    A dump file on its own is effectively invalid, as its journal
**	    entry is now gone.  Allow it to be cleared up when removing
**	    invalid checkpoints.
*/


static DB_STATUS
delete_invalid_ckp(
DMF_JSX	    *jsx,
DMP_DCB	    *dcb,
DM0C_CNF    **config,
bool        only_at_start)
{
    DM0C_CNF		*cnf = NULL;
    i4			most_recent_valid_ckp=-1;
    i4			local_error;
    i4			i;
    i4			j;
    i4   		dmp_history_no;
    DB_STATUS		local_status;
    DB_STATUS		status = E_DB_OK;
    char		local_buffer[ALTERDB_MSG_LEN];
    bool     		dump_history_found;
    bool     		db_open = FALSE;
    bool		other_valid_ckps = FALSE;
    bool     		config_open = FALSE;
    i4    		open_flags;
    i4			oldest_ckpseq;
    i4			newest_ckpseq;
    bool		found_one = FALSE;
    i4			this_seq = 0;
    i4			jnl_count;
    i4			dmp_count;
    bool		found_seq;
    bool		found_valid = FALSE;
    i4			ckp_status;
    bool		all_ok = TRUE;
    DB_ERROR		local_dberr;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    /* only_at_start is set when we are calling this from the end of
    ** delete_oldest_ckp, in which case the cnf is already set up 
    */
    if (only_at_start)
    {
	cnf = *config;
    } else
    {
        *config = 0;

        /*
        ** Open the database to:
        ** 1) Ensure that no one else has it opened exclusively
        ** 2) Allow jsp_file_maint() to work.
        */
        open_flags = DM2D_IGNORE_CACHE_PROTOCOL;
        if ((jsx->jsx_status & JSX_WAITDB) == 0)
            open_flags |=  DM2D_NOWAIT;
        status = dm2d_open_db(dcb, DM2D_A_READ, DM2D_IS,
	    dmf_svcb->svcb_lock_list, open_flags, &jsx->jsx_dberr);

        /*
        ** Perform any security audits.
        ** We do this before checking the result of the OPEN since
        ** want to audit failed open attempts as well as successfull ones.
        */
        if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
        {
	    SXF_ACCESS          access;
	    /*
	    **	Record we are checkpointing this database,
	    **	checkpoint can be regarded as selecting from database
	    */
	    access = SXF_A_SELECT;
	    if (status)
	        access |= SXF_A_FAIL;
	    else
	        access |= SXF_A_SUCCESS;

	    local_status = dma_write_audit( SXF_E_DATABASE,
	        access,
	        dcb->dcb_name.db_db_name, 	/* Object name (database) */
	        sizeof(dcb->dcb_name.db_db_name),
	        &dcb->dcb_db_owner, 	/* Object owner */
	        I_SX270B_CKPDB,     	/*  Message */
	        TRUE,		    	/* Force */
	        &local_dberr,
	        NULL);
        }

        /*
        ** Check for failure. Check explicitly for Database-in-use error.
        */
        if (status != E_DB_OK)
        {
            if (jsx->jsx_dberr.err_code == E_DM004C_LOCK_RESOURCE_BUSY)
            {
		SETDBERR(&jsx->jsx_dberr, 0, E_DM110D_CPP_DB_BUSY);
	    } 
	    else
	    {
	        uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	            (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1108_CPP_OPEN_DB);
	    }
	    all_ok = FALSE;
        }

        if (all_ok)
        {
	    db_open = TRUE;

	    status = dm0c_open(dcb, 0, dmf_svcb->svcb_lock_list, config, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	        all_ok = FALSE;
        }

        if (all_ok)
        {
	    cnf = *config;
	    config_open = TRUE;

            if (cnf->cnf_jnl->jnl_count == 0)
            {
                TRformat(dmf_put_line, 0, local_buffer, sizeof(local_buffer),
                    "%@ ALTERDB: No checkpoints available to delete.\n");
                status = E_DB_INFO;
                all_ok = FALSE;
            }
        }
    }

    if (all_ok)
    {
	/* Find the valid and invalid checkpoints */

	dump_history_found = FALSE;
	jnl_count = cnf->cnf_jnl->jnl_count;
	dmp_count = cnf->cnf_dmp->dmp_count;

	/* First find the oldest checkpoint sequence number */
	oldest_ckpseq = cnf->cnf_jnl->jnl_history[0].ckp_sequence;

	if (cnf->cnf_dmp->dmp_count > 0 &&
	    cnf->cnf_dmp->dmp_history[0].ckp_sequence < oldest_ckpseq)
	{
	    oldest_ckpseq = cnf->cnf_dmp->dmp_history[0].ckp_sequence;
	}

	newest_ckpseq = cnf->cnf_jnl->jnl_history[jnl_count-1].ckp_sequence;

	/* Should be looking as far as highest journal/dump, not lowest */
	if (cnf->cnf_dmp->dmp_count > 0 &&
	    cnf->cnf_dmp->dmp_history[dmp_count-1].ckp_sequence > newest_ckpseq)
	{
	    newest_ckpseq = cnf->cnf_dmp->dmp_history[dmp_count-1].ckp_sequence;
	}

	/* Now work through the checkpoints based on sequence number */
	for (this_seq = oldest_ckpseq;
	     this_seq < newest_ckpseq && all_ok ;
	     this_seq++ )
	{
	    found_seq = FALSE;
	    /* Now find whether we know about this checkpoint */
	    for (i = 0; i < jnl_count-1; i++)
	    {
		if (cnf->cnf_jnl->jnl_history[i].ckp_sequence == this_seq)
		{
		    found_seq = TRUE;
		    ckp_status = cnf->cnf_jnl->jnl_history[i].ckp_jnl_valid;
		}
	    }
	    if (!found_seq)
	    {
		/* Allow to look all through dmp_count values, since an invalid
		** dump file might be in the list on its own - there isn't the
		** same requirement to keep at least one valid dump file as 
		** there is for journal files
		*/
		for (i = 0; i < dmp_count; i++)
		{
		    if (cnf->cnf_dmp->dmp_history[i].ckp_sequence == this_seq)
		    {
			found_seq = TRUE;
			/* A dump file on its own is invalid now, even if it
			** was created valid, since its journal has gone
			** (kibro01) b119961
			*/
			ckp_status = 0;
		    }
		}
	    }
	    if (found_seq)
	    {
		if (ckp_status == 1)
		{
		    if (jsx->jsx_status & JSX_VERBOSE && !only_at_start)
		    {
		        TRformat(dmf_put_line, 0, local_buffer,
				sizeof(local_buffer),
			    "%@ ALTERDB: Found VALID checkpoint seq no: %d\n",
			    cnf->cnf_jnl->jnl_history[i].ckp_sequence);
		    }
		    found_valid = TRUE;
		} else
		{
		    /* Pass in our ckp sequence number, and whether or not
		    ** we have found a valid checkpoint so far (in which case
		    ** we are not deleting completely any more)
	 	    */
		    found_one = TRUE;
		    status = jsp_file_maint(jsx, dcb, &cnf,this_seq,
			(found_valid==TRUE)?FALSE:TRUE);
		    if (status != E_DB_OK)
		    {
			all_ok = FALSE;     
		    }
		}
	    }
	    if (found_valid && only_at_start)
		return (E_DB_OK);
	}
    }
	
    if (all_ok)
    {
	  /*
	  ** If the database is journaled, and if journals are valid,
	  ** update the last valid checkpoint from which a rolldb +j
	  ** may occur.
	  */ 
	  if ( ((cnf->cnf_dsc->dsc_status & DSC_JOURNAL) &&
	     (cnf->cnf_jnl->jnl_first_jnl_seq != 0)) &&
	     (cnf->cnf_jnl->jnl_first_jnl_seq <
	      cnf->cnf_jnl->jnl_history[0].ckp_sequence) )
	  {
	    cnf->cnf_jnl->jnl_first_jnl_seq =
		cnf->cnf_jnl->jnl_history[0].ckp_sequence;
	  } 
	  else
	  {
	    if (!found_one && !only_at_start)
	    {
	      TRformat(dmf_put_line, 0, local_buffer, sizeof(local_buffer),
		"%@ ALTERDB: No checkpoints available to delete.\n");
	      status = E_DB_INFO;
	    }
	  }

	  if (!only_at_start)
	  {
	      status = dm2d_close_db(dcb, dmf_svcb->svcb_lock_list,
	          DM2D_IGNORE_CACHE_PROTOCOL, &jsx->jsx_dberr);
	      if (status != E_DB_OK)
	      {
	          uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL,
		      (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
		  SETDBERR(&jsx->jsx_dberr, 0, E_DM110A_CPP_CLOSE_DB);
	      }
	  }
    }
  
    /*
    ** Preserve possible E_DB_INFO status.
    */
    if (DB_FAILURE_MACRO(status))
    {
	if (db_open)
	{
	    (VOID)dm2d_close_db(dcb, dmf_svcb->svcb_lock_list,
		DM2D_IGNORE_CACHE_PROTOCOL, &local_dberr);
	}

	if (config_open)
	{
	    (VOID)dm0c_close(*config, 0, &local_dberr);
	    *config = 0;
	}

	return(E_DB_ERROR);
    }

    return(status);
}

/*
** Name: convert_unicode -- Converts a non-unicode database to unicode. 
**
** Description:
**     The routine handles conversion of non-Unicode database
**     into Unicode.This involves modification of the config
**     file and setting the DU_UTYPES_ALLOWED bit in the tuple
**     for the database in the iidatabase catalog in iidbdb.
**
** Inputs:
**      jsx                        Journal context.
**      dcb                        Database context.
**      cnf                        Config file control block.
** Output:
**      err_code                   One of the following error codes
**                                      E_DM110D_CPP_DB_BUSY
**                                      E_DM1406_ALT_INV_UCOLLATION 
**                                      E_DM1407_ALT_CON_UNICODE
**                                      E_DM1408_ALT_ALREADY_UNICODE
**                                 and error codes returned by the
**                                 called facilities
** Returns
**      E_DB_OK                    Function completed normally.
**      E_DB_WARN                  Function completed normally with
**                                 a termination status in err_code
**      E_DB_ERROR                 Function completed abnormally with
**                                 a termination status which is in
**                                 err_code.
**      E_DB_FATAL                 Function completed with a fatal
**                                 error which must be handled
**                                 immediately.  The fatal status is in
**                                 err_code.
** History:
**          13-Apr-2004 - (nansa02) Created
**	23-feb-05 (inkdo01)
**	    Add error when db is already Unicode-enabled.
**	2-Nov-2006 (kschendel)
**	    Compiler caught improper success test (== instead of &).
**	18-jun-2008 (gupsh01)
**	    Added E_DM1410_ALT_UCOL_NOTEXIST for situation when invalid
**	    collation name is given.
**      23-Sep-2008 (hanal04) Bug 120927
**          Use local_error not just local_status when closing iidbdb. This
**          prevents earlier err_code values from being lost.
*/
static DB_STATUS
convert_unicode(
    DMF_JSX     *jsx,
    DMP_DCB     *dcb,
    DM0C_CNF    *cnf)
{
    DB_STATUS    status;
    DB_STATUS    local_status;
    i4      local_error;
    ADULTABLE        *tbl;
    ADUUCETAB        *utbl;
    char             *uvtab; 
    i4      dbdb_flag = (i4)0;
    i4      dmxe_flag = (i4)0;
    i4      db_flag   = (i4)0;
    DMP_DCB      *iidcb = (DMP_DCB *)NULL;
    DU_DATABASE  database;
    DB_TAB_ID    iidatabase;
    DMP_RCB      *rcb = (DMP_RCB *)NULL;
    DB_TAB_TIMESTAMP stamp;
    DM_TID           tid;
    DM2R_KEY_DESC    key_list[1];
    CL_ERR_DESC      syserr; 
    i4               lock_id;
    i4               log_id;
    DB_TRAN_ID       tran_id;
    char             error_buffer[ER_MAX_LEN]; 
    char            line_buffer[132];
    bool         db_locked     = FALSE;
    bool         iidbdb_opened = FALSE;
    bool         tran_started  = FALSE;
    bool     iidatabase_opened = FALSE;
    i4       error_length;
    DB_ERROR		local_dberr;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);



    if (jsx->jsx_ucollation)
    {
       /* Check if the ucollation exists */
       if (*jsx->jsx_ucollation && aduucolinit(jsx->jsx_ucollation,
            MEreqmem, &utbl, &uvtab, &syserr) != OK)
       {
          STncpy( line_buffer, jsx->jsx_ucollation, DB_MAXNAME);
          line_buffer[ DB_MAXNAME ] = '\0';
          STtrmwhite(line_buffer);

          uleFormat(NULL, E_DM1410_ALT_UCOL_NOTEXIST, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
            (DB_SQLSTATE *)NULL, error_buffer, ER_MAX_LEN, &error_length,
            &local_error, 2, 0, line_buffer);

          error_buffer[error_length] = 0;
          SIprintf("%s\n", error_buffer);

          uleFormat(NULL, E_DM1410_ALT_UCOL_NOTEXIST, (CL_ERR_DESC *)NULL, ULE_LOG,
            (DB_SQLSTATE *)0, (char *)NULL, (i4)0, (i4 *)NULL,
            &local_error, 2, 0, line_buffer);

           return (E_DB_ERROR);
       }
    }
   do
    {
    /*
    ** Open database database iidbdb
    */
      status = open_iidbdb(jsx, &iidcb, DM2D_A_WRITE, DM2D_IX, dbdb_flag);
      if (DB_FAILURE_MACRO(status))
          break;
      iidbdb_opened = TRUE;

    /*
    ** Lock the user database
    */
      db_flag = DM2D_JLK;
      if ((jsx->jsx_status & JSX_WAITDB) == 0)
          db_flag |=  DM2D_NOWAIT;
      status = dm2d_open_db(dcb, DM2D_A_WRITE, DM2D_X,
          dmf_svcb->svcb_lock_list, db_flag, &jsx->jsx_dberr);
      if (DB_FAILURE_MACRO(status))
      {
          if (jsx->jsx_dberr.err_code == E_DM004C_LOCK_RESOURCE_BUSY)
	     SETDBERR(&jsx->jsx_dberr, 0, E_DM110D_CPP_DB_BUSY);
          break;
      }
      db_locked = TRUE;

    /*
    ** Start the transaction
    */
      dmxe_flag = (i4)DMXE_JOURNAL;
      status = dmxe_begin(DMXE_WRITE, dmxe_flag, iidcb,
          iidcb->dcb_log_id, &iidcb->dcb_db_owner,
          dmf_svcb->svcb_lock_list, &log_id, &tran_id, &lock_id,
	  (DB_DIS_TRAN_ID *)NULL,
          &jsx->jsx_dberr);
      if (DB_FAILURE_MACRO(status))
          break;
      tran_started = TRUE;

    /*
    ** Open iidatabase table
    */
      iidatabase.db_tab_base = DM_B_DATABASE_TAB_ID;
      iidatabase.db_tab_index = 0;

      status = dm2t_open(iidcb, &iidatabase, DM2T_IX, DM2T_UDIRECT,
          DM2T_A_WRITE, 0, 20, 0, log_id, lock_id, 0, 0, DM2T_IX, &tran_id,
          &stamp, &rcb, (DML_SCB *)0, &jsx->jsx_dberr);
      if (DB_FAILURE_MACRO(status))
          break;
      iidatabase_opened = TRUE;

    /*
    ** Locate the record for the user database
    */
      key_list[0].attr_number = DM_1_DATABASE_KEY;
      key_list[0].attr_operator = DM2R_EQ;
      key_list[0].attr_value = (char *)&dcb->dcb_name;

      status = dm2r_position(rcb, DM2R_QUAL, key_list, 1, &tid, &jsx->jsx_dberr);
      if (DB_FAILURE_MACRO(status))
          break;

    /*
    ** Get the record
    */
      status = dm2r_get(rcb, &tid, DM2R_GETNEXT, (PTR) &database, &jsx->jsx_dberr);
      if (DB_FAILURE_MACRO(status))
          break;

    /*Update the database service and the config file with the unicode information */
    if   (jsx->jsx_status2 & (JSX2_ALTDB_UNICODEC | JSX2_ALTDB_UNICODED))
      {
	if(cnf->cnf_dsc->dsc_dbservice & DU_UTYPES_ALLOWED)  
	{
	  /* Can't do Unicode C/D conversion on db that's already Unicode. */
          char            error_buffer[ER_MAX_LEN];
          i4              error_length;

          STncpy( line_buffer, dcb->dcb_name.db_db_name, DB_MAXNAME);
          line_buffer[ DB_MAXNAME ] = '\0';
          STtrmwhite(line_buffer);

          uleFormat(NULL, E_DM1408_ALT_ALREADY_UNICODE, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
            (DB_SQLSTATE *)NULL, error_buffer, ER_MAX_LEN, &error_length,
            &local_error, 2, 0, line_buffer);

          error_buffer[error_length] = 0;
          SIprintf("%s\n", error_buffer);

          uleFormat(NULL, E_DM1408_ALT_ALREADY_UNICODE, (CL_ERR_DESC *)NULL, ULE_LOG,
            (DB_SQLSTATE *)0, (char *)NULL, (i4)0, (i4 *)NULL,
            &local_error, 2, 0, line_buffer);
	  status = E_DB_ERROR;
	  break;
	}
	else
	{
          database.du_dbservice           |= DU_UTYPES_ALLOWED;
          cnf->cnf_dsc->dsc_dbservice     |= DU_UTYPES_ALLOWED;
	  if (jsx->jsx_status2 & JSX2_ALTDB_UNICODEC)
	  {
	    database.du_dbservice |= DU_UNICODE_NFC;
	    cnf->cnf_dsc->dsc_dbservice |= DU_UNICODE_NFC;
	  }
          STcopy(jsx->jsx_ucollation,cnf->cnf_dsc->dsc_ucollation);
	}
      }
    
    /*
    ** Update the record
    */
      status = dm2r_replace(rcb, 0, DM2R_BYPOSITION, (PTR) &database,
			(char *)NULL, &jsx->jsx_dberr);
      if (DB_FAILURE_MACRO(status))
          break;

    } while (FALSE);

    if (iidatabase_opened)
    {
      local_status = dm2t_close(rcb, 0, &local_dberr);
      if (DB_FAILURE_MACRO(local_status) && status == E_DB_OK)
      {
	  jsx->jsx_dberr = local_dberr;
          status = local_status;
      }
    }

    if (tran_started)
    {
      if (DB_FAILURE_MACRO(status))
      {
      /* rollback the changes */
          local_status = dmxe_abort((DML_ODCB *)NULL, iidcb, &tran_id,
              log_id, lock_id, dmxe_flag, 0, 0, &local_dberr);
      }
      else
      {
      /* commit the changes */
          local_status = dmxe_commit(&tran_id, log_id, lock_id,
              dmxe_flag, &stamp, &jsx->jsx_dberr);
          if (DB_FAILURE_MACRO(local_status))
              status = E_DB_ERROR;
      }
    }

    if (iidbdb_opened)
    {
      local_status = dm2d_close_db(iidcb, dmf_svcb->svcb_lock_list,
          dbdb_flag, &local_dberr);
      if (DB_FAILURE_MACRO(local_status) && status == E_DB_OK)
      {
	  jsx->jsx_dberr = local_dberr;
          status = E_DB_FATAL;
      }
    }

    if (iidcb)
      dm0m_deallocate((DM_OBJECT **)&iidcb);

    if (db_locked)
    {
      local_status = dm2d_close_db(dcb, dmf_svcb->svcb_lock_list,
          db_flag, &local_dberr);
      if (DB_FAILURE_MACRO(local_status) && status == E_DB_OK)
      {
          status = local_status;
	  jsx->jsx_dberr = local_dberr;
      }
    }
    if(status == E_DB_OK && cnf->cnf_dsc->dsc_dbservice & DU_UTYPES_ALLOWED)  
    {
        char            error_buffer[ER_MAX_LEN];
        i4              error_length;

        STncpy( line_buffer, dcb->dcb_name.db_db_name, DB_MAXNAME);
        line_buffer[ DB_MAXNAME ] = '\0';
        STtrmwhite(line_buffer);

        uleFormat(NULL, E_DM1407_ALT_CON_UNICODE, (CL_ERR_DESC *)NULL, ULE_LOOKUP,
            (DB_SQLSTATE *)NULL, error_buffer, ER_MAX_LEN, &error_length,
            &local_error, 2, 0, line_buffer);

        error_buffer[error_length] = 0;
        SIprintf("%s\n", error_buffer);

        uleFormat(NULL, E_DM1407_ALT_CON_UNICODE, (CL_ERR_DESC *)NULL, ULE_LOG,
            (DB_SQLSTATE *)0, (char *)NULL, (i4)0, (i4 *)NULL,
            &local_error, 2, 0, line_buffer);
    }
       
    return status;
}


/*
** Name: set_pmode -- sets production mode bits
**
** Description:
**     The routine handles setting the production mode in a
**     database. This involves modification of the config
**     file and setting the DU_PRODUCTION bit in the tuple
**     for the database in the iidatabase catalog in iidbdb.
**
** Inputs:
**      jsx                        Journal context.
**      dcb                        Database context.
**      cnf                        Config file control block.
** Output:
**      err_code                   One of the following error codes
**                                      E_DM1519_PMODE_IIDBDB_ERR
**                                      E_DM1520_PMODE_DB_BUSY
**                                 and error codes returned by the
**                                 called facilities
** Returns
**      E_DB_OK                    Function completed normally.
**      E_DB_WARN                  Function completed normally with
**                                 a termination status in err_code
**      E_DB_ERROR                 Function completed abnormally with
**                                 a termination status which is in
**                                 err_code.
**      E_DB_FATAL                 Function completed with a fatal
**                                 error which must be handled
**                                 immediately.  The fatal status is in
**                                 err_code.
** History:
**      01-sep-1995 (tchdi01)
**          Created for the production mode project
**	01-oct-1998 (nanpr01)
**	    Added the new param in dm2r_replace for enhancement of update
**	    performance.
**	28-Jul-2005 (schka24)
**	    Back out x-integration for bug 111502 (not shown), doesn't
**	    apply to r3.
*/

static DB_STATUS
set_pmode(
    DMF_JSX     *jsx,
    DMP_DCB     *dcb,
    DM0C_CNF    *cnf)
{
    DB_STATUS    status;
    DB_STATUS    local_status;
    i4      local_error;

    i4      dbdb_flag = (i4)0;
    i4      dmxe_flag = (i4)0;
    i4      db_flag   = (i4)0;
    DMP_DCB      *iidcb = (DMP_DCB *)NULL;
    DU_DATABASE  database;
    DB_TAB_ID    iidatabase;
    DMP_RCB      *rcb = (DMP_RCB *)NULL;
    DB_TAB_TIMESTAMP stamp;
    DM_TID       tid;
    DM2R_KEY_DESC   key_list[1];

    i4      lock_id;
    i4      log_id;
    DB_TRAN_ID   tran_id;
    DB_ERROR	 local_dberr;


    bool         db_locked     = FALSE;
    bool         iidbdb_opened = FALSE;
    bool         tran_started  = FALSE;
    bool     iidatabase_opened = FALSE;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    /*
    ** The mode of iidbdb cannot be changed
    */
    if (MEcmp((PTR)&dcb->dcb_name, (PTR)DB_DBDB_NAME,
       sizeof(DB_DB_NAME))==0)
    {
      SETDBERR(&jsx->jsx_dberr, 0, E_DM1519_PMODE_IIDBDB_ERR);
      return (E_DB_ERROR);
    }

    do
    {
    /*
    ** Open database database iidbdb
    */
      status = open_iidbdb(jsx, &iidcb, DM2D_A_WRITE, DM2D_IX, dbdb_flag);
      if (DB_FAILURE_MACRO(status))
          break;
      iidbdb_opened = TRUE;

    /*
    ** Lock the user database
    */
      db_flag = DM2D_JLK;
      if ((jsx->jsx_status & JSX_WAITDB) == 0)
          db_flag |=  DM2D_NOWAIT;
      status = dm2d_open_db(dcb, DM2D_A_WRITE, DM2D_X,
          dmf_svcb->svcb_lock_list, db_flag, &jsx->jsx_dberr);
      if (DB_FAILURE_MACRO(status))
      {
          if (jsx->jsx_dberr.err_code == E_DM004C_LOCK_RESOURCE_BUSY)
	      SETDBERR(&jsx->jsx_dberr, 0, E_DM1520_PMODE_DB_BUSY);
          break;
      }
      db_locked = TRUE;

    /*
    ** Start the transaction
    */
      dmxe_flag = (i4)DMXE_JOURNAL;
      status = dmxe_begin(DMXE_WRITE, dmxe_flag, iidcb,
          iidcb->dcb_log_id, &iidcb->dcb_db_owner,
          dmf_svcb->svcb_lock_list, &log_id, &tran_id, &lock_id,
	  (DB_DIS_TRAN_ID *)NULL,
          &jsx->jsx_dberr);
      if (DB_FAILURE_MACRO(status))
          break;

      tran_started = TRUE;

    /*
    ** Open iidatabase table
    */
      iidatabase.db_tab_base = DM_B_DATABASE_TAB_ID;
      iidatabase.db_tab_index = 0;

      status = dm2t_open(iidcb, &iidatabase, DM2T_IX, DM2T_UDIRECT,
          DM2T_A_WRITE, 0, 20, 0, log_id, lock_id, 0, 0, DM2T_IX, &tran_id,
          &stamp, &rcb, (DML_SCB *)0, &jsx->jsx_dberr);
      if (DB_FAILURE_MACRO(status))
          break;
      iidatabase_opened = TRUE;

    /*
    ** Locate the record for the user database
    */
      key_list[0].attr_number = DM_1_DATABASE_KEY;
      key_list[0].attr_operator = DM2R_EQ;
      key_list[0].attr_value = (char *)&dcb->dcb_name;

      status = dm2r_position(rcb, DM2R_QUAL, key_list, 1, &tid, &jsx->jsx_dberr);
      if (DB_FAILURE_MACRO(status))
          break;

    /*
    ** Get the record
    */
      status = dm2r_get(rcb, &tid, DM2R_GETNEXT, (PTR) &database, &jsx->jsx_dberr);
      if (DB_FAILURE_MACRO(status))
          break;

    if (jsx->jsx_status2 & JSX2_PMODE)
    {
      /*
      ** Set the DU_PRODUCTION bit
      */
      if (jsx->jsx_status & JSX_JON)
      {
          database.du_access         |= DU_PRODUCTION;
          cnf->cnf_dsc->dsc_dbaccess |= DU_PRODUCTION;
      }
      else
      {
          database.du_access         &= ~DU_PRODUCTION;
          cnf->cnf_dsc->dsc_dbaccess &= ~DU_PRODUCTION;
      }
    } 
    else if (jsx->jsx_status2 & JSX2_OCKP)
    {
      /*
      ** Set the DU_NOBACKUP bit
      */
      if (jsx->jsx_status & JSX_JOFF)
      {
          database.du_access         |= DU_NOBACKUP;
          cnf->cnf_dsc->dsc_dbaccess |= DU_NOBACKUP;
      }
      else
      {
          database.du_access         &= ~DU_NOBACKUP;
          cnf->cnf_dsc->dsc_dbaccess &= ~DU_NOBACKUP;
      }
    }

    /*
    ** Update the record
    */
      status = dm2r_replace(rcb, 0, DM2R_BYPOSITION, (PTR) &database, 
			(char *)NULL, &jsx->jsx_dberr);
      if (DB_FAILURE_MACRO(status))
          break;

    } while (FALSE);

    if (iidatabase_opened)
    {
      local_status = dm2t_close(rcb, 0, &local_dberr);
      if (DB_FAILURE_MACRO(local_status) && status == E_DB_OK)
      {
	  jsx->jsx_dberr = local_dberr;
          status = local_status;
      }
    }

    if (tran_started)
    {
      if (DB_FAILURE_MACRO(status))
      {
      /* rollback the changes */
          local_status = dmxe_abort((DML_ODCB *)NULL, iidcb, &tran_id,
              log_id, lock_id, dmxe_flag, 0, 0, &local_dberr);
      }
      else
      {
      /* commit the changes */
          local_status = dmxe_commit(&tran_id, log_id, lock_id,
              dmxe_flag, &stamp, &jsx->jsx_dberr);
          if (DB_FAILURE_MACRO(local_status))
              status = E_DB_ERROR;
      }
    }

    if (iidbdb_opened)
    {
      local_status = dm2d_close_db(iidcb, dmf_svcb->svcb_lock_list,
          dbdb_flag, &local_dberr);
      if (DB_FAILURE_MACRO(local_status) && status == E_DB_OK)
      {
	  jsx->jsx_dberr = local_dberr;
          status = E_DB_FATAL;
      }
    }

    if (iidcb)
      dm0m_deallocate((DM_OBJECT **)&iidcb);

    if (db_locked)
    {
      local_status = dm2d_close_db(dcb, dmf_svcb->svcb_lock_list,
          db_flag, &local_dberr);
      if (DB_FAILURE_MACRO(local_status) && status == E_DB_OK)
      {
          status = local_status;
	  jsx->jsx_dberr = local_dberr;
      }
    }

    return status;
}


/*
** Name: set_accmode -- sets access mode in the .cnf file
**
** Description:
**     The routine handles setting the access mode (readonly or read/write) in a
**     database. It modifies the config file and writes it back.
**
** Inputs:
**      jsx                        Journal context.
**      cnf                        Config file control block.
** 
** Output:
**      err_code                   One of the following error codes
**                                 and error codes returned by the
**                                 called facilities
** Returns
**      E_DB_OK                    Function completed normally.
** 
** History:
** 	14-apr-1997 (wonst02)
**          Created for the readonly database project.
**	12-may-1997 (wonst02)
**	    Disable journaling on a readonly database.
**
*/
static DB_STATUS
set_accmode(
    DMF_JSX     *jsx,
    DMP_DCB	*dcb,
    DM0C_CNF    *cnf)
{
    DB_STATUS    status = E_DB_OK;

    CLRDBERR(&jsx->jsx_dberr);

    if (jsx->jsx_status2 & JSX2_READ_ONLY)
    {
      /*
      ** Set the READONLY access mode
      */
      cnf->cnf_dsc->dsc_access_mode = DSC_READ;
      /*
      ** Make call to disable journaling on the database.
      ** This call may return a warning if journaling is already
      ** disabled on the database; if so, ignore it.
      */
      status = disable_jnl(jsx, dcb, cnf);
      if (status == E_DB_WARN)
      {
        status = E_DB_OK;
	CLRDBERR(&jsx->jsx_dberr);
      }
    } 
    else if (jsx->jsx_status2 & JSX2_READ_WRITE)
    {
      /*
      ** Set the READ/WRITE access mode
      */
      cnf->cnf_dsc->dsc_access_mode = DSC_WRITE;
    }

    return status;
}


/*
** Name: switch_journal -- Close the current journal and start a new one.
**
** Description:
**     The routine notifies DMFACP to perform a journal switch, then 
**     waits until the action is completed.
**
** Inputs:
**      jsx                        Journal context.
**      cnf                        Config file control block.
** 
** Output:
**      err_code                   One of the following error codes
**                                 and error codes returned by the
**                                 called facilities
** Returns
**      E_DB_OK                    Function completed normally.
** 
** History:
** 	31-Jul-1997 (shero03)
**          Created for the Journal switch database project.
**	01-Dec-2006 (kiria01) b117225
**	    Initialise the lockid parameter to LKrequest avoid random implicit
**	    lock conversions.
**      28-Oct-2008 (hanal04) Bug 121140
**          If the acp is dead record the failure, clean up and make sure
**          we return a failure.
*/
static DB_STATUS
switch_journal(
    DMF_JSX     *jsx,
    DMP_DCB	*dcb,
    DM0C_CNF    **config)
{
    DM0C_CNF	*cnf = *config;
    DB_STATUS   status;
    STATUS	cl_status;
    i4	cl_error;
    CL_ERR_DESC	sys_err;
    i4	event;
    i4	length;
    i4	event_mask;
    i4	item;
    i4    	open_flags;
    bool       	db_open = FALSE;
    LK_LOCK_KEY	lockkey;
    LK_LKID	lockid;
    SXF_ACCESS  access;
    char	local_buffer[ALTERDB_MSG_LEN];
    i4		deadacp = 0;
    DB_ERROR	local_dberr;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    if ((cnf->cnf_dsc->dsc_status & DSC_JOURNAL) == 0) 
    {
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1402_ALT_DB_NOT_JOURNALED);
	return (E_DB_ERROR);
    }
    MEfill(sizeof(LK_LKID), 0, &lockid);

    TRformat(dmf_put_line, 0, local_buffer, sizeof(local_buffer),
	"%@ ALTERDB: Switching to the Next Journal for Database '%~t'.\n",
	sizeof(dcb->dcb_name), &dcb->dcb_name);

    /*
    ** The config file must be closed prior to signaling the JSWITCH
    ** event.
    */
    status = dm0c_close(cnf, 0, &jsx->jsx_dberr);
    if (status != E_DB_OK)
       return(status);
    *config = 0;
    
    do 
    {

	/*
	** Get DBU lock to prevent database reorganization during the backup.
	*/
	lockkey.lk_type = LK_CKP_DB; 
	lockkey.lk_key1 = dcb->dcb_id;
	MEcopy((PTR)&dcb->dcb_db_owner, 8, (PTR)&lockkey.lk_key2);
	MEcopy((PTR)&dcb->dcb_name, 12, (PTR)&lockkey.lk_key4);

	cl_status = LKrequest(
	    LK_PHYSICAL, 
	    dmf_svcb->svcb_lock_list, 
	    &lockkey, 
	    LK_X, 
	    (LK_VALUE * )0, 
	    &lockid, 0L, 
	    &sys_err); 
	if (cl_status != E_DB_OK)
	{
	    uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1131_CPP_LK_FAIL);
	    break;
	}

	if (jsx->jsx_status & JSX_TRACE)
	    TRdisplay("%@ ALTERDB: Successfully Lock Out All DBU operations\n");
    
        /*
        ** Open the database to: 
        ** 1) Ensure that no one else has it opened exclusively 
        ** 2) Allow jsp_file_maint() to work.
        */
        open_flags = DM2D_IGNORE_CACHE_PROTOCOL;
        if ((jsx->jsx_status & JSX_WAITDB) == 0)
            open_flags |=  DM2D_NOWAIT;
        status = dm2d_open_db(dcb, DM2D_A_READ, DM2D_IS,
            	dmf_svcb->svcb_lock_list, open_flags,
             	&jsx->jsx_dberr);

        /*
        ** Perform any security audits.
        ** We do this before checking the result of the OPEN since
        ** want to audit failed open attempts as well as successfull ones.
        */
	if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
	{
	    /*
	    **	Record we are checkpointing this database,
	    **	checkpoint can be regarded as selecting from database
	    */
	    access = SXF_A_SELECT;
	    if (status)
		access |= SXF_A_FAIL;
	    else
		access |= SXF_A_SUCCESS;

	    cl_status = dma_write_audit( SXF_E_DATABASE,
		    access,
		    dcb->dcb_name.db_db_name, 	/* Object name (database) */
		    sizeof(dcb->dcb_name.db_db_name), 
		    &dcb->dcb_db_owner, 	/* Object owner */
		    I_SX270B_CKPDB,     	/*  Message */
		    TRUE,		    	/* Force */
		    &local_dberr,
		    NULL);
	}

        /*
        ** Check for failure. Check explicitly for Database-in-use error.
        */
        if (status != E_DB_OK)
        {
            if (jsx->jsx_dberr.err_code == E_DM004C_LOCK_RESOURCE_BUSY)
            {
		SETDBERR(&jsx->jsx_dberr, 0, E_DM110D_CPP_DB_BUSY);
        	break;
            }
            uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
        		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1108_CPP_OPEN_DB);
	    break;
        }

	db_open = TRUE;
	/* Start a transaction to use for LGevent waits. */

	cl_status = LGbegin(
	    LG_NOPROTECT, dcb->dcb_log_id, 
	    (DB_TRAN_ID *)&jsx->jsx_tran_id, &jsx->jsx_tx_id, 
	    sizeof(dcb->dcb_db_owner), dcb->dcb_db_owner.db_own_name, 
	    (DB_DIS_TRAN_ID*)NULL, &sys_err);

	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM900C_BAD_LOG_BEGIN, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1151_CPP_CPNEEDED);
	    break;
	}

	/* Signal Journal Switch Point. */

	cl_status = LGalter(LG_A_SJSWITCH,
	    		(PTR)&dcb->dcb_log_id, 
	    		sizeof(dcb->dcb_log_id), 
	    		&sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code,
		1, 0, LG_A_CPNEEDED);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1151_CPP_CPNEEDED);
	    break;
	}

	if (jsx->jsx_status & JSX_TRACE)
	    TRdisplay("%@ ALTERDB: Wait for Event LG_E_ARCHIVE_DONE to Occur\n");

	event_mask = (LG_E_ARCHIVE_DONE | LG_E_START_ARCHIVER);

	cl_status = LGevent(
	    0, jsx->jsx_tx_id, event_mask, &event, &sys_err);

	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM900F_BAD_LOG_EVENT, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code,
		1, 0, LG_E_ARCHIVE_DONE);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1151_CPP_CPNEEDED);
	    break;
	}
	if (event & (LG_E_START_ARCHIVER))
	{
	    uleFormat(NULL, E_DM1144_CPP_DEAD_ARCHIVER, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    deadacp = E_DM1145_CPP_DEAD_ARCHIVER;
            /* Fall through to clean up */
	}

	if (jsx->jsx_status & JSX_TRACE)
	    TRdisplay("%@ ALTERDB: LG_E_ARCHIVE_DONE occurred.\n");

	cl_status = LGend(jsx->jsx_tx_id, 0, &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1151_CPP_CPNEEDED);
	    break;
	}

	status = dm2d_close_db(dcb, dmf_svcb->svcb_lock_list, 
	    DM2D_IGNORE_CACHE_PROTOCOL, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&jsx->jsx_dberr, 0, NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM110A_CPP_CLOSE_DB);
	    break;
	}
	db_open = FALSE;
    } while (FALSE);
    
    /*
    ** Reopen the config file, it was closed during the journal switcth  
    ** so that the archiver could access it if necessary in order
    ** to complete.
    */
    status = dm0c_open(dcb, 0, dmf_svcb->svcb_lock_list, config, &jsx->jsx_dberr);

    /* Release the DBU operation lock. */

    cl_status = LKrelease(0, dmf_svcb->svcb_lock_list, (LK_LKID *)0,
	    &lockkey, (LK_VALUE * )0, &sys_err); 
    if (cl_status != OK)
    {
	uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1131_CPP_LK_FAIL);
	status = E_DB_ERROR;
    }

    /*
    ** Preserve possible E_DB_INFO status. 
    */
    if (DB_FAILURE_MACRO(status))
    {
	if (db_open)
	{
	    (VOID)dm2d_close_db(dcb, dmf_svcb->svcb_lock_list, 
		DM2D_IGNORE_CACHE_PROTOCOL, &local_dberr);
	}

	status = E_DB_ERROR;
    }
    else
    {
        if(deadacp)
        {
	    SETDBERR(&jsx->jsx_dberr, 0, deadacp);
            status = E_DB_ERROR;
        }
        else
        {
	    status = E_DB_OK;
        }
    }	
    return status;
}

