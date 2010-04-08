
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 

/**
** Name:	rpstr.c		Contains routines to process strings specified
**				in a command.
**
** Description:
**	This file defines:
**
**	r_p_str		processes strings specified in a command.
**
** History:
**	31-mar-87 (grant)	created.
**	8/11/89 (elein) garnet
**		Changed to provide an ITEM expression to the tcmd
**		instead of a literal string.   Used for .ULC and .NULLSTRING
**		commands.
**	9/6/89 (elein) garnet
**		Add badexpr check with better error msg, require msg
**      9/27/89 (elein) garnet
**              Set the tcmd value here if the item is a constant
**              of the correct type and set item type to I_NONE
**              so that rxtcmd can skip evaluation.
**		
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
**/

/* # define's */
/* extern's */
/* static's */

/*{
** Name:	r_p_str		processes strings specified in a command.
**
** Description:
**	This routine processes strings specified in a given command.
**	The format is:
**		.command "string"
**
** Inputs:
**	code		code (P_...) for the command.
**
** Outputs:
**	none
**
**	Returns:
**		none.
**
** Side Effects:
**	Changes codes in Cact_tcmd.
**
** History:
**	31-mar-87 (grant)	implemented.
**	23-oct-1992 (rdrane)
**		Converted r_error() err_num values to hex to facilitate lookup
**		in errw.msg file.
*/

VOID
r_p_str(code)
ACTION	code;
{
    i4	tokvalue;
	ITEM		*item;
	STATUS		status;
	DB_DATA_VALUE	dbv;

	/* start of routine */

	item = (ITEM *)MEreqmem(0,sizeof(ITEM), TRUE, (STATUS *)NULL);
	status = r_g_expr(item, &dbv);

	if( status == BAD_EXP )
	{
		r_error(0x3F8,NONFATAL, Cact_tname, Cact_attribute,
			Cact_command, Cact_rtext, NULL);
		return;
	}
	if (item->item_type == I_NONE )
	{
		r_error(0x3F3, NONFATAL, Cact_tname,
			Cact_attribute, Cact_command, Cact_rtext, NULL);
		return;
	}
	else if (item->item_type == I_CON && 
		 item->item_val.i_v_con->db_datatype == DB_CHR_TYPE)
	{

		item->item_type = I_NONE;
		if( code == P_NULLSTR )
		{
			Cact_tcmd->tcmd_val.t_v_str =
			(char *)item->item_val.i_v_con->db_data;
		}
		else if(  code == P_ULC )
		{
			Cact_tcmd->tcmd_val.t_v_char =
			*(char *)item->item_val.i_v_con->db_data;
		}
	}

	Cact_tcmd->tcmd_code = code;
	Cact_tcmd->tcmd_item = item;

	if (r_g_skip() != TK_ENDSTRING)
	{	/* excess characters found */
		r_error(0x68, NONFATAL, Cact_tname, Cact_attribute,
			Cact_command, Cact_rtext, NULL);
		return;
	}
	return;

}
