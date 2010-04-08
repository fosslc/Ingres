/*
**Copyright (c) 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <di.h>
#include    <tm.h>
#include    <cs.h>
#include    <jf.h>
#include    <si.h>
#include    <cp.h>
#include    <lo.h>
#include    <pc.h>
#include    <er.h>
#include    <tr.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lk.h>
#include    <lg.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <sxf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm2d.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dm2r.h>
#include    <dm0c.h>
#include    <dmfjsp.h>
#include    <dm0d.h>
#include    <dmd.h>
#include    <dma.h>
/*
**
**  Name: DMFDTP.C - DMF Dump Trail Processor.
**
**  Description:
**	Function prototypes in DMFJSP.H.
**
**      This file contains the routines that manage reading dump files
**	and writing dump trail information for the user.
**
**          dmfdtp - Dump trail producer
**
**	(Internal)
**
**	    prepare - Prepare to dump.
**
**  History:
**	27-feb-1989 (EdHsu)
**	    Created for Terminator
**      19-nov-90 (rogerk)
**	    Took out allocation of local ADF_CB as it wasn't used.
**	30-apr-1991 (bryanp)
**	    Fixed lint errors.
**	8-july-1992 (ananth)
**	    Prototyping DMF.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	16-mar-1993 (rmuth)
**	    Include di.h
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	24-may-1993 (andys)
**	    Pass appropriate logfile blocksize to dmd_log and dmd_format_log
**	    rather than using hardwired 4096.
**	13-jul-1993 (ed)
**	    unnest dbms.h
**	18-feb-94 (stephenb)
**	    Add call to dma_write_audit() to audit this action.
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**      21-jan-1999 (hanch04)
**          replace i4  and i4 with i4
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to dma_write_audit() if not C2SECURE.
**      01-sep-2004 (stial01)
**          Support cluster online checkpoint
**	12-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	18-Nov-2008 (jonj)
**	    dm0d_?, dm0c_?, dm2d_? functions converted to DB_ERROR *
**	08-Dec-2008 (jonj)
**	    SIR 120874: dma_? auditing functions converted to DB_ERROR *
[@history_template@]...
**/

/*
**  References to global variables.
*/

/*
**  Forward type references.
*/

typedef struct _DMF_DTP	DMF_DTP;	/* Dump context. */
typedef struct _DTP_TX	DTP_TX;		/* Transaction hash array. */
typedef struct _DTP_TD	DTP_TD;		/* Table descriptions. */

/*
** Name: DMF_DTP - Dump context.
**
** Description:
**      This structure contains information needed by the dump routines.
**
** History:
**	27-feb-1989 (EdHsu)
**	    Created for Terminator
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedef to quiesce the ANSI C 
**	    compiler warning.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	9-mar-94 (stephenb)
**	    Update dma_write_audit() calls to current prototype.
[@history_template@]...
*/
struct _DMF_DTP
{
    DMP_DCB         *dtp_dcb;		/* Pointer to DCB for database. */
    DMF_JSX	    *dtp_jsx;		/* Pointer to dump context. */
    i4	    dtp_f_dmp;		/* First dump file. */
    i4	    dtp_l_dmp;		/* Last dump file. */
    i4	    dtp_block_size;	/* Dump block size. */
};

/*
**  Forward function references.
*/

static DB_STATUS prepare(
    DMF_DTP	*dtp,
    DMF_JSX	*jsx,
    DMP_DCB	*dcb);

/*{
** Name: dmfdtp	- Dump Trail Processor.
**
** Description:
**      This routine implements the dump trail processor. 
**
** Inputs:
**      context                         Pointer to DMF_CONTEXT
**	dcb				Pointer to DCB for the database.
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
**	27-feb-1989 (EdHsu)
**	    Created for Terminator
**	10-feb-1993 (jnash)
**	    Add FALSE (non-verbose) param to dmd_log().
**	24-may-1993 (andys)
**	    Pass appropriate logfile blocksize to dmd_log and dmd_format_log
**	    rather than using hardwired 4096.
**	18-feb-94 (stephenb)
**	    Add call to dma_write_audit() to audit this action.
**	15-jan-1999 (nanpr01)
**	    Padd pointer to pointer to dm0d_close routine.
[@history_template@]...
*/
DB_STATUS
dmfdtp(
DMF_JSX		    *jsx,
DMP_DCB		    *dcb)
{
    PTR			record;
    i4		l_record;
    i4		i;
    DB_STATUS		status;
    DM0D_CTX		*dmp;
    DM0C_CNF		*cnf;
    DMF_DTP		dtp;
    CL_ERR_DESC		k;
    i4			local_error;
    DB_ERROR		local_dberr;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    TRset_file(TR_F_OPEN, "dump_database", 13, &k);

    /*	Prepare to dump this database. */

    status = prepare(&dtp, jsx, dcb);
    if (status != E_DB_OK)
    {
	return (status);
    }

    status = dm0c_open(dcb, DM0C_PARTIAL, dmf_svcb->svcb_lock_list,
	&cnf, &jsx->jsx_dberr);
    if (status != E_DB_OK)
    {
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1331_DTP_CNF_OPEN);
	return (status);
    }

    status = dm0c_close(cnf, 0, &jsx->jsx_dberr);
    if (status != E_DB_OK)
    {
	SETDBERR(&jsx->jsx_dberr, 0, E_DM1332_DTP_CNF_CLOSE);
	return (status);
    }

    /*	Main processing loop. */
    
    for (i = dtp.dtp_l_dmp; i && i >= dtp.dtp_f_dmp; i--)
    {
	/* Open the next dump file. */

	status = dm0d_open(DM0D_MERGE, (char *)&dcb->dcb_dmp->physical, 
            dcb->dcb_dmp->phys_length, 
	    &dcb->dcb_name, dtp.dtp_block_size, i, 0, DM0D_EXTREME, DM0D_M_READ,
	    -1, DM0D_BACKWARD, &dmp, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM132A_DTP_DMP_OPEN);
	    break;
	}


	/*  Loop through the records of this dump file. */

	for (;;)
	{
	    /*	Read the next record from this dump file. */

	    status = dm0d_read(dmp, &record, &l_record, &jsx->jsx_dberr);
	    if (status != E_DB_OK)
	    {
		if (jsx->jsx_dberr.err_code == E_DM0055_NONEXT)
		{
		    status = E_DB_OK;
		    CLRDBERR(&jsx->jsx_dberr);
		}
		else
		    SETDBERR(&jsx->jsx_dberr, 0, E_DM132B_DTP_DMP_READ);
		break;
	    }

	    dmd_log(FALSE, record, dtp.dtp_block_size);

	    continue;
	}

	/*  Close the current dump file. */

	if (dmp)
	{
	    (VOID) dm0d_close(&dmp, &local_dberr);
	}

	if (status != E_DB_OK)
	    break;
    }
    /*
    ** Write an audit to say that we have read dump.
    */
    if ( dmf_svcb->svcb_status & SVCB_C2SECURE )
    {
	DB_STATUS	local_status;
	SXF_ACCESS	access = SXF_A_SELECT;

	if (status != E_DB_OK)
	    access |= SXF_A_FAIL;
	else
	    access |= SXF_A_SUCCESS;

    	local_status = dma_write_audit(SXF_E_DATABASE, access, 
		dcb->dcb_name.db_db_name, 
		sizeof(dcb->dcb_name.db_db_name),
		&dcb->dcb_db_owner, 
		I_SX2747_DUMPDB, 
		TRUE, &local_dberr, NULL);


	(VOID) dm2d_close_db(dcb, dmf_svcb->svcb_lock_list, 0, &local_dberr);

    }
    TRset_file(TR_F_CLOSE, 0, 0, &k);

    return (status);
}

/*{
** Name: prepare	- Prepare to dump.
**
** Description:
**      This routine opens the database, reads the configuration file to
**	compute the dumps to use, creates the output files, and initializes
**	the transaction hash table.
**
** Inputs:
**	jsx				Pointer to dump support context.
**	dcb				Pointer to DCB for database.
**
** Outputs:
**      dtp                             Pointer to the dump context.
**      err_code                        Reason for error code.
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
**	27-feb-1989 (EdHsu)
**	    Created for Terminator
**	30-apr-1991 (bryanp)
**	    Correct error-code handling for E_DM0053 return from dm2d_open_db.
[@history_template@]...
*/
static DB_STATUS
prepare(
DMF_DTP             *dtp,
DMF_JSX		    *jsx,
DMP_DCB		    *dcb)
{
    i4		start;
    i4		end;
    i4		i;
    DM0C_CNF		*cnf;
    DB_STATUS		status;
    i4			*err_code = &jsx->jsx_dberr.err_code;

    CLRDBERR(&jsx->jsx_dberr);

    /*	Lock the database. */

    status = dm2d_open_db(
	dcb, DM2D_A_READ, DM2D_IS, dmf_svcb->svcb_lock_list,
	0, &jsx->jsx_dberr);
    if (status != E_DB_OK)
    {
	if (jsx->jsx_dberr.err_code == E_DM0053_NONEXISTENT_DB)
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM132F_DTP_NON_DB);
	else if (jsx->jsx_dberr.err_code == E_DM004C_LOCK_RESOURCE_BUSY)
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1336_DTP_DB_BUSY);
	else
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1330_DTP_OPEN_DB);
	return (E_DB_ERROR);
    }

    /*  Find dump information. */

    for (;;)
    {
	/*	Make sure that the database is dumped. */
/*
	if ((dcb->dcb_status & DCB_S_DUMP) == 0)
	{
	    status = E_DB_ERROR;
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1328_DTP_DB_NOT_DMP);
	    break;
	}
*/
	/*  Open the configuration file. */

	status = dm0c_open(dcb, DM0C_PARTIAL, dmf_svcb->svcb_lock_list,
	    &cnf, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1331_DTP_CNF_OPEN);
	    break;
	}

	/*
	**  If no times are given the default is everything since the
	**  last checkpoint.  If a time is given, then the set of
	**  dump files to read is calculated from the checkpoint
	**  history.
	*/

	if ((jsx->jsx_status & JSX_BTIME) == 0)
	    start = cnf->cnf_dmp->dmp_count - 1;
	else
	    start = 0;
	end = cnf->cnf_dmp->dmp_count - 1;

	for (i = 0; i < cnf->cnf_dmp->dmp_count; i++)
	{
	    if ((jsx->jsx_status & JSX_BTIME) &&
		TMcmp_stamp(( TM_STAMP * )&jsx->jsx_bgn_time, 
		    ( TM_STAMP * )&cnf->cnf_dmp->dmp_history[i].ckp_date) > 0)
	    {
		start = i;
	    }
	    if ((jsx->jsx_status & JSX_ETIME) &&
		TMcmp_stamp(( TM_STAMP * )&jsx->jsx_end_time, 
		    ( TM_STAMP * )&cnf->cnf_dmp->dmp_history[i].ckp_date) < 0)
	    {
		if (i)
		    end = i - 1;
	    }
	}

	/*  Remember starting and ending dump file numbers. */

	dtp->dtp_f_dmp = cnf->cnf_dmp->dmp_history[start].ckp_f_dmp;
	dtp->dtp_l_dmp = cnf->cnf_dmp->dmp_history[end].ckp_l_dmp;
	dtp->dtp_block_size = cnf->cnf_dmp->dmp_bksz;

	/*  Close the configuration file. */

	status = dm0c_close(cnf, 0, &jsx->jsx_dberr);
	if (status != E_DB_OK)
	{
	    SETDBERR(&jsx->jsx_dberr, 0, E_DM1332_DTP_CNF_CLOSE);
	    break;
	}

	return (E_DB_OK);
    }

    /*	Close the database. */

    if (jsx->jsx_dberr.err_code == E_DM1328_DTP_DB_NOT_DMP)
	return(E_DB_INFO);
    return (E_DB_ERROR);
}
