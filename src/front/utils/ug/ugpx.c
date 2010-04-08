/*
**	Copyright (c) 2004 Ingres Corporation
**      All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<cm.h> 
# include	<er.h>
# include	<st.h>
# include	<fe.h> 
# include	<ug.h>


/*
**   IIUGpx_parmxtract -
**		Decompose a command line parameter, and determine its
**		type, identity, and associated value(s).
**
**		Command line parameters may be flags, metaflags, escaped
**		metaflags, or positional parameters.  Flags and metaflags
**		may have associated values.  Escaped metaflags and positional
**		parameters are treated more as literal strings.
**
**		Flags are introduced by the character "-" and "+",
**		followed by a character other than "/".
**
**		Metaflags are introduced by the characters "-/", followed
**		by a character other than "-".
**
**		Escaped metaflags are introduced by the characters "-/-/".
**		and followed by a character other than "-" and "+".
**
**		Positional parameters are introduced by any character other
**		than "-" and "+".
**
**		Quotes within strings are escaped via SQL rules - they are
**		always doubled.  This addreses SQL string literals and
**		SQL delimited identifiers.
**
**		Value lists are introduced by the character "=".  Values
**		within a value list are separated by the character ",".
**		Arbitrary white space is allowed around the equals sign
**		introducing a value list, as well as around the comma
**		separating values in a value list.
**
** Input:
**	buf	Pointer to the command line parameter string to be decomposed.
**		This is assumed to be a NULL terminated string, and is
**		expected to reflect one ARGV value.
**	ugpx	Pointer to a UG_PX_IDENT structure.  This should never
**		be specified as NULL if buf is non-NULL.
**	ugtag	Memory tag to use when allocating storage for parameter
**		component strings.
**
** Output:
**	ugpx	Structure filled as follows (even if buf was specified as NULL,
**		and usually if an error is detected):
**			px_type	  -
**				Set to indicate the type of parameter.
**			px_e_code -
**				Set to indicate any error detected.
**			px_name	  -
**				Pointer to the flag name, full text of the
**				escaped metaflag, or value of the positional
**				parameter name.
**			px_v_head -
**				Pointer to a chain of UG_PX_VAL structures
**				if the (meta)flag had an associated value,
**				or (UG_PX_VAL *)NULL.  Always (UG_PX_VAL *)NULL
**				for escaped metaflags and positional parameters.
**				For px_e_code of UG_PX_MPARMS (multiple
**				parameters found), this points to a single
**				UG_PX_VAL structure which points to the
**				extraneous parameter text.
**
** Returns:
**	TRUE -	buf was specified as NULL
**		-OR-
**		The parse succeeded. This does not guarantee that the resultant
**		flag, positional parameter, and/or associated values are valid
**		other than that they pass INGRES syntax conventions.  Even then,
**		certain processing shortcuts may mask errors such as unbalanced
**		quotes and short/missing value lists.  These situations affect
**		flags which have their introductory character mis-specified (or
**		just plain missing) and so are treated as positional parameters
**		(along with any value list), escaped metaflags, which are
**		similarly treated as positional parameters, and single value
**		value lists which do not contain embedded commas (unbalanced
**		quote errors only).
**
**	FALSE -	ugpx was specified as NULL and buf was specified as non-NULL
**		-OR -
**		a memory allocation error occured (UG_PX_ALLOC)
**		-OR-
**		the parse results represent known invalid formats
**		Specifically, the parameter string:
**			o contained an invalid (meta)flag introduction sequence
**			o a flag was followed by a character(s) other than '=',
**			  e.g., '-flag extraneous junk' (UG_PX_MPARMS).
**			o an expected value list was not found, e.g.,
**			  '-flag = ' (UG_PX_NO_VLIST).
**			o a value list ended prematurely, e.g.,
**			  '-flag = abc, def, ' (UG_PX_SHORT_VLIST).
**			o a value string had unbalanced quotes, e.g.,
**			  '-flag = "abcd, "def"' (UG_PX_UNBAL_Q).
**		The resultant UG_PX_IDENT should not necesarily be considered
**		valid under these circumstances.
**
** History:
**	4-oct-1993 (rdrane)
**		Created (celebrating the 36th anniversary of SPUTNIK).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


static	UG_PX_VAL	*cur_ugpx_v;
static	bool		IIUGpxv_pxvalue(char *,UG_PX_IDENT *,TAGID,i4);


bool
IIUGpx_parmxtract(char *buf,UG_PX_IDENT *ugpx,TAGID ugtag)
{
	char	*start_buf_ptr;
	i4	f_lgth;
	i4	f_delim;
	i4	q_delim;
	bool	in_quote;
	bool	ret_val;


	if  (ugpx == (UG_PX_IDENT *)NULL)
	{
		return(FALSE);
	}

	/*
	** Default to a positional parameter whose value is the
	** empty string.
	*/
	ugpx->px_type = UG_PX_POS;
	ugpx->px_e_code = UG_PX_NO_ERROR;
	ugpx->px_v_head = (UG_PX_VAL *)NULL;

	/*
	** Deal with the degenerate case ...
	*/
	if  ((buf == NULL) || (*buf == EOS))
	{
		ugpx->px_name = (char *)MEreqmem((u_i4)ugtag,(u_i4)1,
						 FALSE,(STATUS *)NULL);
		if  (ugpx->px_name == NULL)
		{
			ugpx->px_e_code = UG_PX_ALLOC_ERROR;
			return(FALSE);
		}
		*ugpx->px_name = EOS;
		return(TRUE);
	}

	/*
	** First, see if we have a simple positional parameter.  This
	** will be any string which does not start with "-" or "+".
	** Note that this will not detect unmatched delimiting quotes, nor
	** the erroneous presence of a value list (omitted flag introduction
	** character).
	*/

	if  ((*buf != ERx('-')) && (*buf != ERx('+')))
	{
		ugpx->px_name = (char *)MEreqmem((u_i4)ugtag,
					 (u_i4)(STlength(buf) + 1),
					 FALSE,(STATUS *)NULL);
		if  (ugpx->px_name == NULL)
		{
			ugpx->px_e_code = UG_PX_ALLOC_ERROR;
			return(FALSE);
		}
		STcopy(buf,ugpx->px_name);
		return(TRUE);
	}

	/*
	** We have a flag, a metaflag, or an escaped metaflag.  Save and
	** then skip over the initial flag introduction character(s).
	** Determine the type of flag.  If an escaped metaflag, simply copy
	** the whole string (sans initial metaflag introduction characters)
	** to px_name and return.
	** Note that this will not detect unmatched delimiting quotes, nor
	** the erroneous presence of a value list (omitted flag introduction
	** character).
	*/
	ugpx->px_type = UG_PX_FLAG;
	f_delim = *buf;
	CMnext(buf);
	if  ((f_delim == ERx('-')) && (*buf == ERx('/')))
	{
		ugpx->px_type = UG_PX_MFLAG;
		CMnext(buf);
		if  ((*buf != ERx('-')) && (!CMnmchar(buf)))
		{
			ugpx->px_e_code = UG_PX_INVALID_MFLAG;
		}
		else if ((*buf == ERx('-')) && (*(buf + 1) != ERx('/')))
		{
			ugpx->px_e_code = UG_PX_INVALID_EMFLAG;
		}
		else if ((*buf == ERx('-')) && (*(buf + 1) == ERx('/')))
		{
			ugpx->px_type = UG_PX_EMFLAG;
			f_lgth = STlength(buf);
			ugpx->px_name = (char *)MEreqmem((u_i4)ugtag,
				 	(u_i4)f_lgth,
				 	FALSE,(STATUS *)NULL);
			if  (ugpx->px_name == NULL)
			{
				ugpx->px_e_code = UG_PX_ALLOC_ERROR;
				return(FALSE);
			}
			STlcopy(buf,ugpx->px_name,f_lgth);
			return(TRUE);
		}
	}
	else if (!CMnmchar(buf))
	{
		ugpx->px_e_code = UG_PX_INVALID_FLAG;
	}

	/*
	** Extract the flag name.  This is all characters up to
	**	o '='
	**	o EOS
	**	o whitespace
	** We do this even if the flag was invalid, so we can return the
	** bad flag name.
	*/
	start_buf_ptr = buf;

	while ((*buf != ERx('=')) && (*buf != EOS) && (!CMwhite(buf)))
	{
		CMnext(buf);
	}

	f_lgth = buf - start_buf_ptr;
	ugpx->px_name = (char *)MEreqmem((u_i4)ugtag,
			 	(u_i4)f_lgth,
			 	FALSE,(STATUS *)NULL);
	if  (ugpx->px_name == NULL)
	{
		ugpx->px_e_code = UG_PX_ALLOC_ERROR;
		return(FALSE);
	}
	STlcopy(start_buf_ptr,ugpx->px_name,f_lgth);
	
	if  (ugpx->px_e_code != UG_PX_NO_ERROR)
	{
		/*
		** This should always be an invalid flag of some type.
		*/
		return(FALSE);
	}

	/*
	** Skip over any whitespace. There really shouldn't be any
	** unless a value list follows, but we'll let that go...
	** If we don't find an '=' (a value list), then we're all done.
	*/
	while (CMwhite(buf))
	{
		CMnext(buf);
	}
	if  (*buf != ERx('='))
	{
		/*
		** If we're at EOS, then we've gotten a "clean" flag.
		** Otherwise, additional "garbage" (not a valid value
		** list) is being ignored, so pass it back as a
		** single value, and flag the error appropriately.
		*/
		if  (*buf == EOS)
		{
			return(TRUE);
		}
		ugpx->px_e_code = UG_PX_MPARMS;
		ret_val = IIUGpxv_pxvalue(buf,ugpx,ugtag,STlength(buf));
		return(FALSE);
	}

	/*
	** We have a value list, and so now things get a bit tricky.
	** The values are delimited by commas, but we need to be careful
	** about being fooled by a comma within a string.  We'll try a
	** shortcut, since we mostly expect single values without
	** embedded commas.  But first, skip over the equals and any
	** trailing whitespace!  Encountering EOS is an error - we really
	** should have at least one value!
	*/
	CMnext(buf);
	while (CMwhite(buf))
	{
		CMnext(buf);
	}
	if  (*buf == EOS)
	{
		ugpx->px_e_code = UG_PX_NO_VLIST;
		return(FALSE);

	}

	if  (STindex(buf,ERx(","),0) == NULL)
	{
		ret_val = IIUGpxv_pxvalue(buf,ugpx,ugtag,STlength(buf));
		return(ret_val);
	}

	/*
	** The shortcut failed, so we're stuck with parsing out each value
	** in the list, and copying it into a UG_PX_VAL structure.  Sigh.
	*/
	in_quote = FALSE;
	start_buf_ptr = buf;
	while (*buf != EOS)
	{
		switch (*buf)
		{
		case ERx(','):
			/*
			** Probable value delimiter.
			*/
			if  (in_quote)
			{
				CMnext(buf);
				break;
			}
			if  (!IIUGpxv_pxvalue(start_buf_ptr,ugpx,ugtag,
					      (buf - start_buf_ptr)))
			{
				return(FALSE);
			}
			/*
			** Skip the comma and any trailing whitespace.  If we
			** hit EOS, then it's an error (similar to the bare
			** '=' case).
			*/
			CMnext(buf);
			while (CMwhite(buf))
			{
				CMnext(buf);
			}
			if  (*buf == EOS)
			{
				ugpx->px_e_code = UG_PX_SHORT_VLIST;
				return(FALSE);
			}
			/*
			** Reset our pointer to the start of the next value
			*/
			start_buf_ptr = buf;
			break;
		case ERx('\''):
		case ERx('\"'):
			if  (in_quote)
			{
				/*
				** Keep on going if it's not our
				** delimiting quote.
				*/
				if  (*buf != q_delim)
				{
					CMnext(buf);
					break;
				}
				/*
				** If it's a double instance of our delimiting
				** quote, then it's escaped.  Account for them
				** both, and keep on going.
				*/
				if  (*(buf + 1) == q_delim)
				{
					CMnext(buf);
					CMnext(buf);
					break;
				}
				/*
				** Single instance of delimiting quote
				** ends the string.
				*/
				in_quote = FALSE;
			}
			else
			{
				in_quote = TRUE;
				q_delim = *buf;
			}
			CMnext(buf);
			break;
		default:
			CMnext(buf);
			break;
		}
	}

	f_lgth = STlength(start_buf_ptr);
	if  (f_lgth > 0)
	{
		ret_val = IIUGpxv_pxvalue(start_buf_ptr,ugpx,ugtag,
					  f_lgth);
		/*
		** Check unbalanced quotes.  Note that we always
		** include the "partial" value.  Note also that
		** this will obscure any error encountered in
		** IIUGpxv_pxvalue()!
		*/
		if  (in_quote)
		{
			ugpx->px_e_code = UG_PX_UNBAL_Q;
			return(FALSE);
		}
		return(ret_val);
	}

	/*
	** We can only be here by ending on a comma, possibly followed
	** by whitespace.
	*/
	ugpx->px_e_code = UG_PX_SHORT_VLIST;
	return(FALSE);
}



static
bool
IIUGpxv_pxvalue(char *buf,UG_PX_IDENT *ugpx,TAGID ugtag,i4 lgth)
{
	UG_PX_VAL	*ugpx_v;


	/*
	** Allocate a UG_PX_VAL structure for this value.
	*/
	ugpx_v = (UG_PX_VAL *)MEreqmem((u_i4)ugtag,(u_i4)sizeof(UG_PX_VAL),
	 			       FALSE,(STATUS *)NULL);
	if  (ugpx_v == (UG_PX_VAL *)NULL)
	{
		ugpx->px_e_code = UG_PX_ALLOC_ERROR;
		return(FALSE);
	}

	/*
	** If this is the initial allocation, then establish the
	** chain head.  Otherwise, just chain this one on.
	*/
	if  (ugpx->px_v_head == (UG_PX_VAL *)NULL)
	{
		ugpx->px_v_head = ugpx_v;
	}
	else
	{
		cur_ugpx_v->px_v_next = ugpx_v;
	}
	cur_ugpx_v = ugpx_v;
	cur_ugpx_v->px_v_next = (UG_PX_VAL *)NULL;

	/*
	** Allocate the value buffer itself, and copy the value in,
	*/
	cur_ugpx_v->px_value = (char *)MEreqmem((u_i4)ugtag,
		 			(u_i4)(STlength(buf) + 1),
		 			FALSE,(STATUS *)NULL);
	if  (cur_ugpx_v->px_value == NULL)
	{
		ugpx->px_e_code = UG_PX_ALLOC_ERROR;
		return(FALSE);
	}

	STlcopy(buf,cur_ugpx_v->px_value,lgth);
	STtrmwhite(cur_ugpx_v->px_value);

	return(TRUE);
}

