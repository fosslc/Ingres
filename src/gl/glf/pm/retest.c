/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
**  retest.c - simple test program for the RE functions
**
**	Copyright (c) 1986 by University of Toronto.
**	Written by Henry Spencer.  Not derived from licensed software.
**
**	Permission is granted to anyone to use this software for any
**	purpose on any computer system, and to redistribute it freely,
**	subject to the following restrictions:
**
**	1. The author is not responsible for the consequences of use of
**		this software, no matter how awful, even if they arise
**		from defects in it.
**
**	2. The origin of this software must not be misrepresented, either
**		by explicit claim or by omission.
**
**	3. Altered versions must be plainly marked as such, and must not
**		be misrepresented as being the original software.
**
**  Usage: retest re [string [output [-]]]
**  The re is compiled and dumped, REexeced against the string, the result
**  is applied to output using REsub().  The - triggers a running narrative
**  from REexec().  Dumping and narrative don't happen unless DEBUG.
**
**  If there are no arguments, stdin is assumed to be a stream of lines with
**  five fields:  a r.e., a string to match it against, a result code, a
**  source string for REsub, and the proper result.  Result codes are 'c'
**  for compile failure, 'y' for match success, 'n' for match failure.
**  Field separator is tab.
**
**  History:
**	20-oct-92 (tyler)
**		Adapted from the original to be INGRES CL compliant.
**      23-mar-93 (smc)
**              Commented text after endifs.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**    21-apr-1999 (hanch04)
**        Replace STindex with STchr
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	06-mar-2002 (somsa01)
**	    Removed extra blank in 'PROGRAM' definition.
**      03-nov-2010 (joea)
**          Declare local functions static.
*/

# include <compat.h>
# include <gl.h>
# include <si.h>
# include <st.h>
# include <er.h>
# include <pc.h>
# include <lo.h>
# include <pm.h> /* required since re.h isn't externally includable */
# include "re.h"

#ifdef ERRAVAIL
char *progname;
FUNC_EXTERN char *mkprogname();
#endif

static void error(char *s1, char *s2);

#ifdef DEBUG
GLOBALREF i4  regnarrate;
#endif

char buf[BUFSIZ];

GLOBALDEF i4  errreport = 0;		/* Report errors via errseen? */
GLOBALDEF char *errseen = NULL;		/* Error message. */
GLOBALDEF i4  status = 0;		/* Exit status. */

/*
**
PROGRAM = retest
**
NEEDLIBS = PMLIB COMPATLIB
**
UNDEFS = II_copyright
**
*/

/* ARGSUSED */
main(argc, argv)
i4  argc;
char *argv[];
{
	RE_EXP *r;
	i4 i;

#ifdef ERRAVAIL
	progname = mkprogname(argv[0]);
#endif

# ifdef OMIT
	/*
	** This requires REsub(), which isn't double-byte clean.
	*/
	if (argc == 1) {
		multiple();
		PCexit( status );
	}
# endif /* OMIT */

	REcompile( argv[1], &r, 0 );
	if (r == NULL)
		error("REcompile failure", "");
#ifdef DEBUG
	regdump(r);
	if (argc > 4)
		regnarrate++;
#endif
	if (argc > 2) {
		i = REexec(r, argv[2]);
		SIprintf("%d", i);
		for (i = 1; i < NSUBEXP; i++)
			if (r->startp[i] != NULL && r->endp[i] != NULL)
				SIprintf(" \\%d", i);
		SIprintf("\n");
	}
# ifdef OMIT
	if (argc > 3) {
		REsub(r, argv[3], buf);
		SIprintf("%s\n", buf);
	}
# endif /* OMIT */
	PCexit( status );
}

static void
regerror(char *s)
{
	if (errreport)
		errseen = s;
	else
		error(s, "");
}

#ifndef ERRAVAIL
static void
error(char *s1, char *s2)
{
	SIfprintf(stderr, "RE_EXP: ");
	SIfprintf(stderr, s1, s2);
	SIfprintf(stderr, "\n");
	PCexit( FAIL );
}
#endif

i4  lineno;

RE_EXP badregexp;		/* Implicit init to 0. */

# ifdef OMIT

static char 
*getline( char *buf, i4  len )
{
	if( SIgetrec( buf, len, stdin ) != OK )
		return( NULL );
	buf[ STlength( buf ) - 1 ] = EOS;
	return( buf );
}

/*
** This requires REsub(), which isn't double-byte clean.
*/

multiple()
{
	char rbuf[BUFSIZ];
	char *field[5];
	char *scan;
	i4 i;
	RE_EXP *r;
	char tab = '\t';

	errreport = 1;
	lineno = 0;
	while( getline( rbuf, sizeof(rbuf) ) != NULL ) {
		rbuf[STlength(rbuf)-1] = '\0';	/* Dispense with \n. */
		lineno++;
		scan = rbuf;
		for (i = 0; i < 5; i++) {
			field[i] = scan;
			if (field[i] == NULL) {
				complain("bad testfile format", "");
				PCexit( FAIL );
			}
			scan = STchr(scan, tab);
			if (scan != NULL)
				*scan++ = '\0';
		}
		try(field);
	}

	/* And finish up with some naternal testing... */
	lineno = 9990;
	errseen = NULL;
	REcompile( (char *)NULL, &r );
	if ( r != NULL || errseen == NULL )
		complain("REcompile(NULL) doesn't complain", "");
	lineno = 9991;
	errseen = NULL;
	if (REexec((RE_EXP *)NULL, "foo") || errseen == NULL)
		complain("REexec(NULL, ...) doesn't complain", "");
	lineno = 9992;
	REcompile( "foo", &r );
	if (r == NULL) {
		complain("REcompile(\"foo\") fails", "");
		return;
	}
	lineno = 9993;
	errseen = NULL;
	if (REexec(r, (char *)NULL) || errseen == NULL)
		complain("REexec(..., NULL) doesn't complain", "");
	lineno = 9994;
	errseen = NULL;
	REsub((RE_EXP *)NULL, "foo", rbuf);
	if (errseen == NULL)
		complain("REsub(NULL, ..., ...) doesn't complain", "");
	lineno = 9995;
	errseen = NULL;
	REsub(r, (char *)NULL, rbuf);
	if (errseen == NULL)
		complain("REsub(..., NULL, ...) doesn't complain", "");
	lineno = 9996;
	errseen = NULL;
	REsub(r, "foo", (char *)NULL);
	if (errseen == NULL)
		complain("REsub(..., ..., NULL) doesn't complain", "");
	lineno = 9997;
	errseen = NULL;
	if (REexec(&badregexp, "foo") || errseen == NULL)
		complain("REexec(nonsense, ...) doesn't complain", "");
	lineno = 9998;
	errseen = NULL;
	REsub(&badregexp, "foo", rbuf);
	if (errseen == NULL)
		complain("REsub(nonsense, ..., ...) doesn't complain", "");
}

try(fields)
char **fields;
{
	RE_EXP *r;
	char dbuf[BUFSIZ];

	errseen = NULL;
	REcompile( fields[0], &r );
	if (r == NULL) {
		if (*fields[2] != 'c')
			complain("REcompile failure in `%s'", fields[0]);
		return;
	}
	if (*fields[2] == 'c') {
		complain("unexpected REcompile success in `%s'", fields[0]);
		MEfree((char *) r);
		return;
	}
	if (!REexec(r, fields[1])) {
		if (*fields[2] != 'n')
			complain("REexec failure in `%s'", "");
		MEfree((char *)r);
		return;
	}
	if (*fields[2] == 'n') {
		complain("unexpected REexec success", "");
		MEfree((char *)r);
		return;
	}
	errseen = NULL;
	REsub(r, fields[3], dbuf);
	if (errseen != NULL) {
		complain("REsub complanat", "");
		MEfree((char *)r);
		return;
	}
	if (STcompare(dbuf, fields[4]) != 0)
		complain("REsub result `%s' wrong", dbuf);
	MEfree((char *)r);
}

complain(s1, s2)
char *s1;
char *s2;
{
	SIfprintf(stderr, "try: %d: ", lineno);
	SIfprintf(stderr, s1, s2);
	SIfprintf(stderr, " (%s)\n", (errseen != NULL) ? errseen : "");
	status = 1;
}

# endif /* OMIT */

