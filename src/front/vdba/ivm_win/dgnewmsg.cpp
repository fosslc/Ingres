/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : dgnewmsg.cpp , Implementation File                                    //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II / Visual Manager                                            //
//    Author   : Sotheavut UK (uk$so01)                                                //
//                                                                                     //
//                                                                                     //
//    Purpose  : Modeless Dialog to Inform user of new message arriving                //
****************************************************************************************/
/* History:
* --------
* uk$so01: (06-May-1999 created)
*
*
*/

#include "stdafx.h"
#include "ivm.h"
#include "dgnewmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuDlgYouHaveNewMessage dialog


CuDlgYouHaveNewMessage::CuDlgYouHaveNewMessage(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgYouHaveNewMessage::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgYouHaveNewMessage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgYouHaveNewMessage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgYouHaveNewMessage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgYouHaveNewMessage, CDialog)
	//{{AFX_MSG_MAP(CuDlgYouHaveNewMessage)
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgYouHaveNewMessage message handlers

BOOL CuDlgYouHaveNewMessage::OnInitDialog() 
{
	CDialog::OnInitDialog();
	theApp.m_bMessageBoxEvent = TRUE;
	SetWindowPos (&wndTopMost, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_SHOWWINDOW);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CuDlgYouHaveNewMessage::OnOK() 
{
	theApp.m_bMessageBoxEvent = FALSE;
	CDialog::OnOK();
}

void CuDlgYouHaveNewMessage::OnCancel() 
{
	theApp.m_bMessageBoxEvent = FALSE;
	CDialog::OnCancel();
}

void CuDlgYouHaveNewMessage::OnClose() 
{
	theApp.m_bMessageBoxEvent = FALSE;
	CDialog::OnClose();
}

void CuDlgYouHaveNewMessage::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}
