/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#ifndef NT_GENERIC
# include	<unistd.h>
#endif
# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<lo.h>
# include	<me.h>
# include	<nm.h>
# include	<adf.h>
# include	<fmt.h>
# include	<rtype.h>
# include	<rglob.h>
# include	<afe.h>
# include	<cm.h>
# include	<er.h>

/*{
** Name:	r_rep_set() -	Report Writer Set-Up.
**
** Description:
**   R_REP_SET - set up the line buffers, etc for the report.  This
**	allocates space for a starting line buffer for the report,
**	the page, and centering and clears them out.  It sets the
**	current pointers to the correct buffer, clears things out
**	and returns to start the report.
**
**	It also sets up the file where the report is written, and
**	sets up a number of flags needed before the start of the
**	report.
**
** Parameters:
**	none.
**
** Returns:
**	none.
**
** Called by:
**	r_rep_do.
**
** Side Effects:
**	Sets up Ptr_rln_top, Ptr_pln_top, Ptr_cln_top, St_right_margin,
**	St_left_margin, St_cline, St_do_phead, St_in_pbreak,Bs_buf,
**	St_p_rm, St_to_term, En_rf_unit,C_buf,
**	St_rf_open, St_rwithin, St_pwithin, St_cwithin,
**	St_rcoms, St_pcoms, St_ccoms, Ptr_t_rbe, Ptr_t_pbe,
**	Ptr_t_cbe, St_rbe, St_pbe, St_cbe, St_c_rblk,
**	St_c_pblk, St_c_cblk, St_ulchar, St_p_length,
**	St_cdate, St_ctime.
**
** Trace Flags:
**	2.0, 2.3.
**
** Error Messages:
**	none.
**
** History:
**	7/20/81 (ps)	written.
**	7/12/82 (ps)	add check for file name "tt".
**	11/19/83 (gac)	added position_number.
**	1/20/84 (gac)	added St_cdate, St_cday, St_ctime.
**	6/29/84 (gac)	fixed bug 2327 -- default line maximum is
**			width of terminal unless -l or -f is specified.
**	1/8/85 (rlm)	put check on buffer allocation to prevent
**			redoing.
**	5/20/86 (jhw)	Removed VMS-dependent check for terminal as filename.
**	28-jun-89 (sylviap)
**		Added Direct Printing.
**	8/11/89 (elein) garnet
**		Added $variable evaluation for En_filename
**	1/3/90	(elein)	11/1/89 (mdw)   Changed variable dowfmtstr to be a
**			character array to avoid reentrancy problems in IBM.
**	1/24/90 (elein)
**		Use En_file_name as array, not char *. Decl changed
**	05-feb-90 (sylviap)
**		Changed the default page length for scrollable output. US #337
**	10/13/90 (elein) 33427/33806
**		Allow quoted strings for output file names and
**		distinguish between -f$xxx on the command line and
**		.output $xxx.  In the second case do variable evaluation,
**		if the first case not.
**	11/5/90 (elein) 34096
**		Pass token type instead of Tokchar to r_g_string. Bug caused
**		trailing double quote on output filename.
**	8-oct-1992 (rdrane)
**		Use new constants in r_reset invocations.  Converted r_error()
**		err_num value to hex to facilitate lookup in errw.msg file.
**		Ensure db_prec is initialized/propagated for proper DECIMAL
**		support.  Removed declaration of r_gt_ln() since it's in
**		the hdr file(s).
**	19-Feb-93 (fredb) hp9_mpe
**		MPE/iX requires the extra control that SIfopen offers over
**		SIopen.
**	17-mar-1993 (rdrane)
**		Eliminate references to Bs_buf (eliminated in fix for
**		b49991) and C_buf (just unused).
**	06-Apr-93 (fredb)  hp9_mpe
**		Do some fancy coding to establish the record length
**		for the SIfopen call as suggested by rdrane.
**	09-Apr-93 (fredb)  hp9_mpe
**		Fix record length calculation. MOD ('%') operator
**		should have been integer division ('/').
**	13-Apr-93 (fredb)  hp9_mpe
**		Make further adjustments to the record length
**		calculation.  It has been shown to fail with initial
**		lengths between (MPE_SECTOR-(MPE_VRECLGTH-1)) and
**		MPE_SECTOR.  Humbug ...
**	15-nov-93 (connie) Bug #47423
**		Moved the LOcopy statement from r_rep_do() to r_rep_set.
**		Save the location only in case where Nmloc is called.
**		The previous fix to this bug core dumps if using terminal.
**		
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	05-oct-2006 (gupsh01)
**	    Modify date to ingresdate.
**	18-nov-2008 (gupsh01)
**	    Added support for -noutf8align flag.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

#ifdef hp9_mpe
/*
** MPE's file system uses a physical sector size of 256 bytes.  When the
** file type indicates variable length records, 4 bytes out of each record 
** are reserved for an actual length field.  The record can span multiple
** sectors, but the file system works best with a maximum record length that
** is a multiple of the sector size.  The actual length field must be included
** in that length.  For instance, a maximum record length of 508 plus the 4
** bytes the file system adds for the actual record length makes a "real"
** record length of 512 bytes -- exactly 2 disk sectors.
**
** The maximum allowable record size is beyond the scope of what we might
** use here (about 32k) so there is no need to establish an upper limit.
**
** The calculation for the record length to ask the O/S for is a little
** tricky, so here are the steps in plain English:
**	1) Calculate total bytes required for 1 record on disk
**		a = En_lmax + MPE_VRECLGTH;
**	2) Reduce it by 1 as part of the numerical method used here
**		a = a - 1;
**	3) Calculate the number of full sectors required
**		a = a / MPE_SECTOR;
**	4) Add 1 to account for a partial sector being used
**		a = a + 1;
**	5) Calculate the total bytes required on disk from an integral
**	   number of sectors
**		a = a * MPE_SECTOR;
**	6) Subtract the number of bytes the O/S will add automatically
**		a = a - MPE_VRECLGTH;
**
**	All together now ...
**	MPE_recsize = (((((En_lmax + MPE_VRECLGTH)-1) / MPE_SECTOR)+1) *
**		       MPE_SECTOR)-MPE_VRECLGTH;
*/

#define MPE_SECTOR	256
#define MPE_VRECLGTH	4
#endif

VOID
r_rep_set(loc)
LOCATION	*loc;
{
	FMT	*fmt;
	PTR	area;
	char	*outname;
	char	*tempname;
	char	dowfmtstr[7];
	i4	size;
	DB_DATA_VALUE	longtext;
	AFE_DCL_TXT_MACRO(4)	buffer;
	DB_TEXT_STRING	*textstr;
	char	*buf; 
	i4	type;
	i4		MPE_recsize;
	LOCATION	temp_loc;
	static char	loc_buf[MAX_LOC+1];  /*static for later loc reference*/

	/* start of routine */


	/* open the report file */
	STcopy(ERx("d\"Sun\""), dowfmtstr); /* mdw */

	/* global to the report run */
	r_reset(RP_RESET_RELMEM5,RP_RESET_OUTPUT_FILE,RP_RESET_LIST_END);

	if (En_fspec != NULL)
	{	/* This file will override */
		STcopy( En_fspec, En_file_name);
	}

	if ((STlength(En_file_name) == 0) && !(St_print))
	{
		En_rf_unit = Fl_stdout; /* default */
	}
	else/* file name specified */
	{	
		if( STlength(En_file_name) > 0)
		{
			/* only for .output command do quote and
			** variable evaluation 33427
			*/
			if( En_fspec == NULL )
			{
				/* If the output file name is preceded by
		 		** a dollar, evaluate it as a parameter
				** 33806--strip quotes from name--quotes are
				** now passed thru from sreport. They may
				** be "escaping" $2$dua...
		 		*/
				r_g_set(En_file_name);
				switch( type = r_g_skip() )
				{
				case TK_DOLLAR:
					CMnext(Tokchar);
					outname = r_g_name();
					if( (tempname = r_par_get(outname)) == NULL )
					{
						r_error(0x3F0, FATAL, outname, NULL);
					}
					STcopy(tempname, En_file_name);
					break;
				case TK_QUOTE:
				case TK_SQUOTE:
			 		outname = r_g_string(type);
			   		r_strpslsh(outname);
					STcopy(outname, En_file_name);
					break;
				}
			}
			if (r_fcreate(En_file_name, &En_rf_unit, loc) < 0)
			{
				r_error(0x0B, FATAL, En_file_name, NULL);
			}
		}
		else 
		{
			/* Create a temp file for direct printing */
#ifdef hp9_mpe
			/* If this is MPE, calculate the record size first */
			MPE_recsize = (((((En_lmax + MPE_VRECLGTH)-1) / 
				       MPE_SECTOR)+1) * MPE_SECTOR) -
				       MPE_VRECLGTH;
#endif

			if (NMloc(TEMP, PATH, NULL, &temp_loc) != OK ||
			  LOuniq (ERx("ra"),ERx("tmp"),&temp_loc) != OK ||
#ifdef hp9_mpe
			SIfopen(&temp_loc, ERx("w"), SI_TXT, MPE_recsize, 
				&En_rf_unit) != OK)
#else
			SIopen(&temp_loc, ERx("w"), &En_rf_unit) != OK)
#endif
			{
				LOtos (&temp_loc, &buf);
				r_error(0x0B, FATAL, buf, NULL);
			}
			else
				/* copy the location returned from NMloc */
				/* since any later call to NMloc will    */
				/* overwrite the name                    */
				LOcopy(&temp_loc,loc_buf,loc);
		}
		if (!St_pl_set)
		{
			St_p_length = PL_DFLT;
		}
		St_rf_open = TRUE;
	}
	St_ulchar = '_';

	if (SIterminal(En_rf_unit))
	{
		St_to_term = TRUE;

		St_ulchar = '-';
		if (!St_pl_set)
		{
			/* 
			** Need four extra lines for scrollable output -
			** for the report title, status line, the solid line
			** and the command line.
			*/
			St_p_length = En_lines-4;
# ifdef FT3270
			/* changed from 3 to 6.  Hope this is right! */
			St_p_length -= 6;
# endif
		}

		if ( En_need_utf8_adj &&
              	     !(En_need_utf8_adj_frs))
                  En_need_utf8_adj = FALSE;
	}

	/* set up the line buffers, etc. */

	Ptr_rln_top = r_gt_ln((LN *)NULL);
	Ptr_pln_top = r_gt_ln((LN *)NULL);
	Ptr_cln_top = r_gt_ln((LN *)NULL);

	St_c_lntop = St_cline = Ptr_rln_top;
	St_pos_number = St_cline->ln_curpos;
	r_ln_clr(Ptr_rln_top);

	St_p_rm = St_right_margin = St_r_rm;
	St_p_lm = St_left_margin = St_r_lm;

	St_in_pbreak = FALSE;
	St_do_phead = TRUE;

	/* set up the .WITHIN and blocking vars, which may have
	** been set in loading the report */

	Ptr_t_rbe = Ptr_t_pbe = Ptr_t_cbe = NULL;
	St_rbe = St_pbe = St_cbe = NULL;
	St_c_rblk = St_c_pblk = St_c_cblk = 0;
	St_rcoms = St_pcoms = St_ccoms = NULL;
	St_rwithin = St_pwithin = St_cwithin = NULL;

	/* Setup current_time */

	r_convdbv(ERx("now"), ERx("ingresdate"), &St_ctime);

	/* Setup current_date */

	r_convdbv(ERx("today"), ERx("ingresdate"), &St_cdate);

	/* Setup current_day */

	fmt_areasize(Adf_scb, dowfmtstr, &size);
	area = MEreqmem(0,size,TRUE,NULL);
	fmt_setfmt(Adf_scb, dowfmtstr, area, &fmt, NULL);

	longtext.db_datatype = DB_LTXT_TYPE;
	longtext.db_length = sizeof(buffer);
	longtext.db_prec = 0;
	longtext.db_data = (PTR)&buffer;

	fmt_format(Adf_scb, fmt, &St_cdate, &longtext, FALSE);

	MEfree((PTR)area);

	textstr = (DB_TEXT_STRING *) &buffer;
	textstr->db_t_text[3] = EOS;
	r_convdbv(textstr->db_t_text, ERx("text(3)"), &St_cday);

	/* Setup initial space for evaluating expressions */

	r_ges_set();

	r_switch();

	return;
}
