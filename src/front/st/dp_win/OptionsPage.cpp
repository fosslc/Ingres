/*
**
**  Name: OptionsPage.cpp
**
**  Description:
**	This file contains the necessary routines to display the Options
**	property page (the table/index file options of DP).
**
**  History:
**	10-jul-1999 (somsa01)
**	    Created.
*/

#include "stdafx.h"
#include "dp.h"
#include "OptionsPage.h"

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
    //}}AFX_DATA_INIT
}

COptionsPage::~COptionsPage()
{
}

COptionsPage::COptionsPage(CPropertySheet *ps) : CPropertyPage(COptionsPage::IDD)
{
    m_propertysheet = ps;
    m_psp.dwFlags |= (PSP_HASHELP);
}

void COptionsPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(COptionsPage)
    DDX_Control(pDX, IDC_COMBO_II, m_II);
    DDX_Control(pDX, IDC_COMBO_PAGESIZE, m_Pagesize);
    DDX_Control(pDX, IDC_COMBO_VERSION, m_Version);
    DDX_Control(pDX, IDC_IMAGE, m_image);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(COptionsPage, CPropertyPage)
    //{{AFX_MSG_MAP(COptionsPage)
    ON_BN_CLICKED(IDC_CHECK_PB, OnCheckPb)
    ON_BN_CLICKED(IDC_CHECK_PE, OnCheckPe)
    ON_EN_KILLFOCUS(IDC_EDIT_MODPAGES, OnKillfocusEditModpages)
    ON_EN_KILLFOCUS(IDC_EDIT_NUMPAGES, OnKillfocusEditNumpages)
    ON_EN_KILLFOCUS(IDC_EDIT_NUMBYTES, OnKillfocusEditNumbytes)
    ON_BN_CLICKED(IDC_CHECK_RE, OnCheckRe)
    ON_BN_CLICKED(IDC_CHECK_PN, OnCheckPn)
    ON_BN_CLICKED(IDC_CHECK_II, OnCheckIi)
    ON_BN_CLICKED(IDC_CHECK_TS, OnCheckTs)
    ON_BN_CLICKED(IDC_CHECK_RW, OnCheckRw)
    ON_BN_CLICKED(IDC_CHECK_LB, OnCheckLb)
    ON_BN_CLICKED(IDC_CHECK_PS, OnCheckPs)
    ON_BN_CLICKED(IDC_CHECK_TP, OnCheckTp)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** COptionsPage message handlers
*/
BOOL
COptionsPage::OnSetActive() 
{
    m_propertysheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);	
    return CPropertyPage::OnSetActive();
}

BOOL
COptionsPage::OnInitDialog() 
{
    CComboBox *pVersion = (CComboBox *)GetDlgItem(IDC_COMBO_VERSION);
    CComboBox *pII = (CComboBox *)GetDlgItem(IDC_COMBO_II);
    CComboBox *pPagesize = (CComboBox *)GetDlgItem(IDC_COMBO_PAGESIZE);
    CEdit *pModpages = (CEdit *)GetDlgItem(IDC_EDIT_MODPAGES);
    CSpinButtonCtrl *pSpin = (CSpinButtonCtrl *)GetDlgItem(IDC_SPIN_MODPAGES);
    CEdit *pNumpages = (CEdit *)GetDlgItem(IDC_EDIT_NUMPAGES);
    CEdit *pNumbytes = (CEdit *)GetDlgItem(IDC_EDIT_NUMBYTES);
    CEdit *pLogsize = (CEdit *)GetDlgItem(IDC_EDIT_LOGSIZE);
    CEdit *pStart = (CEdit *)GetDlgItem(IDC_EDIT_START);
    CEdit *pEnd = (CEdit *)GetDlgItem(IDC_EDIT_END);
    CEdit *pTracept = (CEdit *)GetDlgItem(IDC_EDIT_TRACEPT);

    Message.LoadString(IDS_OPTIONS_HEADING);
    SetDlgItemText(IDC_OPTIONS_HEADING, Message);
    Message.LoadString(IDS_CHECK_RE);
    SetDlgItemText(IDC_CHECK_RE, Message);
    Message.LoadString(IDS_CHECK_PN);
    SetDlgItemText(IDC_CHECK_PN, Message);
    Message.LoadString(IDS_CHECK_II);
    SetDlgItemText(IDC_CHECK_II, Message);
    Message.LoadString(IDS_CHECK_TS);
    SetDlgItemText(IDC_CHECK_TS, Message);
    Message.LoadString(IDS_CHECK_RW);
    SetDlgItemText(IDC_CHECK_RW, Message);
    Message.LoadString(IDS_CHECK_LB);
    SetDlgItemText(IDC_CHECK_LB, Message);
    Message.LoadString(IDS_CHECK_PB);
    SetDlgItemText(IDC_CHECK_PB, Message);
    Message.LoadString(IDS_CHECK_PE);
    SetDlgItemText(IDC_CHECK_PE, Message);
    Message.LoadString(IDS_CHECK_PS);
    SetDlgItemText(IDC_CHECK_PS, Message);
    Message.LoadString(IDS_CHECK_TP);
    SetDlgItemText(IDC_CHECK_TP, Message);

    CPropertyPage::OnInitDialog();

    pVersion->EnableWindow(FALSE);
    pII->EnableWindow(FALSE);
    pPagesize->EnableWindow(FALSE);
    pModpages->EnableWindow(FALSE);
    pSpin->EnableWindow(FALSE);
    pNumpages->EnableWindow(FALSE);
    pNumbytes->EnableWindow(FALSE);
    pLogsize->EnableWindow(FALSE);
    pStart->EnableWindow(FALSE);
    pEnd->EnableWindow(FALSE);
    pTracept->EnableWindow(FALSE);

    pSpin->SetRange(0, UD_MAXVAL);
    SetDlgItemText(IDC_EDIT_NUMPAGES, "1000");
    SetDlgItemText(IDC_EDIT_NUMBYTES, "1");
    SetDlgItemText(IDC_EDIT_START, "0");

    m_Version.AddString("6.4");
    m_Version.AddString("1.1");
    m_Version.AddString("1.2");
    m_Version.AddString("2.0");
    m_Version.AddString("2.5");
    m_Version.SetCurSel(4);

    m_Pagesize.AddString("2K");
    m_Pagesize.AddString("4K");
    m_Pagesize.AddString("8K");
    m_Pagesize.AddString("16K");
    m_Pagesize.AddString("32K");
    m_Pagesize.AddString("64K");
    m_Pagesize.SetCurSel(0);

    m_II.AddString("iirelation");
    m_II.AddString("iiattribute");
    m_II.SetCurSel(0);

    PBFLAG = PEFLAG = FALSE;

    return TRUE;
}

LRESULT
COptionsPage::OnWizardNext() 
{
    CButton *pCheckRE = (CButton *)GetDlgItem(IDC_CHECK_RE);
    CButton *pCheckPN = (CButton *)GetDlgItem(IDC_CHECK_PN);
    CButton *pCheckII = (CButton *)GetDlgItem(IDC_CHECK_II);
    CButton *pCheckTS = (CButton *)GetDlgItem(IDC_CHECK_TS);
    CButton *pCheckRW = (CButton *)GetDlgItem(IDC_CHECK_RW);
    CButton *pCheckLB = (CButton *)GetDlgItem(IDC_CHECK_LB);
    CButton *pCheckPB = (CButton *)GetDlgItem(IDC_CHECK_PB);
    CButton *pCheckPE = (CButton *)GetDlgItem(IDC_CHECK_PE);
    CButton *pCheckPS = (CButton *)GetDlgItem(IDC_CHECK_PS);
    CButton *pCheckTP = (CButton *)GetDlgItem(IDC_CHECK_TP);
    CString value;

    if (pCheckRE->GetCheck())
	m_Version.GetLBText(m_Version.GetCurSel(), Version);	
    else
	Version = "";
    
    Options1 = "";
    if (pCheckRE->GetCheck())
    {
	Options1 += CString("-re");
	if (Version == "6.4")
	    Options1 += CString("64 ");
	else if (Version == "1.1")
	    Options1 += CString("11 ");
	else if (Version == "1.2")
	    Options1 += CString("12 ");
	else if (Version == "2.0")
	    Options1 += CString("20 ");
	else if (Version == "2.5")
	    Options1 += CString("25 ");
    }
    if (pCheckPN->GetCheck())
    {
	Options1 += CString("-pn");
	GetDlgItemText(IDC_EDIT_MODPAGES, value);
	Options1 += value + CString(" ");
    }
    if (pCheckII->GetCheck())
    {
	m_II.GetLBText(m_II.GetCurSel(), value);
	Options1 += CString("-") + value;
    }
    if (pCheckTS->GetCheck())
    {
	GetDlgItemText(IDC_EDIT_NUMPAGES, value);
	if (atoi(value) < 1000)
	{
	    Message.LoadString(IDS_BAD_NUMPAGES);
	    Message2.LoadString(IDS_TITLE);
	    MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	    return -1;
	}

	Options1 += CString("-ts");
	Options1 += value + CString(" ");
    }
    if (pCheckRW->GetCheck())
    {
	Options1 += CString("-rw");
	GetDlgItemText(IDC_EDIT_NUMBYTES, value);
	Options1 += value + CString(" ");
    }
    if (pCheckLB->GetCheck())
    {
	Options1 += CString("-lb");
	GetDlgItemText(IDC_EDIT_LOGSIZE, value);
	Options1 += value + CString(" ");
    }
    if (pCheckPB->GetCheck())
    {
	Options1 += CString("-pb");
	GetDlgItemText(IDC_EDIT_START, value);
	Options1 += value + CString(" ");
    }
    if (pCheckPE->GetCheck())
    {
	Options1 += CString("-pe");
	GetDlgItemText(IDC_EDIT_END, value);
	Options1 += value + CString(" ");
    }
    if (pCheckPS->GetCheck())
    {
	Options1 += CString("-ps");
	m_Pagesize.GetLBText(m_Pagesize.GetCurSel(), value);
	if (value == "2K")
	    Options1 += CString("2 ");
	else if (value == "4K")
	    Options1 += CString("4 ");
	else if (value == "8K")
	    Options1 += CString("8 ");
	else if (value == "16K")
	    Options1 += CString("16 ");
	else if (value == "32K")
	    Options1 += CString("32 ");
	else if (value == "64K")
	    Options1 += CString("64 ");
    }
    if (pCheckTP->GetCheck())
    {
	char	trvalue[256], trvalue2[256], *ptr, *ptr2;

	GetDlgItemText(IDC_EDIT_TRACEPT, trvalue, sizeof(trvalue));
	ptr = trvalue;
	while (ptr)
	{
	    strcpy(trvalue2, ptr);
	    ptr = strchr(ptr, ',');
	    ptr2 = strchr(trvalue2, ',');
	    if (ptr2)
	    {
		ptr++;
		*ptr2 = '\0';
	    }
	    Options1 += CString("-tp") + CString(trvalue2) + CString(" ");
	}
    }

    return CPropertyPage::OnWizardNext();
}

void
COptionsPage::OnCheckPb() 
{
    CButton *pCheckPB = (CButton *)GetDlgItem(IDC_CHECK_PB);
    CEdit *pStart = (CEdit *)GetDlgItem(IDC_EDIT_START);

    PBFLAG = pCheckPB->GetCheck();
    pStart->EnableWindow(PBFLAG);
}

void
COptionsPage::OnCheckPe() 
{
    CButton *pCheckPE = (CButton *)GetDlgItem(IDC_CHECK_PE);
    CEdit *pEnd = (CEdit *)GetDlgItem(IDC_EDIT_END);

    PEFLAG = pCheckPE->GetCheck();
    pEnd->EnableWindow(PEFLAG);
}

void
COptionsPage::OnKillfocusEditModpages() 
{
    CString value;

    GetDlgItemText(IDC_EDIT_MODPAGES, value);
    if (value == "")
	SetDlgItemText(IDC_EDIT_MODPAGES, "0");
}

void
COptionsPage::OnKillfocusEditNumpages() 
{
    CString value;

    GetDlgItemText(IDC_EDIT_NUMPAGES, value);
    if (value == "")
	SetDlgItemText(IDC_EDIT_NUMPAGES, "1000");
}

void
COptionsPage::OnKillfocusEditNumbytes() 
{
    CString value;

    GetDlgItemText(IDC_EDIT_NUMBYTES, value);
    if (value == "")
	SetDlgItemText(IDC_EDIT_NUMBYTES, "1");	
}

void
COptionsPage::OnCheckRe() 
{
    CButton *pCheckRE = (CButton *)GetDlgItem(IDC_CHECK_RE);
    CComboBox *pVersion = (CComboBox *)GetDlgItem(IDC_COMBO_VERSION);

    pVersion->EnableWindow(pCheckRE->GetCheck());
}

void
COptionsPage::OnCheckPn() 
{
    CButton *pCheckPN = (CButton *)GetDlgItem(IDC_CHECK_PN);
    CButton *pSpin = (CButton *)GetDlgItem(IDC_SPIN_MODPAGES);
    CEdit *pModpages = (CEdit *)GetDlgItem(IDC_EDIT_MODPAGES);

    pModpages->EnableWindow(pCheckPN->GetCheck());
    pSpin->EnableWindow(pCheckPN->GetCheck());
}

void
COptionsPage::OnCheckIi() 
{
    CButton *pCheckII = (CButton *)GetDlgItem(IDC_CHECK_II);
    CComboBox *pII = (CComboBox *)GetDlgItem(IDC_COMBO_II);

    pII->EnableWindow(pCheckII->GetCheck());
}

void
COptionsPage::OnCheckTs() 
{
    CButton *pCheckTS = (CButton *)GetDlgItem(IDC_CHECK_TS);
    CEdit *pNumpages = (CEdit *)GetDlgItem(IDC_EDIT_NUMPAGES);

    pNumpages->EnableWindow(pCheckTS->GetCheck());
}

void
COptionsPage::OnCheckRw() 
{
    CButton *pCheckRW = (CButton *)GetDlgItem(IDC_CHECK_RW);
    CEdit *pNumbytes = (CEdit *)GetDlgItem(IDC_EDIT_NUMBYTES);

    pNumbytes->EnableWindow(pCheckRW->GetCheck());
}

void
COptionsPage::OnCheckLb() 
{
    CButton *pCheckLB = (CButton *)GetDlgItem(IDC_CHECK_LB);
    CEdit *pLogsize = (CEdit *)GetDlgItem(IDC_EDIT_LOGSIZE);

    pLogsize->EnableWindow(pCheckLB->GetCheck());
}

void
COptionsPage::OnCheckPs() 
{
    CButton *pCheckPS = (CButton *)GetDlgItem(IDC_CHECK_PS);
    CComboBox *pPagesize = (CComboBox *)GetDlgItem(IDC_COMBO_PAGESIZE);

    pPagesize->EnableWindow(pCheckPS->GetCheck());
}

void
COptionsPage::OnCheckTp() 
{
    CButton *pCheckTP = (CButton *)GetDlgItem(IDC_CHECK_TP);
    CEdit *pTracept = (CEdit *)GetDlgItem(IDC_EDIT_TRACEPT);

    pTracept->EnableWindow(pCheckTP->GetCheck());
}
