/*
**
**  Name: EditObject.cpp
**
**  Description:
**	This file contains the routines necessary for the edit object dialog box.
**
**  History:
**	06-sep-1999 (somsa01)
**	    Created.
*/

#include "stdafx.h"
#include "perfwiz.h"
#include "EditObject.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
** CEditObject dialog
*/
CEditObject::CEditObject(CWnd* pParent /*=NULL*/)
	: CDialog(CEditObject::IDD, pParent)
{
    //{{AFX_DATA_INIT(CEditObject)
	    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

void CEditObject::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CEditObject)
	    // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CEditObject, CDialog)
	//{{AFX_MSG_MAP(CEditObject)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** CEditObject message handlers
*/
void
CEditObject::OnOK() 
{
    CString Message, Message2;

    GetDlgItemText(IDC_EDIT_NAME, m_ObjectName);
    GetDlgItemText(IDC_EDIT_DESCRIPTION, m_ObjectDescription);
    if ((m_ObjectName == "") || (m_ObjectDescription == ""))
    {
	Message.LoadString(IDS_NEED_INPUT_GROUP);
	Message2.LoadString(IDS_TITLE);
	MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	return;
    }

    CDialog::OnOK();
}

BOOL
CEditObject::OnInitDialog() 
{
    CString Message;

    CDialog::OnInitDialog();

    Message.LoadString(IDS_EDIT_OBJ_NAME);
    SetDlgItemText(IDC_EDIT_OBJ_NAME, Message);
    Message.LoadString(IDS_EDIT_OBJ_DESC);
    SetDlgItemText(IDC_EDIT_OBJ_DESC, Message);
    Message.LoadString(IDS_OK);
    SetDlgItemText(IDOK, Message);
    Message.LoadString(IDS_CANCEL);
    SetDlgItemText(IDCANCEL, Message);

    if (m_ObjectName != "")
	SetDlgItemText(IDC_EDIT_NAME, m_ObjectName);
    if (m_ObjectDescription != "")
	SetDlgItemText(IDC_EDIT_DESCRIPTION, m_ObjectDescription);

    return TRUE;
}
