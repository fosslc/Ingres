/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
**  split param
**
**  Description
**      Takes the node name and displays either the server portion or the
**      database portion.
**
**      Usage splitprm :-
**          splitprm [options]
**
**      Options :-
**          -n         Name        [myserver::]databasename
**          -d         Database    Display database portion
**          -d         Environment Set the environment to required portion
**          -s         Server      Display server portion
**          -h         Help        Display this message
**
PROGRAM=splitprm
NEEDLIBS=COMPATLIB
**
**  History:
**      01-May-96 (fanra01)
**          Created for NT.
**      03-Jul-96 (fanra01)
**          Added option for handling server class.
**
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include <compat.h>
# include <me.h>
# include <si.h>
# include <st.h>

# define MAX_CMD_LINE       128
# define MAX_HELP_LINE      256

# define NO_IGNORE_CASE     1

# define NO_CONV            0

# define ALPHA             0x01
# define NUMERIC           0x02
# define BOOL              0x10

typedef struct cmd
{
    i4      inuse;                      /* inuse flag                        */
    char    order;                      /* command ordering                  */
    char    delim[2];                   /* delimiter to data                 */
    char    command[MAX_CMD_LINE];      /* ascii command string              */
    char    help[MAX_HELP_LINE];        /* help description                  */
    i4      (*action)();                /* command function to parse arg     */
    i4      type;                       /* data type of parameter            */
    i4      conv;                       /* conversion type                   */
}CMDLINE;

typedef struct parm
{
    PTR     progname;
    CMDLINE *opts;
    PTR     dbname;
    bool    db;
    bool    svr;
    bool    class;
    PTR     env;
}PARAM;

STATUS cmdhelp(CMDLINE *, PARAM * , PTR);
STATUS cmddbname(CMDLINE *, PARAM * , PTR);
STATUS cmddb(CMDLINE *, PARAM * , PTR);
STATUS cmdenv(CMDLINE *, PARAM * , PTR);
STATUS cmdserver(CMDLINE *, PARAM * , PTR);
STATUS cmdclass(CMDLINE *, PARAM * , PTR);
STATUS splitparm(PARAM *);

PARAM vals;

CMDLINE options[] = { \
0xFFFF, '\0', "=","-n"," Name        [myserver::]databasename\n       -n=node",
 cmddbname ,ALPHA|NUMERIC,NO_CONV,
0xFFFF, '\0', " ","-d", " Database    Display database portion",
 cmddb     ,BOOL         ,NO_CONV,
0xFFFF, '\0', "=","-e", " Environment Set environment to required portion\n       -e=env",
 cmdenv     ,ALPHA|NUMERIC,NO_CONV,
0xFFFF, '\0', " ","-s", " Server      Display server portion",
 cmdserver ,BOOL         ,NO_CONV,
0xFFFF, '\0', " ","-c", " Class       Display server class portion",
 cmdclass  ,BOOL         ,NO_CONV,
0xFFFF, '\0', "=","-h", " Help        Display this message"    ,
 cmdhelp   ,0            ,NO_CONV,
0x0000, '\0', "\0", "", "" ,
 NULL     ,0            ,NO_CONV } ;


/*
** Name: main
**
** Description:
**      Parses the command line by comparing each argument with the list of
**      available commands.  Each line in the CMDLINE structure describes
**      a valid command and the actions to be performed on the argument.
**
**      Values are parsed from the argument into the PARAM structure.
**      The PARAM structure is the centrlised store passed to other routines.
**
**      Once all command line processing is completed the splitparm function
**      process the parameters.
**
** Inputs:
**      argc            argument count
**      argv            pointer to argument strings
**
** Outputs:
**      none.
**
**    Returns:
**      0   OK
**      1   error
**
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
*/

STATUS
main (int argc , char * * argv)
{
STATUS status = OK;
i4      i,j;
PTR arg;
CMDLINE *entry;
bool     found;

    vals.progname = argv[0];
    vals.opts = options;

    if (argc > 1)
    {
        for (i=1; i < argc; i++)
        {
            arg = STskipblank(argv[i], STlength(argv[i]));
            for (found = 0, j=0, entry = options;
                 (entry->inuse != 0) && !found; j++, entry++)
            {                           /* scan the command table            */
                if (!STncmp(entry->command, arg,STlength(arg)))
                {
                    arg += STlength(entry->command);
                    status = (entry->action)(entry, &vals, arg);
                    found = (status == OK) ? 1 : 0;
                    break;
                }
            }
            if (!found)
            {
                SIfprintf(stdout,"Error: Parameter %s incorrect\n", argv[i]);
                exit(1);
            }
        }
        splitparm(&vals);
    }
    else
    {
        cmdhelp(options, &vals, argv[0]);
    }
    return(status);
}

/*
** Name: cmddbname
**
** Description:
**      Set-up the parameter holding the node name.
**
** Inputs:
**      opts            pointer to command table
**      values          pointer to parameter table
**      arg             pointer to parameter value
**
** Outputs:
**      none.
**
**    Returns:
**      status
**
**    Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
*/

STATUS
cmddbname(CMDLINE *opts, PARAM * values, PTR arg)
{
STATUS status = OK;
PTR param;
PTR ptr;

    param = STskipblank(arg,STlength(arg));
    if ((ptr = STindex(param,opts->delim,0)) != NULL)
        values->dbname = ptr +1; /* skip delimiter */
    else
        status = FAIL;

    return(status);
}

/*
** Name: cmdserver
**
** Description:
**      Set-up the parameter holding the display server flag.
**
** Inputs:
**      opts            pointer to command table
**      values          pointer to parameter table
**      arg             pointer to parameter value
**
** Outputs:
**      none.
**
**    Returns:
**      status
**
**    Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
*/

STATUS
cmdserver(CMDLINE *opts, PARAM * values, PTR arg)
{
STATUS status = OK;

    values->svr = 1;
    return(status);
}

/*
** Name: cmdclass
**
** Description:
**      Setup the parameter holding the display server class flag.
**
** Inputs:
**      opts            pointer to command table
**      values          pointer to parameter table
**      arg             pointer to parameter value
**
** Outputs:
**      none.
**
**    Returns:
**      status
**
**    Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
*/

STATUS
cmdclass(CMDLINE *opts, PARAM * values, PTR arg)
{
STATUS status = OK;

    values->class = 1;
    return(status);
}

/*
** Name: cmddb
**
** Description:
**      Set-up the parameter holding the display database flag.
**
** Inputs:
**      opts            pointer to command table
**      values          pointer to parameter table
**      arg             pointer to parameter value
**
** Outputs:
**      none.
**
**    Returns:
**      status
**
**    Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
*/

STATUS
cmddb(CMDLINE *opts, PARAM * values, PTR arg)
{
STATUS status = OK;

    values->db = 1;
    return(status);
}

/*
** Name: cmdenv
**
** Description:
**      Set the parameter to hold the environment name
**
** Inputs:
**      opts            pointer to command table
**      values          pointer to parameter table
**      arg             pointer to parameter value
**
** Outputs:
**      none.
**
**    Returns:
**      status
**
**    Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
*/

STATUS
cmdenv(CMDLINE *opts, PARAM * values, PTR arg)
{
STATUS status = OK;
PTR ptr;

    if ((ptr = STindex(arg,opts->delim,0)) != NULL)
        values->env =  ptr + 1; /* skip delimiter */
    else
        status = FAIL;
    return(status);
}

/*
** Name: cmdhelp
**
** Description:
**      Display help info for each command.
**
** Inputs:
**      opts            pointer to command table
**      values          pointer to parameter table
**      arg             pointer to parameter value
**
** Outputs:
**      none.
**
**    Returns:
**      status
**
**    Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
*/

STATUS
cmdhelp(CMDLINE *opts, PARAM * values, PTR arg)
{
STATUS status = OK;
i4  i;

    SIfprintf(stdout,"\nUsage %s :-\n", values->progname);
    SIfprintf(stdout,"    %s [options]\n\n",values->progname);
    SIfprintf(stdout,"Options :-\n");
    for(i=0; opts->inuse != 0; i++, opts++)
    {
        SIfprintf(stdout,"    %s        %s\n",opts->command, opts->help);
    }
    return(status);
}

/*
** Name: splitparam
**
** Description:
**      Split the node into the required components.
**
** Inputs:
**      values          pointer to parameter table
**
** Outputs:
**      none.
**
**    Returns:
**      status
**
**    Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
**      03-Jul-96 (fanra01)
**          Added handling of server class.
*/

STATUS
splitparm(PARAM * values)
{
STATUS status = OK;
PTR dbname = values->dbname;
char server[128];
char database[128];
char class[128];
PTR ptr;
PTR pClass;

    if(dbname)
    {
        MEfill(128,0,server);
        MEfill(128,0,class);
        MEfill(128,0,database);

        if ((ptr = STindex(dbname,":",0)) != NULL)
            MEcopy(dbname, ptr-dbname, server);

        if ((pClass = STindex(dbname,"/",0)) == NULL)
        {                               /* cannot find the forward slash     */
            pClass = dbname + STlength(dbname);
                                        /* point end of string, null         */
        }
        STcopy(pClass,class);

        if ((ptr = STrindex(dbname,":",0)) != NULL)
        {
            ptr +=1;
        }
        else
        {
            ptr=dbname;
        }
        STlcopy(ptr,database,pClass - ptr);

        if(values->svr)
        {
            if(values->env)
            {
                SIfprintf(stdout,"%s %s%c%s","set",values->env, '=', server);
            }
            else
            {
                SIfprintf(stdout,"%s\n",server);
            }
        }

        if(values->db)
        {
            if(values->env)
            {
                SIfprintf(stdout,"%s %s%c%s","set",values->env, '=', database);
            }
            else
            {
                SIfprintf(stdout,"%s\n",database);
            }
        }
        if(values->class)
        {
            if(values->env)
            {
                SIfprintf(stdout,"%s %s%c%s","set",values->env, '=', class);
            }
            else
            {
                SIfprintf(stdout,"%s\n",class);
            }
        }
    }
    return(status);
}
