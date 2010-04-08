/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**  Name: tree_dll.cpp
**
**  Description:
**	replaces CA-Custom tree control with mfc 4.0 CTreeCtrl
**
**  History:
**	28-nov-1996 (noifr01)
**	    Added keydown management to mask the ctrl+pgdown and ctrl+pgu
**	    standard behaviour, since RPB had defined custom behaviour in
**	    vdba for those key combinations.
**	02-jun-1997 (noifr01)
**	    Added syscolorchange management Emb.
**	18-feb-2000 (cucjo01)
**	    Properly typecasted LRCreateWindow(). Also, added
**	    AFX_MANAGE_STATE() to LRCreateWindow() to prevent error from
**	    creating an empty document.
**  27-Sep-2000 (schph01)
**      bug #99242 . cleanup for DBCS compliance
** 10-Mar-2004 (schph01)
**    (BUG #111014), defined WINVER for fixing various problems when building
**    with recent version(s) of compiler
** 01-Dec-2004 (uk$so01)
**    VDBA BUG #113548 / ISSUE #13768610 
**    Fix also the assertion in debug mode while fixing the bug above.
** 30-Jan-2007 (drivi01)
**    Make pTree be initialized to NULL in TamponOnNotify function
**    b/c bad values in pTree can cause SEGV when pTree->CanNotify()
**    is being called. pTree needs to be checked for NULL value before
**    pTree->CanNotify() is attempted to be accessed.
**  21-Aug-2008 (whiro01)
**    Undefine _DEBUG around MFC headers so we can always link to Release C runtime.
** 07-May-2009 (drivi01)
**    Define WINVER in effort to port to Visual Studio 2008.
*/

#define WINVER 0x0500

// In order to link DEBUG code with the Release C runtime, we must make MFC
// believe we are in Release mode
#ifdef _DEBUG
#define _REMEMBER_DEBUG_VERSION
#undef _DEBUG
#endif

#include <afxwin.h>
#include <afxcmn.h>
#include <afxtempl.h>

#include <windowsx.h>       // for message cracker
#include "treelb.e"         // replaces traceapi.h
#include "resource.h"       // for standardized icons

#ifdef _REMEMBER_DEBUG_VERSION
#define _DEBUG
#endif

// custom 32 bits macros - assume WORD on 16 bits (0xFFFF)
#define LONG2POINT(l,pt)\
            ((pt).x=(SHORT)LOWORD(l),(pt).y=(SHORT)HIWORD(l))
#define POINT2LONG(x, y)\
            ( ((WORD)(x) & 0xFFFF) | ((((WORD)(y)) << 16) & 0xFFFF0000) )

// dimensions of image
#define IMAGE_WIDTH  16
#define IMAGE_HEIGHT 16
//#define IMAGE_WIDTH  12
//#define IMAGE_HEIGHT 12
//#define IMAGE_WIDTH  32
//#define IMAGE_HEIGHT 32


// MUST be same as #define TREE_CLASS in winmain\winutils.c
// otherwise, funny behaviour when changing font...
static char szTamponClassName[] = "WLL_CN_WIN";

static BOOL     TamponOnExpand       (HWND hwnd, DWORD identifier);
static BOOL     TamponOnCollapse     (HWND hwnd, DWORD identifier);
static BOOL     TamponOnDelete       (HWND hwnd, DWORD identifier);
static BOOL     TamponOnDeleteAll    (HWND hwnd);
static DWORD    TamponOnGetItemData  (HWND hwnd, DWORD identifier);
static DWORD    TamponOnGetParent    (HWND hwnd, DWORD identifier);
static DWORD    TamponOnGetCurSel    (HWND hwnd);
static BOOL     TamponOnSetSel       (HWND hwnd, DWORD identifier);
static BOOL     TamponOnSetString    (HWND hwnd, LPSETSTRING lpst);
static LPCTSTR  TamponOnGetItemText  (HWND hwnd, DWORD identifier);
static DWORD    TamponOnGetItemLevel (HWND hwnd, DWORD identifier);

static BOOL     TamponOnGetItemState (HWND hwnd, DWORD identifier); // TRUE:Expanded
static BOOL     TamponOnSetTopItem   (HWND hwnd, DWORD identifier); 
static BOOL     TamponOnAddBefore    (HWND hwnd, LPLINK_RECORD lpRec); 
static DWORD    TamponOnGetFirstChild(HWND hwnd, DWORD identifier); 
static DWORD    TamponOnGetFirst     (HWND hwnd); 
static DWORD    TamponOnGetNext      (HWND hwnd, DWORD identifier); 
static DWORD    TamponOnGetPrev      (HWND hwnd, DWORD identifier); 
static DWORD    TamponOnGetTopItem   (HWND hwnd); 
static DWORD    TamponOnGetCount     (HWND hwnd);
static DWORD    TamponOnGetMouseItem (HWND hwnd, POINTS ptShort);

static DWORD    TamponOnSetFont      (HWND hwnd, HFONT hFont); 
static BOOL     TamponOnCollapseOne  (HWND hwnd, DWORD identifier);
static BOOL     TamponOnCollapseAll  (HWND hwnd);
static BOOL     TamponOnPrintGetFirst(HWND hwnd, BOOL b, LPLMDRAWITEMSTRUCT lpDraw); 
static BOOL     TamponOnPrintGetNext (HWND hwnd, BOOL b, LPLMDRAWITEMSTRUCT lpDraw); 
static DWORD    TamponOnGetVisState  (HWND hwnd, DWORD identifier);
static DWORD    TamponOnGetNextSibling  (HWND hwnd, DWORD identifier);
static DWORD    TamponOnGetPrevSibling  (HWND hwnd, DWORD identifier);
static HWND     TamponOnGetTreeViewHwnd (HWND hwnd);

// for print with invisible jeff's tree
static void     LBTreePopulate       (HWND hlbtree, HWND hwnd /*TamponWindow*/);

class CTreeDll : public CWinApp
{
public:
  virtual BOOL InitInstance(); // Initialization
  virtual int ExitInstance();  // Termination (WEP-like code)

  // nothing special for the constructor
  CTreeDll(LPCTSTR pszAppName) : CWinApp(pszAppName) { }

  CImageList m_ImageList;
};

extern CTreeDll treeDll; 
// Obsolete since optimization time
class CAssociate: public CObject
{
public:
  CAssociate (const CAssociate&);
  CAssociate ():CObject(){};
  CAssociate (DWORD dwId, HTREEITEM hItem):CObject(), id(dwId), htreeitem(hItem) {};

  DWORD id;
  HTREEITEM htreeitem;
  ~CAssociate (){};

  CAssociate& operator= (const CAssociate& aS);
};




class DbaTree: public CTreeCtrl
{
protected:
  // for speed enhancement: don't use CAssociate class, but a scalar instead
  // An operation (check system box) that took 10 seconds with CAssociate
  // takes 8 seconds with a scalar.
  // make use of conversion between HTREEITEM and DWORD (since HTREEITEM is a pointer)
  // because HTREEITEM cannot be used directly on right side
  // an ASSERT statement in LRCreateWindow checks that no loss of data occurs
  // old map: CMap< DWORD, DWORD, CAssociate, CAssociate& > m_DWORDxHTREEITEM;
  CMap< DWORD, DWORD, DWORD, DWORD > m_DWORDxHTREEITEM;
  CImageList iconList;
  HTREEITEM  curhItem;      // for getnext/getprev assuming "from current"
  HTREEITEM  hPrintedItem;  // for print_getnext assuming "from last printed"
  BOOL       m_bDragging;
  CMap< HICON, HICON, int, int > m_aIconHandle;
  BOOL       bNoNotif;      // to stop notifications
  virtual void PostNcDestroy();

protected:
  // Speed optimization for hItem - dwId 
  // 1) for HTREEITEM GetHTreeItem(DWORD dwId)
  DWORD     GetHTreeItem_LastDwId;        // received parameter
  HTREEITEM GetHTreeItem_LastHtreeItem;   // returned value

  // 2) for DWORD GetItemIdentifier(HTREEITEM hItem)
  HTREEITEM GetItemIdentifier_LastHtreeItem;  // received parameter
  DWORD     GetItemIdentifier_LastDwId;       // returned value

  // 3) second Map with item handle as the key (see m_DWORDxHTREEITEM)
  // make use of conversion between HTREEITEM and DWORD (since HTREEITEM is a pointer)
  // because HTREEITEM cannot be used directly on left side
  //CMap < HTREEITEM, HTREEITEM &, DWORD, DWORD > m_HandleToDwIndex;
  CMap < DWORD, DWORD, DWORD, DWORD > m_HandleToDwIndex;

  // Special to ensure that Right Click notification is sent even though mouse has been moved
  // before RButtonDown and RButtonUp
  BOOL m_bRClickNotified;

public:
  DbaTree();
  ~DbaTree();

  BOOL AssociateIconList();
  int  AddIcon(HICON hIcon);
  int  AddIcon2(HICON hIcon);

  HTREEITEM GetHTreeItem    (DWORD dwId);
  DWORD     GetItemIdentifier (HTREEITEM hItem);
  void SetAssociate (DWORD identifier, HTREEITEM hItem);
  void DelAssociate (DWORD identifier, HTREEITEM hItem);
  void DelAssociate ();

  HTREEITEM GetCurhItem() { return curhItem; };
  void      SetCurhItem(HTREEITEM hItem) { curhItem = hItem; };

  HTREEITEM GetPrintedItem() { return hPrintedItem; };
  void      SetPrintedItem(HTREEITEM hItem) { hPrintedItem = hItem; };

  void BeginDrag ()       {m_bDragging = TRUE; };
  void ResetDrag ()       {m_bDragging = FALSE;};
  BOOL MouseMoveDrag ()   {return m_bDragging ? TRUE: FALSE;};
  BOOL LButtonUpDrag ()   {return m_bDragging ? TRUE: FALSE;};

  void StopNotifications()  {bNoNotif = TRUE; };
  BOOL CanNotify()          {return bNoNotif ? FALSE: TRUE; };

  // Special to ensure that Right Click notification is sent even though mouse has been moved
  // before RButtonDown and RButtonUp
  void SetRClickNotifiedFlag() { ASSERT (!m_bRClickNotified); m_bRClickNotified = TRUE; }

public:
  HWND      m_hTreePrint;
  HWND      m_hOwner;
  RECT      m_rcLocation;
  DWORD     m_dwStyle;
  BOOL      m_bVODrawStyle;

  //
  // hand-made message map (cannot use wizard)
  //
protected:
  afx_msg void OnLButtonUp( UINT nFlags, CPoint point);
  afx_msg void OnMouseMove( UINT nFlags, CPoint point);
  afx_msg void OnLButtonDown (UINT nFlags, CPoint point);
  afx_msg void OnRButtonDown (UINT nFlags, CPoint point);
  afx_msg void OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags);
  afx_msg void OnSysColorChange();
  DECLARE_MESSAGE_MAP()
};

//
// hand-made message map for DbaTree class (cannot use wizard)
//
BEGIN_MESSAGE_MAP(DbaTree, CTreeCtrl)
  ON_WM_MOUSEMOVE()
  ON_WM_LBUTTONUP()
  ON_WM_LBUTTONDOWN()
  ON_WM_RBUTTONDOWN()
  ON_WM_KEYDOWN()
  ON_WM_SYSCOLORCHANGE()
  ON_WM_DESTROY()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Message handlers for Tampon Window Proc

static void TamponOnDestroy(HWND hwnd)
{
  // Extracts the CTreeCtrl * of it's parent window
  // CAST MANDATORY FOR C4.0
  DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);
  if (pTree)
  {
      if (pTree->m_hTreePrint) 
          DestroyWindow (pTree->m_hTreePrint);
//      delete pTree;
  }
}

static void TamponOnSize(HWND hwnd, UINT state, int cx, int cy)
{
  if (state != SIZE_MINIMIZED) {
    DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);
    if (pTree)
      pTree->MoveWindow(0, 0, cx, cy, TRUE);
  }
}

static void TamponOnSetFocus(HWND hwnd, HWND hwndOldFocus)
{
  DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);
  if (pTree)
    pTree->SetFocus();
}

static LRESULT TamponOnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh)
{
  DWORD     vdbaRecId;  // record id from the vdba point of view
  HTREEITEM hItem;      // tree item handle
  LRESULT   lResult;
  DbaTree * pTree = NULL;
  
  pTree = (DbaTree *)GetWindowLong(hwnd, 0);
  // no notifications to parent in termination phase
  if (pTree != NULL && !pTree->CanNotify())
    return 0L;

  switch(pnmh->code) {
    case TVN_ITEMEXPANDING:
      {
        // received parameter is a NM_TREEVIEW FAR *
        NM_TREEVIEW FAR * pnmtv = (NM_TREEVIEW FAR *)pnmh;

        // check state is not expanded state yet
        if (!(pnmtv->itemNew.state & TVIS_EXPANDED)) {
          // get vdba item identifier
          hItem = pnmtv->itemNew.hItem;
          vdbaRecId = pTree->GetItemIdentifier(hItem);

          // notify parent with vdba item identifier
          ::SendMessage(GetParent(hwnd), LM_NOTIFY_EXPAND, (WPARAM)hwnd, (LPARAM)vdbaRecId);
        }
      }
      break;

    case TVN_ITEMEXPANDED:
	{
		NM_TREEVIEW FAR * pnmtv = (NM_TREEVIEW FAR *)pnmh;
		int iconIndex;
		HICON hIconStd;
		BOOL bAdded = FALSE;
		LMQUERYICONS lmIcons;
		hItem = pnmtv->itemNew.hItem;
		lmIcons.hIcon1 = lmIcons.hIcon2 = NULL;
		lmIcons.ulID = pTree->GetItemIdentifier(hItem);
		lResult = ::SendMessage(GetParent(hwnd), LM_QUERYICONS, (WPARAM)hwnd, (LPARAM)&lmIcons);
		if (lmIcons.hIcon1)
			break; // Personal icon.
		if (!pTree->ItemHasChildren (hItem))
			break;

		UINT state = pTree->GetItemState (hItem, TVIS_EXPANDED);

#if defined (EDBC)
		if (state & TVIS_EXPANDED)
			hIconStd = treeDll.m_ImageList.ExtractIcon (1); // Opened Folder
		else
			hIconStd = treeDll.m_ImageList.ExtractIcon (0); // Closed Folder
		bAdded = TRUE;
		iconIndex = pTree->AddIcon2(hIconStd);
#else
		//if (state & TVIS_EXPANDED)
		//	hIconStd = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICON_OPENED));
		//else
			hIconStd = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICON_CLOSED));
#endif
		if (!bAdded)
			iconIndex = pTree->AddIcon(hIconStd);
		pTree->SetItemImage (hItem, iconIndex, iconIndex);
        // notify parent with vdba item identifier
		// Only notify collapse:
		::SendMessage(GetParent(hwnd), LM_NOTIFY_COLLAPSE, (WPARAM)hwnd, (LPARAM)(lmIcons.ulID));
      }
      break;


    case TVN_SELCHANGED:
      {
        // received parameter is a NM_TREEVIEW FAR *
        NM_TREEVIEW FAR * pnmtv = (NM_TREEVIEW FAR *)pnmh;

        // get vdba item identifier
        hItem = pnmtv->itemNew.hItem;
        vdbaRecId = pTree->GetItemIdentifier(hItem);

        // notify parent with vdba item identifier
        ::SendMessage(GetParent(hwnd), LM_NOTIFY_SELCHANGE, (WPARAM)hwnd, (LPARAM)vdbaRecId);

      }
      break;

    case TVN_GETDISPINFO:
      {
        // received parameter is a NM_TREEVIEW FAR *
        TV_DISPINFO FAR * ptvdi = (TV_DISPINFO FAR *)pnmh;
        BOOL bRequestImage;
        BOOL bRequestSelectedImage;

        bRequestImage = (BOOL)(ptvdi->item.mask & TVIF_IMAGE);
        bRequestSelectedImage = (BOOL)(ptvdi->item.mask & TVIF_SELECTEDIMAGE);

        // Trial Emb 20/05/97 - found useless, see fix 21/05/97
        /*
        if (!bRequestSelectedImage) {
          ptvdi->item.mask |= TVIF_SELECTEDIMAGE;
          bRequestSelectedImage = (BOOL)(ptvdi->item.mask & TVIF_SELECTEDIMAGE);
        }
        */

        // only manage image request
        if (bRequestImage || bRequestSelectedImage) {
          LMQUERYICONS lmIcons;

          // clear images handles (0 is valid!)
          if (bRequestImage)
            ptvdi->item.iImage = -1;
          if (bRequestSelectedImage)
            ptvdi->item.iSelectedImage = -1;

          // icons handles empty by default
          lmIcons.hIcon1 = lmIcons.hIcon2 = NULL;

          // vdba item identifier into lmIcons
          hItem = ptvdi->item.hItem;
          lmIcons.ulID = pTree->GetItemIdentifier(hItem);

          // notify parent
          lResult = ::SendMessage(GetParent(hwnd), LM_QUERYICONS, (WPARAM)hwnd, (LPARAM)&lmIcons);

          // use received icon1 (icon2 ignored),
          // or use standardized icons if no returned icon
          int iconIndex;
          if (lmIcons.hIcon1)
            // add the icon in the imagelist
            iconIndex = pTree->AddIcon(lmIcons.hIcon1);
          else 
		  {
				UINT state;
				if (!pTree->ItemHasChildren (hItem))
					iconIndex = -1;
				else 
				{
					HICON hIconStd;
					BOOL bAdded = FALSE;
					state = pTree->GetItemState (hItem, TVIS_EXPANDED);
#if defined (EDBC)
					if (state & TVIS_EXPANDED)
						hIconStd = treeDll.m_ImageList.ExtractIcon (1); // Opened Folder
					else
						hIconStd = treeDll.m_ImageList.ExtractIcon (0); // Closed Folder
					bAdded = TRUE;
					iconIndex = pTree->AddIcon2(hIconStd);
#else
					//if (state & TVIS_EXPANDED)
					//	hIconStd = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICON_OPENED));
					//else
						hIconStd = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICON_CLOSED));
#endif
					if (!bAdded)
						iconIndex = pTree->AddIcon(hIconStd);
				}
            }

          if (iconIndex != -1) {
            // update the field in tv_item
            if (bRequestImage)
              ptvdi->item.iImage = iconIndex;
            if (bRequestSelectedImage)
              ptvdi->item.iSelectedImage = iconIndex;
          }
        }   // endif TVIF_IMAGE
      }
      break;

    case TVN_KEYDOWN:
      {
        // received parameter is a NM_TREEVIEW FAR *
        TV_KEYDOWN FAR * ptvkd = (TV_KEYDOWN FAR *)pnmh;

        // TAB character
        if (ptvkd->wVKey == VK_TAB) {
          // get vdba item identifier
          hItem = pTree->GetSelectedItem();
          vdbaRecId = pTree->GetItemIdentifier(hItem);

          // notify parent with vdba item identifier
          ::SendMessage(GetParent(hwnd), LM_NOTIFY_TABKEY, (WPARAM)hwnd, (LPARAM)vdbaRecId);

          // break the case TVN_KEYDOWN
          break;
        }

        // PageDown character : manage Ctrl+PgDown
        // PageUp   character : manage Ctrl+PgUp
        if (ptvkd->wVKey == VK_NEXT || ptvkd->wVKey == VK_PRIOR) {
          if (GetKeyState(VK_CONTROL) & 0x8000) {
            // get vdba item identifier
            hItem = pTree->GetSelectedItem();

            // get next or previous sibling
            if (ptvkd->wVKey == VK_NEXT)
              hItem = pTree->GetNextSiblingItem(hItem);
            if (ptvkd->wVKey == VK_PRIOR)
              hItem = pTree->GetPrevSiblingItem(hItem);

            // select and ensure visible
            if (hItem != NULL)
              if (pTree->SelectItem(hItem))
                pTree->EnsureVisible(hItem);

          }
          // break the case TVN_KEYDOWN
          break;
        }

      }
      break;  // break the case TVN_KEYDOWN

    case NM_RCLICK:
      // get vdba item identifier
      hItem = pTree->GetSelectedItem();
      vdbaRecId = pTree->GetItemIdentifier(hItem);

      // notify parent with vdba item identifier
      ::SendMessage(GetParent(hwnd), LM_NOTIFY_RBUTTONCLICK, (WPARAM)hwnd, (LPARAM)vdbaRecId);

      pTree->SetRClickNotifiedFlag();

      break;

    // Emb March 5, 96:
    // don't notify the caller, because the expansion is already managed
    // by the control. If we notify the caller, he will ask for another
    // expansion which toggles to a collapse
    // OLD CODE:
    //case NM_DBLCLK:
    //  // get vdba item identifier
    //  hItem = pTree->GetSelectedItem();
    //  vdbaRecId = pTree->GetItemIdentifier(hItem);
    //
    //  // notify parent with vdba item identifier
    //  ::SendMessage(GetParent(hwnd), LM_NOTIFY_LBUTTONDBLCLK, (WPARAM)hwnd, (LPARAM)vdbaRecId);
    //  break;
    // END OF OLD CODE

    case TVN_BEGINDRAG:
      // whatever control key is pressed or not

      // Preamble: change selection if needed
      {
        NM_TREEVIEW FAR* pnmtv = (NM_TREEVIEW FAR*)pnmh;
        HTREEITEM hDraggedItem = pnmtv->itemNew.hItem;
        hItem = pTree->GetSelectedItem();
        if (hDraggedItem != hItem) {
          BOOL bSuccess = pTree->SelectItem(hDraggedItem);
          ASSERT (bSuccess);
          hItem = hDraggedItem;
        }
      }

      vdbaRecId = pTree->GetItemIdentifier(hItem);

      // DO NOT BEGIN DRAG IF MOUSE BUTTON HAS BEEN RELEASED!
      {
        BOOL bButtonPressed = FALSE;
        if (GetSystemMetrics(SM_SWAPBUTTON))
          bButtonPressed = GetAsyncKeyState(VK_RBUTTON);
        else
          bButtonPressed = GetAsyncKeyState(VK_LBUTTON);
        if (!bButtonPressed)
          break;
      }

      // Here we can begin drag
      pTree->BeginDrag ();
      if (GetKeyState(VK_CONTROL) & 0x8000)   // Is control pressed ? yes ---> 1 in wparam, otherwise 0
        ::SendMessage (GetParent(hwnd), LM_NOTIFY_STARTDRAGDROP, (WPARAM)1, (LPARAM)vdbaRecId);
      else
        ::SendMessage (GetParent(hwnd), LM_NOTIFY_STARTDRAGDROP, (WPARAM)0, (LPARAM)vdbaRecId);
      // necessary because parent sets capture on hwnd
      SetCapture(pTree->m_hWnd);
      break;

  }
  return 0L;
}

static BOOL TamponOnAddRecord(HWND hwnd, LPLINK_RECORD lpLr)
{
   DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);
   HTREEITEM hParent;
   HTREEITEM hInsertAfter;
   HTREEITEM hItem;
   BOOL      bRet;

   if (lpLr->ulParentID)
     hParent = pTree->GetHTreeItem(lpLr->ulParentID);
   else
     hParent = TVI_ROOT;
   if (lpLr->ulInsertAfter)
     hInsertAfter = pTree->GetHTreeItem(lpLr->ulInsertAfter);
   else
     hInsertAfter = TVI_LAST;
   hItem = pTree->InsertItem(lpLr->lpString, hParent, hInsertAfter);
   if (!hItem)
     return FALSE;  // Failure!
   pTree->SetAssociate(lpLr->ulRecID, hItem);
   bRet = pTree->SetItemData(hItem, (DWORD)lpLr->lpItemData);
   if (bRet) {
      TV_ITEM item;
      memset(&item, 0, sizeof(item));
      item.hItem = hItem;
      item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
      item.iImage = I_IMAGECALLBACK;
      item.iSelectedImage = I_IMAGECALLBACK;
      bRet = pTree->SetItem(&item);
   }

   return bRet;

}



static BOOL TamponOnExpand (HWND hwnd, DWORD identifier)
{
    DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);
    HTREEITEM hItem;
    
    hItem = pTree->GetHTreeItem (identifier);
    if (hItem)
    {
        UINT state;
        //
        // Expand an item is possible if it's a parent item
        //
        if (!pTree->ItemHasChildren (hItem))
            return FALSE;
        state = pTree->GetItemState (hItem, TVIS_EXPANDED);
        if (!(state & TVIS_EXPANDED))
        {
            //
            // Item is not yet expanded, so expand it
            //
            if (!pTree->Expand (hItem, TVE_EXPAND))
                return FALSE;
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}


static BOOL TamponOnCollapse (HWND hwnd, DWORD identifier)
{
    DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);
    HTREEITEM hItem;
    
    hItem = pTree->GetHTreeItem (identifier);
    if (hItem)
    {
        UINT state;
        //
        // Collapse an item is possible if it's a parent item
        //
        if (!pTree->ItemHasChildren (hItem))
            return FALSE;
        state = pTree->GetItemState (hItem, TVIS_EXPANDED);
        if (state & TVIS_EXPANDED)
        {
            //
            // Item is already expanded, so collapse it
            //
            if (!pTree->Expand (hItem, TVE_COLLAPSE))
                return FALSE;
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}


static BOOL TamponOnDelete (HWND hwnd, DWORD identifier)
{
    DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);
    HTREEITEM hItem;
    
    hItem = pTree->GetHTreeItem (identifier);
    if (hItem)
    {
        if (!pTree->DeleteItem (hItem))
            return FALSE;
        pTree->DelAssociate (identifier, hItem);
        return TRUE;
    }
    return FALSE;
}


static BOOL TamponOnDeleteAll (HWND hwnd)
{
    DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);

    pTree->StopNotifications();
    pTree->DeleteAllItems ();
    pTree->DelAssociate ();
    return TRUE;
}

static DWORD TamponOnGetItemData (HWND hwnd, DWORD identifier)
{
    DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);
    HTREEITEM hItem;
    
    hItem = pTree->GetHTreeItem (identifier);
    if (hItem)
    {
        return pTree->GetItemData (hItem);
    }
    return (DWORD)0;

}


static DWORD TamponOnGetParent (HWND hwnd, DWORD identifier)
{
    DWORD     dwId;
    HTREEITEM hItem;
    HTREEITEM hParentItem;
    DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);
   
    hItem = pTree->GetHTreeItem (identifier);
    if (hItem)
    {
        hParentItem = pTree->GetParentItem (hItem);
        if (hParentItem)
        {
            dwId = pTree->GetItemIdentifier (hParentItem);
            return dwId;
        }
        return (DWORD)0;
    }
    return (DWORD)0;
}

static DWORD TamponOnGetCurSel (HWND hwnd)
{
    HTREEITEM hItem;
    DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);

    hItem = pTree->GetSelectedItem ();
    if (hItem)
        return pTree->GetItemIdentifier (hItem);

    // side effect for tear-out and restart from position:
    // no initial selection, causing a gpf
    // we return the first item

    hItem = pTree->GetRootItem ();
    if (hItem)
        return pTree->GetItemIdentifier (hItem);

    // last chance
    return (DWORD)0;
}


static BOOL TamponOnSetSel (HWND hwnd, DWORD identifier)
{
    HTREEITEM hItem;
    DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);
   
    hItem = pTree->GetHTreeItem (identifier);
    if (hItem)
    {
      // Fix Emb Oct. 02, 96 : return value test was not correct
        if (pTree->SelectItem (hItem))
          return TRUE;
    }
    return FALSE;
}


static BOOL TamponOnSetString (HWND hwnd, LPSETSTRING lpst)
{
    HTREEITEM hItem;
    ASSERT (lpst != NULL);
    DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);
   
    hItem = pTree->GetHTreeItem (lpst->ulRecID);
    if (hItem)
    {
        return pTree->SetItemText (hItem, (LPCTSTR)lpst->lpString);
    }
    return FALSE;
}

static LPCTSTR TamponOnGetItemText (HWND hwnd, DWORD identifier)
{
    HTREEITEM hItem;
    DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);
   
    hItem = pTree->GetHTreeItem (identifier);
    if (hItem)
    {
      CString text = pTree->GetItemText (hItem);
      static char szItemText[100];  // common for all instances
      _tcscpy(szItemText, text);
      return szItemText;
    }
    return NULL;
}


static DWORD TamponOnGetItemLevel(HWND hwnd, DWORD identifier)
{
    DWORD     dwLevel = 0;
    HTREEITEM hItem;
    HTREEITEM hParentItem;
    DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);

    // no selection ---> return level 0
    if(identifier == 0)
      return 0L;
   
    hItem = pTree->GetHTreeItem (identifier);
    hParentItem = hItem;
    while (hParentItem)
    {
        hParentItem = pTree->GetParentItem (hItem);
        hItem = hParentItem;
        if (hParentItem)
            dwLevel++;
    }
    return dwLevel;   
}

static BOOL     TamponOnGetItemState (HWND hwnd, DWORD identifier)
{
    DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);
    HTREEITEM hItem;
    
    hItem = pTree->GetHTreeItem (identifier);
    if (hItem)
    {
        UINT state;
        if (!pTree->ItemHasChildren (hItem))
            return FALSE;
        state = pTree->GetItemState (hItem, TVIS_EXPANDED);
        if (!(state & TVIS_EXPANDED))
            return FALSE;
        else
            return TRUE;
    }
    return FALSE;
}

static BOOL     TamponOnSetTopItem   (HWND hwnd, DWORD identifier)
{
    DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);
    HTREEITEM hItem;
    
    hItem = pTree->GetHTreeItem (identifier);
    if (hItem)
    {
        pTree->Select (hItem, TVGN_FIRSTVISIBLE);
        return TRUE;
    }
    return FALSE;
}

static BOOL     TamponOnAddBefore    (HWND hwnd, LPLINK_RECORD lpRec)
{
    DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);
    HTREEITEM hParent;
    HTREEITEM hBefore;
    HTREEITEM hItem;
    BOOL      bRet;

    //
    // NOTE: 
    // Let X-->Y a relation (R1) that verifies:
    //     GetNext (X) = Y, and GetPrev (Y) = X.
    // If we have relation (R1) then Insert (e, before(Y)) <==>
    // Insert (e, after (X))
    //

    if (lpRec->ulParentID)
        hParent = pTree->GetHTreeItem(lpRec->ulParentID);
    else
        hParent = TVI_ROOT;
    VERIFY (lpRec->ulRecID != 0);
    VERIFY (lpRec->ulInsertAfter != 0);
    hItem = pTree->GetHTreeItem (lpRec->ulInsertAfter);
    hBefore = pTree->GetPrevSiblingItem (hItem);
    if (!hBefore)
        hItem = pTree->InsertItem (lpRec->lpString, hParent, TVI_FIRST);
    else
        hItem = pTree->InsertItem (lpRec->lpString, hParent, hBefore);
    if (!hItem)
        return FALSE; 
  
    pTree->SetAssociate (lpRec->ulRecID, hItem);
    bRet = pTree->SetItemData (hItem, (DWORD)lpRec->lpItemData);
    if (bRet) 
    {
        TV_ITEM item;
        memset(&item, 0, sizeof(item));
        item.hItem = hItem;

        // FIX DLL BUG EMB 21/05/97: the following lines were incomplete or missing!
        item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        item.iImage = I_IMAGECALLBACK;
        item.iSelectedImage = I_IMAGECALLBACK;

        bRet = pTree->SetItem(&item);
    }
    return bRet;
}

static DWORD    TamponOnGetFirstChild(HWND hwnd, DWORD identifier)
{
    DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);
    HTREEITEM hItem;
    HTREEITEM hChildItem;
    
    hItem = pTree->GetHTreeItem (identifier);
    if (hItem)
    {
        if (!pTree->ItemHasChildren (hItem))
            return 0;
        hChildItem = pTree->GetChildItem (hItem);
        if (!hChildItem)
            return 0;
        return pTree->GetItemIdentifier (hChildItem);
    }
    return 0;
}

static DWORD    TamponOnGetFirst     (HWND hwnd)
{
    DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);
    HTREEITEM hItem;
    
    hItem = pTree->GetRootItem ();
    pTree->SetCurhItem(hItem);
    if (!hItem)
        return 0;
    return pTree->GetItemIdentifier (hItem);
}


static DWORD    TamponOnGetNext      (HWND hwnd, DWORD identifier)
{
    HTREEITEM hNextItem;
    HTREEITEM hItem;
    DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);
    if (identifier)
      hItem = pTree->GetHTreeItem (identifier);
    else
      hItem = pTree->GetCurhItem();

    if (!pTree->ItemHasChildren (hItem))
    {
        hNextItem = pTree->GetNextSiblingItem (hItem);
        if (!hNextItem)
        {
            HTREEITEM hParent;
            HTREEITEM hNextSibling;
 
            hParent = pTree->GetParentItem (hItem);
            while ((hNextSibling = pTree->GetNextSiblingItem (hParent)) == NULL)
            {
                hParent = pTree->GetParentItem (hParent);
                if (hParent == TVGN_ROOT)
                    return 0;  
            }
            pTree->SetCurhItem(hNextSibling);
            return pTree->GetItemIdentifier (hNextSibling);
        }
        pTree->SetCurhItem(hNextItem);
        return pTree->GetItemIdentifier (hNextItem);
    }
    else
    {
        pTree->SetCurhItem(pTree->GetChildItem (hItem));
        return pTree->GetItemIdentifier (pTree->GetChildItem (hItem));
    }
}


static HTREEITEM XLastSon (CTreeCtrl* tr, HTREEITEM item)
{
    HTREEITEM k, s = NULL;;

    if (!tr->ItemHasChildren (item))
        return item;
    k = tr->GetChildItem (item);
    while (k)
    {
        s = k;    
        k = tr->GetNextSiblingItem (s);
    }
    return s;
}




static HTREEITEM XLast (CTreeCtrl* tr, HTREEITEM item)
{
    if (!tr->ItemHasChildren (item))
    {
        return item;
    }
    else
    {
        HTREEITEM sonItem;
        sonItem = XLastSon (tr, item);
        return XLast (tr, sonItem);
    }
}

static HTREEITEM XPrev (CTreeCtrl* tr, HTREEITEM item)
{
    HTREEITEM hParentItem;
    HTREEITEM hPrevSibling;

    hParentItem  = tr->GetParentItem (item);
    hPrevSibling = tr->GetPrevSiblingItem (item);
    if (!hPrevSibling)
    {
        if (hParentItem)
            return hParentItem;
        else
            return NULL;    
    }
    return XLast (tr, hPrevSibling);
}





static DWORD    TamponOnGetPrev      (HWND hwnd, DWORD identifier)
{
    DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);
    HTREEITEM hItem;
    HTREEITEM hPrevItem;
    
    hItem = pTree->GetHTreeItem (identifier);
    if (hItem)
    {
        hPrevItem = XPrev (pTree, hItem);
        if (!hPrevItem)
            return 0;
        return pTree->GetItemIdentifier (hPrevItem);
    }
    return 0;
}

static DWORD    TamponOnGetTopItem   (HWND hwnd)
{
    DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);
    HTREEITEM hItem;
    
    hItem = pTree->GetFirstVisibleItem ();
    if (hItem)
    {
        return pTree->GetItemIdentifier (hItem);
    }
    return 0;
}

static DWORD    TamponOnGetCount     (HWND hwnd)
{
    DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);
  
    return (DWORD)pTree->GetCount();
}

static DWORD TamponOnSetFont (HWND hwnd, HFONT hFont)
{
    DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);
    VERIFY (hFont != NULL);
    VERIFY (pTree != NULL);
    pTree->SetFont (CFont::FromHandle (hFont));

    return 0L;
}

static DWORD    TamponOnGetMouseItem (HWND hwnd, POINTS ptShort)
{
  HTREEITEM hItem;
  UINT      flags;
  DWORD     vdbaRecId;
  POINT     point;      // with long values
  DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);

  // prepare point starting from points
  point.x = ptShort.x;
  point.y = ptShort.y;

  // hit test
  hItem = pTree->HitTest(point, &flags);
  if (hItem) {
    vdbaRecId = pTree->GetItemIdentifier(hItem);
    if (vdbaRecId)
      return vdbaRecId;
  }
  return 0L;
}

static void XCollapseBranch (CTreeCtrl* tr, HTREEITEM hBranch)
{
    if (tr->ItemHasChildren (hBranch))
    {
        HTREEITEM hSon = tr->GetChildItem (hBranch);
        tr->Expand (hBranch, TVE_COLLAPSE);

        while (hSon)
        {
            XCollapseBranch (tr, hSon);
            hSon = tr->GetNextSiblingItem (hSon);
        }
    }
}

static BOOL    TamponOnCollapseOne (HWND hwnd, DWORD identifier)
{
    DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);
    HTREEITEM hItem;
    
    hItem = pTree->GetHTreeItem (identifier);
    if (hItem)
    {
        XCollapseBranch (pTree, hItem);
        return TRUE;
    }
    return FALSE;
}

static BOOL    TamponOnCollapseAll (HWND hwnd)
{
    DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);
    HTREEITEM hRoot = pTree->GetRootItem ();
    
    if (!hRoot)
        return FALSE;
    while (hRoot)
    {
        XCollapseBranch (pTree, hRoot);
        hRoot = pTree->GetNextSiblingItem (hRoot);
    }
    return TRUE;
}

static BOOL     TamponOnPrintGetFirst(HWND hwnd, BOOL b, LPLMDRAWITEMSTRUCT lpDraw)
{
    DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);
    HTREEITEM hItem = pTree->GetRootItem(); // GetFirstVisible does not 501s

    if (!hItem)
        return FALSE;
    //
    // Create an invisible Jeff's tree window.
    //
    if (pTree->m_hTreePrint)
        DestroyWindow (pTree->m_hTreePrint);
    pTree->m_hTreePrint = LRCreateWindowPrint (pTree->m_hOwner, &(pTree->m_rcLocation), 0,pTree->m_bVODrawStyle);
    if (!pTree->m_hTreePrint)
        return FALSE;
    //
    // Populate the Jeff's tree window.
    //
    LBTreePopulate (pTree->m_hTreePrint, hwnd);
    SendMessage (pTree->m_hTreePrint, LM_SETSEL, 0, (LPARAM)TamponOnGetFirst (hwnd)); 
    return (BOOL)::SendMessage (pTree->m_hTreePrint, LM_PRINT_GETFIRST, (WPARAM)b, (LPARAM)lpDraw);
}


static BOOL     TamponOnPrintGetNext (HWND hwnd, BOOL b, LPLMDRAWITEMSTRUCT lpDraw)
{
    BOOL bMoreItem;
    DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);
    
    if (!pTree->m_hTreePrint)
        return FALSE;
    bMoreItem = (BOOL)::SendMessage (pTree->m_hTreePrint, LM_PRINT_GETNEXT, (WPARAM)b, (LPARAM)lpDraw);

    if (!bMoreItem)
    {
          DestroyWindow (pTree->m_hTreePrint);
          pTree->m_hTreePrint = NULL;
    }
    return bMoreItem;
}

static DWORD    TamponOnGetVisState  (HWND hwnd, DWORD identifier)
{
    HTREEITEM hItem;
    HTREEITEM hParentItem;
    DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);

    // no selection ---> return error
    if(identifier == 0)
      return (DWORD)(-1L);
   
    hItem = pTree->GetHTreeItem (identifier);
    if (!hItem)
      return (DWORD)(-1L);

    for(hParentItem = pTree->GetParentItem (hItem);
        hParentItem;
        hParentItem = pTree->GetParentItem(hParentItem)) {
      UINT state = pTree->GetItemState(hParentItem, TVIF_STATE) & TVIS_EXPANDED;
      if (!state)
        return FALSE;   // not candidate for visibility
    }
    return TRUE;
}

static DWORD    TamponOnGetNextSibling  (HWND hwnd, DWORD identifier)
{
  HTREEITEM hItem;
  HTREEITEM hSiblingItem;
  DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);

  // no selection ---> return error
  if(identifier == 0)
    return (DWORD)(0L);

  hItem = pTree->GetHTreeItem (identifier);
  if (!hItem)
    return (DWORD)(0L);

  hSiblingItem = pTree->GetNextSiblingItem(hItem);

  if (hSiblingItem)
    return pTree->GetItemIdentifier(hSiblingItem);
  else
    return (DWORD)(0L);
}

static DWORD    TamponOnGetPrevSibling  (HWND hwnd, DWORD identifier)
{
  HTREEITEM hItem;
  HTREEITEM hSiblingItem;
  DbaTree * pTree = (DbaTree *)GetWindowLong(hwnd, 0);

  // no selection ---> return error
  if(identifier == 0)
    return (DWORD)(0L);

  hItem = pTree->GetHTreeItem (identifier);
  if (!hItem)
    return (DWORD)(0L);

  hSiblingItem = pTree->GetPrevSiblingItem(hItem);

  if (hSiblingItem)
    return pTree->GetItemIdentifier(hSiblingItem);
  else
    return (DWORD)(0L);
}

static HWND TamponOnGetTreeViewHwnd (HWND hwnd)
{
    DbaTree* pTree = (DbaTree *)GetWindowLong(hwnd, 0);
    return pTree->m_hWnd;
}


/////////////////////////////////////////////////////////////////////////////
// Tampon Window Proc
LRESULT CALLBACK /* __export */ TamponWndProc ( HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam )
{
  switch (wMsg) {
    HANDLE_MSG(hwnd, WM_DESTROY, TamponOnDestroy);
    HANDLE_MSG(hwnd, WM_SIZE, TamponOnSize);
    HANDLE_MSG(hwnd, WM_SETFOCUS, TamponOnSetFocus);
    HANDLE_MSG(hwnd, WM_NOTIFY, TamponOnNotify);

    // messages LM_xxx
    case LM_ADDRECORD:
      return TamponOnAddRecord(hwnd, (LPLINK_RECORD)lParam);
    case LM_EXPAND:
      return TamponOnExpand(hwnd, (DWORD)lParam);
    case LM_COLLAPSE:
      return TamponOnCollapse(hwnd, (DWORD)lParam);
    case LM_DELETERECORD:
      return TamponOnDelete(hwnd, (DWORD)lParam);
    case LM_DELETEALL:
      return TamponOnDeleteAll(hwnd);
    case LM_GETITEMDATA:
      return TamponOnGetItemData(hwnd, (DWORD)lParam);
    case LM_GETPARENT:
      return TamponOnGetParent(hwnd, (DWORD)lParam);
    case LM_GETSEL:
      return TamponOnGetCurSel(hwnd);
    case LM_SETSEL:
      return TamponOnSetSel(hwnd, (DWORD)lParam);
    case LM_SETSTRING:
      return TamponOnSetString(hwnd, (LPSETSTRING)lParam);
    case LM_GETSTRING:
      return (LRESULT)TamponOnGetItemText(hwnd, (DWORD)lParam);

    case LM_GETLEVEL:
      return TamponOnGetItemLevel (hwnd, (DWORD)lParam);
    case LM_GETSTATE:
      return TamponOnGetItemState (hwnd, (DWORD)lParam);
    case LM_SETTOPITEM:
      return TamponOnSetTopItem   (hwnd, (DWORD)lParam); 
    case LM_ADDRECORDBEFORE:
      return TamponOnAddBefore    (hwnd, (LPLINK_RECORD)lParam); 
    case LM_GETFIRSTCHILD:
      return TamponOnGetFirstChild(hwnd, (DWORD)lParam); 
    case LM_GETFIRST:
      return TamponOnGetFirst     (hwnd); 
    case LM_GETNEXT:
      return TamponOnGetNext      (hwnd, (DWORD)lParam); 
    case LM_GETPREV:
      return TamponOnGetPrev      (hwnd, (DWORD)lParam); 
    case LM_GETTOPITEM:
      return TamponOnGetTopItem   (hwnd); 
    case LM_GETCOUNT:
      return TamponOnGetCount     (hwnd);
    case LM_GETMOUSEITEM:
      return TamponOnGetMouseItem (hwnd, MAKEPOINTS(lParam));
    case LM_SETFONT:
      return TamponOnSetFont      (hwnd, (HFONT)wParam);
    case LM_COLLAPSEBRANCH:
      return TamponOnCollapseOne  (hwnd, (DWORD)lParam);
    case LM_COLLAPSEALL:
      return TamponOnCollapseAll  (hwnd);
    case LM_PRINT_GETFIRST:
      return TamponOnPrintGetFirst(hwnd, (BOOL)wParam, (LPLMDRAWITEMSTRUCT)lParam); 
    case LM_PRINT_GETNEXT:
      return TamponOnPrintGetNext (hwnd, (BOOL)wParam, (LPLMDRAWITEMSTRUCT)lParam); 
    case LM_GETVISSTATE:
      return TamponOnGetVisState  (hwnd, (DWORD)lParam);

    // speed optimization
    case LM_GETNEXTSIBLING:
      return TamponOnGetNextSibling (hwnd, (DWORD)lParam);
    case LM_GETPREVSIBLING:
      return TamponOnGetPrevSibling (hwnd, (DWORD)lParam);
    case LM_GET_TREEVIEW_HWND:
      return (LRESULT)TamponOnGetTreeViewHwnd  (hwnd);

    default:
      return DefWindowProc (hwnd,wMsg,wParam,lParam);
  }
  return 0L;
}

/////////////////////////////////////////////////////////////////////////////
// Public C interface

extern "C" HWND FAR PASCAL EXPORT LRCreateWindow(HWND hOwner,LPRECT rcLocation,DWORD dwStyle,BOOL bVODrawStyle)
{
  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  TRACE0("Inside Catolbox.dll...\n");

  // All DLL entry points should have a top-level TRY/CATCH block.
  // Otherwise, it would be possible to throw an uncaught exception from
  //  an the DLL.  This would most likely cause a crash.

  // WARNING :
  // dwStyle not taken in account
  // bVODrawStyle cannot be managed (3d not available in a DLL)

  HWND hwndTampon;
  CWnd* cParentWnd;
  CRect parentRect;


  // check for map m_HandleToDwIndex - see comment in DbaTree class definition,
  // section "speed optimization
  ASSERT (sizeof(HTREEITEM) == sizeof(DWORD) );

  TRY {

    // code for intermediary window
    hwndTampon = CreateWindow(
      szTamponClassName,
      NULL,       // caption
      WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE,
      rcLocation->left,                   // x
      rcLocation->top,                    // y
      rcLocation->right-rcLocation->left, // dx
      rcLocation->bottom-rcLocation->top, // dy
      hOwner,                             // parent
      0,                        // menu
      AfxGetInstanceHandle(),
      NULL);                    // parameter
    if (!hwndTampon)
      return 0;

    // code for CTreeCtrl, son of hwndTampon
    cParentWnd = CWnd::FromHandle(hwndTampon);
    cParentWnd->GetClientRect(&parentRect);

    BOOL bRet;
  
    DbaTree * pTree = new DbaTree();
    bRet = pTree->Create(
        WS_CHILD | WS_VISIBLE
        | WS_CLIPCHILDREN | WS_CLIPSIBLINGS
        | TVS_HASLINES | TVS_HASBUTTONS
        | TVS_LINESATROOT | TVS_SHOWSELALWAYS,
        parentRect,
        cParentWnd,
        100);           // dummy id!
    pTree->m_hOwner             = hOwner; //GetParent (hwndTampon);
    pTree->m_dwStyle            = dwStyle;
    pTree->m_bVODrawStyle       = bVODrawStyle;
    pTree->m_rcLocation.top     = rcLocation->top;
    pTree->m_rcLocation.left    = rcLocation->left;
    pTree->m_rcLocation.right   = rcLocation->right;
    pTree->m_rcLocation.bottom  = rcLocation->bottom;

    if (bRet)
      if (pTree->AssociateIconList())
        // CTreeCtrl * associated with it's parent window
        // CAST TO LONG MANDATORY FOR C4
        SetWindowLong(hwndTampon, 0, (long)pTree);

    // display
    if (bRet) {
      ShowWindow(hwndTampon, SW_SHOW);
      UpdateWindow(hwndTampon);
    }

  } // end of TRY

  CATCH_ALL(e) {
    // a failure caused an exception.
    if (IsWindow(hwndTampon))
      DestroyWindow(hwndTampon);
    return 0;
  }
  END_CATCH_ALL

  return hwndTampon;
}

/////////////////////////////////////////////////////////////////////////////
// Library init



BOOL CTreeDll::InitInstance()
{
  // any DLL initialization goes here
  TRACE0("CATOLBOX.DLL initializing...\n");

  // register clas of intermediary window
  WNDCLASS  wc;
  wc.style         = 0;
  wc.lpfnWndProc   = TamponWndProc;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = sizeof (DbaTree *);
  wc.hInstance     = AfxGetInstanceHandle();
  wc.hIcon         = NULL;
  wc.hCursor       = ::LoadCursor(NULL,IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE+1);
  wc.lpszMenuName  = NULL;
  wc.lpszClassName = szTamponClassName;
  if (!AfxRegisterClass (&wc))
    return FALSE;
  m_ImageList.Create (IDB_FOLDER, IMAGE_WIDTH, 0, RGB(255,0,255));
  return TRUE;
}

int CTreeDll::ExitInstance()
{
  // any DLL termination goes here (WEP-like code)
  return CWinApp::ExitInstance();
}

CTreeDll treeDll(_T("catolbox.dll"));

/////////////////////////////////////////////////////////////////////////////
// DbaTree class methods

DbaTree::DbaTree ()
  :CTreeCtrl()
{
  m_bDragging = FALSE;
  curhItem = 0;
  bNoNotif = FALSE;

  m_hTreePrint  = NULL;
  m_hOwner      = NULL;
  memset(&m_rcLocation, 0, sizeof(m_rcLocation));
  m_dwStyle       = 0L;
  m_bVODrawStyle  = FALSE;

  GetHTreeItem_LastDwId = -1;
  GetHTreeItem_LastHtreeItem = NULL;

  GetItemIdentifier_LastHtreeItem = NULL;
  GetItemIdentifier_LastDwId = -1;

  m_bRClickNotified = FALSE;
}

DbaTree::~DbaTree()
{
  m_DWORDxHTREEITEM.RemoveAll();
  m_HandleToDwIndex.RemoveAll();
//  iconList.DeleteImageList();
}

BOOL DbaTree::AssociateIconList()
{
  if (!iconList.Create(IMAGE_WIDTH,
                       IMAGE_HEIGHT,
                       TRUE,    // bMask
                       10, 10))
    return FALSE;
  SetImageList(&iconList, TVSIL_NORMAL);
  return TRUE;
}

// adds an icon in the associated iconlist,
// and returns the index on it,
// or -1 if error
int DbaTree::AddIcon(HICON hIcon)
{
//	return AddIcon2(hIcon);
	
	
  int iconIndex = -1;
  CDC * pscreenDc;
  CDC memDc;
  CBitmap bmp;
  CBitmap *pOldBmp;

  // check whether the icon has already been stored
  // in the image list
  // note that the index has been stored with +1
  // since it starts from 0 in CImageList
  // while 0 is not a valid value in a collection
  if (m_aIconHandle.Lookup(hIcon, iconIndex))
    return iconIndex - 1;
  pscreenDc = GetDC();    // méthod of CWnd
  if (pscreenDc) {
    if (memDc.CreateCompatibleDC(pscreenDc)) {
      if (bmp.CreateCompatibleBitmap(pscreenDc, IMAGE_WIDTH, IMAGE_HEIGHT)) {
        pOldBmp = memDc.SelectObject(&bmp);
        if (pOldBmp) {
          // Background Color choice criterion: must not be used in displayed bitmaps,
          // otherwise they would become partially transparent.
          // use of plain color such as full white or full black seems more reliable than use of 
          // GetSysColor(COLOR_WINDOW) or GetSysColor(COLOR_APPWORKSPACE),
          // since they may vary in a lot of colours
          //
          // chosen color: WHITE
          //
          COLORREF backgroundColor = RGB(0xFF, 0xFF, 0xFF);
          CBrush brushForBackground(backgroundColor);
          CRect cr(0, 0, IMAGE_WIDTH, IMAGE_HEIGHT);
          memDc.FillRect(&cr, &brushForBackground);

          memDc.DrawIcon(0, 0, hIcon);
          memDc.SelectObject(pOldBmp);

          // (Cbitmap *)NULL does not work under Windows95!
          // odl code : iconIndex = iconList.Add(&bmp, (CBitmap *)NULL);
          iconIndex = iconList.Add(&bmp, backgroundColor);

          // add in iconHandle table with a +1 offset
          m_aIconHandle.SetAt(hIcon, iconIndex+1);
        }
        bmp.DeleteObject();   // defined in CGdiObject class
      }
      memDc.DeleteDC();
    }
    ReleaseDC(pscreenDc);
  }
  return iconIndex;
}

int DbaTree::AddIcon2(HICON hIcon)
{
	int iconIndex = -1;
	// check whether the icon has already been stored
	// in the image list
	// note that the index has been stored with +1
	// since it starts from 0 in CImageList
	// while 0 is not a valid value in a collection
	if (m_aIconHandle.Lookup(hIcon, iconIndex))
		return iconIndex - 1;
	iconIndex = iconList.Add(hIcon);
	// add in iconHandle table with a +1 offset
	m_aIconHandle.SetAt(hIcon, iconIndex+1);
	return iconIndex;
}


HTREEITEM DbaTree::GetHTreeItem (DWORD dwId)
{
  BOOL found;
  DWORD     dwHItem;  // placeholder for HTREEITEM

  // last-requested test
  if (dwId == GetHTreeItem_LastDwId)
    return GetHTreeItem_LastHtreeItem;

  if (m_DWORDxHTREEITEM.IsEmpty())
    return NULL;

  // store last-requested dwId
  GetHTreeItem_LastDwId = dwId;

  found = m_DWORDxHTREEITEM.Lookup (dwId, dwHItem);
  if (found) {
    GetHTreeItem_LastHtreeItem  = (HTREEITEM)dwHItem;
    return GetHTreeItem_LastHtreeItem;
  }
  else {
    GetHTreeItem_LastHtreeItem  = NULL;
    return NULL;
  }
}

// fast code with second map
DWORD DbaTree::GetItemIdentifier (HTREEITEM hItem)
{
  BOOL found;
  DWORD dwIndex;

  // last-requested test
  if (hItem == GetItemIdentifier_LastHtreeItem)
    return GetItemIdentifier_LastDwId;

  if (m_HandleToDwIndex.IsEmpty())
    return NULL;

  // store last-requested hTreeItem
  GetItemIdentifier_LastHtreeItem = hItem;

  found = m_HandleToDwIndex.Lookup ((DWORD)hItem, dwIndex);
  if (found) {
    GetItemIdentifier_LastDwId  = dwIndex;
    return dwIndex;
  }
  else {
    GetItemIdentifier_LastDwId  = NULL;
    return NULL;
  }
}


// Old code with vut's map - masqued
#ifdef BIDIRECTIONNAL_MAP
DWORD DbaTree::GetItemIdentifier (HTREEITEM hItem)
{
  CAssociate aS;
  POSITION pos;
  DWORD rKey;

  // last-requested test 
  if (hItem == GetItemIdentifier_LastHtreeItem)
    return GetItemIdentifier_LastDwId;

  if (m_DWORDxHTREEITEM.IsEmpty())
    return NULL;

  // store last-requested hTreeItem
  GetItemIdentifier_LastHtreeItem = hItem;

  pos = m_DWORDxHTREEITEM.GetStartPosition ();
  while (pos != NULL)
  {
    m_DWORDxHTREEITEM.GetNextAssoc (pos, rKey, aS);
    if (aS.htreeitem == hItem) {
      GetItemIdentifier_LastDwId = aS.id;
      return aS.id;
    }
  }
  GetItemIdentifier_LastDwId = -1;
  return 0;
}
#endif  // BIDIRECTIONNAL_MAP

void DbaTree::SetAssociate (DWORD identifier, HTREEITEM hItem)
{
  m_DWORDxHTREEITEM.SetAt (identifier, (DWORD)hItem);
  // added for second table
  m_HandleToDwIndex.SetAt ((DWORD)(hItem), identifier);
}

void DbaTree::DelAssociate (DWORD identifier, HTREEITEM hItem)
{
    m_DWORDxHTREEITEM.RemoveKey (identifier);
    m_HandleToDwIndex.RemoveKey ((DWORD)hItem);

    // reset for HTREEITEM GetHTreeItem(DWORD dwId)
    if ( identifier == GetHTreeItem_LastDwId) {
      GetHTreeItem_LastDwId = (DWORD)-1;
      GetHTreeItem_LastHtreeItem = NULL;
    }

    // reset for DWORD GetItemIdentifier(HTREEITEM hItem)
    if (hItem == GetItemIdentifier_LastHtreeItem) {
      GetItemIdentifier_LastHtreeItem = NULL;
      GetItemIdentifier_LastDwId = (DWORD)-1;
    }
}

void DbaTree::DelAssociate ()
{
    m_DWORDxHTREEITEM.RemoveAll();
}

// use mouse move to notify parent with LM_NOTIFY_ONITEM
afx_msg void DbaTree::OnMouseMove( UINT nFlags, CPoint point )
{
  HTREEITEM hItem;
  UINT      flags;
  DWORD     vdbaRecId;
  HWND      hwndTampon;

  hwndTampon = ::GetParent(m_hWnd);
  if (MouseMoveDrag ()) {
    ::SendMessage (::GetParent(hwndTampon), LM_NOTIFY_DRAGDROP_MOUSEMOVE, (WPARAM)hwndTampon, (LPARAM)POINT2LONG (point.x, point.y));
  }
  else {
    hItem = HitTest(point, &flags);
    if (hItem) {
      vdbaRecId = GetItemIdentifier(hItem);
      if (vdbaRecId) {
        // notify parent for 'on item'
        ::SendMessage(::GetParent(hwndTampon),
                      LM_NOTIFY_ONITEM,
                      (WPARAM)hwndTampon,
                      (LPARAM)vdbaRecId);
      }
    }
    CTreeCtrl::OnMouseMove(nFlags, point);
  }
}

afx_msg void DbaTree::OnLButtonUp(UINT nFlags, CPoint point)
{
    HWND      hwndTampon;
    hwndTampon = ::GetParent(m_hWnd);
    if (LButtonUpDrag ())
    {
        ::SendMessage (::GetParent(hwndTampon), LM_NOTIFY_ENDDRAGDROP, (WPARAM)hwndTampon, (LPARAM)0);
        ResetDrag();
    }

    CTreeCtrl::OnLButtonUp(nFlags, point);
}

afx_msg void DbaTree::OnLButtonDown (UINT nFlags, CPoint point)
{  
  // Emb May 7, 93 : SelectItem found needless,
  // and even buggy for last visible item
  /*
    HTREEITEM hHit = HitTest (point, &nFlags);
    if (hHit)
    {
        SelectItem (hHit);
    }
  */

  // Emb 24/06/97: drag with ctrl pressed before lbuttondown on a non selected item
  if (nFlags & MK_CONTROL) {
    HTREEITEM hHit = HitTest (point, &nFlags);
    if (hHit)
      SelectItem (hHit);
  }

	CTreeCtrl::OnLButtonDown (nFlags, point);
}

afx_msg void DbaTree::OnRButtonDown (UINT nFlags, CPoint point)
{  
    HTREEITEM hHit = HitTest (point, &nFlags);
    if (hHit)
    {
        EnsureVisible(hHit);    // Emb May 7, 93 : found necessary prior to SelectItem
                                // for last visible item
        SelectItem (hHit);
    }

  m_bRClickNotified = FALSE;
	CTreeCtrl::OnRButtonDown (nFlags, point);
  if (!m_bRClickNotified) {
    // get vdba item identifier
    HTREEITEM hItem = GetSelectedItem();
    DWORD vdbaRecId = GetItemIdentifier(hItem);

    // notify parent with vdba item identifier
    HWND hwnd = GetParent()->m_hWnd;
    ::SendMessage(::GetParent(hwnd), LM_NOTIFY_RBUTTONCLICK, (WPARAM)hwnd, (LPARAM)vdbaRecId);
  }
}

afx_msg void DbaTree::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags)
{
  if (nChar == VK_NEXT || nChar == VK_PRIOR) {
    if (GetKeyState(VK_CONTROL) & 0x8000) {
      // Don't call CTreeCtrl::OnKeyDown, special management here instead

      // get vdba item identifier
      HTREEITEM hItem = GetSelectedItem();

      // get next or previous sibling
      if (nChar == VK_NEXT)
        hItem = GetNextSiblingItem(hItem);
      else
        hItem = GetPrevSiblingItem(hItem);

      // select and ensure visible
      if (hItem != NULL)
        if (SelectItem(hItem))
          EnsureVisible(hItem);

      return;   // ignore this key combination
    }
  }
  CTreeCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}


/////////////////////////////////////////////////////////////////////////////
// methode of CAssociate class

CAssociate::CAssociate (const CAssociate& aS):CObject()
{
  id = aS.id;
  htreeitem = aS.htreeitem;
}


CAssociate& CAssociate::operator= (const CAssociate& aS)
{
  this->id = aS.id;
  this->htreeitem = aS.htreeitem;
  return (*this); 
}

// Fills the invisible jeff's tree with the current tree image,
// for print purposes
static void LBTreePopulate (HWND hlbtree, HWND hwnd /*TamponWindow*/)
{
    LINK_RECORD   lr;

    UINT first = TamponOnGetFirst (hwnd);

    while (first)
    {
        lr.lpString       = (char*)TamponOnGetItemText (hwnd, first);
        lr.ulRecID        = first;
        lr.ulParentID     = TamponOnGetParent (hwnd, first);
        lr.ulInsertAfter  = 0;    // we don't care since sequential inserts
        lr.lpItemData     = 0;
        SendMessage (hlbtree,  LM_ADDRECORD, 0, (LPARAM) (LPVOID) (LINK_RECORD FAR *) &lr);
        first = TamponOnGetNext (hwnd, first);
    }
    first = TamponOnGetFirst (hwnd);
    while (first)
    {
        if (TamponOnGetItemState (hwnd, first))
            ::SendMessage (hlbtree, LM_EXPAND  , 0, (LPARAM)first);
        else
            ::SendMessage (hlbtree, LM_COLLAPSE, 0, (LPARAM)first);
        first = TamponOnGetNext (hwnd, first);
    }
}


afx_msg void DbaTree::OnSysColorChange()
{
  CTreeCtrl::OnSysColorChange();

  // reset image list
  while (iconList.GetImageCount()) {
    BOOL bOk = iconList.Remove(0);
    ASSERT (bOk);
  }

  // reset map between icon handle and index in image list
  m_aIconHandle.RemoveAll();

  // Force redraw
  InvalidateRect(NULL);
}

void DbaTree::PostNcDestroy() 
{
	delete this;
	CTreeCtrl::PostNcDestroy();
}
/////////////////////////////////////////////////////////////////////////////
