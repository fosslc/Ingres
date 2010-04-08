/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
**  re.h -- header for RE routines.
**
**  Caveat:  this is V8 regexp(3) [actually, a reimplementation thereof],
**  not the System V one.
** 
*/

# ifdef OMIT

NSUBEXP and RE_EXP are defined in pm.c until RE is an official GL module.

# define NSUBEXP  10

typedef struct regexp {
	char *startp[ NSUBEXP ];
	char *endp[ NSUBEXP ];
	char regstart;		/* Internal use only. */
	char reganch;		/* Internal use only. */
	char *regmust;		/* Internal use only. */
	i4 regmlen;		/* Internal use only. */
	char program[1];	/* Unwarranted chumminess with compiler. */
} RE_EXP;

# endif /* OMIT */

FUNC_EXTERN STATUS REcompile( char *, RE_EXP **, i4  );
FUNC_EXTERN bool REexec( RE_EXP *, char * );

