/*
**Copyright (c) 2004 Ingres Corporation
*/
/* 
 * symlink - report if and how a file is a symlink
 *
 * Synopsis:
 *	    symlink [-ltR] file
 *
 * Description:
 * 	Report if a file is a symlink.  Options allow for displaying the
 *	link target, showing the fully expanded pathname of the file
 *	or its target if a link, or recursively following links to their
 *	ultimate target.
 *
 *	Returns exit status 0 for a symlink, 1 for a non-symlinked file
 *	or directory, -1 for an error.  If neither -l or -t options
 *	are specified, output the number of symlinks encountered (0 for a
 *	regular file, 1 for a symlink, 1 or more for a symlink recursively
 *	followed.)
 *
 * Options:
 *	-t	show a trace of symlinks, e.g., foo -> /bar
 *	-l	list full path of file or target
 *	-R	follow symlinks recursively
 *
 * Diagnostics:
 *	Symlink works by executing lstat(2) and readlink(2) on files and
 *	symlinks and chdir(2) and getcwd(3) on their directories.  Errors
 *	are reported if these operations fail.
 *
 *	NOTE:  It is possible to get different exit status on the same
 *	file argument depending on options chosen. 
 *
 *	E.g., if foo is symlinked to non-existent target bar (foo -> bar),
 *		symlink foo
 *	will return "1" and succeed with exit status 0.  However,
 *		symlink -R foo
 *	will report an error and fail with exit status -1, since the attempt
 *	to recursively follow symlinks requires an lstat of bar which fails.
 *
 * History:
 *	10-nov-93 (peterk)
 *		created
 *	10-nov-93 (peterk)
 *		eliminated #include <symlink.h> and unused lines of code.
 *	17-nov-1993 (fredv)
 *		if BUFSIZ is defined, don't define it.
 *	17-nov-1993 (fredv)
 *		Stupid me, I reversed the logic last time. It should be
 *		#ifndef.
 *	13-dec-1993 (nrb)
 *		If S_ISLNK macro is not defined then define it.
 */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


char	Command[1024];
int	Islink = 0;
int	Recurse = 0;
int	List = 0;
int	Trace = 0;

#ifndef BUFSIZ
#define BUFSIZ 1024
#endif

#ifndef S_ISLNK
#define S_ISLNK( m )    (((m) & S_IFMT) == S_IFLNK)
#endif

main (argc, argv)
int     argc;
char   **argv;
{
    char path[BUFSIZ];

    strcpy (Command, *argv);
    if (argc <= 1)
	usage(*argv);

    argv++; argc--;

    for (; **argv == '-'; argv++, argc--)
    {
	char	*a;
	for (a=&argv[0][1]; *a; a++)
	    switch (*a)
	    {
	    case 'R':
		Recurse++;
		break;
	    case 'l':
		List++;
		break;
	    case 't':
		Trace++;
		break;
	    default:
		usage();
	    }
    }
	
    strcpy(path, *argv);
    if (Trace)
	printf("%s", path);
    getstat(path);
}

getstat(path)
char	*path;
{
    char	link[BUFSIZ];
    char	cwd[BUFSIZ];
    struct stat	sbuf;
    int		n;
    char	*p;
    char	*chpath();

    if (lstat(path, &sbuf))
	error(path);

    if (List || Recurse)
    {
	path = chpath(path);
    }

    if (S_ISLNK(sbuf.st_mode))
    {
	Islink++;
	if (Recurse || List || Trace)
	{
	    n = readlink(path, link, BUFSIZ-1);
	    if (n <= 0)
		error(path);
	    link[n] = '\0';
	    if (Trace)
		printf(" -> %s", link);

	    if (Recurse)
		getstat(link);	/* does not return */

	    path = chpath(link);
	}
    }

    if (Trace)
	printf("\n");

    if (List)
    {
	if (!getcwd(cwd, BUFSIZ))
	    error(cwd);
	if (*cwd == '/' && cwd[1] == '\0')
	    *cwd = '\0';
	printf("%s/%s\n", cwd, path);
    }

    if (!List && !Trace) printf("%d\n", Islink);

    if (Islink)
	exit(0);
    else
	exit(1);
}

char *
chpath(path)
char	*path;
{
    char	*p;
    p = strrchr(path, '/');
    if (p)
    {
	/* cd to path directory */
	if (p == path)
	    path = "/";	/* path directory is root */
	else
	    *p = '\0';
	if (chdir(path))
	    error(path);
	return ++p;
    }
    return path;
}

usage()
{
    fprintf (stderr, "Usage: %s file\n", Command);
    exit(-1);
}

error(s)
char *s;
{
    char	b[BUFSIZ];
    printf("\n");
    sprintf(b, "%s: %s", Command, s);
    perror(b);
    exit(-1);
}
