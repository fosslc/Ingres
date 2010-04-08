/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
**	vfgetsize.c
**
**	contains:
**		vfgetSize()
**
**	history:
**		1/28/85 (peterk) - split off from fmt.c
**		03/13/87 (dkh) - Added support for ADTs.
**		19-sep-89 (bruceb)
**			Interface changed by addition of 'dataprec' as
**			fourth argument.  Done for decimal support.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>

FUNC_EXTERN	STATUS	fmt_size();
FUNC_EXTERN	ADF_CB	*FEadfcb();
extern	bool	RBF;

/*
** return the size of a format string as
** well as the max length the data can be
*/
VOID
vfgetSize(fmt, datatype, datalen, dataprec, y, x, ext)
FMT	*fmt;
i4	datatype;
i4	datalen;
i4	dataprec;
i4	*y;
i4	*x;
i4	*ext;
{
	DB_DATA_VALUE	dbv;
	DB_DATA_VALUE	*dbvptr = NULL;

	/*
	**  Return values are ignored for fmt_size since information
	**  that is passed here should already have been checked.
	**
	**  Warning, if we are passed a variable length format, such
	**  as c0, then we can not pass a NULL dbvptr as this will
	**  cause fmt_size to return zero for the number of columns (x).
	**  This will only be a problem if we allow users to create
	**  new fields in RBF, which allows variable length formats.
	*/
	if (datatype != 0)
	{
		dbv.db_datatype = datatype;
		dbv.db_length = datalen;
		dbv.db_prec = dataprec;
		dbvptr = &dbv;
	}

	_VOID_ fmt_size(FEadfcb(), fmt, dbvptr, y, x);
	*ext = (*x) * (*y);

	if (RBF && *ext == 0)
	{
		*ext = *x + *x;
		*y = 2;
	}
}

