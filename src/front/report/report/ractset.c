/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include       <gl.h>
# include       <sl.h>
# include       <iicommon.h>
# include	<adf.h>
# include	<fe.h>
# include	<fmt.h>
# include	<ug.h>
# include	 <rtype.h>
# include	 <rglob.h>
# include	<errw.h>

/*
**   R_ACT_SET - main routine for setting up the RTEXT actions into
**	internal TCMD structures.  Find the code for the action and
**	call the appropriate routine to process the specific action.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
**
**	Trace Flags:
**		3.0, 3.4.
**		Flag 121 will write out the TCMD's done in this step.
**
**	History:
**		3/24/81 (ps) - written.
**		11/19/83 (gac)	added if statement.
**		11/14/85 (mgw)	Bug 6180 - disallow blocks that start in a
**				page section and don't end there.
**		17-nov-86 (mgw) Bug # 9360
**			augment previous fix to work for .end block (C_END),
**			not just .endblk and .endblock (C_EBLOCK)
**		16-dec-86 (mgw) Bug #
**		8/11/89 (elein) garnet
**			Change ULC case to call r_p_str instead of r_p_ulc.
**			This will allow expressions for ulc.  This also
**			makes the functioin r_p_ulc obsolete.
**			back out fix for bug 6180
**		9/5/89 (elein) garnet
**			Add nocache flag for those commands which cannot
**			do lookahead to next command.  Flag is checked in
**			rgeskip()
**		12/21/89 (elein) 
**			Corrected call to rsyserr
**		1/10/90 (elein)
**			Ensure parameter to rsyserr is i4 ptr
**		14-dec-1992 (rdrane)
**			Converted r_error() err_num value to hex to facilitate
**			lookup in errw.msg file.  Eliminate references to
**			r_syserr() and use IIUGerr() directly.  Fix the syserr
**			- it was passing the address of a i4 when it
**			should have passing the address of a nat.  This lets us
**			eliminate the local variable error_num so long as we
**			remember that ACTION is really a nat.
**		21-jun-1993 (rdrane)
**			Add support for suppression of initial formfeed.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

VOID
r_act_set()
{
	/* internal declarations */

	ACTION	rcode;		/* code for RTEXT command */


	/* start of routine */


	if ((rcode = r_gt_code(Cact_command)) == C_ERROR)
	{	/* bad command found */
		r_error(0x64, NONFATAL, Cact_tname, Cact_attribute,
			Cact_command, Cact_rtext, NULL);
		return;
	}	/* bad command found */


	/* if this is first command after the .WITHIN commands,
	** set the St_ccoms command */

	if ((rcode!=C_WITHIN) && (St_cwithin!=NULL) && (St_ccoms==NULL))
	{	/* first command of the .WI block */
		St_ccoms = Cact_tcmd;
	}

	/* if the last command was an adjusting command, this one must
	** be either .PRINT or .PRINTLN */

	if (St_adjusting)
	{
		if((rcode!=C_PRINTLN) && (rcode!=C_TEXT))
		{
			r_error(0x85, NONFATAL, Cact_tname, Cact_attribute,
				Cact_command, Cact_rtext, NULL);
		}
		St_adjusting = FALSE;		/* check was made */
	}

	/* process the proper command */

	r_g_set(Cact_rtext);		/* set up the token routines */
	switch (rcode)
	{
		case(C_PRINTLN):
		case(C_TEXT):
			r_p_text(rcode);
			break;

		case(C_ETEXT):
			r_p_eprint();
			break;

		case(C_NEWLINE):
			St_cache = FALSE;
			r_p_usnum(P_NEWLINE,NL_DFLT,1);
			St_cache = TRUE;
			break;

		case(C_TAB):
			St_cache = FALSE;
			r_p_vpnum(P_TAB);
			St_cache = TRUE;
			break;

		case(C_NEED):
			St_cache = FALSE;
			r_p_usnum(P_NEED,NEED_DFLT,1);
			St_cache = TRUE;
			break;

		case(C_NPAGE):
			St_cache = FALSE;
			r_p_snum(P_NPAGE);
			St_cache = TRUE;
			break;

		case(C_CENTER):
			St_cache = FALSE;
			r_p_vpnum(P_CENTER);
			St_adjusting = TRUE;
			St_cache = TRUE;
			break;

		case(C_RIGHT):
			St_cache = FALSE;
			r_p_vpnum(P_RIGHT);
			St_adjusting = TRUE;
			St_cache = TRUE;
			break;

		case(C_LEFT):
			St_cache = FALSE;
			r_p_vpnum(P_LEFT);
			St_adjusting = TRUE;
			St_cache = TRUE;
			break;

		case(C_FORMAT):
			r_p_format(P_FORMAT);
			break;

		case(C_TFORMAT):
			r_p_format(P_TFORMAT);
			break;

		case(C_POSITION):
			r_p_pos();
			break;

		case(C_WIDTH):
			r_p_width();
			break;

		case(C_LM):
			St_cache = FALSE;
			r_p_snum(P_LM);
			St_cache = TRUE;
			break;

		case(C_RM):
			St_cache = FALSE;
			r_p_snum(P_RM);
			St_cache = TRUE;
			break;

		case(C_PL):
			St_cache = FALSE;
			r_p_usnum(P_PL,PL_DFLT,0);
			St_cache = TRUE;
			break;

		case(C_UL):
			r_p_flag(P_UL);
			break;

		case(C_NOUL):
			r_p_eflag(P_UL);
			break;

		case(C_WITHIN):
			r_p_within();
			break;

		case(C_EWITHIN):
			r_p_ewithin();
			break;

		case(C_BLOCK):
			r_p_flag(P_BLOCK);
			break;

		case(C_EBLOCK):
			r_p_eflag(P_BLOCK);
			break;

		case(C_TOP):
			r_p_flag(P_TOP);
			break;

		case(C_BOTTOM):
			r_p_flag(P_BOTTOM);
			break;

		case(C_LINEEND):
			r_p_flag(P_LINEEND);
			break;

		case(C_LINESTART):
			r_p_flag(P_LINESTART);
			break;

		case(C_END):
			r_p_end();
			break;

		case(C_BEGIN):
			r_p_begin();
			break;

		case(C_ULC):
			St_cache = FALSE;
			r_p_str(P_ULC);
			St_cache = TRUE;
			break;

		case(C_FF):
			r_p_flag(P_FF);
			break;

		case(C_NOFF):
			r_p_eflag(P_FF);
			break;

		case(C_NO1STFF):
			r_p_flag(P_NO1STFF);
			break;

		case(C_IF):
			r_p_if();
			break;

		case(C_ELSE):
		case(C_ELSEIF):
		case(C_ENDIF):
			break;

		case(C_NULLSTR):
			St_cache = FALSE;
			r_p_str(P_NULLSTR);
			St_cache = TRUE;
			break;

		case(C_LET):
			r_p_let();
			break;

		case(C_ELET):
			break;

		default:
			IIUGerr(E_RW0004_r_act_set_bad_code,UG_ERR_FATAL,
				1,&rcode);


	}

	return;
}
