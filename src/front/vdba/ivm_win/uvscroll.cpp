/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : uvscroll.cpp, implementation file
**    Project  : Ingres Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Custom vertical scroll bar
**
** History:
**
** 01-Sep-1999 (uk$so01)
**    created
** 21-Jul-2000 (uk$so01)
**    BUG #102152. Extend the Vertical scroll range limit to be greater than 32767 
** 24-Jul-2000 (noifr01) (additional change for the same bug (102152))
**    with the fix above, scrolling up/down by 1 page could result in an incorrect
**    position (from something like 1 to 3 skipped or overlapped lines of messages
**    for <100000 messages chosen in the preferences, to much more if there were 
**    significantly more lines in the errlog.log file).  
**    (now use the exact row number of the row visible at the top of the window,
**    before adding/substracting nCountPerPage, rather than applying the
**    "zoom factor" to the current position of the scrollbar, because that position
**    is limited to 32766 different values and therefore is not sufficient for the 
**    current calculation)
** 24-Jul-2000 (noifr01) one more change for 102152: 
**    "scroll up one line" did'nt work for the very first lines of the control if the 
**    scroll offset was a few lines only (for example , just do "page down" through the
**    scrollbar, and repeat "line up" (through the "arrow up" of the scrollbar): the 
**    scroll will stop a few lines before reaching the very first) 
**    
**/


#include "stdafx.h"
#include "rcdepend.h"
#include "lsscroll.h"
#include "uvscroll.h"
#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuVerticalScroll

CuVerticalScroll::CuVerticalScroll()
{
	m_pListCtrl = NULL;
	m_dDiv = 1.0;
	m_nRealScrollMax = 0;
	m_nScrollPosExted=0;
}

CuVerticalScroll::~CuVerticalScroll()
{
}


BEGIN_MESSAGE_MAP(CuVerticalScroll, CScrollBar)
	//{{AFX_MSG_MAP(CuVerticalScroll)
	ON_WM_VSCROLL_REFLECT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuVerticalScroll message handlers


void CuVerticalScroll::VScroll(UINT nSBCode, UINT nPos) 
{
	ASSERT (m_pListCtrl);
	if (!m_pListCtrl)
		return;
	SCROLLINFO sc;
	int nCountPerPage = m_pListCtrl->GetCountPerPage();
	int nMin, nMax;
	int nIndex = nPos;
	double dCourrentMax = 0.0;

	switch (nSBCode)
	{
	case SB_THUMBTRACK:
		m_pListCtrl->DrawThumbTrackInfo(nPos);
		break;
	case SB_THUMBPOSITION:
		if (m_nRealScrollMax > 32766)
		{
			GetScrollRange (&nMin, &nMax);
			dCourrentMax = m_dDiv * (double)nIndex;
			double dEps = dCourrentMax - (double)m_nRealScrollMax;
			if (fabs (dEps) < 0.5)
				nIndex = m_nRealScrollMax;
			else
				nIndex = (int)dCourrentMax;
		}

		if (m_pListCtrl->MoveAt (nIndex))
		{
			SetScrollPos (nPos, TRUE, nIndex);
			TRACE1 ("SB_THUMBPOSITION: pos = %d\n", nIndex);
		}
		break;
	case SB_LINEDOWN:
		GetScrollInfo (&sc, SIF_ALL);
		if (sc.nPos == sc.nMax)
			break;

		if (m_pListCtrl->NextItems())
		{
			nIndex = GetScrollPosExtend();
			nIndex +=1;
			SetScrollPos (nIndex, TRUE, nIndex);
			TRACE1 ("SB_THUMBPOSITION: pos = %d\n", nIndex);
		}
		break;

	case SB_PAGEDOWN:
		GetScrollInfo (&sc, SIF_ALL);
		if (sc.nPos == sc.nMax)
			break;

		if (m_nRealScrollMax <= 32766)
		{
			sc.nPos += nCountPerPage;
			if (sc.nPos > sc.nMax)
				sc.nPos = sc.nMax;
		}
		nIndex = sc.nPos;
		
		if (m_nRealScrollMax > 32766)
		{
			nIndex = GetScrollPosExtend()+ nCountPerPage ;
			if (nIndex > m_nRealScrollMax)
				nIndex = m_nRealScrollMax;
		}

		if (m_pListCtrl->MoveAt (nIndex))
		{
			SetScrollPos (sc.nPos, TRUE, nIndex);
			TRACE1 ("SB_PAGEDOWN: pos = %d\n", nIndex);
		}
		break;

	case SB_LINEUP:
		GetScrollInfo (&sc, SIF_ALL);
		if (m_nRealScrollMax <= 32766) {
			if (sc.nPos == sc.nMin)
				break;
		}
		else {
			if (GetScrollPosExtend()<1)
				break;
		}


		if (m_pListCtrl->BackItems())
		{
			nIndex = GetScrollPosExtend();
			nIndex -=1;
			SetScrollPos (nIndex, TRUE, nIndex);
			TRACE1 ("SB_THUMBPOSITION: pos = %d\n", sc.nPos);
		}
		break;

	case SB_PAGEUP:
		GetScrollInfo (&sc, SIF_ALL);
		if (sc.nPos == sc.nMin)
			break;
		
		if (m_nRealScrollMax <= 32766)
		{
			sc.nPos -= nCountPerPage;
			if (sc.nPos < sc.nMin)
				sc.nPos = sc.nMin;
		}

		nIndex = sc.nPos;
		if (m_nRealScrollMax > 32766)
		{
			nIndex = GetScrollPosExtend() - nCountPerPage ;
			if (nIndex < sc.nMin)
				nIndex = sc.nMin;
		}
		
		if (m_pListCtrl->MoveAt (nIndex))
		{
			TRACE1 ("SB_PAGEUP: pos = %d\n", nIndex);
			SetScrollPos (sc.nPos, TRUE, nIndex);
		}
		break;
	case SB_ENDSCROLL:
		m_pListCtrl->DrawThumbTrackInfo(nPos, TRUE);
		break;
	default:
		break;
	}
}

void CuVerticalScroll::SetScrollRange( int nMinPos, int nMaxPos, BOOL bRedraw)
{
	m_nRealScrollMax = nMaxPos;
	ASSERT (nMinPos >= 0);
	if (nMinPos < 0)
		nMinPos = 0;
	if (nMinPos >= 0 && nMaxPos <= 32766)
		CScrollBar::SetScrollRange(nMinPos, nMaxPos, bRedraw);
	else
	{
		double dDiv = (double)nMaxPos / 32766.0;
		if (dDiv < 1.0)
			dDiv = 1.0;

		m_dDiv = dDiv;
		nMaxPos = (int) ((double)nMaxPos / m_dDiv);
		CScrollBar::SetScrollRange(nMinPos, nMaxPos, bRedraw);
	}
}

int CuVerticalScroll::SetScrollPos( int nPos, BOOL bRedraw, int nRealPos)
{
	if (m_nRealScrollMax <= 32766)
	{
		//
		// Just ignore the 'nRealPos':
		SetScrollPosExtend (nPos);
		return CScrollBar::SetScrollPos(nPos, bRedraw);
	}
	else
	{
		if (nRealPos == -1)
		{
			SetScrollPosExtend (nPos);
			return CScrollBar::SetScrollPos(nPos, bRedraw);
		}
		else
		{
			SetScrollPosExtend (nRealPos);
			int nNewPos = (int)((double)nRealPos / m_dDiv);
			return CScrollBar::SetScrollPos(nNewPos, bRedraw);
		}
		return 0;
	}
}
