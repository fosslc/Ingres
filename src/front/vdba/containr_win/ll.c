/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * LL.C
 *
 * Contains the implementation of the link list ADT.
 ****************************************************************************/

#include <windows.h>
#include "ll.h"

/****************************************************************************
 * LLFindHead
 *
 * Purpose:
 *  Finds the head of a linked list given any node.
 *
 * Parameters:
 *  lpNode          LPLINKNODE pointer to a list node
 *
 * Return Value:
 *  LPLINKNODE pointer to the head node.
 *
 ****************************************************************************/

LPLINKNODE LLFindHead(LPLINKNODE lpNode)
   {
   LPLINKNODE lpTemp = lpNode;

   if (!lpNode)
      return (LPLINKNODE) NULL;

   // Back up the list until we find the head
   while (lpTemp->lpPrev)
      lpTemp = PREV(lpTemp);

   return lpTemp;
   }

/****************************************************************************
 * LLFindTail
 *
 * Purpose:
 *  Finds the head of a linked list given any node.
 *
 * Parameters:
 *  lpNode          LPLINKNODE pointer to a list node
 *
 * Return Value:
 *  LPLINKNODE pointer to the tail node.
 *
 ****************************************************************************/

LPLINKNODE LLFindTail(LPLINKNODE lpNode)
   {
   LPLINKNODE lpTemp = lpNode;

   if (!lpNode)
      return (LPLINKNODE) NULL;

   // Traverse down the list until we find the tail
   while (lpTemp->lpNext)
      lpTemp = NEXT(lpTemp);

   return lpTemp;
   }

/****************************************************************************
 * LLAddHead
 *
 * Purpose:
 *  Adds a new list node at the head.
 *
 * Parameters:
 *  lpPos          LPLINKNODE pointer to a list node- usually the head.
 *  lpNew          LPLINKNODE pointer to new node to add.
 *
 * Return Value:
 *  LPLINKNODE pointer to the head node.
 *
 ****************************************************************************/

LPLINKNODE LLAddHead(LPLINKNODE lpPos, LPLINKNODE lpNew)
   {
   LPLINKNODE lpHead = LLFindHead(lpPos);

   if (!lpNew)
      return lpHead;

   lpNew->lpPrev = (LPLINKNODE) NULL;

   // Reconnect the nodes
   if (!lpHead)
      lpNew->lpNext = (LPLINKNODE) NULL;
   else
      {
      lpNew->lpNext = lpHead;
      lpHead->lpPrev = lpNew;
      }

   return lpNew;
   }

/****************************************************************************
 * LLAddTail
 *
 * Purpose:
 *  Adds a new list node at the tail.
 *
 * Parameters:
 *  lpPos          LPLINKNODE pointer to a list node- usually the tail.
 *  lpNew          LPLINKNODE pointer to new node to add.
 *
 * Return Value:
 *  LPLINKNODE pointer to the tail node.
 *
 ****************************************************************************/

LPLINKNODE LLAddTail(LPLINKNODE lpPos, LPLINKNODE lpNew)
   {
   LPLINKNODE lpTail = LLFindTail(lpPos);

   if (!lpNew)
      return lpTail;
   if (!lpTail)
      return LLAddHead(lpPos, lpNew);

   // Reconnect the nodes
   lpNew->lpNext = (LPLINKNODE) NULL;
   lpNew->lpPrev = lpTail;
   lpTail->lpNext = lpNew;
   
   return lpNew;
   }

/****************************************************************************
 * LLRemoveHead
 *
 * Purpose:
 *  Removes the item at the head of the list.
 *
 * Parameters:
 *  lpPos          LPLINKNODE pointer to a list node- usually the head.
 *  lpOldHead      LPLINKNODE FAR * to pointer to existing head.
 *
 * Return Value:
 *  LPLINKNODE pointer to the head node.
 *
 ****************************************************************************/

LPLINKNODE LLRemoveHead(LPLINKNODE lpPos, LPLINKNODE FAR *lpOldHead)
   {
   LPLINKNODE lpHead = LLFindHead(lpPos);

   *lpOldHead = lpHead;

   if (!lpHead)
      return lpHead;

   lpHead = lpHead->lpNext;
   if (!lpHead)
      return lpHead;
   lpHead->lpPrev = (LPLINKNODE) NULL;

   return lpHead;
   }

/****************************************************************************
 * LLRemoveTail
 *
 * Purpose:
 *  Removes the list node at the tail.
 *
 * Parameters:
 *  lpPos          LPLINKNODE pointer to a list node- usually the head.
 *  lpOldTail      LPLINKNODE FAR * to pointer to existing tail.
 *
 * Return Value:
 *  LPLINKNODE pointer to the new tail node.
 *
 ****************************************************************************/

LPLINKNODE LLRemoveTail(LPLINKNODE lpPos, LPLINKNODE FAR *lpOldTail)
   {
   LPLINKNODE lpTail = LLFindTail(lpPos);

   *lpOldTail = lpTail;

   if (!lpTail)
      return lpTail;

   lpTail = lpTail->lpPrev;
   if (!lpTail)
      return lpTail;
   lpTail->lpNext = (LPLINKNODE) NULL;

   return lpTail;
   }

/****************************************************************************
 * LLRemoveNode
 *
 * Purpose:
 *  Removes the current node from the list.
 *
 * Parameters:
 *  lpPos          LPLINKNODE pointer to a list node.
 *
 * Return Value:
 *  LPLINKNODE pointer to the removed node 
 *
 * Comment:
 *  Head and tail pointers can get hosed, users job to check for this!
 *
 ****************************************************************************/

LPLINKNODE LLRemoveNode(LPLINKNODE lpNode)
   {
   if (!lpNode)
      return lpNode;

   if (!lpNode->lpPrev)
      {
      LLRemoveHead(lpNode, &lpNode);    // Head pointer may be hosed
      return lpNode;
      }

   if (!lpNode->lpNext)
      {
      LLRemoveTail(lpNode, &lpNode);    // Tail pointer may be hosed
      return lpNode;
      }

   lpNode->lpPrev->lpNext = lpNode->lpNext;
   lpNode->lpNext->lpPrev = lpNode->lpPrev;

   return lpNode;
   }

/****************************************************************************
 * LLInsertBefore
 *
 * Purpose:
 *  Inserts a new node before the given node in the list.
 *
 * Parameters:
 *  lpPos          LPLINKNODE pointer to a list node to insert before.
 *  lpNew          LPLINKNODE pointer to node to insert.
 *
 * Return Value:
 *  LPLINKNODE pointer to the list head
 *
 ****************************************************************************/

LPLINKNODE LLInsertBefore(LPLINKNODE lpPos, LPLINKNODE lpNew)
   {
   LPLINKNODE lpHead = LLFindHead(lpPos);

   if (!lpNew)
      return lpHead;
   if (!lpHead || lpPos->lpPrev == (LPLINKNODE) NULL)
      return LLAddHead(lpPos, lpNew);
   
   lpNew->lpNext = lpPos;
   lpNew->lpPrev = lpPos->lpPrev;
   lpPos->lpPrev->lpNext = lpNew;
   lpPos->lpPrev = lpNew;

   return lpHead;
   }

/****************************************************************************
 * LLInsertAfter
 *
 * Purpose:
 *  Inserts a new node after the given node in the list.
 *
 * Parameters:
 *  lpPos          LPLINKNODE pointer to a list node to insert after.
 *  lpNew          LPLINKNODE pointer to node to insert.
 *
 * Return Value:
 *  LPLINKNODE pointer to the list head
 *
 * CAUTION!  This routine may inadvertently hose the tail pointer.
 * You must check it upon return (Check for tail->next != NULL)
 *
 ****************************************************************************/

LPLINKNODE LLInsertAfter(LPLINKNODE lpPos, LPLINKNODE lpNew)
   {
   LPLINKNODE lpHead = LLFindHead(lpPos);

   if (!lpNew)
      return lpHead;
   if (!lpHead)
      return LLAddHead(lpPos, lpNew);
   
   if (!lpPos->lpNext)
     {
     // We are adding to the tail but we still return the head.
     LLAddTail(lpPos, lpNew);
     }
   else
     {
     lpNew->lpNext = lpPos->lpNext;
     lpNew->lpPrev = lpPos;
     lpPos->lpNext->lpPrev = lpNew;
     lpPos->lpNext = lpNew;
     }

   return lpHead;
   }

/****************************************************************************
 * LLScrollList
 *
 * Purpose:
 *  Inserts scrolls the link list in either direction.
 *
 * Parameters:
 *  lpPos          LPLINKNODE pointer to a list node to insert before.
 *  lDelta         LONG amount to scroll- can be nagative.
 *
 * Return Value:
 *  LPLINKNODE pointer to the list node scrolled to
 *
 ****************************************************************************/

LPLINKNODE LLScrollList(LPLINKNODE lpPos, LONG lDelta)
   {
   LPLINKNODE lpTemp = lpPos;
   
   if (!lpPos)
      return lpPos;

   if (lDelta >= 0)
      {
      while (lDelta && lpTemp->lpNext)
         {
         lpTemp = lpTemp->lpNext;
         --lDelta;
         }
      }
   else
      {
      while (lDelta && lpTemp->lpPrev)
         {
         lpTemp = lpTemp->lpPrev;
         ++lDelta;
         }
      }

   return lpTemp;
   }
