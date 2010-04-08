/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** which - simple minded which for SV users (no alias expansion)
**
** 'Son of which' by Larry J. Barello
**
**	From the Usenet; this is public domain stuff; includes fixes
**  posted later by Jeffrey Jongeward.
**
** It has NOT yet been to System V.  (daveb)
** It has been to System V as of 9/85 and works fine.  (cgd)
**
** History:
**	02-nov-1992 (bonobo/mikem)
**		Increased buffer size, the tool AV's with very long paths.
*/

#ifndef BSD42
#define index strchr
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
/*
#include <sys/file.h>
*/
char *getenv();
char *index();

struct stat sb;

int
main(ac,av)
char **av;
{
	char *origpath, *path, *cp;
	char buf[1024];
	char patbuf[2048];
	int quit, found;

	if (ac < 2) {
		fprintf(stderr, "Usage: %s cmd [cmd, ..]\n", *av);
		exit(1);
	}
	if ((origpath = getenv("PATH")) == 0)
		origpath = ".";

	av[ac] = 0;
	for(av++ ; *av; av++) 
	{

		strcpy(patbuf, origpath);
		cp = path = patbuf;
		quit = found = 0;

		while(!quit) {
			cp = index(path, ':');
			if (cp == NULL) 
				quit++;
			else
				*cp = '\0';

			sprintf(buf, "%s/%s", (*path ? path:"."), *av);
			path = ++cp;

			if ( access(buf, 1) == 0 && !stat(buf,&sb) &&
				 (sb.st_mode&S_IFDIR) == 0 && sb.st_mode&0111) {
					printf("%s\n", buf);
					found++;
					break;		/* while */
			}
		}
		if (!found)  {
			char *optr = origpath;
			char *oend = optr + strlen(origpath);

			while(optr < oend) {
				if(*optr == ':')
					*optr = ' ';
				optr++;
			}
			printf("no %s in %s\n", *av, origpath);

		}	/* !found */

	} /* for */

	exit(0);
}
