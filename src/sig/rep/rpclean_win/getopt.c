/*
 * $Id: getopt.c,v 1.1 1999/12/09 21:58:10 john Exp $
 *
 * getopt - get options from argv array
 */

/*
LIBRARY = SIGLIB
**
*/
#include <stdio.h>
#include <string.h>

char	*optarg;			/* globally visible argument pointer */
int	optind = 0;			/* globally visible argv index */

static char	*argp = NULL;	/* private argument pointer */

int getopt(int argc, char *argv[], char *optstring)
{
    register char c;
    register char *pos;
    
    optarg = NULL;
    
    if (! argp || ! *argp)
    {
        if (optind == 0)
            optind++;

        if (optind >= argc || argv[optind][0] != '-' || ! argv[optind][1])
	    return EOF;

        if (strcmp(argv[optind], "--") == 0)
        {
            optind++;
            return EOF;
        }

        argp = argv[optind]+1;
        optind++;
    }
    
    c = *argp++;
    pos = strchr(optstring, c);
    
    if (pos == NULL || c == ':')
    {
        fprintf(stderr, "%s: unknown option -%c\n", argv[0], c);
        return '?';
    }
    
    pos++;
    if (*pos == ':')
    {
        if (*argp)
        {
            optarg = argp;
            argp = NULL;
        }
        else
        {
            if (optind >= argc)
            {
                fprintf(stderr, "%s: missing argument after - %c\n",
                        argv[0], c);
                return '?';
            }
            optarg = argv[optind];
            ++optind;
        }
    }

    return c;
}
