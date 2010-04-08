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
**   R_PC_SET -  routine for setting up the time, day, date etc
**	into an item for an expression.
**
**	Parameters:
**		name	name of potential program constant.
**		item	pointer returned to item for program constant.
**
**	Returns:
**		pc code:
**			PC_ERROR if not a program constant.
**			PC_CNAME if w_name.
**			PC_DATE  if current_date.
**			PC_DAY   if current_day.
**			PC_TIME  if current_time.
**
**	Side Effects:
**		none.
**
**	Error messages:
**		150.
**
**	Trace Flags:
**		3.0, 3.5.
**
**	History:
**		10/14/81 (ps) - written.
**		6/20/83 (gac) - added date templates.
**		12/5/83 (gac)	allow unlimited-length parameters.
**		12/20/83 (gac)	creates item for expressions.
*/


/*
**	Control word table.  This table is external to the
**		two routines in this file.  To add program constants,
**		add to this table, and add a code to the RCONS.H
**		file.
*/

struct	pcons_word
{
	char	*name;			/* ptr to one allowable name */
	i2	code;			/* associated constant code */
	bool	primary;		/* TRUE if this is primary ref */
};

static struct	pcons_word Pcommands[] =
{
	ERx("current_date"),	PC_DATE,	TRUE,
	ERx("current_day"),	PC_DAY,		TRUE,
	ERx("current_time"),	PC_TIME,	TRUE,
	ERx("cur_date"),	PC_DATE,	FALSE,
	ERx("cur_day"),	PC_DAY,		FALSE,
	ERx("cur_time"),	PC_TIME,	FALSE,
	ERx("w_name"),	PC_CNAME,	FALSE,
	ERx("wi_name"),	PC_CNAME,	FALSE,
	ERx("within_name"),	PC_CNAME,	TRUE,
	{0},		{0},		{0}
};

i2
r_pc_set(name, item)
char	*name;
ITEM	*item;
{
	/* internal declarations */

	register	i2	pcode;		/* code for program constant */
	i2			r_gt_pcode();

	/* start of routine */


	if ((pcode = r_gt_pcode(name)) == PC_ERROR)
	{	/* not a program constant */
		return(PC_ERROR);
	}


	/* process the proper command */

	if (pcode == PC_CNAME && St_cwithin == NULL)
	{	/* not in a within loop */
		r_error(181,NONFATAL, Cact_tname, Cact_attribute, 
			Cact_command, Cact_rtext, NULL);
	}

	item->item_type = I_PC;
	item->item_val.i_v_pc = pcode;

	/* must be ok if it makes it here */


	return(pcode);
}

/*
**   R_GET_PCODE - return the program constant code for the name given.
**
**	Parameters:
**		*ctext - name to check. May contain upper or lower
**			case characters.
**
**	Returns:
**		Code of the constant, if it exists.  Returns -1 if
**		the command is not in the list.
**
**	Side Effects:
**		none.
**
**	Trace Flags:
**		3.0, 3.5.
**
**	History:
**		103/14/81 (ps) - written.
*/

i2	
r_gt_pcode(ctext)
char	*ctext;

{
	/* internal declarations */

	register struct	pcons_word	*pc;		/* ptr to name table */

	/* start of routine */



	/* mask out any upper case characters */


	for (pc = Pcommands; pc->name; pc++)
	{	/* check next command */
		if ((STbcompare(pc->name, 0,ctext, 0, TRUE) == 0))
			return(pc->code);
	}

	/* not in table */

	return (PC_ERROR);
}
