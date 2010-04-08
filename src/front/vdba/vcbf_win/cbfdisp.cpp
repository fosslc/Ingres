/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : cbfdisp.cpp, Implementation file.
**    Project  : OpenIngres Configuration Manager (Origin IPM Project)
**    Author   : UK Sotheavut
**    Purpose  : CuPageInformation, maintains the information for displaying 
**               the property of CBF.
**               It contain the number of pages, the Dialog ID, the Label for the
**               TabCtrl, .., for each item data of CBF Item
**               CuCbfProperty is maintained for keeping track of the object being
**               displayed in the TabCtrl, its current page,
**
** History:
**
** xx-Feb-1997 (uk$so01)
**    created
** 06-Jun-2000 (uk$so01)
**    bug 99242 Handle DBCS
** 16-Oct-2001 (uk$so01)
**    BUG #106053, free the unused memory
**/

#include "stdafx.h"
#include "cbfdisp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// CuPageInformation Implementation
// ********************************
IMPLEMENT_SERIAL (CuPageInformation, CObject, 1)
CuPageInformation::CuPageInformation()
{
	m_strTitle     = _T("");
	m_strClassName = _T(""); 
	m_nNumber      = 0;  
	m_pTabID       = NULL;         
	m_pDlgID       = NULL;      
	m_pCbfItem     = NULL;
	m_nDefaultPage = 0;
}
   
CuPageInformation::CuPageInformation(CString strClass, int nSize, UINT* tabArray, UINT* dlgArray, UINT nIDTitle)
{
	m_strTitle     = _T("");
	m_strClassName = strClass; 
	m_nNumber      = nSize;  
	m_pTabID       = NULL;         
	m_pDlgID       = NULL;  
	m_pCbfItem     = NULL;
	m_nIDTitle     = nIDTitle;
	if (m_nNumber > 0)
	{
		m_pTabID = new UINT [m_nNumber];
		m_pDlgID = new UINT [m_nNumber];
		for (int i=0; i<m_nNumber; i++)
		{
			m_pTabID [i] = tabArray [i];
			m_pDlgID [i] = dlgArray [i];
		}
	}
	m_nDefaultPage = 0;
}


CuPageInformation::CuPageInformation(CString strClass)
{
	m_strTitle     = _T("");
	m_strClassName = strClass; 
	m_nNumber      = 0;  
	m_pTabID       = NULL;         
	m_pDlgID       = NULL;  
	m_nIDTitle     = 0;
}
    
CuPageInformation::~CuPageInformation()
{
	if (m_pTabID)
		delete m_pTabID;
	if (m_pDlgID)
		delete m_pDlgID;
}



CString CuPageInformation::GetClassName()   {return m_strClassName;}
int     CuPageInformation::GetNumberOfPage(){return m_nNumber;}
UINT*   CuPageInformation::GetTabID ()      {return m_pTabID;}
UINT*   CuPageInformation::GetDlgID ()      {return m_pDlgID;}
UINT    CuPageInformation::GetTabID (int nIndex)
{
	ASSERT(nIndex>=0 && nIndex <m_nNumber); 
	return m_pTabID[nIndex];
}

UINT    CuPageInformation::GetDlgID (int nIndex)
{
	ASSERT(nIndex>=0 && nIndex <m_nNumber); 
	return m_pDlgID[nIndex];
}

void CuPageInformation::GetTitle (CString& strTile)
{
	if (m_nIDTitle == 0)
		strTile = _T("");
	else
		strTile = m_strTitle;
}



void CuPageInformation::Serialize (CArchive& ar)
{

}


//
// CuCbfProperty Implementation
// ****************************
IMPLEMENT_SERIAL (CuCbfProperty, CObject, 1)
CuCbfProperty::CuCbfProperty()
{
	m_nCurrentSelected = -1;
	m_pPageInfo = NULL;
	m_pPropertyData = NULL;
}

CuCbfProperty::CuCbfProperty(int nCurSel, CuPageInformation*  pPageInfo)
{
	m_nCurrentSelected = nCurSel;
	m_pPageInfo = pPageInfo;
	m_pPropertyData = NULL;
}

CuCbfProperty::~CuCbfProperty()
{
    if (m_pPropertyData) delete m_pPropertyData;
}

void CuCbfProperty::SetCurSel  (int nSel) {m_nCurrentSelected = nSel;}
void CuCbfProperty::SetPageInfo(CuPageInformation*  pPageInfo){m_pPageInfo = pPageInfo;}
int  CuCbfProperty::GetCurSel (){return m_nCurrentSelected;}
CuPageInformation* CuCbfProperty::GetPageInfo(){return m_pPageInfo;}
void CuCbfProperty::SetPropertyData (CuCbfPropertyData* pData)
{
	if (m_pPropertyData) delete m_pPropertyData;
	m_pPropertyData = pData;
}

void CuCbfProperty::Serialize (CArchive& ar)
{
}
