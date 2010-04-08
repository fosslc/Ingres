/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xfindmsg.cpp : implementation file
**    Project  : Ingres II/IVM
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Popup dialog for asking the input (message) for searching.
**
** History:
**
** 06-Mar-2003 (uk$so01)
**    Created
**    SIR #109773, Add property frame for displaying the description 
**    of ingres messages.
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "xfindmsg.h"
#include "evtcats.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CxDlgFindMessage::CxDlgFindMessage(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgFindMessage::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgFindMessage)
	m_strMessage = _T("");
	//}}AFX_DATA_INIT
}


void CxDlgFindMessage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgFindMessage)
	DDX_Text(pDX, IDC_EDIT1, m_strMessage);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgFindMessage, CDialog)
	//{{AFX_MSG_MAP(CxDlgFindMessage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgFindMessage message handlers

void CxDlgFindMessage::OnOK() 
{
	const int nCount = 512;
	TCHAR tchszText  [nCount];
	UpdateData(TRUE);
	MSGCLASSANDID msg = getMsgStringClassAndId((LPTSTR)(LPCTSTR)m_strMessage, tchszText, sizeof(tchszText));
	if (msg.lMsgClass == CLASS_OTHERS || msg.lMsgFullID == MSG_NOCODE)
	{
		AfxMessageBox (IDS_MSG_MESSAGE_UNIDENTIFIED);
		return;
	}
	CDialog::OnOK();
}
