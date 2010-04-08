/*
** Copyright (c) 1989, 2008 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include       <er.h>
# include       <lo.h>
# include       <ex.h>
# include	<st.h>
# include	<ug.h>
# include	<si.h>
# include       <ooclass.h>
# include       <abclass.h>
# include       <metafrm.h>
# include       "framegen.h"
# include       <fglstr.h>
# include       <erfg.h>


/**
** Name:	fdflddef.c - Assign field defaults for append
**
** Description:
**	This file defines:
**
**	IIFGafdAssignFieldDefaults	- Assign the field defaults from the VQ
**	...
**
** History:
**	8/11/89 (Mike S)	Initial version
**	11/20/89 (Pete)		Added routine trim_equals().
**      12-oct-1992 (daver)
**              Included header file <fe.h>; re-ordered headers according
**              to guidelines put in place once CL was prototyped.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/

/* # define's */
/* GLOBALDEF's */
/* extern's */
FUNC_EXTERN     LSTR    *IIFGaql_allocQueryLstr();
FUNC_EXTERN	VOID	IIFGbsBeginSection();
FUNC_EXTERN	VOID	IIFGesEndSection();

extern FILE *outfp;
extern METAFRAME *G_metaframe;
extern i4  G_fgqline;
extern char *G_fgqfile;

/* static's */
static const char * _equals = ERx(" =");
static char * trim_equals ();

/*{
** Name: IIFGflddefs      - Assign the field defaults from the VQ
**
** Description:
**	Assign the defaults specified in the VQ to the simple fields of an
**	Append frame.  There is no way to do this to tablefields, since we can't
**	get control when a new row is created, so currently, only the
**	argument "simple_fields" may follow the word "set_default_values".
**
** Inputs:
**      nmbr_words      i4              number of words in the following array
**      p_wordarray     char *[]        words in the query line
**      p_wbuf          char *          left-margin padding
**      infn            char *          template file name
**      line_cnt        i4              current line in template file
**
** Outputs:
**	none
**
**	Returns:
**              STATUS                  OK      if Select was produced
**                                      FAIL    otherwise
**
**      Exceptions:
**              none
**
**
** Side Effects:
**	Assignment is written to output file.
**
** History:
**	8/11/89 (Mike S) Initial version
**	2/90	(Pete)	 Check for an argument following word
**			 "set_default_values". Also changed to
**			 use same exception handling as other fg code.
**			 Add couple new warning messages.
*/
/*ARGSUSED*/
STATUS 
IIFGflddefs(nmbr_words, p_wordarray, p_wbuf, infn, line_cnt)
i4      nmbr_words;     /* Number of words in command */
char    *p_wordarray[]; /* Command line */
char    *p_wbuf;        /* Left-margin padding */
char    *infn;          /* name of input file currently processing */
i4      line_cnt;       /* current line number in input file */
{
	LSTR		*lstr0, *lstr;
	MFTAB		*table;
	MFCOL		*column;
	i4		i;
	i4		written;

	/* Be sure we're in an append frame */
	if (((USER_FRAME *)G_metaframe->apobj)->class != OC_APPFRAME) 
	{
		IIFGerr(E_FG0043_InvFrameType, FGCONTINUE,
			2, (PTR) infn, (PTR) &line_cnt);
	}

        CVlower(p_wordarray[0]);

        if (nmbr_words <= 0)
        {
            /*Note: "##generate set_default_values" already stripped*/
	    /* note that this is a warning, and processing continues */
            IIFGerr (E_FG0051_MissingArgs, FGCONTINUE, 2, (PTR) &line_cnt,
                        (PTR) infn);
        }
        else if ( !STequal(p_wordarray[0],  ERx("simple_fields")))
        {
	    /* note that this is a warning; processing continues */
            IIFGerr (E_FG0052_UnknownArg, FGCONTINUE,
                        3, (PTR) &line_cnt, (PTR) infn, (PTR) p_wordarray[0]);
        }
	else if (nmbr_words > 1)
	{
	    /* note that this is a warning, and processing continues */
            IIFGerr (E_FG0053_TooManyArgs, FGCONTINUE,
                        3, (PTR) &line_cnt, (PTR) infn, (PTR) p_wordarray[0]);
	}

        /* Set up global variables */
        G_fgqline = line_cnt;
        G_fgqfile = infn;

	/* 
	** Look for defaults.  We make two simplifying assumptions:
	**	1.  Only the primary master table has defaults, since they're
	**	    useless for lookup tables, and unimplementable for
	**	    tablefields.
	**	2. This is the first table in the "tabs" array.
	*/
	table = G_metaframe->tabs[0];
	for (i = 0, written = 0; i < table->numcols; i++)
	{
		/* 
		** Look for non-sequenced displayed or variable columns with 
		** non-null info
		*/
		column = table->cols[i];
		if (((column->flags & (COL_USED|COL_VARIABLE)) != 0) && 
		    ((column->flags & COL_SEQUENCE) == 0) && 
		    (column->info != NULL) &&
		    (*column->info != EOS))
		{
			/* Add delimiter the first time */
			if (written == 0)
			{
				IIFGbsBeginSection(outfp, 
						  ERget(F_FG0030_Default),
						  NULL, FALSE);
			}

			/* "field_name = " */
			lstr0 = IIFGaql_allocQueryLstr((LSTR *)NULL, LSTR_NONE);
			STpolycat(2, column->alias, _equals, lstr0->string);

			/* "info;\n" */
			lstr = IIFGaql_allocQueryLstr(lstr0, LSTR_SEMICOLON);
			lstr->nl = TRUE;

			lstr->string = trim_equals(column->info) ;

			/* Write it out */
			IIFGols_outLinkStr(lstr0, outfp, p_wbuf);
			written++;
		}
	}

	/* Add delimiter if we wrote any */
	if (written != 0)
	{
		IIFGesEndSection(outfp, ERget(F_FG0030_Default), NULL, FALSE);
	}

	/* If we made it through the loop, no error occured. */
	return(OK);
}

/*
** If first non-blank character is an
** equals sign (=), then skip over it in the generated code.
** Reason: we code generate our own equals sign. If user also enters one,
** then we wind up with two of them in the generated file; 4gl isn't too
** fond of double equals!
*/
static char *
trim_equals ( info)
char *info;
{
	char *p_info;

	if ( (p_info = STskipblank (info, STlength(info))) != NULL )
	{
	    /* the string contains non blank character(s) */
	    if (*p_info == '=')
		p_info++;
	}
	else
	    p_info = info;

	return p_info;
}
