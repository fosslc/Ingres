/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <tr.h>
#include    <sr.h>
#include    <me.h>
#include    <pc.h>
#include    <di.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dmf.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmucb.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpl.h>
#include    <dmpp.h>
#include    <dmse.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dm1b.h>
#include    <dm1p.h>
#include    <dm1c.h>
#include    <dm1h.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dm1x.h>
#include    <dm0p.h>
#include    <dmpbuild.h>
#include    <dm0m.h>

/**
**
**  Name: DM1HBUILD.C - Routines to build a HASH table.
**
**  Description:
**      This file contains all the routines needed to build
**      a HASH table. 
**
**      The routines defined in this file are:
**      dm1hbbegin          - Initializes HASH file for building.
**      dm1hbend            - Finishes building HASH table.
**      dm1hbput            - Adds a record to a new HASH table
**                            and builds index.
**
**  History:
**      07-feb-86 (jennifer)
**          Created for Jupitor.
**      25-nov-87 (jennifer)
**          Added supporty for multiple locations per table.
**	29-may-89 (rogerk)
**	    Check status from dm1c_comp_rec calls.
**	14-jun-1991 (Derek)
**	    Changed to use the new common build buffering routines that are
**	    aware of th new allocation structures.
**      24-oct-1991 (jnash)
**          In dm1hbend, add error handling & eliminate page and 
**	    num_write params.
**	 7-jul-1992 (rogerk)
**	    Prototyping DMF.
**    	08-jun_1992 (kwatts)
**          6.5 MPF changes. dm1c_add calls become dmpp_load accessors,
**          dm1c_comp_rec call becomes an accessor call using MCT accessors.
**	29-August-1992 (rmuth)
**	    Add call to dm1x_build_SMS to add the FHDR/FMAP(s).
**	30-October-1992 (rmuth)
**	    Change for new DI file extend paradigm
**	08-feb-1993 (rmuth)
**	    On overflow pages set DMPP_OVFL instead of DMPP_CHAIN which is
**	    used for overflow leaf pages.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system typedefs
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**	20-may-95 (hanch04)
**	    Added include dmtcb.h
**	06-mar-1996 (stial01 for bryanp)
**	    Don't allocate tuple buffers on the stack.
**	21-may-1996 (ramra01)
**	    added new param to the load accessor
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          load accessor: changed DMPP_TUPLE_INFO param to table_version
**      10-mar-97 (stial01)
**          dm1hbput: Use mct_crecord to compress a record
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
**	24-Feb-2008 (kschendel) SIR 122739
**	    New row-accessor scheme, reflect here.
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1x?, dm1?b? functions converted to DB_ERROR *
**/

/*
**  Forward definition of static functions.
*/
static VOID	    logErrorFcn(
				i4 		status, 
				DB_ERROR 	*dberr,
				PTR		FileName,
				i4		LineNumber);
#define	log_error(status,dberr) \
	logErrorFcn(status,dberr,__FILE__,__LINE__)

/*{
** Name: dm1hbbegin - Initializes HASH file for modify.
**
** Description:
**    This routine initializes the HASH file for building.
**    All the main pages are allocated here.  Only overflow 
**    pages are allocated as records are added.  Note, 
**    that the number of main pages is determined by the
**    buckets value in the modify context control block.
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
**	06-may-1996 (nanpr01)
**	    Change all the page header access as macro for New Page Format
**	    project.
*/
DB_STATUS
dm1hbbegin(
DM2U_M_CONTEXT   *mct,
DB_ERROR         *dberr)
{
    DM_SVCB	    *svcb = dmf_svcb;
    DB_STATUS       status;
    i4		    num_alloc;
    i4	    num_read;
    i4	    page;
    DB_ERROR	    local_dberr;

    CLRDBERR(dberr);
    
    /*	Allocate build memory using an Overflow buffer. */

    status = dm1xstart(mct, DM1X_OVFL, dberr);
    if (status == E_DB_OK)
    {
	/*  Mark first spot that FHDR can be allocated. */

	status = dm1xreserve(mct, mct->mct_buckets, dberr);
	if (status == E_DB_OK)
	{
	    /*  Get the first page. */

	    status = dm1xnewpage(mct, -1, &mct->mct_curdata, dberr);
	}
	if (status != E_DB_OK)
	    (VOID) dm1xfinish(mct, DM1X_CLEANUP, &local_dberr);
    }
    if (status != E_DB_OK)
    {
	log_error(E_DM924E_DM1H_BBEGIN, dberr);
	return (status);
    }

    /*	Initialize the HASH specific parts of the MCT. */
    DMPP_VPT_INIT_PAGE_STAT_MACRO(mct->mct_page_type, mct->mct_curdata, DMPP_DATA);
    DMPP_VPT_SET_PAGE_MAIN_MACRO(mct->mct_page_type, mct->mct_curdata, 1);
    if (mct->mct_buckets == 1)
	DMPP_VPT_SET_PAGE_MAIN_MACRO(mct->mct_page_type, mct->mct_curdata, 0);
    mct->mct_main = mct->mct_buckets;
    mct->mct_prim = mct->mct_buckets;
    mct->mct_curbucket = 0;
    return (E_DB_OK);
}

/*{
** Name: dm1hbend - Finishes building a HASH file for modify.
**
** Description:
**    This routine finsihes building a HASH for modify.
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
**      24-oct-1991 (jnash)
**	    Added error handling, eliminate page and num_write params.
**	29-August-1992 (rmuth)
**	    Add call to dm1x_build_SMS.
**	30-October-1992 (rmuth)
**	   Change for new DI file extend paradigm, we now build the
**	   FHDR/FMAP and guarantee all the space in dm1xfinish.
*/
DB_STATUS
dm1hbend(
DM2U_M_CONTEXT	*mct,
DB_ERROR        *dberr)
{
    DB_STATUS       status;
    i4	    page;
    DB_ERROR	    local_dberr;

    CLRDBERR(dberr);

    do
    {
    	/* Force rest of buckets to be formatted. */
        status = dm1hbput(mct, mct->mct_buckets, 0, 0, 0, dberr);
        if ( status != E_DB_OK )
	    break;

    	/* Write build pages to disk and deallocate the build memory. */
    	status = dm1xfinish(mct, DM1X_COMPLETE, dberr);
        if (status != E_DB_OK)
	    break;

    } while( FALSE );


    if ( status == E_DB_OK )
	return( E_DB_OK );

    /* 
    ** deallocate but don't flush, dm1xfinish can handle being called even
    ** if we have already called it successfully.
    */
    (VOID) dm1xfinish(mct, DM1X_CLEANUP, &local_dberr);

    log_error(E_DM924F_DM1H_BEND, dberr); 

    return (status); 
}

/*{
** Name: dm1hbput - Adds a record to HASH file. 
**
** Description:
**    This routine builds a HASH table.  Called by modify. All
**    records are assumed to be in sorted order by hash bucket.
**    This routine also assumes that the record is not compressed.
**    Duplicates are noted by an input parameter.  The record
**    is added to the data page if there is room, otherwise it is
**    added to an overflow page or a new main page depending
**    on the value in duplicate.
**    Currently the TCB of the table modifying is used for
**    attribute information needed by the low level routines.
**    If attributes are allowed to be modified, then these
**    build routines will not work. 
**
** Inputs:
**      mct                             Pointer to modify context.
**      bucket                          Value indicating bucket to
**                                      write this record to. 
**      record                          Pointer to an area containing
**                                      record to add.
**      record_size                     Size of record in bytes.
**					If record_size == 0 then no record
**					is put, but the hash bucket will be
**					initialized.  This is used to finish
**					the allocation after the last real
**					record.
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
**    	08-jun_1992 (kwatts)
**          6.5 MPF changes. dm1c_add calls become dmpp_load accessors,
**          dm1c_comp_rec call becomes an accessor call using MCT accessors.
**	08-feb-1993 (rmuth)
**	    On overflow pages set DMPP_OVFL instead of DMPP_CHAIN which is
**	    used for overflow leaf pages.
**	06-mar-1996 (stial01 for bryanp)
**	    Don't allocate tuple buffers on the stack.
**	06-may-1996 (nanpr01)
**	    Change all the page header access as macro for New Page Format
**	    project.
**	21-may-1996 (ramra01)
**	    Added new param tup_info to the load accessor
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          load accessor: changed DMPP_TUPLE_INFO param to table_version
**      10-mar-97 (stial01)
**          dm1hbput: Use mct_crecord to compress a record
**	28-Feb-2001 (thaju02)
**	    Pass mct->mct_ver_number to dmpp_load. (B104100)
**	15-sep-99 (nanpr01)
**	    Changed the way the hash overflow pages are added. Prior to this
**	    change, all overflow pages are added head of the list. This causes
**	    problem when a hash table is scanned, it cannot take advantage
**	    of read-ahead. Now the build routine always adds the overflow
**	    pages at the end, which will take advantage of read ahead on a
**	    scan. However regular put operation will add the page in the
**	    front of the bucket so that the subsequent put operations are
**	    not penalized.
*/
DB_STATUS
dm1hbput(
DM2U_M_CONTEXT   *mct,
i4         bucket,
char            *record,
i4         record_size,
i4         dup,
DB_ERROR         *dberr)
{
    DB_STATUS       status;
    char	    *rec = record;
    i4         rec_size;
    i4		    next_page, save_page;
    i4		    first_ovfl = 0;

    CLRDBERR(dberr);

    /* If we have crossed over to a new bucket switch to new main page. */

    if (bucket != mct->mct_curbucket)
    {
	do
	{
	    /*	Point to next sequential bucket. */

	    mct->mct_curbucket++;

	    /*	Get next page. */

	    status = dm1xnewpage(mct, mct->mct_curbucket, &mct->mct_curdata,
		dberr);
	    if (status != E_DB_OK)
		break;

	    /*	Initialize HASH specific parts. */

	    DMPP_VPT_SET_PAGE_MAIN_MACRO(mct->mct_page_type, mct->mct_curdata, 
				     mct->mct_curbucket + 1);
	    if (mct->mct_curbucket == mct->mct_buckets - 1)
		DMPP_VPT_SET_PAGE_MAIN_MACRO(mct->mct_page_type, mct->mct_curdata,
					 0);
	} while (mct->mct_curbucket < bucket);

	if (status != E_DB_OK && status != E_DB_INFO)
	{
	    log_error(E_DM9250_DM1H_BPUT, dberr);
	    return (status);
	}
    }
    
    /*	Compress record if required. */

    if ((rec_size = record_size) == 0)
    {
	return (E_DB_OK);
    }

    if (mct->mct_data_rac.compression_type != TCB_C_NONE)
    {
	rec = mct->mct_crecord;
        /* Note that the following accessor comes from the MCT and not the
        ** TCB. This is because we want the compression scheme of the table
        ** we are building, not the one we are building from
        */
        status = (*mct->mct_data_rac.dmpp_compress)(&mct->mct_data_rac,
               record, record_size, rec, &rec_size);
	if (status != E_DB_OK)
	{
	    SETDBERR(dberr, 0, E_DM938B_INCONSISTENT_ROW);
	    return (status);
	}
    }

    /*	Add record to current page. */

    /* This accessor is back to the table, since it is constant over the
    ** table.
    */
    if (dm1xbput(mct, mct->mct_curdata, rec, rec_size,
	    DM1C_LOAD_NORMAL, 0, 0,
	    mct->mct_ver_number, dberr) == E_DB_WARN)
    {
	/* 
	** This data page is full, check current overflow page.
	** If the overflow page doesn't exist or doesn't have enough
	** space, allocate a new overflow page for this chain.
	*/
	status = E_DB_WARN;
	if (mct->mct_curovfl &&
	    DMPP_VPT_GET_PAGE_MAIN_MACRO(mct->mct_page_type, mct->mct_curovfl) == 
	    DMPP_VPT_GET_PAGE_MAIN_MACRO(mct->mct_page_type, mct->mct_curdata))
	{

            status = dm1xbput(mct, mct->mct_curovfl, rec, rec_size,
             		DM1C_LOAD_NORMAL, 0, 0,
			mct->mct_ver_number, dberr);
	}
	if (status == E_DB_WARN)
	{
	    /* If first overflow page then link it to the data page */
	    if ( DMPP_VPT_GET_PAGE_OVFL_MACRO(mct->mct_page_type, mct->mct_curdata)
		 == 0 )
	    {
	        status = dm1xnewpage(mct, 0, &mct->mct_curovfl, dberr);
	        if (status != E_DB_OK)
	        {
	  	    log_error(E_DM9250_DM1H_BPUT, dberr);
		    return(status);
	        }

	        DMPP_VPT_SET_PAGE_MAIN_MACRO(mct->mct_page_type, 
				mct->mct_curovfl, 
				DMPP_VPT_GET_PAGE_MAIN_MACRO(mct->mct_page_type, 
							 mct->mct_curdata));

	        DMPP_VPT_SET_PAGE_OVFL_MACRO(mct->mct_page_type, 
				mct->mct_curovfl,
				DMPP_VPT_GET_PAGE_OVFL_MACRO(mct->mct_page_type, 
							 mct->mct_curdata));
	        DMPP_VPT_SET_PAGE_OVFL_MACRO(mct->mct_page_type, 
				mct->mct_curdata, 
				DMPP_VPT_GET_PAGE_PAGE_MACRO(mct->mct_page_type,
							 mct->mct_curovfl));
	    }
	    else 
	    {
		/*
		** Find out next page number we will allocate so that
		** can fix up the overflow chain ptr on current overflow
		** page before we release it
		*/
		status = dm1xnextpage( mct, &next_page, dberr );
		if ( status != E_DB_OK )
		{
	  	    log_error(E_DM9250_DM1H_BPUT, dberr);
		    return(status);
		}
		save_page = DMPP_VPT_GET_PAGE_OVFL_MACRO(mct->mct_page_type, 
			mct->mct_curovfl);

		DMPP_VPT_SET_PAGE_OVFL_MACRO(mct->mct_page_type, mct->mct_curovfl,
			next_page);

	        status = dm1xnewpage(mct, 0, &mct->mct_curovfl, dberr);
	        if (status != E_DB_OK)
	        {
	  	    log_error(E_DM9250_DM1H_BPUT, dberr);
		    return(status);
	        }
	        DMPP_VPT_SET_PAGE_MAIN_MACRO(mct->mct_page_type, 
				mct->mct_curovfl, 
				DMPP_VPT_GET_PAGE_MAIN_MACRO(mct->mct_page_type, 
							 mct->mct_curdata));
	        DMPP_VPT_SET_PAGE_OVFL_MACRO(mct->mct_page_type, 
				mct->mct_curovfl, save_page);
	    }

	    DMPP_VPT_INIT_PAGE_STAT_MACRO(mct->mct_page_type, mct->mct_curovfl,
					  DMPP_DATA | DMPP_OVFL );

            status = dm1xbput(mct, mct->mct_curovfl, rec, rec_size,
			DM1C_LOAD_NORMAL, 0, 0,
			mct->mct_ver_number, dberr);
	}
	if (status != E_DB_OK)
	    log_error(E_DM9250_DM1H_BPUT, dberr);

	return (status);
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
