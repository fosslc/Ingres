/*
** Copyright (c) 1989, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<cm.h>
# include	<cv.h>
# include	<st.h>
# include	<fglstr.h>

/**
** Name:	fgrest.c -	Convert restrictions to 4GL
**
** Description:
**	This file defines:
**
**	fgqrestrict	Convert restriction to 4gl
**
** History:
**	5/12/89 (Mike S) 	First version
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/

/* # define's */
# define K_NOT 		0
# define K_IS		1
# define K_BETWEEN 	2
# define K_IN 		3
# define K_LIKE		4

# define K_AND		-1
# define K_OR		-2

# define K_NONE		-99

/* GLOBALDEF's */
/* extern's */
FUNC_EXTERN char *	STskipblank();
FUNC_EXTERN LSTR *	IIFGaql_allocQueryLstr();

/* static's */
static i4  		get_op_keyword();
static struct kword	*get_keyword();
static struct kword	*get_or_and();
static bool		name_char();
static char 		*skip_string();

static const char _equal[] 	= " = ";
static const char opers[] 	= "!=<>^";
static const char openpar[] 	= "( ";
static const char open2pars[] = "( ( ";
static const char closepar[] = " ) ";
static const char close2pars[] = " ) ) ";
static const char _quote[] 	= "'";
static const char uscore[] 	= "_";

static const char _and[] 	= "AND";
static const char _between[] = "BETWEEN";
static const char _in[] 	= "IN";
static const char _is[] 	= "IS";
static const char _like[] 	= "LIKE";
static const char _not[] 	= "NOT";
static const char _or[] 	= "OR";

struct kword
{
	i4	token;
	char	*string;
	i4	length;
};

static struct kword op_keywords[] =
{
	{ K_NOT, 	_not, 	sizeof(_not) - 1 },
	{ K_IS, 	_is, 	sizeof(_is) - 1 },
	{ K_BETWEEN, 	_between, sizeof(_between) - 1 },
	{ K_IN, 	_in,	sizeof(_in) - 1 },
	{ K_LIKE, 	_like,	sizeof(_like) - 1 },
	{ 0, 		NULL,	0 }
};

static struct kword orand[] =
{
	{ K_OR, 	_or,	sizeof(_or) - 1 },
	{ K_AND, 	_and, 	sizeof(_and) - 1 },
	{ 0, 		NULL,	0 }
};

static struct kword nokey =
{
	K_NONE, NULL, 0
};

/*{
** Name:	IIFGmrs_makeRestrictString	- Convert resriction to 4GL
**
** Description:
**	Substitute in the column name where it it implicit in the restriction.
**	The grammar for a restiction looks like this:
**
**	restriction     :  clause 
**			| restriction AND clause 
**    			| restriction OR clause
**	clause          :  4GL-expression
**			|  predicate
**			|  binop 4GL-expression
**			|  trinop 4GL-expression AND 4GL-expression
**			|  selector ( 4GL-expression , ... )
**	predicate       :  NULL | NOT NULL
**	binop           :  = | != | < | <= | > | >= | <> | ^= | LIKE | NOT LIKE
**	trinop          :  BETWEEN | NOT BETWEEN
**	selector        :  IN | NOT IN
**								
**	So we substitute in at the beginning, and after AND or OR, except at
**	the AND which belongs to a BETWEEN.  We also substitute '=' before
**	a bare 4GL expression.
**
**
** Inputs:
**	field		char *	target field name
**	restriction	char *  restriction string
**
** Outputs:
**	last		LSTR ** last linked string in restriction.
**				undefined if NULL is returned.
**
**	Returns:
**			LSTR *	4GL restriction string
**				NULL if none is generated.
**
**	Exceptions:
**		none
**
** Side Effects:
**		none
**
** History:
**	5/12/89 (Mike S)	First version
**	1/91 (Mike S)
**		Fix bugs:
**		Parenthesize result (Bug 35618)
**		Set "last" correctly after bad syntax (Bug 35620).
*/
LSTR *
IIFGmrs_makeRestrictString( field, restriction, last )
char	*field;
char	*restriction;
LSTR	**last;
{
	char		*at_or_and;
	char		*past_or_and;
	struct kword	*or_and;
	i4		key;
	LSTR		*lstr0 = NULL, *lstr = NULL;

	for ( ; ; )
	{
		char	*opening;		/* One or two parentheses */

		restriction = STskipblank(restriction, STlength(restriction));

		if (restriction == NULL || *restriction == EOS)
		{
			/* 
			** This won't compile  -- It ends in OR or AND,
			** We'll let the compiler generate the error.
			*/
			STcat(lstr->string, closepar);
			*last = lstr;
			return(lstr0);
		}

		/*
		**	Insert field name.  If we don't have a 4GL operator	
		**	insert an equals sign too.	
		*/
		lstr = IIFGaql_allocQueryLstr(lstr, LSTR_NONE);
		if ((lstr0 == NULL) && (lstr != NULL))
		{
			lstr0 = lstr;
			opening = open2pars;
		}
		else
		{
			opening = openpar;
		}
		STpolycat(2, opening, field, lstr->string);
		key = K_NONE;
		if (STindex(opers, restriction, 0) == NULL)
		{
			key = get_op_keyword(restriction);
			if (key == K_NONE)
				STcat(lstr->string, _equal);
		}
		/*
		**	Find the next AND or OR, skipping the AND
		**	that goes with a BETWEEN. If there isn't one,
		**	we're done.
		*/
		lstr = IIFGaql_allocQueryLstr(lstr, LSTR_NONE);
		or_and = get_or_and(restriction, &at_or_and, &past_or_and);
		if (key == K_BETWEEN && or_and->token == K_AND)
			or_and = get_or_and(past_or_and, &at_or_and, 
					    &past_or_and);
		if (or_and->token == K_NONE)
		{
			STpolycat(2, restriction, close2pars, lstr->string);
			lstr->nl = TRUE;
			*last = lstr;
			return(lstr0);
		}
		else
		{
			STlcopy(restriction, lstr->string,
				at_or_and - restriction);
			STcat(lstr->string, closepar);
			STcat(lstr->string, or_and->string);
			restriction = past_or_and;
		}
	}
}

/*	
**	Return the value of the next word, if it's an operator keyword.  
**	If it's "NOT" return the value of the keyword negated, if any.
*/
static i4
get_op_keyword( start )
char	*start;
{
	struct kword 	*pkw;		/* keyword structure */
	i4		token = K_NONE; /* keyword number */
	i4		pass;		/* scan pass number */
	char		*past;		/* character past keyword */

	for (pass = 0; pass < 2; pass ++)
	{
		/* Look for a keyword */
		pkw = get_keyword(start, op_keywords, &past); 
		token = pkw->token;

		/* If we found a NOT on the first pass, try again. */
		if (token != K_NOT || pass > 0)
			break;

		start = past;
	}
	return token;
}

/*
**	Find next AND or OR, if any.
*/
static struct kword *
get_or_and( start, at_kw, past_kw )
char 	*start;		/* Start of string */
char	**at_kw;	/* Pointer to keyword (if not K_NONE) */
char	**past_kw;	/* Pointer past keyword (if not K_NONE) */
{
	char *ptr = start;
	struct kword *kw;

	/* Loop through string, looking for keywords */
	while (*ptr != EOS)
	{
		/* Skip a quoted string */
		if (STbcompare(ptr, MAXI2, _quote, sizeof(_quote)-1, TRUE) == 0)
		{
			ptr = skip_string(ptr);
			continue;
		}

		/* Skip anything that can't start a 4GL name */
		if (!name_char(ptr))
		{
			CMnext(ptr);
			continue;
		}
		
		/* We're at a 4GL name.  Check for "and" or "or" */
		kw = get_keyword(ptr, orand, past_kw);
		switch (kw->token)
		{
		    case K_NONE:
			/* Not a keyword.  Skip the name */
			while (name_char(ptr))
				CMnext(ptr);
			break;							      
		    case K_AND:
		    case K_OR:
			/* Found it */
			*at_kw = ptr;
			return(kw);
			break;
		}
	}
	return &nokey;
}

/*
**	See if a character could be part of a 4GL name, i.e. is alphanumeric 
**	or an underscore.
*/
static bool
name_char( ptr )
char *ptr;
{
	if (ptr == NULL)
		return FALSE;
	else
		return ( CMalpha(ptr) || CMdigit(ptr) || 
			 (CMcmpnocase(ptr, uscore) == 0) );
}

/*
**	If the string begins with a keyword, return a pointer to the keyword
**	strucutre.  Otherwise return a pointer to no_key.
*/
static struct kword
*get_keyword(start, kws, past)
char *start;    	/* Pointer to the start of the string. */
struct kword *kws;	/* Keyword list */
char **past;		/* Returned pointer past the keyword */
{
	char 		*ptr = start;
        struct kword    *pkw;
 
	/* Skip leading blanks */
	ptr = STskipblank(ptr, STlength(ptr));

	/* Look for all keywords */
	for (pkw = kws; pkw->string != NULL; pkw ++)
	{
		if (STbcompare(ptr, MAXI2, pkw->string, pkw->length, TRUE) 
			== 0)
		{
			*past = ptr + pkw->length;
			if (!name_char(*past))
				return(pkw);
		}
	}

	/* No keyword found */
	*past = start;
	return &nokey;
}


/*
**	Skip a quoted string and return a pointer to the character after 
**	the string ends.
*/
static char *
skip_string( ptr )
char *ptr;	/* Pointer to the start of the string. */
{
	char *p_next;	/* Next character */

	/*
	** Look for the closing quote. It's an error if
	** we don't find it, but we'll let the OSL
	** compiler report it.  We'll keep searching if
	** we find two quotes in a row; that's OSL for a quote
	** inside a string.
	*/
	for ( CMnext(ptr); ; CMnext(ptr) )
	{
		if (*ptr == EOS)
		{
			return (ptr);		/* The string isn't closed */
		}
		else if (STbcompare(ptr, MAXI2, _quote, sizeof(_quote)-1, TRUE)
			== 0)
	       	{
	       		p_next = ptr;
			CMnext(p_next);
			if (STbcompare(p_next, MAXI2, _quote, 
				       sizeof(_quote)-1, TRUE) != 0)
				return (p_next);/* We found the close quote */
			else
				ptr = p_next;	/* We found "''"; keep going */
		}
	}
}
