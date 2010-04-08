/*
**  Copyright (C) 2005-2006 Ingres Corporation.
*/

/*
**    Source   : barinfo.cpp, Implementation file (from diskinfo)
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : The data type of Statistic bar. (Disk statistic of occupation)
**
** History:
**
** xx-Apr-1997 (uk$so01)
**    Created
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 08-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
**/

#include "stdafx.h"
#include <math.h>
#include "statfrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CaPieChartCreationData::CaPieChartCreationData()
{
	m_bDBLocation     = TRUE;
	m_bUseLegend      = TRUE;
	m_bLegendRight    = FALSE;
	m_bUseLButtonDown = TRUE;

	m_bShowPercentage = FALSE;
	m_bShowValue      = FALSE;
	m_bValueInteger   = FALSE;

	m_nLines          = 2;
	m_nColumns        = 1;

	m_b3D             = FALSE;
	m_bShowBarTitle   = TRUE;
	m_bDrawAxis       = TRUE;
	m_strBarWidth     = _T("EEEE");
	m_crFree          = NULL;
	m_pPalette        = NULL;

	m_pfnCreatePalette = NULL;

	m_uIDS_IncreasePaneSize = 0;
	m_uIDS_PieIsEmpty = 0;
	m_uIDS_LegendCount= 0;
}


CaPieChartCreationData CaPieChartCreationData::operator = (const CaPieChartCreationData& c)
{
	Copy (c);
	return *this;
}

void CaPieChartCreationData::Copy (const CaPieChartCreationData& c)
{
	m_bDBLocation     = c.m_bDBLocation;
	m_bUseLegend      = c.m_bUseLegend;
	m_bLegendRight    = c.m_bLegendRight;
	m_bUseLButtonDown = c.m_bUseLButtonDown;

	m_bShowPercentage = c.m_bShowPercentage;
	m_bShowValue      = c.m_bShowValue;
	m_bValueInteger   = c.m_bValueInteger;

	m_nLines          = c.m_nLines;
	m_nColumns        = c.m_nColumns;

	m_b3D             = c.m_b3D;
	m_bShowBarTitle   = c.m_bShowBarTitle;
	m_bDrawAxis       = c.m_bDrawAxis;
	m_strBarWidth     = c.m_strBarWidth;
	m_crFree          = c.m_crFree;
	m_pPalette        = c.m_pPalette;

	m_pfnCreatePalette= c.m_pfnCreatePalette;

	m_uIDS_IncreasePaneSize = c.m_uIDS_IncreasePaneSize;
	m_uIDS_PieIsEmpty = c.m_uIDS_PieIsEmpty;
	m_uIDS_LegendCount= c.m_uIDS_LegendCount;
}


//
// Location definition
// *******************
IMPLEMENT_SERIAL (CaBarPieItem, CObject, 1)
CaBarPieItem::CaBarPieItem ()
{
	m_colorRGB = NULL;
	m_strName  = _T("");
	m_bHatch = FALSE;
	m_colorHatch = FALSE;
}

CaBarPieItem::CaBarPieItem (LPCTSTR lpszBar, COLORREF color)
{
	m_strName = lpszBar;
	m_colorRGB= color;
	m_bHatch = FALSE;
	m_colorHatch = FALSE;
}

CaBarPieItem::CaBarPieItem (LPCTSTR lpszBar, COLORREF color, BOOL bHatch, COLORREF colorHatch)
{
	m_strName = lpszBar;
	m_colorRGB= color;
	m_bHatch = bHatch;
	m_colorHatch = colorHatch;
}

CaBarPieItem::CaBarPieItem (const CaBarPieItem* pRefLocationData)
{
	ASSERT (pRefLocationData);

	m_strName  = pRefLocationData->m_strName;
	m_colorRGB = pRefLocationData->m_colorRGB;
	m_bHatch   = pRefLocationData->m_bHatch;
	m_colorHatch = pRefLocationData->m_colorHatch;
}

void CaBarPieItem::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strName;
		ar << m_colorRGB;
		ar << m_bHatch; 
		ar << m_colorHatch;
	}
	else
	{
		ar >> m_strName;
		ar >> m_colorRGB;
		ar >> m_bHatch; 
		ar >> m_colorHatch;
	}
}

//
// Occupant definition
// *******************
IMPLEMENT_SERIAL (CaOccupantData, CaBarPieItem, 1)
CaOccupantData::CaOccupantData (): CaBarPieItem(){}
CaOccupantData::CaOccupantData (LPCTSTR lpszOccupant, COLORREF color, double fPercentage)
	:CaBarPieItem (lpszOccupant, color), m_fPercentage(fPercentage)
{
	m_bFocus = FALSE;
}

CaOccupantData::CaOccupantData (const CaOccupantData* pRefOccupantData)
	:CaBarPieItem (pRefOccupantData->m_strName, pRefOccupantData->m_colorRGB)
{
	ASSERT (pRefOccupantData);

	m_fPercentage = pRefOccupantData->m_fPercentage;
	m_rcOccupant  = pRefOccupantData->m_rcOccupant ;
	m_bFocus      = pRefOccupantData->m_bFocus     ;
}

CaOccupantData::CaOccupantData (LPCTSTR lpszOccupant, COLORREF color, double fPercentage, BOOL bHatch, COLORREF colorHatch)
	:CaBarPieItem (lpszOccupant, color, bHatch, colorHatch), m_fPercentage(fPercentage)
{
	m_bFocus = FALSE;
}



void CaOccupantData::Serialize (CArchive& ar)
{
	CaBarPieItem::Serialize (ar);
	if (ar.IsStoring())
	{
		ar << m_fPercentage; 
		ar << m_rcOccupant;
	}
	else
	{
		ar >> m_fPercentage; 
		ar >> m_rcOccupant;
	}
}

//
// Disk Unit Definition
// ********************
IMPLEMENT_SERIAL (CaBarData, CObject, 1)
CaBarData::CaBarData()
{
	m_nCapacity = 0.0;
	m_bFocus = FALSE;
}

	
CaBarData::CaBarData(LPCTSTR lpszBarUnit, double dCapacity)
{
	m_nCapacity   = dCapacity;
	m_strBarUnit = lpszBarUnit;
	m_bFocus = FALSE;
}

CaBarData::CaBarData(const CaBarData* pRefBarData)
{
	ASSERT (pRefBarData);

	m_strBarUnit = pRefBarData->m_strBarUnit;
	m_nCapacity  = pRefBarData->m_nCapacity;
	m_rcBarUnit = pRefBarData->m_rcBarUnit;

	// For lists: need to allocate objects along with lists dup
	POSITION pos = pRefBarData->m_listOccupant.GetHeadPosition ();
	while (pos != NULL) 
	{
		CaOccupantData* pOccupant = pRefBarData->m_listOccupant.GetNext (pos);
		ASSERT (pOccupant);
		CaOccupantData* pNewOccupant = new CaOccupantData(pOccupant);
		m_listOccupant.AddTail(pNewOccupant);
	}

}

CaBarData::~CaBarData()
{
	while (!m_listOccupant.IsEmpty())
	{
		delete m_listOccupant.RemoveHead();
	}
}

void CaBarData::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strBarUnit;
		ar << m_nCapacity;
		ar << m_rcBarUnit;
	}
	else
	{
		ar >> m_strBarUnit;
		ar >> m_nCapacity;
		ar >> m_rcBarUnit;
	}
	m_listOccupant.Serialize (ar);
}


//
// Disk Space Statistic of Occupation
// **********************************
IMPLEMENT_SERIAL (CaBarInfoData, CObject, 1)
CaBarInfoData::CaBarInfoData()
{
	m_Tracker.m_rect.left   = 10;
	m_Tracker.m_rect.top    = 10;
	m_Tracker.m_rect.right  = 20;
	m_Tracker.m_rect.bottom = 20;
#if defined (MAINWIN)
	m_strBarCaption = _T("Partition");
#else
	m_strBarCaption = _T("Disk");
#endif
	m_strUnit = _T("Mb");
	m_strSmallUnit = _T("");
}

CaBarInfoData::CaBarInfoData(const CaBarInfoData* pRefBarInfoData)
{
	ASSERT (pRefBarInfoData);

	m_Tracker.m_nStyle  = pRefBarInfoData->m_Tracker.m_nStyle;
	m_Tracker.m_rect    = pRefBarInfoData->m_Tracker.m_rect;
	m_strBarCaption     = pRefBarInfoData->m_strBarCaption;
	m_strUnit           = pRefBarInfoData->m_strUnit;
	m_strSmallUnit      = pRefBarInfoData->m_strSmallUnit;
	// For lists: need to allocate objects along with lists dup
	POSITION pos = pRefBarInfoData->m_listBar.GetHeadPosition ();
	while (pos != NULL) 
	{
		CaBarPieItem* pLocation = pRefBarInfoData->m_listBar.GetNext (pos);
		ASSERT (pLocation);
		CaBarPieItem* pNewLocation = new CaBarPieItem(pLocation);
		m_listBar.AddTail(pNewLocation);
	}
	pos = pRefBarInfoData->m_listUnit.GetHeadPosition ();
	while (pos != NULL) 
	{
		CaBarData* pBar = pRefBarInfoData->m_listUnit.GetNext (pos);
		ASSERT (pBar);
		CaBarData* pNewBar = new CaBarData(pBar);
		m_listUnit.AddTail(pNewBar);
	}
}

void CaBarInfoData::Cleanup()
{
	while (!m_listBar.IsEmpty())
	{
		delete m_listBar.RemoveHead();
	}
	while (!m_listUnit.IsEmpty())
	{
		delete m_listUnit.RemoveHead();
	}
}


CaBarInfoData::~CaBarInfoData()
{
	Cleanup();
}

void CaBarInfoData::SetTrackerSelectObject (CRect rcObject, BOOL bSelect)
{
	m_Tracker.m_rect.left  = rcObject.left;
	m_Tracker.m_rect.top   = rcObject.top;
	m_Tracker.m_rect.right = rcObject.right;
	m_Tracker.m_rect.bottom= rcObject.bottom;
	if (bSelect)
	{
		m_Tracker.m_nStyle = CRectTracker::solidLine|CRectTracker::resizeOutside;
	}
	else
	{
		m_Tracker.m_nStyle = 0;
	}
}

void CaBarInfoData::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_Tracker.m_nStyle;
		ar << m_Tracker.m_rect;
		ar << m_strBarCaption;
		ar << m_strUnit;
		ar << m_strSmallUnit;
	}
	else
	{
		DWORD nStyle;
		ar >> nStyle;
		ar >> m_Tracker.m_rect;
		m_Tracker.m_nStyle = nStyle;
		ar >> m_strBarCaption;
		ar >> m_strUnit;
		ar >> m_strSmallUnit;
	}
	m_listBar.Serialize (ar);
	m_listUnit.Serialize (ar);
}

double CaBarInfoData::GetMaxCapacity ()
{
	double dMax = 0.0;
	CaBarData* pBar = NULL;
	POSITION pos = m_listUnit.GetHeadPosition();
	while (pos != NULL)
	{
		pBar = (CaBarData*)m_listUnit.GetNext(pos);
		if (pBar->m_nCapacity > dMax)
			dMax = pBar->m_nCapacity;
	}
	return dMax;
}

DWORD lRandom(VOID)
{
	static DWORD glSeed = (DWORD)-365387184;

	glSeed *= 69069;
	return(++glSeed);
}
void CaBarInfoData::AddBar (LPCTSTR lpszBar)
{
	int r, g, b;
	r = lRandom() % 255;
	g = lRandom() % 255;
	b = lRandom() % 255;
	m_listBar.AddTail (new CaBarPieItem(lpszBar, RGB (r, g, b)));
}

void CaBarInfoData::AddBar (LPCTSTR lpszBar, COLORREF color)
{
	m_listBar.AddTail (new CaBarPieItem(lpszBar, color));
}

void CaBarInfoData::AddBar (LPCTSTR lpszBar, COLORREF color, BOOL bHatch, COLORREF colorHatch)
{
	m_listBar.AddTail (new CaBarPieItem(lpszBar, color, bHatch, colorHatch));
}

void CaBarInfoData::AddBarUnit (LPCTSTR lpszBar, double dCapacity)
{
	m_listUnit.AddTail (new CaBarData(lpszBar, dCapacity));
}

void CaBarInfoData::AddOccupant (LPCTSTR lpszDisk, LPCTSTR lpszOccupant, double fPercentage)
{
	CaOccupantData* pOccupant = NULL;
	POSITION pos = m_listUnit.GetHeadPosition();
	while (pos != NULL)
	{
		CaBarData* pBarUnit = m_listUnit.GetNext (pos); 
		if (pBarUnit->m_strBarUnit.CompareNoCase (lpszDisk) == 0)
		{
			POSITION p = m_listBar.GetHeadPosition();
			CaBarPieItem* pData = NULL;
			while (p != NULL)
			{
				pData = (CaBarPieItem*)m_listBar.GetNext (p);
				if (pData->m_strName.CompareNoCase (lpszOccupant) == 0)
				{
					pOccupant = new CaOccupantData (
						lpszOccupant, 
						pData->m_colorRGB, 
						fPercentage, 
						pData->m_bHatch, 
						pData->m_colorHatch);
					pBarUnit->m_listOccupant.AddTail (pOccupant);
					return;
				}
			}
			//
			// You must first add the location 'lpszOccupant' 
			// by calling AddBar().
			ASSERT (FALSE);
		}
	}
	//
	// No Disk that has the name 'lpszDisk'
	ASSERT (FALSE);
}

void CaBarInfoData::DropBar(LPCTSTR lpszBar)
{



}



void CaBarInfoData::DropBarUnit(LPCTSTR lpszBar)
{

}


void CaBarInfoData::DropOccupant(LPCTSTR lpszDisk, LPCTSTR lpszOccupant)
{


}

double CaBarInfoData::GetBarCapacity(LPCTSTR lpszDisk)
{
	POSITION pos = m_listUnit.GetHeadPosition();
	while (pos != NULL)
	{
		CaBarData* pBarUnit = m_listUnit.GetNext (pos);
		if (pBarUnit->m_strBarUnit.CompareNoCase (lpszDisk) == 0)
			return pBarUnit->m_nCapacity;
	}
	//
	// You must specify the disk that you have already added.
	ASSERT (FALSE);
	return 0.0;
}


IMPLEMENT_SERIAL (CaLegendData, CaBarPieItem, 1)
CaLegendData::CaLegendData (LPCTSTR lpszPie, double dPercentage, COLORREF color)
	:CaBarPieItem (lpszPie, color), m_dPercentage(dPercentage)
{
	m_bFocus = FALSE;
	m_dValue = 0.0;
}

CaLegendData::CaLegendData (LPCTSTR lpszLegend, COLORREF color)
	:CaBarPieItem (lpszLegend, color), m_dPercentage(0.0)
{
	m_bFocus = FALSE;
	m_dValue = 0.0;
}

CaLegendData::CaLegendData (const CaLegendData* refLegendData)
	:CaBarPieItem (refLegendData->m_strName, refLegendData->m_colorRGB)
{
	ASSERT (refLegendData);
	m_dPercentage = refLegendData->m_dPercentage;
	m_dValue      = refLegendData->m_dValue;
	m_bFocus      = refLegendData->m_bFocus;
	m_dDegStart   = refLegendData->m_dDegStart;
	m_dDegEnd     = refLegendData->m_dDegEnd;
}

void CaLegendData::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_dPercentage;
		ar << m_dValue;
		ar << m_bFocus;
		ar << m_dDegStart;
		ar << m_dDegEnd;
	}
	else
	{
		ar >> m_dPercentage;
		ar >> m_dValue;
		ar >> m_bFocus;
		ar >> m_dDegStart;
		ar >> m_dDegEnd;
	}
	CaBarPieItem::Serialize (ar);
}

void CaLegendData::SetSector (double degStart, double degEnd, BOOL bAssert)
{
	if (fabs (degStart) <= 0.0001)
		degStart = 0.0;
	if (bAssert)
	{
		if (fabs (degEnd - 360.0) <= 0.0001)
			degEnd = 360.0;
		ASSERT (degStart >= 0.0 && degEnd <= 360.0);
	}
	m_dDegStart = degStart;
	m_dDegEnd	= degEnd;
}


IMPLEMENT_SERIAL (CaPieInfoData, CObject, 1)
CaPieInfoData::CaPieInfoData()
{
	m_dCapacity = 0.0; 
	m_strUnit = _T("");
	m_strSmallUnit = _T("");
	m_strTitle = _T("");
	m_strMainItem   = _T("");
	m_strFormat  = _T("%s = (%0.2f%%, %0.2f%s)");
	m_bError = FALSE;
	m_bDrawOrientation = FALSE;

	m_degStartNextCP = -1.0;
	m_nBlockPrevCP = -1;
	m_nBlockNextCP = 0;
	m_nBlockBOF = 0;
	m_nBlockEOF = 0;
	m_nBlockTotal = 0;
}

CaPieInfoData::CaPieInfoData(double dCapacity, LPCTSTR lpszUnit)
{
	m_dCapacity     = dCapacity;
	m_strUnit       = lpszUnit;
	m_strSmallUnit  = _T("");
	m_strTitle      = _T("");
	m_strMainItem   = _T("");
	m_strFormat  = _T("%s = (%0.2f%%, %0.2f%s)");
	m_bError = FALSE;
	m_bDrawOrientation = FALSE;
	m_degStartNextCP = -1.0;
	m_nBlockPrevCP = -1;
	m_nBlockNextCP = 0;
	m_nBlockBOF = 0;
	m_nBlockEOF = 0;
	m_nBlockTotal = 0;
}

CaPieInfoData::CaPieInfoData(const CaPieInfoData* pRefPieInfoData)
{
	ASSERT (pRefPieInfoData);

	m_dCapacity        = pRefPieInfoData->m_dCapacity;
	m_strUnit          = pRefPieInfoData->m_strUnit;
	m_strSmallUnit     = pRefPieInfoData->m_strSmallUnit;
	m_strTitle         = pRefPieInfoData->m_strTitle;
	m_strMainItem      = pRefPieInfoData->m_strMainItem;
	m_strFormat        = pRefPieInfoData->m_strFormat;
	m_bError           = pRefPieInfoData->m_bError;
	m_bDrawOrientation = pRefPieInfoData->m_bDrawOrientation;
	m_degStartNextCP = pRefPieInfoData->m_degStartNextCP;
	m_nBlockPrevCP = pRefPieInfoData->m_nBlockPrevCP;
	m_nBlockNextCP = pRefPieInfoData->m_nBlockNextCP;
	m_nBlockBOF = pRefPieInfoData->m_nBlockBOF;
	m_nBlockEOF = pRefPieInfoData->m_nBlockEOF;
	m_nBlockTotal = pRefPieInfoData->m_nBlockTotal;

	// For lists: need to allocate objects along with lists dup
	POSITION pos = pRefPieInfoData->m_listPie.GetHeadPosition ();
	while (pos != NULL) {
		CaLegendData* pLegend = pRefPieInfoData->m_listPie.GetNext (pos);
		ASSERT (pLegend);
		CaLegendData* pNewLegend = new CaLegendData(pLegend);
		m_listPie.AddTail(pNewLegend);
	}
	pos = pRefPieInfoData->m_listLegend.GetHeadPosition ();
	while (pos != NULL) {
		CaLegendData* pLegend = pRefPieInfoData->m_listLegend.GetNext (pos);
		ASSERT (pLegend);
		CaLegendData* pNewLegend = new CaLegendData(pLegend);
		m_listLegend.AddTail(pNewLegend);
	}

}

CaPieInfoData::~CaPieInfoData()
{
	Cleanup();
}

void CaPieInfoData::Cleanup()
{
	while (!m_listPie.IsEmpty())
	{
		delete m_listPie.RemoveHead();
	}
	while (!m_listLegend.IsEmpty())
	{
		delete m_listLegend.RemoveHead();
	}
}

void CaPieInfoData::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_dCapacity;
		ar << m_strUnit;
		ar << m_strTitle;
		ar << m_strMainItem;
		ar << m_bError;
		ar << m_strUnit;
		//
		// Log File specific:
		ar << m_bDrawOrientation;
		ar << m_degStartNextCP;
		ar << m_nBlockNextCP;
		ar << m_nBlockPrevCP;
		ar << m_nBlockBOF;
		ar << m_nBlockEOF;
		ar << m_nBlockTotal;
	}
	else
	{
		ar >> m_dCapacity;
		ar >> m_strUnit;
		ar >> m_strTitle;
		ar >> m_strMainItem;
		ar >> m_bError;
		ar >> m_strUnit;
		//
		// Log File specific:
		ar >> m_bDrawOrientation;
		ar >> m_degStartNextCP;
		ar >> m_nBlockNextCP;
		ar >> m_nBlockPrevCP;
		ar >> m_nBlockBOF;
		ar >> m_nBlockEOF;
		ar >> m_nBlockTotal;
	}
	m_listPie.Serialize (ar);
	m_listLegend.Serialize (ar);
}

void CaPieInfoData::SetCapacity(double dCapacity, LPCTSTR lpszUnit)
{
	m_dCapacity = dCapacity;
	m_strUnit	= lpszUnit;
}


void CaPieInfoData::AddPie (LPCTSTR lpszPie, double dPercentage)
{
	//
	// You must set the total capacity.
	ASSERT (m_dCapacity > 0.0);
	int r, g, b;
	r = lRandom() % 255;
	g = lRandom() % 255;
	b = lRandom() % 255;
	double start = 0.0;
	CaLegendData* pData = new CaLegendData(lpszPie, dPercentage, RGB (r, g, b));
	POSITION pos = m_listPie.GetTailPosition ();
	if (pos != NULL)
	{
		CaLegendData* pLegend = m_listPie.GetPrev (pos);
		start = pLegend->m_dDegEnd;
	}
	pData->SetSector (start, start + dPercentage * 3.6);
	m_listPie.AddTail (pData);
	//
	// Add Legend.
	AddLegend (lpszPie, RGB (r, g, b));
	pos = m_listLegend.GetTailPosition();
	pData = m_listLegend.GetAt (pos);
	if (pData)
		pData->m_dPercentage = dPercentage;
}


void CaPieInfoData::AddPie (LPCTSTR lpszPie, double dPercentage, COLORREF color)
{
	//
	// You must set the total capacity.
	ASSERT (m_dCapacity > 0.0);
	double start = 0.0;
	CaLegendData* pData = new CaLegendData(lpszPie, dPercentage, color);
	POSITION pos = m_listPie.GetTailPosition ();
	if (pos != NULL)
	{
		CaLegendData* pLegend = m_listPie.GetPrev (pos);
		start = pLegend->m_dDegEnd;
	}
	pData->SetSector (start, start + dPercentage * 3.6);
	m_listPie.AddTail (pData);
	//
	// Add Legend.
	AddLegend (lpszPie, color);
	pos = m_listLegend.GetTailPosition();
	pData = m_listLegend.GetAt (pos);
	if (pData)
		pData->m_dPercentage = dPercentage;
}

void CaPieInfoData::AddPie (LPCTSTR lpszPie, double dPercentage, COLORREF color, double dDegStrat)
{
	//
	// You must set the total capacity.
	ASSERT (m_dCapacity > 0.0);
	double start = dDegStrat;
	CaLegendData* pData = new CaLegendData(lpszPie, dPercentage, color);

	POSITION pos = m_listPie.GetTailPosition ();
	if (pos != NULL)
	{
		CaLegendData* pLegend = m_listPie.GetPrev (pos);
		start = pLegend->m_dDegEnd;
	}
	pData->SetSector (start, start + dPercentage * 3.6, FALSE);
	m_listPie.AddTail (pData);
	//
	// Add Legend.
	AddLegend (lpszPie, color);
	pos = m_listLegend.GetTailPosition();
	pData = m_listLegend.GetAt (pos);
	if (pData)
		pData->m_dPercentage = dPercentage;
}



void CaPieInfoData::AddPie2 (LPCTSTR lpszPie, double dPercentage, double dValue)
{
	//
	// You must set the total capacity.
	ASSERT (m_dCapacity > 0.0);
	int r, g, b;
	r = lRandom() % 255;
	g = lRandom() % 255;
	b = lRandom() % 255;
	double start = 0.0;
	CaLegendData* pData = new CaLegendData(lpszPie, dPercentage, RGB (r, g, b));
	POSITION pos = m_listPie.GetTailPosition ();
	if (pos != NULL)
	{
		CaLegendData* pLegend = m_listPie.GetPrev (pos);
		start = pLegend->m_dDegEnd;
	}
	pData->SetSector (start, start + dPercentage * 3.6);
	m_listPie.AddTail (pData);
	//
	// Add Legend.
	AddLegend (lpszPie, RGB (r, g, b));
	pos = m_listLegend.GetTailPosition();
	pData = m_listLegend.GetAt (pos);
	if (pData)
	{
		pData->m_dPercentage = dPercentage;
		pData->m_dValue = dValue;
	}
}


void CaPieInfoData::AddPie2 (LPCTSTR lpszPie, double dPercentage, COLORREF color, double dValue)
{
	//
	// You must set the total capacity.
	ASSERT (m_dCapacity > 0.0);
	double start = 0.0;
	CaLegendData* pData = new CaLegendData(lpszPie, dPercentage, color);
	POSITION pos = m_listPie.GetTailPosition ();
	if (pos != NULL)
	{
		CaLegendData* pLegend = m_listPie.GetPrev (pos);
		start = pLegend->m_dDegEnd;
	}
	pData->SetSector (start, start + dPercentage * 3.6);
	m_listPie.AddTail (pData);
	//
	// Add Legend
	AddLegend (lpszPie, color);
	pos = m_listLegend.GetTailPosition();
	pData = m_listLegend.GetAt (pos);
	if (pData)
	{
		pData->m_dPercentage = dPercentage;
		pData->m_dValue = dValue;
	}
}



void CaPieInfoData::AddLegend (LPCTSTR lpszLegend, COLORREF color, double dPercentage, double dValue)
{
	CaLegendData* pData = NULL;
	POSITION pos = m_listLegend.GetHeadPosition();
	while (pos != NULL)
	{
		pData = (CaLegendData*)m_listLegend.GetNext (pos);
		if (pData->m_strName.CompareNoCase (lpszLegend) == 0)
			return;
	}
	pData = new CaLegendData(lpszLegend, color);
	if (pData)
	{
		pData->m_dPercentage = dPercentage;
		pData->m_dValue = dValue;
		m_listLegend.AddTail (pData);
	}
}

void CaPieInfoData::AddLegend (LPCTSTR lpszLegend, double dPercentage, double dValue)
{
	int r, g, b;
	CaLegendData* pData = NULL;
	POSITION pos = m_listLegend.GetHeadPosition();
	while (pos != NULL)
	{
		pData = (CaLegendData*)m_listLegend.GetNext (pos);
		if (pData->m_strName.CompareNoCase (lpszLegend) == 0)
			return;
	}
	r = lRandom() % 255;
	g = lRandom() % 255;
	b = lRandom() % 255;
	pData = new CaLegendData(lpszLegend, RGB (r, g, b));
	if (pData)
	{
		pData->m_dPercentage = dPercentage;
		pData->m_dValue = dValue;
		m_listLegend.AddTail (pData);
	}
}

