/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <cm.h>
# include <st.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <rpu.h>

/**
** Name:	delimnam.c - delimited identifiers (names)
**
** Description:
**	Defines
**		RPedit_name - edit an Ingres name
**		RPgetwords - parse a string into words
**
** History:
**	16-dec-96 (joea)
**		Created based on delimnam.c in replicator library.
**	23-jul-98 (abbjo03)
**		Remove CLITERAL made obsolete by generic RepServer. Replace
**		DLNM_SIZE by formula based on DB_MAXNAME.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	06-Nov-2006 (kiria01) b117042
**	    Conform CMalpha/CMdigit macro calls to relaxed bool form
**/

static char	Edited_Name_Buf[DB_MAXNAME*2+3];


/*{
** Name:	RPedit_name - edit an Ingres name
**
** Description:
**	Edit an Ingres name as specified.  The edited name is placed in a
**	global buffer.
**
** Inputs:
**	edit_type	- edit type
**			  EDNM_ALPHA	- replace special chars with '_'.
**			  EDNM_DELIMIT	- Ingres delimited name.
**			  EDNM_SLITERAL	- SQL single quoted string.
**			  else		- no edit
**	name		- name to edit
**
** Outputs:
**	edited_name	- if supplied, this buffer is filled with the edited
**			  name;  if NULL, a global buffer is used and the
**			  caller should do an STcopy() upon return.
**
** Returns:
**	pointer to the edited name
**/
char *
RPedit_name(
i4	edit_type,
char	*name,
char	*edited_name)
{
	char	tmp_name[DB_MAXNAME*2+3];
	char	*t = tmp_name;
	char	*en;
	char	*n;

	if (edited_name != NULL)
		en = edited_name;
	else
		en = Edited_Name_Buf;

	switch (edit_type)
	{
	case EDNM_ALPHA:		/* alphanumeric */
		for (n = name; *n != EOS; CMnext(n))
		{
			if (CMalpha(n) || (n != name && CMdigit(n)))
				CMcpychar(n, t);
			else
				CMcpychar(ERx("_"), t);
			CMnext(t);
		}
		*t = EOS;
		break;

	case EDNM_DELIMIT:		/* delimited */
		CMcpychar(ERx("\""), t);
		CMnext(t);
		for (n = name; *n != EOS; CMnext(n))
		{
			CMcpychar(n, t);
			if (!CMcmpcase(t, ERx("\"")))
			{
				CMnext(t);
				CMcpychar(ERx("\""), t);
			}
			CMnext(t);
		}
		CMcpychar(ERx("\""), t);
		CMnext(t);
		*t = EOS;
		break;

	case EDNM_SLITERAL:		/* SQL quoted */
		CMcpychar(ERx("'"), t);
		CMnext(t);
		for (n = name; *n != EOS; CMnext(n))
		{
			if (!CMcmpcase(n, ERx("'")))
			{
				CMcpychar(ERx("'"), t);
				CMnext(t);
			}
			CMcpychar(n, t);
			CMnext(t);
		}
		CMcpychar(ERx("'"), t);
		CMnext(t);
		*t = EOS;
		break;

	default:			/* no edit */
		t = name;
		break;
	}

	STcopy(tmp_name, en);
	return (en);
}

/*{
** Name:	RPgetDbNodeName - Search for vnode in database param
**
** Description:
**	If the passed string is of form vnode::dbname, split them in two.
**
** Inputs:
**      string          - Pointer to the input string
**      outCount        - Pointer to result counter
**      dbname          - Pointer the the dbname pointer
**      vnode           - Pointer the vnode pointer
**
** Outputs:
**      outCount        - Number of items found: 1 or 2
**      dbname          - Always points to the database name
**      vnode           - May point to a vnode name if supplied.
**
** Returns:
**	Status          - OK, -1 otherwise
** History:
**      05-Oct-2009 (coomi01) b90678
**            Created.
**          
**/
STATUS
RPgetDbNodeName(char *string, i4 *outCount, char **dbname, char **vnode)
{
    char *endsVnode;
    i4 soFar  = 0;
    *outCount = 1; 	/* i.e. By default, this routine does nothing */
    *dbname  = string;
    *vnode   = NULL;

    if ((NULL == string) || (EOS == *string))
	return (-1);

    /* 
    ** It is important that we are not fooled into 
    ** accepting an initial vnode that is too long.
    */
    while ((EOS != *string) && (soFar < DB_MAXNAME))
    {
	/* Update the length */
	soFar += 1;

	/* 
	** Looking for '::'
	*/
	if ( ':' != *string )
	{
	    /* Nothing special, step directly to next */
	    CMnext(string);
	}
	else
	{
	    /*
	    ** Note where we saw it
	    */
	    endsVnode = string;

	    /* 
	    ** Step character 
	    */
	    CMnext(string);

	    /*
	    ** Look again
            ** - Note A vnode name may contain one, but not two,
            **   embeded colons. 
	    */
	    if ( ':' == *string )		
	    {
		/* This is what we want, now step onto dbName */
		CMnext(string);

		if ( EOS == *string )	
		{
                        /* If a double colon terminates the string, we
                        ** should not return a null database name.
                        ** - Actually, GCA will reject this possibility on startup.
                        */ 
			return OK;
		}


		/* 
		** Ok, terminate the first string
		** - which was the vnode not the dbase name
		**   So we have to change our minds a little here
		*/
		*vnode  = *dbname;
		*dbname = string;
		*outCount = 2;

		/*
		** Not forgetting to terminate the vnode substring
		*/
		*endsVnode = EOS;

		return (OK);
	    }
	}
    }

    return (OK);
}

/*{
** Name:	RPgetwords - parse a string into words
**
** Description:
**	RPgetwords() is similar to the CL STgetwords(), but allows
**	for the specification of word delimiters.  Consecutive word
**	delimiters in the parse string result in blank words.  The character
**	that follows the last non-white space character of each word is
**	replaced by the EOS character.
**
** Inputs:
**	delims		- pointer to string of delimiters
**	string		- pointer to string to be parsed
**	wordcount	- size of 'wordarray'
**
** Outputs:
**	wordcount	- number of words found
**	wordarray	- array of character pointers holding starting address
**			  of each word.
**
** Returns:
**	none
**
** Side effects:
**	Words in 'string' are terminated with EOS.
**/
void
RPgetwords(
char	*delims,
char	*string,
i4	*wordcount,
char	**wordarray)
{
	i4	count = 0;
	char	**org;
	char	*p;

	org = wordarray;

	while (count < *wordcount)
	{
		*wordarray++ = string;
		++count;

		p = string;
		while (*string != EOS && STindex(delims, string, 0) == NULL)
		{
			if (!CMwhite(string))
				p = string;
			CMnext(string);
		}
		if (p < string && !CMwhite(p))
			CMnext(p);

		if (*string == EOS)
		{
			*p = EOS;
			break;
		}

		*p = EOS;
		CMnext(string);
	}

	if (count < *wordcount)
		org[count] = NULL;
	*wordcount = count;
}
