/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rxpc.c	30.1	11/14/84"; */

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
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
**   R_X_PC - evaluate a program constant.
**		If it is the current_date or current_time, the value
**		is a date structure. If it is the current_day or w_name, it is
**		a string.
**
**	Parameters:
**		pc_code	pc code.
**		val	pointer to the value of the program constant
**			returned by this routine.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
**
**	Called by:
**		r_e_item.
**
**	Trace Flags:
**		none.
**
**	History:
**		2/3/84 (gac)	written.
**		23-oct-1992 (rdrane)
**			Ensure set/propagate precision in DB_DATA_VALUE
**			structures.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


VOID
r_x_pc(pc_code,val)
i2		pc_code;
DB_DATA_VALUE	*val;
{
	char	*str;

	/* start of routine */

	switch (pc_code)
	{
	case(PC_DAY):
		MEcopy((PTR)&St_cday, (u_i2)sizeof(DB_DATA_VALUE), (PTR)val);
		break;

	case(PC_DATE):
		MEcopy((PTR)&St_cdate, (u_i2)sizeof(DB_DATA_VALUE), (PTR)val);
		break;

	case(PC_TIME):
		MEcopy((PTR)&St_ctime, (u_i2)sizeof(DB_DATA_VALUE), (PTR)val);
		break;

	case(PC_CNAME):
		val->db_datatype = DB_CHR_TYPE;
		val->db_data = r_gt_att(A_GROUP)->att_name;
		val->db_length = STlength(val->db_data);
		val->db_prec = 0;
		break;
	}
}
