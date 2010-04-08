// Discon3.h : header file
//

extern "C" {
#include "monitor.h"    // LPNODEUSERDATAMIN
}

/////////////////////////////////////////////////////////////////////////////
// CDlgDisconnUser dialog

class CDlgDisconnUser : public CDialog
{
// Construction
public:
	LPNODEUSERDATAMIN m_pUserDataMin;
	CDlgDisconnUser(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgDisconnUser)
	enum { IDD = IDD_NODE_DISCONNECT };
	CListBox	m_windowslist;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgDisconnUser)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgDisconnUser)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
