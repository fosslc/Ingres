/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<cm.h>
# include	<cv.h>
# include	<er.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<ui.h>

static	bool	IIUGdic_ChkdlmChar();

/*
** Name:	uglower.c -	Front-End Utility DBMS Case Matching Module.
**
** Description:
**	Contains routines to adjust the case of a DBMS object name, which may
**	be a delimited identifier.  All new code is expected to use the
**	IIUGdlm_ChkdlmBEobject() and/or IIUGdbo_dlmBEobject() routine.  The
**	IIUGlbo_lowerBEobject() routine, which historically lowered a DBMS object
**	name only if it was INGRES and hence the "lower" in the routine name, is
**	retained for compatibility, but will simply reference the new routine.
**
**
** History:
**	5-nov-1987 danielt written
**	5-jun-92 (rdrane)
**		Added the IIUGdbo_dlmBEobject() routine to support casing of delimited
**		identifiers (Release 6.5).
**	26-jun-92 (rdrane)
**		Added more routines to support delimited identifiers.
**		IIUGdlm_ChkdlmBEobject() accepts a string, determines if it is a
**		delimited identifier, and conditionally resolves it to its BE
**		representation.  String has the form ["]name["].  IIUGrqd_Requote_dlm()
**		re-quotes a delimited identifier (or any string) and re-escapes any
**		embedded double quotes.
**	16-jul-92 (rdrane)
**		IIUGdlm_ChkdlmBEobject() changes.  Handle STtrmwhite() overzealousness.
**		Conditionalize SQL reserved word lookup.
**	31-jul-92 (rdrane)
**		IIUGdlm_ChkdlmBEobject() changes - effect identifier validation and
**		return indication of identifier type or invalid.
**	11-aug-1992 (rdrane)
**		Fix-up handling of underscore in IIUGdlm_ChkdlmBEobject().
**	18-aug-1992 (rdrane)
**		Fix-up handling of INGRES "special characters" in
**		IIUGdlm_ChkdlmBEobject().  Watch for calls when pre-6.5 connection.
**		Use IIUGIsSQLKeyWord() since IIUGsrw_LookUp() is now defunct.
**	04-sep-1992 (rdrane)
**		Finalize validation of delimited ID's.  Add static routine
**		IIUGdic_ChkdlmChar().
**	14-sep-1992 (rdrane)
**		Explicitly check for terminating delimiting quote in
**		IIUGdlm_ChkdlmBEobject() if is_nrml is FALSE.
**	15-sep-1992 (rdrane)
**		Explicitly fail otherwise delimited ID in IIUGdlm_ChkdlmBEobject()
**		if is_nrml is FALSE and it does not have delimiting quotes.  Correct
**		handling of overlapping strings.
**	1-oct-1992 (rdrane)
**		Add IIUGxri_id().
**	14-oct-1992 (rdrane)
**		Allow $, #, @ in delimited identifiers.
**	20-oct-1993 (rdrane)
**		Allow # and @ to be the initial character of a delimited identifier.
**	14-jan-1994 (rdrane)
**		Added single quote escape/de-escape routines.
**  	16-jun-95 (harpa06)
**              Changed UGlbo_lowerBEobject() so that if the object to be 
**		converted is a delimited id, don't do it.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
*/


/*
** Name:	IIUGdlm_ChkdlmBEobject()
**
** Description:
**	Accepts a string, determines if it is a delimited identifier, and
**	conditionally normalizes it to its BE representation.  String has the form
**              ["]name["]
**	Determination rules depend upon whether or not the name is specified as
**	already being normalized, i.e., retrieved from the database vs. from user
**	input (command line, language, forms prompt).
**
**	The string is examined for otherwise invalid INGRES name characters (e.g.,
**	", space, ., etc.).  If any are found, then the string is determined to be
**	a delimited identifier.  Note that this test will treat the presence of
**	quotes as indicating delimited regardless of origin (command line/database).
**	Note also that while the ANSI standard is more restrictive than INGRES,
**	any occurance of the characters #, $, @, or underscore will NOT by
**	themselves cause the string to be considered delimited.  However,
**	identifiers which begin with the character $ will continue to be
**  considered delimited.
**
**  The presence of SQL pattern matching characters (escaped or not) will
**	cause the string to be considered delimited.
**
**	Trailing blanks are always stripped, and do not automatically denote that
**	the string is a delimited identifier.  Note that they should never exist
**	in the database, and that if significant on user input, then the delimiting
**	quotes will be present anyway!
**
**	If the resultant string is lexically equivalent to an SQL reserved word
**	(after stripping blanks and regardless of case), then it is determined to
**	be a delimited identifier.
**
**	If is_nrml is TRUE and the string does not match case-wise, then it is
**	determined to be a delimited identifier.  Note that when the match indicates
**	that it could be either, we decide in favor of the regular identifier!
**	For the specific instances of INGRES (lower,lower) and FIPS (UPPER,Mixed)
**		Contains chars		INGRES		FIPS
**	   ----------------	   --------	   ------
**		all lower			regular		delimited
**		all UPPER			 -----		regular
**		Mixed				 -----		delimited
**
**	Thus, in the absence of special characters or keyword matches, all
**	names retrieved from INGRES databases will be considered regular
**	identifiers.
**
**	This routine diagnoses unbalanced delimiting and/or escaped quotes
**	and input/resultant strings greater than DB_MAX_DELIMID/DB_MAXNAME.
**  In these cases it returns indication that the identifier is invalid.
**
**	If is_nrml is FALSE, then unless the string is explicitly delimited,
**	any determinitation that the identifier is delimited results in its
**	being declared invalid.
**
**	When connected to a pre-6.5 database, all delimited identifiers are
**	considered to be invalid!
**
**
**	This routine will not recognize and/or decompose an owner.tablename
**	construct.  Use FE_decompose() and then call this routine for each part.
**
** Input:
**	name		Character pointer to NULL terminated string which is the
**				identifier to be checked and optionally resolved.
**	strip_name	Character pointer to a buffer of at least (DB_MAXNAME + 1)
**				or NULL.
**	is_nrml		If TRUE, then the name is in normalized form.  In that case,
**				any quotes found are expected to represent non-escaped embedded
**				quotes, and case matching is a determining factor.  If FALSE,
**				then a leading quote is a delimiter, embedded quotes are
**				expected to be escaped, and case is not a factor in determining
**				delimited/regular status.
**
** Output:
**	name		Any trailing whitespace will be trimmed from name.  This is
**				more of a side effect than a feature.
**	strip_name	Conditionally set to contain the resolved identifier.
**				If non-NULL on input, then the input string minus any
**				delimiting quotes, escaping quotes, trailing spaces, and
**				properly cased is placed there.  If the return status is
**				UI_BOGUS_ID, then this will reflect EOS.
**
** Returns:
**	i4			UI_REG_ID   - valid regular identifier
**				UI_DELIM_ID - valid delimited identifier
**				UI_BOGUS_ID - invalid identifier (includes NULL pointer and
**							  empty string cases).
**
** Side Effects:
**	IIUGdbo_dlmBEobject() is called if strip_name was specified as non_NULL,
**	and may have have potential side effects of its own, specifically
**	IIUIdbdata() may be called, and will set up the static DB capabilities
**	structure if it is not already set up.
**
** History:
**	26-jun-92 (rdrane)
**		Created.
**	16-jul-92 (rdrane)
**		STtrmwhite() can return an empty string if all whitespace - need to
**		detect this and force the result to be one space.  Conditionalize
**		SQL reserved word lookup - only do it if not already considered
**		delimited.  Handle NULL pointer/empty input string gracefully.
**	31-jul-1992 (rdrane)
**		Because of conflicts/inadequacies with FEchkname(), do full validation
**		of the identifier here, and change the return from a boolean
**		delimited/regular indicator to a i4  delimited/regular/bogus indicator.
**		Expand the local s_name buffer to handle a badly formed escaped
**		delimited identifier.  Use the input name directly.  Treat input string
**		as unsigned so we don't get differing results if 8-bit characters.
**	11-aug-1992 (rdrane)
**		Renamed in_quote to start-quote for readability.  Removed test for
**		is_nrml FALSE within test for start_quote since start_quote will never
**		be set to TRUE if is_nrml is specified as TRUE.  Underscore is valid
**		in regular identifier so long as it's not the last character.
**	18-aug-1992 (rdrane)
**		Because of backwards compatibility considerations, instances of
**		@,$,#, and underscore will NOT cause the identifier to be
**		considered delimited.  Note that the "Underscore is valid
**      in regular identifier so long as it's not the last character" is
**		Intermediate SQL, and we're only doing Entry SQL right now.
**		If we're connected to a pre-6.5 database, then any delimited
**		identifier is actually invalid.  Call the "existing" IIUGIsSQLKeyWord()
**		routine instead of the now defunct IIUGsrw_LookUp().
**	04-sep-1992 (rdrane)
**		Finalize validation of delimited ID's.  Add static routine
**		IIUGdic_ChkdlmChar().
**	14-sep-1992 (rdrane)
**		Explicitly check for terminating delimiting quote.  It wasn't
**		treating "abc as UI_BOGUS_ID if is_nrml is FALSE.
**	15-sep-1992 (rdrane)
**		Explicitly fail otherwise delimited ID in IIUGdlm_ChkdlmBEobject()
**		if is_nrml is FALSE and it does not have delimiting quotes.
**		Thus, abc" will now fail if is_nrml is FALSE.  Correct handling of
**		overlaping strings.  The clearing of strip_name reduced the input to
**		an empty string.  So, ignore clearing strip_name!
**	14-oct-1992 (rdrane)
**		Allow $, #, @ in delimited identifiers (the LRC has spoken!).
**	18-mar-1993 (rdrane)
**		Screen out totally bogus unnormalized delim identifiers - |"|, |""|.
**		These were being passed as "delimited, equivalent to one space".  Note
**		that the new logic also provides for immediate detection of invalid
**		unnormalized delim identifiers of the form |"x| (one character, no
**		terminating quote).  This fixes bug 50519.
**	 31-dec-1993 (rdrane)
**		Use dlm_Case capability directly to determine if delimited identifiers
**		are suppported.
**       22-sep-1995 Bug# 71533 (lawst01)
**              removed test of is_dlm switch from check of valid delimited
**              characters.
*/

i4
IIUGdlm_ChkdlmBEobject(name,strip_name,is_nrml)
		char	*name;
		char	*strip_name;
		bool	is_nrml;
{
	register char	*i_ptr;
	register char	*o_ptr;
			i4		w_lgth;
			bool	is_dlm;
			bool	start_quote;
			bool	no_delim_id;
			char	s_name[DB_MAX_DELIMID + 1];
			char	w_name[(DB_MAXNAME + 1)];

	
	i_ptr = name;
	o_ptr = &s_name[0];
	is_dlm = FALSE;
	start_quote = FALSE;
	if  (IIUIdlmcase() == UI_UNDEFCASE)
	{
		no_delim_id = TRUE;
	}
	else
	{
		no_delim_id = FALSE;
	}

	/*
	** Ensure we're dealing with an input string that we can handle, and
	** trash any trailing spaces.  Otherwise, we will detect them as
	** "special" and consider the identifier to be delimited.
	*/
	if  ((i_ptr == NULL) || (*i_ptr == EOS) ||
		 ((w_lgth = STtrmwhite(i_ptr)) > DB_MAX_DELIMID))
	{
		return(UI_BOGUS_ID);
	}

	/*
	** If the name is all whitespace, then we know it's a delimited identifier
	** (or bogus if is_nrml is FALSE or we're pre-6.5). Force it to be one
	** space (since we lost everything!).  Don't forget to fix-up the input
	** string!
	*/
	if  (w_lgth == 0)
	{
		STcopy(ERx(" "),i_ptr);
		if  ((!is_nrml) || (no_delim_id))
		{
			return(UI_BOGUS_ID);
		}
		if  (strip_name != NULL)
		{
			STcopy(i_ptr,strip_name);
		}
		return(UI_DELIM_ID);
	}

	/*
	** If name starts with a double quote, we know it's a delimited identifier.
	** If it's not normalized, then the last character must be one too.
	*/
	if  (*i_ptr == '"')
	{
		if  (!is_nrml)
		{
			/*
			** Guard against the degenerate cases of:
			**		"
			**		""
			**		un-terminated unnormalized ID, e.g. "x
			*/
			if  ((w_lgth = STlength(i_ptr)) <= 2)
			{
				return(UI_BOGUS_ID);
			}
			if  (*(i_ptr + (w_lgth - 1)) != '"')
			{
				return(UI_BOGUS_ID);
			}
			start_quote = TRUE;	/* Start us on our way ...	*/
			CMnext(i_ptr);
		}
		is_dlm = TRUE;
	}
	/*
	** Ditto if the name starts with an otherwise invalid character.  Here,
	** though, we keep the character.
	*/
	else if (!CMnmstart(i_ptr))
	{
		/*
		** Just because it's invalid for INGRES doesn't mean it's valid
		** for delimited!
		*/
		if (!IIUGdic_ChkdlmChar(i_ptr))
		{
			return(UI_BOGUS_ID);
		}
		CMcpyinc(i_ptr,o_ptr);	/* Add the char to the current buffer.	*/
		is_dlm = TRUE;
	}
	else
	{
		/*
		** Underscore is a valid INGRES 1st chatracter, but ANSI says its
		** presence indicates a delimited identifier.  However, for backwards
		** compatibility, we'll let it go ...
		*/
		CMcpyinc(i_ptr,o_ptr);	/* Add the char to the current buffer.	*/
	}

	while (*i_ptr != EOS)
	{
		/*
		** If not "regular" character, then it's a delimited identifier.
		** Underscore, pound sign, at sign, and dollar sign are all valid
		** INGRES non-1st chatracters, but ANSI says their presence indicates
		** a delimited identifier.  Note that for underscore, this is only
		** TRUE if it's the last character in the identifier.  However, for
		** backwards compatibility, we'll let them go ...
		*/
		if  (!CMnmchar(i_ptr))
		{
			/*
			** Just because it's invalid for INGRES doesn't mean it's valid
			** for delimited!
			*/
/*
** Bug 71533 (lawst01)
**  removed test || !is_dlm from if statement 
**
*/
			if (!IIUGdic_ChkdlmChar(i_ptr))
			{
				return(UI_BOGUS_ID);
			}
			switch(*i_ptr)
			{
			/*
			** If we started with a delimiting quote, then we need to look for
			** escaped embedded quotes and only save one.  If we didn't start
			** with a delimiting quote, then we need to save all quotes since
			** they're not escaped.  If we see an escaped/terminating quote
			** that is badly formed, consider the identifier to be bogus.
			*/
			case '"':
				if  (start_quote)	/* Skip any escaped/terminating quote.	*/
				{
					CMnext(i_ptr);
					if  ((*i_ptr != '"') && (*i_ptr != EOS))
					{
						return(UI_BOGUS_ID);
					}
				}
				is_dlm = TRUE;
				if  (*i_ptr != EOS)	/* Preserve any EOS!	*/
				{
					CMcpyinc(i_ptr,o_ptr);
				}
				break;
			default:
				is_dlm = TRUE;
									/* Add the char to the current buffer.	*/
				CMcpyinc(i_ptr,o_ptr);
				break;
			}
		}
		else
		{
			CMcpyinc(i_ptr,o_ptr);	/* Add the char to the current buffer.	*/
		}
	}

	/*
	** Ensure stripped string NULL terminated!  Strip any trailing blanks
	** which were inside delimiting quotes!  Since we need to verify final
	** lengths, do it regardless of regular/delimited since STtrmwhite()
	** returns the string length as well as STlength().  A zero length implies
	** an explicitly quoted delimited identifier which was all whitespace,
	** and so is_dlm should already be set to TRUE.
	*/
	*o_ptr = EOS;
	w_lgth = STtrmwhite(&s_name[0]);
	if  (w_lgth > DB_MAXNAME)
	{
		return(UI_BOGUS_ID);
	}
	if  (w_lgth == 0)				/* All whitespace?	*/
	{
		STcopy(ERx(" "),&s_name[0]);
	}

	/*
	** Here we need to call a routine to check for SQL keywords, unless we
	** already know that we're delimited.
	*/
	if  ((!is_dlm) && (IIUGIsSQLKeyWord(&s_name[0])))
	{
		is_dlm = TRUE;
	}

	/*
	** If still not sure and name is from the database, do the case matching
	** tests.  Note that we have biased the test in favor of regular
	** identifiers.  What we're really doing is assuming that it is regular,
	** effecting the case conversion, and checking for a no-op.
	*/
	if  ((!is_dlm) && (is_nrml))
	{
		STcopy(&s_name[0],&w_name[0]);
		IIUGdbo_dlmBEobject(&w_name[0],FALSE);
		if  (!STequal(&s_name[0],&w_name[0]))
		{
			is_dlm = TRUE;
		}
	}

	/*
	** Screen invalid delimited ID's.  Not allowed if pre-6.5 connection,
	** if we had an invalid "funny" character, or if it is not normalized
	** and not explicitly delimited.  Note that an unbalanced trailing
	** quote, e.g., |abc"|, will manifest itself as is_dlm and so fail if
	** is_nrml == FALSE.
	*/
	if  ((is_dlm) && ((no_delim_id) || ((!is_nrml) && (!start_quote))))
	{
		return(UI_BOGUS_ID);
	}

	if  (strip_name != NULL)
	{
		IIUGdbo_dlmBEobject(&s_name[0],is_dlm);
		STcopy(&s_name[0],strip_name);
	}

	if  (is_dlm)
	{
		return(UI_DELIM_ID);
	}
	else
	{
		return(UI_REG_ID);
	}
}

/*
** Name:	IIUGdic_ChkdlmChar()
**
** Description:
**	Accepts a character and determines if it is valid for inclusion in
**	a delimited identifier.  Note that this routine assumes that the character
**	has already failed to pass CMnmstart()/CMnmchar().
**
** Input:
**	chk_char	Pointer to character to be tested for inclusion.
**
** Output:
**	Nothing.
**
** Returns:
**	TRUE		Character valid within a delimited identifier.
**	FALSE		Character invalid within a delimited identifier.
**
** History:
**	04-sep-1992 (rdrane)
**		Created.
**	20-oct-1993 (rdrane)
**		Allow # and @ to be the initial character of a delimited identifier.
*/

static
bool
IIUGdic_ChkdlmChar(chk_char)
		char	*chk_char;
{


	/*
	** If it's not printable, then it can't possibly be valid.
	*/
	if  (!CMprint(chk_char))
	{
		return(FALSE);
	}

	/*
	** Specifically exclude the known invalid characters.  If
	** we failed on these, then we're totally bogus.  Note that
	** we should only see #, $, or @ if we're called because of
	** a failed CMnmstart().
	*/
	switch(*chk_char)
	{
		case '!':
		case '$':
		case '\\':
		case '^':
		case 0x60:			/* leftquote	*/
		case '{':
		case '}':
		case '~':			/* Tilde		*/
		case 0x7F:			/* DEL			*/
			return(FALSE);
			break;
		default:
			break;
	}

	return(TRUE);
}



/*
**	IIUGdbo_dlmBEobject() - Adjust Case	of DBMS object Name
**
**	This routine is a replacement of IIUGlbo_lowerBEobject(), and behaves
**	similarly.  See the history and description for IIUGlbo_lowerBEobject()
**	which has been retained below.
**
**	This routines assumes that the client knows whether or not the object
**	in question is a delimited identifier, and has set the boolean is_dlm
**	appropriately.  The use of a boolean is predicated upon the assumption that
**	client routines will maintain the object name in a structure which also
**	has a boolean indicator.  Thus, the client can execute code of the
**	form
**
**			IIUGdbo_dlmBEobject(s_ptr->obj_name,s_ptr->is_dlm);
**
**	instead of
**
**			if  (delimited)
**				IIUGdbo_dlmBEobject(s_ptr->obj_name);
**			else
**				IIUGlbo_lowerBEobject(s_ptr->obj_name);
**
** Input:
**	obj_name	Character pointer to NULL terminated string which is the DBMS
**				object name whose case is to be adjusted.
**
**	is_dlm		A boolean which indicates if the DBMS object name represents a
**				delimited identifier (TRUE) or a regular identified (FALSE).
**
** Output:
**	obj_name	The string is uppercased, lowercased, or unchanged, depending
**				upon the rules for DBMS object name casing for the type of
**				identifier as reflected by the iidbcapabilities catalog.
**
**
**
**	CAVEAT:
**		Do NOT pass this routine constructs of the form
**
**				owner."tablename" -or- "owner".tablename
**
**		This routine will NOT decompose the construct, and so will return
**		both portions cased as per the value of is_dlm.
**
**		Do NOT call this routine if delimited identifiers are not
**		supported - the routine is then an effective no-op.
*/

VOID
IIUGdbo_dlmBEobject(obj_name,is_dlm)
		char	*obj_name;
		bool	is_dlm;
{
		i4		name_case;


	if  ((obj_name == NULL) || (*obj_name == EOS))
	{
		return;
	}

	if (is_dlm)
	{
		name_case = IIUIdlmcase();
	}
	else
	{
		name_case = IIUIcase();
	}

	switch(name_case)
	{
	case UI_LOWERCASE:
		CVlower(obj_name);
		break;
	case UI_UPPERCASE:
		CVupper(obj_name);
		break;
	default:
		break;
	}

	return;
}


/*
**	IIUGlbo_lowerBEobject()	 - Adjust Case of DBMS object name	
**
** Description:  This routine takes as input the address of a NULL
**	terminated string, which it lowers if the DB_OBJECT_CASE
**	tuple in the iidbcapablities catalog has the value "LOWER",
**	is uppercased if it has the value "UPPER", or leaves alone
**	if it is set to "MIXED_CASE".  This routine should always
**	be called after reading a user entered DBMS object name,
**	if that name will be compared with an object name as present
**	in the standard catalogs. 
**	e.g. -
*		char  table_name[DB_MAXNAME];
**	
**		IIUFask("enter table name", TRUE, table_name, 0);
**		IIUGlbo_lowerBEobject(table_name);
**		if (FErelexists(table_name))
**		{
**			...stuff...
**		}
**
**	Note #1: DBMS object name refers to table/view/index names,
**	as well as column names.  It does not refer to database names.
**
**	Note #2: Historically this
**	routine lowered a DBMS object name only against an INGRES
**	DBMS, hence the "lower" in the routine name.
**		  
** Input:
**	obj_name	Address of string to be lowered.
** Output:
**	obj_name	Object name that is uppercased, lowercased, or
**			left alone, depending on the DB_OBJECT_NAME entry
**			in the iidbcapabilities catalog.
**
** Returns:
**	nothing
**
** History:
**	05-nov-1987 (danielt)	Written.
**	18-jul-1988 (danielt)  Changed to relect the new DB_OBJECT_NAME
**	 	entry in the iidbcapabilities catalog.
**  05-jun-92 (rdrane)
**		Change this routine to simply call the new casing routine as
**			IIUGdbo_dlmBEobject(obj_name,FALSE);
**  16-jun-95 (harpa06)
**		If the object is a delimited id, don't do any conversion of it.
*/

VOID
IIUGlbo_lowerBEobject(obj_name)
		char	*obj_name;
{
	if ( IIUGdlm_ChkdlmBEobject(obj_name,NULL,TRUE) !=UI_DELIM_ID)
		IIUGdbo_dlmBEobject(obj_name,FALSE);

	return;
}



/*
** Name:	IIUGrqd_Requote_dlm()
**
** Description:
**	Re-quotes a delimited identifier and re-escapes any embedded double quotes.
**	The string is not expected to have extraneous trailing blanks, is not
**	expected to exceed DB_MAXNAME in length, and is	expected to be NULL
**	terminated.
**
**	It would be nice to migrate this routine to ST and have it
**	become part of the CL.
**
** Input:
**	in_str		Character pointer to the NULL terminated string to be
**				quoted.  Assumed to reside in a buffer of max length
**				(DB_MAXNAME + 1).
**	out_str		Character pointer to the buffer in which to place the
**				result string, which will be delimited and have double
**				quote characters escaped.  Assumed to go into a buffer with
**				a minimum of (DB_MAX_DELIMID + 1) available length.
**
** Output:
**	out_str		The re-quoted string representation of the input.  A NULL
**				or empty input string will be reflected as "".
**
** Returns:
**	Nothing.
**
** Side Effects:
**	None.
**
** History:
**	26-jun-92 (rdrane)
**		Created
**	16-jul-92 (rdrane)
**		Handle NULL pointers/empty input string gracefully.
*/

VOID
IIUGrqd_Requote_dlm(in_str,out_str)
		char	*in_str;
		char	*out_str;
{
	register char	*i_ptr;
	register char	*o_ptr;

	
	if  (out_str == NULL)		/* So be paranoid!	*/
	{
		return;
	}
	
	o_ptr = out_str;
	*o_ptr++ = '"';				/* Begin the string with a quote.	*/
	if  (in_str == NULL)		
	{
		*o_ptr++ = '"';
		*o_ptr++ = EOS;
		return;
	}
		
	i_ptr = in_str;
	while (*i_ptr != EOS)
	{
		if  (*i_ptr == '"')		/* Double all existing quotes.		*/
		{
			*o_ptr++ = '"';
			*o_ptr++ = '"';
			i_ptr++;
		}
		else
		{
			CMcpyinc(i_ptr,o_ptr);
		}
	}
	*o_ptr++ = '"';		/* End the string with a quote.		*/
	*o_ptr = EOS;		/* Ensure result NULL terminated.	*/

    return;
}



/*
** Name:	IIUGxri_id()
**
** Description:
**	Un-normalizes an identifier for display in a user-modifiable or
**	inclusion in generated procedural language output senario.
**	If id is determined to be a regular identifier, then it is simply
**	copied.  If id is determined to be a delimited identifier, then it
**	un-normalizes the identifier via IIUGrqd_Requote_dlm().
**
**	This routine is essentially a cover for the sequence
**		IIUGdlm_ChkdlmBEobject() (both components)
**		STcopy()/IIUGrqd_Requote_dlm()
**	and is intended for clients which need to externalize identifiers
**	already in normalized form (e.g., from a database) and which
**	have not already determined and/or saved the identifier's type.
**
**	If the client knows the identifier type, then it is much more
**	efficient to call STcopy()/IIUGrqd_Requote_dlm() directly and
**	avoid the non-trivial overhead of re-determination.
**
** Input:
**	id			Pointer to the identifier to be un-normalized.
**	result_id	Pointer to a field in which to place the un-normalized
**				identifier.  This should be at least (DB_MAX_DELIMID + 1)
**				bytes in length.
** Output:
**	result_id	Set to the un-normalized identifier or EOS if id
**				is UI_BOGUS_ID and not the catalog owner
**				(UI_FE_CAT_ID_65).
**
** Returns:
**	bool		TRUE/FALSE depending upon the return status of
**				IIUGdlm_ChkdlmBEobject().  FALSE is set only
**				for UI_BOGUS_ID where the id is NOT the catalog
**				owner (UI_FE_CAT_ID_65).
**
** History:
**	1-oct-1992 (rdrane)
**		Created.
**	29-apr-1993 (rdrane)
**		Allow the catowner (nominally $ingres) as a valid ID, even though
**		IIUGdlm_ChkdlmBEobject() will declare it to be UI_BOGUS_ID.
*/


bool
IIUGxri_id(char *id,char *result_id)
{


	if  (result_id == NULL)
	{
		return(FALSE);
	}

	switch(IIUGdlm_ChkdlmBEobject(id,NULL,TRUE))
	{
	case UI_REG_ID:
		STcopy(id,result_id);
		break;
	case UI_DELIM_ID:
		IIUGrqd_Requote_dlm(id,result_id);
		break;
	default:
		if  (STbcompare(id,0,UI_FE_CAT_ID_65,0,TRUE) == 0)
		{
			STcopy(id,result_id);
		}
		else
		{
			*result_id = EOS;
			return(FALSE);
		}
		break;
	}

	return(TRUE);
}

/*
** Name:    IIUIea_escapos()
**
** Description:
**	Copies source to dest, preceding single quotes (') with another single
**	quote, as per SQL escapement rules for single quote delimited strings.
**
** History:
**    14-jan-1994 (rdrane)
**		Created from REPORT/rfbstcpy.c to address a range of bugs
**		regarding delimited identifiers containing embedded single quotes.
*/

static	char	*sq_char = ERx("\'");

VOID
IIUIea_escapos(char *src, char *dest)
{


	while (*src != EOS)
	{
		if  (CMcmpnocase(src,sq_char) == 0)
		{
			CMcpychar(sq_char,dest);
			CMnext(dest);
		}
		CMcpyinc(src,dest);
	}
	*dest = EOS;

	return;
}

/*
** Name:    IIUIuea_unescapos()
**
** Description:
**	Copies src to dest, de-escaping single quotes (').
**
** History:
**    14-jan-1994 (rdrane)
**		Created from REPORT/rfbstcpy.c to address a range of bugs
**		regarding delimited identifiers containing embedded single quotes.
*/

VOID
IIUIuea_unescapos(char *src, char *dest)
{


	while (*src != EOS)
	{
		/*
		** If we see a single quote, skip it and check the
		** next character.
		** NOT a single quote:
		**	Put one in to "replace" the one we just
		**	skipped, then continue so that the non-single quote
		**	character is placed in the dest string.
		** IS a single quote:
		**	Continue so that the single quote is placed in the dest
		**	string - we already skipped its "escapement".
		** If we don't see a single quote, just place the character
		** in the dest string.
		*/
		if  (CMcmpnocase(src,sq_char) == 0)
		{
			CMnext(src);
			if  (CMcmpnocase(src,sq_char) != 0)
			{
				CMcpychar(sq_char,dest);
				CMnext(dest);
			}
		}
		CMcpyinc(src,dest);
	}
	*dest = EOS;

	return;
}

