/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"rbftype.h"
# include	"rbfglob.h"
# include	<cm.h>
# include	<ug.h>
# include	<flddesc.h>
# include	<vifrbf.h>

/*
**    RFCOLS - called when the Create Column/Aggregate selection is made on
**	the main RBF layout form.  This displays a form which allows
**	users to select a column.
**
**	Parameters:
**
**	Returns:
**		TRUE	if everything is OK,  FALSE if anything wrong.
**
**	Called by:
**		VIFRED routines.
**
**	Side Effects:
**
**	Trace Flags:
**
**	History:
**		30-oct-89 (cmr)	written.
**		03-jan-90 (cmr)	changed to use ListPick.
**		15-jan-90 (cmr)	fixed resume logic in rFlpCreateCol().
**		09-mar-90 (cmr) When creating a column only list those
**				columns that do not already exist.  Give
**				an error msg if there are no more columns.
**		27-mar-90 (cmr)	ColExists(): use < instead of <= in for loop.
**		02-apr-90 (cmr)	Since we no longer show extant cols the 
**				ordinal != the index returned by the listpick 
**				handler.  Use an array to keep track of ords.
**		17-may-90 (cmr)	Give IIFDlpListPick() 10 for the table field
**				length (0 means length is # of items).
**		29-aug-90 (sylviap)	
**			Added another parameter to rFcols to return the
**			report style.
**		19-oct-1990 (mgw)
**			Fixed include of local rbftype.h and rbfglob.h to ""
**			instead of <>
**		30-oct-1992 (rdrane)
**			Ensure that unsupported datatype fields can't be
**			selected.  Declare rFlpCreateCol() as static VOID
**			and not as FUNCT_EXTERN nat.
**		12-nov-1992 (rdrane)
**			Demote this file to a plain vanilla ".c".  Eliminate
**			"overflw_buf" local variable - unused and confused
**			with "ovrflw_buf".
**		11-mar-93 (essi)
**			Fixed bug 45261. Also fixed a potential memory leak.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

static		VOID	rFlpCreateCol();
static		bool	ColExists();

TAGID		rfcols_tag = 0;
ATTRIB		ordinals[DB_GW2_MAX_COLS];
struct LpData
{
	i4	lp_ord;
	bool	lp_aflag;
	i4	lp_agindex;
	bool	lp_cum;
};	

VOID
rFcols( aggflag, attord, agindex, cum, style )
bool	aggflag;
ATTRIB	*attord; 
i4	*agindex;
bool	*cum;
i2	*style;
{
	/* internal declarations */

#define	LISTPIK_BUFSIZ	1024
	i4	i;			/* counter */
	char	*attname;
	char	*s, *startptr, *endptr;
	char	choices[LISTPIK_BUFSIZ];
	char	*ovrflw_buf;
	i4	index = -1, len;
	ATTRIB	*ord = ordinals;
	ATT		*att;
	struct LpData	lpdata;
register COPT	*copt;
	ATTRIB	srtordr;


	*style = St_style;
	rfcols_tag = FEgettag();
	s = startptr = choices;
	endptr = &choices[LISTPIK_BUFSIZ - 1];
	for ( i = 1; i <= En_n_attribs; i++ )
	{
		/*
		** Guard against unsupported datatype.
		*/
		if  ((att = r_gt_att(i)) == (ATT *)NULL)
		{
			continue;
		}
		attname = att->att_name;
		/* 
		** When creating a column do not include
		** extant columns in the selection list.
		*/
		if ( !aggflag && ColExists(attname) )
			continue;
		if ( (len = STlength(attname)) > endptr - s )
		{
			if ( (ovrflw_buf = (char *)FEreqmem( (u_i4)rfcols_tag, 
				(s - startptr) + LISTPIK_BUFSIZ, TRUE, 
				(STATUS *)NULL)) == NULL )
			{
				IIUGbmaBadMemoryAllocation(ERx("rFcols"));
			}
			STcopy( startptr, ovrflw_buf );
			endptr = &ovrflw_buf[(s - startptr) + LISTPIK_BUFSIZ - 1];
			startptr = ovrflw_buf;
			s = startptr + STlength( startptr );
		}
		STcopy( attname, s );
		s += len;
		*s++ = '\n';

		*ord++ = (ATTRIB)i;
	}
	if ( s == startptr )	/* no columns available */
	{
		IIUGerr(E_RF0081_No_columns_avail, UG_ERR_ERROR, 0);
		FEfree(rfcols_tag);
		return;
	}
	*--s = EOS;
	lpdata.lp_aflag = aggflag;
	IIFDlpListPick( aggflag ? ERget(F_RF0055_Create_Aggregate) :
			ERget(F_RF0054_Create_Column), startptr, 10, 
			LPGR_FLOAT, LPGR_FLOAT, 
			ERget(F_RF0056_Help_Column_Agg), H_COLUMNS, 
			rFlpCreateCol, (PTR)&lpdata );

	/* If creating aggregates, make sure the column is not a sort
	** column of higher order (bug 45261).
	*/
	if ( aggflag )
	{
		srtordr = r_fnd_sort(lpdata.lp_ord);
		for ( i = 1; i <= En_n_attribs; i++ )
		{
			copt = rFgt_copt( i );
			if (copt->copt_break == 'y' &&
			    copt->copt_brkftr == 'y' &&
			    (srtordr > 0 && srtordr < copt->copt_sequence))
			{
				IIUGerr(E_RF001B_Higher_order_sort,
					UG_ERR_ERROR, 0);
				FEfree( rfcols_tag );
				return;
			}
		}	
	}

	*attord = (ATTRIB)lpdata.lp_ord;
	*agindex = lpdata.lp_agindex;
	*cum = lpdata.lp_cum;
	FEfree( rfcols_tag );
	return;
}

static
VOID
rFlpCreateCol(data, choice, resume)
PTR	data;
i4	choice;
bool	*resume;
{
	struct LpData	*d = ( struct LpData *)data;


	*resume = FALSE;
	switch ( choice )
	{
		case -1: 
			d->lp_ord = 0;
			d->lp_agindex = -1;
			break;
		default:
			d->lp_ord = ordinals[choice];
			if ( d->lp_aflag )
			{
				d->lp_agindex = rFagg( d->lp_ord, &d->lp_cum );
				if ( d->lp_agindex == -1 )
					*resume = TRUE;
			}
			break;
	}
	return;
}

static bool
ColExists(colname)
char	*colname;
{
	LIST	*list;
	FIELD	*f;
	i4	i;
	FLDHDR	*FDgethdr();

	for ( i = 0; i < Cs_length; i++ )
	{
		for ( list = Cs_top[i].cs_flist; list; list = list->lt_next )
		{
			f = list->lt_field;
			if ( (FDgethdr(f))->fhd2flags & fdDERIVED )
				continue;
			if (STcompare(f->fldname, colname ) == 0 )
				return(TRUE);
		}
	}
	return(FALSE);
}
