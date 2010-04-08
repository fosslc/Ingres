/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : fminifrm.h : header file
**  Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
**  Author   : UK Sotheavut (uk$so01)
**  Purpose  : This file contains the mini-frame that controls modeless 
**             property sheet CuPSheetDetailText
**
** History :
**
** 06-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**    created
** 07-Mar-2003 (uk$so01)
**    SIR #109773, Add property frame for displaying the description 
**    of ingres messages.
**/


#if !defined (FRAME_MINIPROPERTY_FRAME_HEADER)
#define FRAME_MINIPROPERTY_FRAME_HEADER
class CuPropertySheet;

class CfMiniFrame : public CMiniFrameWnd
{
	DECLARE_DYNCREATE(CfMiniFrame)
public:
	CfMiniFrame();
	CfMiniFrame(CTypedPtrList < CObList, CPropertyPage* >& listPages, BOOL bDeleteOnDestroy, int nActive = 0);

	//
	// If bDeleteOnDestroy = TRUE, then this mini-frame will use the delete operator
	// to delete the objects in the list listPages after they have been destoyed (their hWnd).
	void NewPages (CTypedPtrList < CObList, CPropertyPage* >& listPages, BOOL bDeleteOnDestroy, int nActive = 0);
	void RemoveAllPages();
	void HandleData(LPARAM lpData);
	void SetDefaultPageCaption(LPCTSTR lpszCaption){m_strDefaultTextPageCaption = lpszCaption;}
	void SetNoPageMessage(LPCTSTR lpszMsg){m_strEmpty = lpszMsg;}
	CString GetNoPageMessage(){return m_strEmpty;}
	CTypedPtrList < CObList, CPropertyPage* >& GetUserPages(){return m_listUserPages;}

	//
	// Only if the miniframe has the default page (the default edit page)
	void SetText (LPCTSTR lpszText, LPCTSTR lpszTabCaption);

public:
	CuPropertySheet* m_pModelessPropSheet;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CfMiniFrame)
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void HandleNoPage();
	void HandleDefaultPageCaption();

	int  m_nActive;
	BOOL m_bDeleteOnDestroy;
	CTypedPtrList < CObList, CPropertyPage* > m_listUserPages;
	LPARAM m_lpData;
	CString m_strEmpty;
	CString m_strDefaultTextPageCaption;
public:
	virtual ~CfMiniFrame();

	// Generated message map functions
	//{{AFX_MSG(CfMiniFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // FRAME_MINIPROPERTY_FRAME_HEADER
