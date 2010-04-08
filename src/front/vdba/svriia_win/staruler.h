/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : staruler.h : header file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Static Control that looks like a ruler
**
** History:
**
** 19-Dec-2000 (uk$so01)
**    Created
**/


#if !defined(AFX_STARULER_H__0C3109AF_D5B8_11D4_8739_00C04F1F754A__INCLUDED_)
#define AFX_STARULER_H__0C3109AF_D5B8_11D4_8739_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//
// CLASS CaHeaderMarker:
// ************************************************************************************************
class CaHeaderMarker: public CObject
{
public:
	CaHeaderMarker():m_point(CPoint(0, 0)){}
	CaHeaderMarker(CPoint point):m_point(point){}
	virtual ~CaHeaderMarker(){}
	void SetPoint(CPoint point){m_point = point;}
	CPoint GetPoint(){return m_point;}
protected:
	CPoint m_point;

};


//
// CLASS CuStaticRuler:
// ************************************************************************************************
class CuStaticRuler : public CStatic
{
public:
	CuStaticRuler();

	void SetGridColor(COLORREF rgb = RGB(255, 255, 255)){m_rgbGrid = rgb;}
	void SetBodyColor(COLORREF rgb = RGB( 10, 192, 192)){m_rgbBody = rgb;}

	void OffsetOrigin(int nOffset);
	CPoint AddHeaderMarker(CPoint p);
	CPoint RemoveHeaderMarker(CPoint p);
	int GetPointArray (CArray< CPoint,CPoint >& pointArray);
	BOOL ExistHeaderMarker(CPoint& point);
	CaHeaderMarker* GetHeaderMarkerAt(CPoint point);
	CPoint ChangePoint(CaHeaderMarker* pHeader, CPoint newPoint);
	int GetCount(){return m_listHeader.GetCount();}
	void Cleanup()
	{
		while (!m_listHeader.IsEmpty())
			delete m_listHeader.RemoveHead();
	}

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuStaticRuler)
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	int m_nOffsetOrigin;
	int m_nLeftMagin;
	COLORREF m_rgbBody;
	COLORREF m_rgbGrid;

public:
	virtual ~CuStaticRuler();
protected:
	CTypedPtrList< CObList, CaHeaderMarker* > m_listHeader;

	void DrawRuler(CDC* pDC);
	void DrawHeaderMarker(CDC* pDC, CaHeaderMarker* pMarker);
	void AdjustPoint (CPoint& point);


	// Generated message map functions
protected:
	//{{AFX_MSG(CuStaticRuler)
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STARULER_H__0C3109AF_D5B8_11D4_8739_00C04F1F754A__INCLUDED_)
