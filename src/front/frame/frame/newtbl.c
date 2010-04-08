/*
**	newtbl.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	newtbl.c - Allocate structures for a table field.
**
** Description:
**	This file contains routines to allocate structures for a
**	table field.  Routines defined here are:
**	- FDtblalloc - cover routine to allocate full table field structure.
**	- FDwtblalloc - real allocator of table field structures.
**	- FDnewcol - allocates new table field column structures.
**	- FDcolalloc - cover routine to allocate full column structure.
**	- FDwcolalloc - real allocator of table field column structures.
**
** History:
**	xx/xx/xx (xxx) - Initial version.
**	05/17/85 (dkh) - Added support for forced lower/upper case.
**	02/08/87 (dkh) - Added support for ADTs.
**	19-jun-87 (bruceb) Code cleanup.
**	13-jul-87 (bruceb) Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	12/31/88 (dkh) - Per changes.
**	01/31/89 (dkh) - Moved calculation of field name chekcsums
**			 so vifred/copyform can work properly.
**	07/22/89 (dkh) - Integrated changes from IBM.
**	12/31/89 (dkh) - Integrated IBM porting changes.
**	06-mar-90 (bruceb)
**		NULL out the scroll struct's scr_fmt pointer.
**	17-apr-90 (bruceb)
**		Lint changes.  Arg removed in call on IIFDdsDbufSize.
**		Args also removed in defs of FD[w]tblalloc and FD[w]colalloc.
**	09/27/92 (dkh) - Updated new catch unsupported datatypes.
**	12/12/92 (dkh) - Fixed trigraph warnings from acc.
**	12/12/92 (dkh) - Added include of erfi.h to pick up new error id.
**	12/12/92 (dkh) - Added support for inputmasking toggle.
**	03/12/93 (dkh) - Rolled back previous change since it is no
**			 longer needed.
**	08/18/93 (dkh) - Changed code to eliminate compile problems on NT.
**    11-oct-1996 (angusm)
**    Force zeroing out of memory allocated for data content
**    of table fields in FDwcolalloc() if running MWS.
**    (protocol for MacWs and Upfront).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-Aug-2009 (kschendel) 121804
**	    Need afe.h to satisfy gcc 4.3.
**/

# include	<compat.h>
# include	<gl.h>
# include	<me.h>
# include	<si.h>
# include	<st.h>
# include	<er.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<afe.h>
# include	<frame.h>
# include	<scroll.h>
# include	"erfd.h"
# include	<erfi.h>


FUNC_EXTERN	VOID		IIFDdsDbufSize();
FUNC_EXTERN	i4		IIFDcncCreateNameChecksum();


# define	ADJ_FCTR	(sizeof(ALIGN_RESTRICT) - 1)
# define	SIZE_ADJ(s)	(((s) + ADJ_FCTR) & ~ADJ_FCTR)

/*{
** Name:	FDtblalloc - allocate subcomponents of a table field.
**
** Description:
**	Cover routine to FDwtblalloc() for allocating subcomponents
**	of a table field.  It is assumed that structure members
**	"tfrows", and "tfcols"
**	have already been set before calling this routine.  If this is
**	not the case, then FDwtblalloc() will detect the error.
**
** Inputs:
**	tbl	Pointer to the table field.
**
** Outputs:
**	Returns:
**		TRUE	Successful allocation.
**		FALSE	Unsuccessful allocation.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/08/87 (dkh) - Added support for ADTs.
*/
i4
FDtblalloc(tbl)
reg TBLFLD	*tbl;
{
	return FDwtblalloc(tbl);
}




/*{
** Name:	FDwtblalloc - Real allocator of subcompents for a table field.
**
** Description:
**	Allocate subcomponents of a table field, namely structure members
**	"tfflds" and "tfwins".	It is an error if structure members
**	"tfcols" and "tfrows" are less than or equal to zero.
**	This routine may be called directly from VIFRED/QBF to build
**	default forms and newly created table fields.
**
** Inputs:
**	tbl	Pointer to the table field to allocate subcomponent.
**
** Outputs:
**	Returns:
**		TRUE	If allocation was successful.
**		FALSE	If allocation failed.
**	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	02/08/87 (dkh) - Added support for ADTs.
*/
i4
FDwtblalloc(tbl)
reg TBLFLD	*tbl;
{
    reg FLDHDR	*hdr;
    char	*memptr;
    PTR		alloc_ptr;
    i4		flds_size;
    i4		wins_size;
    i4		flgs_size;
    i4		alloc_size;

    hdr = &tbl->tfhdr;
    if (tbl->tfcols <= 0)
    {
	SIfprintf(stderr, ERget(E_FD0014_no_columns),
		hdr->fhdname);
	SIflush(stderr);
	return(FALSE);
    }
    if (tbl->tfrows <= 0)
    {
	SIfprintf(stderr, ERget(E_FD0015_no_rows),
		hdr->fhdname);
	SIflush(stderr);
	return(FALSE);
    }
    flds_size = SIZE_ADJ(tbl->tfcols * sizeof(FLDCOL *));
    wins_size = SIZE_ADJ(tbl->tfcols * tbl->tfrows * sizeof(FLDVAL));
    flgs_size = SIZE_ADJ(tbl->tfcols * tbl->tfrows * sizeof(i4));
    alloc_size = flds_size + wins_size + flgs_size;

    if ((alloc_ptr = FEreqmem((u_i4) 0, (u_i4) alloc_size, TRUE,
	(STATUS *) NULL)) == NULL)
    {
	IIUGbmaBadMemoryAllocation(ERx("FDwtblalloc"));
	return(FALSE);
    }

    /*
    **  Cast allocated buffer to a char * pointer so we can do
    **  arithmetic on it below.
    */
    memptr = (char *) alloc_ptr;

    tbl->tfflds = (FLDCOL **) memptr;
    memptr += flds_size;
    tbl->tfwins = (FLDVAL *) memptr;
    memptr += wins_size;
    tbl->tffflags = (i4 *) memptr;

    /*
    **	We now just return and let FTnewfrm
    **	allocate windows (or whatever) for us.
    */

    return(TRUE);
}



/*{
** Name:	FDnewcol - Allocate a new table field column structure.
**
** Description:
**	Allocate a new FLDCOL (table field column) structure.
**	Assign the passed in name and sequence number to the appropriate
**	places in the allocated structure.
**
** Inputs:
**	name	Name of column to set in structure.
**	seq	Sequence number to set in structure.
**
** Outputs:
**	Returns:
**		NULL	If allocation was not done.
**		ptr	A successfully allocated FLDCOL structure pointer.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/08/87 (dkh) - Added support for ADTs.
*/
FLDCOL *
FDnewcol(name, seq)
char	*name;
i4	seq;
{
    FLDCOL	*ret;
    reg FLDHDR	*hdr;
    i4		status;
    char	err_buf[256];

    if ((ret = (FLDCOL *)FEreqmem((u_i4)0, (u_i4)(sizeof(FLDCOL)), TRUE,
	(STATUS *)&status)) == NULL)
    {
	ERreport(status, err_buf);
	SIfprintf(stderr, ERx("%s\n"), err_buf);
	SIflush(stderr);
	return NULL;
    }
    hdr = &ret->flhdr;
    STlcopy(name, hdr->fhdname, FE_MAXNAME);
    hdr->fhseq = seq;
    return ret;
}


/*{
** Name:	FDcolalloc - Allocate subcomponents for a column.
**
** Description:
**	Cover routine to allocate subcomponents of a column.  Calls
**	FDwcolalloc to do the real work.
**
** Inputs:
**	tbl	Pointer to a table field in above form.
**	col	Pointer to a column that belongs in above table field.
**
** Outputs:
**	Returns:
**		TRUE	If allocation was successful.
**		FALSE	If allocation failed.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/08/87 (dkh) - Added support for ADTs.
*/
i4
FDcolalloc(tbl, col)			/* FDFLALLOC: */
TBLFLD	*tbl;					/* field pointer */
FLDCOL	*col;
{
    return(FDwcolalloc(tbl, col, WIN));
}


/*{
** Name:	FDwcolalloc - Real allocator for subcomponents of a column.
**
** Description:
**	Routine allocates the data area for each field in the
**	passed in column.  Additional display buffer storage
**	for each field is also allocated if argument "wins" is TRUE.
**	This routine may be called by VIFRED/QBF for building newly
**	created fields or default forms.
**
** Inputs:
**	tbl	Pointer to a table field.
**	col	Pointer to a column that belongs to above table field.
**	wins	Allocate additional components in preparation for
**		allowing user access to column if this is TRUE.
**
** Outputs:
**	Returns:
**		TRUE	If allocation was successful.
**		FALSE	If allocation failed.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/08/87 (dkh) - Added support for ADTs.
**      11-oct-1996(angusm)
**      force zeroing out of memory if running under MWS
*/
i4
FDwcolalloc(tbl, col, wins)		/* FDFLALLOC: */
TBLFLD	*tbl;					/* field pointer */
FLDCOL	*col;
bool	wins;
{
    reg FLDTYPE 	*type;
    reg FLDHDR		*hdr;
    reg FLDVAL		*val;
    FLDHDR		*chdr;
    i4			i;
    char		*memptr;
    char		*mem2ptr;
    PTR			alloc_ptr;
    DB_DATA_VALUE	*dbv;
    DB_TEXT_STRING	*text;
    SCROLL_DATA		*scdata;
    i4			dbvsize;
    i4			structsize;
    i4			allocsize;
    i4			scroll_size;
    i4			a_scroll_size;
    i4			scroll_width;
    i4			dbufsize;
    i4			a_dbufsize;
    i4			datalen;
    i4			scsize;
    i4			can_scroll = FALSE;
    DB_DATA_VALUE	ldbv;
    bool                memmode=FALSE;

    type = &col->fltype;
    chdr = &col->flhdr;
    hdr = &tbl->tfhdr;

    /*
    **  Make sure field has a valid datatype.
    */
    ldbv.db_datatype = type->ftdatatype;
    if (!IIAFfedatatype(&ldbv))
    {
	i4	bad_dtype;

	bad_dtype = type->ftdatatype;
        IIFDerror(E_FI1F54_801A, 3, hdr->fhdname, chdr->fhdname, &bad_dtype);
        return(FALSE);
    }

    val = tbl->tfwins+chdr->fhseq;

    /*
    **  Calculate checksum for table field name.
    */
    if (hdr->fhdfont == 0)
    {
	hdr->fhdfont = IIFDcncCreateNameChecksum(hdr->fhdname);
    }

    if (wins)
    {
    	IIFDdsDbufSize(chdr, type, &dbufsize, &scroll_size);
    	dbvsize = SIZE_ADJ(sizeof(DB_DATA_VALUE));
    	scsize = SIZE_ADJ(sizeof(SCROLL_DATA));
	datalen = SIZE_ADJ(type->ftlength);
	a_dbufsize = SIZE_ADJ(dbufsize);
    	structsize = dbvsize + dbvsize;
	allocsize = structsize + datalen + a_dbufsize;
	if (chdr->fhd2flags & fdSCRLFD)
	{
		can_scroll = TRUE;
		structsize += scsize;
		a_scroll_size = SIZE_ADJ(scroll_size);
		allocsize += scsize + a_scroll_size;
	}
    	allocsize *= tbl->tfrows;
# ifdef IBM370
    	if ((memptr = (PTR) FEreqmem((u_i4) 0, (u_i4) allocsize, TRUE,
# else
	if (IIMWimIsMws())
		memmode = TRUE;
    	if ((alloc_ptr = (PTR) FEreqmem((u_i4) 0, (u_i4) allocsize, memmode,
# endif /* IBM370 */
		(STATUS *) NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("Column-allocation"));
		return(FALSE);
    	}

	/*
	**  Cast allocated buffer to a char * pointer so we can do
	**  arithmetic on it below.
	*/
	memptr = (char *) alloc_ptr;

    	structsize *= tbl->tfrows;

    	mem2ptr = memptr + structsize;
    }

    for (i = 0; i < tbl->tfrows; val += tbl->tfcols, i++)
    {
	/*
	**  Calculate checksum for column name.
	*/
	chdr->fhdfont = IIFDcncCreateNameChecksum(chdr->fhdname);

	if (hdr->fhdflags & fdNOTFLINES)
	{
		val->fvdatay = tbl->tfstart + (i * tbl->tfwidth);
	}
	else
	{
		val->fvdatay = tbl->tfstart + i*(tbl->tfwidth + 1);
	}


	if(!wins)
	{
		continue;
	}

	/*
	**  Allocate field dbv.
	*/
	dbv = (DB_DATA_VALUE *) memptr;
	dbv->db_length = type->ftlength;
	dbv->db_datatype = type->ftdatatype;
	dbv->db_prec = type->ftprec;
	val->fvdbv = dbv;

	memptr += dbvsize;

	if (chdr->fhdflags & fdSTOUT)
	{
		chdr->fhdflags |= fdRVVID;
	}

	/*
	**  Allocate display dbv.
	*/
	dbv = (DB_DATA_VALUE *) memptr;
	dbv->db_datatype = DB_LTXT_TYPE;
	dbv->db_prec = 0;
	val->fvdsdbv = dbv;

	memptr += dbvsize;

	if (can_scroll)
	{
		val->fvdatawin = (PTR *) memptr;
		memptr += scsize;
	}

	/*
	**  Allocate buffer for field dbv.
	**  Don't need to call adc_getempty at this point
	**  since IIaddrun() will eventually cause FDsrowtrv
	**  to run FDdat & FDclr (which will calls adc_getempty)
	**  on each cell of the tablefield.
	*/
	val->fvdbv->db_data = (PTR) mem2ptr;

	/*
	**  Allocate buffer space for display dbv.
	*/
	mem2ptr += datalen;
	dbv->db_data = mem2ptr;
	dbv->db_length = dbufsize;
	mem2ptr += a_dbufsize;

	if (can_scroll)
	{
		/*
		**  If scrolling field, allocate SCROLL_DATA buffer space.
		**  The -1 for scroll_width is necessary, do not change
		**  it unless ftscrfld.c is also changed.
		*/
		scroll_width = scroll_size - 1;
		scdata = (SCROLL_DATA *)val->fvdatawin;
		scdata->left = mem2ptr;
		mem2ptr += a_scroll_size;
		scdata->right = scdata->left + scroll_width - 1;

		/*
		**  Clear out underlying bufer to blanks.
		*/
		IIFTcsfClrScrollFld(scdata,
		    ((chdr->fhdflags & fdREVDIR) ? (bool)TRUE : (bool)FALSE));

		/*
		**  Blank out the extra buffer space at the end of under_buf.
		*/
		*(scdata->right + 1) = ' ';

		scdata->scr_fmt = NULL;
	}

	text = (DB_TEXT_STRING *)dbv->db_data;
	val->fvbufr = text->db_t_text;
    }

    /*
    **	We now just return and let FTnewfrm
    **	allocate windows (or whatever) it needs.
    */

    return(TRUE);
}
