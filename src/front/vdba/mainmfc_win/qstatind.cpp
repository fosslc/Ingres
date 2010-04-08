/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : qstatind.cpp, Implementation file                                     //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres Visual DBA.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Draw the static window as an indicator bar of percentage.             //
//                                                                                     //
****************************************************************************************/
#include "stdafx.h"
#include "qstatind.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_SERIAL (CaStaticIndicatorData, CObject, 1)
void CaStaticIndicatorData::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_crPercent;
		ar << m_dPercent;
	}
	else
	{
		ar >> m_crPercent;
		ar >> m_dPercent;
	}
}


/////////////////////////////////////////////////////////////////////////////
// CuStaticIndicatorWnd

CuStaticIndicatorWnd::CuStaticIndicatorWnd()
{
	m_bVertical = FALSE;
	m_crColor   = RGB (0, 0, 255);
	m_dTotal    = 0.0;
	m_strText   = _T("");
}

void CuStaticIndicatorWnd::DeleteIndicator()
{
	while (!m_listPercent.IsEmpty())
		delete m_listPercent.RemoveHead();
	m_dTotal = 0.0;
}

CuStaticIndicatorWnd::~CuStaticIndicatorWnd()
{
	DeleteIndicator();
}


BEGIN_MESSAGE_MAP(CuStaticIndicatorWnd, CStatic)
	//{{AFX_MSG_MAP(CuStaticIndicatorWnd)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CuStaticIndicatorWnd message handlers

void CuStaticIndicatorWnd::OnPaint() 
{
	CPaintDC dc(this); 
	int W, w, ixStart = 0;
	double dDiff = 0.0;
	CRect r, rcInit;
	GetClientRect (rcInit);
	r = rcInit;
	CBrush rb (m_crColor);
	CBrush* pOld = dc.SelectObject (&rb);
	dc.FillRect (rcInit, &rb);
	CaStaticIndicatorData* pData = NULL;
	POSITION pos = m_listPercent.GetHeadPosition();
	W = m_bVertical? rcInit.Height(): rcInit.Width();
	ixStart = m_bVertical? rcInit.top: rcInit.left;
	while (pos != NULL)
	{
		pData = (CaStaticIndicatorData*)m_listPercent.GetNext (pos);
		dDiff = pData->m_dPercent - 0.0;
		if (dDiff < 0)
			dDiff = -dDiff;
		if (dDiff < 0.01)
			continue;
		w = (int)((pData->m_dPercent * (double)W)/100.00);
		CBrush rb (pData->m_crPercent);
		dc.SelectObject (&rb);
		if (m_bVertical)
		{
			r.top    = ixStart;
			//r.bottom = (pos == NULL)? W: ixStart + w;
			r.bottom = W; 
			dc.FillRect (r, &rb);
			ixStart+= w;
		}
		else
		{
			r.left  = ixStart;
			r.right = (pos == NULL)? W: ixStart + w;
			dc.FillRect (r, &rb);
			ixStart+= w;
		}
	}
	if (!m_strText.IsEmpty())
	{
		int nBkOldMode = dc.SetBkMode(TRANSPARENT);
		GetClientRect(&r);
		dc.DrawText(m_strText,-1, r, DT_SINGLELINE|DT_LEFT|DT_VCENTER);
		dc.SetBkMode(nBkOldMode);
	}
	dc.SelectObject (pOld);
}
