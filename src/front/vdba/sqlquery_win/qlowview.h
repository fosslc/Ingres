/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qlowview.h, Header file
**    Project  : CA-OpenIngres Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Lower view of the sql frame. It contains a CTabCtrl to display the 
**               history of result of the queries
**
** History:
**
** xx-Oct-1997 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/

#if !defined(AFX_QLOWVIEW_H__21BF2677_4A04_11D1_A20C_00609725DDAF__INCLUDED_)
#define AFX_QLOWVIEW_H__21BF2677_4A04_11D1_A20C_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "qresudlg.h"


class CvSqlQueryLowerView : public CView
{
protected:
	CvSqlQueryLowerView();
	DECLARE_DYNCREATE(CvSqlQueryLowerView)

public:
	CuDlgSqlQueryResult* GetDlgSqlQueryResult() {return m_pQueryResultDlg;}

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvSqlQueryLowerView)
	public:
	virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL);
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrintPreview(CDC* pDC, CPrintInfo* pInfo, POINT point, CPreviewView* pView);
	//}}AFX_VIRTUAL

public:
	void DoFilePrint();
	void DoFilePrintPreview();

private:
	CFont m_printerFont;
	CFont m_printerCaptFont;  // For captions
	CFont m_printerBoldFont;  // For container header

	int m_nLineHeight;        // Height of one line using printer font
	int m_nCaptLineHeight;    // Height of one line using printer captions font
	int m_nBoldLineHeight;    // Height of one line using printer bold font

	int m_nPageHeight;        // printing page height, margins deducted (updated while printing)
	int m_nPageWidth;         // printing page height, margins deducted (updated while printing)

	int m_ScalingFactorX;     // according to current resolution - power of 2
	int m_ScalingFactorY;     // according to current resolution - power of 2

	int m_PixelXPrintScale;   // for widths available in displayed pixels, such as columns display width for resulting tuples
	int m_PixelYPrintScale;   // for heights available in displayed pixels

	int m_nHeaderHeight;      // Height of printed header
	int m_nFooterHeight;      // Height of printed footer

	int m_nbPrintPage;        // Number of pages to be printed, calculated at first call to PrintPage()

	CaQueryPageData* m_pPageData; // data to be printed, picked from the current property page

private:
	void PrintPageHeader(CDC* pDC, int pageNum);
	void PrintPageFooter(CDC* pDC, int pageNum);
	void PrintPage(CDC* pDC, int pageNum);

	// Specific print methods according to pane type
	void PrintSelect   (CDC* pDC, int pageNum, int x, int &y, CaQuerySelectPageData* pPageData, int pageDataHeight);
	void PrintNonSelect(CDC* pDC, int pageNum, int x, int &y, CaQueryNonSelectPageData* pPageData, int pageDataHeight);
	void PrintQep      (CDC* pDC, int pageNum, int x, int &y, CaQueryQepPageData* pPageData, int pageDataHeight);
	void PrintRaw      (CDC* pDC, int pageNum, int x, int &y, CaQueryRawPageData* pPageData, int pageDataHeight);

	// utility functions
	void MultilineTextOut(CDC* pDC, int x, int& y, CString& rString, int pageDataHeight);

	//
	// Implementation
protected:
	CuDlgSqlQueryResult* m_pQueryResultDlg;
	virtual ~CvSqlQueryLowerView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	//
	// Generated message map functions
protected:
	//{{AFX_MSG(CvSqlQueryLowerView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QLOWVIEW_H__21BF2677_4A04_11D1_A20C_00609725DDAF__INCLUDED_)
