/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: icesvrloc
**
** Description:
**      Command line program to register ice database users.
**      Provided to configure ice server for backward compatibility.
**
** History:
**      14-Jan-2000 (fanra01)
**          Bug 100036
**          Created.
**	21-Jan-2000 (somsa01)
**	    Modified include libraries for MING.
*/
/*
PROGRAM=	icesvrloc

DEST=		bin

NEEDLIBS=	ABFRTLIB COMPATLIB C_APILIB DDFLIB ICECLILIB UGLIB
*/

# include <compat.h>
# include <cm.h>
# include <si.h>
# include <st.h>

# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>

# include <ice_c_api.h>

# include "erwu.h"

# define MAX_MSG_SIZE       256
# define E_USAGE            2
# define E_MISSING_MESSAGE  3
# define E_UNKNOWN_OPTION   4
# define E_DUPLICATE        5
# define E_INVALID_ACTION   6
# define E_UNNAMED_TARGET   7

char* actions[] = { "select", "insert", "update", "delete", NULL };
char* loctype[] = { "HTTP", "ICE ", "PUBLIC" };

ICE_C_PARAMS locselect[] =
{
    {ICE_IN,  "action",             "select"},
    {ICE_OUT, "loc_id",             NULL    },
    {ICE_OUT, "loc_name",           NULL    },
    {ICE_OUT, "loc_path",           NULL    },
    {ICE_OUT, "loc_extensions",     NULL    },
    {ICE_OUT, "loc_http",           NULL    },
    {ICE_OUT, "loc_ice",            NULL    },
    {ICE_OUT, "loc_public",         NULL    },
    {0,       NULL,                 NULL    }
};

ICE_C_PARAMS svrloc[] =
{
    {ICE_IN,            "action",           NULL    },
    {ICE_IN | ICE_OUT,  "loc_id",           NULL    },
    {ICE_IN | ICE_OUT,  "loc_name",         NULL    },
    {ICE_IN | ICE_OUT,  "loc_path",         NULL    },
    {ICE_IN | ICE_OUT,  "loc_extensions",   NULL    },
    {ICE_IN | ICE_OUT,  "loc_http",         NULL    },
    {ICE_IN | ICE_OUT,  "loc_ice",          NULL    },
    {ICE_IN | ICE_OUT,  "loc_public",       NULL    },
    {0,      NULL,                  NULL }
};

# define    LOC_ITEMS   (sizeof(svrloc) / sizeof(ICE_C_PARAMS)) - 1

static char values[LOC_ITEMS][100] = { "\0" };
static char usevalues[LOC_ITEMS][100] = { "\0" };

/*
** Name: getermessage
**
** Description:
**      Function looks up the message text for the err or id and returns a
**      copy of it.
**
** Inputs:
**      id      ER error id
**
** Outputs:
**      msg     pointer to the error message
**
** Returns:
**      OK      Success
**      !OK     Failure
**
** History:
**      16-Jan-2000 (fanra01)
**          Created.
*/
STATUS
getermessage( ER_MSGID id, i4 flags, PTR* msg )
{
    STATUS      status = OK;
    CL_ERR_DESC clerror;
    char        msgstr[MAX_MSG_SIZE];
    i4          msglen = 0;

    *msg = NULL;
    if ((status = ERslookup ( id, NULL, flags, NULL, msgstr, sizeof(msgstr),
        -1, &msglen, &clerror, 0, NULL )) == OK)
    {
        *msg = STalloc( msgstr );
    }
    return(status);
}

/*
** Name: iceapierror
**
** Description:
**      Displays the error returned from the ice api.
**
** Inputs:
**      errstr  Error string returned from ICE_C_LastError
**
** Outputs:
**      None.
**
** Returns:
**      None.
**
** History:
**      16-Jan-2000 (fanra01)
**          Created.
*/
VOID
iceapierror( PTR errstr )
{
    char*       msg;

    if (getermessage( E_WU0005_ICE_API, 0, &msg ) == OK)
    {
        SIprintf( msg, errstr );
        MEfree( msg );
    }
    return;
}

/*
** Name: enumaction
**
** Description:
**      Enumerates the action string.
**
** Inputs:
**      action  action string select/insert/update/delete
**
** Outputs:
**      None.
**
** Returns:
**      enumerated action value     sucess
**      E_NIVALID_ACTION            failure
**
** History:
**      16-Jan-2000 (fanra01)
**          Created.
*/
i4
enumaction( char* action )
{
    i4  retval = E_INVALID_ACTION;
    i4  i;

    for(i = 0; actions[i] != NULL; i+=1)
    {
        if (STbcompare( actions[i], 0, action, 0, TRUE ) == 0)
        {
            retval = i;
            break;
        }
    }
    return(retval);
}

/*
** Name: actionsvrloc
**
** Description:
**      Function prompts for admin user and password and connects to the
**      ice server to perform the specified action.
**
** Inputs:
**      svrloc     array of ICE_C_PARAMS
**
** Outputs:
**      None.
**
** Returns:
**      OK                  success
**      E_INVALID_ACTION
**      E_MISSING_MESSAGE
**      !0
**
** History:
**      16-Jan-2000 (fanra01)
**          Created.
*/
STATUS
actionsvrloc( ICE_C_PARAMS* sloc )
{
    STATUS          retval = OK;
    ICE_C_CLIENT    client = NULL;
    ICE_STATUS      status = 0;
    char            admin[80];      /* admin user */
    char            password[80];   /* admin user password */
    i4              i;
    i4              entryfound = 0;
    i4              actval = enumaction( sloc[0].value );
    char*           msg = NULL;

    if (actval == E_INVALID_ACTION)
    {
        IIUGerr( E_WU0003_INVALID_ACTION, UG_ERR_ERROR, 0, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL );
        retval = actval;
        return(retval);
    }

    if (getermessage( S_WU0006_ADMIN_USER, ER_TEXTONLY, &msg ) == OK)
    {
        IIUGprompt( msg , 1, 0, admin, sizeof(admin), 0 );
        MEfree( msg );
        if (getermessage( S_WU0007_ADMIN_PASSWORD, ER_TEXTONLY, &msg ) == OK)
        {
            IIUGprompt( msg , 1, 3, password, sizeof(password), 0 );
            MEfree( msg );
        }
    }
    else
    {
        retval = E_MISSING_MESSAGE;
        return(retval);
    }
    ICE_C_Initialize ();
    /*
    ** Connect to the ice server. NULL node ==> local machine.
    */
    if ((status = ICE_C_Connect (NULL, admin, password, &client)) == OK)
    {
        /*
        ** Execute a select to get all database users.
        */
        if ((status=ICE_C_Execute( client, "ice_locations", locselect )) == OK)
        {
            i4 end;

            do
            {
                /*
                ** Fetch each user
                */
                if (((status = ICE_C_Fetch (client, (II_INT4 *)&end)) == OK) &&
		    (!end))
                {
                    char *p;

                    for (i=1; i < LOC_ITEMS; i+=1)
                    {
                        p = ICE_C_GetAttribute (client, i);
                        if (p && *p)
                        {
                            STcopy( p, values[i] );
                        }
                        else
                        {
                            values[i][0] = EOS;
                        }
                    }
                    switch(actval)
                    {
                        case 0:     /* select */
                            SIprintf("%6s %-24s %-40s %s %s%s%s\n", values[1],
                                values[2], values[3],
                                ((values[4]) ? values[4] : "" ),
                                ((values[5] && *values[5]) ? "h" : "-" ),
                                ((values[6] && *values[6]) ? "i" : "-" ),
                                ((values[7] && *values[7]) ? "p" : "-" ) );
                            break;
                        case 1:     /* insert */
                            if (STcompare( sloc[2].value, values[2] ) == 0)
                            {
                                entryfound += 1;
                                retval = E_DUPLICATE;
                            }
                            break;
                        case 2:     /* update */
                        case 3:     /* delete */
                            if (sloc[2].value != NULL)
                            {
                                if (STcompare( sloc[2].value, values[2] ) == 0)
                                {
                                    entryfound += 1;
                                    /*
                                    ** save the id value for use later
                                    */
                                    STcopy(values[1], usevalues[1]);
                                    sloc[1].value = usevalues[1];
                                    for (i=2; i < LOC_ITEMS; i+=1 )
                                    {
                                        if (sloc[i].value == NULL)
                                        {
                                            usevalues[i][0] = EOS;
                                            sloc[i].value = usevalues[i];
                                        }
                                    }
                                }
                            }
                            else
                            {
                                IIUGerr( E_WU0009_UNNAMED_TARGET, UG_ERR_ERROR,
                                    0, NULL, NULL, NULL, NULL, NULL, NULL,
                                    NULL, NULL, NULL, NULL );
                                status = E_UNNAMED_TARGET;
                            }
                            break;
                    }
                }
            }
            while((status == OK) && (!end));
            ICE_C_Close (client);
            switch(actval)
            {
                case 0:     /* select */
                    break;
                case 1:     /* insert */
                    if (entryfound == 0)
                    {
                        if ((status=ICE_C_Execute (client, "ice_locations",
                            sloc)) == OK)
                        {
                            if ((status=getermessage( S_WU0008_STATUS_MESSAGE,
                                ER_TEXTONLY, &msg )) == OK)
                            {
                                SIprintf( msg,
                                    ICE_C_GetAttribute (client, 2),
                                    ICE_C_GetAttribute (client, 1),
                                    ERx( "created" ) );
                                MEfree( msg );
                            }
                        }
                        else
                        {
                            iceapierror( ICE_C_LastError (client) );
                            status = FAIL;
                        }
                        ICE_C_Close (client);
                    }
                    else
                    {
                        IIUGerr( E_WU0004_DUPLICATE_ENTRY, UG_ERR_ERROR, 0,
                            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                            NULL, NULL );
                    }
                    break;
                case 2:     /* update */
                case 3:     /* delete */
                    if (entryfound == 1)
                    {
                        if ((status=ICE_C_Execute (client, "ice_locations",
                            sloc)) == OK)
                        {
                            if ((status=getermessage( S_WU0008_STATUS_MESSAGE,
                                ER_TEXTONLY, &msg )) == OK)
                            {
                                SIprintf( msg,
                                    ICE_C_GetAttribute (client, 2),
                                    ICE_C_GetAttribute (client, 1),
                                    (actval == 2) ?
                                        ERx( "updated" ) : ERx( "deleted" ) );
                                MEfree( msg );
                            }
                        }
                        else
                        {
                            iceapierror( ICE_C_LastError (client) );
                            status = FAIL;
                        }
                        ICE_C_Close (client);
                    }
                    break;
            }
        }
        else
        {
            iceapierror( ICE_C_LastError (client) );
        }
        ICE_C_Disconnect (&client);
    }
    else
    {
        iceapierror( ICE_C_LastError (client) );
    }
    return( status );
}

/*
** Name: main
**
** Description:
**      Function parses the command line arguments for processing by
**      actiondbuser.
**
** Inputs:
**      argc    count of arguments
**      argv    array of argument vectors.
**
** Outputs:
**      None.
**
** Returns:
**      OK      success
**      !OK     failure
**
** History:
**      16-Jan-2000 (fanra01)
**          Created.
*/
STATUS
main( i4 argc, char* * argv )
{
    STATUS          status = OK;
    i4      i;
    i4      j;
    char*   p;
    bool    httploc = FALSE;
    bool    iceloc  = FALSE;
    bool    publicloc = FALSE;

    if ((argc == 2) || (argc == 3) ||  (argc == 6))
    {
        for (i=0; i < argc; i++)
        {
            p = argv[i];
            while(CMwhite( p )) CMnext( p );
            if (*p == '-')
            {
                CMnext(p);      /* skip the '-' character */
                do
                {
                    switch(*p)
                    {
                        case 'h':
                            CMnext( p );
                            httploc = TRUE;
                            break;
                        case 'i':
                            CMnext( p );
                            iceloc = TRUE;
                            break;
                        case 'p':
                            CMnext( p );
                            publicloc = TRUE;
                            break;
                        default:
                            status = E_UNKNOWN_OPTION;
                            CMnext( p );
                            break;
                    }
                }
                while( *p != EOS );
                i+=1;           /* move on to next argument */
                break;
            }
        }
        i = (i < argc) ? i : 1; /* if no options reset argument counter */
        /*
        ** load svrloc array with values
        */
        for (j = 0 ; j < LOC_ITEMS; j+=1)
        {
            switch(j)
            {
                case 1:         /* skip the id */
                    break;
                case 5:
                    if (httploc == TRUE)
                    {
                        svrloc[j].value = STalloc( ERx("checked") );
                    }
                    break;
                case 6:
                    if (iceloc == TRUE)
                    {
                        svrloc[j].value = STalloc( ERx("checked") );
                    }
                    break;
                case 7:
                    if (publicloc == TRUE)
                    {
                        svrloc[j].value = STalloc( ERx("checked") );
                    }
                    break;
                default:
                    if (i < argc)
                    {
                        svrloc[j].value = argv[i];
                        i+=1;
                    }
                    break;
            }
        }
        status = actionsvrloc( svrloc );
    }
    else
    {
        IIUGerr( E_WU000A_SVRLOC_USAGE, UG_ERR_ERROR, 0, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL );
        status = E_USAGE;
    }
    return(status);
}
