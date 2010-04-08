/*
**	iitblist.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<menu.h>
# include	<runtime.h>
# include	<me.h>
# include	<er.h>
# include	<rtvars.h>
# include	<erfi.h>

/**
** Name:	iitblist.c	-	dataset list routines
**
** Description:
**	Routines to handle all references to dataset lists
**
**	Public (extern) routines defined:
**		IIdsinsert()	Insert between 2 records of a dataset
**		IIt_rfree()	Free a passed dataset record
**		IIt_rdel()	Put datset records on the delete list
**		IIfree_ds()	Free all dataset records
**		IIds_init()	Initialize dataset memory manager
**		IIdsempt()	Make a data set row 'empty'
**	Private (static) routines defined:
**
** History:
**	04-mar-1983 	- written (ncg)
**	10-may-1985 	- modified to use new data set memory 
**			  management in the DSMEM structure (ncg)
**	18-feb-1987	- Added IIdsempt for DB_DATA_VALUES
**	04/09/87 (dkh) - Added forward declaration of IIdsempt.
**	25-jun-87 (bruceb)	Code cleanup.
**	13-jul-87 (bruceb)	Changed memory allocation to use [FM]Ereqmem.
**	07/22/87 (dkh) - Eliminated allocation for dm_work.  No one uses it.
**	08/14/87 (dkh) - ER changes.
**	09/01/87 (dkh) - Added change bit for datasets.
**	09/16/87 (dkh) - Integrated table field change bit.
**	11/11/87 (dkh) - Code cleanup.
**	17-apr-89 (bruceb)
**		If a row that was user-set to a _STATE of stUNDEF is
**		deleted, place on the free list, NOT the deleted list.
**		(This means a row that was created empty and was
**		never changed.)
**	11-jul-89 (bruceb)
**		Added new TBSTRUCT arg to IIdsinsert and IIdsempt to
**		support derivation processing.
**	07/22/89 (dkh) - Fixed bug 6765.
**	07/28/90 (dkh) - Added support for table field cell highlighting.
**	13-apr-92 (seg)
**	    Can't dereference or do arithmetic on a PTR variable.  Must cast.
**	07/18/93 (dkh) - Added support for the purgetable statement.
**	04/15/97 (cohmi01)
**	    Keep track of ordinal position in dataset. (b81574)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

VOID	IIdsempt();


FUNC_EXTERN	STATUS		adc_getempty();
FUNC_EXTERN	DB_DATA_VALUE	*IITBgidGetIDBV();
FUNC_EXTERN	STATUS		IIFDsdvSetDeriveVal();


/*{
** Name:	IIdsinsert	-	Insert ds record between 2 row ptrs
**
** Description:
**	Insert a dataset record between two row pointers.
**
**	Depending on the values of the row pointers insert a new
**	row between them and maybe update data set's rows. It now
**	tries to use the data set free list before allocating a new
**	chunk of memory.
** 
**	May update ds->top and ds->bot depending on the values of the
**	row pointers.
**
** Inputs:
**	tb		Ptr to table field runtime structure
**	ds		Ptr to the datset to insert into
**	rp1		Insert between row 1
**	rp2			       and row 2
**	rstate		New state to assign (to the row?)
**
** Outputs:
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	18-feb-1987	Added new routine IIdsempt to create an 'empty' 
**			row - necessary since an empty DB_DATA_VALUE is not 
**			necessarily composed of the null byte (drh)
**	13-apr-92 (seg)
**	    Can't dereference or do arithmetic on a PTR variable.  Must cast.
**	12-may-97 (cohmi01)
**	    Set tfputrow so MWS knows that rows have been added.
**	23-dec-97 (kitch01)
**		Bugs 87709, 87808, 87812, 87841.
**		Remove setting of tfputrow as it is no longer required by MWS.
**  23-jul-1998 (rodjo04) Cross-integrated change 271728 from 6.4/06
**      07-jan-94 (rudyw)
**          Bug fix for 47408/58269. Make sure IIfrmio is set before using it.
**          The situation known to cause a problem is the setting of IIfrmio
**          to NULL in IIdelfrm as part of removing an ii_lookup table which
**          has been invoked in a form having a tablefield.  A subsequent
**          select into or clear of the tablefield will result in a fake row
**          addition to the tablefield which gets us into IIdsinsert with
**          IIfrmio still not set up. Treating here is treating the result
**          far from the cause, but analysis is complex, this is safe and
**          is better than leaving a core dumping bug in ARCH status.
*/

IIdsinsert(tb, ds, rp1, rp2, rstate )
TBSTRUCT	*tb;
DATSET		*ds;
register TBROW	*rp1;
register TBROW	*rp2;
i2		rstate;
{
	TBROW		*tmprow;		/* temp new row */
	PTR		blk;
	register DSMEM	*dm;
	FRAME		*frm;

	/* 
	** Initialize fields of new row to be added.
	** Check data set free list for unused records. 
	*/
	dm = ds->ds_mem;
	if ( dm->dm_freelist != NULL )
	{
	    	tmprow = dm->dm_freelist;
		dm->dm_freelist = tmprow->nxtr;
		tmprow->nxtr = tmprow->prevr = NULL; 		/* Disconnect */
	}
	else
	{
		if ((blk = FEreqmem(dm->dm_memtag, sizeof(TBROW) + dm->dm_size,
			TRUE, (STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("IIdsinsert"));
		}
		tmprow = (TBROW *) blk;
		tmprow->rp_data = (PTR) ((char *)blk + sizeof(TBROW));
		tmprow->prevr = tmprow->nxtr = NULL;
	}
	tmprow->rp_state = rstate;

	/* We are about to use IIfrmio and need to make sure it is set */
    if ( IIfrmio == NULL )
            IIfsetio(NULL);

	/*
	**  Fill the row with the 'empty' value for each column's data
	**  and query operator.
	*/

	frm = IIfrmio->fdrunfrm;
	frm->frres2->rownum = NOTVISIBLE;
	frm->frres2->dsrow = (PTR) tmprow;

	IIdsempt(frm, tb, ds->ds_rowdesc, tmprow->rp_data);


	/*
	** Insert new row into data set and rearrange links. Update ds->top
	** and ds->bot based on the values of rp1 and rp2.
	*/
	if ( (rp1 == NULL) && (rp2 == NULL) )		/* empty list */
	{
		ds->top = ds->bot = tmprow;
	}
	else if (rp1 == NULL)				/* adding to top */
	{
		tmprow->nxtr = rp2;
		rp2->prevr = tmprow;
		ds->top = tmprow;
	}
	else if (rp2 == NULL)				/* adding to bottom */
	{
		tmprow->prevr = rp1;
		rp1->nxtr = tmprow;
		ds->bot = tmprow;
	}
	else						/* insert between */
	{
		tmprow->nxtr = rp2;
		rp2->prevr = tmprow;
		tmprow->prevr = rp1;
		rp1->nxtr = tmprow;
	}

	return (TRUE);
}


/*
** IIT_RFREE() - Free the passed data set record onto data set free list.
**
** Parameters:	ds - Data set,
**		rp - Record pointer.
** Returns:	None
*/

IIt_rfree(ds, rp)
register DATSET	*ds;
register TBROW	*rp;				/* record to free */
{
	register DSMEM 	*dm = ds->ds_mem;

	if (rp != NULL)
	{
		rp->prevr = NULL;		/* unlink from data set list */
		rp->rp_state = stUNDEF;		/* undefine (for safety) */
		rp->nxtr = dm->dm_freelist;	/* link to data set free list */
		dm->dm_freelist = rp;
	}
}


/*{
** Name:	IIt_rdel	-	Delete dataset record
**
** Description:
** 	Delete the passed record, and stick it onto the dataset's
**	delete list. If the record was NEW (was appended by the user
**	at runtime), then just free it.  The delete list only keeps
**	track of records that were "Equelled" into the data set and
**	then deleted.
**
** Inputs:
**	ds		Ptr to the dataset descriptor
**	rp		Ptr to the row to delete
**
** Outputs:
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	
*/

IIt_rdel(ds, rp)
register DATSET	*ds;
register TBROW	*rp;
{
	/* was appended at runtime */
	if (rp->rp_state == stNEWEMPT || rp->rp_state == stUNDEF
		|| rp->rp_state == stNEW)
	{
		IIt_rfree(ds, rp);
	}
	else					/* add to delete list */
	{
		rp->prevr = NULL;		/* unlink from data set list */
		rp->rp_state = stDELETE;
		rp->nxtr = ds->dellist;
		ds->dellist = rp;
	}
	return(TRUE);
}


/*{
** Name:	IIfree_ds	-	Free all dataset records
**
** Description:
**	Free all records belonging to the datset, both regular
**	and deleted records.
**
** Inputs:
**	ds		Ptr to the dataset descriptor
**
** Outputs:
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
** 	10-may-1985	- modified to attach to data set free list (ncg)
**	04/15/97 (cohmi01)
**	    Keep track of ordinal position in dataset. (b81574)
*/

IIfree_ds(ds)
register DATSET	*ds;
{
	register DSMEM	*dm;			/* data set memory manager */

	dm = ds->ds_mem;
	if (ds->top || ds->dellist || dm->dm_freelist)
	{
		/* free all regular records by attaching to free list */
		FEfree(dm->dm_memtag);
		dm->dm_freelist = NULL;
		ds->top = NULL;
		ds->distop = NULL;
		ds->distopix = 0;
		ds->bot = NULL;
		ds->disbot = NULL;
		ds->crow = NULL;
		ds->dellist = NULL;
		ds->ds_records = 0;
	}
	return (TRUE);
}


/*{
** Name:	IIds_init	-	Initialzie data set memory mgr
**
** Description:
**	Initialize the dataset memory manager
**
**	Written to efficiently use all the record fragments that are
**	allocated by table fields.
**
** Inputs:
**	ds		Ptr to the dataset descriptor
**
** Outputs:
**
** Returns:
**	void
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	19-feb-1987	Changed rp_rec references to the new rp_data
**			structure member (drh).
**	
*/

void
IIds_init(ds)
register DATSET	*ds;
{
	register DSMEM	*dm;		/* memory manager */
	i4		size;		/* new size of record */

	dm = ds->ds_mem;
	size = ds->ds_rowdesc->r_realsize;
	/* Check if this is second time round (dm_size > 0) */
	if ( dm->dm_size > 0 )
	{
		if ( dm->dm_size == size )	/* Leave alone */
			return;
		if ( dm->dm_size > size ) /* Don't free but update size */
		{
		    /* Old size is larger than new size, so just reduce */
		    dm->dm_size = size;
		    return;
		}
		MEfree(dm->dm_work);
		dm->dm_work = NULL;
	}
	dm->dm_size = size;
	if ((dm->dm_work = MEreqmem(0, size, TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("IIds_init"));
	}
}

/*{
** Name:	IIdsempt	-	Empty a dataset row
**
** Description:
**	Provided with a pointer to a dataset row, fill the row with
**	the 'empty' value for each column's data value and query operator
**	by calling adc_getempty.
**
**	For derived columns, if a constant value derivation, set the
**	column to that value.
**
**	This routine is internal to TBACC.
**
** Inputs:
**	tb		Ptr to table field runtime structure
**	rds		Ptr to the row descriptor
**	rowdata		Ptr to the actual 'data chunk' for the row
**
** Outputs:
**	The data chunk pointed to by rowdata will be updated with the 
**	empty values for each column.
**
** Returns:
**	VOID
**
*/

VOID
IIdsempt(frm, tb, rds, rowdata)
FRAME		*frm;
TBSTRUCT	*tb;
ROWDESC		*rds;	
PTR		rowdata;
{
	DB_DATA_VALUE	opdbv;
	DB_DATA_VALUE	cbdbv;
	COLDESC		*colptr;
	ADF_CB		*cb;
	FIELD		fld;
	i4		colno;
	DB_DATA_VALUE	*idbv;
	FLDHDR		*hdr;
	FLDTYPE		*type;
	bool		qry;
	i4		*attrptr;

	fld.fltag = FTABLE;
	fld.fld_var.fltblfld = tb->tb_fld;
	qry = (tb->tb_mode == fdtfQUERY);

	/*
	**  Get a pointer to the session control block
	*/

	cb = FEadfcb();

	/*
	**  Set up the local DB_DATA_VALUE for the query operator
	**  data for each column.
	*/

	opdbv.db_datatype = DB_INT_TYPE; 
	opdbv.db_length = sizeof(i4);
	opdbv.db_prec = 0;
	opdbv.db_data = NULL;

	/*
	**  Set up the local DB_DATA_VALUE for the change bit
	**  data for each column.
	*/

	cbdbv.db_datatype = DB_INT_TYPE; 
	cbdbv.db_length = sizeof(i4);
	cbdbv.db_prec = 0;

	/*
	**  Loop through all the displayed columns, putting the empty
	**  value for each into the data chunk for the row.
	**  The exception is for constant-value derived columns when
	**  not in query mode; these columns are evaluated.
	*/

	for (colptr = rds->r_coldesc, colno = 0; colptr != NULL;
		colptr = colptr->c_nxt, colno++)
	{
	    /*
	    **  Clear change and display attribute bits (set them to zero).
	    */
	    attrptr = (i4 *) ((char *) rowdata + colptr->c_chgoffset);
	    *attrptr = 0;

	    hdr = &(tb->tb_fld->tfflds[colno]->flhdr);
	    if ((hdr->fhd2flags & fdDERIVED)
		&& (hdr->fhdrv->drvflags & fhDRVCNST) && !qry)
	    {
		/*
		** If it's a constant derivation, evaluate it.
		*/
		type = &(tb->tb_fld->tfflds[colno]->fltype);
		idbv = IITBgidGetIDBV(frm, &fld, colno);
		if ((IIFVedEvalDerive(frm, hdr, type, idbv)) == FAIL)
		{
		    IIFDidfInvalidDeriveFld(frm, &fld, colno);
		}
		else
		{
		    IIFDudfUpdateDeriveFld(frm, &fld, colno);
		}
	    }
	    else
	    {
		/*
		**  Initialize the column's data value
		*/

		colptr->c_dbv.db_data = (PTR) ( (char *) rowdata +
			colptr->c_offset );
		
		if ((adc_getempty(cb, &colptr->c_dbv)) != OK)
		{
			/* internal error */
			return;
		}

		/*
		**  Initialize the query operator
		*/

		opdbv.db_data = (PTR) ((char *) rowdata + colptr->c_qryoffset);

		if ( ( adc_getempty( cb, &opdbv ) ) != OK )
		{
			/* internal error */
			return;
		}
	    }
	}

	/*
	**  Loop through all the hidden columns, giving each of them the
	**  'empty' value too.  Note that hidden columns don't have
	**  query operators.
	*/

	for ( colptr = rds->r_hidecd; colptr != NULL; colptr = colptr->c_nxt )
	{
		/*
		**  Initialize the hidden column's data value
		*/

		colptr->c_dbv.db_data = (PTR) ( (char *) rowdata +
							colptr->c_offset );

		if ( ( adc_getempty( cb, &colptr->c_dbv ) ) != OK )
		{
			/* internal error */
			return;
		}
	}
}


/*{
** Name:	IITBpgPurgeTable - Routine that support purgetable statement
**
** Description:
**	This routine does the actual work in support of the purgetable
**	statement.  Any deleted rows will be moved to the free list.
**
** Inputs:
**	tb		Pointer to the TBSTRUCT structure for the table
**			field to operate on.
**
** Outputs:
**	None.
**
**	Returns:
**		TRUE		If no problems occurred.
**		FALSE		If there is no dataset associated with
**				the table field or the statement is issued
**				within the context of an unloadtable statement.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	07/18/93 (dkh) - Initial version.
*/
i4
IITBptPurgeTable(tb)
TBSTRUCT	*tb;
{
	DATSET	*ds;
	DSMEM	*dm;
	TBROW	*rp;
	TBROW	*prp;

	if (IIunldstate() == tb)
	{
		/*
		**  If within an unload loop, then issue an error.
		*/
		IIFDerror(E_FI205A_8282, 1, tb->tb_name);
		return(FALSE);
	}

	/*
	**  Check to see if there is a dataset.
	*/
	if ((ds = tb->dataset) == NULL)
	{
		/*
		**  No dataset.  Can't purge a bare table field.
		*/
		IIFDerror(E_FI2059_8281, 1, tb->tb_name);
		return(FALSE);
	}


	/*
	**  Nothing to purge, just return.
	*/
	if ((rp = ds->dellist) == NULL)
	{
		return(TRUE);
	}

	/*
	**  Look for last node in freelist.
	**  Also, set each record to the undefines state
	**  so it can be reused again.
	*/
	for ( ; rp != NULL; rp = rp->nxtr)
	{
		rp->rp_state = stUNDEF;
		prp = rp;
	}

	dm = ds->ds_mem;

	/*
	**  Move deleted list to free list by chaining current
	**  freelist to end of deleted list and
	**  make head of deleted the head of the free list.
	*/
	prp->nxtr = dm->dm_freelist;
	dm->dm_freelist = ds->dellist;

	/*
	**  Clear out deleted list.
	*/
	ds->dellist = NULL;

	return(TRUE);
}
