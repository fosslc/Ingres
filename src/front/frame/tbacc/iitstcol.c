/*
**	iitsetcol.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<st.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<menu.h>
# include	<runtime.h>
# include	<frserrno.h>
# include	<me.h>		/* put after runtime due to Greg's change */
# include	<er.h>
# include	<ug.h>
# include	"ertb.h"
# include	<rtvars.h>


/**
** Name:	iitsetcol.c	-	Set column value from variable
**
** Description:
**
**	Public (extern) routines defined:
**		IItcolset()
**		IItcoputio()
**		IITBceColEnd()
**	Private (static) routines defined:
**		tb_set()
**		ds_set()
**		set_col()
**
** History:
**	19-jun-87 (bruceb) Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	09/16/87 (dkh) - Integrated table field change bit.
**	01/21/88 (dkh) - Fixed SUN compilation problems.
**	02/25/88 (dkh) - Fixed to correctly clear change bit when values
**			 are put to a table field that is not currently
**			 displayed.
**	08-jul-88 (bruceb)
**		In ds_set(), call IIFDtccb() with only 3 args, not the
**		4 it was previously being called with.
**	14-apr-89 (bruceb)
**		Allow setting of _STATE in insertrow (in a inittable'd
**		table field) and loadtable statements to either UNDEFINED
**		or UNCHANGED (the default).  If row set to UNDEFINED and
**		anything else is set (or even attempted), generate error
**		message and reset the state to UNCHANGED.  Use
**		IITBceColEnd() as means of determining if user set any
**		other visible column.
**	08-jun-89 (bruceb)
**		New semantics:  can now set _STATE in insertrow and
**		loadtable to UNDEFINED, NEW, UNCHANGED and CHANGE.
**	14-jul-89 (bruceb)
**		Handle setting of derivation source fields.  Disallow
**		setting of values in derived fields.  Also, invalidate
**		aggregate source columns if modified by the put.
**	08/23/90 (dkh) - Put in changes to support table field
**			 cell highlighting.
**	30-aug-90 (bruceb)	Fix for bug 32798.
**		Call IIFRndNullDBV() when adding user values to the table field.
**	09/25/90 (dkh) - Fixed bug 33480.  Put back code that got lost in
**			 codeline shuffle.
**	02/22/91 (dkh) - Fixed bug 35879 (by rolling back the changes
**			 for bug 32798; fix for 35879 will be done in
**			 a different way).
**	19-feb-92 (leighb) DeskTop Porting Change:
**		adh_evcvtdb() has only 3 args, bogus 4th one deleted.
**	07/18/93 (dkh) - Added support to allow _STATE to be set with
**			 the putrow statement.
**      24-sep-96 (hanch04)
**          Global data moved to data.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

/*
** Set IITBccv to FALSE in IItbact and IItinsert; to TRUE in
** IItcoputio if setting any visible columns.
** Check value in IITBceColEnd if state is stUNDEF.
*/
GLOBALREF bool	IITBccvChgdColVals ;

static i4	tb_set();		/* put data into frame table field */
static i4	ds_set();		/* put data into data set record   */
static i4	set_col();		/* set columns in data set records */

/* Allow users to load in an operator */
# define	NOCOLOP		(i4)(-2)
static i4	oper = NOCOLOP;		/* Putoper value and flag */

GLOBALREF	TBSTRUCT	*IIcurtbl;

FUNC_EXTERN	COLDESC *IIcdget();
FUNC_EXTERN	STATUS	adh_evcvtdb();
FUNC_EXTERN	FLDCOL	*FDfndcol();
FUNC_EXTERN	i4	FDdispupd();
FUNC_EXTERN	DB_DATA_VALUE	*IITBgidGetIDBV();
FUNC_EXTERN	i4	*IITBgfGetFlags();
FUNC_EXTERN	STATUS	IIFVedEvalDerive();
FUNC_EXTERN	VOID	IIFDidfInvalidDeriveFld();
FUNC_EXTERN	VOID	IIFDudfUpdateDeriveFld();
FUNC_EXTERN	i4	(*IIseterr())();
FUNC_EXTERN	i4	IIFDdecDerErrCatcher();
FUNC_EXTERN	VOID	IIFViaInvalidateAgg();

/*{
** Name:	IItcolset	-	Set column value from variable
**
** Description:
**	Put the value of a given variable into a column .  This may
**	be a call to just display the value, display the value and
**	store it in the data set, or just store the value in the data
**	set.
**
**	This routine is part of TBACC's external interface as
**	a compatability cover for pre 6.0 EQUEL programs.
**
** Inputs:
**	columnname;		Column name
**	variable;		Variable argument
**	type;			Type
**	len;			Length
**	data;			Ptr to legal C type
**
** Outputs:
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Example and Code Generation:
**	## insertrow "" tbl1 (col1=buf)
**
**	if (IItbsetio(5, "","tbl1",-2) != 0 )
**	{
**		if (IItinsert() != 0 )
**		{
**			IItcolset("col1",1,32,0,buf);
**		}
**	}
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	04-mar-1983	- Written (ncg)
**	30-apr-1984	- Improved interface to FD routines (ncg)
**	03-jul-1984	- Added putrow within unload context (ncg)
**	24-sep-1984	- Added the putoper() option. (ncg)
**	5/17/85		- Added support for forced lower/upper case
**			- attribute support.  Placed here for
**			- efficiency reasons. (dkh)
**	7/2/86		- removed calls to FDcaseconv() since
**			  semantics of force upper/lower have
**			  changed. Fix for bug 9390 (bab)
**	01/10/87 (dkh) - ADT stubbing changes.
**	20-feb-1987	- Converted to a compatability cover routine that
**			  calls t_colput (drh)
**
**	07/25/87 (dkh) - Fixed jup bug 515.
*/

i4
IItcolset(char *colname, i4 variable, i4 type, i4 len, char *data)
{
	return( IItcoputio( colname, (i2 *) NULL, variable, type, len, data ) );
}

/*{
** Name:	IItcoputio	-	Set column value from variable
**
** Description:
**	Put the value of a given variable into a column .  This may
**	be a call to just display the value, display the value and
**	store it in the data set, or just store the value in the data
**	set.
**
**	This routine is part of TBACC's external interface
**
**	Check that the data transfer operation can be executed.	 If the
**	data is being stored in the display then call FDputcol(). If
**	the data is to be loaded into the data set, then just use
**	IIconvert() after type matching. As in IItretcol() there are
**	different cases of setting when dealing with a data set--should
**	the data be displayed after loading into data set; is the
**	column hidden or not...Here too each problem is solved as a
**	unique case.
**
**	There are four possible states that the table field may be
**	in when arriving to this routine.  All states are handled
**	differently, and based on whether a data set is linked or not.
**
**	tbDISPLAY   -	A bare display request.	 The source call may
**			have been SCROLL or DELETEROW with an IN list,
**			(No IN lists can be used if the table is linked
**			to a data set.)	 The source call may also have
**			been a PUTROW or an INSERTROW (with no data set
**			linked)
**	tbINSERT    -	An insertion request.  The source call was
**			INSERTROW with a data set linked.
**	tbLOAD	    -	Loading the data set. Call was LOADTABLE.
**	tbUNLDPUT   -	A PUTROW on the current row within the context
**			of an UNLOADTABLE.  IItsetio() already settup
**			the row number to see if display is required.
**
** Inputs:
**	columnname;		Column name
**	ind;			Null indicator
**	variable;		Variable argument
**	type;			Type
**	len;			Length
**	data;			Ptr to legal C type
**
** Outputs:
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Example and Code Generation:
**	## insertrow "" tbl1 (col1=buf:ind)
**
**	if (IItbsetio(5, "","tbl1",-2) != 0 )
**	{
**		if (IItinsert() != 0 )
**		{
**			IItcoputio("col1",&ind,1,32,4,buf);
**			IITBceColEnd();
**		}
**	}
**
** Exceptions:
**	none
**
** Side Effects:
**	The row/column value will be updated.
**
** History:
**	20-feb-1987	Created from code extracted from IItcolset, modified
**			for ADTs and NULLs (drh)
**
*/

/* VARARGS5 */
i4
IItcoputio(const char *colname, i2 *ind, i4 variable, i4 type, i4 len, void *data)
{
	char			*colptr;	/* to column name */
	char			c_buf[MAXFRSNAME+1];
	DB_EMBEDDED_DATA	edv;
	DB_DATA_VALUE		opdbv;
	ADF_CB			*ladfcb;
	bool			setstate;
	i4			stateval;

	ladfcb = FEadfcb();
	colptr = IIstrconv(II_CONV, colname, c_buf, (i4)MAXFRSNAME);
	if (colptr == NULL )
	{
		/* no column name given for table I/O statement */
		IIFDerror(TBNOCOL, 1, IIcurtbl->tb_name);
		return (FALSE);
	}

	/*
	**  Build an embedded data variable structure from the caller's
	**  parameters
	*/

	edv.ed_type = type;
	edv.ed_length = len;
	edv.ed_data = (variable || type == DB_CHR_TYPE) ? ((PTR) data) : ((PTR) &data );
	edv.ed_null = ind;

	/*
	**  If query operator request, then save in the static oper
	**  variable
	*/

	if (IIputoper((i4)0))
	{
		if (IIcurtbl->tb_mode != fdtfQUERY)
		{
			IIFDerror(TBQRYMD, 1, (char *) IIcurtbl->tb_name);
			return (FALSE);
		}

		opdbv.db_datatype = DB_INT_TYPE;
		opdbv.db_length = sizeof(i4);
		opdbv.db_prec = 0;
		opdbv.db_data = (PTR) &oper;

		if ( adh_evcvtdb( ladfcb, &edv, &opdbv ) != OK )
		{
			if (ladfcb->adf_errcb.ad_errcode ==
				E_AD1012_NULL_TO_NONNULL)
			{
				IIFDerror(TBSNLNNL, 2, IIcurtbl->tb_name,
					colptr);
			}
			else
			{
				IIFDerror(TBDTPOP, 2, IIcurtbl->tb_name,
					colptr);
			}
			return(FALSE);
		}

		if (oper < fdNOP || oper > fdGE )
			return ( FALSE );
	}
	else
	{
		oper = NOCOLOP;
	}

	/*
	** If no data set is linked then we must be in the DISPLAY state.
	** Copy caller's data into corresponding column of frame's table
	** field in the current row.
	*/

	setstate = (STcompare(ERx("_state"), colptr) == 0);
	if (IIcurtbl->dataset == NULL)
	{
		/* Not legal to set _STATE on bare table fields. */
		if (setstate)
		{
		    IIFDerror(E_FI2264_8804_no_STATE, 1, IIcurtbl->tb_name);
		    return(FALSE);
		}
		return (tb_set( colptr, &edv ) );
	}
	else
	{
		if (setstate)
		{
		    /* Can only set _STATE in loadtable or insertrow. */
		    if (IIcurtbl->tb_state != tbLOAD
			&& IIcurtbl->tb_state != tbINSERT
			&& IIcurtbl->tb_state != tbUNLDPUT
			&& IIcurtbl->tb_state != tbDISPLAY)
		    {
			IIFDerror(E_FI2264_8804_no_STATE, 1, IIcurtbl->tb_name);
			return(FALSE);
		    }

		    opdbv.db_datatype = DB_INT_TYPE;
		    opdbv.db_length = sizeof(i4);
		    opdbv.db_prec = 0;
		    opdbv.db_data = (PTR) &stateval;

		    /* Legal values for setting _STATE are 0, 1, 2 and 3. */
		    if (adh_evcvtdb(ladfcb, &edv, &opdbv) != OK)
		    {
			IIFDerror(E_FI2265_8805_bad_STATE_val, 1,
			    IIcurtbl->tb_name);
			return(FALSE);
		    }
		    if (IIcurtbl->tb_state == tbLOAD ||
			IIcurtbl->tb_state == tbINSERT)
		    {
			if ((stateval < stUNDEF) || (stateval > stCHANGE))
		    	{
			    IIFDerror(E_FI2265_8805_bad_STATE_val, 1,
			        IIcurtbl->tb_name);
			    return(FALSE);
		        }
		    }
		    else
		    {
			i4	tstate;

			tstate = IIcurtbl->dataset->crow->rp_state;

			/*
			**  If the current state is stNEW then the
			**  new state can be stNEW, stUNCHANGED or
			**  stCHANGED.  But if the current state is
			**  either stUNCHANGED or stCHANGED, then
			**  the new state may only be stUNCHANGED or
			**  stCHANGED.
			**
			**  No other state transitions are allowed.
			*/
			if (!(((tstate == stNEW) && (stateval == stNEW ||
				stateval == stUNCHANGED ||
				stateval == stCHANGE)) ||
				((tstate == stUNCHANGED ||
				tstate == stCHANGE) &&
				(stateval == stUNCHANGED ||
				stateval == stCHANGE))))
			{
				IIFDerror(E_FI226F_8815_BAD_PUTROW_STATE, 1,
					IIcurtbl->tb_name);
				return(FALSE);
			}

		    }

		    IIcurtbl->dataset->crow->rp_state = stateval;
		    return(TRUE);
		}

		/*
		** Data set I/O request.
		** Fill data set records with caller's data.
		*/
		return (ds_set(colptr, &edv) );
	}

}


/*{
** Name:	tb_set	-	Update frame data value
**
** Description:
**	Copy caller's data directly into the corresponding column
**	of the frame's table field.
**
**	This routine must be in the DISPLAY state to be called.
**
**	This routine is called by t_colput, and is INTERNAL to tbacc.
**
** Inputs:
**	colptr		Column name to update
**	edvptr		Embedded data value to put into the column
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
**	20-feb-1987	Modified for ADTs and NULLs (drh)
**
*/

static i4
tb_set( colptr, edvptr )
char			*colptr;
DB_EMBEDDED_DATA	*edvptr;
{
	register TBSTRUCT	*tb;		/* current table field */
	DB_DATA_VALUE		*dbvptr;
	ADF_CB			*ladfcb;
	FRAME			*frm = IIfrmio->fdrunfrm;
	FLDCOL			*col;
	FLDHDR			*hdr;
	bool			dummy;
	i4			(*oldproc)();
	TBLFLD			*tbl;
	i4			retval = TRUE;
	i4			d_processed = FALSE;

	ladfcb = FEadfcb();

	tb = IIcurtbl;
	tbl = tb->tb_fld;
	/*
	** If column is derived, indicate error and return.  If column
	** doesn't exist, allow existing routines to generate errors.
	** Not a problem for production systems since they shouldn't
	** reference non-existent columns.
	*/
	if ((col = FDfndcol(tbl, colptr)) != NULL)
	{
	    hdr = &(col->flhdr);
	    if (hdr->fhd2flags & fdDERIVED)
	    {
		IIFDerror(E_FI2267_8807_SetDerived, 0, (char *)NULL);
		return( FALSE );
	    }
	}
	if (oper != NOCOLOP)
	{
		/*
		**  Put value into query operator for the column
		**
		*/

		if ( FDsetoper( frm, tb->tb_name, colptr,
			tb->tb_rnum - 1, oper) != OK )
		{
			retval = FALSE;
		}
	}
	else
	{
		/*
		**  Get the form's db_data_value for the row/column
		*/

		if ( FDdbvget( frm, tb->tb_name, colptr,
			tb->tb_rnum - 1, &dbvptr ) != OK )
		{
			return( FALSE );
		}

		if (tb->dataset)
		{
		    hdr->fhd2flags |= fdCOLSET;
		}

		IIFDtccb(tbl, tb->tb_rnum - 1, colptr);

		/*
		**  Convert the embedded data value from the user into the
		**  internal (db_data_value) for the row/column.
		*/
		if (adh_evcvtdb(ladfcb, edvptr, dbvptr) != OK)	 
		{
			if (ladfcb->adf_errcb.ad_errcode ==
				E_AD1012_NULL_TO_NONNULL)
			{
				IIFDerror(TBSNLNNL, 2, tb->tb_name, colptr);
			}
			else
			{
				IIFDerror(TBDTSET, 2, tb->tb_name, colptr);
			}
			retval = FALSE;
		}
		else
		{
			/*
			** If this is a derivation source column, the table
			** field has a dataset, and the table field is not in
			** query mode, validate the column and evaluate the
			** 'set' derivation tree.
			*/
			if (hdr->fhdrv && tb->dataset
			    && (tb->tb_mode != fdtfQUERY))
			{
			    if (tbl->tfhdr.fhd2flags & fdAGGSRC)
			    {
				IIFViaInvalidateAgg(hdr, fdIASET);
				frm->frmflags |= fdINVALAGG;
			    }

			    oldproc = IIseterr(IIFDdecDerErrCatcher);

			    retval = FDvalidate(frm, tbl->tfhdr.fhseq,
				FT_TBLFLD, hdr->fhseq, tb->tb_rnum - 1, &dummy);

			    _VOID_ IIseterr(oldproc);
			    d_processed = TRUE;
			}
			/*
			** If FDvalidate failed (retval is FALSE), than
			** call FDdispupd anyway.  Value should always
			** be shown even if not valid.  Also call
			** FDdispupd if this column has nothing to do
			** with a derivation.
			*/
			if ((retval == FALSE) || !d_processed)
			{
			    /*
			    **  Update the display with the new column value
			    */
			    retval = FDdispupd(frm, tb->tb_name, colptr,
				tb->tb_rnum - 1);
			}
		}
	}
	return(retval);
}



/*{
** Name:	ds_set	-	Update data set for column.
**
** Description:
**	Data set I/O request.
**
**	If loading data into the data set (## LOADTABLE, ## INSERTROW)
**	then update the values in the data set.	 If this is not done
**	then we would not be able to distinguish between states
**	stCHANGED and stUNCHANGED at a later stage (in IIretrow() ).
**	Note that ## PUTROW cannot append or "insert" new data.	 It can
**	only change data, with the result of a final state stCHANGED.
**	Hence a PUTROW will not access data record.
**
**	Always check for hidden column.
**	Now that a PUTROW can be done within an unload we must make
**	sure if the state is tbUNLDPUT then update the data set too.
**
** Inputs:
**	colptr		Column to update
**	edv		Embedded data value to put in the column
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
**	20-feb-1987	Modified for ADT/NULL support (drh)
**
*/

static i4
ds_set(colptr, edvptr)
char			*colptr;
DB_EMBEDDED_DATA	*edvptr;
{
	register TBSTRUCT	*tb;		/* current table field */
	register DATSET		*ds;		/* current data set */
	DB_DATA_VALUE		*dbvptr;	/* data value pointer */
	COLDESC			*cd;		/* column descriptor */
	i4			qop;		/* query oper */
	FRAME			*frm = IIfrmio->fdrunfrm;
	FLDCOL			*col;
	FLDHDR			*hdr;
	i4			colno = BADCOLNO;
	TBLFLD			*tbl;
	i4			*tfflags;
	i4			*dsflags;

	tb = IIcurtbl;
	ds = tb->dataset;

	/*
	** Must be a column request - get column descriptor
	*/
	if ( (cd = IIcdget(tb, colptr)) == NULL)
	{
		/* column name is not found in table */
		IIFDerror(TBBADCOL, 2, tb->tb_name, colptr);
		return (FALSE);
	}

	/*
	** If column is hidden can only use data set record
	*/

	if (cd->c_hide)
	{
		return(set_col(ds, cd, edvptr, colno));
	}

	/* Attempt to set the value of a visible column. */
	IITBccvChgdColVals = TRUE;

	/*
	** If the table is in the DISPLAY state then transfer data to
	** frame's table field.	  (PUTROW)
	*/
	if (tb->tb_state == tbDISPLAY)
	{
		return (tb_set(colptr, edvptr ) );
	}

	/*
	** LOADing or INSERTing - first put directly into data set record
	** and then into frame.
	** A PUTROW using the current row within an UNLODATABLE accesses the
	** data set too.
	*/

	if ( (tb->tb_state != tbLOAD) && (tb->tb_state != tbINSERT) &&
	     (tb->tb_state != tbUNLDPUT) )
	{
		IItbug(ERget(S_TB0007_Bad_table_state));
		return (FALSE);
	}

	tbl = tb->tb_fld;
	col = FDfndcol(tbl, colptr);
	hdr = &(col->flhdr);
	if (hdr->fhd2flags & fdDERIVED)
	{
	    IIFDerror(E_FI2267_8807_SetDerived, 0, (char *)NULL);
	    return( FALSE );
	}
	else if (hdr->fhdrv && (tb->tb_mode != fdtfQUERY) && (oper == NOCOLOP))
	{
	    /* Derivation source column in a non-query mode table field. */
	    colno = hdr->fhseq;
	    if (tbl->tfhdr.fhd2flags & fdAGGSRC)
	    {
		IIFViaInvalidateAgg(hdr, fdIASET);
		frm->frmflags |= fdINVALAGG;
	    }
	}
	hdr->fhd2flags |= fdCOLSET;

	if (!set_col(ds, cd, edvptr, colno))
		return (FALSE);
	/*
	** If we're loading the data set, then we may be past the
	** display bottom, and thus need not display the new info.
	*/
	if ((tb->tb_state == tbLOAD) && (ds->crow != ds->disbot))
	{
		return (TRUE);
	}

	/*
	** If doing a putrow current within an unloadtable, and current row is
	** rowNONE (set by IItsetio()), then no need to display new data.
	** Update the state if necessary.
	*/

	if (tb->tb_state == tbUNLDPUT)
	{
		if (ds->crow->rp_state == stUNDEF)
			ds->crow->rp_state = stNEW;
		else if (ds->crow->rp_state == stUNCHANGED)
			ds->crow->rp_state = stCHANGE;
		if (tb->tb_rnum == rowNONE)
			return (TRUE);
	}
	/*
	** Otherwise, display what was just put into data set.
	*/

	if (oper != NOCOLOP)
	{
		/* Putoper request */

		qop = (i4) *( ( ( char * ) ds->crow->rp_data ) +
					cd->c_qryoffset);
		if ( FDsetoper(frm, tb->tb_name, colptr,
			tb->tb_rnum - 1, qop) != OK )
		{
			IIFDerror(TBDTPOP, 2, IIcurtbl->tb_name, cd->c_name);
			return (FALSE);
		}
		else
		{
			return(TRUE);
		}

	}
	else
	{
		/*
		**  Get the form's db_data_value for the row/column
		*/

		if ( FDdbvget(frm, tb->tb_name, colptr,
			tb->tb_rnum - 1, &dbvptr) != OK )
		{
			return( FALSE );
		}

		/*
		**  Copy the dataset db_data_value into the form's
		*/

		if ( (dbvptr->db_datatype != cd->c_dbv.db_datatype) ||
		     (dbvptr->db_length != cd->c_dbv.db_length) )
		{
			return( FALSE );
		}

		MEcopy( cd->c_dbv.db_data, (u_i2) dbvptr->db_length,
			dbvptr->db_data );

		/*
		** If derivation source field and the table field isn't in
		** query mode, copy the flags from the dataset.
		** colno is set only if the above conditions are met.
		** Do so only for putrow in an unloadtable.  Otherwise
		** do it in IITBceColEnd().
		*/
		if ((colno != BADCOLNO) && (tb->tb_state == tbUNLDPUT))
		{
			tfflags = tbl->tffflags +
				((tb->tb_rnum - 1) * tbl->tfcols) + colno;
			*tfflags &= ~(fdI_CHG | fdX_CHG | fdVALCHKED);
			dsflags = (i4 *) ((char *) ds->crow->rp_data +
				cd->c_chgoffset);
			*tfflags |= (*dsflags &(fdI_CHG | fdX_CHG |fdVALCHKED));
		}
		else
		{
		    IIFDtccb(tbl, tb->tb_rnum - 1, colptr);
		}

		/*
		**  Throw away the fake row flag since the
		**  application is placing a value into the row.
		*/
		tfflags = tbl->tffflags + ((tb->tb_rnum - 1) * tbl->tfcols);
		*tfflags &= ~(fdtfFKROW | fdtfRWCHG);

		/*
		**  Update the display with the new column value.
		**  This call is needed for a few reasons (as opposed to
		**  waiting until IITBceColEnd().  One is that people
		**  linking up applications without re-preprocessing won't
		**  have calls on IITBceColEnd.  The other is that
		**  IITBceColEnd doesn't deal with table fields in query mode.
		*/
		return(FDdispupd(frm, tb->tb_name, colptr, tb->tb_rnum - 1));
	}
}


/*{
** Name:	set_col -	Data transfer with dataset record
**
** Description:
**	Execute data transfer with data set record.  The record is
**	treated as one chunk of data, and the column is referenced via
**	the offset in the column descriptor.
**
**
** Inputs:
**	ds		Ptr to the dataset
**	cd		Ptr to the column descriptor
**	edv		Embedded value to put in the dataset
**	colno		Column number (BADCOLNO if not derivation source column)
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
**	27-nov-1985 (marian)	bug # 6664
**		If table field in Qry mode, then make sure the qry data
**		is not still NULL.
**	20-feb-1987	Added ADT/NULL support (drh)
*/

static i4
set_col(ds, cd, edv, colno)
DATSET			*ds;
register COLDESC	*cd;
DB_EMBEDDED_DATA	*edv;
i4			colno;
{
	PTR		datarec;
	i4		*opptr;
	ADF_CB		*ladfcb;
	FRAME		*frm;
	FIELD		fld;
	i4		*flgs;
	i4		(*oldproc)();

	ladfcb = FEadfcb();

	/* put info into column of current row in data set */

	datarec = ds->crow->rp_data;

	if (oper != NOCOLOP)
	{
		/*
		**  Put query operator value into the column/row data
		*/

		if (cd->c_hide)
		{
			/* cannot query on hidden column */
			IIFDerror(TBQRYHIDE, 2, IIcurtbl->tb_name, cd->c_name);
			return (FALSE);
		}
		opptr = (i4 *) ( ((char *) datarec ) + cd->c_qryoffset );
		*opptr = (i4) oper;

		return (TRUE);
	}
	else
	{
		/*
		**  Put data value into the row/column
		*/

		cd->c_dbv.db_data = (PTR) ( (char *) datarec + cd->c_offset );

		if (adh_evcvtdb(ladfcb, edv, &cd->c_dbv) != OK)	 
		{
			if (ladfcb->adf_errcb.ad_errcode ==
				E_AD1012_NULL_TO_NONNULL)
			{
				IIFDerror(TBSNLNNL, 2, IIcurtbl->tb_name,
					cd->c_name);
			}
			else
			{
				IIFDerror(TBDTSET, 2, IIcurtbl->tb_name,
					cd->c_name);
			}
			return( FALSE );
		}

		/*
		** colno != BADCOLNO indicates a derivation source column.
		** If putrow inside an unloadtable, derive now.  Otherwise,
		** do it in IITBceColEnd().
		*/
		if ((colno != BADCOLNO) && (IIcurtbl->tb_state == tbUNLDPUT))
		{
		    flgs = (i4 *)((char *)datarec + cd->c_chgoffset);
		    *flgs &= ~(fdX_CHG | fdI_CHG | fdVALCHKED);
		    frm = IIfrmio->fdrunfrm;
		    frm->frres2->rownum = NOTVISIBLE;
		    frm->frres2->dsrow = (PTR)ds->crow;
		    fld.fltag = FTABLE;
		    fld.fld_var.fltblfld = IIcurtbl->tb_fld;

		    oldproc = IIseterr(IIFDdecDerErrCatcher);

		    _VOID_ IIFDvsfValidateSrcFld(frm, &fld, colno);
		    _VOID_ IIFDsdvSetDeriveVal(frm, &fld, colno);

		    _VOID_ IIseterr(oldproc);
		}
	}

	/*
	** BUG 6664: If table field in Qry mode, then make sure the qry data
	** is not still NULL, as it will translate later to fdNOP, which will
	** zap the real data. (ncg)  Updated for ADT/NULL (drh)
	*/

	if (IIcurtbl->tb_mode == fdtfQUERY)
	{
		opptr = (i4 *) ( ((char * )datarec) + cd->c_qryoffset) ;

		if (*opptr == (i4) fdNOP)	/* Default initialization  */
		{
			*opptr = (i4) fdEQ;
		}
	}
	return (TRUE);
}


/*{
** Name:	IITBceColEnd -	Do work after a variety of 3GL commands against
**				a row.
**
** Description:
**	Currently called after the following 3GL statements:
**		loadtable
**		insertrow
**		putrow
**		getrow
**		clearrow
**		unloadtable
**
**	If the data set row state was set to stUNDEF, and user also set the
**	value of a visible column, generate a warning message.
**
**	Also used to properly set tflastrow if nothing in a newly added row
**	was made visible (such as if only hidden columns were modified)--which
**	would otherwise update tflastrow.
**
**	Handle derivations if inserting or loading rows.
**
** Inputs:
**
** Outputs:
**
** Returns:
**	VOID
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	17-apr-89 (bruceb)	Written.
*/

VOID
IITBceColEnd()
{
    DATSET	*ds;
    TBROW	*rp;
    TBLFLD	*tbl;
    i4		tstate;
    i4		rownum;
    i4		col;
    FLDHDR	*hdr;
    FRAME	*frm;
    COLDESC	*cd;
    FIELD	fld;
    bool	dummy;
    i4		(*oldproc)();
    FLDVAL	*val;
    DB_DATA_VALUE	*idbv;

    if ((ds = IIcurtbl->dataset) != NULL)
    {
	tstate = IIcurtbl->tb_state;
	frm = IIfrmio->fdrunfrm;
	tbl = IIcurtbl->tb_fld;

	if (tstate == tbINSERT || tstate == tbLOAD)
	{
	    rownum = IIcurtbl->tb_rnum - 1;
	    rp = ds->crow;

	    /*
	    ** If nothing visible was added to the row, then
	    ** tflastrow never updated.  Only useful for loadtable.
	    */
	    if ((tstate == tbLOAD) && (rownum > tbl->tflastrow))
	    {
		tbl->tflastrow = rownum;
	    }

	    /*
	    ** If user set the _STATE value to 0 in this loadtable or insertrow
	    ** statement, and set something else as well, well that's bad.
	    ** Generate a warning message.
	    */
	    if ((rp->rp_state == stUNDEF) && (IITBccvChgdColVals == TRUE))
	    {
		IIFDerror(E_FI2266_8806_data_in_undef, 1, IIcurtbl->tb_name);
	    }

	    /*
	    ** Display any validated, derived columns.  Only needed for
	    ** insertrow and loadtable since the value is already visible
	    ** for other commands.  If the table field is in query mode,
	    ** just turn off the fdVALCHKED flag in the display.
	    */

	    /*
	    ** If inserting, rownum is the TF row number.  Else, if loading,
	    ** then if the current row (rp) is the bottom row, then also
	    ** visible since always load at the end of the ds.
	    */
	    if (IIcurtbl->tb_mode != fdtfQUERY);
	    {
		oldproc = IIseterr(IIFDdecDerErrCatcher);

		if ((tstate == tbINSERT) || (rp == ds->disbot))
		{
		    for (col = 0, cd = IIcurtbl->dataset->ds_rowdesc->r_coldesc;
			col < tbl->tfcols; col++, cd = cd->c_nxt)
		    {
			hdr = &(tbl->tfflds[col]->flhdr);
			/*
			** If not involved in a derivation, just display the
			** value.  Otherwise, if a derivation source field,
			** or a constant-value derivation, validate the
			** value and derive, derive, derive.  Of course, it's
			** somewhat useful to copy the constant derivation
			** value from the dataset into the table field before
			** validating.  Other than for constant-value
			** derivations, only display if actually set by
			** the user.
			*/
			if (hdr->fhdrv == NULL)
			{
			    if (hdr->fhd2flags & fdCOLSET)
			    {
				_VOID_ FDdispupd(frm, IIcurtbl->tb_name,
				    hdr->fhdname, rownum);
			    }
			}
			else if (!(hdr->fhd2flags & fdDERIVED))
			{
			    if (hdr->fhd2flags & fdCOLSET)
			    {
				/* Derivation source column. */
				if (tbl->tfhdr.fhd2flags & fdAGGSRC)
				{
				    IIFViaInvalidateAgg(hdr, fdIASET);
				    frm->frmflags |= fdINVALAGG;
				}
				_VOID_ FDvalidate(frm, tbl->tfhdr.fhseq,
				    FT_TBLFLD, col, rownum, &dummy);
			    }
			}
			else if (hdr->fhdrv->drvflags & fhDRVCNST)
			{
			    if (tbl->tfhdr.fhd2flags & fdAGGSRC)
			    {
				IIFViaInvalidateAgg(hdr, fdIASET);
				frm->frmflags |= fdINVALAGG;
			    }
			    val = tbl->tfwins + (rownum * tbl->tfcols) + col;
			    idbv = val->fvdbv;
			    /*
			    ** Coldesc's db_data was set up in set_col().
			    */
			    MEcopy(cd->c_dbv.db_data, (u_i2)idbv->db_length,
				idbv->db_data);
			    IIFDtccb(tbl, rownum, hdr->fhdname);
			    /*
			    ** Calling FDvalidate to display the constant
			    ** and any of its dependents.
			    */
			    _VOID_ FDvalidate(frm, tbl->tfhdr.fhseq, FT_TBLFLD,
				col, rownum, &dummy);
			}
		    }
		}
		else	/* Dataset rows. */
		{
		    frm->frres2->rownum = NOTVISIBLE;
		    frm->frres2->dsrow = (PTR)ds->crow;
		    fld.fltag = FTABLE;
		    fld.fld_var.fltblfld = tbl;

		    for (col = 0, cd = IIcurtbl->dataset->ds_rowdesc->r_coldesc;
			col < tbl->tfcols; col++, cd = cd->c_nxt)
		    {
			hdr = &(tbl->tfflds[col]->flhdr);
			/*
			** Only validate and derive if a derivation source
			** field (not itself derived) or a constant-value
			** derivation.
			*/
			if (hdr->fhdrv
			    && (!(hdr->fhd2flags & fdDERIVED)
				|| (hdr->fhdrv->drvflags & fhDRVCNST)))
			{
			    if (tbl->tfhdr.fhd2flags & fdAGGSRC)
			    {
				IIFViaInvalidateAgg(hdr, fdIASET);
				frm->frmflags |= fdINVALAGG;
			    }
			    _VOID_ IIFDvsfValidateSrcFld(frm, &fld, col);
			    _VOID_ IIFDsdvSetDeriveVal(frm, &fld, col);
			}
		    }
		}

		_VOID_ IIseterr(oldproc);
	    }
	}

	/*
	** If any columns have been 'set' (tb_set/ds_set), then turn
	** off the per-column flag and also the global indicator.
	*/
	if (IITBccvChgdColVals)
	{
	    for (col = 0; col < tbl->tfcols; col++)
	    {
		hdr = &(tbl->tfflds[col]->flhdr);
		hdr->fhd2flags &= ~fdCOLSET;
	    }
	    IITBccvChgdColVals = FALSE;
	}
    }
}
