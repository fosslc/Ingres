/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cm.h>
# include	<me.h>
# include	<lo.h>
# include	<si.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<ui.h>
# include	<ooclass.h>
# include	<er.h>
# include	<erde.h>
# include	"doglob.h"

# define	HT			'\t'
# define	CR			'\r'
# define	LF			'\n'
# define	FF			'\f'
# define	SPACE			' '

# define	DQUOTE			'\"'

# define	DO_COMMENT_DELIM	'#'

# define	DO_MAXLINE		256
# define	DO_MAX_NAMES_PER_LINE	16

	static	VOID	do_getwords(char *,OOID);
	static	VOID	do_if_name(char *,OOID);
	static	i4	do_readln(FILE *,char *,i2);

/*
** Name:	doifile.c
**
** Description:
**	Extract the FE object names for a supplied ASCII file.
**
** History:
**	12-jan-1993 (rdrane)
**		Written.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*
** Name:	do_ifile_parse
**
** Description:
**	This routine opens the supplied ASCII file for reading, retrieves each
**	line, parses out each word on that line, places the word in an FE_DEL
**	structure as the FE object name, expands the name as required, closes
**	the ASCII file, and returns.
**
** Inputs:
**	filename	The name of the ASCII file containing the FE object
**			names.  If Ptr_fd_top is not NULL (there were FE object
**			names on the command line), then Cact_fd is expected
**			to point to the tail FE_DEL structure in the chain.
**	del_obj_type	Class of FE object to delete.  Eventually passed to
**			do_expand_name();
**
** Outputs:
**	Cact_fd will point to the tail FE_DEL structure in the chain (unless the
**	file open fails or the file contains no FE object names, in which case
**	it is unchanged).
**
** Returns:
**	FAIL if the specified input file cannot be opened.
**	OK otherwise.  Note that an empty file is not considered
**	to be an error.
**
** Side Effects:
**	The FE_DEL structure list may be augmented.
**
** History:
**	12-jan-1993 (rdrane)
**		Written.
*/

i4
do_ifile_parse(char *filename,OOID del_obj_type)
{
	i4	i;
	FILE	*fp;
	STATUS	status;
	LOCATION loc;
	char	buf[(DO_MAXLINE + 1)];


	LOfroms((PATH & FILENAME),filename,&loc);
	status = SIopen(&loc,ERx("r"),&fp);
	if  (status != OK)
	{
		IIUGerr(E_DE000D_Dobj_open_fail,UG_ERR_ERROR,1,filename);
		return(FAIL);
	}

	while ((i = do_readln(fp,&buf[0],sizeof(buf))) >= 0)
	{
		if  (i == 0)
		{
			/*
			** Ignore empty lines
			*/
			continue;
		}
		do_getwords(&buf[0],del_obj_type);
	}

	SIclose(fp);

	return(OK);
}


/*
** Name:	do_getwords
**
** Description:
**	This routine is an alternate implementation of STgetwords(), since that
**	routine fails for "words" of the form "owner".fe_object (it returns two
**	words "owner" and .fe_object instead of the single word
**	"owner".fe_object).  White space (space, tab, carriage return,) and
**	the comment delimiter are the only recognized word delimiters, and
**	these are not recognized within double quotes (to handle delimited
**	owners like "this owner").  Embedded double quotes must be escaped in
**	the usual manner, e.g., "ab""cd" contains one embedded double quote.
**
**	Comments are ignored.  They are introduced by DO_COMMENT_DELIM and
**	terminated by end-of-line.  Thus, there is no nesting or spanning of
**	lines.  A comment is a word delimiter.
**
**			***  N O T I C E  ***
**
**	At the moment, DELOBJ does NOT support either owner qualification of
**	FE_object names or specification of the name components as delimited
**	identifiers.  Still, this routine will be retained so that if/when this
**	support is added, we don't have to totally re-develop it.
**
**
** Inputs:
**	buf -		A pointer to a buffer containing the string to be
**			parsed.  It is assumed to be terminated by a NULL
**			(as formatted by do_readln()), and to NOT contain any
**			LF or FF.
**	del_obj_type	Class of FE object to delete.  Eventually passed to
**			do_expand_name();
**
** Outputs:
**	Cact_fd will point to the tail FE_DEL structure in the chain (unless the
**	file contains no FE object names/expandable patterns, in which case
**	it is unchanged).
**
** Returns:
**	Nothing.
**
** Side Effects:
**	The input string is essentially destroyed, since terminating white space
**	characters are replaced with NULL to effect word string termination.
**
** History:
**	12-jan-1993 (rdrane)
**		Written.
*/

static
VOID
do_getwords(char *buf_ptr,OOID del_obj_type)
{
	bool	in_quote;
	char	*word_ptr;

	
	in_quote = FALSE;
	word_ptr = NULL;
	while (*buf_ptr != EOS)
	{
		switch(*buf_ptr)
		{
		case DO_COMMENT_DELIM:
			/*
			** If we're not in a quoted word and we have a word
			** in progress, end it now.
			*/
			if  ((!in_quote) && (word_ptr != NULL))
			{
				/*
				** Terminate word in place, call expansion
				** routine to chain the name in, and then
				** reset word_ptr.
				*/
				*buf_ptr = EOS;
				do_if_name(word_ptr,del_obj_type);
				word_ptr = NULL;
			}
			/*
			** Finished with this line!
			*/
			return;
			break;

		case DQUOTE:
			/*
			** Start a word if we haven't already. (An escaped
			** quote starting a word is the user's problem!)
			*/
			if  (word_ptr == NULL)
			{
				/*
				** Set start of this word
				*/
				word_ptr = buf_ptr;
			}
			/*
			** Look at the next character in case it's an escaped
			** quote
			*/
			CMnext(buf_ptr);		
			if  (*buf_ptr == EOS)
			{
				/*
				** We should ALWAYS see LF before EOS!
				*/
				break;
			}
			if  (*buf_ptr == DQUOTE)
			{
				CMnext(buf_ptr);
				break;
			}
			/*
			** Flip-flop our quote state since it wasn't escaped.
			*/
			in_quote ^= TRUE;
			break;
			
		case SPACE:	/* Check for end of word.	*/
		case HT:
		case CR:
			/*
			** Ignore white space which occurs within a quoted,
			** word, and before the start of a word!  Yes, the
			** compare is expressed as negative logic, but it
			** avoids having to repeat the CMnext().
			*/
			if  ((!in_quote) && (word_ptr != NULL))
			{
				*buf_ptr = EOS;
				do_if_name(word_ptr,del_obj_type);
				word_ptr = NULL;
			}
			CMnext(buf_ptr);
			break;

		default:
			if  (word_ptr == NULL)
			{
				/*
				** Set start of this word
				*/
				word_ptr = buf_ptr;
			}
			CMnext(buf_ptr);
			break;
		}
	}
	
	/*
	** Process any pending word, which is already implicitly
	** by the EOS at EOL!
	*/
	if  (word_ptr != NULL)
	{
		do_if_name(word_ptr,del_obj_type);
	}

	return;
}



/*
**   DO_IF_NAME - Allocate an FE_DEL structure for the current word and call
**		  do_expand_name() to process it.
**
** Parameters:
**	fd		file descriptor of file to read.
**	buf_ptr		buffer to hold text string from file line.
**	maxchar		maximum number of characters to return.
**
** Returns:
**	Number of characters read.  This may be zero for empty lines.
**	The terminating LF or FF is NEVER returned.
**		-1	indicates EOF or an error.
**
** History:
**	12-jan-1993 (rdrane)
**		Created.
*/
static
VOID
do_if_name(char *wrd_ptr,OOID del_obj_type)
{
	FE_DEL	*fd;
	FE_DEL	*prev_fd;


	/*
	** Allocate a new FE_DEL for this FE object name
	*/
	fd = (FE_DEL *)MEreqmem(0,sizeof(FE_DEL),TRUE,NULL);

	/*
	** If Ptr_fd_top is NULL, then there were no FE object
	** names on the command line, so start the FE_DEL chain
	** here.  Otherwise, chain on to whatever currently exists.
	*/
	if  (Ptr_fd_top == (FE_DEL *)NULL)
	{
		Ptr_fd_top = fd;
		prev_fd = (FE_DEL *)NULL;
	}
	else
	{
		Cact_fd->fd_below = fd;
		prev_fd = Cact_fd;
	}

	/*
	** Make this the new current FE_DEL
	*/
	Cact_fd = fd;
	Cact_fd->fd_name_info = (FE_RSLV_NAME *)MEreqmem(0,
				sizeof(FE_RSLV_NAME),TRUE,NULL);
	Cact_fd->fd_name_info->name = STalloc(wrd_ptr);
	Cact_fd->fd_below = (FE_DEL *)NULL;

	Cact_fd = do_expand_name(Cact_fd,&prev_fd,del_obj_type);
	/*
	** Returns ptr to current end of FE_DEL chain which
	** may be NULL if 1st and no match.  If no match, the
	** FE_DEL and its name will have been de-allocated,
	** and prev_fd and and Ptr_fd_top will be altered as
	** required.
	*/

	return;
}


/*
**   DO_READLN - Read in one text line from the referenced file.  If more than
**		 the specified number of chars are read before finding an
**		 end-of-line, then return with what has already been read.
**
** Parameters:
**	fd		file descriptor of file to read.
**	buf_ptr		buffer to hold text string from file line.
**	maxchar		maximum number of characters to return.
**
** Returns:
**	Number of characters read.  This may be zero for empty lines.
**	The terminating LF or FF is NEVER returned.
**		-1	indicates EOF or an error.
**
** History:
**	12-jan-1993 (rdrane)
**		Created from REPORT r_readln() routine.  Corrected loop limit
**		by not reading the second byte of a double-byte character when
**		it would exceed maxchar.  Note that implication that maxchar
**		will never exceed (sizeof(*buf_ptr) - 1).  Also corrected case
**		of empty file - ptr should not be incremented, as no character
**		was placed into buf_ptr.
*/

static
i4
do_readln(FILE *fp,char *buf_ptr,i2 maxchar)
{
	char	*ptr;			/* Working pointer into line buffer */
	i4	count;			/* Number of chars read */
	STATUS	status;


	ptr = buf_ptr;
	while (maxchar > 0)
	{
		status = SIread(fp,sizeof(char),&count,ptr);
		CMbytedec(maxchar,ptr);
		if ((CMdbl1st(ptr)) && (maxchar > 0))
		{
			status = SIread(fp,sizeof(char),&count,(ptr + 1));
			CMbytedec(maxchar,ptr);
		}
		switch(status)
		{
		case(ENDFILE):
			*ptr = EOS;
			count = STlength(buf_ptr);
			if  (count > 0)
			{
				/*
				** We hit EOF on a file that ended abruptly
				** without a final end-of-line character.
				** Return the final line's count this time -
				** we'll return EOF on the next call.
				*/
				return(count);
			}
			return(-1);
			break;

		case(OK):
			if ((*ptr == CR) || (*ptr == LF) || (*ptr == FF))
			{
				/*
				** Do NOT include the end-of-line character!
				*/
				*ptr = EOS;
				return(STlength(buf_ptr));
			}
			CMnext(ptr);
			break;

		default:
			/*
			** According to the documentation, we shouldn't
			** be able to get here.
			*/
			IIUGerr(E_DE000E_Dobj_read_fail,UG_ERR_ERROR,0);
			return(-1);
		}
	}

	/*
	** If we wind up here, then
	** we've exceeded maxchars!
	*/

	*ptr = EOS;
	return(STlength(buf_ptr));
}

