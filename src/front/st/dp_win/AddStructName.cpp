/*
**
**  Name: AddStructName.cpp
**
**  Description:
**	This file contains the necessary routines to add a table/index
**	structure to the structure listbox.
**
**  History:
**	10-jul-1999 (somsa01)
**	    Created.
*/

#include "stdafx.h"
#include "dp.h"
#include "AddStructName.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
** AddStructName dialog
*/
AddStructName::AddStructName(CWnd* pParent /*=NULL*/)
	: CDialog(AddStructName::IDD, pParent)
{
    //{{AFX_DATA_INIT(AddStructName)
	    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

void AddStructName::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(AddStructName)
	    // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(AddStructName, CDialog)
    //{{AFX_MSG_MAP(AddStructName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** AddStructName message handlers
*/
void
AddStructName::OnOK() 
{
    GetDlgItemText(IDC_EDIT_STRUCTNAME, StructName);
    if ((!isalpha(StructName[0])) && (StructName[0] != '_'))
    {
	Message.LoadString(IDS_INVALID_STRUCTNAME);
	Message2.LoadString(IDS_TITLE);
	MessageBox(Message, Message2, MB_ICONEXCLAMATION | MB_OK);
    }
    else 
	CDialog::OnOK();
}

BOOL AddStructName::OnInitDialog() 
{
    CDialog::OnInitDialog();
    
    Message.LoadString(IDS_TBNAME_HEADING);
    SetDlgItemText(IDC_TBNAME_HEADING, Message);
    Message.LoadString(IDS_OK);
    SetDlgItemText(IDOK, Message);
    Message.LoadString(IDS_CANCEL);
    SetDlgItemText(IDCANCEL, Message);
    
    return TRUE;
}
