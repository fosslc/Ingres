/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<cm.h>
# include       "rbftype.h"
# include       "rbfglob.h"
# include	"errf.h"
# include       <flddesc.h>

/*
**   RFUTILS - conatins utility routines for RBF
**
**   Defines:
**   IIRFfwd_FullWord		- matches the word with a partial string;
**			 	  used for matching abbreviations with full
**				  words.
**   IIRFwid_WideReport		- prompts user if RBF should create a report
**				  beyond En_lmax.
**   IIRFisl_IsLabel 		- returns TRUE if the report is a label report.
**   IIRFckc_CheckComment 	- returns TRUE if string is pointing to the 
**				  exact words:
**					/* DO NOT MODIFY.
**				  then skips over the next 8 words - should
**				  be:
**				     DOING SO MAY CAUSE REPORT TO BE UNUSABLE
**	
**	History:
**	20-feb-90 (sylviap)	
**		Created.
**	02-apr-90 (sylviap)	
**		Added IIRFwid_WideReport.
**	4/3/90 (martym)
**		Added a parameter to IIRFwid_WideReport() to indicate 
**		how many spaces are required for the current report.
**	29-aug-90 (sylviap)	
**		Added IIRFisl_IsLabel.
**	09-nov-90 (sylviap)	
**		Added IIRFckc_CheckComment. (#33894)
**	01-jun-95 (peeje01)
**		Cross integration of Double Byte Character Set (DBCS) changes
**		From ingres63p 6405pdb_su4_us5:
** 		17-jan-1994 (poh)
**			Take out the checking of E_RW1413_do_not_modify as
**			it is not portable across system with translated msg. 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

FUNC_EXTERN char        *IIRFfwd_FullWord();


char *
IIRFfwd_FullWord(part_wrd, index, wrd_tbl)
char	*part_wrd;		/* input string */
i4	index;			/* index into the wrd_tbl */
char	**wrd_tbl;		/* array of full words to match against 
				** part_wrd */
{

	i4	i;
	i4	in_len, out_len;

	/* get length of partial string */
	in_len = STlength(part_wrd);

	/* search for matching word in array */
	for (i = 0; i < index; i++)
	{
		out_len = STlength(part_wrd);
		/* if a match is found, then return pointer to full word */
		if (STbcompare (part_wrd,in_len,wrd_tbl[i],out_len,TRUE ) == 0)
			return (char *)wrd_tbl[i];
	}
	/* no match found */
	return (char *)(NULL);
}

/*
**   IIRFwid_WideReport
**	Prompts user to create a report beyond En_lmax (-l flag)
**
**   Returns:
**   OK		- build report
**   FAIL	- do not build report
**
**	History:
**	02-apr-90 (sylviap)	
**		Created.
*/

STATUS
IIRFwid_WideReport(NeedWidth, MaxWidth)
i4  	NeedWidth;
i4	MaxWidth;
{
	char	help_scr[FE_PROMPTSIZE + 1];
	char	prompt[FE_PROMPTSIZE + 1];
	char	*choices;	
	i4	i;

	/*
	** Display Popup with the 'break column' options.  User
	** may choose yes or no.
	*/

	STcopy (ERget(S_RF0085_wide_report), help_scr);
	_VOID_ STprintf(prompt, ERget(S_RF0087_wide_rep_title), NeedWidth,
		MaxWidth);
	choices   = ERget(F_RF006B_wide_rep_choices);
	i = IIFDlpListPick(prompt, choices, 0,
		LPGR_FLOAT, LPGR_FLOAT, help_scr, H_WIDREP,
		NULL,  NULL);
	if (i == 0)
		return (OK);
	else
		return (FAIL);
}

/*
**   IIRFisl_IsLabel
**	Returns TRUE if the report is a label report.  Needed for rfvifred, 
**	to know if can create a column heading.
**
**   Returns:
**	TRUE if St_style  = RS_LABELS
**	FALSE otherwise
**
**   	St_style = 	0 - RS_NULL
**			1 - RS_DEFAULT
**			2 - RS_COLUMN
**			3 - RS_WRAP
**			4 - RS_BLOCK
**			5 - RS_LABELS
**			6 - RS_TABULAR
**			7 - RS_INDENTED
**			8 - RS_MASTER_DETAIL
**
**	History:
**	29-aug-90 (sylviap)	
**		Created.
*/

bool
IIRFisl_IsLabel()
{
	if (St_style == RS_LABELS)
		return (TRUE);
	else
		return (FALSE);
}

/*
**   IIRFckc_CheckComment
**	Returns TRUE if string is pointing to the exact words:
**		/* DO NOT MODIFY
**
**
**   Returns:
**	TRUE  if string is pointing to '/* do not modify'
**	FALSE otherwise
**
**	end_str is pointing to the end of the comment
**
**	History:
**	06-nov-90 (sylviap)	
**		Created.
*/

bool
IIRFckc_CheckComment(str, end_str)
char	*str;
char	**end_str;
{
	i4  token_type;           /* used for parsing */
	i4 i;
	char *tmp_char = NULL;

	r_g_set (str);             /* set up parsing string */

	token_type = r_g_skip();   /* looking for a backslash */
	if (token_type != TK_MULTOP)
		return (FALSE);
	CMnext (Tokchar);

# ifndef DOUBLEBYTE
	token_type = r_g_skip();   /* looking for a asterisk */
				   /* could be "**" or "* *" */
	if ((token_type != TK_POWER) && (token_type != TK_MULTOP))
		return (FALSE);
	CMnext (Tokchar);

	token_type = r_g_skip();   /* looking for second asterisk */
	if (token_type != TK_MULTOP)
		return (FALSE);
	CMnext (Tokchar);

	token_type = r_g_skip();   /* looking for the word "do" */
	if (token_type != TK_ALPHA)
		return (FALSE);
	tmp_char = r_g_name();
	if (STbcompare(tmp_char,0, ERx("do"),0, TRUE) != 0)
		return (FALSE);

	token_type = r_g_skip();   /* looking for the word "not" */
	if (token_type != TK_ALPHA)
		return (FALSE);
	tmp_char = r_g_name();
	if (STbcompare(tmp_char,0, ERx("not"),0, TRUE) != 0)
		return (FALSE);

	token_type = r_g_skip();   /* looking for the word "modify" */
	if (token_type != TK_ALPHA)
		return (FALSE);
	tmp_char = r_g_name();
	if (STbcompare(tmp_char,0, ERx("modify"),0, TRUE) != 0)
		return (FALSE);

	token_type = r_g_skip();   /* looking for a period */
	if (token_type != TK_PERIOD)
		 return (FALSE);
	CMnext (Tokchar);

	for (i = 0; i < 8; i++)
	{
		/* Skip over 'DOING SO MAY CAUSE REPORT TO BE UNUSABLE' */
		 token_type = r_g_skip();   /* skip any white space */
		 if (token_type != TK_ALPHA)
			 return (FALSE);
		 tmp_char = r_g_name();

	}

	token_type = r_g_skip();   /* looking for a period */
	if (token_type != TK_PERIOD)
		 return (FALSE);
	CMnext (Tokchar);
	*end_str = Tokchar;	   /* return a ptr AFTER the comment */
# else
	/* This routine used to be comparing Tokchar with the hard coded 
	** DO NOT MODIFY... string. For double byte build, decided not 
	** to check for this comment as it makes reports not portable on
	** system with translated msg. Instead, search for the "*" char
	** before the end of comment which enclose the M/D type 
	** description for the Joindef comment block. For the Union Cluse
	** comment block, this will enclose F_RF008A_union_comment2. 
	** However the routine (r_JDLoadQuery) that check for Union Cluse
	** comment block will just skip this until the end of the comment */

	for ( ; *Tokchar != '\0'; )
	{
		if (CMcmpcase(Tokchar, "*") == 0)
		{
			/* Stop scaning if end of comment */ 
			if (CMcmpcase(Tokchar + 1, "/") == 0) 
				break;
			else
			/* Store the current instance of "*" and 
			** continue searching	*/
				tmp_char = Tokchar;
		}
		CMnext(Tokchar);
	}

	/* Can't find end of comment */
	if (*Tokchar == '\0') 
		return (FALSE);

	/* return a ptr AFTER the comment */
	*end_str = (tmp_char != NULL) ? tmp_char : Tokchar; 
# endif

	return (TRUE);
}
