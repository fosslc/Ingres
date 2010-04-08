//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
//
// PAN/LCM source control information:
//
//  Archive name    ^[Archive:j:\vo4clip\lbtree\arch\message.cpp]
//  Modifier        ^[Author:rumew01(1600)]
//  Base rev        ^[BaseRev:None]
//  Latest rev      ^[Revision:1.0 ]
//  Latest rev time ^[Timestamp:Thu Aug 18 16:12:10 1994]
//  Check in time   ^[Date:Thu Aug 18 16:51:45 1994]
//  Change log      ^[Mods:// ]
// 
// Thu Aug 18 16:12:10 1994, Rev 1.0, rumew01(1600)
//  init
// 
// Fri Apr 29 16:40:50 1994, Rev 68.0, sauje01(0500)
//  Add LM_DISABLEEXPANDALL message
// 
// Thu Mar 24 17:11:58 1994, Rev 67.0, sauje01(0500)
// 
// Sun Mar 20 11:46:54 1994, Rev 66.0, sauje01(0500)
// 
// Fri Mar 18 15:39:30 1994, Rev 65.0, sauje01(0500)
// 
// Thu Mar 17 17:49:38 1994, Rev 64.0, schma09(0600)
//  MS0001 - return VK_F1 to parent
// 
// Mon Jan 24 12:11:28 1994, Rev 63.0, sauje01(0500)
//  p
//  p
// 
// Fri Jan 07 17:31:06 1994, Rev 62.0, sauje01(0500)
// 
// Wed Jan 05 14:40:14 1994, Rev 61.0, sauje01(0500)
// 
// Tue Jan 04 17:15:18 1994, Rev 60.0, sauje01(0500)
// 
// Mon Jan 03 16:44:12 1994, Rev 59.0, sauje01(0500)
// 
// Wed Nov 24 16:51:36 1993, Rev 58.0, sauje01(0500)
// 
// Wed Nov 24 15:49:12 1993, Rev 57.0, sauje01(0500)
// 
// Tue Nov 16 16:29:40 1993, Rev 56.0, sauje01(0500)
// 
// Sat Oct 30 13:40:56 1993, Rev 55.0, sauje01(0500)
// 
//	14-Feb-2000 (hanch04)
//	    UNIX does not have heapmin().
//
//  27-Sep-2000 (schph01)
//      bug #99242 . cleanup for DBCS compliance
//  08-May-2009 (drivi01)
//	Add return type to NotifyReturn function to
//	clean up errors during Visual Studio 2008 port.
//////////////////////////////////////////////////////////////////////////////
#include "lbtree.h"

extern "C"
{
#include <windows.h>
#include <windowsx.h>
#include <memory.h>
#include <string.h>
#include <malloc.h>
#include "winlink.h"
#include "compat.h"
#include "cm.h"
#include <tchar.h>
}


static BOOL Cmp (LPSTR p, int c)
{
  LPSTR lp =_tcschr(p, '\t');
  if (lp)
    CMnext(lp);
  else
    lp=p;
  return  (_tcsupr(lp)==_tcsupr((LPSTR) (DWORD) c));
}

LRESULT LinkedListWindow:: ON_COMMAND (WPARAM wParam,LPARAM lParam)
{
#ifdef WIN32
    HWND hwndCtl = (HWND)lParam;
    int id = (int)LOWORD(wParam);
    UINT codeNotify = (UINT)HIWORD(wParam);
#else
    HWND hwndCtl = (HWND)LOWORD(lParam);
    int id = (int)wParam;
    UINT codeNotify = (UINT)HIWORD(lParam);
#endif

  if (hWndEdit && hWndEdit==hwndCtl && codeNotify==EN_CHANGE ) 
    {
    RECT rc;
    if (cursel && !GetItemRect(cursel,&rc))
      {
      firstvisible=cursel;
      InvalidateRect(hWnd,NULL,FALSE);
      VerifyScroll();
      UpdateWindow(hWnd);
      }
    bEditChanged=TRUE;
    }
  return 1;
}

LRESULT LinkedListWindow::ON_USER (WPARAM wParam,LPARAM lParam)
{
  if (ownerDraw)
    GetExtents();
  if (hWndEdit)
    SetFocus(hWndEdit);
  return 0;
}

static LRESULT LLCallProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
  HWND hWndParent=GetParent(hWnd);
  if (hWndParent)
    {
    LinkedListWindow * ll=(LinkedListWindow *)GetWindowLong(hWndParent,0); 
    if (ll) 
      {
      WNDPROC wp= (WNDPROC) ll->GetEditProc();
      if (wp)
        return CallWindowProc(wp,hWnd,message,wParam,lParam);
      }
    }
  return 0L;
}

void NotifyReturn(void);

static BOOL EditKeyDown(HWND hWnd,WPARAM wParam,LPARAM lParam)
{
  switch (wParam)
    {
    case VK_NEXT:
    case VK_PRIOR:
    case VK_UP:
    case VK_DOWN:
    case VK_RETURN:
    case VK_TAB:
    case VK_ESCAPE:
      {
      HWND hWndParent=GetParent(hWnd);
      if (hWndParent)
        {
        LinkedListWindow * ll=(LinkedListWindow *)GetWindowLong(hWndParent,0); 
        if (ll) 
          {
          if (wParam == VK_RETURN)
            ll->NotifyReturn();
          ll->ON_KEYDOWN(wParam,lParam);
          UpdateWindow(ll->HWnd());
          return TRUE;
          }
        }
      }
      break;
    }
  return FALSE;
}

static void EditKeyFocus(HWND hWnd,WPARAM wParam,LPARAM lParam)
{
  HWND hWndParent=GetParent(hWnd);
  if (hWndParent && hWndParent!=(HWND)wParam)
    {
    LinkedListWindow * ll=(LinkedListWindow *)GetWindowLong(hWndParent,0); 
    if (ll)
      {
      RECT rc;
      ll->EditNotify(FALSE); 
      if (ll->GetItemRect (ll->Cursel(), &rc))  
        InvalidateRect(ll->HWnd(),&rc,FALSE);
      }
    }
}

extern "C" LRESULT CALLBACK __export EditSubWndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
  switch (message)
    {
    case WM_KEYDOWN:
      if (EditKeyDown(hWnd,wParam,lParam))
        return 0;
      break;
    case WM_KILLFOCUS:
      EditKeyFocus(hWnd,wParam,lParam);
      break;
    }
  return (LLCallProc(hWnd,message,wParam,lParam));
}

LRESULT LinkedListWindow::ON_CREATE (WPARAM wParam,LPARAM lParam)
{
  if (!hWnd || !hWndOwner ||  (GetWindowLong(hWnd,0)!=(long)this))
    return -1;
  if (lParam)
    VODrawStyle= ((((LPCREATESTRUCT)lParam)->lpCreateParams)!=0);
  ownerDraw=((GetWindowLong(hWnd,GWL_STYLE)&LMS_OWNERDRAW)!=0);
  epilogDraw=((GetWindowLong(hWnd,GWL_STYLE)&LMS_EPILOGDRAW)!=0);
  if (GetWindowLong(hWnd,GWL_STYLE)&LMS_CANEDIT)
    {
    hWndEdit=CreateWindow("edit",NULL,WS_CHILD|WS_CLIPSIBLINGS|ES_LEFT|ES_WANTRETURN|ES_AUTOHSCROLL|ES_WANTRETURN,0,0,0,0,hWnd,NULL,ghDllInstance,0);
    if (hWndEdit)
      {
      lpOldEditProc = SetWindowLong(hWndEdit,GWL_WNDPROC,(LONG) EditSubWndProc);
      Edit_LimitText(hWndEdit, MAX_EDIT_TEXT_LENGTH);
      ShowWindow(hWndEdit,SW_HIDE);
      }
    }

  GetExtents();
  SetScrollRange(hWnd,SB_VERT,0,MAX_SCROLL,FALSE);
  SetScrollPos  (hWnd,SB_VERT,0,FALSE);
  SetScrollRange(hWnd,SB_HORZ,0,MAX_SCROLL,FALSE);
  SetScrollPos  (hWnd,SB_HORZ,0,FALSE);

  PostMessage(hWnd,WM_USER,0,0L);
  return 0;
}

LRESULT LinkedListWindow::ON_CHAR    (WPARAM wParam,LPARAM lParam)
{
  if (!Count())
    return 0;
  switch (wParam)
    {
    case _T('+'):
      ON_KEYDOWN (VK_ADD,0);
      break;
    case _T('-'):
      ON_KEYDOWN (VK_SUBTRACT,0);
      break;
    default:
      if (wParam>_T(' '))
        {
        Link * find=0;
        current=cursel;

        for (Link * l=Next(); !find &&  l; l=Next())
          {
          char *p=GetString(l);
          if (p )
            {
            if (Cmp(p,wParam))
              find=l;
            }
          }

        if (!find)
          
        for (Link * l=First(); !find &&  l!=cursel; l=Next())
          {
          char *p=GetString(l);
          if (p )
            {
            if (Cmp(p,wParam))
              find=l;
            }
          }

        if (find)
          {
          RECT rc;
          if (!GetItemRect(find,&rc) || (rc.bottom>ptSizeWin.y))
            {
            firstvisible=find;
            ChangeSelection (find,TRUE);
            InvalidateRect(hWnd,NULL,FALSE);
            }
          else
            ChangeSelection (find,FALSE);
          VerifyScroll();
          UpdateWindow(hWnd);
          }
        current=cursel;
        }
      break;
    }
  return 0;
}



//
// Modified 11/7/95 Emb : if received lParam is 0xabcd, send
// synchronous notification message LM_NOTIFY_EXPAND
//
// Used when expanding branch with sub-branch from the application
//
// Was already implemented some weeks ago, but modification lost
// at hand-made merge time...
//
// Modified 24/8/95 Emb : if received lParam is 0xdcba, for collapse,
// don't call toggle mode which scans the whole tree, but only set
// the item to collapsed state. - SPEED ENHANCEMENT -
// To be used only in domdisp when we are about to create a single
// dummy sub-item for a branch
//
LRESULT LinkedListWindow::ON_KEYDOWN (WPARAM wParam,LPARAM lParam)
{
  if (!Count())
    {
    if (wParam==VK_TAB)
      PostMessage(hWndOwner,LM_NOTIFY_TABKEY,(WPARAM)hWnd,lParam);
    return 0;
    }
  switch (wParam)
    {
    case VK_RETURN:
      PostMessage(hWndOwner,LM_NOTIFY_RETURNKEY,(WPARAM)hWnd,cursel->Item());
      break;

    case VK_TAB:
      PostMessage(hWndOwner,LM_NOTIFY_TABKEY,(WPARAM)hWnd,lParam);
      break;
 
    case VK_ESCAPE:
      PostMessage(hWndOwner,LM_NOTIFY_ESCKEY,(WPARAM)hWnd,lParam);
      break;
 
    case VK_LEFT:
      ON_HSCROLL (SB_LINELEFT,0);
      break;

    case VK_RIGHT:
      ON_HSCROLL (SB_LINERIGHT,0);
      break;

    case VK_ADD:
      // Emb 16/3/95 : masqued expandbranch and expandall keyboard tilt
      //if (GetKeyState(VK_CONTROL)&0x8000)
      //  {
      //  if (GetKeyState(VK_SHIFT)&0x8000)
      //    ON_EXPANDALL (0,0L);
      //  else
      //    ON_EXPANDBRANCH(0,0L);
      //  }
      //else
      // end of Emb 16/3/95 masqued expandbranch and expandall keyboard tilt
        {
        HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
        SetVisible(cursel);
        if (cursel && cursel->IsParent() && !cursel->IsExpanded())
          {
          RECT rc;
          ToggleMode(cursel);

          // Send or post according to special request from the caller
          if (lParam == 0xabcd)
            SendMessage(hWndOwner, LM_NOTIFY_EXPAND, (WPARAM)hWnd, cursel->Item());
          else 
            PostMessage(hWndOwner, LM_NOTIFY_EXPAND, (WPARAM)hWnd, cursel->Item());

          bDirty|=0x01;
          if (GetItemRect (cursel, &rc))
            {
            rc.bottom=ptSizeWin.y;
            InvalidateRect(hWnd,&rc,FALSE);
            }
          }
        SetCursor(hOldCursor);
        }
      break;

    case VK_SUBTRACT:
      if (GetKeyState(VK_CONTROL)&0x8000)
        {
        if (GetKeyState(VK_SHIFT)&0x8000)
          ON_COLLAPSEALL (0,0L);
        else
          ON_COLLAPSEBRANCH(0,0L);
        }
      else
        {
        SetVisible(cursel);
        if (cursel && cursel->IsExpanded())
          {
          RECT rc;
          if (lParam == 0xdcba)
            {
            cursel->ToggleMode(); // simply change the flag
            }
          else
            {
            ToggleMode(cursel);
            if (cursel->IsParent())
              {
              SendMessage(hWndOwner,LM_NOTIFY_COLLAPSE,(WPARAM)hWnd,cursel->Item());
              bDirty|=0x01;
              if (GetItemRect (cursel, &rc))
                {
                rc.bottom=ptSizeWin.y;
                InvalidateRect(hWnd,&rc,FALSE);
                }
              }
            }
          }
        }
      break;

    case VK_NEXT:
      SetVisible(cursel);
      if (GetKeyState(VK_CONTROL)&0x8000)
        {
        Link *newSel=current=cursel;
        int  reqlevel = newSel->Level();
        Link *l;
        for(l=Next(); l; l=Next())
          {
          int level = l->Level();
          if (level == reqlevel)
            {
            RECT rc;
            if (!GetItemRect(l,&rc) || rc.bottom>ptSizeWin.y)
              {
              firstvisible = l;
              ChangeSelection (l,TRUE);
              InvalidateRect(hWnd,NULL,FALSE);
              }
            else
              ChangeSelection (l,FALSE);
            VerifyScroll();
            UpdateWindow(hWnd);
            return 0;
            }
          }
        }
      else 
        {
        RECT rc;
        int  i=nbVisible>1 ? nbVisible-1 : 1;
        Link *newSel=current=cursel;
        while(i--)
          {
          Link *l=Next();
          if (l)
            newSel=l;
          }
        if (newSel==Last())
          {
          ON_KEYDOWN (VK_END,0);
          UpdateWindow(hWnd);
          return 0;
          }
        if (!GetItemRect(newSel,&rc))
          {
          firstvisible=cursel;
          ChangeSelection (newSel,TRUE);
          InvalidateRect(hWnd,NULL,FALSE);
          }
        else
          ChangeSelection (newSel,FALSE);
        VerifyScroll();
        UpdateWindow(hWnd);
        }
        break;

    case VK_PRIOR:
      SetVisible(cursel);
      if (GetKeyState(VK_CONTROL)&0x8000)
        {
        Link *newSel=current=cursel;
        int  reqlevel = newSel->Level();
        Link *l;
        for(l=Prev(); l; l=Prev())
          {
          int level = l->Level();
          if (level == reqlevel)
            {
            RECT rc;
            if (!GetItemRect(l,&rc))
              {
              firstvisible = l;
              ChangeSelection (l,TRUE);
              InvalidateRect(hWnd,NULL,FALSE);
              }
            else
              ChangeSelection (l,FALSE);
            VerifyScroll();
            UpdateWindow(hWnd);
            return 0;
            }
          }
        }
      else 
        {
        RECT rc;
        int  i=nbVisible>1 ? nbVisible-1 : 1;
        Link *newSel=current=cursel;
        while(i--)
          {
          Link *l=Prev();
          if (l)
            newSel=l;
          }
        if (newSel==First())
          {
          ON_KEYDOWN (VK_HOME,0);
          UpdateWindow(hWnd);
          return 0;
          }
        if (!GetItemRect(newSel,&rc))
          {
          firstvisible=newSel;
          ChangeSelection (newSel,TRUE);
          InvalidateRect(hWnd,NULL,FALSE);
          }
        else
          ChangeSelection (newSel,FALSE);
        VerifyScroll();
        UpdateWindow(hWnd);
        }
        break;

    case VK_HOME:
      SetVisible(cursel);
      if (1)
        {
        ChangeSelection (First(),TRUE);
        ON_VSCROLL (SB_TOP,0);
        }
      break;

    case VK_END:
      SetVisible(cursel);
      if (1)
        {
        ChangeSelection (last,TRUE);
        ON_VSCROLL (SB_BOTTOM,0);
        }
      break;

    case VK_UP:
        {
        SetVisible(cursel);
        RECT    rc;
        current=cursel;
        Link *l=Prev();
        if (l)
          {
          if (!GetItemRect(l,&rc))
            {
            ChangeSelection (l,TRUE);
            inDrag=TRUE;
            if (!hWndEdit)
              ON_VSCROLL (SB_LINEUP,0);
            inDrag=0;
            }
          else
            ChangeSelection (l,FALSE);
          }
        }
      break;

    case VK_DOWN:
        {
        SetVisible(cursel);
        RECT    rc;
        current=cursel;
        Link *l=Next();
        if (l)
          {
          if (!GetItemRect(l,&rc) || rc.bottom>ptSizeWin.y)
            {
            ChangeSelection (l,TRUE);
            inDrag=TRUE;
            ON_VSCROLL (SB_LINEDOWN,0);
            inDrag=0;
            }
          else
            ChangeSelection (l,FALSE);
          }
        }
      break;
//  MS0001 - return to parent    
    case VK_F1:
    	return SendMessage(hWndOwner, WM_KEYDOWN, VK_F1, lParam);  

    }
  return 0;
}

LRESULT LinkedListWindow::ON_DESTROY (WPARAM wParam,LPARAM lParam)
{
  EditNotify(FALSE); 
  if (hWndEdit && lpOldEditProc)
    SetWindowLong(hWndEdit,GWL_WNDPROC,lpOldEditProc);
  SendMessage(hWndOwner,LM_NOTIFY_DESTROY,(WPARAM)hWnd,0L);
  SetWindowLong(hWnd,0,0);
  //delete this;    // UKS masks this line March 1' 1996.
#ifndef MAINWIN
  _heapmin();
#endif /* MAINWIN */
  return 0;
}


LRESULT LinkedListWindow::ON_GETMINMAXINFOS (WPARAM wParam,LPARAM lParam)
{
  return SendMessage(hWndOwner,LM_NOTIFY_GETMINMAXINFOS,(WPARAM)hWnd,lParam);
}

LRESULT LinkedListWindow::ON_PAINT  (WPARAM wParam,LPARAM lParam)
{
  LMDRAWITEMSTRUCT lmStruct;
  PAINTSTRUCT      ps;
  HFONT            hOld=0;
  HCURSOR          hOldCursor; // Emb 27/6/95

  hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT)); // Emb 27/6/95

  if (bDirty)
    {
    RetrieveLines ();
    Clear();
    }

  BeginPaint(hWnd,&ps);


  if (hFont)
    hOld=(HFONT)SelectObject(ps.hdc,hFont);

  CalcTabStop(ps.hdc,&lmStruct.iTabStopSize);


  lmStruct.rcItem.left=-GetScrollPos(hWnd,SB_HORZ);
  lmStruct.rcItem.right=ptSizeWin.x;
  lmStruct.rcItem.top=0;
  lmStruct.rcItem.bottom=0;
  lmStruct.uiIndentSize =sizeIndent;

  lmStruct.hWndItem=hWnd;
  lmStruct.hDC=ps.hdc;

  for (Link *l =current=firstvisible;l;l=Next())
    {
    lmStruct.rcItem.top=lmStruct.rcItem.bottom;
    lmStruct.rcItem.bottom+=sizeItem;
    if (lmStruct.rcItem.top>ps.rcPaint.bottom)
      break;
    if (IntersectRect(&lmStruct.rcPaint,&lmStruct.rcItem,&ps.rcPaint))
      {
      ULONG    lRet=0;
      lmStruct.ulID=l->Item();
      lmStruct.lpString=GetString(l);
      lmStruct.uiLevel=l->Level();
      lmStruct.bExpanded=l->IsExpanded();
      lmStruct.bParent=l->IsParent();

      lmStruct.lpiPosLines=(LPINT)l->GetData();

      lmStruct.uiItemState=ITEMSTATEDRAWENTIRE;

      lmStruct.textColor = GetSysColor(COLOR_WINDOWTEXT);
      lmStruct.bkColor   = bkColor;

      if (l==cursel)
        {
        if (!hWndEdit||!IsWindowVisible(hWndEdit))
          lmStruct.uiItemState|=ITEMSTATESELECTED;
        else
          EditMove();
        if ((!bPaintSelectOnFocus||(GetFocus()==hWnd)) && (!hWndEdit||!IsWindowVisible(hWndEdit)))
          lmStruct.uiItemState|=ITEMSTATEFOCUS;
        }
      if (ownerDraw)
        lRet=SendMessage(hWndOwner,LM_DRAWITEM,0,(LPARAM) (LPLMDRAWITEMSTRUCT) &lmStruct);
      if (lRet!=LMDRAWALL)
        {
        LMQUERYICONS icons;
        icons.ulID = lmStruct.ulID;
        icons.hIcon1 = (HICON)NULL;
        icons.hIcon2 = (HICON)NULL;
        if (iIconsMargin != 0)
            SendMessage(hWndOwner,LM_QUERYICONS,(WPARAM)hWnd,(LPARAM)&icons);
        if (VODrawStyle)
          PaintVODrawStyle(&lmStruct,lRet,icons.hIcon1,icons.hIcon2,iIconsMargin);
        else
          PaintItem(&lmStruct,lRet,icons.hIcon1,icons.hIcon2,iIconsMargin);
        }
	   if (epilogDraw)
		 SendMessage(hWndOwner, LM_EPILOGDRAW, 0, (LPARAM)(LPLMDRAWITEMSTRUCT) &lmStruct);
      }
    }

  if (lmStruct.rcItem.bottom<ps.rcPaint.bottom)
    {
    HBRUSH hBr=CreateSolidBrush(VODrawStyle ? GetSysColor(COLOR_BTNFACE) : bkColor);
    if (hBr)
      {
      lmStruct.rcItem.left=ps.rcPaint.left;
      lmStruct.rcItem.right=ps.rcPaint.right;
      lmStruct.rcItem.top=lmStruct.rcItem.bottom;
      lmStruct.rcItem.bottom=ps.rcPaint.bottom;
      FillRect(ps.hdc,&lmStruct.rcItem,hBr);
      DeleteObject(hBr);
      }
    }
  SelectObject(ps.hdc,hOld ? hOld : GetStockObject(SYSTEM_FONT));
  EndPaint(hWnd,&ps);

  SetCursor(hOldCursor);  // Emb 27/6/95

  return 0;
}

LRESULT LinkedListWindow::ON_SIZE   (WPARAM wParam,LPARAM lParam)
{
  RECT rc;

  if (wParam!=SIZE_MINIMIZED)
    {
    GetWindowRect(hWnd,&rc);
    rc.right -=rc.left;
    rc.bottom-=rc.top;

    ptSizeWin.x=LOWORD(lParam);
    ptSizeWin.y=HIWORD(lParam);  
    nbVisible=(ptSizeWin.y)/sizeItem;

    if (ptPete.x!=rc.right || ptPete.y!=rc.bottom)
      {
      ptPete.x=rc.right;
      ptPete.y=rc.bottom;

      VerifyScroll();
      VerifyScrollH();
      }

    if (1 || VODrawStyle)
      InvalidateRect(hWnd,NULL,FALSE);
    }
  return 0;
}

LRESULT LinkedListWindow::ON_VSCROLL (WPARAM wParam,LPARAM lParam)
{
  ULONG ulCount=Count();
#ifdef WIN32
  int nScrollCode = LOWORD(wParam);
  short nPos = HIWORD(wParam);
  HWND hwndScrollBar = (HWND)lParam;
#else
  int nScrollCode = (int)wParam;
  int nPos = LOWORD(lParam);
  HWND hwndCtl = HIWORD(lParam);
#endif

  if (ulCount>(ULONG)nbVisible)
    {
    ULONG ulOld  =GetIndex(firstvisible);
    ULONG ulNew  =ulOld;
    ulCount-=nbVisible;

    switch (nScrollCode)
      {
      case SB_LINEUP:
        if (ulNew)
          ulNew--;
        break;
    
      case SB_LINEDOWN:
        ulNew++;
        break;

      case SB_PAGEUP:
        if (ulNew>(ULONG)nbVisible)
          ulNew-=nbVisible;
        else
          ulNew=0;
        break;

      case SB_PAGEDOWN:
        ulNew+=nbVisible;
        break;

      case SB_TOP:
        ulNew=0;
        break;

      case SB_ENDSCROLL:
        UpdateWindow(hWnd);
        return 0;
        break;

      case SB_BOTTOM:
        ulNew=Count();
        break;

      case SB_THUMBTRACK:
      {
        ulNew=CalcThumbPos(ulCount,nPos);
        InvalidateRect(hWnd,NULL,FALSE);
        break;
      }
      }

    if (ulNew>ulCount)
      ulNew=ulCount;

    if (ulNew!=ulOld)
      {
      Link *pl=First();
      for (ULONG l=0; l<ulNew && pl;l++,pl=Next());
      if (pl)
        {
        RECT rcScroll;
        int  iScroll;
        firstvisible=pl;

        SetScrollPos(hWnd,SB_VERT,CalcScrollPos(ulCount,ulNew),TRUE);

        if (hWndEdit)
          {
          RECT rc;
          if (!GetItemRect(cursel,&rc))
            MoveWindow(hWndEdit,0,0,0,0,TRUE);
          }

        rcScroll.left=0;
        rcScroll.right=ptSizeWin.x;
        switch (wParam)
          {
          case SB_LINEUP:
            if (ptSizeWin.y<sizeItem*2)
              InvalidateRect(hWnd,NULL,FALSE);
            else
              {
              iScroll=sizeItem;
              rcScroll.top=inDrag ? sizeItem: 0;
              rcScroll.bottom=sizeItem*nbVisible;
              }
            break;
          case SB_LINEDOWN:
            if (ptSizeWin.y<sizeItem*3)
              InvalidateRect(hWnd,NULL,FALSE);
            else
              {
              iScroll=-sizeItem;
              rcScroll.top=sizeItem;

              // Emb 29/09/95 : too much optimized for NT...
#ifdef WIN32
              rcScroll.bottom = ptSizeWin.y;
#else
              rcScroll.bottom=inDrag ? ptSizeWin.y-sizeItem*2: ptSizeWin.y;
#endif
              }
            break;

          case SB_PAGEUP:
          case SB_PAGEDOWN:
          case SB_TOP:
          case SB_BOTTOM:
            InvalidateRect(hWnd,NULL,FALSE);
            return 0;
            break;
          default :
            return 0;
            break;
          } 
        ScrollWindow(hWnd,0,iScroll,&rcScroll,NULL);
        UpdateWindow(hWnd);
        }
      }
    }
  return 0;
}

LRESULT LinkedListWindow::ON_HSCROLL (WPARAM wParam,LPARAM lParam)
{
  int iPos,iMin,iMax;
#ifdef WIN32
  int nScrollCode = LOWORD(wParam);
  short nPos = HIWORD(wParam);
  HWND hwndScrollBar = (HWND)lParam;
#else
  int nScrollCode = (int)wParam;
  int nPos = LOWORD(lParam);
  HWND hwndCtl = HIWORD(lParam);
#endif

  GetScrollRange    (hWnd,SB_HORZ,&iMin,&iMax);
  iPos=GetScrollPos (hWnd,SB_HORZ);

  if (iMax>1)
    {
    int iNew=iPos;

    switch (nScrollCode)
      {
      case SB_LINELEFT:
        iNew-=sizeIndent;
        break;
    
      case SB_LINERIGHT:
        iNew+=sizeIndent;
        break;

      case SB_PAGELEFT:
        iNew-=(ptSizeWin.x-sizeIndent);
        break;

      case SB_PAGERIGHT:
        iNew+=(ptSizeWin.x-sizeIndent);
        break;

      case SB_LEFT:
        iNew=iMin;
        break;

      case SB_RIGHT:
        iNew=iMax;
        break;

      case SB_ENDSCROLL:
        UpdateWindow(hWnd);
        return 0;
        break;

      case SB_THUMBTRACK:
      {
        iNew=nPos;
        if (iNew!=iPos)
          {
          InvalidateRect(hWnd,NULL,FALSE);
          SetScrollPos(hWnd,SB_HORZ,iNew,TRUE);
          }
        return 0;
        break;
      }
      }

    if (iNew>iMax)
      iNew=iMax;

    else if (iNew<iMin)
      iNew=iMin;

    if (iNew!=iPos)
      {
      SetScrollPos(hWnd,SB_HORZ,iNew,TRUE);
      if (ptSizeWin.x>sizeIndent)
        {
        if (!VODrawStyle && ((nScrollCode==SB_LINERIGHT) || (nScrollCode==SB_LINELEFT)))
          {
          RECT rcScroll;   
          int  iScroll=iPos-iNew;

          rcScroll.left=0;
          rcScroll.top=0;
          rcScroll.right=ptSizeWin.x;
          rcScroll.bottom=ptSizeWin.y;
          if (iScroll>0)
            rcScroll.right-=iScroll;
          else
            rcScroll.left=-iScroll;
          ScrollWindow(hWnd,iScroll,0,&rcScroll,NULL);
          UpdateWindow(hWnd);
          }
        else
          {
          switch (nScrollCode)
            {
            case SB_LINELEFT:
            case SB_LINERIGHT:
            case SB_PAGEUP:
            case SB_PAGEDOWN:
            case SB_TOP:
            case SB_BOTTOM:
              InvalidateRect(hWnd,NULL,FALSE);
              return 0;
              break;
            default :
              return 0;
              break;
            }
          }
        }
      else
        InvalidateRect(hWnd,NULL,FALSE);
      }
    }
  return 0;
}

LRESULT LinkedListWindow::ON_ADDRECORD   (WPARAM wParam,LPARAM lParam)
{
  LPLINK_RECORD lpLR=(LPLINK_RECORD) lParam;
  if (lpLR)
    {
    if (!lpLR->ulRecID)
      return 0;

    Link *lCurrent=Add(lpLR->ulRecID,lpLR->ulParentID,lpLR->lpString,lpLR->lpItemData,lpLR->ulInsertAfter,0);
    if (lCurrent)
      {
      bDirty|=0x02;
      if (!firstvisible)
        firstvisible=lCurrent;
      if (!cursel)
        {
        Link * newSel=lCurrent;
        ChangeSelection (newSel,TRUE);
        EditNotify(TRUE);
        }
      InvalidateRect(hWnd,NULL,FALSE);
      return 1;
      }
    }
  return 0;
}

LRESULT LinkedListWindow::ON_DELETERECORD(WPARAM wParam,LPARAM lParam)
{
  Link *l = Find(lParam);
  if (l)
    {
    bDirty|=0x03;
    current=cursel;

    if (firstvisible==l)
      {
      firstvisible=0;
      for (Link *l2=l->Next();l2 && !firstvisible;l2=l2->Next())
        {
        if (l2->IsVisible() && (l2->Level()<=l->Level()) )
          firstvisible=l2;
        }
      }

    if (l==cursel)
      {
      cursel=Next();
      if (!cursel)
        {
        current=l;
        cursel=Prev();
        }
      if (cursel)
        PostMessage(hWndOwner,LM_NOTIFY_SELCHANGE,(WPARAM)hWnd,cursel->Item());
      }

    Delete(l);
    cursel=current;

    if (!firstvisible)
      firstvisible=cursel;
    VerifyScroll();
    InvalidateRect(hWnd,NULL,FALSE);

    if (hWndEdit)
      {
      if (cursel)
        EditNotify(TRUE);
      else
        ShowWindow(hWndEdit,SW_HIDE);
      }
    }
  return 0;
}

LRESULT LinkedListWindow::ON_DELETEALL(WPARAM wParam,LPARAM lParam)
{

  for (Link *l=first; l;l=l->Next())
    delete(l);

  first=last=current=lastparent=0;
  firstNonResolved=0;
  firstvisible=0;
  cursel=0;
  inDrag=0;
  bDirty=0x03;
  count=0;
  InvalidateRect(hWnd,NULL,FALSE);
  if (hWndEdit)
    ShowWindow(hWndEdit,SW_HIDE);
  return 1;
}


LRESULT LinkedListWindow::ON_SETSEL      (WPARAM wParam,LPARAM lParam)
{
  Link * l=lParam ? Find(lParam) : first;
  if (l)
    {
    RECT rc;
    if (!GetItemRect(l,&rc) || (rc.bottom>ptSizeWin.y))
      {
      BOOL bAfter=FALSE;
      for (Link *l2=firstvisible;l2 && !bAfter ;l2=l2->Next())
        {
        if (l2==l)
          bAfter=TRUE;
        }
      if (bAfter)
        {
        int toSkip=nbVisible;
        for (Link *l3=current=l;l3;l3=Prev(),toSkip--)
          {
          if (toSkip==1)
            {
            firstvisible= l3;
            break;
            }
          }
        }
      else
        firstvisible= l;
      ChangeSelection(l,TRUE);
      InvalidateRect(hWnd,NULL,FALSE);
      VerifyScroll();
      }
    else
      ChangeSelection (l,FALSE);
    return 1;
    }
  return 0;
}

LRESULT LinkedListWindow::ON_GETSEL      (WPARAM wParam,LPARAM lParam)
{
  return cursel ? cursel->Item() : 0 ;
}

LRESULT LinkedListWindow::ON_SETFOCUS       (WPARAM wParam,LPARAM lParam)
{
  RECT rc;
  if (GetItemRect (cursel, &rc))  
    {
    InvalidateRect(hWnd,&rc,FALSE);
    curMessage=cursel;
    PostMessage(hWndOwner,LM_NOTIFY_ONITEM,(WPARAM)hWnd,curMessage->Item());
    }
  if (hWndEdit && hWndEdit!=(HWND)wParam)
    {
    EditNotify(TRUE); 
    }
  return 0;
}

LRESULT LinkedListWindow::ON_KILLFOCUS       (WPARAM wParam,LPARAM lParam)
{
  RECT rc;
  if (GetItemRect (cursel, &rc))  
    InvalidateRect(hWnd,&rc,FALSE);
  return 0;
}



VOID    LinkedListWindow::MouseClick (POINT ptMouse,BOOL bRightButton)
{
  RECT  rc;
  for (Link *l =current=firstvisible;l;l=Next())
    {
    if (GetItemRect (l, &rc) && PtInRect(&rc,ptMouse))
      {
      ChangeSelection   (l,FALSE);
      if (bRightButton)
        {
        SendMessage(hWndOwner,LM_NOTIFY_RBUTTONCLICK,(WPARAM)hWnd,l->Item());
        }
      else
        {
        RECT    rcBtn;
        LRESULT lRet=SendMessage(hWndOwner,LM_NOTIFY_LBUTTONCLICK,(WPARAM)hWnd,MAKELONG(ptMouse.x,l->Level()));

        rcBtn=rc;

        if (VODrawStyle)
          {
          int midX=rc.left+ ((sizeIndent*l->Level())+DISTANCE+1)+(sizeIndent/2);
          int midY=(rc.top+rc.bottom)/2;

          rcBtn.top=midY-   (SIZE_ICONV/2);
          rcBtn.bottom=midY+(SIZE_ICONV/2);

          rcBtn.left=midX- (SIZE_ICONH/2);
          rcBtn.right=midX+(SIZE_ICONH/2);
          }
        else
          {
          rcBtn.left+=(sizeIndent*(l->Level()+1))-(SIZE_ICONH/2);
          rcBtn.right=rcBtn.left+SIZE_ICONH;
          }
        if (lRet || PtInRect(&rcBtn,ptMouse))
          {
          if (l->IsExpanded())
            ON_KEYDOWN (VK_SUBTRACT,0);
          else
            ON_KEYDOWN (VK_ADD,0);
          }
        }
      break;
      }
    }
}

LRESULT LinkedListWindow::ON_LBUTTONDOWN   (WPARAM wParam,LPARAM lParam)
{
    POINT pt;
#ifdef WIN32
    LONG2POINT(lParam, pt);
#else
    pt = MAKEPOINT(lParam);
#endif
  MouseClick (pt, FALSE);
  return 0;
}

LRESULT LinkedListWindow::ON_TIMER (WPARAM wParam,LPARAM lParam)
{
  Link *l=current=firstvisible;

  switch (wParam)
    {
    case 1:
      l = Prev();
      if (l)
        {
        ChangeSelection(l,TRUE);
        ON_VSCROLL (SB_LINEUP,0);
        }
      break;

    case 2:
      {
      for (int i=0;l && i<nbVisible;i++,l=Next());
      if (l)
        {
        ChangeSelection(l,TRUE);
        ON_VSCROLL (SB_LINEDOWN,0);
        }
      }
      break;

    case 3:
      ON_KEYDOWN (VK_PRIOR,0);
      break;

    case 4:
      ON_KEYDOWN (VK_NEXT,0);
      break;

    }
  return 0;
}

LRESULT LinkedListWindow::ON_LBUTTONUP   (WPARAM wParam,LPARAM lParam)
{
  inDrag=0;
  KillTimer(hWnd,1);
  KillTimer(hWnd,2);
  KillTimer(hWnd,3);
  KillTimer(hWnd,4);

  // Emb 18/4/95 for drag/drop management
  if (inDragDrop)
    {
    SendMessage(hWndOwner, LM_NOTIFY_ENDDRAGDROP, (WPARAM)hWnd, 0L);
    inDragDrop = 0;
    }

  return 0;
}

//
// Emb 18/4/95 for drag/drop management
// returns the item behind the mouse, or 0L if none
//
LRESULT LinkedListWindow::ON_GETMOUSEITEM (WPARAM wParam,LPARAM lParam)
{
  RECT  rc;
  POINT pt;
#ifdef WIN32
    LONG2POINT(lParam, pt);
#else
    pt=MAKEPOINT(lParam);
#endif
  
  Link * behindTheMouse=0;

  if(pt.y>=0 && pt.y<=ptSizeWin.y)
    for (Link * l=current=firstvisible;l && !behindTheMouse ;l=Next())
      if (GetItemRect (l,&rc) && pt.y>=rc.top && pt.y<rc.bottom)
        behindTheMouse=l;

  if (behindTheMouse)
    return behindTheMouse->Item();
  else
    return 0L;
}

//
// Emb 18/4/95 for drag/drop management
// Cancels the dragdrop mode in progress
//
LRESULT LinkedListWindow::ON_CANCELDRAGDROP (WPARAM wParam,LPARAM lParam)
{
  inDragDrop = 0;       // No more drag/drop
  return 0L;            // any value acceptable
}

LRESULT LinkedListWindow::ON_MOUSEMOVE   (WPARAM wParam,LPARAM lParam)
{
  RECT  rc;
  POINT pt;
#ifdef WIN32
    LONG2POINT(lParam, pt);
#else
  pt=MAKEPOINT(lParam);
#endif
  Link * behindTheMouse=0;

  // Emb 22/6/95 : we constantly receive mousemoves even though
  // the mouse is not being moved...
  // don't do anything if not moved!
  if (pt.x == lastpt.x && pt.y == lastpt.y)
    return 0;
  lastpt.x = pt.x;
  lastpt.y = pt.y;

  if(pt.y>=0 && pt.y<=ptSizeWin.y)
    for (Link * l=current=firstvisible;l && !behindTheMouse ;l=Next())
      if (GetItemRect (l,&rc) && pt.y>=rc.top && pt.y<rc.bottom)
        behindTheMouse=l;

  if (GetCapture()==hWnd)
    {
    KillTimer(hWnd,1);
    KillTimer(hWnd,2);
    KillTimer(hWnd,3);
    KillTimer(hWnd,4);
    inDrag=0;

    if (pt.y<0 && !inDragDrop)
      {
      int numTimer=pt.y< (sizeItem*(-3)) ? 3 : 1;
      SetTimer(hWnd,numTimer,50,0);
      inDrag=1;
      return 0;
      }

    if (pt.y>ptSizeWin.y && !inDragDrop)
      {
      int numTimer=pt.y>(ptSizeWin.y+sizeItem*3) ? 4 : 2;
      SetTimer(hWnd,numTimer,50,0);
      inDrag=1;
      return 0;
      }

    if (behindTheMouse && behindTheMouse!=cursel)
      {
      if (!inDragDrop)
        {
        ChangeSelection   (behindTheMouse,FALSE);
        return 0;
        }
    }
  }


  // Emb 18/4/95 for drag/drop management
  if (inDragDrop)
    SendMessage(hWndOwner, LM_NOTIFY_DRAGDROP_MOUSEMOVE, (WPARAM)hWnd, lParam);
  else
    {
    if (behindTheMouse && (wParam & MK_LBUTTON) )
      {
      if (wParam & MK_CONTROL)
        DragDropMode = 1;   // Copy
      else 
        DragDropMode = 0;   // Move

      // set variable BEFORE SendMessage, since we can receive
      // a LM_CANCELDRAGDROP that will reset the variable
      // while processing the message!
      inDragDrop = TRUE;
      SendMessage(hWndOwner, LM_NOTIFY_STARTDRAGDROP,
                  DragDropMode, behindTheMouse->Item());
      }
    }

  if (behindTheMouse && !inDragDrop)
    {
    if ((curMessage!=behindTheMouse)||(GetWindowLong(hWnd,GWL_STYLE)&LMS_ALLONITEMMESSAGES))
      {
      curMessage=behindTheMouse;
      PostMessage(hWndOwner,LM_NOTIFY_ONITEM,(WPARAM)hWnd,curMessage->Item());
      }
    }
  return 0;
}

LRESULT LinkedListWindow::ON_LBUTTONDBLCLK(WPARAM wParam,LPARAM lParam)
{
  RECT  rc;
  POINT pt;
#ifdef WIN32
    LONG2POINT(lParam, pt);
#else
    pt=MAKEPOINT(lParam);
#endif
  for (Link *l =current=firstvisible;l;l=Next())
    if (GetItemRect (l, &rc) && PtInRect(&rc,pt))
      {
      SendMessage(hWndOwner,LM_NOTIFY_LBUTTONDBLCLK,(WPARAM)hWnd,l->Item());
      break;
      }
  return 0;
}


LRESULT LinkedListWindow::ON_RBUTTONDBLCLK(WPARAM wParam,LPARAM lParam)
{
  RECT  rc;
  POINT pt;
#ifdef WIN32
    LONG2POINT(lParam, pt);
#else
    pt=MAKEPOINT(lParam);
#endif
  for (Link *l =current=firstvisible;l;l=Next())
    if (GetItemRect (l, &rc) && PtInRect(&rc,pt))
      {
      SendMessage(hWndOwner,LM_NOTIFY_RBUTTONDBLCLK,(WPARAM)hWnd,l->Item());
      break;
      }
  return 0;
}


LRESULT LinkedListWindow::ON_RBUTTONDOWN(WPARAM wParam,LPARAM lParam)
{
  POINT pt;
#ifdef WIN32
    LONG2POINT(lParam, pt);
#else
    pt=MAKEPOINT(lParam);
#endif

  MouseClick (pt,TRUE);

  return 0;
}

LRESULT LinkedListWindow::ON_SETFONT      (WPARAM wParam,LPARAM lParam)
{
  HFONT hOldFont=hFont;
  hFont=(HFONT)wParam;
  GetExtents();

  for (Link * l=first; l; l=l->Next())
    l->strExtent=0;
  InvalidateRect(hWnd,NULL,FALSE);
  if (hWndEdit)
    {
    SetWindowFont(hWndEdit, wParam, TRUE);
    EditMove();
    }
  return  (LRESULT)hOldFont;
}

LRESULT LinkedListWindow::ON_GETFONT      (WPARAM wParam,LPARAM lParam)
{
  return (LRESULT)hFont;
}

//
// Modified 11/7/95 Emb : if received wParam is 0xabcd, pass it along
// to ON_KEYDOWN
//
// Used to force synchronous notification message LM_NOTIFY_EXPAND
// when expanding branch with sub-branch
//
// Was already implemented some weeks ago, but modification lost
// at hand-made merge time...
//
LRESULT LinkedListWindow::ON_EXPAND      (WPARAM wParam,LPARAM lParam)
{
  Link *l= lParam ? Find(lParam) : cursel;

  if (!l)
    return 0;

  if (l->IsExpanded())
    return 0;

  Link *old= cursel;
  cursel = l;
  if (wParam == 0xabcd)
    ON_KEYDOWN (VK_ADD, 0xabcd);
  else
    ON_KEYDOWN (VK_ADD, 0);
  cursel = old;
  return 1;
}

LRESULT LinkedListWindow::ON_COLLAPSE      (WPARAM wParam,LPARAM lParam)
{
  Link *l= lParam ? Find(lParam,TRUE) : cursel;

  if (!l)
    return 0;

  if (!l->IsExpanded())
    return 0;
  Link *old= cursel;
  cursel = l;
  if (wParam == 0xdcba)
    ON_KEYDOWN (VK_SUBTRACT, 0xdcba);
  else
    ON_KEYDOWN (VK_SUBTRACT,0);
  cursel = old;
  return 1;
}

LRESULT LinkedListWindow::ON_EXPANDALL  (WPARAM wParam,LPARAM lParam)
{
  BOOL bChanged=FALSE;
  SetVisible(cursel);

  if (bDisableExpandAll)
    return 1;

  for (Link *l=current=First(); l; l=l->Next())
    if (!l->IsExpanded() && l->IsParent())
      {
      bChanged=TRUE;
      ToggleMode(l);
      PostMessage(hWndOwner,LM_NOTIFY_EXPAND,(WPARAM)hWnd,l->Item());
      }
  current=cursel;
  if (bChanged)
    {
    bDirty|=0x01;
    InvalidateRect(hWnd,NULL,FALSE);
    }
  return 1;
}

LRESULT LinkedListWindow::ON_COLLAPSEALL (WPARAM wParam,LPARAM lParam)
{
  BOOL bChanged=FALSE;
  Link *oldSel;

  if (bEditChanged && !SendMessage(hWndOwner,LM_NOTIFY_STRINGHASCHANGE,(WPARAM)hWnd,cursel->Item()))
    RetrieveEditString();

  for (Link *l=current=First(); l; l=l->Next())
    {
    if (l->IsExpanded())
      {
      if (l->IsParent())
        {
        ToggleMode(l);
        bChanged=TRUE;
        SendMessage(hWndOwner,LM_NOTIFY_COLLAPSE,(WPARAM)hWnd,l->Item());
        }
      else
        l->ToggleMode();
      }
    }
  oldSel=current=cursel;
  if (bChanged)
    {
    bDirty|=0x01;
    while( cursel->Parent())
      cursel=cursel->Parent();
    SetVisible(cursel);
    if (oldSel!=cursel && IsWindow(hWndEdit))
      {
      EditNotify(TRUE);
      }
    InvalidateRect(hWnd,NULL,FALSE);
    }
  return 1;
}

LRESULT LinkedListWindow::ON_EXPANDBRANCH (WPARAM wParam,LPARAM lParam)
{
  Link *l= lParam ? Find(lParam) : cursel;
  if (!lParam)
    SetVisible(l);
  if (SetExpOneLevel(l,TRUE))
    {
    RECT rc;
    bDirty|=0x01;

    if (GetItemRect(l,&rc))
      {
      rc.bottom=ptSizeWin.y;
      InvalidateRect(hWnd,&rc,FALSE);
      }
    else
      InvalidateRect(hWnd,NULL,FALSE);
    }

  return 1;
}

LRESULT LinkedListWindow::ON_COLLAPSEBRANCH (WPARAM wParam,LPARAM lParam)
{
  Link *l= lParam ? Find(lParam,TRUE) : cursel;
  Link *oldSel=cursel;

  if (bEditChanged && !SendMessage(hWndOwner,LM_NOTIFY_STRINGHASCHANGE,(WPARAM)hWnd,cursel->Item()))
    RetrieveEditString();

  if (!lParam)
    SetVisible(l);
  if (SetExpOneLevel(l,FALSE))
    {
    RECT rc;
    bDirty|=0x01;
    if (GetItemRect(l,&rc))
      {
      rc.bottom=ptSizeWin.y;
      InvalidateRect(hWnd,&rc,FALSE);
      }
    else
      InvalidateRect(hWnd,NULL,FALSE);
    }

 if (oldSel!=cursel && IsWindow(hWndEdit))
   {
   EditNotify(TRUE);
   }
  return 1;
}


LRESULT LinkedListWindow::ON_GETSTRING (WPARAM wParam,LPARAM lParam)
{
  Link *l= lParam ? Find(lParam) : cursel;

  if (l==cursel && hWndEdit && bEditChanged)
    return (LRESULT) (LPSTR) RetrieveEditString();
  else
    return (LRESULT) (LPSTR) GetString(l);
}

LRESULT LinkedListWindow::ON_SETSTRING (WPARAM wParam,LPARAM lParam)
{
  LPSETSTRING lpS=(LPSETSTRING)lParam;
  if (lpS)
    {
    Link *l= lpS->ulRecID ? Find(lpS->ulRecID) : cursel;
    if (l->SetNewString(lpS->lpString))
      {
      RECT rc;
      bDirty|=0x01;
      if (l==cursel && IsWindow(hWndEdit))
        SetWindowText(hWndEdit,lpS->lpString);
      if (GetItemRect(l,&rc))
        InvalidateRect(hWnd,&rc,FALSE);
      return TRUE;
      }
    }
  return FALSE;
}

LRESULT LinkedListWindow::ON_GETITEMDATA (WPARAM wParam,LPARAM lParam)
{
  Link *l= lParam ? Find(lParam) : cursel;
  return ( l ? (LRESULT) (LPVOID) l->GetItemData() : 0);
}

LRESULT LinkedListWindow::ON_SETITEMDATA (WPARAM wParam,LPARAM lParam)
{
  LPSETITEMDATA lpI = (LPSETITEMDATA) lParam;
  if (lpI && lpI->ulRecID)
    {
    Link *l= lpI->ulRecID ? Find(lpI->ulRecID) : cursel;
    return ( l ? (LRESULT) (LPVOID) l->SetItemData(lpI->lpItemData) : 0);
    }
  return 0;
}


LRESULT LinkedListWindow::ON_GETFIRST (WPARAM wParam,LPARAM lParam)
{
  curAsk=first;
  return (curAsk ? curAsk->Item() : 0 );
}

LRESULT LinkedListWindow::ON_GETPREV (WPARAM wParam,LPARAM lParam)
{
  Link *l = lParam  ? Find(lParam) : cursel;
  if (!l || !l->Prev())
    return 0;
  return l->Prev()->Item();
}

LRESULT LinkedListWindow::ON_GETNEXT (WPARAM wParam,LPARAM lParam)
{
  Link *l = lParam  ? Find(lParam) : curAsk;

  if (!l || !l->Next())
    return 0;

  curAsk=l->Next();
  return curAsk->Item();
}

LRESULT LinkedListWindow::ON_GETPARENT (WPARAM wParam,LPARAM lParam)
{
  Link *l= Find(lParam);
  return ( (l && l->Parent()) ? l->Parent()->Item() : 0 );
}

LRESULT LinkedListWindow::ON_GETLEVEL (WPARAM wParam,LPARAM lParam)
{
  Link *l= Find(lParam);
  return l ? l->Level() : 0 ;
}

void LinkedListWindow::RetrievePaintInfos(LPLMDRAWITEMSTRUCT lpLM)
{
 if (lpLM)
   {
   lpLM->hWndItem=hWnd;
   lpLM->ulID=curAsk->Item();
   lpLM->lpString=GetString(curAsk);
   lpLM->uiLevel=curAsk->Level();
   lpLM->bExpanded=(uiPrintFlag&PRINT_ALL)? TRUE : curAsk->IsExpanded();
   lpLM->lpiPosLines=(LPINT)curAsk->GetData();
   lpLM->bParent=curAsk->IsParent();

   CalcTabStop(lpLM->hDC,&lpLM->iTabStopSize);


   if (lpLM->uiItemState&LMPRINTSELECTONLY)
     {
     lpLM->uiLevel-=cursel->Level();
     if (curAsk==cursel)
       lpLM->lpiPosLines=NULL;
     else if (lpLM->lpiPosLines)
      lpLM->lpiPosLines+=cursel->Level();
     lpLM->uiItemState=ITEMSTATEDRAWENTIRE|LMPRINTMODE|LMPRINTSELECTONLY;
     }
   else
     lpLM->uiItemState=ITEMSTATEDRAWENTIRE|LMPRINTMODE;
   }
}

LRESULT LinkedListWindow::ON_GETFIRSTPRESENT (WPARAM wParam,LPARAM lParam)
{
  LPLMDRAWITEMSTRUCT lpDraw =(LPLMDRAWITEMSTRUCT) lParam;
  ON_PAINT (wParam, lParam); //UKS add, March 1' 1996
  if (lpDraw)
    {
    curAsk= (lpDraw->uiItemState&LMPRINTSELECTONLY) ? cursel : first;
    if (curAsk)
      {
      RetrievePaintInfos(lpDraw);
      if (wParam)
        {
        ULONG lRet=0;
        if (ownerDraw)
          lRet=SendMessage(hWndOwner,LM_DRAWITEM,0,lParam);
        if (lRet!=LMDRAWALL)
          PaintVODrawStyle(lpDraw,lRet,0,0,iIconsMargin);
        }
      }
    return (curAsk ? curAsk->Item() : 0 );
    }
  return 0;
}

LRESULT LinkedListWindow::ON_GETEDITHANDLE  (WPARAM wParam,LPARAM lParam)
{
 return (LRESULT) hWndEdit;
}

LRESULT LinkedListWindow::ON_GETNEXTPRESENT (WPARAM wParam,LPARAM lParam)
{
  LPLMDRAWITEMSTRUCT lpDraw =(LPLMDRAWITEMSTRUCT) lParam;
  if (lpDraw)
    {
    if (curAsk)
      current=curAsk;
    if ((uiPrintFlag&PRINT_ALL) && curAsk)
      curAsk=curAsk->Next();
    else
      curAsk=Next();
    if (curAsk)
      {
      if (lpDraw->uiItemState&LMPRINTSELECTONLY)
        if (curAsk->Level()<=cursel->Level())
          return 0;
      RetrievePaintInfos((LPLMDRAWITEMSTRUCT) lParam);
      if (wParam)
        {
        ULONG            lRet=0;
        if (ownerDraw)
          lRet=SendMessage(hWndOwner,LM_DRAWITEM,0,lParam);
        if (lRet!=LMDRAWALL)
          PaintVODrawStyle((LPLMDRAWITEMSTRUCT)lParam,lRet,0,0,iIconsMargin);
        }
      }
    return (curAsk ? curAsk->Item() : 0 );
    }
  return 0;
}


LRESULT LinkedListWindow::ON_ITEMFROMPOINT (WPARAM wParam,LPARAM lParam)
{
  POINT pt;
#ifdef WIN32
    LONG2POINT(lParam, pt);
#else
    pt=MAKEPOINT(lParam);
#endif

  if (pt.y>=0 && pt.y<=ptSizeWin.y && pt.x>=0 && pt.x<=ptSizeWin.x && firstvisible) 
    {
    for (Link *l = current = firstvisible;l; l =Next())
      {
      RECT  rc;
      if (GetItemRect(l,&rc) && PtInRect(&rc,pt))
        return l->Item();
      }
    }
  return 0;
}



LRESULT LinkedListWindow::ON_GETFIRSTCHILD (WPARAM wParam,LPARAM lParam)
{
  Link *l = lParam  ? Find(lParam) : cursel;
  if (l && l->IsParent() && l->Next() && l->Next()->Parent()==l)
    return l->Next()->Item();
  return 0;
}


LRESULT LinkedListWindow::ON_EXISTSTRING (WPARAM wParam,LPARAM lParam)
{
  LPSTR lpStr=(LPSTR) lParam;
  for (Link *l =first; l ; l=l->Next())
    if (GetString(l) && (_tcscmp(GetString(l),lpStr)==0))
      return 1;
  return 0;
}


LRESULT LinkedListWindow::ON_SETBKCOLOR (WPARAM wParam,LPARAM lParam)
{
  COLORREF oldColor=bkColor;
  bkColor=(COLORREF)lParam;
  InvalidateRect(hWnd,NULL,FALSE);
  return oldColor;
}


LRESULT LinkedListWindow::ON_GETSTATE (WPARAM wParam,LPARAM lParam)
{
  Link *l = Find(lParam);
  if (!l)
    return (LRESULT) -1;
  return l->IsExpanded();
}

// Emmanuel B. 13/2/95 : says whether the line is candidate
// for visibility (none of it's parents are collapsed)
LRESULT LinkedListWindow::ON_GETVISSTATE (WPARAM wParam,LPARAM lParam)
{
  Link *l = Find(lParam);
  if (!l)
    return (LRESULT) -1;
  return l->IsVisible();
}


LRESULT LinkedListWindow::ON_GETMAXEXTENT (WPARAM wParam,LPARAM lParam)
{
  return MAKELONG(maxExtent,sizeItem);
}


LRESULT LinkedListWindow::ON_FINDFROMITEMDATA (WPARAM wParam,LPARAM lParam)
{
  for (Link *l =first; l; l=l->Next())
    if (l->GetItemData()==(void *) lParam)
      return l->Item();
  return 0;
}


LRESULT LinkedListWindow::ON_GETTOPITEM (WPARAM wParam,LPARAM lParam)
{
  return firstvisible ? (LRESULT) firstvisible->Item() : 0;
}

LRESULT LinkedListWindow::ON_SETTOPITEM (WPARAM wParam,LPARAM lParam)
{
  Link *l = Find(lParam);
  if (l && firstvisible!=l && l->IsVisible() )
    {
    firstvisible=l;
    VerifyScroll();
    InvalidateRect(hWnd,NULL,FALSE);
    if (hWndEdit)
      {
      RECT rc;
      if (!GetItemRect(cursel,&rc))
        MoveWindow(hWndEdit,0,0,0,0,TRUE);
      }
    return TRUE;
    }
  return FALSE;
}

LRESULT LinkedListWindow::ON_FINDNEXT(WPARAM wParam,LPARAM lParam)
{
  if (cursel && cursel->Next())
    {
    return lParam ? FindFromString(cursel->Next(),(LPSTR) lParam,wParam) : FindFromString(cursel->Next());
    }
  return 0;
}

LRESULT LinkedListWindow::ON_FINDFIRST(WPARAM wParam,LPARAM lParam)
{
  return (first&&lParam) ? FindFromString(first,(LPSTR) lParam,wParam) : 0;
}

LRESULT LinkedListWindow::ON_GETSEARCHSTRING (WPARAM wParam,LPARAM lParam)
{
  return (LRESULT) pStrToSearch;
}

LRESULT LinkedListWindow::ON_GETITEMRECT (WPARAM wParam,LPARAM lParam)
{
  LPITEMRECT lp = (LPITEMRECT) lParam;
  Link * l      = Find(lp->ulRecID);
  if (l && GetItemRect (l,&lp->rect))
    {
    if (VODrawStyle)
      {
      lp->rect.top+=  DISTANCE;
      lp->rect.left+= sizeIndent*(l->Level())+DISTANCE;
      lp->rect.right-=DISTANCE;
      }
    else
      {
      lp->rect.left+=sizeIndent*(l->Level()+2);
      }
    return TRUE;
    }
  memset((char *) &lp->rect,0,sizeof(lp->rect));
  return FALSE;
}

LRESULT LinkedListWindow::ON_SETTABSTOP(WPARAM wParam,LPARAM lParam)
{
  int iTemp=iTabStopSize;
  iTabStopSize=(int)wParam;
  if (iTabStopSize<0)
    iTabStopSize=0;
  bDirty|=0x01;

  for (Link *l=First();l;l=Next())
    l->strExtent=0;

  InvalidateRect(hWnd,NULL,FALSE);
  return iTemp;
}

LRESULT LinkedListWindow::ON_GETSTYLE(WPARAM wParam,LPARAM lParam)
{
	return VODrawStyle;
}

LRESULT LinkedListWindow::ON_SETSTYLE(WPARAM wParam,LPARAM lParam)
{
  WORD wOldStyle=VODrawStyle;

  VODrawStyle=wParam;
  GetExtents();               // Emb 30/5/95 : update line item height
  InvalidateRect(hWnd,NULL,FALSE);
  return wOldStyle;
}

LRESULT LinkedListWindow::ON_SETPRINTFLAGS(WPARAM wParam,LPARAM lParam)
{
  uiPrintFlag=wParam;
  return TRUE;
}

LRESULT LinkedListWindow::ON_GETCOUNT(WPARAM wParam,LPARAM lParam)
{
	Link * 	l;
	LRESULT	lRet=0L;

	switch(wParam)
	{
		case COUNT_ALL:
			for (l =first; l ; l=l->Next())
				lRet++;
			break;

		case COUNT_VISIBLE:
			lRet=Count();
			break;

		case COUNT_DISPLAYABLE:
			lRet=nbVisible;
			break;
	}
	return lRet;
}


LRESULT LinkedListWindow::ON_SETICONSPACING(WPARAM wSpace, LPARAM lParam)  
{
  iIconsMargin = (int)wSpace;
  return TRUE;

}

LRESULT LinkedListWindow::ON_DISABLEEXPANDALL(WPARAM wParam,LPARAM lParam)
{
  BOOL oldValue=bDisableExpandAll;
  bDisableExpandAll=wParam;
  return oldValue;
}


LRESULT LinkedListWindow::ON_ADDRECORDBEFORE(WPARAM wParam,LPARAM lParam)
{
  LPLINK_RECORD lpLR=(LPLINK_RECORD) lParam;
  if (lpLR)
    {
    if (!lpLR->ulRecID)
      return 0;

    Link *lCurrent=Add(lpLR->ulRecID,lpLR->ulParentID,lpLR->lpString,lpLR->lpItemData,0,lpLR->ulInsertAfter);

    if (lCurrent)
      {
      bDirty|=0x02;
      if (!firstvisible)
        firstvisible=lCurrent;
      if (!cursel)
        {
        Link * newSel=lCurrent;
        ChangeSelection (newSel,TRUE);
        EditNotify(TRUE);
        }
      InvalidateRect(hWnd,NULL,FALSE);
      return 1;
      }
    }
  return 0;
}


LRESULT LinkedListWindow::ON_GETEDITCHANGES(WPARAM wParam,LPARAM lParam)
{
  EditNotify(FALSE); 
  return 0;
}

// Debug Emb 26/6/95 : performance measurement data
LRESULT LinkedListWindow::ON_RESET_DEBUG_EMBCOUNTS(WPARAM wParam,LPARAM lParam)
{
  ResetDebugEmbCounts();
  return 0;
}
