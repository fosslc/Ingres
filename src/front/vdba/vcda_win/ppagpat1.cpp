/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ppagpat1.cpp : implementation file
**    Project  : INGRES II/ Visual Configuration Diff Control (vcda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Restore: Preliminary Step - Uninstall patches
**
** History:
**
** 04-Oct-2002 (uk$so01)
**    Created
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
** 08-Sep-2008 (drivi01)
**    Updated wizard to use Wizard97 style and updated smaller
**    wizard graphic.
*/


#include "stdafx.h"
#include "vcda.h"
#include "pshresto.h"
#include "rctools.h"  // Resource symbols of rctools.rc
#include "ppagpat1.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CuPropertyPageRestorePatch1, CPropertyPage)


/////////////////////////////////////////////////////////////////////////////
// CuPropertyPageRestorePatch1 property page

CuPropertyPageRestorePatch1::CuPropertyPageRestorePatch1() : CPropertyPage(CuPropertyPageRestorePatch1::IDD)
{
	//{{AFX_DATA_INIT(CuPropertyPageRestorePatch1)
	m_strMsg = _T("");
	//}}AFX_DATA_INIT
	m_bitmap.SetCenterVertical(TRUE);
	m_psp.dwFlags |= PSP_DEFAULT|PSP_HIDEHEADER;
}

CuPropertyPageRestorePatch1::~CuPropertyPageRestorePatch1()
{
}

void CuPropertyPageRestorePatch1::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuPropertyPageRestorePatch1)
	DDX_Control(pDX, IDC_STATIC_IMAGE2, m_cIconWarning);
	DDX_Control(pDX, IDC_STATIC_IMAGE, m_bitmap);
	DDX_Text(pDX, IDC_STATIC1, m_strMsg);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuPropertyPageRestorePatch1, CPropertyPage)
	//{{AFX_MSG_MAP(CuPropertyPageRestorePatch1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



BOOL CuPropertyPageRestorePatch1::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	HICON hIcon = theApp.LoadStandardIcon(IDI_EXCLAMATION);
	m_cIconWarning.SetIcon(hIcon);
	m_cIconWarning.Invalidate();
	DestroyIcon(hIcon);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CuPropertyPageRestorePatch1::OnSetActive() 
{
	CxPSheetRestore* pParent = (CxPSheetRestore*)GetParent();
	UINT nMode = PSWIZB_NEXT;

	pParent->SetWizardButtons(nMode);
	return CPropertyPage::OnSetActive();
}
