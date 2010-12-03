/*
** Copyright (c) 1989, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cx.h>
#include    <di.h>
#include    <jf.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <lk.h>
#include    <lg.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm0c.h>
#include    <dm0d.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0m.h>

/**
**
**  Name: DM0D.C - Dump routines.
**
**  Description:
**      This file contains the routines that implement the dump file
**	I/O primitives.
**
**	NOTE: There is quite a bit of code in this file which was intended to
**	support a VAXCluster version of Online Checkpoint. However, only a
**	partial version of Online Checkpoint exists in a cluster installation.
**	This is because we don't have support for merging the Before Image
**	files which are (would be) made in a multi-node online checkpoint.
**	Therefore, at most one node is allowed to be accessing a database when
**	online checkpoint is running. The dm0d_merge() routine
**	does not exist, and the static support routines (get_next_dmp(),
**	store_files(), and count_files()) are not currently used. I am not
**	aware of all the issues involved in adding Cluster support; at a
**	minimum you should analyze the code which attempts to ignore failed
**	checkpoints during roll-forward and see if it needs change.
**
**          dm0d_close - Close dump file.
**	    dm0d_create	- Create a dump file.
**          dm0d_open - Open dump file.
**          dm0d_read - Read from dump file.
**          dm0d_write - Write to dump file.
**	    dm0d_update - Update dump file.
**	    dm0d_truncate - Truncate dump file.
**          dm0d_merge - Merge VAXcluster dump files. (NOT IMPLEMENTED)
**          dm0d_config_restore - Copy config file from dump directory.
**	    dm0d_backread - Read from dump file in reverse direction.
**
**  History:    $Log-for RCS$
**      06-jan-1989 (EdHsu)
**          Created for Terminator.
**	 6-dec-1989 (rogerk)
**	    Added dm0d_config_restore routine.
**	4-may-90 (bryanp)
**	    Added a number of comments about the lack of cluster support. Also
**	    added a new parameter to the (UNUSED) get_next_dmp() routine in
**	    support of the "recovery from last valid checkpoint" feature.
**	    Thirdly, added support to dm0d_config_restore() to allow it to
**	    restore the config file even if there is no current config file. If
**	    no config file is found, we create one.
**	    Changed arguments to dm0d_config_restore so that it could restore
**	    the 'Cnnnnnnn.dmp' version of the file or the 'aaaaaaaa.cnf'
**	    version of the file, as indicated by the caller.
**	30-apr-1991 (bryanp)
**	    If CL call fails, log CL error status.
**	7-July-1992 (rmuth)
**	    Prototyping DMF.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Cast some parameters to quiet compiler complaints following
**		    prototyping of LK, CK, JF, SR.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	18-apr-1994 (jnash)
**          fsync project.  Remove DIforce() calls.
**      14-sep-1994 (andyw)
**          Partial Backup/Recovery Project:
**              Created dm0d_filename so that the rollforwarddb can determine
**              the dump filename to pass to OpenCheckpoint.
**      06-mar-1996 (stial01)
**          Variable Page Size Project:
**              Length in log records is now 4 bytes.
**	03-sep-1998 consi01 bug 91957 problem INGSRV 463
**	    Changes to support page sizes > 8k. Cloned routines from dm0j.c
**          as these are able to support records spanning the block size.
**	    Removed comments around code in dm0d_write to bring in into line
**	    with dm0j_write. Removed dm0d_read and replaced it with a modified
**	    version of dm0j_read. Cloned dm0j_backread and support functions 
**	    count_recs and get_recptr.
**      10-May-1999 (stial01)
**          dm0d_open() Added direction parameter, save end blob number.
**          dm0d_read() Removed direction paramter, get it from DM0D_CTX.
**          dm0d_write() Set dh->dh_first_rec in the DM0D_HEADER
*           Removed dmd_scan(), dm0d_end_sequence is intialized in dm0d_open.
**          (related change 438561 by consi01 for B91957)
**      08-Oct-2002 (hanch04)
**          Removed dm0m_lcopy,dm0m_lfill.  Use MEcopy, MEfill instead.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      01-sep-2004 (stial01)
**          Support cluster online checkpoint
**      30-sep-2004 (stial01)
**          dm0d_open() Remove code setting sequence 0 to sequence 1,
**          it causes merge of cluster dump files to fail reading the file.
**          unless sequence DM0D_EXTREME is specified.
**      20-oct-2004 (stial01)
**          store_files() add journal files to the file array in sequence order
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	14-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	19-Aug-2009 (kschendel) 121804
**	    Need cx.h for proper CX declarations (gcc 4.3).
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Prototype static dm0d_backread().
**/

/*
**  Definition of static functions.
*/
static DB_STATUS 	store_files(
				DM0D_CS     	*cs,
				char		*filename,
				i4    	l_filename,
				CL_ERR_DESC	*sys_err );

static DB_STATUS 	count_files(
				i4     	*count,
				char		*filename,
				i4    	l_filename,
				CL_ERR_DESC	*sys_err );

static DB_STATUS 	get_next_dmp(
				DM0C_CNF    	*cnf,
				DM0D_CTX    	**cur_dmp,
				i4		dmp_history_no,
				DB_ERROR	*dberr );

static DB_STATUS 	logErrorFcn(
				DM0D_CTX        *jctx,
				i4	    	dmf_error,
				CL_ERR_DESC	*sys_err,
				DB_ERROR	*dberr,
				PTR		FileName,
				i4		LineNumber);
#define log_error(jctx,dmf_error,sys_err,dberr)\
	logErrorFcn(jctx,dmf_error,sys_err,dberr,__FILE__,__LINE__)

static i4  	count_recs(
    				char           *page);

static char *   	get_recptr(
    				char           *page,
    				i4        recnum);

static DB_STATUS dm0d_backread(
			DM0D_CTX            *jctx,
			PTR		    *record,
			i4		    *l_record,
			DB_ERROR	    *dberr);

/*{
** Name: dm0d_close	- Close a dump file.
**
** Description:
**      This routine will close a dump context that was previously
**	opened.
**
** Inputs:
**      jctx                            The dump context.
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
**      06-jan-1989 (EdHsu)
**          Created for Terminator.
**	30-apr-1991 (bryanp)
**	    If CL call fails, log CL error status.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	15-jan-1999 (nanpr01)
**	    Pass pointer to pointer in dm0m_deallocate.
[@history_template@]...
*/
DB_STATUS
dm0d_close(
    DM0D_CTX    **jctx,
    DB_ERROR	*dberr )
{
    DM0D_CTX		*j = *jctx;
    DB_STATUS		status;
    CL_ERR_DESC		sys_err;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    for (;;)
    {
	/*  Close the dump file. */

	status = JFclose(&j->dm0d_jfio, &sys_err);
	if (status != OK)
	    break;

	/*  Deallocate the DM0D control block. */

	dm0m_deallocate((DM_OBJECT **)jctx);
        return( status );

    }


    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char * )0, 0L, (i4 *)NULL, 
			err_code, 0);

    return (log_error(j, E_DM9353_BAD_DMP_CLOSE, &sys_err, dberr));
}

/*{
** Name: dm0d_create	- Create a dump file.
**
** Description:
**      This routine creates dump file.
**
** Inputs:
**      flag;                           Must be zero or DM0D_MERGE
**                                      if you do not want the cluster name.
**	device				Pointer to device name.
**	l_device			Length of device name.
**	dmp_seq				Dump file sequence number.
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
**      06-jan-1989 (EdHsu)
**          Created for Terminator.
**	30-apr-1991 (bryanp)
**	    If CL call fails, log CL error status.
**	15-mar-1993 (rogerk)
**	    Add parameter count to ule_format call.
[@history_template@]...
*/
DB_STATUS
dm0d_create(
    i4     flag,
    char	*device,
    i4		l_device,
    i4     dmp_seq,
    i4    	bksz,
    i4    	blkcnt,
    i4     node_id,
    DB_ERROR	*dberr )
{
    DB_STATUS		    status;
    i4		    i;
    DM_FILE		    file;
    JFIO		    jf_io;
    CL_ERR_DESC		    sys_err;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*	Construct the file name. */
    dm0d_filename( flag, dmp_seq, node_id, &file);

    /*  Open and position the dump file. */

    status = JFcreate(&jf_io, device, l_device, file.name, sizeof(file), 
	bksz, blkcnt, &sys_err);
    if (status == OK)
	return (E_DB_OK);

    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char * )0, 0L, (i4 *)NULL, 
			err_code, 0);
    
    uleFormat(NULL, E_DM9354_BAD_DMP_CREATE, &sys_err, 
            ULE_LOG, NULL, 0, 0, 0, err_code, 3,
	    l_device, device, 
	    sizeof(DM_FILE), &file,
	    0, dmp_seq);

    /*	Map from the logging error to the return error. */

    SETDBERR(dberr, 0, E_DM935F_DM0D_CREATE_ERROR);
    return (E_DB_ERROR);
}

/*{
** Name: dm0d_delete 	- Delete a dump file.
**
** Description:
**      This routine creates dump file.
**
** Inputs:
**      flag;                           Must be zero or DM0D_MERGE
**                                      if you do not want the cluster name.
**	l_device			Length of device name.
**	dmp_seq				File sequence number.
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
**      06-jan-1989 (EdHsu)
**          Created for Terminator.
**	30-apr-1991 (bryanp)
**	    If CL call fails, log CL error status.
[@history_template@]...
*/
DB_STATUS
dm0d_delete(
    i4     flag,
    char    	*device,
    i4	    	l_device,
    i4     dmp_seq,
    DB_ERROR	*dberr )
{
    DB_STATUS		    status;
    i4		    i;
    i4		    number;
    DM_FILE		    file;
    JFIO		    jf_io;
    CL_ERR_DESC		    sys_err;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*	Construct the file name. */

    number = dmp_seq;
    file.name[0] = 'd';
    file.name[1] = ((number / 1000000) % 10) + '0';
    file.name[2] = ((number / 100000) % 10) + '0';
    file.name[3] = ((number / 10000) % 10) + '0';
    file.name[4] = ((number / 1000) % 10) + '0';
    file.name[5] = ((number / 100) % 10) + '0';
    file.name[6] = ((number / 10) % 10) + '0';
    file.name[7] = (number % 10) + '0';
    file.name[8] = '.';
    file.name[9] = 'd';
    file.name[10] = 'm';
    file.name[11] = 'p';
    
    /*  Open and position the dump file. */

    status = JFdelete(&jf_io, device, l_device, file.name, sizeof(file), 
	&sys_err);
    if (status == OK || status == JF_NOT_FOUND)
	return (E_DB_OK);

    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char * )0, 0L, (i4 *)NULL, 
			err_code, 0);

    log_error(NULL, E_DM935B_BAD_DMP_DELETE, &sys_err, dberr);
    return (E_DB_ERROR);
}

/*{
** Name: dm0d_open	- Open a dump file.
**
** Description:
**      This routine opens a dump file and initializes the dump
**	context area.
**
** Inputs:
**      flag;                           Must be zero or DM0D_MERGE
**                                      if you do not want the cluster name.
**	l_device			Length of device name.
**	bksz				Block size.
**	dmp_seq				Dump file sequence number.
**	ckp_seq				Checkpoint sequence number.
**	sequence			Dump block sequence.
**	mode				Either DM0D_M_READ or DM0D_M_WRITE.
**      nodeid                         Only valid of flag = DM0D_MERGE.
**                                      If value = -1, nodeid not used
**                                      else node_id is used to create name. 
**
** Outputs:
**	jctx				Pointer to allocated dump context.
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
**      06-jan-1989 (EdHsu)
**          Created for Terminator.
**	03-sep-1998 consi01 bug 91957 problem INGSRV 463
**	    Initialize dh->dh_first_rec to 0
**	    Remove dh->dh_extra as it no longer exists
**	15-jan-1999 (nanpr01)
**	    Pass pointer to pointer in dm0m_deallocate.
**      10-May-1999 (stial01)
**          dm0d_open() Added direction parameter, save end blob number.
[@history_template@]...
*/
DB_STATUS
dm0d_open(
    i4      flag,
    char	*device,
    i4		l_device,
    DB_DB_NAME	*db_name,
    i4	bksz,
    i4	dmp_seq,
    i4	ckp_seq,
    i4	sequence,
    i4	mode,
    i4     node_id,
    i4          direction,
    DM0D_CTX	**jctx,
    DB_ERROR	*dberr )
{
    DM0D_CTX		    *j = 0;
    DM0D_HEADER		    *dh;
    DB_STATUS		    status;
    i4		    i;
    i4		    number;
    i4		    end_sequence;
    i4                      dmp_end_sequence;
    DM_FILE		    file;
    CL_ERR_DESC		    sys_err;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*	Construct the file name. */
    dm0d_filename( flag, dmp_seq, node_id, &file);
    TRdisplay("DMP open flag %d dmp_seq %d filename %12.12s\n", flag, 
	    dmp_seq, file.name);

    for (;;)
    {
	/*  Allocate a DM0D control block. */

	status = dm0m_allocate(sizeof(DM0D_CTX) + bksz + DM0D_MRZ,
	    0, DM0D_CB, DM0D_ASCII_ID, (char *)&dmf_svcb, 
	    (DM_OBJECT **)jctx, dberr);
	if (status != E_DB_OK)
	    break;

	/*  Initialize the rest of the control block. */

	j = *jctx;
	j->dm0d_record = (char *)j + sizeof(DM0D_CTX);
	j->dm0d_page = (char *)j->dm0d_record + DM0D_MRZ;
	j->dm0d_dmp_seq = dmp_seq;
	j->dm0d_ckp_seq = ckp_seq;
	j->dm0d_sequence = sequence;
	j->dm0d_bksz = bksz;
	j->dm0d_next_byte = (char *)j->dm0d_page + sizeof(DM0D_HEADER);
	j->dm0d_bytes_left = bksz - sizeof(DM0D_HEADER);
	j->dm0d_next_recnum = 0;
	j->dm0d_direction = direction;
	j->dm0d_dbname = *db_name;

	/*  Open and position the dump file. */

	status = JFopen(&j->dm0d_jfio, device, l_device,
	    file.name, sizeof(file), 
	    bksz, &dmp_end_sequence, &sys_err);
	if (status != OK)
	{
	    if ((status == JF_NOT_FOUND) || (status == JF_DIR_NOT_FOUND))
		SETDBERR(dberr, 0, E_DM9352_BAD_DMP_NOT_FOUND);
	    else
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char * )0, 0L, (i4 *)NULL, 
			err_code, 0);
		SETDBERR(dberr, 0, E_DM9355_BAD_DMP_OPEN);
	    }
	    break;
	}

	/*
	** Initialize context dependent on open mode and direction.
	** If Open for read, set next_byte and bytes_left to zero so that
	** the first dm0j_read request will cause a new block to be read.
	**
	** If the starting sequence number passed in was DM0D_EXTREME, then
	** set the starting page number to the begining or end of the file
	** depending on the read direction.
	**
	** (Actually set to one past where first read will be done since
	** the dm0j_read call will increment/decrement the sequence number
	** for the first read).
	*/
	if (mode == DM0D_M_READ)
	{
	    j->dm0d_next_byte = 0;
	    j->dm0d_bytes_left = 0;

	    if (sequence == DM0D_EXTREME)
	    {
		if (direction == DM0D_BACKWARD)
		    j->dm0d_sequence = dmp_end_sequence + 1;
		else
		    j->dm0d_sequence = 0;
	    }
	}

	/*  Initialize the dump page header. */
	dh = (DM0D_HEADER*) j->dm0d_page;
	dh->dh_type = DH_V1_S1;
	STRUCT_ASSIGN_MACRO(*db_name, dh->dh_dbname);
	dh->dh_seq = sequence;
	dh->dh_dmp_seq = j->dm0d_dmp_seq;
	dh->dh_ckp_seq = j->dm0d_ckp_seq;
	dh->dh_offset = sizeof(*dh);
	dh->dh_first_rec = 0;

    	return (E_DB_OK);
    }

    /*	Error recovery. */

    if (*jctx)
    {
	DB_ERROR	local_dberr;

	dm0m_deallocate((DM_OBJECT **)jctx);
	if (dberr->err_code != E_DM9352_BAD_DMP_NOT_FOUND)
	    log_error(*jctx, dberr->err_code, &sys_err, &local_dberr);
    }

    return (E_DB_ERROR);
}
/*{
** Name: dm0d_backread  - Read records from dump file in reverse order.
**
** Description:
**      This routine returns a pointer to the next record in the
**      dump file.  Rows are returned in reverse order until the beginning
**      of the dump file is reached.
**
** Inputs:
**      jctx                            The dump context.
**
** Outputs:
**      record                          Pointer to record.
**      l_record                        The length of the record.
**      err_code                        The reason for an error return status.
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	03-sep-1998 consi01 bug 91957 problem INGSRV 463
**	   Creation - based on dm0j_backread in dm0j.c.
[@history_template@]...
*/
DB_STATUS
dm0d_backread(
DM0D_CTX            *jctx,
PTR		    *record,
i4		    *l_record,
DB_ERROR	    *dberr)
{
    DM0D_CTX		*j = jctx;
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
    if (j->dm0d_bytes_left == 0)
    {
	j->dm0d_sequence--;
	if (j->dm0d_sequence <= 0)
	{
	    SETDBERR(dberr, 0, E_DM0055_NONEXT);
	    return (E_DB_ERROR);
	}

	status = JFread(&j->dm0d_jfio, j->dm0d_page, 
                        j->dm0d_sequence, &sys_err);
	if (status != OK)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char * )0, 0L, (i4 *)NULL, err_code, 0);
	    return (log_error(j, E_DM9356_BAD_DMP_READ, &sys_err, dberr));
	}

	j->dm0d_bytes_left 
          = ((DM0D_HEADER*)j->dm0d_page)->dh_offset - sizeof(DM0D_HEADER);

	/*
	** Initialize the next-record indexes to the last record on the page
	** If there are no records on this page (or only a portion of a
	** record which has spanned over from a previous page) then recnum
	** is set to 0 and next_byte to the 1st byte past the block header.
	*/
	j->dm0d_next_recnum = count_recs(j->dm0d_page);
	j->dm0d_next_byte = get_recptr(j->dm0d_page, j->dm0d_next_recnum);
    }

    /*
    ** If the next record is fully contained on the page, just return a pointer.
    ** If the record is unaligned on a BYTE_ALIGN machine or spans a page
    ** then copy the buffer to pre-allocated buffer, dm0j_record.
    ** (If dm0j_next_recnum is non-zero then it points to the start of a valid
    ** record.  If it is zero, then the rest of the used bytes on this page -
    ** if any - are just the overflow of a record which starts on a previous 
    ** dump page.)
    **
    ** Note the assumption here that if the start of the next record is
    ** contained on this page that the entire record is fully contained on the
    ** page.  This is guaranteed because if the record were to overflow onto
    ** the next page, it would have already been returned when the overflow
    ** record processing below was done on the last page we processed.
    */
    if (j->dm0d_next_recnum > 0)
    {
	I4ASSIGN_MACRO((*j->dm0d_next_byte), length);
	if (length > DM0D_MRZ)
	{
	    return (log_error(j, E_DM935A_BAD_DMP_LENGTH, NULL, dberr));
	}

	*record = (PTR) j->dm0d_next_byte;
	*l_record = length;

#ifdef	BYTE_ALIGN
	/*
	** If byte alignment is required and the dump record is not,
	** then copy the record to our save buffer.
	*/

        /* lint truncation warning if size of ptr > int, but code valid */
	if ((i4)j->dm0d_next_byte & (sizeof(ALIGN_RESTRICT) - 1))
	{
	    MEcopy((PTR)j->dm0d_next_byte, length, (PTR)j->dm0d_record);
	    *record = (PTR) j->dm0d_record;
	}
#endif

	j->dm0d_bytes_left -= length;

	/*
	** Position the next-record indexes to the previous record on the page.
	*/
	j->dm0d_next_recnum--;
	j->dm0d_next_byte = get_recptr(j->dm0d_page, j->dm0d_next_recnum);

	return (E_DB_OK);
    }

    /*
    ** Record spans multiple pages.  Copy this tail of the record into the
    ** preallocated buffer, then read the previous dump page and prepend
    ** the front of the record onto it.
    **
    ** If the previous dump page also has only a portion of the record,
    ** then keep reading backwards until the front of the record is found.
    */
    amount_copied = 0;
    buf_ptr = j->dm0d_record + DM0D_MRZ;

    while (j->dm0d_next_recnum == 0)
    {
	/*
	** Make sure that we are not overflowing our dump buffer.
	*/
	amount = j->dm0d_bytes_left;
	if ((amount_copied + amount) > DM0D_MRZ)
	{
	    return (log_error(j, E_DM935A_BAD_DMP_LENGTH, NULL, dberr));
	}

	/*
	** Copy any trailing bytes of an incomplete dump record to the
	** end of the dump buffer.
	*/
	if (amount)
	{
	    buf_ptr -= amount;
	    MEcopy((PTR)j->dm0d_next_byte, amount, (PTR)buf_ptr);
	    amount_copied += amount;
	}

	/*
	** Read the previous dump page.
	*/
	j->dm0d_sequence--;
	if (j->dm0d_sequence < 0)
	{
	    uleFormat(NULL, E_DM0055_NONEXT, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    status = FAIL;
	    break;
	}

	status = JFread(&j->dm0d_jfio, j->dm0d_page, 
                        j->dm0d_sequence, &sys_err);
	if (status != OK)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    break;
	}

	j->dm0d_bytes_left 
             = ((DM0D_HEADER*)j->dm0d_page)->dh_offset - sizeof(DM0D_HEADER);

	j->dm0d_next_recnum = count_recs(j->dm0d_page);
	j->dm0d_next_byte = get_recptr(j->dm0d_page, j->dm0d_next_recnum);
    }

    if (status != OK)
    {
	return (log_error(j, E_DM9356_BAD_DMP_READ, (CL_ERR_DESC*)NULL, dberr));
    }

    /*
    ** Now prepend the front of the dump record to the dump buffer
    ** and return a pointer to that buffer.
    */
    amount = ((DM0D_HEADER*)j->dm0d_page)->dh_offset - 
					(j->dm0d_next_byte - j->dm0d_page);
    buf_ptr -= amount;
    MEcopy((PTR)j->dm0d_next_byte, amount, (PTR)buf_ptr);

    j->dm0d_bytes_left -= amount;
    amount_copied += amount;

    /*
    ** Extract the length and compare it against what we copied into the
    ** dump buffer.
    */
    I4ASSIGN_MACRO((*j->dm0d_next_byte), length);
    if (length != amount_copied)
    {
	return (log_error(j, E_DM935A_BAD_DMP_LENGTH, NULL, dberr));
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
	MEcopy((PTR)buf_ptr, length, (PTR)j->dm0d_record);
	*record = (PTR) j->dm0d_record;
    }
#endif

    /*
    ** Position the next-record indexes to the previous record on the page.
    */
    j->dm0d_next_recnum--;
    j->dm0d_next_byte = get_recptr(j->dm0d_page, j->dm0d_next_recnum);

    return (E_DB_OK);
}

/*{
** Name: dm0d_read	- Read record from dump file.
**
** Description:
**      This routine returns a pointer to the next record in the
**	dump file.
**
** Inputs:
**      jctx                            The dump context.
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
**      06-jan-1989 (EdHsu)
**          Created for Terminator.
**	30-apr-1991 (bryanp)
**	    If CL call fails, log CL error status.
**	03-sep-1998 consi01 bug 91957 - INGSRV 463
**	    New version cloned from dm0j_read to support records spanning
**	    the block size.
**      10-May-1999 (stial01)
**          dm0d_read() Removed direction paramter, get it from DM0D_CTX.
[@history_template@]...
*/
DB_STATUS
dm0d_read(
    DM0D_CTX    *jctx,
    PTR		*record,
    i4	*l_record,
    DB_ERROR	*dberr )
{
    DM0D_CTX		*j = jctx;
    i4		length;
    i4		amount;
    char		*r;
    DB_STATUS		status;
    CL_ERR_DESC		sys_err;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (j->dm0d_direction == DM0D_BACKWARD)
    {
	status = dm0d_backread(j, record, l_record, dberr);
	return (status);
    }

    /*	Read next page if current is empty. */

    if (j->dm0d_bytes_left == 0)
    {
	j->dm0d_sequence++;
	status = JFread(&j->dm0d_jfio, j->dm0d_page, 
                        j->dm0d_sequence, &sys_err);
	if (status != OK)
	{
	    {
		SETDBERR(dberr, 0, E_DM0055_NONEXT);
		return (E_DB_ERROR);
	    }

	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			err_code, 0);

	    return (log_error(j, E_DM9356_BAD_DMP_READ, &sys_err, dberr));
	}
	j->dm0d_next_byte = j->dm0d_page + sizeof(DM0D_HEADER);
	j->dm0d_bytes_left 
          = ((DM0D_HEADER*)j->dm0d_page)->dh_offset - sizeof(DM0D_HEADER);
    }

    /*	Check that the length field in log record doesn't span pages. */

    if (j->dm0d_bytes_left < sizeof(i4))
	return (log_error(j, E_DM9359_BAD_DMP_FORMAT, NULL, dberr));

    /*	Get the length of the next record. */

    I4ASSIGN_MACRO((*j->dm0d_next_byte), length);
    if (length > DM0D_MRZ)
    {
	return (log_error(j, E_DM935A_BAD_DMP_LENGTH, NULL, dberr));
    }

    /*
    ** If the next record is contained on the page, just return a pointer.
    ** If the record is unaligned on a BYTE_ALIGN machine or spans a page
    ** then copy the buffer to pre-allocated buffer, dm0j_record.
    **	(greg)
    */

    if ((length <= j->dm0d_bytes_left)
# ifdef	BYTE_ALIGN
        /* lint truncation warning if size of ptr > i4, but code valid */
	&&
	(((i4)j->dm0d_next_byte & (sizeof(ALIGN_RESTRICT) - 1)) == 0)
# endif
       )
    {
	*record = (PTR) j->dm0d_next_byte;
	j->dm0d_bytes_left -= length;
	j->dm0d_next_byte += length;
	*l_record = length;
	return (E_DB_OK);
    }

    /*	Record spans multiple pages, copy into preallocated buffer. */

    *l_record = length;
    *record = (PTR) j->dm0d_record;
    r = j->dm0d_record;
    while (length)
    {
	/*  Move part of the record to the record buffer. */

	amount = length;
	if (amount > j->dm0d_bytes_left)
	    amount = j->dm0d_bytes_left;
	MEcopy(j->dm0d_next_byte, amount, r);
	length -= amount;
	j->dm0d_bytes_left -= amount;
	j->dm0d_next_byte += amount;
	r += amount;
	if (length == 0)
	    return (E_DB_OK);

	/*	Read the next block. */

	j->dm0d_sequence++;
	status = JFread(&j->dm0d_jfio, j->dm0d_page, 
                        j->dm0d_sequence, &sys_err);
	if (status != OK)
	    break;
	j->dm0d_next_byte = j->dm0d_page + sizeof(DM0D_HEADER);
	j->dm0d_bytes_left 
             = ((DM0D_HEADER*)j->dm0d_page)->dh_offset - sizeof(DM0D_HEADER);
    }

    if (status == JF_END_FILE)
    {
	SETDBERR(dberr, 0, E_DM0055_NONEXT);
	return (E_DB_ERROR);
    }

    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char * )0, 0L, (i4 *)NULL, 
			err_code, 0);

    return (log_error(j, E_DM9356_BAD_DMP_READ, &sys_err, dberr));

}

/*{
** Name: dm0d_write	- Write record to dump file.
**
** Description:
**      This routine formats a record into the dump page buffer.
**
** Inputs:
**      jctx                            The dump context.
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
**      06-jan-1989 (EdHsu)
**          Created for Terminator.
**	30-apr-1991 (bryanp)
**	    If CL call fails, log CL error status.
**      06-mar-1996 (stial01)
**          Variable Page Size Project:
**              Length in log records is now 4 bytes.
**	03-sep-1998 consi01 bug 91957 problem INGSRV 463
**	    Restore code to split large record over blocks, this brings
**	    dm0d_write into line with dm0j_write.
**      10-May-1999 (stial01)
**          dm0d_write() Set dh->dh_first_rec in the DM0D_HEADER
[@history_template@]...
*/
DB_STATUS
dm0d_write(
    DM0D_CTX    *jctx,
    PTR	        record,
    i4     l_record,
    DB_ERROR	*dberr )
{
    DM0D_CTX		*j = jctx;
    DM0D_HEADER		*dh = (DM0D_HEADER*) j->dm0d_page;
    DB_STATUS		status;
    CL_ERR_DESC		sys_err;
    bool		first_record;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** If this is the first record which starts on this journal block
    ** then update the first_rec pointer to point to the start of this
    ** record.  (Make sure that the start of this record will actually
    ** be written on this block - if there is less than 4 bytes available
    ** then we will skip to the next block).
    */
    if (j->dm0d_bytes_left >= sizeof(i4))
    {
	if (dh->dh_first_rec == 0)
	{
	    /*
	    ** we can fit at least the length portion of
	    ** this record on this page and it's the first 
	    */
	    dh->dh_first_rec = (char *)j->dm0d_next_byte - (char *)j->dm0d_page;
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
	if (j->dm0d_bytes_left > l_record) 
	{
	    MEcopy(record, l_record, j->dm0d_next_byte);
	    j->dm0d_next_byte += l_record;
	    j->dm0d_bytes_left -= l_record;
	    return (E_DB_OK);
	}


	if (j->dm0d_bytes_left >= sizeof(i4))
	{
	    MEcopy(record, j->dm0d_bytes_left, j->dm0d_next_byte);
	    l_record -= j->dm0d_bytes_left;
	    record += j->dm0d_bytes_left;
	    j->dm0d_bytes_left = 0;
	}

	dh->dh_offset = j->dm0d_bksz - j->dm0d_bytes_left;
    
	status = JFwrite(&j->dm0d_jfio, (PTR)dh, j->dm0d_sequence, &sys_err);
	if (status != OK)
	    break;

	j->dm0d_next_byte = (char *)dh + sizeof(DM0D_HEADER);
	j->dm0d_bytes_left = j->dm0d_bksz - sizeof(DM0D_HEADER);

	/*  Initialize the Dump Page header. */

	dh->dh_seq = ++j->dm0d_sequence;
	if (first_record)
	{
	    first_record = FALSE;
	    dh->dh_first_rec = (char *)j->dm0d_next_byte - (char *)j->dm0d_page;
	}
	else
	{
	    dh->dh_first_rec = 0;
	}
	
    }

    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char * )0, 0L, (i4 *)NULL, 
			err_code, 0);

    return (log_error(j, E_DM9357_BAD_DMP_WRITE, &sys_err, dberr));
}

/*{
** Name: dm0d_update	- Update dump file.
**
** Description:
**      This routine updates the dump file.
**
** Inputs:
**      jctx                            The dump context.
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
**      06-jan-1989 (EdHsu)
**          Created for Terminator.
**	30-apr-1991 (bryanp)
**	    If CL call fails, log CL error status.
[@history_template@]...
*/
DB_STATUS
dm0d_update(
    DM0D_CTX    *jctx,
    i4    	*sequence,
    DB_ERROR	*dberr )
{
    DM0D_CTX		*j = jctx;
    DM0D_HEADER		*dh = (DM0D_HEADER*)j->dm0d_page;
    DB_STATUS		status = OK;
    CL_ERR_DESC		sys_err;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*	Check for empty next block. */

    if (j->dm0d_bytes_left != j->dm0d_bksz - sizeof(DM0D_HEADER))
    {
	/*	Update offset on page. */

	dh->dh_offset = j->dm0d_bksz - j->dm0d_bytes_left;

	/*	Write the last block of the file, and update the file header. */

	status = JFwrite(&j->dm0d_jfio, j->dm0d_page, 
                         j->dm0d_sequence, &sys_err); 
    }
    else
	j->dm0d_sequence--;

    if (status == OK)
	status = JFupdate(&j->dm0d_jfio, &sys_err);
    if (status == OK)
    {
	*sequence = j->dm0d_sequence;
	if (j->dm0d_sequence == 0)
	    j->dm0d_sequence = 1;
	return (E_DB_OK);
    }

    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char * )0, 0L, (i4 *)NULL, 
			err_code, 0);

    return (log_error(j, E_DM9358_BAD_DMP_UPDATE, &sys_err, dberr));
}

/*{
** Name: dm0d_truncate	- Truncate dump file.
**
** Description:
**      This routine truncates the dump files for the current
**	checkpoint series.
**
** Inputs:
**      jctx                            The dump context.
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
**      06-jan-1989 (EdHsu)
**          Created for Terminator.
**	30-apr-1991 (bryanp)
**	    If CL call fails, log CL error status.
[@history_template@]...
*/
DB_STATUS
dm0d_truncate(
    DM0D_CTX    *jctx,
    DB_ERROR	*dberr )
{
    DM0D_CTX		*j = jctx;
    DB_STATUS		status = OK;
    CL_ERR_DESC		sys_err;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    status = JFtruncate(&j->dm0d_jfio, &sys_err);
    if (status == OK)
	return (E_DB_OK);

    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char * )0, 0L, (i4 *)NULL, 
			err_code, 0);

    return (log_error(j, E_DM935D_BAD_DMP_TRUNCATE, &sys_err, dberr));
}

/*{
** Name: LogErrorFcn	- DM0D error logging routine.
**
** Description:
**      This routine is called to log all DM0D errors that occur.
**	The DM0D error is translated to a generic dump error.
** Inputs:
**      jctx                            Dump context.
**	dmf_error			DMF error code to log.
**	sys_err				Pointer to CL error.
**
** Outputs:
**      err_code			Pointer to resulting DMF error.
**	Returns:
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      06-jan-1989 (EdHsu)
**          Created for Terminator.
**	10-dec-1989 (rogerk)
**	    Pass error parameter count to ule_format.
**	18-Nov-2008 (jonj)
**	    Renamed to LogErrorFcn, invoked by log_error macro 
**	    with source information.
[@history_template@]...
*/
static DB_STATUS
logErrorFcn(
    DM0D_CTX        *jctx,
    i4	    dmf_error,
    CL_ERR_DESC	    *sys_err,
    DB_ERROR	    *dberr,
    PTR		    FileName,
    i4		    LineNumber)
{
    DM0D_CTX		*j = jctx;
    i4		i;
    i4			local_err;
    const static	struct _map_error
	{
	    i4	    error_dm0d;
	    i4	    error_dmp;
	}   map_error[] =
	{
	    { E_DM9360_DM0D_OPEN_ERROR, E_DM9355_BAD_DMP_OPEN },
	    { E_DM935E_DM0D_CLOSE_ERROR, E_DM9353_BAD_DMP_CLOSE },
	    { E_DM9361_DM0D_READ_ERROR, E_DM9356_BAD_DMP_READ },
	    { E_DM9362_DM0D_WRITE_ERROR, E_DM9357_BAD_DMP_WRITE },
	    { E_DM9363_DM0D_UPDATE_ERROR, E_DM9358_BAD_DMP_UPDATE },
	    { E_DM9364_DM0D_FORMAT_ERROR, E_DM9359_BAD_DMP_FORMAT },
	    { E_DM9365_DM0D_LENGTH_ERROR, E_DM935A_BAD_DMP_LENGTH },
	    { E_DM935F_DM0D_CREATE_ERROR, E_DM9354_BAD_DMP_CREATE},
	};

    /* Populate DB_ERROR with source information */
    dberr->err_code = dmf_error;
    dberr->err_data = 0;
    dberr->err_file = FileName;
    dberr->err_line = LineNumber;

    /*	Log the error with parameters. */

    if (j)
    {
	uleFormat(dberr, 0, sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 3,
		    sizeof(DB_DB_NAME), &j->dm0d_dbname,
		    0, j->dm0d_dmp_seq, 0, j->dm0d_sequence);
    }

    /*	Map from the logging error to the return error. */

    for (i = 0; i < sizeof(map_error)/sizeof(struct _map_error); i++)
    {
	if (map_error[i].error_dm0d == dmf_error)
	{
	    dberr->err_code = map_error[i].error_dmp;
	    break;
	}
    }

    return (E_DB_ERROR);
}

/*{
** Name: get_next_dmp - Open or Create and open the next dump.
**
** Description:
**	This routine is a utility routine for the dm0d_merge
**      procedure.  This routine Creates and open a new dump 
**      file during the merge phase.
**
**	This routine is NOT currently used, as no Cluster version of Online
**	Checkpoint exists. If a cluster version is ever written, this routine
**	should be re-examined to ensure that it is valid.
**
** Inputs:
**      cnf                             Configuration file pointer.
**      dmp                             Dump file io descriptor.
**	dmp_history_no			Index into the config file's dump
**					history array indicating which dump
**					history entry is being merged.
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
**      06-jan-1989 (EdHsu)
**          Created for Terminator.
**	4-may-90 (bryanp)
**	    Added dmp_history_no parameter in case we ever revive this routine
**	    and use it.
[@history_template@]...
*/
static DB_STATUS
get_next_dmp(
    DM0C_CNF    *cnf,
    DM0D_CTX    **cur_dmp,
    i4	dmp_history_no,
    DB_ERROR	*dberr )
{
    DM0C_CNF	    *config = cnf;
    DB_STATUS	    status;
    CL_ERR_DESC	    sys_err;
    i4	    fil_seq;
    i4	    blk_seq;
    i4	    length;
    DM_PATH         *dpath;
    i4              l_dpath;
    i4         i;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

	/*  Find dump location. */

	for (i = 0; i < config->cnf_dsc->dsc_ext_count; i++)
	{
	    if (config->cnf_ext[i].ext_location.flags & DCB_DUMP)
		break;
	}

	l_dpath = config->cnf_ext[i].ext_location.phys_length;
	dpath = &config->cnf_ext[i].ext_location.physical;

	
	/* Create the dump file. */	    	


	/* Update the last dump file for this 
	** checkpoint, it it is the first, set it also.
	**
	** In the case of recovery from last valid checkpoint, we may need
	** to be able to merge dump entries in histories other than the
	** last history, so use 'dmp_history_no' as the index of the history
	** entry to use.
	**
	** i = config->cnf_dmp->dmp_count -1;
	*/

	i = dmp_history_no;

	if (config->cnf_dmp->dmp_history[i].ckp_f_dmp == 0)
	{
	    
	    config->cnf_dmp->dmp_history[i].ckp_f_dmp = 
		    config->cnf_dmp->dmp_fil_seq;
	    
	    config->cnf_dmp->dmp_history[i].ckp_l_dmp = 
			config->cnf_dmp->dmp_fil_seq;
	}
    	fil_seq = config->cnf_dmp->dmp_history[i].ckp_l_dmp;
	TRdisplay("%@ CPP: Create new dump file sequence number %d.\n",
		config->cnf_dmp->dmp_fil_seq);
	status = dm0d_create(DM0D_MERGE, &dpath->name[0], l_dpath,
		fil_seq,
		config->cnf_dmp->dmp_bksz, config->cnf_dmp->dmp_blkcnt, 
                -1, dberr);

	/*  Handle error creating dump file. */

	if (status != OK)
	{
	    log_error(NULL, E_DM9354_BAD_DMP_CREATE, &sys_err, dberr);
	    return(E_DB_ERROR);
	}


	status = dm0d_open(DM0D_MERGE, &dpath->name[0], l_dpath, 
		    &config->cnf_dsc->dsc_name, config->cnf_dmp->dmp_bksz,
		    fil_seq, config->cnf_dmp->dmp_ckp_seq,
		    1, DM0D_M_WRITE, -1, DM0D_FORWARD, cur_dmp, dberr);
	if (status != E_DB_OK)
	{
	    log_error(0, E_DM9355_BAD_DMP_OPEN, &sys_err, dberr);
	    return(E_DB_ERROR);
	}


	return (E_DB_OK);
}

/*{
** Name: store_files	- Stores all valid cluster dump file names.
**
** Description:
**      This routine is called to store all valid cluster dump file
**      names in the file_array. 
**	This routine will only keep the following
**	file names:
**		Dnnnnnnn.Nxx   Cluster dump files.
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
**      06-jan-1989 (EdHsu)
**          Created for Terminator.
[@history_template@]...
*/
static DB_STATUS
store_files(
    DM0D_CS     *cs,
    char	*filename,
    i4    	l_filename,
    CL_ERR_DESC	*sys_err )
{
    DB_STATUS	    status;
    DB_STATUS	    *err_code;
    JFIO	    jf_file;
    i4         number, node;
    i4		j, store_index;
    i4		src, dst;
        
    if (l_filename == sizeof(DM_FILE) &&
	filename[8] == '.' &&
	filename[0] == 'd' &&
        filename[9] == 'n' )

    {
	TRdisplay("%@ DM0D_MERGE: Storing file %t\n",
	    l_filename, filename);

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
	    return (OK);	

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
				    sizeof(DM0D_CFILE),
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
** Name: count_files	- Counts all valid cluster dump file names.
**
** Description:
**      This routine is called to count all valid cluster dump file
**      names in the file_array. 
**	This routine will only count the following
**	file names:
**		Dnnnnnnn.Nxx   Cluster dump files.
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
**      06-jan-1989 (EdHsu)
**          Created for Terminator.
[@history_template@]...
*/
static DB_STATUS
count_files(
    i4     *count,
    char	*filename,
    i4    	l_filename,
    CL_ERR_DESC	*sys_err )
{
    DB_STATUS	    status;
    DB_STATUS	    *err_code;
    JFIO	    jf_file;
    i4         number, node;
        
    if (l_filename == sizeof(DM_FILE) &&
	filename[8] == '.' &&
	filename[0] == 'd' &&
        filename[9] == 'n' )

    {
	(*count)++;
    }
    return (OK);
}

/*{
** Name: dm0d_config_restore - copy config file from dump directory.
**
** Description:
**	This routine copies the saved config file in the dump directory
**	to the database directory.
**
**	This is done during rollforward from an online checkpoint just
**	after the database is restored from the checkpoint saveset.  The
**	dump version of the config file is used since it is guaranteed to
**	be in a consistent state.
**
**	As of May, 1990, we use this routine for both online AND offline
**	checkpoints. This is because the config file which is in the dump
**	directory contains the information about the extra (non-default)
**	locations for this database -- this information is used during the
**	restore process in order to support simultaneous restoring of multiple
**	locations. Thus we restore the config file from the dump directory
**	BEFORE the database is restored, use the information in the config
**	file to perform the restore, then re-restore the config file from
**	the dump directory. And we do this for ALL checkpoints.
**
**	At the time that this routine is invoked, there may not be a config
**	file in the database directory. In fact, unless this is a restart of
**	a failed rollforward, there typically is NOT a config file in the
**	database directory. Therefore, we do NOT treat it as an error if the
**	database directory's config file is not found. Instead, we create a
**	new config file.
**
**	Typically, the DM0C_DUMP flag is passed to this routine, indicating
**	that the Cnnnnnnn.dmp version of the file is wanted. Restoring from
**	the aaaaaaaa.cnf version of the file is only done during recovery from
**	a complete loss of the database directory.
**
** Inputs:
**	dcb				Database control block.
**	flags				flags to control operation:
**	    DM0C_DUMP			Restore the Cnnnnnnn.DMP version of the
**					config file.
**	ckp_seq				Checkpoint sequence number.
**
** Outputs:
**	err_code			Filled in on error.
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
**       6-dec-1989 (rogerk)
**          Created for Terminator.
**	4-may-90 (bryanp)
**	    If config file is not found in database directory, create new one.
**	    Removed 'cnf' argument (unused), and added 'flags' argument. Added
**	    code to restore the config file from either the dump copy or the
**	    up-to-date copy, depending on the flags value.
**	30-apr-1991 (bryanp)
**	    If CL call fails, log CL error status.
**	18-apr-1994 (jnash)
**          fsync project.  Remove DIforce() calls.
[@history_template@]...
*/
DB_STATUS
dm0d_config_restore(
    DMP_DCB     *dcb,
    i4    	flags,
    i4    	ckp_seq,
    DB_ERROR	*dberr )
{
    DB_STATUS	    return_status = E_DB_ERROR;
    STATUS	    status;
    DI_IO	    di_file1, di_file2;
    CL_ERR_DESC	    sys_err;
    i4	    cnf_pages, dmp_pages;
    i4		    n, i;
    i4		    page;
    char	    cnf_data[DM_PG_SIZE];
    char	    *filename;
    char	    file_buf[16];
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    for (;;)
    {
	/*
	** Get config file name using dump sequence number.The dump (checkpoint)
	** copy is named 'cNNNNNNN.dmp' - where NNN is derived from the
	** dump sequence number. The standard copy is just named aaaaaaaa.cnf
	*/
	if (flags & DM0C_DUMP)
	{
	    filename = file_buf;
	    MEfill(sizeof(file_buf), '\0', (PTR)file_buf);
	    dm0c_dmp_filename(ckp_seq, file_buf);
	}
	else
	{
	    filename = "aaaaaaaa.cnf";
	}

	/*
	** Open the dump config file.
	*/
	status = DIopen(&di_file1, dcb->dcb_dmp->physical.name,
		    dcb->dcb_dmp->phys_length,
		    filename, (u_i4)STlength(filename),
		    (i4)DM_PG_SIZE, DI_IO_WRITE, DI_SYNC_MASK, &sys_err);
	if (status != OK)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char * )0, 0L, (i4 *)NULL, 
			err_code, 0);

	    uleFormat(NULL, E_DM9004_BAD_FILE_OPEN, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 4,
		    sizeof(DB_DB_NAME), dcb->dcb_name, 11, "NOT_A_TABLE",
		    dcb->dcb_dmp->phys_length, dcb->dcb_dmp->physical.name,
		    (i4)STlength(filename), filename);
	    if (status == DI_FILENOTFOUND)
	    {
		uleFormat(NULL, E_DM9393_DM0D_NO_DUMP_CONFIG, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    }
	    break;
	}

	/*
	** Check the size of the file.
	** Increment by one since DIsense returns the last page number.
	*/
	status = DIsense(&di_file1, &dmp_pages, &sys_err);
	if (status != OK)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			err_code, 0);

	    uleFormat(NULL, E_DM9007_BAD_FILE_SENSE, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 4,
		    sizeof(DB_DB_NAME), dcb->dcb_name, 11, "NOT_A_TABLE",
		    dcb->dcb_dmp->phys_length, dcb->dcb_dmp->physical.name,
		    (i4)STlength(filename), filename);
	    break;
	}
	dmp_pages++;

	/*
	** Open the database config file. If the file is not found, create
	** it. (This is not an error because RollForward has probably just
	** finished renaming the old aaaaaaaa.cnf file to aaaaaaaa.rfc in
	** order to preserve it).
	*/
	status = DIopen(&di_file2, dcb->dcb_location.physical.name, 
		dcb->dcb_location.phys_length, "aaaaaaaa.cnf", (i4)12, 
		(i4)DM_PG_SIZE, DI_IO_WRITE, DI_SYNC_MASK, &sys_err);
	if (status != OK)
	{
	    if (status == DI_FILENOTFOUND)
	    {
		status = DIcreate(&di_file2, dcb->dcb_location.physical.name,
			dcb->dcb_location.phys_length,
			"aaaaaaaa.cnf", (i4)12,
			(i4)DM_PG_SIZE, &sys_err);

		if (status != OK)
		{
		    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			err_code, 0);

		    uleFormat(NULL, E_DM9002_BAD_FILE_CREATE, &sys_err, 
			    ULE_LOG, NULL,
			    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 4,
			    sizeof(DB_DB_NAME), dcb->dcb_name,
			    11, "NOT_A_TABLE",
			    dcb->dcb_location.phys_length,
			    dcb->dcb_location.physical.name, 
			    (i4)12, "aaaaaaaa.cnf");
		    break;
		}

		status = DIopen(&di_file2, dcb->dcb_location.physical.name, 
			dcb->dcb_location.phys_length,
			"aaaaaaaa.cnf", (i4)12, 
			(i4)DM_PG_SIZE, DI_IO_WRITE,
			DI_SYNC_MASK, &sys_err);

	    }
	    if (status != OK)
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			err_code, 0);

		uleFormat(NULL, E_DM9004_BAD_FILE_OPEN, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, err_code, 4,
			sizeof(DB_DB_NAME), dcb->dcb_name, 11, "NOT_A_TABLE",
			dcb->dcb_location.phys_length,
			dcb->dcb_location.physical.name, 
			(i4)12, "aaaaaaaa.cnf");
		break;
	    }
	}

	/*
	** Make sure there is space in the database config file.
	*/
	status = DIsense(&di_file2, &cnf_pages, &sys_err);
	if (status != OK)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			err_code, 0);

	    uleFormat(NULL, E_DM9007_BAD_FILE_SENSE, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 4,
		    sizeof(DB_DB_NAME), dcb->dcb_name, 11, "NOT_A_TABLE",
		    dcb->dcb_location.phys_length,
		    dcb->dcb_location.physical.name, 
		    (i4)12, "aaaaaaaa.cnf");
	    break;
	}
	cnf_pages++;

	if (dmp_pages > cnf_pages)
	{
	    status = DIalloc(&di_file2, (i4)(dmp_pages - cnf_pages), &page,
		&sys_err);
	    if (status != OK || page != cnf_pages)
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			err_code, 0);

		uleFormat(NULL, E_DM9000_BAD_FILE_ALLOCATE, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, err_code, 4,
			sizeof(DB_DB_NAME), dcb->dcb_name, 11, "NOT_A_TABLE",
			dcb->dcb_location.phys_length,
			dcb->dcb_location.physical.name, 
			(i4)12, "aaaaaaaa.cnf");
		break;
	    }
	    status = DIflush(&di_file2, &sys_err);
	    if (status != OK)
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			err_code, 0);

		uleFormat(NULL, E_DM9008_BAD_FILE_FLUSH, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, err_code, 4,
			sizeof(DB_DB_NAME), dcb->dcb_name, 11, "NOT_A_TABLE",
			dcb->dcb_location.phys_length,
			dcb->dcb_location.physical.name, 
			(i4)12, "aaaaaaaa.cnf");
		break;
	    }
	}

	/*
	** Copy the data from the dump version to the database config file.
	*/
	n = 1;
	for (i = 0; i < dmp_pages; i++)
	{
	    status = DIread(&di_file1, &n, i, cnf_data, &sys_err);
	    if (status != OK)
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			err_code, 0);

		uleFormat(NULL, E_DM9005_BAD_FILE_READ, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, err_code, 5,
			sizeof(DB_DB_NAME), dcb->dcb_name, 11, "NOT_A_TABLE",
			dcb->dcb_dmp->phys_length, dcb->dcb_dmp->physical.name,
			(i4)STlength(filename), filename, 0, i);
		break;
	    }

	    status = DIwrite(&di_file2, &n, i, cnf_data, &sys_err);
	    if (status != OK)
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			err_code, 0);

		uleFormat(NULL, E_DM9006_BAD_FILE_WRITE, &sys_err, ULE_LOG, NULL,
			(char *)NULL, (i4)0, (i4 *)NULL, err_code, 5,
			sizeof(DB_DB_NAME), dcb->dcb_name, 11, "NOT_A_TABLE",
			dcb->dcb_location.phys_length,
			dcb->dcb_location.physical.name, 
			(i4)12, "aaaaaaaa.cnf", 0 ,i);
		break;
	    }
	}
	if (status != E_DB_OK)
	    break;

        return_status = E_DB_OK;
        break;
    }

    status = DIclose(&di_file1, &sys_err);
    if (status != OK)
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			err_code, 0);

	uleFormat(NULL, E_DM9001_BAD_FILE_CLOSE, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 4,
		sizeof(DB_DB_NAME), dcb->dcb_name, 11, "NOT_A_TABLE",
		dcb->dcb_dmp->phys_length, dcb->dcb_dmp->physical.name,
		(i4)STlength(filename), filename);
	return_status = E_DB_ERROR;
    }
    status = DIclose(&di_file2, &sys_err);
    if (status != OK)
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			err_code, 0);

	uleFormat(NULL, E_DM9001_BAD_FILE_CLOSE, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 4,
		sizeof(DB_DB_NAME), dcb->dcb_name, 11, "NOT_A_TABLE",
		dcb->dcb_location.phys_length,
		dcb->dcb_location.physical.name, 
		(i4)12, "aaaaaaaa.cnf");
	return_status = E_DB_ERROR;
    }

    if (return_status != E_DB_OK)
	SETDBERR(dberr, 0, E_DM9392_DM0D_CONFIG_RESTORE);
    return (return_status);
}

/*{
** Name: dm0d_d_config - delete dump copy of the config file.
**
** Description:
**	This routine deletes the saved copy of the config file stored in the
**	dump directory by online checkpoint.
**
**	This is done during rollforward when the '-d' option is specified.
**
**	A warning (rather than an error) is returned of the config file
**	does not exist.
**
** Inputs:
**	dcb				Database control block.
**	cnf				Config file structure which was filled
**					in by a valid config file.
**	ckp_seq				Checkpoint sequence number.
**
** Outputs:
**	err_code			Filled in on error.
**	Returns:
**	    E_DB_OK
**	    E_DB_WARN
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**       6-dec-1989 (rogerk)
**          Created for Terminator.
**	30-apr-1991 (bryanp)
**	    If CL call fails, log CL error status.
[@history_template@]...
*/
DB_STATUS
dm0d_d_config(
    DMP_DCB   	*dcb,
    DM0C_CNF    *cnf,
    i4    	ckp_seq,
    DB_ERROR	*dberr )
{
    STATUS	    status;
    DI_IO	    di_file;
    CL_ERR_DESC	    sys_err;
    char	    filename[16];
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** Get config file name using dump sequence number.  The config
    ** copy is named 'cNNNNNNN.dmp' - where NNN is derived from the
    ** dump sequence number.
    */
    MEfill(sizeof(filename), '\0', (PTR)filename);
    dm0c_dmp_filename(ckp_seq, filename);

    /*
    ** Delete the dump config file.
    */
    status = DIdelete(&di_file, dcb->dcb_dmp->physical.name,
		dcb->dcb_dmp->phys_length,
		filename, (u_i4)STlength(filename), &sys_err);
    if (status != OK)
    {
	/*
	** If file doesn't exist, return a warning - this is not necessarily
	** an error.
	*/
	if (status == DI_FILENOTFOUND)
	    return (E_DB_WARN);

	uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, 
			err_code, 0);

	uleFormat(NULL, E_DM9003_BAD_FILE_DELETE, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 4,
		sizeof(DB_DB_NAME), dcb->dcb_name, 11, "NOT_A_TABLE",
		dcb->dcb_dmp->phys_length, dcb->dcb_dmp->physical.name,
		(i4)STlength(filename), filename);
	SETDBERR(dberr, 0, E_DM9394_DM0D_CONFIG_DELETE);
	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}

/*{
** Name: dm0d_filename  - Given a sequence number returns the dump filename.
**
** Description:
**
** Inputs:
**      flag;                           Must be zero or DM0D_MERGE
**                                      if you do not want the cluster name.
**      dmp_seq                         Dump file sequence number.
**      nodeid                          Only valid of flag = DM0D_MERGE.
**                                      If value = -1, nodeid not used
**                                      else node_id is used to create name.
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
**      14-sep-1994 (andyw)
**          Partial backup/Recovery Project:
**              Created.
*/
VOID
dm0d_filename(
i4             flag,
i4             dmp_seq,
i4             node_id,
DM_FILE             *filename)
{
    static char 	basename[] = "d0000000.dmp";
    static char		digitlookup[] = { '0', '1', '2', '3', '4', '5', 
			    '6', '7', '8', '9' };
    i4                  i, number;
 
    MEcopy( basename, 12, filename->name );
    number = dmp_seq;
    for ( i = 7; number > 0 && i > 0; i-- )
    {
	filename->name[i] = digitlookup[number % 10];
	number = number / 10;
    }

    flag &= DM0D_MERGE;
    if ( ( flag && (node_id > 0) ) || ( !flag && CXcluster_enabled() ) )
    {
        filename->name[9] = 'n';
        filename->name[10] = digitlookup[(node_id / 10) % 10];
        filename->name[11] = digitlookup[node_id % 10];
 
    }
}
/*{
** Name: get_recptr     - Get a pointer to an arbitrary dump record.
**
** Description:
**      This routine takes a dump page and returns a pointer to the nth
**      dump record on that page.
**
** Inputs:
**      page                            The dump page.
**      recnum                          The index of the record desired.
**
** Outputs:
**      Returns:
**          A character pointer to the requested record.
**
** Side Effects:
**          none
**
** History:
**	03-sep-1998 consi01 bug 91957 problem INGSRV 463
**	    Creation - base on version in dm0j.c
[@history_template@]...
*/
static char *
get_recptr(
char	   *page,
i4	   recnum)
{
    DM0D_HEADER		*dh;
    char		*rec_ptr;
    i4             length;
    i4		count;

    if (recnum == 0)
	return (page + sizeof(DM0D_HEADER));

    dh = (DM0D_HEADER *) page;

    rec_ptr = (char *)page + dh->dh_first_rec;

    for (count = 1; count < recnum; count++)
    {
	I4ASSIGN_MACRO((*rec_ptr), length);
	rec_ptr += length;
    }

    return (rec_ptr);
}
/*{
** Name: count_recs     - Count the number of dump records on a dump page.
**
** Description:
**      This routine takes a dump page and returns the number of dump
**      records that are either contained on the page or started on the page
**      and then overflow onto the next page.
**
**      Records which started on a previous page and overflow onto this one
**      are not counted.
**
** Inputs:
**      page                            The dump page.
**
** Outputs:
**      Returns:
**          The number of records on the page.
**
** Side Effects:
**          none
**
** History:
**	03-sep-1998 consi01 bug 91957 problem INGSRV 463
**	    Creation - based on version in dm0j.c
[@history_template@]...
*/
static i4
count_recs(
char	   *page)
{
    DM0D_HEADER		*dh;
    char		*rec_ptr;
    i4             length;
    i4		count = 0;

    dh = (DM0D_HEADER *) page;

    /*
    ** The dh_first_rec header value gives the offset to the start of the
    ** first record on the page.  If it is zero, there are no records which
    ** which begin on this page.
    */
    if (dh->dh_first_rec == 0)
	return (0);

    rec_ptr = (char *)page + dh->dh_first_rec;
    while (rec_ptr < ((char *)page + dh->dh_offset))
    {
	count++;

	I4ASSIGN_MACRO((*rec_ptr), length);
	rec_ptr += length;
    }

    return (count);
}


/*{
** Name: dm0d_merge	- Merge dumps node dumps into standard ones.
**
** Description:
**	This routine merges dump files created by each node into 
**      standard dump files.
**
**	Each dump record contains a Log Sequence Number, which was generated
**	when the original log record was written. The node-specific dump
**	files are merged according to this Log Sequence Number.
**
**	The node dump files are named Dnnnnnnn.Nxx and the standard ones
**	Dnnnnnnn.dmp,  Where nnnnnnn is the dump sequence number starting
**	with 1 and xx is a cluster id with values of 1-16.  Therefore node 3
**	could create the first node dump with the name D0000001.N03 and node
**	15 could create the next one with the name D0000002.N15.  At checkpoint
**	time these would be merged into D0000001.DMP.
**
**      If there are no node dump files, a standard dump file will not be 
**      created. (Checkpoints do not have empty dump files)
**
** Inputs:
**      cnf                             Configuration file pointer.
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
** 	01-sep-2004 (stial01)
**          Created from dm0j_merge in dm0j.c
**      25-Apr-2007 (kibro01) b115996
**          Ensure the misc_data pointer in a MISC_CB is set correctly and
**          that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**          since that's what we're actually using (the type is MISC_CB)
** 	03-jun-2008 (stial01)
**	    Updated to support cluster online ckpdb.
*/
DB_STATUS
dm0d_merge(
    DM0C_CNF	*cnf,
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
    DM_PATH         *dpath;
    i4              l_dpath;
    DM0D_CTX        *cur_dmp = 0;   /* init, wont get set if auditdb.     */
    JFIO	    listfile_jfio;
    i4         first_file; 
    i4	    last_file;
    DM0D_CNODE      node_array[DM_CNODE_MAX];
    DM0D_CS         cs;
    PTR             record; 
    i4         l_record;
    DM0L_HEADER	    *h;
    DMP_MISC        *file_cb;
    DM0D_CFILE      *file_array;
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
    DB_ERROR		local_dberr;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*  Find dump location. */
    for (i = 0; i < config->cnf_dsc->dsc_ext_count; i++)
    {
	if (config->cnf_ext[i].ext_location.flags & DCB_DUMP)
	    break;
    }

    l_dpath = config->cnf_ext[i].ext_location.phys_length;
    dpath = &config->cnf_ext[i].ext_location.physical;
	
    i = config->cnf_dmp->dmp_count - 1;
    fil_seq = config->cnf_dmp->dmp_history[i].ckp_l_dmp;
    first_file = config->cnf_dmp->dmp_history[i].ckp_l_dmp;
    last_file = config->cnf_dmp->dmp_fil_seq;
    sequence = config->cnf_dmp->dmp_blk_seq;

    /*
    ** Loop through the node section of config file, get 
    ** the node sequence numbers.  If they are all zero, no
    ** files have been created and there is nothing more to do. 
    */

    done = TRUE;
    for ( i = 0; i < DM_CNODE_MAX; i++ )
	if (config->cnf_dmp->dmp_node_info[i].cnode_fil_seq != 0) 
	{
	    done = FALSE;
	    break;
	}

    if (done)
    {
	status = dm0c_close(cnf, DM0C_LEAVE_OPEN, dberr);
	if (status != E_DB_OK)
	    return(status);
	return (E_DB_OK);
    }

    /* Create the file */
    created = FALSE;
    /* FIX ME 3rd param may not always be correct */
    status = get_next_dmp(cnf, &cur_dmp, config->cnf_dmp->dmp_count -1, dberr);
    if (status != E_DB_OK)
	return  (status);

    created = TRUE;

    for (;;)
    {
	/*
	** Allocate space for dump file record buffers (one per 
	** active node) 
	*/
	
	numnodes = 0;
	for ( i = 0; i < DM_CNODE_MAX; i++ )
	    if ((n = config->cnf_dmp->dmp_node_info[i].cnode_fil_seq) != 0)
	    {
		activenodes[numnodes++] = i;
	    }

	rec_array_size = numnodes * DM0D_MRZ;
	status = dm0m_allocate(rec_array_size + sizeof(DMP_MISC),
	    (i4)0, (i4)MISC_CB, (i4)MISC_ASCII_ID, (char *)NULL, 
	    (DM_OBJECT **)&rec_cb, dberr);
	if (status != OK)
	{
	    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM9329_DM0J_M_ALLOCATE);
	    break;
	}
	rec_cb->misc_data = (char*) (&rec_cb[1]);

	/*  Set up array of nodes, determine current file for each node. */

	k = 0;
	for ( i = 0; i < DM_CNODE_MAX; i++ )
	{
	    char 	*rec_adr;

	    node_array[i].count = 0;
	    node_array[i].state = DM0D_CINVALID;
	    node_array[i].cur_fil_seq = 0;
	    node_array[i].cur_dmp = 0;
	    if ((n = config->cnf_dmp->dmp_node_info[i].cnode_fil_seq) != 0) 
	    {
		node_array[i].state = DM0D_CVALID;
		node_array[i].last_fil_seq = n;	    
		node_array[i].lsn.lsn_high = -1;	
		node_array[i].lsn.lsn_low = -1;	
		rec_adr = (char *)(((char *)rec_cb + sizeof(DMP_MISC)) + 
			(k * DM0D_MRZ));
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
	status = JFlistfile(&listfile_jfio, dpath->name,
		l_dpath, count_files, (PTR)&file_count, &sys_err);
	if (status != JF_END_FILE)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)&sys_err, ULE_LOG , NULL, 
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM932A_DM0J_M_LIST_FILES);
	    break;
	}
	file_array_size = file_count * sizeof(DM0D_CFILE);
	status = dm0m_allocate(file_array_size + sizeof(DMP_MISC),
	    (i4)0, (i4)MISC_CB, (i4)MISC_ASCII_ID, (char *)NULL, 
	    (DM_OBJECT **)&file_cb, dberr);
	if (status != OK)
	{
	    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM9329_DM0J_M_ALLOCATE);
	    break;
	}
	file_array = (DM0D_CFILE *)((char *)file_cb + sizeof(DMP_MISC));
	file_cb->misc_data = (char*)file_array;
	cs.file_array = file_array;
	cs.index = 0;
	cs.first = first_file;
	cs.last = last_file;

	/* Get the list of files, fill in file array. */
	status = JFlistfile(&listfile_jfio, dpath->name,
		l_dpath, store_files, (PTR)&cs,
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
		if (node_array[node_id].state == DM0D_CINVALID)
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
	** Run through the node_array, open the current dump files 
	** and read the first record.  If valid, extract the LSN 
	** (the lowest for this node) and plug it in the node_array.  If 
	** there are no records in the dump, flag it DM0D_CINVALID.
	*/

	open_count     = 0;
	start_lsn.lsn_high = -1;
	start_lsn.lsn_low  = -1;
	lowest_index   = -1;

    	for ( node = 0; node < numnodes; node++ )
	{
	    i = activenodes[node];
	    if (node_array[i].state == DM0D_CVALID)
	    {
		done = FALSE;
		while ( ! done )
		{
		    status = dm0d_open(DM0D_MERGE, (char *)dpath, l_dpath, 
			&config->cnf_dsc->dsc_name, config->cnf_dmp->dmp_bksz,
			node_array[i].cur_fil_seq, config->cnf_dmp->dmp_ckp_seq,
			0, DM0D_M_READ, i, DM0D_FORWARD, 
			&node_array[i].cur_dmp, dberr);
		    if (status != E_DB_OK)
		    {
			log_error((DM0D_CTX *)NULL, E_DM9355_BAD_DMP_OPEN, 
				(CL_ERR_DESC *)NULL, dberr);
			break;
		    }

		    status = dm0d_read(node_array[i].cur_dmp, 
					&record, &l_record, dberr);
		    if (status != E_DB_OK)
		    {
			if (dberr->err_code == E_DM0055_NONEXT)
			{
			    /* Empty dump */
			    status = dm0d_close(&node_array[i].cur_dmp, 
                                                dberr);
			    if ( status != E_DB_OK )
				/* XXX new error XXX */
				TRdisplay(
				 "%@ DM0D_MERGE: dm0d_close failure ignored\n");

			    count--;
			    if (--node_array[i].count == 0)
			    {
				/* Finished with this node. */
				node_array[i].state = DM0D_CINVALID;
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
	** Now we can merge.  Examine each record from each dump, 
	** write to the merged dump in LSN order.  The first record 
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
                    if (node_array[k].state != DM0D_CVALID)
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
		   ("DM0D_WRITE: Write file %d, lsn_low %d to merged dump\n",
			lowest_lsn, node_array[lowest_index].lsn.lsn_low);
#endif

		/*
		** If we are to hand this record to a processing routine, do
		** so. Otherwise, write the current lowest to the merged
		** dump, read the next record from this file
		*/
		status = dm0d_write(cur_dmp,
					    node_array[lowest_index].record,
					    node_array[lowest_index].l_record,
					    dberr);
		if (status != E_DB_OK)
		{
		    log_error(cur_dmp, E_DM9284_DM0J_WRITE_ERROR, 
				(CL_ERR_DESC *)NULL, dberr);
		    break;
		}

		status = dm0d_read(node_array[lowest_index].cur_dmp, 
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
		    log_error(node_array[lowest_index].cur_dmp, 
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
		** (Add Code here when we want to create new dump
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
		    status = dm0d_close(&node_array[lowest_index].cur_dmp, 
                                                dberr);
		    node_array[lowest_index].cur_dmp = 0;
		    if (--count == 0)
		    {
			/* No more nodes */
			done = TRUE;
			break;
		    }	

		    if (--node_array[lowest_index].count == 0)
		    {
			/* Finished with this node. */
			node_array[lowest_index].state = DM0D_CINVALID;
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
		    
			status = dm0d_open(DM0D_MERGE, (char *)dpath, l_dpath, 
			    &config->cnf_dsc->dsc_name, 
                            config->cnf_dmp->dmp_bksz,
			    node_array[lowest_index].cur_fil_seq, 
			    config->cnf_dmp->dmp_ckp_seq,
			    0, DM0D_M_READ, lowest_index, DM0D_FORWARD,
                            &node_array[lowest_index].cur_dmp,
			    dberr);

			if (status != E_DB_OK)
			{
			    log_error((DM0D_CTX *)NULL, E_DM9355_BAD_DMP_OPEN, 
				(CL_ERR_DESC *)NULL, dberr);
			    break;
			}

			status = dm0d_read(node_array[lowest_index].cur_dmp, 
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
			    TRdisplay("%@ DM0D_MERGE: dm0d_read error: %d\n",
				dberr->err_code);
			    break;
			}
		    }   /* of next file for this node */
		}       /* ! next_found */
	    }           /* of eof processing */
	}               /* ! done */

	if (status != E_DB_OK)
	    break;

	/* All .Nxx files should be closed at this point. 
	** Now close the .DMP file update the config file, 
        ** then delete all the .nxx files. 
	*/

	/* XXXXX Add update of blk_seq for this dump. */

	/*  Update dump file. */

	status = dm0d_update(cur_dmp, &sequence, dberr);
	if (status != E_DB_OK)
	{
	    log_error(cur_dmp, E_DM9353_BAD_DMP_CLOSE, (CL_ERR_DESC *)NULL, 
		dberr);
	    break;
	}

	status = dm0d_close(&cur_dmp, dberr);
	if (status != E_DB_OK)
	{
	    log_error(cur_dmp, E_DM9353_BAD_DMP_CLOSE, (CL_ERR_DESC *)NULL, 
		dberr);
	    break;
	}

	/*
	** Update the current file and block sequence values for the
	** cluster merged dump files.
	*/
	i = config->cnf_dmp->dmp_count - 1;
	config->cnf_dmp->dmp_fil_seq 
                    = config->cnf_dmp->dmp_history[i].ckp_l_dmp;
	config->cnf_dmp->dmp_blk_seq = sequence;

	/*
	** Clear the current file and block sequence number for the
	** individual cluster node dump files since we just threw
	** them all away after merging.
	**
	** The dump log address value (cnode_la) is still needed
	** however.  This records the place in this node's log file to
	** which we have completed archiving log records for the database.
	** The next time archiving is needed from this node, we will start
	** processing at that spot.
	*/
	for ( i = 0; i < DM_CNODE_MAX; i++)
	{
	    config->cnf_dmp->dmp_node_info[i].cnode_fil_seq = 0;
	    config->cnf_dmp->dmp_node_info[i].cnode_blk_seq = 0;
	}	


   	status = dm0c_close(cnf, DM0C_UPDATE | DM0C_LEAVE_OPEN | DM0C_COPY,
   			    dberr);
	if (status != E_DB_OK)
	{
	    TRdisplay("%@ DM0D_MERGE: Configuration file close error\n");
	    break;
	}
	
	for ( i= 0; i < cs.index; i++)
	{
	    local_status = JFdelete(&listfile_jfio, dpath->name, l_dpath, 
                                file_array[i].filename.name, 
                                file_array[i].l_filename, 
				&sys_err);
	    if ( local_status != OK )
		TRdisplay("%@ DM0D_MERGE: JFdelete error on %s ignored\n", 
			file_array[i].filename);
	}

	return (E_DB_OK);	
	
    } 

    /* If here we had an error, close any open files. return 
    ** error. 
    */

    /* Close merged dump file, ignore errors, if any. */
    local_status = dm0d_close(&cur_dmp, &local_dberr);

    for (i = 0; i < DM_CNODE_MAX; i++)
    {
	if (node_array[i].cur_dmp != 0)
	    local_status = dm0d_close(&node_array[i].cur_dmp, &local_dberr);
    }
    if (created == TRUE)
	local_status = dm0d_delete(DM0D_MERGE, (char *)dpath, 
			   l_dpath, fil_seq, &local_dberr);
	
    return(status == E_DB_OK ? E_DB_ERROR : status);
    
}
