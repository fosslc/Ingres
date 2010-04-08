/*
** Name: ServerSelect.cpp
**
** Description:
**	This file contains the routines for implememting the server selection
**	tree.
**
** History:
**	17-sep-1999 (somsa01)
**	    Created.
**	15-oct-1999 (somsa01)
**	    The Server selection page is now the final page.
**	21-oct-1999 (somsa01)
**	    Moved creation of the chart file to here as the final step.
**	02-nov-1999 (somsa01)
**	    While waiting for winstart to complete, added a sleep to
**	    prevent the main application from taking up 100% CPU.
**	24-jan-2000 (somsa01)
**	    In CreateChartThread(), return the different values from
**	    CreatePerfChart().
**	27-jan-2000 (somsa01)
**	    In OnSelchangedTreeServers(), if the server class is embedded
**	    within the server id, then this is a PC vnode. Therefore, we
**	    must change the IMA server id search to have an uppercase
**	    hostname. Also, added Sleep(0 call while waiting for threads.
**	31-jan-2000 (somsa01)
**	    Send to "perfmon" the chart name in quotes.
**	27-nov-2001 (somsa01)
**	    In CServerSelect::FillNodeRoot(), make sure we set
**	    item.cChildren to 1.
**	07-May-2008 (drivi01)
**	    Apply Wizard97 template to this utility.
*/

#include "stdafx.h"
#include "perfwiz.h"
#include "ServerSelect.h"
#include "gcnquery.h"
#include "WaitDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int  GetVnodeThread();
static int  CreateChartThread();

extern "C" struct nodelist	*nodeptr;

extern int	ReadInitFile();
extern int	CreatePerfChart(CString ChartName);
extern void	Cleanup();
extern "C" long	ping_iigcn();
extern "C" int	GetVnodes();
extern "C" void	ListDestructor();
extern "C" int	GetServerList(char target[128]);

/*
** CServerSelect property page
*/
IMPLEMENT_DYNCREATE(CServerSelect, CPropertyPage)

CServerSelect::CServerSelect() : CPropertyPage(CServerSelect::IDD)
{
    //{{AFX_DATA_INIT(CServerSelect)
	    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

CServerSelect::CServerSelect(CPropertySheet *ps) : CPropertyPage(CServerSelect::IDD)
{
    m_ImageList.Create (IDB_NODETREE, 16, 4, (COLORREF)RGB(255,0,255));
    m_propertysheet = ps;
	m_psp.dwFlags |= PSP_DEFAULT|PSP_HIDEHEADER;
}

CServerSelect::~CServerSelect()
{
    m_ImageList.DeleteImageList();
}

void CServerSelect::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CServerSelect)
    DDX_Control(pDX, IDC_TREE_SERVERS, m_ServerList);
    DDX_Control(pDX, IDC_IMAGE, m_image);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CServerSelect, CPropertyPage)
    //{{AFX_MSG_MAP(CServerSelect)
    ON_BN_CLICKED(IDC_BUTTON_REFRESH, OnButtonRefresh)
    ON_NOTIFY(TVN_ITEMEXPANDING, IDC_TREE_SERVERS, OnItemexpandingTreeServers)
    ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_SERVERS, OnSelchangedTreeServers)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** CServerSelect message handlers
*/
BOOL
CServerSelect::OnSetActive() 
{
    m_propertysheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);
    return CPropertyPage::OnSetActive();
}

void
CServerSelect::OnButtonRefresh() 
{
    CString	Message, Message2;
    CWaitDlg	wait;
    HANDLE	hThread;
    DWORD	ThreadID, status;
    
    m_ServerList.DeleteAllItems();

    if (ping_iigcn())
    {
	STARTUPINFO	    siStInfo;
	PROCESS_INFORMATION piProcInfo;

	Message.LoadString(IDS_INGRES_DOWN);
	Message2.LoadString(IDS_TITLE);
	status = MessageBox(Message, Message2,
			    MB_ICONQUESTION | MB_YESNO);
	if (status == 7)
	    goto CONTINUE2;

	memset (&siStInfo, 0, sizeof (siStInfo)) ;

	siStInfo.cb          = sizeof (siStInfo);
	siStInfo.lpReserved  = NULL;
	siStInfo.lpReserved2 = NULL;
	siStInfo.cbReserved2 = 0;
	siStInfo.lpDesktop   = NULL;
	siStInfo.dwFlags     = STARTF_USESHOWWINDOW;
	siStInfo.wShowWindow = SW_SHOW;

        CreateProcess(NULL, "winstart.exe /start", NULL, NULL, TRUE, 0,
		      NULL, NULL, &siStInfo, &piProcInfo);

	/*
	** Cause repaints to the two windows while
	** winstart is up.
	*/
	EnableWindow(FALSE);
	while (WaitForSingleObject(piProcInfo.hProcess, 0) != WAIT_OBJECT_0)
	{
	    UpdateWindow();
	    GetParent()->UpdateWindow();
	    Sleep(200);
	}
	EnableWindow(TRUE);
    }

    wait.m_WaitHeading.LoadString(IDS_GETVNODE_WAIT);
    MainWnd = NULL;
    hThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)GetVnodeThread,
			    NULL, 0, &ThreadID );
    wait.DoModal();
    while (WaitForSingleObject(hThread, 0) != WAIT_OBJECT_0)
		Sleep(200);
    GetExitCodeThread(hThread, &status);

    if (status)
    {
	char	outmsg[2048];

	Message.LoadString(IDS_GETVNODE_ERR);
	sprintf(outmsg, Message, status);
	Message2.LoadString(IDS_TITLE);
	MessageBox(outmsg, Message2, MB_OK | MB_ICONEXCLAMATION);
    }

CONTINUE2:
    FillNodeRoot();
}

BOOL
CServerSelect::OnInitDialog() 
{
    CString	Message, Message2;
    CWaitDlg	wait;
    HANDLE	hThread;
    DWORD	ThreadID, status;

    CPropertyPage::OnInitDialog();

    Message.LoadString(IDS_SERVER_HEADING);
    SetDlgItemText(IDC_SERVER_HEADING, Message);
    Message.LoadString(IDS_BUTTON_REFRESH);
    SetDlgItemText(IDC_BUTTON_REFRESH, Message);
    Message.LoadString(IDS_SERVER_COMBO);
    SetDlgItemText(IDS_SERVER_COMBO, Message);

    m_ServerList.SetImageList(&m_ImageList, TVSIL_NORMAL);

    if (ping_iigcn())
    {
	STARTUPINFO	    siStInfo;
	PROCESS_INFORMATION piProcInfo;

	/*
	** Ask if the user wants to start the installation.
	*/
	Message.LoadString(IDS_INGRES_DOWN);
	Message2.LoadString(IDS_TITLE);
	status = MessageBox(Message, Message2,
			    MB_ICONQUESTION | MB_YESNO);
	if (status == 7)
	    goto CONTINUE;

	/*
	** Spawn off winstart.
	*/
	memset (&siStInfo, 0, sizeof (siStInfo)) ;

	siStInfo.cb          = sizeof (siStInfo);
	siStInfo.lpReserved  = NULL;
	siStInfo.lpReserved2 = NULL;
	siStInfo.cbReserved2 = 0;
	siStInfo.lpDesktop   = NULL;
	siStInfo.dwFlags     = STARTF_USESHOWWINDOW;
	siStInfo.wShowWindow = SW_SHOW;

        CreateProcess(NULL, "winstart.exe /start", NULL, NULL, TRUE, 0,
		      NULL, NULL, &siStInfo, &piProcInfo);

	/*
	** Cause repaints to the two windows while
	** winstart is up.
	*/
	EnableWindow(FALSE);
	while (WaitForSingleObject(piProcInfo.hProcess, 0) != WAIT_OBJECT_0)
	{
	    UpdateWindow();
	    GetParent()->UpdateWindow();
	    Sleep(200);
	}
	EnableWindow(TRUE);
    }

    wait.m_WaitHeading.LoadString(IDS_GETVNODE_WAIT);
    MainWnd = NULL;
    hThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)GetVnodeThread,
			    NULL, 0, &ThreadID );
    wait.DoModal();
    while (WaitForSingleObject(hThread, 0) != WAIT_OBJECT_0)
		Sleep(200);
    GetExitCodeThread(hThread, &status);

    if (status)
    {
	char	outmsg[2048];

	Message.LoadString(IDS_GETVNODE_ERR);
	sprintf(outmsg, Message, status);
	Message2.LoadString(IDS_TITLE);
	MessageBox(outmsg, Message2, MB_OK | MB_ICONEXCLAMATION);
    }

CONTINUE:
    FillNodeRoot();
    
    return TRUE;
}

void
CServerSelect::FillNodeRoot()
{
    TV_ITEM	item;
    HTREEITEM	hItem;

    item.hItem = m_ServerList.InsertItem("Nodes", TVI_ROOT);
    item.mask = TVIF_CHILDREN | TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    item.iImage = item.iSelectedImage = 0;
    item.cChildren = 1;
    m_ServerList.SetItem(&item);

    hItem = m_ServerList.GetNextItem(0, TVGN_ROOT);
    m_ServerList.SelectItem(hItem);

    m_ServerList.Expand(m_ServerList.GetRootItem(), TVE_EXPAND);
}

BOOL
CServerSelect::FillNodes(HTREEITEM hParent)
{
    TV_ITEM	    item;
    struct nodelist *tmpptr = nodeptr;
    CWaitCursor	    wait;
    BOOL	    NodeFilled = FALSE;

    while (tmpptr)
    {
	item.hItem = m_ServerList.InsertItem(tmpptr->nodename, hParent);
	item.mask = TVIF_CHILDREN | TVIF_HANDLE| TVIF_IMAGE |
		    TVIF_SELECTEDIMAGE; 
	item.iImage = item.iSelectedImage = 9;
	item.cChildren = 1;
	m_ServerList.SetItem(&item);
	NodeFilled = TRUE;

	tmpptr = tmpptr->next;
    }

    return(NodeFilled);
}

void
CServerSelect::OnItemexpandingTreeServers(NMHDR* pNMHDR, LRESULT* pResult) 
{
    NM_TREEVIEW*    pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    TV_ITEM	    tvi;
    HTREEITEM	    hItem;
    BOOL	    status;

    hItem = m_ServerList.GetParentItem(pNMTreeView->itemNew.hItem);
    if (pNMTreeView->action == TVE_EXPAND)
    {
	if (!m_ServerList.GetChildItem(pNMTreeView->itemNew.hItem))
	{
	    if (!hItem)
		status = FillNodes(pNMTreeView->itemNew.hItem);
	    else
		FillServers(pNMTreeView->itemNew.hItem);
	}
    }

    if (!hItem)
    {
	/*
	** If it is a root folder, show open or closed folder
	*/
	tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE; 
	tvi.hItem = (pNMTreeView->itemNew).hItem;
	    
	if ((pNMTreeView->action == TVE_COLLAPSE) || !status)
	{ 
	    tvi.iImage = 0;    /* Closed Folder */
	    tvi.iSelectedImage = 0;
	    m_ServerList.SetItem(&tvi);
	}
	else if (pNMTreeView->action == TVE_EXPAND)
	{
	    tvi.iImage = 1;   /* Open Folder */
	    tvi.iSelectedImage = 1;
	    m_ServerList.SetItem(&tvi);
	}
    }

    *pResult = 0;
}

void
CServerSelect::FillServers(HTREEITEM hParent)
{
    HTREEITEM		hItem;
    TV_ITEM		item;
    CHAR		target[128];
    struct serverlist   *tmpptr;
    struct nodelist	*nptr = nodeptr;
    int			status;
    CString		Message, Message2;
    CWaitCursor		wait;

    hItem = m_ServerList.GetParentItem(hParent);

    strcpy(target, m_ServerList.GetItemText(hParent));
    status = GetServerList(target);
    if (status)
    {
	Message.LoadString(IDS_CANNOT_CONNECT);
	Message2.LoadString(IDS_TITLE);
	MessageBox(Message, Message2, MB_OK | MB_ICONINFORMATION);
	return;
    }
    else if (status < 0)
    {
	char	msgout[512];

	Message.LoadString(IDS_SQL_ERROR);
	sprintf(msgout, Message, status);
	Message2.LoadString(IDS_TITLE);
	MessageBox(msgout, Message2, MB_OK | MB_ICONINFORMATION);
	return;
    }

    while (strcmp(nptr->nodename, target))
	nptr = nptr->next;
    if (!nptr->svrptr)
    {
	Message.LoadString(IDS_NO_SERVERS);
	Message2.LoadString(IDS_TITLE);
	MessageBox(Message, Message2, MB_OK | MB_ICONINFORMATION);
	return;
    }

    tmpptr = nptr->svrptr;
    while (tmpptr)
    {
	if (!strcmp(tmpptr->server_class, "IUSVR"))
	    sprintf(target, "%s  (RECOVERY)", tmpptr->server_id);
	else if (!strcmp(tmpptr->server_class, "ICESVR"))
	    sprintf(target, "%s  (ICE)", tmpptr->server_id);
	else if (!strcmp(tmpptr->server_class, "INGRES"))
	    sprintf(target, "%s  (INGRES)", tmpptr->server_id);
	else if (!strcmp(tmpptr->server_class, "STAR"))
	    sprintf(target, "%s  (STAR)", tmpptr->server_id);

	item.hItem = m_ServerList.InsertItem(target, hParent);
	item.mask = TVIF_CHILDREN | TVIF_HANDLE| TVIF_IMAGE |
		    TVIF_SELECTEDIMAGE;
	if (!strcmp(tmpptr->server_class, "IUSVR"))
	    item.iImage = item.iSelectedImage = 13;
	else if (!strcmp(tmpptr->server_class, "INGRES"))
	    item.iImage = item.iSelectedImage = 4;
	else if (!strcmp(tmpptr->server_class, "STAR"))
	    item.iImage = item.iSelectedImage = 8;
	else if (!strcmp(tmpptr->server_class, "ICESVR"))
	    item.iImage = item.iSelectedImage = 6;

	item.cChildren = 0;
	m_ServerList.SetItem(&item);

	tmpptr = tmpptr->next;
    }
}

void
CServerSelect::OnSelchangedTreeServers(NMHDR* pNMHDR, LRESULT* pResult) 
{
    NM_TREEVIEW*    pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    HTREEITEM	    hItem, hItem2;
    char	    server_id[65], *cptr, search_str[40];
    CString	    Message;
    struct nodelist *nptr;
    struct serverlist	*sptr;

    hItem = m_ServerList.GetParentItem(pNMTreeView->itemNew.hItem);

    if (hItem)
    {
	if ((hItem2 = m_ServerList.GetParentItem(hItem)))
	{
	    /* This is the Ingres server id that's selected. */
	    nptr = nodeptr;
	    while (strcmp(nptr->nodename, m_ServerList.GetItemText(hItem)))
		nptr = nptr->next;
	    ChosenVnode = CString(nptr->nodename);
	    strcpy( server_id,
		    m_ServerList.GetItemText(pNMTreeView->itemNew.hItem));
	    ChosenServer = CString(server_id);
	    cptr = strchr(server_id, ' ');
	    *cptr = '\0';

	    /*
	    ** If the server class is embedded within the server id, change
	    ** the hostname to all uppercase for IMA queries.
	    */
	    sptr = nptr->svrptr;
	    do
	    {
		sprintf(search_str, "\\%s\\", sptr->server_class);
		if (strstr(server_id, search_str))
		    break;
		sptr = sptr->next;
	    } while (sptr);

	    if (sptr)
		Message = CString(strupr(nptr->hostname));
	    else
		Message = CString(nptr->hostname);    

	    Message += CString("::/@") + CString(server_id);
	}
	else
	    Message = "None Selected";

	SetDlgItemText(IDC_SERVER_ID, Message);
    }
    else
	SetDlgItemText(IDC_SERVER_ID, "None Selected");
    
    *pResult = 0;
}

static int
GetVnodeThread()
{
    int	status;

    status = GetVnodes();

    Sleep(2000);
    while (!MainWnd);
    ::SendMessage(MainWnd, WM_CLOSE, 0, 0);
    return(status);
}

BOOL CServerSelect::OnWizardFinish() 
{
    CString CmdLine, ServerID, Message, Message2;
    
    GetDlgItemText(IDC_SERVER_ID, ServerID);
    if (ServerID == "None Selected")
    {
	Message.LoadString(IDS_NO_SERVER_SELECTED);
	Message2.LoadString(IDS_TITLE);
	MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	return 0;
    }
    else
    {
	if (ChosenVnode != "(local)")
	    SetEnvironmentVariable("II_PF_VNODE", ChosenVnode);
	SetEnvironmentVariable("II_PF_SERVER", ServerID);
    }

    /*
    ** Write out the chart file, if chosen.
    */
    if (!UseExisting)
    {
	CWaitDlg    wait;
	HANDLE	    hThread;
	DWORD	    ThreadID, status = 0;

	wait.m_WaitHeading.LoadString(IDS_CREATE_CHARTFILE);
	MainWnd = NULL;
	hThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)CreateChartThread,
				NULL, 0, &ThreadID );
	wait.DoModal();
	while (WaitForSingleObject(hThread, 0) != WAIT_OBJECT_0)
		Sleep(200);
	GetExitCodeThread(hThread, &status);
	if (status)
	{
	    char    *outmsg;

	    Message.LoadString(IDS_CANNOT_CREATE_CHART);
	    outmsg = (char *)malloc(strlen(Message) + 16);
	    sprintf(outmsg, Message, status);
	    Message2.LoadString(IDS_TITLE);
	    MessageBox(outmsg, Message2, MB_OK | MB_ICONEXCLAMATION);
	    free(outmsg);
	    return 0;
	}
    }

    /*
    ** Run the Performance Monitor.
    */
    CmdLine = CString("perfmon.exe \"") + ChartName + CString("\"");
    WinExec(CmdLine, SW_SHOW);

    Cleanup();

    return CPropertyPage::OnWizardFinish();
}

static int
CreateChartThread()
{
    int	status;

    /*
    ** Write out the PerfMon chart file (.pmc) with the counters selected.
    */
    if ((status = CreatePerfChart(ChartName)))
    {
	while (!MainWnd);
	::SendMessage(MainWnd, WM_CLOSE, 0, 0);
	return(status);
    }

    Sleep(2000);
    while (!MainWnd);
    ::SendMessage(MainWnd, WM_CLOSE, 0, 0);
    return 0;
}

LRESULT CServerSelect::OnWizardBack() 
{
    if (!UseExisting)
	return CPropertyPage::OnWizardBack();
    else
    {
	if (PersonalGroup)
	    return IDD_PERSONAL_HELP_PAGE;
	else
	    return IDD_INTRO_PAGE;
    }
}
