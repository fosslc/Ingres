/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : bmpbtn.h, Header file.
**    Project  : CA-OpenIngres/Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Owner draw button does not response to the key SPACE, RETURN
**               when the button has focus.
** 
** Histoty:
** xx-Feb-1997 (uk$so01)
**    Created.
** 26-Fev-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
*/

#ifndef BITMAPBUTTON_HEADER
#define BITMAPBUTTON_HEADER

class CuBitmapButton : public CButton
{
public:
	CuBitmapButton();
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuBitmapButton)
	//}}AFX_VIRTUAL
	virtual ~CuBitmapButton();

	// Generated message map functions
protected:
	//{{AFX_MSG(CuBitmapButton)
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
CSize GetToolBarButtonSize(CWnd* pWnd, UINT nToolBarID);

#endif // BITMAPBUTTON_HEADER
