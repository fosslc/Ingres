/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include       "rbftype.h"
# include       "rbfglob.h"

/*
**   RFRESET - routine to reset the values of variables in
**	RBF.  This sets the values of variables
**	to defaults (or NULL) before various stages of the
**	program are done.
**
**	Parameters:
**	type... Type codes to indicate the nature of the reset.
**		The number of type codes cannot exceed the total number
**		of possible type codes plus one.  The last type code
**		must be RP_RESET_LIST_END.  The codes are defined in
**		hdr/rcons.h.
**
**	Returns:
**		none.
**
**	Side Effects:
**		resets everything.
**
**	Called by:
**		rFdisplay, rFrbf.
**
**	Trace Flags:
**		2.0, 2.3.
**
**	History:
**		8/23/82 (ps)	written.
**		12/3/84 (rlk) - Changed to use a single COLUMNOPTION form.
**		1/14/85 (rlk) - Added calls to free LIST elements (from
**				vifred).
**		1/85	(ac)	Remove Froptions, Fcoloptions, Frcatalog,
**				Frsave, Frmain and Frreport.
**		2/85	(ac)	Initialized form name to null, so that
**				we know whether the form has been
**				initialized or not.
**		2/2/87 (rld) -	add calls to free agg tables.
**		5/11/87 (prs)	take out Nrreport ref.
**	06/29/88 (dkh) - Fixed reference to Fmt_tag to be i2 instead of nat.
**      9/22/89 (elein) UG changes ingresug change #90045
**	changed <rbftype.h> & <rbfglob.h> to "rbftype.h" & "rbfglob.h"
**		01-aug-89 (cmr)	changes to reflect new Sections data struct.
**		1/3/89 (martym) Added initialization of "Nindtopts".
**	31-jan-90 (cmr)
**		Reset St_lp_set.  This is not done in r_reset() because
**		in R/W this bool gets set because of a command line flag 
**		(-v) and therefore must not get reset in R/W.  But we use 
**		this flag to determine if the user specified a pagelength
**		in the ReportOptions frame. See rFscan() and rFmlayout() 
**		for more details.
**	5/29/90 (martym)
**		Added the reseting of "Nrfbrkopt", and "Nrfnwpag".
**	21-jun-90 (cmr) Reset RbfAggSeq.
**	9-oct-1992 (rdrane)
**		Replace bare constant reset state codes with #define constants.
**		Add explicit return statement.  Declare rF_reset() as
**		returning VOID, and declare rF_rreset() as being static.  At
**		some point, the GLOBALREF's shoiuld be removed, as they should
**		be in HDR Files.
**	8-dec-1992 (rdrane)
**		Add Rbf_xns_given boolean.
**	9-mar-1993 (rdrane)
**		Add Rbf_var_seq.
**	23-sep-1993 (rdrane)
**		Remove Rbf_xns_given boolean.  RBF calls RP_RESET_REPORT
**		multiple times when editing an existing report.  This is
**		totally bogus behavior, but this is the only troublesome
**		at the moment global.  Part of fix for b54949.
*/

GLOBALREF i2	Agg_tag;
GLOBALREF AGG_TYPE *Agg_top;

static	VOID	rF_rreset();

/* VARARGS1 */
VOID
rFreset(t1,t2,t3)
i2	t1,t2,t3;
{
	/* start of routine */

	if (t1 != 0)
	{
	   rF_rreset(t1);
	   if (t2 != 0)
	   {
	      rF_rreset(t2);
	      if (t3 != 0)
		  IIUGerr(E_RF003C_rFreset__too_many_arg, UG_ERR_FATAL, 0);
	   }
	}
}

GLOBALREF	i2	Fmt_tag;

static
VOID
rF_rreset(type)
i2	type;
{

	switch (type)
	{
	case RP_RESET_PGM:
		/* Start of new program */
		Nrdetails = ERx("");		/* name */
		Nroptions = ERx("");		/* name */
		Nstructure = ERx("");	/* name */
		Ncoloptions = ERx("");	/* name */
		Nrfbrkopt = ERx("");	/* name */
		Nrfnwpag = ERx("");	/* name */
		Naggregates = ERx("");
		Nindtopts = ERx("");   /* name */
		Nlayout = ERx("");
		Nrsave = ERx("");		/* name */
		Fmt_tag = FEgettag();
		break;

	case RP_RESET_REPORT:
		/* At start of new report */
		Rbfchanges = FALSE;	/* TRUE if changes made to form */
		Rbf_var_seq = 0;

		/*	Free the Cs structure */

		Cs_top = NULL;		/* start of array of CS structs */
		if (Cs_tag > 0)		/* Free the memory */
		{
			FEfree(Cs_tag);
			Cs_tag = -1;
		}

		Cs_length = 0;	/* number of attributes */

		/*	Free the LIST nodes (from vifred phase) */

		linit();
		ndinit();

		/*	Free the Copt structure */

		Copt_top = NULL;	/* array of COPT structures */
		if (Copt_tag > 0)	/* Free the associated memory */
		{
			FEfree(Copt_tag);
			Copt_tag = -1;
		}

		/*	Free the Aggregate info arrays */

		Agg_top = NULL;		/* array of AGG_TYPE and
						AGG_INFO structures */
		if (Agg_tag > 0)	/* Free the associated memory */
		{
			FEfree(Agg_tag);
			Agg_tag = -1;
		}

		Agg_count = 0;
		RbfAggSeq = 0;
		
		/*	Free the format strings */

		if (Fmt_tag > 0)	/* Free the associated memory */
		{
			FEfree(Fmt_tag);
			Fmt_tag = FEgettag();
		}

		Other.cs_tlist = NULL;
		Other.cs_flist = NULL;
		Other.cs_name = NULL;

		/*	Initialize and free storage for the RBF frame */

		Rbf_frame = NULL;	/* ptr to RBF frame */
		if (Rbfrm_tag > 0)
		{
			FEfree(Rbfrm_tag);
			Rbfrm_tag = -1;
		}

		sec_list_remove_all( &Sections );
		Curx = 0;
		Curtop = 1;
		Curordinal = 0;
		Cury = 1;
		Curaggregate = 0;
		/* even though this is primarily a R/W variable we */
		/* need to reset it because r_reset() does not.    */
		St_pl_set = FALSE;
		break;
	}

	return;
}
