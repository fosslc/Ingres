# include	 <compat.h>
# include	 <me.h>
# include	 <st.h>

# ifdef VMS
/*
** IICNVPACK -  C routines to convert to and from packed data types for 
**		VMS packed decimal data types used by Equel/Cobol.
**
** Defines:	IIdtop -- Convert f8 into packed decimal.
** 		IIltop -- Convert f4 into packed decimal.
**		IIptod -- Convert packed decimal into f8.
** History:
**		06-feb-1985 - Merged old files into one for Equel. (ncg)
**		20-jul-1989 - Fixed bug 7063 where conversion of negative
**			      float values fails because CVfa gets an
**			      insufficient width if the number of digits
**			      of the converted value is actually as big
**			      as 'len'.  (When calculating the width for
**			      the CVfa call, we weren't counting the sign 
**			      byte required for negative numbers.)
**			      (barbara)
**		01-Aug-1989 - shut ranlib up with static. (GordonW)
**		21-jan-1991 - Fix bug 35373. Modify IIdtop to use a
**			      maximum width for CVfa call to avoid
**			      overflow (i.e., '*******') situation.
**			      If number is too large to fit into
**			      host variable, let macro routine IIntop
**			      do appropriate COBOL truncation. (teresal)
**			      
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
**  IIDTOP -- convert f8 floating to packed decimal
**
**	Parameters:
**		value -- f8 to convert
**		addr  -- address of buffer for packed decimal number
**		len   -- length of packed decimal number
**		scale -- scaling factor (10**scale)
**	Returns:
**		None
*/

void
IIdtop( value, addr, len, scale )
f8	value;
char	*addr;
i4	len;
i4	scale;
{

	register char	*a;
	register i2	l;
	char		temp[100];
	i2		dummy;
	i4		float_width;

	if (value >= 0.0)
	{
	    temp[0] = '+';
	    a = &temp[1];
	}
	else
	{
	    a = temp;
	}

	/* 
	** We always want CVfa to return a formatted number here,
	** even if the number is too large to fit into the host
	** variable (let IIntop truncate number, if needed).  To 
	** avoid an overflow ('******') result and thus a stack dump,
	** use size of temp buffer as formatted width size (should
	** be big enough for any reasonable number). (bug #35373)
	*/
	float_width = (sizeof(temp) - 1);
	CVfa(value, float_width, scale, 'f', '.', a, &dummy);

	/* rip out the decimal point */
	if (scale)
	{
		l = STlength(temp);
		MEcopy(temp + l - scale, scale + 1, temp + l - scale - 1);
	}
	/* We now have leading numeric separate string to convert to packed */
	IIntop(temp, STlength(temp) - 1, addr, len);
}	


/*
**  IILTOP -- convert longword to packed decimal
**
**	Parameters:
**		value -- integer to convert
**		addr  -- address of buffer for packed decimal number
**		len   -- length of packed decimal number
**		scale -- scaling factor (10**scale)
**	Returns:
**		None
*/

void
IIltop(value, addr, len, scale)
i4	value;
char	*addr;
i4	len;
i4	scale;
{
	register char	*a;
	char		temp[20];
	char		ptemp[20];

	a = &temp[1];
	CVla(value, a);

	/* make sure we have a sign */
	if (*a != '-')
		*--a = '+';

	/* we now have leading numeric separate string to convert to packed */
	IIntop(a, STlength(a) - 1, ptemp, len);
	/* 
	** Now scale for decimal positions. Note that scale can only be 
	** positive until we support implicit decimal positions.
	*/
	IIpscale(ptemp, len, addr, len, scale, 5);
}	

/*
**  IIPTOD -- convert packed decimal to f8 floating
**
**	Parameters:
**		value -- address of f8 to return
**		addr  -- address of buffer for packed decimal number
**		len   -- length of packed decimal number
**		scale -- scaling factor (10**scale)
**	Returns:
**		None
*/

void
IIptod(value, addr, len, scale)
f8	*value;
char	*addr;
i4	len;
i4	scale;
{

	char		temp[30];

	IIpton(addr, len, temp, len);
	temp[len+1] = '\0';
	MEcopy(&temp[len - scale + 1], scale + 1, &temp[len - scale + 2]);
	temp[len - scale + 1] = '.';
	CVaf(temp, '.', value);
}	
# else
static	i4 	ranlib_dummy;
# endif /* VMS */
