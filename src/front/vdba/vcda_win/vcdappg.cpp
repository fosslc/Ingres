/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vcdappg.cpp : Implementation of the CuVcdaPropPage property page class.
**    Project  : Visual Config Diff 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Property page
**
** History:
**
** 02-Oct-2002 (uk$so01)
**    Created
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
**/

#include "stdafx.h"
#include "vcda.h"
#include "vcdappg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuVcdaPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CuVcdaPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CuVcdaPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CuVcdaPropPage, "VCDA.VcdaPropPage.1",
	0xeaf345eb, 0xd514, 0x11d6, 0x87, 0xea, 0, 0xc0, 0x4f, 0x1f, 0x75, 0x4a)


/////////////////////////////////////////////////////////////////////////////
// CuVcdaPropPage::CuVcdaPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CuVcdaPropPage

BOOL CuVcdaPropPage::CuVcdaPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_VCDA_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CuVcdaPropPage::CuVcdaPropPage - Constructor

CuVcdaPropPage::CuVcdaPropPage() :
	COlePropertyPage(IDD, IDS_VCDA_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CuVcdaPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CuVcdaPropPage::DoDataExchange - Moves data between page and properties

void CuVcdaPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CuVcdaPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CuVcdaPropPage message handlers
