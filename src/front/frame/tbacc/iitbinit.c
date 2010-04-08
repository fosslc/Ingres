/*	
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<menu.h>
# include 	<runtime.h> 
# include	<frserrno.h>
# include	<me.h>
# include	<er.h>
# include	<rtvars.h>

/**
** Name:	iitbinit.c	-	Link tblfld to dataset
**
** Description:
**
**	Public (extern) routines defined:
**		IItbinit()
**		IItfill()
**	Private (static) routines defined:
**		IIrdget()
**
** History:
**	02-feb-1987	-	Modified IIrdget for COLDESC with 
**				DB_DATA_VALUEs, aligned data buffer (drh)
**	25-jun-87 (bruceb)	Code cleanup.
**	13-jul-87 (bruceb)	Changed memory allocation to use [FM]Ereqmem.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	09/01/87 (dkh) - Added change bit for datasets.
**	09/16/87 (dkh) - Integrated table field change bit.
**	23-mar-89 (bruceb)
**		If table field being inittable'd is the field user was
**		last on (the current field as of return to user code from
**		the FRS), reset origfld to bad value to force an entry
**		activation.
**      19-jul-89 (bruceb)
**              Flag region in dataset is now an i4.
**		Set up for derivation aggregate processing.
**	07/22/89 (dkh) - Fixed bug 6765.
**	13-apr-92 (seg)
**	    Can't dereference or do arithmetic on a PTR variable.  bufptr is
**	    assumed throughout to be (char *), so make it so.
**	29-oct-93 (swm)
**	    Added comments to indicate possible lint truncation warnings
**	    where pointer values are cast to i4  but the code is valid.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

static	ROWDESC	*IIrdget();			/* initialize row descriptor */

GLOBALREF	TBSTRUCT	*IIcurtbl;

FUNC_EXTERN	VOID		IIds_init();
FUNC_EXTERN	VOID		IIFDiaaInvalidAllAggs();

/*{
** Name:	IItbinit	-	Link tblfld to dataset
**
** Description:
**	Links a tablefield to a dataset.
**	
**	Set up correct I/O frame and table.  If the table is already 
**	linked to a data set then free the existing rows (regular
**	and deleted) but keep the row descriptor.  Otherwise allocate
**	space for a new data set and call local IIrdget() to set up
**	the new row descriptor. Update the modes.
**	Call FDtblput() to initialize data strorage area in display.
** 
**	Updates IIcurtbl if initialized table successfully.  This 
**	update allows possible following IIhidecol() to reference the
**	correct table field.
**
**	This routine is part of TBACC's external interface.
**	
** Inputs:
**	formname	Name of the form
**	tabname		Name of the tablefield
**	mode		Mode of the display
**
** Outputs:
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Example and Code Generation:
**	## inittable "form1" "tbl1" fill (hide1=c10)
**
**	if (IItbinit("form1","tbl1","f")!= 0 )
**	{
**		IIthidecol("hide1","c10");
**		IItfill();
**	}
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	04-mar-1983 	- Written (ncg)
**	12-jan-1984	- Added Query mode (ncg)
**	30-apr-1984	- Improved interface to FD routines (ncg)
**	06-jun-1985	- Now uses DSMEM. (ncg)
**	14-jun-1985	- Initial UPDATE mode is READ mode. (ncg)
**
*/

IItbinit (formname, tabname, tabmode)
char	*formname;
char	*tabname;
char	*tabmode;
{
	TBSTRUCT	*tb;		/* table to initialize */
	DATSET		*ds;		/* data set to initialize */
	char		*mode;
	char		m_buf[MAXFRSNAME+1];
	char		*procname = ERx("IItbinit");
	i4		*origfld;
	FRAME		*frm;
	TBLFLD		*tbl;

	/* if not found in set up searches, either error or not there 	*/
	if ( (tb = IItstart(tabname, formname) ) == NULL)
	{
		return (FALSE);
	}

	frm = IIfrmio->fdrunfrm;
	tbl = tb->tb_fld;
	origfld = &(frm->frres2->origfld);
	if (tbl->tfhdr.fhseq == *origfld)
	{
	    /* Illegal sequence number for the 'current' field--force an EA. */
	    *origfld = BADFLDNO;
	}

	IIFDiaaInvalidAllAggs(frm, tbl);

	mode = IIstrconv(II_CONV, tabmode, m_buf, (i4)MAXFRSNAME);
	
	switch (*mode)				/* set tab mode */
	{
	  case 'r':				/* READ */
	  case 'R':
		tb->tb_mode = fdtfREADONLY;
		break;

	  case 'u':				/* UPDATE */
	  case 'U':
		tb->tb_mode = fdtfUPDATE;
		break;

	  case 'f':				/* FILL */
	  case 'F':
		tb->tb_mode = fdtfAPPEND;
		break;

	  case 'q':				/* QUERY */
	  case 'Q':
		tb->tb_mode = fdtfQUERY;
		break;

	  default:
		/* bad initializing mode */
		IIFDerror(TBINITMD, 2, (char *) tb->tb_name, mode);
		/* clean up previous table definition */
		if (tb->tb_state != tbUNDEF)
		{
			if (tb->dataset != NULL)
				IIfree_ds(tb->dataset);
			tb->tb_display = 0;
			tb->tb_rnum = 0;
			tb->tb_mode = fdtfUPDATE;
			tb->tb_state = tbUNDEF;
			FDtblput(tbl, tb->tb_mode, fdopSCRUP, 
				 fdopSCRDN, (i4) fdNOCODE, (i4) fdNOCODE);
		}
		return (FALSE);
		break;
	}
	/*
	 *  If this is a re-initialization of table -- free rows but keep row 
	 *  descriptor of displayed columns (hidden ones are released).
	 */
	if ((ds = tb->dataset) != NULL)
	{
		/* free all rows */
		IIfree_ds(ds);
	}
	else
	{
		/* allocate space for new data set */
		if ((ds = (DATSET *)MEreqmem((u_i4)0, (u_i4)(sizeof(DATSET)),
			TRUE, (STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(procname);
		}
		/* allocate space and initialize row descriptor */
		if ((ds->ds_rowdesc = IIrdget(tb)) == NULL)
		{
			return (FALSE);
		}
		tb->dataset = ds;
		/* allocate new dataset memory manager */
		if ((ds->ds_mem = (DSMEM *)MEreqmem((u_i4)0,
			(u_i4)(sizeof(DSMEM)), TRUE, (STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(procname);
		}
		ds->ds_mem->dm_memtag = FEgettag();
	}
	/* 
	** No hidden column descriptors yet. (If there were any they are lost.
	** Should use a free list for these.)
	*/
	FEfree(ds->ds_rowdesc->r_memtag);
	ds->ds_rowdesc->r_memtag = 0;
	ds->ds_rowdesc->r_hidecd = NULL;

	/*
	** Update the real size of the data set record to the size of the
	** displayed row.  The real size of the data set record may grow if
	** hidden columns are added to the table.
	*/
	ds->ds_rowdesc->r_realsize = ds->ds_rowdesc->r_dissize;

	tb->tb_display = 0;		/* update display and current row # */
	tb->tb_rnum = 0;
	tb->tb_state = tbINIT;

	/*
	** Initialize values in frame's representation of the table field.
	** fdopSCRUP and fdoSCRDN are the codes to return when a cursor-scroll
	** is attempted. (These are handled in IItputfrm() .)
	** UPDATE mode without any rows is initially READ mode.
	*/
	FDtblput(tbl, tb->tb_mode==fdtfUPDATE ? fdtfREADONLY:tb->tb_mode,
		 fdopSCRUP, fdopSCRDN, (i4) fdNOCODE, (i4) fdNOCODE);
	IIcurtbl = tb;

	return (TRUE);
}


/*{
** Name:	IIrdget		-	Set up dataset and col descriptors
**
** Description:
**	Set up the dataset and column descriptors for visible columns.
**
**	Get the table field description from the frame driver and
**	build the column descriptions one by one.  The 'offset' in
**	the columns descriptor is an offset into each row for the
**	actual column/row data value.
**
** Inputs:
**	tb		Ptr to tablefield to create
**
** Outputs:
**
** Returns:
**	Ptr to a row descriptor (ROWDESC) for the tablefield
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	17-feb-1987 (drh)	Modified to use DB_DATA_VALUEs in the
**				column descriptor, and call afe_pad to
**				do correct data alignment in the row buffer.
**	24-mar-1987 (drh)	Use ftlength instead of ftdataln as the
**				data length in the column's DB_DATA_VALUE.
**	13-apr-92 (seg)
**	    Can't dereference or do arithmetic on a PTR variable.  bufptr is
**	    assumed throughout to be (char *), so make it so.
**	19-May-2009 (kschendel) b122041
**	    Since bufptr is really an offset, make it such, and use
**	    straightforward align instead of afe_pad (which just does an
**	    ALIGN_RESTRICT alignment anyway).
**	
*/

static ROWDESC *
IIrdget(tb)
TBSTRUCT	*tb;
{
	ROWDESC			*rd;		/* row descriptor to return */
	register COLDESC	*cd;		/* column desc to fill */
	register i4		i;
	TBLFLD			*tfld;		/* table field in frame */
	FLDCOL			*fcol;		/* column desc. in frame */
	i4			pad;		/* pad area for char data */
	DB_DATA_VALUE		op_dbv;		/* dbv for query operator */
	ADF_CB			*cb;		/* session control block */
	i4			offset;		/* Current layout position */
	char			*procname = ERx("IIrdget");

	/*
	** Allocate a row descriptor
	*/

	if ((rd = (ROWDESC *)MEreqmem((u_i4)0, (u_i4)(sizeof(ROWDESC)), TRUE,
		(STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(procname);	/* never returns */
	}
	
	/*
	**  Get the tablefield descriptor from the frame
	*/

	tfld = tb->tb_fld;

	/*
	**  Find the number of columns in the tablefield, and
	**  allocate the same number of column descriptors
	*/

	rd->r_ncols = tfld->tfcols;

	if ((rd->r_coldesc = (COLDESC *)MEreqmem((u_i4)0,
		(u_i4)(rd->r_ncols * sizeof(COLDESC)),
		TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(procname);
	}

	/*
	**	Initialize a DB_DATA_VALUE for the query operator
	*/

	op_dbv.db_datatype = DB_INT_TYPE;
	op_dbv.db_length = sizeof(i4);
	op_dbv.db_prec = 0;
	op_dbv.db_data = 0;

	cb = FEadfcb();		/* get session control blk ptr */

	/*
	**  Loop through the frame's table field's column descriptors, updating
	**  calculating the aligned offset into the row buffer for the
	**  column's data value, and query operator data value.  Keep
	**  track of the size required for the row buffer.
	*/

	offset = 0;
	for (i = 0, cd = rd->r_coldesc; i < rd->r_ncols; i++, cd++)
	{
		/* get current col desc. in frame */
		
		fcol = tfld->tfflds[i];

		cd->c_name = (char *) STalloc(FDGCName(fcol));
		cd->c_hide = 0;		
		cd->c_dbv.db_datatype = fcol->fltype.ftdatatype;
		cd->c_dbv.db_length = fcol->fltype.ftlength;
		cd->c_dbv.db_prec = fcol->fltype.ftprec;
		cd->c_dbv.db_data = 0;

		/*
		**  Get aligned offset for column's data value
		*/
		offset = DB_ALIGN_MACRO(offset);
		cd->c_offset = offset;	/* offset for col data */

		offset += cd->c_dbv.db_length;

		/*
		**  Get aligned offset for query operator data value
		*/
		offset = DB_ALIGN_MACRO(offset);
		cd->c_qryoffset = offset;

		offset += op_dbv.db_length;

		/*
		**  Get aligned offset for change bit.
		*/
		offset = DB_ALIGN_MACRO(offset);
		cd->c_chgoffset = offset;
		offset += sizeof(i4);	/* Size of flag region. */

		/* convert array to linked list */

		if (i == rd->r_ncols -1)
			cd->c_nxt = NULL;	/* last in list */
		else
			cd->c_nxt = cd +1;
	}

	rd->r_dissize = offset;

	return (rd);
}


/*{
** Name:	IItfill		-	Create 'fake' tblfld row
**
** Description:
** 	Used only if the table field is in FILL or QUERY mode.  After allocating
**	space for hidden columns, then if using an APPEND type table prepare 
**	the "fake" insertion record. This  must be done after the hidden 
**	columns are allocated, as when IItinit() is called, the rd->r_realsize
**	is not yet known.  If no hidden columns were allocated then 
**	rd->r_realsize happens to be rd->r_dissize, and we have not lost 
**	anything.
**
**	Calls II_dsinit to initialize the dataset, and IIscr_fake to 
**	build the fake row.
**
**	This routine is part of TBACC's external interface.
**	
** Inputs:
**
** Outputs:
**
** Returns:
**
** Example and Code Generation:
**	## inittable "form1" "tbl1" fill (hide1=c10)
**
**	if (iitbinit("form1","tbl1","f") != 0 )
**	{
**		IIthidecol("hide1","c10");
**		IItfill();
**	}
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	
*/

IItfill()
{
	IIds_init(IIcurtbl->dataset);		/* Initialize the memory */
	if (IIcurtbl->tb_mode == fdtfAPPEND || IIcurtbl->tb_mode == fdtfQUERY)
	{
		IIscr_fake(IIcurtbl, IIcurtbl->dataset);
	}
}
