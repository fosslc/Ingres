/*
** Copyright (c) 2004 Ingres Corporation
*/

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
# include	<er.h>


/*
**   R_GT_FUNCT - return the operator code for a function or aggregate name.
**
**	Parameters:
**		name		name of function.
**
**	Returns:
**		Operator code of the function, if it exists.
**		Returns OP_NONE if the function name does not exist.
**
**	History:
**		1/24/84 (gac)	written.
*/



ADI_OP_ID	
r_gt_funct(name)
char	*name;
{
    ADI_OP_ID	op_code;
    STATUS	status;

    if (STbcompare(name, 0, ERx("break"), 0, TRUE) == 0)
    {
	op_code = OP_BREAK;
    }
    else
    {
	status = afe_opid(Adf_scb, name, &op_code);
	if (status != OK)
	{
	    op_code = OP_NONE;
	}
    }

    return op_code;
}
