/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
**  doasroot.c -- perform an operation as root
**
**	After building this program, you must manually set the SU bin on and
**	make the executable owned by root.
**
**  History:
**	12-jun-1998 (walro03)
**		Created.  This program has existed for ages, but only as
**		folklore.  Now it is "official".
*/
#include <stdio.h>
#include <pwd.h>

main(argc, argv)
int     argc;
char    *argv[];
{
        int             i,ret_val,ouid,uid  = -1;
        struct passwd   *pass;
        char    *p, *getenv();
        char    *nargv[5];

        pass = getpwnam("root");
        if(setuid(pass->pw_uid) == -1) {
            fprintf(stderr, "Not allowed...\n");
            exit(1);
        }
        if( ! argv[1] ) {
                nargv[0] = "/bin/sh";
                if((p=getenv("SHELL")))
                        nargv[0] = p;
                nargv[1] = "-i";
                nargv[2] = NULL;
                execvp(nargv[0],&nargv[0]);
        }
        else
                execvp(argv[1],&argv[1]);
}


