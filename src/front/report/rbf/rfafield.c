/*
**	 Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rfafield.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<cv.h>		 
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<st.h>
# include       "rbftype.h"
# include       "rbfglob.h"

/*
**   RFAFIELD - add a FIELD structure to the frame.  This uses the
**	information in Curtop, Curx, Cury, Curatt to find out
**	where the FIELD is to go.  It adds the format structure
**	and other FIELD fields and then adds a pointer to
**	the FIELD structure into the CS structure or the Other list.
**	Note that FIELD structures in RBF, unlike VIFRED, never
**	contain title fields.  Instead, titles are merely linked
**	trim.
**	Aggregates:  If we are adding an aggregate field to the frame
**	we use Curaggregate to calculate the seq # passed to FDnewfld()
**	and we make up a unique field name by concatenating "agg_"
**	with CVna(Curaggregate).  Also, we get the format for the field 
**	from the PEL passed in.  We dummy up a DB_DATA_VALUE to pass 
**	to fmt_size() using the ACC/CUM in the PEL.  Finally, when inserting
**	this field into the CS array use Curaggregate to calculate the index.
**
**	Parameters:
**		agg_flag	TRUE if this is an aggregate field,
**				FALSE if this is a regular field.
**		agg_pel		pointer to the PEL for the aggregate.
**
**	Returns:
**		field	address of field structure added.
**
**	Trace Flags:
**		200, 212.
**
**	Side Effects:
**		Updates the Cs structure or the Other list.
**
**	History:
**		2/23/82 (ps)	written.
**		2/2/84 (gac)	added date type.
**		7/11/84 (gac)	fixed bug 3636 -- extra lines no longer put
**				under long text (cj0.35) field.
**		9/13/84 (gac)	fixed bug 3981 -- c0.n where n >= field length
**				now does not bomb RBF with an overlap error.
**		1/85	(ac)	Initialized lt_utag to DS_PFIELD for
**				DS routines.
**		1/27/87 (rld)	set f (FIELD) members using new fmt routines
**	06/29/88 (dkh) - Fixed reference to Fmt_tag to be i2 instead of nat.
**              9/22/89 (elein) UG changes
**                      ingresug change #90045
**			changed <rbftype.h> and <rbfglob.h> to
**			"rbftype.h" and "rbfglob.h"
**		22-nov-89 (cmr)	added 2 new args to rFafield(). The first
**			is a bool flag that indicates that the field to be
**			added is an agg and the second arg is the PEL for
**			for the agg.  See note above about aggregates.
**		19-apr-90 (cmr)
**			If this is a unique agg then append it to the copt
**			agu_list.  Later if the user changes the sort seq
**			of the attribute to 0 we can use this list to
**			find all the unique aggs on this attribute.
**		21-jun-90 (cmr)
**			For aggregates pass the correct seq # to FDnewfld().
**		04-mar-92 (leighb) DeskTop Porting Change:
**			Moved function declaration outside of calling function.
**			This is so function prototypes will work.
**		23-oct-1992 (rdrane)
**			Propagate precision to result DB_DATA_VALUE and FIELD
**			structure.  Remove declarations of FEtsalloc(),
**			r_gt_att(), FDnewfld(), and FDgethdr() since they're
**			already declared in hdr files already included by this
**			module.
**		28-oct-1992 (rdrane)
**			Replace previously added #define of flprec  with its
**			actual expansion (cf. frame.h for explanation).
**	15-Nov-1993 (tad)
**		Bug #56449
**		Replace %x with %p for pointer values where necessary.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

static	VOID	rFset_valstr();

GLOBALREF	i2	Fmt_tag;

FIELD	*
rFafield(agg_flag, agg_pel)
bool	agg_flag;
PEL	*agg_pel;
{
	/* internal declarations */

	register FIELD	*f;			/* fast field ptr */
	register ATT	*att;			/* fast ATT ptr */
	register LIST	*list;			/* ptr to list to hold field */
	DB_DATA_VALUE	dbv;
	DB_DATA_VALUE	*val;
	ACC		*acc = NULL;
	CUM		*cum = NULL;
	CS		*cs;			/* CS struct, while .WITHIN */
	LIST		*olist;			/* used in linking list */
	FMT		*fmt;
	i4		fmt_numrows;		/* used in fmt_size */
	i4		fmt_numcols;		/* used in fmt_size */
	i4		retstatus;		/* function return status */
	char		temp_fmtstr[MAX_FMTSTR];
	char		agg_name[FE_MAXNAME + 1];
	char		unique_ext[FE_MAXNAME + 1];
	i4		index;

	/* start of routine */

#	ifdef	xRTR1
	if (TRgettrace(200,0) || TRgettrace(212,0))
	{
		SIprintf(ERx("rFafield: entry.\n"));
	}
#	endif

#	ifdef	xRTR2
	if (TRgettrace(212,0))
	{
		SIprintf(ERx("	Curx:%d"),Curx);
		SIprintf(ERx("	Cury:%d\n"),Cury);
		SIprintf(ERx("	Curordinal:%d\n"),Curordinal);
		SIprintf(ERx("	Curtop:%d"),Curtop);
		SIprintf(ERx("	Curbot:%d\n"));
	}
#	endif

	/* allocate the FIELD */

	if ( !agg_flag )
	{
		att = r_gt_att(Curordinal);
		if (att == (ATT *)NULL)
		{
			IIUGerr(E_RF0001_rFafield__Bad_att_ord, UG_ERR_FATAL, 0);
		}
		f = FDnewfld(att->att_name, Curordinal, FREGULAR);
		fmt = att->att_format;
		val = &att->att_value;
	}
	else
	{
		CVna(Curaggregate, unique_ext);
		STcopy(ERx("agg_"), agg_name);
		STcat(agg_name, unique_ext);
		f = FDnewfld(agg_name, Curaggregate + En_n_attribs, FREGULAR);
		(FDgethdr(f))->fhd2flags |= fdDERIVED;
		fmt = agg_pel->pel_fmt;
		if ( agg_pel->pel_item.item_type == I_ACC )
		{
			acc = agg_pel->pel_item.item_val.i_v_acc;
			dbv.db_datatype = acc->acc_datatype;
			dbv.db_length = acc->acc_length;
			dbv.db_prec = acc->acc_prec;
		}
		else
		{
			cum = agg_pel->pel_item.item_val.i_v_cum;
			dbv.db_datatype = cum->cum_datatype;
			dbv.db_length = cum->cum_length;
			dbv.db_prec = cum->cum_prec;
		}
		val = &dbv;
		/* set the validation str to the text for the agg on save */
		rFset_valstr( agg_pel, f );
	}

	f->fldatatype = val->db_datatype;
	f->fllength = val->db_length;
	f->fld_var.flregfld->fltype.ftprec = val->db_prec;
	f->flposx = Curx;
	f->flposy = Cury;
	f->fldatax = 0;
	f->fldatay = 0;

	retstatus = fmt_fmtstr(FEadfcb(), fmt, temp_fmtstr);
	if (retstatus != OK)
	{
		IIUGerr(E_RF0002_rFafield__Can_t_get_f, UG_ERR_FATAL, 0);
	}
	f->flfmtstr = FEtsalloc(Fmt_tag, temp_fmtstr);

	retstatus = fmt_size(FEadfcb(), fmt, val, &fmt_numrows, &fmt_numcols);
	if (retstatus != OK)
	{
		IIUGerr(E_RF0003_rFafield__Can_t_get_f, UG_ERR_FATAL, 0);
	}
	f->flmaxx = fmt_numcols;
	f->flmaxy = (fmt_numrows != 0) ? fmt_numrows : 2;
	f->flwidth = fmt_numcols;
	f->fldataln = fmt_numcols;

#	ifdef	xRTR2
	if (TRgettrace(212,0))
	{
		SIprintf(ERx("	Field structure at end of rFafield.\n"));
		FDflddmp(f);
	}
#	endif

	list = lalloc(LFIELD);
	list->lt_field = f;
	list->lt_utag = DS_PFIELD;

	/* find out what it is associated with */

	olist = NULL;
	if ( agg_flag )
	{
		index = En_n_attribs + Curaggregate;
		cs = &(Cs_top[index - 1]);
	}
	else 
		if ((Curordinal>0)&&(Curordinal<=En_n_attribs))
		{	/* associated with a column */
			cs = &(Cs_top[Curordinal-1]);
		}
		else	
			cs = &Other;

	if((olist=cs->cs_flist) == NULL)
	{
		cs->cs_flist = list;
	}

	/* if a list exists, add to the end of the existing one */

	if (olist!=NULL)
	{
		while (olist->lt_next != NULL)
		{	/* get to end of list */
			olist = olist->lt_next;
		}
		olist->lt_next = list;
	}
	if (agg_flag)
	{
		/* append this agg fld to the copt's agu_list if it's unique */
		if ( agg_pel->pel_item.item_type == I_ACC )
		{
			if (acc->acc_unique)
				rFagu_append(f, acc->acc_attribute);
		}
		else
		{
			if (cum->cum_unique)
			rFagu_append(f, cum->cum_attribute);
		}
	}

#	ifdef	xRTR2
	if (TRgettrace(212,0))
	{
		SIprintf(ERx("	At end of rFafield.\n"));
		SIprintf(ERx("		Previous list add:%p\n"),olist);
		if (olist!=NULL)
		{
			SIprintf(ERx("			lt->next:%p\n"),olist->lt_next);
			rFpr_list(olist,LFIELD,FALSE);
		}
		SIprintf(ERx("		New list add:%p\n"),list);
		if (list!=NULL)
		{
			SIprintf(ERx("			lt->next:%p\n"),list->lt_next);
			rFpr_list(list,LFIELD,FALSE);
		}
	}
#	endif

	return(f);
}



/*
** rFset_valstr() - given an ACC/CUM set the validation string for the field. **
**	For derived fields (aggregates) the validation string will be the **
**	text that gets written to the database when the report is saved.  **
**
**      3-nov-1992 (rdrane)
**		Ensure attribute names generated in unnormalized form.
**		Ensure that unsupported datatype fields are "ignored".
*/

static
VOID
rFset_valstr( pel, fld )
PEL 	*pel;
FIELD	*fld;
{
	i4	i;
	ACC	*acc;
	CUM	*cum;
	ATT	*att;
	COPT	*copt;
	AGG_INFO info;
	ADI_FI_ID agfi;
	char	*aggname;
	bool	agunique;
	char	valstr[(FE_MAXNAME + FE_UNRML_MAXNAME + 6 + 1)];
	char	tmp_buf[(FE_UNRML_MAXNAME + 1)];


	if ( pel->pel_item.item_type == I_ACC )
	{
		acc = pel->pel_item.item_val.i_v_acc;
		att = r_gt_att( acc->acc_attribute );
		copt = rFgt_copt( acc->acc_attribute );
		agfi = acc->acc_ags.adf_agfi;
		agunique = acc->acc_unique;
	}
	else
	{
		cum = pel->pel_item.item_val.i_v_cum;
		att = r_gt_att( cum->cum_attribute );
		copt = rFgt_copt( cum->cum_attribute );
		agfi = cum->cum_ags.adf_agfi;
		agunique = cum->cum_unique;
	}

	/*
	** We probably shouldn't be here if the underlying
	** attribute isn't a supported datatype.
	*/
	if  (att == (ATT *)NULL)
	{
		fld->flvalstr = STalloc(ERx(""));
		return;
	}

	for ( i = 0; i < copt->num_ags; i++ )
	{
		info = copt->agginfo[i];
		if (info.agnames->ag_fid == agfi && 
			info.agnames->ag_prime == agunique)
		{
			aggname = info.agnames->ag_funcname;
			break;
		}
	}

	_VOID_ IIUGxri_id(att->att_name,&tmp_buf[0]);
	if ( pel->pel_item.item_type == I_CUM )
	{
		STpolycat( 5, ERx("cum "), aggname, ERx("("), 
			&tmp_buf[0], ERx(")") , valstr);
	}
	else
	{
		STpolycat( 4, aggname, ERx("("), &tmp_buf[0], 
			ERx(")") , valstr);
	}

	fld->flvalstr = STalloc( valstr );

	return;
}
