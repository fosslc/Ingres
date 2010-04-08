/*
 ** History:
 **
 **     23-jul-1991 (donj) 
 **             Modify needlibs to allow for building on UNIX with MING.
 **	14-jun-1992 (donj)
 **		Finally got NEEDLIBS down to just what's mandatory.
 **	19-jan-1996 (bocpa01)
 **		Added NT support (NT_GENERIC)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
#include <achilles.h>

#ifdef VMS
GLOBALDEF char	    *dbname   = NULL;	     /* Optional database name */
GLOBALDEF FILE	    *envfile  = NULL;	     /* Optional environment file */
GLOBALDEF FILE	    *logfile  = NULL;	     /* Optional log file */
GLOBALDEF i4	     numtests;		     /* Number of test types */
GLOBALDEF char	    *testpath = NULL;	     /* Opt. location of frontends */
GLOBALDEF TESTDESC **tests;		     /* Array of test types */
#ifndef NT_GENERIC 
GLOBALDEF UID	     uid;		     /* Real user ID of this process */
#endif /* #ifndef NT_GENERIC */
GLOBALDEF char	     input_char, stat_char;
#else
extern char *ctime();
#endif

/*
PROGRAM = readlog
**
NEEDLIBS = ACHILLESLIB LIBINGRES
*/

main (argc, argv)
char *argv[];
{
	int fd;
	int i;
	char *ct;
	FILE *in;
	LOGBLOCK log;

	for (i = 1 ; i < argc ; i++) {
		if ( (in = fopen(argv[i], "r") ) == NULL) {
			perror(argv[i]);
			continue;
		}
		if (argc > 2)
			printf("%s:\n\n", argv[i]);
		puts("     Time           Test Child  Iter.   PID   Action       Code Runtime\n");
		while (fread((char *) &log, sizeof(log), 1, in) > 0) {
			ct = ctime(&(log.l_sec) );
			printf("%.15s.%02d  %3d   %3d   %5d  %5d  %-12s", ct+4, 
			  log.l_usec, log.l_test, log.l_child, log.l_iter, log.l_pid, 
			  log_actions[(int) log.l_action]);
			if ( (log.l_action == C_TERM) || (log.l_action == C_EXIT) )
				printf("  %3d  %3d:%02d", log.l_code, 
				  log.l_runtime / 60, log.l_runtime % 60);
			putchar('\n');
		}
		fclose(in);
	}
}
