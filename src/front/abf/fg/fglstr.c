/*
** Copyright (c) 1989, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<cm.h>
# include	<er.h>
# include	<lo.h>
# include	<si.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<fglstr.h>

/**
** Name:	fglstr.c -	Linked string routines
**
** Description:
**	This file defines:
**
**	IIFGals_allocLinkStr		Allocate a linked string
**	IIFGols_outLinkStr		Output a linked string to a file
**	IIFGglsGenLinkStr		Generate text from linked strings
**	IIFGfls_freeLinkStr		Free linked string memory
**	IIFGwrj_writeRightJustified	Write a right-justified string
**
** History:
**	15 may 1989 (Mike S)	Initial version
**	5 jul 1989 (Mike S)	Add IIFGfls_freeLinkStr
**	8 jan 1990 (Mike S)	IIFGglsGenLinkStr
**	11-Feb-93 (fredb)
**	   Porting change for hp9_mpe: SIfopen is required for additional
**	   control of file characteristics in IIFGdls_dumpLinkStr.
**	20-jan-97 (mcgem01)
**	   A variable name starting with a "_" violates the coding
**	   standards and is illegal when using MS Visual C++.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
**/

static  TAGID lstrtag = 0;       /* TAG for dynamic memory */

FUNC_EXTERN VOID	IIFGfls_freeLinkStr();
FUNC_EXTERN VOID	IIFGglsGenLinkStr();

/*
**	The following define the format to be produced.  We produce 78-column
**	lines (73 plus the "follow" string); 78 rather than 80 because they
**	are examined in a tablefield during error processing.  Split lines 
**	have an extra two-column margin.
*/
# define WIDTH		78
# define LCOLUMNS	(WIDTH - 5)
# define EXTRA_PAD	"  "

/* Intermediate buffer size */
# define BUFSIZE	256


/*
**	Text represenations of the follow strings.  Thes must appear in
**	the same order as the constans in fglstr.h.
*/
static const char ii_space[]           = ERx(" ");
static const char ii_and[]             = ERx(" AND ");
static const char ii_comma[]           = ERx(", ");
static const char ii_semicolon[]       = ERx("; ");
static const char ii_equals[]          = ERx("=");
static const char ii_open[]           = ERx("(");
static const char ii_close[]           = ERx(") ");

static struct
{
	const char *string;
	i4     size;
} str_follow[] =
{
	{ ii_space, sizeof(ii_space) - 1 },
	{ ii_and, sizeof(ii_and) - 1 },
	{ ii_comma, sizeof(ii_comma) - 1 },
	{ ii_semicolon, sizeof(ii_semicolon) - 1 },
	{ ii_equals, sizeof(ii_equals) - 1 },
	{ ii_open, sizeof(ii_open) - 1 },
	{ ii_close, sizeof(ii_close) - 1 }
};

static	VOID	file_write();

/*{
** Name:	IIFGals_allocLinkStr	Allocate a linked string
**
** Description:
**	Allocate an LSTR structure.  
**
**	Note that memory is allocated on the "lstrtag", whcih we keep locally.
**	Memory will be deallocated after we write the strings out.
**
** Inputs:
**	parent		LSTR *	parent structure
**	follow		i4	What follows string
**
** Outputs:
**	none
**
**	Returns:
**			LSTR *	Allocated sturcture
**				(NULL on allocation failure)
**
**	Exceptions:
**		none
**
** Side Effects:
**		none
**
** History:
**	15 May 1989 (Mike S)	Initial version
*/
LSTR *
IIFGals_allocLinkStr(parent, follow)
LSTR 	*parent;
i4	follow;
{
	LSTR *ptr;

	/* Get a tag, if we don't have one */
	if (lstrtag == 0) lstrtag = FEgettag();

	/* Allocate memory and initialize structure */
	ptr = (LSTR *)FEreqmem(lstrtag, (u_i4)sizeof(LSTR), FALSE, NULL);
	if (ptr != NULL)
	{
		ptr->string = ptr->buffer;
		ptr->buffer[0] = EOS;
		ptr->follow = follow;
		ptr->next = NULL;
		ptr->nl = FALSE;
		ptr->sc = FALSE;
	}

	/* Link to parent, if one was specified */
	if (parent != NULL)
		parent->next = ptr;
	return(ptr);
}

/*{
** Name:	IIFGols_outLinkStr
**
** Description:
**	Output a linked set of strings to a file.
**
** Inputs:
**	strings		LSTR *	Linked strings to output.
**	file		FILE *  File to write to
**	pad		char *  Left-margin padding.
**
** Outputs:
**	none
**
**	Returns:
**		none
**
**	Exceptions:
**		none
**
** Side Effects:
**		The strings are written to the file, and the memory associated
**		with them is freed.
**
** History:
**	18 May 1989 (Mike S)	Initial version
**	11 Aug 1989 (Mike S)	Handle case where string contains '%'
*/
VOID 
IIFGols_outLinkStr(strings, file, pad)
LSTR 	*strings;
FILE 	*file;	
char 	*pad;
{
	/* Write each line produced to "file" via static func. "file_write" */
	IIFGglsGenLinkStr(strings, pad, file_write, (PTR)file);
}

/*{
** Name:	IIFGglsGenLinkStr - Generate linked strings
**
** Description:
**	Generate the text from a set of linked strings, and call
**	the output function for each line.
**
** Inputs:
**	strings		LSTR *		Linked strings
**	pad		char *		Left margin pad
**	ofunc		VOID (*)()	Output function
**	data		PTR		Data for function
**
**
**	The output function is called as:
**
**		ofunc(line, data)
**			char *line;	/* Line to output
**			PTR  data;	/* Data from caller
**
**	for each line produced.
**
** Outputs:
**	none
**
**	Returns:
**		none
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	1/8/90 (Mike S)	Initial version
*/
VOID
IIFGglsGenLinkStr(strings, pad, ofunc, data)
LSTR	*strings;		/* Linked strings to output */
char	*pad;			/* Left-margin pad */
VOID	(*ofunc)();		/* Output function */
PTR	data;
{
	char	buffer[BUFSIZE];	/* Line buffer */
	char	*bufptr = buffer;	/* Next position in buffer */
	i4 	pad_size;		/* Number of spaces pad equates to */
	bool	partline = FALSE;	/* Have we written a partial line ? */
	bool	split = FALSE;		/* Have we split a long line? */
	i4	follow;
	char	*p;

	/* 
	** Calculate the zero-based column number after the pad, assuming tab
	** stops every 8.  Weird things like newlines and backspaces in the 
	** pad aren't allowed for.
	*/
	for (p = pad, pad_size = 0; *p != EOS; CMnext(p))
	{
		if (*p == '\t')
			pad_size = 8 * (pad_size / 8 + 1);
		else
			pad_size += CMbytecnt(p);
	}

	for ( ; ; )
	{
		/*
		**	If we're done, complete any partial line, free the 
		**	memory, and return.
		*/
		if (strings == NULL)
		{
			if (partline) 
				(*ofunc)(buffer, data);
			IIFGfls_freeLinkStr();
			return;
		}

		/*
		**	If we're at column zero, pad to the left margin.
		*/
		if (bufptr == buffer)
		{
			i4 pad_spaces;		/* Number of spaces to pad */

			pad_spaces = pad_size;
			if (split)
			{
				pad_spaces += sizeof(EXTRA_PAD) - 1;
			}
			while (bufptr < buffer + pad_spaces)
				*bufptr++ = ' ';
			*bufptr = EOS;
		}

		/*
		**	Add the next string, if it fits.  We always add
		**	the first string.
		*/
		if ( !partline || 
		     ((bufptr - buffer) + STlength(strings->string) < LCOLUMNS))
		{
			follow = strings->follow;
			if ((follow < LSTR_NONE) || (follow > LSTR_CLOSE))
				follow = LSTR_NONE;
			STcopy(strings->string, bufptr);
			bufptr += STlength(strings->string);
			STcopy(str_follow[follow].string, bufptr);
			bufptr += str_follow[follow].size;
			partline = TRUE;

			/* 
			**	See if a newline or staement terminator
			**	is needed here.
			*/
			if (strings->sc)
			{
				STcopy(ii_semicolon, bufptr);
				bufptr += sizeof(ii_semicolon) - 1;
			}
			if (strings->nl)
			{
				(*ofunc)(buffer, data);
				bufptr = buffer;
				partline = FALSE;
				split = FALSE;
			}
			strings = strings->next;
		}
		else
		/*
		**	If it doesn't fit, complete the current line
		**	and go around again.
		*/
		{
			(*ofunc)(buffer, data);
			bufptr = buffer;
			partline = FALSE;
			split = TRUE;
		}
	}
}

#ifdef DEBUG

/*{
** Name:  IIFGdls_dumpLinkStr - dump a LSTR linked string structure to a file.
**
** Description: compile with CCFLAGS set to "-DDEBUG" to include this routine.
**
** Inputs:
**	char *fn	file name to write dump to (appends dump to bottom)
**	LSTR *p_ls;	beginning of linked list of LSTR structures
**
** History:
**	6/20/89 (pete)	Initial version.
**	11-Feb-93 (fredb)
**	   Porting change for hp9_mpe: SIfopen is required for additional
**	   control of file characteristics.
*/
IIFGdls_dumpLinkStr(fn, p_ls)
char *fn;	/* file name to write dump to */
LSTR *p_ls;	/* beginning of linked list of LSTR structures */
{
	LOCATION lsdmploc;
	FILE	*lsdmpfile;

	LOfroms (FILENAME&PATH, fn, &lsdmploc);
#ifdef hp9_mpe
        if (SIfopen (&lsdmploc, ERx("a"), SI_TXT, 252, &lsdmpfile) != OK)
#else
        if (SIopen (&lsdmploc, ERx("a"), &lsdmpfile) != OK)
#endif
	{
            SIprintf("Error: unable to open LSTR dump file: %s", fn);
            return;
        }

        SIfprintf (lsdmpfile, "\nLSTR linked list:\n");

        while ( p_ls != NULL )
	{
	    SIfprintf (lsdmpfile, "\n(&%d) %s, &%d, %d, %d",
		p_ls, p_ls->string, p_ls->next, p_ls->follow, p_ls->nl);

	    p_ls = p_ls->next;
	}

	SIfprintf (lsdmpfile, "\n");

	if (lsdmpfile != NULL)
	    SIclose (lsdmpfile);
}
#endif


/*{
** Name:	IIFGfls_freeLinkStr - Free linked string memory
**
** Description:
**	Free the memory for the linked string structures
**
** Inputs:
**	none
**
** Outputs:
**	none
**
**	Returns:
**		none
**
**	Exceptions:
**		none
**
** Side Effects:
**	Free dynamic memory.
**
** History:
**	5 Jul 1989 (Mike S)	Initial version.
*/
VOID
IIFGfls_freeLinkStr()
{
	if (lstrtag != 0) FEfree(lstrtag);
}


/*{
** Name: IIFGwrj_writeRightJustified	Write a right-justified string
**
** Description:
**	Write a right-justified string to the generated code.  If it
**	doesn't fit, left-justify it.  We will do the padding with
**	tabs where possible.
**
** Inputs:
**	string		char *		String to write
**	file		FILE *		Output file
**
** Outputs:
**	none	
**
**	Returns:
**		none
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	9/19/89 (Mike S)	Initial version
*/
VOID
IIFGwrj_writeRightJustified(string, file)
char *string;
FILE *file;
{
	i4	to_pad = max(WIDTH - STlength(string), 0);

	/* Use tabs as much as possible */		
	while (to_pad >= 8)
	{
		SIputc('\t', file);
		to_pad -= 8;
	}

	/* Now use spaces */
	while (to_pad > 0)
	{
		SIputc(' ', file);
		to_pad--;
	}

	SIfprintf(file, ERx("%s"), string);
	SIputc('\n', file);
}

/*
** output functions
*/

/*
** 	Write a line to a file
*/
static VOID
file_write(line, file)
char	*line;	/* Line to write */
PTR	file;	/* File to write to */
{
	SIfprintf((FILE *)file, "%s\n", line);
}

