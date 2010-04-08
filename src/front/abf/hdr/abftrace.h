/*
** Copyright (c) 2009 Ingres Corporation
*/

/*
** Name:	abftrace.h - ABF Trace Definitions File.
**
** Description:
** A multitude of defined constants for use with the trace flags
** facility.
**
** The values, and the meanings of the flags are taken from the data in
** the trace relations of the database abf.
**
** History:
**	20-nov-1990 (sandyd) - Initial
**	31-Mar-2009 (bonro01) - Add prototypes for abftrace to fix compile
**		problems introduced by change 496819.
**
*/

/*
** TRACE SIZE
*/
# define	ABTRACESIZE	13

/*
** flags which ifdef out tTf calls
# define	txROUT
# define	txDB
# define	txOBJS
# define	txCOMP
# define	txDEBUG
# define	txDEP
# define	txRTS
*/
/*
** Flags for use with the trace routines, and
** Lots of room is left between them so that special meaning
** can be given to the different bits.
*/
/*
  Provide Information at the beginning of routines.
*/
# define        ABTROUTENT               0

/*
  Provide trace information at the end of routines.
*/
# define        ABTROUTEXIT              1

/*
  Provide Trace information about database operations.
*/
# define        ABTDBOPS                 2

/*
  Provide trace information on the compilation of source files
  of the aapplication.
*/
# define        ABTCOMPSCR              3

/*
  Provide Trace information on the compilation of image files.
*/
# define        ABTCOMPIMAGE            4

/*
  Provide trace information on the application's internal
  structure.
*/
# define        ABTAPPL                 5

/*
  Provide trace information on the internal structures for
  frames.
*/
# define        ABTFRM                  6

/*
  Provide trace information for lists.
*/
# define        ABTLIST                 7

/*
  Trace the calls to the sub process facility.
*/
# define        ABTSUBP                 8

/*
** Debugging code.
** These only use one word.
*/
/*
** Allow calls on debugging frames.
*/
# define	ABTDEBUG		9
/*
** Print errors that would normally be logged.
*/
# define	ABTLOGERR		10
/*
** Print out dependency info.
**	0 - dump procedure return type file.
**	1 - dump procedure return type file to screen.
**	2 - dump use file.
**	3 - dump uses file to screen.
**	4 - Print out state as abdepupd runs.
*/
# define	ABTDEP			11
# define	ABTRTS			12

/*
** ABF trace function prototypes
*/

VOID abtrcprint(char *fmt, char *a1, char *a2, char *a3, char *a4, char *a5, char *a6 , char *a7);
VOID abtrctime(i4    val);
