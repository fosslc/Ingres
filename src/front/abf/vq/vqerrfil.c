/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<lo.h>
# include	<si.h>
# include	<er.h>
# include	<st.h>
# include	<cm.h>
# include	<cv.h>
# include	<eros.h>
# include	<abfcnsts.h>
# include	"ervq.h"

/**
** Name:	vqerrfil.c - Parse OSL error file
**
** Description:
**	Read back the errors from an OSL error file.
**
**	IIVQcefCloseErrorFile	Close error file
**	IIVQoefOpenErrorFile	Open error file
**	IIVQgneGetNextError	Get next error from error file
**	IIVQuefUpdateErrorFile	Write new "fixed" information to error file
**
** History:
**	10/12/89 (Mike S) Initial version
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */

/* Width of the tablefield we'll scroll through error files through */
# define WINDOW_SIZE	78

/* 
**  Words in error line  --
**			"<<* Error, Section Append\Master Line 4 :"
**			 0   1      2       3             4    5
** or
*/
# define EL_SEVERITY	1
# define EL_SECTION 	3
# define EL_SECLINE	5
# define EL_WORDS	6

/* GLOBALDEF's */
/* extern's */
/* static's */
static FILE 	*error_file = NULL;		/* Error file */
i4		cur_record;			/* Current file record */
i4		cur_row;			/* Current tablefield row */

/* Indicators which begin error lines */
static char errind[10] = ERx("");
static char secerrind[10] = ERx("");
static char warncode[20] = ERx("");

static i4	line_rows();

/*{
** Name:	IIVQcefCloseErrorFile   Close error file
**
** Description:
**	Close the file.  If no file is open, do nothing gracefully.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
**	Returns:
**		STATUS from the close.
**
**	Exceptions:
**		none
**
** Side Effects:
**	The file, if open, is closed.
**
** History:
**	10/12/89 (Mike S) Initial version
*/
STATUS
IIVQcefCloseErrorFile()
{
	STATUS status;

	if (error_file == NULL)
	{
		return (OK);
	}
	else
	{
		status = SIclose(error_file);
		error_file = NULL;
		return (status);
	}
}

/*{
** Name:	IIVQoefOpenErrorFile    Open error file
**
** Description:
**	Open the file for reading.
**
** Inputs:
**	file	- LOCATION *	file to open
**
** Outputs:
**	none
**
**	Returns:
**		STATUS status from file open attempt
**
**	Exceptions:
**		none
**
** Side Effects:
**	The file is opened.
**
** History:
**	10/12/89 (Mike S)	Initial version
*/
STATUS IIVQoefOpenErrorFile(file)
LOCATION *file;
{
	STATUS status;

	/* Initialize error indicators */
	if (*errind == EOS)
	{
		STcopy(ERget(S_OS0233_ErrStart), errind);
		STcopy(ERget(S_OS0234_SecErrStart), secerrind);
		STcopy(ERget(S_OS0235_WarningName), warncode);
	}

	/* If the previous file is still open, close it */
	if (error_file != NULL) 
		_VOID_ IIVQcefCloseErrorFile();

	/* Try to open the file */
	status = SIfopen(file, ERx("r"), SI_TXT, (i4)0, &error_file);
	if (status != OK) error_file = NULL;
	cur_row = 0;
	cur_record = 0;
	return (status);
}

/*{
** Name:	IIVQgneGetNextError        Get next error from "-g" file
**
** Description:
**	Get the next error from an osl error file.
**	An error has this form:
**
**      1.      Otherwise blank line ending with a caret
**	2.	A blank line, or the word "FIXED"
**	3.	A line like the following:
**			"<<* Error, Section Append\Master Line 4 :"
**				or
**			"<<* Error, Section None Line 23:"
**				or
**			"<< Error, line 23:
**
**		The first form is an error from a "-g" listing, which is
**		inside a comment-delimited section.  The second is outside
**		such a section.  The third is from a "-l" listing.
**
**		The second word is "Warning," (in some language) for warnings.
**		These are treated differently, so we return that fact.
**
**	4.	Error summary line.
**
**	More may follow the error summary line, but we're not interested in it.
**
** Inputs:
**	summln		i4	length of summary buffer
**	secln		i4	length of section buffer
**
** Outputs:
**	sectlist	bool *	Was it a "-g" listing 
**				(i.e. does it know about sections)
**	summary		char *	Error summary buffer
**	section		char *	Section name buffer
**				(Not returned if sectlist == FALSE)	
**	errrow		i4 * Error row number (in tablefield)
**	errrecord	i4 * Error record number (in file)
**	secline		i4 *	Line number within section (from error line)
**				(Not returned if sectlist == FALSE)	
**	fixed		bool *	Was error already fixed
**	warning		bool *  Is it a warning
**
**	Returns:
**		STATUS		OK if there's no error from SI
**				ENDFILE if no errors remain.
**				Other SI errors.
**
**	Exceptions:
**		none
**
** Side Effects:
**	On end-of-file or any error, the error file is closed.
**
** History:
**	10/12/89 (Mike S) Initial version
*/
STATUS
IIVQgneGetNextError(summln, secln, sectlist, summary, section, errrow, 
		    errrecord, secline, fixed, warning)
i4  	summln;
i4  	secln;
bool	*sectlist;
char	*summary;
char	*section;
i4	*errrow;
i4	*errrecord;
i4	*secline;
bool	*fixed;
bool	*warning;
{
	char record[512];	/* Record buffer */
	char *ptr;
	STATUS status;		
	bool found;
	char	*wptr[EL_WORDS];
	i4	num_wrds;


	if (error_file == NULL) return(FAIL);	/* No error file */

	/* Loop until we find an error or run out of file */
	for (; ; )
	{
                /* Look for an otherwise blank line ending in a caret */
                if ((status = SIgetrec(record, sizeof(record), error_file))
                        != OK)
                        break;
                cur_row += line_rows(record);
		cur_record ++;
                for (ptr = record, found = FALSE; *ptr != EOS; CMnext(ptr))
                {
                        if (!CMwhite(ptr))
                        {
                                /* Found a non-whitespace character */
                                if ((*ptr == '^') && (*(ptr + 1) == '\n'))
                                        found = TRUE;
                                break;
                        }
                }
                if (!found) continue;
 
		/* Check for a blank line, or the "FIXED" indicator */
		if ((status = SIgetrec(record, sizeof(record), error_file))
			!= OK)
			break;
		cur_row += line_rows(record);
		cur_record ++;
		if (*record == '\n') 
			*fixed = FALSE;
		else if (STcompare(record, ERget(S_VQ0025_OSLFixedError)) == 0)
			*fixed = TRUE;
		else
			continue;	/* Not a possible line */

		/* Check for a line beginning with an error indicator */
		if ((status = SIgetrec(record, sizeof(record), error_file))
			!= OK)
			break;
		cur_row += line_rows(record);
		cur_record ++;
		if (STbcompare(record, 0, secerrind, STlength(secerrind), TRUE)
			== 0)
		{
			/* We found an error message with a section name */	
			*sectlist = TRUE;

			/* Parse line into words */
			num_wrds = EL_WORDS;
			STgetwords(record, &num_wrds, wptr);
			if (num_wrds < EL_WORDS)
				continue;

			/* Copy section name */
			STlcopy(wptr[EL_SECTION], section, secln);


			/* Get line number within section */
			if ((status = CVan(wptr[EL_SECLINE], secline)) != OK)
				continue;
		
			/* Check whether it's a warning */
			*warning = 
				(STcompare(warncode, wptr[EL_SEVERITY]) == 0);
		}
		else if (STbcompare(record, 0, errind, STlength(errind), TRUE)
				== 0)
		{
			/* We found an error message without a section name */
			*sectlist = FALSE;

			/* Parse line into words */
			num_wrds = EL_WORDS;
			STgetwords(record, &num_wrds, wptr);
			if (num_wrds < EL_SEVERITY + 1)
				continue;
		
			/* Check whether it's a warning */
			*warning = 
				(STcompare(warncode, wptr[EL_SEVERITY]) == 0);
		}
		else
		{
			/* We haven't found an error message after all */
			continue;
		}

		/* The next line is the error summary */
		if ((status = SIgetrec(record, sizeof(record), error_file)) 
			!= OK)
			break;
		cur_row += line_rows(record);
		cur_record ++;

		/* Fill in common output parameters */
		*errrow = cur_row - 4;
		*errrecord = cur_record - 4;
		STlcopy(record, summary, min(STlength(record) - 1, summln));
		return (OK);
	}

	/* An error occured, or we're at end of file */
	_VOID_ IIVQcefCloseErrorFile();
	return (status);
}

/*
** Determine how many tablefield rows a line will take.  We ignore the
** complications caused by word-boundary line  wrap, since the answer is almost
** certainly less than three.
*/
static i4
line_rows(line)
char 	*line;	/* Line to examine */
{
	register i4  length;
	register char *ptr;

	/* calculate line length */
	for (ptr = line, length = 0; *ptr != EOS && *ptr != '\n'; ptr++)
	{
		if (*ptr == '\t')
			length = (length / 8 + 1) * 8;
		else
			length += CMbytecnt(ptr);
	}

	/* calculate number of rows */
	if (length <= WINDOW_SIZE)
		return 1;
	else	
		return ((length - 1) / WINDOW_SIZE + 1);
}


/*{
** Name:	IIVQuefUpdateErrorFile - write new "fixed" information to file
**
** Description:
**	Given a list of errors which have changed, their line numbers,
**	and whether they are now fixed or not, rewrite the fixed indicators.
**
**		Blank for unfixed,
**		S_VQ0025_OSLFixedError for fixed.
**
** Inputs:
**	errfile		LOCATION *	listing file location
**	no_errs		i4		number of errors
**	err_records	i4[]	record numbers of errors
**					We actually change the "fixed" 
**					indicator, which is two lines further 
**					down.
**	err_fixed	bool[]		Whether fixed or not
**
** Outputs:
**	none
**
**	Returns:
**		STATUS	OK if everything worked
**			SI and LO errors otherwise
**
**	Exceptions:
**		none
**
** Side Effects:
**	The error file is rewritten
**
** History:
**	11/13/89 (Mike S)	Initial version
*/
STATUS
IIVQuefUpdateErrorFile(errfile, no_errs, err_records, err_fixed)
LOCATION	*errfile;
i4		no_errs;
i4		err_records[];
bool		err_fixed[];
{
	STATUS status;
	FILE	*oldfile = NULL;	/* Existing error file */
	FILE	*newfile = NULL;	/* New error file */
	LOCATION newloc;		/* New error location */
	char	nlbuf[MAX_LOC + 1];	/* New error location's buffer */
	i4	cur_rec;		/* Current record number */
	i4	cur_err;		/* Current error number */
	i4	left;			/* Records left to copy */
	char	record[512];
	i4	i;

	/* Open files */
	status = SIfopen(errfile, ERx("r"), SI_TXT, (i4)0, &oldfile);
	if (status != OK) goto cleanup;

	LOcopy(errfile, nlbuf, &newloc);
	status = LOuniq(ERx("vqe"), ABLISEXTENT +1, &newloc);
	if (status != OK) goto cleanup;
	status = SIfopen(&newloc, ERx("w"), SI_TXT, (i4)0, &newfile);
	if (status != OK) goto cleanup;

	/* Copy old file to new */
	cur_rec = cur_err = 0;
	while (cur_err < no_errs)
	{
		/* Copy until we get to the next record to change */
		left = err_records[cur_err] - cur_rec + 1;
		for (i = 0; i < left; i++)
		{
			status = SIgetrec(record, sizeof(record), 
					  oldfile);
			if (status != OK) goto cleanup;
			SIfprintf(newfile, ERx("%s"), record);
		}

		/* Output the "fixed" indicator */
		status = SIgetrec(record, sizeof(record), oldfile);
		if (status != OK) goto cleanup;
		SIfprintf(newfile, ERx("%s"), 
			  err_fixed[cur_err] ? 
			       ERget(S_VQ0025_OSLFixedError) : ERx("\n"));
		cur_rec = err_records[cur_err] + 2;
		cur_err++;
	}

	/* Nothing left to change.  Copy the rest */
	status = SIgetrec(record, sizeof(record), oldfile);
	while (status == OK)
	{
		SIfprintf(newfile, ERx("%s"), record);
		status = SIgetrec(record, sizeof(record), oldfile);
	}
	if (status != ENDFILE) goto cleanup;

	/* Delete the old file and rename the new one */
	_VOID_ SIclose(oldfile);
	oldfile = NULL;
	_VOID_ SIclose(newfile);
	newfile = NULL;
	LOdelete(errfile);
	status = LOrename(&newloc, errfile);
	if (status != OK)
		_VOID_ LOdelete(&newloc);
	return (status);

cleanup:
	/* Something has gone wrong.  Close the files and delete the new one */
	if (oldfile != NULL) _VOID_ SIclose(oldfile);
	if (newfile != NULL) 
	{
		_VOID_ SIclose(newfile);
		LOdelete(&newloc);
	}
	return (status);
}
