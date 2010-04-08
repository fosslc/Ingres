/*
**
**  Name: PropSheet.cpp
**
**  Description:
**	This file contains the necessary routines for the master property
**	sheet.
**
**  History:
**	10-jul-1999 (somsa01)
**	    Created.
*/

#include "stdafx.h"
#include "dp.h"
#include "PropSheet.h"
#include "AuthPage.h"
#include "LocPage.h"
#include "OptionsPage.h"
#include "Options2Page.h"
#include "Options3Page.h"
#include "Options4Page.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
** PropSheet
*/
IMPLEMENT_DYNAMIC(PropSheet, CPropertySheet)

PropSheet::PropSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
}

PropSheet::PropSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
    AddPage(new AuthPage(this));
    AddPage(new LocPage(this));
    AddPage(new COptionsPage(this));
    AddPage(new COptions2Page(this));
    AddPage(new COptions3Page(this));
    AddPage(new COptions4Page(this));
    SetWizardMode();
}

PropSheet::~PropSheet()
{
}


BEGIN_MESSAGE_MAP(PropSheet, CPropertySheet)
    //{{AFX_MSG_MAP(PropSheet)
	    // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** PropSheet message handlers
*/
