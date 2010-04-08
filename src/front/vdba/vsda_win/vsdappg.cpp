/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vsdappg.cpp : Implementation of the CVsdaPropPage property page class
**    Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Property Page
**
** History:
**
** 26-Aug-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
*/

#include "stdafx.h"
#include "vsda.h"
#include "vsdappg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CVsdaPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CVsdaPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CVsdaPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CVsdaPropPage, "VSDA.VsdaPropPage.1",
	0xcc2da2b7, 0xb8f1, 0x11d6, 0x87, 0xd8, 0, 0xc0, 0x4f, 0x1f, 0x75, 0x4a)


/////////////////////////////////////////////////////////////////////////////
// CVsdaPropPage::CVsdaPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CVsdaPropPage

BOOL CVsdaPropPage::CVsdaPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_VSDA_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CVsdaPropPage::CVsdaPropPage - Constructor

CVsdaPropPage::CVsdaPropPage() :
	COlePropertyPage(IDD, IDS_VSDA_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CVsdaPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CVsdaPropPage::DoDataExchange - Moves data between page and properties

void CVsdaPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CVsdaPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CVsdaPropPage message handlers
