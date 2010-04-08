//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
//
// PAN/LCM source control information:
//
//  Archive name    ^[Archive:j:\vo4clip\lbtree\arch\winlink.cpp]
//  Modifier        ^[Author:rumew01(1600)]
//  Base rev        ^[BaseRev:None]
//  Latest rev      ^[Revision:1.0 ]
//  Latest rev time ^[Timestamp:Thu Aug 18 16:12:10 1994]
//  Check in time   ^[Date:Thu Aug 18 16:51:53 1994]
//  Change log      ^[Mods:// ]
// 
// Thu Aug 18 16:12:10 1994, Rev 1.0, rumew01(1600)
//  init
// 
// Fri Apr 29 16:36:32 1994, Rev 48.0, sauje01(0500)
//  Add LM_DISABLEEXPANDALL message
// 
// Fri Mar 18 15:30:48 1994, Rev 47.0, sauje01(0500)
// 
// Fri Jan 07 17:31:06 1994, Rev 46.0, sauje01(0500)
// 
// Wed Jan 05 14:40:14 1994, Rev 45.0, sauje01(0500)
// 
// Tue Jan 04 17:13:54 1994, Rev 44.0, sauje01(0500)
// 
// Mon Jan 03 16:22:08 1994, Rev 43.0, sauje01(0500)
// 
// Mon Dec 20 16:49:08 1993, Rev 42.0, sauje01(0500)
// 
// Mon Dec 20 15:50:46 1993, Rev 41.0, sauje01(0500)
// 
// Mon Dec 20 15:23:44 1993, Rev 40.0, sauje01(0500)
// 
// Wed Nov 24 16:46:50 1993, Rev 39.0, sauje01(0500)
// 
// Wed Nov 24 16:25:14 1993, Rev 38.0, sauje01(0500)
// 
// Wed Nov 24 16:14:38 1993, Rev 37.0, sauje01(0500)
// 
// Wed Nov 24 16:06:26 1993, Rev 36.0, sauje01(0500)
// 
// Wed Nov 24 15:50:08 1993, Rev 35.0, sauje01(0500)
// 
// Sat Oct 30 13:40:56 1993, Rev 34.0, sauje01(0500)
// 
// 27-Sep-2000 (schph01)
//    bug #99242 . cleanup for DBCS compliance
// 01-Nov-2001 (hanje04)
//    Moved declaration of work outside for loop to resolve non-ANSI errors
//    on Linux.
//
//////////////////////////////////////////////////////////////////////////////
#include "lbtree.h"

extern "C"
{
#include <windows.h>
#include <windowsx.h>
#include <memory.h>
#include <string.h>
#include <malloc.h>
#include <tchar.h>
#include "winlink.h"
#include "icons.h"
#include "compat.h"
#include "cm.h"
}

static char * StrChrExt(char *p,char *pc) /* 28-Sep-2000 (schph01) changed prototype because of DBCS constrainst*/
{
  int w1=(int) (DWORD) _tcsupr(pc);
  while(*p)
    {
    int w2=(int) (DWORD) _tcsupr(p);
    if (w1==w2)
      return p;
    CMnext(p);
    }
  return 0;
}

static int StrCmpExt(char *pSrc,char *pDst)
{
  while (*pDst)
    {
    switch(*pSrc)
      {
      case _T('\0'):
        return (*pDst);

      case _T('?'):
        break;

      case _T('*'):
        {
        CMnext(pSrc);
        if (!(*pSrc))
          return 0;
        while (*pDst)
          {
          pDst = StrChrExt(pDst,pSrc);
          if (!pDst)
            return *pSrc;
          if (!StrCmpExt(pSrc,pDst))
            return 0;
          CMnext(pDst);
          }
        return *pSrc;
        }
        break;

      default:
        {
        int w1=(int) (DWORD) _tcsupr(pSrc);
        int w2=(int) (DWORD) _tcsupr(pDst);
        if (w1!=w2)
          return (w1-w2);
        }
        break;
      }
    CMnext(pSrc);
    CMnext(pDst);
    }

  while (*pSrc && (*pSrc==_T('*') || *pSrc==_T('?')))
    {
    CMnext(pSrc);
    }

  return (*pSrc);
}

int CalcScrollPos(ULONG nElem,ULONG nFirstVisible)
{
  if (nElem && nFirstVisible)
    {
    int iRet=MulDiv((int)nFirstVisible,MAX_SCROLL,(int)nElem);
    if (iRet>MAX_SCROLL)
      iRet=MAX_SCROLL;
    else if (iRet<0)
      iRet=0;
    return (iRet);
    }
  return (0);
}

ULONG CalcThumbPos(ULONG nElem,ULONG thumbPosition)
{
  int iRet=MulDiv((int)thumbPosition,(int)nElem,MAX_SCROLL);
  if (iRet>MAX_SCROLL)
    iRet=MAX_SCROLL;
  else if (iRet<0)
    iRet=0;
  return ((ULONG) iRet);
}


ULONG   LinkedListWindow::GetIndex  (Link * l)
{
  ULONG lRes=0;
  for (Link *pl=First();pl && pl!=l;lRes++,pl=Next());
  return lRes;
}

BOOL LinkedListWindow::GetItemRect (Link * pl, LPRECT lpRC)
{
  lpRC->left=-GetScrollPos(hWnd,SB_HORZ);
  lpRC->right=ptSizeWin.x;
  lpRC->top=0;
  lpRC->bottom=0;

  for (Link *l =current=firstvisible;l;l=Next())
    {
    lpRC->top=lpRC->bottom;
    lpRC->bottom+=sizeItem;
    if (lpRC->top>ptSizeWin.y)
      break;
    if (l==pl)
      return TRUE;
    }
  return FALSE;
}

void LinkedListWindow::SetVisible(Link *l)
{
  RECT rc;
  if (l && cursel)
    {
    current=cursel;
    if (!GetItemRect(l,&rc))
      {
      firstvisible=l;
      VerifyScroll();
      InvalidateRect(hWnd,NULL,FALSE);
      }
    }
}

void LinkedListWindow::VerifyScroll()
{
  BOOL bEnable=((int)Count())>nbVisible;

  if (firstvisible!=first)
    {
    Link *save=current;
    int nbStr=0;
    for (Link *l=current=firstvisible;l && nbStr<nbVisible ;l=Next())
      nbStr++;
    if (nbStr<nbVisible)
      {
      current=firstvisible;
      for (int i=nbVisible-nbStr;i;i--)
        if (firstvisible!=first)
          firstvisible=Prev();
      InvalidateRect(hWnd,NULL,FALSE);
      }
    current=save;
    }

  SetScrollPos(hWnd,SB_VERT,(int) CalcScrollPos( bEnable ? Count()-nbVisible : 0 ,GetIndex(firstvisible)),TRUE);

  if (GetWindowLong(hWnd,GWL_STYLE)&LMS_AUTOVSCROLL)
    ShowScrollBar(hWnd, SB_VERT, bEnable);
  else
    {
#if WINVER>0x300
    EnableScrollBar(hWnd, SB_VERT, bEnable ? ESB_ENABLE_BOTH : ESB_DISABLE_BOTH);
#endif
    }
}

void LinkedListWindow:: VerifyScrollH()
{
  int  iPos=GetScrollPos(hWnd,SB_HORZ);
  int  iRange=maxExtent-ptSizeWin.x;
  BOOL bRedraw=FALSE;
  BOOL bEnable=iRange>1;


  if (iPos+ptSizeWin.x>maxExtent)
    {
    bRedraw=TRUE;
    iPos=maxExtent-ptSizeWin.x;
    if (iPos<0)
      iPos=0;
    }
  if (iRange<1)
    {
    bRedraw= iPos ? TRUE : FALSE;
    iRange=1;
    }

  // Test added Emb 14/2/95 : don't set if the window has
  // autoscroll style with bEnable not set
  if (bEnable || !(GetWindowLong(hWnd, GWL_STYLE) & LMS_AUTOHSCROLL) )
    {
    SetScrollRange(hWnd,SB_HORZ,0,iRange,FALSE);
    SetScrollPos(hWnd,SB_HORZ,iPos,TRUE);
    }

  if (bRedraw)
    InvalidateRect(hWnd,NULL,FALSE);

  if (GetWindowLong(hWnd,GWL_STYLE)&LMS_AUTOHSCROLL)
    {
      SetWindowRedraw(hWnd, FALSE);
      ShowScrollBar(hWnd, SB_HORZ, bEnable);
      SetWindowRedraw(hWnd, TRUE);
      InvalidateRect(hWnd, NULL, FALSE);
    }
  else
    {
#if WINVER>0x300
    EnableScrollBar(hWnd, SB_HORZ, bEnable ? ESB_ENABLE_BOTH : ESB_DISABLE_BOTH);
#endif
    }

}

void LinkedListWindow:: RetrieveLines ()
{
  if (!(bDirty&0x04))
    {
    HDC hDC=GetDC(hWnd);
    if (hDC)
      {
      int iTabStop = 0;
      HFONT hOld=0;
      maxExtent=1;

      if (iTabStopSize && !VODrawStyle)
        CalcTabStop(hDC,&iTabStop);


      if (hFont)
        hOld=(HFONT)SelectObject(hDC,hFont);
      for (Link *work=First();work ;work=Next())
        {
        int iSizeStr=(work->Level()+3)*sizeIndent;
        if (VODrawStyle)
          iSizeStr+=sizeIndent;

        if (iIconsMargin)
          iSizeStr+=iIconsMargin+POSITION_TEXT;

        if (!work->strExtent)
          {
          char *p=GetString(work);
          if (p)
            {
            LPSTR lpTab =_tcschr(p, _T('\t'));

            if (lpTab && (VODrawStyle || iTabStop))
              {
              CMnext(lpTab);
              if (VODrawStyle)
                {
                work->strExtent=LOWORD(GetTextExtent(hDC,lpTab,_tcslen(lpTab)))+(sizeIndent*3);
                }
              else
                {
                work->strExtent=LOWORD(GetTextExtent(hDC,lpTab,_tcslen(lpTab)))+iTabStop;
                }
              }
            else
              {
              work->strExtent=LOWORD(GetTextExtent(hDC,p,_tcslen(p)));
              }
            }
          }
        iSizeStr+=work->strExtent;
        if (iSizeStr>maxExtent)
          maxExtent=iSizeStr;
        }
      SelectObject(hDC,hOld ? hOld : GetStockObject(SYSTEM_FONT));
      ReleaseDC(hWnd,hDC);
      }
    }

  if (bDirty&0x02)
    {
    // FNN Feb 13, 1995 loop(s) were rewritten for speed reasons
    // revized Emb 27/6/95 for more speed
    Link * lastwork;
    Link * work;
    for (work=first; work ;work=work->Next())
      {
      int  wlev = work->Level();
      LPINT pint=wlev ? new int[wlev] : 0;
      if (pint)
        memset( (char *) pint,0,sizeof(int) * wlev);
      work->SetData(pint);
      lastwork=work;
      }

    #ifdef DEBUGEMB
    // Debug Emb 27/6/95 : performance measurement data
    long cptDebugEmbNext = 0;
    long cptDebugEmbPrev = 0;
    #endif

    #ifdef DEBUGEMB
    for (work=first; work ;work=work->Next(), cptDebugEmbNext++)
    #else
    for (work=first; work ;work=work->Next())
    #endif
      {
        int wlev = work->Level();
        BOOL bLast = TRUE;
        Link *l;

        // optimize : don't start from lastwork, but from last who
        // has a parent relation with the current item
        #ifdef DEBUGEMB
        for (l=work->Next(); l; l=l->Next(), cptDebugEmbNext++)
        #else
        for (l=work->Next(); l; l=l->Next())
        #endif
          if (l->ParentLevel(work)==0)
            break;

        // old code without the optimize
        //for (l =lastwork;l!=work;l=l->Prev()) {

        if (!l)
          l = lastwork;
        #ifdef DEBUGEMB
        for (   ;l!=work;l=l->Prev(), cptDebugEmbPrev++) {
        #else
        for (   ;l!=work;l=l->Prev()) {
        #endif
          int lev1=l->ParentLevel(work);
          if (lev1==1) {
            if (bLast) {
              l->SetInt(wlev,3);
              bLast = FALSE;
            }
            else 
              l->SetInt(wlev,2);
          }
          if (lev1>1 && !bLast) 
            l->SetInt(wlev,1);
        }
      }

    #ifdef DEBUGEMB
    // Debug Emb 27/6/95 : performance measurement data
    cptDebugEmbNext = 0L;
    cptDebugEmbPrev = 0L;
    #endif

    // FNN Feb 13, 1995 end of modification
    }

  bDirty=0;
  VerifyScroll();
  VerifyScrollH();
}

void Linked_List::Clear()
{
  if (!firstNonResolved)
    return;

  for (LinkNonResolved *l2 =firstNonResolved; l2 ; l2= (LinkNonResolved *) l2->Next())
    if (!l2->bDeleted)
      return;

  for (LinkNonResolved *l =firstNonResolved; l ; l= (LinkNonResolved *) l->Next())
    {
    if (l->bDeleted)
      {
      if (l==firstNonResolved)
        firstNonResolved=(LinkNonResolved *)l->Next();
      delete(l);
      }
    }
}

char FAR * LinkedListWindow::RetrieveEditString(void)
{
 char * lpNewStr=GetString(cursel);
 int    iStrlen=Edit_GetTextLength(hWndEdit);
 if (iStrlen>=0)
   {
   lpNewStr = new char[iStrlen * sizeof(TCHAR) + sizeof(TCHAR)];
   if (lpNewStr)
     {
     if (Edit_GetText(hWndEdit, lpNewStr, iStrlen+1)<=0L)
       *lpNewStr=0;
     cursel->SetString(lpNewStr);
     RetrieveLines ();
     }
   }
 return lpNewStr;
}

void LinkedListWindow::EditMove(void)
{
 RECT rc;
 if (GetItemRect (cursel, &rc))
   {
   RECT rcWin;
   if (VODrawStyle)
     {
     rcWin.left   = rc.left+(sizeIndent*(cursel->Level()+1))+DISTANCE+2;
     rcWin.right  = rc.right-(DISTANCE+1);
     rcWin.top    = rc.top+DISTANCE+1;
     rcWin.bottom = rc.bottom-1;
     }
   else
     {
     rcWin.left   = rc.left+(sizeIndent*(cursel->Level()+2));
     rcWin.right  = rc.right;
     rcWin.top    = rc.top;
     rcWin.bottom = rc.bottom;
     }
   MoveWindow(hWndEdit,rcWin.left,rcWin.top,rcWin.right-rcWin.left,rcWin.bottom-rcWin.top,TRUE);
   }
 else
   MoveWindow(hWndEdit,0,0,0,0,TRUE);
}


void LinkedListWindow::NotifyReturn(void)
{
  if (!hWndEdit || !IsWindow(hWndEdit) || !cursel)
    return;
  if (bEditChanged && !SendMessage(hWndOwner,LM_NOTIFY_STRINGHASCHANGE,(WPARAM)hWnd,cursel->Item()))
    RetrieveEditString();
  bEditChanged=FALSE;
}

void LinkedListWindow::EditNotify(BOOL bNewItem)
{
  if (!hWndEdit || !IsWindow(hWndEdit) || !cursel)
    return;
  if (!bNewItem)
    {
    if (bEditChanged && !SendMessage(hWndOwner,LM_NOTIFY_STRINGHASCHANGE,(WPARAM)hWnd,cursel->Item()))
      RetrieveEditString();
    ShowWindow(hWndEdit,SW_HIDE);
    }
  else
    {
    char *p=GetString(cursel);
    if (!p)
      {
      char c=0;
      p=&c;
      }
    Edit_SetText(hWndEdit, p);
    Edit_SetSel(hWndEdit, 0, -1);
    EditMove();
    ShowWindow(hWndEdit,SW_NORMAL);
    SetFocus(hWndEdit);
    }
  bEditChanged=FALSE;
}

void LinkedListWindow::CalcTabStop(HDC hDC,int * lpIPixelSize)
{
  if (iTabStopSize)
    {
    TEXTMETRIC tm;
    GetTextMetrics(hDC,&tm);
    *lpIPixelSize=iTabStopSize*tm.tmAveCharWidth;
    return;
    }
  *lpIPixelSize=0;
}

LRESULT LinkedListWindow::ChangeSelection   (Link * lNew,BOOL bRepaint)
{
  LRESULT bRet=0;
  if (lNew!=cursel)
    {
    if (lNew)
      PostMessage(hWndOwner,LM_NOTIFY_SELCHANGE, (WPARAM)hWnd,lNew->Item());
    if (bDirty && !bRepaint)
      RetrieveLines ();

    HDC hDC=GetDC(hWnd);

    if (hDC)
      {
      LMDRAWITEMSTRUCT lmStruct;
      ULONG            lRet=0;
      HFONT            hOld=0;

      if (hFont)
        hOld=(HFONT)SelectObject(hDC,hFont);

      CalcTabStop(hDC,&lmStruct.iTabStopSize);

      lmStruct.hDC          =hDC;
      lmStruct.hWndItem     =hWnd;
      lmStruct.uiIndentSize =sizeIndent;
      
      if (GetItemRect (cursel, &lmStruct.rcItem))  
        {
        if (bRepaint)
          InvalidateRect(hWnd,&lmStruct.rcItem,FALSE);
        else
          {
          lmStruct.ulID         =cursel->Item();
          lmStruct.lpString     =GetString(cursel); 
          lmStruct.uiLevel      =cursel->Level();
          lmStruct.bExpanded    =cursel->IsExpanded();
          lmStruct.bParent      =cursel->IsParent();
          lmStruct.lpiPosLines  =(LPINT)cursel->GetData();
          lmStruct.uiItemState  =ITEMSTATENORMAL;    
          lmStruct.rcPaint      =lmStruct.rcItem;

          lmStruct.textColor = GetSysColor(COLOR_WINDOWTEXT);
          lmStruct.bkColor   = bkColor;
          

          if (ownerDraw)
            lRet=SendMessage(hWndOwner,LM_DRAWITEM,(WPARAM)hWnd,(LPARAM) (LPLMDRAWITEMSTRUCT) &lmStruct);
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
          }
        }
      if (GetItemRect (lNew, &lmStruct.rcItem))  
        {
        if (bRepaint)
          InvalidateRect(hWnd,&lmStruct.rcItem,FALSE);
        else
          {
          lmStruct.ulID         =lNew->Item();
          lmStruct.lpString     =GetString(lNew); 
          lmStruct.uiLevel      =lNew->Level();
          lmStruct.bExpanded    =lNew->IsExpanded();
          lmStruct.bParent      =lNew->IsParent();
          lmStruct.lpiPosLines  =(LPINT)lNew->GetData();
          lmStruct.uiItemState  =ITEMSTATENORMAL;    
          lmStruct.rcPaint      =lmStruct.rcItem;



          if (!hWndEdit||!IsWindowVisible(hWndEdit))
            lmStruct.uiItemState|=ITEMSTATESELECTED;

          if ((!bPaintSelectOnFocus||(GetFocus()==hWnd)) && (!hWndEdit||!IsWindowVisible(hWndEdit)))
            lmStruct.uiItemState|=ITEMSTATEFOCUS;

          lmStruct.textColor = GetSysColor(COLOR_WINDOWTEXT);
          lmStruct.bkColor   = bkColor;

          if (ownerDraw)
            lRet=SendMessage(hWndOwner,LM_DRAWITEM,(WPARAM)hWnd,(LPARAM) (LPLMDRAWITEMSTRUCT) &lmStruct);
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
          }
        }
      SelectObject(hDC,hOld ? hOld : GetStockObject(SYSTEM_FONT));
      ReleaseDC(hWnd,hDC);
      }
    EditNotify(FALSE); 
    cursel=lNew;
    EditNotify(TRUE); 
    bRet=TRUE;
    }
  if (curMessage!=cursel)
    {
    curMessage=cursel;
    PostMessage(hWndOwner,LM_NOTIFY_ONITEM,(WPARAM)hWnd,curMessage->Item());
    }
  return bRet;
}

char FAR * LinkedListWindow::GetString (Link * l)
{
  if (!l)
    return 0;

  char FAR * p=l->String();
  if (!p)
    p=(char FAR * )SendMessage(hWndOwner,LM_QUERYSTRING,(WPARAM)hWnd,(LPARAM) (LPLMDRAWITEMSTRUCT) l->Item());
  if (!p)
    p=pszGlobalDefaultString;
  return p;
}


void PaintItem(LPLMDRAWITEMSTRUCT lm,DWORD ulFlags,HICON hIcon1,HICON hIcon2,int iIconsMargin)
{
  RECT    rcInter;
    
  unsigned int uiIndentation=lm->uiIndentSize*(lm->uiLevel+2);
  if ((lm->uiItemState & ITEMSTATEDRAWENTIRE) && !(ulFlags&LMDRAWLINES) ) 
    {
    RECT    rc;
    HBRUSH  hBrush;
    unsigned int     i;
    int     iLinePos;
    int     iRight;

    rc.top    =lm->rcItem.top;
    rc.bottom =lm->rcItem.bottom;
    rc.left   =lm->rcItem.left;
    rc.right  =rc.left+uiIndentation;


    SetTextColor(lm->hDC,lm->textColor);

    if (IntersectRect(&rcInter,&rc,&lm->rcPaint))
      {
      hBrush=CreateSolidBrush(lm->bkColor);
      if (hBrush)
        {
        FillRect(lm->hDC,&rcInter,hBrush);
        DeleteObject(hBrush);
        }
      }

    iLinePos =rc.left;

    iRight = (lm->bParent &&lm->bExpanded ) ? rc.right-lm->uiIndentSize : rc.right-2;
    for (i=0 ;i<lm->uiLevel;i++)
      {
      iLinePos+=lm->uiIndentSize;

      switch (*(lm->lpiPosLines+i))
        {
        case 1:
          MoveToEx (lm->hDC, iLinePos,rc.top, NULL);
          LineTo (lm->hDC, iLinePos,rc.bottom);
          break;

        case 2: 
          {
          int iVertPos=(rc.top+rc.bottom)/2;

          MoveToEx (lm->hDC, iLinePos,rc.top, NULL);
          LineTo (lm->hDC, iLinePos,rc.bottom);

          MoveToEx (lm->hDC, iLinePos ,iVertPos, NULL);
          LineTo (lm->hDC, iRight   ,iVertPos);

          }
          break;

        case 3:
          {
          int iVertPos=(rc.top+rc.bottom)/2;

          MoveToEx (lm->hDC, iLinePos,rc.top, NULL);
          LineTo (lm->hDC, iLinePos,iVertPos);

          MoveToEx (lm->hDC, iLinePos ,iVertPos, NULL);
          LineTo (lm->hDC, iRight   ,iVertPos);
          }
          break;

        default:
          break;
        }
      }

    if (lm->bParent)
      {
      int x,y;
      int iVertPos=(rc.top+rc.bottom)/2;
      int iLinePos=rc.right-lm->uiIndentSize;

      if (lm->bExpanded )
        {
        MoveToEx (lm->hDC, iLinePos,iVertPos, NULL );
        LineTo (lm->hDC, iLinePos,rc.bottom);
        }

      if (IntersectRect(&rcInter,&rc,&lm->rcPaint))
        {
        if (lm->uiItemState & LMPRINTMODE)
          {
          char c= (lm->bExpanded ? _T('-') : _T('+'));
          TextOut(lm->hDC,iLinePos-(lm->uiIndentSize/8),lm->rcItem.top,(LPSTR)&c,1);
          }
        else
          {
          HICON hIcon= LoadIcon(ghDllInstance,MAKEINTRESOURCE(lm->bExpanded ? IDI_ICON_MINUS2 :IDI_ICON_PLUS2 ));
          if (hIcon)
            {
            x=iLinePos-(SIZE_ICONH/2);
            y=iVertPos-(GetSystemMetrics(SM_CYICON)/2);
            DrawIcon(lm->hDC,x,y,hIcon);
            }
          }
        }
      }
    }

  if (!ulFlags&LMDRAWSTRING) 
    {
    RECT    rc;
    HBRUSH  hBrush;

    rc.top=lm->rcItem.top;
    rc.bottom=lm->rcItem.bottom;

    if (lm->lpString)
      {
      int iStrLen=_tcslen(lm->lpString)*sizeof(TCHAR);
//
//RPB - Removed in leiu of the SETICONSPACING message
//      int iIconsMargin = (GetWindowLong(lm->hWndItem,GWL_STYLE)&LMS_WITHICONS) ?  POSITION_TEXT : 0;


      LPSTR lpDuplicate = (LPSTR) GlobalAllocPtr(GHND, iStrLen+sizeof(TCHAR));
      if (lpDuplicate)
        {
        _tcscpy(lpDuplicate,lm->lpString);
        LPSTR lpTab =_tcschr(lpDuplicate, _T('\t'));

        if (lpTab)
          {
          if(lm->iTabStopSize)
            iStrLen=lpTab-lpDuplicate;
          else
            *lpTab=_T(' ');
          }

        rc.left   =lm->rcItem.left+uiIndentation;

        if (lm->iTabStopSize && lpTab)
          rc.right  =rc.left+lm->iTabStopSize;
        else
//
//RPB - Modify position so that SETICONSPACING can effect the entire line
//          rc.right  =rc.left+LOWORD(GetTextExtent(lm->hDC,lpDuplicate,iStrLen))+1+iIconsMargin;
          rc.right  =rc.left+LOWORD(GetTextExtent(lm->hDC,lpDuplicate,iStrLen))+1+
		(2*iIconsMargin) + (iIconsMargin ? POSITION_TEXT : 0);

        SetTextColor(lm->hDC,lm->uiItemState&ITEMSTATESELECTED ? GetSysColor (COLOR_HIGHLIGHTTEXT) : lm->textColor );
        SetBkColor  (lm->hDC,lm->uiItemState&ITEMSTATESELECTED ? GetSysColor (COLOR_HIGHLIGHT    ) : lm->bkColor   );

        if (IntersectRect(&rcInter,&rc,&lm->rcPaint))
          {
//
//RPB - Following line replaced to allow dynamic positioning of icons
//          ExtTextOut(lm->hDC,rc.left+iIconsMargin,rc.top,ETO_OPAQUE|ETO_CLIPPED,&rc,lpDuplicate,iStrLen,NULL);
          ExtTextOut(lm->hDC,rc.left+(2*iIconsMargin)+(iIconsMargin ? POSITION_TEXT : 0),rc.top,ETO_OPAQUE|ETO_CLIPPED,&rc,lpDuplicate,iStrLen,NULL);
          if (iIconsMargin)
            {
            int iVertPos=((rc.top+rc.bottom)/2)-OFFSETVERT_ICON;
            if (hIcon1)
              DrawIcon(lm->hDC,rc.left+POSITION_FIRST_ICON,iVertPos ,hIcon1);
            if (hIcon2)
              DrawIcon(lm->hDC,rc.left+POSITION_SECOND_ICON+iIconsMargin,iVertPos,hIcon2);
//
//RPB - Following line replace by one above to allow dynamic positioning
//      via the SETICONSPACING message
//              DrawIcon(lm->hDC,rc.left+POSITION_SECOND_ICON,iVertPos,hIcon2);
            }
          }

          
        if (lm->iTabStopSize && lpTab)
          {
          RECT newRC;
          CMnext(lpTab);
          newRC.top=rc.top;
          newRC.bottom=rc.bottom;
          newRC.left=rc.right;
          newRC.right=rc.right+LOWORD(GetTextExtent(lm->hDC,lpTab,_tcslen(lpTab)))+1;
          rc.right=newRC.right;
          if (IntersectRect(&rcInter,&newRC,&lm->rcPaint))
            ExtTextOut(lm->hDC,newRC.left,newRC.top,ETO_OPAQUE|ETO_CLIPPED,&newRC,lpTab,_tcslen(lpTab),NULL);
          }
        GlobalFreePtr(lpDuplicate);
        }
      }
    else // no string, fill the entire rectangle
      rc.left=rc.right=0;

    if (lm->uiItemState & ITEMSTATEFOCUS)
      DrawFocusRect(lm->hDC,&rc);

    // fill right edge

    hBrush=CreateSolidBrush(lm->bkColor);
    if (hBrush)
      {
      rc.left=rc.right;
      rc.right=lm->rcItem.right;
      if (IntersectRect(&rcInter,&rc,&lm->rcPaint))
        FillRect(lm->hDC,&rc,hBrush);
      DeleteObject(hBrush);
      }
    }
}

static void __inline VODrawTabString(HDC hDC,LPRECT lpRC,int iTxtPos,LPSTR lpStr,LPSTR lpTab,BOOL bFocus,int indentSize)
{
  DWORD dwOldTxtColor;
  int   oldBKMode;
  RECT  rc=*lpRC;

  rc.right=iTxtPos+indentSize;

  dwOldTxtColor=SetTextColor(hDC,bFocus ? RGB(0,0,0) : RGB(255,255,255));
  ExtTextOut(hDC,iTxtPos,rc.top,ETO_OPAQUE|ETO_CLIPPED,&rc,lpStr,lpTab-lpStr,NULL);
  oldBKMode=SetBkMode(hDC,TRANSPARENT);
  SetTextColor(hDC,bFocus ?  RGB(255,255,255) : RGB(0,0,0));
  ExtTextOut(hDC,iTxtPos+1,rc.top+1,ETO_CLIPPED,lpRC,lpStr,lpTab-lpStr,NULL);
  SetTextColor(hDC,dwOldTxtColor);

  SetBkMode(hDC,oldBKMode);
  rc.left=rc.right;
  rc.right=lpRC->right;
  CMnext(lpTab);
  ExtTextOut(hDC,rc.left,rc.top,ETO_OPAQUE|ETO_CLIPPED,&rc,lpTab,_tcslen(lpTab),NULL);

}

void PaintVODrawStyle(LPLMDRAWITEMSTRUCT lm,DWORD ulFlags,HICON hIcon1,HICON hIcon2,int iIconsMargin)
{
  DWORD dwStyle=GetWindowLong(lm->hWndItem,GWL_STYLE);
//  int iIconsMargin = (GetWindowLong(lm->hWndItem,GWL_STYLE)&LMS_WITHICONS) ?  POSITION_TEXT : 0;

  unsigned int uiIndentation=(lm->uiIndentSize*(lm->uiLevel))+DISTANCE;
  RECT  rcText;
  RECT  rcInter;

  rcText.top    = lm->rcItem.top+DISTANCE+1;
  rcText.bottom = lm->rcItem.bottom-1;
  rcText.left   = lm->rcItem.left+uiIndentation+1;

  if (lm->uiItemState & LMPRINTMODE)
    {
    UINT  uiStrExtent;
    LPSTR lpTab =_tcschr(lm->lpString, _T('\t'));

    // Problems with some printer drivers
    if (rcText.top>=rcText.bottom)
      {
      rcText.top=lm->rcItem.top;
      rcText.bottom=lm->rcItem.bottom;
      }

    if (lpTab)
      {
      CMnext(lpTab);
//
//RPB - replace following line to allow dynamic postioning of icons
//      uiStrExtent=LOWORD(GetTextExtent(lm->hDC,lpTab,_tcslen(lpTab)))+(lm->uiIndentSize*3)+iIconsMargin;
      uiStrExtent=LOWORD(GetTextExtent(lm->hDC,lpTab,_tcslen(lpTab)))+(lm->uiIndentSize*3)+(2*iIconsMargin)+(iIconsMargin ? POSITION_TEXT : 0);
      }
    else
//
//RPB - replace following line to allow dynamic postioning of icons
//      uiStrExtent=LOWORD(GetTextExtent(lm->hDC,lm->lpString,_tcslen(lm->lpString)))+iIconsMargin;
      uiStrExtent=LOWORD(GetTextExtent(lm->hDC,lm->lpString,_tcslen(lm->lpString)))+(2*iIconsMargin)+(iIconsMargin ? POSITION_TEXT : 0);
    rcText.right = rcText.left+uiStrExtent+(lm->uiIndentSize*2);
    }
  else
    rcText.right  = lm->rcItem.right-(DISTANCE+1);

  if (!ulFlags&LMDRAWSTRING) 
    {
    SetTextColor(lm->hDC,RGB(0,0,0));
    SetBkColor  (lm->hDC,GetSysColor( ((lm->uiItemState&ITEMSTATEFOCUS) &&  (lm->uiItemState&ITEMSTATESELECTED)) ? COLOR_BTNSHADOW  : COLOR_BTNFACE));
    if (lm->lpString)
      {
      if (IntersectRect(&rcInter,&rcText,&lm->rcPaint))
        {
        LPSTR lpTab =_tcschr(lm->lpString, _T('\t'));

        if (lpTab)
//RPB
//          VODrawTabString(lm->hDC,&rcText,rcText.left+lm->uiIndentSize+1+iIconsMargin,lm->lpString,lpTab,((lm->uiItemState&ITEMSTATEFOCUS) &&  (lm->uiItemState&ITEMSTATESELECTED)),lm->uiIndentSize*3);
          VODrawTabString(lm->hDC,&rcText,rcText.left+lm->uiIndentSize+1+(2*iIconsMargin)+(iIconsMargin ? POSITION_TEXT : 0),lm->lpString,lpTab,((lm->uiItemState&ITEMSTATEFOCUS) &&  (lm->uiItemState&ITEMSTATESELECTED)),lm->uiIndentSize*3);
        else
//          ExtTextOut(lm->hDC,rcText.left+lm->uiIndentSize+1+iIconsMargin,rcText.top,ETO_OPAQUE|ETO_CLIPPED,&rcText,lm->lpString,_tcslen(lm->lpString),NULL);
          ExtTextOut(lm->hDC,rcText.left+lm->uiIndentSize+1+(2*iIconsMargin)+(iIconsMargin ? POSITION_TEXT : 0),rcText.top,ETO_OPAQUE|ETO_CLIPPED,&rcText,lm->lpString,_tcslen(lm->lpString),NULL);

        if (lm->bParent)
          {
          if (lm->uiItemState & LMPRINTMODE)
            {
            char c= (lm->bExpanded ? _T('-') : _T('+'));
            TextOut(lm->hDC,rcText.left+ (lm->uiIndentSize/2),lm->rcItem.top+DISTANCE+1,(LPSTR)&c,1);
            }
          else
            {
            HICON hIcon= LoadIcon(ghDllInstance,MAKEINTRESOURCE(lm->bExpanded ? IDI_ICON_MINUS :IDI_ICON_PLUS ));
            if (hIcon)
              {
              int x=rcText.left+(lm->uiIndentSize/2)-(SIZE_ICONH/2);
              int y=(rcText.top+rcText.bottom-GetSystemMetrics(SM_CYICON))/2;
              DrawIcon(lm->hDC,x,y,hIcon);
              }
            }
          }

        if (iIconsMargin)
          {
          int iVertPos=((rcText.top+rcText.bottom)/2)-OFFSETVERT_ICON;
          if (hIcon1)
            DrawIcon(lm->hDC,rcText.left+lm->uiIndentSize+1+POSITION_FIRST_ICON,iVertPos ,hIcon1);
          if (hIcon2)
            DrawIcon(lm->hDC,rcText.left+lm->uiIndentSize+1+iIconsMargin+POSITION_SECOND_ICON,iVertPos,hIcon2);
          }
        }
      }
    }

  if ((lm->uiItemState & ITEMSTATEDRAWENTIRE) || (!ulFlags&LMDRAWSTRING))
    {
    HPEN   hPenBlack= CreatePen(PS_SOLID,1,RGB(0,0,0));
    HPEN   hPenWhite= CreatePen(PS_SOLID,1,GetSysColor(COLOR_BTNHIGHLIGHT));
    HPEN   hOldPen  = 0;
    int    iLinePos = 0;
    RECT   rcClip;
    BOOL   bRestoreClip=FALSE;
    UINT   uiMargin;

    if (lm->uiItemState&LMPRINTMODE)
      { 
      uiMargin=(lm->rcItem.bottom-lm->rcItem.top)/2;
      if (!uiMargin)
        uiMargin=1;
      }
    else
      uiMargin=0;

    // Draw shadow
    InflateRect(&rcText,1,1);

    //hOldPen= SelectObject(lm->hDC,((lm->uiItemState & LMPRINTMODE)||(lm->uiItemState&ITEMSTATESELECTED)) ? hPenBlack : hPenWhite);
    hOldPen= (HPEN)SelectObject(lm->hDC,(lm->uiItemState & LMPRINTMODE) ? hPenBlack : hPenWhite);

    MoveToEx (lm->hDC, rcText.left ,rcText.top, NULL);
    LineTo (lm->hDC, rcText.right,rcText.top);

    MoveToEx (lm->hDC, rcText.left ,rcText.top, NULL);
    LineTo (lm->hDC, rcText.left ,rcText.bottom);

//    SelectObject(lm->hDC,((lm->uiItemState & LMPRINTMODE)||(!(lm->uiItemState&ITEMSTATESELECTED))) ?  hPenBlack : hPenWhite);
    SelectObject(lm->hDC,hPenBlack);

    MoveToEx (lm->hDC, rcText.right-1 ,rcText.top+1, NULL);
    LineTo (lm->hDC, rcText.right-1 ,rcText.bottom);

    MoveToEx (lm->hDC, rcText.left+1  ,rcText.bottom-1, NULL);
    LineTo (lm->hDC, rcText.right   ,rcText.bottom-1);


    if (!(lm->uiItemState&LMPRINTMODE))
      {
      HBRUSH hBr  = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
      bRestoreClip=(GetClipBox(lm->hDC,&rcClip)!=ERROR);

      ExcludeClipRect(lm->hDC,rcText.left,rcText.top,rcText.right,rcText.bottom);
      // Fill backgound
      if (hBr)
        {
        if (!(ulFlags&LMDRAWLINES))
          {
          FillRect(lm->hDC,&lm->rcItem,hBr);
          DeleteObject(hBr);
          }
        }
      }

    // Draw Lines

    if (!(ulFlags&LMDRAWLINES))
      {
      SelectObject(lm->hDC,hPenBlack);
      iLinePos = lm->rcItem.left+DISTANCE + (lm->uiIndentSize/2);

      for (unsigned int i=0 ;i<lm->uiLevel;i++,iLinePos+=lm->uiIndentSize)
        {
        switch (*(lm->lpiPosLines+i))
          {
          case 1:
            MoveToEx (lm->hDC, iLinePos,lm->rcItem.top-uiMargin, NULL);
            LineTo (lm->hDC, iLinePos,lm->rcItem.bottom);
            break;

          case 2: 
            {
            int iVertPos=(lm->rcItem.top+lm->rcItem.bottom+DISTANCE)/2;
            MoveToEx (lm->hDC, iLinePos,lm->rcItem.top-uiMargin, NULL);
            LineTo (lm->hDC, iLinePos,lm->rcItem.bottom);
            MoveToEx (lm->hDC, iLinePos ,iVertPos, NULL);
            LineTo (lm->hDC, rcText.left ,iVertPos);
            }
            break;

          case 3:
            {
            int iVertPos=(lm->rcItem.top+lm->rcItem.bottom+DISTANCE)/2;

            MoveToEx (lm->hDC, iLinePos,lm->rcItem.top-uiMargin, NULL);
            LineTo (lm->hDC, iLinePos,iVertPos);
            MoveToEx (lm->hDC, iLinePos ,iVertPos, NULL);
            LineTo (lm->hDC, rcText.left,iVertPos);
            }
            break;
          }
        }

      SelectObject(lm->hDC,hOldPen ? hOldPen : GetStockObject(BLACK_PEN));
      }

    if (hPenBlack)
      DeleteObject(hPenBlack);
    if (hPenWhite)
      DeleteObject(hPenWhite);

    if (bRestoreClip)
      {
      HRGN hRgn=CreateRectRgnIndirect(&rcClip);
      if (hRgn)
        {
        SelectObject(lm->hDC,hRgn);
        DeleteObject(hRgn);
        }
      }
    if (lm->uiItemState & LMPRINTMODE)
      {
      HBRUSH hBrush=CreateSolidBrush(RGB(192,192,192));
      if (hBrush) 
        {
        RECT rc;
        uiMargin  = (uiMargin+1)/2;
        rc.top    = rcText.bottom;
        rc.bottom = rcText.bottom+uiMargin;
        rc.left   = rcText.left+uiMargin;
        rc.right  = rcText.right;
        FillRect(lm->hDC,&rc,hBrush);
        rc.top    = rcText.top+uiMargin;
        rc.bottom = rcText.bottom+uiMargin;
        rc.left   = rcText.right;
        rc.right  = rcText.right+uiMargin;
        FillRect(lm->hDC,&rc,hBrush);
        DeleteObject(hBrush);
        }
      }
    }

}

BOOL LinkedListWindow::SetExpOneLevel(Link * pParent,BOOL bExpanded)
{
  BOOL bRet=FALSE;
  for (Link *l=pParent;l;l=l->Next())
    {
    if (l->Parent()==pParent || l==pParent)
      {
      if ( ((l->IsExpanded() && !bExpanded) || (!l->IsExpanded() && bExpanded)) )
        {
        bRet=TRUE;
        ToggleMode(l);
        if (l->IsParent())
          if (bExpanded)
            PostMessage(hWndOwner, LM_NOTIFY_EXPAND,(WPARAM)hWnd,l->Item());
          else 
          SendMessage(hWndOwner, LM_NOTIFY_COLLAPSE,(WPARAM)hWnd,l->Item());
        }
      if (l->Parent()==pParent)
        bRet|=SetExpOneLevel(l,bExpanded);
      }
    }
  return bRet;
}

void LinkedListWindow::GetExtents()
{
  DWORD dwExtent=0;

  if (ownerDraw)
    dwExtent=SendMessage(hWndOwner,LM_MEASUREITEM,(WPARAM)hWnd,0L);

  if (!dwExtent)
    {
    HDC hDC=GetDC(hWnd);
    if (hDC)
      {
      char  c=VODrawStyle ? _T('0') : _T(' ');
      DWORD dw;

      HFONT hOldFont=hFont ? (HFONT) SelectObject(hDC,hFont) : (HFONT)0;
      dw=GetTextExtent(hDC,&c,1);
      dwExtent=MAKELONG(LOWORD(dw)*3,HIWORD(dw)+1);
      if (hOldFont)
        SelectObject(hDC,hOldFont);
      ReleaseDC(hWnd,hDC);
      }
    }
  sizeItem = HIWORD(dwExtent); if (sizeItem<SIZE_ICONV) sizeItem=SIZE_ICONV;
  sizeIndent=LOWORD(dwExtent); if (sizeItem<SIZE_ICONH) sizeIndent=SIZE_ICONH;
  if (VODrawStyle)
    sizeItem+=(DISTANCE+2);
  bDirty|=0x01;
  nbVisible=(ptSizeWin.y)/sizeItem;
}

LONG LinkedListWindow::FindFromString(Link * From)
{
  if (pStrToSearch)
    {
    for (Link *l = From; l ; l=l->Next())
      {
      BOOL   bDuplicate = FALSE;
      char * pStr      = GetString(l);
      char * pToSearch = pStrToSearch;

      if (pStr)
        {
        char *p= _tcschr(pStr,_T('\t'));
        if (p)
          pStr=_tcsinc(p);
        }
      // Quick and dirty

      if (!bSearchMatch && (StrCmpExt(pToSearch,pStr)!=0))
        {
        pToSearch = new char [(_tcslen(pStrToSearch)*sizeof(TCHAR))+(3*sizeof(TCHAR))];
        if (pToSearch)
          {
          bDuplicate =TRUE;
          _tcscpy(pToSearch,_T("*"));
          _tcscat(pToSearch,pStrToSearch);
          _tcscat(pToSearch,_T("*"));
          }
        }
      if (pToSearch && (StrCmpExt(pToSearch,pStr)==0))
        {
        if (!l->IsVisible())
          {
          Link * work; 
          for (work= l; work->Parent();work=work->Parent());
          ON_EXPANDBRANCH (0,work->Item());
          }
        ON_SETSEL (0,l->Item());
        if (bDuplicate)
          delete[]pToSearch;
        return l->Item();
        }
      if (bDuplicate)
        delete[]pToSearch;
      }
    }
  return 0;
}

LONG LinkedListWindow::FindFromString(Link * From,char * pStr,BOOL bMatch)
{
  bSearchMatch = bMatch;
  if (pStrToSearch)
    delete [] pStrToSearch;
  pStrToSearch = new char [(_tcslen(pStr)*sizeof(TCHAR))+ sizeof(TCHAR)];
  if (pStrToSearch)
    _tcscpy(pStrToSearch,pStr);

  return FindFromString(From);
}


LinkedListWindow::LinkedListWindow(HWND hWindow)  
{
  hWnd=hWindow;
  hWndOwner=GetParent(hWindow);
  DWORD dwStyle=GetWindowLong(hWnd,GWL_STYLE);

  iIconsMargin = (dwStyle&LMS_WITHICONS) ?  1 : 0;

  bkColor      = GetSysColor(COLOR_WINDOW);

  pStrToSearch = 0;
  bSearchMatch = 0;
  iTabStopSize = 0;
  uiPrintFlag  =0;

  bDisableExpandAll=FALSE;

  hWndEdit=0;
  bEditChanged=FALSE;
  lpOldEditProc=0;
  hFont=0;
  ownerDraw =0;
  VODrawStyle=0;
  firstvisible=0;
  bPaintSelectOnFocus =0;
  cursel=0;
  curAsk=0;
  inDrag=0;
  curMessage=0;
  bDirty=3;
  maxExtent=1;
  ptSizeWin.x=ptSizeWin.y=0;
  ptPete.x=ptPete.y=0;

  bSort = (dwStyle & LMS_SORT) ? 1 : 0;

  // Emb 18/4/95 for drag/drop management
  inDragDrop = 0;
  DragDropMode = 0;
}

LinkedListWindow::~LinkedListWindow() 
{
  if (pStrToSearch)
    delete [] pStrToSearch;
  SetWindowLong(hWnd,0,0);
}



