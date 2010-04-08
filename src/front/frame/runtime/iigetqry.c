/*
**	iigetqry.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	iigetqry.c - Get query information for a field.
**
** Description:
**	This file contains routines to get query information for
**	a field.  They are only to be called from ABF/OSL.  This
**	is not an external interface for reguarl users.
**
** History:
**	Revision 6.0  87/06/09  dave
**	06/09/87 (dkh) - Initial version.
**	13-jul-87 (bruceb) Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**
**	Revision 6.1  88/04/07  bruceb
**	07-apr-88 (bruceb)
**		Changed from using sizeof(DB_TEXT_STRING)-1 to using
**		DB_CNTSIZE.  Previous calculation is in error.
**	03-aug-88 (wong)  Added 'dml_level' parameter.
**	07/27/89 (dkh) - Added special marker parameter on call to fmt_multi.
**	01-dec-89 (bruceb)	Fix for bug 2624.
**		Handle reverse fields.  NOTE:  Since qualification on table
**		fields hasn't yet been implemented, that section of the bug
**		fix has not been tested.
**	02/19/91 (dkh) - Added support for 4gl table field qualifications.
**      26-sep-91 (jillb/jroy--DGC) (from 6.3)
**            Changed fmt_init to IIfmt_init since it is a fairly global
**            routine.  The old name, fmt_init, conflicts with a Green Hills
**            Fortran function name.
**	08/24/92 (dkh) - Fixed acc complaints.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

#include	<compat.h>
#include	<me.h>		/* 6-x_PC_80x86 */
#include	<st.h>		/* 6-x_PC_80x86 */
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ui.h>
#include	<ft.h>
#include	<fmt.h>
#include	<adf.h>
#include	<frame.h>
#include	<menu.h>
#include	<runtime.h>
#include	<frserrno.h>
# include	<rtvars.h>


FUNC_EXTERN	FIELD	*FDfndfld();
FUNC_EXTERN	FLDVAL	*FDgetval();
FUNC_EXTERN	STATUS	FDrngchk();
FUNC_EXTERN	FLDCOL	*FDfndcol();
FUNC_EXTERN	STATUS	FDfmtget();
FUNC_EXTERN	char	*iiugOpRep();
FUNC_EXTERN     STATUS  fmt_isreversed();
FUNC_EXTERN	i4	IITBcieColIsEmpty();

static u_i4	qtag = 0;	/* local static tag */

/*{
** Name:	IIgetquery - Get query spec for field and process it.
**
** Description:
**	Given a field in a form, get the query specification for that
**	field and process it by calling FDrngchk().  If the field
**	is a table field, then process the passed in column for the
**	current row.  The current row is defined by the row that
**	is currently being unloaded in an "unloadtable" loop.
**
**	DML compatibility level is passed in and we assume that we will
**	be building the query, not just simply doing a validation
**	check.  Fields are assumed to be not display only.  The
**	DB_DATA_VALUE that is passed to FDrngchk() must be READ
**	ONLY to FDrngchk() and FDrngchk() must copy the value
**	if it wants to use it later on.
**
**	The current code is designed not to work with range query
**	buffers at the moment and only allows ONE qualification
**	per field.  When the switch to use range query
**	buffers is made, the code below (as well as some code in
**	FT) must be changed to call FTgetdisp().  Also, the use
**	of an "unloadtable" loop is invalid at this point and
**	another mechanism must be used to unload the range query
**	buffers for a table field.
**
**	Yes, the code below is looking at the forms structures
**	but it is only until we go to range query buffers.
**
** Inputs:
**	form		{char *}  Name of form containing field.
**	field		{char *}  Name of field on which to operate.
**	column		{char *}  Name of column if field is a table field.
**	tblname		{char *}  Name of database table to pass to FDrngchk().
**	dml_level	{nat}  DML compatibility level passed to 'FDrngchk()':
**					UI_DML_QUEL
**					UI_DML_SQL
**					UI_DML_GTWSQL
**	att		{char *}  Name of attribute in database table to pass
**				to FDrngchk().
**	prefix		{char *}  Special information to be passed
**				to FDrngchk().
**	func		{STATUS (*)()}  Function reference to pass
**				to FDrngchk().
**	data		{PTR}  Parameter to be passed to "func" by FDrnghck().
**
** Returns:
**	{STATUS}  OK	If everything worked.
**		  FAIL	If something went wrong.
**
** History:
**	06/09/87 (dkh) - Initial version.
**	08/25/88 (jhw) - Fixed Jup bug #3161.  Allow DISPONLY fields to
**				be qualified.
**      3/21/91 (elein)         (b35574) Add FALSE parameter to call to
**                              fmt_multi. TRUE is used internally for boxed
**                              messages only.  They need to have control
**                              characters suppressed.
*/
STATUS
IIgetquery (char *form, char *field, char *column, char *tblname,
	i4 dml_level, char *attr, char *prefix, STATUS (*func)(), PTR data )
{
	RUNFRM		*runf;
	FIELD		*fld;
	DB_DATA_VALUE	*dispdbv;
	FLDVAL		*val;
	STATUS		stat;
	bool		nonseq;
	DB_DATA_VALUE	ldbv;
	TBLFLD		*tbl;
	FLDCOL		*col;
	DB_TEXT_STRING	*text;
	DB_TEXT_STRING	*otext;
	ADF_CB		*ladfcb;
	FMT		*cfmt;
	i4		oper;
	char		*opchar;
	i4		qrycnt;
	DB_DATA_VALUE	valdbv;
	i4		rows = 0;
	i4		columns = 0;
	bool		reversed;
	i4		len;
	i4		cursize;
	i4		i;
	u_char		*c;
	u_char		*end;


	/*
	**  Do various checks on existence of form, field, etc.
	**  Return FAIL if something is missing.  Assume that
	**  we are being called from 'C', so no need to call
	**  IIstrconv().
	*/
	runf = ( form == NULL || *form == EOS ) ? IIstkfrm
			: RTfindfrm(form);
	if ( runf == NULL )
	{
		return FAIL;
	}

	/*
	**  Find field in the form, return FAIL if field does not exist.
	**  (Allow DISPONLY fields to be qualified, however.)
	*/
	if ( (fld = FDfndfld(runf->fdrunfrm, field, &nonseq)) == NULL )
	{
		IIFDerror( GFFLNF, 1, field );
		return FAIL;
	}

	ladfcb = FEadfcb();

	if ( (stat = FDfmtget(runf->fdrunfrm, field, column, 0, &cfmt)) != OK )
	{
	    return(stat);
	}
	if ( cfmt == NULL )
	{
	    return FAIL;
	}
	_VOID_ fmt_isreversed(ladfcb, cfmt, &reversed);

	if (qtag == 0)
	{
	    qtag = FEgettag();
	}

	if (fld->fltag == FREGULAR)
	{
		/*
		**  Just point to the simple field's display
		**  buffer.  It should be good.
		*/
		val = FDgetval(fld);
		dispdbv = val->fvdsdbv;
		if (reversed)
		{
		    /*
		    **  Find out information for formatting.
		    */
		    if (fmt_size(ladfcb, cfmt, val->fvdbv, &rows, &columns)
			!= OK)
		    {
			return(FAIL);
		    }

		    MEcopy((PTR) val->fvdsdbv, (u_i2) sizeof(DB_DATA_VALUE),
			(PTR) &valdbv);
		    if ((valdbv.db_data = FEreqmem(qtag,
			(u_i4)valdbv.db_length, TRUE, &stat)) == NULL)
		    {
			FEfree(qtag);
			return(FAIL);
		    }
		    text = (DB_TEXT_STRING *) valdbv.db_data;
		    otext = (DB_TEXT_STRING *) val->fvdsdbv->db_data;
		    cursize = text->db_t_count = otext->db_t_count;
		    MEcopy((PTR)otext->db_t_text, (u_i2)cursize,
			(PTR)text->db_t_text);

		    c = text->db_t_text;
		    len = rows * columns;

		    if (len > cursize)
		    {
			MEfill((u_i2)(len - cursize), (unsigned char)' ',
			    (PTR)(c + cursize));
			text->db_t_count = len;
		    }

		    for (i = 0; i < rows; i++)
		    {
			f_revrsbuf(columns, (bool)TRUE, c);
			c += columns;
		    }

		    /*
		    **  Need to trim trailing blanks at this point.
		    **  Don't need to use CM routines here, since Kanji
		    **  and reverse don't work together.
		    */
		    end = text->db_t_text;
		    c = end + text->db_t_count - 1;
		    while ( c >= end && *c-- == ' ' )
		    {
			--(text->db_t_count);
		    }

		    dispdbv = &valdbv;
		}
	}
	else
	{ /* a table field */
		FMT	scrfmt;		/* for scrolling columns */

		tbl = fld->fld_var.fltblfld;
		/*
		**  Need to get value and query operator from
		**  dataset and then convert into a LONG_TEXT
		**  struct for display.  First, check for
		**  existence of column.
		*/
		if ((col = FDfndcol(tbl, column)) == NULL)
		{
			return(FAIL);
		}

		/*
		**  Allocate local copies of column value and display
		**  buffers.
		*/
		val = tbl->tfwins + col->flhdr.fhseq;
			/* + ( 0 * tbl->tfcols ) */
		MEcopy((PTR) val->fvdbv, (u_i2) sizeof(DB_DATA_VALUE),
			(PTR) &valdbv);
		MEcopy((PTR) val->fvdsdbv, (u_i2) sizeof(DB_DATA_VALUE),
			(PTR) &ldbv);
		if ( (valdbv.db_data = FEreqmem(qtag, (u_i4)valdbv.db_length,
				TRUE, &stat)) == NULL ||
			(ldbv.db_data = FEreqmem(qtag, (u_i4)ldbv.db_length,
				TRUE, &stat)) == NULL )
		{
			FEfree(qtag);
			return FAIL;
		}

		if (IITBcieColIsEmpty(column))
		{
			text = (DB_TEXT_STRING *) ldbv.db_data;
			text->db_t_count = 0;
		}
		else
		{
			/*
			**  Get value for column from current row.
			**  Note that this is a direct call to internal routine
			**  and changes made must be done carefully.
			*/
			if ( !IItcogetio((i2 *) NULL, TRUE, DB_DBV_TYPE, 0,
				&valdbv, column) )
			{
				FEfree(qtag);
				return(FAIL);
			}

			/*
			**  Now get query operator.
			*/
			IIgetoper(1);
			if ( !IItcogetio((i2 *) NULL, TRUE, DB_INT_TYPE,
				sizeof(oper), &oper, column) )
			{
				FEfree(qtag);
				return(FAIL);
			}

			/*
			**  Format value.
			*/
			/*
			** If it's a scrolling column, make a format for all the
			** data, not just the fraction that can be displayed 
			** at once.
			*/
			if ((col->flhdr.fhd2flags & fdSCRLFD) != 0)
			{
				STRUCT_ASSIGN_MACRO(*cfmt, scrfmt);
				scrfmt.fmt_width = ldbv.db_length - 2;
				cfmt = &scrfmt;
			}

			/*
			**  Find out information for formatting.
			*/
			if (fmt_size(ladfcb, cfmt, &valdbv, &rows,
				&columns) != OK)
			{
				FEfree(qtag);
				return(FAIL);
			}

			/*
			**  Just format output if single line output.
			*/
			if (rows == 1)
			{
		    		if (fmt_format(ladfcb, cfmt, &valdbv, &ldbv,
					TRUE) != OK)
		    		{
					FEfree(qtag);
					return(FAIL);
		    		}
			}
			else
			{
				reg u_char	*dsptr;
				DB_TEXT_STRING	*ftext;
				PTR		buffer;
				DB_DATA_VALUE	wksp;
				i4		length = 0;

				wksp.db_datatype = DB_LTXT_TYPE;
				wksp.db_prec = 0;
				wksp.db_length = DB_CNTSIZE + columns;
				if ( (wksp.db_data = FEreqmem(qtag,
					(u_i4)wksp.db_length, TRUE, &stat))
					== NULL )
				{
					FEfree(qtag);
					return(FAIL);
				}
				fmt_workspace(ladfcb, cfmt, &valdbv, &length);
				if ( (buffer = FEreqmem(qtag, (u_i4)length,
					TRUE, &stat)) == NULL )
				{
					FEfree(qtag);
					return(FAIL);
				}

				/*
				**  Do multi-line output.
				*/

				IIfmt_init(ladfcb, cfmt, &valdbv, buffer);

				text = (DB_TEXT_STRING *) ldbv.db_data;
				text->db_t_count = 0;
				dsptr = text->db_t_text;
				ftext = (DB_TEXT_STRING *) wksp.db_data;
				for (;;)
				{
					reg i4		j;
					reg u_char	*fptr;
					i4		fcount;
					bool		more = FALSE;

					if (fmt_multi(ladfcb, cfmt, &valdbv,
						buffer, &wksp,&more,TRUE, FALSE)
					   != OK)
					{
						FEfree(qtag);
						return(FAIL);
					}

					if (!more)
					{
						break;
					}

					/*
					** put into fields display buffer.
					*/
					fcount = ftext->db_t_count;
					fptr = ftext->db_t_text;
					for (j = 0; j < fcount; j++)
					{
						*dsptr++ = *fptr++;
					}
					text->db_t_count += fcount;
				}
			}

			text = (DB_TEXT_STRING *) ldbv.db_data;

			if (reversed)
			{
		    		c = text->db_t_text;
		    		cursize = text->db_t_count;
		    		len = rows * columns;

		    		if (len > cursize)
		    		{
					MEfill((u_i2)(len - cursize),
						(unsigned char)' ',
			    			(PTR)(c + cursize));
					text->db_t_count = len;
		    		}

		    		for (i = 0; i < rows; i++)
		    		{
					f_revrsbuf(columns, (bool)TRUE, c);
					c += columns;
		    		}
			}

			/*
			**  Need to trim trailing blanks at this point.
			*/
			end = text->db_t_text;
			c = end + text->db_t_count - 1;
			while ( c >= end && *c-- == ' ' )
			{
		    		--(text->db_t_count);
			}

			/*
			**  Insert query operators up front.
			*/
			if ( oper != fdNOP && oper != fdEQ &&
			    (qrycnt = STlength(opchar = iiugOpRep(oper))) > 0 )
			{
				reg u_char	*dsptr;
				reg u_char	*head;
				reg i4		j;
				reg i4		cnt;

				head = text->db_t_text + text->db_t_count - 1;
				dsptr = head + qrycnt;
				cnt = text->db_t_count;
				for (j = 0; j < cnt; j++)
				{
					*dsptr-- = *head--;
				}
				text->db_t_text[0] = opchar[0];
				if (qrycnt == 2)
				{
					text->db_t_text[1] = opchar[1];
				}
				text->db_t_count += qrycnt;
			}
		}
		dispdbv = &ldbv;
	}

	stat = FDrngchk(runf->fdrunfrm, field, column, dispdbv, (bool) TRUE,
		tblname, attr, (bool) TRUE, dml_level, prefix, func, data);

	if (qtag != 0)
	{
		FEfree(qtag);
	}
	return stat;
}
