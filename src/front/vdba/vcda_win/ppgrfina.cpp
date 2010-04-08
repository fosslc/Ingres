/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ppgrfina.cpp : implementation file
**    Project  : INGRES II/ Visual Configuration Diff Control (vcda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Restore: Final step of restore operation
**
** History:
**
** 18-Mar-2003 (uk$so01)
**    Created
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
**    Created
** 06-Jan-2004 (schph01)
**    SIR #111462, Put string into resource files
** 21-Jan-2004 (uk$so01)
**    SIR #109221, Before adding items, clean the content of 'm_cListBox' first.
** 08-Sep-2008 (drivi01)
**    Updated wizard to use Wizard97 style and updated smaller
**    wizard graphic.
*/

#include "stdafx.h"
#include "vcda.h"
#include "pshresto.h"
#include "ppgrfina.h"
#include "rctools.h"  // Resource symbols of rctools.rc

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define LAYOUT_NUMBER 3


IMPLEMENT_DYNCREATE(CuPropertyPageRestoreFinal, CPropertyPage)

CuPropertyPageRestoreFinal::CuPropertyPageRestoreFinal() : CPropertyPage(CuPropertyPageRestoreFinal::IDD)
{
	//{{AFX_DATA_INIT(CuPropertyPageRestoreFinal)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bitmap.SetCenterVertical(TRUE);
	m_strSnapshot.LoadString (IDS_TAB_SNAPSHOT1);
	m_psp.dwFlags |= PSP_DEFAULT|PSP_HIDEHEADER;
}

CuPropertyPageRestoreFinal::~CuPropertyPageRestoreFinal()
{
}

void CuPropertyPageRestoreFinal::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuPropertyPageRestoreFinal)
	DDX_Control(pDX, IDC_LIST1, m_cListBox);
	DDX_Control(pDX, IDC_STATIC_CHECKMARK2, m_cStaticCheckMark2);
	DDX_Control(pDX, IDC_STATIC_CHECKMARK1, m_cStaticCheckMark1);
	DDX_Control(pDX, IDC_STATIC_IMAGE, m_bitmap);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuPropertyPageRestoreFinal, CPropertyPage)
	//{{AFX_MSG_MAP(CuPropertyPageRestoreFinal)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuPropertyPageRestoreFinal message handlers

BOOL CuPropertyPageRestoreFinal::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST2, this));

	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	LSCTRLHEADERPARAMS2 lsp[LAYOUT_NUMBER] =
	{
		{IDS_TAB_TYPE,      120,  LVCFMT_LEFT,  FALSE},
		{IDS_TAB_SNAPSHOT1, 300,  LVCFMT_LEFT,  FALSE},
		{IDS_TAB_RESTORE_AS,300,  LVCFMT_LEFT,  FALSE}
	};
	InitializeHeader2(&m_cListCtrl, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp, LAYOUT_NUMBER);
	LVCOLUMN vc;
	memset (&vc, 0, sizeof (vc));
	vc.mask = LVCF_TEXT;
	vc.pszText = (LPTSTR)(LPCTSTR)m_strSnapshot;
	m_cListCtrl.SetColumn(1, &vc);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CuPropertyPageRestoreFinal::OnSetActive() 
{
	CxPSheetRestore* pParent = (CxPSheetRestore*)GetParent();
	CaRestoreParam& Restore = pParent->GetData();

	UINT nMode = PSWIZB_FINISH | PSWIZB_BACK;
	pParent->SetWizardButtons(nMode);
	CTypedPtrList< CObList, CaCdaCouple* >& l = Restore.m_listSubStituteInfo;
	CTypedPtrList< CObList, CaCdaDifference* >* ld = Restore.m_plistDifference;
	POSITION pos = ld->GetHeadPosition();

	m_cListBox.ResetContent();
	while (pos != NULL)
	{
		CaCdaDifference* pObj = ld->GetNext(pos);
		if (pObj->GetType() != CDA_GENERAL)
			break;

		if (pObj->GetName().CompareNoCase(_T("HOST NAME")) == 0 ||
		    pObj->GetName().CompareNoCase(_T("II_SYSTEM")) == 0 ||
		    pObj->GetName().CompareNoCase(_T("II_INSTALLATION")) == 0)
		{
			m_cListBox.AddString(pObj->GetName());
		}
	}

	CString strAffect = _T(": ");
	CString strMainLabel = _T("???");
	int iIndex = -1;
	m_cListCtrl.DeleteAllItems();
	pos = l.GetHeadPosition();
	while (pos != NULL)
	{
		CaCdaCouple* pObj = l.GetNext(pos);
		switch (pObj->m_p1->GetType())
		{
		case CDA_CONFIG:
			strMainLabel.LoadString(IDS_PARAM_CONFIGURATION);// = _T("Configuration");
			strAffect = _T(": "); 
			break;
		case CDA_ENVSYSTEM:
			strMainLabel.LoadString(IDS_PARAM_SYSTEM );//= _T("System Variable");
			strAffect = _T("= "); 
			break;
		case CDA_ENVUSER:
			strMainLabel.LoadString(IDS_PARAM_ENVUSER );//= _T("User Variable");
			strAffect = _T("= "); 
			break;
		case CDA_VNODE:
			strMainLabel.LoadString(IDS_PARAM_VNODE );//= _T("Virtual Node");
			strAffect = _T(": "); 
			break;
		default:
			break;
		}

		iIndex = m_cListCtrl.InsertItem(m_cListCtrl.GetItemCount(), strMainLabel, -1);
		if (iIndex != -1)
		{
			m_cListCtrl.SetItemText(iIndex, 1, pObj->m_p1->GetName() + strAffect + pObj->m_p1->GetValue());
			m_cListCtrl.SetItemText(iIndex, 2, pObj->m_p2->GetName() + strAffect + pObj->m_p2->GetValue());
		}
	}


	return CPropertyPage::OnSetActive();
}

BOOL CuPropertyPageRestoreFinal::OnWizardFinish() 
{
	CWaitCursor doWaitCursor;
	CxPSheetRestore* pParent = (CxPSheetRestore*)GetParent();
	CaRestoreParam& Restore = pParent->GetData();
	UpdateData (TRUE);

	BOOL bOK = VCDA_RestoreParam (&Restore);
	if (!bOK)
		return FALSE;

	return CPropertyPage::OnWizardFinish();
}
