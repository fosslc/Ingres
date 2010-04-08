/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : cbfdisp.h, Header file.                                               //
//                                                                                     //
//                                                                                     //
//    Project  : OpenIngres Configuration Manager (Origin IPM Project)                 //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : CuPageInformation, maintains the information for displaying           //
//               the property of CBF.                                                  //
//               It contain the number of pages, the Dialog ID, the Label for the      //
//               TabCtrl, .., for each item data of CBF Item                           //
//               CuCbfProperty is maintained for keeping track of the object being     //
//               displayed in the TabCtrl, its current page, ...                       //
/***************************************************************************************/

#ifndef CBFDISPLAY_HEADER
#define CBFDISPLAY_HEADER
#include "cbfprdta.h"

class CuCbfListViewItem;
class CuPageInformation: public CObject
{
	DECLARE_SERIAL (CuPageInformation)
public:
	CuPageInformation();
	CuPageInformation(CString strClass, int nSize, UINT* tabArray,UINT* dlgArray, UINT nIDTitle = 0);
	CuPageInformation(CString strClass);
	~CuPageInformation();
	
	CString GetClassName();
	int     GetNumberOfPage();
	UINT    GetTabID (int nIndex);
	UINT    GetDlgID (int nIndex);
	UINT*   GetTabID ();
	UINT*   GetDlgID ();
	
	void    SetDefaultPage (UINT iPage = 0){m_nDefaultPage = iPage;}
	UINT    GetDefaultPage (){return m_nDefaultPage;}
	void    SetIDTitle (UINT nIDTitle){m_nIDTitle = nIDTitle;};
	UINT    GetIDTitle (){return m_nIDTitle;};
	void    GetTitle   (CString& strTile);
	void    SetCbfItem (CuCbfListViewItem* pItem) {m_pCbfItem = pItem;};
	CuCbfListViewItem* GetCbfItem (){return m_pCbfItem;};
	virtual void Serialize (CArchive& ar);

private:
	int     m_nDefaultPage;         // The default page to be display.
	CString m_strTitle;             // The title.
	CString m_strClassName;         // The name of the definition class.
	int     m_nNumber;              // The number of Pages (Pages of CTabCtrl)
	UINT*   m_pTabID;               // Array of String ID for the label of Tab.
	UINT*   m_pDlgID;               // Array of Dialog (Page) ID.
	UINT    m_nIDTitle;             // The String ID for the title;
	CuCbfListViewItem*  m_pCbfItem; 
};


class CuCbfProperty: public CObject
{
	DECLARE_SERIAL (CuCbfProperty)
public:
	CuCbfProperty();
	CuCbfProperty(int nCurSel, CuPageInformation*  pPageInfo);
	~CuCbfProperty();
	
	void SetCurSel   (int nSel);
	void SetPageInfo (CuPageInformation*  pPageInfo);
	int  GetCurSel ();
	CuPageInformation* GetPageInfo();
	CuCbfPropertyData* GetPropertyData(){return m_pPropertyData;}
	void SetPropertyData (CuCbfPropertyData* pData);
	virtual void Serialize (CArchive& ar);
private:
	int     m_nCurrentSelected;         // Current selection of TabCtrl, or -1 if none
	CuPageInformation*  m_pPageInfo;    // Current object displayed in the TabCtrl.
	CuCbfPropertyData*  m_pPropertyData;// Data of the Current page.    
};

#endif

