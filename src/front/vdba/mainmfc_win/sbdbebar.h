/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sbdbebar.h, Header file
**    Project  : CA-OpenIngres/Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
** 
*    Purpose  : DBEvent Dialog Bar. 
**              When the control bar is floating, its size is the vertical dialog bar template.
**
** HISTORY:
** xx-Feb-1997 (uk$so01)
**    created.
** 21-Fev-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
*/


#ifndef SBDBEBAR_HEADER
#define SBDBEBAR_HEADER

class CuDbeDlgBar : public CDialogBar
{
	CWnd* m_pParent;
public:
	CuDbeDlgBar();
	~CuDbeDlgBar();
	BOOL Create (CWnd* pParent, UINT nIDTemplate, UINT nStyle, UINT nID, BOOL bChange = TRUE);
	BOOL Create (CWnd* pParent, LPCTSTR lpszTemplateName, UINT nStyle, UINT nID, BOOL bChange = TRUE);
	void UpdateDisplay();
	CComboBox*       GetComboDatabase()   {return &m_ComboDB;};
	CEdit*           GetEditMaxLine()     {return &m_MaxLine;};
	CSpinButtonCtrl* GetSpinCtrl()        {return &m_Spin1;};
	CButton*         GetPopupButton()     {return &m_Check1;}
	CButton*         GetSysDBEventButton(){return &m_Check2;}
	CButton*         GetClearFirstButton(){return &m_Check3;}

	//
	// Overrides
	virtual CSize CalcDynamicLayout (int nLength, DWORD dwMode);
public:
	CString m_strMaxLine;
private:
	CRect ctrlVRect [9]; // The control's placement.
	CRect ctrlHRect [9]; // The control's placement.
	HICON m_arrayIcon[2];

protected:
	CSize           m_HdlgSize;      // Control bar size when it is Horizontal docked.
	CSize           m_VdlgSize;      // Control bar size when it is Vertical docked.  
	CPoint          m_Button01Position;
	CButton         m_Button01;      // Refresh
	CButton         m_Button02;      // Reset
	CComboBox       m_ComboDB;       // Database
	CButton         m_Check1;        // Popup on Raise
	CButton         m_Check2;        // System DB Event
	CButton         m_Check3;        // Clear First When Max Reached.
	CStatic         m_MaxLineText;   // Max Line:
	CEdit           m_MaxLine;       // Edit Box
	CSpinButtonCtrl m_Spin1;         // Spin Ctrl

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CuIpmBarDlgTemplate dialog

class CuDbeBarDlgTemplate : public CDialog
{
public:
	CuDbeBarDlgTemplate(CWnd* pParent = NULL);

	// Dialog Data
	//{{AFX_DATA(CuDbeBarDlgTemplate)
	enum { IDD = IDD_DBEVENTBARV };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDbeBarDlgTemplate)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CuDbeBarDlgTemplate)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
#endif
