/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : ivmdisp.h, Header file.                                               //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II / Visual Manager                                            //
//    Author   : Sotheavut UK (uk$so01)                                                //
//                                                                                     //
//                                                                                     //
//    Purpose  : CaPageInformation, maintains the information for displaying           //
//               the property of Ingres Visual Manager.                                //
//               It contain the number of pages, the Dialog ID, the Label for the      //
//               TabCtrl, .., for each item data of IVM Item                           //
//               CaCbfProperty is maintained for keeping track of the object being     //
//               displayed in the TabCtrl, its current page, ...                       //
/***************************************************************************************/
/* History:
* --------
* uk$so01: (15-Dec-1998 created)
*
*
*/

#if !defined (IVMDISPLAY_HEADER)
#define IVMDISPLAY_HEADER
#include "compdata.h"

class CaPageInformation: public CObject
{
public:
	CaPageInformation();
	CaPageInformation(CString strClass, int nSize, UINT* tabArray,UINT* dlgArray, UINT nIDTitle = 0);
	CaPageInformation(CString strClass);
	~CaPageInformation();
	
	CString GetClassName();
	int   GetNumberOfPage();
	UINT  GetTabID (int nIndex);
	UINT  GetDlgID (int nIndex);
	UINT* GetTabID ();
	UINT* GetDlgID ();
	
	void  SetDefaultPage (UINT iPage = 0){m_nDefaultPage = iPage;}
	UINT  GetDefaultPage (){return m_nDefaultPage;}
	void  SetIDTitle (UINT nIDTitle){m_nIDTitle = nIDTitle;};
	UINT  GetIDTitle (){return m_nIDTitle;};
	void  GetTitle   (CString& strTile);
	void  SetIvmItem (CaTreeComponentItemData* pItem) {m_pIvmItem = pItem;};
	CaTreeComponentItemData* GetIvmItem (){return m_pIvmItem;};

	CaEventFilter& GetEventFilter() {return m_eventFilter;}

private:
	int     m_nDefaultPage;         // The default page to be display.
	CString m_strTitle;             // The title.
	CString m_strClassName;         // The name of the definition class.
	int     m_nNumber;              // The number of Pages (Pages of CTabCtrl)
	UINT*   m_pTabID;               // Array of String ID for the label of Tab.
	UINT*   m_pDlgID;               // Array of Dialog (Page) ID.
	UINT    m_nIDTitle;             // The String ID for the title;
	CaTreeComponentItemData*  m_pIvmItem; 

	CaEventFilter m_eventFilter;
};


class CaIvmProperty: public CObject
{
public:
	CaIvmProperty();
	CaIvmProperty(int nCurSel, CaPageInformation*  pPageInfo);
	~CaIvmProperty();
	
	void SetCurSel   (int nSel);
	void SetPageInfo (CaPageInformation*  pPageInfo);
	int  GetCurSel ();
	CaPageInformation* GetPageInfo();
private:
	int     m_nCurrentSelected;         // Current selection of TabCtrl, or -1 if none
	CaPageInformation*  m_pPageInfo;    // Current object displayed in the TabCtrl.
};

#endif

