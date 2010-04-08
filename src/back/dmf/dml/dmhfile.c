/*
** Copyright (c) 1999, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <pc.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <sr.h>
#include    <ex.h>
#include    <dicl.h>
#include    <bt.h>
#include    <adf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmxcb.h>
#include    <dmtcb.h>
#include    <dmucb.h>
#include    <dmhcb.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dm2r.h>
#include    <dm2t.h>
#include    <dm2f.h>
#include    <dm1b.h>
#include    <dmse.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dmxe.h>
#include    <dm0m.h>

/**
**
**  Name: DMHFILE.C - Hash partition file management.
**
**  Description:
**      This file contains the routines that implement DMF hash partition
**	file access:
**
**          dmh_open - Create filename and open/create file.
**	    dmh_close - Close (and destroy) file.
**	    dmh_read - Read a block from file.
**	    dmh_write - Write a block to file.
**
**	These functions use the SR interface to the DI CL developed for
**	sort external files. The SR interface offers all the needed 
**	functionality for hash partitioning.
**
**
**  History:
**      12-apr-99 (inkdo01)
**	    Written.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
*/
 
/*
**  Definition of static variables and forward static functions.
*/


/** Name: dmh_open	- Create/open a file.
**
** Description:
**      This routine creates a filename with logic cloned from dm2f_filename,
**	based on a naming scheme based on a server-unique id number and an
**	incrementing hash file number.  Names thus generated are
**	guaranteed unique for the life of the server.  The naming scheme
**	theoretically allows for 37^10 / 512 files before it falls off the
**	end.  In order to use CSadjust_counter, the actual naming scheme
**	stops at max(u_i4) files, which is still enough files to generate
**	over 100 filenames per second for more than a year.
**
**	If DMH_CREATE is not specified, we don't do anything.
**	Hash files aren't closed, so re-opening them is a no-op.
**	
**	The file is placed on a work location determined by the hash
**	number, so that successive files should go on different locations.
**
** Inputs:
**      dmhcb				Pointer to the DMH_CB.
**		.dmh_file_context	pointer to SR file context block
**		.dmh_filename		pointer to filename area
**		.dmh_db_id		pointer to database description
**		.dmh_tran_id		pointer to transaction ID
**
** Outputs:
**		.dmh_file_context	pointer to init'ed SR file context
**		.dmh_filename		pointer to filename 
**		.dmh_locidxp		DMF location index stored thru here;
**					other DMH calls supply locidxp just
**					for nice error messages
**		.error			error code (if any)
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR			????
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	12-apr-99 (inkdo01)
**	    Written (for hash join project).
**	27-May-2005 (schka24)
**	    Error if retry loop runs out, and retry 1369 times (max allowed
**	    by current filename scheme).  Fix typo and be endian-neutral
**	    on 64-bit.  Extend filename scheme to better allow for really
**	    huge hash-joins by using radix 37.
**	3-Jun-2005 (schka24)
**	    Switch to new naming scheme with guaranteed unique names, so
**	    that we can delete files if they are leftover from prior
**	    server startups.
**	18-Jul-2005 (schka24)
**	    Don't reference DI_IO member before delete.  (Delete has been
**	    fixed to not reference it either.)
**	5-Aug-2009 (kschendel) SIR 122512
**	    Eliminate dmh_numlocs, not used.  Use non-truncating delete if
**	    (leftover) file is found.
**
*/
DB_STATUS
dmh_open(
DMH_CB		*dmhcb)

{

    /* Static arrays used to construct filenames. */

    /* 37 assuredly legal filename chars, even on VMS/DOS/etc. */
    static const char rad37[37] = {'a', 'b', 'c', 'd', 'e', 'f', 'g',
				   'h', 'i', 'j', 'k', 'l', 'm', 'n',
				   'o', 'p', 'q', 'r', 's', 't', 'u',
				   'v', 'w', 'x', 'y', 'z', '0', '1',
				   '2', '3', '4', '5', '6', '7', '8',
				   '9', '_'};

    DM_SVCB	*svcb = dmf_svcb;
    DML_XCB	*xcb = (DML_XCB *)dmhcb->dmh_tran_id;
    DML_ODCB	*odcb = (DML_ODCB *)dmhcb->dmh_db_id;
    DMP_LOC_ENTRY *locarray = odcb->odcb_dcb_ptr->dcb_ext->ext_entry;
    DML_LOC_MASKS *lm = xcb->xcb_scb_ptr->scb_loc_masks;
    char	*filename = &dmhcb->dmh_filename[0];
    i4		numlocs, fileloc;
    i4		toss_err;
    i4		i, extno;
    u_i4	unumber;
    STATUS	stat;
    i4		err_code;
    CL_ERR_DESC	sys_err;

    CLRDBERR(&dmhcb->error);


    if ((dmhcb->dmh_flags_mask & DMH_CREATE) == 0)
	return (E_DB_OK);		/* Non-create is noop */

    numlocs = BTcount((PTR)lm->locm_w_use, lm->locm_bits);

    /* The filename generator is a (server wide) file counter,
    ** expressed in radix 37.  (26 letters, 10 numbers, and underscore.)
    ** Unlike dm2f names, the server ID number goes into the extension,
    ** not the filename part.
    */

    unumber = (u_i4) CSadjust_counter((i4 *) &svcb->svcb_hfname_gen, 1);
    if (unumber == 0xffffffff)
    {
	/* Oops.  We ran off the end.  There's plenty of space left in the
	** filename, so if this happens, just add another generator, or
	** make hfname_gen an i8 and add a csadjust-counter-i8.
	*/
	uleFormat(NULL, E_DM92A3_NAME_GEN_OVF, NULL, ULE_LOG,
		NULL, NULL, 0, NULL, &toss_err, 0);
	EXsignal(EX_DMF_FATAL, 0);
    }
    fileloc = unumber % numlocs;

    /* Put the server ID in the extension, plenty of room there.
    ** The .hxx extension separates these from dm2f_filename names.
    */

    filename[8] = '.';
    filename[9] = 'h';
    extno = svcb->svcb_tfname_id;
    filename[11] = rad37[extno % 37]; extno /= 37;
    filename[10] = rad37[extno % 37]; ;

    /* Name counter goes into the filename part */
    filename[7] = rad37[unumber % 37]; unumber /= 37;
    filename[6] = rad37[unumber % 37]; unumber /= 37;
    filename[5] = rad37[unumber % 37]; unumber /= 37;
    filename[4] = rad37[unumber % 37]; unumber /= 37;
    filename[3] = rad37[unumber % 37]; unumber /= 37;
    filename[2] = rad37[unumber % 37]; unumber /= 37;
    filename[1] = rad37[unumber % 37]; unumber /= 37;
    filename[0] = rad37[unumber % 37];

    /* Target location is the fileloc'th work location. */
    i = -1;
    do {
	i = BTnext(i, (PTR)lm->locm_w_use, lm->locm_bits);
	if (i != -1)
	    --fileloc;
    } while (fileloc >= 0);
    *dmhcb->dmh_locidxp = (i2) i;	/* save back in caller */

    stat = SRopen((SR_IO *)dmhcb->dmh_file_context, 
		locarray[i].physical.name, locarray[i].phys_length,
		filename, 12, dmhcb->dmh_blksize, SR_IO_CREATE,
		1, &sys_err);
    if (stat == 0) return(E_DB_OK);

    if (stat == SR_BADCREATE || stat == SR_BADOPEN)
    {
	CL_ERR_DESC clerror;

	/* Delete the pre-existing file, must be a leftover */
	stat = DIdelete(NULL,
		locarray[i].physical.name, locarray[i].phys_length,
		filename, 12, &clerror);
	if (stat == OK)
	{
	    stat = SRopen((SR_IO *)dmhcb->dmh_file_context,
		locarray[i].physical.name, locarray[i].phys_length,
		filename, 12, dmhcb->dmh_blksize, SR_IO_CREATE,
		1, &sys_err);
	    if (stat == 0) return (E_DB_OK);
	}
    }

    /* Fatal error - return a message and exit. */
    uleFormat(&dmhcb->error, E_DM9711_SR_OPEN_ERROR, &sys_err, ULE_LOG,
		NULL, NULL, 0, NULL, &dmhcb->error.err_code, 2, 
		locarray[i].phys_length,
		locarray[i].physical.name,
		12, filename);
    if (stat == SR_EXCEED_LIMIT)
	SETDBERR(&dmhcb->error, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);

    return(E_DB_ERROR);

}


/** Name: dmh_write	- Write a block to a hash partition file.
**
** Description:
**      This routine writes a block to a specified RBN in a hash partition
**	file. 
**
** Inputs:
**      dmhcb				Pointer to the DMH_CB.
**		.dmh_file_context	pointer to SR file context block
**		.dmh_locidxp		Ptr to location index, for msgs
**		.dmh_rbn		RBN of block to be written
**		.dmh_buffer		pointer to buffer to be written
**		.dmh_blksize		size (in bytes) of block
**		.dmh_nblocks		Number of blocks to write
**
** Outputs:
**		.error			error code (if any)
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR			????
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	12-apr-99 (inkdo01)
**	    Written (for hash join project).
**	01-may-07 (hayke02)
**	    If SR_ENDFILE or SR_EXCEED_LIMIT is returned from SRwrite(), set
**	    stat to E_DB_ERROR. This will ensure that E_QE0018_BAD_PARAM_IN_CB
**	    is returned to the front end and errlog.log, as well as
**	    E_DM9713_SR_WRITE_ERROR to the errlog.log. Previously,
**	    SR_EXCEED_LIMIT would give E_QE0018, but SR_ENDFILE would give
**	    no error and 0 rows. This change fixes bug 117084.
**	6-Mar-2007 (kschendel) SIR 122512
**	    Allow multi-page writes.
**
*/
DB_STATUS
dmh_write(
DMH_CB		*dmhcb)

{

    CL_ERR_DESC	sys_err;
    STATUS	stat;

    CLRDBERR(&dmhcb->error);


    /* Simply pass on the write request. */

    stat = SRwriteN((SR_IO *)dmhcb->dmh_file_context, dmhcb->dmh_nblocks,
	dmhcb->dmh_rbn, dmhcb->dmh_buffer, &sys_err);

    if (stat)
    {
	DML_ODCB *odcb = (DML_ODCB *)dmhcb->dmh_db_id;
	DMP_LOC_ENTRY *locarray = odcb->odcb_dcb_ptr->dcb_ext->ext_entry;
	i4	locidx = *dmhcb->dmh_locidxp;

	uleFormat(&dmhcb->error, E_DM9713_SR_WRITE_ERROR, &sys_err, ULE_LOG, NULL, 
	    NULL, 0, NULL, &dmhcb->error.err_code, 2, 
	    locarray[locidx].phys_length,
	    locarray[locidx].physical.name, 0, dmhcb->dmh_rbn);
	if ((stat == SR_ENDFILE) || (stat == SR_EXCEED_LIMIT))
	    stat = E_DB_ERROR;
    }

    return(stat);

}


/** Name: dmh_read	- Read a block (by RBN) from a hash partition file.
**
** Description:
**      This routine reads a block previously written to a hash partition
**	file.  The file position (in pages) and number of pages/blocks to
**	read is specified.  SR doesn't guarantee that you can read past
**	the end of the file, so it's up to the caller to make sure that
**	the number of pages requested doesn't carry the read past EOF.
**	If it does, an error may be returned.
**
** Inputs:
**      dmhcb				Pointer to the DMH_CB.
**		.dmh_file_context	pointer to SR file context block
**		.dmh_locidxp		Ptr to location index, for msgs
**		.dmh_rbn		RBN of block to be read
**		.dmh_buffer		pointer to buffer to be read
**		.dmh_blksize		size (in bytes) of block
**		.dmh_nblocks		number of blocks to read
**
** Outputs:
**		.error			error code (if any)
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR			????
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	12-apr-99 (inkdo01)
**	    Written (for hash join project).
**	7-Mar-2007 (kschendel) SIR 122512
**	    Allow multi-page reads.
**
*/
DB_STATUS
dmh_read(
DMH_CB		*dmhcb)

{

    CL_ERR_DESC	sys_err;
    STATUS	stat;

    CLRDBERR(&dmhcb->error);


    /* Simply pass on the read request. */

    stat = SRreadN((SR_IO *)dmhcb->dmh_file_context, dmhcb->dmh_nblocks,
	dmhcb->dmh_rbn, dmhcb->dmh_buffer, &sys_err);

    if (stat)
    {
	DML_ODCB *odcb = (DML_ODCB *)dmhcb->dmh_db_id;
	DMP_LOC_ENTRY *locarray = odcb->odcb_dcb_ptr->dcb_ext->ext_entry;
	i4	locidx = *dmhcb->dmh_locidxp;

	uleFormat(&dmhcb->error, E_DM9712_SR_READ_ERROR, &sys_err, ULE_LOG, NULL, 
	    NULL, 0, NULL, &dmhcb->error.err_code, 2, 
	    locarray[locidx].phys_length,
	    locarray[locidx].physical.name, 0, dmhcb->dmh_rbn);
    }

    return(stat);

}


/** Name: dmh_close	- Close/destroy a hash partition file.
**
** Description:
**      This routine closes a hash partition file and destroys it.
**
** Inputs:
**      dmhcb				Pointer to the DMH_CB.
**		.dmh_file_context	pointer to SR file context block
**
** Outputs:
**		.error			error code (if any)
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR			????
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	12-apr-99 (inkdo01)
**	    Written (for hash join project).
**	22-apr-04 (inkdo01)
**	    Check for "already closed".
*/
DB_STATUS
dmh_close(
DMH_CB		*dmhcb)

{
    SR_IO	*fptr = (SR_IO *)dmhcb->dmh_file_context;
    i4		srflag;
    CL_ERR_DESC	sys_err;


    /* Set flag and close the file. */

    srflag = (dmhcb->dmh_flags_mask & DMH_DESTROY) ?
	SR_IO_DELETE : SR_IO_NODELETE;
    if (fptr->io_type != DI_IO_CLOSE_INVALID)
	return(SRclose((SR_IO *)dmhcb->dmh_file_context, srflag, &sys_err));
    else return(OK);

}
