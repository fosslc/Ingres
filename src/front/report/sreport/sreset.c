/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<stype.h>
# include	<sglob.h>
# include	<er.h>
# include	"ersr.h"

/*
**   S_RESET - routine to reset the values of variables in
**	the report specifier.  This sets the values of variables
**	to defaults (or NULL) before various stages of the
**	report specifier are done.
**
**	Parameters:
**		type... Type codes to indicate the nature of the reset.
**			The number of type codes cannot exceed the total number
**			of possible type codes plus one.  The last type code
**			must be RP_RESET_LIST_END.  The codes are defined in
**			rcons.h.
**
**	Returns:
**		none.
**
**	Side Effects:
**		resets everything.
**
**	Called by:
**		many routines in SREPORT and RBF.
**
**	Trace Flags:
**		2.0, 2.3.
**
**	History:
**		8/23/82 (ps)	written.
**		11/16/87 (elein) garnet
**			Add St_clean_given and St_setup_given
**			to group of items to be reset when
**			beginning a new report.
**		1/25/90 (elein)
**			Decl for En_rcommands was changed to char []. 
**		17-apr-90 (sylviap)
**			Reset En_width_given for .PAGEWIDTH support (#129, #588)
**		24-apr-90 (sylviap)
**			Added St_one_name. (US #347)
**		7/13/90 (elein)
**			Reset Declare sequence number for each new report
**			to allow unordered multiple declares.
**		12-aug-1992 (rdrane)
**			Replace bare constant reset state codes with #define
**			constants.  Add explicit return statement.  
**			Eliminate references to r_syserr() and use IIUGerr()
**			directly.
**		9-oct-1992 (rdrane)
**			Declare s_reset() as returning VOID, and declare
**			s_rreset() as being static.
**		25-nov-1992 (rdrane)
**			Rename references to .ANSI92 to .DELIMID as per the LRC.
**		05-Apr-93 (fredb)
**			Removed references to "St_txt_open", "Fl_input" will
**			be used instead.
*/

static	VOID	s_rreset();


/* VARARGS1 */
VOID
s_reset(t1,t2,t3,t4)
i2	t1,t2,t3,t4;
{
	/* start of routine */


	if (t1 != RP_RESET_LIST_END)
	{
	   s_rreset(t1);
	   if (t2 != RP_RESET_LIST_END)
	   {
	      s_rreset(t2);
	      if (t3 != RP_RESET_LIST_END)
	      {
		 s_rreset(t3);
		 if (t4 != RP_RESET_LIST_END)
		 {
		    IIUGerr(E_SR000C_s_reset_Too_many_args,
			    UG_ERR_FATAL,0);
		 }
	      }
	   }
	}

	return;
}

static
VOID
s_rreset(type)
i2	type;
{
	switch (type)
	{
	case RP_RESET_PGM:
		/* start of program */
		St_keep_files = FALSE;
		St_one_name = FALSE;
		En_sfile = ERx("");	/* ptr to file name of text file */
		En_rcommands[0] = EOS;	/* name of Rcommands file */
		break;

	case RP_RESET_SRC_FILE:
		/* Start of new file */
		St_rco_open = FALSE;	/* TRUE when En_rcommand open */
		Line_num = 0;		/* line number in text file */
		Ptr_ren_top = NULL;	/* top of REN linked list */
		Cact_ren = NULL;	/* current REN structure */
		break;

	case RP_RESET_REPORT:
		/* start of new report */
		Cact_rso = NULL;	/* Current RSO structure */
		St_q_started = FALSE;	/* started query for this report */
		St_o_given = FALSE;	/* specified .output given  */
		St_s_given = FALSE;	/* specified if .sort given */
		St_b_given = FALSE;	/* specified if .break given */
		St_sr_given = FALSE;	/* specified if .shortrem given */
		St_lr_given = FALSE;	/* specified if .longrem given */
		St_d_given = FALSE;	/* specified if .declare given */
		St_setup_given = FALSE;	/* specified if .setup given */
		St_clean_given = FALSE;	/* specified if .cleanup given */
		St_width_given = FALSE;	/* specified if .pagewidth given */
		St_xns_given = FALSE;	/*
								** specified if .delimid (eXpanded Name Space)
								** given
								*/
		Csequence = 0;		/* current rcosequence */
		Seq_declare = 0;	/* Sequence of declared vars */
		break;
	}

	return;
}
