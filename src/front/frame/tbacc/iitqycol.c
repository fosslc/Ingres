/*
**	iitqrycol.c
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
# include	<frserrno.h>
# include	<qg.h>
# include       <er.h>

/**
** Name:	iitqrycol.c	-	Get column value, query mode
**
** Description:
**	Query mode column value routines.
**
**	Public (extern) routines defined:
**		IItqrycol()
**	Private (static) routines defined:
**
** History:
**	19-jun-87 (bab)	Code cleanup.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

GLOBALREF	TBSTRUCT	*IIcurtbl;

FUNC_EXTERN	FLDCOL	*FDfndcol();


/*{
** Name:	IItqrycol	-	Get column value, query mode
**
** Description:
**	Provided with a tablefield in QUERY mode, build a qualification
**	specification for the column that can be used in a WHERE clause.  
**	Only one qualfication is allowed per column.  This routine
**	is the tablefield version of IIqryfield.
**
**	This supports QBF query mode in all instances (i.e. the PC
**	product, and the CMS version) where scrolling fields in QBF
**	queries are not supported. 
**
**	The query specification built by this routine is designed for
**	the 6.0 Query Generator module interface.  Each QRY_SPEC build
**      is sent to the argument qgfunc.
**
**	most of the work is done by FDrngchk.
**
**	WARNING:  This routine is for QBF use ONLY.
**
** Inputs:
**	colname	Name of the column to get the value from 
**
**	relname		The relation name to use for the claue.
**
**	attrname	The attribute name for the field's query.
**
**	qlang		The query language - FEDMLQUEL or FEDMLSQL
**
**	prefix		A string to put out before an clause is put
**			out.
**
**	qgfunc		The function to call for each QRY_SPEC built.
**
**	qgvalue		A value to pass to qgfunc.
**
** Returns:
**		OK if succeeded, and some QRY_SPECs were produced.
**		QG_NOSPEC if succeeded and no QRY_SPECs were produced.
**		not OK if errors.
**
** Exceptions:
**	none
**
** Side Effects:
**	Expects IIcurtbl to point to the tablefield the column is from.
**
** History:
**	12-feb-1986	- Written (ncg)
**	05-jan-1987 (peter)	Change RTfnd* to FDfnd.
**	03-apr-1987	Changed to call FDrngchk (drh)
**
**	14-may-1987 (Joe)
**		Changed to use new QG function interface.
**
*/

static i2	IIqtag ZERO_FILL;

STATUS
IItqrycol(char *colname, char *relname, char *attrname, i4 qlang,
	char *prefix, STATUS (*qgfunc)(), PTR qgvalue)
{
	char			*colptr;
	char			c_buf[MAXFRSNAME+1];
	register TBSTRUCT	*tb;
	register FLDCOL		*col;
	register TBLFLD		*tf;
	register FLDHDR		*hdr;
	i4			fldno;
	FRAME			*frm;
	DB_DATA_VALUE		*display;
	STATUS			result;
	STATUS			FDrngchk();

	tb = IIcurtbl;
	if ((colptr = IIstrconv(II_CONV, colname, c_buf, MAXFRSNAME))
		== NULL)
	{
		/* no column name given for table I/O statement */
		IIFDerror(TBNOCOL, 1, IIcurtbl->tb_name);
		return (FAIL);
	}

	if (tb->tb_mode != fdtfQUERY)
	{
		IIFDerror(TBQRYMD, 1, tb->tb_name);
		return (FAIL);
	}

	tf = tb->tb_fld;

	if (!(tf->tfhdr.fhdflags & fdtfQUERY))
	{
		IIFDerror(TBQRYMD, 1, tb->tb_name);
		return (FAIL);
	}

	if ((col = FDfndcol(tf, colptr)) == NULL)
	{
		IIFDerror(TBBADCOL, 2, (&(tf->tfhdr))->fhdname, colptr);
		return (FAIL);
	}

	FDftbl(tf, &frm, &fldno);
	if (frm == NULL)
	{
		IIFDerror(TBLNOFRM, 0, (char *) NULL);
		return (FAIL);
	}

	hdr  = &(col->flhdr);

	/*
	**  Get the display buffer for the column
	*/

	if (IIqtag == 0)
	{
		IIqtag = FEgettag();
	}
	FTgetdisp( frm, fldno, hdr->fhseq, IIqtag, (bool) FALSE, &display );

	/*
	**  Parse the query, build a query descriptor
	*/

	result =  FDrngchk( frm, tb->tb_name, colptr, display, (bool) TRUE,
		relname, attrname, (bool) TRUE, qlang, prefix, qgfunc, qgvalue);
	FEfree(IIqtag);
	return( result );
}
