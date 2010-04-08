/* $Header: /cmlib1/ingres63p.lib/unix/tools/port/others/context.c,v 1.1 90/03/09 09:18:05 source Exp $ */

/*
 * Copyright (C) 2004 Ingres Corporation 
 *
 * This program, and any documentation for it, is copyrighted by North Coast
 * Programming.  It may be copied for non-commercial use only, provided that
 * any and all copyright notices are preserved.
 *
 * Please report any bugs and/or fixes to:
 *
 *		North Coast Programming
 *		6504 Chestnut Road
 *		Independence, OH 44131
 *
 *		...decvax!cwruecmp!ncoast!bsa
 *		ncoast!bsa@Case.CSNet
 *
 *****************************************************************************
 *									     *
 *		context - display the context of located text		     *
 *									     *
 *****************************************************************************
 *
 * $Log:	context.c,v $
 * Revision 1.1  90/03/09  09:18:05  source
 * Initialized as new code.
 * 
 * Revision 1.1  89/08/01  12:36:00  source
 * sc
 * 
 * Revision 1.1  87/07/30  18:48:49  roger
 * This is the new stuff.  We go dancing in.
 * 
**		Revision 1.2  85/10/15  11:14:11  seiwald
**		exit(0) so make doesn't roll over.
**		
 * Revision 1.1  85/10/10  01:02:07  daveb
 * Initial revision
 * 
 * Revision 1.4  85/05/27  21:51:34  bsa
 * Placed in the public domain.
 * 
 * Revision 1.3  85/04/04  12:29:48  bsa
 * Made file line-number seeking smarter.
 * 
 * Revision 1.2  85/04/03  23:48:17  bsa
 * Changed header format; fixed minor bug in high range of context display.
 * 
 * Revision 1.1  85/04/03  23:26:26  bsa
 * Initial revision
 * 
**	26-Aug-2009 (kschendel) b121804
**	    Use stdlib.h to shut up the aux-info checker.
 */

#ifndef lint
static char RcsId[] = "$Header: /cmlib1/ingres63p.lib/unix/tools/port/others/context.c,v 1.1 90/03/09 09:18:05 source Exp $ [Copyright (C) 1985 NCP]";
#endif

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

char curfile[512];
FILE *curfp = NULL;
int cxtrange = 3;

char *gets();

main(argc, argv)
char **argv; {
	char context[512], fline[512], *cp, *fcp;
	long cxtln, lolino, hilino, curln;

	if (argc > 2) {
		fprintf(stderr, "Usage: context [nlines] < listfile\n");
		exit(1);
	}
	if (argc == 2)
		if ((cxtrange = atol(argv[1])) < 1 || cxtrange > 25)
			cxtrange = 3;
	while (gets(context) != (char *) 0) {
		for (cp = context; *cp != '.' && *cp != '/' && *cp != '-' && *cp != '_' && !isalnum(*cp); cp++)
			if (*cp == '\0')
				break;
		if (*cp == '\0')
			continue;
		strcpy(fline, cp);
		for (fcp = cp, cp = fline; *cp == '_' || *cp == '.' || *cp == '/' || *cp == '-' || isalnum(*cp); cp++, fcp++)
			;
		if (*cp == '\0')
			continue;
		*cp = '\0';
		if (curfp == (FILE *) 0 || strcmp(curfile, fline) != 0) {
			if (curfp != (FILE *) 0)
				fclose(curfp);
			if ((curfp = fopen(fline, "r")) == (FILE *) 0) {
				perror(fline);
				continue;
			}
			strcpy(curfile, fline);
			curln = 0;
		}
		for (; !isdigit(*fcp); fcp++)
			;
		cxtln = atol(fcp);
		lolino = (cxtln < cxtrange - 1? 1: cxtln - cxtrange);
		hilino = cxtln + cxtrange;
		if (lolino < curln) {
			fseek(curfp, 0L, 0);
			curln = 0;
		}
		if (cxtln == curln) {		/* already shown */
			printf("*****\n* %s\n*****\n\n", context);
			continue;
		}
		printf("**************\n* %s\n*****\n", context);
		while (fgets(fline, 512, curfp) != (char *) 0 && ++curln < lolino)
			;
		if (curln < lolino)
			continue;
		out(fline, curln == cxtln);
		while (fgets(fline, 512, curfp) != (char *) 0 && ++curln <= hilino)
			out(fline, curln == cxtln);
		putchar('\n');
	}
	exit(0);
}

out(s, flg)
char *s; {
	printf("%c %s", (flg? '*': ' '), s);
}
