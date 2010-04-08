/*
** Copyright (c) 2005 Ingres Corporation
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<adf.h>
# include	<afe.h>
# include	<fmt.h>
# include	<cm.h>
# include	"format.h"

/*
**   F_COLFMT - return one column of a string, formatted by justification
**	or folding if requested.  This routine should be called by a
**	higher level routine in a loop to return lines which are to printed
**	in column format.
**
**	This routine defines the end of a word as a blank, tab, or newline.
**	Note that newlines return immediately.	Tabs round to the nearest
**	column a multiple of eight (relative to the start).  If more than
**	one blank in a row is found, the second (and subsequent blanks)
**	will always be retained.  This routine always pads with blanks
**	to the end of BUF.
**
**	Parameters:
**		string	workspace containing the string to format.
**		buf	buffer to put formatted string into.
**		width	the width of one column.
**		type	type of formatting.  Values are:
**			F_C	simply transfer to the string, and
**				truncate when "width" characters transferred,
**				whether in the middle of a word or not.
**			F_CF	fold the string, that is break at the end
**				of a word (if possible).
**			F_CFE	same as F_CF but trailing spaces are preserved 
**				rather than compressed.
**			F_CJ	justify the string, that is fold and then
**				pad with blanks at word boundaries to right
**				justify the string.
**			F_CJE	same as F_CJ but preserve trailing spaces.
**			F_CU	fold the string, break at quoted strings
**				as well as words (internal format used by
**				unloaddb()).
**			F_T	same as F_C, except represent control characters
**				as \c or \ddd.
**		mark_byte1 	TRUE to insert a special marker when splitting a 
**				2-byte char.  FALSE to insert a space.  It is
**				really only applicable to multi line formatting.
**	Returns:
**		OK
**
**	Called by:
**		User.
**
**	Side Effects:
**		Updates buf.
**
**	Error Messages:
**		none.
**
**	History:
**		7/25/81 (ps)	written.
**		1/12/85 (rlm)	modified for cfe, keeping data for
**				adjustment as search is done rather than
**				rescanning string, and making use of some
**				CL calls
**		2/12/86 (drh)	modified to not add a hyphen when splitting a
**				word.
**		10/20/86 (KY)  -- Changed CH.h to CM.h.
**	12-dec-86 (grant)	changed to handle text strings instead of
**				null-terminated C strings.
**				added F_T format.
**	28-aug-87 (rdesmond)	enhanced to wrap on '(', ')' and ',' as well 
**				as white space.  Also now avoids wrapping 
**				within strings if possible.
**	10/28/87 (dkh) - Changed to leave control chars alone, unless
**			 it is a \t, \r or \n.
**	20-apr-89 (cmr)		Modification to rdesmond change of 28-aug-87.
**				Break on quoted strings *only* for the
**				format F_CU (used internally by unloaddb()).
**	25-jul-89 (cmr)		Fix for bug 6398 -- do not treat delimiters 
**				like whitespace.  When we reach the point of 
**				folding (wrapping) do NOT set 'fold' to TRUE 
**				if the next src char is a delimiter otherwise 
**				that character gets dropped!
**	26-jul-89 (cmr)		Fix for bug 5069 -- if the last byte in the 
**				formatted buf is the first byte of a 2-byte 
**				char then "fix" the byte and set fmws_text 
**				so that the entire char gets read the next
**				time through.  If mark_byte1 is TRUE then 
**				insert a special marker else insert a space.  
**				Only applicable to multi line format.
**	28-jul-89 (cmr)		Add support for new formats 'cfe' and 'cje'.  
**				Preserve trailing spaces rather than 
**				compressing them.
**	18-Aug-89 (kathryn)	Addition to bug fix 5069.
**		Ensure strptr is on the last byte of the formatted buffer.
**      21-feb-90 (stevet)      Fixed Kanji blank (Bug #9466) problem as well
**                              as a number of display problems with Kanji
**                              characters when using the 'cf' and 'cj'
**                              formats.  Convert Kanji blank to two spaces when
**                              using formats other than F_C and F_T formats.
**	27-mar-90 (cmr)		Fixed jupbug #20203.  Ignore trailing blanks 
**				for format CJ.  This bug was introduced when 
**				the new format CJE was added.
**	15-apr-90 (cmr)		Rewrote the code that handles folding on quoted
**				strings.  It now handles single quotes as well
**				as double quotes and it can handle a quoted 
**				string that spans lines or is unterminated.
**	17-apr-90 (cmr)		Fix for b9835.  Don't fold on open paren '('.
**	15-feb-91 (mgw)		Make F_inquote a i4  to match GLOBALREF in
**				fmmulti.c and use F_INQ and F_NOQ for TRUE and
**				FALSE in quote tests.
**	3/21/91 (elein)		(b35574) Add parameter for FRS, boxed messages
**				which need to change control characters to
**				spaces in order to display boxed messages
**				correctly.  For internal use only.
**	10/16/92 (dkh) - Fixed f_colfmt() so that it does not format beyond
**			 the end of the user's buffer.
**	10/19/92 (dkh) - Free up old memory area before allocating a new one.
**	11/04/92 (dkh) - Fixed bug 47702.  Justification format changed the
**			 original "width" value that was passed in.  This caused
**			 an incorrect amount of data to be passed back.  We
**			 now save off the width value and use that when moving
**			 data.
**	11/06/92 (fredv) - Replace the STlcopy() calls with MEcopy()'s
**			because STlcopy will write the extra null byte
**			to terminate the string and write beyond the
**			user buffer.
**      04/03/93 (smc) - Cast parameter of MEfree to match prototype.
**	07/10/93 (dkh) - Fixed bug 41631.  For the CU format, hex literal
**			 strings (x'NN' or X'NN') will now be kept
**			 together when folding strings.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**      11/30/94 (harpa06) - Integrated bug fix #63235 from 6.4 codeline by
**                       abowler: handle delimiters when justifying strings.
**	07/04/95 (kch) - Fix bug 53327 - remove repeated output when
**			 the T format is used to print a varchar by
**                       presetting the user's buffer to blanks.
**  02/05/95 (kch) - Fix bug 64312 - add leading blanks when the F_CU
**           format is used by unloaddb in the creation of the
**           cp[username].in files.
**           This compensates for the removal of blanks when
**           the procedure text (column text_segment in the
**           database catalogue iiprocedures) spans multiple
**           lines in the cp[username].in files (when
**           text_segment is greater than XF_LINELEN (70)
**           characters).
**    23/05/95 (kch) - Fix bug 52238 - check that a second (escaped)
**             single quotation mark in the procedure text is not
**             the XF_LINELENth (70th) character (the previous
**             change added a leading blank so the bug now occurs
**             if the second single quote is the 70th and not the
**             71st character).
**  26/07/95 (kch) - Fix bug 70127 - this bug is a side effect of the
**           fix for bug 64312. The incrementing of the counter i,
**           to compensate for the addition of a leading blank,
**           is lost when a tab is processed. Therefore, the
**           counter must be decremented so that the tabbing
**           is correct and then 're-incremented'.
**  30-nov-1995 (angusm)
**          bug 71653, DOUBLEBYTE only. We can reposition
**          too early after a fold, and get ourselves
**          into an infinite loop.
**  12-feb-1996 (kch)
**	    Tidy up previous tab processing change (for bug 70127, which was
**	    a side effect of the fix for bug 64312). We now re-increment the
**	    counter i after c and cprev have been assigned. Previously, an
**	    extra blank was being incorrectly added by the c = bufstart + i
**	    assignment. This change fixes bug 74264.
**  01-apr-1996 (tsale01)
**		Further to fix by angusm for bug 71653, make sure the folding
**		is not done for unloaddb or copydb as we definitely want to
**		fold at delimiters or spaces only.
**		Original fix can split names arbitrarily into two chunks.
**		The two chunks are then read in as two separate names!
**  09-jul-1996 (kch)
**		Check that i is less than width before re-incrementing after
**		tab processing.
**  14-Feb-1997 (hanal04)
**          Due to the fix for bug# 64312 and the introduction of single quotes
**          (re: 15-apr-1990) in some instance an additional white space is
**          inserted to character strings. See scripts for Bug# 80739. Changed
**          function f_colfmt to treat double quotes that follow an open
**          single quote as literals.
**  25-sep-96 (mcgem01)
**              Global data moved to fmtdata.c
**  28-Jul-1998 (nicph02)
**          Bug 89352: The processing of a  literal single quote
**          - coded as '' (for \') - was confusing the algorithm
**         which thought that a single quote string was closed.
**  03-jul-98 (hayke02)
**	    Various changes to correct problems with folding quoted strings in
**	    procedure text which is formatted during copydb/unloaddb. For
**	    various scenarios, the F_inquote global was not being maintained
**	    correctly. This change fixes bug 91825.
**	24-nov-1998 (kitch01)
**		Bug 94226. Amend f_delim() to also check for '+'.
**	09-Jul-1999 (carsu07)
**	    When multiple quotes occur at the end of a line of procedure text 
**	    and need to be folded onto the next line the quotes are split 
**	    incorrectly. The F_inquote global needs to be set. (Bug 96734)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	25-jan-2002 (rigka01) bug #81256
**	     if folding occurs within a quoted string and the string contains
**	     more than one blank before the point of fold, the blanks are
**	     lost. Fix is in f_colfmt(). 
**      21-Apr-2003 (drivi01)
**           Crossed unsubmitted change from 2.5 done by huazh01 to fix
**           bug 107358, INGSRV1730.  After including the fix for 81256,
**           F_inquote global was not being set properly.  This leads to an
**           error when we need to fold a string located inside double quotes.
**       1-Apr-2003 (hanal04) Bug 109584 INGSRV 2102
**           F_CU used by copydb used spaces as padding. This padding was
**           then stripped in t_getline in xferdb. This has lead to bug
**           after bug and a level of unecessary complexity in this
**           function.
**           This change implements NULL padding for the F_CU format
**           and prevents multiple bugs by implementing the very simple
**           rule of not folding if we are in quoted strings. Though
**           quoted words may now be split across lines the text in
**           the quotes is the text that's returned.
**      28-May-2003 (hanal04) Bug 107358 INGSRV1730, Bug 110296 INGSRV2275
**           Original fix for Bug 107358 introduces bug 110296.
**           Rewrite that fix to ensure folding on double quotes
**           adjusts strptr accordingly before we exit.
**	30-jun-2003 (drivi01)
**	     Fixed f_colfmt method to think that comma separated float or
**	     decimal are part of one word and shouldn't be broken into two
**	     lines when we run copydb/unloaddb.
**	11-jul-2003 (gupsh01)
**	     Fixed f_colfmt so that the double quoted long strings for F_CU
**	     unloaddb/copydb case, are dealt correctly. Previously the data
**	     between the fold point and the last open quote was missed out in 
**	     copying to the output. 
**      30-Sep-2003 (hanal04) Bug 110748 INGCBT 486
**           Extend the above change so that '\0' characters are replaced
**           with spaces when we skip over characters in the output
**           stream, as we do for the '\t' case.
**	21-Oct-2003 (rigka01) bug #110759
**	     Fixed f_colfmt for II_DECIMAL when it occurs at the line
**	     width. Followed precedent for fixing bug 109912.
**       6-Nov-2003 (hanal04) Bug 111244 INGDBA265
**           Remove changes for bug 70127. We don't have the leading space
**           anymore and this change causes nb to be set incorrectly
**           during tab processing.
**           Rewrite change for bug 110527 which was incorrectly adjusting
**           strptr for testcase2 of this bug.
**           Went on to create testcase3. Found that delimite table names
**           must not be folded inside the doubquote. Instead we adjust
**           the fold_point to the last open quote.
**      30-Apr-2004 (hanal04) bug 112244 INGSRV2810
**           Set F_inquote to true if we are in a local_quote and returning
**           because we found "\n" or "\r". Failure to do so causes us to
**           lose track of the fact that we are in quotes.
**	13-Aug-2004 (hanal04/raodi01) bug 112196 ingsrv2804
**	    Escaped quotes that was split is causing problem. Hence it is moved
**	    a character back and adjust width to force the escaped quotes to be
**	    handled in the next line
**	    Also whenever escaped quotes are encountered,it is being checked  
**	    if either local_quote is TRUE or F_inquote is set to F_INQ
**	29-Nov-2004 (raodi01) bug 112714 ingsrv2869
**	   Included double quotes as part of escaped quotes handling.
**	19-Jan-2005 (raodi01) bug 113656 INGSRV3097
**	   Handling of characters within double quotes when the number of 
**	   characters goes beyond 70. Wrapping is done instead of folding. 
**	16-Nov-2004 (komve01)
**	    Updated so that copy.in works right for views. Bug 113498.
**	30-Nov-2004 (komve01)
**	    Undo changes of Bug 113498, earlier submission.Added = as one of 
**	    delimiters for folding, so that copy.in works correct for long
**	    table and column names.
**	29-Dec-2004 (komve01)
**	    Added the type F_CU when considering = as a delimiter.
**	24-Feb-2005 (komve01)
**	    Bug #113950/ Issue #:13976988
**	    Added local_quote as additional condition while checking for Kanji
**	    spaces, in case of a double quoted column name with a white space in
**	    its name.
**       18-apr-2006 (stial01)
**          Handle procedure comments, because quote inside comment should not
**          be parsed as string delimiter (b115704)
**          Moved fcolfmt global and static variables to FM_WORKSPACE
**
*/


static bool f_delim();

f_colfmt(string,buf,width,type,mark_byte1, frs_boxmsg )
FM_WORKSPACE	*string;
DB_TEXT_STRING	*buf;
i4		width;
i4		type;
bool		mark_byte1;
bool		frs_boxmsg;
{
    /* internal declarations */

    register u_char	*c;		/* used for fast pointing */
    register i4	i;		/* fast integer */
    i4			nw;		/* count for words */
    bool		inword;		/* true while in words */
    bool		fold;
    i4			ntoadd;		/* num of extra spaces per word break */
    i4			nextra;		/* number of extra spaces */
    i4			nb;		/* last blank in output */
    i4			num_orig_bytes = width;
    u_char		*strptr, *snext, *stemp;
	u_char		*strstart;
    u_char		*strend;
    u_char		*bufstart;
    u_char		*F_T_bufstart;
    u_char		*bufend;
    u_char		*str_quote, *buf_quote;
    bool		local_quote = FALSE;	/* like fmws_inquote but only */
						/* used during this call   */
						/* and only boolean        */
    DB_TEXT_STRING	*pbuf;
    i4			orig_width = width;
    i4			lastoquote = 0; /* Position of last open quote */
    i4			lastcquote = 0; /* Position of last close quote */
    i4			firstcquote = 0; /* Position of first close quote */
    i4			firstoquote = 0; /* Position of first open quote */
    bool		doublequote = FALSE; /* true if quote is double quote*/ 
    u_char              *pnb = NULL;
    u_char		*nullchar;
	/* start of routine */

    strptr = string->fmws_text;
    strend = strptr + string->fmws_count;
    strstart = strptr;
    /*
    **  We can't format directly into user's buffer since we can format
    **  beyond by one byte (on double byte situations and when handling
    **  escapes quotes in an embedded string).  Overflowing by one byte
    **  probably won't hurt anything right now but it may eventually.
    **  So we need to put in this extra overhead of using a local buffer
    **  and then moving it to the user's buffer after things are cleaned
    **  up.
    **  
    **  If no buffer, or we failed allocation last time a size
    **  change was attempted allocate it using the size in multi_size.
    */
    if (string->fmws_multi_buf == NULL)
    {
	/*
	**  If the user needs a size bigger, just use that size.
	**  Note that fmws_multi_size starts out at 2K and increases,
	**  so this will be rarely used.
	*/
    	if (width + FMT_OVFLOW > string->fmws_multi_size)
	{
		string->fmws_multi_size = width + FMT_OVFLOW;
	}

	if ((string->fmws_multi_buf = (DB_TEXT_STRING *) MEreqmem((u_i4) 0,
		string->fmws_multi_size + sizeof(buf->db_t_count),
		FALSE, NULL)) == NULL)
	{
		/*
		**  Use existing buffers if we can't allocate a new one.
		*/
		pbuf = buf;
	}
	else
	{
		pbuf = string->fmws_multi_buf;
	}
    }
    else
    {
	/*
	**  If the buffer is big enough, just use it.
	*/
    	if (width + FMT_OVFLOW <= string->fmws_multi_size)
    	{
		pbuf = string->fmws_multi_buf;
    	}
    	else
    	{
		/*
		**  Free up the old buffer.  If we can't allocate a
		**  new buffer, we use the user's anyway so freeing
		**  things here is not a problem.
		*/
		MEfree((PTR)string->fmws_multi_buf);

		/*
		**  User size is bigger, we need to
		**  double the size of the data area.
		*/
		string->fmws_multi_size += (string->fmws_multi_size - FMT_OVFLOW);

		/*
		**  If the user's size is still bigger, just use
		**  that size instead.
		*/
		if (width + FMT_OVFLOW > string->fmws_multi_size)
		{
			string->fmws_multi_size = width + FMT_OVFLOW;
		}
		if ((string->fmws_multi_buf = 
			(DB_TEXT_STRING *) MEreqmem((u_i4) 0,
			string->fmws_multi_size + sizeof(buf->db_t_count),
			FALSE, NULL)) == NULL)
		{
			/*
			**  Use existing buffers if we can't allocate a new one.
			*/
			pbuf = buf;
		}
		else
		{
			pbuf = string->fmws_multi_buf;
		}
    	}
    }

    bufstart = pbuf->db_t_text;
    bufend = bufstart+width;
    buf->db_t_count = pbuf->db_t_count = width;

    /* first preset buffer to blanks, or NULLs */
    if (type == F_CU)
    {
	MEfill((u_i2) width, '\0', bufstart);
    }
    else
    {
        MEfill((u_i2) width, ' ', bufstart);
    }

    /* reduce the total number of characters left to output by the width */
    string->fmws_left -= width;

    if (type == F_T)
    {
	/* convert string to T format, converting control chars to \c & \ddd */
        /* Bug 53327:
        ** Preset the user's buffer to blanks. This ensures that only valid
        ** text (and not text from previous rows) is printed when a row is
        ** partially or not filled.
        */
        F_T_bufstart = buf->db_t_text;
        MEfill((u_i2) width, ' ', F_T_bufstart);
	f_strT(&(string->fmws_text), &(string->fmws_count),
		     buf->db_t_text, width);
	return;
    }

    nb = nw = i = 0;

    c = bufstart;
    inword = FALSE;

    for (; strptr < strend; CMnext(strptr))
    {
	    switch(*strptr)
	    {
		case('\n'):
		case('\r'):
		    /* on newline return */
		    string->fmws_text = CMnext(strptr);
		    string->fmws_count = strend-strptr;
		    if (buf != pbuf)
		    {
		    	MEcopy((PTR) pbuf->db_t_text, (u_i2) orig_width,
				(PTR) buf->db_t_text);
		    }
		    if (local_quote)
		    {
                        string->fmws_inquote = F_INQ;
		    }
		    return;

		case('\t'):
		    nullchar = c;


		    /* go to multiple of eight */
		    i += 8 - (i % 8);


		    if (i > width)
		    {
			i = width;
		    }
		    c = bufstart + i;
		    string->fmws_cprev = c-1;/* c increased >= 1 & <= 8 single blank chars */

                    if (type == F_CU)
                    {
	                for(; nullchar < c; nullchar++)
	                {
	                    *nullchar = ' ';
	                }
                    }
 
		    nb = i;
		    inword = FALSE;
		    break;

		default:

		    /* Handle comments */
		    if (!(local_quote || string->fmws_inquote) && strptr > strstart
			&& !CMdbl1st(strptr))
		    {
			if (string->fmws_incomment && *(strptr-1) == '*' && *strptr == '/')
			    string->fmws_incomment = FALSE;
			else if (!string->fmws_incomment && 
					*(strptr-1) == '/' && *strptr =='*')
			    string->fmws_incomment = TRUE;
		    }

		    if (CMwhite(strptr) && (!CMdbl1st(strptr) || type != F_C))
		    {
		    	string->fmws_cprev = c;
			/* Convert Kanji blank to two single byte blank */
			if ( CMdbl1st(strptr))
			{
				*strptr = *(strptr+1) = ' ';
			}
			*c++ = ' ';
			i++; 
			nb = i;
			inword = FALSE;
		    }
		    else if ((f_delim(stemp=strptr) || (*strptr=='=' && (type==F_CU))) && !( *strptr==',' && CMdigit(CMnext(stemp))))
		    {
		    	string->fmws_cprev = c;
			CMcpychar(strptr, c);
			CMnext(c);
			CMbyteinc(i, strptr);
			nb = i;
			inword = FALSE;
		    }
		    /* change control characters to spaces. 35574 */
		    else if ( (frs_boxmsg) && CMcntrl(strptr) )
		    {
		    	string->fmws_cprev = c;
			*c    = ' ';
			CMnext(c);
			CMbyteinc(i, strptr);
			nb = i;
			inword = FALSE;
		    }
		    else
		    {    /* printable char */
			snext = strptr;
			CMnext(snext);
			if (*snext == ' ')
			  pnb = strptr;
			/* Is it an open or close quote? */
                        /* If double quotes lies within single quotes
                           ensure it is treated as a literal  - 80739 */
			if (!string->fmws_incomment &&
			   ((type == F_CU && *strptr == '"' 
			   && *string->fmws_cprev != '\\'
                           && !((local_quote || string->fmws_inquote) 
			   && string->fmws_quote == '\''))
				|| (type == F_CU && *strptr == '\'')))
			{ 
                                /* If now checks string not string->fmws_quotes - 80739 */
				/* handle escaped quotes here */
				if ((*strptr == '\'')||(*strptr == '"'))
				{
					snext = strptr;
					CMnext(snext);
					if ((*snext == *strptr)  && (local_quote 
					    || string->fmws_inquote == F_INQ))
					{ 
						if (i < (width-1))
						{
							CMbyteinc(i, strptr);
							CMcpyinc(strptr, c);
							CMbyteinc(i, strptr);
							CMcpychar(strptr, c);	
							string->fmws_cprev = c;
							CMnext(c);
							break;
						}
						else
						{
					       /* We must not split escaped
                                               ** quotes.Move back a character
                                               ** and adjust width to force
                                               ** the escaped quotes to be
                                               ** handled on the next line.
                                               */	
                                                 	strptr = CMprev(strptr, string->fmws_text);	
                                                 	width--;
                                                 	break;
						}	
					}
				}
				/* Is this the first char? */
				if (strptr == string->fmws_text) 
				{
					/* Is it a folded string? */
					if (string->fmws_inquote == F_INQ) 
					{
						if (*strptr == string->fmws_quote)
						{
							string->fmws_inquote = F_NOQ;
							lastcquote = i;
							if (!firstcquote)
						 	   firstcquote = i;
						}
					}
					else  /* start of new quoted string */
					{
						local_quote = TRUE;
						lastoquote = i;
						if (!firstoquote)
						    firstoquote = i;
						string->fmws_quote = *strptr;
						str_quote = strptr;
						buf_quote = c;
					}
				}
				else
				{
					/*
					**  If we have a folded string (i.e.,
					**  the beginning of the quoted
					**  string was on a previous line)
					**  and we are not quoted on the
					**  current line and the quote
					**  character is the same as the
					**  beginning quote, then this
					**  must be the end quote for a
					**  folded string.
					*/
					if (string->fmws_inquote == F_INQ && !local_quote
						&& *strptr == string->fmws_quote)
					{
						string->fmws_inquote = F_NOQ;
						lastcquote = i;
						if (!firstcquote)
						   firstcquote = i;
					}
					else if (local_quote &&
						*strptr == string->fmws_quote)
					{
						/*
						**  If we are in a quoted
						**  string on the current
						**  line and the quote
						**  character matches the
						**  beginning quote character,
						**  then this must be the
						**  end of a quoted string
						**  on the current line.
						*/
						local_quote = FALSE;
						string->fmws_inquote = F_NOQ;
						lastcquote = i;
						if (!firstcquote)
						   firstcquote = i;
					}
					else 
					{
						/*
						**  Otherwise, must be the
						**  beginning of a quoted
						**  string on the current
						**  line.  Set state info
						**  to reflect this fact.
						*/
						local_quote = TRUE;
						lastoquote = i;
						if (!firstoquote)
						    firstoquote = i;
						string->fmws_quote = *strptr;
						str_quote = strptr;
						buf_quote = c;
					}
				}
			}
			if (! inword)
			    ++nw;
			inword = TRUE;
		    	string->fmws_cprev = c;
			CMcpychar(strptr, c);
			CMnext(c);
		    	CMbyteinc(i, strptr);
		    }
		    break;
	    }


	/* see if anymore to go */

	if (i >= width)
	{
            u_char	*cur_strptr;
            cur_strptr = strptr;

	    if (type == F_CJ || type == F_CF || type == F_CU || type == F_CFE || type == F_CJE)
	    {	/* do folding */

		if ( type == F_CU && local_quote && string->fmws_inquote == F_NOQ )
		{
			u_char	*quote;

			quote = str_quote;

			nw--;
			/* need to back up one char */
			/* we move the strptr to one place
			** before start of a quote point in
			** the string text. 
			*/
			strptr = CMprev(str_quote, string->fmws_text);
			/*
			**  Back up one more character if the character
			**  in front of a single quote character is
			**  a 'x' or 'X'.  This indicates a hex string
			**  literal and must be kept together.
			**  But if the hex string is at the beginning
			**  then, we have the just fold the string.
			**  Not much else we can do.
			*/
			if (*quote == '\'' &&
				(*strptr == 'x' || *strptr == 'X'))
			{
				if (strptr == string->fmws_text)
				{
					strptr = cur_strptr;
					string->fmws_inquote = F_INQ;
				}
				else
				{
					strptr = CMprev(strptr, string->fmws_text);
					buf_quote = CMprev(buf_quote, bufstart);
					string->fmws_inquote = F_NOQ;
					MEfill((u_i2)(bufend-buf_quote), '\0',
						buf_quote);
				}
			}
			else  /* undo string manoeuvres above */
			  if (*quote == '\'') 
			  {
				string->fmws_inquote = F_INQ;
			 	strptr = cur_strptr;
			 	str_quote = quote;	
				nw++;
			  }
			  else /* double quote is not a quote situation */
			  if (*quote == '"') 
			  {
				if(type == F_CU)
			 	    strptr = cur_strptr;
				doublequote = TRUE; 
			  }
		}
		    if ( type == F_CU && string->fmws_inquote == F_INQ)
		    {
			fold=FALSE;
		    }
	            else
		    {
			if ( nb > 0 ) /* found a prev blank so we can fold */
			   fold = TRUE;
		   	else
			   fold = FALSE;
	
			/*
			** Do not fold (i.e. backup) if the previous character
			** was a space or a delimeter.
			** unless the prev is a comma but the next is a digit
			** which is the clue that II_DECIMAL is set to comma 
			*/
			if ( CMspace(string->fmws_cprev) || 
				(f_delim(stemp=string->fmws_cprev) &&
		             !( *string->fmws_cprev==',' && (stemp=strptr) && 
				CMdigit(CMnext(stemp)) )) )
				fold = FALSE;

			/* Do not fold local quoted strings */
                        if(local_quote && !doublequote && (type == F_CU))
			{
				fold = FALSE;
			}

			/*
			** Check the next source character.  If we are at 
			** the end of the string or the next character is 
			** white space there is no need to fold(backup).
			** And skip over the next source character.
			*/
			/*
			** Fix for bug 6398 -- do not treat delimiters
			** like whitespace!  Removed call to f_delim().
			*/
			/*
			 ** Changed to fix problem with first byte of a double 
			 ** byte Kanji on the last column of a field that is 
			 ** then followed by a white space.
			 */
    	if (( strptr + 1 >= strend || CMwhite(strptr + 1) )&& (!local_quote))
			{
				fold = FALSE;
				/* should we preserve spaces? */
				if ( type != F_CJE && type != F_CFE )
				{
					CMnext(strptr);
					while (strptr+CMbytecnt(strptr) < strend 
						&& CMwhite(strptr+CMbytecnt(strptr)))
						CMnext(strptr);
				}
			}
# ifdef DOUBLEBYTE
/*
* bug 71653:
* strptr and i may not be in phase with one another
* when DBL characters are in string - suppress
fold here and output DBL string wrapped at current position
*/
			if ( (CMdbl1st(strptr)) &&
				((strptr - strstart) < width) &&
				i == width &&
				type != F_CU )
				fold = FALSE;
# endif

			if ( fold )
			{
				i4	fold_point;

				/* For F_CU we must not fold inside quotes.
				** Though we are not currently in quotes the
				** last space fold point may be inside ealier
				** quotes. Prevent the folding inside quotes
				** by folding at the last close quote instead.
				*/
				if ((!doublequote || lastoquote < lastcquote) &&
				    ((type == F_CU) && (lastcquote >= nb)))
                                {
					fold_point=lastcquote+1;
				}
				else
				{
					fold_point=nb;
				}

				nw--;
				if (!doublequote || (lastoquote < lastcquote) ||
                                       (type == F_CU))
		 		    strptr -= width-fold_point;

                                if (doublequote && local_quote &&
					 (lastoquote > lastcquote))
                                {
				    /* strptr will currently point to the
				    ** character before the last open quote.
				    ** It must be adjusted to the fold point.
				    ** Check the fold point is not inside
				    ** quotes. Testcase3 for bug 111244.
				    */
				    if(nb > lastoquote)
				    {
                                        fold_point = lastoquote;
				    }
                                    strptr = cur_strptr - (width-fold_point);
                                }
				
                		/*
				** Bug 64312 - decrement last blank position to
				** compensate for addition of a leading blank.
				** Also, check if last blank position is in a
				** quoted string. 
				*/
				if (type == F_CU)
				{
			            /* Case 2: other
			            ** check if fold occurs after at least
				    ** one blank within quoted string
			            ** do not fold after a blank, otherwise 
				    ** blanks at end of line will get truncated 
			    	    ** fold after most recent non-blank in
				    ** string and  write blanks on next line
				    */ 

				    /* We no longer fold quoted strings */
				    MEfill((u_i2)(bufend-(bufstart+fold_point)), '\0', bufstart+fold_point);					
				}
				else
                                {
				     MEfill((u_i2)(bufend-(bufstart+fold_point)), ' ', bufstart+fold_point);
				}
				num_orig_bytes = fold_point; /* don't count blank padding for cje */
		      }
                      else
                      {
                          if(doublequote)
			  {
			      strptr = cur_strptr;
			      if(local_quote)
			      {
				  string->fmws_inquote = F_INQ;
			      }
			  }
                      }

                }
		/*
		** now adjust if needed - again, only white
		** space in output buffer is spaces.
		** nb will now be used to count needed blanks
		*/

		if (type == F_CJ || type == F_CJE)
		{
		    if (type == F_CJE)
		    	nb = width - num_orig_bytes; 
		    else
			nb = width - f_lentrmwhite(bufstart, width);
		    width -= nb;
		    c = bufstart;
		    nw--;	/* don't count first word */

		    /* now add the blanks */

		    if (nb > 0 && nw > 0)
		    {   /* only if not at end of line */
			ntoadd = nb / nw;
			nextra = nb % nw;
			for (; nw > 0; nw--)
			{   /* find next word */
			    while (CMspace(c))
			    {
				CMbytedec(width, c);
				CMnext(c);
			    }

/* Integrated bug fix #63235 from 6.4 codeline by abowler: handle delimiters 
** when justifying strings. (harpa06)
*/

                            /* Handle delimiters when justifying
                            ** - insert whatever space is needed
                            ** after the delim, and decrement the
                            ** word count - bug 63235.
                            */
                            while ((!CMspace(c)) && (!f_delim(c)))
                            {
                                CMbytedec(width, c);
                                CMnext(c);
                            }
 
                            if (f_delim(c))
                            {
                                CMbytedec(width, c);
                                CMnext(c);
                            }
 
			    nb = (nw <= nextra)? ntoadd+1 : ntoadd;
			    if (nb > 0)
			    {    /* shift buffer right nb chars */
				f_strrgt(c, width, c, width+nb);
				width += nb;
			    }
			}
		    }
		}
	    }

	    /*
	    ** If the last byte in the formatted string happens to be the first
	    ** byte of a 2-byte char then mask it and set fmws_text accordingly
	    ** so the full 2-byte char gets read next time through.  
	    ** Fix for bug 5069 (cmr).
	    */
	    if ( CMdbl1st( strptr ) && (i > width) )
	    {
	       if(type == F_CU)
	       {
		   *(bufend - 1) = (mark_byte1 ? F_BYTE1_MARKER : '\0');
	       }
	       else
	       {
	           *(bufend - 1) = (mark_byte1 ? F_BYTE1_MARKER : ' ');
	       }
	       string->fmws_text = strptr;
	    }
	    else
	    	string->fmws_text = CMnext(strptr);

	    string->fmws_count = strend-strptr;

	    if (buf != pbuf)
	    {
		/*
		**  Using orig_width since justification above may have
		**  changed the value of "width" that was passed in.
		*/
		MEcopy((PTR) pbuf->db_t_text, (u_i2) orig_width,
			(PTR) buf->db_t_text);
	    }
	    return;
	}
    }

    if (buf != pbuf)
    {
	/*
	**  Using orig_width since justification above may have
	**  changed the value of "width" that was passed in.
	*/
     	MEcopy((PTR) pbuf->db_t_text, (u_i2) orig_width, 
			(PTR) buf->db_t_text);
    }
    string->fmws_text = strptr;
    string->fmws_count = 0;
    return;
}
/*{
** Name:	f_delim - checks for candidate delimiters for folding.
**
** Description:
**	Returns true if the character pointed to is a character for which 
**	it is always legal to break a line on
**	(for the terminal moniter scanner anyway).
**
** Inputs:
**	strptr		points to string char to check.
**
** Outputs:
**
**	Returns:
**		TRUE	strptr points to valid delimiter.
**		FALSE	strptr doesn't point to valid delimiter.
**
**	Exceptions:
**		none.
**
** Side Effects:
**	none.
**
** History:
**	25-aug-1987 (rdesmond)
**		written.
**  24-nov-1998 (kitch01)
**		Bug 94226. If the string is greater than 70 characters and
**		does not contain any spaces or other word deleimiters then
**		the string may be folded in the middle of a column name.
**		Include + as a valid delimiter.
**  30-Nov-2004 (komve01)
**              Undo changes of Bug 113498, earlier submission.Added = as one of
**              delimiters for folding, so that copy.in works correct for long
**              table and column names.
**  29-Dec-2004 (komve01)
**              Undo changes of adding = as a delimiter. We will not add a new
**              delimiter, instead we will have a validation when required.
*/
static
bool
f_delim(strptr)
char	*strptr;
{
	return (*strptr == ')' || *strptr == ','  || *strptr == '+')
	  ? TRUE : FALSE;
}
