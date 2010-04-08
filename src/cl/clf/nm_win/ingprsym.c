/*
**  Copyright (c) 1996, 2001 Ingres Corporation
*/

/*
**  Print Environment variable in the order ingres would search for it.
**
**  History.
**      02-May-96 (fanra01)
**          Created.
**	27-may-97 (mcgem01)
**	    Clean up compiler warnings.
**      27-Oct-97 (fanra01)
**          Add second argument as path to put output.
**	07-feb-2001 (somsa01)
**	    In resolve(), use SIZE_TYPE as the type for "len".
*/

# include <stdlib.h>

# include <compat.h>
# include <er.h>
# include <gl.h>
# include <me.h>
# include <pc.h>
# include <si.h>
# include <st.h>
# include <nm.h>
# include <lo.h>
# include "nmlocal.h"

# define DELIM_CHR_START    '$'
# define DELIM_CHR_OPEN     '{'
# define DELIM_CHR_CLOSE    '}'

/*
**  ingprsym
**
**  Description
**      Program takes the environment variable name as the argument passed
**      and uses it to obtain the value.
**      If the environment variable exists its value is displayed and the
**      program returns 0 otherwise the program returns 1.
**
** History:
**      14-Apr-1999 (fanra01)
**          Add ability for embedding variables into the requested value
**          e.g. II_CHARSET${II_INSTALLATION}.
*/

static char* resolve( char* value );

LOCATION    sFileLoc;
char        szLoc[MAX_LOC + 1];
FILE        * pTmpFile = NULL;
FILE        * pStdio = NULL;

STATUS
main ( int argc , char ** argv)
{
PTR env;
STATUS status = OK;

    if (argc < 2)
    {
        SIfprintf(stdout,"Usage : %s <environment variable> [<output file>]\n", *argv);
        PCexit(FAIL);
    }
    if (argv[2] != NULL)
    {
        STlcopy( argv[2], szLoc, sizeof( szLoc ) );
        LOfroms( PATH & FILENAME, szLoc, &sFileLoc );

        if( SIfopen( &sFileLoc, ERx( "w" ), SI_TXT, SI_MAX_TXT_REC,
                     &pTmpFile ) != OK )
        {
            SIfprintf(stderr, "Unable to open file %s\n",argv[2]);
            PCexit(FAIL);
        }
        pStdio = pTmpFile;
    }
    else
    {
        pStdio = stdout;
    }
    if ((env = getenv(argv[1])) == NULL)
    {
        /* Use the ingres allocator instead of malloc/free default (daveb) */
        MEadvise( ME_INGRES_ALLOC );

        if ( NMreadsyms() != OK )
        {
            SIfprintf( stderr, "%s: Unable to read symbols from \"%s\"\n",
                       argv[ 0 ], NMSymloc.path );
            PCexit(FAIL);
        }
        else
        {
            /* display only the value of a specified symbol */
            char *s_val;
            char* valarg;

            if ((valarg = resolve (argv[1])) == NULL)
            {
                valarg = argv[1];
            }

            NMgtIngAt( valarg, &s_val );
            if( s_val != NULL && *s_val != EOS )
            {
                SIfprintf(pStdio, ERx( "%s\n"), s_val);
            }
        }
 
        status = OK;
    }
    else
    {
        SIfprintf(pStdio, ERx( "%s\n"), env);
    }
    if (pTmpFile != NULL)
    {
        SIclose(pTmpFile);
    }
    return(status);
}

/*
** Name: Resolve
**
** Description:
**      Scan the input parameter string and resolve all variables.
**
** Inputs:
**      value   string to be resolved for variables.
**
** Outputs:
**      None.
**
** Returns:
**      resolved variable string
**      NULL if not resolved.
**
** History:
**      14-Apr-1999 (fanra01)
**          Created.
**	07-feb-2001 (somsa01)
**	    Modified type of "len" to SIZE_TYPE.
*/
static char*
resolve( char* value )
{
    STATUS	status = OK;
    char*	p;
    char*	v;              /* start of variable */
    char*	line;           /* line to be resolved */
    i4		i;
    i4		lev = 0;        /* level of nesting */
    i4		c = 0;          /* count of variables */
    i4		d = 0;          /* delimiter level */
    SIZE_TYPE	len = (value && *value) ? STlength( value ) : 0;

    /*
    ** make a copy of the string
    */
    if (value)
    {
        line = STalloc( value );
    }
    /*
    ** scan for the maximum number of levels of nesting.
    ** Use this number to determine which variable to work on first.
    */
    for (i=0, p = line; p && *p ; i+=1, p+=1)
    {
        if ((*p == DELIM_CHR_START) && (*(p+1) == DELIM_CHR_OPEN))
        {
            d+=1;
            c+=1;
            lev = max( d, lev);
        }
        else if ((*p == DELIM_CHR_CLOSE) && (d > 0))
        {
            d-=1;
        }
    }

    for (i=0, p = line; p && *p && c; p+=1)
    {
        if ((*p == DELIM_CHR_START) && (*(p+1) == DELIM_CHR_OPEN))
        {
            d+=1;
            i+=1;
            /*
            ** Rewinding the levels or resolving individual variables.
            */
            if ((i == lev) || ((lev == 0) && (i == c)))
            {
                v = p+2;
            }
        }
        else if ((*p == DELIM_CHR_CLOSE) && (d > 0))
        {
            d-=1;
            /*
            ** Rewinding the levels or resolving individual variables.
            */
            if ((i == lev) || ((lev == 0) && (i == c)))
            {
                char*		val;
                char*		tmp;
                char*		end;
                char*		begin;
                SIZE_TYPE	l;

                c-=1;
                if (lev > 0)
                {
                    lev-=1;
                }
                end = p+1;
                *p = EOS;
                NMgtAt(v, &val);
                l = (val && *val) ? STlength( val ) : 0;

                if ((tmp = MEreqmem( 0, len + l, TRUE, &status)) != NULL)
                {
                    /*
                    ** Copy string upto variable name
                    */
                    begin = tmp;
                    for (p=line; p && *p && p < (v-2); p+=1, tmp+=1)
                    {
                        *tmp = *p;
                    }
                    if (val && *val)
                    {
                        /*
                        ** Insert value for variable
                        */
                        STcat(tmp, val);
                        tmp += STlength( val );
                    }
                    /*
                    ** Complete string copy
                    */
                    for (p=end; p && *p; p+=1, tmp+=1)
                    {
                        *tmp = *p;
                    }
                    MEfree( line );     /* release previous string */
                    p = begin - 1 ;     /* adjust for for loop increment */
                    line = begin;       /* restart resolve from beginning */
                    i=0;
                }
            }
        }
    }
    return( line );
}
