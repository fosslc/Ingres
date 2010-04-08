/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	 <stype.h> 
# include	 <sglob.h> 
# include	<cm.h>
# include	<er.h>

/*
**   S_GET_SCODE - get the code for the report specifier command, if
**	specified.
**
**	Parameters:
**		cname - name of command to check in table.
**
**	Returns:
**		Code of command, if it exists.  Returns S_ERROR if
**		the command is not in the list.
**
**	Side Effects:
**		none.
**
**	Trace Flags:
**		13.0, 13.7.
**
**	History:
**		6/1/81 (ps)	written.
**		25-mar-1987 (yamamoto)
**			Modified for double byte characters.
**		6/19/89 (elein) garnet
**			Added recognition for setup/cleanup commands
**		8/21/89 (elein) garnet
**			Added recognition for include command
**		05-apr-90 (sylviap)
**			Added PScommands for support for the .PAGEWIDTH command.
**			(#129, #588)
**		24-sep-92 (rdrane)
**			Added support for 6.5 .ANSI92 expanded name space
**			(xns) command.
**		25-nov-92 (rdrane)
**			Rename .ANSI92 expanded name space (xns) command
**			to .DELIMID as per the LRC.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

	/* declaration of command table */

/*
**	Table of Report Specifier Commands.
**
**		To add commands to the report specifier, add to this
**		table (keep alphabetical) and to the SCONS.H file.
**
**	Note that this is external for presetting.
*/

struct	rspec_word
{
	char	*name;			/* command name */
	ACTION	code;			/* associated code */
};

static struct	rspec_word BScommands[] =
{
	ERx("break "),	S_BREAK,
	ERx("brk "),		S_BREAK,
	0,		0
};

static struct	rspec_word CScommands[] =
{
	ERx("cleanup "),	S_CQUERY,
	0,		0
};
static struct	rspec_word DScommands[] =
{
	ERx("dat "),			S_DATA,
	ERx("data "),			S_DATA,
	ERx("declare "),		S_DECLARE,
	ERx("delimitedidentifier "),	S_DELIMID,
	ERx("delimid "),		S_DELIMID,
	ERx("detail "),			S_DETAIL,
	0,		0
};

static struct	rspec_word EScommands[] =
{
	ERx("endrem "),	S_ENDREMARK,
	ERx("endremark "),	S_ENDREMARK,
	0,		0
};

static struct	rspec_word FScommands[] =
{
	ERx("foot "),	S_FOOTER,
	ERx("footer "),	S_FOOTER,
	ERx("footing "),	S_FOOTER,
	0,		0
};

static struct	rspec_word HScommands[] =
{
	ERx("head "),	S_HEADER,
	ERx("header "),	S_HEADER,
	ERx("heading "),	S_HEADER,
	0,		0
};
static struct	rspec_word IScommands[] =
{
	ERx("include "),	S_INCLUDE,
	0,		0
};

static struct	rspec_word LScommands[] =
{
	ERx("longremark "),	S_LREMARK,
	ERx("lrem "),	S_LREMARK,
	0,		0
};

static struct	rspec_word NScommands[] =
{
	ERx("nam "),		S_NAME,
	ERx("name "),	S_NAME,
	0,		0
};

static struct	rspec_word OScommands[] =
{
	ERx("out "),		S_OUTPUT,
	ERx("output "),	S_OUTPUT,
	0,		0
};

static struct	rspec_word PScommands[] =
{
	ERx("pw "),	S_PGWIDTH,
	ERx("pagewidth "),	S_PGWIDTH,
	0,		0
};

static struct	rspec_word QScommands[] =
{
	ERx("quel "),	S_QUERY,
	ERx("quer "),	S_QUERY,
	ERx("query "),	S_QUERY,
};

static struct	rspec_word RScommands[] =
{
	ERx("report "),	S_NAME,
	0,		0
};

static struct	rspec_word SScommands[] =
{
	ERx("setup "),	S_SQUERY,
	ERx("shortremark "),	S_SREMARK,
	ERx("sort "),	S_SORT,
	ERx("srem "),	S_SREMARK,
	ERx("srt "),		S_SORT,
	0,		0
};

static struct	rspec_word TScommands[] =
{
	ERx("table "),	S_DATA,
	0,		0
};

static struct	rspec_word VScommands[] =
{
	ERx("view "),	S_DATA,
	0,		0
};


i4
s_get_scode(cname)
char	*cname;
{
	/* internal declarations */

	register struct	rspec_word	*word;		/* ptr to command table */
	register	char		*sp = cname;	/* ptr to char */

	register	char		*wp;

	/* start of routine */



	word = NULL;
	switch(*sp)
	{
		case 'b':
			word = BScommands;
			break;

		case 'c':
			word = CScommands;
			break;
		case 'd':
			word = DScommands;
			break;

		case 'e':
			word = EScommands;
			break;

		case 'f':
			word = FScommands;
			break;

		case 'h':
			word = HScommands;
			break;

		case 'i':
			word = IScommands;
			break;

		case 'l':
			word = LScommands;
			break;

		case 'n':
			word = NScommands;
			break;

		case 'o':
			word = OScommands;
			break;

		case 'p':
			word = PScommands;
			break;

		case 'q':
			word = QScommands;
			break;

		case 'r':
			word = RScommands;
			break;

		case 's':
			word = SScommands;
			break;

		case 't':
			word = TScommands;
			break;

		case 'v':
			word = VScommands;
			break;

		default:
			break;
	}

	if (word == NULL)
	{
		return(S_ERROR);
	}
	else
	{
		for (; word->name; word++)
		{
			sp = cname;
			wp = word->name;
			while (CMcmpcase(sp, wp) == 0)
			{
				CMnext(sp);
				CMnext(wp);
			}
			if (*sp == '\0' && CMspace(wp))
			{
				return(word->code);
			}
		}
		return(S_ERROR);
	}
}
