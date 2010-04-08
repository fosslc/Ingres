/*	
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
# include	<er.h>
# include	<rtvars.h>

FUNC_EXTERN	VOID	FTgetdisp();
FUNC_EXTERN	STATUS	FDrngchk();
FUNC_EXTERN	REGFLD	*FDqryfdop();

/**
** Name:	iiqryfld.c
**
** Description:
**	Provide 'get' access to a field in QBF QUERY mode
**	This is support for the PC only.
**
**	Public (extern) routines defined:
**		IIqryfield()	
**	Private (static) routines defined:
**
** History:
**	19-jun-87 (bab)	Code cleanup.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/


/*{
** Name:	IIqryfield	-	Get data from query mode field
**
** Description:
**	Provided with a field in QUERY mode, build a qualification
**	specification for the field that can be used in a WHERE clause.  
**	Only one qualfication is allowed per field.
**
**	This supports QBF query mode in all instances (i.e. the PC
**	product) where scrolling fields in QBF queries are not supported. 
**
**	The query specification built by this routine is designed for
**	the 6.0 Query Generator module interface.  It builds several QRY_SPEC
**	passing each to the argument qgfunc.
**
**	The real work is node by FDrngchk.
**
**	WARNING:  This routine is for QBF use ONLY.
**
** Inputs:
**	name		Name of the field
**
**	relname		The relation name to use for the claue.
**
**	attrname	The attribute name for the field's query.
**
**	dml_level	{nat}  DML compatibility level, UI_DML_QUEL, etc.
**				(Passed through to 'FDrngchk()'.)
**
**	prefix		A string to put out before an clause is put
**			out.
**
**	qgfunc		The function to call for each QRY_SPEC built.
**
**	qgvalue		A value to pass to qgfunc.
**
** Returns:
**		OK if succeeded and some QRY_SPECs were sent.
**		QG_NOSPEC if no QRY_SPECs were sent
**		not OK otherwise.
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	27-mar-1987	Modified parameters, changed for ADTs and NULLs.
**			Eliminated IIqrybuf routine (drh).
**	14-may-1987 (Joe)
**		Changed to use qgfunc, and now puts relname.attrname.
**	
*/

static i2	IIqtag ZERO_FILL;
STATUS
IIqryfield(char *name, char *relname, char *attrname,
	i4 dml_level, char *prefix, STATUS (*qgfunc)(), PTR qgvalue)
{
	char			*namestr;
	char			fbuf[MAXFRSNAME+1];
    	register FRAME		*frm;
    	register REGFLD		*fld;
	register FLDHDR		*hdr;
	DB_DATA_VALUE		*display;
	STATUS			result;


	namestr = IIstrconv(II_CONV, name, fbuf, MAXFRSNAME);
	if (namestr == NULL)
	{
		IIFDerror(RTRFFL, 2, IIfrmio->fdfrmnm, ERx(""));
		return(FAIL);
	}

	/*
	** Make sure the frame is in Query mode, else return NOOP.
	*/
        if (IIfrmio->fdrunmd != fdrtQRY)
                return (FAIL);

	frm = IIfrmio->fdrunfrm;

	if ((fld = FDqryfdop(frm, namestr)) == NULL)
		return (FAIL);

	hdr = &(fld->flhdr);


	if (IIqtag == 0)
	{
		IIqtag = FEgettag();
	}
	FTgetdisp( frm, hdr->fhseq, 0, IIqtag, (bool) FALSE, &display );
	result = ( FDrngchk( frm, hdr->fhdname, (char *) NULL, display, 
	        (bool) TRUE, relname, attrname, (bool) TRUE, 
		dml_level, prefix, qgfunc, qgvalue ));
	FEfree(IIqtag);

	return( result );
}
