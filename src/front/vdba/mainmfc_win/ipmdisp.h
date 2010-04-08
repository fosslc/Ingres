/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : IpmDisp.h, Header file.                                               //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : CuPageInformation, maintains the information for displaying           //
//               the property of IPM.                                                  //
//               It contain the number of pages, the Dialog ID, the Label for the      //
//               TabCtrl, .., for each item data of IPM Tree.                          //
//               CuIpmProperty is maintained for keeping track of the object being     //
//               displayed in the TabCtrl, its current page, ...                       //
/***************************************************************************************/

#ifndef IPMDISPLAY_HEADER
#define IPMDISPLAY_HEADER
#include "ipmprdta.h"
#include "ipmicdta.h"

extern "C"
{
#include "monitor.h"
}

class CTreeItem;
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

    void    SetIDTitle (UINT nIDTitle){m_nIDTitle = nIDTitle;};
    UINT    GetIDTitle (){return m_nIDTitle;};
    void    GetTitle   (CString& strTile);
    void    SetTitle   (LPCTSTR  strItem, CTreeItem* pItem, int hNode = -1);
    void    SetUpdateParam (LPIPMUPDATEPARAMS ups);
    LPIPMUPDATEPARAMS GetUpdateParam (){return m_Ups;};
    virtual void Serialize (CArchive& ar);

    int      GetImageType(){return nImageType;};
    void     SetImage (UINT  nBitmapID);
    void     SetImageMask(COLORREF crMask){m_crMask = crMask;} // For the bitmap image
    void     SetImage (HICON hIcon);
    HICON    GetIconImage(){return m_hIcon;}
    UINT     GetBitmapImage(){return m_nBitmap;}
    COLORREF GetImageMask(){return m_crMask;} // For the bitmap image

protected:
    CString  m_strTitle;             // The title.(Text)
    int      nImageType;             // -1: non, 0: icon, 1: bitmap's ID.
    HICON    m_hIcon;                // Image on the left of the Text
    UINT     m_nBitmap;              // Image on the left of the Text
    COLORREF m_crMask;               // Mask for the Bimap image
private:
    CString m_strClassName;         // The name of the definition class.
    int     m_nNumber;              // The number of Pages (Pages of CTabCtrl)
    UINT*   m_pTabID;               // Array of String ID for the label of Tab.
    UINT*   m_pDlgID;               // Array of Dialog (Page) ID.
    UINT    m_nIDTitle;             // The String ID for the title;
    LPIPMUPDATEPARAMS   m_Ups;      // The pointor to the tructure needed by MonGetFirst and MonGetDetailInfo.
};


class CuIpmProperty: public CObject
{
    DECLARE_SERIAL (CuIpmProperty)
public:
    CuIpmProperty();
    CuIpmProperty(int nCurSel, CuPageInformation*  pPageInfo);
    ~CuIpmProperty();

    void SetCurSel   (int nSel);
    void SetPageInfo (CuPageInformation*  pPageInfo);
    int  GetCurSel ();
    CuPageInformation* GetPageInfo();
    CuIpmPropertyData* GetPropertyData(){return m_pPropertyData;}
    void SetPropertyData (CuIpmPropertyData* pData);
    virtual void Serialize (CArchive& ar);
private:
    int     m_nCurrentSelected;         // Current selection of TabCtrl, or -1 if none
    CuPageInformation*  m_pPageInfo;    // Current object displayed in the TabCtrl.
    CuIpmPropertyData*  m_pPropertyData;// Data of the Current page.    
};

#endif

