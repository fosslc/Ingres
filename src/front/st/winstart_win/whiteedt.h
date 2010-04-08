#if !defined(AFX_WHITEEDIT_H__DC42CCF8_7E2F_11D2_B0FD_00805FE6FB5C__INCLUDED_)
#define AFX_WHITEEDIT_H__DC42CCF8_7E2F_11D2_B0FD_00805FE6FB5C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// WhiteEdit.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWhiteEdit window

class CWhiteEdit : public CEdit
{
// Construction
public:
    CWhiteEdit();
    COLORREF m_clrText;
    COLORREF m_clrBkgnd;
    CBrush m_brBkgnd;
// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWhiteEdit)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWhiteEdit();

	// Generated message map functions
protected:
	//{{AFX_MSG(CWhiteEdit)
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WHITEEDIT_H__DC42CCF8_7E2F_11D2_B0FD_00805FE6FB5C__INCLUDED_)
