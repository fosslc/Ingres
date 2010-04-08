// ChBoxBar.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CuCheckBoxBar window
#ifndef CHBOXBAR_HEADER
#define CHBOXBAR_HEADER


class CuCheckBoxBar : public CButton
{
// Construction
public:
	CuCheckBoxBar();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuCheckBoxBar)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CuCheckBoxBar();

	// Generated message map functions
protected:
	//{{AFX_MSG(CuCheckBoxBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif
/////////////////////////////////////////////////////////////////////////////
