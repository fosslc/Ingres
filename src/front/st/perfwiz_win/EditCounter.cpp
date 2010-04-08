/*
**
**  Name: EditCounter.cpp
**
**  Description:
**	This file contains the routines necessary for the Counter Edit
**	Dialog Box, which allows users to edit a personal counter's
**	attributes.
**
**  History:
**	25-oct-1999 (somsa01)
**	    Created.
**	29-oct-1999 (somsa01)
**	    Before returning, we'll check the query via the CheckQuery()
**	    function.
*/

#include "stdafx.h"
#include "perfwiz.h"
#include "EditCounter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern "C" int  CheckQuery(char *dbname, char *qry, char *errtxt);

/*
** CEditCounter dialog
*/

CEditCounter::CEditCounter(CWnd* pParent /*=NULL*/)
	: CDialog(CEditCounter::IDD, pParent)
{
    //{{AFX_DATA_INIT(CEditCounter)
	    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

void CEditCounter::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CEditCounter)
	    // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CEditCounter, CDialog)
    //{{AFX_MSG_MAP(CEditCounter)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** CEditCounter message handlers
*/
BOOL
CEditCounter::OnInitDialog() 
{
    char    buffer[16];
    CString Message;

    CDialog::OnInitDialog();

    Message.LoadString(IDS_EDIT_COUNTER_NAME);
    SetDlgItemText(IDC_EDIT_COUNTER_NAME, Message);
    Message.LoadString(IDS_COUNTER_SCALE);
    SetDlgItemText(IDC_COUNTER_SCALE, Message);
    Message.LoadString(IDS_DB_NAME);
    SetDlgItemText(IDC_DB_NAME, Message);
    Message.LoadString(IDS_COUNTER_QUERY);
    SetDlgItemText(IDC_COUNTER_QUERY, Message);
    Message.LoadString(IDS_EDIT_COUNTER_DESC);
    SetDlgItemText(IDC_EDIT_COUNTER_DESC, Message);

    SetDlgItemText(IDC_EDIT_NAME, m_CounterName);
    SetDlgItemText(IDC_EDIT_SCALE, itoa(m_CounterScale, buffer, 10));
    SetDlgItemText(IDC_EDIT_DBNAME, m_CounterDBName);
    SetDlgItemText(IDC_EDIT_QUERY, m_CounterQuery);
    SetDlgItemText(IDC_EDIT_DESCRIPTION, m_CounterDescription);
	
    return TRUE;
}

void
CEditCounter::OnOK() 
{
    CString buffer, Message, Message2;
    char    errtxt[1024];

    GetDlgItemText(IDC_EDIT_NAME, m_CounterName);
    GetDlgItemText(IDC_EDIT_SCALE, buffer);
    GetDlgItemText(IDC_EDIT_DBNAME, m_CounterDBName);
    GetDlgItemText(IDC_EDIT_QUERY, m_CounterQuery);
    GetDlgItemText(IDC_EDIT_DESCRIPTION, m_CounterDescription);

    Message2.LoadString(IDS_TITLE);
    if (m_CounterName == "")
    {
	Message.LoadString(IDS_NEED_CTRNAME);
	MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
    }
    else if (buffer == "")
    {
	Message.LoadString(IDS_NEED_SCALE);
	MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
    }
    else if (m_CounterDBName == "")
    {
	Message.LoadString(IDS_NEED_DBNAME);
	MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
    }
    else if (m_CounterQuery == "")
    {
	Message.LoadString(IDS_NEED_QUERY);
	MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
    }
    else if (m_CounterDescription == "")
    {
	Message.LoadString(IDS_NEED_CTRDESC);
	MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
    }
    else
    {
	m_CounterScale = atoi(buffer);
	if ((!m_CounterScale) && (strcmp(buffer, "0") != 0))
	{
	    Message.LoadString(IDS_INVALID_SCALE);
	    MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	    return;
	}

	/*
	** Make sure that this query brings back a singe numeric value
	** as the result.
	*/
	if (CheckQuery( m_CounterDBName.GetBuffer(0),
		        m_CounterQuery.GetBuffer(0), errtxt) != 0 )
	{
	    char    outmsg[2048];

	    Message.LoadString(IDS_CHKQRY_ERROR);
	    sprintf(outmsg, Message, errtxt);
	    MessageBox(outmsg, Message2, MB_OK | MB_ICONEXCLAMATION);
	    return;
	}

	CDialog::OnOK();
    }
}
