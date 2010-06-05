/*
**Copyright (c) 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <tm.h>
#include    <di.h>
#include    <er.h>
#include    <pc.h>
#include    <tr.h>
#include    <cs.h>
#include    <sl.h>
#include    <si.h>
#include    <st.h>
#include    <me.h>
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
#include    <dmfjsp.h>
#include    <sxf.h>
#include    <dma.h>

/**
**
**  Name: DMFINFO.C - DMF Information.
**
**  Description:
**
**	Funtion prototypes defined in DMFJSP.H.
**
**      This file contains the routines that display information about a 
**	database.
**
**          dmfinfo - Database information.
**
**  History:
**      08-nov-1986 (Derek)
**          Created for Jupiter.
**	03-feb-1989 (EdHsu)
**	    Updated for online backup.
**	13-oct-1989 (Sandyh)
**	    Updated in conjunction w/ fix for bug 8352.
**	17-may-90 (bryanp)
**	    Display the new DUMP_DIR_EXISTS status in the database descriptor.
**      25-feb-1991 (rogerk)
**          Added check for JOURNAL_DISABLED status.  Also added some
**          comment messages for some database states.  These were added
**          for the Archiver Stability project.
**      25-mar-1991 (rogerk)
**          Added checks for NOLOGGING status and nologging inconsistency types.
**      30-apr-1991 (bryanp)
**          Support trace processing ("#x").
**      15-aug-1991 (jnash) merged 24-sep-1990 (rogerk)
**          Merged 6.3 changes into 6.5.
**	8-july-1992 (ananth)
**	    Prototyping DMF.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project: Changed database inconsistent codes.
**	18-jan-1993 (rogerk)
**	    Fixed problem with reporting inconsistency codes added in last
**	    integration.  Shifted TRdisplay vector over one to get correct
**	    output.
**	30-feb-1993 (rmuth)
**	    Include di.h for dm0c.h
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system type definitions.
**	24-may-1993 (jnash)
**	    Show log address in <%d:%d:%d> format.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**	20-sep-1993 (bryanp)
**	    Fix TRdisplay calls to line up properly with DB_MAX_NAME=32.
**	20-sep-1993 (jnash)
**	    Fix problem where display of dump information not performed
**	    due to incorrect version check.  Also, include config file version 
**	    database version in infodb output.
**	18-oct-1993 (rmuth)
**	    CL prototype, include <si.h>
**	18-oct-93 (jrb)
**	    Added informational line to say when the db is NOT journaled.
**	22-nov-1993 (jnash)
**	    B53797.  Apply Walt's 6.4 VMS fix where if a database never 
**	    checkpointed, INFODB AVs.  Use CL_OFFSETOF rather than zero-pointer
**          construction in TRdisplay statement.  Bug caused by compiler
**          bug when dmfinfo.c containing original expression was compiled
**          with /opt.
**	28-jan-1994 (kbref)
**	    B57199. Display the database status flag DSC_CKP_INPROGRESS as
**	    "CKP_INPROGRESS" instead of "CKP".
**	    B56095. Add display of earliest checkpoint which journals may
**	    may be applied to (jnl_first_jnl_seq).
**      15-feb-1994 (andyw)
**          Modified dmfinfo to check for checkpoint sequence number of
**          journals and dumps instead of using DSC_CKP_INPROGRESS.
**      15-feb-1994 (andyw)
**          Modified the dump log address incorporating the standard
**          address format <a:b:c> bug reference 58553.
**      14-mar-94 (stephenb)
**          Add new parameter to dma_write_audit() to match current prototype.
**	25-apr-1994 (bryanp) B62023
**	    Used CL_OFFSETOF to format and display the components of the
**		checkpoint history and dump history. The old technique using
**		casts of null pointers doesn't work on the Alpha.
**      13-Dec-1994 (liibi01) 
**          Cross integration from 6.4 (Bug 56364).
**          Added new inconsistency class instance RFP_FAIL as
**          side effect of fix to bug 56364.
**      24-jan-1995 (stial01)
**          BUG 66473: display if checkpoint is table checkpoint
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**	21-aug-95 (tchdi01)
**	    Modified the infodb output to display the mode of the database:
**	    production or development
**	12-sep-1995 (thaju02)
**	    Added routine output_tbllist().  Implementation of
**	    sending to output the contents of the checkpoint table list
**	    file.
**	16-sep-1995 (canor01)
**	    Added <me.h>
**	 6-feb-1996 (nick)
**	    Add prototype for output_tbllist(), add missing parameter in call
**	    to it which yielded a SIGBUS, move place of calling it and 
**	    finally rewrite the function.
**	12-mar-96 (nick)
**	    'Next' should be 'Last' for table id. #48797
**	    Check for checkpointed database and existing sequence number
**	    when running with '#c'. #75347, #75348.
**	29-mar-96 (nick)
**	    DSC_CKP_INPROGRESS does not mean that a checkpoint is in 
**	    progress - it means that you must perform a rollforward +c 
**	    ( i.e. back to saveset ) BEFORE you can do a rollforward -c +j.
**	17-may-96 (nick)
**	    RFP_FAIL wasn't being printed correctly ; it was in the wrong
**	    position in the TRformat().
**	24-oct-96 (nick)
**	    LINT stuff.
**      08-Dec-1997 (hanch04)
**          Use new la_block field when block number is needed instead
**          of calculating it from the offset
**          Keep track of block number and offset into the current block.
**          for support of logs > 2 gig
**      21-apr-1999 (hanch04)
**	    Added st.h
**	07-aug-2000 (somsa01)
**	    In dmfinfo(), when printing out the table list, we were
**	    incorrectly passing an argument to TRformat().
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to dma_write_audit() if not C2SECURE.
**      01-sep-2004 (stial01)
**	    Support cluster online checkpoint
**      12-apr-2007 (stial01)
**          Added support for rollforwarddb -incremental
**	12-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	18-Nov-2008 (jonj)
**	    dm0c_? functions converted to DB_ERROR *
**	08-Dec-2008 (jonj)
**	    SIR 120874: dma_? auditing functions converted to DB_ERROR *
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

/*
**  Static functions.
*/

static	DB_STATUS	output_tbllist(
    DMF_JSX         *jsx,
    DMP_DCB         *dcb,
    DM0C_CNF	    *cnf);


/*{
** Name: dmfinfo	- Database information display.
**
** Description:
**
** Inputs:
**      journal_context			Pointer to DMF_JSX
**	dcb				Pointer to DCB.
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
**      01-nov-1986 (Derek)
**          Created for Jupiter.
**	03-feb-1989 (EdHsu)
**	    Updated for online backup.
**	09-apr-1990 (Sandyh)
**	    Added inconsistency reason.
**	17-may-90 (bryanp)
**	    Display the new DUMP_DIR_EXISTS status in the database descriptor.
**      25-feb-1991 (rogerk)
**          Added check for JOURNAL_DISABLED status.  Also added some
**          comment messages for some database states.  These were added
**          for the Archiver Stability project.
**      25-mar-1991 (rogerk)
**          Added checks for NOLOGGING status and nologging inconsistency types.
**      30-apr-1991 (bryanp)
**          Support trace processing ("#x").
**      8-nov-1992 (ed)
**          remove DB_MAXNAME dependency
**	04-nov-92 (jrb)
**	    Changed "SORT" to "WORK" for multi-location sorts project.
**	30-nov-92 (robf)
**	     Add C2 security auditing.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project: Changed database inconsistent codes.
**	18-jan-1993 (rogerk)
**	    Fixed problem with reporting inconsistency codes added in last
**	    integration.  Shifted TRdisplay vector over one to get correct
**	    output.
**	24-may-1993 (jnash)
**	    Show last journaled log address in <%d:%d:%d> format.
**	20-sep-1993 (bryanp)
**	    Fix TRdisplay calls to line up properly with DB_MAX_NAME=32.
**	20-sep-1993 (jnash)
**	    Fix problem where dump information not presented.  Also include
**	    version information in infodb output.
**	14-oct-93 (jrb)
**	    Added informational line to say when the db is NOT journaled.
**	22-nov-1993 (jnash)
**	    B53797.  Apply Walt's 6.4 VMS fix where if a database never 
**	    checkpointed, INFODB AVs.  Use CL_OFFSETOF rather than zero-pointer
**          construction in TRdisplay statement.  Bug caused by compiler
**          bug when dmfinfo.c containing original expression was compiled
**          with /opt.
**      15-feb-1994 (andyw)
**          Modified dmfinfo to check for checkpoint sequence number of
**          journals and dumps instead of using DSC_CKP_INPROGRESS.
**      15-feb-1994 (andyw)
**          Modified the dump log address incorporating the standard
**          address format <a:b:c> bug reference 58553.
**	25-apr-1994 (bryanp) B62023
**	    Used CL_OFFSETOF to format and display the components of the
**		checkpoint history and dump history. The old technique using
**		casts of null pointers doesn't work on the Alpha.
**      13-Dec-1994 (liibi01)
**          Cross integration from 6.4 (Bug 56364).
**          Added new inconsistency class instance RFP_FAIL as
**          side effect of fix to bug 56364.
**      24-jan-1995 (stial01)
**          BUG 66473: display if checkpoint is table checkpoint
**      12-sep-1995 (thaju02)
**          Added routine output_tbllist().  Implementation of 
**          sending to output the contents of the checkpoint table list
**          file.
**	 6-feb-1996 (nick)
**	    Call to output_tbllist() was missing the err_code param - this
**	    caused an access violation if output_tbllist() tried to set it.
**	    Moved call to output_tbllist() to a) ensure we security audit
**	    the operation, b) ensure the checkpoint in question exists and
**	    c) the output format looks like normal 'infodb'.
**	12-mar-96 (nick)
**	    'Next table id' is actually the last one.
**	29-mar-96 (nick)
**	    Change CKP_INPROGRESS back to CKP.
**	17-may-96 (nick)
**	    Move RFP_FAIL in the TRformat().
**	28-feb-1997 (angusm)
**	    Output collation sequence defined for database. (SIR 72251)
**	07-aug-2000 (somsa01)
**	    When printing out the table list, we were incorrectly passing
**	    an argument to TRformat().
**	12-apr-2005 (gupsh01)
**	    Added support for unicode information.
**      25-oct-2005 (stial01)
**          New db inconsistency codes DSC_INDEX_INCONST, DSC_TABLE_INCONST
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Utilize DSC_STATUS, DSC_INCONST_CODE defines.
**	05-Mar-2010 (frima01) SIR 121619
**	    For usability explicitly state that MVCC is enabled.
[@history_template@]...
*/
DB_STATUS
dmfinfo(
DMF_JSX		    *jsx,
DMP_DCB		    *dcb)
{
    DM0C_CNF		*config;
    DM0C_CNF		*cnf;
    i4		i;
    i4		length;
    DB_STATUS		status;
    DB_STATUS		local_status;
    STATUS		cl_status;
    bool                jnode_info = FALSE;
    bool		dnode_info = FALSE;
    char		line_buffer[132];
    CL_ERR_DESC         sys_err;
    SXF_ACCESS		access;
    LG_HEADER		log_hdr;
    i4			local_error;
    DB_ERROR		local_dberr;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);


    if (jsx->jsx_status & JSX_TRACE) 
	TRset_file(TR_F_OPEN, "infodb.dbg", 10, &sys_err);

    /*	Pretend the database is exclusive. */

    dcb->dcb_status |= DCB_S_EXCLUSIVE;

    /*	Lock the database. */

    status = dm0c_open(dcb, 0, dmf_svcb->svcb_lock_list, &config,
	&jsx->jsx_dberr);

    /*
    **	Make sure access is security audited.
    */
    if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
    {
	access = SXF_A_SELECT;
	if (status)
	    access |= SXF_A_FAIL;
	else
	    access |= SXF_A_SUCCESS;
	
	local_status = dma_write_audit( SXF_E_DATABASE,
		    access,
		    dcb->dcb_name.db_db_name, /* Object name (database) */
		    sizeof(dcb->dcb_name.db_db_name), /* Object name (database) */
		    &dcb->dcb_db_owner, /* Object owner */
		    I_SX2711_INFODB,   /*  Message */
		    TRUE,		    /* Force */
		    &local_dberr, NULL);

	if (local_status != E_DB_OK)
	{
	    jsx->jsx_dberr = local_dberr;
	    status=local_status;
	}
    }

    if (status != E_DB_OK)
	return (status);
    cnf = config;

    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"%18*=%@ Database Information%17*=\n\n");
    if (STcompare("                                ",
		  cnf->cnf_dsc->dsc_collation)==0)
	STcopy("default", cnf->cnf_dsc->dsc_collation);
    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"    Database : (%~t,%~t)  ID : 0x%x  Default collation : %~t\n",
	sizeof(cnf->cnf_dsc->dsc_name), &cnf->cnf_dsc->dsc_name,
	sizeof(cnf->cnf_dsc->dsc_owner), &cnf->cnf_dsc->dsc_owner,
        cnf->cnf_dsc->dsc_dbid,
	sizeof(cnf->cnf_dsc->dsc_collation) ,&cnf->cnf_dsc->dsc_collation);

    /* Provide Unicode support information here */
    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"    Unicode enabled : %s\n",
	cnf->cnf_dsc->dsc_dbservice & DU_UTYPES_ALLOWED ? "Yes" : "No");

    if (cnf->cnf_dsc->dsc_dbservice & DU_UTYPES_ALLOWED)
    {	
      TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
      "    Default unicode collation : %~t \t Unicode normalization : %s\n", 
	sizeof(cnf->cnf_dsc->dsc_ucollation) ,&cnf->cnf_dsc->dsc_ucollation,
        (cnf->cnf_dsc->dsc_dbservice & DU_UNICODE_NFC) ? "NFC" : "NFD");
    }

    if (jsx->jsx_status1 & JSX1_OUT_FILE)
    {
        status = output_tbllist(jsx, dcb, cnf);
    	(void)dm0c_close(cnf, 0, &local_dberr);
        return(status);
    }

    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"    Extents  : %d    Last Table Id : %d\n",
	cnf->cnf_dsc->dsc_ext_count, cnf->cnf_dsc->dsc_last_table_id);
    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"    Config File Version Id : 0x%x   Database Version Id : %d\n", 
	cnf->cnf_dsc->dsc_cnf_version, cnf->cnf_dsc->dsc_c_version);

    /*
    ** Show mode of operation: production on | off 
    **                         online checkpoint enabled | disabled
    */
    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"    Mode     : DDL %s, ONLINE CHECKPOINT %s\n",
	cnf->cnf_dsc->dsc_dbaccess & DU_PRODUCTION ? "DISALLOWED" : "ALLOWED",
	cnf->cnf_dsc->dsc_dbaccess & DU_NOBACKUP ? "DISABLED" : "ENABLED");

    /*
    ** The flag is 'DSC_DUMP_DIR_EXISTS', but we report it as 'CFG_BACKUP',
    ** since its current use is to enable config file auto-backup.
    */
    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
        "    Status   : %v\n\n", DSC_STATUS, cnf->cnf_dsc->dsc_status);

    /*
    ** Print database status information comments.
    */

    if (!(cnf->cnf_dsc->dsc_status & DSC_VALID))
    {
        TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
            "%15* The Database is Inconsistent.\n");
        TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
            "%19* Cause of Inconsistency:  %w\n",
            DSC_INCONST_CODE, cnf->cnf_dsc->dsc_inconst_code);  
    }

    if (cnf->cnf_jnl->jnl_ckp_seq != 0)
    {
        TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
            "%15* The Database has been Checkpointed.\n");
    }

    if (cnf->cnf_dsc->dsc_status & DSC_JOURNAL)
    {
        TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
            "%15* The Database is Journaled.\n");
    }
    else
    {
        TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
            "%15* The Database is not Journaled.\n");
    }


    if (cnf->cnf_dsc->dsc_status & DSC_DISABLE_JOURNAL)
    {
        TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
          "\n%15* Journaling has been disabled on this database by alterdb.\n");
        TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
          "%15* Run 'ckpdb +j' to re-enable journaling.\n\n");
    }

    if (cnf->cnf_dsc->dsc_dbid != IIDBDB_ID)
    {
        TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
          "\n%15* MVCC is %s in this database.\n",
          cnf->cnf_dsc->dsc_status & DSC_NOMVCC ? "disabled" : "enabled");
    }

    if (cnf->cnf_dsc->dsc_status & DSC_NOLOGGING)
    {
        TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
          "\n%15* Database is being accessed with Set Nologging, allowing\n");
        TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
          "%15* transactions to run while bypassing the logging system.\n");
    }


    /*
    ** Show the earliest checkpoint which journals may be applied to.
    */

    if (cnf->cnf_jnl->jnl_first_jnl_seq != 0)
    {
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "\n%15* Journals are valid from checkpoint sequence : %d\n",
	    cnf->cnf_jnl->jnl_first_jnl_seq);
    }
    else
    {
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "\n%15* Journals are not valid from any checkpoint.\n");
    }

    /* Show the log header for log address calculation */

    cl_status = LGshow(LG_A_HEADER, (PTR)&log_hdr, sizeof(log_hdr),
		    &length, &sys_err);
    if (cl_status)
    {
	SIprintf("Can't show log header, error %d.\n", cl_status);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM9017_BAD_LOG_SHOW);
	return(E_DB_ERROR);
    }

    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer), "\n");
    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
        "----Journal information%57*-\n");
    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"    Checkpoint sequence : %10d    Journal sequence : %17d\n",
	cnf->cnf_jnl->jnl_ckp_seq, cnf->cnf_jnl->jnl_fil_seq);
    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"    Current journal block : %8d    Journal block size : %15d\n",
	cnf->cnf_jnl->jnl_blk_seq, cnf->cnf_jnl->jnl_bksz);
    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"    Initial journal size : %9d    Target journal size : %14d\n",
	cnf->cnf_jnl->jnl_blkcnt, cnf->cnf_jnl->jnl_maxcnt);
    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"    Last Log Address Journaled : <%d:%d:%d>\n",
	cnf->cnf_jnl->jnl_la.la_sequence, 
	cnf->cnf_jnl->jnl_la.la_block,
	cnf->cnf_jnl->jnl_la.la_offset);

    if (cnf->cnf_dsc->dsc_cnf_version >= CNF_V3)
    {
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "----Dump information%60*-\n"); 
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "    Checkpoint sequence : %10d    Dump sequence : %20d\n",
	    cnf->cnf_dmp->dmp_ckp_seq, cnf->cnf_dmp->dmp_fil_seq);
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "    Current dump block : %11d    Dump block size : %18d\n",
	    cnf->cnf_dmp->dmp_blk_seq, cnf->cnf_dmp->dmp_bksz);
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "    Initial dump size : %12d    Target dump size : %17d\n",
	    cnf->cnf_dmp->dmp_blkcnt, cnf->cnf_dmp->dmp_maxcnt);
        TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
            "    Last Log Address Dumped : <%d:%d:%d>\n",
            cnf->cnf_dmp->dmp_la.la_sequence,
            cnf->cnf_dmp->dmp_la.la_block,
            cnf->cnf_dmp->dmp_la.la_offset);
    }


    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"----Checkpoint History for Journal%46*-\n");
    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"    Date                      Ckp_sequence  First_jnl   Last_jnl  valid  mode\n    %76*-\n");
    if (cnf->cnf_jnl->jnl_count)
    {
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "%#.#{    %?    %11d  %9d  %8d  %5d  %v\n%}",
	    cnf->cnf_jnl->jnl_count, sizeof(struct _JNL_CKP), cnf->cnf_jnl->jnl_history,
	    CL_OFFSETOF(struct _JNL_CKP, ckp_date),
	    CL_OFFSETOF(struct _JNL_CKP, ckp_sequence),
	    CL_OFFSETOF(struct _JNL_CKP, ckp_f_jnl),
	    CL_OFFSETOF(struct _JNL_CKP, ckp_l_jnl),
	    CL_OFFSETOF(struct _JNL_CKP, ckp_jnl_valid), "OFFLINE,ONLINE,TABLE",
	    CL_OFFSETOF(struct _JNL_CKP, ckp_mode));
    }
    else
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "    None.\n");

    if (cnf->cnf_dsc->dsc_cnf_version >= CNF_V3)
    {
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "----Checkpoint History for Dump%49*-\n");
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "    Date                      Ckp_sequence  First_dmp   Last_dmp  valid  mode\n    %76*-\n");
	if (cnf->cnf_dmp->dmp_count)
	{
	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%#.#{    %?    %11d  %9d  %8d  %5d  %v\n%}",
		cnf->cnf_dmp->dmp_count, sizeof(struct _DMP_CKP), cnf->cnf_dmp->dmp_history,
		CL_OFFSETOF(struct _DMP_CKP, ckp_date),
		CL_OFFSETOF(struct _DMP_CKP, ckp_sequence),
		CL_OFFSETOF(struct _DMP_CKP, ckp_f_dmp),
		CL_OFFSETOF(struct _DMP_CKP, ckp_l_dmp),
		CL_OFFSETOF(struct _DMP_CKP, ckp_dmp_valid), 
					"OFFLINE,ONLINE,TABLE",
		CL_OFFSETOF(struct _DMP_CKP, ckp_mode));
	}
	else
	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"    None.\n");
    }


    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"----Cluster Journal History%53*-\n");
    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"    Node ID   Current Journal   Current Block   Last Log Address\n    %60*-\n");
    for (i=0; i < DM_CNODE_MAX; i++)
    {
    	struct _JNL_CNODE_INFO *p = 0;
	p = &cnf->cnf_jnl->jnl_node_info[i];
	
	if (p->cnode_fil_seq || p->cnode_la.la_sequence)
        {	
	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"    %7d   %15d   %13d   %8x %8x %8x\n",
		i, p->cnode_fil_seq, p->cnode_blk_seq, 
		p->cnode_la.la_sequence,
		p->cnode_la.la_block,
		p->cnode_la.la_offset);
	    jnode_info = TRUE;
	}
    }
    if (jnode_info == FALSE)
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "    None.\n");

    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"----Cluster Dump History%53*-\n");
    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"    Node ID   Current Dump      Current Block   Last Log Address\n    %60*-\n");
    for (i=0; i < DM_CNODE_MAX; i++)
    {
    	struct _DMP_CNODE_INFO *p = 0;
	p = &cnf->cnf_dmp->dmp_node_info[i];
	
	if (p->cnode_fil_seq || p->cnode_la.la_sequence)
        {	
	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"    %7d   %15d   %13d   %8x %8x %8x\n",
		i, p->cnode_fil_seq, p->cnode_blk_seq, 
		p->cnode_la.la_sequence,
		p->cnode_la.la_block,
		p->cnode_la.la_offset);
	    dnode_info = TRUE;
	}
    }
    if (dnode_info == FALSE)
	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "    None.\n");

    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"----Extent directory%60*-\n");
    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
    "    Location                          Flags             Physical_path\
\n    %66*-\n");

    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"%#.#{    %#.#s  %16v  %.*s\n%}", 
	cnf->cnf_dsc->dsc_ext_count, sizeof(DM0C_EXT), cnf->cnf_ext,
	sizeof(DB_LOC_NAME), sizeof(DB_LOC_NAME), 
	CL_OFFSETOF(DM0C_EXT, ext_location.logical),
	"ROOT,DATA,JOURNAL,CHECKPOINT,ALIAS,WORK,DUMP,AWORK",
	CL_OFFSETOF(DM0C_EXT, ext_location.flags),
	CL_OFFSETOF(DM0C_EXT, ext_location.phys_length),
	CL_OFFSETOF(DM0C_EXT, ext_location.physical));

    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"%80*=\n");

    if (jsx->jsx_status & JSX_TRACE)
    {
        TRdisplay("%@ INFODB: Config file:\n");
        dump_cnf(cnf);
    }

    /*	Update and close the configuration file. */

    status = dm0c_close(cnf, 0, &jsx->jsx_dberr);

    return (status);
}

/*
** Name: output_tbllist - Output  list of the tables in checkpoint
**
** Description:
**	Print all user tables saved in a given checkpoint.
**	
**
** Inputs:
**      jsx                             Pointer to Journal support info.
**      dcb                             Pointer to DCB for this database.
**	cnf				Pointer to config file information.
**
** Returns:
**      E_DB_OK
**      E_DB_ERROR
** Exceptions:
**      none
**
** Side Effects:
**
** History:
**      11-sep-1995 (thaju02)
**          Created.
**	 6-feb-1996 (nick)
**	    Rewritten.
**	12-jan-96 (nick)
**	    Check database has been checkpointed, use last sequence number
**	    if none specified and check the sequence number exists.
**	    Fixes #75347 and #75348.
**      26-Sep-2008 (ashco01) Bug:120558
**          'break' if list of tables in checkpoint completes within current
**          block to prevent spurious text being appended to the table list output.
*/
static DB_STATUS
output_tbllist(
DMF_JSX         *jsx,
DMP_DCB         *dcb,
DM0C_CNF	*cnf)
{
    DB_TAB_NAME         *table_list;
    TBLLST_CTX		tblctx;
    DB_STATUS           status = E_DB_OK;
    DB_STATUS		local_status;
    char		line_buffer[132];
    i4		local_err;
    i4		i;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    tblctx.seq = jsx->jsx_ckp_number;
    tblctx.blkno = -1;
    tblctx.blksz = TBLLST_BKZ;
    tblctx.phys_length = dcb->dcb_dmp->phys_length;
    STRUCT_ASSIGN_MACRO(dcb->dcb_dmp->physical, tblctx.physical);
    table_list = (DB_TAB_NAME *)&tblctx.tbuf;

    do
    {
    	if (cnf->cnf_jnl->jnl_ckp_seq == 0)
	{
            TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
        	"\n%15* The Database has not been Checkpointed.\n");
    	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%80*=\n");
	    break;
	}

	/* last checkpoint if we didn't specify a particular one */
    	if (tblctx.seq == 0)
	    tblctx.seq = cnf->cnf_jnl->jnl_ckp_seq;

	/*
	** check checkpoint sequence number selected
	** exists ( at least in the config file history )
	*/
	for (i = 0; i < cnf->cnf_jnl->jnl_count; i++)
	    if (cnf->cnf_jnl->jnl_history[i].ckp_sequence == tblctx.seq)
		break;

	if (i == cnf->cnf_jnl->jnl_count)
	{
	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
        	"\n%15* Checkpoint Sequence %d does not exist.\n",
		tblctx.seq);
    	    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"%80*=\n");
	    break;
	}

	status = tbllst_open(jsx, &tblctx);
	if (status != E_DB_OK)
	{
	    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char * )NULL, 0L, (i4 *)NULL, &local_err, 0);
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1365_RFP_CHKPT_LST_OPEN);
	    break;
	}
    	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
		"\n----Checkpoint Sequence %d contains:%43*-\n", tblctx.seq);

	while (status == E_DB_OK)
	{
	    status = tbllst_read(jsx, &tblctx);
	    if (status != E_DB_OK)
	    {
		if (status == E_DB_WARN)
		{
		    status = E_DB_OK;
		    CLRDBERR(&jsx->jsx_dberr);
		    break;
		}
		else
		{
	    	    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
			(char * )NULL, 0L, (i4 *)NULL, &local_err, 0);
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM1366_RFP_CHKPT_LST_READ);
		}
	    }
	    else
	    {
	    	for (i = 0; (i < TBLLST_MAX_TAB) && 
		    	(MEcmp((char *)&table_list[i],"\0", 1) != 0); i++)
		    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
			"	%t\n", DB_TAB_MAXNAME, &table_list[i]);
	    }
            if (i < TBLLST_MAX_TAB)
            {
                /* End of list reached */
                break;
            }
	}

    	TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	    "%80*=\n");

	local_status = tbllst_close(jsx, &tblctx);
	if (local_status != E_DB_OK)
	{
	    uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char * )NULL, 0L, (i4 *)NULL, &local_err, 0);
	    if (status == E_DB_OK)
	    {
		status = local_status;
		SETDBERR(&jsx->jsx_dberr, 0, E_DM1367_RFP_CHKPT_LST_CLOSE);
	    }
	}
    } while (FALSE);

    if (DB_FAILURE_MACRO(status))
    {
	uleFormat(&jsx->jsx_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char * )NULL, 0L, (i4 *)NULL, &local_err, 1,
		sizeof(DM_FILE), tblctx.filename.name);
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1059_JSP_INFODB_ERR);
    }
    return(status);
}
