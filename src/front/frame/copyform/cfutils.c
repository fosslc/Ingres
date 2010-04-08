/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<st.h>
# include	<cm.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ooclass.h>
# include	<cf.h>

/**
** Name:	cfutils.c - utility procedures for COPYFORM.
**
** Description:
**	This file defines:
**
**	cf_rectype	get the record type for an intermediate file record.
**	cf_gettok	get the next tab-separated token in a string.
**	cf_inarray	determine is string exists previously in array.
**
** History:
**	Revision 6.0  87/04/22  rdesmond
**	Written.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
** Name:	cf_rectype - get the record type of a record from the
**				intermediate COPYFORM file.
**
** Description:
**	Given a ptr to an input string, returns the record type of the
**	intermediate form file.	 The record type is determined by a pattern
**	at the beginning of the string passed.	If the first character of
**	the input string is a tab, then the record type is determined from the
**	record type from the last call to this procedure.
**
** Input params:
**	token		ptr to record from file.
**
** Output params:
**
** Returns:
**	record_type	RT_FORMHDR
**			RT_FORMDET
**			RT_FIELDHDR
**			RT_FIELDDET
**			RT_TRIMHDR
**			RT_TRIMDET
**			RT_QBFHDR
**			RT_QBFDET
**			RT_JDEFHDR
**			RT_JDEFDET
**			RT_BADTYPE
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	23-Apr-87 (rdesmond) written.
*/
i4
cf_rectype(token)
char	*token;
{
	static i4  cur_type;

	if (token[0] == '\t')
	{
		return(cur_type);
	}
	else if (STbcompare(token, 8, ERx("QBFNAME:"), 8, FALSE) == 0)
	{
		cur_type = RT_QBFDET;
		return(RT_QBFHDR);
	}
	else if (STbcompare(token, 8, ERx("JOINDEF:"), 8, FALSE) == 0)
	{
		cur_type = RT_JDEFDET;
		return(RT_JDEFHDR);
	}
	else if (STbcompare(token, 5, ERx("FORM:"), 5, FALSE) == 0)
	{
		cur_type = RT_FORMDET;
		return(RT_FORMHDR);
	}
	else if (STbcompare(token, 6, ERx("FIELD:"), 6, FALSE) == 0)
	{
		cur_type = RT_FIELDDET;
		return(RT_FIELDHDR);
	}
	else if (STbcompare(token, 5, ERx("TRIM:"), 5, FALSE) == 0)
	{
		cur_type = RT_TRIMDET;
		return(RT_TRIMHDR);
	}
	else
	{
		return(RT_BADTYPE);
	}
}

/*{
** Name:	cf_gettok - get next tab-separated token from string
**
** Description:
**	Simple scanner to retrieve tokens from a string.  Called first
**	with the pointer to a string, then successively called with a null 
**	pointer to return the next token in the string.	 A pointer to the
**	token is returned with a null character replacing the tab separator
**	(this is destructive to the input string).  A token with a null
**	value is legal, i.e. if there are two consecutive tabs in the input
**	string.	 When the end of the string is reached, delimited by either
**	a null or a new-line character, a null pointer will be returned.
**	Tokens may have embedded spaces, and there is no compression done
**	on tokens.
**
** Input params:
**	instring	pointer to input string or NULL for successive calls
**
** Output params:
**	none
**
** Returns:
**	pointer to next token in string
**
** Exceptions:
**	none
**
** Side Effects:
**	Destroys input string by replacing tabs with nulls.
**
** History:
**	19-Apr-87 (rdesmond) written.
*/
char *
cf_gettok(instring)
char	*instring;
{
	char	*start_loc;
	char	*curr_loc;
	static char	*save_loc;

	start_loc = (instring != NULL) ? instring : save_loc;
	if (start_loc == NULL)
		return(NULL);

	curr_loc = start_loc;

	while (*curr_loc != '\0' && *curr_loc != '\t' && *curr_loc != '\n')
		CMnext(curr_loc);

	save_loc = (*curr_loc == '\t') ? curr_loc + 1 : NULL;
	*curr_loc = '\0';
	return(start_loc);
}

/*{
** Name:	cf_inarray - determine if string appears in array of strings
**
** Description:
**	given an array of ptrs to strings and an index, determine if the string
**	pointed to by the element of the array referenced by index appears in
**	the array before the given index.  This is used when there can be
**	multiple occurences of a string in the array to determine if the
**	string occurs previously.
**	
**
** Input params:
**	array		array of pointers to strings
**	index		index into array pointing to string to be checked.
**
** Output params:
**	none
**
** Returns:
**	TRUE		string appears previously in array.
**	FALSE		string does not appear previously in array.
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	22-Apr-87 (rdesmond) - written.
*/
bool
cf_inarray(array, index)
char	*array[];
i4	index;
{
	i4 i;

	for (i = 0; STcompare(array[i], array[index]) != 0; i++)
		;

	if (i == index)
		return(FALSE);
	else
		return(TRUE);
}
