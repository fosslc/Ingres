// Toolbars.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Utility functions

BOOL HasToolbar(CMDIChildWnd *pCurFrame);
BOOL IsToolbarVisible(CMDIChildWnd *pCurFrame);
BOOL SetToolbarInvisible(CMDIChildWnd *pCurFrame, BOOL bUpdateImmediate = FALSE);
BOOL SetToolbarVisible(CMDIChildWnd *pCurFrame, BOOL bUpdateImmediate = FALSE);
BOOL SetToolbarCaption(CMDIChildWnd *pCurFrame, LPCSTR caption);


/////////////////////////////////////////////////////////////////////////////
// CDlgToolbars dialog

class CDlgToolbars : public CDialog
{
// Construction
public:
	CDlgToolbars(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgToolbars)
	enum { IDD = IDD_TOOLBARS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgToolbars)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgToolbars)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	CCheckListBox m_CheckListBox;
};
