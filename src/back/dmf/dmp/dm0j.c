/*
** Copyright (c) 1986, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>

#include    <cs.h>
#include    <di.h>
#include    <jf.h>
#include    <me.h>
#include    <pc.h>
#include    <tr.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <lg.h>
#include    <lk.h>
#include    <cx.h>

#include    <ulf.h>

#include    <dmf.h>

#include    <dm.h>

#include    <dmp.h>
#include    <dmpp.h>
#include    <dm0c.h>
#include    <dm0j.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0m.h>
#include    <lgclustr.h>
#include    <dm2f.h>

/**
**
**  Name: DM0J.C - Journal routines.
**
**  Description:
**      This file contains the routines that implement the journal file
**	I/O primitives.
**
**          dm0j_close - Close journal file.
**	    dm0j_create	- Create a journal file.
**          dm0j_open - Open journal file.
**          dm0j_read - Read from journal file.
**          dm0j_write - Write to journal file.
**	    dm0j_update - Update journal file.
**	    dm0j_truncate - Truncate journal file.
**          dm0j_merge - Merge VAXcluster journal files.
**	    dm0j_delete - Delete journal file.
**	    dm0j_backread - Read journal file backwards.
**	    dm0j_position - Find a log record matching LSN, used
**			    by MVCC undo.
**
**  History:
**      04-aug-1986 (Derek)
**          Created for Jupiter.
**      09-jun-1987 (Jennifer)
**          Added cluster support.
**	24-feb-1989 (greg)
**	    Integrate annie and Ed's fix getting the number of parameters to 
**	    JFupdate correct This resulted in an AV during ckpdb shutdown 
**	    bringing down the ACP.
**      27-Nov-1989 (ac)
**          Fixed several multiple journal files merging problems.
**      12-Dec-1989 (ac)
**          Don't decrease the sequence number in dm0j_update() if the 
**	    sequence number indicating the first block of the journal file.
**	19-Dec-1989 (walt)
**	    Fixed bug 9063.  Structure elements declared backwards in
**	    log_error().
**	26-dec-1989 (greg)
**	    dm0j_read()
**	        argument RECORD is PTR* not PTR**
**	        if dm0j_next_byte is not aligned copy into dm0j_record
**	20-may-90 (bryanp)
**	    After updating the config file, make a backup copy of it.
**	05-sep-1990 (Sandyh)
**	    Always decrement the sequence number in dm0j_update so that a pass
**	    thru an archive series without any qualifying data after the first
**	    open will not cause the first block to be skipped. (bug 32124)
**	19-nov-1990 (bryanp)
**	    Clear the cnode_la field in the jnl_node_info after performing
**	    merge. This avoids leaving 'stale' log addresses for infodb.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	     3-may-1991 (rogerk)
**		Back out the above change to clear the cnode_la field in the
**		jnl_node_info after a merge.  While the cluster info relating
**		to the sequence/block number of the cluster journal files are
**		no longer needed after a merge, the log file address to which
**		the db has been journaled on this node is still needed.
**	    14-may-1991 (bryanp)
**		Log more complete error information when encounter jnl errors.
**          24-oct-1991 (jnash)
**              Remove incorrect initialization of local variable detected 
**		by LINT.
**	07-july-1992 (jnash)
**	    Add DMF function prototypes, replace err_code DB_ERROR
**	    definitions with i4's.
**	08-jul-1992 (mikem)
**	    Fixed code problems uncovered by prototypes.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Fixed compile warnings.
**	    Added support for backward reading of journal files for rollforward
**		undo processing.
**	    Added dm0j_backread routine for backward journal reads (and support
**		routines get_rec and count_recs.
**	    Added dm0j_direction and dm0j_next_recnum fields in the context
**		control block and a new field jh_first_rec in the block header.
**	16-mar-1993 (rmuth)
**	    Include di.h
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		dm0j_merge now merges by Log Sequence Number, not by LPS.
**              Modify calling sequence to dm0j_merge so that it can be called
**		    by auditdb, thus allowing us to remove the duplicated code
**		    in dmfatp.c.
**		Cast some parameters to quiet compiler complaints following
**		    prototyping of LK, CK, JF, SR.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	24-may-1993 (jnash)
**	    Add new error message after failure in JFcreate.  This arises
**	    from new AUDITDB -init_jnl_file behaviour.
**	21-june-1993 (jnash)
**	    Fix BOF sequence number check in dm0j_backread().
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Fix error handling bug in dm0j_merge().
**	26-jul-1993 (jnash)
**	    When reading journal file backwards, properly return last part of 
**	    journal record when the last bit resides on a different page.
**	23-aug-1993 (bryanp)
**	    Correct (char *)/PTR mismatch in dm0j_merge.
**      10-sep-93 (smc)
**          Commented lint pointer truncation warning.
**	20-sep-1993 (bryanp)
**	    Fix cluster support bugs in dm0j_merge.
**	26-oct-1994 (harpa06)
**          Fixed the lint problem "next_found == TRUE" (bug #65264)
**      22-nov-1994 (andyw)
**          Partial Backup/Recovery Project:
**              Created dm0j_filename so that the rollforwarddb can determine
**              the journal filename to pass to OpenCheckpoint
**	 2-apr-96 (nick)
**	    Ensure we don't 'lose' a journal record in dm0j_write(). #75790
**      06-mar-1996 (stial01)
**          Variable Page Size Project: Length in log records is now 4 bytes.
**              dm0j_backread() Use dm0m_lcopy to manipulate the dm0l_record  
**              field, since it may exceed 64K in length.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-may-2001 (devjo01)
**	    Add lgclustr.h for LGcluster() macro. s103715.
**	23-mar-2002 (devjo01)
**	    Rework this to correct merge sorting error, and to 
**	    improve amazingly inefficient sort.  Also replace
**	    LGcluster with CXcluster_enabled, and take advantage
**	    of dm0j_filename to create filenames.
**      11-jul-2002 (stial01)
**          Added consistency check for DM0j_MRZ = LG_MAX_RSZ
**      08-Oct-2002 (hanch04)
**          Removed dm0m_lcopy,dm0m_lfill.  Use MEcopy, MEfill instead.
**      20-oct-2004 (stial01)
**          store_files() add journal files to the file array in sequence order
**      03-dec-2004 (stial01)
**          dm0j_merge() After merge, Bump jnl_fil_seq, TRdisplay journals
**      01-apr-2005 (stial01)
**          dm0j_merge() removed 03-dec-2004 changed. Fixed more TRdisplays
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**      16-jul-2008 (stial01)
**          Added DM0J_RFP_INCR for incremental rollforwarddb
**	14-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Deleted DM0J_MRZ, use LG_MAX_RSZ instead.
**/

/*
**  Definition of static functions.
*/
static DB_STATUS count_files(
    i4	*count,
    char	*filename,
    i4	l_filename,
    CL_ERR_DESC	*sys_err);

static DB_STATUS get_next_jnl(
    DM0C_CNF	*cnf,
    DM0J_CTX	**cur_jnl,
    DB_ERROR	*dberr);

static DB_STATUS logErrorFcn(
    DM0J_CTX	*jctx,
    DB_STATUS	dmf_error,
    CL_ERR_DESC	*sys_err,
    DB_ERROR	*dberr,
    PTR		FileName,
    i4		LineNumber);
#define log_error(jctx,dmf_error,sys_err,dberr)\
	logErrorFcn(jctx,dmf_error,sys_err,dberr,__FILE__,__LINE__)

static DB_STATUS store_files(
    DM0J_CS	*cs,
    char	*filename,
    i4	l_filename,
    CL_ERR_DESC	*sys_err);

static i4	count_recs(
    char	   *page);

static char *	get_recptr(
    char	   *page,
    i4	   recnum);


/*{
** Name: dm0j_close	- Close a journal file.
**
** Description:
**      This routine will close a journal context that was previously
**	opened.
**
** Inputs:
**      jctx                            The journal context.
**
** Outputs:
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
**      04-aug-1986 (Derek)
**          Created for Jupiter.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    14-may-1991 (bryanp)
**		Log CL error status if JFclose fails.
**	08-jul-1992 (mikem)
**	    Reorganized logic from a for (;;) loop to an if/else to get around
**	    sun4/acc compiler error "end-of-loop code not reached".
**	15-jan-1999 (nanpr01)
**	    Pass pointer to pointer in dm0m_deallocate.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Don't JFclose if not open,
**	    unconditionally deallocate jctx.
**	03-Feb-2010 (jonj)
**	    Check dm0j_opened, not dm0j_jnl_seq, which can sometimes
**	    be zero even when file has been opened.
[@history_template@]...
*/
DB_STATUS
dm0j_close(
DM0J_CTX            **jctx,
DB_ERROR	    *dberr)
{
    DM0J_CTX		*j = *jctx;
    DB_STATUS		status = OK;
    CL_ERR_DESC		sys_err;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*  Close the journal file, if open */
    if ( j->dm0j_opened )
	status = JFclose(&j->dm0j_jfio, &sys_err);

    if ( status )
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char * )0, 0L, (i4 *)NULL, 
			err_code, 0);

	status = log_error(j, E_DM902A_BAD_JNL_CLOSE, &sys_err, dberr);
    }

    /*  Deallocate the DM0J control block. */
    dm0m_deallocate((DM_OBJECT **)jctx);
    return(status);
}

/*{
** Name: dm0j_create	- Create a journal file.
**
** Description:
**      This routine creates journal file.
**
** Inputs:
**      flag;                           Zero or DM0J_MERGE or DM0J_RFP_INCR
**                                      If you do not want the cluster name,
**                                      must be DM0J_MERGE or DM0J_RFP_INCR
**	device				Pointer to device name.
**	l_device			Length of device name.
**	jnl_seq				Journal file sequence number.
**	bksz				Block size.
**	blkcnt				Count of blocks to allocate.
**      node_id                         Node id for cluster file else zero.
**
** Outputs:
**      err_code                        Reason for error return status.
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
**      04-aug-1986 (Derek)
**	    Created for Jupiter.
**      03-jun-1987 (jennifer)
**          Updated to handle cluster file names. 
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    14-may-1991 (bryanp)
**		Log CL error status if JFcreate fails.
**	24-may-1993 (jnash)
**	    New error message after failure in JFcreate to include  
**	    allocated amount.
**	23-mar-2002 (devjo01)
**	    Take advantage of dm0j_filename.
[@history_template@]...
*/
DB_STATUS
dm0j_create(
i4             flag,
char		    *device,
i4		    l_device,
i4		    jnl_seq,
i4		    bksz,
i4		    blkcnt,
i4		    node_id,
DB_TAB_ID	    *sf_tab_id,
DB_ERROR	    *dberr)
{
    DB_STATUS		    status;
    DM_FILE		    file;
    JFIO		    jf_io;
    CL_ERR_DESC		    sys_err;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*	Construct the file name. */
    dm0j_filename( flag, jnl_seq, node_id, &file, sf_tab_id );

    /*  Open and position the journal file. */

    status = JFcreate(&jf_io, device, l_device, file.name, sizeof(file), 
	bksz, blkcnt, &sys_err);
    if (status == OK)
	return (E_DB_OK);

    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			err_code, 0);

    uleFormat(NULL, E_DM9037_BAD_JNL_CREATE, &sys_err, 
            ULE_LOG, NULL, 0, 0, 0, err_code, 4,
	    l_device, device, 
	    sizeof(DM_FILE), &file,
	    0, jnl_seq, 0, blkcnt);

    /*	Map from the logging error to the return error. */

    SETDBERR(dberr, 0, E_DM9281_DM0J_CREATE_ERROR);

    return (E_DB_ERROR);
}

/*{
** Name: dm0j_delete 	- Delete a journal file.
**
** Description:
**      This routine creates journal file.
**
** Inputs:
**      flag;                           Zero or DM0J_MERGE or DM0J_RFP_INCR
**                                      If you do not want the cluster name,
**                                      must be DM0J_MERGE or DM0J_RFP_INCR
**	l_device			Length of device name.
**	jnl_seq				File sequence number.
**
** Outputs:
**      err_code                        Reason for error return status.
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
**      04-aug-1986 (Derek)
**	    Created for Jupiter.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    14-may-1991 (bryanp)
**		Log CL error status if CL call fails.
**	    11-oct-2005 (mutma03)
**		Fixed journal file cleanup on cluster nodes during "ckpdb -d"
**		by passing node_id of zero to dm0j_filename when merge flag is
**		set in order to generate the merged file name.
**
[@history_template@]...
*/
DB_STATUS
dm0j_delete(
i4             flag,
char		    *device,
i4		    l_device,
i4		    jnl_seq,
DB_TAB_ID	    *sf_tab_id,
DB_ERROR	    *dberr)
{
    DB_STATUS		    status;
    i4			    node_id=0;
    i4		    i;
    DM_FILE		    file;
    JFIO		    jf_io;
    CL_ERR_DESC		    sys_err;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*	Construct the file name. pass node id of zero when the 
        merge flag is set in order to generate the merge file name.  */

    if (!flag) node_id = CXnode_number(NULL);
    dm0j_filename( flag, jnl_seq, node_id, &file, sf_tab_id );
    
    /*  Open and position the journal file. */

    status = JFdelete(&jf_io, device, l_device, file.name, sizeof(file), 
	&sys_err);
    if (status == OK || status == JF_NOT_FOUND)
	return (E_DB_OK);

    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			err_code, 0);

    log_error(NULL, E_DM9032_BAD_JNL_DELETE, &sys_err, dberr);

    return (E_DB_ERROR);
}

/*{
** Name: dm0j_open	- Open a journal file.
**
** Description:
**      This routine opens a journal file and initializes the journal
**	context area.
**
** Inputs:
**      flag;                           Zero or DM0J_MERGE or DM0J_RFP_INCR
**                                      If you do not want the cluster name,
**                                      must be DM0J_MERGE or DM0J_RFP_INCR
**	l_device			Length of device name.
**	bksz				Block size.
**	jnl_seq				Journal file sequence number.
**	ckp_seq				Checkpoint sequence number.
**	sequence			Journal block sequence.
**					If DM0J_EXTREME and mode is READ then
**					the value is automatically initialized
**					dependent on the read direction:
**					    DM0J_FORWARD - same as specifying 0
**					    DM0J_BACKWARD - set to last pageno
**						in the journal file.
**	mode				Either DM0J_M_READ or DM0J_M_WRITE.
**      nodeid                          Only valid of flag = DM0J_MERGE.
**                                      If value = -1, nodeid not used
**                                      else node_id is used to create name. 
**	direction			Read direction:
**					DM0J_FORWARD - read from start of
**					    journal to the end.
**					DM0J_BACKWARD - read from last block
**					    of the journal back to the 1st.
**
** Outputs:
**	jctx				Pointer to allocated journal context.
**      err_code                        Reason for error return status.
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
**      04-aug-1986 (Derek)
**	    Created for Jupiter.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    14-may-1991 (bryanp)
**		Save node_id in the DM0J_CTX structure for error logging use.
**		Log CL error status if CL call fails.
**      08-jul-1992 (mikem)
**          Reorganized logic from a for (;;) loop to a do/while to get around
**          sun4/acc compiler error "end-of-loop code not reached".
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Added support for backward reading of journal files for rollforward
**		undo.  Added direction parameter and abilitiy to specify
**		DM0J_EXTREME as a sequence value.
**	15-jan-1999 (nanpr01)
**	    Pass pointer to pointer in dm0m_deallocate.
**	23-mar-2002 (devjo01)
**	    Take advantage of dm0j_filename.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: New input flag DM0J_KEEP_CTX. Allocate DM0J_CTX
**	    first time needed, keep it. It's up to the caller to free the
**	    memory.
[@history_template@]...
*/
DB_STATUS
dm0j_open(
i4             flag,
char		    *device,
i4		    l_device,
DB_DB_NAME	    *db_name,
i4		    bksz,
i4		    jnl_seq,
i4		    ckp_seq,
i4		    sequence,
i4		    mode,
i4                  node_id,
i4		    direction,
DB_TAB_ID	    *sf_tab_id,
DM0J_CTX	    **jctx,
DB_ERROR	    *dberr)
{
    DM0J_CTX		    *j = 0;
    DM0J_HEADER		    *jh;
    DB_STATUS		    status;
    i4		    i;
    i4		    jf_sequence;
    DM_FILE		    file;
    CL_ERR_DESC		    sys_err;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*	Construct the file name. */
    dm0j_filename( flag, jnl_seq, node_id, &file, sf_tab_id );

    do
    {
	if ( !(flag & DM0J_KEEP_CTX) || !*jctx )
	{
	    /*  Allocate a DM0J control block from short term memory */

	    status = dm0m_allocate(sizeof(DM0J_CTX) + bksz + LG_MAX_RSZ,
		DM0M_SHORTTERM, DM0J_CB, DM0J_ASCII_ID, 
		(char *)&dmf_svcb, (DM_OBJECT **) jctx, dberr);
	    if (status != E_DB_OK)
		break;
	}

	/*  Initialize the rest of the control block. */

	j = *jctx;
	j->dm0j_record = (char *)j + sizeof(DM0J_CTX);
	j->dm0j_page = (char *)j->dm0j_record + LG_MAX_RSZ;
	j->dm0j_jnl_seq = jnl_seq;
	j->dm0j_ckp_seq = ckp_seq;
	j->dm0j_sequence = sequence;
	j->dm0j_bksz = bksz;
	j->dm0j_next_byte = (char *)j->dm0j_page + sizeof(DM0J_HEADER);
	j->dm0j_bytes_left = bksz - sizeof(DM0J_HEADER);
	j->dm0j_next_recnum = 0;
	j->dm0j_direction = direction;
	j->dm0j_dbname = *db_name;
	if (sf_tab_id)
	    STRUCT_ASSIGN_MACRO(*sf_tab_id, j->dm0j_sf_tabid);
	else
	{
	    j->dm0j_sf_tabid.db_tab_base = 0;
	    j->dm0j_sf_tabid.db_tab_index = 0;
	}

	/*
	** Save node id for future error messages.  If the filename ends
	** with a ".jnl" suffix, then this is not a cluster journal file,
	** so we set the node id to -1.  Cluster journal file's end with
	** a ".j01" type suffix.
	*/
	if (file.name[10] != 'n')
	    j->dm0j_node_id = node_id;
	else
	    j->dm0j_node_id = -1;

	/*  Open and position the journal file. */

	status = JFopen(&j->dm0j_jfio, device, l_device,
	    file.name, sizeof(file), 
	    bksz, &jf_sequence, &sys_err);
	if (status != OK)
	{
	    if ((status == JF_NOT_FOUND) || (status == JF_DIR_NOT_FOUND))
		SETDBERR(dberr, 0, E_DM9034_BAD_JNL_NOT_FOUND);
	    else
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			err_code, 0);
		SETDBERR(dberr, 0, E_DM902C_BAD_JNL_OPEN);
	    }
	    /* Show file not open */
	    j->dm0j_opened = FALSE;
	    break;
	}
	/* Show file open */
	j->dm0j_opened = TRUE;

	/*
	** Initialize context dependent on open mode and direction.
	** If Open for read, set next_byte and bytes_left to zero so that
	** the first dm0j_read request will cause a new block to be read.
	**
	** If the starting sequence number passed in was DM0J_EXTREME, then
	** set the starting page number to the begining or end of the file
	** depending on the read direction.
	**
	** (Actually set to one past where first read will be done since
	** the dm0j_read call will increment/decrement the sequence number
	** for the first read).
	*/
	if (mode == DM0J_M_READ)
	{
	    j->dm0j_next_byte = 0;
	    j->dm0j_bytes_left = 0;

	    if (sequence == DM0J_EXTREME)
	    {
		if (direction == DM0J_BACKWARD)
		    j->dm0j_sequence = jf_sequence + 1;
		else
		    j->dm0j_sequence = 0;
	    }
	}

	/*  Initialize the journal page header. */

	jh = (DM0J_HEADER*) j->dm0j_page;
	jh->jh_type = JH_V1_S1;
	STRUCT_ASSIGN_MACRO(*db_name, jh->jh_dbname);
	jh->jh_seq = sequence;
	jh->jh_jnl_seq = j->dm0j_jnl_seq;
	jh->jh_ckp_seq = j->dm0j_ckp_seq;
	jh->jh_offset = sizeof(*jh);
	jh->jh_first_rec = 0;
	jh->jh_low_lsn.lsn_high = jh->jh_low_lsn.lsn_low = 0;
	jh->jh_high_lsn.lsn_high = jh->jh_high_lsn.lsn_low = 0;

    	if (status == OK)
	    return(E_DB_OK);

    } while (FALSE);

    /*	Error recovery. */

    /* Free DM0J_CTX, clearing caller's pointer */
    if (*jctx)
    {
	dm0m_deallocate((DM_OBJECT **)jctx);
	if (dberr->err_code != E_DM9034_BAD_JNL_NOT_FOUND)
	    log_error(*jctx, dberr->err_code, &sys_err, dberr);
    }

    return (E_DB_ERROR);
}

/*{
** Name: dm0j_position	- Find a log record by LSN in the journals.
**
** Description:
**      Called by dm0p::make_consistent to locate a specific log record
**	in the journals.
**
**	Reads the journal(s) forwards/backwards as needed to locate
**	the matching LSN.
**
** Inputs:
**	tran_id				Transaction id of caller, only
**					used for trace messages.
**      jctx                            The journal context.
**	dcb				The database.
**	CRhdrPtr			Pointer to CRhdr specifying
**					the LSN of the record we seek
**					and it's anticipated journal
**					fileseq and block.
**
** Outputs:
**	LogRecord			Pointer to the found log record.
**	jread				Running tally of journal blocks read.
**	jhit				Running tally of direct hits.
**      dberr                           The reason for an error return status.
**		E_DM0055_NONEXT		if the LSN could not be found.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	The journal context (DM0J_CTX) is allocated here and preserved,
**      but must be deallocated by the caller.
**
** History:
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Created.
**	19-Mar-2010 (jonj)
**	    Must refresh the block when changing read direction or
**	    bytes_left will be wrong.
*/
DB_STATUS
dm0j_position(
DB_TRAN_ID	    *tran_id,
DM0J_CTX            **jctx,
DMP_DCB		    *dcb,
PTR	    	    CRhdrPtr,
PTR	    	    *LogRecord,
i4		    *jread,
i4		    *jhit,
DB_ERROR	    *dberr)
{
    DM0J_CTX		*j = *jctx;
    DB_STATUS		status = E_DB_OK;
    CL_ERR_DESC		sys_err;
    DM0J_HEADER		*jh;
    DM0L_HEADER		*record;
    DM0L_CRHEADER	*CRhdr;
    LG_LSN		TargetLSN;
    LG_JFA		TargetJFA;
    i4			err_code;
    i4			LogRecordLen;
    i4			ReadDirection;
    i4			CurrFilseq;
    i4			CurrBlock;
    i4			BlocksRead = 0;
    i4			length;

    /* Establish our objective */
    CRhdr = (DM0L_CRHEADER*)CRhdrPtr;
    TargetLSN = CRhdr->prev_lsn;
    TargetJFA = CRhdr->prev_jfa;

    /* Start at the anticipated filseq/block, read backwards */
    CurrFilseq = TargetJFA.jfa_filseq;
    CurrBlock = TargetJFA.jfa_block;
    ReadDirection = DM0J_BACKWARD;

    if ( dcb->dcb_status & (DCB_S_MVCC_TRACE | DCB_S_MVCC_JTRACE) )
	TRdisplay("%@ dm0j_position:%d tran %x seeking LSN %x %d,%d\n",
    			__LINE__,
			tran_id->db_low_tran,
			TargetLSN.lsn_low,
			TargetJFA.jfa_filseq,
			TargetJFA.jfa_block);

    while ( status == E_DB_OK )
    {
	/* If we need a (new) journal file, close any open one, open a new one */
	if ( !j || j->dm0j_jnl_seq != CurrFilseq )
	{
	    if ( j  && j->dm0j_jnl_seq != 0 )
	    {
		status = JFclose(&j->dm0j_jfio, &sys_err);
		if ( status )
		{
		    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG,
				NULL, NULL, 0L, (i4 *)NULL, 
				&err_code, 0);
		    return (log_error(j, E_DM902A_BAD_JNL_CLOSE, &sys_err, dberr));
		}
	    }

	    if ( status == E_DB_OK )
	    {
	        /* Open journal file and position to CurrBlock */
	        status = dm0j_open(DM0J_KEEP_CTX, 
	       			    (char *)&dcb->dcb_jnl->physical,
				    dcb->dcb_jnl->phys_length,
				    &dcb->dcb_name,
				    dcb->dcb_jnl_block_size,
				    CurrFilseq,
				    0,	/* ckp_seq */
				    CurrBlock,
				    DM0J_M_READ,
				    -1,	/* node_id */
				    ReadDirection,
				    (DB_TAB_ID*)NULL,
				    jctx,
				    dberr);
		if ( status == E_DB_OK )
		{
		    j = *jctx;

		    /*
		    ** NB: dm0j_open has set dm0j_next_byte,
		    **     dm0j_bytes_left to zero, and
		    **	   zeroed jh_low_lsn, jh_high_lsn.
		    **
		    **	   It has not yet read the block.
		    **
		    ** If positioning to first or last block of journal
		    ** (DM0J_EXTREME), set sequence to first or last
		    ** block.
		    */
		    if ( CurrBlock == DM0J_EXTREME )
		    {
		        if ( ReadDirection == DM0J_BACKWARD )
			    j->dm0j_sequence--;
			else
			    j->dm0j_sequence++;

			/* Set the block to read, below */
			CurrBlock = j->dm0j_sequence;
		    }
		}
		else if ( dberr->err_code == E_DM9034_BAD_JNL_NOT_FOUND )
		{
		    /*
		    ** Sometimes the CRhdr can contain a predicted filseq
		    ** that is ahead of the last filseq written by the
		    ** archiver, but shouldn't be off by more than one filseq.
		    **
		    ** Try opening the previous file and start the search
		    ** from there.
		    */
		    if ( CurrFilseq == TargetJFA.jfa_filseq && ReadDirection == DM0J_BACKWARD )
		    {
		        if ( --CurrFilseq > 0 )
			{
			    CurrBlock = DM0J_EXTREME;
			    j->dm0j_jnl_seq = 0;
			    status = E_DB_OK;
			    continue;
			}
		    }
		    SETDBERR(dberr, 0, E_DM0055_NONEXT);
		}
	    }
	}
	
	if ( status == E_DB_OK )
	{
	    jh = (DM0J_HEADER*)j->dm0j_page;

	    /* If target is contained in current block, start where we left off */
	    if ( LSN_GTE(&TargetLSN, &jh->jh_low_lsn) &&
		 LSN_LTE(&TargetLSN, &jh->jh_high_lsn) )
	    {
	        /* j->dm0j_bytes_left = 0; */
		CurrBlock = j->dm0j_sequence;
	    }
	    else
	    {
		/* Read new block */
		j->dm0j_sequence = CurrBlock;
		j->dm0j_direction = ReadDirection;
		j->dm0j_bytes_left = 0;
		j->dm0j_last_lsn.lsn_high = 0;
		j->dm0j_last_lsn.lsn_low = 0;

		status = JFread(&j->dm0j_jfio, j->dm0j_page, 
				j->dm0j_sequence, &sys_err);

		if ( status != OK )
		{
		    if ( status == JF_END_FILE )
		    {
			if ( dcb->dcb_status & (DCB_S_MVCC_TRACE | DCB_S_MVCC_JTRACE) )
			    TRdisplay("%@ dm0j_position:%d tran %x EOF %d %d,%d\n",
					    __LINE__,
					    tran_id->db_low_tran,
					    j->dm0j_jfio.jf_blk_cnt,
					    CurrFilseq,
					    CurrBlock);

			if ( j->dm0j_jfio.jf_blk_cnt == 0 )
			{
			    /*
			    ** No blocks have been written.
			    **
			    ** The archiver won't update the block count
			    ** until it completes the archive cycle for
			    ** this DB (by calling dm0j_update()), so
			    ** we'll have to wait for that to happen.
			    **
			    ** Close the file, signal the archiver, 
			    ** sleep five seconds, then try again.
			    */
			    status = JFclose(&j->dm0j_jfio, &sys_err);
			    if ( status )
			    {
				uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err,
					    ULE_LOG, NULL, NULL, 0L, (i4 *)NULL, 
					    &err_code, 0);
				return (log_error(j, E_DM902A_BAD_JNL_CLOSE, 
						&sys_err, dberr));
			    }

			    /* Signal the archiver to make sure it's running */
			    (VOID)LGalter(LG_A_ARCHIVE, NULL, 0, &sys_err);

			    if ( dcb->dcb_status & (DCB_S_MVCC_TRACE | DCB_S_MVCC_JTRACE) )
				TRdisplay("%@ dm0j_position:%d tran %x sleeping %d,%d\n",
					    __LINE__,
					    tran_id->db_low_tran,
					    CurrFilseq,
					    CurrBlock);

			    CSsuspend(CS_LOG_MASK | CS_TIMEOUT_MASK, 5, NULL);
			    /* Show already closed */
			    j->dm0j_jnl_seq = 0;
			    continue;
			}

			/*
			** If EOF on first attempted block
			** try last written block in the file and
			** go from there.
			*/
			if ( BlocksRead == 0 )
			{
			    CurrBlock = j->dm0j_jfio.jf_blk_cnt;
			    status = E_DB_OK;
			    continue;
			}

			/*
			** No such jnl_block.
			** Open next/prev file.
			*/
			if ( ReadDirection == DM0J_FORWARD )
			    CurrFilseq++;
			else
			    CurrFilseq--;

			CurrBlock = DM0J_EXTREME;

			if ( dcb->dcb_status & (DCB_S_MVCC_TRACE | DCB_S_MVCC_JTRACE) )
			    TRdisplay("%@ dm0j_position:%d tran %x %s %d,%d\n",
					    __LINE__,
					    tran_id->db_low_tran,
					    (ReadDirection == DM0J_FORWARD)
					        ? "FORWARD" : "BACKWARD",
					    CurrFilseq,
					    CurrBlock);

			/* Move on to next/prev journal file */
			status = E_DB_OK;
			continue;
		    }

		    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG,
				NULL, NULL, 0L, (i4 *)NULL, 
				&err_code, 0);

		    return (log_error(j, E_DM902D_BAD_JNL_READ, &sys_err, dberr));
		}
		/* Count another block read */
		BlocksRead++;
	    }

	    if ( jh->jh_low_lsn.lsn_low == 0 )
	    {
		/*
		** This can happen because of DM0LJNLSWITCH,
		** which writes an "eof" record without an
		** LSN, or the block just read has no
		** record that begins on this block (spanned).
		**
		** jh_first_rec will also be zero.
		**
		** Skip this block and keep going in the
		** requested direction.
		*/
	        if ( ReadDirection == DM0J_FORWARD )
		    CurrBlock++;
		else if ( --CurrBlock == 0 )
		{
		    CurrFilseq--;
		    CurrBlock = DM0J_EXTREME;
		}
		continue;
	    }

	    /*
	    ** If the LSN we seek is less than the first started
	    ** record on the page, then read previous block.
	    */
	    if ( LSN_LT(&TargetLSN, &jh->jh_low_lsn) )
	    {
		/*
		** Read previous block.
		**
		** If that would be block zero, then open and
		** read previous file.
		*/
		if ( --CurrBlock == 0 )
		{
		    CurrFilseq--;
		    CurrBlock = DM0J_EXTREME;
		}

		ReadDirection = DM0J_BACKWARD;
		continue;
	    }

	    /* 
	    ** If the LSN we seek is greater than the last started record
	    ** on the page, read next block.
	    */
	    if ( LSN_GT(&TargetLSN, &jh->jh_high_lsn) )
	    {
		CurrBlock++;
		ReadDirection = DM0J_FORWARD;
		continue;
	    }

	    /* LSN of interest should be in this block */

	    if ( LSN_EQ(&TargetLSN, &jh->jh_low_lsn) ||
		 LSN_EQ(&TargetLSN, &jh->jh_high_lsn) )
	    {
		/*
		** It's the first or last record in the block.
		** Read forward from the beginning of the block
		** (backread doesn't handle this well if the
		** last record in the block spans to the next- it assumes
		** it read the block after this one in order to get
		** to this one).
		*/
	        j->dm0j_bytes_left = 0;
		ReadDirection = DM0J_FORWARD;
	    }
	    else if ( j->dm0j_last_lsn.lsn_high )
	    {
		/*
		** If we're previously read from this block
		** and our TargetLSN is > the last one read,
		** move forward in the block.
		**
		** If < the last one read, move backward.
		**
		** If the same LSN, fresh read forward.
		*/
	        if ( LSN_GT(&TargetLSN, &j->dm0j_last_lsn) )
		    ReadDirection = DM0J_FORWARD;
		else if ( LSN_LT(&TargetLSN, &j->dm0j_last_lsn) )
		    ReadDirection = DM0J_BACKWARD;
		else if ( LSN_EQ(&TargetLSN, &j->dm0j_last_lsn) )
		{
		    ReadDirection = DM0J_FORWARD;
		    j->dm0j_bytes_left = 0;
		}
		/*
		** If changing read direction, must refresh the block
		** because dm0j_bytes_left is relative to the last
		** read direction.
		*/
		if ( ReadDirection != j->dm0j_direction )
		    j->dm0j_bytes_left = 0;
	    }


	    /* If first read on fresh block, figure out first/last record in block */
	    if ( j->dm0j_bytes_left == 0 )
	    {
		if ( ReadDirection == DM0J_BACKWARD )
		{
		    /* Point to last record on page, compute bytes_left */
		    j->dm0j_next_recnum = count_recs(j->dm0j_page);
		    j->dm0j_next_byte = get_recptr(j->dm0j_page, j->dm0j_next_recnum);
		    j->dm0j_bytes_left = jh->jh_offset - sizeof(DM0J_HEADER);

		    /* If last record on page is incomplete, must read forward */
		    I4ASSIGN_MACRO((*j->dm0j_next_byte), length);

		    if ( j->dm0j_next_byte + length - (PTR)jh > j->dm0j_bksz )
		        ReadDirection = DM0J_FORWARD;
		}

		if ( ReadDirection == DM0J_FORWARD )
		{
		    /* Point to first record on page, compute bytes_left */
		    j->dm0j_next_byte = j->dm0j_page + jh->jh_first_rec;
		    j->dm0j_bytes_left = j->dm0j_bksz - jh->jh_first_rec;
		}
	    }

	    /* Set read direction for dm0j_read */
	    j->dm0j_direction = ReadDirection;

	    while ( status == E_DB_OK )
	    {
		status = dm0j_read(j, (char**)&record, &LogRecordLen, dberr);

		if ( status == E_DB_OK )
		{

		    /* Crash if block does not have expected LSN */
		    if ( CurrBlock == j->dm0j_sequence &&
		         BlocksRead == 0 &&
		         (LSN_LT(&record->lsn, &jh->jh_low_lsn) ||
		          LSN_GT(&record->lsn, &jh->jh_high_lsn)) )
		    {
		        dmd_check(E_DM902D_BAD_JNL_READ);
		    }

		    /*
		    ** dm0j_read may read next/prior block for spanned records,
		    ** keep in synch with it.
		    */
		    CurrBlock = j->dm0j_sequence;

		    if ( LSN_EQ(&TargetLSN, &record->lsn) )
		    {
		        /* Remember the last lsn read */
			j->dm0j_last_lsn = TargetLSN;

			/*
			** Count a direct hit if we're within a few of target block.
			**
			** Being off by 1 or 2 happens because, when completing a cycle,
			** the Archiver updates the config file with the last
			** block written (which may or may not be full), and the
			** next cycle opens the config file and starts writing
			** at config block+1. LGwrite long ago passed this
			** block and based its JFIB filseq/block computation
			** on the assumption that the block would be filled,
			** not shorted.
			**
			** So, for example, if an archive cycle ends at block 79
			** and the next one starts at block 80, the recorded
			** log record's jnl_block will be 79 instead of 80 for
			** the Archiver-shorted records.
			*/
			if ( BlocksRead <= 3 )
			{
			    (*jhit)++;

			    if ( dcb->dcb_status & (DCB_S_MVCC_TRACE | DCB_S_MVCC_JTRACE) )
				TRdisplay("%@ dm0j_position:%d tran %x %d,%d %s %d,%d LSN %x,%x %s\n",
						__LINE__,
						tran_id->db_low_tran,
						TargetJFA.jfa_filseq,
						TargetJFA.jfa_block,
						( BlocksRead == 0 )
						    ? "same jblock"
						    : (BlocksRead == 1)
						        ? "exact jblock"
							: "close jblock",
						CurrFilseq,
						CurrBlock,
						jh->jh_low_lsn.lsn_low,
						jh->jh_high_lsn.lsn_low,
						(ReadDirection == DM0J_FORWARD) 
						    ? "F" : "B");
			}
			else if ( dcb->dcb_status & (DCB_S_MVCC_TRACE | DCB_S_MVCC_JTRACE) )
			{
			    TRdisplay("%@ dm0j_position:%d tran %x %d,%d mishit %d,%d LSN %x,%x %s off by %d\n",
					    __LINE__,
					    tran_id->db_low_tran,
					    TargetJFA.jfa_filseq,
					    TargetJFA.jfa_block,
					    CurrFilseq,
					    CurrBlock,
					    jh->jh_low_lsn.lsn_low,
					    jh->jh_high_lsn.lsn_low,
					    (ReadDirection == DM0J_FORWARD) 
						? "F" : "B",
					    BlocksRead-1);
			}

			/* Return pointer to found record */
			*LogRecord = (PTR)record;
			break;
		    }
		}
	    }

	    break;
	}
    }

    /* Accrete blocks read */
    *jread += BlocksRead;

    return (status);
}

/*{
** Name: dm0j_read	- Read record from journal file.
**
** Description:
**      This routine returns a pointer to the next record in the
**	journal file.
**
** Inputs:
**      jctx                            The journal context.
**
** Outputs:
**      record                          Pointer to record.
**      l_record                        The length of the record.
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
**      04-aug-1986 (Derek)
**          Created for Jupiter.
**	26-dec-1989 (greg)
**	    argument RECORD is PTR* not PTR**
**	    if dm0j_next_byte is not aligned copy into dm0j_record
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    14-may-1991 (bryanp)
**		Log more information if journal record length is wrong.
**		If CL call fails, log CL error status.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Added support for backward reading of journal files for rollforward
**		undo.  Added call to dm0j_backread if the file was open for
**		reading backwards.
**      06-mar-1996 (stial01)
**          Variable Page Size Project: Length in log records is now 4 bytes.
[@history_template@]...
*/
DB_STATUS
dm0j_read(
DM0J_CTX            *jctx,
PTR		    *record,
i4		    *l_record,
DB_ERROR	    *dberr)
{
    DM0J_CTX		*j = jctx;
    i4		length;
    i4		amount;
    char		*r;
    DB_STATUS		status;
    CL_ERR_DESC		sys_err;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** Call to backward reading routine if read direction is backwards.
    */
    if (j->dm0j_direction == DM0J_BACKWARD)
    {
	status = dm0j_backread(j, record, l_record, dberr);
	return (status);
    }

    /*	Read next page if current is empty. */

    if (j->dm0j_bytes_left == 0)
    {
	j->dm0j_sequence++;
	status = JFread(&j->dm0j_jfio, j->dm0j_page, 
                        j->dm0j_sequence, &sys_err);
	if (status != OK)
	{
	    if (status == JF_END_FILE)
	    {
		SETDBERR(dberr, 0, E_DM0055_NONEXT);
		return (E_DB_ERROR);
	    }

	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			err_code, 0);

	    return(log_error(j, E_DM902D_BAD_JNL_READ, &sys_err, dberr));
	}
	j->dm0j_next_byte = j->dm0j_page + sizeof(DM0J_HEADER);
	j->dm0j_bytes_left 
          = ((DM0J_HEADER*)j->dm0j_page)->jh_offset - sizeof(DM0J_HEADER);
    }

    /*	Check that the length field in log record doesn't span pages. */

    if (j->dm0j_bytes_left < sizeof(i4))
    {
	return(log_error(j, E_DM9030_BAD_JNL_FORMAT, &sys_err, dberr));
    }

    /*	Get the length of the next record. */

    I4ASSIGN_MACRO((*j->dm0j_next_byte), length);
    if (length < 0 || length > LG_MAX_RSZ)
    {
	uleFormat(NULL, E_DM9C06_JNL_BLOCK_INFO, (CL_ERR_DESC *)NULL, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			err_code, 5,
			0, j->dm0j_sequence,
			0, j->dm0j_bytes_left,
			0, (char *)j->dm0j_next_byte - (char *)j->dm0j_page,
			0, length,
			0, (i4)LG_MAX_RSZ);

	return(log_error(j, E_DM9031_BAD_JNL_LENGTH, &sys_err, dberr));
    }

    /*
    ** If the next record is contained on the page, just return a pointer.
    ** If the record is unaligned on a BYTE_ALIGN machine or spans a page
    ** then copy the buffer to pre-allocated buffer, dm0j_record.
    **	(greg)
    */

    if ((length <= j->dm0j_bytes_left)
# ifdef	BYTE_ALIGN
        /* lint truncation warning if size of ptr > i4, but code valid */
	&&
	(((i4)j->dm0j_next_byte & (sizeof(ALIGN_RESTRICT) - 1)) == 0)
# endif
       )
    {
	*record = (PTR) j->dm0j_next_byte;
	j->dm0j_bytes_left -= length;
	j->dm0j_next_byte += length;
	*l_record = length;
	return (E_DB_OK);
    }

    /*	Record spans multiple pages, copy into preallocated buffer. */

    *l_record = length;
    *record = (PTR) j->dm0j_record;
    r = j->dm0j_record;
    while (length)
    {
	/*  Move part of the record to the record buffer. */

	amount = length;
	if (amount > j->dm0j_bytes_left)
	    amount = j->dm0j_bytes_left;
	MEcopy(j->dm0j_next_byte, amount, r);
	length -= amount;
	j->dm0j_bytes_left -= amount;
	j->dm0j_next_byte += amount;
	r += amount;
	if (length == 0)
	    return (E_DB_OK);

	/*	Read the next block. */

	j->dm0j_sequence++;
	status = JFread(&j->dm0j_jfio, j->dm0j_page, 
                        j->dm0j_sequence, &sys_err);
	if (status != OK)
	    break;
	j->dm0j_next_byte = j->dm0j_page + sizeof(DM0J_HEADER);
	j->dm0j_bytes_left 
             = ((DM0J_HEADER*)j->dm0j_page)->jh_offset - sizeof(DM0J_HEADER);
    }

    if (status == JF_END_FILE)
    {
	SETDBERR(dberr, 0, E_DM0055_NONEXT);
	return (E_DB_ERROR);
    }

    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			err_code, 0);

    return(log_error(j, E_DM902D_BAD_JNL_READ, &sys_err, dberr));
}

/*{
** Name: dm0j_write	- Write record to journal file.
**
** Description:
**      This routine formats a record into the journal page buffer.
**
** Inputs:
**      jctx                            The journal context.
**      record                          Pointer to record to write.
**      l_record                        The length of the record.
**
** Outputs:
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
**      04-aug-1986 (Derek)
**          Created for Jupiter.
**      26-jan-1989 (roger)
**          JFwrite() was being called with err_code instead of &sys_err.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    14-may-1991 (bryanp)
**		If CL call fails, log CL error status.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Added support for backward reading of journal files for rollforward
**		undo.  Fill in jh_first_rec offset when write the first record
**		to a journal block.
**	23-aug-1993 (bryanp)
**	    Use local pointer -- can't do arithmetic on a PTR datatype.
**	 2-apr-96 (nick)
**	    We would mark the second record in a journal page as the first
**	    if we had less than two bytes left on the current page. #75790
**      06-mar-1996 (stial01)
**          Variable Page Size Project: Length in log records is now 4 bytes.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Record low/high LSN in page's DM0J_HEADER.
*/
DB_STATUS
dm0j_write(
DM0J_CTX            *jctx,
PTR		    record,
i4		    l_record,
DB_ERROR	    *dberr)
{
    DM0J_CTX		*j = jctx;
    DM0J_HEADER		*jh = (DM0J_HEADER*) j->dm0j_page;
    DB_STATUS		status;
    CL_ERR_DESC		sys_err;
    char		*rec_ptr = (char *)record;
    bool		first_record;
    i4			*err_code = &dberr->err_code;
    DM0L_HEADER		*dm0lHeader = (DM0L_HEADER*)record;
    bool		HaveLSN;

    CLRDBERR(dberr);

    /* Some oddball journal records (DM0LJNLSWITCH) don't have LSNs */
    HaveLSN = (dm0lHeader->lsn.lsn_low) ? TRUE : FALSE;

    /*
    ** If this is the first record which starts on this journal block
    ** then update the first_rec pointer to point to the start of this
    ** record.  (Make sure that the start of this record will actually
    ** be written on this block - if there is less than 4 bytes available
    ** then we will skip to the next block).
    */
    if (j->dm0j_bytes_left >= sizeof(i4))
    {
	if (jh->jh_first_rec == 0)
	{
	    /*
	    ** we can fit at least the length portion of
	    ** this record on this page and it's the first 
	    */
	    jh->jh_first_rec = (char *)j->dm0j_next_byte - (char *)j->dm0j_page;

	    /* Record LSN of lowest starting record on page */
	    jh->jh_low_lsn = dm0lHeader->lsn;
	}
	first_record = FALSE;

    }
    else
    {
	/*
	** this record will go on the next journal
	** page so note this fact for use later
	*/
	first_record = TRUE;
    }

    for (;;)
    {
	/* Record highest started LSN on this page */
	if ( jh->jh_first_rec && !first_record && HaveLSN )
	    jh->jh_high_lsn = dm0lHeader->lsn;
	
	if (j->dm0j_bytes_left > l_record) 
	{
	    /* 
	    ** we can fit all this record on the page
	    */
	    MEcopy(rec_ptr, l_record, j->dm0j_next_byte);
	    j->dm0j_next_byte += l_record;
	    j->dm0j_bytes_left -= l_record;
	    return (E_DB_OK);
	}

	/*  Make sure that the record length doesn't span pages. */

	if (j->dm0j_bytes_left >= sizeof(i4))
	{
	    MEcopy(rec_ptr, j->dm0j_bytes_left, j->dm0j_next_byte);
	    l_record -= j->dm0j_bytes_left;
	    rec_ptr += j->dm0j_bytes_left;
	    j->dm0j_bytes_left = 0;
	}
	jh->jh_offset = j->dm0j_bksz - j->dm0j_bytes_left;
    
	status = JFwrite(&j->dm0j_jfio, (PTR)jh, j->dm0j_sequence, &sys_err);
	if (status != OK)
	    break;

	j->dm0j_next_byte = (char *)jh + sizeof(DM0J_HEADER);
	j->dm0j_bytes_left = j->dm0j_bksz - sizeof(DM0J_HEADER);

	/*  Initialize the Journal Page header. */

	jh->jh_seq = ++j->dm0j_sequence;
	if (first_record)
	{
	    first_record = FALSE;
	    jh->jh_first_rec = (char *)j->dm0j_next_byte - (char *)j->dm0j_page;

	    /* Record LSN of first record on page */
	    jh->jh_low_lsn = dm0lHeader->lsn;
	}
	else
	{
	    jh->jh_first_rec = 0;
	    /* Until a record is started on this page, the LSNs will be zero */
	    jh->jh_low_lsn.lsn_high = jh->jh_low_lsn.lsn_low = 0;
	    jh->jh_high_lsn.lsn_high = jh->jh_high_lsn.lsn_low = 0;
	}
    }

    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			err_code, 0);

    return(log_error(j, E_DM902E_BAD_JNL_WRITE, &sys_err, dberr));
}

/*{
** Name: dm0j_update	- Update journal file.
**
** Description:
**      This routine updates the journal file. Also, callers to this
**	routine should always follow it with a call to dm0j_close().
**	This is to insure that the block sequence numbers are kept in
**	sync from one archive pass to the next.
**
**	After calling dm0j_update, the caller may not execute any dm0j_write
**	calls without having closed and reopened the journal file.  This
**	routine does not leave the file positioned so that it can be written
**	again.
**
** Inputs:
**      jctx                            The journal context.
**
** Outputs:
**	sequence			Last block sequence written.
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
**      04-aug-1986 (Derek)
**          Created for Jupiter.
**      12-Dec-1989 (ac)
**          Don't decrease the sequence number if the sequence number indicating
**	    the first block of the journal file.
**	05-sep-1990 (Sandyh)
**	    Always decrement the sequence number so that a pass thru an archive
**	    series without any qualifying data after the first open will not
**	    cause the first block to be skipped. (bug 32124)
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    14-may-1991 (bryanp)
**		If CL call fails, log CL error status.
*/
DB_STATUS
dm0j_update(
DM0J_CTX            *jctx,
i4		    *sequence,
DB_ERROR	    *dberr)
{
    DM0J_CTX		*j = jctx;
    DM0J_HEADER		*jh = (DM0J_HEADER*)j->dm0j_page;
    DB_STATUS		status = OK;
    CL_ERR_DESC		sys_err;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*	Check for empty next block. */

    if (j->dm0j_bytes_left != j->dm0j_bksz - sizeof(DM0J_HEADER))
    {
	/*	Update offset on page. */

	jh->jh_offset = j->dm0j_bksz - j->dm0j_bytes_left;

	/*	Write the last block of the file, and update the file header. */

	status = JFwrite(&j->dm0j_jfio, j->dm0j_page, 
                         j->dm0j_sequence, &sys_err); 
    }
    else
    {
	j->dm0j_sequence--;
    }

    if (status == OK)
	status = JFupdate(&j->dm0j_jfio, &sys_err);

    if (status == OK)
    {
	*sequence = j->dm0j_sequence;
	return (E_DB_OK);
    }

    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			err_code, 0);

    return(log_error(j, E_DM902F_BAD_JNL_UPDATE, &sys_err, dberr));
}

/*{
** Name: dm0j_truncate	- Truncate journal file.
**
** Description:
**      This routine truncates the journal files for the current
**	checkpoint series.
**
** Inputs:
**      jctx                            The journal context.
**
** Outputs:
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
**      04-aug-1986 (Sandyh)
**          Created for Jupiter.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    14-may-1991 (bryanp)
**		If CL call fails, log CL error status.
[@history_template@]...
*/
DB_STATUS
dm0j_truncate(
DM0J_CTX            *jctx,
DB_ERROR	    *dberr)
{
    DM0J_CTX		*j = jctx;
    DB_STATUS		status = OK;
    CL_ERR_DESC		sys_err;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    status = JFtruncate(&j->dm0j_jfio, &sys_err);
    if (status == OK)
	return (E_DB_OK);

    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			err_code, 0);

    return(log_error(j, E_DM9035_BAD_JNL_TRUNCATE, &sys_err, dberr));
}

/*{
** Name: logErrorFcn	- DM0J error logging routine.
**
** Description:
**      This routine is called to log all DM0J errors that occur.
**	The DM0J error is translated to a generic journal error.
** Inputs:
**      jctx                            Journal context.
**	dmf_error			DMF error code to log.
**	sys_err				Pointer to CL error.
**	FileName			Error source file name.
**	LineNumber			Error source line number.
**
** Outputs:
**      dberr				Pointer to resulting DMF error.
**	Returns:
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      23-sep-1986 (Derek)
**	    Created for Jupiter.
**	19-Dec-1989 (walt)
**	    Fixed bug 9063.  Structure elements of _map_error declared
**	    backwards.  Didn't match the initialization elements.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    14-may-1991 (bryanp)
**		If journal context is provided, log info message 
**		E_DM9C00_JNL_FILE_INFO.
**	18-Nov-2008 (jonj)
**	    Renamed to logErrorFcn, called by log_error macro providing
**	    error source information.
*/
static DB_STATUS
logErrorFcn(
DM0J_CTX            *jctx,
DB_STATUS	    dmf_error,
CL_ERR_DESC	    *sys_err,
DB_ERROR	    *dberr,
PTR		    FileName,
i4		    LineNumber)
{
    DM0J_CTX		*j = jctx;
    i4		i;
    DB_STATUS		status;
    DM_FILE		file;
    i4			error;
    const static	struct _map_error
	{
	    DB_STATUS	    error_jnl;
	    DB_STATUS	    error_dm0j;
	}   map_error[] =
	{
	    { E_DM9282_DM0J_OPEN_ERROR, E_DM902C_BAD_JNL_OPEN },
	    { E_DM9280_DM0J_CLOSE_ERROR, E_DM902A_BAD_JNL_CLOSE },
	    { E_DM9283_DM0J_READ_ERROR, E_DM902D_BAD_JNL_READ },
	    { E_DM9284_DM0J_WRITE_ERROR, E_DM902E_BAD_JNL_WRITE },
	    { E_DM9285_DM0J_UPDATE_ERROR, E_DM902F_BAD_JNL_UPDATE },
	    { E_DM9286_DM0J_FORMAT_ERROR, E_DM9030_BAD_JNL_FORMAT },
	    { E_DM9287_DM0J_LENGTH_ERROR, E_DM9031_BAD_JNL_LENGTH },
	    { E_DM9281_DM0J_CREATE_ERROR, E_DM902B_BAD_JNL_CREATE},
	};

    /* Populate DB_ERROR with source information */
    if ( dberr->err_code != dmf_error )
    {
	dberr->err_data = 0;
	dberr->err_file = FileName;
	dberr->err_line = LineNumber;
    }

    /*	Log the error with parameters. */

    if (j)
    {
	DB_TAB_ID	*sf_tab_id;

	if (j->dm0j_sf_tabid.db_tab_base != 0)
	    sf_tab_id = &j->dm0j_sf_tabid;
	else
	    sf_tab_id = NULL;

	/*	Construct the file name. */
	dm0j_filename( DM0J_MERGE, j->dm0j_jnl_seq, j->dm0j_node_id, &file, sf_tab_id );
    
	dberr->err_code = E_DM9C00_JNL_FILE_INFO;
	uleFormat(dberr, 0, sys_err, ULE_LOG, NULL, NULL, 0, NULL,
			&error, 5,
			sizeof(file), &file,
			sizeof(j->dm0j_dbname), &j->dm0j_dbname,
			0, j->dm0j_jnl_seq,
			0, j->dm0j_ckp_seq,
			0, j->dm0j_sequence);

	dberr->err_code = dmf_error;
	uleFormat(dberr, 0, sys_err, ULE_LOG, NULL, NULL, 0, NULL, 
	    &error, 3, 
	    sizeof(DB_DB_NAME), &j->dm0j_dbname,
	    0, j->dm0j_jnl_seq,
	    0, j->dm0j_sequence);
    }

    /*	Map from the logging error to the return error. */

    for (i = 0; i < sizeof(map_error)/sizeof(struct _map_error); i++)
	if (map_error[i].error_dm0j == dmf_error)
	{
	    dberr->err_code = map_error[i].error_jnl;
	    break;
	}

    return (E_DB_ERROR);
}

/*{
** Name: dm0j_merge	- Merge journals node journals into standard ones.
**
** Description:
**	This routine merges journal files created by each node into 
**      standard journal files.
**
**	It is also called by programs such as auditdb, which wish to process
**	journal records in merged order, but which do not need to actually
**	produced the final merged output file.
**
**	Each journal record contains a Log Sequence Number, which was generated
**	when the original log record was written. The node-specific journal
**	files are merged according to this Log Sequence Number.
**
**	The node journal files are named Jnnnnnnn.Nxx and the standard ones
**	Jnnnnnnn.jnl,  Where nnnnnnn is the journal sequence number starting
**	with 1 and xx is a cluster id with values of 1-16.  Therefore node 3
**	could create the first node journal with the name J0000001.N03 and node
**	15 could create the next one with the name J0000002.N15.  At checkpoint
**	time these would be merged into J0000001.JNL.
**
**	If this routine is called for a non-cluster installation, it does not
**	attempt to process any node journal files.  For the non-cluster case,
**	this routine creates an empty journal file, if no files have been
**	created at the time of the checkpoint.  Every checkpoint has at least
**	one Journal files associated with it even if it is empty.
**
** Inputs:
**      cnf                             Configuration file pointer. If the
**					    config file is not yet open, this
**					    should be zero.
**	dcb				Database control block pointer. This is
**					    needed only if the config file is
**					    not yet open; otherwise it should be
**					    0.
**	process_ptr			A context block point to be passed to
**					    the record processing routine. If
**					    this pointer is zero, then the
**					    record processing routine is not
**					    called and a merged journal file
**					    is created instead.
**	process_record			Auditdb record processing routine. If
**					    this is NOT auditdb, then this
**					    parameter should be zero.
**
** Outputs:
**      err_code                        The reason for error return status.
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
**      22-jun-1987 (Jennifer)
**          Created for Jupiter.
**      27-Nov-1989 (ac)
**          Fixed several multiple journal files merging problems.
**	20-may-90 (bryanp)
**	    After updating the config file, make a backup copy of it.
**	19-nov-1990 (bryanp)
**	    Reset cnode_la in the jnl_node_info when done with the merge.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	     3-may-1991 (rogerk)
**		Back out the above change to clear the cnode_la field in the
**		jnl_node_info after a merge.  While the cluster info relating
**		to the sequence/block number of the cluster journal files are
**		no longer needed after a merge, the log file address to which
**		the db has been journaled on this node is still needed.
**	    14-may-1991 (bryanp)
**		If CL call fails, log CL error status.
**	    24-oct-1991 (jnash) 
**		Remove erroneous initialization of "h" caught by lint.  It
**		was in any case initialized inline.
**      08-jul-1992 (mikem)
**          Reorganized logic from a for (;;) loop to a do/while to get around
**	05-aug-1992 (jnash)
**	    6.5 Recovery Project: Sort by cluster-wide LSN (included in each
**	    log record) rather than LPS (written as separate record per 
**	    log block).  This required retaining the last record
**	    in each journal file, space for which is allocated dynamically
**	    and pointed to within DM0J_CNODE.  
**	26-apr-1993 (bryanp)
**	    Enhanced so that this version could be shared by auditdb, thus
**		eliminating much duplicate code in dmfatp.c.
**	    Pass a JFIO to JFlistfile.
**	26-jul-1993 (bryanp)
**	    Fix error handling bug in dm0j_merge().
**	20-sep-1993 (bryanp)
**	    Fix cluster support bugs in dm0j_merge.
**      31-sep-1994 (cohmi01)
**          dm0j_merge() - Init cur_jnl as it wont be set for AUDITDB, though
**              it gets passed to error rtn, causing coredump. Change setting
**              of node info in merge phase (in EOF processing) to use
**              'lowest_index' instead of a stale value of 'i' set during
**              the initialization loop. For AUDITDB error/warning handling,
**              pass back actual status values from callback routines
**              provided by auditdb, as auditdb handles warning seperately
**              from DB_ERROR conditions. Ensure status set throughout code.
**      26-oct-1994 (harpa06)
**          Fixed the lint problem "next_found == TRUE" (bug #65264)
**	23-mar-2002 (devjo01)
**	    Fixed defective/inefficient merge algorithim.
**	06-May-2005 (jenjo02)
**	    Handle E_DM0055_NONEXT return from "process_record"
**	    function. The function has seen enough of the
**	    journal and wants it closed.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
*/
DB_STATUS
dm0j_merge(
    DM0C_CNF	*cnf,
    DMP_DCB	*dcb,
    PTR		process_ptr,
    DB_STATUS	(*process_record)
		    (PTR process_ptr, PTR record, i4 *err_code),
    DB_ERROR	*dberr)
{
    DM0C_CNF	    *config = cnf;
    DB_STATUS	    status,local_status;
    CL_ERR_DESC	    sys_err;
    i4	    fil_seq;
    i4	    blk_seq;
    i4	    sequence;
    i4	    length;
    i4	    next_found;
    DM_PATH         *jpath;
    i4              l_jpath;
    DM0J_CTX        *cur_jnl = 0;   /* init, wont get set if auditdb.     */
    JFIO	    listfile_jfio;
    i4         first_file; 
    i4	    last_file;
    DM0J_CNODE      node_array[DM_CNODE_MAX];
    DM0J_CS         cs;
    PTR             record; 
    i4         l_record;
    DM0L_HEADER	    *h;
    DMP_MISC        *file_cb;
    DM0J_CFILE      *file_array;
    DMP_MISC        *rec_cb;
    i4	    rec_array_size;
    bool            created;
    i4         i,k,n;
    bool            done,eof;
    i4         count; 
    i4	    open_count;
    i4         file_count;
    i4         file_array_size;
    i4         node_id;
    LG_LSN	    start_lsn;
    i4         lowest_index;
    i4		node,numnodes;
    i4		activenodes[DM_CNODE_MAX];
    i4			*err_code = &dberr->err_code;
    DB_ERROR		local_dberr;

    CLRDBERR(dberr);

    if (cnf == 0)
    {
	/*
	** If config file is not already open, open it. Auditdb uses this
	** feature.
	*/
	status = dm0c_open(dcb, 0, dmf_svcb->svcb_lock_list, &config, dberr);
	if (status)
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM923A_CONFIG_OPEN_ERROR);
	    return (status);
	}
    }

    /*  Find journal location. */

    for (i = 0; i < config->cnf_dsc->dsc_ext_count; i++)
    {
	if (config->cnf_ext[i].ext_location.flags & DCB_JOURNAL)
	    break;
    }

    l_jpath = config->cnf_ext[i].ext_location.phys_length;
    jpath = &config->cnf_ext[i].ext_location.physical;
	
    i = config->cnf_jnl->jnl_count - 1;
    fil_seq = config->cnf_jnl->jnl_history[i].ckp_l_jnl;
    first_file = config->cnf_jnl->jnl_history[i].ckp_l_jnl;
    last_file = config->cnf_jnl->jnl_fil_seq;
    sequence = config->cnf_jnl->jnl_blk_seq;

    created = FALSE;

    if (process_ptr == (PTR)NULL)
    {
	/*
	** If this is not auditdb, then we need to create a journal file.
	*/
	    
	status = dm0j_open(DM0J_MERGE, (char *)jpath, l_jpath, 
		    &config->cnf_dsc->dsc_name, config->cnf_jnl->jnl_bksz,
		    fil_seq, config->cnf_jnl->jnl_ckp_seq,
		    sequence + 1, DM0J_M_WRITE, -1, DM0J_FORWARD,
		    (DB_TAB_ID *)0, &cur_jnl, dberr);

	if (status != E_DB_OK)
	{
	    if (dberr->err_code != E_DM9034_BAD_JNL_NOT_FOUND)	
	    {
		log_error((DM0J_CTX *)NULL, E_DM902C_BAD_JNL_OPEN, 
		    (CL_ERR_DESC *)NULL, dberr);
		return(status);
	    }

	    /* Create the file */
	    status = get_next_jnl(cnf, &cur_jnl, dberr);
	    if (status != E_DB_OK)
		return  (status);

	    created = TRUE;
	}
    }

    /*
    ** Loop through the node section of config file, get 
    ** the node sequence numbers.  If they are all zero, no
    ** files have been created and there is nothing more to do. 
    */

    done = TRUE;
    for ( i = 0; i < DM_CNODE_MAX; i++ )
	if (config->cnf_jnl->jnl_node_info[i].cnode_fil_seq != 0) 
	{
	    done = FALSE;
	    break;
	}

    if (CXcluster_enabled() == 0 || (done == TRUE))
    {
	/*
	** Not running on a cluster or nothing to do, just close files and
	** return.  We must close the journal file we opened and if we created
	** a journal file, then we must update the config file to show the
	** newly-created journal file.
	*/

	if (process_ptr != (PTR)NULL)
	{
	    status = dm0c_close(config, 0, dberr);
	    return (status);
	}

	i = config->cnf_jnl->jnl_count - 1;
	config->cnf_jnl->jnl_fil_seq = 
	    config->cnf_jnl->jnl_history[i].ckp_l_jnl;

	status = dm0c_close(cnf, DM0C_UPDATE | DM0C_LEAVE_OPEN | DM0C_COPY,
   				dberr);
	if (status != E_DB_OK)
	    return(status);

	status = dm0j_close(&cur_jnl, dberr);
	if (status != E_DB_OK)
	{
	    log_error(cur_jnl, E_DM902A_BAD_JNL_CLOSE, (CL_ERR_DESC *)NULL, 
		dberr);
	    return(status);
	}

	return (E_DB_OK);
    }

    for (;;)
    {
	/*
	** Allocate space for journal file record buffers (one per 
	** active node) 
	*/
	
	numnodes = 0;
	for ( i = 0; i < DM_CNODE_MAX; i++ )
	    if ((n = config->cnf_jnl->jnl_node_info[i].cnode_fil_seq) != 0)
	    {
		activenodes[numnodes++] = i;
	    }

	rec_array_size = numnodes * LG_MAX_RSZ;
	status = dm0m_allocate(rec_array_size + sizeof(DMP_MISC),
	    (i4)0, (i4)MISC_CB, (i4)MISC_ASCII_ID, (char *)NULL, 
	    (DM_OBJECT **)&rec_cb, dberr);
	if (status != OK)
	{
	    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM9329_DM0J_M_ALLOCATE);
	    break;
	}
	rec_cb->misc_data = (char*)rec_cb + sizeof(DMP_MISC);

	/*  Set up array of nodes, determine current file for each node. */

	k = 0;
	for ( i = 0; i < DM_CNODE_MAX; i++ )
	{
	    char 	*rec_adr;

	    node_array[i].count = 0;
	    node_array[i].state = DM0J_CINVALID;
	    node_array[i].cur_fil_seq = 0;
	    node_array[i].cur_jnl = 0;
	    if ((n = config->cnf_jnl->jnl_node_info[i].cnode_fil_seq) != 0) 
	    {
		node_array[i].state = DM0J_CVALID;
		node_array[i].last_fil_seq = n;	    
		node_array[i].lsn.lsn_high = -1;	
		node_array[i].lsn.lsn_low = -1;	
		rec_adr = (char *)(((char *)rec_cb + sizeof(DMP_MISC)) + 
			(k * LG_MAX_RSZ));
		node_array[i].record = rec_adr;
		k++;
	    }	
	}

	/*
	** Figure out how many files have been created during this
        ** checkpoint.  Allocate enough space for each of these files
        ** in file array.
	*/

	file_count = 0;
	status = JFlistfile(&listfile_jfio, jpath->name,
		l_jpath, count_files, (PTR)&file_count, &sys_err);

	TRdisplay("%@ DM0J_MERGE: jnl file count %d first %d last %d\n", 
	    file_count, first_file, last_file);

	if (status != JF_END_FILE)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM932A_DM0J_M_LIST_FILES);
	    break;
	}
	file_array_size = file_count * sizeof(DM0J_CFILE);
	status = dm0m_allocate(file_array_size + sizeof(DMP_MISC),
	    (i4)0, (i4)MISC_CB, (i4)MISC_ASCII_ID, (char *)NULL, 
	    (DM_OBJECT **)&file_cb, dberr);
	if (status != OK)
	{
	    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM9329_DM0J_M_ALLOCATE);
	    break;
	}
	file_array = (DM0J_CFILE *)((char *)file_cb + sizeof(DMP_MISC));
	file_cb->misc_data = (char*)file_array;
	cs.file_array = file_array;
	cs.index = 0;
	cs.first = first_file;
	cs.last = last_file;

	/* Get the list of files, fill in file array. */
	status = JFlistfile(&listfile_jfio, jpath->name,
		l_jpath, store_files, (PTR)&cs,
		&sys_err);
	if (status != JF_END_FILE)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			err_code, 0);

	    SETDBERR(dberr, 0, E_DM932A_DM0J_M_LIST_FILES);
	    break;
	}
	    
	/* If there are no files, there is a problem. */

	if (cs.index == 0)
	{
	    SETDBERR(dberr, 0, E_DM932B_DM0J_M_NO_FILES);
	    break;
	}	

	/*
	** Find first file for each node, insert 
	** seq number of this file into node_array.
	** Also update the count of files in each node and the
	** total number of files to process.
	*/

	count = 0;
	for (i = 0; i < cs.index; i++)
	{
	    /*
	    ** If node_id is greater than possible, then 
            ** this file has already been processed. 
	    */

	    node_id = file_array[i].node_id;
	    if (node_id > DM_CNODE_MAX) 
                 continue; 
	    else
	    {
		if (node_array[node_id].state == DM0J_CINVALID)
		{
		    SETDBERR(dberr, 0, E_DM932C_DM0J_M_BAD_NODE_JNL);
		    status = E_DB_ERROR;
		    break;
		}

		count++;
		node_array[node_id].count++;
		if (node_array[node_id].cur_fil_seq == 0)
		{
		    node_array[node_id].cur_fil_seq = file_array[i].seq;

		    /* Note this has been used by setting node id too large. */
		    file_array[i].node_id = DM_CNODE_MAX + 1;
		}
	    }
	}

	/* If none of the files were legal, there is a problem. */
	if (dberr->err_code)
	    break;
	if (count == 0)
	{
	    status = E_DB_ERROR;  /* init, as we now return specific status */
	    SETDBERR(dberr, 0, E_DM932C_DM0J_M_BAD_NODE_JNL);
	    break;
	}	
	
	/* 
	** Run through the node_array, open the current journal files 
	** and read the first record.  If valid, extract the LSN 
	** (the lowest for this node) and plug it in the node_array.  If 
	** there are no records in the journal, flag it DM0J_CINVALID.
	*/

	open_count     = 0;
	start_lsn.lsn_high = -1;
	start_lsn.lsn_low  = -1;
	lowest_index   = -1;

    	for ( node = 0; node < numnodes; node++ )
	{
	    i = activenodes[node];
	    if (node_array[i].state == DM0J_CVALID)
	    {
		done = FALSE;
		while ( ! done )
		{
		    TRdisplay("%@ DM0J_MERGE: merge node %d seq %d\n", i,
		        node_array[i].cur_fil_seq);
		    status = dm0j_open(DM0J_MERGE, (char *)jpath, l_jpath, 
			&config->cnf_dsc->dsc_name, config->cnf_jnl->jnl_bksz,
			node_array[i].cur_fil_seq, config->cnf_jnl->jnl_ckp_seq,
			0, DM0J_M_READ, i, DM0J_FORWARD, 
			(DB_TAB_ID *)0, &node_array[i].cur_jnl, dberr);
		    if (status != E_DB_OK)
		    {
			log_error((DM0J_CTX *)NULL, E_DM902C_BAD_JNL_OPEN, 
				(CL_ERR_DESC *)NULL, dberr);
			break;
		    }

		    status = dm0j_read(node_array[i].cur_jnl, 
					&record, &l_record, dberr);
		    if (status != E_DB_OK)
		    {
			if (dberr->err_code == E_DM0055_NONEXT)
			{
			    /* Empty journal */
			    status = dm0j_close(&node_array[i].cur_jnl, 
                                                dberr);
			    if ( status != E_DB_OK )
			    {
				/* XXX new error XXX */
				TRdisplay(
				 "%@ DM0J_MERGE: dm0j_close failure ignored\n");
			    }
			    CLRDBERR(dberr);

			    count--;
			    if (--node_array[i].count == 0)
			    {
				/* Finished with this node. */
				node_array[i].state = DM0J_CINVALID;
				done = TRUE;
				break;
			    }			    
			    for (k=0; k < cs.index; k++)
			    {
				if (file_array[k].node_id == i)
				{
				    node_array[i].cur_fil_seq = 
                                               file_array[k].seq;
				    file_array[k].node_id = DM_CNODE_MAX + 1;
				    break;			
				}
			    }
			}
			else
			{
			    /* Error other than empty file. */
			    SETDBERR(dberr, 0, E_DM932D_DM0J_M_BAD_READ);
			    break;
			}
		    }
		    else     /* Status is OK, save what we need. */
		    {
			h = (DM0L_HEADER *)record;
			node_array[i].lsn.lsn_high = h->lsn.lsn_high;
			node_array[i].lsn.lsn_low = h->lsn.lsn_low;
			node_array[i].l_record = l_record;
			MEcopy((PTR)record, l_record, 
				(PTR)node_array[i].record);
			open_count++;

			/* 
			** Save the lowest thus far encountered 
			** New macro?
			*/
			if ((node_array[i].lsn.lsn_high < 
				start_lsn.lsn_high) || 
			    (((node_array[i].lsn.lsn_high == 
				   start_lsn.lsn_high)
			       && (node_array[i].lsn.lsn_low < 
					start_lsn.lsn_low)) ) )
			{
	    		    start_lsn = node_array[i].lsn;
			    lowest_index = i;
			}

		        done = TRUE;
		    }

		}    /* of processing one node */

		if (dberr->err_code)
		    break;

	    }       /* of processing valid node_array entry. */
	}           /* of processing all nodes */ 

	if (dberr->err_code)
	    break;

	if ((open_count == 0) || 
	    (lowest_index == -1) ||
	    ((start_lsn.lsn_high == -1) && (start_lsn.lsn_low == -1)))
	    done = TRUE;
	else
	    done = FALSE;


	/* 
	** Now we can merge.  Examine each record from each journal, 
	** write to the merged journal in LSN order.  The first record 
	** from each file has already been read.  
	** Note that "done" now refers to the entire merge, not 
	** processing one node.
	*/
	while ( ! done )
	{
	    eof = FALSE;
	    for (;;)
	    {
		/* Determine which of the records has the lowest lsn */
                start_lsn.lsn_low  = -1;
		start_lsn.lsn_high = -1;
                lowest_index   = -1;
		for ( node = 0; node < numnodes; node++ )
		{
		    k = activenodes[node];
                    if (node_array[k].state != DM0J_CVALID)
                        continue;
                    if ( (node_array[k].lsn.lsn_high < start_lsn.lsn_high) || 
                        ( (node_array[k].lsn.lsn_high == start_lsn.lsn_high)
                               &&  (node_array[k].lsn.lsn_low < 
					start_lsn.lsn_low) ) )
                    {
			start_lsn = node_array[k].lsn;    /* struct assign */
                        lowest_index = k;
                    }
                }

                if (lowest_index == -1)
		{
		    done = TRUE;
		    break;
		}

#ifdef xMERGE_TEST
		TRdisplay
		   ("DM0J_WRITE: Write file %d, lsn_low %d to merged journal\n",
			lowest_lsn, node_array[lowest_index].lsn.lsn_low);
#endif

		/*
		** If we are to hand this record to a processing routine, do
		** so. Otherwise, write the current lowest to the merged
		** journal, read the next record from this file
		*/
		if (process_record)
		{
		    status = (*process_record)(process_ptr,
					    node_array[lowest_index].record,
					    err_code);
		    if ( status )
			SETDBERR(dberr, 0, *err_code);

		    /*
		    ** If the callback returns NONEXT, it's logically
		    ** done with the merged journals, so quit now.
		    */
		    if ( status && dberr->err_code == E_DM0055_NONEXT )
		    {
			status = dm0j_close(&node_array[lowest_index].cur_jnl, 
                                                dberr);

			node_array[lowest_index].cur_jnl = 0;
			eof = TRUE;
			done = TRUE;
			break;
		    }
		}
		else
		{
		    status = dm0j_write(cur_jnl,
					    node_array[lowest_index].record,
					    node_array[lowest_index].l_record,
					    dberr);
		}
		if (status != E_DB_OK)
		{
		    log_error(cur_jnl, E_DM9284_DM0J_WRITE_ERROR, 
				(CL_ERR_DESC *)NULL, dberr);
		    break;
		}

		status = dm0j_read(node_array[lowest_index].cur_jnl, 
                              &record, &l_record, dberr);

		if (status != E_DB_OK && dberr->err_code == E_DM0055_NONEXT)
		{
		    eof = TRUE;
		    status = E_DB_OK;
		    CLRDBERR(dberr);
		    break;
		}
		else if (status != E_DB_OK)
		{
		    log_error(node_array[lowest_index].cur_jnl, 
			E_DM9284_DM0J_WRITE_ERROR,
			(CL_ERR_DESC *)NULL, dberr);
		    break;
		}

		/* Update node_array w new info */
		h = (DM0L_HEADER *)record;
		node_array[lowest_index].lsn = h->lsn;
		node_array[lowest_index].l_record = l_record;
		MEcopy((PTR)record, l_record, 
			(PTR)node_array[lowest_index].record);

		/* 
		** Check the block sequence number, may need new file. 
		** (Add Code here when we want to create new journal
		** file whem merged file gets too big.)
		*/
	    }

	    if (status != E_DB_OK)
		break;

	    /* If eof detected, skip to next file in sequence or end */
	    if ( (eof == TRUE) && ( ! done ) )
	    {
		next_found = FALSE;
		while ( ! next_found )
		{
		    status = dm0j_close(&node_array[lowest_index].cur_jnl, 
                                                dberr);
		    node_array[lowest_index].cur_jnl = 0;
		    if (--count == 0)
		    {
			/* No more nodes */
			done = TRUE;
			break;
		    }	

		    if (--node_array[lowest_index].count == 0)
		    {
			/* Finished with this node. */
			node_array[lowest_index].state = DM0J_CINVALID;
			break;
		    }			    
		    else   /* get next file for this node */
		    {	    
			for (k=0; k < cs.index; k++)
			{
			    if (file_array[k].node_id == lowest_index)
			    {
				node_array[lowest_index].cur_fil_seq = 
						   file_array[k].seq;
				file_array[k].node_id = DM_CNODE_MAX + 1;
				break;			
			    }
			}
		    
			TRdisplay("%@ DM0J_MERGE: merge node %d seq %d\n", 
				lowest_index,
				node_array[lowest_index].cur_fil_seq);

			status = dm0j_open(DM0J_MERGE, (char *)jpath, l_jpath, 
			    &config->cnf_dsc->dsc_name, 
                            config->cnf_jnl->jnl_bksz,
			    node_array[lowest_index].cur_fil_seq, 
			    config->cnf_jnl->jnl_ckp_seq,
			    0, DM0J_M_READ, lowest_index, DM0J_FORWARD,
			    (DB_TAB_ID *)0,
                            &node_array[lowest_index].cur_jnl,
			    dberr);

			if (status != E_DB_OK)
			{
			    log_error((DM0J_CTX *)0, E_DM902C_BAD_JNL_OPEN, 
				(CL_ERR_DESC *)NULL, dberr);
			    break;
			}

			status = dm0j_read(node_array[lowest_index].cur_jnl, 
				    &record, &l_record, dberr);

			if (status == E_DB_OK )
			{
		            /* Update node_array w new info */
			    h = (DM0L_HEADER *)record;
			    i = lowest_index;
			    node_array[i].lsn.lsn_high = h->lsn.lsn_high;
			    node_array[i].lsn.lsn_low = h->lsn.lsn_low;
			    node_array[i].l_record = l_record;
			    MEcopy((PTR)record, l_record,
                                (PTR)node_array[i].record);
			    next_found = TRUE;
			}
			else if ( dberr->err_code == E_DM0055_NONEXT )
			{
			    continue;
			}
			else
			{
			    /* XXX New error XXX */
			    TRdisplay("%@ DM0J_MERGE: dm0j_read error: %d\n",
				dberr->err_code);
			    break;
			}
		    }   /* of next file for this node */
		}       /* ! next_found */
	    }           /* of eof processing */
	}               /* ! done */

	if (status != E_DB_OK)
	    break;

	if (process_ptr != (PTR)NULL)
	{
	    /*
	    ** All records have been processed. Just close the config file we
	    ** locally opened and return.
	    */
	    status = dm0c_close(config, 0, dberr);
	    return(status);
	}

	/* All .Nxx files should be closed at this point. 
	** Now close the .JNL file update the config file, 
        ** then delete all the .nxx files. 
	*/

	/* XXXXX Add update of blk_seq for this journal. */

	/*  Update journal file. */

	status = dm0j_update(cur_jnl, &sequence, dberr);
	if (status != E_DB_OK)
	{
	    log_error(cur_jnl, E_DM902A_BAD_JNL_CLOSE, (CL_ERR_DESC *)NULL, 
		dberr);
	    break;
	}

	status = dm0j_close(&cur_jnl, dberr);
	if (status != E_DB_OK)
	{
	    log_error(cur_jnl, E_DM902A_BAD_JNL_CLOSE, (CL_ERR_DESC *)NULL, 
		dberr);
	    break;
	}

	/*
	** Update the current file and block sequence values for the
	** cluster merged journal files.
	*/
	TRdisplay("%@ DM0J_MERGE: Update jnl_fil_seq %d->%d\n",
		config->cnf_jnl->jnl_fil_seq,
		config->cnf_jnl->jnl_history[i].ckp_l_jnl);

	i = config->cnf_jnl->jnl_count - 1;
	config->cnf_jnl->jnl_fil_seq 
                    = config->cnf_jnl->jnl_history[i].ckp_l_jnl;
	config->cnf_jnl->jnl_blk_seq = sequence;

	/*
	** Clear the current file and block sequence number for the
	** individual cluster node journal files since we just threw
	** them all away after merging.
	**
	** The journaled log address value (cnode_la) is still needed
	** however.  This records the place in this node's log file to
	** which we have completed archiving log records for the database.
	** The next time archiving is needed from this node, we will start
	** processing at that spot.
	*/
	for ( i = 0; i < DM_CNODE_MAX; i++)
	{
	    config->cnf_jnl->jnl_node_info[i].cnode_fil_seq = 0;
	    config->cnf_jnl->jnl_node_info[i].cnode_blk_seq = 0;
	}	


   	status = dm0c_close(cnf, DM0C_UPDATE | DM0C_LEAVE_OPEN | DM0C_COPY,
   			    dberr);
	if (status != E_DB_OK)
	{
	    TRdisplay("%@ DM0J_MERGE: Configuration file close error\n");
	    break;
	}
	
	for ( i= 0; i < cs.index; i++)
	{
	    TRdisplay("%@ DM0J_MERGE: JFdelete %12.12s \n",
		file_array[i].filename.name);
	    local_status = JFdelete(&listfile_jfio, jpath->name, l_jpath, 
                                file_array[i].filename.name, 
                                file_array[i].l_filename, 
				&sys_err);
	    if ( local_status != OK )
		TRdisplay("%@ DM0J_MERGE: JFdelete error on %s ignored\n", 
			file_array[i].filename.name);
	}

	TRdisplay("%@ DM0J_MERGE: Begin listfile after merge \n");
	status = JFlistfile(&listfile_jfio, jpath->name,
			l_jpath, store_files, (PTR)&cs, &sys_err);
	TRdisplay("%@ DM0J_MERGE: End listfile after merge \n");

	return (E_DB_OK);	
	
    } 

    /* If here we had an error, close any open files. return 
    ** error. 
    */

    if (process_ptr == (PTR)NULL)
    {
	/* Close merged journal file, ignore errors, if any. */
	local_status = dm0j_close(&cur_jnl, &local_dberr);
    }

    for (i = 0; i < DM_CNODE_MAX; i++)
    {
	if (node_array[i].cur_jnl != 0)
	    local_status = dm0j_close(&node_array[i].cur_jnl, &local_dberr);
    }
    if (created == TRUE)
	local_status = dm0j_delete(DM0J_MERGE, (char *)jpath, 
			   l_jpath, fil_seq, (DB_TAB_ID *)0, &local_dberr);
	
    /* To support warnings for AUDITDB - as requested by ICL               */
    /* Just to be safe, if we are here (must be error) and status is 'OK'  */
    /* then default to E_DB_ERROR, else return real status.                */

    return(status == E_DB_OK ? E_DB_ERROR : status);
    
}

/*{
** Name: get_next_jnl - Open or Create and open the next journal.
**
** Description:
**	This routine is a utility routine for the dm0j_merge
**      procedure.  This routine Creates and open a new journal 
**      file during the merge phase.
**
** Inputs:
**      cnf                             Configuration file pointer.
**      jnl                             Journal file io descriptor.
**
** Outputs:
**      err_code                        The reason for error return status.
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
**      22-jun-1987 (Jennifer)
**          Created for Jupiter.
**	15-mar-1993 (rogerk)
**	    Added direction parameter to dm0j_open routine.
[@history_template@]...
*/
static DB_STATUS
get_next_jnl(
DM0C_CNF            *cnf,
DM0J_CTX            **cur_jnl,
DB_ERROR	    *dberr)
{
    DM0C_CNF	    *config = cnf;
    DB_STATUS	    status;
    CL_ERR_DESC	    sys_err;
    i4	    fil_seq;
    i4	    blk_seq;
    i4	    length;
    char	    *jpath;
    i4              l_jpath;
    i4         i;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

	/*  Find journal location. */

	for (i = 0; i < config->cnf_dsc->dsc_ext_count; i++)
	{
	    if (config->cnf_ext[i].ext_location.flags & DCB_JOURNAL)
		break;
	}

	l_jpath = config->cnf_ext[i].ext_location.phys_length;
	jpath = (char *) &config->cnf_ext[i].ext_location.physical;

	
	/* Create the journal file. */	    	


	/* Update the last journal file for this 
	** checkpoint, it it is the first, set it also.
	*/

	i = config->cnf_jnl->jnl_count -1;
	if (config->cnf_jnl->jnl_history[i].ckp_f_jnl == 0)
	{
	    
	    config->cnf_jnl->jnl_history[i].ckp_f_jnl = 
		    config->cnf_jnl->jnl_fil_seq;
	    
	    config->cnf_jnl->jnl_history[i].ckp_l_jnl = 
			config->cnf_jnl->jnl_fil_seq;
	}
    	fil_seq = config->cnf_jnl->jnl_history[i].ckp_l_jnl;
	TRdisplay("%@ DM0J_MERGE: Create merged jnl file %d\n", fil_seq);
	status = dm0j_create(DM0J_MERGE, jpath, l_jpath,
		fil_seq,
		config->cnf_jnl->jnl_bksz, config->cnf_jnl->jnl_blkcnt, 
                -1, (DB_TAB_ID *)0, dberr);

	/*  Handle error creating journal file. */

	if (status != OK)
	{
	    log_error(NULL, E_DM902B_BAD_JNL_CREATE, &sys_err, dberr);
	    return(E_DB_ERROR);
	}


	TRdisplay("%@ DM0J_MERGE: Open merged jnl file %d\n", fil_seq);
	status = dm0j_open(DM0J_MERGE, jpath, l_jpath, 
		    &config->cnf_dsc->dsc_name, config->cnf_jnl->jnl_bksz,
		    fil_seq, config->cnf_jnl->jnl_ckp_seq,
		    1, DM0J_M_WRITE, -1, DM0J_FORWARD,
		    (DB_TAB_ID *)0, cur_jnl, dberr);
	if (status != E_DB_OK)
	{
	    log_error(NULL, E_DM902C_BAD_JNL_OPEN, &sys_err, dberr);
	    return(E_DB_ERROR);
	}


	return (E_DB_OK);
}

/*{
** Name: store_files	- Stores all valid cluster journal file names.
**
** Description:
**      This routine is called to store all valid cluster journal file
**      names in the file_array. 
**	This routine will only keep the following
**	file names:
**		Jnnnnnnn.Nxx   Cluster journal files.
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
**	    JF_END_FILE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      23-jun-1987 (Jennifer)
**          Created for Jupiter.
[@history_template@]...
*/
static DB_STATUS
store_files(
DM0J_CS             *cs,
char		    *filename,
i4		    l_filename,
CL_ERR_DESC	    *sys_err)
{
    DB_STATUS	    status;
    DB_STATUS	    *err_code;
    JFIO	    jf_file;
    i4         number, node;
    i4		j, store_index;
    i4		src, dst;
        
    if (l_filename == sizeof(DM_FILE) &&
	filename[8] == '.' &&
	filename[0] == 'j' &&
        filename[9] == 'n' )

    {
	/* Get sequence number and node id from file name. */
	

	number = 0;
	node = 0;
	number = (number * 10) + (filename[1] - '0');
	number = (number * 10) + (filename[2] - '0');
	number = (number * 10) + (filename[3] - '0');
	number = (number * 10) + (filename[4] - '0');
	number = (number * 10) + (filename[5] - '0');
	number = (number * 10) + (filename[6] - '0');
	number = (number * 10) + (filename[7] - '0');
	node = (node * 10) + (filename[10] - '0');
	node = (node * 10) + (filename[11] - '0');
	
	if ((number < cs->first) || (number > cs->last))
	{
	    TRdisplay("%@ DM0J_MERGE: ignoring   %t\n", l_filename, filename);
	    /* FIX ME ignored files make sure they are empty and JFdelete */
	    return (OK);	
	}

	TRdisplay("%@ DM0J_MERGE: store_file %t\n",
	    l_filename, filename);

	/*
	** Add to the list, keep it sorted by sequence
	** Note there may be duplicate sequence numbers,
	** however (node, sequence) will be unique
	*/
	store_index = cs->index;
	if (cs->index > 0)
	{
	    for (j = 0; j < cs->index; j++)
	    {
	        if (cs->file_array[j].seq > number)
		{
		    store_index = j;
		    /* Starting at last entry, right shift all entries */
		    for (src = cs->index - 1, dst = cs->index; 
			    src >= j; src--, dst --)
			MEcopy(&cs->file_array[src],
				    sizeof(DM0J_CFILE),
					&cs->file_array[dst]);
		    break;
		}
	    }
	}

	cs->file_array[store_index].node_id = node;
	cs->file_array[store_index].seq     = number;
	MEmove(l_filename, filename, ' ', 
               sizeof(DM_FILE), (PTR)&cs->file_array[store_index].filename);
	cs->file_array[store_index].l_filename = l_filename;
	cs->index++;
	return (OK);

    }
    else 
    {
	return (OK);
    }

}

/*{
** Name: count_files	- Counts all valid cluster journal file names.
**
** Description:
**      This routine is called to count all valid cluster journal file
**      names in the file_array. 
**	This routine will only count the following
**	file names:
**		Jnnnnnnn.Nxx   Cluster journal files.
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
**	    JF_END_FILE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      23-jun-1987 (Jennifer)
**          Created for Jupiter.
[@history_template@]...
*/
static DB_STATUS
count_files(
i4             *count,
char		    *filename,
i4		    l_filename,
CL_ERR_DESC	    *sys_err)
{
    DB_STATUS	    status;
    DB_STATUS	    *err_code;
    JFIO	    jf_file;
    i4         number, node;
        
    if (l_filename == sizeof(DM_FILE) &&
	filename[8] == '.' &&
	filename[0] == 'j' &&
        filename[9] == 'n' )

    {

	(*count)++;

    }
    return (OK);
}

/*{
** Name: dm0j_backread	- Read records from journal file in reverse order.
**
** Description:
**      This routine returns a pointer to the next record in the
**	journal file.  Rows are returned in reverse order until the beginning
**	of the journal file is reached.
**
**	The journal file must be first opened via dm0j_open with the
**	DM0J_BACKWARD direction parameter.
**
** Inputs:
**      jctx                            The journal context.
**
** Outputs:
**      record                          Pointer to record.
**      l_record                        The length of the record.
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
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV: written.
**	26-jul-1993 (jnash)
**	    Fix BOF dm0j_sequence number check to be <= 0 instead of < 0.
**      06-mar-1996 (stial01)
**          Variable Page Size Project: Length in log records is now 4 bytes.
**              Use dm0m_lcopy to manipulate the dm0l_record field, since
**              it may exceed 64K in length.
[@history_template@]...
*/
DB_STATUS
dm0j_backread(
DM0J_CTX            *jctx,
PTR		    *record,
i4		    *l_record,
DB_ERROR	    *dberr)
{
    DM0J_CTX		*j = jctx;
    i4		length;
    i4		amount;
    i4		amount_copied;
    char		*buf_ptr;
    STATUS		status;
    CL_ERR_DESC		sys_err;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** Read the next page if current is empty or exhausted.
    */
    if (j->dm0j_bytes_left == 0)
    {
	j->dm0j_sequence--;
	if (j->dm0j_sequence <= 0)
	{
	    SETDBERR(dberr, 0, E_DM0055_NONEXT);
	    return (E_DB_ERROR);
	}

	status = JFread(&j->dm0j_jfio, j->dm0j_page, 
                        j->dm0j_sequence, &sys_err);
	if (status != OK)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	    return(log_error(j, E_DM902D_BAD_JNL_READ, &sys_err, dberr));
	}

	j->dm0j_bytes_left 
          = ((DM0J_HEADER*)j->dm0j_page)->jh_offset - sizeof(DM0J_HEADER);

	/*
	** Initialize the next-record indexes to the last record on the page
	** If there are no records on this page (or only a portion of a
	** record which has spanned over from a previous page) then recnum
	** is set to 0 and next_byte to the 1st byte past the block header.
	*/
	j->dm0j_next_recnum = count_recs(j->dm0j_page);
	j->dm0j_next_byte = get_recptr(j->dm0j_page, j->dm0j_next_recnum);
    }

    /*
    ** If the next record is fully contained on the page, just return a pointer.
    ** If the record is unaligned on a BYTE_ALIGN machine or spans a page
    ** then copy the buffer to pre-allocated buffer, dm0j_record.
    ** (If dm0j_next_recnum is non-zero then it points to the start of a valid
    ** record.  If it is zero, then the rest of the used bytes on this page -
    ** if any - are just the overflow of a record which starts on a previous 
    ** journal page.)
    **
    ** Note the assumption here that if the start of the next record is
    ** contained on this page that the entire record is fully contained on the
    ** page.  This is guaranteed because if the record were to overflow onto
    ** the next page, it would have already been returned when the overflow
    ** record processing below was done on the last page we processed.
    */
    if (j->dm0j_next_recnum > 0)
    {
	I4ASSIGN_MACRO((*j->dm0j_next_byte), length);
	if (length > LG_MAX_RSZ)
	{
	    uleFormat(NULL, E_DM9C06_JNL_BLOCK_INFO, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 5,
		0, j->dm0j_sequence, 0, j->dm0j_bytes_left,
		0, (char *)j->dm0j_next_byte - (char *)j->dm0j_page,
		0, length, 0, (i4)LG_MAX_RSZ);
	    return(log_error(j, E_DM9031_BAD_JNL_LENGTH, NULL, dberr));
	}

	*record = (PTR) j->dm0j_next_byte;
	*l_record = length;

#ifdef	BYTE_ALIGN
	/*
	** If byte alignment is required and the journal record is not,
	** then copy the record to our save buffer.
	*/

        /* lint truncation warning if size of ptr > int, but code valid */
	if ((i4)j->dm0j_next_byte & (sizeof(ALIGN_RESTRICT) - 1))
	{
	    MEcopy((PTR)j->dm0j_next_byte, length, (PTR)j->dm0j_record);
	    *record = (PTR) j->dm0j_record;
	}
#endif

	j->dm0j_bytes_left -= length;

	/*
	** Position the next-record indexes to the previous record on the page.
	*/
	j->dm0j_next_recnum--;
	j->dm0j_next_byte = get_recptr(j->dm0j_page, j->dm0j_next_recnum);

	return (E_DB_OK);
    }

    /*
    ** Record spans multiple pages.  Copy this tail of the record into the
    ** preallocated buffer, then read the previous journal page and prepend
    ** the front of the record onto it.
    **
    ** If the previous journal page also has only a portion of the record,
    ** then keep reading backwards until the front of the record is found.
    */
    amount_copied = 0;
    buf_ptr = j->dm0j_record + LG_MAX_RSZ;

    while (j->dm0j_next_recnum == 0)
    {
	/*
	** Make sure that we are not overflowing our journal buffer.
	*/
	amount = j->dm0j_bytes_left;
	if ( amount < 0 || (amount_copied + amount) > LG_MAX_RSZ )
	{
	    uleFormat(NULL, E_DM9C06_JNL_BLOCK_INFO, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 5,
		0, j->dm0j_sequence, 0, j->dm0j_bytes_left,
		0, (char *)j->dm0j_next_byte - (char *)j->dm0j_page,
		0, amount_copied + amount, 0, LG_MAX_RSZ);
	    return(log_error(j, E_DM9031_BAD_JNL_LENGTH, NULL, dberr));
	}

	/*
	** Copy any trailing bytes of an incomplete journal record to the
	** end of the journal buffer.
	*/
	if (amount)
	{
	    buf_ptr -= amount;
	    MEcopy((PTR)j->dm0j_next_byte, amount, (PTR)buf_ptr);
	    amount_copied += amount;
	}

	/*
	** Read the previous journal page.
	*/
	j->dm0j_sequence--;
	if (j->dm0j_sequence < 0)
	{
	    uleFormat(NULL, E_DM0055_NONEXT, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    status = FAIL;
	    break;
	}

	status = JFread(&j->dm0j_jfio, j->dm0j_page, 
                        j->dm0j_sequence, &sys_err);
	if (status != OK)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    break;
	}

	j->dm0j_bytes_left 
             = ((DM0J_HEADER*)j->dm0j_page)->jh_offset - sizeof(DM0J_HEADER);

	j->dm0j_next_recnum = count_recs(j->dm0j_page);
	j->dm0j_next_byte = get_recptr(j->dm0j_page, j->dm0j_next_recnum);
    }

    if (status != OK)
	return(log_error(j, E_DM902D_BAD_JNL_READ, (CL_ERR_DESC*)NULL, dberr));

    /*
    ** Now prepend the front of the journal record to the journal buffer
    ** and return a pointer to that buffer.
    */
    amount = ((DM0J_HEADER*)j->dm0j_page)->jh_offset - 
					(j->dm0j_next_byte - j->dm0j_page);
    buf_ptr -= amount;
    MEcopy((PTR)j->dm0j_next_byte, amount, (PTR)buf_ptr);

    j->dm0j_bytes_left -= amount;
    amount_copied += amount;

    /*
    ** Extract the length and compare it against what we copied into the
    ** journal buffer.
    */
    I4ASSIGN_MACRO((*j->dm0j_next_byte), length);
    if (length != amount_copied)
    {
	uleFormat(NULL, E_DM9C06_JNL_BLOCK_INFO, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 5,
	    0, j->dm0j_sequence, 0, j->dm0j_bytes_left,
	    0, (char *)j->dm0j_next_byte - (char *)j->dm0j_page,
	    0, length, 0, (i4)amount_copied);
	return(log_error(j, E_DM9031_BAD_JNL_LENGTH, NULL, dberr));
    }

    *record = (PTR) buf_ptr;
    *l_record = length;

#ifdef	BYTE_ALIGN
    /*
    ** If byte alignment is required and the buffer copy of the  record is not,
    ** then copy the record back to the start of our save buffer.
    */

    /* lint truncation warning if size of ptr > int, but code valid */
    if ((i4) buf_ptr & (sizeof(ALIGN_RESTRICT) - 1))
    {
	MEcopy((PTR)buf_ptr, length, (PTR)j->dm0j_record);
	*record = (PTR) j->dm0j_record;
    }
#endif

    /*
    ** Position the next-record indexes to the previous record on the page.
    */
    j->dm0j_next_recnum--;
    j->dm0j_next_byte = get_recptr(j->dm0j_page, j->dm0j_next_recnum);

    return (E_DB_OK);
}

/*{
** Name: count_recs	- Count the number of journal records on a journal page.
**
** Description:
**	This routine takes a journal page and returns the number of journal
**	records that are either contained on the page or started on the page
**	and then overflow onto the next page.
**
**	Records which started on a previous page and overflow onto this one
**	are not counted.
**
** Inputs:
**      page                            The journal page.
**
** Outputs:
**	Returns:
**	    The number of records on the page.
**
** Side Effects:
**	    none
**
** History:
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV: written.
**      06-mar-1996 (stial01)
**          Variable Page Size Project: Length in log records is now 4 bytes.
[@history_template@]...
*/
static i4
count_recs(
char	   *page)
{
    DM0J_HEADER		*jh;
    char		*rec_ptr;
    i4             length;
    i4		count = 0;

    jh = (DM0J_HEADER *) page;

    /*
    ** The jh_first_rec header value gives the offset to the start of the
    ** first record on the page.  If it is zero, there are no records which
    ** which begin on this page.
    */
    if (jh->jh_first_rec == 0)
	return (0);

    rec_ptr = (char *)page + jh->jh_first_rec;
    while (rec_ptr < ((char *)page + jh->jh_offset))
    {
	count++;

	I4ASSIGN_MACRO((*rec_ptr), length);
	rec_ptr += length;
    }

    return (count);
}

/*{
** Name: get_recptr	- Get a pointer to an arbitrary journal record.
**
** Description:
**	This routine takes a journal page and returns a pointer to the nth
**	journal record on that page.
**
** Inputs:
**      page                            The journal page.
**	recnum				The index of the record desired.
**
** Outputs:
**	Returns:
**	    A character pointer to the requested record.
**
** Side Effects:
**	    none
**
** History:
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV: written.
**	26-jul-1993 (jnash)
**	    If recnum == 0, return address of first byte past journal header,
**	    fixing a problem where only part of a journal record was read if 
**	    part of it was on another page.
**      06-mar-1996 (stial01)
**          Variable Page Size Project: Length in log records is now 4 bytes.
[@history_template@]...
*/
static char *
get_recptr(
char	   *page,
i4	   recnum)
{
    DM0J_HEADER		*jh;
    char		*rec_ptr;
    i4             length;
    i4		count;

    if (recnum == 0)
	return (page + sizeof(DM0J_HEADER));

    jh = (DM0J_HEADER *) page;

    rec_ptr = (char *)page + jh->jh_first_rec;

    for (count = 1; count < recnum; count++)
    {
	I4ASSIGN_MACRO((*rec_ptr), length);
	rec_ptr += length;
    }

    return (rec_ptr);
}

/*{
** Name: dm0j_filename  - Given a sequence number returns the journal filename.
**
** Description:
**
** Inputs:
**      flag;                           Zero or DM0J_MERGE or DM0J_RFP_INCR
**                                      If you do not want the cluster name,
**                                      must be DM0J_MERGE or DM0J_RFP_INCR
**      jnl_seq                         Journal file sequence number.
**      nodeid                          Only valid of flag = DM0J_MERGE.
**                                      If value = -1, nodeid not used
**                                      else node_id is used to create name.
**      sf_tab_id			Table id when journal file is 
**					online modify sidefile
**
** Outputs:
**      filename                        Pointer to filename
**      Returns:
**          None.
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      22-nov-1994 (andyw)
**          Partial backup/Recovery Project:
**              Created.
*/
VOID
dm0j_filename(
i4             flag,
i4             jnl_seq,
i4             node_id,
DM_FILE             *filename,
DB_TAB_ID	*sf_tab_id)
{
    static char 	basename[] = "j0000000.jnl";
    static char		digitlookup[] = { '0', '1', '2', '3', '4', '5', 
			    '6', '7', '8', '9' };
    i4                  i, number;
 
 
    if (sf_tab_id)
    {
	dm2f_filename(DM2F_TAB, sf_tab_id, 0, 0, 0, filename);
	filename->name[8] = '.';
	filename->name[9] = 'j';
	filename->name[10] = 'n';
	filename->name[11] = 'l';
    }
    else
    {
	MEcopy( basename, 12, filename->name );
	number = jnl_seq;
	for ( i = 7; number > 0 && i > 0; i-- )
	{
	    filename->name[i] = digitlookup[number % 10];
	    number = number / 10;
	}
    }

    if (flag & DM0J_RFP_INCR)
    {
	TRdisplay("dm0j_filename flag %x (DM0J_RFP_INCR)\n",flag);
	filename->name[9] =  'r';
	filename->name[10] = 'f';
	filename->name[11] = 'p';
	return;
    }
 
    flag &= DM0J_MERGE;
    if ( ( flag && (node_id > 0) ) || ( !flag && CXcluster_enabled() ) )
    {
        filename->name[9] = 'n';
        filename->name[10] = digitlookup[(node_id / 10) % 10];
        filename->name[11] = digitlookup[node_id % 10];
 
    }
 
    return;
 
}
