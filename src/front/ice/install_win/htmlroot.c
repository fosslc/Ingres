/*
** Copyright (C) 1998 Ingres Corporation
*/
/*
** Name: gethtml.c
**
** Description:
**      Program to scan the known registry keys of MS, NS and SG web servers
**      to determine where the configuration is.
**      Open the configuration file and display the html root document
**      directory.
**
** History:
**      09-Oct-98 (fanra01)
**          Created.
**      09-Nov-98 (fanra01)
**          Add extraction of root document directory for Spyglass server.
**      13-Nov-98 (fanra01)
**          Add extraction of root document for different versions of
**          netscape server.
**      20-Nov-1998 (fanra01)
**          Modified functions to return the root document instead of printing.
**      11-Dec-1998 (fanra01)
**          Add function to handle apache server.
**          Also changed Spyglass function to deal with unquoted paths.
**      24-Mar-1999 (fanra01)
**          Store error return from RegQueryValueEx and retry on not found
**          or not exist errors.
**	12-aug-1999 (mcgem01)
**	    Change nat to i4.
*/

# include <compat.h>
# include <cm.h>
# include <cv.h>
# include <si.h>
# include <st.h>

# define DISPERROR(n)   if (n == TRUE) SIprintf

typedef struct w3conf
{
    char*   id;             /* type of HTTP server */
    char*   reg;            /* registry key to read */
    char*   val;            /* return value of this registry entry */
    STATUS  (*gethtml)();   /* function to execute */
} W3CONF, *PW3CONF;

STATUS GetMSroot (PW3CONF conf, char* regkey, char** docpath);
STATUS GetNSroot (PW3CONF conf, char* regkey, char** docpath);
STATUS GetNSroot2(PW3CONF conf, char* regkey, char** docpath);
STATUS GetNSroot3(PW3CONF conf, char* regkey, char** docpath);
STATUS GetSGroot (PW3CONF conf, char* regkey, char** docpath);
STATUS GetAProot (PW3CONF conf, char* regkey, char** docpath);

static W3CONF httpsrv[] =
{
    {
        "MS",
        "SYSTEM\\CurrentControlSet\\Services\\W3SVC\\Parameters\\Virtual Roots",
        "/",
        GetMSroot,
    },
    {
        "NS",
        "%s",
        "ConfigurationPath",
        GetNSroot,
    },
    {
        "NS3",
        "SOFTWARE\\Netscape\\Enterprise\\3.5.1\\http%%c-%s",
        "ConfigurationPath",
        GetNSroot3,
    },
    {
        "NS2",
        "SOFTWARE\\Netscape\\http%%c-%s",
        "ConfigurationPath",
        GetNSroot2,
    },
    {
        "SG",
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\Thor.exe",
        "Path",
        GetSGroot,
    },
    {
        "AP",
        "SOFTWARE\\Apache Group\\Apache\\1.3.3",
        "ServerRoot",
        GetAProot,
    },
    {
        NULL, NULL, NULL, NULL,
    }
};

static char systemroot[]={ "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion" };
static char recbuf[SI_MAX_TXT_REC];

static i4  enumwebsrv = 0;
static i4  errflg = TRUE;

/*
** Name: GetMSroot
**
** Description:
**      Display the home HTML directory for MS IIS
**
** Inputs:
**      conf        pointer to the MS config structure.
**      regkey      generated registry key.
**
** Outputs:
**      docpath     contains the HTML root path
**
** Returns:
**      none
**
** History:
**      09-Oct-98 (fanra01)
**          Created.
**      20-Nov-1998 (fanra01)
**          Add docpath.
**      24-Mar-1999 (fanra01)
**          Store error return from RegQueryValueEx and retry on not found
**          or not exist errors.
*/
STATUS
GetMSroot (PW3CONF conf, char* regkey, char** docpath)
{
    STATUS  status = FAIL;
    i4      i =0;
    DWORD   dwstat = 0;
    char    val[10];
    HKEY    key;
    char    regval[256];
    DWORD   valtype;
    DWORD   valsize = sizeof(regval);


    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regkey, 0, KEY_QUERY_VALUE,
        &key) == ERROR_SUCCESS)
    {
        /*
        ** Need this loop as microsoft annoyingly adds a comma to the key name
        ** when any HTTP paths have been updated.
        */
        for (i = 0; i < 2; i++)
        {
            STcopy (httpsrv[enumwebsrv].val, val);
            if (i == 1) STcat (val, ",");
            if ((dwstat = RegQueryValueEx (key, val, NULL,
                &valtype, regval, &valsize)) == ERROR_SUCCESS)
            {
                char* p;

                if ((p = STindex (regval, ",", 0)) != NULL)
                    *p = EOS;
                *docpath = STalloc (regval);
                status = OK;
            }
            else
            {
                switch (dwstat)
                {
                    case ERROR_FILE_NOT_FOUND:
                    case ERROR_CLASS_DOES_NOT_EXIST:
                    case ERROR_ENVVAR_NOT_FOUND:
                        /*
                        ** Can't find? Try again with a comma
                        */
                        break;
                    default:
                        SIprintf ("Error : %d Retrieving value %s. Info %d\n",
                            dwstat, val, GetLastError());
                        i = 2; /* terminate loop */
                        break;
                }
            }
        }
    }
    else
    {
        DISPERROR(errflg)("Error : No Microsoft servers found\n");
    }
    return (status);
}

/*
** Name: GetNSroot
**
** Description:
**      Display the home HTML directory for NS
**
** Inputs:
**      conf        pointer to the NS config structure.
**      regkey      generated registry key.
**
** Outputs:
**      docpath     contains the HTML root path
**
** Returns:
**      none
**
** History:
**      09-Oct-98 (fanra01)
**          Created.
**      20-Nov-1998 (fanra01)
**          Add docpath.
*/
STATUS
GetNSroot (PW3CONF conf, char* mc, char** docpath)
{
    STATUS  status = FAIL;
    PW3CONF p = conf+1;
    char    reg[256];
    i4      i;

    errflg = FALSE;
    for (i=0; (STbcompare (p->id, 2, "NS", 2, TRUE) == 0) &&
        (status == FAIL); i+=1, p+=1)
    {
        STprintf (reg, p->reg, mc);
        status = p->gethtml (p, reg, docpath);
    }

    if (status != OK)
    {
        SIprintf ("Error : No Netscape servers found\n");
    }
    return (status);
}

/*
** Name: GetNSroot2
**
** Description:
**      Display the home HTML directory for NS 2.x
**
** Inputs:
**      conf        pointer to the NS config structure.
**      regkey      generated registry key.
**
** Outputs:
**      docpath     contains the HTML root path
**
** Returns:
**      none
**
** History:
**      12-Nov-98 (fanra01)
**          Created.
**      20-Nov-1998 (fanra01)
**          Add docpath.
*/
STATUS
GetNSroot2 (PW3CONF conf, char* template, char** docpath)
{
    STATUS  status = FAIL;
    HKEY    key;
    char    svrtype[3] = { "sd" };
    char    regkey[256];
    char    regval[256];
    DWORD   valtype;
    DWORD   valsize;
    i4      i;
    i4      errdisp = (errflg == TRUE) ? FALSE : TRUE;

    for (i=0; (svrtype[i] != 0) && (status == FAIL); i++)
    {
        STprintf (regkey, template, svrtype[i]);
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regkey, 0, KEY_QUERY_VALUE,
            &key) == ERROR_SUCCESS)
        {
            valsize = sizeof(regval);
            if (RegQueryValueEx (key, conf->val, NULL,
                &valtype, regval, &valsize) == ERROR_SUCCESS)
            {
                char* p;

                FILE*   fd;
                LOCATION    loc;
                char    buf[256];
                char*   tmp;

                /*
                ** Generate DOS path from the registry value
                */
                for (p = regval; *p != EOS; CMnext(p))
                {
                    if (*p == '/')
                        *p = '\\';
                }
                STcat (regval, "\\obj.conf");
                LOfroms (PATH & FILENAME, regval, &loc);
                if (SIopen (&loc, "r", &fd) == OK)
                {
                    do
                    {
                        status = SIgetrec (buf, sizeof(buf), fd);
                        if ((p = STstrindex (buf, "root=", 0,
                            TRUE)) != NULL)
                        {
                            p+=5;
                            tmp = p;
                            for (; *tmp != EOS; CMnext(tmp))
                            {
                                if (*tmp == '/')
                                *tmp = '\\';
                            }
                            STcopy (p, regval);
                            status = OK;
                            break;
                        }
                    }
                    while ((status == OK));
                    SIclose (fd);
                }
                if (status == OK)
                {
                    *docpath = STalloc (regval);
                }
            }
            else
            {
                DISPERROR(errflg)("Error : %d Retrieving value %s\n",
                    GetLastError(), conf->val);
            }
        }
        else
        {
            DISPERROR(errdisp)("Error : No Netscape servers found\n");
            if (errflg == TRUE)
            {
                if (errdisp == FALSE) errdisp = TRUE;
            }
        }
    }
    return (status);
}

/*
** Name: GetNSroot3
**
** Description:
**      Display the home HTML directory for NS 3.x
**
** Inputs:
**      conf        pointer to the NS config structure.
**      regkey      generated registry key.
**
** Outputs:
**      docpath     contains the HTML root path
**
** Returns:
**      none
**
** History:
**      12-Nov-98 (fanra01)
**          Created.
**      20-Nov-1998 (fanra01)
**          Add docpath.
*/
STATUS
GetNSroot3 (PW3CONF conf, char* template, char** docpath)
{
    STATUS  status = FAIL;
    HKEY    key;
    char    svrtype[3] = { "sd" };
    char    regkey[256];
    char    regval[256];
    DWORD   valtype;
    DWORD   valsize;
    i4      i;
    i4      errdisp = (errflg == TRUE) ? FALSE : TRUE;

    for (i=0; (svrtype[i] != 0) && (status == FAIL); i++)
    {
        STprintf (regkey, template, svrtype[i]);
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regkey, 0, KEY_QUERY_VALUE,
            &key) == ERROR_SUCCESS)
        {
            valsize = sizeof(regval);
            if (RegQueryValueEx (key, conf->val, NULL,
                &valtype, regval, &valsize) == ERROR_SUCCESS)
            {
                char* p;

                FILE*   fd;
                LOCATION    loc;
                char    buf[256];
                char*   tmp;

                /*
                ** Generate DOS path from the registry value
                */
                for (p = regval; *p != EOS; CMnext(p))
                {
                    if (*p == '/')
                        *p = '\\';
                }
                STcat (regval, "\\obj.conf");
                LOfroms (PATH & FILENAME, regval, &loc);
                if (SIopen (&loc, "r", &fd) == OK)
                {
                    do
                    {
                        status = SIgetrec (buf, sizeof(buf), fd);
                        if ((p = STstrindex (buf, "root=", 0,
                            TRUE)) != NULL)
                        {
                            p+=5;
                            tmp = p;
                            for (; *tmp != EOS; CMnext(tmp))
                            {
                                if (*tmp == '/')
                                *tmp = '\\';
                            }
                            STcopy (p, regval);
                            status = OK;
                            break;
                        }
                    }
                    while ((status == OK));
                    SIclose (fd);
                }
                if (status == OK)
                {
                    *docpath = STalloc (regval);
                }
            }
            else
            {
                DISPERROR(errflg)("Error : %d Retrieving value %s\n",
                    GetLastError(), conf->val);
            }
        }
        else
        {
            DISPERROR(errdisp)("Error : No Netscape servers found\n");
            if (errflg == TRUE)
            {
                if (errdisp == FALSE) errdisp = TRUE;
            }
        }
    }
    return (status);
}

/*
** Name: GetSGroot
**
** Description:
**      Display the home HTML directory for SG
**
** Inputs:
**      conf        pointer to the SG config structure.
**      regkey      generated registry key.
**
** Outputs:
**      docpath     contains the HTML root path
**
** Returns:
**      none
**
** History:
**      09-Oct-98 (fanra01)
**          Created.
**      20-Nov-1998 (fanra01)
**          Add docpath.
**      11-Dec-1998 (fanra01)
**          Deal with unqouted paths.
*/
STATUS
GetSGroot (PW3CONF conf, char* regkey, char** docpath)
{
    STATUS      status = FAIL;
    HKEY        key;
    char        regval[256];
    DWORD       valtype;
    DWORD       valsize = sizeof(regval);
    LOCATION    loc;
    char        cfgpath[MAX_LOC+1];
    FILE*       fd;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, systemroot, 0, KEY_QUERY_VALUE,
        &key) == ERROR_SUCCESS)
    {

        /*
        ** Get System root path
        */
        if (RegQueryValueEx (key, "SystemRoot", NULL,
            &valtype, regval, &valsize) == ERROR_SUCCESS)
        {
            /*
            ** Check for existance of server.cfg in system root directory
            */
            STprintf (cfgpath, "%s\\%s", regval, "server.cfg");
            status = LOfroms (PATH & FILENAME, cfgpath, &loc);
            if ((status == OK) && ((status = LOexist (&loc)) == OK))
            {
                status = SIopen (&loc, "r", &fd);
            }
            else
            {
                /*
                ** Check for server.cfg in spyglass install directory.
                */
                CloseHandle (key);
                if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regkey, 0,
                    KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
                {
                    valsize = sizeof (regval);  /* reset buffer size */
                    if (RegQueryValueEx (key, conf->val, NULL,
                        &valtype, regval, &valsize) == ERROR_SUCCESS)
                    {
                        STprintf (cfgpath, "%s\\%s", regval, "server.cfg");
                        status = LOfroms (PATH & FILENAME, cfgpath, &loc);
                        if ((status == OK) &&
                            ((status = LOexist (&loc)) == OK))
                        {
                            status = SIopen (&loc, "r", &fd);
                        }
                        else
                        {
                            DISPERROR(errflg)("Error : %s not found\n",
                                cfgpath);
                        }
                    }
                    CloseHandle (key);
                }
                else
                {
                    DISPERROR(errflg)("Error : No Spyglass servers found\n");
                }
            }

            if ((status == OK) && (fd != NULL))
            {
                i4  found = 0;
                do
                {
                    *recbuf = EOS;
                    status = SIgetrec (recbuf, SI_MAX_TXT_REC, fd);
                    if (status != ENDFILE)
                    {
                        switch (found)
                        {
                            case 0:
                                /*
                                ** scan for mapped directory section
                                */
                                if (STstrindex (recbuf, "[DirMaps]", 0,
                                    TRUE) != NULL)
                                {
                                    found = 1;
                                }
                                break;

                            case 1:
                                if (*recbuf &&
                                    ((*recbuf != 0x0A) || (*recbuf != 0x0D)))
                                {
                                    /*
                                    ** First entry after section marker is
                                    ** root directory find last quote and
                                    ** terminate string
                                    */

                                    char*   p1;
                                    char*   p;
                                    char    delimchr = '\"';
                                    i4      len;

                                    if ((p1 = STrindex (recbuf, "\"", 0))
                                        == NULL)
                                    {
                                        p1 = STrindex (recbuf, " ", 0);
                                        delimchr = ' ';
                                    }
                                    p = p1;
                                    if (p != NULL)
                                    {
                                        found = 2;
                                        if (delimchr == '\"')
                                            CMnext(p);
                                        *p = EOS;
                                        len = STlength (recbuf);
                                        p = STrindex (recbuf, "\\", 0);
                                        if ((p != NULL) &&
                                            (delimchr == '\"') &&
                                            (len > 3))
                                        {
                                            /*
                                            ** remove trailing backslash and
                                            ** replace with quote
                                            */
                                            *p = delimchr;
                                            CMnext (p);
                                            *p = EOS;
                                        }
                                        else if ((p != NULL) &&
                                            (delimchr == ' ') &&
                                            (len > 1))
                                        {
                                            *p = EOS;
                                        }
                                        *docpath = STalloc (recbuf);
                                        status = OK;
                                    }
                                }
                                break;
                        }
                    }
                }
                while ((status == OK) && (found != 2));

                SIclose (fd);
            }
            else
            {
                DISPERROR(errflg)("Error: Accessing %s\n", cfgpath);
            }
        }
    }
    return (status);
}

/*
** Name: GetAProot
**
** Description:
**      Display the home HTML directory for Apache 1.3.3
**
** Inputs:
**      conf        pointer to the AP config structure.
**      regkey      registry key.
**
** Outputs:
**      docpath     contains the HTML root path
**
** Returns:
**      none
**
** History:
**      11-Dec-1998 (fanra01)
**          Created.
*/
STATUS
GetAProot (PW3CONF conf, char* regkey, char** docpath)
{
    STATUS  status = FAIL;
    HKEY    key;
    char    regval[256];
    DWORD   valtype;
    DWORD   valsize;
    i4      errdisp = (errflg == TRUE) ? FALSE : TRUE;
    char*   confparm = "DocumentRoot";

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regkey, 0, KEY_QUERY_VALUE,
        &key) == ERROR_SUCCESS)
    {
        valsize = sizeof(regval);
        if (RegQueryValueEx (key, conf->val, NULL,
            &valtype, regval, &valsize) == ERROR_SUCCESS)
        {
            char* p;

            FILE*   fd;
            LOCATION    loc;
            char    buf[256];
            char*   tmp;

            /*
            ** Generate DOS path from the registry value
            */
            for (p = regval; *p != EOS; CMnext(p))
            {
                if (*p == '/')
                    *p = '\\';
            }
            STcat (regval, "\\conf\\srm.conf");
            LOfroms (PATH & FILENAME, regval, &loc);
            if (SIopen (&loc, "r", &fd) == OK)
            {
                do
                {
                    status = SIgetrec (buf, sizeof(buf), fd);
                    if (STbcompare (buf, 0, confparm, STlength(confparm),
                        TRUE) == 0)
                    {
                        i4  delim = FALSE;
                        i4  found = FALSE;

                        if ((p = STstrindex (buf, "\"", 0, TRUE)) != NULL)
                        {
                            tmp = p;
                            for (; (*tmp != EOS) && (!found); CMnext(tmp))
                            {
                                switch (*tmp)
                                {
                                    case '/':
                                        *tmp = '\\';
                                        break;

                                    case '\"':
                                        if (delim == FALSE)
                                        {
                                            delim = TRUE;
                                        }
                                        else
                                        {
                                            found = TRUE;
                                            CMnext(tmp);
                                            break;
                                        }
                                        break;
                                }
                            }
                            *tmp = EOS;
                            STcopy (p, regval);
                            status = OK;
                            break;
                        }
                    }
                }
                while ((status == OK));
                SIclose (fd);
            }
            if (status == OK)
            {
                *docpath = STalloc (regval);
            }
        }
        else
        {
            DISPERROR(errflg)("Error : %d Retrieving value %s\n",
                GetLastError(), conf->val);
        }
    }
    else
    {
        DISPERROR(errdisp)("Error : No Apache servers found\n");
        if (errflg == TRUE)
        {
            if (errdisp == FALSE) errdisp = TRUE;
        }
    }
    return (status);
}

void
main (i4  argc, char** argv)
{
    i4      i;
    i4      found;
    char    mc[128];
    char    regkey[256];
    char*   reg = regkey;
    char*   htmlroot;
    STATUS  status;

    if (argc != 2)
    {
        SIprintf ("Usage:\n");
        SIprintf ("      %s <MS | NS | NS2 | NS3 | AP | SG>\n", argv[0]);
        SIprintf ("         MS = Microsoft\n");
        SIprintf ("         NS = Netscape\n");
        SIprintf ("         NS2= Netscape 2.x\n");
        SIprintf ("         NS3= Netscape 3.x\n");
        SIprintf ("         AP = Apache 1.3.3\n");
        SIprintf ("         SG = Spyglass\n");
    }
    else
    {
        i4      len = sizeof (mc);
        for (i=0, found = 0; (httpsrv[i].id != NULL); i++)
        {
            if (STbcompare (argv[1], 0, httpsrv[i].id, 0, TRUE) == 0)
            {
                enumwebsrv = i;
                found = 1;
                break;
            }
        }
        if (found)
        {
            switch (enumwebsrv)
            {
                case 0:
                case 4:
                case 5:
                    STcopy (httpsrv[enumwebsrv].reg, reg);
                    break;

                case 1:
                case 2:
                case 3:
                    if (GetComputerName (mc, &len))
                    {
                        CVlower (mc);
                        STprintf (reg, httpsrv[enumwebsrv].reg, mc);
                    }
                    break;
            }
            status = httpsrv[enumwebsrv].gethtml (&httpsrv[enumwebsrv], reg,
                &htmlroot);
            if (status == OK)
            {
                if (htmlroot != NULL)
                {
                    SIprintf (htmlroot);
                    MEfree (htmlroot);
                }
            }
        }
    }
    return;
}
