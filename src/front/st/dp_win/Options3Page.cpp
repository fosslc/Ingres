/*
**
**  Name: Options3Page.cpp
**
**  Description:
**	This file contains the necessary routines to display the Options3
**	property page (the check options of DP).
**
**  History:
**	10-jul-1999 (somsa01)
**	    Created.
*/

#include "stdafx.h"
#include "dp.h"
#include "Options3Page.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
** COptions3Page property page
*/
IMPLEMENT_DYNCREATE(COptions3Page, CPropertyPage)

COptions3Page::COptions3Page() : CPropertyPage(COptions3Page::IDD)
{
    //{{AFX_DATA_INIT(COptions3Page)
	    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

COptions3Page::~COptions3Page()
{
}

COptions3Page::COptions3Page(CPropertySheet *ps) : CPropertyPage(COptions3Page::IDD)
{
    m_propertysheet = ps;
    m_psp.dwFlags |= (PSP_HASHELP);
}

void COptions3Page::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(COptions3Page)
    DDX_Control(pDX, IDC_IMAGE, m_image);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(COptions3Page, CPropertyPage)
    //{{AFX_MSG_MAP(COptions3Page)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** COptions3Page message handlers
*/
BOOL
COptions3Page::OnSetActive() 
{
    CButton *pCheckCK7 = (CButton *)GetDlgItem(IDC_CHECK_CK7);
    CButton *pCheckCK8 = (CButton *)GetDlgItem(IDC_CHECK_CK8);

    m_propertysheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

    if (!FileInput)
    {
	pCheckCK7->EnableWindow(TRUE);
	pCheckCK8->EnableWindow(TRUE);
    }
    else
    {
	pCheckCK7->SetCheck(FALSE);
	pCheckCK8->SetCheck(FALSE);
	pCheckCK7->EnableWindow(FALSE);
	pCheckCK8->EnableWindow(FALSE);
    }

    return CPropertyPage::OnSetActive();
}

LRESULT
COptions3Page::OnWizardNext() 
{
    CButton *pCheckCK0 = (CButton *)GetDlgItem(IDC_CHECK_CK0);
    CButton *pCheckCK1 = (CButton *)GetDlgItem(IDC_CHECK_CK1);
    CButton *pCheckCK2 = (CButton *)GetDlgItem(IDC_CHECK_CK2);
    CButton *pCheckCK3 = (CButton *)GetDlgItem(IDC_CHECK_CK3);
    CButton *pCheckCK4 = (CButton *)GetDlgItem(IDC_CHECK_CK4);
    CButton *pCheckCK5 = (CButton *)GetDlgItem(IDC_CHECK_CK5);
    CButton *pCheckCK6 = (CButton *)GetDlgItem(IDC_CHECK_CK6);
    CButton *pCheckCK7 = (CButton *)GetDlgItem(IDC_CHECK_CK7);
    CButton *pCheckCK8 = (CButton *)GetDlgItem(IDC_CHECK_CK8);
    CButton *pCheckHC = (CButton *)GetDlgItem(IDC_CHECK_HC);

    Options3 = "";
    if (pCheckCK0->GetCheck())
	Options3 += CString("-ck0 ");
    if (pCheckCK1->GetCheck())
	Options3 += CString("-ck1 ");
    if (pCheckCK2->GetCheck())
	Options3 += CString("-ck2 ");
    if (pCheckCK3->GetCheck())
	Options3 += CString("-ck3 ");
    if (pCheckCK4->GetCheck())
	Options3 += CString("-ck4 ");
    if (pCheckCK5->GetCheck())
	Options3 += CString("-ck5 ");
    if (pCheckCK6->GetCheck())
	Options3 += CString("-ck6 ");
    if (pCheckCK7->GetCheck())
	Options3 += CString("-ck7 ");
    if (pCheckCK8->GetCheck())
	Options3 += CString("-ck8 ");
    if (pCheckHC->GetCheck())
	Options3 += CString("-hc ");
	
    return CPropertyPage::OnWizardNext();
}

BOOL COptions3Page::OnInitDialog() 
{
    CPropertyPage::OnInitDialog();
    
    Message.LoadString(IDS_OPTIONS3_HEADING);
    SetDlgItemText(IDC_OPTIONS3_HEADING, Message);
    Message.LoadString(IDS_CHECK_CK0);
    SetDlgItemText(IDC_CHECK_CK0, Message);
    Message.LoadString(IDS_CHECK_CK1);
    SetDlgItemText(IDC_CHECK_CK1, Message);
    Message.LoadString(IDS_CHECK_CK2);
    SetDlgItemText(IDC_CHECK_CK2, Message);
    Message.LoadString(IDS_CHECK_CK3);
    SetDlgItemText(IDC_CHECK_CK3, Message);
    Message.LoadString(IDS_CHECK_CK4);
    SetDlgItemText(IDC_CHECK_CK4, Message);
    Message.LoadString(IDS_CHECK_CK5);
    SetDlgItemText(IDC_CHECK_CK5, Message);
    Message.LoadString(IDS_CHECK_CK6);
    SetDlgItemText(IDC_CHECK_CK6, Message);
    Message.LoadString(IDS_CHECK_CK7);
    SetDlgItemText(IDC_CHECK_CK7, Message);
    Message.LoadString(IDS_CHECK_CK8);
    SetDlgItemText(IDC_CHECK_CK8, Message);
    Message.LoadString(IDS_CHECK_HC);
    SetDlgItemText(IDC_CHECK_HC, Message);
    
    return TRUE;
}
