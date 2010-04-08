/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<st.h>
# include	<stype.h>
# include	<sglob.h>
# include	<cm.h>
# include	<er.h>
# include	"ersr.h"

static	STATUS	do_param();	/* function called once per parameter */
static	i4	a_strlen();
static	VOID	a_strcat();

/*
**   S_W_ACTION - put a command and some number of string parameters in the
**	Ccommand and Ctext fields for use by s_w_row.
**
**	Parameters:
**		command		string containing command.
**		param1,param2.. strings containing pieces of parameters
**				ended with a zero.
**
**	Returns:
**		STATUS. OK   -- If successful.
**			FAIL -- If not successful.
**
**	Error Messages:
**		Syserr: Bad command.
**
**	Trace Flags:
**		13.0, 13.5.
**
**	History:
**		4/4/82 (ps)	written.
**		1/17/83 (gac)	insert extra '\' for '\' because it gets
**					stripped out with INGRES copy.
**		8/5/83	(gac)	bug 1452 fixed -- large trim causes syserr.
**
**		7/12/86 (roger) Replaced code to handle variable no. of arg-
**				uments that made assumptions about stack layout.
**				This routine now takes a maximum of 9 arguments
**				(including command).  It always (and still does)
**				expected a minimum of 2 (command and null param.
**				if no params for that command).
**				Hopefully we will figure out a better long-term
**				solution (e.g., providing a "varargs" facility).
**		2/16/90 (martym) Changed s_w_action() and do_param() to return 
**				STATUS, since s_w_row() now returns STATUS.
**		3/20/90 (elein) performance
**				Change STcompare to STequal
**		3-aug-1992 (rdrane)
**			Re-design the parameter processing loop.  The ANSI
**			compiler complained (it never did execute the loop more
**			than once), so load the parameters into a local array
**			and loop through it.  The additional memory should be
**			offset by the elimination of all the in-line code, and
**			the performance penalty for loading the array should be
**			very small.  Eliminate references to r_syserr() and use
**			IIUGerr() directly.  Eliminate extern declaration of
**			s_w_row() - it's now in hdr file.  do_param, a_strcat,
**			and strlen are all static to this module.  Change int's
**			to more proper nat's.
**		27-oct-1992 (rdrane)
**			Oops!  Forget to initialize the loop index ...
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


STATUS
s_w_action(command, p1, p2, p3, p4, p5, p6, p7, p8)
char	*command;
char	*p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8;
{

	register i4	i;
		bool		flag = FALSE;	/* storage used by do_param() */
		STATUS		stat;
		char		*p_ptr[(8 + 1)];


	if (command == NULL)
	{
		IIUGerr(E_SR0012_s_w_action_Null_cmd,UG_ERR_FATAL,0);
	}


	STcopy(command, Ccommand);
	Ctext[0] = '\0';

	p_ptr[0] = p1;
	p_ptr[1] = p2;
	p_ptr[2] = p3;
	p_ptr[3] = p4;
	p_ptr[4] = p5;
	p_ptr[5] = p6;
	p_ptr[6] = p7;
	p_ptr[7] = p8;
	p_ptr[8] = NULL;
	i = 0;
	while (p_ptr[i] != NULL)
	{
		if  ((stat = do_param(p_ptr[i],&flag)) != OK)
		{
			return(stat);
		}
		i++;
	}

	return(s_w_row());
}



/*
** History:
**
**	30-oct-1992 (rdrane)
**		This routine is only called by RBF, which now
**		generates only single quoted strings.  However,
**		for backwards compatibility, look for either character
**		at the start of a parameter and set a quote character
**		accordingly.
**	2-jul-1993 (rdrane)
**		Corrections for RBF support of user override of the
**		hdr/ftr timestamp/page #.  Specifically, when breaking
**		up long string use single quotes!
*/

static
STATUS
do_param(param, instring)
char *param;
bool *instring;
{
	char	quote_str[2] = "";
	STATUS	stat = OK;


	if  ((*instring) && (STequal(param,quote_str)))
	{
		*instring = FALSE;
	}
	else if ((!(*instring)) &&
		 ((STequal(param,ERx("\'"))) ||
		  (STequal(param,ERx("\"")))))
	{
		STcopy(param,&quote_str[0]);
		*instring = TRUE;
	}

	if (*instring)
	{
		while ((STlength(Ctext) + a_strlen(param)) > (MAXRTEXT - 1))
		{	/*
			** String won't fit.  Put it out in parts.
			*/
			a_strcat(Ctext, &param, MAXRTEXT-1);
			STcat(Ctext, ERx("\'"));
			if  ((stat = s_w_row()) != OK)
			{
				return(stat);
			}
			STcopy(ERx("\'"), Ctext);
		}
	}
	else if ((STlength(Ctext) + a_strlen(param)) > MAXRTEXT)
	{	/*
		** Won't fit. Put out command
		 */
		if  ((stat = s_w_row()) != OK)
		{
			return(stat);
		}
		Ctext[0] = '\0';
	}

	a_strcat(Ctext,&param,MAXRTEXT);

	return(OK);
}


static
i4
a_strlen(s)
register char *s;
{
	register i4	length = 0;

	while (*s)
	{
/*
** KJ 03/25/87 (TY)
*/
		CMbyteinc(length, s);
		if (*s == '\\')
			length++;
		CMnext(s);
	}
	return(length);
}

static
VOID
a_strcat(s, t, maxlen)
char	*s;
char	**t;
i4	maxlen;
	/* Concatenate t to s, inserting extra '\' for every '\'.
	** If the length of s exceeds maxlen, cut off s at maxlen.
	** t will point to after the last character concatenated. */
{
	register char	*sp;
	register char	*tp;
	register i4	ml;

	sp = s + STlength(s);
	tp = *t;
	ml = maxlen - STlength(s);
	while (*tp && ml > 0)
	{
		CMcpychar(tp, sp);
		CMbytedec(ml, tp);

		if (*tp == '\\')
		{
			if (ml > 2)
			{
				CMnext(sp);
				CMnext(tp);
				*sp++ = '\\';
				ml--;
				CMcpychar(tp, sp);
				CMbytedec(ml, tp);
				CMnext(sp);
				if (*tp == '\\')
				{
					*sp++ = '\\';
					ml--;
				}
				CMnext(tp);
			}
			else
			{
				break;
			}
		}
		else
		{
			CMnext(sp);
			CMnext(tp);
		}
	}
	*sp = '\0';
	*t = tp;

	return;
}
