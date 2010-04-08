/*
**    Copyright (c) 1985, Ingres Corporation
*/

# include	<compat.h>
#include    <gl.h>
# include	<cm.h>

/*{
** Name:	cmkcheck	- check single or double byte character
**
** Description:
**	Check the next character in string for double byte character or
**	single.  If the next character is 1st byte of double byte 
**	character, this routine returnes the value _DBL1.  If the 
**	character is 2nd byte, this returnes the value _DBL2.  The
**	other characters, such as ASCII or Katakana, are represented
**	by the return value _A.  If the string is null or the next 
**	character is invalid, not range of double byte code, cmkcheck
**	retruns the value FALSE. 
**
** Inputs:
**	point	pointer to character to be checked.
**	strstart	start pointer of the string being processed
**		(or a location in the string which you know contains
**		the start of a character) as double byte processing
**		must reprocess the string from the start to decide
**		whether the character pointed by 'point' is double
**		byte character or single.
**
** Outputs:
**	Returns:
**		_DBL1	1st byte of double byte character
**		_DBL2	2nd byte of double byte character
**		_A	single byte character
**		FAIL	null string or invalid character
**
** Side Effects:
**	None
**
** History:
**	10-sep-1986 (yamamoto)
**		first written.
**	sep-90 (bobm)
**		changed old #define's (_A, etc) to new (CM_A_***).
**      16-jul-93 (ed)
**	    added gl.h
*/

u_i2
cmkcheck(point, strstart)
register u_char	*point;
u_char		*strstart;
{
	register u_char	*ptr;
	register u_i2	class;

	if (point == NULL || strstart == NULL)
		return (FAIL);

	class = CM_A_ALPHA;		/* instead of the initial state */
	for (ptr = strstart ; ptr <= point; ptr++)
	{
		switch (class)
		{
		case CM_A_ALPHA:
			if (CMdbl1st(ptr))
				class = CM_A_DBL1;
			break;

		case CM_A_DBL1:
			if (cmdbl2nd(ptr))
				class = CM_A_DBL2;
			else
				return (FAIL);		/* following character of _DBL1 must be _DBL2 */
			break;

		case CM_A_DBL2:
			if (CMdbl1st(ptr))
				class = CM_A_DBL1;
			else
				class = CM_A_ALPHA;
			break;
		}
	}

	return (class);
}

