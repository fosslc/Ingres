/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sbdlgbar.h, header file 
**    Project  : Visual DBA.
**    Author   : UK Sotheavut  (uk$so01)
** 
**    Purpose  : Virtual Node Dialog Bar Resizable.
**               OnSize and OnPaint:
**               The parent window (MainFrame window) must call the member
**               function RecalcLayout() to resize correctly this control bar.
**
** HISTORY:
** xx-Feb-1997 (uk$so01)
**    created.
** 15-feb-2000 (somsa01)
**    Properly declared OnMove().
** 26-Fev-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
*/

#ifndef SBDLGBAR_HEADER
#define SBDLGBAR_HEADER
#include "bmpbtn.h"
#include "splitter.h"
#include "vtreeglb.h"
#include "vtreectl.h"

#define NUMBEROFBUTTONS   12
class CuResizableDlgBar : public CDialogBar
{
public:
	CuResizableDlgBar();
	~CuResizableDlgBar();
	BOOL Create (CWnd* pParent, UINT nIDTemplate, UINT nStyle, UINT nID, BOOL bChange = TRUE);
	BOOL Create (CWnd* pParent, LPCTSTR lpszTemplateName, UINT nStyle, UINT nID, BOOL bChange = TRUE);
	void UpdateDisplay();
	CTreeGlobalData* GetPTreeGD() {return m_pTreeGlobalData; }
	void SetPTreeGD(CTreeGlobalData* pTreeGD) {m_pTreeGlobalData = pTreeGD; }

	//
	// Overrides
	virtual UINT GetCurrentAlignment();
	virtual UINT GetCurrentAlignment(CRect r, CRect rMDIClient);

	virtual CSize CalcDynamicLayout (int nLength, DWORD dwMode);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	CVtreeCtrl m_Tree;
	//
	// Attibutes
	CSize m_sizeDocked;
	CSize m_sizeFloating;
	BOOL  m_bChangeDockedSize;
private:
	HICON m_arrayIcon[NUMBEROFBUTTONS];
	DWORD m_WindowVersion;
	CTreeGlobalData* m_pTreeGlobalData;
protected:
	CWnd*      m_pWndMDIClient;
	BOOL       m_bCapture;
	UINT       m_Alignment;
	CPen*      m_pXorPen;
	CPen*      m_pOldPen;
	CPoint     m_Button01Position;
	CuSplitter m_SplitterLeft;
	CuSplitter m_SplitterRight;
	CuSplitter m_SplitterTop;
	CuSplitter m_SplitterBottom;
	CPoint     m_Pa;
	CPoint     m_Pb;

	CuBitmapButton m_Button01;  // Connect DOM Window 
	CuBitmapButton m_Button02;  // Open SQL Test Window
	CuBitmapButton m_Button03;  // Open Monitor Window 
	CuBitmapButton m_Button04;  // Open DBEvent Tree Window 
	CuBitmapButton m_ButtonSC;  // Scratchpad DOM Window
	CuBitmapButton m_Button05;  // Disconnect Virtual Node
	CuBitmapButton m_Button06;  // Close the Selected Window
	CuBitmapButton m_Button07;  // Activate the Select Window
	CuBitmapButton m_Button08;  // Add 
	CuBitmapButton m_Button09;  // Alter
	CuBitmapButton m_Button10;  // Drop 
	CuBitmapButton m_Button11;  // Force Refresh the List of Virtual Nodes 

	CWnd* GetMDIClientWnd();
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnInitDialog();
	afx_msg void OnItemexpandingTree1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd*, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnVnodebarHide();
	afx_msg void OnVnodebarDockedview();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
	afx_msg void OnMove (int x, int y);
	DECLARE_MESSAGE_MAP()
};

#endif
