// ShutSrv.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CShutdownServer dialog

class CShutdownServer : public CDialog
{
// Construction
public:
	CString m_caption;
	BOOL    m_bSoft;
	CShutdownServer(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CShutdownServer)
	enum { IDD = IDD_SHUTDOWN_SERVER };
	CButton	m_softRadio;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CShutdownServer)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CShutdownServer)
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioHard();
	afx_msg void OnRadioSoft();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
