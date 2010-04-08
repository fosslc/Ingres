/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : statview.h, Header file.  (View Window)
**    Project  : CA-OpenIngres/Monitoring
**    Author   : UK Sotheavut
**    Purpose  : Drawing the statistic bar 
**
** History:
**
** xx-Apr-1997 (uk$so01)
**    Created
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
*/


#ifndef STATVIEW_HEADER
#define STATVIEW_HEADER
#define EFFECT_TOP   0x0001
#define EFFECT_RIGHT 0x0002
#define EFFECT_ALL   (EFFECT_TOP | EFFECT_RIGHT)

class CaBarData;
class CaOccupantData;
class CvStatisticBarView : public CScrollView
{
protected:
	CvStatisticBarView();
	DECLARE_DYNCREATE(CvStatisticBarView)
	CSize m_sizeTotal;

	void DrawBars  (CDC* pDC, CRect r, int nBar, CaBarData* pBar, double dDensity, int nWidth);
	void DrawAxis  (CDC* pDC, CRect rect, double dDensity);
	void DrawSelectInfo (CaBarData* pBar, CaOccupantData* pOccupant = NULL, BOOL bCaption = FALSE);
	void DrawBars3D(CDC* pDC, CRect r, int nBar, CaBarData* pBar, double dDensity, int nWidth, BOOL bUsesPalette);

	void Draw3DHatch (CDC* pDC, CRect r, BOOL bHatch, COLORREF crColorHatch);
	void Draw3DEffect(CDC* pDC, CRect r, COLORREF crColor, UINT nEffect = EFFECT_ALL);
	void DrawBorder(CDC* pDC, CRect r);
	void Draw2D(CDC* pDC);
	void Draw3D(CDC* pDC);

public:
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CvStatisticBarView)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnInitialUpdate();     // first time after construct
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

	// Implementation
protected:
	virtual ~CvStatisticBarView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	BOOL m_b3D;             // Default = FALSE
	BOOL b_bShowBarTitle;   // Default = TRUE;
	int m_nXHatchWidth;
	int m_nXHatchHeight;

	// Generated message map functions
	//{{AFX_MSG(CvStatisticBarView)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	int m_xBarSeperator;
	int m_xLeftMargin;
	int m_yTopMargin;
	int m_yBottomMargin;
	int m_nBarWidth;

	int m_nTextHeight; // For the popup.
	COLORREF m_cColorFree;
	CString  m_strFree;
};

#endif
