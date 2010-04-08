%{
/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: readinp.c
**
** Description:
**
** History:
**      21-Feb-2001 (fanra01)
**          Sir 103813
**          Created.
**      03-Apr-2001 (fanra01)
**          Extend filenames expression to ensure filenames start with an
**          alphanumeric and can contain path and other separators.
**          Correct -f expression to include an option white space character.
*/
# include <iiapi.h>
# include <iceconf.h>

#include "iceutil.h"

static II_INT  endoffile = 0;
static REGPARAMS    regparams = { 0 };
static II_INT   lineno = 0;
%}

%s UNIT
%s LOC
%s FLAGS
%s TYPE
%s OWNER

%%

-o[ \t]*            { BEGIN OWNER; }
<OWNER>[^ -]+       { regparams.owner = STRDUP( yytext ); BEGIN 0; }

-u[ \t]*            { BEGIN UNIT; }
<UNIT>[^ -]+        { regparams.unit = STRDUP( yytext ); BEGIN 0; }

-l[ \t]*            { BEGIN LOC; }
<LOC>[^ -]+         { regparams.location = STRDUP( yytext ); BEGIN 0; }

-f[ \t]*            { BEGIN FLAGS; }
<FLAGS>[^ -]+       { BEGIN 0;
{
    char* f = yytext;

    for ( ; *f != 0; f+=1 )
    {
        switch(*f)
        {
            case 'e':
            {
                regparams.flags |= IA_EXTERNAL;
                regparams.flags &= ~(IA_PRE_CACHE | IA_FIX_CACHE | IA_SESS_CACHE);
            }
            break;

            case 'r':
            {
                regparams.flags &= ~IA_EXTERNAL;  BEGIN FLAGS;
            }
            break;
            case 'g':
            {
                regparams.flags |= IA_PUBLIC;
            }
            break;
            case 'p':
            {
                if ((regparams.flags & IA_EXTERNAL) == 0)
                {
                    regparams.flags &= ~(IA_FIX_CACHE | IA_SESS_CACHE);
                    regparams.flags |= IA_PRE_CACHE;
                }
            }
            break;
            case 'f':
            {
                if ((regparams.flags & IA_EXTERNAL) == 0)
                {
                    regparams.flags &= ~(IA_PRE_CACHE | IA_SESS_CACHE);
                    regparams.flags |= IA_FIX_CACHE;
                }
            }
            break;
            case 's':
            {
                if ((regparams.flags & IA_EXTERNAL) == 0)
                {
                    regparams.flags &= ~(IA_PRE_CACHE | IA_FIX_CACHE);
                    regparams.flags |= IA_SESS_CACHE;
                }
            }
            break;
        }
    }
}
                    } /* FLAGS state */

-t[ \t]*            { BEGIN TYPE; }
<TYPE>[^ -]+        { BEGIN 0;
{
    char* f = yytext;

    for ( ; *f != 0; f+=1 )
    {
        switch(*f)
        {
            case 'p':
            {
                regparams.type |= IA_PAGE; regparams.type &= ~IA_FACET;
            }
            break;
            case 'f':
            {
                regparams.type |= IA_FACET; regparams.type &= ~IA_PAGE;
            }
            break;
        }
    }
}
                    } /* TYPE state */
[a-zA-Z0-9][-a-zA-Z0-9.:_/\\]+       { regparams.filename = STRDUP( yytext );
                      BEGIN 0;
                    }
[\n]                { BEGIN 0; lineno+=1; return(TRUE); }
[ \t]+

%%

/*
** Name: yywrap
**
** Description:
**      A lexer defined function that is called when an end-of-file is reached.
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      0               If more to process.  Allows the modification of
**                      yyin to a new stream.
**      1               No more to process.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
i4
yywrap()
{
    MEMFILL( sizeof(REGPARAMS), 0, &regparams );
    endoffile = 1;
    return(1);
}

/*
** Name: readinpfile
**
** Description:
**      Function performs lexical anaylsis from the input file and returns
**      when a complete line of tokens has been parsed.  The values are
**      stored within the regparms structure.
**
** Inputs:
**      f               Input stream handle.
**      p               Pointer to regparams structure.
**
** Outputs:
**      owner
**      unit
**      location
**      type
**      flags
**      filename
**
** Returns:
**      OK              Completed successfully.
**      FAIL            Lexer failed.
**
** History:
**      21-Feb-2001 (fanra01)
**          Created.
*/
ICE_STATUS
readinpfile( FILE* f, PREGPARAMS p )
{
    ICE_STATUS status = FAIL;

    /*
    ** Reset the structure for the next line
    */
    MEMFILL( sizeof(REGPARAMS), 0, &regparams );

    /*
    ** Set lex input stream to the one passed in.  Otherwise
    ** it lexer assumes stdin.
    */
    yyin = f;

    if (yylex() == TRUE)
    {
        p->owner    = regparams.owner;
        p->unit     = regparams.unit;
        p->location = regparams.location;
        p->type     = regparams.type;
        p->flags    = regparams.flags;
        p->filename = regparams.filename;
        status = OK;
    }

    return(status);
}
