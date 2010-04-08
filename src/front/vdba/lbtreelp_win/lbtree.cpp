//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
//
// PAN/LCM source control information:
//
//  Archive name    ^[Archive:j:\vo4clip\lbtree\arch\lbtree.cpp]
//  Modifier        ^[Author:rumew01(1600)]
//  Base rev        ^[BaseRev:None]
//  Latest rev      ^[Revision:1.0 ]
//  Latest rev time ^[Timestamp:Thu Aug 18 16:12:10 1994]
//  Check in time   ^[Date:Thu Aug 18 16:51:38 1994]
//  Change log      ^[Mods:// ]
// 
// Thu Aug 18 16:12:10 1994, Rev 1.0, rumew01(1600)
//  init
// 
// Wed Jan 26 10:09:44 1994, Rev 24.0, sauje01(0500)
// 
// Tue Jan 25 13:52:40 1994, Rev 23.0, sauje01(0500)
// 
// Mon Jan 24 12:30:18 1994, Rev 22.0, sauje01(0500)
// 
// Fri Jan 21 15:15:40 1994, Rev 21.0, sauje01(0500)
// 
// Fri Jan 07 17:31:06 1994, Rev 20.0, sauje01(0500)
// 
// Wed Jan 05 14:40:14 1994, Rev 19.0, sauje01(0500)
// 
// Mon Jan 03 16:35:22 1994, Rev 18.0, sauje01(0500)
// 
// Wed Nov 24 16:42:22 1993, Rev 17.0, sauje01(0500)
// 
// Wed Nov 24 15:33:28 1993, Rev 16.0, sauje01(0500)
// 
// Thu Nov 04 18:31:08 1993, Rev 15.0, sauje01(0500)
// 
// Thu Nov 04 17:22:10 1993, Rev 14.0, sauje01(0500)
// 
// Sat Oct 30 13:40:56 1993, Rev 13.0, sauje01(0500)
// 
// 27-Sep-2000 (schph01)
//    bug #99242 . cleanup for DBCS compliance
//
//////////////////////////////////////////////////////////////////////////////
#include  "lbtree.h"

extern "C"
{
#include "compat.h"
#include "cm.h"
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <tchar.h>
}


static int __inline IsBefore(char *p1 ,char *p2)
{
  char *p;
  if (!p1 || !p2)
    return 0;
  for (p=p1; *p ; CMnext(p))
    {
    if (_istalpha (*p))
      {
      p1=p;
      break;
      }
    }
  for (p=p2; *p ; CMnext(p))
    {
    if (_istalpha (*p))
      {
      p2=p;
      break;
      }
    }
  return (_tcsicmp (p1,p2)<0);
}

LinkNonResolved::LinkNonResolved (unsigned long ul,Link * prev,char * p,unsigned long ulParent,void * lpItemData) : Link(ul,0,prev,0,p,lpItemData) 
{ 
  lparentitemdata=ulParent;
  bDeleted=0;
}

Link::Link(unsigned long ID ,Link *pParent,Link * insertAfter,Link * insertBefore,char * pString,void * lpItemData)
{
  parent=pParent;
  litemdata=ID;
  expanded=1;
  prev=next=0;
  string=0;
  hasChild=0;
  pdata=0;
  strExtent=0;
  pExternItemData=lpItemData;
  if (insertBefore)
    {
    next=insertBefore;
    prev=insertBefore->prev;
    insertBefore->prev=this;
    if (prev)
      prev->next=this;
    }
  else if (insertAfter)
    {
    if (insertAfter->Parent()==pParent && insertAfter->IsParent() && insertAfter->next)
      {
      while(insertAfter->next)
        {
        if (!insertAfter->next->Parent() && pParent)
          break;
        if (insertAfter->next->Parent() && pParent && (insertAfter->next->Parent()->Level()<pParent->Level()))
          break;
        if ((insertAfter->next->Parent()==pParent))
          break;
        else
          insertAfter=insertAfter->next;
        }
      }
    prev=insertAfter;
    next=insertAfter->next;
    insertAfter->next=this;
    if (next)
      next->prev=this;
    }
  if (pString && (string = new char [_tcslen(pString)*sizeof(TCHAR)+sizeof(TCHAR)]))
    _tcscpy(string,pString);
}

Link::~Link()
{
  if (prev)
    prev->next=next;
  if (next)
    next->prev=prev;
  if (string)
    delete string;
}

int Link::ToggleMode() 
{ 
  expanded=!expanded;
  return (!expanded);
}

int Link::Level() 
{ 
  int ret=0;
  for (Link *l=this;l->Parent();l=l->Parent())
    ret++;
  return (ret);
}

int Link::IsVisible() 
{ 
  for (Link *l=parent;l;l=l->Parent())
    if (!l->IsExpanded())
      return 0;
  return 1;
}

int Link::SetNewString(char *p)
{
 if (p)
  {
  delete string;
  if (string = new char [_tcslen(p)*sizeof(TCHAR)+sizeof(TCHAR)] )
    {
    _tcscpy(string,p);
    return 1;
    }
  }
 return 0;
}

Link *  Linked_List::Add(unsigned long ID,unsigned long parentID,char *pStr, void * lpItemData,unsigned long uiInsertAfter,unsigned long uiInsertBefore)
{
  Link * insertAfter=last;
  Link * insertBefore=0;
  Link * parent=0;

  if (uiInsertAfter)
    {
    Link *l =Find(uiInsertAfter);
    if (l)
      insertAfter=l;
    }

  if (parentID)
    {
    if (lastparent && lastparent->Item()==parentID)
      {
      parent=insertAfter=lastparent;
      }
    else
      {
      // Emb 28/6/95 : performance improvement using Find()
      Link * l = Find(parentID);
      if (l)
        parent=lastparent=insertAfter=l;

      // original code, masqued :
      //for (Link *l=first; l && !parent;l=l->Next())
      //  {
      //  if (l->Item()==parentID)
      //    {
      //    parent=lastparent=insertAfter=l;
      //    }
      //  }
      }
    if (parent)
      {
      if (!parent->hasChild)
        {
        uiInsertAfter=parent->Item();
        parent->hasChild=1;
        }
      int   lev=parent->Level()+1;
      for (Link *l2=parent->Next();l2;l2=l2->Next())
        {
        if (l2->Parent()!=parent && l2->Level()<=lev)
          break;
        insertAfter=l2;
        if (uiInsertAfter && l2->Item()==uiInsertAfter)
          break;
        }
      }
    else
      {
      //for (LinkNonResolved *l =firstNonResolved;l && l->Next(); l= (LinkNonResolved *) l->Next());
      LinkNonResolved *ll =new LinkNonResolved(ID,firstNonResolved,pStr,parentID,lpItemData);
      if (ll && !firstNonResolved)
        firstNonResolved=ll;
      return 0;
      }
    }


  if (bSort && !uiInsertAfter && pStr)
    {
    for (Link *l = parent ? parent->Next():first ; l && !insertBefore; l=l->Next())
      {
      if ((l->Parent()==parent) && IsBefore(pStr,l->String()))
        {
        insertBefore=l;
        }
      }
    }

  if (uiInsertBefore)
    {
    Link *l =Find(uiInsertBefore);
    if (l && ((parent==l) || (parent==l->Parent())))
      {
      insertBefore=l;
      }
    }

  Link *l =new Link(ID,parent,insertAfter,insertBefore,pStr,lpItemData);
  if (l)
    {
    count++;
    current=l;
    if (!l->Prev())
      first=l;
    if (!l->Next())
      last=l;
    if (insertBefore && insertBefore==first)
      first=l;
    if (firstNonResolved)
      ResolveLink(ID);
    }
  return l;
}

void Linked_List::ResolveLink(unsigned long ID)
{
  for (LinkNonResolved * l = firstNonResolved ; l ; l = (LinkNonResolved *) l->Next())
    {
    if (l->lparentitemdata==ID && !l->bDeleted)
      {
      l->bDeleted=1;
      Add(l->Item(),ID,l->String(),l->GetItemData(),0,0);
      }
    }
}

void Linked_List::Delete(Link *pLink)
{
  // Emb 21/6/95
  int bVisible = pLink->IsVisible();

  lastparent=0;

  for (Link *l=pLink->Next(); l ; l=l->Next())
    if (l->Parent()==pLink)
      Delete(l);

  // Emb 21/6/95 : added test
  if (bVisible)
    count--;

  if (pLink->Parent())
    {
    pLink->Parent()->hasChild=0;
    for (Link *l=pLink->Parent()->Next(); l && !pLink->Parent()->hasChild ;l=l->Next())
      {
      if ( (l->Parent()==pLink->Parent()) && (l!=pLink) )
        pLink->Parent()->hasChild=1;
      }
    }

  if (current==pLink)
    current=pLink->Next() ? pLink->Next() : pLink->Prev();
  if (last==pLink)
    last=pLink->Next() ? pLink->Next() : pLink->Prev();
  if (first==pLink)
    first=pLink->Prev() ? pLink->Prev() : pLink->Next();

  // Emb 28/6/95 : Find() performance improvement
  if (lastfound==pLink)
    lastfound=pLink->Next() ? pLink->Next() : pLink->Prev();

  delete (pLink);
}

void Linked_List::Delete(unsigned long ID)
{
  // Emb 28/6/95 : performance improvement using Find()
  Link * l = Find(ID);
  if (l)
    Delete(l);

  // original code, masqued :
  //for (Link *l=first; l;l=l->Next())
  //  if (l->Item()==ID)
  //    {
  //    Delete(l);
  //    break;
  //    }
}

Link * Linked_List::Prev()
{
  for (Link *l=current->Prev(); l; l=l->Prev())
    if (l->IsVisible())
      {
      current=l;
      return l;
      }
  return 0;
}

Link * Linked_List::Next()
{
  #ifdef DEBUGEMB
  // Debug Emb 26/6/95 : performance measurement data
  lNextADebugEmbCount++;
  #endif //DEBUGEMB

  for (Link *l=current->Next(); l; l=l->Next())
    if (l->IsVisible())
      {
      current=l;
      return l;
      }
  return 0;
}

Linked_List::Linked_List()
{
  first=last=current=lastparent=0;
  firstNonResolved=0;
  bSort =0;
  count=0;

  // Debug Emb 26/6/95 : performance measurement data
  lFindDebugEmbCount = 0;   // nb of calls to Find()
  lNextDebugEmbCount = 0;   // nb of calls to Next() from Find()
  lFind2DebugEmbCount = 0;  // nb of calls to Find(2params)
  lNext2DebugEmbCount = 0;  // nb of calls to Next() from Find(2params)
  lNextADebugEmbCount = 0;  // total nb of calls to Next() from any place

  // Emb 27/6/95 : Find() performance improvement
  lastfound     = 0;        // found by the last Find() : initialized to NULL
  // Emb 23/8/95 : Find() performance improvement - bis
  beforelastfound = 0;        // found by the last Find() : initialized to NULL
}

Linked_List::~Linked_List()
{
  for (Link *l=first; l;l=l->Next())
    delete(l);
}

Link * Linked_List::Find(unsigned long ID)
{
  Link *lcur;
  Link *l;

  Link *tampoo;

  #ifdef DEBUGEMB
  lFindDebugEmbCount++;
  #endif

  // optimized by Emb 26 to 28/6/95 (sections optimize 1 and 2)
  // re-optimized by Emb 23/8/95    (other sections)

  // optimize 1: manage several calls on the same item
  if (lastfound)
    lcur = lastfound;
  else
    lcur = first;
  if (!lcur)
    return lcur;    // otherwise GPF on next line
  if (lcur->Item()==ID)
    return lcur;

  // optimize 1 bis 23/8/95 : manage "before the last" found
  if (beforelastfound)
    if (beforelastfound->Item()==ID) {
      // swap parties
      tampoo = beforelastfound;
      beforelastfound = lastfound;
      lastfound = tampoo;
      return lastfound;
    }

  // optimize 2: scan linked list from lastfound,
  // then only if not found, scan from lastfound until beginning
  if (lastfound)
    lcur = lastfound;
  else {
    lcur = first;
    if (lcur->Item()==ID) {
      beforelastfound = lastfound;
      lastfound = lcur;
      return lcur;
    }
  }

  // --- TRIAL : forward from beforelastfound
  // MASQUED - NOT SIGNIFICANT ENOUGH ( gain 11" on 1'31")
  // if (beforelastfound)
  //   lcur = beforelastfound;
  // else {
  //   lcur = first;
  //   if (lcur->Item()==ID) {
  //     beforelastfound = lastfound;
  //     lastfound = lcur;
  //     return lcur;
  //   }
  // }
  // --- TRIAL : forward from beforelastfound

  // forward, from lastfound
  l = lcur;
  #ifdef DEBUGEMB
  for (l=l->Next(); l; l=l->Next(), lNextDebugEmbCount++)
  #else
  for (l=l->Next(); l; l=l->Next())
  #endif
    if (l->Item()==ID) {
      beforelastfound = lastfound;
      lastfound = l;
      return l;
    }

  // backwards, from lastfound
  l = lcur;
  #ifdef DEBUGEMB
  for (l=l->Prev(); l; l=l->Prev(), lNextDebugEmbCount++)
  #else
  for (l=l->Prev(); l; l=l->Prev())
  #endif
    if (l->Item()==ID) {
      beforelastfound = lastfound;
      lastfound = l;
      return l;
    }
  return 0;

  // original code, masqued :
  //for (Link *l=first; l; l=l->Next())
  //  if (l->Item()==ID)
  //    return l;
  //return 0; 
}

Link * Linked_List::Find(unsigned long ID,int bSearchNonResolved)
{
  #ifdef DEBUGEMB
  lFind2DebugEmbCount++;
  #endif

  Link * link =  Find(ID);

  if (!link && bSearchNonResolved) 
    {
    #ifdef DEBUGEMB
    for (LinkNonResolved *l =firstNonResolved;
         l ;
         l= (LinkNonResolved *) l->Next(), lNext2DebugEmbCount++)
    #else
    for (LinkNonResolved *l =firstNonResolved;
         l ;
         l= (LinkNonResolved *) l->Next())
    #endif
      if (l->Item()==ID)
        return l;
    }
  return link;
}

int Linked_List::ToggleMode(unsigned long ID)
{
  return ToggleMode(Find(ID));
}

int Linked_List::ToggleMode(Link *l)
{ 
  int ret=0;
  if (l)
    {
    current=l;
    ret=l->ToggleMode();
    if (!current->IsVisible())
      current=Prev();
    count=0;
    for (l=first; l;l=l->Next())
      {
      if  (l->IsVisible())
        {
        last=l;
        count++;
        }
      }
    }
  return (ret);
}

// Debug Emb 26/6/95 : performance measurement data
void Linked_List::ResetDebugEmbCounts()
{
  lFindDebugEmbCount  = 0;  // nb of calls to Find()
  lNextDebugEmbCount  = 0;  // nb of calls to Next() from Find()
  lFind2DebugEmbCount = 0;  // nb of calls to Find(2params)
  lNext2DebugEmbCount = 0;  // nb of calls to Next() from Find(2params)
  lNextADebugEmbCount = 0;  // total nb of calls to Next() from any place
}

