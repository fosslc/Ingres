/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h>
# include	 <rglob.h>
# include	<errw.h>
# include       <er.h>

/*
**   R_EVAL_POS - evaluate the positioning of a .center, .right
**	or .left command to determine the correct position.
**	This is used in setting the margins for centering, and
**	for adding adusted text to the current output line.
**
**	Parameters:
**		tcmd	TCMD structure of adjusting command.
**		curpos	current position in line (used in relative)
**		werror	TRUE if called from r_x_adjust, and routine
**			is to write error messages, if found.
**
**	Returns:
**		position referred to in the command.
**
**	Called by:
**		r_x_adjust, r_stp_adjust, r_scn_tcmd.
**
**	Side Effects:
**		none.
**
**	Trace Flags:
**		5.0, 5.11.
**
**	Error Messages:
**		Syserr:Bad tcmd structure.
**		220, 221.
**
**	History:
**		7/21/81 (ps)	written.
**		6/7/84	(gac)	changed from i4  to f4.
**		12/21/89 (elein)
**			Fixing call to rsyserr
**		1/10/90 (elein)
**			Ensure call to rsyserr uses i4 ptr
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



f4
r_eval_pos(tcmd,curpos,werror)
register TCMD	*tcmd;
i4		curpos;
bool		werror;
{
	/* internal declarations */
	ATT		*r_gt_att();	/* (mmm) added declaration */

	/* internal declarations */

	register VP	*vp;			/* fast VP ptr */
	f4		position = -1;		/* found position */
	ATT		*att;			/* pointer to ATT if B_COLUMN */
	i4		err_num;


	/* start of routine */



	if (tcmd == NULL)
	{
		r_syserr(E_RW0023_r_eval_pos_Null_tcmd_);
	}

	if ((tcmd->tcmd_code!=P_CENTER) && (tcmd->tcmd_code!=P_RIGHT)
		&& (tcmd->tcmd_code!=P_LEFT))
	{
		err_num = tcmd->tcmd_code;
		r_syserr(E_RW0024_r_eval_pos_Bad_tcmd,&err_num);
	}

	vp = tcmd->tcmd_val.t_v_vp;

	switch(vp->vp_type)
	{
		case(B_ABSOLUTE):
			position = vp->vp_value;
			break;

		case(B_RELATIVE):
			position = curpos + vp->vp_value;
			break;

		case(B_COLUMN):
			att = r_gt_att((ATTRIB)vp->vp_value);
			switch(tcmd->tcmd_code)
			{
				case(P_CENTER):
					position = att->att_position +
						(r_ret_wid((ATTRIB)vp->vp_value)/2.0);
					break;

				case(P_LEFT):
					position = att->att_position;
					break;

				case(P_RIGHT):
					position = att->att_position +
						r_ret_wid((ATTRIB)vp->vp_value);
					break;
			}
			break;

		default:
			/* B_DEFAULT will be dealt with below */
			break;
	}

	/* check for bad position number */

	if (!(vp->vp_type == B_DEFAULT))
	{	/* check validity of value */
		if (((position<=0) && ((tcmd->tcmd_code==P_CENTER)
				||    (tcmd->tcmd_code==P_RIGHT)))
		   || ((position < 0) && (tcmd->tcmd_code==P_LEFT)))
		{
			if (werror)
			{
				r_error(220, NONFATAL, Cact_tname, Cact_attribute, NULL);
			}
			position = -1;
		}

		else if (((position>=En_lmax) && ((tcmd->tcmd_code==P_CENTER)
				||	    (tcmd->tcmd_code==P_LEFT)))
		   || ((position>En_lmax) && (tcmd->tcmd_code==P_RIGHT)))
		{
			if (werror)
			{
				r_error(221, NONFATAL, Cact_tname, Cact_attribute, NULL);
			}
			position = -1;
		}
	}

	/* if errors occurred, or if B_DEFAULT, set to default */

	if (position < 0)
	{
		switch(tcmd->tcmd_code)
		{
			case(P_CENTER):
				position = (St_left_margin+St_right_margin) / 2.0;
				break;

			case(P_LEFT):
				position = St_left_margin;
				break;

			case(P_RIGHT):
				position = St_right_margin;
				break;
		}

	}


	return(position);
}
