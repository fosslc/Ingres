/*
**	Copyright (c) 2004 Ingres Corporation
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
**   IIUGgci_getcid -
**		 Get an identifier string from the internal line.  An
**		 identifier may be delimited or regular, but is always
**		 considered NOT to be in normalized form.  Thus, if it
**		 starts with a double quote, then it must end with a
**		 double quote.
**
**		 Arbitrary white space is allowed following the compound
**		 construct delimiter, but leading white space, e.g.,
**			["]a["] .["]b["]
**		 and comments, e.g.,
**			["]a["] << This is a Comment >> .["]b["]
**		 are not supported.
**
**		 This routine differs from UI/FE_decompose() in that
**		 o Identifiers are unconditionaly treated as being unnormalized.
**		 o It does not utilize the FE_RSLV_NAME structure.
**		 o It does not assume that the identifier is the only
**		   token contained in the input buffer, and so can be
**		   incorporated into client parsers.
**		 o It conditionally recognizes/allows compound constructs.
**
**		 A regular identifier is considered to be terminated by
**		 any character which fails CMnmchar() unless that character
**		 is a period and compound identifiers are valid in this context.
**
**		 A delimited identifier is terminated by whatever character
**		 follows a non-doubled double quote which is not the leading
**		 delimiting double quote.  However, in order to support
**		 constructs of the form ["]a["].["]b["] where a is a correlation
**		 name/owner and b is a columnname/tablename/procedure, look
**		 ahead for a delimiting period and continue if found and
**		 compound constructs are allowed in this context.
**
**		 An identifier (or component thereof) which exceeds the size
**		 of the ident buffer will be silently truncated.
**
** Input:
**	buf	Pointer to the line buffer containing the identifier.
**		The identifier is assumed to start at the beginning of the
**		buffer, but not necessarily to end at the buffer's EOS.
**		If NULL, then this routine becomes an effective no-op.
**	ugid	Pointer to a FE_GCI_IDENT structure.  This should never
**		be specified as NULL if buf is non-NULL.
**	c_ok	TRUE if a compound identifier (["]a["].["]b["]) is
**		allowable in this context.
**
** Output:
**	ugid	Structure filled as follows (even if buf was specified as NULL):
**			name -	 the full text of the identifier string,
**				 unaltered (but perhaps truncated) and NULL
**				 terminated.  Forced to EOS if buf was
**				 specified as NULL.
**			c_oset - offset in name of the compound delimiting
**				 period.  If zero, then the identifier is not
**				 a compound identifier.
**			b_oset - offset in the input buffer where the parse
**				 stopped. This indicates which character
**				 the client should use to resume the parse.
**				 Forced to 0 if buf was specified as NULL.
**			owner -	 the owner/correlationname component, also NULL
**				 terminated.  This is simply separated from
**				 the compound identifier - it is not normalized
**				 in any way.  This reflect EOS if the identifier
**				 is not a compound identifier.
**			object - the object (columnname/tablename/procedure)
**				 component, also NULL terminated.  This is
**				 simply separated from the compound identifier
**				 - it is not normalized in any way.  Forced to
**				 EOS if buf was specified as NULL.
** Returns:
**	TRUE -	buf was specified as NULL
**		-OR-
**		The parse succeeded. This does not guarantee that the resultant
**		identifier and/or its components will pass
**		IIUGdlm_ChkdlmBEobject(), or that unbalanced quotes did
**		not "prematurely" terminate the scan.
**	FALSE -	ugid was specified as NULL and buf was specified as non-NULL
**		-OR -
**		the parse results represent known invalid identifier.
**		Specifically, the identifier construct was of the form:
**			["]owner["].
**			.["]object["]
**			[["]owner["].]""
**			""[.["]object["]]
**		all whitespace, or had an odd number of quotes
**		(not recognized as an error in all cases ...).
**		Note that even under these FAIL conditions, the
**		resultant FE_GCI_IDENT will be filled in an intuitive
**		manner, e.g.,
**			name ==> .["]object["]
**			c_oset ==> 0
**			b_oset ==> 7 (9 if delimited)
**			e_code ==> FE_GCI_BARE_CID
**			owner ==> EOS
**			object ==> ["]object["]
**
** History:
**	14-apr-1993 (rdrane)
**		Created (inspired by the original SREPORT/sgident.c).
**		The quirks of this routine can be traced to the requirements
**		of its initial clients - Report-Writer and 3GL decomposition
**		of owner.procedure constructs.
**	11-jun-1993 (rdrane)
**		Allow for NULL buf and/or ugid.  The former can allegedly occur
**		as a result of:
**			## delete cursor c1;
**
**		Since there's no table name, EQUEL will generate something
**		like:
**
**			IIcsDelete((char *)0,"c1",(int)22616,(int)8082);
**
**		where the first parameter is supposed to be table name and is
**		NULL.  IIcsDelete() calls IIUGgci_getcid() with this NULL
**		pointer, and IIUGgci_getcid() fails with an access violation.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



bool
IIUGgci_getcid(buf,ugid,c_ok)
char		*buf;
FE_GCI_IDENT	*ugid;
bool		c_ok;
{
	char	*start_buf_ptr;
	char	*tk_ptr;
	char	*end_tk_ptr;
	char	*compound_delim;
	i4	quote_cnt;
	i4	id_lgth;
	bool	init_quote;


	if  (buf == NULL)
	{
		if  (ugid != (FE_GCI_IDENT *)NULL)
		{
			ugid->name[0] =EOS;
			ugid->c_oset = 0;
			ugid->b_oset = 0;
			ugid->e_code = FE_GCI_NO_ERROR;
			ugid->owner[0] = EOS;
			ugid->object[0] = EOS;
		}
		return(TRUE);
	}

	if  (ugid == (FE_GCI_IDENT *)NULL)
	{
		return(FALSE);
	}

	quote_cnt = 0;
	init_quote = FALSE;
	compound_delim = NULL;
	start_buf_ptr = buf;
	tk_ptr = &ugid->name[0];
	*tk_ptr = EOS;
	end_tk_ptr = &ugid->name[0] + (sizeof(ugid->name) - 1) - 1;

	while (*buf != EOS)
	{
		if  (tk_ptr >= end_tk_ptr)
		{
			CMnext(buf);
			continue;
		}
		if  (*buf == '\"')
		{
			quote_cnt++;
			CMcpyinc(buf,tk_ptr);
			if  (!init_quote)
			{
				init_quote = TRUE;
				continue;
			}
			if  (*buf == EOS)
			{
				break;
			}
			/*
			** A non-initial quote followed by other than another
			** quote is end-of-identifier, since all embedded quotes
			** must be escaped.  However, we must support constructs
			** of the form ["]a["].["]b["] where a is a correlation
			** name/owner and b is a columnname/tablename/procname.
			** So, if compound constructs are valid in this
			** context, look ahead for a delimiting period and
			** continue if found immediately.
			*/
			if  (*buf != '\"')
			{
				if  (!c_ok)
				{
					break;
				}
				/*
				** Any compound delimiting period must be the
				** next immediate character.  Otherwise, we can
				** become confused by multiple RW commands on a
				** line, e.g.,
				** 	... FROM "tbl" .SORT ...
				** (Yes, Virginia, Report-Writer uses this
				** routine too)
				*/
				if  (*buf != '.')
				{
					break;
				}
				compound_delim = tk_ptr;
				init_quote = FALSE;
				CMcpyinc(buf,tk_ptr);
				while (CMwhite(buf))
				{
					CMnext(buf);
				}
			}
			else
			{
				quote_cnt++;
				CMcpyinc(buf,tk_ptr);
			}
			continue;
		}
		if  (init_quote)
		{
			/*
			** We're in a delimited identifier.  Accept anything,
			*/
			CMcpyinc(buf,tk_ptr);
			continue;
		}
		/*
		** We're in a regular identifier.  Any non-name character
		** is a delimiter, except period which is conditionally a
		** delimiter.
		*/
		if  (!CMnmchar(buf))
		{
			if  (!c_ok)
			{
				break;
			}
			/*
			** Any compound delimiting period must be the next
			** immediate character.  Otherwise, we can become
			** confused by multiple RW commands on a line, e.g.,
			**	... FROM tbl .SORT ...
			** (Yes, Virginia, Report-Writer uses this routine too)
			*/
			if  (*buf != '.')
			{
				break;
			}
			compound_delim = tk_ptr;
			CMcpyinc(buf,tk_ptr);
			while (CMwhite(buf))
			{
				CMnext(buf);
			}
			continue;
		}
		CMcpyinc(buf,tk_ptr);
	}

	*tk_ptr = EOS;

	/*
	** Establish offset in input buffer where the
	** parse ended.  This is the character with
	** which to resume the parse, and not the last
	** character in name.
	*/
	ugid->b_oset = buf - start_buf_ptr;

	/*
	** Break up the identifier as required, and don't bother to
	** normalize or validate it.  Note that we can't have a compound
	** construct unless c_ok == TRUE.
	*/
	if  (compound_delim != NULL)
	{
		*compound_delim = EOS;
		STcopy(&ugid->name[0],&ugid->owner[0]);
		*compound_delim = '.';
		STcopy((compound_delim + 1),&ugid->object[0]);
		ugid->c_oset = compound_delim - &ugid->name[0];
	    
	}
	else
	{
		ugid->owner[0] = EOS;
		STcopy(&ugid->name[0],&ugid->object[0]);
		ugid->c_oset = 0;
	}

	/*
	** Screen out no identifier (all whitespace), compound delimiter
	** as first or last character, delimited identifier component(s)
	** which are empty, and odd numbers of quotes.  Note that compound
	** delimiter as last character should occur only if the parse hits
	** end-of-line, since white space after a compound delimiter is
	** compressed/ignored.
	*/
	if  (ugid->name[0] == EOS)
	{
		ugid->e_code = FE_GCI_EMPTY;
		return(FALSE);
	}

	if  ((compound_delim != NULL) &&
	     ((ugid->c_oset == 0) ||
	      (ugid->name[(ugid->b_oset - 1)] == '.')))
	{
		ugid->e_code = FE_GCI_BARE_CID;
		return(FALSE);
	}

	if  ((ugid->owner[0] == '\"') &&
	     (((id_lgth = STlength(&ugid->owner[0])) <= 2) ||
	      (ugid->owner[(id_lgth - 1)] != '\"')))
	{
		ugid->e_code = FE_GCI_DEGEN_DID;
		return(FALSE);
	}

	if  ((ugid->object[0] == '\"') &&
	     (((id_lgth = STlength(&ugid->object[0])) <= 2) ||
	      (ugid->object[(id_lgth - 1)] != '\"')))
	{
		ugid->e_code = FE_GCI_DEGEN_DID;
		return(FALSE);
	}

	/*
	** Unbalanced quotes are not recognized in all cases - they often
	** simply "prematurely" terminate the scan, e.g.,
	**	"a"b"	==>	"a"		NO ERROR
	**	"a"""b"	==>	"a"""		NO ERROR
	** Similarly,
	**	"	==>	"
	**	""a	==>	""
	** will recognize FE_GCI_DEGEN_DID, not FE_GCI_UNBAL_Q.
	** This behavior is due in part to Report-Writer requirements
	** to provide backwards compatibility with double-quoted
	** string constants, e.g.,
	**	.PRINT "abc""def"
	** is the equivalent of
	**	.PRINT "abc", "def"
	*/
	if  ((quote_cnt % 2) != 0)
	{
		ugid->e_code = FE_GCI_UNBAL_Q;
		return(FALSE);
	}

	ugid->e_code = FE_GCI_NO_ERROR;
	return(TRUE);
}

