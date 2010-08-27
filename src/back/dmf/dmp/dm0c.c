/*
** Copyright (c) 1986, 2010 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cv.h>
#include    <di.h>
#include    <lo.h>
#include    <me.h>
#include    <nm.h>
#include    <er.h>
#include    <pc.h>
#include    <cs.h>
#include    <jf.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dm2f.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dm0c.h>
#include    <dm0d.h>
#include    <dm0m.h>

/**
**
**  Name: DM0C.C - Configuration file manipulation routines.
**
**  Description:
**      This file contains routines that manipulate the configuration
**	file.
**
**          dm0c_open - Open configuration file.
**	    dm0c_close - Close and/or update configuration file.
**	    dm0c_mk_consistnt - force config file status = DSC_VALID
**	    dm0c_dmp_filename - get filename for dump version of config file.
**
**	Since the configuration file is SO critical, and since it is not
**	protected by before-image logging, we attempt to ensure the safety of
**	the config file by copying to backup areas. There are two principal
**	backup: (1) Cnnnnnnn.dmp, the checkpoint copy, is made by dmfcpp when
**	it begins the checkpoint of a database. This copy stores the info 
**	which is relevant to restoring the checkpoint, and also has the
**	advantage that it is consistent, which may not be true of the actual
**	checkpointed copy if the checkpoint was "online". There is a checkpoin
**	copy per checkpoint (unless explicitly deleted).
**
**	There is also one standard "up to date backup" of the config file in
**	the dump directory. This copy is named aaaaaaaa.cnf, and is updated
**	frequently so that it is relatively up to date. It is used by
**	rollforward if the database config file cannot be found.
**
**	In both backup copies, the 'open_count' field is forced to 0 when the
**	copy is made.
**
**  History:
**      26-oct-1986 (Derek)
**	    Created for Jupiter.
**      04-dec-1987 (jennifer)
**          Changed to not use dm2f_ calls for which are different
**          for multiple locations support.
**	15-Jun-1988 (teg)
**	    created dm0c_mk_consistnt to provide a method to access inconsistent
**	    databases for release 6.1 and higher.
**	21-jan-1989 (mikem)
**	    Added DI_SYNC_MASK to the DIopen of the config file.  We will always
**	    guarantee to disk config file writes.
**	15-may-1989 (rogerk)
**	    Made sure that dm0c_open returns an err_code value when it gets
**	    an error.
**	13-jun-1989 (dds)
**	    Include files must be lowercase (NM.h and LO.h)
**	28-jun-1989 (rogerk)
**	    Changed config file format - put dmp info at front of config
**	    file, put all the extent information at the end of the config file.
**	    Changed dm0c_convert to convert to new format.  Changed checks for
**	    config version to look at dsc_cnf_version rather than dsc_version
**	    (which is the database version and should really be checked by
**	    OPENDB for correct database version level) or dsc_c_version - which
**	    records the original version the db was created under.
**	13-jul-1989 (rogerk)
**	    Fix bug introduced in DM0C_COPY case.
**	20-aug-1989 (ralph)
**	    Check DM0C_CVCFG before converting config file.
**	16-oct-1989 (rogerk)
**	    Turn off journaling on database when convert config file.
**	 5-dec-1989 (rogerk)
**	    Added DM0C_DUMP option to dm0c_close.
**	 6-dec-1989 (rogerk)
**	    Fixed bug in DM0C_COPY option where config file was not large
**	    enough to hold the new data.
**	    Added dm0c_dmp_filename routine.
**	02-jan-1989 (greg)
**	    dm0c_open -- break up consistency/sanity check for ICL compiler
**	13-mar-1989 (rogerk)
**	    Fix convert bugs.  After converting config file, check new size
**	    before flushing converted data to disk - the conversion may have
**	    allocated a new page to the file.  Also make sure we send the
**	    correct type arguments to DIwrite call.  Fix problem with
**	    terminating extended location list.  Also take into account the
**	    space required to terminate the list when we calculate the space
**	    needed to hold the config file.
**      21-mar-1990 (mikem)
**	    bug #20769
**	    Callers of this routine would expect the DM0C_CNF fields which
**	    point into the dynamically allocated portion (cnf_data), to be
**	    valid upon return from the routine (ie. the cnf_jnl, cnf_ext , ...).
**	    Previously this routine would return with the header pointers 
**	    pointing at the deallocated memory, rather than at the newly
**	    allocated memory.
**
**	    Also eliminated DIread() loop as all DI CL's should now accept
**	    unlimited sized reads (looping internal to the CL if necessary).
**	    This loop which was meant to deal with > 64k config files never
**	    would have worked as the second read would over write the data
**	    read in by the first read.
**	26-apr-1990 (walt)
**	    Change dm0c_close to use the journal version of the checkpoint
**	    seq number rather than the dump version of the seq number when
**	    making a copy of the config file in the dump area.  The dump
**	    version of the sequence number doesn't always get incremented
**	    in a sequence of online and offline backups.
**	16-may-90 (bryanp)
**	    Find the dump location from the cnf_dcb if possible, or from the
**	    config file if it isn't in the DCB, and complain if not found.
**	    The dump directory only exists once a checkpoint has been taken,
**	    so use dsc_status & DSC_DUMP_DIR_EXISTS to know whether a dump
**	    directory should exist or not.
**	    Updated comments to reflect current config file copying usage, and
**	    changed config file copy routine to force the open_count field
**	    to 0 in the backup config file copies.
**	    Support the DM0C_TRYDUMP flag in dm0c_open() which indicates that
**	    the copy of the config file in the dump directory is to be opened.
**	    Fixed a bug in error recovery where memory was freed twice.
**	22-jun-1990 (bryanp)
**	    Fix bad cast (was (DMP_DCB *), changed to (DMP_LOC_ENTRY *)).
**	24-oct-1990 (bryanp)
**	    Changed log_di_dump_error() to be passed the CNF, rather than the
**	    DCB, so that log_di_dump_error could be able to check for a
**	    partially-built DCB (i.e., with dcb_dmp == 0), and not try to
**	    fetch the dump location name from the partial DCB. (bug #34099)
**	04-mar-1991 (ralph)
**	    Fix problem in dm0c_convert with converting config files of db's
**	    whose names are exactly 24 characters in length.
**      15-aug-1991 (jnash) merged 17-aug-1989 (Derek)
**          Checked for B1/non-B1 system opeing a non-B1/B1 database.
**	23-Oct-1991 (rmuth)
**	    Clearing up various lint errors.
**		- Changed log_di_error() and log_di_dump_error() to VOID.
**		- A call to log_dio_error had to many parameters
**	22-nov-91 (jnash)
**	    Addressing VMS bugs 40638 and 31546, reclaim file descriptors
**	    when DI_EXCEED_LIMIT returned from DIopen of config file
**	    on dump directory.
**	11-may-1992 (ananth)
**	    Increasing IIrelation tuple size project.
**	14-apr-1992 (rogerk)
**	    Removed the unitialized 'sys_err' parameter that was being
**	    passed to ule_format in dm0c_mk_consistent.
**	 7-jul-1992 (rogerk)
**	    Prototyping DMF.
**	22-sep-1992 (ananth)
**	    Get rid of core catalog section in the config file.
**      8-nov-1992 (ed)
**          remove DB_MAXNAME dependency, integrate Anath config file
**	    changes
**	06-nov-92 (jrb)
**	    Changed E_DM013B to E_DM012A since the former was being used for
**	    two different errors.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project: Changed database inconsistent codes.
**	15-mar-1993 (rogerk)
**	    Fix compile warnings.
**	30-feb-1993 (rmuth)
**	    Prototyping DI showed up some errors.
**	26-apr-1993 (andys)
**	    Cluster 6.5 Project I:
**	    Added uleformat calls to print status when LK requests fsil.
**	    Include <lg.h> to pick up logging system type definitions.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	20-sep-1993 (rogerk)
**	    Move over 6.4 changes to 6.5 code line:
**
**	    12-feb-1992 (seg)
**		FILE_OPEN is defined elsewhere under OS/2.  Undef'ed the macro
**		here so that it doesn't conflict.
**		Print correct message for OS2 on DI error
**      18-oct-93 (jrb)
**          Added conversion support for MLSorts.
**	31-jan-1994 (bryanp)
**	    B58653, B58654, B58655, B58656, B58662, B58665, B58666, B58668:
**	    Add di_retcode argument to log_di_error and log_di_dump_error so
**		that the DI return code will be logged if a DI call fails.
**	    Log a "bad-force" error message if DIforce fails, rather than the
**		bad-write message, so that force errors don't look like write
**		errors.
**	    Log a "bad-sense" error if DIsense fails, rather than bad-read.
**	    Check for out-of-disk-space errors when allocating and writing the
**		config file backup, so that resource-quota-exceeded can be
**		returned as the error.
**	    Don't return E_DM923F_DM2F_OPEN_ERROR if dm0c_open fails. Rather,
**		return the dm0c open failure message.
**	    If an unknown db inconsistent code is encountered in
**		dm0c_mk_consistnt, log the unexpected code for diagnosis.
**	    Check for out-of-disk-space errors when extending config file.
**	    Refuse attempts to convert config files larger than 64K. The
**		current code uses MEcopy and MEfill, and thus can't handle
**		config files larger than 64K. For now, we'll refuse any such
**		request, since I'm not sure that such config files actually
**		exist.
**	18-apr-1994 (jnash)
**          fsync project.  Remove DIforce() calls.
**      20-may-95 (hanch04)
**          Added include dmtcb.h
**      03-jun-1996 (canor01)
**          For operating-system threads, remove external semaphore protection
**          for NMingpath().
** 	04-jun-1997 (wonst02)
**	    Add DM0C_READONLY & DM0C_IO_READ flags for readonly databases.
**	04-aug-1997 (wonst02)
**	    If opening for write access fails, try as a readonly database.
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm2t_reclaim_tcb() calls.
**	02-sep-1997 (wonst02)
**	    Replaced reference to EACCES w/ new CL return code DI_ACCESS.
**	04-feb-1998 (hanch04)
**	    Added dm0c_convert_to_CNF_V5, the LG_LA structure changed.
**	24-mar-1998 (hanch04)
**	    Missed jnl_first_jnl_seq when upgrading cnf file.
**	01-oct-1998 (somsa01)
**	    In dm0c_close(), added check for DI_NODISKSPACE.
**	07-oct-1998 (somsa01)
**	    In dm0c_extend(), added check for DI_NODISKSPACE.
**	18-dec-1998 (hanch04)
**	    The size of ckp_history has been increased.  Initialize empty 
**	    slots in an upgrade.
**	15-jan-1999 (nanpr01)
**	    Pass pointer to pointer in dm0m_deallocate.
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**      15-Mar-1999 (hanal04)
**          If DM0C_TIMEOUT then specify a timeout of DM0C_CONFIG_WAIT
**          for config file lock request in dm0c_open. b55788.
**      21-mar-1999 (nanpr01)
**          Support raw location.
**	15-apr-1999 (hanch04)
**	    Change dm0c_convert_to_CNF_V6 to initialize new raw fields.
**	    Removed upgrade code that was not needed.
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**      27-May-1999 (hanal04)
**          Removed and reworked the above change for bug 55788 in 
**          dm0c_open() and made the necessary changes to dm0c_close().
**          b97083.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**      23-mar-2001 (stial01)
**          Cross integrate dm0c_convert_to_CNF_V6 to Ingres 2.6 config upgrade
**	02-Apr-2001 (jenjo02)
**	    Dropped raw_next from EXT, reinstated "extra" as 2 i4's.
**      27-apr-2001 (stial01)
**          dm0c_convert_to_CNF_V6() test for page size 0 in config file
**      12-jul-2001 (stial01)
**	    dm0c_convert_to_CNF_V5() should use DM0C_V5EXT (B105232)
**      24-aug-2001 (stial01)
**          dm0c_convert_to_CNF_V4() should use DM0C_V5EXT (B105677)
**      08-may-2002 (stial01)
**          Check for invalid setting of dbservice & DU_LP64
**      20-Sep-2002 (stial01)
**          dm0c_open() Moved dbservice warning to dm2d_open
**      22-jan-2003 (stial01)
**          dm0c_mk_consistnt() Added case for DSC_RFP_FAIL
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      25-oct-2005 (stial01)
**          New db inconsistency codes DSC_INDEX_INCONST, DSC_TABLE_INCONST
**      09-oct-2007 (stial01)
**          New db inconsistency codes DSC_INCR_RFP_INCONS
**          Added DM0C_RFP_NEEDLOCK open flag
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**      14-may-2008 (stial01)
**          Removed DSC_INCR_RFP_INCONS
**	18-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	    Macroized log_di_error, log_di_dump_error to add source info.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**      21-apr-2009 (stial01)
**          In upgrade routines use sizeof structure instead of DB_MAXNAME
**          Added dm0c_convert_to_CNF_V7 for config changes for long names
**          Added dm0c_upgrade_config and dm0c_test_upgrade
**	12-Nov-2009 (kschendel) SIR 122882
**	    Re-interpret cmptlvl as u_i4 instead of char[4].
**	16-Nov-2009 (kschendel) SIR 122890
**	    Don't include dudbms when not needed.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	29-Apr-2010 (kschendel)
**	    Made the change to set cnf version to V7, fix up extend to not
**	    smash the config buffer (broke the converters).
**/


/*
**  Forward definition of internal functions.
*/

static VOID	logDiErrorFcn(
			    STATUS	    di_retcode,
			    DB_STATUS	    error,
			    CL_ERR_DESC	    *sys_err,
			    DMP_DCB	    *dcb,
			    PTR		    FileName,
			    i4		    LineNumber);
#define log_di_error(di_retcode,error,sys_err,dcb) \
        logDiErrorFcn(di_retcode,error,sys_err,dcb,__FILE__,__LINE__)

static VOID	logDiDumpErrorFcn(
			    STATUS	    di_retcode,
			    DB_STATUS	    error,
			    CL_ERR_DESC	    *sys_err,
			    DM0C_CNF	    *cnf,
			    char	    *file_name,
			    i4	    	    fname_len,
			    PTR		    FileName,
			    i4		    LineNumber);
#define log_di_dump_error(di_retcode,error,sys_err,cnf,file_name,fname_len) \
        logDiDumpErrorFcn(di_retcode,error,sys_err,cnf,file_name,fname_len,\
				__FILE__,__LINE__)


static	DB_STATUS   dm0c_convert_to_CNF_V3(
    DM0C_CNF	    *config,
    DB_ERROR	    *dberr);

static	DB_STATUS   dm0c_convert_to_CNF_V4(
    DM0C_CNF	    *config,
    DB_ERROR	    *dberr);

static	DB_STATUS   dm0c_convert_to_CNF_V5(
    DM0C_CNF	    *config,
    DB_ERROR	    *dberr);

static	DB_STATUS   dm0c_convert_to_CNF_V6(
    DM0C_CNF	    *config,
    DB_ERROR	    *dberr);

static	DB_STATUS   dm0c_convert_to_CNF_V7(
    DM0C_CNF	    *config,
    DB_ERROR	    *dberr);

static DB_STATUS    dm0c_upgrade_config(
    DM0C_CNF	    *cnf,
    i4		    *cnf_version,
    DB_ERROR	    *dberr);

static DB_STATUS dm0c_test_upgrade(
    DMP_DCB	    *dcb,
    i4		    cnf_bytes,
    DB_ERROR	    *dberr);

/*
** Define Config file name for error messages.
*/
# define CONF_FILE "aaaaaaaa.cnf"


/*{
** Name: dm0c_close	- Close configuration file.
**
** Description:
**      This routine is called to close a previously opened configuration
**	file and to deallocate the memory that was used to contain the contents
**	of the configuration file.
**
**	If DM0C_COPY is requested, then this routine also writes out a backup
**	copy of the config file to the II_DUMP location, naming it either
**	aaaaaaaa.cnf or Cnnnnnnn.DMP, depending on whether DM0C_DUMP was passed
**	as well. In order to do this, we must be able to find the II_DUMP
**	location. If possible, we follow the cnf_dcb pointer to the DCB and
**	find the II_DUMP location from the DCB. If that fails (because the
**	DCB is only partially built), we look in the cnf_ext area of the
**	config info. If that fails as well, the DM0C_COPY operation is rejected
**	with an error. This may mean that callers of dm0c_open will be forced
**	to use a full read of the config file rather than a partial one if they
**	wish to copy the config file upon close and do not have a valid DCB.
**
** Inputs:
**      cnf                             Pointer to configuration info.
**      flags                           Flags.
**	    DM0C_UPDATE			Write the config file to disk.
**	    DM0C_LEAVE_OPEN		Don't close/deallocate/unlock the file
**	    DM0C_COPY			Make a copy of the config file.
**	    DM0C_DUMP			When used with DM0C_COPY, causes a
**					'checkpoint copy' of the file to be
**					made, named Cnnnnnnn.dmp.
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
**      26-oct-1986 (Derek)
**          Created for Jupiter.
**      04-dec-1987 (jennifer)
**          Changed dm2f_ calls for multiple locations support.
**	16-mar-1989 (EdHsu)
**	    Added DM0C_COPY flag to save config file
**	26-June-1989 (ac)
**	    Fixed the DM0C_COPY problems to make it work.
**	28-June-1989 (rogerk)
**	    Changed DM0C_COPY code to not assume the dump location is the
**	    last extent in the extent list.
**	13-jul-1989 (rogerk)
**	    Fix bug introduced in DM0C_COPY case, don't break till found
**	    the desired dump location.
**	11-oct-1989 (ac)
**	    Fixed the DM0C_COPY problem. Need to flush it after creating.
**	 5-dec-1989 (rogerk)
**	    Added DM0C_DUMP option to make copy of the config file in the
**	    dump directory for rollforward.  The config file is copied to
**	    the filename 'C(N).DMP'.  Also broke the DM0C_COPY code out
**	    of the DM0C_UPDATE case so it can be called independently.
**	 6-dec-1989 (rogerk)
**	    When we copy the config file to the dump directory, and we
**	    overwrite an already existing config file, make sure there is space
**	    in the CONFIG file to copy the data.
**	26-apr-1990 (walt)
**	    Change dm0c_close to use the journal version of the checkpoint
**	    seq number rather than the dump version of the seq number when
**	    making a copy of the config file in the dump area.  The dump
**	    version of the sequence number doesn't always get incremented
**	    in a sequence of online and offline backups.
**	16-may-90 (bryanp)
**	    Find the dump location from the cnf_dcb if possible, or from the
**	    config file if it isn't in the DCB, and complain if not found.
**	    Updated comments to reflect current config file copying usage, and
**	    changed config file copy routine to force the open_count field
**	    to 0 in the backup config file copies.
**	22-nov-91 (jnash)
**	    Addressing VMS bugs 40638 and 31546, reclaim file descriptors
**	    when DI_EXCEED_LIMIT returned from DIopen of config file
**	    on dump directory.
**	18-apr-1994 (jnash)
**          fsync project.  Remove DIforce() calls.
**	01-oct-1998 (somsa01)
**	    Added check for DI_NODISKSPACE.
**      27-May-1999 (hanal04)
**          Don't perform any locking actions on the CNF file is the
**          caller has specified DM0C_CNF_LOCKED. b97083.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Update LG's journal context if known to LG
**	    and it's been updated.
*/
DB_STATUS
dm0c_close(
DM0C_CNF            *config,
i4		    flags,
DB_ERROR	    *dberr)
{
    DM0C_CNF		*cnf = config;
    DMP_LOC_ENTRY	*dump = NULL;
    DB_STATUS		status;
    i4			n, i, length;
    i4			page;
    CL_ERR_DESC		sys_err;
    DM0C_EXT		*ext;
    DI_IO		di_file;
    i4		cnf_pages;
    i4		number;
    char		*filename;
    char		filebuf[16];
    i4		preserve_open_count;
    i4			*err_code = &dberr->err_code;
    LG_JFIB		jfib;
    i4			type;

    CLRDBERR(dberr);

    if (flags & DM0C_UPDATE)
    {
#ifdef xDEBUG
	if (!cnf->cnf_lkid.lk_unique)
	{
	    /* Updating without a lock on config */
	    TRdisplay("%@ UPDATE config status %x inconst %d cnf_lkid %d\n",
	    cnf->cnf_dsc->dsc_status,cnf->cnf_dsc->dsc_inconst_code, 
	    cnf->cnf_lkid.lk_unique);
	}
#endif

	/*
	** If the journal information is being updated and
	** the DB is MVCC eligible and otherwise in good order
	** and is known to the logging system, pass the current   
	** config journal file information to LGalter in the form of
	** a LG_JFIB.
	**
	** LGalter will decide if anthing in the db's JFIB
	** needs changing.
	*/
	if ( flags & DM0C_UPDATE_JNL &&
	     cnf->cnf_dcb && cnf->cnf_dcb->dcb_status & DCB_S_MVCC &&
	     cnf->cnf_dsc->dsc_status & DSC_VALID )
	{
	    /* Find by DCB's connection to DB, if known */
	    if ( (jfib.jfib_db_id = cnf->cnf_dcb->dcb_log_id) )
	        type = LG_S_JFIB;
	    else
	    {
		/* Find by external database id */
	        jfib.jfib_db_id = cnf->cnf_dsc->dsc_dbid;
		type = LG_S_JFIB_DB;
	    }

	    /* Get the log journal context for this DB, ignore status */
	    (void) LGshow(type, (PTR)&jfib, sizeof(jfib),
	    		    &length, &sys_err);
	    /*
	    ** If DB is not known to logging or is ineligible
	    ** for MVCC in logging terms, length will be zero.
	    **
	    ** Update local JFIB with what's now in the config file,
	    ** pass that to LGalter.
	    */
	    if ( length )
	    {
		if ( (i = jfib.jfib_nodeid) )
		{
		    jfib.jfib_jfa.jfa_filseq = cnf->cnf_jnl->jnl_node_info[i].cnode_fil_seq;
		    jfib.jfib_jfa.jfa_block = cnf->cnf_jnl->jnl_node_info[i].cnode_blk_seq;
		}
		else
		{
		    jfib.jfib_jfa.jfa_filseq = cnf->cnf_jnl->jnl_fil_seq;
		    jfib.jfib_jfa.jfa_block = cnf->cnf_jnl->jnl_blk_seq;
		}

		jfib.jfib_bksz = cnf->cnf_jnl->jnl_bksz;
		jfib.jfib_maxcnt = cnf->cnf_jnl->jnl_maxcnt;
		jfib.jfib_ckpseq = cnf->cnf_jnl->jnl_ckp_seq;

		(void) LGalter(LG_A_JFIB_DB, (PTR)&jfib,
				    sizeof(jfib), &sys_err);
	    }
	}
	/*
	** Write the number of pages held in the config data buffer.
	*/

	n  = cnf->cnf_bytes / DM_PG_SIZE;
	status = DIwrite((DI_IO *)cnf->cnf_di, &n, 0, cnf->cnf_data, &sys_err);
	if (status != OK)
	{
	    log_di_error(status, E_DM9006_BAD_FILE_WRITE, &sys_err, 
                                  cnf->cnf_dcb);
	    SETDBERR(dberr, 0, E_DM923B_CONFIG_CLOSE_ERROR);
	    return (E_DB_ERROR);
	}
    }

    /*
    ** Check if caller has specified to copy the config file to the dump
    ** directory.  This is done for two reasons:
    **
    **	  - Each time significant updates are made to the config file, it is
    **	    copied to the dump file as a backup copy for fault tolerance.  
    **	    If the database directory gets smashed, the backup copy of the
    **	    config file can (hopefully) be located and used by rollforward.
    **
    **	  - When a backup (checkpoint) is done, the config file is copied to the
    **	    dump directory with the filename C(N).DMP - where (N) is the
    **	    checkpoint sequence number.  When the checkpoint is restored, this
    **	    config file (who's consistency is guaranteed) is used instead of
    **	    the one in the backup saveset (which is in an unknown state).
    **	    Note that we use this backup copy for both online and offline
    **	    backups because we want to read information from the .cnf file
    **	    prior to restoring any locations (so that we can hopefully
    **	    restore multiple locations concurrently).
    **
    ** If the dsc_status field in the descriptor does not have the
    ** DSC_DUMP_DIR_EXISTS flag set, then no dump directory has been created
    ** so the copy should be quietly ignored (unless it's an explicit DUMP
    ** copy in which case this indicates an error situation).
    */
    if ((flags & DM0C_COPY) != 0 && (flags & DM0C_DUMP) != 0 &&
	(cnf->cnf_dsc->dsc_status & DSC_DUMP_DIR_EXISTS) == 0)
    {
	uleFormat(NULL, E_DM928D_NO_DUMP_LOCATION, (CL_ERR_DESC *)NULL,
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		err_code, 0);
	SETDBERR(dberr, 0, E_DM923B_CONFIG_CLOSE_ERROR);
	return (E_DB_ERROR);
    }

    if ((flags & DM0C_COPY) != 0 &&
	(cnf->cnf_dsc->dsc_status & DSC_DUMP_DIR_EXISTS) != 0)
    {
	/* Get number of pages to copy. */
	n  = cnf->cnf_bytes / DM_PG_SIZE;

	/*
	** Find dump location in location list. First try to find it from the
	** DCB, if possible. Then try cnf_ext array. Then give up.
	*/
	if (cnf->cnf_dcb)
	{
	    dump = cnf->cnf_dcb->dcb_dmp;
	}

	if (dump == 0 && cnf->cnf_ext != (DM0C_EXT *)0)
	{
	    for (i = 0; i < cnf->cnf_dsc->dsc_ext_count; i++)
	    {
		if (cnf->cnf_ext[i].ext_location.flags & DCB_DUMP)
		{
		    dump = &(cnf->cnf_ext[i].ext_location);
		    break;
		}
	    }
	}
	if (dump == NULL)
	{
	    uleFormat(NULL, E_DM928D_NO_DUMP_LOCATION, (CL_ERR_DESC *)NULL,
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		err_code, 0);
	    SETDBERR(dberr, 0, E_DM923B_CONFIG_CLOSE_ERROR);
	    return (E_DB_ERROR);
	}

	/*
	** Copy to 'aaaaaaaa.cnf' or 'cNNNNNNN.dmp' depending on which
	** type of COPY this is.
	*/
	filename = "aaaaaaaa.cnf";
	if (flags & DM0C_DUMP)
	{
	    filename = filebuf;
	    MEfill(sizeof(filebuf), '\0', (PTR)filebuf);
	    dm0c_dmp_filename(cnf->cnf_jnl->jnl_ckp_seq, filebuf);
	}

	for (;;)
	{
	    status = DIopen(&di_file, dump->physical.name,
			dump->phys_length,
			filename, (u_i4)STlength(filename),
			(i4)DM_PG_SIZE, DI_IO_WRITE, 
			DI_SYNC_MASK, &sys_err);

	    if (status == DI_EXCEED_LIMIT)
	    {
		status = dm2t_reclaim_tcb(cnf->cnf_lock_list, (i4)0, dberr);
		if (status == E_DB_OK)
		    continue;

		if (dberr->err_code == E_DM9328_DM2T_NOFREE_TCB)
		{
		    log_di_dump_error( OK, E_DM9004_BAD_FILE_OPEN, &sys_err, 
				  cnf, filename,
				  (i4)STlength(filename));
		    SETDBERR(dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
		    return (E_DB_ERROR);
		}

		uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		SETDBERR(dberr, 0, E_DM923B_CONFIG_CLOSE_ERROR);
		return (E_DB_ERROR);
	    }
	    else
		break;
	}

	if (status == DI_FILENOTFOUND)
	{
	    status = DIcreate(&di_file, dump->physical.name,
		    dump->phys_length,
		    filename, (u_i4)STlength(filename), 
		    (i4)DM_PG_SIZE, &sys_err);


	    if (status != OK)
	    {
		log_di_dump_error(status, E_DM9002_BAD_FILE_CREATE, &sys_err, 
				  cnf, filename,
				  (i4)STlength(filename));
		SETDBERR(dberr, 0, E_DM923B_CONFIG_CLOSE_ERROR);
		return (E_DB_ERROR);
	    }

	    status = DIopen(&di_file, dump->physical.name,
		    dump->phys_length,
		    filename, (u_i4)STlength(filename), 
		    (i4)DM_PG_SIZE, DI_IO_WRITE, 
		    DI_SYNC_MASK, &sys_err);

	    if (status != OK)
	    {
		log_di_dump_error(status, E_DM9004_BAD_FILE_OPEN, &sys_err, 
				  cnf, filename,
				  (i4)STlength(filename));
		SETDBERR(dberr, 0, E_DM923B_CONFIG_CLOSE_ERROR);
		return (E_DB_ERROR);
	    }

	    cnf_pages = 0;
	}
	else if (status == E_DB_OK)
	{
	    /*
	    ** If the file already exists, check that it has enough space for
	    ** the information being copied.  Increment cnf_pages after the
	    ** sense, since DIsense returns the last page number.
	    */
	    status = DIsense(&di_file, &cnf_pages, &sys_err);
	    if (status != OK)
	    {
		log_di_dump_error(status, E_DM9007_BAD_FILE_SENSE, &sys_err,
				cnf,
				filename, (i4)STlength(filename));
		SETDBERR(dberr, 0, E_DM923B_CONFIG_CLOSE_ERROR);
		return (E_DB_ERROR);
	    }
	    cnf_pages++;
	}
	else
	{
	    log_di_dump_error(status, E_DM9004_BAD_FILE_OPEN, &sys_err, 
				  cnf,
				  filename, (i4)STlength(filename));
	    SETDBERR(dberr, 0, E_DM923B_CONFIG_CLOSE_ERROR);
	    return (E_DB_ERROR);
	}

	/*
	** Allocate space in file if necessary.
	*/
	if (n > cnf_pages)
	{
	    status = DIalloc(&di_file, (i4)(n - cnf_pages), &page,
		&sys_err);
	    if (status != OK || page != cnf_pages)
	    {
		log_di_dump_error(status, E_DM9000_BAD_FILE_ALLOCATE,
				    &sys_err, cnf,
				    filename, (i4)STlength(filename));
		if (status == DI_EXCEED_LIMIT)
		    SETDBERR(dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
		else
		    SETDBERR(dberr, 0, E_DM923B_CONFIG_CLOSE_ERROR);
		return (E_DB_ERROR);
	    }
	    status = DIflush(&di_file, &sys_err);
	    if (status != OK)
	    {
		log_di_dump_error(status, E_DM9008_BAD_FILE_FLUSH, &sys_err, 
				  cnf,
				  filename, (i4)STlength(filename));
		SETDBERR(dberr, 0, E_DM923B_CONFIG_CLOSE_ERROR);
		return (E_DB_ERROR);
	    }
	}

	/*
	** Force the open_count in the backup copy to 0, while preserving
	** the actual value for the caller.
	*/
	preserve_open_count = cnf->cnf_dsc->dsc_open_count;
	cnf->cnf_dsc->dsc_open_count = 0;

	status = DIwrite(&di_file, &n, 0, cnf->cnf_data, &sys_err);

	cnf->cnf_dsc->dsc_open_count = preserve_open_count;

	if (status != OK)
	{
	    log_di_dump_error(status, E_DM9006_BAD_FILE_WRITE, &sys_err, 
				  cnf,
				  filename, (i4)STlength(filename));
	    if ((status == DI_EXCEED_LIMIT) || (status == DI_NODISKSPACE))
		SETDBERR(dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
	    else
		SETDBERR(dberr, 0, E_DM923B_CONFIG_CLOSE_ERROR);
	    return (E_DB_ERROR);
	}

	status = DIclose(&di_file, &sys_err);
	if (status != OK)
	{
	    log_di_dump_error(status, E_DM9001_BAD_FILE_CLOSE, &sys_err, 
				      cnf,
				      filename, (i4)STlength(filename));
	    SETDBERR(dberr, 0, E_DM923B_CONFIG_CLOSE_ERROR);
	    return (E_DB_ERROR);
	}
    }

    if (flags & DM0C_LEAVE_OPEN)
	return (E_DB_OK);

    status = DIclose((DI_IO *)cnf->cnf_di, &sys_err);
    if (status != OK)
    {
	log_di_error(status, E_DM9001_BAD_FILE_CLOSE, &sys_err, 
                                  cnf->cnf_dcb);
	SETDBERR(dberr, 0, E_DM923B_CONFIG_CLOSE_ERROR);
	return (E_DB_ERROR);
    }

    /*	Unlock the configuration file. */

    if ((cnf->cnf_lkid.lk_unique) && ((flags & DM0C_CNF_LOCKED) == 0))
    {
	status = LKrelease(0, cnf->cnf_lock_list, (LK_LKID *)&cnf->cnf_lkid,
	    (LK_LOCK_KEY *)0, (LK_VALUE *)0, &sys_err); 
	if (status != OK)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL, NULL, 0, NULL,
		    err_code, 0);
	    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, 
                       ULE_LOG, NULL, NULL, 0, NULL, err_code, 1,
		0, cnf->cnf_lock_list); 
	    SETDBERR(dberr, 0, E_DM923B_CONFIG_CLOSE_ERROR);
	    return (E_DB_ERROR);
	}
    }

    /*	Deallocate the configuration block. */

    if (cnf->cnf_data)
    {
	cnf->cnf_data = ((char *)cnf->cnf_data -sizeof(DMP_MISC));
	dm0m_deallocate((DM_OBJECT **) &cnf->cnf_data);
    }
    dm0m_deallocate((DM_OBJECT **) &cnf);

    return (E_DB_OK);
}

/*{
** Name: dm0c_open	- Open configuration file.
**
** Description:
**      This routine opens and locks the configuration file so
**	that an update can be performed.
**
** Inputs:
**      dcb                             Pointer to DCB for this database.
**	flags				DM0C_NOLOCK - don't request config lock
**					DM0C_TRYRFC - use "aaaaaaaa.rfc"
**					DM0C_PARTIAL - don't read in whole
**					    config file, just the header and
**					    core catalog descriptors.
**					DM0C_TRYDUMP - read "aaaaaaaa.cnf" from
**					    dump directory location.
**					DM0C_READONLY - read-only database: the 
**					    cnf file is in the II_DATABASE area.
**					DM0C_IO_READ (internal use only)- 
**					    use DI_IO_READ access mode
**	lock_list			Lock list to use.
** Outputs:
**      config				Pointer to allocated DM0C_CNF.
**					- set to 0 if config open failed.
**      err_code                        Reason for error return status.
**					    E_DM923A_CONFIG_OPEN_ERROR
**					    E_DM0112_RESOURCE_QUOTA_EXCEED
**					    E_DM0113_DB_INCOMPATABLE
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
**      26-oct-1986 (Derek)
**          Created.
**	19-jan-1986 (Derek)
**	    Changed allocation of DI_IO in CNF.
**	12-mar-1987 (Derek)
**	    Added support for alternate configuration file.
**	 8-dec-1988 (rogerk)
**	    Added check for bad DIclose in error recovery.
**	21-jan-1989 (mikem)
**	    Added DI_SYNC_MASK to the DIopen of the config file.  We will always
**	    guarantee to disk config file writes.
**	27-jan-1989 (EdHsu)
**	    Added code to support online backup
**	15-may-1989 (rogerk)
**	    Made sure to return an err_code value if encounter error.
**	20-aug-1989 (ralph)
**	    Check DM0C_CVCFG before converting config file.
**	02-jan-1989 (greg)
**	    dm0c_open -- break up consistency/sanity check for ICL compiler
**	13-mar-1990 (rogerk)
**	    Fix convert bugs.  After converting config file, check new size
**	    before flushing converted data to disk - the conversion may have
**	    allocated a new page to the file.  Also make sure we send the
**	    correct type arguments to DIwrite call.
**      21-mar-1990 (mikem)
**	    Also eliminated DIread() loop as all DI CL's should now accept
**	    unlimited sized reads (looping internal to the CL if necessary).
**	    This loop which was meant to deal with > 64k config files never
**	    would have worked as the second read would over write the data
**	    read in by the first read.
**	18-may-90 (bryanp)
**	    Support the DM0C_TRYDUMP flag in dm0c_open() which indicates that
**	    the copy of the config file in the dump directory is to be opened.
**	    If we fail to open the config file, but have already allocated
**	    memory, we must not only free the memory but must also set the
**	    caller's memory pointer back to 0 so that caller doesn't think
**	    there's memory to free.
**	22-jun-1990 (bryanp)
**	    Fix bad cast (was (DMP_DCB *), changed to (DMP_LOC_ENTRY *)).
**      15-aug-1991 (jnash) merged 17-aug-1989 (Derek)
**          Checked for B1/non-B1 system opeing a non-B1/B1 database.
**	11-may-1992 (ananth)
**	    Increasing IIrelation tuple size project.
**	    Fix core catalog sanity checks since there is no meaningfull
**	    information in them (except for relprim in iirelation).
**	22-sep-1992 (ananth)
**	    Get rid of core catalog section in the config file.
**	25-may-1993 (robf)
**	    Remove ORANGE code, rewrote check for B1/C2 servers
**      18-apr-1994 (jnash)
**          fsync project.  Remove DIforce() calls.
** 	04-jun-1997 (wonst02)
**	    Add DM0C_READONLY & DM0C_IO_READ flags for readonly databases.
**	04-aug-1997 (wonst02)
**	    If opening for write access fails, try as a readonly database.
**	02-sep-1997 (wonst02)
**	    Replaced reference to EACCES w/ new CL return code DI_ACCESS.
**      15-Mar-1999 (hanal04)
**          If DM0C_TIMEOUT then specify a timeout of DM0C_CONFIG_WAIT
**          for config file lock request. b55788.
**      27-May-1999 (hanal04)
**          Removed and reworked the above change for bug 55788. Check
**          to see if the caller has already locked the CNF file. If so
**          do not request or release the lock. b97083.
**      22-sep-2004 (fanch01)
**          Remove meaningless LK_TEMPORARY flag from LKrequest call.
**	20-Feb-2006 (kschendel)
**	    Sanity check the name against the expected database name.
**	    If someone has been swapping config files, we want to find out
**	    before e.g. rollforward deleting the (other) data directory...
**	7-Mar-2006 (kschendel)
**	    Above change broke readonly databases, which is a case
**	    in which the db names do NOT necessarily match.  Fix.
**	09-aug-2010 (maspa05) b123189, b123960
**	    Need to distinguish between a database being opened for read-only
**          access (flags & DM0C_READONLY) and a readonly database where the
**          cnf file is always in II_DATABASE
*/
DB_STATUS
dm0c_open(
DMP_DCB             *dcb,
i4		    flags,
i4             lock_list,
DM0C_CNF	    **config,
DB_ERROR	    *dberr)
{
    DM0C_CNF		*cnf = 0;
    DM0C_DSC		*desc;
    DM0C_ODSC		*odesc;
    DM0C_JNL		*jnl;
    DM0C_DMP		*dmp;
    DM0C_EXT		*ext;
    DM0C_EXT		*next_item;
    DM0C_EXT		*dump;
    char		*filename = "aaaaaaaa.cnf";
    char		*cnf_dbname;
    DM_OBJECT		*cnf_buffer;
    i4		ext_count;
    i4		cnf_pages;
    i4		page_num;
    i4		n;
    i4			pgs;
    i4		open_flag = DM2F_A_WRITE;
    i4		state = 0;
#define			    LOCK_GRANTED    0x01
#define			    FILE_OPEN	    0x02
#define			    RETURN_INFO	    0x04
    i4			cnf_dbnamelen;
    STATUS              s;
    DB_STATUS		status;
    CL_ERR_DESC          sys_err;
    LK_LOCK_KEY		cf_lockkey;
    i4		dmp_needed = 0;
    i4			major_id, minor_id;
    DMP_LOC_ENTRY	*loc;
    DMP_LOC_ENTRY	rdloc;
    i4		dsc_status, dsc_cnf_version;
    i4             timeout;
    i4			*err_code = &dberr->err_code;
    char	*dbg_upgr = NULL;

    CLRDBERR(dberr);

    /*	Allocate control block for configuration information. */

    status = dm0m_allocate((i4)sizeof(DM0C_CNF) + sizeof(DI_IO),
	(i4)0, (i4)CNF_CB, (i4)CNF_ASCII_ID, (char *)dcb, 
	(DM_OBJECT **)config, dberr);
    if (status != E_DB_OK)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM923A_CONFIG_OPEN_ERROR);
	return (E_DB_ERROR);
    }

    cnf = *config;
    cnf->cnf_dcb = dcb;
    cnf->cnf_di = (DI_IO *)&cnf[1];
    cnf->cnf_ext = 0;
    cnf->cnf_c_ext = 0;
    cnf->cnf_lkid.lk_unique = 0;
    cnf->cnf_lkid.lk_common = 0;
    cnf->cnf_free_bytes = 0;
    cnf->cnf_data = 0;

    for (;;)
    {
        i4     part1=0;

	/*  Lock the configuration file. */

        /* b97083 - Don't lock CNF if called has already done so */
	if (((dcb->dcb_status & DCB_S_ROLLFORWARD) == 0 ||
		(flags & DM0C_RFP_NEEDLOCK)) &&
	    (flags & (DM0C_NOLOCK | DM0C_CNF_LOCKED)) == 0)
	{
	    MEcopy((PTR)&dcb->dcb_name, LK_KEY_LENGTH,
		(PTR)&cf_lockkey.lk_key1);
	    cf_lockkey.lk_type = LK_CONFIG;
 
	    status = LKrequest(LK_PHYSICAL, lock_list,
			&cf_lockkey, LK_X, (LK_VALUE * )0,
			(LK_LKID *)&cnf->cnf_lkid, 0L, &sys_err); 
	    if (status != OK)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, NULL, NULL, 0, NULL,
		    err_code, 0);
		uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, 
                    ULE_LOG, NULL, NULL, 0, NULL,
		    err_code, 2, 0, LK_X, 0, lock_list);
		if (status == LK_NOLOCKS)
		    SETDBERR(dberr, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
	        break;
	    }
	    state |= LOCK_GRANTED;
	}

	/*  Check for alternate configuration. This may involve using a
	**  different filename, a different location, or both.
	*/

	if (flags & DM0C_TRYRFC)
	{
	    filename = "aaaaaaaa.rfc";
	    open_flag = DM2F_E_WRITE;
	}
	loc = &cnf->cnf_dcb->dcb_location;
	if (flags & DM0C_TRYDUMP)
	{
	    loc = cnf->cnf_dcb->dcb_dmp;
	    if (loc == (DMP_LOC_ENTRY *)0)
	    {
		uleFormat(NULL, E_DM928D_NO_DUMP_LOCATION, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    err_code, 0);
		SETDBERR(dberr, 0, E_DM923A_CONFIG_OPEN_ERROR);
		break;
	    }
	}


	for (;;)
	{
	    if (dcb->dcb_status & DCB_S_RODB) 
	    {
	    	/* 
	    	** A readonly database's .cnf file is in the II_DATABASE area.
	    	*/
	    	LOCATION	db_location;
    	    	char		db_name[DB_DB_MAXNAME+1];
    	    	char		*physical_name;

	    	loc = &rdloc;
    	    	STncpy(db_name, dcb->dcb_name.db_db_name, DB_DB_MAXNAME );
		db_name[ DB_DB_MAXNAME ] = '\0';
    	    	STtrmwhite(db_name);
	    	LOingpath(ING_DBDIR, db_name, LO_DB, &db_location);
	    	LOtos(&db_location, &physical_name);
	    	loc->phys_length = STlength(physical_name);
    	    	MEmove(loc->phys_length, physical_name, ' ',
			sizeof(loc->physical), (PTR)&loc->physical);
	    }

	    /*  Open configuration file. */

	    s = DIopen((DI_IO *)cnf->cnf_di, 
		(char *) &loc->physical, loc->phys_length,
		filename, (i4)12, (i4)DM_PG_SIZE, 
		(flags & DM0C_IO_READ) ? DI_IO_READ : DI_IO_WRITE, 
		DI_SYNC_MASK, &sys_err);
	    if (s == DI_EXCEED_LIMIT)
	    {
		status = dm2t_reclaim_tcb(lock_list, (i4)0, dberr);
		if (status == E_DB_OK)
		    continue;

		/*
		** If we can't proceed due to file open quota, 
                ** log the reason for
		** failure and return RESOURCE_EXCEEDED.
		*/
		if (dberr->err_code == E_DM9328_DM2T_NOFREE_TCB)
		{
		    log_di_error(OK, E_DM9004_BAD_FILE_OPEN, &sys_err, 
				cnf->cnf_dcb);
		    SETDBERR(dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
		    break;
		}

		uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		SETDBERR(dberr, 0, E_DM923A_CONFIG_OPEN_ERROR);
		break;
	    }

	    if (s != OK)
            {
	    	if ((s == DI_ACCESS) && 
		    ((flags & (DM0C_READONLY |DM0C_TRYRFC |DM0C_TRYDUMP)) == 0))
		{
		    flags |= DM0C_READONLY;	/* Try as a readonly database */
		    continue;
		}
                if (open_flag <= DM2F_A_WRITE)
		{
                    log_di_error(s, E_DM9004_BAD_FILE_OPEN, &sys_err,
                                  cnf->cnf_dcb);
		}
                if (s == DI_FILENOTFOUND)
                {
		    SETDBERR(dberr, 0, E_DM012A_CONFIG_NOT_FOUND);
                    if (flags & DM0C_TRYRFC)
                        state |= RETURN_INFO;
                }
                break;
            }
            state |= FILE_OPEN;
            break;
	}

	if (s != OK)
	    break;

	/*
	** Figure out size of data buffer needed.  If DM0C_PARTIAL is specified
	** then allocate space for 4 pages of the config file.
	** Otherwise, DIsense the file to find out how big a buffer to allocate.
	** If the config file becomes too large, we may want to come up with
	** a different way to process it than to allocate a huge buffer here
	** for the whole file.
	*/
	cnf_pages = DM0C_PGS_HEADER;
	if ((flags & DM0C_PARTIAL) == 0)
	{
	    status = DIsense((DI_IO *)cnf->cnf_di, &cnf_pages, &sys_err);
	    if (status != OK)
	    {
		log_di_error(status,
			    E_DM9007_BAD_FILE_SENSE, &sys_err, cnf->cnf_dcb);
		break;
	    }
	    /* Increment cnf_pages since DIsense returns last page number */
	    cnf_pages++;
	}

	/*
	** Allocate buffer to read config file into.
	*/
	status = dm0m_allocate((cnf_pages * DM_PG_SIZE) + sizeof(DMP_MISC),
	    (i4) 0, (i4)MISC_CB, (i4)MISC_ASCII_ID,
	    (char *) cnf, &cnf_buffer, dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM923A_CONFIG_OPEN_ERROR);
	    break;
	}
	cnf->cnf_data = (char *)cnf_buffer + sizeof(DMP_MISC);
	((DMP_MISC *)cnf_buffer)->misc_data = cnf->cnf_data;
	cnf->cnf_bytes = cnf_pages * DM_PG_SIZE;

	/*
	** Read data in config file.
	** Assumes DI will handle more than 32 pages in one read call, 
	** previous code tried to loop doing 32 page reads (that code
	** never worked).
	*/
	n = cnf_pages;
	status = DIread((DI_IO *)cnf->cnf_di, &n, 0, cnf->cnf_data, &sys_err);
	if (status != OK)
	{
	    log_di_error(status,
			E_DM9005_BAD_FILE_READ, &sys_err, cnf->cnf_dcb);
	    break;
	}

	/*
	** Check config file version level.
	**
	** If the config version level is at level CNF_V2 (6.0) then convert
	** it to 7.0 version.
	**
	** If the config version is not compatable, then we can't open the db.
	** The config file is not compatable if:
	**	- the major id number is different than the major id this
	**	  server was built with.
	**	- the minor id number is less than the minor id this server
	**	  was built with - note that an old server may open a config
	**	  file with has a compatable major id and a greater minor id.
	*/
	desc = (DM0C_DSC*)cnf->cnf_data;

	/*
	** We're doing a kludge here to figure out the version of the config
	** file. The version # 'dsc_cnf_version' used to be in the middle of
	** the datastructure DM0C_DSC in config files prior to version CNF_V4.
	** This made life very inconvenient because we couldn't afford to
	** change any of the fields above it. To fix this problem we've moved
	** all version #s to the beginning of the datastucture in config files
	** with version >= CNF_V4 (after the length and type fields). Now we
	** are guaranteed that the offset of the version # will not change.
	**
	** We're still left with a problem. How do we figure out the version #
	** of the config file if we aren't sure of it offset? Here's the 
	** kludge. When changing the config file version # from CNF_V3 to
	** CNF_V4 we also changed the type value of the DM0C_DSC datastructure
	** from 1 to 6. Hence we first check the type field (which is 
	** guaranteed to be at the same offset) and depending on the type,
	** cast the descriptor in the config file to the pre or post CNF_V4 
	** descriptor datastructure.
	*/

	if  (desc->type == DM0C_T_DSC)
	{
	    dsc_cnf_version = desc->dsc_cnf_version;
	    dsc_status	= desc->dsc_status;
	    cnf_dbname = &desc->dsc_name.db_db_name[0];
	    cnf_dbnamelen = sizeof(desc->dsc_name.db_db_name);
	}
	else if	(desc->type == DM0C_T_ODSC)
	{
	    odesc = (DM0C_ODSC *)desc;
	    dsc_cnf_version = odesc->dsc_cnf_version;
	    dsc_status = odesc->dsc_status;
	    cnf_dbname = &odesc->dsc_name[0];
	    cnf_dbnamelen = sizeof(odesc->dsc_name);
	}
	else
	{
	    uleFormat(NULL, E_DM928E_CONFIG_FORMAT_ERROR, 0,
		ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    break;
	}
	/* Before we do anything else, sanity-check the database name
	** to make sure that nobody has been switching cnf files around
	** on us!  which would lead to misery pretty quickly.
	** Exception is when we're reading the "real" config file for
	** the database underlying a read-only database, when we expect
	** to get a mismatch.
	*/
	if ((flags & DM0C_IO_READ) == 0
	  && STncasecmp(cnf_dbname,&dcb->dcb_name.db_db_name[0],cnf_dbnamelen) != 0)
	{
	    uleFormat(NULL, E_DM9079_CONFIG_NAME_MISMATCH, 0,
		ULE_LOG, NULL, NULL, 0, NULL, err_code, 2,
		cnf_dbnamelen, cnf_dbname,
		sizeof(dcb->dcb_name), &dcb->dcb_name.db_db_name[0]);
	    break;
	}

	if ((flags & DM0C_CVCFG) && dsc_cnf_version == CNF_VCURRENT &&
		MEcmp("test_config_upgrade_db", cnf_dbname, 22) == 0)
	{
	    /*
	    ** To get here ... connect with upgradedb options and the 
	    ** database name must be test_config_upgrade_db
	    **     sql test_config_upgrade_db -u\$ingres +U -A6
	    */
	    NMgtAt(ERx("II_UPGRADEDB_DEBUG"), &dbg_upgr);
	    if (dbg_upgr != NULL && dsc_cnf_version == CNF_VCURRENT)
	    {
		/* try to run through upgrade code with dummy config */
		TRdisplay("II_UPGRADEDB_DEBUG %s\n", dbg_upgr);
		if (!STbcompare(dbg_upgr, 0, ERx("config"), 0, TRUE))
		{
		    status = dm0c_test_upgrade(dcb, cnf->cnf_bytes, dberr);
		}
	    }
	}

	if  (dsc_cnf_version != CNF_VCURRENT) 	    
	{
	    /*
	    ** DM0C_CVCFG is a special flag that tells us the this upgradedb
	    ** thats opening the config file. Hence try to convert the 
	    ** config file to the current version.
	    */

	    if (flags & DM0C_CVCFG)
	    {
		status = dm0c_upgrade_config(cnf, &dsc_cnf_version, dberr);
		if  (status != E_DB_OK)
		    break;
		/*
		** Write out converted config file.
		** We must recalculate the Config file size as it may have been
		** extended by the CONVERT.
		*/
		pgs = cnf->cnf_bytes / DM_PG_SIZE;
		status = DIwrite((DI_IO *)cnf->cnf_di, &pgs, 0, 
				    cnf->cnf_data, &sys_err);
		if (status != OK)
		{
		    log_di_error(status, E_DM9006_BAD_FILE_WRITE,
				    &sys_err, cnf->cnf_dcb);
		    break;
		}
	    }

	    /*
	    ** Check for current config compat level
	    ** Get new 'desc' ptr as the convert routine may have changed the
	    ** address of the config data block.
	    */
	    major_id = dsc_cnf_version / 0x10000;
	    minor_id = dsc_cnf_version % 0x10000;
	    if ((major_id != CNF_VCURRENT / 0x10000) ||
		(minor_id <  CNF_VCURRENT % 0x10000))
	    {
		i4	err_parm1, err_parm2, err_parm3, err_parm4;

		err_parm1 = dsc_cnf_version / 0x10000;
		err_parm2 = dsc_cnf_version & 0xFFFF;
		err_parm3 = CNF_VCURRENT / 0x10000;
		err_parm4 = CNF_VCURRENT & 0xFFFF;
		uleFormat(NULL, E_DM928B_CONFIG_NOT_COMPATABLE, NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, err_code, 4,
		    sizeof(i4), &err_parm1, sizeof(i4), &err_parm2,
		    sizeof(i4), &err_parm3, sizeof(i4), &err_parm4);
		SETDBERR(dberr, 0, E_DM0113_DB_INCOMPATABLE);
		break;
	    }
	}

	/*
	**  Calculate addresses for basic objects within configuration file
	**  and verify that they look reasonable.
	*/

	desc = (DM0C_DSC *)cnf->cnf_data;
	jnl = (DM0C_JNL *)(desc + 1);
	dmp = (DM0C_DMP *)(jnl + 1);

	/*
	** Break the sanity check up for ICL compiler.
	** Not a performance issue. 
	*/
	part1 = (desc->length != sizeof(DM0C_DSC) || desc->type != DM0C_T_DSC ||
	         jnl->length != sizeof(DM0C_JNL) || jnl->type != DM0C_T_JNL ||
	         dmp->length != sizeof(DM0C_DMP) || dmp->type != DM0C_T_DMP);

	if (part1)
	{
	    uleFormat(NULL, E_DM928E_CONFIG_FORMAT_ERROR, NULL,
		       ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	    break;
	}

	/*
	** Process the extent entries unless the caller has requested only
	** the header and descriptor information.
	*/
	if ((flags & DM0C_PARTIAL) == 0)
        {
	    status = E_DB_OK;
	    next_item = (DM0C_EXT*)(dmp + 1);
	    for (ext = next_item, ext_count = 0;;)
	    {
		/*
		** Make sure extent item does not go past the buffer we
		** read in.
		*/
		if (next_item->length == 0)
		    break;

		if ((next_item->type == DM0C_T_EXT) &&
		    ((char *)next_item + next_item->length <
			(char *)cnf->cnf_data + cnf->cnf_bytes))
		{
		    ext_count++;
		    next_item =
			(DM0C_EXT*)((char *)next_item + next_item->length);
		    continue;
		}
		else
		{
		    status = E_DB_ERROR;
		    break;
		}
	    }

	    if (ext_count != desc->dsc_ext_count)
	    {
		uleFormat(NULL, E_DM923C_CONFIG_FORMAT_ERROR, NULL, 
                           ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
		break;
	    }
	}

	/*  Store information into the configuration block. */

	cnf->cnf_dsc = desc;
	cnf->cnf_jnl = jnl;
	cnf->cnf_dmp = dmp;
	cnf->cnf_lock_list = lock_list;

	/* 
	** If this is the first time a readonly database is opened,
	**   copy certain information from the original .cnf file. 
	*/

	if (dcb->dcb_status & DCB_S_RODB 
			&& desc->dsc_access_mode == DSC_READ
	    && ! desc->dsc_status & DSC_VALID)
	{
    	    DM0C_CNF		*rdcnf = 0;

	    /* need to do a partial open not as readonlydb so as to get
	     * the original cnf file info */

	    dcb->dcb_status &= ~DCB_S_RODB ;

	    status = dm0c_open(dcb, DM0C_PARTIAL | DM0C_NOLOCK | DM0C_IO_READ, 
	    		       lock_list, &rdcnf, dberr);

	    dcb->dcb_status |= DCB_S_RODB ;

	    if (status)
		break;

	    desc->dsc_c_version = rdcnf->cnf_dsc->dsc_c_version;
	    desc->dsc_version = rdcnf->cnf_dsc->dsc_version;
	    desc->dsc_status = DSC_VALID | DSC_NOLOGGING;
	    desc->dsc_sysmod = rdcnf->cnf_dsc->dsc_sysmod;
	    desc->dsc_ext_count = rdcnf->cnf_dsc->dsc_ext_count;
	    desc->dsc_last_table_id = rdcnf->cnf_dsc->dsc_last_table_id;
	    desc->dsc_dbcmptlvl = rdcnf->cnf_dsc->dsc_dbcmptlvl;
	    desc->dsc_1dbcmptminor = rdcnf->cnf_dsc->dsc_1dbcmptminor;
	    desc->dsc_iirel_relprim = rdcnf->cnf_dsc->dsc_iirel_relprim;
	    desc->dsc_iirel_relpgsize = rdcnf->cnf_dsc->dsc_iirel_relpgsize;
	    desc->dsc_iiatt_relpgsize = rdcnf->cnf_dsc->dsc_iiatt_relpgsize;
	    desc->dsc_iiind_relpgsize = rdcnf->cnf_dsc->dsc_iiind_relpgsize;
	    desc->dsc_iirel_relpgtype = rdcnf->cnf_dsc->dsc_iirel_relpgtype;
	    desc->dsc_iiatt_relpgtype = rdcnf->cnf_dsc->dsc_iiatt_relpgtype;
	    desc->dsc_iiind_relpgtype = rdcnf->cnf_dsc->dsc_iiind_relpgtype;
	    MEcopy(rdcnf->cnf_dsc->dsc_collation, sizeof desc->dsc_collation,
		   desc->dsc_collation);

	    /* Close the original .cnf file. */
	    status = dm0c_close(rdcnf, 0, dberr);
	}

	/*
	** Fill in extent info and free_bytes only if entire config file was
	** read in.
	*/
	if ((flags & DM0C_PARTIAL) == 0)
	{
	    cnf->cnf_ext = ext;
	    cnf->cnf_c_ext = ext_count;
	    cnf->cnf_free_bytes =  cnf->cnf_bytes -
                      (((char *)next_item + 8) - (char *)cnf->cnf_data);
	}

	return status;
    }

    /*	Error recovery. */

    if (cnf)
    {
	DB_STATUS	    local_err_code;

	if (state & FILE_OPEN)
	{
	    status = DIclose((DI_IO *)cnf->cnf_di, &sys_err);
	    if (status != E_DB_OK)
	    {
		log_di_error(status, E_DM9001_BAD_FILE_CLOSE, &sys_err, 
		    cnf->cnf_dcb);
	    }
	}

	if (state & LOCK_GRANTED)
	{
	    if (status = LKrelease(0, lock_list, (LK_LKID *)&cnf->cnf_lkid,
		    (LK_LOCK_KEY *)0, (LK_VALUE *)0, &sys_err))
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, NULL, NULL, 0, NULL,
		    err_code, 0);
		uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, NULL,
		    0, 0, 0, &local_err_code, 1, 0, lock_list);
	    }
	}

	if (cnf->cnf_data)
	{
	    cnf->cnf_data = ((char *)cnf->cnf_data -sizeof(DMP_MISC));
	    dm0m_deallocate((DM_OBJECT **) &cnf->cnf_data);
	}
	dm0m_deallocate((DM_OBJECT **) &cnf);
	/*
	** Set caller's config pointer to 0 so caller doesn't accidentally
	** try to reference or free this already freed memory.
	*/
	*config = 0;

	if (state & RETURN_INFO)
	    return (E_DB_INFO);
    }	    

    if ((dberr->err_code > E_DM_INTERNAL) || (dberr->err_code == 0))
	SETDBERR(dberr, 0, E_DM923A_CONFIG_OPEN_ERROR);
    return (E_DB_ERROR);
}


/*{
** Name: dm0c_upgrade_config	- Upgrade configuration file.
**
** Description:
**      This routine upgrades the config
**
** Inputs:
**      cnf
**      cnf_version
**
** Outputs:
**      cnf_version
**
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
**      21-apr-2009 (stial01)
**          Created from code in dm0c_open
*/
static DB_STATUS
dm0c_upgrade_config(
DM0C_CNF	*cnf,
i4		*cnf_version,
DB_ERROR		*dberr)
{
    DB_STATUS		status = E_DB_OK;
    i4			dsc_cnf_version = *cnf_version;
    DM0C_ODSC		*odesc;
    i4			*err_code = &dberr->err_code;

    while (dsc_cnf_version != CNF_VCURRENT)
    {
	switch (dsc_cnf_version)
	{
	    case CNF_V2 : 
		status = dm0c_convert_to_CNF_V3(cnf, dberr); 
		odesc = (DM0C_ODSC *)cnf->cnf_dsc;
		dsc_cnf_version = odesc->dsc_cnf_version;
		break;
	    case CNF_V3 :
		status = dm0c_convert_to_CNF_V4(cnf, dberr);
		dsc_cnf_version = cnf->cnf_dsc->dsc_cnf_version;
		break;
	    case CNF_V4 :
		status = dm0c_convert_to_CNF_V5(cnf, dberr);
		dsc_cnf_version = cnf->cnf_dsc->dsc_cnf_version;
		break;
	    case CNF_V5 : 
		status = dm0c_convert_to_CNF_V6(cnf, dberr); 
		dsc_cnf_version = cnf->cnf_dsc->dsc_cnf_version;
		break;
	    case CNF_V6 : 
		status = dm0c_convert_to_CNF_V7(cnf, dberr); 
		dsc_cnf_version = cnf->cnf_dsc->dsc_cnf_version;
		break;
	    default :
		{
		    i4	err_parm1, err_parm2, err_parm3, 
				err_parm4;

		    err_parm1 = dsc_cnf_version / 0x10000;
		    err_parm2 = dsc_cnf_version & 0xFFFF;
		    err_parm3 = CNF_VCURRENT / 0x10000;
		    err_parm4 = CNF_VCURRENT & 0xFFFF;
		    uleFormat(NULL, E_DM928B_CONFIG_NOT_COMPATABLE, NULL,
			ULE_LOG, NULL, NULL, 0, NULL, err_code, 4,
			sizeof(i4), 
			&err_parm1, sizeof(i4), 
			&err_parm2, sizeof(i4), 
			&err_parm3, sizeof(i4), 
			&err_parm4);
		    SETDBERR(dberr, 0, E_DM0113_DB_INCOMPATABLE);
		    status = E_DB_ERROR;
		}
		break;
	}
	if  (status != E_DB_OK)
	    break;
    }

    *cnf_version = dsc_cnf_version;
    return (status);
}


/*{
** Name: dm0c_mk_consistnt - Undo the mark database inconsistent.
**
** Description:
**      This routine is called to remove the inconsistent mark from a database's
**	config file.  It assumes that the config file has been open and read
**	into memory before dm0c_mk_consistnt is called. 
**
**	This routine provides a timestamp of when it forced the config file 
**	valid (same as unmarking the database inconsistent).  It also increments
**	the count of the number of times that the database is forced consistent.
**
**	If this is the 1st time that the database is being forced consistent,
**	dm0c_mk_consistnt copies the timestamp into dsc_1st_patch.
**
**	This history information may prove useful in analysing what is going
**	wrong when a database goes inconsistent.  (Additionally, the DMF
**	routines that mark a database inconsistent will leave a timestamp and
**	identifier when they do so.)
**
**	This routine DOES NOT close the config file.  It merely modifies the
**	header file struct that is passed to it.  It is up to the caller
**	to assure that this header information makes it into the config file.
**
** Inputs:
**      cnf                             Pointer to configuration info.
**
** Outputs:
**      err_code                        Reason for error return status.
**	Returns:
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    informative message output to error log.
**
** History:
**      15-Jun-1988 (teg)
**          Created for verifydb (release 6.1)
**	18-Dec-1988 (teg)
**	    fixed bad parameter in msg.
**	14-apr-1992 (rogerk)
**	    Removed the unitialized 'sys_err' parameter that was being
**	    passed to ule_format.  Depending on stack garbage, it could
**	    cause unpredicatable ule_format messages.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project: Changed database inconsistent codes.
**	31-jan-1994 (bryanp) B58665
**	    Log the actual code value if an unknown inconsistent code is
**		encountered.
*/
DB_STATUS
dm0c_mk_consistnt(
DM0C_CNF            *cnf)   /* config file info (header) record */
{
#define             ID_SIZE             32
    char	    id[ID_SIZE];    /* string identifying proc that marked db incons */
    i4	    err_code;
    CL_ERR_DESC	    sys_err;

    /* provide history info */
    ++( cnf -> cnf_dsc -> dsc_patchcnt );  /* increment count */
    cnf -> cnf_dsc -> dsc_last_patch = TMsecs();  /* timestamp when database
						  ** is being patched */
    if (cnf -> cnf_dsc -> dsc_patchcnt == 1) /* this is first time, so */
         cnf -> cnf_dsc -> dsc_1st_patch = cnf -> cnf_dsc -> dsc_last_patch;

    MEfill(sizeof(id), '\0', (PTR)id);

    /* log what went wrong */
    switch (cnf -> cnf_dsc -> dsc_inconst_code )
    {	case DSC_OPENDB_INCONSIST:
	case DSC_RECOVER_INCONST:
	    MEcopy ("CRASH RECOVERY  ",ID_SIZE, (PTR )id);
	    break;
	case DSC_WILLING_COMMIT:
	    MEcopy ("2PHASE RECOVERY ",ID_SIZE, (PTR )id);
	    break;
	case DSC_REDO_INCONSIST:
	    MEcopy ("REDO RECOVERY   ",ID_SIZE, (PTR )id);
	    break;
	case DSC_UNDO_INCONSIST:
	    MEcopy ("UNDO RECOVERY   ",ID_SIZE, (PTR )id);
	    break;
	case DSC_DM2D_ALREADY_OPEN:
	    MEcopy ("DM2D_OPEN_DB    ",ID_SIZE, (PTR )id);
	    break;
	case DSC_ERR_NOLOGGING:
	case DSC_OPN_NOLOGGING:
	    MEcopy ("NOLOGGING ERROR ",ID_SIZE, (PTR )id);
	    break;
	case DSC_RFP_FAIL:
	    MEcopy ("ROLLFORWARDDB   ",ID_SIZE, (PTR )id);
	    break;
	case DSC_INDEX_INCONST:
	    MEcopy ("INDEX ERROR     ", ID_SIZE, (PTR )id);
	    break;
	case DSC_TABLE_INCONST:
	    MEcopy ("TABLE ERROR     ", ID_SIZE, (PTR )id);
	    break;
	 
	default:
	    STprintf(id, "<Unknown ID %x>", cnf->cnf_dsc->dsc_inconst_code);
	    break;
    }

    uleFormat(NULL, E_DM9347_CONFIG_FILE_PATCHED, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL,
		&err_code, 4, sizeof(DB_DB_NAME), &cnf->cnf_dsc->dsc_name,
		0, id, 
		sizeof(cnf->cnf_dsc->dsc_inconst_time), 
		&cnf->cnf_dsc->dsc_inconst_time,
		sizeof(cnf->cnf_dsc->dsc_last_patch), 
		&cnf->cnf_dsc->dsc_last_patch);


    /* now patch to make config file look healthy */
    cnf -> cnf_dsc -> dsc_status |= DSC_VALID;
    cnf -> cnf_dsc -> dsc_open_count = 0;

    return (E_DB_OK);

}

/*{
** Name: dm0c_extend	- Add new page to the configuration file.
**
** Description:
**	This routine is called when a new location is being added to the
**	config file and there is no more room in the location list.
**	The file is extended to make room for the new location.
**
**	A new (larger) buffer is allocated to hold the config file data,
**	and the old buffer copied to the new one.  All of the cnf
**	pointers into the config file are updated to point to the
**	new buffer, using the old offsets.  (The data in the cnf buffer
**	might be an old style config in the process of being converted,
**	so doing something like "cnf->cnf_dsc->length" is wrong and
**	won't work.)
**
**	ANY CALLER-HELD POINTERS TO THE OLD CONFIG ARE INVALID.
**	The only pointers that are fixed are the ones in the passed-in
**	DM0C_CNF structure itself.
**
**	Since the extended page is not formatted (even though we zero
**	fill it here, it should not be considered guaranteed to be so),
**	there is no logging necessary with a config file extend; if the
**	system crashes before the DIflush, then the next add location call
**	will require another dm0c_extend().
**
** Inputs:
**      cnf                             Pointer to configuration info.
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
**      21-jan-1988 (rogerk)
**          Created for Jupiter.
**      21-mar-1990 (mikem)
**	    bug #20769
**	    Callers of this routine would expect the DM0C_CNF fields which
**	    point into the dynamically allocated portion (cnf_data), to be
**	    valid upon return from the routine (ie. the cnf_jnl, cnf_ext , ...).
**	    Previously this routine would return with the header pointers 
**	    pointing at the deallocated memory, rather than at the newly
**	    allocated memory.
**
**	    Also eliminated DIread() loop as all DI CL's should now accept
**	    unlimited sized reads (looping internal to the CL if necessary).
**	    This loop which was meant to deal with > 64k config files never
**	    would have worked as the second read would over write the data
**	    read in by the first read.
**	22-sep-1992 (ananth)
**	    Get rid of core catalog section in the config file.
**	31-jan-1994 (bryanp) B58666
**	    Check for out-of-disk-space when writing the new page.
**	07-oct-1998 (somsa01)
**	    Added check for DI_NODISKSPACE.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
**	30-Apr-2010 (kschendel)
**	    The new-style config converters operate in-memory, so tossing
**	    out the old buffer and re-reading from disk isn't such a good
**	    idea any more.  Rework to preserve the memory contents, and
**	    make no assumptions about what cnf->cnf_xxx points to.
*/
DB_STATUS
dm0c_extend(
DM0C_CNF            *cnf,
DB_ERROR	    *dberr)
{
    char		*old_data, *new_data;
    DM_OBJECT		*cnf_buffer;
    DB_STATUS		status;
    i4		page_num;
    i4		cnf_pages;
    i4		n;
    i4			num_pgs;
    CL_ERR_DESC		sys_err;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** Extend the config file by 1 page and flush the EOF marker.
    */
    status = DIalloc((DI_IO *)cnf->cnf_di, (i4) 1, &page_num, &sys_err);
    if (status != OK)
    {
	log_di_error(status,
		    E_DM9000_BAD_FILE_ALLOCATE, &sys_err, cnf->cnf_dcb);
	if (status == DI_EXCEED_LIMIT)
	    SETDBERR(dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
	else
	    SETDBERR(dberr, 0, E_DM928A_CONFIG_EXTEND_ERROR);
	return (E_DB_ERROR);
    }

    status = DIflush((DI_IO *)cnf->cnf_di, &sys_err);
    if (status != OK)
    {
	log_di_error(status, E_DM9008_BAD_FILE_FLUSH, &sys_err, cnf->cnf_dcb);
	SETDBERR(dberr, 0, E_DM928A_CONFIG_EXTEND_ERROR);
	return (E_DB_ERROR);
    }

    /*
    ** Allocate a new config data buffer.
    */
    cnf_pages = page_num + 1;
    status = dm0m_allocate((cnf_pages * DM_PG_SIZE) + sizeof(DMP_MISC),
	(i4) 0, (i4)MISC_CB, (i4)MISC_ASCII_ID,
	(char *) cnf, &cnf_buffer, dberr);
    if (status != E_DB_OK)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM928A_CONFIG_EXTEND_ERROR);
	return (E_DB_ERROR);
    }
    new_data = (char *) cnf_buffer + sizeof(DMP_MISC);
    ((DMP_MISC *)cnf_buffer)->misc_data = new_data;
    old_data = cnf->cnf_data;

    /* Copy old to new, set pointers */

    MEcopy(old_data, cnf->cnf_bytes, new_data);
    cnf->cnf_data = new_data;
    cnf->cnf_jnl = (DM0C_JNL *) (((char *)(cnf->cnf_jnl) - old_data) + new_data);
    cnf->cnf_dmp = (DM0C_DMP *) (((char *)(cnf->cnf_dmp) - old_data) + new_data);
    cnf->cnf_dsc = (DM0C_DSC *) (((char *)(cnf->cnf_dsc) - old_data) + new_data);
    cnf->cnf_ext = (DM0C_EXT *) (((char *)(cnf->cnf_ext) - old_data) + new_data);

    cnf->cnf_bytes = cnf_pages * DM_PG_SIZE;
    cnf->cnf_free_bytes += DM_PG_SIZE;

    /*
    ** Write out a blank page so its not full of garbage.
    ** Re-use the old buffer for this, it will be big enough.
    */
    MEfill(DM_PG_SIZE, '\0', old_data);
    num_pgs = 1;
    status = DIwrite((DI_IO *)cnf->cnf_di, &num_pgs, page_num, 
	old_data, &sys_err);
    if (status != OK)
    {
	log_di_error(status, E_DM9006_BAD_FILE_WRITE, &sys_err, cnf->cnf_dcb);
	if ((status == DI_EXCEED_LIMIT) || (status == DI_NODISKSPACE))
	    SETDBERR(dberr, 0, E_DM0112_RESOURCE_QUOTA_EXCEED);
	else
	    SETDBERR(dberr, 0, E_DM928A_CONFIG_EXTEND_ERROR);
	return (E_DB_ERROR);
    }

    old_data = old_data - sizeof(DMP_MISC);
    dm0m_deallocate((DM_OBJECT **) &old_data);

    return (E_DB_OK);
}

/*{
** Name: logDiErrorFcn	- Log DI error and return CNF error.
**
** Description:
**      Handle common error logging for this module.
**
** Inputs:
**	di_retcode			The DI call's return code
**      err_code                        The error code to log.
**      sys_err                         Pointer to sys_err.
**	dcb				Pointer to dcb.
**	FileName			Error source file name
**	LineNumber			Line number within that source.
**
** Outputs:
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      11-feb-1987 (Derek)
**	    Created.
**	23-Oct-1991 (rmuth)
**	    Changed to be VOID.
**	20-sep-1993 (rogerk)
**	    Move over 6.4 changes to 6.5 code line:
**
**	    12-feb-1992 (seg)
**		FILE_OPEN is defined elsewhere under OS/2.  Undef'ed the macro
**		here so that it doesn't conflict.
**		Print correct message for OS2 on DI error
**	31-jan-1994 (bryanp) B58653
**	    Log the DI return code to the error log for diagnosis.
**	18-Nov-2008 (jonj)
**	    Renamed to logDiErrorFcn, invoked by log_di_error macro.
[@history_template@]...
*/
static VOID
logDiErrorFcn(
STATUS		    di_retcode,
DB_STATUS           error,
CL_ERR_DESC	    *sys_err,
DMP_DCB		    *dcb,
PTR		    FileName,
i4		    LineNumber)
{
    DB_STATUS	    err_code;
    DB_ERROR	    dberr;

    /* Populate DB_ERROR with source info */
    dberr.err_data = 0;
    dberr.err_file = FileName;
    dberr.err_line = LineNumber;

    if (di_retcode)
    {
	dberr.err_code = di_retcode;
	uleFormat(&dberr, 0, sys_err, ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
    }

    dberr.err_code = error;

    uleFormat(&dberr, 0, sys_err, ULE_LOG, NULL, NULL, 0, NULL, &err_code, 5,
	    sizeof(dcb->dcb_name), &dcb->dcb_name,
	    11, "Not a table", 
            dcb->dcb_location.phys_length, &dcb->dcb_location.physical,
	    sizeof(CONF_FILE)-1, CONF_FILE, 0, 0);
    return;
}

/*{
** Name: logDiDumpErrorFcn	- Log DI error and return CNF error.
**
** Description:
**	This routine is similar to log_di_error(), but it is called when the
**	error occurs on one of the files in the dump directory (one of the
**	backup copies of aaaaaaaa.cnf, that is). This routine accepts additional
**	parameters to indicate the dump file name.
**
**	Since the DCB may be a partial DCB (i.e., dcb_dmp may not be filled in),
**	we have to do some work to find the right location to place in the
**	error message.
**
** Inputs:
**	di_retcode			The DI call's return code
**      err_code                        The error code to log.
**      sys_err                         Pointer to sys_err.
**	cnf				The CNF structure.
**	FileName			Error source file name
**	LineNumber			Line number within that source.
**
** Outputs:
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	17-may-90 (bryanp)
**	    Created.
**	24-oct-1990 (bryanp)
**	    Changed log_di_dump_error() to be passed the CNF, rather than the
**	    DCB, so that log_di_dump_error could be able to check for a
**	    partially-built DCB (i.e., with dcb_dmp == 0), and not try to
**	    fetch the dump location name from the partial DCB. (Bug #34099)
**	23-Oct-1991 (rmuth)
**	    Changed function to be VOID.
**      24-oct-1991 (jnash)
**	    Add & to err_code param in ule_format for LINTsake.       
**	31-jan-1994 (bryanp) B58653
**	    Log the DI return code to the error log for diagnosis.
**	18-Nov-2008 (jonj)
**	    Renamed to logDiDumpErrorFcn, invoked by log_di_dump_error macro.
*/
static VOID
logDiDumpErrorFcn(
STATUS		    di_retcode,
DB_STATUS           error,
CL_ERR_DESC	    *sys_err,
DM0C_CNF            *cnf,
char		    *file_name,
i4		    fname_len,
PTR		    FileName,
i4		    LineNumber)
{
    DB_STATUS		err_code;
    DMP_LOC_ENTRY	*dump = NULL;
    i4			i;
    DB_ERROR	    dberr;

    /* Populate DB_ERROR with source info */
    dberr.err_data = 0;
    dberr.err_file = FileName;
    dberr.err_line = LineNumber;

    if (di_retcode)
    {
	dberr.err_code = di_retcode;
	uleFormat(&dberr, 0, sys_err, ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
    }

    /*
    ** Find dump location in location list. First try to find it from the
    ** DCB, if possible. Then try cnf_ext array. Then give up.
    */
    if (cnf->cnf_dcb)
    {
	dump = cnf->cnf_dcb->dcb_dmp;
    }

    if (dump == 0 && cnf->cnf_ext != (DM0C_EXT *)0)
    {
	for (i = 0; i < cnf->cnf_dsc->dsc_ext_count; i++)
	{
	    if (cnf->cnf_ext[i].ext_location.flags & DCB_DUMP)
	    {
		dump = &(cnf->cnf_ext[i].ext_location);
		break;
	    }
	}
    }

    dberr.err_code = error;

    if (dump == NULL)
    {
	uleFormat(NULL, E_DM928D_NO_DUMP_LOCATION, (CL_ERR_DESC *)NULL,
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		&err_code, 0);
    }
    else if (cnf->cnf_dcb)
    {
	uleFormat(&dberr, 0, sys_err, ULE_LOG, NULL, NULL, 0, NULL, &err_code, 5,
	    sizeof(cnf->cnf_dcb->dcb_name), &cnf->cnf_dcb->dcb_name,
	    11, "Not a table", 
            dump->phys_length, &dump->physical,
	    fname_len, file_name, 0, 0);
    }
    else
    {
	uleFormat(&dberr, 0, sys_err, ULE_LOG, NULL, NULL, 0, NULL, &err_code, 5,
	    14, "Not a database",
	    11, "Not a table", 
            dump->phys_length, &dump->physical,
	    fname_len, file_name, 0, 0);
    }

    return;
}

/*{
** Name: dm0c_convert_to_CNF_V3	- Convert database config file to 6.3 format.
**
** Description:
**
** Inputs:
**      cnf                             Pointer to configuration info.
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
**	07-feb-1989 (EdHsu)
**	    Created for online backup.
**	30-jun-1989 (greg)
**	    NMgetenv is not part of CL interface; use NMgtAt()
**	10-jul-1989 (rogerk)
**	    Changed config file format - put dmp info at front of config
**	    file, put all the extent information at the end of the config file.
**	    Added check for available space in config file.
**	16-oct-1989 (rogerk)
**	    Turned off journaling on database so convert operations will
**	    not be written to journal files.
**	13-mar-1989 (rogerk)
**	    Fix problem with terminating extended location list - don't do
**	    extra increment of 'next_ext'.  Also take into account the space
**	    required to terminate the list when we calculate the space needed
**	    to hold the config file.
**	04-mar-1991 (ralph)
**	    Fix problem in dm0c_convert with converting config files of db's
**	    whose names are exactly 24 characters in length.
**	11-may-1992 (ananth)
**	    Increasing IIrelation tuple size project.
**	    Don't update core catalogs info in the config file.
**	22-sep-1992 (ananth)
**	    Get rid of core catalog section in the config file.
**	    Rename this function to dm0c_convert_to_CNF_V3.
**	31-jan-1994 (bryanp) B58668
**	    Refuse attempts to convert config files larger than 64K. The
**		current code uses MEcopy and MEfill, and thus can't handle
**		config files larger than 64K. For now, we'll refuse any such
**		request, since I'm not sure that such config files actually
**		exist.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
*/
static DB_STATUS
dm0c_convert_to_CNF_V3(
DM0C_CNF            *config,
DB_ERROR	    *dberr)
{
    DM0C_CNF		*cnf = config;
    DM_OBJECT		*cnf_buffer;
    DM0C_OEXT		*ext, *dump, *new_ext;
    DM0C_V3JNL		*ojnl;
    DM0C_V4JNL		*new_jnl;
    DM0C_V4DMP		*dmp;
    DM0C_ODSC		*desc, *new_desc;
    DM0C_TAB		*rel, *new_rel;
    DM0C_TAB		*relx, *new_relx;
    DM0C_TAB		*att, *new_att;
    DM0C_TAB		*idx, *new_idx;
    LOCATION		loc;
    CL_ERR_DESC		sys_err;
    DB_STATUS		status = E_DB_OK;
    PTR			old_config;
    i4			pre_space;
    i4			i, space_needed;
    char		*path_str;
    char		*end_ptr;
    char		db_name[DB_DB_MAXNAME+1];
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);
    TRdisplay("Convert config to V3 free space %d\n", cnf->cnf_free_bytes);

    /*
    ** Figure out the space necessary in the config file to hold the
    ** new config file.  Need room for DSC, JNL and DMP information,
    ** the iirelation, iirelidx, iiattribute and iiindex core catalogs,
    ** the extended location list and one extra location - the dump location.
    ** We also require 8 extra bytes at the end to terminate the extended
    ** location list.
    */
    desc = (DM0C_ODSC*)cnf->cnf_data;
    ojnl = (DM0C_V3JNL *)((char *)desc + desc->length);

    /* 
    ** This config file is at version CNF_V2. The core catalog info is
    ** still in the file.
    */

    rel = (DM0C_TAB *)((char *)ojnl + ojnl->length);
    relx = (DM0C_TAB *)((char *)rel + rel->length);
    att = (DM0C_TAB *)((char *)relx + relx->length);
    idx = (DM0C_TAB *)((char *)att + att->length);
    space_needed = sizeof(DM0C_ODSC) + sizeof(DM0C_V4JNL) + sizeof(DM0C_V4DMP) +
		   rel->length + relx->length + att->length + idx->length +
		   ((desc->dsc_ext_count + 1) * sizeof(DM0C_OEXT)) + 8;

    if (space_needed > MAXI2 || cnf->cnf_bytes > MAXI2)
    {
	uleFormat(NULL, E_DM004A_INTERNAL_ERROR, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM928C_CONFIG_CONVERT_ERROR);
	return (E_DB_ERROR);
    }

    /*
    ** If not enough room in the present config file, then extend it.
    */
    while (space_needed > cnf->cnf_bytes)
    {
	pre_space = cnf->cnf_bytes;
	status = dm0c_extend(cnf, dberr);
	if (status || cnf->cnf_bytes <= pre_space)
	    break;
    }
    if (status || space_needed > cnf->cnf_bytes)
    {
	if (status == E_DB_OK)
	    SETDBERR(dberr, 0, E_DM928A_CONFIG_EXTEND_ERROR);
	if (dberr->err_code > E_DM_INTERNAL)
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM928C_CONFIG_CONVERT_ERROR);
	}
	return (E_DB_ERROR);
    }

    /*
    ** Get dump location for dump extent info.
    ** Build location path for this database using ING_DUMPDIR as root.
    */
    NMgtAt(ING_DMPDIR, &path_str);
    if ((path_str == NULL) || (*path_str == '\0'))
    {
	uleFormat(NULL, E_DM936E_DM0C_GETENV, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM936E_DM0C_GETENV);
        return (E_DB_ERROR);
    }

    MEmove(sizeof(cnf->cnf_dcb->dcb_name.db_db_name),
	    cnf->cnf_dcb->dcb_name.db_db_name, 0,
	    sizeof(db_name), db_name);
    STtrmwhite(db_name);
    status = LOingpath(ING_DMPDIR, db_name, LO_DMP, &loc);
    if (!status)
	LOtos(&loc, &path_str);
    if (status || *path_str == EOS)
    {
	uleFormat(NULL, E_DM936F_DM0C_LOINGPATH, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM936F_DM0C_LOINGPATH);
        return (E_DB_ERROR);
    }

    /*
    ** Allocate buffer and copy config file to it.  Then build new config
    ** file into the CNF config file buffer.
    */
    status = dm0m_allocate(cnf->cnf_bytes + sizeof(DMP_MISC),
	(i4) 0, (i4)MISC_CB, (i4)MISC_ASCII_ID,
	(char *) cnf, &cnf_buffer, dberr);
    if (status != E_DB_OK)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM928C_CONFIG_CONVERT_ERROR);
	return (E_DB_ERROR);
    }
    old_config = (PTR)((char *)cnf_buffer + sizeof(DMP_MISC));
    ((DMP_MISC *)cnf_buffer)->misc_data = old_config;
    MEcopy((PTR)cnf->cnf_data, cnf->cnf_bytes, old_config);

    /*
    ** Get pointers to cnf data sections.
    */
    desc = (DM0C_ODSC *)old_config;
    ojnl = (DM0C_V3JNL *)((char *)desc + desc->length);
    rel = (DM0C_TAB *)((char *)ojnl + ojnl->length);
    relx = (DM0C_TAB *)((char *)rel + rel->length);
    att = (DM0C_TAB *)((char *)relx + relx->length);
    idx = (DM0C_TAB *)((char *)att + att->length);
    ext = (DM0C_OEXT *)((char *)idx + idx->length);

    /*
    ** Format new config file.
    */
    MEfill(cnf->cnf_bytes, '\0', (PTR)cnf->cnf_data);

    /* Put in DESC info */
    new_desc = (DM0C_ODSC *)cnf->cnf_data;
    STRUCT_ASSIGN_MACRO(*desc, *new_desc);

    /*
    ** Build new JNL info from old JNL struct
    */
    new_jnl = (DM0C_V4JNL *)(new_desc + 1);
    new_jnl->length = sizeof(DM0C_V4JNL);
    new_jnl->type = DM0C_T_JNL;
    new_jnl->jnl_count = ojnl->jnl_count;
    new_jnl->jnl_ckp_seq = ojnl->jnl_ckp_seq;
    new_jnl->jnl_fil_seq = ojnl->jnl_fil_seq;
    new_jnl->jnl_blk_seq = ojnl->jnl_blk_seq;
    new_jnl->jnl_update = ojnl->jnl_update;
    new_jnl->jnl_bksz = ojnl->jnl_bksz;
    new_jnl->jnl_blkcnt = ojnl->jnl_blkcnt;
    new_jnl->jnl_maxcnt = ojnl->jnl_maxcnt;
    MEfill(sizeof(new_jnl->jnl_history), '\0', (PTR) new_jnl->jnl_history);
    for (i = 0; i < CKP_OHISTORY; i++)
    {
	new_jnl->jnl_history[i].ckp_sequence =
	    ojnl->jnl_history[i].ckp_sequence;
	STRUCT_ASSIGN_MACRO(ojnl->jnl_history[i].ckp_date,
	    new_jnl->jnl_history[i].ckp_date);
	new_jnl->jnl_history[i].ckp_f_jnl = ojnl->jnl_history[i].ckp_f_jnl;
	new_jnl->jnl_history[i].ckp_l_jnl = ojnl->jnl_history[i].ckp_l_jnl;
	new_jnl->jnl_history[i].ckp_jnl_valid = 0;
	new_jnl->jnl_history[i].ckp_mode = 0;
    }
    for (i = 0; i < DM_CNODE_MAX; i++)
    {
	new_jnl->jnl_node_info[i].cnode_fil_seq =
	    ojnl->jnl_node_info[i].cnode_fil_seq;
	new_jnl->jnl_node_info[i].cnode_blk_seq =
	    ojnl->jnl_node_info[i].cnode_blk_seq;
	STRUCT_ASSIGN_MACRO(ojnl->jnl_node_info[i].cnode_la,
	    new_jnl->jnl_node_info[i].cnode_la);
    }

    /*
    ** Build and initialize new DMP information.
    */
    dmp = (DM0C_V4DMP *)(new_jnl + 1);
    dmp->length = sizeof(DM0C_V4DMP);
    dmp->type = DM0C_T_DMP;
    dmp->dmp_count = 0;
    dmp->dmp_ckp_seq = 0;
    dmp->dmp_fil_seq = 0;
    dmp->dmp_blk_seq = 0;
    dmp->dmp_update = 0;
    dmp->dmp_bksz = 8192;
    dmp->dmp_blkcnt = 256;
    dmp->dmp_maxcnt = 1024;
    MEfill(sizeof(dmp->dmp_history), '\0', (PTR) dmp->dmp_history);


    /*
    ** We're converting to CNF_V3 format. This format still had the core
    ** catalog info. Hence copy the core catalog info.
    **
    ** Note: The only usefull information in this section of the config file
    ** is the relprim value in the iirelation tuple. We will copy all the
    ** core catalog information to maintain compatibility with previous 
    ** config files.
    */

    new_rel = (DM0C_TAB *)(dmp + 1);
    new_relx = (DM0C_TAB *)((char *)new_rel + rel->length);
    new_att = (DM0C_TAB *)((char *)new_relx + relx->length);
    new_idx = (DM0C_TAB *)((char *)new_att + att->length);
    MEcopy((PTR)rel, rel->length, (PTR)new_rel);
    MEcopy((PTR)relx, relx->length, (PTR)new_relx);
    MEcopy((PTR)att, att->length, (PTR)new_att);
    MEcopy((PTR)idx, idx->length, (PTR)new_idx);

    /*
    ** Copy LOCATION list.
    */
    new_ext = (DM0C_OEXT *)((char *)new_idx + idx->length);
    for (i = 0; i < desc->dsc_ext_count; i++, ext++, new_ext++)
    {
	STRUCT_ASSIGN_MACRO(*ext, *new_ext);
    }

    /*
    ** Add DUMP location to LOCATION list.
    */
    new_desc->dsc_ext_count++;
    dump = new_ext;
    dump->length = sizeof(*dump);
    dump->type = DM0C_T_EXT;
    dump->ext_location.flags = DCB_DUMP;
    dump->ext_location.phys_length = 0;
    MEmove((sizeof(ING_DMPDIR)-1), (PTR) ING_DMPDIR, ' ', 
	    sizeof(DB_LOC_NAME), dump->ext_location.logical);
    CVlower(dump->ext_location.logical);

    dump->ext_location.phys_length = STlength(path_str);
    if (dump->ext_location.phys_length >
				    sizeof(dump->ext_location.physical.name))
	dump->ext_location.phys_length=sizeof(dump->ext_location.physical.name);
    STmove(path_str, ' ', sizeof(dump->ext_location.physical.name),
	dump->ext_location.physical.name);

    /*
    ** Make sure next location entry terminates the list.
    */
    new_ext++;
    new_ext->length = 0;
    new_ext->type = 0;

    /*
    ** Update config version number.
    ** Don't update the createdb version (dsc_version) as it reflects the
    ** version of the DBMS when the databas was created.
    */
    new_desc->dsc_version = DSC_V2;
    new_desc->dsc_cnf_version = CNF_V3;

    /*
    ** Update cnf control block info.
    */
    end_ptr = (char *)dump + sizeof(DM0C_OEXT);
    cnf->cnf_free_bytes = cnf->cnf_bytes - (end_ptr - cnf->cnf_data);
    cnf->cnf_c_ext = new_desc->dsc_ext_count;
    cnf->cnf_jnl = (DM0C_JNL *)new_jnl;
    cnf->cnf_dmp = (DM0C_DMP *)dmp;
    cnf->cnf_dsc = (DM0C_DSC *)new_desc;
    cnf->cnf_ext = (DM0C_EXT *)((char *)new_idx + idx->length);

    /*
    ** Now turn off journaling on database so that upgradedb changes will
    ** not be journaled.
    */
    new_desc->dsc_status &= ~DSC_JOURNAL;

    /*
    ** Deallocate memory used to hold config copy.
    */
    dm0m_deallocate(&cnf_buffer);

    return (E_DB_OK);
}

/*{
** Name: dm0c_convert_to_CNF_V4	- Convert database config file to 6.5 format.
**
** Description:
**
** Inputs:
**      cnf                             Pointer to configuration info.
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
**	22-sep-1992 (ananth)
**	    Get rid of core catalog section in the config file.
**      18-oct-93 (jrb)
**          Now converts "ii_sort" in old config files to "ii_work" and fills
**          in the appropriate path.
**	31-jan-1994 (bryanp) B58668
**	    Refuse attempts to convert config files larger than 64K. The
**		current code uses MEcopy and MEfill, and thus can't handle
**		config files larger than 64K. For now, we'll refuse any such
**		request, since I'm not sure that such config files actually
**		exist.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
*/
static DB_STATUS
dm0c_convert_to_CNF_V4(
DM0C_CNF            *config,
DB_ERROR	    *dberr)
{
    DM0C_CNF		*cnf = config;
    DM_OBJECT		*cnf_buffer;
    DM0C_OEXT		*old_ext;
    DM0C_V5EXT		*new_ext;
    DM0C_V4JNL		*new_jnl, *old_jnl;
    DM0C_V4DMP		*new_dmp, *old_dmp;
    DM0C_ODSC_32	*new_desc;
    DM0C_ODSC		*old_desc;
    DM0C_TAB		*rel, *relx, *att, *idx;
    CL_ERR_DESC		sys_err;
    DB_STATUS		status = E_DB_OK;
    STATUS		cl_status;
    PTR			old_config;
    i4		space_needed, pre_space, i;
    char		*end_ptr;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);
    TRdisplay("Convert config to V4 free space %d\n", cnf->cnf_free_bytes);

    /*
    ** Figure out the space necessary in the config file to hold the
    ** new config file.  Need room for DSC, JNL, DMP and EXT information.
    */

    old_desc = (DM0C_ODSC *)cnf->cnf_data;

    space_needed = sizeof(DM0C_ODSC_32) + sizeof(DM0C_V4JNL) + sizeof(DM0C_V4DMP) +
	((old_desc->dsc_ext_count) * sizeof(DM0C_V5EXT)) + 8;

    if (space_needed > MAXI2 || cnf->cnf_bytes > MAXI2)
    {
	uleFormat(NULL, E_DM004A_INTERNAL_ERROR, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM928C_CONFIG_CONVERT_ERROR);
	return (E_DB_ERROR);
    }

    /*
    ** If not enough room in the present config file, then extend it.
    */
    while (space_needed > cnf->cnf_bytes)
    {
	pre_space = cnf->cnf_bytes;
	status = dm0c_extend(cnf, dberr);
	if (status || cnf->cnf_bytes <= pre_space)
	    break;
    }
    if (status || space_needed > cnf->cnf_bytes)
    {
	if (status == E_DB_OK)
	    SETDBERR(dberr, 0, E_DM928A_CONFIG_EXTEND_ERROR);
	if (dberr->err_code > E_DM_INTERNAL)
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM928C_CONFIG_CONVERT_ERROR);
	}
	return (E_DB_ERROR);
    }

    /*
    ** Allocate buffer and copy config file to it.  Then build new config
    ** file into the CNF config file buffer.
    */
    status = dm0m_allocate(cnf->cnf_bytes + sizeof(DMP_MISC),
	(i4) 0, (i4)MISC_CB, (i4)MISC_ASCII_ID,
	(char *) cnf, &cnf_buffer, dberr);
    if (status != E_DB_OK)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM928C_CONFIG_CONVERT_ERROR);
	return (E_DB_ERROR);
    }

    old_config = (PTR)((char *)cnf_buffer + sizeof(DMP_MISC));
    ((DMP_MISC *)cnf_buffer)->misc_data = old_config;
    MEcopy((PTR)cnf->cnf_data, cnf->cnf_bytes, old_config);

    /* Get pointers to cnf data sections. */

    old_desc = (DM0C_ODSC *)old_config;
    old_jnl = (DM0C_V4JNL *)((char *)old_desc + old_desc->length);
    old_dmp = (DM0C_V4DMP *)((char *)old_jnl + old_jnl->length);

    /*
    ** This config file is at version CNF_V3. The core catalog section is
    ** still in the file.
    */

    rel = (DM0C_TAB *)((char *)old_dmp + old_dmp->length);
    relx = (DM0C_TAB *)((char *)rel + rel->length);
    att = (DM0C_TAB *)((char *)relx + relx->length);
    idx = (DM0C_TAB *)((char *)att + att->length);
    old_ext = (DM0C_OEXT *)((char *)idx + idx->length);

    /*
    ** Format new config file.
    */
    MEfill(cnf->cnf_bytes, '\0', (PTR)cnf->cnf_data);

    /* 
    ** Copy in DESC info. Expand dsc_name, dsc_owner and dsc_collation to
    ** 32 bytes. Move the relprim value for iirelation into dsc_iirel_relprim.
    */

    new_desc = (DM0C_ODSC_32 *)cnf->cnf_data;

    new_desc->length = sizeof(DM0C_ODSC_32);
    new_desc->type = DM0C_T_DSC;

    /* Update config version number. */

    new_desc->dsc_cnf_version = CNF_V4;

    new_desc->dsc_c_version = old_desc->dsc_c_version;
    new_desc->dsc_version = old_desc->dsc_version;
    new_desc->dsc_status = old_desc->dsc_status;
    new_desc->dsc_open_count = old_desc->dsc_open_count;
    new_desc->dsc_sync_flag = old_desc->dsc_sync_flag;
    
    new_desc->dsc_type = old_desc->dsc_type;
    new_desc->dsc_access_mode = old_desc->dsc_access_mode;
    new_desc->dsc_dbid= old_desc->dsc_dbid;
    new_desc->dsc_sysmod = old_desc->dsc_sysmod;
    new_desc->dsc_ext_count = old_desc->dsc_ext_count;
    new_desc->dsc_last_table_id = old_desc->dsc_last_table_id;
    new_desc->dsc_dbaccess = old_desc->dsc_dbaccess;
    new_desc->dsc_dbservice = old_desc->dsc_dbservice;
    new_desc->dsc_dbcmptlvl = old_desc->dsc_dbcmptlvl;
    new_desc->dsc_patchcnt = old_desc->dsc_patchcnt;
    new_desc->dsc_1st_patch = old_desc->dsc_1st_patch;
    new_desc->dsc_last_patch = old_desc->dsc_last_patch;
    new_desc->dsc_inconst_code = old_desc->dsc_inconst_code;
    new_desc->dsc_inconst_time = old_desc->dsc_inconst_time;

    /* Fill in the iirelation relprim value. */

    new_desc->dsc_iirel_relprim = rel->tab_relation.relprim;

    /* Copy the name and owner into the expanded fields. */

    MEmove(sizeof(old_desc->dsc_name), (PTR)&old_desc->dsc_name,
    	' ', sizeof(new_desc->dsc_name), (PTR)&new_desc->dsc_name);

    MEmove(sizeof(old_desc->dsc_owner), (PTR)&old_desc->dsc_owner,
    	' ', sizeof(new_desc->dsc_owner), (PTR)&new_desc->dsc_owner);

    MEmove(sizeof(old_desc->dsc_collation), (PTR)&old_desc->dsc_collation,
    	' ', sizeof(new_desc->dsc_collation), (PTR)&new_desc->dsc_collation);

    /* Copy the JNL info. */

    new_jnl = (DM0C_V4JNL *)(new_desc + 1);
    MEcopy((PTR)old_jnl, old_jnl->length, (PTR)new_jnl);

    /* Copy the DMP info. */

    new_dmp = (DM0C_V4DMP *)(new_jnl + 1);
    MEcopy((PTR)old_dmp, old_dmp->length, (PTR)new_dmp);

    /* 
    ** Copy the EXT info.
    ** Note: We're getting rid of the core catalog info here by moving
    ** the EXT info directly after the DMP info.
    */

    new_ext = (DM0C_V5EXT *)(new_dmp + 1);

    for (i = 0; i < new_desc->dsc_ext_count; i++, old_ext++, new_ext++)
    {
	new_ext->length = sizeof(DM0C_V5EXT);
	new_ext->type = old_ext->type;
	new_ext->extra[0] = old_ext->extra[0];
	new_ext->extra[1] = old_ext->extra[1];

	/* Expand the logical location name to 32 bytes. */

	/* Special conversion for II_SORT location: we convert it to II_WORK
	** and fill in the appropriate path...
	*/
	if (	(old_ext->ext_location.flags & DCB_WORK)
	    &&  MEcmp(old_ext->ext_location.logical,
			"ii_sort", sizeof("ii_sort")-1) == 0
	   )
	{
	    LOCATION	work_loc;
	    char	db_name[DB_DB_MAXNAME+1];
	    char	*path_str;

	    /* Fill in II_WORK info... */

	    new_ext->ext_location.flags = DCB_WORK;
	    MEmove(sizeof("ii_work")-1, "ii_work", ' ',
		sizeof(new_ext->ext_location.logical),
		(PTR)&new_ext->ext_location.logical);
	    
	    MEmove(sizeof(cnf->cnf_dcb->dcb_name.db_db_name),
		    cnf->cnf_dcb->dcb_name.db_db_name, 0,
		    sizeof(db_name), db_name);
	    STtrmwhite(db_name);

	    cl_status = LOingpath(ING_WORKDIR, db_name, LO_WORK, &work_loc);

	    if (cl_status == OK)
		LOtos(&work_loc, &path_str);

	    if (cl_status != OK  ||  path_str == NULL)
	    {
		SETDBERR(dberr, 0, E_DM936F_DM0C_LOINGPATH);
		dm0m_deallocate(&cnf_buffer);
		return(E_DB_ERROR);
	    }

	    new_ext->ext_location.phys_length = STlength(path_str);
	    STmove(path_str, ' ', sizeof(new_ext->ext_location.physical.name),
			 new_ext->ext_location.physical.name);
	}
	else
	{
	    /* This is an ordinary location; copy it over... */
	    MEmove(sizeof(old_ext->ext_location.logical), 
		    (PTR)&old_ext->ext_location.logical, 
		    ' ', sizeof(new_ext->ext_location.logical),
		    (PTR)&new_ext->ext_location.logical);

	    new_ext->ext_location.flags = old_ext->ext_location.flags;
	    new_ext->ext_location.phys_length =
					old_ext->ext_location.phys_length;
	    STRUCT_ASSIGN_MACRO (
	       old_ext->ext_location.physical, new_ext->ext_location.physical); 
	}
    }

    /* Make sure next location entry terminates the list. */

    new_ext++;
    new_ext->length = 0;
    new_ext->type = 0;

    /* Update cnf control block info. */

    end_ptr = (char *)new_ext + sizeof(DM0C_V5EXT);
    cnf->cnf_free_bytes = cnf->cnf_bytes - (end_ptr - cnf->cnf_data);
    cnf->cnf_c_ext = new_desc->dsc_ext_count;
    cnf->cnf_jnl = (DM0C_JNL *)new_jnl;
    cnf->cnf_dmp = (DM0C_DMP *)new_dmp;
    cnf->cnf_dsc = (DM0C_DSC *) new_desc;
    cnf->cnf_ext = (DM0C_EXT *)(new_dmp + 1);

    /*
    ** Now turn off journaling on database so that upgradedb changes will
    ** not be journaled.
    */
    new_desc->dsc_status &= ~DSC_JOURNAL;

    /* Deallocate memory used to hold config copy. */

    dm0m_deallocate(&cnf_buffer);

    return (E_DB_OK);
}

/*{
** Name: dm0c_convert_to_CNF_V5	- Convert database config file to OI2.5 format.
**
** Description:
**
** Inputs:
**      cnf                             Pointer to configuration info.
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
**	02-feb-1998 (hanch04)
**	    Created from dm0c_convert_to_CNF_V4, needed for new LG_LA format.
**      28-may-1998 (stial01)
**          dm0c_convert_to_CNF_V5() Config file changes for FUTURE (3.0)
**          support of VPS system catalogs, init dsc_ii*_relpgsize.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
*/
static DB_STATUS
dm0c_convert_to_CNF_V5(
DM0C_CNF            *config,
DB_ERROR	    *dberr)
{
    DM0C_CNF		*cnf = config;
    DM_OBJECT		*cnf_buffer;
    DM0C_V5EXT		*old_ext, *new_ext;
    DM0C_V4JNL		*old_jnl;
    DM0C_JNL		*new_jnl;
    DM0C_V4DMP		*old_dmp;
    DM0C_DMP		*new_dmp;
    DM0C_ODSC_32	*new_desc, *old_desc;
    CL_ERR_DESC		sys_err;
    DB_STATUS		status = E_DB_OK;
    STATUS		cl_status;
    PTR			old_config;
    i4		space_needed, pre_space, i;
    char		*end_ptr;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);
    TRdisplay("Convert config to V5 free space %d\n", cnf->cnf_free_bytes);

    /*
    ** Figure out the space necessary in the config file to hold the
    ** new config file.  Need room for DSC, JNL, DMP and EXT information.
    */

    old_desc = (DM0C_ODSC_32 *)cnf->cnf_data;

    space_needed = sizeof(DM0C_ODSC_32) + sizeof(DM0C_JNL) + sizeof(DM0C_DMP) +
	((old_desc->dsc_ext_count) * sizeof(DM0C_V5EXT)) + 8;

    if (space_needed > MAXI2 || cnf->cnf_bytes > MAXI2)
    {
	uleFormat(NULL, E_DM004A_INTERNAL_ERROR, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM928C_CONFIG_CONVERT_ERROR);
	return (E_DB_ERROR);
    }

    /*
    ** If not enough room in the present config file, then extend it.
    */
    while (space_needed > cnf->cnf_bytes)
    {
	pre_space = cnf->cnf_bytes;
	status = dm0c_extend(cnf, dberr);
	if (status || cnf->cnf_bytes <= pre_space)
	    break;
    }
    if (status || space_needed > cnf->cnf_bytes)
    {
	if (status == E_DB_OK)
	    SETDBERR(dberr, 0, E_DM928A_CONFIG_EXTEND_ERROR);
	if (dberr->err_code > E_DM_INTERNAL)
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM928C_CONFIG_CONVERT_ERROR);
	}
	return (E_DB_ERROR);
    }

    /*
    ** Allocate buffer and copy config file to it.  Then build new config
    ** file into the CNF config file buffer.
    */
    status = dm0m_allocate(cnf->cnf_bytes + sizeof(DMP_MISC),
	(i4) 0, (i4)MISC_CB, (i4)MISC_ASCII_ID,
	(char *) cnf, &cnf_buffer, dberr);
    if (status != E_DB_OK)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM928C_CONFIG_CONVERT_ERROR);
	return (E_DB_ERROR);
    }

    old_config = (PTR)((char *)cnf_buffer + sizeof(DMP_MISC));
    ((DMP_MISC *)cnf_buffer)->misc_data = old_config;
    MEcopy((PTR)cnf->cnf_data, cnf->cnf_bytes, old_config);

    /* Get pointers to cnf data sections. */

    old_desc = (DM0C_ODSC_32 *)old_config;
    old_jnl = (DM0C_V4JNL *)((char *)old_desc + old_desc->length);
    old_dmp = (DM0C_V4DMP *)((char *)old_jnl + old_jnl->length);
    old_ext = (DM0C_V5EXT *)((char *)old_dmp + old_dmp->length);

    /*
    ** Format new config file.
    */
    MEfill(cnf->cnf_bytes, '\0', (PTR)cnf->cnf_data);

    /* 
    ** Copy in DESC info. Expand dsc_name, dsc_owner and dsc_collation to
    ** 32 bytes. Move the relprim value for iirelation into dsc_iirel_relprim.
    */

    new_desc = (DM0C_ODSC_32 *)cnf->cnf_data;
    MEcopy((PTR)old_desc, old_desc->length, (PTR)new_desc);

    /* Update config version number. */

    new_desc->dsc_cnf_version = CNF_V5;

    /* Initialize core catalog page sizes */
    new_desc->dsc_iirel_relpgsize = DM_PG_SIZE;
    new_desc->dsc_iiatt_relpgsize = DM_PG_SIZE;
    new_desc->dsc_iiind_relpgsize = DM_PG_SIZE;

    new_desc->dsc_iirel_relpgtype = TCB_PG_V1;
    new_desc->dsc_iiatt_relpgtype = TCB_PG_V1;
    new_desc->dsc_iiind_relpgtype = TCB_PG_V1;

    /*
    ** Build new JNL info from old JNL struct
    ** Set la_block to zero
    */
    new_jnl = (DM0C_JNL *)(new_desc + 1);
    new_jnl->length = sizeof(DM0C_JNL);
    new_jnl->type = DM0C_T_JNL;
    new_jnl->jnl_count = old_jnl->jnl_count;
    new_jnl->jnl_ckp_seq = old_jnl->jnl_ckp_seq;
    new_jnl->jnl_fil_seq = old_jnl->jnl_fil_seq;
    new_jnl->jnl_blk_seq = old_jnl->jnl_blk_seq;
    new_jnl->jnl_update = old_jnl->jnl_update;
    new_jnl->jnl_bksz = old_jnl->jnl_bksz;
    new_jnl->jnl_blkcnt = old_jnl->jnl_blkcnt;
    new_jnl->jnl_maxcnt = old_jnl->jnl_maxcnt;
    new_jnl->jnl_la.la_sequence = old_jnl->jnl_la.la_sequence;
    new_jnl->jnl_la.la_block = 0;
    new_jnl->jnl_la.la_offset = old_jnl->jnl_la.la_offset;
    new_jnl->jnl_first_jnl_seq = old_jnl->jnl_first_jnl_seq;
    MEfill(sizeof(new_jnl->jnl_history), '\0', (PTR) new_jnl->jnl_history);
    for (i = 0; i < CKP_OHISTORY; i++)
    {
	new_jnl->jnl_history[i].ckp_sequence =
	    old_jnl->jnl_history[i].ckp_sequence;
	STRUCT_ASSIGN_MACRO(old_jnl->jnl_history[i].ckp_date,
	    new_jnl->jnl_history[i].ckp_date);
	new_jnl->jnl_history[i].ckp_f_jnl = old_jnl->jnl_history[i].ckp_f_jnl;
	new_jnl->jnl_history[i].ckp_l_jnl = old_jnl->jnl_history[i].ckp_l_jnl;
	new_jnl->jnl_history[i].ckp_jnl_valid = 
	   old_jnl->jnl_history[i].ckp_jnl_valid;
	new_jnl->jnl_history[i].ckp_mode = old_jnl->jnl_history[i].ckp_mode;
    }
    for (i = 0; i < DM_CNODE_MAX; i++)
    {
	new_jnl->jnl_node_info[i].cnode_fil_seq =
	    old_jnl->jnl_node_info[i].cnode_fil_seq;
	new_jnl->jnl_node_info[i].cnode_blk_seq =
	    old_jnl->jnl_node_info[i].cnode_blk_seq;
	new_jnl->jnl_node_info[i].cnode_la.la_sequence =
	   old_jnl->jnl_node_info[i].cnode_la.la_sequence =
	new_jnl->jnl_node_info[i].cnode_la.la_block = 0;
	new_jnl->jnl_node_info[i].cnode_la.la_offset =
	   old_jnl->jnl_node_info[i].cnode_la.la_offset;
    }

    /*
    ** Build new JNL info from old JNL struct
    ** Set la_block to zero
    */
    new_dmp = (DM0C_DMP *)(new_jnl + 1);
    new_dmp->length = sizeof(DM0C_DMP);
    new_dmp->type = DM0C_T_DMP;
    new_dmp->dmp_count = old_dmp->dmp_count;
    new_dmp->dmp_ckp_seq = old_dmp->dmp_ckp_seq;
    new_dmp->dmp_fil_seq = old_dmp->dmp_fil_seq;
    new_dmp->dmp_blk_seq = old_dmp->dmp_blk_seq;
    new_dmp->dmp_update = old_dmp->dmp_update;
    new_dmp->dmp_bksz = old_dmp->dmp_bksz;
    new_dmp->dmp_blkcnt = old_dmp->dmp_blkcnt;
    new_dmp->dmp_maxcnt = old_dmp->dmp_maxcnt;
    new_dmp->dmp_la.la_sequence = old_dmp->dmp_la.la_sequence;
    new_dmp->dmp_la.la_block = 0;
    new_dmp->dmp_la.la_offset = old_dmp->dmp_la.la_offset;
    MEfill(sizeof(new_dmp->dmp_history), '\0', (PTR) new_dmp->dmp_history);
    for (i = 0; i < CKP_OHISTORY; i++)
    {
	new_dmp->dmp_history[i].ckp_sequence =
	    old_dmp->dmp_history[i].ckp_sequence;
	STRUCT_ASSIGN_MACRO(old_dmp->dmp_history[i].ckp_date,
	    new_dmp->dmp_history[i].ckp_date);
	new_dmp->dmp_history[i].ckp_f_dmp = old_dmp->dmp_history[i].ckp_f_dmp;
	new_dmp->dmp_history[i].ckp_l_dmp = old_dmp->dmp_history[i].ckp_l_dmp;
	new_dmp->dmp_history[i].ckp_dmp_valid = 
	   old_dmp->dmp_history[i].ckp_dmp_valid;
	new_dmp->dmp_history[i].ckp_mode = old_dmp->dmp_history[i].ckp_mode;
    }
    for (i = 0; i < DM_CNODE_MAX; i++)
    {
	new_dmp->dmp_node_info[i].cnode_fil_seq =
	    old_dmp->dmp_node_info[i].cnode_fil_seq;
	new_dmp->dmp_node_info[i].cnode_blk_seq =
	    old_dmp->dmp_node_info[i].cnode_blk_seq;
	new_dmp->dmp_node_info[i].cnode_la.la_sequence =
	   old_dmp->dmp_node_info[i].cnode_la.la_sequence =
	new_dmp->dmp_node_info[i].cnode_la.la_block = 0;
	new_dmp->dmp_node_info[i].cnode_la.la_offset =
	   old_dmp->dmp_node_info[i].cnode_la.la_offset;
    }

    new_ext = (DM0C_V5EXT *)(new_dmp + 1);
    for (i = 0; i < new_desc->dsc_ext_count; i++, old_ext++, new_ext++)
    {
	MEcopy((PTR)old_ext, old_ext->length, (PTR)new_ext);
    }

    /* Update cnf control block info. */

    end_ptr = (char *)new_ext + sizeof(DM0C_V5EXT);
    cnf->cnf_free_bytes = cnf->cnf_bytes - (end_ptr - cnf->cnf_data);
    cnf->cnf_c_ext = new_desc->dsc_ext_count;
    cnf->cnf_jnl = new_jnl;
    cnf->cnf_dmp = new_dmp;
    cnf->cnf_dsc = (DM0C_DSC *) new_desc;
    cnf->cnf_ext = (DM0C_EXT *)(new_dmp + 1);

    /*
    ** Now turn off journaling on database so that upgradedb changes will
    ** not be journaled.
    */
    new_desc->dsc_status &= ~DSC_JOURNAL;

    /* Deallocate memory used to hold config copy. */

    dm0m_deallocate(&cnf_buffer);

    return (E_DB_OK);
}

/*{
** Name: dm0c_convert_to_CNF_V6	- Convert database config file to II2.6 format.
**
** Description:
**
** Inputs:
**      cnf                             Pointer to configuration info.
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
**	12-mar-1999 (nanpr01)
**	    Created from dm0c_convert_to_CNF_V5, needed for new raw location.
**	08-feb-2000 (abbjo03)
**	    Add replication location information.
**      03-mar-2001 (stial01)
**          Moved replication location information to CNF_V7 conversion
**	25-Mar-2001 (jenjo02)
**	    Initialize new raw location fields.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
**	29-Apr-2010 (kschendel)
**	    We're outputting a DM0C_ODSC_32 now.
*/
static DB_STATUS
dm0c_convert_to_CNF_V6(
DM0C_CNF            *config,
DB_ERROR	    *dberr)
{
    DM0C_CNF		*cnf = config;
    DM_OBJECT		*cnf_buffer;
    DM0C_V6EXT		*new_ext;
    DM0C_V5EXT		*old_ext;
    DM0C_JNL		*new_jnl, *old_jnl;
    DM0C_DMP		*new_dmp, *old_dmp;
    DM0C_ODSC_32	*new_desc, *old_desc;
    DB_STATUS		status = E_DB_OK;
    PTR			old_config;
    i4		space_needed, pre_space, i;
    char		*end_ptr;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);
    TRdisplay("Convert config to V6 free space %d\n", cnf->cnf_free_bytes);

    /*
    ** Figure out the space necessary in the config file to hold the
    ** new config file.  Need room for DSC, JNL, DMP and EXT information.
    */

    old_desc = (DM0C_ODSC_32 *)cnf->cnf_data;

    space_needed = sizeof(DM0C_ODSC_32) + sizeof(DM0C_JNL) + sizeof(DM0C_DMP) +
	((old_desc->dsc_ext_count) * sizeof(DM0C_V6EXT)) + 8;

    if (space_needed > MAXI2 || cnf->cnf_bytes > MAXI2)
    {
	uleFormat(NULL, E_DM004A_INTERNAL_ERROR, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM928C_CONFIG_CONVERT_ERROR);
	return (E_DB_ERROR);
    }

    /*
    ** If not enough room in the present config file, then extend it.
    */
    while (space_needed > cnf->cnf_bytes)
    {
	pre_space = cnf->cnf_bytes;
	status = dm0c_extend(cnf, dberr);
	if (status || cnf->cnf_bytes <= pre_space)
	    break;
    }
    if (status || space_needed > cnf->cnf_bytes)
    {
	if (status == E_DB_OK)
	    SETDBERR(dberr, 0, E_DM928A_CONFIG_EXTEND_ERROR);
	if (dberr->err_code > E_DM_INTERNAL)
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM928C_CONFIG_CONVERT_ERROR);
	}
	return (E_DB_ERROR);
    }

    /*
    ** Allocate buffer and copy config file to it.  Then build new config
    ** file into the CNF config file buffer.
    */
    status = dm0m_allocate(cnf->cnf_bytes + sizeof(DMP_MISC),
	(i4) 0, (i4)MISC_CB, (i4)MISC_ASCII_ID,
	(char *) cnf, &cnf_buffer, dberr);
    if (status != E_DB_OK)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM928C_CONFIG_CONVERT_ERROR);
	return (E_DB_ERROR);
    }

    old_config = (PTR)((char *)cnf_buffer + sizeof(DMP_MISC));
    ((DMP_MISC *)cnf_buffer)->misc_data = old_config;
    MEcopy((PTR)cnf->cnf_data, cnf->cnf_bytes, old_config);

    /* Get pointers to cnf data sections. */

    old_desc = (DM0C_ODSC_32 *)old_config;
    old_jnl = (DM0C_JNL *)((char *)old_desc + old_desc->length);
    old_dmp = (DM0C_DMP *)((char *)old_jnl + old_jnl->length);
    old_ext = (DM0C_V5EXT *)((char *)old_dmp + old_dmp->length);

    /*
    ** Format new config file.
    */
    MEfill(cnf->cnf_bytes, '\0', (PTR)cnf->cnf_data);

    /* Copy in DESC info */

    new_desc = (DM0C_ODSC_32 *)cnf->cnf_data;
    MEcopy((PTR)old_desc, old_desc->length, (PTR)new_desc);

    /* Update config version number. */

    new_desc->dsc_cnf_version = CNF_V6;

    /*
    ** We need the (new) page type to properly determine page accessors
    ** and page size to properly call the page accessors during the bootstrap
    */
    if (new_desc->dsc_iirel_relpgsize == 0)
	new_desc->dsc_iirel_relpgsize = DM_COMPAT_PGSIZE;

    if (new_desc->dsc_iirel_relpgsize == DM_COMPAT_PGSIZE)
	new_desc->dsc_iirel_relpgtype = TCB_PG_V1;
    else
	new_desc->dsc_iirel_relpgtype = TCB_PG_V2;

    if (new_desc->dsc_iiatt_relpgsize == 0)
	new_desc->dsc_iiatt_relpgsize = DM_COMPAT_PGSIZE;

    if (new_desc->dsc_iiatt_relpgsize == DM_COMPAT_PGSIZE)
	new_desc->dsc_iiatt_relpgtype = TCB_PG_V1;
    else
	new_desc->dsc_iiatt_relpgtype = TCB_PG_V2;

    if (new_desc->dsc_iiind_relpgsize == 0)
	new_desc->dsc_iiind_relpgsize = DM_COMPAT_PGSIZE;

    if (new_desc->dsc_iiind_relpgsize == DM_COMPAT_PGSIZE)
	new_desc->dsc_iiind_relpgtype = TCB_PG_V1;
    else
	new_desc->dsc_iiind_relpgtype = TCB_PG_V2;

    /*
    ** Build new JNL info from old JNL struct
    */
    new_jnl = (DM0C_JNL *)(new_desc + 1);
    MEcopy((PTR)old_jnl, old_jnl->length, (PTR)new_jnl);
    new_jnl->length = sizeof(DM0C_JNL);

    /*
    ** Build new JNL info from old JNL struct
    ** Set la_block to zero
    */
    new_dmp = (DM0C_DMP *)(new_jnl + 1);
    MEcopy((PTR)old_dmp, old_dmp->length, (PTR)new_dmp);
    new_dmp->length = sizeof(DM0C_JNL);

    new_ext = (DM0C_V6EXT *)(new_dmp + 1);
    for (i = 0; i < new_desc->dsc_ext_count; i++, old_ext++, new_ext++)
    {
	MEcopy((PTR)old_ext, old_ext->length, (PTR)new_ext);
	new_ext->length = sizeof(DM0C_V6EXT);
	new_ext->ext_location.raw_start   = 0;
	new_ext->ext_location.raw_blocks  = 0;
	new_ext->ext_location.raw_total_blocks  = 0;
    }

    /* Update cnf control block info. */

    end_ptr = (char *)new_ext + sizeof(DM0C_V6EXT);
    cnf->cnf_free_bytes = cnf->cnf_bytes - (end_ptr - cnf->cnf_data);
    cnf->cnf_c_ext = new_desc->dsc_ext_count;
    cnf->cnf_jnl = new_jnl;
    cnf->cnf_dmp = new_dmp;
    cnf->cnf_dsc = (DM0C_DSC *) new_desc;
    cnf->cnf_ext = (DM0C_EXT *)(new_dmp + 1);

    /*
    ** Now turn off journaling on database so that upgradedb changes will
    ** not be journaled.
    */
    new_desc->dsc_status &= ~DSC_JOURNAL;

    /* Deallocate memory used to hold config copy. */

    dm0m_deallocate(&cnf_buffer);

    return (E_DB_OK);
}


/*{
** Name: dm0c_dmp_filename - Generate filename to save dump config file.
**
** Description:
**	This routine generates a filename for the saved config file in the
**	the dump file using the given checkpoint sequence number.
**
**	The filename used is of the form "cNNNNNNN.dmp" - where NNNNNNN is
**	the ascii representation of the checkpoint sequence number.
**
** Inputs:
**	sequence			Sequence number.
**
** Outputs:
**	filename			Filled in with dump file name.
**	Returns:
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**       6-dec-1989 (rogerk)
**          Created for Terminator.
[@history_template@]...
*/
VOID
dm0c_dmp_filename(
i4		    sequence,
char		    *filename)
{
    filename[0] = 'c';
    filename[1] = ((sequence / 1000000) % 10) + '0';
    filename[2] = ((sequence / 100000) % 10) + '0';
    filename[3] = ((sequence / 10000) % 10) + '0';
    filename[4] = ((sequence / 1000) % 10) + '0';
    filename[5] = ((sequence / 100) % 10) + '0';
    filename[6] = ((sequence / 10) % 10) + '0';
    filename[7] = (sequence % 10) + '0';
    filename[8] = '.';
    filename[9] = 'd';
    filename[10] = 'm';
    filename[11] = 'p';
}


/*{
** Name: dm0c_convert_to_CNF_V7	- Convert database config file to V7.
**
** Description: This is an upgrade of DM0C_DSC and DM0C_EXT
**
** Inputs:
**      cnf                             Pointer to configuration info.
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
**      21-apr-2009 (stial01)
**	    Created for long object names (DM0C_DSC and DM0C_EXT)
*/
static DB_STATUS
dm0c_convert_to_CNF_V7(
DM0C_CNF            *config,
DB_ERROR	    *dberr)
{
    DM0C_CNF		*cnf = config;
    DM_OBJECT		*cnf_buffer;
    DM0C_EXT		*new_ext;
    DM0C_V6EXT		*old_ext; /* V6 EXT has DB_MAXNAME 32 */
    DM0C_JNL		*new_jnl, *old_jnl;
    DM0C_DMP		*new_dmp, *old_dmp;
    DM0C_ODSC_32	*old_desc; /* old DSC has DB_MAXNAME 32 */
    DM0C_DSC		*new_desc;
    DB_STATUS		status = E_DB_OK;
    PTR			old_config;
    i4		space_needed, pre_space, i;
    char		*end_ptr;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);
    TRdisplay("Convert config to V7 free space %d\n", cnf->cnf_free_bytes);

    /*
    ** Figure out the space necessary in the config file to hold the
    ** new config file.  Need room for DSC, JNL, DMP and EXT information.
    */

    old_desc = (DM0C_ODSC_32 *)cnf->cnf_data;

    space_needed = sizeof(DM0C_DSC) + sizeof(DM0C_JNL) + sizeof(DM0C_DMP) +
	((old_desc->dsc_ext_count) * sizeof(DM0C_EXT)) + 8;

    if (space_needed > MAXI2 || cnf->cnf_bytes > MAXI2)
    {
	uleFormat(NULL, E_DM004A_INTERNAL_ERROR, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM928C_CONFIG_CONVERT_ERROR);
	return (E_DB_ERROR);
    }

    /*
    ** If not enough room in the present config file, then extend it.
    */
    while (space_needed > cnf->cnf_bytes)
    {
	pre_space = cnf->cnf_bytes;
	status = dm0c_extend(cnf, dberr);
	if (status || cnf->cnf_bytes <= pre_space)
	    break;
    }
    if (status || space_needed > cnf->cnf_bytes)
    {
	if (status == E_DB_OK)
	    SETDBERR(dberr, 0, E_DM928A_CONFIG_EXTEND_ERROR);
	if (dberr->err_code > E_DM_INTERNAL)
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    SETDBERR(dberr, 0, E_DM928C_CONFIG_CONVERT_ERROR);
	}
	return (E_DB_ERROR);
    }

    /*
    ** Allocate buffer and copy config file to it.  Then build new config
    ** file into the CNF config file buffer.
    */
    status = dm0m_allocate(cnf->cnf_bytes + sizeof(DMP_MISC),
	(i4) 0, (i4)MISC_CB, (i4)MISC_ASCII_ID,
	(char *) cnf, &cnf_buffer, dberr);
    if (status != E_DB_OK)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM928C_CONFIG_CONVERT_ERROR);
	return (E_DB_ERROR);
    }

    old_config = (PTR)((char *)cnf_buffer + sizeof(DMP_MISC));
    ((DMP_MISC *)cnf_buffer)->misc_data = old_config;
    MEcopy((PTR)cnf->cnf_data, cnf->cnf_bytes, old_config);

    /* Get pointers to cnf data sections. */

    old_desc = (DM0C_ODSC_32 *)old_config;
    old_jnl = (DM0C_JNL *)((char *)old_desc + old_desc->length);
    old_dmp = (DM0C_DMP *)((char *)old_jnl + old_jnl->length);
    old_ext = (DM0C_V6EXT *)((char *)old_dmp + old_dmp->length);

    /*
    ** Format new config file.
    */
    MEfill(cnf->cnf_bytes, '\0', (PTR)cnf->cnf_data);

    /* 
    ** Copy in DESC info. Expand dsc_name, dsc_owner, dsc_collation and
    ** dsc_ucollation to new DB_MAXNAME.
    */

    new_desc = (DM0C_DSC *)cnf->cnf_data;
    new_desc->length = sizeof(DM0C_DSC);
    new_desc->type = DM0C_T_DSC;

    /* Update config version number. */
    new_desc->dsc_cnf_version = CNF_V7;

    /* Copy each field from old DSC to new DSC, expand names */
    new_desc->dsc_c_version = old_desc->dsc_c_version;
    new_desc->dsc_version = old_desc->dsc_version;
    new_desc->dsc_status = old_desc->dsc_status & ~DSC_NOTUSED;
    new_desc->dsc_open_count = old_desc->dsc_open_count;
    new_desc->dsc_sync_flag = old_desc->dsc_sync_flag;
    new_desc->dsc_type = old_desc->dsc_type;
    new_desc->dsc_access_mode = old_desc->dsc_access_mode;
    new_desc->dsc_dbid = old_desc->dsc_dbid;
    new_desc->dsc_sysmod = old_desc->dsc_sysmod;
    new_desc->dsc_ext_count = old_desc->dsc_ext_count;
    new_desc->dsc_last_table_id = old_desc->dsc_last_table_id;
    new_desc->dsc_dbaccess = old_desc->dsc_dbaccess;
    new_desc->dsc_dbservice = old_desc->dsc_dbservice;
    new_desc->dsc_dbcmptlvl = old_desc->dsc_dbcmptlvl;
    new_desc->dsc_1dbcmptminor = old_desc->dsc_1dbcmptminor;
    new_desc->dsc_patchcnt = old_desc->dsc_patchcnt;
    new_desc->dsc_1st_patch = old_desc->dsc_1st_patch;
    new_desc->dsc_last_patch = old_desc->dsc_last_patch;
    new_desc->dsc_inconst_code = old_desc->dsc_inconst_code;
    new_desc->dsc_inconst_time = old_desc->dsc_inconst_time;
    new_desc->dsc_iirel_relprim = old_desc->dsc_iirel_relprim;
    new_desc->dsc_iirel_relpgsize = old_desc->dsc_iirel_relpgsize;
    new_desc->dsc_iiatt_relpgsize = old_desc->dsc_iiatt_relpgsize;
    new_desc->dsc_iiind_relpgsize = old_desc->dsc_iiind_relpgsize;
    new_desc->dsc_iirel_relpgtype = old_desc->dsc_iirel_relpgtype;
    new_desc->dsc_iiatt_relpgtype = old_desc->dsc_iiatt_relpgtype;
    new_desc->dsc_iiind_relpgtype = old_desc->dsc_iiind_relpgtype;
    MEfill(sizeof(new_desc->dsc_extra), 0, &new_desc->dsc_extra);
    MEfill(sizeof(new_desc->dsc_extra1), 0, &new_desc->dsc_extra1);

    /* Copy the name and owner into the expanded fields. */
    MEmove(sizeof(old_desc->dsc_name), (PTR)&old_desc->dsc_name,
    	' ', sizeof(new_desc->dsc_name), (PTR)&new_desc->dsc_name);

    MEmove(sizeof(old_desc->dsc_owner), (PTR)&old_desc->dsc_owner,
    	' ', sizeof(new_desc->dsc_owner), (PTR)&new_desc->dsc_owner);

    MEmove(sizeof(old_desc->dsc_collation), (PTR)&old_desc->dsc_collation,
    	' ', sizeof(new_desc->dsc_collation), (PTR)&new_desc->dsc_collation);

    MEmove(sizeof(old_desc->dsc_ucollation), (PTR)&old_desc->dsc_ucollation,
    	' ', sizeof(new_desc->dsc_ucollation), (PTR)&new_desc->dsc_ucollation);


    /*
    ** Build new JNL info from old JNL struct
    */
    new_jnl = (DM0C_JNL *)(new_desc + 1);
    MEcopy((PTR)old_jnl, old_jnl->length, (PTR)new_jnl);
    new_jnl->length = sizeof(DM0C_JNL);

    /*
    ** Build new DMP info from old JNL struct
    ** Set la_block to zero
    */
    new_dmp = (DM0C_DMP *)(new_jnl + 1);
    MEcopy((PTR)old_dmp, old_dmp->length, (PTR)new_dmp);
    new_dmp->length = sizeof(DM0C_JNL);

    /*
    ** At the time of writing of this code ...
    ** New ext is same as old extent except ... sizeof location name
    ** since DB_MAXNAME has changed
    */
    new_ext = (DM0C_EXT *)(new_dmp + 1);
    for (i = 0; i < new_desc->dsc_ext_count; i++, old_ext++, new_ext++)
    {
	new_ext->length = sizeof(DM0C_EXT);
	new_ext->type = old_ext->type;
	new_ext->extra[0] = old_ext->extra[0];
	new_ext->extra[1] = old_ext->extra[1];
	new_ext->ext_location.raw_start = new_ext->ext_location.raw_start;
	new_ext->ext_location.raw_blocks = new_ext->ext_location.raw_blocks;
	new_ext->ext_location.raw_total_blocks =
		new_ext->ext_location.raw_total_blocks;

	/* Copy the location name into the expanded fields */
	MEmove(sizeof(old_ext->ext_location.logical), 
		(PTR)&old_ext->ext_location.logical, 
		' ', sizeof(new_ext->ext_location.logical),
		(PTR)&new_ext->ext_location.logical);
	new_ext->ext_location.flags = old_ext->ext_location.flags;
	new_ext->ext_location.phys_length =
				    old_ext->ext_location.phys_length;
	STRUCT_ASSIGN_MACRO (
	   old_ext->ext_location.physical, new_ext->ext_location.physical); 
    }

    /* Update cnf control block info. */

    end_ptr = (char *)new_ext + sizeof(DM0C_EXT);
    cnf->cnf_free_bytes = cnf->cnf_bytes - (end_ptr - cnf->cnf_data);
    cnf->cnf_c_ext = new_desc->dsc_ext_count;
    cnf->cnf_jnl = new_jnl;
    cnf->cnf_dmp = new_dmp;
    cnf->cnf_dsc = new_desc;
    cnf->cnf_ext = (DM0C_EXT *)(new_dmp + 1);

    /*
    ** Now turn off journaling on database so that upgradedb changes will
    ** not be journaled.
    */
    new_desc->dsc_status &= ~DSC_JOURNAL;

    /* Deallocate memory used to hold config copy. */

    dm0m_deallocate(&cnf_buffer);

    return (E_DB_OK);
}



/*{
** Name: dm0c_test_upgrade	- Performs basic testing of config upgrade
**
** Description:
**      This routine creates a dummy cnf with version CNF_V2 and then
**      calls dm0c_upgrade_cnf(). This routine only gets called when
**      II_UPGRADEDB_DEBUG=config is set before starting the dbms server
**      and connecting with upgradedb connect options to a database named
**      test_config_upgrade_db
** 
**          sql test_config_upgrade_db -u\$ingres +U -A6
**
** Inputs:
**      cnf
**      cnf_version
**
** Outputs:
**      cnf_version
**
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
**      21-apr-2009 (stial01)
**          Created for basic upgradedb testing connecting to CNF_VCURRENT db
** 
*/
static DB_STATUS
dm0c_test_upgrade(
DMP_DCB		*dcb,
i4		cnf_bytes,
DB_ERROR	*dberr)
{
    DM_OBJECT		*cnf_buffer;
    DM0C_CNF		config;
    DM0C_CNF		*cnf;
    DB_STATUS		status;
    struct
    {
	DM0C_ODSC	odesc;
	DM0C_V3JNL	ojnl;
	DM0C_TAB	rel;
	DM0C_TAB	relx;
	DM0C_TAB	att;
	DM0C_TAB	idx;
    } v2cnf;
    i4			cnf_version;
    DM0C_ODSC		*odesc;
    DM0C_V3JNL		*ojnl;
    i4			*err_code = &dberr->err_code;
    char		*end_ptr;

    cnf = &config;
    cnf->cnf_dcb = dcb;
    cnf->cnf_di = NULL; /* not going to write it */
    cnf->cnf_ext = 0;
    cnf->cnf_c_ext = 0;
    cnf->cnf_lkid.lk_unique = 0;
    cnf->cnf_lkid.lk_common = 0;

    /* Init a config version CNF_V2 and try to upgrade to current */
    MEfill(sizeof(v2cnf), 0, &v2cnf);
    cnf_version = CNF_V2;

    /* Init DM0C_ODSC enough to run through config file upgrades */
    odesc = &v2cnf.odesc;
    odesc->length = sizeof(*odesc);
    odesc->type = DM0C_T_ODSC;
    odesc->dsc_status = DSC_VALID;
    odesc->dsc_open_count = 0;
    odesc->dsc_sync_flag = 0;
    MEmove(4, "test", ' ', sizeof(odesc->dsc_name), (PTR)&odesc->dsc_name);
    MEmove(7, "$ingres", ' ', sizeof(odesc->dsc_owner), (PTR)&odesc->dsc_owner);
    odesc->dsc_type = DSC_PUBLIC;
    odesc->dsc_access_mode = DSC_WRITE;
    odesc->dsc_c_version = DSC_V2;
    odesc->dsc_version = DSC_V2;
    odesc->dsc_dbid = 1;
    odesc->dsc_sysmod = 0;
    odesc->dsc_last_table_id = 200;
    odesc->dsc_cnf_version = CNF_V2;
    odesc->dsc_dbaccess = 0; /* FIX ME */
    odesc->dsc_dbservice = 0; /* FIX ME */
    odesc->dsc_dbcmptlvl = DU_3DBV63; /* ancient enough for you? */
    odesc->dsc_1dbcmptminor = 0;
    odesc->dsc_patchcnt = 0;
    odesc->dsc_1st_patch = 0;
    odesc->dsc_last_patch = 0;
    odesc->dsc_inconst_code = 0;
    odesc->dsc_inconst_time = 0;
    MEfill(sizeof(odesc->dsc_collation), ' ',odesc->dsc_collation);

    /* Init DM0C_V3JNL it's mostly just zeroes from MEfill above */
    ojnl = &v2cnf.ojnl;
    ojnl->length = sizeof(*ojnl);
    ojnl->type = DM0C_T_JNL;

    /* init relprim in core catalog tuples in the config */
    v2cnf.rel.tab_relation.relprim = 16;
    v2cnf.relx.tab_relation.relprim = 16;
    v2cnf.att.tab_relation.relprim = 16;
    v2cnf.idx.tab_relation.relprim = 16;

    /*
    ** Allocate buffer and copy v2cnf to the buffer
    ** Make sure the cnf buffer is big enough for the upgrade
    ** so this test never tries to dm0c_extend which this dummy cnf
    ** is cannot handle
    */
    cnf_bytes += sizeof(DM0C_DSC) + sizeof(DM0C_JNL) + sizeof(DM0C_DMP) + 
	sizeof(DM0C_EXT);
    cnf_bytes *= 2;

    status = dm0m_allocate(cnf_bytes + sizeof(DMP_MISC),
	(i4) 0, (i4)MISC_CB, (i4)MISC_ASCII_ID,
	(char *) cnf, &cnf_buffer, dberr);
    if (status != E_DB_OK)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM928C_CONFIG_CONVERT_ERROR);
	return (E_DB_ERROR);
    }

    cnf->cnf_data = (char *)cnf_buffer + sizeof(DMP_MISC);
    ((DMP_MISC *)cnf_buffer)->misc_data = cnf->cnf_data;
    end_ptr = (char *)cnf->cnf_data + sizeof(v2cnf);
    cnf->cnf_bytes = cnf_bytes;
    cnf->cnf_free_bytes = cnf->cnf_bytes - (end_ptr - cnf->cnf_data);
    MEcopy((PTR)&v2cnf, sizeof(v2cnf), cnf->cnf_data);

    status = dm0c_upgrade_config(cnf, &cnf_version, dberr);

    dm0m_deallocate(&cnf_buffer);
}
