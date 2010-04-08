// ComboBar.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CuComboToolBar window
#ifndef COMBOBAR_HEADER
#define COMBOBAR_HEADER

class CuComboToolBar : public CComboBox
{
// Construction
public:
	CuComboToolBar();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuComboToolBar)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CuComboToolBar();

	// Generated message map functions
protected:
	//{{AFX_MSG(CuComboToolBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
#endif
