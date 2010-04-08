# include <compat.h>
# include <si.h>
# include <st.h>

# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>

# include <ice_c_api.h>

# define DISPLAY    0x00000001
# define UPDATE     0x00000002
# define INSERT     0x00000004

ICE_C_PARAMS iceuserget[] =
{
    {ICE_IN,  "action",         "select"},
    {ICE_OUT, "user_id",        NULL    },
    {ICE_OUT, "user_name",      NULL    },
    {ICE_OUT, "user_comment",   NULL    },
    {ICE_OUT, "user_dbuser",    NULL    },
    {ICE_OUT, "user_timeout",   NULL    },
    {0,       NULL,             NULL    }
};

ICE_C_PARAMS iceuser[] =
{
    {ICE_IN, "action",          NULL },
    {ICE_IN, "user_id",         NULL },
    {ICE_IN, "user_name",       NULL },
    {ICE_IN, "user_password1",  NULL },
    {ICE_IN, "user_password2",  NULL },
    {ICE_IN, "user_dbuser",     NULL },
    {ICE_IN, "user_comment",    NULL },
    {ICE_IN, "user_timeout",    NULL },
    {0,      NULL,              NULL }
};

static char user_id[20] = { "\0" };
static char user_name[80] = { "\0" };
static char user_dbuser[40] = { "\0" };
static char user_timeout[20] = { "\0" };

int
main (i4 argc, char** argv)
{
    ICE_C_CLIENT    client = NULL;
    ICE_STATUS      status = 0;
    char            admin[80];      /* admin user */
    char            password[80];   /* admin user password */
    char*           action;         /* action string */
    char*           user;           /* user to update from command line */
    char*           password1;      /* user password from command line */
    char*           password2;      /* conf password from command line */
    char*           hint;           /* hint password from command line */
    i4              duplicates;     /* flag ensures we remove only one user */
    i4              actionflag = 0;

    switch (argc)
    {
        case 1:
            actionflag = DISPLAY;
            break;

        case 8:
            if (STbcompare (argv[1], 0, "insert", 0, TRUE) != 0)
            {
                SIfprintf (stderr, "Unknown action %s\n", argv[1]);
                status = 1;
                break;
            }
            actionflag |= INSERT;
            STcopy (argv[6], user_dbuser);
            STcopy (argv[7], user_timeout);
        case 6:
            if (STbcompare (argv[1], 0, "update", 0, TRUE) != 0)
            {
                if (argc == 6)
                {
                    SIfprintf (stderr, "Unknown action %s\n", argv[1]);
                    status = 1;
                    break;
                }
            }
            else
            {
                actionflag |= UPDATE;
            }
            action = argv[1];
            user = argv[2];
            password1 = argv[3];
            password2 = argv[4];
            hint = argv[5];
            if ((status == 0) && (STcompare(password1, password2) != 0))
            {
                SIfprintf (stderr, "Passwords do not match\n");
                status = 1;
            }
            break;

        default:
            SIprintf ("Usage:\n\n    %s <action> <user> <password1> <password2> <\"hint\"> <db id> <to>\n\n",
                argv[0]);
            SIprintf ("        action      insert | update\n");
            SIprintf ("        user        user name to create or update\n");
            SIprintf ("        password1   password for user\n");
            SIprintf ("        password2   confirmation password for user\n");
            SIprintf ("        hint        hint string for password\n");
            SIprintf ("        db id       db user id for database access (insert only)\n");
            SIprintf ("        to          session timeout in seconds (insert only)\n");
            status = 1;
            break;
    }
    if (status == 0)
    {
        IIUGprompt("ICE Admin User?", 1, 0, admin, sizeof(admin), 0);
        IIUGprompt("Password?", 1, 1, password, sizeof(password), 0);
        ICE_C_Initialize ();
        /*
        ** Connect to the ice server. NULL node ==> local machine.
        */
        if ((status = ICE_C_Connect (NULL, admin, password, &client)) == OK)
        {
            /*
            ** Execute a select to get all users.
            */
            if ((status = ICE_C_Execute (client, "user", iceuserget)) == OK)
            {
                int end;

                if (actionflag & DISPLAY)
                {
                    SIprintf ("%8s %-20s %8s %10s %s\n",
                        "User Id", "User Name", "DB User", "Timeout",
                        "Hint");
                }
                duplicates = -1;
                do
                {
                    /*
                    ** Fetch each user
                    */
                    if (((status = ICE_C_Fetch (client, &end)) == OK) &&
                        (!end))
                    {
                        char *p = ICE_C_GetAttribute (client, 2);

                        if (p && *p)
                        {
                            STcopy (p, user_name);
                            /*
                            ** Compare name with one supplied on command line.
                            */
                            if (STbcompare (user_name, 0, user, 0, FALSE) == 0)
                            {
                                STcopy (ICE_C_GetAttribute (client, 1),
                                    user_id);
                                STcopy (ICE_C_GetAttribute (client, 4),
                                    user_dbuser);
                                STcopy (ICE_C_GetAttribute (client, 5),
                                    user_timeout);
                                duplicates +=1;
                            }
                            if (actionflag & DISPLAY)
                            {
                                SIprintf ("%8s %-20s %8s %10s %s\n",
                                    ICE_C_GetAttribute (client, 1),
                                    user_name, ICE_C_GetAttribute (client, 4),
                                    ICE_C_GetAttribute (client, 5),
                                    ICE_C_GetAttribute (client, 3));
                            }
                        }
                    }
                }
                while ((status == 0) && (!end));
                ICE_C_Close (client);
                if ((actionflag & ~(DISPLAY)) && (status == 0))
                {
                    if ((actionflag & UPDATE) &&
                        ((duplicates != 0) || (user_id[0] == '\0')))
                    {
                        SIfprintf (stderr,
                            "Error: Update %s found user id %s with %d duplicates\n",
                            user, user_id, duplicates);
                        status = 1;
                    }
                    if (status == 0)
                    {
                        iceuser[0].value = action;
                        iceuser[1].value = user_id;
                        iceuser[2].value = user;
                        iceuser[3].value = password1;
                        iceuser[4].value = password2;
                        iceuser[5].value = user_dbuser;
                        iceuser[6].value = hint;
                        iceuser[7].value = user_timeout;
                        if ((status=ICE_C_Execute (client, "user", iceuser))
                            == OK)
                        {
                            SIprintf ("User %-20s ID %8s %s\n", user, user_id,
                            (actionflag & UPDATE) ? "updated" : "created");
                        }
                        else
                        {
                            SIfprintf (stderr,
                                "Error: %s\n", ICE_C_LastError (client));
                            status = 1;
                        }
                    }
                }
            }
            else
            {
                SIfprintf (stderr, "Error: %s\n", ICE_C_LastError (client));
                status = 1;
            }
            ICE_C_Disconnect (&client);
        }
        else
        {
            SIfprintf (stderr, "Error: %s\n", ICE_C_LastError (client));
            status = 1;
        }
    }
    return (status);
}
