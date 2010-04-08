
/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
#include    <compat.h>
#include    <dbms.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <cs.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dm2d.h>
#include    <dm2t.h>
#include    <dm2r.h>
#include    <dm0c.h>
#include    <dmfjsp.h>
#include    <tr.h>
*/
#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <tm.h>
#include    <di.h>
#include    <er.h>
#include    <pc.h>
#include    <tr.h>
#include    <cs.h>
#include    <sl.h>
#include    <si.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
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
 

/*
 *  dmfinfo1.c - Dbalert version
 *
 *  Modified by Jeremy Hall - Jan 1995
 */

/*
 *  Manifests for DBalert.
 */

#define private    static
#define public     /* Empty */
#define elif       else if
#define equal(a,b) (strcmp( a, b ) == 0)

private char dbalert_unknown[] = "UNKNOWN UNKNOWN";


#define INCON_ERRORS  11

#ifdef ANSI
private const char * const incon_list[] =
#else
private char * incon_list[] =
#endif
    {
    "VALID_CLR",
    "ABORT_DCB",
    "ABORT_NODCB",
    "RCP2_DCB",
    "RCP2_NODCB",
    "OPEN_CNT",
    "REDO_FAIL",
    "WC_FAIL",
    "NOLOGGING_ERROR",
    "NOLOGGING_OPENDB",
    "RFP_FAIL"
    };


/**
**
**  Name: DMFINFO.C - DMF Information.
**
**  Description:
**      This file contains the routines that display information about a
**      database.
**
**          dmfinfo - Database information.
**
**  History:    $Log-for RCS$
**      08-nov-1986 (Derek)
**          Created for Jupiter.
**      03-feb-1989 (EdHsu)
**          Updated for online backup.
**      13-oct-1989 (Sandyh)
**          Updated in conjunction w/ fix for bug 8352.
**      17-may-90 (bryanp)
**          Display the new DUMP_DIR_EXISTS status in the database descriptor.
**      25-feb-1991 (rogerk)
**          Added check for JOURNAL_DISABLED status.  Also added some
**          comment messages for some database states.  These were added
**          for the Archiver Stability project.
**      25-mar-1991 (rogerk)
**          Added checks for NOLOGGING status and nologging inconsistency types.
**      30-apr-1991 (bryanp)
**          Support trace processing ("#x").
**      10-apr-1992 (walt)
**          Fix bug 37300 by using CL_OFFSETOF rather than zero-pointer
**          construction in TRdisplay statement.  Bug caused by compiler
**          bug when dmfinfo.c containing original expression was compiled
**          with /opt.
**      17-aug-1994 (nick)
**          Fix bug 52830 - we were checking dsc_c_version ( i.e version
**          database was created at ) rather than dsc_version ( i.e. current
**          version of that database ) when deciding if dump information
**          should be printed.  Hence, upgraded databases created before
**          online checkpoints became available don't get their info. printed
**      21-sep-1994 (nick)
**          Added new inconsistency class instance RFP_FAIL as
**          side effect of fix to bug 56364.
**	27-feb-95 (angusm)
**	    Brought in line with CL.
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**	22-apr-1999 (hanch04)
**	    Added st.h and me.h
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STcopy
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	12-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	18-Nov-2008 (jonj)
**	    dm0c_? functions converted to DB_ERROR *
[@history_template@]...
**/

/*{
** Name: dmfinfo1        - Database information display.
**
** Description:
**
** Inputs:
**      journal_context                 Pointer to DMF_JSX
**      dcb                             Pointer to DCB.
**
** Outputs:
**      err_code                        Reason for error return status.
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
**      01-nov-1986 (Derek)
**          Created for Jupiter.
**      03-feb-1989 (EdHsu)
**          Updated for online backup.
**      09-apr-1990 (Sandyh)
**          Added inconsistency reason.
**      17-may-90 (bryanp)
**          Display the new DUMP_DIR_EXISTS status in the database descriptor.
**      25-feb-1991 (rogerk)
**          Added check for JOURNAL_DISABLED status.  Also added some
**          comment messages for some database states.  These were added
**          for the Archiver Stability project.
**      25-mar-1991 (rogerk)
**          Added checks for NOLOGGING status and nologging inconsistency types.
**      30-apr-1991 (bryanp)
**          Support trace processing ("#x").
**      10-apr-1992 (walt)
**          Fix bug 37300 by using CL_OFFSETOF rather than zero-pointer
**          construction in TRdisplay statement.  Bug caused by compiler
**          bug when dmfinfo.c containing original expression was compiled
**          with /opt.
**      17-aug-1994 (nick)
**          Fix bug 52830 - we were checking dsc_c_version ( i.e version
**          database was created at ) rather than dsc_version ( i.e. current
**          version of that database ) when deciding if dump information
**          should be printed.  Hence, upgraded databases created before
**          online checkpoints became available don't get their info. printed
**      21-sep-1994 (nick)
**          Added new inconsistency class instance RFP_FAIL as
**          side effect of fix to bug 56364.
**	27-feb-95 (angusm)
**	    Brought in line with CL.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Utilize DSC_STATUS, DSC_INCONST_CODE defines.
[@history_template@]...
*/
DB_STATUS
dmfinfo1(
DMF_JSX             *jsx,
DMP_DCB             *dcb)
{
    DM0C_CNF            *config;
    DM0C_CNF            *cnf;
    i4             i;
    DB_STATUS           status;
    bool                node_info = FALSE;
    char                line_buffer[132];
    char		status_buffer[50];
    CL_ERR_DESC         sys_err;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    if (jsx->jsx_status & JSX_TRACE)
	TRset_file(TR_F_OPEN, "infodb.dbg", 10, &sys_err);

    /*  Pretend the database is exclusive. */

    dcb->dcb_status |= DCB_S_EXCLUSIVE;

    /*  Lock the database. */

    status = dm0c_open(dcb, 0, dmf_svcb->svcb_lock_list, &config,
	&jsx->jsx_dberr);
    if (status != E_DB_OK)
	return (status);
    cnf = config;


   /*
    *  Print database status information.
    *  dbname  dbowner  id  status (reason if inconsistent)
    */
    MEfill(sizeof(status_buffer), ' ', status_buffer);
    MEfill(sizeof(line_buffer), ' ', line_buffer); 

    if( cnf->cnf_dsc->dsc_status & DSC_VALID )
		STcopy("VALID", status_buffer );
    else
    {
	    TRformat(NULL, 0, status_buffer, sizeof(status_buffer),
	    "INCONSISTENT( %w )",
	    DSC_INCONST_CODE, cnf->cnf_dsc->dsc_inconst_code);
    }	

    TRformat(dmf_put_line, 0, line_buffer, sizeof(line_buffer),
	"%t%t%08x\t%~t",
	sizeof(cnf->cnf_dsc->dsc_name), &cnf->cnf_dsc->dsc_name,
	sizeof(cnf->cnf_dsc->dsc_owner), &cnf->cnf_dsc->dsc_owner,
	cnf->cnf_dsc->dsc_dbid ,
	sizeof(status_buffer),
	&status_buffer);

    /*  Update and close the configuration file. */

    status = dm0c_close(cnf, 0, &jsx->jsx_dberr);
    if (status != E_DB_OK)
	return (status);

    /*  Return. */

    return (E_DB_OK);
}
