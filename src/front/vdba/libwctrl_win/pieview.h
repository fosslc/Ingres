/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : pieview.h, Header file.  (View Window)
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Drawing the Pie chart
**
** History:
**
** xx-Apr-1997 (uk$so01)
**    Created
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
*/


#ifndef PIEVIEW_HEADER
#define PIEVIEW_HEADER

class CvStatisticPieView : public CView
{
protected:
	CvStatisticPieView();
	DECLARE_DYNCREATE(CvStatisticPieView)
	BOOL IsInsideDisk   (CPoint point);
	void DrawLogFileExtraInfo (CDC* pDC, CaPieInfoData* pPie);

public:
	void DrawPie   (CDC* pDC, double degStart, double degEnd, COLORREF color, BOOL bSplit = FALSE);
	BOOL IsInsideSector (double degStart, double degEnd, CPoint point);
	
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvStatisticPieView)
	public:
	virtual void OnInitialUpdate();
	virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL);
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL
	
	// Implementation
protected:
	virtual ~CvStatisticPieView();

	int m_nXDiffRadius;
	int m_nYDiffRadius;

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	void DrawMessage (CDC* pDC, const CString& strMessage);
	void DrawUperBorder (CDC* pDC, double dDegStart, double dDegEnd);
	void LogFileSetMargins();

	// Generated message map functions
protected:
	CPoint m_center;
	CRect  m_EllipseRect;
	double m_dRadialX;
	double m_dRadialY;
	
	int    m_xLeftMargin;
	int    m_xRightMargin;
	int    m_yTopMargin;
	int    m_yBottomMargin;
	
	//{{AFX_MSG(CvStatisticPieView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	int    m_pieHeight;
	BOOL   m_bInitialize;
	BOOL   m_bDrawPieBorder;
	CSize  m_size;
	int    m_nTextHeight;
	CImageList m_imageDirection;
	
	int  fx (double deg);
	int  fy (double deg);
	int  fx (double deg, CPoint center);
	int  fy (double deg, CPoint center);

	int  Fx (double deg);
	int  Fy (double deg);
	int  Fx (double deg, CPoint center);
	int  Fy (double deg, CPoint center);
	
	BOOL IsFirstQuarter   (double degStart, double degEnd);
	BOOL IsSecondQuarter  (double degStart, double degEnd);
	BOOL IsThirdQuarter   (double degStart, double degEnd);
	BOOL IsFourthQuarter  (double degStart, double degEnd);
	
	BOOL PtInFisrtQuarter (double degStart, double degEnd, CPoint point);
	BOOL PtInSecondQuarter(double degStart, double degEnd, CPoint point);
	BOOL PtInThirdQuarter (double degStart, double degEnd, CPoint point);
	BOOL PtInFourthQuarter(double degStart, double degEnd, CPoint point);
	
	void EllipseTracker (double degStart, double degEnd);
	void DrawSelectInfo (CaPieInfoData* pPie, CaLegendData* pData);

};

#endif
