/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
**  beingres - run a shell or command as ingres
**
**  usage:	beingres [command args ...]
**
**  History:
**	26-oct-93 (peterk)
**		created, based on rblumer's runas.c
**

MODE = SETUID

*/
#include <sys/types.h>
#include <sys/errno.h>
#include <pwd.h>

int execve();

main(argc, argv, envp)
int argc;
char **argv;
char **envp;
{
    uid_t uid = geteuid();
    struct passwd *pw;
	
    setuid(uid);
    setgid(getegid());

    if (argc < 2)
    {
	pw = getpwuid(uid);
	argv[1] = (char *)pw->pw_shell;
	argv[++argc] = (char *) 0;
    }

    execvp(argv[1], argv+1, envp);

    perror(argv[1]);
    exit(1);
}

