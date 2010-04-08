/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : ijactppg.cpp , Implementation File
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Implementation of the CIjaCtrlPropPage property page class 

** History:
**
** 22-Dec-1999 (uk$so01)
**    Created
**
**/

#include "stdafx.h"
#include "ijactrl.h"
#include "ijactppg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CIjaCtrlPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CIjaCtrlPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CIjaCtrlPropPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CIjaCtrlPropPage, "IJACTRL.IjaCtrlPropPage.1",
	0xc92b8428, 0xb176, 0x11d3, 0xa3, 0x22, 0, 0xc0, 0x4f, 0x1f, 0x75, 0x4a)


/////////////////////////////////////////////////////////////////////////////
// CIjaCtrlPropPage::CIjaCtrlPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CIjaCtrlPropPage

BOOL CIjaCtrlPropPage::CIjaCtrlPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_IJACTRL_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CIjaCtrlPropPage::CIjaCtrlPropPage - Constructor

CIjaCtrlPropPage::CIjaCtrlPropPage() :
	COlePropertyPage(IDD, IDS_IJACTRL_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CIjaCtrlPropPage)
	m_bPaintForegroundTransaction = TRUE;
	//}}AFX_DATA_INIT
	m_bPaintForegroundTransaction = theApp.m_property.m_bPaintForegroundTransaction;
}



/////////////////////////////////////////////////////////////////////////////
// CIjaCtrlPropPage::DoDataExchange - Moves data between page and properties

void CIjaCtrlPropPage::DoDataExchange(CDataExchange* pDX)
{
	DDP_Check(pDX, IDC_CHECK1, m_bPaintForegroundTransaction, _T("PaintForegroundTransaction") );
	//{{AFX_DATA_MAP(CIjaCtrlPropPage)
	DDX_Check(pDX, IDC_CHECK1, m_bPaintForegroundTransaction);
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CIjaCtrlPropPage message handlers


BOOL CIjaCtrlPropPage::OnInitDialog() 
{
	COlePropertyPage::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
