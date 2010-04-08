/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <pc.h>
#include    <sr.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dmucb.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmse.h>
#include    <dm0p.h>
#include    <dm1b.h>
#include    <dm1c.h>
#include    <dm1s.h>
#include    <dm1p.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dm1x.h>
#include    <dmpl.h>
#include    <dmpp.h>
#include    <dmpbuild.h>
#include    <dm0m.h>

/**
**
**  Name: DM1SBUILD.C - Routines to build a SEQuential table.
**
**  Description:
**      This file contains all the routines needed to build
**      a SEQuential table. 
**
**      The routines defined in this file are:
**      dm1sbbegin          - Initializes SEQuential file for building.
**      dm1sbend            - Finishes building SEQuential table.
**      dm1sbput            - Adds a record to a new SEQuential table
**                            and builds index.
**
**  History:
**      07-feb-86 (jennifer)
**          Created for Jupiter.
**	29-may-89 (rogerk)
**	    Check status from dm1c_comp_rec calls.
**	14-jun-1991 (Derek)
**	    Changed to use the new common build buffering routines that
**	    know about the new allocation structures.  Changed the build
**	    algorithm to allow an existing heap with or without data to be
**	    loaded or reloaded in place.
**	 7-jul-1992 (rogerk)
**	    Prototyping DMF.
**      09-jun-1992 (kwatts)
**          6.5 MPF Changes. Replaced calls of dm1c_comp_rec and
**          dm1c_add with accessors.
**	29-August-1992 (rmuth)
**	    Alter for new FHDR/FMAP build algorithm.
**	30-October-1992 (rmuth)
**	    Change for new file extend DI paradigm.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system typedefs
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**	20-may-95 (hanch04)
**	    Added #include<dmtcb.h>
**	06-mar-1996 (stial01 for bryanp)
**	    Don't allocate tuple buffers on the stack.
**	2-may-1996 (ramra01)
**	    Added new param tup_info to the load accessor
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**      18-jul-1996 (ramra01 for bryanp)
**          Pass 0 as the current table version to dmpp_load.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          load accessor: changed DMPP_TUPLE_INFO param to table_version
**      10-mar-97 (stial01)
**          dm1sbput: Use mct_crecord to compress a record
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          Arg changes for page accessors (Variable Page Type SIR 102955)
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**	12-Aug-2004 (schka24)
**	    mct usage cleanups.
**	21-Feb-2008 (kschendel) SIR 122739
**	    Minor changes for the new row-accessor structure.
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1x?, dm1?b? functions converted to DB_ERROR *
**      24-nov-2009 (stial01)
**          Check if table has segmented rows (instead of if pagetype supports)
**/
static VOID	    logErrorFcn(
				i4 		status, 
				DB_ERROR 	*dberr,
				PTR		FileName,
				i4		LineNumber);
#define	log_error(status,dberr) \
	logErrorFcn(status,dberr,__FILE__,__LINE__)


/*{
** Name: dm1sbbegin - Initializes SEQuential file for modify.
**
** Description:
**    This routine initializes the SEQuential file for building.
**    Only the first page is allocated here.
**
** Inputs:
**      mct                             Pointer to modify context.
**
**
** Outputs:
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**      
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**	07-feb-86 (jennifer)
**          Created for Jupiter.
**	29-August-1992 (rmuth)
**	    Alter for new FHDR/FMAP build algorithm.
**	06-may-96 (nanpr01)
**	    Changed all page header access as macro for New Page Format 
**	    project.
*/
DB_STATUS
dm1sbbegin(
DM2U_M_CONTEXT  *mct,
DB_ERROR        *dberr)
{
    DM2U_M_CONTEXT  *m = mct;
    STATUS          status;
    i4		    num_alloc;
    i4		    page;
    DB_ERROR	    local_dberr;

    CLRDBERR(dberr);

    if (! m->mct_rebuild)
    {
	/* Append to existing heap */
	do
	{
	    status = dm1xstart(m, DM1X_KEEPDATA , dberr);
	    if (status != E_DB_OK)
		break;

	    status = dm1xnextpage(mct, &page, dberr);
	    if ( status != E_DB_OK )
		break;

	    /*
	    ** Update current last data page to point to first new data page
	    */
	    DMPP_VPT_SET_PAGE_OVFL_MACRO(mct->mct_page_type, mct->mct_last_dpage,
				     page);

	    status = dm1xnewpage(m, 0, &m->mct_curdata, dberr);
	    if ( status != E_DB_OK )
		break;

	} while (0);
    }
    else
    {
	/* Brand new heap structure */
	status = dm1xstart(m, 0, dberr);
	if (status == E_DB_OK)
	    status = dm1xnewpage(m, 0, &m->mct_curdata, dberr);
    }
    if (status != E_DB_OK)
    {
	(VOID) dm1xfinish(m, DM1X_CLEANUP, &local_dberr);
	log_error(E_DM924B_DM1S_BBEGIN, dberr);
	return (status);
    }

    /*	Initialize HEAP specific parts of MCT. */

    m->mct_main = 1;
    m->mct_prim = 1;
    DMPP_VPT_INIT_PAGE_STAT_MACRO(m->mct_page_type, m->mct_curdata, DMPP_DATA);
    return (E_DB_OK);
}

/*{
** Name: dm1sbend - Finishes building a SEQuential file for modify.
**
** Description:
**    This routine finsihes building a SEQuential for modify.
**    It allocates and writes the pages leftover after a build operation.
**
** Inputs:
**      mct                             Pointer to modify context.
**
**
** Outputs:
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**      
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**	07-feb-86 (jennifer)
**          Created for Jupiter.
**	29-August-1992 (rmuth)
**          Changed for new FHDR/FMAP build algorithm.
**	30-October-1992 (rmuth)
**	    Change for new DI file extend paradigm, we now build
**	    the FHDR/FMAP and guarantee space in dm1xfinish.
*/
DB_STATUS
dm1sbend(
DM2U_M_CONTEXT	*mct,
DB_ERROR        *dberr)
{
    DM2U_M_CONTEXT  *m = mct;
    DB_STATUS       status;
    DB_ERROR	    local_dberr;

    CLRDBERR(dberr);

    if (mct->mct_relwid > dm2u_maxreclen(mct->mct_page_type, mct->mct_page_size))
    {
	i4	nextpage;

	/*
	**  Make sure last page is DMPP_OVFL page
	*/

	status = dm1xnextpage(mct, &nextpage, dberr);
	DMPP_VPT_SET_PAGE_OVFL_MACRO(mct->mct_page_type, mct->mct_curdata, 
				 nextpage);
	if (status == E_DB_OK)
	{
	    /*  Get next page. */

	    status = dm1xnewpage(mct, 0, &mct->mct_curdata, dberr);
	}
	if (status != E_DB_OK)
	{
	    log_error(E_DM924C_DM1S_BEND, dberr);
	    return (status);
	}

	/*  Set the page status. */
	DMPP_VPT_INIT_PAGE_STAT_MACRO(mct->mct_page_type, mct->mct_curdata, 
				  DMPP_DATA);
    }

    status = dm1xfinish(m, DM1X_COMPLETE, dberr);
    if ( status == E_DB_OK )
    {
	return(E_DB_OK );
    }
    else
    {
	(VOID) dm1xfinish(m, DM1X_CLEANUP, &local_dberr);
	log_error(E_DM924C_DM1S_BEND, dberr);
    	return (status);
    }

}

/*{
** Name: dm1sbput - Adds a record to SEQuential file. 
**
** Description:
**    This routine builds a SEQuential table.  Called by modify. 
**    No assumptions are made about the record order. 
**    This routine also assumes that the record is not compressed.
**    The record is added to the page if there is room otherwise
**    a new page is added.
**    Currently the TCB of the table modifying is used for
**    attribute information needed by the low level routines.
**    If attributes are allowed to be modified, then these
**    build routines will not work. 
**
** Inputs:
**      mct                             Pointer to modify context.
**      record                          Pointer to an area containing
**                                      record to add.
**      record_size                     Size of record in bytes.
**      dup                             A flag indicating if this
**                                      record is a duplicate key.
**
** Outputs:
**      err_code                        A pointer to an area to return error
**                                      codes if return status not E_DB_OK.
**      
**	Returns:
**
**	    E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none.
**
** History:
**	07-feb-86 (jennifer)
**          Created for Jupiter.
**	29-may-89 (rogerk)
**	    Check status from dm1c_comp_rec calls.
**      09-jun-1992 (kwatts)
**          6.5 MPF Changes. Replaced calls of dm1c_comp_rec and
**          dm1c_add with accessors.
**	06-mar-1996 (stial01 for bryanp)
**	    Don't allocate tuple buffers on the stack.
**	06-may-96 (nanpr01)
**	    Changed all page header access as macro for New Page Format 
**	    project. Also use the mct->mct_acc_plv instead of old page  
**	    accessors.
**	20-may-96 (ramra01)
**	    Added new param tup_info to the load accessor
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**      18-jul-1996 (ramra01 for bryanp)
**          Pass 0 as the current table version to dmpp_load.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          load accessor: changed DMPP_TUPLE_INFO param to table_version
**      22-nov-1996 (nanpr01)
**          init the version with the current table version to dmpp_load.
**      10-mar-97 (stial01)
**          dm1bsput: Use mct_crecord to compress a record
*/
DB_STATUS
dm1sbput(
DM2U_M_CONTEXT  *mct,
char            *record,
i4         record_size,
i4         dup,
DB_ERROR        *dberr)
{
    DB_STATUS       status;
    char	    *rec = record;
    i4         rec_size, nextpage;

    CLRDBERR(dberr);

    /* 
    ** See if there is room on current page,
    ** there is always room for the first record.
    */

    rec_size = record_size;
    if (mct->mct_data_rac.compression_type != TCB_C_NONE)
    {
	rec = mct->mct_crecord;
	status = (*mct->mct_data_rac.dmpp_compress)(&mct->mct_data_rac,
		    record, record_size, rec, &rec_size);
	if (status != E_DB_OK)
	{

	    SETDBERR(dberr, 0, E_DM938B_INCONSISTENT_ROW);
	    return (status);
	}
    }

    /*	Add record to current page. */
    while (dm1xbput(mct, mct->mct_curdata, rec, rec_size, DM1C_LOAD_NORMAL,
            0, 0, mct->mct_ver_number, dberr) == E_DB_WARN)
    {
	/*
	**  Current page is full, point current page at nextpage.
	*/

	status = dm1xnextpage(mct, &nextpage, dberr);
	DMPP_VPT_SET_PAGE_OVFL_MACRO(mct->mct_page_type, mct->mct_curdata, 
				 nextpage);
	if (status == E_DB_OK)
	{
	    /*  Get next page. */

	    status = dm1xnewpage(mct, 0, &mct->mct_curdata, dberr);
	}
	if (status != E_DB_OK)
	{
	    log_error(E_DM924D_DM1S_BPUT, dberr);
	    return (status);
	}

	/*  Set the page status. */
	DMPP_VPT_INIT_PAGE_STAT_MACRO(mct->mct_page_type, mct->mct_curdata, 
				  DMPP_DATA);
    }

    return (E_DB_OK); 
}

static VOID
logErrorFcn(
i4	    status,
DB_ERROR    *dberr,
PTR	    FileName,
i4	    LineNumber)
{
    i4		local_err;

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
	    (i4)0, (i4 *)NULL, &local_err, 0);

	dberr->err_code = status;
	dberr->err_file = FileName;
	dberr->err_line = LineNumber;
    }
}
