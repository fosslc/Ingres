/*
**	iiretrow.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include       <adudate.h>   
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<menu.h>
# include	<runtime.h>
# include	<afe.h>
# include	<frserrno.h>
# include       <er.h>

/**
** Name:	iiretrow.c
**
** Description:
**	Update the tablefield's data set with values from the 
**	displayed form
**
**	Public (extern) routines defined:
**		IIretrow()
**	Private (static) routines defined:
**
** History:
**	08/14/87 (dkh) - ER changes.
**	09/01/87 (dkh) - Added change bit for dataset.
**	09/16/87 (dkh) - Integrated table field change bit.
**	11/12/87 (dkh) - Put in hack for C datatype comparison.
**	17-apr-89 (bruceb)
**		Now that users can set the _STATE value of rows to
**		stUNDEF, when checking for changed values to update
**		the _STATE value, check for both stNEWEMPT and stUNDEF.
**	07/11/89 (dkh) - Fixed bug 6629.
**	19-jul-89 (bruceb)
**		Flag region in dataset is now an i4.  Change bit code now
**		changed to deal with fdI_CHG, fdX_CHG, fdVALCHKED.
**	16-aug-89 (bruceb)
**		Derived columns are ignored in determining if the row
**		has changed.
**	07/26/90 (dkh) - Added support for table field cell highlighting.
**	02/05/91 (dkh) - Added support for 4gl table field qualifcation.
**	12/28/92 (dkh) - Fixed bug 48170 & 48526.  Support for abf table field
**			 qualification now properly handles rows that
**			 are initially empty and later updated.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-May-2002 (hanje04)
**	    Replaced nat bought in by X-Integ with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

FUNC_EXTERN	STATUS	adc_compare();
FUNC_EXTERN	COLDESC	*IIcdget();

GLOBALREF	TBSTRUCT	*IIcurtbl;

/*{
** Name:	IIretrow	-	Update data set with data from form
**
** Description:
**	Get the data for a tablefield row from the form.  If it's different
**	from the data in the corresponding data set row, update the
**	row with the new data.
**
**	Using the column descriptors of the passed table fetch a 
**	"chunk" of data from the display. Offset acts as a pointer into
**	this data record to fetch the "chunk".
**
**	Uses FDgetcol to get a pointer to the internal value for the row/
**	column from the form.  Compares that DB_DATA_VALUE to the corresponding
**	one from the data by calling adc_compare.  If the values are 
**	different, copies the value from the form into the dataset.  Does
**	the same for the query operator if the tablefield is in query mode.
**
** Inputs:
**	tb		Ptr to the tablefield
**	rownum		Number of the displayed row to use
**	rp		Ptr to the dataset row to update
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
**	If the displayed value and the dataset values for the row are
**	different, the dataset is updated with the values from the form.
**
** History:
**	04-mar-1983 	- written (ncg)
**	05-oct-1983	- comparisons for floats (ncg)
**	12-jan-1984	- added query mode test (ncg)
**	30-apr-1984	- Improved interface to FD routines (ncg)
**	06-jun-1985	- Now uses local work buffer (ncg)
**	23-feb-1987	- modified for ADTs and NULLs (drh)
**	3-oct-86 (bruceb)	fix for bug 10360
**		rather than comparing floats as numerics to
**		determine status of _STATE, format both dataset
**		and table fields data and STcompare them.
**		Re-format the table field's float because the
**		fvbufr string isn't always set when routine
**		is called.
**	06/11/87 (dkh) - Changed to deal with query ops properly.
**      21-may-2001 (fruco01)	
**	          Included header file adudate.h for AD_DN_TIMESPEC flag.   	**		  Added the logic to force date fields with specified but zero
**		  times to differ from date fileds with blank time. Bug #101931.
**		     
*/

void
IIretrow(TBSTRUCT *tb, i4 rownum, TBROW *rp)
{
	register ROWDESC	*rd;		/* row desc of data set */
	register COLDESC	*cd;		/* column descriptor */
	register i4		i;
	i4			result;		/* comparison result */
	i4			oper;
	i4			*dsopptr;
	DB_DATA_VALUE		opdbv;		/* data val for query oper */
	DB_DATA_VALUE		*flddbv;	/* field's internal data val */
	i2			chg = 0;	/* set if data was changed */
	i4			chgbit;
	i4			*dsflags;
	i4			isctype;
	STATUS			stat;
	DB_DT_ID		oldtype;
	TBLFLD			*tf;
	FLDCOL			**fcol;
	FLDHDR			*hdr;
	i4			*tfflags;
	i4			scr_row;


	rd = tb->dataset->ds_rowdesc;
	tf = tb->tb_fld;

	/* 
	**  Fetch displayed data and check for changes. 
	**  Hidden columns are ignored.
	*/

	for (i = 0, cd = rd->r_coldesc, fcol = tf->tfflds; i < rd->r_ncols;
		i++, cd = cd->c_nxt, fcol++)
	{
		FDgetcol((i4)FALSE, tf, rownum, cd->c_name, &flddbv );

		/*
		**  Get change bit for column.
		*/
		IIFDgccb(tf, rownum, cd->c_name, &chgbit);

		cd->c_dbv.db_data = (PTR) (  cd->c_offset + 
				( (char *) rp->rp_data ) );
		
		/*
		**  Check if we are doing a compare for C datatype.
		**  If so, then change to CHAR before doing comparison.
		**  This will sidestep blanks are NOT significant
		**  for C datatype.  This is a cheat for speed.
		**  The handling of the nullable datatype is sneaky
		**  but necessary since there is no easy to make the
		**  datatype to be nullable without changing the size
		**  of the dbv.
		*/
		if (abs(flddbv->db_datatype) == DB_CHR_TYPE)
		{
			oldtype = flddbv->db_datatype;
			if (AFE_NULLABLE_MACRO(oldtype))
			{
				opdbv.db_datatype = DB_CHA_TYPE;
				opdbv.db_length = 10;
				opdbv.db_prec = 0;
				AFE_MKNULL_MACRO(&opdbv);
				flddbv->db_datatype = cd->c_dbv.db_datatype =
					opdbv.db_datatype;
			}
			else
			{
				flddbv->db_datatype = DB_CHA_TYPE;
				cd->c_dbv.db_datatype = DB_CHA_TYPE;
			}
			isctype = TRUE;
		}
		else
		{
			isctype = FALSE;
		}

		stat = adc_compare(FEadfcb(), flddbv, &cd->c_dbv, &result);

		   
		if (isctype)
		{
			flddbv->db_datatype = oldtype;
			cd->c_dbv.db_datatype = oldtype;
		}

		if (stat != OK)
		{
			return;
		}


          /* If this is a datetype and the form field we've just read's  */
	  /* AD_DN_TIMESPEC flag is different from the database rec      */
	  /* then we want to write the form record so we fudge  the      */
	  /* result even though adc_compare returned zero. See b101931   */

                if ( DB_DTE_TYPE ==  abs((i4) flddbv->db_datatype) ) 
		{
                   if( !result )
		   { 
		       if( (((AD_DATENTRNL*)flddbv->db_data)->dn_status
			    & AD_DN_TIMESPEC)  !=
                            (((AD_DATENTRNL*)cd->c_dbv.db_data)->dn_status 
			    & AD_DN_TIMESPEC) ) 

			     result = 1;
                   }
                }

		if ( result != 0 )	
		{
			hdr = &((*fcol)->flhdr);
			/* Only indicate a change if not derived. */
			if (!(hdr->fhd2flags & fdDERIVED))
			    chg++;

			/* 
			**  Field for the row/column and the dataset have
			**  different values.  Update the dataset.
			*/

			MEcopy( flddbv->db_data, (u_i2) cd->c_dbv.db_length,
				cd->c_dbv.db_data );
		}

		/*
		**  Set change bit into dataset, regardless of anything
		**  else.
		*/
		dsflags = (i4 *) (cd->c_chgoffset + ((char *) rp->rp_data));
		*dsflags &= ~(fdI_CHG | fdX_CHG | fdVALCHKED);
		*dsflags |= chgbit;

		scr_row = rownum;
		FDtblOK(tf, &scr_row);
		tfflags = tf->tffflags + (scr_row * tf->tfcols) + i;
		if (*tfflags & tfDISP_EMPTY)
		{
			*dsflags |= dsDISP_EMPTY;
		}
		else
		{
			*dsflags &= ~dsDISP_EMPTY;
		}

		if (tb->tb_mode == fdtfQUERY)
		{
			/*
			**  Get the query operator for the field
			*/

			opdbv.db_datatype = DB_INT_TYPE;
			opdbv.db_length = sizeof( i4  );
			opdbv.db_prec = 0;
			opdbv.db_data = (PTR) &oper;

			FDcolop(tf, rownum, cd->c_name,	&oper );

			/*
			**  Point to the query operator for the dataset
			**  row/column (note that this is an aligned nat), 
			**  and compare to the one from the the field.
			*/

			dsopptr = (i4 *)( cd->c_qryoffset + 
				( (char *) rp->rp_data ) );

			if ( oper != *dsopptr )
			{
				chg++;

				/* 
				**  Query operators are different in the
				**  dataset and the field.  Update the dataset.
				*/

				MEcopy((PTR) &oper, (u_i2) sizeof(i4),
					(PTR) dsopptr );
			}

		}
	}

	/*
	**  If the data was changed, set the state of the row if
	**  necessary.
	*/

	if (chg)
	{
		if (rp->rp_state == stNEWEMPT || rp->rp_state == stUNDEF)
			rp->rp_state = stNEW;
		else if (rp->rp_state != stNEW)
			rp->rp_state = stCHANGE;
	}
	return;
}


/*{
** Name:	IITBcisColIsEmpty - Is table field column empty?
**
** Description:
**	This routine checks the passed column in the "current" row
**	(and the "current" table field) to see if the "display" is
**	(or was) empty.  Note that the empty display is means that
**	the screen representation is all blanks and that it is NOT
**	the same as the internal value which is derived from the
**	blanks.
**
**	This routine is ONLY for supporting the 4gl table field
**	qualification feature and assumes that the 4gl has set
**	the table field to query mode and has correctly set
**	up an unload loop before this routine should be called.
**
**	This is an internal use only routine.
**
** Inputs:
**	column		Name of column.
**
** Outputs:
**
**	Returns:
**		1	If column was empty
**		0	If column was not empty
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/19/91 (dkh) - Initial version.
*/
i4
IITBcieColIsEmpty(column)
char	*column;
{
	TBSTRUCT	*tb;
	COLDESC		*cd;
	DATSET		*ds;
	i4		*dsflags;

	tb = IIcurtbl;
	ds = tb->dataset;

	/*
	**  No need to call IIstrconv() since this is an internal
	**  use routine.
	*/
	if ((cd = IIcdget(tb, column)) == NULL)
	{
		IIFDerror(TBBADCOL, 2, (char *) tb->tb_name, column);
		return(0);
	}

	dsflags = (i4 *) (cd->c_chgoffset + ((char *)ds->crow->rp_data));

	if (*dsflags & dsDISP_EMPTY)
	{
		return(1);
	}
	else
	{
		return(0);
	}
}
