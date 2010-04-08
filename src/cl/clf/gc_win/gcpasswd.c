/*
** Copyright (c) 1987, 2010 Ingres Corporation
*/

#include	<compat.h>
#include        <ci.h>
#include        <cs.h>
#include	<gc.h>
#include	<gccl.h>
#include	<st.h>
#include	<pc.h>
#include	<me.h>
#include	<tr.h>
#include	"gcpasswd.h"

/******************************************************************************
**
**  Name: GCPASSWD.C - CL routines for prompting from user id and password.
**
**  Description:
**
**  The routines in this file are used to prompt a user for user id and
**  password on an environment which does not support secure loggins (Windows
**  95).
**
**      GCpasswd_prompt -   Prompt for user id and password if one is required.
**                          If for a particular node the user is set to
**                          IIPROMPTALL -   every connection request to the
**                                          node will require password
**                                          validation.
**                          IIPROMPT1   -   password validation is require the
**                                          first time the user attempts a
**                                          connection to the node.  Since
**                                          the list is held in GCN then
**                                          when an app terminates the entries
**                                          persist and performs the same as
**                                          IIPROMPTONCE.  Needs fixing if
**                                          login per app is required.
**                          IIPROMPTONCE-   validate password first time user
**                                          attempts to connect to a node.
**                                          No further login required if
**                                          connection is successful.
**  History:
**      13-Nov-96 (fanra01)
**          Modified for OpenIngres on WIndows 95.  Encrypt function is taken
**          from the gcnencry.c module and renamed to remove conflict and to
**          reduce build dependencies.
**      20-Nov-96 (fanra01)
**          Removed wn.h from includes.
**	24-Apr-97 (mcgem01)
**	    Removed restriction that enabled the IIPROMPTALL feature
**	    on Windows '95 only.
**      27-may-97 (mcgem01)
**          Cleaned up compiler warnings.
**      30-sep-97 (chudu01)
**          Define vnode_cache_file differently for Jasmine and Ingres.
**          Since vnode_cache_file will be used as a key for
**          CreateFileMapping(), we want the names to be unique between
**          Jasmine and Ingres.
**      18-Dec-97 (fanra01)
**          Updated DLL name to reflect OI.
**	08-jan-1998 (canor01)
**	    Updated DLL name for Jasmine.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	08-feb-2001 (somsa01)
**	    Use INVALID_HANDLE_VALUE rather than 0xFFFFFFFF when calling
**	    CreateFileMapping().
**	28-jun-2001 (somsa01)
**	    Since we may be dealing with Terminal Services, prepend
**	    "Global\" to the shared memory name, to specify that this
**	    shared memory segment is to be opened from the global name
**	    space.
**	14-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**      28-Feb-2006 Bug 115802 (Ralph Loen)
**          In GCpasswd_prompt(), removed encryption of password if
**          received from the IIPROMPT dialog box.  Updated references
**          to the compatlib module.
**	06-Dec-2006 (drivi01)
**	    Adding support for Vista, Vista requires "Global\" prefix for
**	    shared objects as well.  Replacing calls to GVosvers with 
**	    GVshobj which returns the prefix to shared objects.
**	17-Dec-2009 (Bruce Lunsford) Sir 122536
**	    Add support for long identifiers; add include for gccl.h to
**	    pick up #defines for OS-dependent max length for username,
**	    which GC_L_USER_ID #defined in gcpasswd.h now depends on.
**	    Add tracing.
**	17-Dec-2009 (Bruce Lunsford) Sir 122536 addendum
**	    Change name of shared memory vnode cache file (append "2") to
**	    prevent sharing with prior versions of Ingres, since entries
**	    within the cache are now longer due to the increased length
**	    of the username.
**	    Also, the 2nd or subsequent use of a vnode with IIPROMPT1 or
**	    IIPROMPTONCE can crash IIGCN if a long name and/or password
**	    is used.  The previous change handled long names whenever
**	    the dialog prompt was presented (always with IIPROMPTALL and
**	    1st time in for IIPROMPT1/ONCE) by not overlaying directly
**	    the caller's input parms for userid and password.  Similar
**	    logic needed when userid/password retrieved from cache.
**	    Also clear static user and password fields for security.
*/

GLOBALREF i4  GC_trace;
# define GCTRACE( n )  if ( GC_trace >= n )(void)TRdisplay

#define MAXVNODES 128

static VNODECACHE   *lpVnodeCache       = NULL;
static VNODECACHE   *xferbuf            = NULL;
static char    szUsername[GC_L_USER_ID] ={0};
static u_i4    lenUsername = 0;
static char    szPassword[GC_L_PASSWORD]={0};
static u_i4    lenPassword = 0;
static char    *vnode_cache_file = "gcn_mipc2";

HINSTANCE   hDLLInst = NULL;
HWND        hWnd     = NULL;

HANDLE      hCacheMutex = NULL;

bool GCpasswd_prompt (char **lpusername, char **lppassword, char *vnode);
void GCpasswd_delete_vnode_info (char *vnode);
BOOL PASCAL PasswdDlg (HWND hDlgWnd, WORD wMsgID, WORD wParam, LONG lParam) ;
static HWND GetConsoleHwnd(void);
static bool lookup_local_vnode_info(char *vnode, char *username, char *password);
static void save_local_vnode_info (char *vnode, char *username, char *password);
static VNODECACHE * init_vnode_cache(char * name);
static STATUS partnerparse( char *in, char **vnode, char **dbname, char **class );

static STATUS encrypt(char *userid, char *passwd, char *etxtbuff);
static STATUS dbnameparse(char *in, char **vnode, char **dbname, char **class);
static BOOL DetectCache( char *name );

#ifndef STcopy
extern long STcompare(char *a,char *b);
extern void STcopy(char *string,char *buffer);
#endif

FUNC_EXTERN VOID GVshobj(char **ObjectPrefix);

/*
**  Structure used in the memory mapped area.
*/
typedef struct mappedipc
{
    char CacheMutex[128];               /* name of cross process mutex used  */
                                        /* for protecting simultaneous access*/
                                        /* of the vnode cache                */
    VNODECACHE vcache[MAXVNODES + 1];   /* vnode cache                       */
} MIPC;

/******************************************************************************
** Name:    GCPasswd_prompt - prompt for username and password
**
** Description:
**
** This routine is called from gcn during the resolution of user information
** when connecting to a vnode.
** The funtciotn checks to see which of following conditions holds:
**     (a) use NETU entry - (returns FALSE in this case)
**     (b) prompts for username, password - returns TRUE and modifies
**         "password", "username"
**     (c) uses info previously prompted for - returns TRUE and modifies
**         "password", "username"
** Inputs:
**      lpusername  Ptr to name of user from vnode (IIPROMPT1, etc)
**      lppassword  Ptr to password from vnode (likely empty or dummy string)
**      vnode       pointer to a string which is displayed on the dialog as the
**                  vnode requiring user and password information
**
** Outputs:
**      lpusername  name of user given from dialog; ptr also changed if
**	            allocated new larger area.
**      lppassword  password given from dialog; ptr also changed if
**	            allocated new larger area.
**
** Returns:
**      TRUE        username and password modified
**      FALSE       netu entry used
**
** Exceptions:
**      None
**
** Side Effects:
**      None
**
** History:
**      18-Dec-97 (fanra01)
**          Updated DLL name to reflect OI.
**	17-Dec-2009 (Bruce Lunsford) Sir 122536
**	    Add support for long identifiers. If user-entered username
**	    and/or password (from prompt dialog) are too long to fit
**	    into input parm username and password fields, then allocate
**	    new area(s) for them.
**	17-Dec-2009 (Bruce Lunsford) Sir 122536 addendum
**	    The 2nd or subsequent use of a vnode with IIPROMPT1 or
**	    IIPROMPTONCE can crash IIGCN if a long name and/or password
**	    is used.  The previous change handled long names whenever
**	    the dialog prompt was presented (always with IIPROMPTALL and
**	    1st time in for IIPROMPT1/ONCE) by not overlaying directly
**	    the caller's input parms for userid and password.  Similar
**	    logic needed when userid/password retrieved from cache.
**	    Also clear static user and password fields for security.
*/

bool
GCpasswd_prompt (
	char **lpusername,
	char **lppassword,
	char *vnode)
{
    char *username = *lpusername;
    char *password = *lppassword;
    i2   SaveInfoType = 0;

    GCTRACE(3)("GCpasswd_prompt: Entry point.  vnode=%s\n", vnode);

    hWnd = GetConsoleHwnd();

    /* If first call, allocate the local cache and the transfer area */
    if ((lpVnodeCache = init_vnode_cache(vnode_cache_file)) == NULL)
    {
        return (FALSE);
    }

    /*
    ** If username is IIPROMPTALL then we always call DialogBox() to
    ** prompt user for the real username and password.
    **
    ** If IIPROMPT1 or IIPROMPTONCE then just prompt the first time that we
    ** try to connect to the Database.
    */
    if (!STcompare(username, IIPROMPTALL))
    {
        SaveInfoType = 0; /* prompt and don't save into cache */
    }
    else if (!STcompare(username, IIPROMPT1) ||
             !STcompare(username, IIPROMPTONCE))
    {
        /*
        ** Check if we already have the username/password for this vnode
        ** in the local table. if so, lookup_local_vnode_info() will
        ** return the information. Otherwise, we will prompt for it.
        */
        if (lookup_local_vnode_info(vnode, szUsername, szPassword))
	{
	    GCTRACE(3)("GCpasswd_prompt: Auth info retrieved from IIPROMPT* cache\n");
            SaveInfoType = 2; /* Vnode found in cache...don't prompt or resave*/
	    lenUsername = STlength(szUsername);
	    lenPassword = STlength(szPassword);
	}
        else
            SaveInfoType = 1; /* Vnode not found in cache...prompt and save into cache */
    }
    else
    {
        return (TRUE);
    }

    if (SaveInfoType == 2 ||
        DialogBoxParam(GetModuleHandle("IILIBCOMPAT.DLL"),
                       (LPCSTR)"CONNECT", hWnd,
                       (DLGPROC)PasswdDlg,(LPARAM)vnode) == IDOK ||
	DialogBoxParam(GetModuleHandle("IILIBCOMPAT.DLL"),
                       (LPCSTR)"CONNECT", NULL,
                       (DLGPROC)PasswdDlg,(LPARAM)vnode) == IDOK)
    {
	if (SaveInfoType != 2)
	{
	    GCTRACE(3)("GCpasswd_prompt: User entered auth info on IIPROMPT* dialog\n");
	}
	/*
	** The only thing known about the lengths of the incoming parms
	** for username and password is that they are large enough to
	** hold whatever string is already there passed as input.
	** If the incoming length is large enough to hold the username
	** or password entered by the user or retrieved from the cache,
	** then overlay it with the entered (or cached) value.
	** Eg., "IIPROMPTALL" can be replaced by any
	** entered user name of length 11 or less (not including EOS).
	** If input length is not long enough, allocate a new area and
	** change the username and/or password pointers accordingly.
	*/
	if (lenUsername > STlength(username))
	{
	    username = MEreqmem(0, lenUsername + 1, FALSE, NULL);
	    *lpusername = username;
	    GCTRACE(3)("GCpasswd_prompt: username area reallocated\n");
	}
        STcopy(szUsername, username);
	MEfill(lenUsername, 0, szUsername);  /* Clear for security */
	lenUsername = 0;

	if (lenPassword > STlength(password))
	{
	    password = MEreqmem(0, lenPassword + 1, FALSE, NULL);
	    *lppassword = password;
	    GCTRACE(3)("GCpasswd_prompt: password area reallocated\n");
	}
        STcopy(szPassword, password);
	MEfill(lenPassword, 0, szPassword);  /* Clear for security */
	lenPassword = 0;

        switch (SaveInfoType)
        {
            case 1:
                save_local_vnode_info(vnode, username, password);
                break;
            default:
                break;
        }
        /*
        ** DialogBox() can cause yield to another process, so make sure to
        ** restore context.
        */
        return (TRUE);
    }
    else
    {
	GCTRACE(3)("GCpasswd_prompt: Error or User hit Cancel on IIPROMPT* dialog\n");
        /*
        ** DialogBox() can cause yield to another process, so make sure to
        ** restore context.
        */
        return (FALSE);
    }

}

/******************************************************************************
** Name:    GCpasswd_delete_vnode_info - remove vnode from cache list.
**
** Description:
**
** This routine is called from GCA of the client if the login fails and
** removes the vnode details from the list.
**
** Inputs:
**      vnode       pointer to the vnode to be removed from the list
**
** Outputs:
**      None
**
** Returns:
**      None
**
** Exceptions:
**      None
**
** Side Effects:
**      None
**
** History:
**      13-Feb-2007 (drivi01)
**          Added a null check for lpVnodeCache.
**          if init_vnode_cache returns a NULL this routine will result
**          in SEGV.
**	22-Feb-2007 (drivi01)
**	    Remove init_vnode_cache from this function, cache is
** 	    now being initialized in GCpasswd_prompt and doesn't
**	    need to be initialized here.
**          
**	
*/

void
GCpasswd_delete_vnode_info(char	*partner)
{
VNODECACHE  *vinfo;
i2          i;

char	*vnode = NULL;
char	*dbname= NULL;
char	*class = NULL;

    /* parse dbname into components */
    partnerparse(partner,&vnode, &dbname, &class);

    if (vnode != NULL)
    {
        /* Remove the vnode entry from the local table. */
        if (lpVnodeCache == NULL)
	    return;

        if (hCacheMutex != NULL)
            WaitForSingleObject(hCacheMutex,INFINITE);

	if (lpVnodeCache != NULL)
	{
            vinfo = lpVnodeCache;
            for (i = 0; i < MAXVNODES; i++)
            {
                if (!STcompare(vnode, vinfo->vnode))
                {
                    vinfo->vnode[0] = '\0';
                    vinfo->username[0] = '\0';
                    vinfo->password[0] = '\0';
                    break;
                }
                vinfo++;
            }
	}

        if (hCacheMutex != NULL)
            ReleaseMutex(hCacheMutex);
    }
}

/******************************************************************************
** Name:    lookup_local_vnode_info - scan the vnode cache list for the entry
**
** Description:
**
** Inputs:
**      vnode       pointer to the vnode br found
**
** Outputs:
**      username    pointer to the username last entered for connection
**      password    pointer to the password last entered for connection
**
** Returns:
**      TRUE        entries found
**      FALSE       no entries found
**
** Exceptions:
**      None
**
** Side Effects:
**      None
**
** History:
**
*/

static bool
lookup_local_vnode_info(
    char *vnode,
    char *username,
    char *password)
{
    VNODECACHE  *vinfo;
    i2          i;

    if (hCacheMutex != NULL)
        WaitForSingleObject(hCacheMutex,INFINITE);

    vinfo = lpVnodeCache;
    for (i = 0; i < MAXVNODES; i++)
    {
        if (!STcompare(vnode, vinfo->vnode))
        {
            STcopy (vinfo->username, username);
            STcopy (vinfo->password, password);
            if (hCacheMutex != NULL)
                ReleaseMutex(hCacheMutex);
            return (TRUE);
        }
        vinfo++;
    }
    if (hCacheMutex != NULL)
        ReleaseMutex(hCacheMutex);

    return (FALSE);
}

/******************************************************************************
** Name:    save_local_vnode_info - store the vnode info into the cache list
**
** Description:
**
** Inputs:
**      vnode       pointer to the vnode to be stored
**      username    pointer to the username
**      password    pointer to the password
**
** Outputs:
**      None
**
** Returns:
**      None
**
** Exceptions:
**      None
**
** Side Effects:
**      None
**
** History:
**
*/

static void
save_local_vnode_info (
    char *vnode,
    char *username,
    char *password)
{
i2  i;
VNODECACHE  *vinfo;
VNODECACHE  *emptyslot;

    /* If vnode already exists, update it */
    if (hCacheMutex != NULL)
        WaitForSingleObject(hCacheMutex,INFINITE);

    vinfo = lpVnodeCache;
    emptyslot = NULL;
    for (i = 0; i < MAXVNODES; i++)
    {
        if (vinfo->vnode[0] == '\0')
        {
            if (emptyslot == NULL)
                emptyslot = vinfo;
        }
        else if (!STcompare(vnode, vinfo->vnode))
        {
            STcopy(username, vinfo->username);
            STcopy(password, vinfo->password);
            if (hCacheMutex != NULL)
                ReleaseMutex(hCacheMutex);
            return;
        }
        vinfo++;
    }

    /* If not found, enter the new vnode info (only if we have space) */
    if (emptyslot)
    {
        vinfo = emptyslot;

        STcopy (vnode, vinfo->vnode);
        STcopy (username, vinfo->username);
        STcopy (password, vinfo->password);
    }
    if (hCacheMutex != NULL)
        ReleaseMutex(hCacheMutex);
}

/******************************************************************************
** Name:    init_vnode_cache - setup the shared memory area for the cache
**
** Description:
**      The lpVnodeCache pointer is global storage which is used to determine
**      if the vnode cache has been initialised.  A DetectCache call is made
**      to try to open an existing mapping.  If one does not exist then a
**      mapping is created.
**      On creation of the mapping a name for the cross process mutex is
**      stored into the first field of the mapped area and the mutex is
**      created.
**      On subsequent calls the pointer to the mapping is returned.
**
** Inputs:
**      name        pointer to name of shared memory section
**
** Outputs:
**      None
**
** Returns:
**      lpCache     pointer to memory mapped area
**      NULL        error
**      None
**
** Exceptions:
**      None
**
** Side Effects:
**      None
**
** History:
**
*/

static VNODECACHE *
init_vnode_cache(char * name)
{
HANDLE hMapping;
MIPC * ptMipc;
VNODECACHE * lpCache;
SECURITY_ATTRIBUTES	sa;
i4  mapsize = sizeof(MIPC);
char tempname[256];
char *ObjectPrefix;

    GVshobj(&ObjectPrefix);
    STprintf(tempname, "%s%s", ObjectPrefix, name);

    if (lpVnodeCache == NULL)
    {
        if (DetectCache(name) == FALSE)
        {
            iimksec(&sa);

            if ((hMapping = CreateFileMapping(INVALID_HANDLE_VALUE,
                                              &sa,
                                              PAGE_READWRITE,
                                              0,
                                              mapsize,
                                              tempname )) == NULL)
            {
                DWORD err = GetLastError( );
                return(NULL);
            }
		
            if ((ptMipc  = (MIPC * )MapViewOfFile(  hMapping,
                                                    FILE_MAP_ALL_ACCESS,
                                                    0,
                                                    0,
                                                    0)) == NULL)
            {
                return(NULL);
            }
		
            /* initialise the mutex name */
            MEfill(128,0,ptMipc->CacheMutex);
			STprintf(ptMipc->CacheMutex, "%sGCcacheMutex", ObjectPrefix);

            /* create the mutex */
            if ((hCacheMutex = CreateMutex(&sa,FALSE,
                                           ptMipc->CacheMutex)) == NULL)
            {
                DWORD err = GetLastError( );
                return(NULL);
            }
        }
        else
        {
            if ((hMapping = OpenFileMapping( FILE_MAP_ALL_ACCESS,
						TRUE, tempname)) == NULL)
		return(NULL);

	    if ((ptMipc = (MIPC * )MapViewOfFile(  hMapping,
                                                   FILE_MAP_ALL_ACCESS,
                                                   0,
                                                   0,
                                                   0)) == NULL)
                return(NULL);
            if ((hCacheMutex = OpenMutex(MUTEX_ALL_ACCESS,TRUE,
                                         ptMipc->CacheMutex))== NULL)
            {
                DWORD err = GetLastError( );
                return(NULL);
            }
        }
        lpCache = &ptMipc->vcache[0];

    }
    else
    {
        lpCache = lpVnodeCache;
    }
    return(lpCache);
}

/******************************************************************************
** Name:    detect_cache - try to setup a mapping to the memory area
**
** Description:
**      Function tries to setup a mapping to the memory area.  If it is
**      successful then the area has already been created and need only be
**      opened.
**
** Inputs:
**      name        pointer to name of shared memory section
**
** Outputs:
**      None
**
** Returns:
**      TRUE        mapping exsits
**      FALSE       mapping does not exist
**
** Exceptions:
**      None
**
** Side Effects:
**      None
**
** History:
**
*/

static BOOL
DetectCache( char *name )
{
    HANDLE	hMapping;
    char	*ObjectPrefix;
    char	tempname[256];

    GVshobj(&ObjectPrefix);
    STprintf(tempname, "%s%s", ObjectPrefix, name);

    if (( hMapping = OpenFileMapping(FILE_MAP_ALL_ACCESS,
                                     TRUE, tempname)) == NULL)
        return FALSE;

    CloseHandle( hMapping );
        return TRUE;
}

/******************************************************************************
** Name:    PasswdDlg - password dialog procedure
**
** Description:
**      Handles the controls on the dialog for username and password entry.
**
** Inputs:
**      hDlgWnd     Handle to the dialog window
**      wMsgID      Message to be handled
**      wParam
**      lParam
**
** Outputs:
**      szUsername
**      szPassword
**
** Returns:
**      TRUE    OK
**      FALSE   CANCEL
**
** Exceptions:
**      None
**
** Side Effects:
**      None
**
** History:
**
**	17-Dec-2009 (Bruce Lunsford) Sir 122536
**	    Add support for long identifiers; change hard-coded lengths
**	    (=32) to #defines GC_L_USER_ID and GC_L_PASSWORD.  Also get
**	    lengths of entered fields from GetDlgItemText() return value
**	    rather than executing STlength()s, which is not needed.
**	    Save the lengths for later checking against the input parm lengths.
*/

BOOL PASCAL
PasswdDlg (
    HWND hDlgWnd,
    WORD wMsgID,
    WORD wParam,
    LONG lParam)
{
char vnodetxt[80];

    switch(wMsgID)
    {
        case WM_INITDIALOG:
            sprintf(vnodetxt,"Enter Login Information for : %s",
                   (char *) lParam);
            SetDlgItemText(hDlgWnd, IDC_VNODE, vnodetxt);
            return TRUE;

        case WM_COMMAND:
            switch(wParam)
            {
                case IDOK:
                    lenUsername = GetDlgItemText(hDlgWnd, IDC_USER, szUsername, GC_L_USER_ID);
                    lenPassword = GetDlgItemText(hDlgWnd, IDC_PASSWD, szPassword, GC_L_PASSWORD);
                    if (!lenUsername || !lenPassword)
                    {
                        MessageBox(hDlgWnd, ERRMSG, NULL, MB_OK);
                    }
                    else
                    {
                        EndDialog(hDlgWnd, (int)TRUE);
                    }
                    return (IDOK);

                case IDCANCEL:
                    EndDialog(hDlgWnd, (int)FALSE);
                    return (IDCANCEL);
            }
            break;

        case WM_WINDOWPOSCHANGED:
        {
        WINDOWPOS * wp = (WINDOWPOS *)lParam;

            if ((wp->flags & SWP_SHOWWINDOW)  == SWP_SHOWWINDOW)
            {
                SetForegroundWindow(wp->hwnd);
                return(TRUE);
            }
            break;
        }
    }
    return FALSE;
}

/******************************************************************************
** Name:    GetConsoleHwnd - get a handle to the console window.
**
** Description:
**      Gets the current console title of which there may be more than one.
**      Then changes the title for a unique one and then does a find window
**      on the new title before restoring the window title.
**
** Inputs:
**      None.
**
** Outputs:
**      None
**
** Returns:
**      hwndFound   window found
**      NULL        no window found
**
** Exceptions:
**      None
**
** Side Effects:
**      None
**
** History:
**
*/

static
HWND GetConsoleHwnd(void)
{
#define MY_BUFSIZE 1024             /* buffer size for console window titles */
HWND hwndFound;                     /* this is what is returned to the caller*/
char pszNewWindowTitle[MY_BUFSIZE]; /* contains fabricated WindowTitle       */
char pszOldWindowTitle[MY_BUFSIZE]; /* contains original WindowTitle         */

    /* fetch current window title */

    GetConsoleTitle(pszOldWindowTitle, MY_BUFSIZE);

    /* format a "unique" NewWindowTitle */

    wsprintf(pszNewWindowTitle,"%d/%d",
             GetTickCount(),
             GetCurrentProcessId());

    /* change current window title */

    SetConsoleTitle(pszNewWindowTitle);

    /* ensure window title has been updated */

    Sleep(40);

    /* look for NewWindowTitle */

    hwndFound=FindWindow(NULL, pszNewWindowTitle);

    /* restore original window title */

    SetConsoleTitle(pszOldWindowTitle);

    return(hwndFound);
}

#ifndef CRYPT_SIZE
# define CRYPT_SIZE 8
#endif

#define    GCN_PASSWD_END    '0'


/*{
** Name: encrypt     - Encrypt A password
**
** Description:
**    This routine encrypts a user password. the encrypted password
**    is returned as a string.
**
** Inputs:
**    userid    - The user ID string. Max length GC_L_USER_ID
**    passwd  - The straight text password string. Max length, enough
**          to so as not to generate etxtbuff grater than GC_L_PASSWORD.
**          Max length is (GC_L_PASSWORD/2 -1) (ie. 31). Note that
**          NULL pointer or NULL sring will both result in NULL string
**          etxtbuff.
**
** Outputs:
**    etxtbuff - The encrypted text buffer string. Note that it
**           must have a max size of GC_L_PASSWORD
**
** Returns:
**    OK  - succeed
**    FAIL - buffer size problem.
**
** Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**
**      23-Mar-88 (Lin)
**          Initial function creation.
**      14-Nov-96 (fanra01)
**          This function was taken from the gcnencry.c file.
**
[@history_template@]...
*/
static STATUS
encrypt( char *userid, char *passwd, char *etxtbuff )
{
CI_KS ksch;
i4  ulen, plen, i, diff;
char *ptr;
char keystr[GC_L_USER_ID];
char pass[GC_L_PASSWORD], ebinbuff[GC_L_PASSWORD];

    if((userid == NULL) || (etxtbuff == NULL))
        return(FAIL);

    if((passwd == NULL) || (passwd[0] == EOS))
    {
        etxtbuff[0] = EOS;
        return(OK);
    }

    ulen = STlength(userid);
    if(ulen > (GC_L_USER_ID -1))
        return(FAIL);

    plen = STlength(passwd);
    if(plen > (GC_L_PASSWORD -1))
        return(FAIL);
    
    /* Assume that this password size
       will not exceed CI_TOTEXT_SIZE_MACRO()
    */
    if(plen > (GC_L_PASSWORD/2 -1))
        return(FAIL);

    /* Coerce key string to length CRYPT_SIZE
       or truncate if greater.
    */

    STcopy(userid,keystr);
    ptr = keystr + ulen;
    if (ulen < CRYPT_SIZE)    /* Pad with spaces */
    {
        for (i = 0 ; i < CRYPT_SIZE - ulen ; i++)
        {
            *ptr++ = ' ';
        }
    }
    keystr[CRYPT_SIZE] = '\0'; /* null-termination at CRYPT_SIZE chars */

    /* Add trailing marker to handle passwords with trailing SPACE */
    STcopy(passwd,pass);
    pass[plen++] = GCN_PASSWD_END;
    pass[plen]   = EOS;

    /* Pad password with spaces to modulo CRYPT_SIZE length. */
    diff = (plen % CRYPT_SIZE);
    if(diff !=0) 
    {
        diff = CRYPT_SIZE - diff;
        ptr = pass + plen;
        for (i = 0 ; i < diff; i++)
        {
            *ptr++ = ' ';
        }
        *ptr = '\0'; /* ensure null-termination */
    }

    CIsetkey(keystr, ksch);
   
    /* Encrypt the text */
    CIencode(pass, plen + diff, ksch, ebinbuff);

    /* Text the encrypted binary */
    CItotext(ebinbuff, plen + diff, etxtbuff);

    return(OK);
}

/*{
** Name: parse() - parse vnode::dname/class
**
** Description:
**	Parses the target name the user typed on the command line.
**
** Inputs:
**	in	the target name
**
** Outputs:
**	vnode	pointer to vnode component, or NULL
**	dbname	pointer to dbname component
**	class	pointer to class component, or NULL
**
** Side effects:
**	Puts null bytes into the 'in' buffer.
**
** Returns:
**	STATUS
**
** History:
*/

static STATUS
partnerparse( char *in, char **vnode, char **dbname, char **class )
{
char c;

static char null[] = "";

    *vnode = *class = null;
    *dbname = in;

    do switch( c = *in++ )
    {
        case '"':
            /*
            ** Copy literally + inclusively the quoted string.
            */
            do c = *in++;
            while( c && c != '"' );
            if( !c )
                return FAIL;
            break;
        case ':':
            /*
            ** Terminate vnode and start dbname.  Only for first ::.
            */
            if( *vnode != null )
                break;
            if( *in++ != ':' )
                return FAIL;
            in[-2] = '\0';
            *vnode = *dbname;
            *dbname = in;
            break;
        case '/':
            /*
            ** Terminate dbname and start class.  Only one / allowed.
            */
            if( *class != null )
                return FAIL;
            in[-1] = '\0';
            *class = in;

            if( *class[0] == '@' )
                return OK;
            break;
    }
    while( c );

return OK;
}
