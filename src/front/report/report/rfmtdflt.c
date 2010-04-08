/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rfmtdflt.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 

/*
**   R_FMT_DFLT - set up a default FMT structure for an attribute,
**	according to it's data type.  This uses the information from
**	the ATT structure for an attribute to come up with a default
**	FMT structure.  Character strings formats are set to the 
**	width from INGRES (unless over 35 characters, in which case
**	a wrapping format is used).  Numeric domains are set to 
**	the same defaults as the terminal monitor uses.
**
**	Parameters:
**		ordinal	ptr to an ATT ordinal.
**
**	Returns:
**		none.
**
**	Called by:
**		r_set_dflt, r_st_adflt.
**
**	Side Effects:
**		Uses the token routines (r_g_set, r_g_format)
**
**	Trace Flags:
**		140, 144.
**
**	History:
**		9/30/81 (ps)	written.
**		2/1/84 (gac)	added date type.
**		08-jun-90 (sylviap)
**			Tries to fit the charcater columns within the 
**			pagewidth, otherwise wraps the columns. (US #969)
**		23-oct-1992 (rdrane)
**			Ensure set precision in DB_DATA_VALUE structures.
**			Converted r_error() err_num values to hex to
**			facilitate lookup in errw.msg file.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Mar-2005 (lakvi01)
**	    Correctly prototype function.
*/


VOID
r_fmt_dflt(ordinal)
ATTRIB		ordinal;
{

	/* external declarations */
	ATT		*r_gt_att();		/* (mmm) added declaration */

	/* internal declarations */

	char		cfmt[MAX_FMTSTR];	/* temp holder of format str */
	register FMT	*fmt;			/* temp holder of format */
	register ATT	*att;			/* fast ptr to ATT struct */
	DB_DATA_VALUE	*pdb;
	DB_DATA_VALUE	dbdv;
	i4		db_type;		/* datatype */
	i4		fmt_len, title_len;	/* title and default display 
						** format lengths */
	i4		fmt_max;		/* title and default display */ 
	i4		tmp_len;		/* for error handling */


	/* start of routine */

	att = r_gt_att(ordinal);
	pdb = &(att->att_value);
	fmt_max = MAX_CF;

	/* get absolute value to be able to handle NULLs */
	db_type = abs(pdb->db_datatype);
	if (db_type == DB_MNY_TYPE && St_compatible)
	{
		/* if -a set and attribute is money, use the old format
		** for money which is the same as the current float */

		dbdv.db_datatype = DB_FLT_TYPE;
		dbdv.db_length = sizeof(f8);
		dbdv.db_prec = 0;
		pdb = &dbdv;
	}
	else if (db_type == DB_CHR_TYPE || db_type ==  DB_CHA_TYPE ||
		 db_type == DB_TXT_TYPE || db_type == DB_VCH_TYPE)
	{
		/* Check if title and format will fit within the page width */
		if ((title_len = STlength(att->att_name) + TTLE_DFLT) < En_lmax)
		{
			fmt_len = min(MAX_CF, pdb->db_length);
			if (title_len + fmt_len > En_lmax)
			{
				fmt_max = En_lmax - title_len;
				/* 
				** if the format reasonable? For fields > 50,
				** the folding field must be at least 5.
				*/
				if ((fmt_max < 5) && (fmt_len > 50))
				{
					tmp_len = En_lmax;
					r_error(0x40C, FATAL, &tmp_len, NULL);
				}
			}
		}
	}

	/* get a default format for the type */

	fmt_deffmt(Adf_scb, pdb, fmt_max, TRUE, cfmt);

	/* now process the format by using token routines */

	r_g_set(cfmt);
	fmt = r_g_format();


	/* now move to the FMT structure in the att */

	r_x_sformat(ordinal,fmt);

	return;
}
