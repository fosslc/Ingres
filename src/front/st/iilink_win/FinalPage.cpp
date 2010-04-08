/*
**
**  Name: FinalPage.cpp
**
**  Description:
**	This file contains all functions needed to display the Final
**	wizard page.
**
**  History:
**	23-apr-1999 (somsa01)
**	    Created.
**	02-mar-2001 (gupsh01)
**	    Added nvarchar udt.
**	01-may-2001 (somsa01)
**	    Properly added nvarchar object file.
**	07-dec-2001 (somsa01)
**	    Libraries on Win64 now have a "64" in their names.
**	26-dec-2001 (somsa01)
**	    Corrected compile string for Win64.
**	26-feb-2004 (somsa01)
**	    Updated compile string for Win64. Also, distinguish between
**	    64-bit Windows and 64-bit WIndows on Itanium, when needed.
**	22-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	09-sep-2004 (somsa01)
**	    iiudtnt* becomes iilibudt*.
**	03-apr-2008 (drivi01)
**	    Put spatial objects support back.
**	06-May-2008 (drivi01)
**	    Apply Wizard97 template to iilink.
**	01-Oct-2009 (drivi01)
**	    Update the compiler flags and add command for manifesting
**	    the shared libraries.  This will ensure that the dependencies
**	    are setup correctly.
*/

#include "stdafx.h"
#include "iilink.h"
#include "FinalPage.h"
#include "WaitDlg.h"
#include "FailDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
** FinalPage property page
*/
IMPLEMENT_DYNCREATE(FinalPage, CPropertyPage)

FinalPage::FinalPage() : CPropertyPage(FinalPage::IDD)
{
    //{{AFX_DATA_INIT(FinalPage)
	//}}AFX_DATA_INIT
}

FinalPage::FinalPage(CPropertySheet *ps) : CPropertyPage(FinalPage::IDD)
{
    m_propertysheet = ps;
    m_psp.dwFlags &= ~(PSP_HASHELP);
	m_psp.dwFlags |= PSP_DEFAULT|PSP_HIDEHEADER;
}

FinalPage::~FinalPage()
{
}

void FinalPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(FinalPage)
    DDX_Control(pDX, IDC_EDIT_LINK, m_LinkStmt);
    DDX_Control(pDX, IDC_EDIT_COMPILE, m_CompileStmt);
    DDX_Control(pDX, IDC_IMAGE, m_image);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(FinalPage, CPropertyPage)
    //{{AFX_MSG_MAP(FinalPage)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** FinalPage message handlers
*/
BOOL
FinalPage::OnSetActive() 
{
    CString II_SYSTEM = getenv("II_SYSTEM");
    struct node *NodePtr;

    /*
    ** Set the compilation and link commands.
    */
    CompileCmd = "";
#ifdef WIN64
    LinkCmd = CString("link /NOLOGO /DLL /DEF:iilibudt64.def ") +
	      CString("/OUT:IILIBUDT64.DLL ");
#else
    LinkCmd = CString("link /NOLOGO /DLL /DEF:iilibudt.def ") +
	      CString("/OUT:IILIBUDT.DLL ");
#endif  /* WIN64 */
    if (IncludeDemo || UseUserUdts)
    {
#ifdef NT_IA64
	CompileCmd = CString("cl -c -GD -DIA64=1 -D_IA64_ -DNT_GENERIC -DNT_IA64 /O2 /G2 -D_DLL -D_MT /Wp64 ") +
#else
	CompileCmd = CString("cl -c -DNT_GENERIC -Od -D_DLL -D_MT ") +
#endif  /* Win64 */
		     CString("/nologo -MD -I. -I\"") +
		     II_SYSTEM + CString("\\ingres\\files\"");
	if (IncludeDemo)
	{
	    CompileCmd +=
		CString(" common.c cpx.c intlist.c op.c zchar.c numeric.c nvarchar.c ");
	}
	if (UseUserUdts)
	{
	    /*
	    ** Add the user UDT source files.
	    */
	    NodePtr = FileSrcList;
	    while (NodePtr)
	    {
		CompileCmd += CString(" \"") + UserLoc + CString("\\") +
			      CString(NodePtr->filename) + CString("\"");
		NodePtr = NodePtr->next;
	    }
	}

	if (IncludeDemo)
	{
	    LinkCmd +=  CString("common.obj cpx.obj intlist.obj op.obj ") +
			CString("zchar.obj numeric.obj iicvpk.obj iimhpk.obj nvarchar.obj ");
	}
	LinkCmd += CString("msvcrt.lib kernel32.lib user32.lib \"") +
#ifdef WIN64
		   II_SYSTEM + CString("\\ingres\\lib\\libingres64.lib\"");
#else
		   II_SYSTEM + CString("\\ingres\\lib\\libingres.lib\"");
#endif  /* WIN64 */
	if (IncludeSpat)
	{
	    LinkCmd += CString(" \"") + II_SYSTEM +
#ifdef WIN64
		       CString("\\ingres\\lib\\libspat64.lib\"");
#else
		       CString("\\ingres\\lib\\libspat.lib\"");
#endif  /* WIN64 */
	}
	else
	{
	    LinkCmd += CString(" \"") + II_SYSTEM +
		       CString("\\ingres\\lib\\iiclsadt.obj\"");
	}
	if (UseUserUdts)
	{
	    /*
	    ** Add the User UDT object / library files.
	    */
	    NodePtr = FileObjList;
	    while (NodePtr)
	    {
		LinkCmd += CString(" \"") + UserLoc + CString("\\") +
			   CString(NodePtr->filename) + CString("\"");
		NodePtr = NodePtr->next;
	    }
	}
    }
    
    if ((!IncludeSpat) && (!IncludeDemo) && (!UseUserUdts))
    {
	LinkCmd += CString("msvcrt.lib kernel32.lib user32.lib \"") +
		   II_SYSTEM + CString("\\ingres\\lib\\iiclsadt.obj\" \"") +
		   II_SYSTEM + CString("\\ingres\\lib\\iiuseradt.obj\"");
    }

	/* Build manifest file containing the dependencies into a shared library */
	ManifestCmd = CString("mt.exe -nologo -manifest iilibudt.dll.manifest -outputresource:iilibudt.dll;#2");

    m_CompileStmt.SetWindowText(CompileCmd);
    m_LinkStmt.SetWindowText(LinkCmd);

    m_propertysheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);	
    return CPropertyPage::OnSetActive();
}

LRESULT
FinalPage::OnWizardBack() 
{
    if (m_CompileStmt.GetModify() || m_LinkStmt.GetModify())
    {
	int	status;
	CString	Message, Message2;

	Message.LoadString(IDS_NOSAVE_CHANGES);
	Message2.LoadString(IDS_TITLE);
	status = MessageBox(Message, Message2,
			    MB_ICONINFORMATION | MB_YESNO);
	if (status == IDNO)
	    return -1;
    }

    if (!UseUserUdts)
	return IDD_OPTIONS_PAGE;
    else
	return CPropertyPage::OnWizardBack();
}

BOOL
FinalPage::OnWizardFinish() 
{
    CString	UdtDir, II_SYSTEM = getenv("II_SYSTEM");
    CWaitDlg	wait;
    FailDlg	fail;
    HANDLE	hThread;
    DWORD	ThreadID, status = 0;
    CString	Message, Message2;

    /*
    ** Change directory to the demo UDT directory.
    */
    UdtDir = II_SYSTEM + CString("\\ingres\\demo\\udadts");
    SetCurrentDirectory(UdtDir);

    /*
    ** Now, obtain any changes in the commands.
    */
    m_CompileStmt.GetWindowText(CompileCmd);
    m_LinkStmt.GetWindowText(LinkCmd);

    /*
    ** Now, execute the commands.
    */
    hThread = CreateThread( NULL, 0,
			    (LPTHREAD_START_ROUTINE)ExecuteCmds,
			    NULL, 0, &ThreadID);
    wait.DoModal();
    while (WaitForSingleObject(hThread, 0) != WAIT_OBJECT_0);
    GetExitCodeThread(hThread, &status);
    if (status)
    {
	fail.DoModal();
	return 0;
    }

    /*
    ** Move the new DLL into place if Ingres is not running.
    */
    if (!IngresUp)
    {
	if (Backup)
	{
#ifdef WIN64
	    CString	BackupDll = CString("..\\..\\bin\\iilibudt64.")+BackupExt;
	    if (!CopyFile("..\\..\\bin\\iilibudt64.DLL", BackupDll, FALSE))
#else
	    CString	BackupDll = CString("..\\..\\bin\\iilibudt.")+BackupExt;
	    if (!CopyFile("..\\..\\bin\\iilibudt.DLL", BackupDll, FALSE))
#endif  /* WIN64 */
	    {
		LPVOID lpMsgBuf;
		FormatMessage(  FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR) &lpMsgBuf, 0, NULL );
		Message2.LoadString(IDS_TITLE);
		Message.LoadString(IDS_FAIL_BACKUP);
		Message += CString((LPTSTR)lpMsgBuf);
		MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
		LocalFree(lpMsgBuf);
		return 0;
	    }
	}

#ifdef WIN64
	if (!CopyFile("iilibudt64.DLL", "..\\..\\bin\\iilibudt64.DLL", FALSE))
#else
	if (!CopyFile("iilibudt.DLL", "..\\..\\bin\\iilibudt.DLL", FALSE))
#endif  /* WIN64 */
	{
	    LPVOID lpMsgBuf;
	    FormatMessage(  FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			    FORMAT_MESSAGE_FROM_SYSTEM |
			    FORMAT_MESSAGE_IGNORE_INSERTS,
			    NULL, GetLastError(),
			    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			    (LPTSTR) &lpMsgBuf, 0, NULL );
	    Message2.LoadString(IDS_TITLE);
	    Message.LoadString(IDS_FAIL_REPLACE);
	    Message += CString((LPTSTR)lpMsgBuf);
	    MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	    LocalFree(lpMsgBuf);
	    return 0;
	}

	Message.LoadString(IDS_SUCCESS);
	Message2.LoadString(IDS_TITLE);
	MessageBox(Message, Message2, MB_OK | MB_ICONINFORMATION);	
    }
    else
    {
	char msgout[2048];

	Message.LoadString(IDS_SUCCESS_INGUP);
	sprintf(msgout, Message, getenv("II_SYSTEM"), getenv("II_SYSTEM"));
	Message2.LoadString(IDS_TITLE);
	MessageBox(msgout, Message2, MB_OK | MB_ICONINFORMATION);
    }

    return CPropertyPage::OnWizardFinish();
}

BOOL
FinalPage::OnInitDialog() 
{
    CString	Message;

    CPropertyPage::OnInitDialog();

    Message.LoadString(IDS_FINAL_HEADING);
    SetDlgItemText(IDC_FINAL_HEADING, Message);
    Message.LoadString(IDS_COMPILE_MSG);
    SetDlgItemText(IDC_COMPILE_MSG, Message);
    Message.LoadString(IDS_LINK_MSG);
    SetDlgItemText(IDC_LINK_MSG, Message);

    return TRUE;
}
