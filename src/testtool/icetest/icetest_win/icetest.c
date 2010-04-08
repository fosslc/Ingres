/*
** Name: icetest.c
**
** Description:
**      The program takes the userid and password and connects to the ice
**      server.  The cookie is used in subsequent requests with the ice
**      server until a logout is performed.
**
**      The program reads stdin for each url and variables upto a newline.
**      This is then used to invoke the oiice cgi process.
**      The output from oiice is read and output to stdout.
**
** History:
**      17-Jun-1999 (fanra01)
**          Created.
**      09-Aug-1999 (fanra01)
**          Update argument handling to take into account running sep with
**          no parameters.
**          Also add terminating character define and add linefeed character
**          to terminating character list in interactive mode.
*/
# include <compat.h>
# include <cs.h>
# include <cv.h>
# include <er.h>
# include <me.h>
# include <pc.h>
# include <si.h>
# include <st.h>
#ifdef SEP
# include <tc.h>
#endif /* SEP */

# define TERM_CHAR  0x13                /* CTRL S */
# define ICE_BIN    ERx("oiice.exe")
# define MAX_STR    256
# define MAX_READ   4096

static char*    ice_env[][2] = {
/* 0 */ { "CONTENT_TYPE", NULL },
/* 1 */ { "CONTENT_LENGTH", NULL },
/* 2 */ { "PATH_INFO", NULL },
/* 3 */ { "HTTP_HOST", NULL },
/* 4 */ { "SERVER_PORT", "80" },
/* 5 */ { "HTTP_USER_AGENT", "command" },
/* 6 */ { "HTTP_COOKIE", NULL },
/* 7 */ { "II_SYSTEM", NULL },
/* 8 */ { NULL, NULL } };

static char     login[MAX_STR] = {0};
static char     logout[MAX_STR] = { "ii_action=disconnect" };
static char     chreadbuf[MAX_READ] = {0};
static char     conreadbuf[MAX_READ] = {0};
static char*    cookie = NULL;

# ifdef UNIX
static char*    iceprc = { "/ingres/ice/bin/oiice.cgi" };
# else
static char*    iceprc = { "\\ingres\\ice\\bin\\oiice.exe" };
# endif

HANDLE  childhandle[2];

#ifdef SEP
char*       sep = NULL;
LOCATION    inLoc;
LOCATION    outLoc;
char        inFile[MAX_LOC+1];
char        outFile[MAX_LOC+1];
GLOBALDEF   TCFILE *IIICEincomm = NULL;
GLOBALDEF   TCFILE *IIICEoutcomm = NULL;
#endif /* SEP */


/*
** Name: waitchild
**
** Description:
**      Function invoked as a thread on NT to determine when the child process
**      terminates.  When the child process goes close the pipe handle to
**      break out of the read in the main thread.
**
** Inputs:
**      param   pointer to an array of handles.
**              [0] = process handle
**              [1] = pipe handle
**
** Outputs:
**      None.
**
** Returns:
**      0
**
** History:
**      21-Jun-1999 (fanra01)
**          Created.
*/
DWORD WINAPI
waitchild( LPVOID param)
{
    HANDLE* p = (HANDLE*)param;

    WaitForSingleObject( p[0], INFINITE );
    CloseHandle( p[1] );
    p[1] = NULL;
    return(0);
}

/*
** Name: usage
**
** Description:
**
** Inputs:
**
** Outputs:
**
** Returns:
**
** History:
**      21-Jun-1999 (fanra01)
**          Created.
*/
VOID
usage(char* prog)
{
    SIprintf("Usage:\n        %s ICE20 <first page> [<vars>]\n",
        prog);
    SIprintf("        %s <userid> <password> <first page> [<vars>]\n",
        prog);
}

/*
** Name: createiopipes
**
** Description:
**      Create the pipes used to communicate between parent and child
**      processes.
**
** Inputs:
**      None.
**
** Outputs:
**      hStdinRead
**      hStdinWrite
**      hStdoutRead
**      hStdoutWrite
**
** Returns:
**      OK      successful
**      FAIL    otherwise
**
** History:
**      21-Jun-1999 (fanra01)
**          Created.
*/
STATUS
createiopipes( HANDLE* hStdinRead, HANDLE* hStdinWrite,
    HANDLE* hStdoutRead, HANDLE* hStdoutWrite )
{
    DWORD   pipe_size = 0;
    SECURITY_ATTRIBUTES sa;
    bool    status;

    iimksec( &sa );

    /*
    ** Create anonymous pipe for stdout
    */
    if((status = CreatePipe( hStdoutRead, hStdoutWrite, &sa, pipe_size ))
        != TRUE)
    {
        SIprintf( ERx("CreatePipe stdout pipe failed error=%d\n"),
            GetLastError() );
    }

    /*
    ** Create anonymous pipe for stdin
    */
    if ((status == TRUE) &&
        ((status = CreatePipe( hStdinRead, hStdinWrite, &sa, pipe_size ))
        != TRUE))
    {
        SIprintf( ERx("CreatePipe stdin pipe failed error=%d\n"),
            GetLastError() );
        CloseHandle( *hStdoutRead );
        CloseHandle( *hStdoutWrite );
    }

    if ((status == TRUE) &&
        (status = DuplicateHandle( GetCurrentProcess(), *hStdinWrite,
        GetCurrentProcess(), hStdinWrite, 0, FALSE,
        DUPLICATE_SAME_ACCESS)) != TRUE)
    {
        SIprintf( ERx("DuplicateHandle failed error=%d\n"),
            GetLastError() );
        CloseHandle( *hStdinRead );
        CloseHandle( *hStdinWrite );
        CloseHandle( *hStdoutRead );
        CloseHandle( *hStdoutWrite );
    }
    return(((status == TRUE) ? OK : FAIL));
}

/*
** Name: request
**
** Description:
**      Sets up the environment before creating the oiice process.  Variables
**      are written to the stdin of the child.
**      A wait thread is created to detect when the child process exits.
**
** Inputs:
**      hInRead     read end of child stdin
**      hInWrite    write end of child stdin pipe
**      hOutWrite   write end of stdout pipe handle
**      page        requested page
**      vars        variables to be passed to child process
**
** Outputs:
**      None.
**
** Returns:
**      OK          completed successfully
**      !0          otherwise
**
** History:
**      21-Jun-1999 (fanra01)
**          Created.
*/
STATUS
request( HANDLE hInRead, HANDLE hInWrite, HANDLE hOutWrite,
    char* page, char* vars )
{
    STATUS status = FAIL;
    SECURITY_ATTRIBUTES sa;
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    i4  i;
    i4  vlen;
    char    clength[20];
    bool    blstatus = TRUE;
    DWORD   written = 0;
    char    command[MAX_STR];

    for (i=0; (ice_env[i][0] != NULL) && (blstatus == TRUE); i++)
    {
        switch (i)
        {
            case 1: /* CONTENT_LENGTH */
                if (vars != NULL)
                {
                    vlen = STlength( vars );
                    CVna(vlen, clength);
                }
                else
                {
                    CVna(0, clength);
                }
                ice_env[i][1] = STalloc( clength );
                break;
            case 2: /* PATH_INFO */
                if (page != NULL)
                {
                    ice_env[i][1] = STalloc( page );
                }
                break;
            case 6: /* HTTP_COOKIE */
                if (cookie != NULL)
                {
                    ice_env[i][1] = cookie;
                }
            default:
                break;
        }
        if (ice_env[i][1] != NULL)
        {
            blstatus = SetEnvironmentVariable( ice_env[i][0], ice_env[i][1] );
        }
    }
    if (blstatus == TRUE)
    {
        iimksec( &sa );

        /*
        **      Initialize the startinfo structure.
        */
        MEfill(sizeof(si), 0, &si);
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdInput = hInRead;
        si.hStdOutput= hOutWrite;
        si.hStdError = hOutWrite;

        STprintf(command, "%s%s", ice_env[7][1], iceprc);

        if ((blstatus = CreateProcess( command, NULL, &sa, NULL, TRUE,
            NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi )) == TRUE)
        {
            DWORD threadid;

            if (vars != NULL)
            {
                WriteFile( hInWrite, vars, vlen, &written, NULL);
            }
            childhandle[0] = pi.hProcess;
            childhandle[1] = hOutWrite;
            CreateThread (NULL, 0, waitchild, childhandle, 0, &threadid);
            status = OK;
        }
        else
        {
            SIprintf( "CreateProcess failed error=%d\n", GetLastError() );
        }
    }
    for (i=0; (ice_env[i][0] != NULL) ; i++)
    {
        switch (i)
        {
            case 1: /* CONTENT_LENGTH */
            case 2: /* PATH_INFO */
                if (ice_env[i][1] != NULL)
                {
                    MEfree(ice_env[i][1]);
                    ice_env[i][1] = NULL;
                    SetEnvironmentVariable( ice_env[i][0], "" );
                }
                break;
            default:
                break;
        }
    }
    return(status);
}

/*
** Name: processpage
**
** Description:
**      Having created the pipes and the oiice process the function reads the
**      stdout pipe until read fails when the child process terminates.
**
** Inputs:
**      page    Requested page
**              ICE20   /location/filename.html
**              ICE25   /businessunit[filename.html]
**
**      vars    Variables to be passed.
**              variable1=value1&variable2=value2
**
** Outputs:
**      None.
**
** Returns:
**      OK      Completed successfully
**      !0      failed.
**
** History:
**      21-Jun-1999 (fanra01)
**          Created.
*/
STATUS
processpage(char* page, char* vars)
{
    STATUS  status = FAIL;
    HANDLE  hStdoutReadPipe = NULL;
    HANDLE  hStdoutWritePipe= NULL;
    HANDLE  hStdinReadPipe= NULL;
    HANDLE  hStdinWritePipe= NULL;
    u_i4      i;
    i4      written = 0;
    DWORD   read = 0;

    if ((status = createiopipes( &hStdinReadPipe, &hStdinWritePipe,
        &hStdoutReadPipe, &hStdoutWritePipe )) == OK)
    {
        if ((status = request( hStdinReadPipe, hStdinWritePipe,
            hStdoutWritePipe, page, vars )) == OK)
        {
            while ((status = ReadFile( hStdoutReadPipe, chreadbuf, MAX_READ,
                &read, NULL)) == TRUE)
            {
                char* p;

                if ((cookie == NULL) &&
                    (p = STstrindex( chreadbuf, "ii_cookie=", read,
                    TRUE )) != NULL)
                {
                    char cookenv[MAX_STR];
                    for (i=0; (p[i] != 0) && (p[i] != ' '); i++)
                    {
                        cookenv[i] = p[i];
                    }
                    p[i] = 0;
                    cookie = STalloc(cookenv);
                }
# ifdef SEP
                if (IIICEoutcomm != NULL)
                {
                    char* c = chreadbuf;
                    for (i = 0, c = chreadbuf; i < read; i+=1, c+=1)
                    {
                        TCputc( *c, IIICEoutcomm );
                    }
                }
                else
                {
# endif /* SEP */
                SIwrite( read, chreadbuf, &written, stdout );
# ifdef SEP
                }
# endif /* SEP */

            }
        }
        CloseHandle( hStdinReadPipe );
        hStdinReadPipe = NULL;
        CloseHandle( hStdinWritePipe );
        hStdinWritePipe = NULL;
        CloseHandle( hStdoutReadPipe );
        hStdoutReadPipe = NULL;
        if (status == FAIL)
        {
            CloseHandle( hStdoutWritePipe );
            hStdoutWritePipe = NULL;
            childhandle[1] = NULL;
        }
    }
    return(status);
}

/*
** Name: main
**
** Description:
**      Parses the command line and sets up to call the processpage function.
**      When processing 2.5 the process does not terminate until a
**      ctrl-Z character is entered.
**
**      icetest userid password page [variables]
**
**      e.g.
**          icetest ingres ingresnt /plays[play_home.html]
**
**      When processing 2.0 the process terminates after each page.
**
**      e.g.
**          icetest ICE20 /fortunoff/introduction.htm
**
** Inputs:
**      argc
**      argv
**
** Outputs:
**      None.
**
** Returns:
**      OK      completed successfully
**      !0      failed.
**
** History:
**      21-Jun-1999 (fanra01)
**          Created.
**      09-Aug-1999 (fanra01)
**          Update argument handling to take into account running sep with
**          no parameters.
**          Also add terminating character define and add linefeed character
**          to terminating character list in interactive mode.
*/
int
main(int argc, char** argv)
{
    STATUS  clstatus = FAIL;
    char*   userid = NULL;
    char*   password = NULL;
    char*   page1 = NULL;
    char*   vars = NULL;
    char    envstr[MAX_STR];
    char    hostname[MAX_STR];
    i4      i;
    char*   inpage = NULL;
    char*   invars = NULL;
    WORD wVersion = MAKEWORD( 2, 0 );
    WSADATA wsadata;
    bool    ice20   = FALSE;
    bool    params  = FALSE;

    for (i=0; (ice_env[i][0] != NULL); i++)
    {
        switch (i)
        {
            case 0: /* CONTENT_TYPE */
                ice_env[i][1] = STalloc( "application/x-www-form-urlencoded" );
                break;
            case 1: /* CONTENT_LENGTH */
                break;
            case 2: /* PATH_INFO */
                break;
            case 3: /* HTTP_HOST */
                if (WSAStartup (wVersion, &wsadata) == 0)
                {
                    if (gethostname (hostname, MAX_STR) == 0)
                    {
                        ice_env[i][1] = STalloc( hostname );
                    }
                }
                break;
            default:
                if (GetEnvironmentVariable( ice_env[i][0], envstr, MAX_STR )
                    != 0)
                {
                    ice_env[i][1] = STalloc( envstr );
                }
                break;
        }
    }

    if (argc > 1)
    {
        if (STbcompare( argv[1], 0, "ICE20", 0, TRUE ) == 0)
        {
            ice20 = TRUE;
            switch (argc)
            {
# ifdef SEP
                case 5:
                    /*
                    ** Sep Started and HTML parameters entered
                    */
                    params = TRUE;
# endif /* SEP */
                case 4:
# ifdef SEP
                    sep = argv[argc - 1];
                    /*
                    ** If this argument is sep I/O
                    */
                    if (sep != NULL && *sep == '-' && *(sep+1) == '*')
                    {
                        sep += 2;
                        STpolycat( 3, ERx("in_"), sep, ERx(".sep"), inFile );
                        STpolycat( 3, ERx("out_"), sep, ERx(".sep"), outFile );
                        LOfroms(FILENAME & PATH, inFile, &inLoc);
                        LOfroms(FILENAME & PATH, outFile, &outLoc);
                        if (TCopen(&inLoc,ERx("r"),&IIICEincomm) == OK)
                        {
                            if (TCopen(&outLoc,ERx("w"),&IIICEoutcomm) == OK)
                            {
                                TCputc(TC_BOS,IIICEoutcomm);
                                TCflush(IIICEoutcomm);
                            }
                            else
                            {
                                IIICEincomm = NULL;
                            }
                        }
                    }
                    else
                    {
                        /*
                        ** otherwise this is argument is HTML parameters
                        */
                        params = TRUE;
                    }
# endif /* SEP */
                    if (params == TRUE)
                    {
                        STcat( login, argv[3] );
                    }
                case 3:
                    page1 = argv[2];
                    clstatus = OK;
                    break;
                default:
                    usage( argv[0] );
                    break;
            }
        }
        else
        {
            switch (argc)
            {
#ifdef SEP
                case 6:
                    params = TRUE;
#endif /* SEP */
                case 5:
#ifdef SEP
                    sep = argv[argc - 1];
                    if (sep != NULL && *sep == '-' && *(sep+1) == '*')
                    {
                        sep += 2;
                        STpolycat( 3, ERx("in_"), sep, ERx(".sep"), inFile );
                        STpolycat( 3, ERx("out_"), sep, ERx(".sep"), outFile );
                        LOfroms(FILENAME & PATH, inFile, &inLoc);
                        LOfroms(FILENAME & PATH, outFile, &outLoc);
                        if (TCopen(&inLoc,ERx("r"),&IIICEincomm) == OK)
                        {
                            if (TCopen(&outLoc,ERx("w"),&IIICEoutcomm) == OK)
                            {
                                TCputc(TC_BOS,IIICEoutcomm);
                                TCflush(IIICEoutcomm);
                            }
                            else
                            {
                                IIICEincomm = NULL;
                            }
                        }
                    }
                    else
                    {
                        params = TRUE;
                    }        
#endif
                case 4:
                    clstatus = OK;
                    userid = argv[1];
                    password = argv[2];
                    page1 = argv[3];
                    STprintf( login,
                        "ii_action=connect&ii_userid=%s&ii_password=%s",
                        userid, password );
                    if (params == TRUE)
                    {
                        STcat( login, "&" );
                        STcat( logout, "&" );
                        STcat( login, argv[4] );
                        STcat( logout, argv[4] );
                    }
                    break;
                default:
                    usage( argv[0] );
                    break;
            }
        }
    }
    else
    {
        usage( argv[0] );
    }
    /*
    ** Connect to ice server and get cookie.
    */
    if (clstatus == OK)
    {
        if ((clstatus = processpage( page1, login )) == OK)
        {
            do
            {
                if (ice20 == FALSE)
                {
# ifdef SEP
                    i4 c;

                    if (IIICEincomm != NULL)
                    {
                        for (i = 0; i < MAX_READ; )
                        {
                            c = TCgetc(IIICEincomm,0);
                            if (c == '\0')
                                continue;
                            if (c == TC_EOQ)
                            {
                                TCputc(TC_EQR,IIICEoutcomm);
                                TCflush(IIICEoutcomm);
                                continue;
                            }
                            if (c == TC_EOF)
                            {
                                c = EOF;
                            }
                            if (c == '\n')
                            {
                                break;
                            }
                            else
                            {
                                if (c == TERM_CHAR)
                                {
                                    TCflush(IIICEoutcomm);
                                    clstatus = ENDFILE;
                                    break;
                                }
                                else
                                {
                                    conreadbuf[i++] = (char)c;
                                }
                            }
                        }
                    }
                    else
                    {
# endif /* SEP */
                    clstatus = SIgetrec(conreadbuf, MAX_READ, stdin);
# ifdef SEP
                    }
# endif /* SEP */
                    if (clstatus == OK)
                    {
                        char*   inpage = conreadbuf;
                        char*   invars = NULL;
                        /*
                        ** Start reading stdin until empty line
                        ** url + variables 1 per line
                        ** e.g. /application/unit/file.html?var1=val1&var2=val2
                        ** line starting with linefeed also terminates.
                        */
                        if ((*inpage == 0) || (*inpage == '\n'))
                            break;
                        invars = STindex( conreadbuf, "?", STlength(conreadbuf) );
                        if (invars != NULL)
                        {
                            *invars = 0;
                            invars+=1;
                        }
                        clstatus = processpage( inpage, invars );
                    }
                }
            }
            while((clstatus == OK) && (ice20 == FALSE));
            if (ice20 == FALSE)
            {
                clstatus = processpage( page1, logout );
            }
        }
    }
# ifdef SEP
    if (IIICEoutcomm != NULL)
    {
        TCflush(IIICEoutcomm);
        TCclose( IIICEincomm );
        TCclose( IIICEoutcomm );
    }
# endif /* SEP */
    return (clstatus);
}
