// Discon2.h : header file
//

extern "C" {
#include "monitor.h"    // LPNODESVRCLASSDATAMIN
}

/////////////////////////////////////////////////////////////////////////////
// CDlgDisconnServer dialog

class CDlgDisconnServer : public CDialog
{
// Construction
public:
	LPNODESVRCLASSDATAMIN m_pServerDataMin;
	CDlgDisconnServer(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgDisconnServer)
	enum { IDD = IDD_NODE_DISCONNECT };
	CListBox	m_windowslist;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgDisconnServer)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgDisconnServer)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
