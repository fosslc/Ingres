/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	 <fmt.h> 
# include	 <cm.h> 
# include	"format.h"
# include	<st.h>

/*
**	All get, copy, and form routines update the string pointer 'str'
**
**	History:
**
**		1/20/85 (rlm)	split out from dateutil so that these more
**				"generic" utilities can be used without
**				loading the data structure defined in
**				dateutil.
**		10/20/86 (KY)  -- Changed CH.h to CM.h.
**		9-sep-1992 (mgw) Bug #46546
**			Changed f_getnumber()'s third arg to a i4
**			to ensure at least 32 bit int.
**		09-jul-93 (essi)
**			Fixed bug 39148 by making sure the year-string
**			consist of digits only.
**		07-sep-93 (essi)
**			Backed out previous bug fix. Implemented a generic
**			one in findate.c.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

i4
f_getword(str, maxlen, word)
char	**str;
i4	maxlen;
char	*word;
	/* Get alphabetic 'word' from 'str' and return length of 'word'.
	** If 'maxlen' is greater than zero, then at most 'maxlen' chars will
	** be scanned.
	*/
{
	register char	*s;
	register i4	length = 0;

	if (maxlen == 0)
	{
		maxlen = 2000;
	}

	s = *str;
	while (maxlen > 0 && CMwhite(s))
	{
		CMbytedec(maxlen, s);
		CMnext(s);
	}

	while (maxlen > 0 && CMalpha(s))
	{
		CMbyteinc(length, s);
		CMbytedec(maxlen, s);
		CMcpyinc(s, word);
	}

	*word = EOS;
	*str = s;
	return(length);
}

i4
f_getnumber(str, maxlen, number)
char	**str;
i4	maxlen;
i4	*number;
	/* Get 'number' from 'str'.
	** Returns length of number (number of digits).
	** Number must be of the form:
	**	{<blank>} [+|-] {<blank>} <digit> {<digit>}
	** If 'maxlen' is greater than zero, then at most 'maxlen' chars will
	** be scanned.
	*/
{
	register char	*s;
	register char	*n;
	char		numstr[30];
	i4		length = 0;

	if (maxlen == 0)
	{
		maxlen = 2000;
	}

	s = *str;

	n = numstr;
	while (maxlen > 0 && CMwhite(s))
	{
		CMbytedec(maxlen, s);
		CMnext(s);
	}

	if (maxlen > 0 && (*s == '-' || *s == '+'))
	{
		CMbytedec(maxlen, s);
		CMcpyinc(s, n);
	}

	while (maxlen > 0 && CMwhite(s))
	{
		CMbytedec(maxlen, s);
		CMnext(s);
	}

	while (maxlen > 0 && CMdigit(s))
	{
		CMbyteinc(length, s);
		CMbytedec(maxlen, s);
		CMcpyinc(s, n);
	}

	*n = EOS;
	*str = s;
	if (length > 0)
	{
		CVal(numstr, number);
	}

	return(length);
}

f_copyword(word,str)
register char	*word;
char		**str;
	/* copy 'word' to 'str' */
{
	char	*s;

	s = *str;
	while (*word != EOS)
	{
		CMcpyinc(word, s);
	}

	*str = s;
}

i4
f_formnumber(number,length,str)
i4	number;
i4	length;
char	**str;
	/* Convert 'number' to a string. If its length <= 'length', place it
	   with its rightmost digit at 'str'; otherwise place it left-
	   justified. Leading blanks are not put in the string.
	   Returns length of number converted. */
{
	char	numstr[20];
	char	*s;
	i4	actual_len;

	s = *str;
	CVna(number, numstr);
	actual_len = STlength(numstr);
	if (actual_len <= length)
	{
		s -= (actual_len - 1);
	}
	else
	{
		s -= (length - 1);
	}

	f_copyword(numstr, &s);
	*str = s;
	return(actual_len);
}

i4
f_formword(table,index,template,str)
KYW		*table;
i4		index;
register char	*template;
char		**str;
	/* get word from the keyword 'table' indexed by 'index' and copy it
	   to 'str' using 'template' to determine the length of the word
	   and what letters are upper- and lower-case. Returns the length
	   of the word gotten. */
{
	register char	*word;
	register char	*s;
	bool		lastcharwasupper;
	i4		length;

	s = *str;
	word = table[index].k_name;
	length = STlength(word);
	while (*word != EOS)
	{
		if (*template != EOS)
		{
			lastcharwasupper = CMupper(template);
			CMnext(template);
		}

		if (lastcharwasupper)
		{
			CMtoupper(word, s);
		}
		else
		{
			CMcpychar(word, s);
		}

		CMnext(word);
		CMnext(s);
	}

	*str = s;
	return(length);
}
