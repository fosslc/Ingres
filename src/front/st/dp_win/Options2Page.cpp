/*
**
**  Name: Options2Page.cpp
**
**  Description:
**	This file contains the necessary routines to display the Options2
**	property page (printout options of DP).
**
**  History:
**	10-jul-1999 (somsa01)
**	    Created.
*/

#include "stdafx.h"
#include "dp.h"
#include "Options2Page.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
** COptions2Page property page
*/
IMPLEMENT_DYNCREATE(COptions2Page, CPropertyPage)

COptions2Page::COptions2Page() : CPropertyPage(COptions2Page::IDD)
{
    //{{AFX_DATA_INIT(COptions2Page)
	    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

COptions2Page::~COptions2Page()
{
}

COptions2Page::COptions2Page(CPropertySheet *ps) : CPropertyPage(COptions2Page::IDD)
{
    m_propertysheet = ps;
    m_psp.dwFlags |= (PSP_HASHELP);
}

void COptions2Page::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(COptions2Page)
    DDX_Control(pDX, IDC_IMAGE, m_image);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(COptions2Page, CPropertyPage)
    //{{AFX_MSG_MAP(COptions2Page)
    ON_BN_CLICKED(IDC_CHECK_FF, OnCheckFf)
    ON_BN_CLICKED(IDC_CHECK_OF, OnCheckOf)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** COptions2Page message handlers
*/
void
COptions2Page::OnCheckFf() 
{
    CButton *pCheckFF = (CButton *)GetDlgItem(IDC_CHECK_FF);

    FFFLAG = pCheckFF->GetCheck();
}

void
COptions2Page::OnCheckOf() 
{
    CButton *pCheckOF = (CButton *)GetDlgItem(IDC_CHECK_OF);

    OFFLAG = pCheckOF->GetCheck();	
}

BOOL
COptions2Page::OnInitDialog() 
{
    CPropertyPage::OnInitDialog();
	
    Message.LoadString(IDS_OPTIONS2_HEADING);
    SetDlgItemText(IDC_OPTIONS2_HEADING, Message);
    Message.LoadString(IDS_CHECK_BS);
    SetDlgItemText(IDC_CHECK_BS, Message);
    Message.LoadString(IDS_CHECK_FF);
    SetDlgItemText(IDC_CHECK_FF, Message);
    Message.LoadString(IDS_CHECK_OF);
    SetDlgItemText(IDC_CHECK_OF, Message);
    Message.LoadString(IDS_CHECK_MS);
    SetDlgItemText(IDC_CHECK_MH, Message);
    Message.LoadString(IDS_CHECK_BG79416RO);
    SetDlgItemText(IDC_CHECK_BG79416RO, Message);
    Message.LoadString(IDS_CHECK_BG79416RW);
    SetDlgItemText(IDC_CHECK_BG79416RW, Message);
    Message.LoadString(IDS_CHECK_FX);
    SetDlgItemText(IDC_CHECK_FX, Message);
    Message.LoadString(IDS_CHECK_OH);
    SetDlgItemText(IDC_CHECK_OH, Message);
    Message.LoadString(IDS_CHECK_OK);
    SetDlgItemText(IDC_CHECK_OK, Message);
    Message.LoadString(IDS_CHECK_OD);
    SetDlgItemText(IDC_CHECK_OD, Message);
    Message.LoadString(IDS_CHECK_OT);
    SetDlgItemText(IDC_CHECK_OT, Message);
    Message.LoadString(IDS_CHECK_OS);
    SetDlgItemText(IDC_CHECK_OS, Message);
    Message.LoadString(IDS_CHECK_OM);
    SetDlgItemText(IDC_CHECK_OM, Message);

    FFFLAG = OFFLAG = FALSE;	

    return TRUE;
}

BOOL
COptions2Page::OnSetActive() 
{
    CButton *pCheckFF = (CButton *)GetDlgItem(IDC_CHECK_FF);
    CButton *pCheckMH = (CButton *)GetDlgItem(IDC_CHECK_MH);
    CButton *pCheckOF = (CButton *)GetDlgItem(IDC_CHECK_OF);
    CButton *pCheckBGRO = (CButton *)GetDlgItem(IDC_CHECK_BG79416RO);
    CButton *pCheckBGRW = (CButton *)GetDlgItem(IDC_CHECK_BG79416RW);
    CButton *pCheckFX = (CButton *)GetDlgItem(IDC_CHECK_FX);

    m_propertysheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

    pCheckFF->EnableWindow(!FileInput);

    if ((Version == "6.4") && !FileInput)
    {
	pCheckMH->EnableWindow(TRUE);
	pCheckBGRO->EnableWindow(TRUE);
	pCheckBGRW->EnableWindow(TRUE);
    }
    else
    {
	pCheckMH->SetCheck(FALSE);
	pCheckMH->EnableWindow(FALSE);
	pCheckBGRO->EnableWindow(FALSE);
	pCheckBGRW->EnableWindow(FALSE);
    }

    if (!PBFLAG && !PEFLAG && !FileInput)
	pCheckOF->EnableWindow(TRUE);
    else
    {
	pCheckOF->SetCheck(FALSE);
	pCheckOF->EnableWindow(FALSE);
    }

    if (FileInput)
	pCheckFX->EnableWindow(TRUE);
    else
    {
	pCheckFX->SetCheck(FALSE);
	pCheckFX->EnableWindow(FALSE);
    }

    return CPropertyPage::OnSetActive();
}

LRESULT
COptions2Page::OnWizardNext() 
{
    CButton *pCheckBS = (CButton *)GetDlgItem(IDC_CHECK_BS);
    CButton *pCheckFF = (CButton *)GetDlgItem(IDC_CHECK_FF);
    CButton *pCheckOF = (CButton *)GetDlgItem(IDC_CHECK_OF);
    CButton *pCheckMH = (CButton *)GetDlgItem(IDC_CHECK_MH);
    CButton *pCheckBG79416RO = (CButton *)GetDlgItem(IDC_CHECK_BG79416RO);
    CButton *pCheckBG79416RW = (CButton *)GetDlgItem(IDC_CHECK_BG79416RW);
    CButton *pCheckFX = (CButton *)GetDlgItem(IDC_CHECK_FX);
    CButton *pCheckOH = (CButton *)GetDlgItem(IDC_CHECK_OH);
    CButton *pCheckOK = (CButton *)GetDlgItem(IDC_CHECK_OK);
    CButton *pCheckOD = (CButton *)GetDlgItem(IDC_CHECK_OD);
    CButton *pCheckOT = (CButton *)GetDlgItem(IDC_CHECK_OT);
    CButton *pCheckOS = (CButton *)GetDlgItem(IDC_CHECK_OS);
    CButton *pCheckOM = (CButton *)GetDlgItem(IDC_CHECK_OM);

    Options2 = "";
    if (pCheckBS->GetCheck())
	Options2 += CString("-bs ");
    if (pCheckFF->GetCheck())
	Options2 += CString("-ff ");
    if (pCheckOF->GetCheck())
	Options2 += CString("-of ");
    if (pCheckMH->GetCheck())
	Options2 += CString("-mh ");
    if (pCheckBG79416RO->GetCheck())
	Options2 += CString("-bg79416ro ");
    if (pCheckBG79416RW->GetCheck())
	Options2 += CString("-bg79416rw ");
    if (pCheckFX->GetCheck())
	Options2 += CString("-fx ");
    if (pCheckOH->GetCheck())
	Options2 += CString("-oh ");
    if (pCheckOK->GetCheck())
	Options2 += CString("-ok ");
    if (pCheckOD->GetCheck())
	Options2 += CString("-od ");
    if (pCheckOT->GetCheck())
	Options2 += CString("-ot ");
    if (pCheckOS->GetCheck())
	Options2 += CString("-os ");
    if (pCheckOM->GetCheck())
	Options2 += CString("-om ");

    return CPropertyPage::OnWizardNext();
}
