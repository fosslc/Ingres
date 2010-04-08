/*
**
**  Name: OptionsPage.cpp
**
**  Description:
**	This file defines the functions needed for the Options Page.
**
**  History:
**	28-apr-1999 (somsa01)
**	    Created.
**    	22-jun-2004 (somsa01)
**          Cleaned up code for Open Source.
**	03-apr-2008 (drivi01)
**	    Put spatial objects support back.
**	06-May-2008 (drivi01)
**	    Apply Wizard97 template to iilink.
*/

#include "stdafx.h"
#include "iilink.h"
#include "OptionsPage.h"
#include "ChooseDir.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
** COptionsPage property page
*/
IMPLEMENT_DYNCREATE(COptionsPage, CPropertyPage)

COptionsPage::COptionsPage() : CPropertyPage(COptionsPage::IDD)
{
    //{{AFX_DATA_INIT(COptionsPage)
	// NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

COptionsPage::COptionsPage(CPropertySheet *ps) : CPropertyPage(COptionsPage::IDD)
{
    m_propertysheet = ps;
    m_psp.dwFlags &= ~(PSP_HASHELP);
	m_psp.dwFlags |= PSP_DEFAULT|PSP_HIDEHEADER;
}

COptionsPage::~COptionsPage()
{
}

void COptionsPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(COptionsPage)
    DDX_Control(pDX, IDC_IMAGE, m_image);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(COptionsPage, CPropertyPage)
    //{{AFX_MSG_MAP(COptionsPage)
    ON_BN_CLICKED(IDC_CHECK_SPATIAL, OnCheckSpatial)
    ON_BN_CLICKED(IDC_CHECK_USER_UDT, OnCheckUserUdt)
    ON_BN_CLICKED(IDC_BUTTON_LOC, OnButtonLoc)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** COptionsPage message handlers
*/
void
COptionsPage::OnCheckSpatial() 
{
    CButton	*pCheckSpatial = (CButton *)GetDlgItem(IDC_CHECK_SPATIAL);
    CButton	*pCheckDemos = (CButton *)GetDlgItem(IDC_CHECK_DEMOS);

    if (pCheckSpatial->GetCheck())
    {
	DemosChecked = pCheckDemos->GetCheck();
	pCheckDemos->SetCheck(TRUE);
	pCheckDemos->EnableWindow(FALSE);
    }
    else
    {
	pCheckDemos->SetCheck(DemosChecked);
	pCheckDemos->EnableWindow(TRUE);
    }	
}

void
COptionsPage::OnCheckUserUdt() 
{
    CButton	*pCheckUserUdt = (CButton *)GetDlgItem(IDC_CHECK_USER_UDT);

    UseUserUdts = pCheckUserUdt->GetCheck();
    if (UseUserUdts)
    {
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_PERSONAL_LOC), TRUE);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_LOCATION), TRUE);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_BUTTON_LOC), TRUE);
    }
    else
    {
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_PERSONAL_LOC), FALSE);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_LOCATION), FALSE);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_BUTTON_LOC), FALSE);
    }	
}

BOOL
COptionsPage::OnSetActive() 
{
    m_propertysheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);	
    return CPropertyPage::OnSetActive();
}

BOOL
COptionsPage::OnInitDialog() 
{
    CString	Message;

    CPropertyPage::OnInitDialog();
	
    Message.LoadString(IDS_SPATIAL_DESC);
    SetDlgItemText(IDC_SPATIAL_DESC, Message);
    Message.LoadString(IDS_CHECK_SPATIAL);
    SetDlgItemText(IDC_CHECK_SPATIAL, Message);
    Message.LoadString(IDS_DEMO_UDT_DESC);
    SetDlgItemText(IDC_DEMO_UDT_DESC, Message);
    Message.LoadString(IDS_CHECK_DEMOS);
    SetDlgItemText(IDC_CHECK_DEMOS, Message);
    Message.LoadString(IDS_USER_UDT_DESC);
    SetDlgItemText(IDC_USER_UDT_DESC, Message);
    Message.LoadString(IDS_CHECK_USER_UDT);
    SetDlgItemText(IDC_CHECK_USER_UDT, Message);
    Message.LoadString(IDS_PERSONAL_LOC);
    SetDlgItemText(IDC_PERSONAL_LOC, Message);
    Message.LoadString(IDS_BUTTON_LOC);
    SetDlgItemText(IDC_BUTTON_LOC, Message);

    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_PERSONAL_LOC), FALSE);
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_LOCATION), FALSE);
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_BUTTON_LOC), FALSE);
	
    UserLoc = "";
	
    return TRUE;
}

void
COptionsPage::OnButtonLoc() 
{
    CString	Message, name;

    Message.LoadString(IDS_TITLE);
    CChooseDir	cdir(Message);

    GetDlgItemText(IDC_EDIT_LOCATION, cdir.m_path);
    if (cdir.DoModal() == IDOK)
	SetDlgItemText(IDC_EDIT_LOCATION, cdir.m_path);	
}

LRESULT
COptionsPage::OnWizardNext() 
{
    CFileFind	fFind;
    CString	OldLoc, Message, Message2;
    CButton	*pCheckSpat = (CButton *)GetDlgItem(IDC_CHECK_SPATIAL);
    CButton	*pCheckDemo = (CButton *)GetDlgItem(IDC_CHECK_DEMOS);

    IncludeSpat = pCheckSpat->GetCheck();
    IncludeDemo = pCheckDemo->GetCheck();

    /*
    ** The user must chose at least one option.
    */
    if (!IncludeDemo && !IncludeSpat && !UseUserUdts)
    {
	Message.LoadString(IDS_CHECK_SOMETHING);
	Message2.LoadString(IDS_TITLE);
	MessageBox(Message, Message2, MB_ICONEXCLAMATION | MB_OK);
	return -1;
    }
    
    /*
    ** If we are linking with spatial objects, then we must also
    ** link with the demo UDTs.
    */
    if (IncludeSpat)
	IncludeDemo = TRUE;

    if (UseUserUdts)
    {
	GetDlgItemText(IDC_EDIT_LOCATION, Message);
	OldLoc = UserLoc;
	UserLoc = Message;
	if (UserLoc == "")
	{
	    Message.LoadString(IDS_NOSRC_DIR2);
	    Message2.LoadString(IDS_TITLE);
	    MessageBox(Message, Message2, MB_ICONINFORMATION | MB_OK);
	    return -1;
	}
	else if (GetFileAttributes(UserLoc) == -1)
	{
	    Message.LoadString(IDS_SRCDIR_INVALID);
	    Message2.LoadString(IDS_TITLE);
	    MessageBox(Message, Message2, MB_ICONINFORMATION | MB_OK);
	    return -1;
	}

	Message = UserLoc + CString("\\*.c");
	if (!fFind.FindFile(Message, 0))
	{
	    fFind.Close();
	    Message = UserLoc + CString("\\*.obj");
	    if (!fFind.FindFile(Message, 0))
	    {
		fFind.Close();
		Message = UserLoc + CString("\\*.lib");
		if (!fFind.FindFile(Message, 0))
		{
		    fFind.Close();
		    Message.LoadString(IDS_SRCDIR_NOSOURCE);
		    Message2.LoadString(IDS_TITLE);
		    MessageBox(Message, Message2, MB_ICONINFORMATION | MB_OK);
		    return -1;
		}
	    }
	}

	fFind.Close();
	LocChanged = ((OldLoc == UserLoc) ? FALSE : TRUE);
	return CPropertyPage::OnWizardNext();
    }
    else
	return IDD_FINAL_PAGE;
}
