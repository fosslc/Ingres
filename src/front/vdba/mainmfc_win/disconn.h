// Disconn.h : header file
//

extern "C" {
#include "monitor.h"    // LPNODEDATAMIN
}

/////////////////////////////////////////////////////////////////////////////
// CDlgDisconnect dialog

class CDlgDisconnect : public CDialog
{
// Construction
public:
	LPNODEDATAMIN m_pNodeDataMin;
	CDlgDisconnect(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgDisconnect)
	enum { IDD = IDD_NODE_DISCONNECT };
	CListBox	m_windowslist;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgDisconnect)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgDisconnect)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
