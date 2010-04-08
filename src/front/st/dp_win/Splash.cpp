/*
**
**  Name: Splash.cpp
**
**  Description:
**	This file contains the routines to implement the splash screen dialog.
**
**  History:
**	??-???-???? (mcgem01)
**	    Created.
*/

#include "stdafx.h"
#include "dp.h"
#include "Splash.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define SPLASH_DURATION (3000)

static void
PreloadBitmaps()
{
    static int arr[]={101,102};

    for (int i=0; i<sizeof(arr)/sizeof(arr[0]); i++)
    {
	HRSRC hrsrc=FindResource(AfxGetInstanceHandle(),MAKEINTRESOURCE(arr[i]),RT_BITMAP);

	if (hrsrc)
	{
	    HGLOBAL h=LoadResource(AfxGetInstanceHandle(),hrsrc);

	    if (h)
		LockResource(h);
	}
    }
}

/*
** CSplash dialog
*/
CSplash::CSplash(CWnd* pParent)
: CDialog(CSplash::IDD, pParent)
{
    //{{AFX_DATA_INIT(CSplash)
    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

void CSplash::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CSplash)
    DDX_Control(pDX, IDC_IMAGE, m_image);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSplash, CDialog)
    //{{AFX_MSG_MAP(CSplash)
    ON_WM_TIMER()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** CSplash message handlers
*/
BOOL
CSplash::OnInitDialog() 
{
    CPoint pt;

    CDialog::OnInitDialog();
	
    m_image.GetBitmapDimension(pt);
    SetWindowPos(&CWnd::wndTop,0,0,pt.x,pt.y,SWP_NOMOVE|SWP_NOZORDER);
    m_image.SetWindowPos(&CWnd::wndTop,0,0,pt.x,pt.y,SWP_NOZORDER);
	
    CenterWindow();
    UpdateWindow();
    PreloadBitmaps();
    SetTimer(1,SPLASH_DURATION,0);
	
    return TRUE;
}

void
CSplash::OnTimer(UINT nIDEvent) 
{
    KillTimer(1);
    EndDialog(TRUE);
}
