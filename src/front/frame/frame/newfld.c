/*
**	newfld.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	newfld.c - Allocate structures and data for a new field.
**
** Description:
**	This file contains routines to allocate data structures for
**	new fields.  Routines defined here are:
**	- FDnewfld - Allocate a new field.
**	- FDflalloc - Cover routine for FDwflalloc.
**	- FDwflalloc - Allocate subcomponents of a field.
**	- FDstoralloc - allocate data storage for a field.
**	- FDdbufalloc - allocate display buffer storage for a field.
**
**	This routine is used for allocating space for fields in the frame
**	driver.  It allocates windows, structs, and storage for fields
**	and evaluates data as needed.
**
**	Fdnewfld() - allocate field struct and sequence number
**	Arguments:  name  -  name of the field
**		    fldno -  number of this field
**
**	Fdflalloc() - evaluate data in field struct and allocate other
**			structures, windows, and storage areas.
**
**
**
** History:
**	JEN - 1 Nov 1981 (written)
**	12-20-84 (rlk) - Replaced ME*alloc calls with FE*alloc calls.
**	5/17/85 - Added support for forced lower/upper case.
**	02/06/87 (dkh) - Added support for ADTs.
**	04/04/87 (dkh) - Deleted references to adc_lenchk.
**	19-jun-87 (bruceb)	Code cleanup.
**	13-jul-87 (bruceb)	Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	09-nov-87 (bruceb)
**		Added code to allocate scrolling field data structures
**		when appropriate, including a larger buffer for the
**		display dbv.
**	08-feb-88 (bruceb)
**		Changed buffer allocation size calculations to use sizeof
**		db_t_count rather than sizeof u_i2.
**	09-feb-88 (bruceb)
**		fdSCRLFD now in fhd2flags; can't use most significant
**		bit of fhdflags.
**	10/25/88 (dkh) - Performance changes.
**	12/31/88 (dkh) - Perf changes.
**	01/31/89 (dkh) - Moved calculation of field name checksums
**			 so vifred/copyform can work properly.
**	07/11/89 (dkh) - Fixed bug 6756.
**	08/03/89 (dkh) - Updated with interface change to adc_lenchk().
**	06-mar-90 (bruceb)
**		NULL out the scroll struct's scr_fmt pointer.
**	17-apr-90 (bruceb)
**		Lint changes.  Removed args in FD[w]flalloc and IIFDdsDbufSize.
**	09/27/92 (dkh) - Updated new catch unsupported datatypes.
**	12/12/92 (dkh) - Added include of erfi.h to pick up new error id.
**	12/12/92 (dkh) - Added support for inputmasking toggle.
**	03/12/93 (dkh) - Rolled back previous change since it is no
**			 longer needed.
**	08/18/93 (dkh) - Changed code to eliminate compile problems on NT.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-Aug-2009 (kschendel) 121804
**	    Need afe.h to satisfy gcc 4.3.
**/


# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<adf.h>
# include	<afe.h>
# include	<fmt.h>
# include	<frame.h>
# include	<frserrno.h>
# include	"fdfuncs.h"
# include	<si.h>
# include	<er.h>
# include	<scroll.h>
# include	<erfi.h>

FUNC_EXTERN	VOID		IIFDdsDbufSize();
FUNC_EXTERN	i4		IIFDcncCreateNameChecksum();


# define	ADJ_FCTR	(sizeof(ALIGN_RESTRICT) - 1)
# define	SIZE_ADJ(s)	(((s) + ADJ_FCTR) & ~ADJ_FCTR)


/*{
** Name:	FDnewfld - Allocate data structures for a new field.
**
** Description:
**	Allocate the appropriate skeleton data structures for a field.
**	Different structures are allocated based on whether a regular
**	or table field is being allocated.
**	Subcomponents of the regular or table field are not allocated
**	here.  The name of the field, sequence number and type of
**	field are set to the passed in values.
**
** Inputs:
**	name	Name of the field.
**	fldno	Sequence number of field.
**	type	Type of field to allocate (FREGULAR for a reguarl field and
**		FTABLE for a table field).
**
** Outputs:
**	Returns:
**		NULL	If allocation failed.
**		ptr	Pointer to allocated field.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/06/87 (dkh) - Added support for ADTs.
*/
FIELD *
FDnewfld(name, fldno, type)		/* FDNEWFLD: */
reg char	*name;			/* name of field to be created */
reg i4		fldno;			/* sequence number of the field */
i4		type;
{
    FIELD	*fld;
    FLDHDR	*hdr;
    STATUS	status;
    i4		fld_size;
    i4		rt_size;
    char	*memptr;
    PTR		alloc_ptr;
    char	err_buf[256];

    fld_size = SIZE_ADJ(sizeof(FIELD));
    if (type == FREGULAR)
    {
	rt_size = SIZE_ADJ(sizeof(REGFLD));
    }
    else
    {
	rt_size = SIZE_ADJ(sizeof(TBLFLD));
    }
    if ((alloc_ptr = FEreqmem((u_i4) 0, (u_i4) (fld_size + rt_size), TRUE,
	&status)) == NULL)
    {
	ERreport(status, err_buf);
	SIfprintf(stderr, ERx("%s\n"), err_buf);
	SIflush(stderr);
	return(NULL);
    }

    /*
    **  Cast allocated buffer to a char * pointer so we can do
    **  arithmetic on it below.
    */
    memptr = (char *) alloc_ptr;

    fld = (FIELD *) memptr;
    memptr += fld_size;
    fld->fltag = type;
    if (type == FREGULAR)
    {
    	fld->fld_var.flregfld = (REGFLD *) memptr;
	hdr = &fld->fld_var.flregfld->flhdr;
    }
    else
    {
	fld->fld_var.fltblfld = (TBLFLD *) memptr;
	hdr = &fld->fld_var.fltblfld->tfhdr;
    }

    STlcopy(name, hdr->fhdname, FE_MAXNAME);
    hdr->fhseq = fldno;
    return(fld);
}



/*{
** Name:	FDflalloc - Allocate subcomponents of a regular field.
**
** Description:
**	Cover routine for allocating subcomponents of a regular field.
**	Calls FDwflalloc() to do the real work.
**
** Inputs:
**	FIELD	Pointer to the field for additional allocation.
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
**	02/06/87 (dkh) - Added support for ADTs.
*/
i4
FDflalloc(tf)				/* FDFLALLOC: */
FIELD		*tf;
{
	return FDwflalloc(tf, WIN);
}



/*{
** Name:	FDwflalloc - Real allocator of regular field subcomponent.
**
** Description:
**	Does the real work of allocating subcomponents of a regular field.
**	It is an error if the passed in field is not a regular field.
**	This routine may be called by VIFRED/QBF to build fields
**	directly.
**
** Inputs:
**	tf	Pointer to field.
**	wins	Allocate additional subcomponents (the display buffer)
**		if this is TRUE.
**
** Outputs:
**	Returns:
**		TRUE	If allocation was successful.
**		FALSE	If not a regular field or out of memory.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/06/87 (dkh) - Added support for ADTs.
*/
i4
FDwflalloc(tf, wins)			/* FDFLALLOC: */
FIELD		*tf;
bool		wins;
{
    reg FLDHDR		*hdr;			/* field pointer */
    FLDTYPE		*type;
    FLDVAL		*val;
    FLDHDR		*FDgethdr();
    FLDTYPE		*FDgettype();
    FLDVAL		*FDgetval();
    char		*memptr;
    PTR			alloc_ptr;
    DB_DATA_VALUE	*dbv;
    DB_TEXT_STRING	*text;
    SCROLL_DATA		*scdata;
    i4			dbvsize;
    i4			allocsize;
    i4			dbufsize;
    i4			a_dbufsize;
    i4			scroll_size;
    i4			a_scroll_size;
    i4			datalen;
    i4			scroll_width;
    i4			a_scdata_size;
    DB_DATA_VALUE	ldbv;


    if (tf->fltag != FREGULAR)
    	return(FALSE);
    hdr = FDgethdr(tf);
    type = FDgettype(tf);
    val = FDgetval(tf);

    /*
    **  Make sure field has a valid datatype.
    */
    ldbv.db_datatype = type->ftdatatype;
    if (!IIAFfedatatype(&ldbv))
    {
	i4	bad_dtype;

	bad_dtype = type->ftdatatype;
	IIFDerror(E_FI1F53_8019, 2, hdr->fhdname, &bad_dtype);
	return(FALSE);
    }

    /*
    **  Calculate checksum for field name.
    */
    hdr->fhdfont = IIFDcncCreateNameChecksum(hdr->fhdname);

    if (!wins)
    {
	return(TRUE);
    }

    dbvsize = SIZE_ADJ(sizeof(DB_DATA_VALUE));

    IIFDdsDbufSize(hdr, type, &dbufsize, &scroll_size);

    a_dbufsize = SIZE_ADJ(dbufsize);
    a_scroll_size = SIZE_ADJ(scroll_size);
    datalen = SIZE_ADJ(type->ftlength);
    a_scdata_size = SIZE_ADJ(sizeof(SCROLL_DATA));

    /*
    **  Allocate one chunk of memory needed for a field.
    */
    allocsize = dbvsize + dbvsize + datalen + a_dbufsize + a_scdata_size +
	a_scroll_size;
    if ((alloc_ptr = (PTR) FEreqmem((u_i4) 0, (u_i4) allocsize,
	FALSE, (STATUS *) NULL)) == NULL)
    {
	IIUGbmaBadMemoryAllocation(ERx("FDflalloc"));
	return(FALSE);
    }

    /*
    **  Cast allocated buffer to a char * pointer so we can do
    **  arithmetic on it below.
    */
    memptr = (char *) alloc_ptr;

    /*
    **  Allocate field dbv.
    */
    dbv = (DB_DATA_VALUE *) memptr;
    dbv->db_length = type->ftlength;
    dbv->db_datatype = type->ftdatatype;
    dbv->db_prec = type->ftprec;

    val->fvdbv = dbv;

    memptr += dbvsize;

    if (hdr->fhdflags & fdSTOUT)
    {
	hdr->fhdflags |= fdRVVID;
    }

    /*
    **  Allocate display dbv.
    */
    dbv = (DB_DATA_VALUE *) memptr;
    dbv->db_datatype = DB_LTXT_TYPE;
    val->fvdsdbv = dbv;

    memptr += dbvsize;

    if (hdr->fhd2flags & fdSCRLFD)
    {
	/*
	**  If scrolling field, allocate the SCROLL_DATA structure.
	*/
        val->fvdatawin = (PTR *) memptr;
        memptr += a_scdata_size;
    }

    /*
    **  Allocate buffer for field dbv and initialize.
    */
    val->fvdbv->db_data = memptr;
    _VOID_ adc_getempty(FEadfcb(), val->fvdbv);

    /*
    **  Allocate buffer space for display dbv.
    */
    memptr += datalen;
    dbv->db_data = memptr;
    dbv->db_length = dbufsize;
    dbv->db_prec = 0;

    if (hdr->fhd2flags & fdSCRLFD)
    {
	/*
	**  If scrolling field, allocate SCROLL_DATA buffer space.
	**  The -1 for scroll_width is necessary, do not change
	**  it unless ftscrfld.c is also changed.
	*/
	scroll_width = scroll_size - 1;
	memptr += a_dbufsize;
	scdata = (SCROLL_DATA *)val->fvdatawin;
	scdata->left = memptr;
	scdata->right = scdata->left + scroll_width - 1;

	/*
	**  Clear out underlying buffer to blanks.
	*/
	IIFTcsfClrScrollFld(scdata,
	    ((hdr->fhdflags & fdREVDIR) ? (bool) TRUE: (bool) FALSE));

	/*
	**  Blank out the extra buffer space at the end of under_buf.
	*/
	*(scdata->right + 1) = ' ';

	scdata->scr_fmt = NULL;
    }

    text = (DB_TEXT_STRING *)dbv->db_data;
    text->db_t_count = 0;
    val->fvbufr = text->db_t_text;

    /*
    **  We just return now since FTnewfrm will
    **  allocate the window stuff for us.
    */

    return(TRUE);
}




/*{
** Name:	IIFDdsDbufSize - Calculate display buffer sizes for
**				 a field/column.
**
** Description:
**	Calculate the display buffer and scrolling field sizes (if
**	a scrolling field) for a field/column.
**
** Inputs:
**	hdr	Pointer to FLDHDR structure for field.
**	type	Pointer to FLDTYPE structure for field.
**
** Outputs:
**	None.
**
**	Returns:
**		dbufsize	Size of display buffer.
**		scrollsize	Size of scroll buffer if a scrolling field.
**				Else 0.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	11/28/88 (dkh) - Initial version.
*/
VOID
IIFDdsDbufSize(hdr, type, dbufsize, scrollsize)
FLDHDR	*hdr;
FLDTYPE	*type;
i4	*dbufsize;
i4	*scrollsize;
{
	i4		scroll_width;
	DB_DATA_VALUE	tdbv;
	DB_DATA_VALUE	sdbv;
	DB_TEXT_STRING	dbts;	/* dummy variable used for sizeof */

	*scrollsize = 0;

	if (hdr->fhd2flags & fdSCRLFD)
	{
		tdbv.db_datatype = type->ftdatatype;
		tdbv.db_length = type->ftlength;
		tdbv.db_prec = type->ftprec;
		_VOID_ adc_lenchk(FEadfcb(), FALSE, &tdbv, &sdbv);

		scroll_width = sdbv.db_length;

		/* length of scrolling field's under_buf plus overhead */
		/*
		** Calculation is based on the size of a DB_TEXT_STRING's
		** db_t_count field plus the length of the buffer.
		*/
		*scrollsize = scroll_width + 1;

		*dbufsize = scroll_width + sizeof(dbts.db_t_count);
	}
	else
	{
		/*
		** Calculation is based on sthe size of a DB_TEXT_STRING's
		** db_t_count field plus the length of the buffer.
		*/
		*dbufsize = (((type->ftwidth + type->ftdataln - 1)/
			type->ftdataln) * type->ftdataln) +
			sizeof(dbts.db_t_count);
	}
}
