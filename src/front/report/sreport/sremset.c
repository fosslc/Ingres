
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<stype.h>
# include	<sglob.h>
# include	<st.h>
# include	<cm.h>
# include	<oocat.h>

/**
** Name:	sremset.c	contains the routines s_srem_set and s_lrem_set
**				to process the short and long remark commands.
**
** Description:
**	This file defines:
**
**	s_srem_set	processes the short remark command.
**	s_lrem_set	processes the long remark command.
**
** History:
**	2-apr-87 (grant)	created.
**	25-nov-1987 (peter)
**		Add globals St_sr_given, St_lr_given.
**	4/19/90 (elein) 20246
**		Check validity of Cact_ren before using
**      11-oct-90 (sylviap)
**              Added new paramter to s_g_skip.
**	8/1/91 (elein) 38159
**		Tabs can get this far as of 6.3/03.  They
**		used to be substituted with a space.  However
**		the tab expansion routine no longer works (if
**		if ever did?) and so for short and long remarks
**		we are reverting to the space substitution.
**		methods.
**	26-aug-1992 (rdrane)
**		Converted s_error() err_num value to hex to facilitate
**		lookup in errw.msg file.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/* define's */
/* extern's */


/*{
** Name:	s_srem_set	processes the short remark command.
**
** Description:
**	This routine processes the short remark command. The syntax is:
**
**		.SHORTREMARK <remark is any character up to the newline>
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**		OK	if OK
**		FAIL	if error occurred.
**
** Side Effects:
**	Cact_ren->ren_shortrem gets set to the string found.
**	Tokchar gets updated.
**
** History:
**	2-apr-87 (grant)	implemented.
*/

STATUS
s_srem_set()
{
    char	*s;
    char	*send;
    i4		pos;
    i4		n;
    char	short_remark[OOSHORTREMSIZE+1];

    if (St_sr_given)
    {
	s_error(0x3A8, NONFATAL, NULL);
	s_cmd_skip();
	return FAIL;
    }
    if (Cact_ren == NULL)
    {
	    s_error(0x38A, FATAL, NULL);
    }

    St_sr_given = TRUE;

    Tokchar++;
    while (CMspace(Tokchar) || *Tokchar == '\t')
    {
	CMnext(Tokchar);
    }

    s = short_remark;
    send = s + OOSHORTREMSIZE;
    while (*Tokchar != '\n' && *Tokchar != EOS)
    {
        if (s <= send-CMbytecnt(Tokchar))
	{
	    if (*Tokchar == '\t')
	    {
		/* if tab, replace it with the one blank */
		*s = ' ';
		s++;
		Tokchar++;
	    }
	    else
	    {
		CMcpyinc(Tokchar, s);
	    }
	}
	else
	{
	    CMnext(Tokchar);
	}
    }

    *s = EOS;
    Cact_ren->ren_shortrem = STalloc(short_remark);

    return OK;
}

/*{
** Name:	s_lrem_set	processes the long remark command.
**
** Description:
**	This routine processes the long remark command. The syntax is:
**
**		.LONGREMARK      Leading white space will be ignored.  Anything
** between the commands is a remark which can span an arbitrary number of lines.
** Newlines will converted to blanks.  Tabs will be converted to an appropriate
** number of blanks.  Spacing will be preserved.  .ENDREMARK
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**		OK	if OK
**		FAIL	if error occurred.
**
** Side Effects:
**	Cact_ren->ren_longrem gets set to the characters of the remark
**	encountered.
**	Tokchar gets updated.
**
** History:
**	3-apr-87 (grant)	implemented.
**	
**	15-Jan-2000 (inifa01)  bug 103512 INGCBT321
**		sreport db_name rep_file.rw seg faults when long remark
**		string in report file contains a period.
**		The buffer allocated to hold this string expects the 
**		the string to be no longer than 12 characters. The length
**		of the string could be much longer if in remark string.
**		Increased buffer size.
*/

STATUS
s_lrem_set()
{
    char	*b;
    char	*bend;
    i4		pos;
    i4		n;
    i4		length;
    # ifdef UNIX
    char	word[OOLONGREMSIZE+1]; 
    # else
    char        word[MAXCMD+1];
    # endif
    i4		code;
    i4		count;
    char	*save_Tokchar;
    char	long_remark[OOLONGREMSIZE+1];
    i4          rtn_char;               /* dummy variable for sgskip */

    if (St_lr_given)
    {
	s_error(0x3A9, NONFATAL, NULL);
	s_cmd_skip();
	return FAIL;
    }
    if (Cact_ren == NULL)
    {
	    s_error(0x38A, FATAL, NULL);
    }
    St_lr_given = TRUE;

    /* skip leading white space */

    s_g_skip(TRUE, &rtn_char);

    /* handle rest of remark up to the .ENDREMARK */

    code = S_ERROR;
    b = long_remark;
    bend = b + OOLONGREMSIZE;

    for (;;)
    {
	while (*Tokchar != '\n' && *Tokchar != EOS)
	{
	    if (*Tokchar == '.' && CMalpha(Tokchar+1))
	    {
		/* check for .ENDREMARK */

		save_Tokchar = Tokchar;
		Tokchar++;
		r_gt_word(word);
		CVlower(word);
		code = s_get_scode(word);
		if (code == S_ENDREMARK)
		{
		    break;
		}
		else
		{
		    Tokchar = save_Tokchar;
		}
	    }

	    if (b <= bend-CMbytecnt(Tokchar))
	    {
		if (*Tokchar == '\t')
		{
			/* if tab, replace it with the one blank */
			*b = ' ';
			b++;
			Tokchar++;
		}
		else
		{
		    CMcpyinc(Tokchar, b);
		}
	    }
	    else
	    {
		CMnext(Tokchar);
	    }
	}

	if (code == S_ENDREMARK)
	{
	    *b = EOS;
	    Cact_ren->ren_longrem = STalloc(long_remark);
	    break;
	}
	else
	{
	    if (b < bend)
	    {
	        *b++ = ' ';
	    }
	    count = s_next_line();
	    if (count == 0)
	    {
		/* ERROR: EOF in middle of remark */
		s_error(0x3A3, FATAL, NULL);
		break;
	    }
	}
    }

    return OK;
}
