/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: icevar.c
**
** Description:
**      Program retrieves/removes a variable from the specified context.
**
**      icevar <cookie> <action> <scope> <name> [<value>]
**
**      if the value is null the variable is retrieved.
**      if the value is not null the variable is set.
**
** History:
**      21-May-2001 (fanra01)
**          Created.
*/
/*
PROGRAM=        icevar

DEST=		bin

NEEDLIBS=	ABFRTLIB COMPATLIB C_APILIB DDFLIB ICECLILIB UGLIB SETUPLIB
*/
# include <stdio.h>
# include <iiapi.h>
# include <iceconf.h>

# include "iceutil.h"

# define    IC_STR_RET  "ret"
# define    IC_STR_SET  "set"

typedef struct _tag_params
{
    II_CHAR*    name;
    II_INT4     action;
# define IC_VAR_RET     0
# define IC_VAR_SET     1
    II_CHAR*    node;
    II_CHAR*    cookie;
    II_CHAR*    scope;
    II_CHAR*    variable;
    II_CHAR*    value;
}PARAMS;

PARAMS  progparms = { NULL, NULL, IC_VAR_RET, NULL, NULL, NULL, NULL };
II_CHAR  user[80] = "";
II_CHAR  passwd[80]= "";

/*
** Name: icevar
**
** Description:
**
**
** Inputs:
**      parms       command line parameters.
**
** Outputs:
**      None.
**
** Returns:
**      OK                          Completed successfully
**      !OK                         Failed.
**
** History:
**      23-May-2001 (fanra01)
**          Created.
*/
ICE_STATUS
icevar( PARAMS* parms )
{
    ICE_STATUS status = OK;
    II_CHAR cookie[80];
    HICECTX hicectx;

    STprintf( cookie, "ii_cookie=%s", progparms.cookie );

    if ((status = ic_initialize( NULL, NULL, NULL, cookie, &hicectx )) == OK)
    {
        switch(parms->action)
        {
            case IC_VAR_SET:
                status = ic_setvariable( hicectx, parms->scope,
                    parms->variable, parms->value );
                break;
            case IC_VAR_RET:
                status = ic_retvariable( hicectx, parms->scope,
                    parms->variable, &parms->value );
            break;
        }
        ic_close( hicectx );
    }
    if (status != OK)
    {
        IIUGerr( status, UG_ERR_ERROR, 0, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL );
    }
    else
    {
        SIprintf("%s: %s=%s\n", parms->name, parms->variable,
            ((parms->value != NULL) ? parms->value : "\0") );
    }
    return(status);
}

/*
** Name: main
**
** Description:
**      Program entry point handles processing of command line arguments.
**
** Inputs:
**      argc            Count of number of parameters
**      argv            List of parameters.
**
** Outputs:
**      None.
**
** Returns:
**      OK                          Completed successfully
**      !OK                         Failed.
**
** History:
**      23-May-2001 (fanra01)
**          Created.
*/
ICE_STATUS
main(int argc, II_CHAR** argv)
{
    ICE_STATUS  status = OK;
    II_INT4      opt = 1;

    if (argc == 1)
    {
        IIUGerr( E_WU0024_ICEVAR_USAGE, UG_ERR_ERROR, 0, NULL, NULL,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL );
        return(FAIL);
    }
    switch(argc)
    {
        case 6:
            progparms.value = argv[5];
        case 5:
            progparms.variable = argv[4];
            progparms.scope = argv[3];
            if (STcompare(argv[2], IC_STR_RET) == 0)
            {
                progparms.action = IC_VAR_RET;
            }
            else if (STcompare(argv[2], IC_STR_SET) == 0)
            {
                progparms.action = IC_VAR_SET;
            }
            progparms.cookie = argv[1];
            progparms.name = argv[0];       /* setup for use in messages */
            status = icevar( &progparms );
            break;

        default:
            IIUGerr( E_WU0024_ICEVAR_USAGE, UG_ERR_ERROR, 0, NULL, NULL,
                NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL );
            status = FAIL;
            break;
    }
    return(status);
}
