/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include	<ctype.h>
# include	<stdio.h>
# include	<string.h>
# include	<stdlib.h>
# include      "yystrip.h"

# define	MAXLINE 300	/* maximum line length of source file */
# define	NUMYYS 	8

/*
** YYSTRIP - Strip the YY table definitions from the output of Yacc, because
**	     they overflow the DEC C compiler symbol tables.  Write each one
** out to a file with the same name and replace by an external reference.
** This is required for Equel grammars as they are the only ones that overflow
** the compile time symbols tables - there are so many rules and tokens.
**
** Note that this is VERY dependant on the output format of the PG tool and the
** Dec C compiler.
**
** History:	15-apr-1984	Written for Dec C conversion. (ncg)
**	10/28/92 (dkh) - Alpha porting changes.
**	07-jul-1995 (kosma01) 
**		Added include string.h for ris_us5. 
**      03-Sep-2009 (frima01) 122490
**          Add include of stdtlib.h and yystrip.h
**          Add return type for yyremove
*/

char	yymap[NUMYYS][10] = 
	{
		"yyexca", "yyact", "yypact", "yypgo",
		"yyr1", "yyr2", "yychk", "yydef"
	};

main(argc,argv)
	int	argc;
	char	*argv[];
{
	char	*lp, *yy;		/* index into the line array */
	char	line[MAXLINE];		/* source input line */
	FILE 	*src, *dest;	/* file pointers */
	char	yyword[10]; 
	int	ignore = 0;
	int	i;

	if (argc < 3)
	{
		printf("Usage is: YYSTRIP Infile Outfile\n");
		exit(-1);
	}

	/* 
	** Read each line of input file until an include line is found.
	** If so, remove the angle brackets and rewrite the line.
	*/
			
	if ((src = fopen(argv[1],"r")) == NULL)
	{
		printf("Cannot read file <%s>.\n", argv[1]);
		exit(-1);
	}
	if ((dest = fopen(argv[2], "w")) == NULL)
	{
		printf("Cannot write file <%s>.\n", argv[2]);
		exit(-1);
	}
	while (fgets(line, MAXLINE, src) != 0)	/* get input line */
	{
		if (ignore)
		{
			fputs(line, dest);
			continue;
		}
		if (strncmp(line, "yyparse", 7) == 0)	/* passed decls */
		{
			ignore = 1;
			fputs(line, dest);	/* writeback line to file */
			continue;
		}
		if (strncmp(line, "short", 5) != 0)	/* short yyxx[] = { */
		{
			fputs(line, dest);	/* writeback line to file */
			continue;
		}
		lp = line +5;			/* get to blanks and skip */
		for(; *lp == ' ' || *lp == '\t';)
			lp++;
		for (yy = yyword; isalnum(*lp);)
			*yy++ = *lp++;
		*yy = '\0';
		for (i = 0; i < NUMYYS; i++)	/* check if one of our names */
		{
			if (strcmp(yymap[i], yyword) == 0)
				break;
		}
		if (i < NUMYYS)			/* strip out table */
		{
			yyremove(src, yyword, line); 
		}
		fputs(line, dest);		/* writeback line to file */
	}
	fclose(src);
	fclose(dest);
	exit(1);
}				

int
yyremove(src, yyword, line)
FILE	*src;
char	*yyword;
char	*line;
{
	char	yyfile[20];
	FILE	*yydest;

	sprintf(yyfile, "%s.c", yyword);
	if ((yydest = fopen(yyfile, "w")) == NULL)
	{
		printf("Cannot write file <%s>.\n", yyfile);
		exit(-1);
	}
	fprintf(yydest, "/*\n**\t INITIALIZATION OF <%s>\n*/\n", yyword);
	/* put first line back to new file */
	fprintf(yydest, "globaldef %s", line);
	while (fgets(line, MAXLINE, src) != 0)		/* get input line */
	{
		fputs(line, yydest);
		if (strpbrk(line, ";") != NULL)		/* last line of table */
			break;
	}
	fclose(yydest);
	/* modify original source line to globalref the new table */
	sprintf(line, "globalref short %s[];\t\t/* Copied to %s.c */\n", yyword, 
		      yyword);
}
