/*
**Copyright (c) 2004 Ingres Corporation
*/
#include <compat.h>
#include <gl.h>
#ifdef UNIX
#include <systypes.h>
#include <clconfig.h>
#endif
#include <errno.h>
#include <cm.h>
#include <er.h>
#include <lo.h>
#include <nm.h>
#include <si.h>
#include <st.h>

#include <dl.h>
#include	"dlint.h"

#define	NOMATCH	-1
#define	DL_COMMENT_LINE	1
#define	DL_MODNAME_LINE	2
#define	DL_EXENAME_LINE	3
#define	DL_VERSION_LINE	4
#define	DL_DATECREATED_LINE	5
#define	DL_FUNCTION_LINE	6
#define	DL_LIB_LINE	7

static struct parsedescs {
	char	*pr_string;
	int		pr_len;
	int	pr_ret;
} descs[] = {
	{ "comment", sizeof("comment")-1, DL_COMMENT_LINE},
	{ "dlmodname", sizeof("dlmodname")-1, DL_MODNAME_LINE },
	{ "exename", sizeof("exename")-1, DL_EXENAME_LINE },
	{ "functions", sizeof("functions")-1, DL_FUNCTION_LINE},
	{ "inputlibs", sizeof("inputlibs")-1, DL_LIB_LINE},
	{ "version", sizeof("version")-1, DL_VERSION_LINE },
	{ "date-created", sizeof("date-created")-1,
		DL_DATECREATED_LINE },
	{ NULL, 0, 0}
};

/*
** Name:	DLparsedesc
**
** Description:
** 		Parse the DL text file in "locp", filling in all of the items
**		given. It's up to the invoking routine to zero out these [output]
**		variables before invoking this routine.
**
**		IT'S ASSUMED THAT EVERY POINTER TO THIS ROUTINE IS INITIALIZED
**		BY THE INVOKING ROUTINE.
**
** Inputs:
**	locp		Location (assumed to exist) of text file to be parsed.
** Outputs:
**	exename		string from 'exename = %s' line
**	dlmodname	string from 'dlmodname = %s' line
**	vers		string from 'vers = "%s"' line. Note the double quotes.
**	creationdate		string from 'created = "%s"' line. Note quotes.
**	functionbuf	text area for functions (below) specified.
**	functions	strings from 'functions = %s [%s ...]' line(s).
**	func_cnt	Number of functions specified in 'functions = ...' line(s).
**	libbuf	text area for libraries specified.
**	libs	strings from 'libs = %s [%s ...]' line(s).
**	lib_cnt	Number of libraries specified in 'libs = ...' line(s).
**	errp		A CL_ERR_DESC that's filled in on error.
** Returns:
**	OK		The location exists, and it's a directory.
**	DL_NOT_IMPLEMENTED	Not implemented on this platform.
**	DL_LOOKUP_BAD_HANDLE	Handle is invalid for some reason.
**	other		System-specific error status, or file 
** Side effects:
**	See description.
** History
**	8-jun-93 (ed)
**	   use CL_PROTOTYPED instead of CL_HAS_PROTOS
**	19-apr-95 (canor01)
**	    added <errno.h>
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/
#ifdef CL_PROTOTYPED
STATUS DLparsedesc(LOCATION *locp, char *exename,
	char *dlmodname, char *vers, char *creationdate,
	char *functionbuf, char *functions[], i4  *func_cnt,
	char *libbuf, char *libs[], i4  *lib_cnt, CL_ERR_DESC *errp)
#else
STATUS DLparsedesc(locp, exename, dlmodname, vers, creationdate,
	functionbuf, functions, func_cnt,
	libbuf, libs, lib_cnt, errp)
LOCATION *locp;
char *exename;
char *dlmodname;
char *vers;
char *creationdate;
char *functionbuf;
char *functions[];
i4  *func_cnt;
char *libbuf;
char *libs[];
i4  *lib_cnt;
CL_ERR_DESC *errp;
#endif	/* CL_PROTOTYPED */
{
	STATUS ret;
	FILE *fp;
	char line[2*BUFSIZ], *list[BUFSIZ];
	int linesz = sizeof(line);
	i4 lineno = 0;
	char *bufp, **startp, **finalp;
	struct parsedescs *p;
	i4 cnt, expectedcnt, *cntp;

	if ((ret = SIfopen(locp, "r", SI_TXT, BUFSIZ, &fp)) != OK)
	{
			SETCLERR(errp, ret, 0);
			return(ret);
	}
	while (SIgetrec(line, linesz=sizeof(line), fp) != ENDFILE)
	{
		i4 len;

		lineno++;
		_VOID_ STtrmwhite(line);
		len = STlength(line);
		while (line[len-1] == '\\')
		{
			lineno++;
			if (SIgetrec(&line[len-1], linesz-len, fp) == ENDFILE)
			{
				DLdebug("Malformed line %d: \"%s\"\n", lineno, line);
				SETCLERR(errp, DL_MALFORMED_LINE, 0);
				return(ret);
			}
			_VOID_ STtrmwhite(line);
			len = STlength(line);
		}
		for (p = descs;
			       p->pr_string != NULL &&
		           STbcompare(p->pr_string,p->pr_len,
				   line,sizeof(line),TRUE) != 0; p++)
			/* NULL */;
		if (p->pr_string == NULL)
		{
			DLdebug("Unrecognized line %d \"%s\" ignored\n", lineno, line);
			continue;
		}
		if (p->pr_ret == DL_COMMENT_LINE)
			continue;
		bufp = STchr(line, '=');
		if (bufp == NULL)
		{
			SETCLERR(errp, DL_MALFORMED_LINE, 0);
			return(DL_MALFORMED_LINE);
		}
		bufp = CMnext(bufp);
		startp = list;
		cntp = &cnt;
		finalp = NULL;
		expectedcnt = 0;
		switch (p->pr_ret)
		{
			case DL_FUNCTION_LINE:
				finalp = NULL;
				startp = functions;
				STcopy(bufp, functionbuf); bufp = functionbuf;
				cntp = func_cnt;
				expectedcnt = 0;
				break;
			case DL_LIB_LINE:
				finalp = NULL;
				startp = libs;
				STcopy(bufp, libbuf); bufp = libbuf;
				cntp = lib_cnt;
				expectedcnt = 0;
				break;
			default:
				*cntp = 0;
				DLdebug("Ignoring \"%s\"\n", line);
				break;
			case DL_MODNAME_LINE:
				*cntp = 1; expectedcnt = 1;
				finalp = &dlmodname;
				break;
			case DL_VERSION_LINE:
				*cntp = 1; expectedcnt = 1;
				finalp = &vers;
				break;
			case DL_EXENAME_LINE:
				*cntp = 1; expectedcnt = 1;
				finalp = &exename;
				break;
			case DL_DATECREATED_LINE:
				*cntp = 1; expectedcnt = 1;
				finalp = &creationdate;
				break;
		}
		if (*cntp > BUFSIZ)
			*cntp = BUFSIZ;
		if (*cntp > 0)
		{
			i4 incnt = *cntp;
			STgetwords(bufp, cntp, startp);
			if (expectedcnt > 0 && expectedcnt != *cntp)
			{
				SETCLERR(errp, DL_MALFORMED_LINE, 0);
				return(DL_MALFORMED_LINE);
			}
			if (finalp)
			{
				while (cnt-- > 0)
					STcopy(*startp++, *finalp++);
			}
		}
	}
	DLdebug("Parsed text file, module name was \"%s\"\n",
		dlmodname[0] ? dlmodname : "<unspecified>");
	DLdebug("Executable filename was \"%s\"\n",
		exename[0] ? exename : "<unspecified>");
	DLdebug("Created on \"%s\"\n",
		creationdate[0] ? creationdate : "<unspecified>");
	DLdebug("Version \"%s\"\n",
		vers[0] ? vers : "<unspecified>");
	{
		i4 i;

		for (i = 0; i < *func_cnt; i++)
			if (functions[i])
				DLdebug("Function %d in module: \"%s\"\n", i, functions[i]);
		for (i = 0; i < *lib_cnt; i++)
			DLdebug("Library %d in module: \"%s\"\n", i, libs[i]);
	}
	_VOID_ SIclose(fp);
	return(OK);
}
